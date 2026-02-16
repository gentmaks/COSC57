#!/usr/bin/env bash
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

OPTIMIZER_PATH="${1:-${SCRIPT_DIR}/optimizer}"
TEST_DIR="${2:-${REPO_ROOT}/optimizer_test_results}"
OUT_DIR="${3:-${SCRIPT_DIR}/diff_outputs}"

if [[ ! -x "${OPTIMIZER_PATH}" ]]; then
  echo "Optimizer not found or not executable: ${OPTIMIZER_PATH}"
  echo "Build it first with: make -C \"${SCRIPT_DIR}\""
  exit 1
fi

if [[ ! -d "${TEST_DIR}" ]]; then
  echo "Test directory not found: ${TEST_DIR}"
  exit 1
fi

mkdir -p "${OUT_DIR}"

total=0
matched=0
different=0
skipped=0

echo "Optimizer : ${OPTIMIZER_PATH}"
echo "Test dir  : ${TEST_DIR}"
echo "Output dir: ${OUT_DIR}"
echo

extract_function_regions() {
  local input_file="$1"
  local output_file="$2"

  awk '
    BEGIN { in_func = 0 }
    /^[[:space:]]*define[[:space:]]/ {
      in_func = 1
      print
      next
    }
    {
      if (in_func) {
        print
        if ($0 ~ /^[[:space:]]*}[[:space:]]*$/) {
          in_func = 0
          print ""
        }
      }
    }
  ' "${input_file}" > "${output_file}"
}

for input_ll in "${TEST_DIR}"/*.ll; do
  [[ -e "${input_ll}" ]] || continue

  filename="$(basename "${input_ll}")"
  if [[ "${filename}" == *_opt.ll ]]; then
    continue
  fi

  base="${filename%.ll}"
  expected="${TEST_DIR}/${base}_opt.ll"

  if [[ ! -f "${expected}" ]]; then
    echo "SKIP ${base}: missing expected file ${base}_opt.ll"
    skipped=$((skipped + 1))
    continue
  fi

  generated="${OUT_DIR}/${base}_generated_opt.ll"
  expected_regions="${OUT_DIR}/${base}_expected_regions.ll"
  generated_regions="${OUT_DIR}/${base}_generated_regions.ll"
  diff_file="${OUT_DIR}/${base}.diff"
  total=$((total + 1))

  if ! "${OPTIMIZER_PATH}" "${input_ll}" "${generated}"; then
    echo "FAIL ${base}: optimizer run failed"
    different=$((different + 1))
    continue
  fi

  extract_function_regions "${expected}" "${expected_regions}"
  extract_function_regions "${generated}" "${generated_regions}"

  if diff -u "${expected_regions}" "${generated_regions}" > "${diff_file}"; then
    echo "MATCH ${base}"
    matched=$((matched + 1))
    rm -f "${diff_file}"
  else
    echo "DIFF  ${base} (see ${diff_file})"
    different=$((different + 1))
  fi
done

echo
echo "Summary:"
echo "  Compared : ${total}"
echo "  Matched  : ${matched}"
echo "  Different: ${different}"
echo "  Skipped  : ${skipped}"

if [[ "${different}" -gt 0 ]]; then
  exit 2
fi

exit 0
