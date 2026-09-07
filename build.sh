#!/bin/bash
#
# build.sh - 编译脚本
#
# 功能：在项目根目录的 build_cmake 文件夹下完成所有编译。
# 使用方法：
#   ./build.sh              # Debug 模式编译
#   ./build.sh Release      # Release 模式编译
#   ./build.sh clean        # 清理 build_cmake 目录
#   ./build.sh rebuild      # 清理后重新编译（Debug）
#   ./build.sh -j8          # Debug 模式，8 线程并行编译
#   ./build.sh Release -j8  # Release 模式，8 线程并行编译
#
# 注意：本机 LD_LIBRARY_PATH 指向 VSCode 扩展旧 libstdc++，
#       会导致 cmake/make 库冲突，脚本自动清除后执行。

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build_cmake"
BUILD_TYPE="Debug"
JOBS=$(nproc)  # 默认使用所有 CPU 核心

# 解析参数
PARSE_JOBS=""
for arg in "$@"; do
    case "$arg" in
        Debug|Release|RelWithDebInfo|MinSizeRel)
            BUILD_TYPE="$arg"
            ;;
        clean)
            echo "清理 build_cmake 目录..."
            rm -rf "${BUILD_DIR}"
            echo "完成。"
            exit 0
            ;;
        rebuild)
            echo "清理 build_cmake 目录..."
            rm -rf "${BUILD_DIR}"
            ;;
        -j*)
            JOBS="${arg#-j}"
            ;;
        *)
            echo "未知参数: $arg"
            echo "用法: $0 [Debug|Release] [-jN] [clean|rebuild]"
            exit 1
            ;;
    esac
done

echo "=========================================="
echo "  API 项目编译"
echo "  构建类型: ${BUILD_TYPE}"
echo "  构建目录: ${BUILD_DIR}"
echo "  并行线程: ${JOBS}"
echo "=========================================="

# 创建构建目录
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# 运行 cmake（清除 LD_LIBRARY_PATH 避免 VSCode 扩展的旧 libstdc++ 冲突）
env -u LD_LIBRARY_PATH cmake "${PROJECT_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 编译
env -u LD_LIBRARY_PATH make -j"${JOBS}"

echo ""
echo "=========================================="
echo "  编译完成！"
echo "  产物: ${BUILD_DIR}/lib/"
echo "=========================================="