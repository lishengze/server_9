#!/bin/bash
#
# build.sh - 编译脚本（默认在 docker 容器 otc 中编译）
#
# 功能：在 docker 容器 otc 的项目目录 /mnt/work/api_trunk 下的 build_cmake 文件夹完成编译。
# 使用方法：
#   ./build.sh              # Debug 模式编译
#   ./build.sh Release      # Release 模式编译
#   ./build.sh clean        # 清理 build_cmake 目录
#   ./build.sh rebuild      # 清理后重新编译（Debug）
#   ./build.sh -j8          # Debug 模式，8 线程并行编译
#   ./build.sh Release -j8  # Release 模式，8 线程并行编译
#
# 说明：
#   - 宿主机（/home/lsz/code）执行时，自动转发到 docker 容器 otc（挂载 /mnt）编译。
#   - 脚本在容器内（/mnt/work/api_trunk）运行时，直接执行实际编译。
#   - 容器内编译时清除 LD_LIBRARY_PATH，避免 VSCode 扩展旧 libstdc++ 造成库冲突。

set -e

# docker 容器名与容器内项目路径（宿主机 /home/lsz/code 挂载到 /mnt）
DOCKER_CONTAINER="otc"
DOCKER_PROJECT_DIR="/mnt/work/api_trunk"

# 当前脚本所在目录（宿主机或容器内）
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"

# ================= 宿主机：转发到 docker 容器内编译 =================
if [[ "${PROJECT_DIR}" != /mnt/* ]]; then
    echo "检测到宿主机项目目录（${PROJECT_DIR}），转发到 docker 容器 ${DOCKER_CONTAINER} 编译..."
    docker exec "${DOCKER_CONTAINER}" bash -lc "cd ${DOCKER_PROJECT_DIR} && ./build.sh $*"
    exit $?
fi

# ================= 已在 docker 容器内：执行实际编译 =================
BUILD_DIR="${PROJECT_DIR}/build_cmake"
BUILD_TYPE="Debug"
JOBS=$(nproc)  # 默认使用所有 CPU 核心

# 解析参数
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
echo "  API 项目编译 (docker 容器: ${DOCKER_CONTAINER})"
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