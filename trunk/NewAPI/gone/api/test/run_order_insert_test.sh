#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
library_dir="${1:-$repo_root/api/lib}"
result_dir="$repo_root/claude-task/2026-11-04-order-insert/test-build"
mkdir -p "$result_dir"
g++ -std=c++17 -O0 -g -pthread -I"$repo_root/api/include" -I"$repo_root/api/src" \
  -I"$repo_root/api/counter98" -I"$repo_root/../common/include" -I"$repo_root/include" \
  "$repo_root/api/tests/order_insert_test.cpp" -L"$library_dir" -Wl,-rpath,"$library_dir" \
  -lg1api -o "$result_dir/order_insert_test"
"$result_dir/order_insert_test"
