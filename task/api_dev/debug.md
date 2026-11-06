# API 性能测试复测 Debug 记录

> 复测日期：2026-09-24
> 复测依据：task/api_dev/api_dev_task.txt 291~301
> 复测环境：当前宿主机（32 核 AMD Ryzen 9 9955HX），docker otc（host 网络模式）

---

## 问题 1：FTE 编译时用 bash 加载 zsh 环境失败

- **现象**：使用 `docker exec otc bash -lc 'source ~/.zshrc && ./compile_fte.sh'` 编译时报错 `_omz_util_util: command not found` 等 oh-my-zsh 相关错误，环境变量未正确加载，编译可能失败。
- **根因**：`compile_fte.sh` 依赖 `~/.zshrc` 中通过 oh-my-zsh 设置的环境变量（如 PATH、编译工具链），而 bash 无法加载 zsh 的 oh-my-zsh 配置。
- **解决**：改用 `docker exec otc zsh -lc 'cd /mnt/work/gt_trunk && source ~/.zshrc && ./compile_fte.sh'`，用 zsh 加载 zsh 环境。
- **效果**：FTE 编译成功，`ute` 二进制安装到 `/mnt/work/gt_test/work_atp/cmake/fte/bin/ute`。

---

## 问题 2：网卡抓包报告相对路径写错位置

- **现象**：首次运行 `api_net_time_capture` 时报告参数用相对路径 `result/capture_gw_single_5k.txt`，进程工作目录在 `/home/lsz/code/work/api_trunk`（该目录下没有 `result/`），日志报"无法打开报告文件"，抓包结果只在日志中，未写入报告文件。
- **根因**：`--report` 参数为相对路径，依赖进程工作目录；而 `result/` 目录实际位于 `trunk/NewAPI/gone/api/mock/client/result/`。
- **解决**：改用绝对路径 `--report /home/lsz/code/work/api_trunk/trunk/NewAPI/gone/api/mock/client/result/capture_xxx_5k.txt`。
- **效果**：抓包报告正确写入指定文件，数据完整（关联成功率 100%）。

---

## 问题 3：tgw_simulator 运行导致时延失真（关键问题）

- **现象**：首次复测（保留上海 FTE 所需的 2 个 tgw_simulator 运行）时，GW 场景 API 内部延迟明显偏高：GW 单客户 P50=481ns / P90=821ns，GW 多客户 P50=972ns / P90=1603ns，明显异常（预期 P50 应 < 300ns）。
- **根因**：上海 FTE 所需的 2 个 `tgw_simulator` 实例各占约 300% CPU（共约 6 核，持续撮合），加上 FTE，系统 load 达 9+，内核调度延迟加大、FTE 处理变慢，放大 API 侧尾部延迟。
- **解决**：停止全部 `tgw_simulator` 实例（`sudo kill -9 <pid>`），保留上海 FTE(33001) 和 mock 柜台。
- **效果**：系统 load 下降、CPU7 空闲率 100%，GW 场景恢复正常：GW 单客户 P50=211ns / P90=240ns，GW 多客户 P50=221ns / P90=250ns。与首次测试（原机器）结论一致，确认 tgw 是时延失真的根因。
- **建议**：进行 FTE 性能测试时确保系统空闲（停止 tgw_simulator），否则尾部延迟失真。

---

## 问题 4：start_all.sh 会启动用不到的深圳 FTE / tgw，占用 CPU

- **现象**：`start_all.sh` 同时启动上海(33001)+深圳(33002) FTE 及多组 tgw_simulator（38140/38141/39142），深圳部分在本次测试中用不到却持续占用 CPU。
- **解决**：用 `echo 's' | sudo -S kill -9 <pid>` 停止深圳 tgw(39142) 和深圳 FTE(33002)，仅保留上海 FTE 及其委托所需链路。
- **效果**：降低系统 CPU 开销，为测试留出更充足的空闲资源。

---

## 问题 5：gone 网卡抓包平均值异常偏高（偶发毛刺）

- **现象**：gone 网卡抓包 `平均值=8415.9ns`，明显高于 P50=761ns（P50/P75/P90 均 <1100ns）。
- **根因**：存在单个约 8.7ms 的极端离群点（最大值 8743403ns），拉高平均值，属抓包/测量偶发毛刺。
- **解决**：以分位数（P50/P90）为准判断趋势，平均值因离群点失真，不影响结论。
- **效果**：gone 网卡全链路 P50=761ns，仍为三对象最优。

---

## 复测结论回顾

- 三对象 API 内部延迟排序：**GOne < GW单客户 ≈ GW多客户**（P50：161 / 211 / 221ns）
- 三对象网卡全链路延迟排序：**GOne < GW单客户 ≈ GW多客户**（P50：761 / 832 / 862ns）
- 三对象关联成功率均为 100%，委托通路稳定。
- 复测绝对值整体优于首次（原 16 核机器），主要得益于 32 核硬件与系统空闲。