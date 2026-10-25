#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

DYNAMIC_EVALUATOR="DYNAMIC"
STATIC_EVALUATOR="STATIC"
FARROW_EVALUATOR="FARROW"
DEFAULT_EVALUATORS="$DYNAMIC_EVALUATOR,$STATIC_EVALUATOR"

trim() {
  local value="$1"
  value="${value#"${value%%[![:space:]]*}"}"
  value="${value%"${value##*[![:space:]]}"}"
  printf '%s' "$value"
}

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
  local run_status=${PIPESTATUS[0]}
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
    eval=$(trim "$eval")
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

run_driver() {
  local driver="$1"
  local programs="$2"
  local evaluators="$3"
  local -a program_list
  local -a matches
  local program input
  local pass=true
  IFS=',' read -ra program_list <<< "$programs"
  for program in "${program_list[@]}"; do
    program=$(trim "$program")
    if [[ "$program" == *"*"* || "$program" == *"?"* || "$program" == *"["* ]]; then
      mapfile -t matches < <(compgen -G "$program" | LC_ALL=C sort)
    else
      matches=("$program")
    fi

    local found_file=false
    for input in "${matches[@]}"; do
      [ -f "$input" ] || continue
      found_file=true
      run_driver_once "$driver" "$input" "$evaluators" || pass=false
    done
    if ! $found_file; then
      echo "No program files found for: $program" >&2
      pass=false
    fi
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

TESTS_FILE="$SCRIPT_DIR/compare-evaluators-scala.txt"
if [ ! -r "$TESTS_FILE" ]; then
  echo "Cannot read test table: $TESTS_FILE" >&2
  exit 2
fi

failures=0
total=0
line_number=0

while IFS= read -r line || [ -n "$line" ]; do
  line_number=$((line_number + 1))
  line=$(trim "${line%%#*}")
  [ -z "$line" ] && continue

  if [[ "$line" != *"|"*"|"* || "$line" == *"|"*"|"*"|"* ]]; then
    echo "$TESTS_FILE:$line_number: expected driver | programs | evaluators" >&2
    exit 2
  fi
  IFS="|" read -r driver args evals <<< "$line"
  driver=$(trim "$driver")
  args=$(trim "$args")
  evals=$(trim "$evals")
  if [ -z "$driver" ]; then
    echo "$TESTS_FILE:$line_number: driver is missing" >&2
    exit 2
  fi
  if [ -z "$evals" ]; then
    echo "$TESTS_FILE:$line_number: evaluators are missing" >&2
    exit 2
  fi
  total=$((total + 1))
  return_code=0
  run_driver "$driver" "$args" "$evals" || return_code=$?
  if [ $return_code -ne 0 ]; then
    failures=$((failures + 1))
  fi
  echo ""
done < "$TESTS_FILE"

if [ "$total" -eq 0 ]; then
  echo "No tests found in $TESTS_FILE" >&2
  exit 2
fi

echo "=============================="
echo "$((total - failures))/$total passed"
if [ $failures -gt 0 ]; then
  echo "$failures FAILED"
  exit 1
fi
echo "All passed!"
