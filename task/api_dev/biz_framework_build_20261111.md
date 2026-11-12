# biz_framework 与 abf 库编译研究（2026-11-11）

> 目标：为 DSE 提供 `libabf_te.so` 等依赖库。
> 环境：docker `otc`（CentOS7 / gcc 4.8.5 / glibc 2.17）。所有编译在此容器内完成。

---

## 1. 工程结构（`gt_trunk/DYS-FRAMEWORK/biz_framework`）

| 子目录 | `project()` 名 | 产物 | 说明 |
|--------|----------------|------|------|
| `te/` | `abf_te` | `libabf_te.so` | **核心**：交易引擎业务框架（账户/委托/回报/风控等） |
| `crc/` | `abf_crc` | `libabf_crc.so` | 校验/CRC |
| `detail/` | `abf_detail` | `libabf_detail.so` | 明细，`target_link_libraries(... abf_te)` |
| `grisk/` | `abf_grisk` | `libabf_grisk.so` | 风控（OpenMP） |
| `md/` | `abf_md` | `libabf_md.so` | 行情，依赖 `abf_te` |
| `query/` | `abf_query` | `libabf_query.so` | 查询，依赖 `abf_te` |

- 构建入口：`build.sh`（`-r` Release / `-d` Debug / 默认 Fast / `-i` 增量），内部 `cmake ../.. && make install`。
- 关键环境变量（build.sh 内置）：`BOOST_ROOT`、`AMI_RES_ROOT`、`TRD_RES_ROOT`、`WORK_ATP_ROOT`、`CMAKE_ATP_RES_ROOT=$WORK_ATP_ROOT/test_time_<user>`。
- 头文件来源：`AMI_RES_ROOT/include`（abf 报文头）+ 仓库内 `te/include/messages/*.h`（封装访问器）。

## 2. 编译结果与阻塞

| 尝试 | 结果 |
|------|------|
| `./build.sh -r`（全量 6 库） | ❌ `grisk/src/grisk_process_pworker.cpp` 编译失败 |
| `make abf_te abf_crc abf_detail abf_md abf_query` | ❌ 大量“abf 头版本不匹配”错误 |
| override 头目录 + 1 处本地适配后 | ◑ 部分错误消除，仍余 5+ 处 |

**版本不匹配清单（编译期实测）**：

| 缺失项 | 出处 | 含义 |
|--------|------|------|
| `abf::util::PktExternalQuery::not_need_check_pwd` | `te/include/messages/external_query.h` | 外部查询"无需验密"新字段 |
| `abf::util::PktIPOAndXjbFundFrzReport` / `kPktIPOAndXjbFundFrzReport` | `te/include/messages/ipo_and_xjb_fund_frz_report.h` | 新股/现金宝资金冻结回报（新报文） |
| `abf::util::InstructionType::kFundFrzUnfrzOtc` | `te/include/messages/fund_frz_unfrz_otc_ins.h` | 场外资金冻结/解冻指令类型（新） |
| `abf::util::PktBlockTradePreorder::counter_party_account_id` | `te/...` | 大宗预埋单新增对手方账户字段 |
| `abf::util::PktExtCTEToUTEStandardIns::index_exponent` | `te/...` | CTE→UTE 标准指令新增指数乘数 |
| `abf::util::PktGRiskInternalReportInstruction::array_company_stk` | `te/...` | GRisk 内部报表新增公司股票数组 |
| `abf::util::kPktCreditT0TradeReport` / `PktCreditT0TradeReport` / `PktGriskCompanyStkUnit` | `te/...` | 信用 T0 成交回报、GRisk 公司股票单元（新类型） |

> **判定**：`biz_framework` 源码依赖的 **ABF/AMI 版本高于本机 `work_lib/AMI`**（全盘头文件均无上述字段/类型，`grep -rl not_need_check_pwd --include=*.h /mnt/work` 只命中 biz_framework 自身 4 个仓的同名文件）。

## 3. 已做的本地适配（可回滚）

| 文件 | 改动 | 备份 |
|------|------|------|
| `te/include/messages/external_query.h` | `NotNeedCheckPwd()/SetNotNeedCheckPwd()` 改为**进程内缓存字段**（不写报文），并加注释「[本地适配 2026-11-11][待上游 abf 资源包升级后还原]」 | `external_query.h.bak_abfver` |
| `/mnt/work/gt_test/work_lib/AMI_ov`（新增目录） | override 头目录：软链 **ATP `common/include/util/include` 优先** + AMI 旧头补齐；`lib64` 软链 AMI/lib64 | — |

> 适配只解决了 `PktIPOAndXjbFundFrzReport` 一类问题（ATP 头里有），但 `not_need_check_pwd`、大宗/CTE/GRisk/信用T0 等**在整机均无定义**，无法靠路径解决。

> **2026-11-12 跟踪：`external_query.h` 的适配已恢复为原始实现**（`GetPacket()->not_need_check_pwd`）。
> 依据：`Common/util/include/messages_atp_te.h` 已于 2026-11-12 同步为 ATP 新版，
> `PktExternalQuery` 含 `Bool_def not_need_check_pwd`（该文件第 2855 行）；用工程真实编译参数
> （`build_root/compile_commands.json`）对 `cust_pwd_checker.cpp`、`order_query.cpp`、
> `engine_business_processor.cpp` 做 `-fsyntax-only` 均 **rc=0 / 0 error**。
> ⚠️ **耦合前提**：该 util 头同步目前是 gt_trunk 的**未提交工作区改动**（HEAD 版无该字段），
> 恢复后的 `external_query.h` 必须与 util 头同步**同批提交**，否则干净 HEAD 检出仍会编译失败。
> 语义影响：原适配是"进程内缓存、不写报文"，而 `not_need_check_pwd` 由上上游按报文下发，
> 缓存版读到的恒为 0 → `cust_pwd_checker` 会退回"一律验密"；恢复后按报文取标志，语义正确。
> 备份 `external_query.h.bak_abfver` 已可删除（等 util 头同步提交后）。

## 4. 与 DSE `exit=157` 的关系（结论）

- 自编 dse 链接时用 8 个 **stub 空归档**即可通过（说明 dse 未直接引用 abf_te/query/md/detail 的符号）；
- `ldd dse` 显示只动态依赖 `libabf_util.so`/`libabf_tech.so`（均在 `dse_deps` 中，无缺失）；
- `nm -D dse` 显示 291 个 abf 动态符号，字符串中仅出现 `libabf_tech.so`/`libabf_util.so`，**不含 `libabf_te.so`**，且无 `dlopen`；
- 排除项：依赖缺失、`dse.xml` 位置、Context 缺失（DS 内已存在）、实例名、`TargetPort`（32001/28444/5222 均为 157）、`UserName` 项、大页、igmp、shm 残留。

> **结论**：`dse exit=157` 与 `libabf_te.so` 缺失**无直接关系**；它是 AMI 主流程返回的自定义码（gdb 证 `main` 正常 return）。编出 `libabf_te.so` 并不能解决 157。

## 5. 建议（需外部输入）

| 优先级 | 方案 | 说明 |
|:------:|------|------|
| 1 | **获取与 biz_framework 匹配的 ABF/AMI 资源包** | 含 `not_need_check_pwd`、大宗/CTE/GRisk/信用T0 等新定义；替换 `work_lib/AMI` 后可一次编出全部 6 个 abf 库 |
| 2 | **获取已验证的 dse 部署包**（含运行库） | 直接解决 157，绕开自编 |
| 3 | 向 DSE/AMI 维护方确认 **ExitCode 157** 含义 | 若是配置项缺失，可在 `dse.xml`/DS 配置侧修复 |

## 6. 复现命令

```bash
# 编译（容器内）
docker exec otc bash -lc 'cd /mnt/work/gt_trunk/DYS-FRAMEWORK/biz_framework && \
  export WORK_ROOT=/mnt/work/gt_test BOOST_ROOT=$WORK_ROOT/work_lib/3rd/boost_1_62_0/ \
         AMI_RES_ROOT=$WORK_ROOT/work_lib/AMI_ov TRD_RES_ROOT=$WORK_ROOT/work_lib/3rd && \
  ./build.sh -r'
# 单库
cd build_root/build_release && make abf_te -j8
```
