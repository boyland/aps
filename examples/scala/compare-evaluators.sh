#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

DYNAMIC_EVALUATOR="DYNAMIC"
STATIC_EVALUATOR="STATIC"
FARROW_EVALUATOR="FARROW"
DEFAULT_EVALUATORS="$DYNAMIC_EVALUATOR,$STATIC_EVALUATOR"

extract_results() {
  sed -n '/^Results:$/,$p'
}

xml_escape() {
  LC_ALL=C tr -d '\000-\010\013\014\016-\037' |
    sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g; s/"/\&quot;/g; s/'"'"'/\&apos;/g'
}

record_test() {
  local driver="$1" name="$2" duration="$3" status="$4"
  report_tests=$((report_tests + 1))
  if ! {
    printf '    <testcase classname="%s" name="%s" time="%s">\n' \
      "$(printf '%s' "$driver" | xml_escape)" \
      "$(printf '%s' "$name" | xml_escape)" "$duration" &&
    if [ "$status" -ne 0 ]; then
      report_failures=$((report_failures + 1))
      printf '      <failure message="Evaluator comparison failed">\n' &&
      xml_escape &&
      printf '\n      </failure>\n'
    fi &&
    printf '    </testcase>\n'
  } >> "$report_cases"; then
    echo "Cannot record test result for: $driver $name" >&2
    return 2
  fi
}

finish_report() {
  local status=$?
  trap - EXIT
  if [ "$status" -ne 0 ] && [ "$report_failures" -eq 0 ]; then
    record_test "Test configuration" "Comparison runner" 0 "$status" \
      <<< "Comparison runner exited with status $status. See the job log for details." || status=2
  fi
  if ! {
    printf '<?xml version="1.0" encoding="UTF-8"?>\n<testsuites>\n' &&
    printf '  <testsuite name="Scala evaluator comparisons" tests="%s" failures="%s" errors="0" skipped="0" time="%s">\n' \
      "$report_tests" "$report_failures" "$((SECONDS - report_start))" &&
    cat "$report_cases" &&
    printf '  </testsuite>\n</testsuites>\n'
  } > "$JUNIT_REPORT"; then
    echo "Cannot write test report: $JUNIT_REPORT" >&2
    status=2
  fi
  rm -f "$report_cases" || status=2
  exit "$status"
}

if [ -n "${JUNIT_REPORT:-}" ]; then
  if ! mkdir -p "$(dirname "$JUNIT_REPORT")" || ! : > "$JUNIT_REPORT"; then
    echo "Cannot write test report: $JUNIT_REPORT" >&2
    exit 2
  fi
  report_cases=$(mktemp) || exit 2
  report_tests=0
  report_failures=0
  report_start=$SECONDS
  trap finish_report EXIT
fi

run_with_evaluator() {
  local evaluator="$1"
  local driver="$2"
  local args="$3"
  local outfile="$4"

  if ! make EVALUATOR="$evaluator" "$driver.class" > "$outfile.log" 2>&1; then
    cat "$outfile.log"
    return 1
  fi
  local start end
  start=$(date +%s.%N)
  make EVALUATOR="$evaluator" ARGS="$args" "$driver.run" > "$outfile.log" 2>&1
  local run_status=$?
  extract_results < "$outfile.log" > "$outfile"
  if [ "$run_status" -ne 0 ]; then
    cat "$outfile.log"
  fi
  end=$(date +%s.%N)
  printf "  %s finished in %.2fs\n" "$evaluator" "$(echo "$end - $start" | bc)"
  return "$run_status"
}

run_driver_once() {
  local driver="$1"
  local args="$2"
  local -a evals
  IFS=',' read -ra evals <<< "$3"
  local pass=true
  local build_failed=false
  local tmpdir
  tmpdir=$(mktemp -d)

  echo "--- $driver ${args:+(${args})} ---"

  local built_evaluators=()
  for eval in "${evals[@]}"; do
    echo "  running $eval ..."
    if ! run_with_evaluator "$eval" "$driver" "$args" "$tmpdir/$eval"; then
      echo "  FAIL: $eval failed"
      build_failed=true
      continue
    fi
    if [ ! -s "$tmpdir/$eval" ]; then
      echo "  WARNING: $eval produced no Results: output"
      pass=false
      continue
    fi
    built_evaluators+=("$eval")
  done

  if $build_failed; then
    rm -rf "$tmpdir"
    return 1
  fi

  local n=${#built_evaluators[@]}
  if [ $n -lt 2 ]; then
    if [ $n -eq 1 ] && [ ${#evals[@]} -eq 1 ] && $pass; then
      echo "  OK: ${built_evaluators[0]} ran successfully"
      rm -rf "$tmpdir"
      return 0
    elif [ $n -eq 1 ]; then
      echo "  FAIL: only ${built_evaluators[0]} ran successfully"
    else
      echo "  FAIL: no evaluators ran successfully"
    fi
    rm -rf "$tmpdir"
    return 1
  fi

  for ((i=0; i<n-1; i++)); do
    for ((j=i+1; j<n; j++)); do
      local e1="${built_evaluators[$i]}"
      local e2="${built_evaluators[$j]}"
      if diff -q "$tmpdir/$e1" "$tmpdir/$e2" > /dev/null 2>&1; then
        echo "  PASS ($e1 == $e2)"
      else
        echo "  FAIL ($e1 != $e2)"
        diff "$tmpdir/$e1" "$tmpdir/$e2" | sed 's/^/    /'
        pass=false
      fi
    done
  done

  rm -rf "$tmpdir"
  $pass
}

run_reported_driver() {
  if [ -z "${JUNIT_REPORT:-}" ]; then
    run_driver_once "$@"
    return $?
  fi

  local log start end status
  log=$(mktemp) || return 2
  start=$(date +%s.%N)
  run_driver_once "$@" 2>&1 | tee "$log"
  local -a run_statuses=("${PIPESTATUS[@]}")
  status=${run_statuses[0]}
  if [ "${run_statuses[1]}" -ne 0 ]; then
    echo "Cannot capture test output for: $1 $2" >&2
    status=2
  fi
  end=$(date +%s.%N)
  record_test "$1" "$1: $2 ($3)" "$(echo "$end - $start" | bc)" "$status" \
    < "$log" || status=2
  rm -f "$log" || status=2
  return "$status"
}

run_driver() {
  local driver="$1"
  local programs="$2"
  local evaluators="$3"
  local -a program_list
  local pass=true
  IFS=',' read -ra program_list <<< "$programs"
  for program in "${program_list[@]}"; do
    run_reported_driver "$driver" "$program" "$evaluators" || pass=false
  done
  $pass
}

if [ $# -gt 0 ]; then
  driver="$1"
  args="${2:-}"
  evals="${3:-$DEFAULT_EVALUATORS}"
  run_driver "$driver" "$args" "$evals"
  exit $?
fi

TESTS=(
  "BroadFiberCycleDriver|tiny.program|$DEFAULT_EVALUATORS"
  "BelowFiberCycleDriver|tiny.program|$DEFAULT_EVALUATORS"
  "BelowSingleFiberCycleDriver|tiny.program|$DEFAULT_EVALUATORS"
  "LocalFiberCycleDriver|tiny.program|$DEFAULT_EVALUATORS"
  "TestCollDriver|tiny.program|$DEFAULT_EVALUATORS"
  "TestUseCollDriver|tiny.program|$DEFAULT_EVALUATORS"
  "TestCycleDriver|tiny.program|$DEFAULT_EVALUATORS"
  "UseGlobal|tiny.program|$DEFAULT_EVALUATORS"
  "FarrowUbdDriver|farrow-ubd.program|$DEFAULT_EVALUATORS,$FARROW_EVALUATOR"
  "FarrowUbdFiberDriver|farrow-ubd.program|$DEFAULT_EVALUATORS"
  "NestedUbdDriver|nested-ubd.program|$DEFAULT_EVALUATORS,$FARROW_EVALUATOR"
  "NestedUbdFiberDriver|nested-ubd.program|$DEFAULT_EVALUATORS"
  "TestFieldsDriver|tiny.program|$DEFAULT_EVALUATORS"
  "FirstDriver|grammar.cfg|$DEFAULT_EVALUATORS"
  "FollowDriver|grammar.cfg|$DEFAULT_EVALUATORS"
  "NullableDriver|grammar.cfg|$DEFAULT_EVALUATORS"
  "SimpleBindingDriver|simple.program|$DEFAULT_EVALUATORS"
  "BiggerBindingDriver|bigger.program|$DEFAULT_EVALUATORS"
  "SimpleBinding1Driver|simple.program|$DEFAULT_EVALUATORS"
  "SimpleBinding2Driver|simple.program|$DEFAULT_EVALUATORS"
  "SimpleBinding3Driver|simple.program|$DEFAULT_EVALUATORS"
  "TestForDriver|tiny.program|$DEFAULT_EVALUATORS"
  "TestForPatternsDriver|tiny.program,tiny-singleton.program,tiny-empty.program|$DEFAULT_EVALUATORS"
  "TestCasePatternsDriver|tiny.program,tiny-singleton.program,tiny-empty.program|$DEFAULT_EVALUATORS"
  "TestNestedPatternsDriver|tiny.program,tiny-singleton.program,tiny-empty.program|$DEFAULT_EVALUATORS"
  "SimpleSncDriver|simple.program|$DYNAMIC_EVALUATOR,$FARROW_EVALUATOR"
  "SimpleSncFiberDriver|simple.program|$DYNAMIC_EVALUATOR"
)

failures=0
total=0

for test in "${TESTS[@]}"; do
  IFS="|" read -r driver args evals <<< "$test"
  evals="${evals:-$DEFAULT_EVALUATORS}"
  total=$((total + 1))
  return_code=0
  run_driver "$driver" "$args" "$evals" || return_code=$?
  if [ $return_code -ne 0 ]; then
    failures=$((failures + 1))
  fi
  echo ""
done

echo "=============================="
echo "$((total - failures))/$total passed"
if [ $failures -gt 0 ]; then
  echo "$failures FAILED"
  exit 1
fi
echo "All passed!"
