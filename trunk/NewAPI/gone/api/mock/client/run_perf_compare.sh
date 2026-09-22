#!/bin/bash
# run_perf_compare.sh - FTE (gw counter) 与 GOne (fpga_direct) 顺序性能测试对比
#
# 测试要求：10000 TPS，持续 10 秒，两个柜台在相同场景与测试要求下对比。
#
# 流程：
#   1. 先运行 FTE（gw counter）功能测试 + 性能测试
#   2. 再运行 GOne（fpga_direct）功能测试 + 性能测试
#   3. 输出两份报告到 perf_report_fte.txt / perf_report_gone.txt
#
# 前置条件：
#   - FTE 柜台（ute）运行于 33001，counter98_mock 运行于 9002
#   - gone_counter_mock 运行于 44001/44002，counter98_mock 运行于 9003
#   - liblbapi.so 已构建于 build_cmake/lib

set -e

BASE_DIR=/mnt/work/api_trunk
CLIENT_DIR=$BASE_DIR/trunk/NewAPI/gone/api/mock/client
MOCK_CLIENT=$BASE_DIR/build_cmake/bin/mock_client
LIB=$BASE_DIR/build_cmake/lib/liblbapi.so

cd "$CLIENT_DIR"

echo "================================================================"
echo "  顺序性能测试：FTE (gw counter) vs GOne (fpga_direct)"
echo "  测试要求：10000 TPS，持续 10 秒"
echo "================================================================"

# ============ 1/2 FTE (gw counter) 性能测试 ============
echo ""
echo "########## [1/2] FTE (gw counter) 性能测试 ##########"
LD_LIBRARY_PATH=$BASE_DIR/build_cmake/lib:$LD_LIBRARY_PATH \
  $MOCK_CLIENT \
  --config config/connection_config.json \
  --testcase config/test_cases/fte_combo.json \
  --report result/test_report_fte.txt

# ============ 2/2 GOne (fpga_direct) 性能测试 ============
echo ""
echo "########## [2/2] GOne (fpga_direct) 性能测试 ##########"
LD_LIBRARY_PATH=$BASE_DIR/build_cmake/lib:$LD_LIBRARY_PATH \
  $MOCK_CLIENT \
  --config config/connection_config_gone.json \
  --testcase config/test_cases/gone_combo.json \
  --report result/test_report_gone.txt

echo ""
echo "================================================================"
echo "  性能测试完成"
echo "  FTE 报告: $CLIENT_DIR/result/perf_report_fte.txt"
echo "  GOne 报告: $CLIENT_DIR/result/perf_report_gone.txt"
echo "================================================================"