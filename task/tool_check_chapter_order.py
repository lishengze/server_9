#!/usr/bin/env python3
"""检查 Markdown 文档章节顺序：编号章节应递增，附录类章节必须在最后。
用法: python3 task/tool_check_chapter_order.py <file.md> [...]"""
import re, sys, pathlib
ok = True
for f in sys.argv[1:]:
    secs = [l for l in pathlib.Path(f).read_text().splitlines() if re.match(r'^## [^#]', l)]
    nums, bad = [], []
    for s in secs:
        m = re.match(r'^## (\d+)\.', s)
        nums.append(int(m.group(1)) if m else None)
    known = [n for n in nums if n is not None]
    if known != sorted(known):
        bad.append(f"编号乱序: {known}")
    # 附录必须在最后一段
    idx_app = [i for i, s in enumerate(secs) if s.startswith("## 附")]
    if idx_app and idx_app[-1] != len(secs) - 1:
        bad.append("附录不在最后")
    print(("  OK   " if not bad else "  FAIL ") + f + (" — " + "; ".join(bad) if bad else ""))
    ok = ok and not bad
sys.exit(0 if ok else 1)
