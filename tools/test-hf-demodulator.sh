#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"
build_dir="$(mktemp -d)"
trap 'rm -rf "$build_dir"' EXIT
common=(-std=c++17 -Wall -Wextra -Werror -pedantic -Iapps/orcsdr-tab5/ui)
sources=(tests/hf_demodulator_tests.cpp apps/orcsdr-tab5/ui/hf_demodulator.cpp apps/orcsdr-tab5/ui/shortwave_model.cpp)
"${CXX:-c++}" "${common[@]}" -O2 "${sources[@]}" -o "$build_dir/hf-tests"
"$build_dir/hf-tests"
"${CXX:-c++}" "${common[@]}" -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer "${sources[@]}" -o "$build_dir/hf-tests-sanitized"
ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$build_dir/hf-tests-sanitized"
ui_sources=(tests/hf_dashboard_tests.cpp apps/orcsdr-tab5/ui/shortwave_dashboard.cpp apps/orcsdr-tab5/ui/shortwave_model.cpp apps/orcsdr-tab5/ui/receiver_tuning_controls.cpp)
"${CXX:-c++}" "${common[@]}" -Itests/support/hf -O2 "${ui_sources[@]}" -o "$build_dir/hf-ui"
"$build_dir/hf-ui"
"${CXX:-c++}" "${common[@]}" -Itests/support/hf -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer "${ui_sources[@]}" -o "$build_dir/hf-ui-sanitized"
ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$build_dir/hf-ui-sanitized"
