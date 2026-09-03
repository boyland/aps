#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

DEFAULT_EVALUATORS="DYNAMIC,STATIC"

extract_results() {
  sed -n '/^Results:$/,$p'
}

run_with_evaluator() {
  local evaluator="$1"
  local driver="$2"
  local args="$3"
  local outfile="$4"

  if ! make EVALUATOR="$evaluator" "$driver.class" > /dev/null 2>&1; then
    return 1
  fi
  local start end
  start=$(date +%s.%N)
  make EVALUATOR="$evaluator" ARGS="$args" "$driver.run" 2>&1 | extract_results > "$outfile"
  end=$(date +%s.%N)
  printf "  %s finished in %.2fs\n" "$evaluator" "$(echo "$end - $start" | bc)"
}

run_driver() {
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
      echo "  FAIL: $eval build failed"
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
    if [ $n -eq 1 ]; then
      echo "  OK: ${built_evaluators[0]} ran successfully"
    else
      echo "  FAIL: no evaluators ran successfully"
    fi
    rm -rf "$tmpdir"
    return $(( n == 0 ))
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
  "FarrowUbdDriver|farrow-ubd.program|$DEFAULT_EVALUATORS"
  "FarrowUbdFiberDriver|farrow-ubd.program|$DEFAULT_EVALUATORS"
  "NestedUbdDriver|nested-ubd.program|$DEFAULT_EVALUATORS"
  "NestedUbdFiberDriver|nested-ubd.program|$DEFAULT_EVALUATORS"
  "TestFieldsDriver|tiny.program|$DEFAULT_EVALUATORS"
  "FirstDriver|grammar.cfg|$DEFAULT_EVALUATORS"
  "FollowDriver|grammar.cfg|$DEFAULT_EVALUATORS"
  "NullableDriver|grammar.cfg|$DEFAULT_EVALUATORS"
  "SimpleSncDriver|simple.program|DYNAMIC"
  "SimpleBindingDriver|simple.program|$DEFAULT_EVALUATORS"
  "SimpleBinding1Driver|simple.program|$DEFAULT_EVALUATORS"
  "SimpleBinding2Driver|simple.program|$DEFAULT_EVALUATORS"
  "SimpleBinding3Driver|simple.program|$DEFAULT_EVALUATORS"
  "TestForDriver|tiny.program|$DEFAULT_EVALUATORS"
)

failures=0
total=0

for test in "${TESTS[@]}"; do
  IFS="|" read -r driver args evals <<< "$test"
  evals="${evals:-$DEFAULT_EVALUATORS}"
  total=$((total + 1))
  return_code=0
  run_driver "$driver" "$args" "$evals" || return_code=$?
  if [ $return_code -eq 1 ]; then
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
