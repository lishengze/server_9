#pragma once

/**
 * @file ring_window.h
 * @brief 环形窗口模块
 *
 * 从大到小排序的窗口，用于少量（如最多几百个）需排序的对象，如行情价格挡位，
 * 替代map。
 *
 * 采用两级数组：主级数组存储key范围和分块窗口，以保证查找内存跨度较小。
 *             子级数组存储数据，排序采用二分查找，并会移动数据来保证顺序
 *
 * 要求：
 *    1. 数据大小应尽量小，特别可用于存储数据指针。
 *
 * 特征：
 *    插入，若找到所在key范围的窗口，优先插入该窗口，否则先插入前向
 *    窗口，再后向，以先填满前面的窗口。
 *    窗口提供V的内存，插入成功返回存储指针。
 *    窗口大小是动态增加，但并不缩小
 *
 * @note 使用模板实现，N必须是2的幂
 */

#include "comm_errno.h"
#include "comm_sys.h"
// #include "mutils.h"

#include <cstring>
#include <new>

namespace lb_common {

/**
 * @brief 环形窗口键值对结构体
 *
 * 用于存储环形窗口中的单个键值对
 *
 * @tparam T_K 键类型
 * @tparam T_V 值类型
 */
template <typename T_K, typename T_V> struct ring_window_data {
  T_K key; ///< 键
  T_V val; ///< 值

  /**
   * @brief 默认构造函数
   */
  ring_window_data() = default;

  /**
   * @brief 构造函数
   *
   * @param[in] k 键值
   * @param[in] v 值
   */
  ring_window_data(const T_K &k, const T_V &v) : key(k), val(v) {}
};

/**
 * @brief 环形窗口地址结构体
 *
 * 用于表示在环形窗口中的位置和状态信息
 */
struct ring_window_addr {
  int32 index; ///< 索引位置
  int32 flag;  ///< 状态标志：0-空；1-存在；2-下一个；3-末尾

  /**
   * @brief 默认构造函数
   */
  ring_window_addr() : index(-1), flag(0) {}

  /**
   * @brief 构造函数
   *
   * @param[in] idx 索引位置
   * @param[in] f 状态标志
   */
  ring_window_addr(int32 idx, int32 f) : index(idx), flag(f) {}
};

/**
 * @brief 环形窗口类
 *
 * 从大到小排序的窗口，用于少量（如最多几百个）需排序的对象，如行情价格挡位。
 * 采用两级数组：主级数组存储key范围和分块窗口，以保证查找内存跨度较小。
 * 子级数组存储数据，排序采用二分查找，并会移动数据来保证顺序。
 *
 * @tparam N 窗口大小，必须是2的幂
 * @tparam K 键类型
 * @tparam V 值类型
 * @tparam CMP 比较器类型
 */
template <int N, typename K, typename V, class CMP> class ring_window {
private:
  /** @brief 键值对类型别名 */
  typedef ring_window_data<K, V> m_it;

  /** @brief 当前使用的元素数量 */
  int32 used;
  /** @brief 起始索引位置 */
  int32 start_idx;
  /** @brief 掩码，用于环形索引计算，等于N-1 */
  int32 mask;
  /** @brief 填充标志 */
  int32 filled;
  /** @brief 缓冲区数组，存储键值对 */
  m_it bufs[N];

  /**
   * @brief 获取下一个索引位置
   *
   * @param[in] idx 当前索引
   * @return 下一个索引位置
   */
  FORCE_INLINE int32 next(int32 idx) const { return ((idx + 1) & mask); }
  /**
   * @brief 获取上一个索引位置
   *
   * @param[in] idx 当前索引
   * @return 上一个索引位置
   */
  FORCE_INLINE int32 prev(int32 idx) const { return ((idx + mask) & mask); }
  /**
   * @brief 将位置转换为索引
   *
   * @param[in] pos 位置（从起始位置开始）
   * @return 对应的数组索引
   */
  FORCE_INLINE int32 pos_to_idx(int32 pos) const { return ((start_idx + pos) & mask); }
  /**
   * @brief 将索引转换为位置
   *
   * @param[in] idx 数组索引
   * @return 对应的位置（从起始位置开始）
   */
  FORCE_INLINE int32 idx_to_pos(int32 idx) const { return ((idx - start_idx + N) & mask); }

  /**
   * @brief 查找键在窗口中的位置
   *
   * 使用二分查找算法定位键的位置
   *
   * @param[out] o_pos 输出参数，返回找到的位置
   * @param[in] key 要查找的键
   * @return int32 状态码：-1
   * 空；0-头+key相等；1-头+下一个；2-中间+key相等；3-中间+下一个；4-末尾
   */
  int32 find_pos(int32 &o_pos, K key) {
    if (used == 0) {
      o_pos = 0;
      return -1;
    }

    CMP cmp;
    int64 cmp_res = cmp(bufs[start_idx].key, key);
    if (cmp_res == 0) {
      o_pos = 0;
      return 0;
    } else if (cmp_res < 0) {
      o_pos = 0;
      return 1;
    } else if (used == 1) {
      o_pos = 1;
      return 4;
    }

    int32 left = 0;
    int32 right = used;
    while (left < right) {
      int32 mid = left + ((right - left) >> 1);
      cmp_res = cmp(bufs[pos_to_idx(mid)].key, key);

      if (cmp_res == 0) {
        o_pos = mid;
        return 2;               // 已存在，返回当前位置
      } else if (cmp_res > 0) { // 目标key更小，插入右侧
        left = mid + 1;
      } else {
        right = mid;
      }
    }
    o_pos = left;
    return (left < used ? 3 : 4); // 目标key 的下一key 位置
  }

  /**
   * @brief 向后移动数据
   *
   * 将数据从起始位置到结束位置向后移动一个位置
   *
   * @param[in] move_end 移动结束位置（闭区间）
   * @param[in] start_next 起始下一个位置
   */
  void move_back(int32 move_end, int32 start_next) {
    int32 dst = start_next;
    while (dst != move_end) {
      int32 src = prev(dst);
      bufs[dst] = bufs[src];
      dst = src;
    }
  }

  /**
   * @brief 向前移动数据
   *
   * 将数据从起始位置到结束位置向前移动一个位置
   *
   * @param[in] move_end 移动结束位置（开区间）
   * @param[in] move_start 移动起始位置（闭区间）
   */
  void move_forward(int32 move_end, int32 move_start) {
    int32 src = move_start;
    int32 dst = prev(src);
    while (src != move_end) {
      bufs[dst] = bufs[src];
      dst = src;
      src = next(src);
    }
  }

  /**
   * @brief 分裂移动数据到另一个环形窗口
   *
   * 将当前窗口中从指定位置开始的数据移动到目标窗口
   *
   * @param[out] o_ring 目标环形窗口
   * @param[in] move_start 移动起始位置
   */
  void split_move(ring_window &o_ring, int move_start) {
    int32 move_end = ((start_idx + used) & mask);
    int32 src = move_start;
    int32 i = 0;
    int32 dst = 1;
    while (src != move_end) {
      o_ring.bufs[dst] = bufs[src];
      src = next(src);
      dst = next(dst);
      i++;
    }
    used -= i;
    o_ring.start_idx = 1;
    o_ring.used = i;
  }

public:
  /**
   * @brief 获取指定索引的键值对
   *
   * @param[in] idx 索引位置
   * @return 键值对引用
   */
  FORCE_INLINE ring_window_data<K, V> &get(int32 idx) {
    assert(idx >= 0 && idx < N);
    return bufs[idx];
  }
  /**
   * @brief 获取指定索引的值
   *
   * @param[in] idx 索引位置
   * @return 值的引用
   */
  FORCE_INLINE V &get_val(int32 idx) {
    assert(idx >= 0 && idx < N);
    return bufs[idx].val;
  }

  /**
   * @brief 获取第一个元素的索引
   *
   * @return int32 第一个元素的索引，空时返回-1
   */
  FORCE_INLINE int32 first_i() const { return (used > 0 ? start_idx : -1); }
  /**
   * @brief 获取最后一个元素的索引
   *
   * @return int32 最后一个元素的索引，空时返回-1
   */
  FORCE_INLINE int32 last_i() const { return (used > 0 ? ((start_idx + used - 1) & mask) : -1); }

  /**
   * @brief 获取下一个元素的索引
   *
   * @param[in] idx 当前索引
   * @return int32 下一个元素的索引，无下一个时返回-1
   */
  FORCE_INLINE int32 next_i(int32 idx) const {
    if (used > 0 && idx != ((start_idx + used - 1) & mask))
      return ((idx + 1) & mask);
    return -1;
  }

  /**
   * @brief 获取上一个元素的索引
   *
   * @param[in] idx 当前索引
   * @return int32 上一个元素的索引，无上一个时返回-1
   */
  FORCE_INLINE int32 prev_i(int32 idx) const {
    if (used > 0 && idx != start_idx)
      return ((idx + mask) & mask);
    return -1;
  }

  /**
   * @brief 获取当前元素数量
   *
   * @return int32 当前使用的元素数量
   */
  FORCE_INLINE int32 size() const { return used; }
  /**
   * @brief 检查是否还有空间
   *
   * @return bool true表示还有空间，false表示已满
   */
  FORCE_INLINE bool have_space() const { return used < N; }
  /**
   * @brief 查找键在窗口中的位置和状态
   *
   * @param[out] o_addr 输出参数，返回地址和状态信息
   * @param[in] key 要查找的键
   * @return bool 是否找到（flag为1表示找到）
   */
  bool find(ring_window_addr &o_addr, K key) {
    int32 fpos;
    int32 fret = find_pos(fpos, key);

    switch (fret) {
    case 0: // 头相等
      o_addr.index = start_idx;
      o_addr.flag = 1;
      break;
    case 1: // 优于头，头为下一个
      o_addr.index = start_idx;
      o_addr.flag = 2;
      break;
    case 2: // 中间或末尾相等
      o_addr.index = pos_to_idx(fpos);
      o_addr.flag = 1;
      break;
    case 3: // 下一个中间
      o_addr.index = pos_to_idx(fpos);
      o_addr.flag = 2;
      break;
    case 4: // 末尾
      o_addr.index = ((start_idx + used - 1) & mask);
      o_addr.flag = 3;
      break;
    default: // case -1:
      start_idx = (N >> 1);
      o_addr.index = start_idx;
      o_addr.flag = 0;
      break;
    }

    return (o_addr.flag == 1);
  }

  /**
   * @brief 设置指定索引的值
   *
   * @param[in] val 要设置的值
   * @param[in] idx 索引位置
   */
  FORCE_INLINE void set_val(const V &val, int32 idx) {
    assert(idx >= 0 && idx < N);
    bufs[idx].val = val;
  }

  /**
   * @brief 插入并查找键值对
   *
   * 根据查找结果插入键值对，可能覆盖或移动现有数据
   *
   * @param[out] o_move 输出参数，被移动的数据
   * @param[in] find_addr 查找地址结果
   * @param[in] val 要插入的值
   * @param[in] key 要插入的键
   * @return int32 返回码：0-未满新插入；1-覆盖插入；-1-窗口满且key最小未插入
   */
  int32 insert_find(ring_window_data<K, V> &o_move, ring_window_addr &find_addr, const V &val, K key) {
    int32 ret = 0;
    int32 dst_idx = find_addr.index;

    if (find_addr.flag < 2) {
      // dst_idx = find_addr.index;
      bufs[dst_idx].key = key;
      bufs[dst_idx].val = val;
    } else if (find_addr.flag == 3) {
      if (used < N) {
        dst_idx = ((start_idx + used) & mask);
        bufs[dst_idx].key = key;
        bufs[dst_idx].val = val;
      } else {
        ret = -1;
      }
    } else if (dst_idx == start_idx) {
      dst_idx = prev(find_addr.index);
      start_idx = dst_idx;
      if (used >= N) {
        o_move = bufs[dst_idx];
        ret = 1;
      }
      bufs[dst_idx].key = key;
      bufs[dst_idx].val = val;
    } else {
      if (used >= N) {
        o_move = bufs[prev(start_idx)];
        ret = 1;
      }
      // dst_idx = find_addr.index;
      if (idx_to_pos(dst_idx) < (used >> 1)) {
        move_forward(dst_idx, start_idx);
        start_idx = prev(start_idx);
        dst_idx = prev(dst_idx);
      } else { // 中间
        int last_pos = (used < N) ? ((start_idx + used - 1) & mask) : prev(start_idx);
        move_back(dst_idx, last_pos);
      }
      bufs[dst_idx].key = key;
      bufs[dst_idx].val = val;
    }

    if (find_addr.flag != 1 && used < N)
      used++;

    return ret;
  }

  /**
   * @brief 在头部插入键值对
   *
   * @param[in] val 要插入的值
   * @param[in] key 要插入的键
   * @return int32 插入位置的索引，失败返回-1
   */
  int32 push_head(const V &val, K key) {
    if (used >= N)
      return -1;

    int32 dst_idx = prev(start_idx);
    bufs[dst_idx].key = key;
    bufs[dst_idx].val = val;
    start_idx = dst_idx;
    used++;
    return dst_idx;
  }

  /**
   * @brief 在尾部插入键值对
   *
   * @param[in] val 要插入的值
   * @param[in] key 要插入的键
   * @return int32 插入位置的索引，失败返回-1
   */
  int32 push_tail(const V &val, K key) {
    if (used >= N)
      return -1;

    int32 dst_idx = ((start_idx + used) & mask);
    bufs[dst_idx].key = key;
    bufs[dst_idx].val = val;
    used++;
    return dst_idx;
  }

  /**
   * @brief 删除最优元素（优先删除最优元素）
   *
   * @param[out] o_move 输出参数，被删除的元素
   * @return bool 成功返回true，空窗口返回false
   */
  bool pop(ring_window_data<K, V> &o_move) {
    if (used == 0) {
      return false;
    }

    o_move = bufs[start_idx];
    start_idx = next(start_idx);
    used--;
    return true;
  }

  /**
   * @brief 删除指定键的元素
   *
   * @param[out] o_move 输出参数，被删除的值
   * @param[in] key 要删除的键
   * @return bool 成功返回true，键不存在返回false
   */
  bool erase(V &o_move, K key) {
    int32 fpos;
    int32 fret = find_pos(fpos, key);
    int32 dst_idx;

    switch (fret) {
    case 0: // 头相等
      o_move = bufs[start_idx].val;
      start_idx = next(start_idx);
      break;
    case 1: // 优于头，头为下一个
      return false;
    case 2: // 中间相等
      dst_idx = pos_to_idx(fpos);
      o_move = bufs[dst_idx].val;

      if (fpos < (used >> 1)) {
        move_back(start_idx, dst_idx);
        start_idx = next(start_idx);
      } else {
        move_forward(((start_idx + used) & mask), next(dst_idx));
      }

      break;
    case 3: // 下一个中间
      return false;
    case 4: // 尾相等
      dst_idx = pos_to_idx(fpos);
      o_move = bufs[dst_idx].val;
      break;
    default: // case -1://5
      return false;
    }

    used--;
    return true;
  }

  /**
   * @brief 重置环形窗口
   *
   * 清空所有数据，重置索引和状态
   */
  void reset() {
    start_idx = (N >> 1);
    used = 0;
    mask = N - 1;
    filled = 0;
  }

  /**
   * @brief 构造函数
   */
  ring_window() : used(0), start_idx((N >> 1)), mask(N - 1), filled(0) {
    static_assert((N & (N - 1)) == 0, "N must be a power of 2");
  }

  /**
   * @brief 析构函数
   */
  ~ring_window() = default;
};

/**
 * @brief 环形范围数据结构体
 *
 * 用于存储键范围和对应的窗口指针
 *
 * @tparam K 键类型
 * @tparam V 值类型（窗口类型）
 * @tparam CMP 比较器类型
 */
template <class K, class V, class CMP> struct ring_range_data {
  K first_key; ///< 范围的第一个键
  K last_key;  ///< 范围的最后一个键
  V *val;      ///< 指向窗口的指针

  /**
   * @brief 默认构造函数
   */
  ring_range_data() : first_key(), last_key(), val(nullptr) {}

  /**
   * @brief 初始化范围数据
   *
   * @param[in] pv 窗口指针
   * @param[in] key 初始键
   */
  void init(V *pv, K key) {
    first_key = key;
    last_key = key;
    val = pv;
  }

  /**
   * @brief 更新键范围
   *
   * 根据新键更新first_key和last_key
   *
   * @param[in] new_key 新键
   */
  void update(K new_key) {
    CMP cmp;
    if (cmp(first_key, new_key) < 0) {
      first_key = new_key;
    }
    if (cmp(last_key, new_key) > 0) {
      last_key = new_key;
    }
  }
};

/**
 * @brief 环形范围地址结构体
 *
 * 用于表示在环形范围缓冲区中的位置和状态信息
 */
struct ring_range_addr {
  int32 index;               ///< 范围索引
  int32 flag;                ///< 状态标志：0-空；1-存在；2-当前；3-末尾
  ring_window_addr win_addr; ///< 窗口地址信息

  /**
   * @brief 默认构造函数
   */
  ring_range_addr() : index(-1), flag(0) {}

  /**
   * @brief 构造函数
   *
   * @param[in] idx 范围索引
   * @param[in] f 状态标志
   * @param[in] addr 窗口地址信息
   */
  ring_range_addr(int32 idx, int32 f, const ring_window_addr &addr) : index(idx), flag(f), win_addr(addr) {}
};

/**
 * @brief 环形范围缓冲区类
 *
 * 采用两级数组结构：主级数组存储key范围和分块窗口，子级数组存储实际数据。
 * 用于管理多个环形窗口的键范围，支持动态扩展缓冲区容量。
 *
 * @tparam N 窗口大小，必须是2的幂
 * @tparam K 键类型
 * @tparam V 值类型
 * @tparam CMP 比较器类型
 */
template <int N, class K, class V, class CMP> class ring_range_buf {
private:
  using m_val = ring_window<N, K, V, CMP>;
  using m_it = ring_range_data<K, m_val, CMP>;

#define RING_RANGE_VAL_BUFNUM 8

  /** @brief 当前已使用的范围数量 */
  int32 used;
  /** @brief 起始索引位置 */
  int32 start_idx;
  /** @brief 掩码，用于环形索引计算 */
  int32 mask;
  /** @brief 当前存储的值总数 */
  int32 v_num;
  /** @brief 范围缓冲区数组 */
  m_it *bufs;
  /** @brief 空闲窗口数量 */
  int32 free_num;
  /** @brief 第一个空闲窗口索引 */
  int32 free_first;
  /** @brief 空闲窗口缓存池 */
  m_val *free_vals[RING_RANGE_VAL_BUFNUM];

  /**
   * @brief 获取下一个索引位置
   *
   * @param[in] idx 当前索引
   * @return int32 下一个索引位置
   */
  FORCE_INLINE int32 next(int32 idx) const { return ((idx + 1) & mask); }
  /**
   * @brief 获取上一个索引位置
   *
   * @param[in] idx 当前索引
   * @return int32 上一个索引位置
   */
  FORCE_INLINE int32 prev(int32 idx) const { return ((idx + mask) & mask); }
  /**
   * @brief 将位置转换为索引
   *
   * @param[in] pos 位置（从起始位置开始）
   * @return int32 对应的数组索引
   */
  FORCE_INLINE int32 pos_to_idx(int32 pos) const { return (start_idx + pos) & mask; }
  /**
   * @brief 将索引转换为位置
   *
   * @param[in] idx 数组索引
   * @return int32 对应的位置（从起始位置开始）
   */
  FORCE_INLINE int32 idx_to_pos(int32 idx) const { return (idx - start_idx + mask + 1) & mask; }

  /**
   * @brief 更新范围项的键范围
   *
   * 根据窗口中的第一个和最后一个元素更新范围的first_key和last_key
   *
   * @param[in] pi 范围项引用
   */
  static inline void update_it(m_it &pi) {
    m_val *pw = pi.val;
    int tf = pw->first_i();
    int tl = pw->last_i();
    pi.first_key = pw->get(tf).key;
    pi.last_key = pw->get(tl).key;
  }

  /**
   * @brief 查找键在范围缓冲区中的位置
   *
   * 使用二分查找算法定位键的位置
   *
   * @param[out] o_pos 输出参数，返回找到的位置
   * @param[in] key 要查找的键
   * @return int32 状态码：-1
   * 空；0-头+key相等；1-头+下一个；2-中间+key相等；3-中间+下一个；4-末尾
   */
  int32 find_pos(int32 &o_pos, K key) {
    if (used == 0) {
      o_pos = 0;
      return -1;
    }

    CMP cmp;
    m_it *pi = bufs + start_idx;
    int64 first_res = cmp(pi->first_key, key);
    int64 last_res = cmp(pi->last_key, key);
    if (first_res >= 0 && last_res <= 0) {
      o_pos = 0;
      return 0;
    } else if (first_res < 0) {
      o_pos = 0;
      return 1;
    } else if (used == 1) {
      o_pos = 1;
      return 4;
    }

    int32 left = 0;
    int32 right = used;
    while (left < right) {
      int mid = left + ((right - left) >> 1);
      pi = bufs + pos_to_idx(mid);
      first_res = cmp(pi->first_key, key);
      last_res = cmp(pi->last_key, key);

      if (first_res >= 0 && last_res <= 0) {
        o_pos = mid;
        return 2;                // 已存在，返回当前位置
      } else if (last_res > 0) { // 目标key更小，插在右侧
        left = mid + 1;
      } else {
        right = mid;
      }
    }
    o_pos = left;
    return (left < used ? 3 : 4); // 目标key 的下一key 位置
  }

  /**
   * @brief 向后移动数据
   *
   * 将数据从起始位置到结束位置向后移动一个位置
   *
   * @param[in] move_end 移动结束位置（闭区间）
   * @param[in] start_next 起始下一个位置
   */
  void move_back(int32 move_end, int32 start_next) {
    int dst = start_next;
    while (dst != move_end) {
      int src = prev(dst);
      bufs[dst] = bufs[src];
      dst = src;
    }
  }

  /**
   * @brief 向前移动数据
   *
   * 将数据从起始位置到结束位置向前移动一个位置
   *
   * @param[in] move_end 移动结束位置（开区间）
   * @param[in] move_start 移动起始位置（闭区间）
   */
  void move_forward(int32 move_end, int32 move_start) {
    int32 src = move_start;
    int32 dst = prev(src);
    while (src != move_end) {
      bufs[dst] = bufs[src];
      dst = src;
      src = next(src);
    }
  }

  /**
   * @brief 扩展范围缓冲区容量
   *
   * @param[in] prev_idx 前一个索引位置
   * @return int32 新的起始索引，失败返回错误码
   */
  int32 expand(int32 prev_idx) {
    int32 tn;
    // int32 tadd;
    if (mask > 0) {
      assert(used == mask + 1);
      tn = (mask + 1) * 2;
      // tadd = mask+1;
    } else {
      tn = RING_RANGE_VAL_BUFNUM * 2;
      // tadd = RING_RANGE_VAL_BUFNUM*2;
    }

    m_it *pn = new (std::nothrow) m_it[tn];
    if (NULL == pn) {
      return LBERR_MEM_ALLOC_FAIL;
    }

    if (mask == 0) {
      used = 0;
      start_idx = 4;
      mask = tn - 1;
      v_num = 0;
      bufs = pn;
      free_first = 0;
      free_num = 0;
      return start_idx;
    }

    int32 i = 0;
    int32 td = 4;
    int32 ts = start_idx;
    int32 te = next(prev_idx);
    while (ts != te) {
      pn[td] = bufs[ts];
      td = next(td);
      ts = next(ts);
      i++;
    }
    int32 ret = td;
    td = next(td);

    while (i < (mask + 1)) {
      pn[td] = bufs[ts];
      td = next(td);
      ts = next(ts);
      i++;
    }
    delete[] bufs;

    bufs = pn;
    start_idx = 4;
    mask = tn - 1;
    return ret;
  }

  /**
   * @brief 分配一个值窗口
   *
   * @param[out] o_val 输出参数，返回分配的窗口指针
   * @return int32 0表示成功，负数表示错误码
   */
  int32 alloc_val(m_val *&o_val) {
    if (free_num <= 0) {
      free_first = 0;
      int i = 0;
      while (i < 4) {
        m_val *tp = new (std::nothrow) m_val;
        if (NULL == tp)
          return LBERR_MEM_ALLOC_FAIL;
        free_vals[i] = tp;
        i++;
      }
      free_num = 4;
    }
    o_val = free_vals[free_first];
    free_first++;
    free_num--;
    return 0;
  }

  /**
   * @brief 释放值窗口
   *
   * @param[in] pval 要释放的窗口指针
   */
  void free_val(m_val *pval) {
    pval->reset();
    if (free_first + free_num < RING_RANGE_VAL_BUFNUM) {
      free_vals[free_first] = pval;
      free_num++;
    } else if (free_num < RING_RANGE_VAL_BUFNUM) {
      free_first--;
      free_vals[free_first] = pval;
      free_num++;
    } else {
      delete pval;
    }
  }

public:
  /**
   * @brief 获取第一个值
   *
   * @param[out] o_val 输出参数，返回第一个值
   * @return bool 成功返回true，空或无数据返回false
   */
  FORCE_INLINE bool first_val(V &o_val) {
    if (unlikely(used <= 0))
      return false;
    m_val *tf_win = bufs[start_idx].val;
    int32 tfi_win = tf_win->first_i();
    if (unlikely(tfi_win == -1))
      return false;
    o_val = tf_win->get_val(tfi_win);
    return true;
  }

  /**
   * @brief 获取指定索引的窗口
   *
   * @param[in] idx 索引位置
   * @return ring_window<N,K,V,CMP>* 窗口指针
   */
  FORCE_INLINE ring_window<N, K, V, CMP> *get_window(int32 idx) { return bufs[idx].val; }

  /**
   * @brief 获取起始索引
   *
   * @return int32 起始索引，空时返回-1
   */
  FORCE_INLINE int32 start_i() const { return used > 0 ? start_idx : -1; }

  /**
   * @brief 获取结束索引
   *
   * @return int32 结束索引，空时返回-1
   */
  FORCE_INLINE int32 last_i() const { return used > 0 ? ((start_idx + used - 1) & mask) : -1; }

  /**
   * @brief 获取下一个索引
   *
   * @param[in] idx 当前索引
   * @return int32 下一个索引，无下一个时返回-1
   */
  FORCE_INLINE int32 next_i(int32 idx) const {
    if (used > 0 && idx != ((start_idx + used - 1) & mask))
      return ((idx + 1) & mask);
    return -1;
  }

  /**
   * @brief 获取上一个索引
   *
   * @param[in] idx 当前索引
   * @return int32 上一个索引，无上一个时返回-1
   */
  FORCE_INLINE int32 prev_i(int32 idx) const {
    if (used > 0 && idx != start_idx)
      return ((idx + mask) & mask);
    return -1;
  }

  /**
   * @brief 获取值的总数
   *
   * @return int32 值的总数
   */
  FORCE_INLINE int32 num() const { return v_num; }

  /**
   * @brief 获取范围数量
   *
   * @return int 使用的范围数量
   */
  FORCE_INLINE int size() const { return used; }

  /**
   * @brief 查找键在范围缓冲区中的位置和状态
   *
   * @param[out] o_addr 输出参数，返回地址和状态信息
   * @param[in] key 要查找的键
   * @return bool 是否找到（flag为1表示找到）
   */
  bool find(ring_range_addr &o_addr, K key) {
    int32 fpos = 0;
    int32 fret = find_pos(fpos, key);
    int32 dst_win;
    m_val *pval;

    switch (fret) {
    case 0: // 头相等
      pval = bufs[start_idx].val;
      o_addr.index = start_idx;
      o_addr.flag = pval->find(o_addr.win_addr, key);
      if (o_addr.flag == 0) {
        o_addr.flag = 2;
      }
      break;
    case 1: // 优于头，头为下一个
      pval = bufs[start_idx].val;
      o_addr.index = start_idx;
      o_addr.flag = 2;
      o_addr.win_addr.index = pval->first_i();
      o_addr.win_addr.flag = 2;
      break;
    case 2: // 中间或末尾相等
      dst_win = pos_to_idx(fpos);
      pval = bufs[dst_win].val;
      o_addr.index = dst_win;
      o_addr.flag = pval->find(o_addr.win_addr, key);
      if (o_addr.flag == 0) {
        o_addr.flag = 2;
      }
      break;
    case 3: // 下一个中间
      fpos--;
      dst_win = pos_to_idx(fpos);
      pval = bufs[dst_win].val;
      o_addr.index = dst_win;
      o_addr.flag = 2;
      o_addr.win_addr.index = pval->last_i();
      o_addr.win_addr.flag = 3;
      break;
    case 4: // 末尾
      dst_win = ((start_idx + used - 1) & mask);
      pval = bufs[dst_win].val;
      o_addr.index = dst_win;
      o_addr.flag = 3;
      o_addr.win_addr.index = pval->last_i();
      o_addr.win_addr.flag = 3;
      break;
    default: // case -1:
      start_idx = 2;
      o_addr.index = start_idx;
      o_addr.flag = 0;
      o_addr.win_addr.index = (N >> 1);
      o_addr.win_addr.flag = 0;
      break;
    }

    return (o_addr.flag == 1);
  }

  /**
   * @brief 获取指定地址的值
   *
   * @param[in] addr 范围地址
   * @return V& 值的引用
   */
  FORCE_INLINE V &get_val(ring_range_addr &addr) {
    m_val *pval = bufs[addr.index].val;
    return pval->get_val(addr.win_addr.index);
  }

  /**
   * @brief 插入并查找键值对
   *
   * 根据查找结果插入键值对到适当的范围窗口中
   *
   * @param[in] find_addr 查找地址结果
   * @param[in] val 要插入的值
   * @param[in] key 要插入的键
   * @return int32 返回码：0-新插入；1-已存在；<0出错
   */
  int32 insert_find(ring_range_addr &find_addr, const V &val, K key) {
    int32 dst_win;
    int32 ret;
    m_val *pval;

    if (find_addr.flag >= 2) {
      ring_window_data<K, V> td;

      dst_win = find_addr.index;
      pval = bufs[dst_win].val;
      ret = pval->insert_find(td, find_addr.win_addr, val, key);

      if (ret == 0) {
        bufs[dst_win].update(key);
        v_num++;
        return 0;
      } else if (ret == 1) {
        update_it(bufs[dst_win]);
      } else {
        td.key = key;
        td.val = val;
      }

      m_val *pw_new = NULL;
      dst_win = next_i(dst_win);
      if (dst_win != -1) {
        pw_new = bufs[dst_win].val;
        int32 dst_idx = pw_new->push_head(td.val, td.key);
        if (dst_idx >= 0) {
          bufs[dst_win].first_key = td.key;
          v_num++;
          return 0;
        }
        dst_win = prev(dst_win);
      }

      if (used < mask + 1) {
        int32 trc = alloc_val(pw_new);
        if (trc < 0)
          return trc;
        pw_new->push_tail(td.val, td.key);

        if (idx_to_pos(dst_win) < (used >> 1)) {
          move_forward(next(dst_win), start_idx);
          start_idx = prev(start_idx);
        } else {
          dst_win = next(dst_win);
          move_back(dst_win, ((start_idx + used) & mask));
        }

        bufs[dst_win].init(pw_new, td.key);
        v_num++;
        used++;
      } else {
        int32 trc = expand(dst_win);
        if (trc < 0)
          return trc;
        trc = alloc_val(pw_new);
        if (trc < 0)
          return trc;

        pw_new->push_tail(td.val, td.key);
        bufs[trc].init(pw_new, td.key);
        v_num++;
        used++;
      }
    } else if (find_addr.flag == 1) {
      dst_win = find_addr.index;
      pval = bufs[dst_win].val;
      pval->set_val(val, dst_win);
      return 1;
    } else {
      int32 trc;
      if (unlikely(mask == 0)) {
        trc = expand(find_addr.index);
        if (trc < 0)
          return trc;
      }
      trc = alloc_val(pval);
      if (trc < 0)
        return trc;

      pval->push_tail(val, key);
      start_idx = 4;
      bufs[start_idx].init(pval, key);
      used = 1;
      v_num = 1;
    }
    return 0;
  }

  /**
   * @brief 删除指定键的元素
   *
   * @param[out] o_move 输出参数，被删除的值
   * @param[in] key 要删除的键
   * @return bool 成功返回true，键不存在返回false
   */
  bool erase(V &o_move, K key) {
    int32 fpos = 0;
    int32 fret = find_pos(fpos, key);
    if (fret != 0 && fret != 2) {
      return false;
    }

    int32 dst_idx = pos_to_idx(fpos);
    m_val *pw = bufs[dst_idx].val;
    if (!pw->erase(o_move, key)) {
      return false;
    }

    if (pw->size() > 0) {
      update_it(bufs[dst_idx]);
      v_num--;
      return true;
    }
    bufs[dst_idx].val = NULL;
    free_val(pw);

    if (fpos < (used >> 1)) {
      move_back(start_idx, dst_idx);
      start_idx = next(start_idx);
    } else if (fpos < used) {
      move_forward(((start_idx + used) & mask), next(dst_idx));
    }
    used--;
    v_num--;
    return true;
  }

  /**
   * @brief 删除最优元素（优先删除最优元素）
   *
   * @param[out] o_move 输出参数，被删除的值
   * @return bool 成功返回true，空缓冲区返回false
   */
  bool pop(V &o_move) {
    int32 tfi_lv = start_i();
    if (unlikely(tfi_lv == -1)) {
      return false;
    }
    ring_window<N, K, V, CMP> *tf_win = get_window(tfi_lv);
    ring_window_data<K, V> td;
    if (unlikely(!tf_win->pop(td))) {
      return false;
    }

    o_move = td.val;
    if (tf_win->size() > 0) {
      update_it(bufs[tfi_lv]);
      v_num--;
      return true;
    }

    bufs[tfi_lv].val = NULL;
    free_val(tf_win);
    used--;
    v_num--;
    return true;
  }

  /**
   * @brief 初始化范围缓冲区
   */
  void init() {
    used = 0;
    start_idx = 0;
    mask = 0;
    v_num = 0;
    bufs = NULL;
    free_num = 0;
    for (int32 i = 0; i < 8; i++) {
      free_vals[i] = NULL;
    }
  }

  /**
   * @brief 关闭范围缓冲区并释放资源
   */
  void close() {
    if (mask > 0 && NULL != bufs) {
      int32 tidx = start_idx;
      for (int32 i = 0; i < used; i++) {
        m_val *pw = bufs[tidx].val;
        if (NULL != pw) {
          delete pw;
          bufs[tidx].val = NULL;
        }
        tidx = next(tidx);
      }

      delete[] bufs;
    }

    if (free_num > 0) {
      int32 tpos = free_first;
      for (int32 i = 0; i < free_num; i++) {
        m_val *pw = free_vals[tpos];
        if (NULL != pw) {
          delete pw;
          free_vals[tpos] = NULL;
        }
        tpos++;
      }
    }

    init();
  }

  /**
   * @brief 构造函数
   */
  ring_range_buf() : used(0), start_idx(0), mask(0), v_num(0), bufs(NULL), free_num(0) {}

  /**
   * @brief 析构函数
   */
  ~ring_range_buf() { close(); }
};

} // namespace lb_common
