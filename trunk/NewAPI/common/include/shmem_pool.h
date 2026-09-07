#pragma once

/**
 * @file shmem_pool.h
 * @brief 共享内存池模块
 *
 * 这是基于共享内存的动态内存池。
 *
 * 限制：
 *    1. 存储的数据必须为POD类型
 *
 * 特征：
 *    1. 内存按区域和分页来管理，区域用来扩展共享内存，
 *       每个区域分页管理，每个页存储一项或多项数据
 *    2. 内存申请按页分配，分配的页私有化，类似dync_pool的两级管理
 *    3. 每个页存储同一数据，不同页的数据可以不同
 *
 * @note 仅单进程获取释放内存，其他进程仅依据地址读内存
 */

#include "comm_sys.h"
#include "mlock.h"

#include <vector>

namespace lb_common {

/**
 * @brief 共享内存数据地址结构体
 *
 * 用于标识共享内存中数据的精确位置。
 * 通过区域ID、数据ID和块ID三层定位体系精确定位数据。
 */
struct alignas(8) shm_data_addr {
  /** @brief 区域ID，标识数据所在的共享内存区域 */
  int16 area_id;
  /** @brief 数据ID，标识数据在块中的位置 */
  int16 data_id;
  /** @brief 块ID，标识数据所在的内存块 */
  int32 block_id;

  /**
   * @brief 初始化地址为无效值
   *
   * 将所有ID设置为-1，表示地址无效。
   */
  FORCE_INLINE void init() {
    area_id = -1;
    data_id = -1;
    block_id = -1;
  }
};
/**
 * @brief 共享内存地址联合体
 *
 * 提供地址和位置两种表示方式的转换。
 * 支持原子操作，用于多线程环境下的安全地址访问。
 */
union shm_addr_unit {
  /** @brief 地址形式，包含区域、数据、块三层信息 */
  shm_data_addr addr;
  /** @brief 位置形式，用于原子操作和快速比较 */
  int64 pos;
};
/**
 * @brief 检查共享内存地址是否有效
 *
 * @param[in] src 要检查的共享内存地址
 * @return true 地址有效，false 地址无效
 */
STATIC_FORCE_INLINE bool shm_addr_valid(shm_data_addr &src) { return (src.area_id != -1 && src.block_id != -1); }

/**
 * @brief 原子设置共享内存地址单元
 *
 * 使用原子操作设置目标地址单元，确保多线程安全。
 *
 * @param[out] dst 目标地址单元
 * @param[in] src 源地址单元
 */
STATIC_FORCE_INLINE void set_shm_unit(shm_addr_unit &dst, shm_addr_unit &src) { atomic_store64(&(dst.pos), src.pos); }
/**
 * @brief 原子设置共享内存地址单元（通过参数）
 *
 * 使用原子操作设置目标地址单元，确保多线程安全。
 *
 * @param[out] dst 目标地址单元
 * @param[in] area_id 区域ID
 * @param[in] data_id 数据ID
 * @param[in] block_id 块ID
 */
STATIC_FORCE_INLINE void set_shm_unit(shm_addr_unit &dst, int16 area_id, int16 data_id, int32 block_id) {
  shm_addr_unit td;
  td.addr.area_id = area_id;
  td.addr.data_id = data_id;
  td.addr.block_id = block_id;
  atomic_store64(&(dst.pos), td.pos);
}
/**
 * @brief 原子初始化共享内存地址单元为无效值
 *
 * 将地址单元的所有字段设置为-1，表示地址无效。
 *
 * @param[out] dst 要初始化的地址单元
 */
STATIC_FORCE_INLINE void init_shm_unit(shm_addr_unit &dst) {
  shm_addr_unit td;
  td.addr.area_id = -1;
  td.addr.data_id = -1;
  td.addr.block_id = -1;
  atomic_store64(&(dst.pos), td.pos);
}

/**
 * @brief 共享内存池恢复接口类
 *
 * 用于进程重启后恢复共享内存中的数据。
 */
class shmem_pool_recover {
public:
  /**
   * @brief 恢复数据的纯虚函数
   * @param[in] data_addr 数据地址
   * @param[out] pdata 数据指针
   * @param[in] data_size 数据大小
   * @param[in] data_type 数据类型
   * @return 是否有效，true表示有效，false表示无效
   */
  virtual bool recover_data(shm_data_addr &data_addr, char *pdata, int32 data_size, int32 data_type) = 0;

  /** @brief 构造函数 */
  shmem_pool_recover() {}
  /** @brief 虚析构函数 */
  virtual ~shmem_pool_recover() {}
};

/**
 * @brief 共享内存区域信息结构体
 *
 * 描述一个共享内存区域的元数据信息，包括区域大小、块管理、
 * 空闲链表等信息。每个区域包含多个内存块用于数据存储。
 */
struct shmem_area_info {
  /** @brief 原子锁，仅第一个区域有效，用于区域间同步 */
  atomic_lock mlock;
  /** @brief 第一个空闲区域ID，仅第一个区域有效 */
  int16 first_free;
  /** @brief 下一个空闲区域ID */
  int16 next_free;
  /** @brief 当前空闲块ID */
  int32 free_block;
  /** @brief 当前空闲块数量 */
  int32 free_block_num;
  /** @brief 区域ID */
  int16 area_id;
  /** @brief 是否使用大页面，1表示使用 */
  int16 is_hugepage;
  /** @brief 块大小（字节） */
  int32 block_size;
  /** @brief 区域内总块数 */
  int32 all_block_num;
  /** @brief 总区域数量，仅第一个区域有效，最大32 */
  int32 area_num;

  /** @brief 区域状态，0表示未初始化，1表示已初始化 */
  int32 state;
  /** @brief 区域大小（字节） */
  int64 area_size;
  /** @brief 保留字段，用于未来扩展 */
  char reserved[16];
};
/**
 * @brief 共享内存块信息结构体
 *
 * 描述单个内存块的详细信息，包括数据存储、链表管理、
 * 使用状态等。每个块可以存储多个相同类型的数据项。
 */
struct shmem_block_info {
  /** @brief 下一个块地址（用于链表管理） */
  shm_addr_unit next_block;
  /** @brief 前一个块地址（用于双向链表） */
  shm_addr_unit prev_block;
  /** @brief 头部大小，保持数据从64字节对齐开始 */
  int32 head_size;
  /** @brief 单个数据大小 */
  int32 data_size;
  /** @brief 块ID */
  int32 block_id;
  /** @brief 所属区域ID */
  int16 area_id;
  /** @brief 块状态标志：0-idle,1-arealist,2-freelist,3-usedlist */
  int16 flag;
  /** @brief 块级原子锁，用于块内数据操作同步 */
  atomic_lock mlock;
  /** @brief 数据类型标识 */
  int32 data_type;
  /** @brief 第一个空闲数据位置 */
  int16 first_pos;
  /** @brief 块内数据总数 */
  int16 data_num;
  /** @brief 块大小 */
  int32 block_size;
  /** @brief 空闲数据位置数组，柔性数组成员 */
  int16 data_free_pos[0];
};

/**
 * @brief 共享内存块池管理器
 *
 * 基于共享内存的动态内存池，采用区域和分页管理：
 * - 区域：用来扩展共享内存
 * - 分页：每个区域分页管理，每个页存储一项或多项数据
 *
 * 内存申请按页分配，分配的页私有化，类似dync_pool的两级管理。
 * 每个页存储同一数据，不同页的数据可以不同。
 *
 * 限制：存储的数据必须为POD类型
 *
 * 仅单进程获取释放内存，其他进程仅依据地址读内存
 */
class shm_block_pool {
private:
  cmutex map_lock;
  shmem_area_info *area0;
  shmem_area_info *free_area;
  int32 block_size;
  int8 shm_exist;
  int8 is_write;
  int16 is_hugepage;
  std::vector<shmem_area_info *> area_mems;

  int64 area_size;
  char shm_name[256 - 32 - sizeof(std::vector<shmem_area_info *>) - sizeof(cmutex)];

public:
  FORCE_INLINE int32 get_block_size() const { return block_size; }
  /**
   * @brief 计算指定数据大小可容纳的数据数量
   *
   * @param[in] data_size 数据大小（字节）
   * @return 可容纳的数据数量
   */
  int16 calc_block_data_num(int32 data_size);

  /**
   * @brief 根据区域ID获取区域信息指针
   *
   * @param[in] area_id 区域ID
   * @return 区域信息指针，失败返回NULL
   */
  shmem_area_info *addr2area(int16 area_id);

  /**
   * @brief 根据地址获取块信息指针
   *
   * @param[in] addr 共享内存地址
   * @return 块信息指针，失败返回NULL
   */
  FORCE_INLINE shmem_block_info *addr2block(shm_data_addr &addr) {
    shmem_area_info *tpa = addr2area(addr.area_id);
    if (likely(NULL != tpa)) {
      char *tpb_first = ((char *)(tpa)) + sizeof(shmem_area_info);
      return (shmem_block_info *)(tpb_first + (addr.block_id * block_size));
    }
    return NULL;
  }

  /**
   * @brief 根据地址获取数据指针
   *
   * @param[in] addr 共享内存地址
   * @return 数据指针，失败返回NULL
   */
  FORCE_INLINE char *addr2data(shm_data_addr &addr) {
    shmem_area_info *tpa = addr2area(addr.area_id);
    if (likely(NULL != tpa)) {
      char *tpb_first = ((char *)(tpa)) + sizeof(shmem_area_info);
      shmem_block_info *tpb = (shmem_block_info *)(tpb_first + (addr.block_id * block_size));
      tpb_first = ((char *)(tpb)) + tpb->head_size;
      return tpb_first + addr.data_id * tpb->data_size;
    }
    return NULL;
  }
  /**
   * @brief 计算块中空闲数据数量
   *
   * @param[in] pblock 块信息引用
   * @return 空闲数据数量
   */
  STATIC_FORCE_INLINE int16 block_free_data_num(shmem_block_info &pblock) {
    return (pblock.data_num - pblock.first_pos);
  }

  /**
   * @brief 单线程获取页块
   *
   * @param[out] o_block 输出参数，返回获取到的块指针
   * @param[in] data_type 数据类型
   * @param[in] data_size 数据大小
   * @return 成功返回0，失败返回错误码
   */
  int32 get_block(shmem_block_info *&o_block, int32 data_type, int32 data_size);
  /**
   * @brief 单线程释放页块
   *
   * @param[in] pblock 要释放的块指针
   */
  void back_block(shmem_block_info *pblock);

  /**
   * @brief 单线程从块中获取数据
   *
   * @param[out] o_data 输出参数，返回数据指针
   * @param[in] pblock 块指针
   * @return 成功返回数据ID，失败返回LBERR_OBJ_IS_EMPTY
   */
  static inline int16 get_data(char *&o_data, shmem_block_info *pblock) {
    int16 rid = LBERR_OBJ_IS_EMPTY;
    if (likely(pblock->first_pos < pblock->data_num)) {
      rid = pblock->data_free_pos[pblock->first_pos];
      pblock->first_pos++;
      o_data = ((char *)(pblock)) + pblock->head_size + rid * pblock->data_size;
    }
    return rid;
  }

  /**
   * @brief 单线程释放数据回块中
   *
   * @param[in] pblock 块指针
   * @param[in] data_id 要释放的数据ID
   */
  static inline void back_data(shmem_block_info *pblock, int16 data_id) {
    if (likely(pblock->first_pos > 0)) {
      pblock->data_free_pos[pblock->first_pos] = data_id;
      pblock->first_pos--;
    }
  }

  /**
   * @brief 多线程获取页块
   *
   * @param[out] o_block 输出参数，返回获取到的块指针
   * @param[in] data_type 数据类型
   * @param[in] data_size 数据大小
   * @return 成功返回0，失败返回错误码
   */
  int32 take_block(shmem_block_info *&o_block, int32 data_type, int32 data_size);
  /**
   * @brief 多线程释放页块
   *
   * @param[in] pblock 要释放的块指针
   */
  void release_block(shmem_block_info *pblock);

  /**
   * @brief 多线程从块中获取数据
   */
  static inline int16 take_data(char *&o_data, shmem_block_info *pblock) {
    int16 rid = LBERR_OBJ_IS_EMPTY;
    pblock->mlock.lock();
    if (likely(pblock->first_pos < pblock->data_num)) {
      rid = pblock->data_free_pos[pblock->first_pos];
      pblock->first_pos++;
      o_data = ((char *)(pblock)) + pblock->head_size + rid * pblock->data_size;
    }
    pblock->mlock.unlock();
    return rid;
  }
  /**
   * @brief 多线程释放数据回块中
   */
  static inline void release_data(shmem_block_info *pblock, int16 data_id) {
    pblock->mlock.lock();
    if (likely(pblock->first_pos > 0)) {
      pblock->data_free_pos[pblock->first_pos] = data_id;
      pblock->first_pos--;
    }
    pblock->mlock.unlock();
  }

  bool is_shm_exist() { return shm_exist == 1; }
  /**
   * @brief 初始化共享内存池
   *
   * @param[in] prev_name 共享内存名称前缀
   * @param[in] tarea_size 区域大小
   * @param[in] tblock_size 块大小
   * @param[in] used_hugepage 是否使用大页面
   * @param[in] create_mem 是否创建内存
   * @return 成功返回0，失败返回错误码
   */
  int32 init(const char *prev_name, int64 tarea_size, int32 tblock_size, int16 used_hugepage, int16 create_mem);
  /**
   * @brief 恢复共享内存数据
   *
   * 读进程不需调用该函数，读进程应依据数据恢复，而不是内存
   *
   * @param[in] pfunc 恢复接口指针
   */
  void recover(shmem_pool_recover *pfunc);
  /**
   * @brief 关闭共享内存池
   */
  void close();
  /**
   * @brief 重建共享内存池
   *
   * @return 成功返回0，失败返回错误码
   */
  int32 rebuild();

  /**
   * @brief 构造函数
   */
  shm_block_pool() : area0(NULL), free_area(NULL) {}
  /**
   * @brief 析构函数
   */
  ~shm_block_pool() {}

private:
  /**
   * @brief 添加区域到空闲链表
   *
   * @param[in,out] pinfo 区域信息指针
   */
  FORCE_INLINE void add_area_list(shmem_area_info *pinfo) {
    pinfo->next_free = area0->first_free;
    area0->first_free = pinfo->area_id;
    free_area = pinfo;
  }
  /**
   * @brief 从空闲链表中弹出区域
   */
  FORCE_INLINE void pop_area_list() {
    free_area->free_block_num = 0;
    int16 tnext = free_area->next_free;
    area0->first_free = tnext;
    if (tnext != -1)
      free_area = area_mems[tnext];
    else
      free_area = NULL;
  }

  /**
   * @brief 从区域中获取一个空闲块
   *
   * @return 块指针，失败返回NULL
   */
  FORCE_INLINE shmem_block_info *take_area_block() {
    if (likely(free_area->free_block >= 0)) {
      char *tpb_first = ((char *)(free_area)) + sizeof(shmem_area_info);
      shmem_block_info *tpb = (shmem_block_info *)(tpb_first + (free_area->free_block * block_size));
      free_area->free_block = tpb->next_block.addr.block_id;
      free_area->free_block_num--;
      return tpb;
    }
    return NULL;
  }

  /**
   * @brief 计算块可容纳的数据数量和头部大小
   *
   * @param[out] o_head_size 输出参数，返回头部大小
   * @param[in] data_size 数据大小
   * @return 可容纳的数据数量
   */
  int16 calc_block_data(int32 &o_head_size, int32 data_size);

  /**
   * @brief 构建块数据结构
   *
   * @param[out] pb 块指针
   * @param[in] data_type 数据类型
   * @param[in] data_size 数据大小
   */
  void build_block_data(shmem_block_info *pb, int32 data_type, int32 data_size);

  /**
   * @brief 将块归还给区域
   *
   * @param[in,out] pinfo 区域信息指针
   * @param[in,out] tpb 要归还的块指针
   */
  FORCE_INLINE void back_area_block(shmem_area_info *pinfo, shmem_block_info *tpb) {
    int32 tnext = pinfo->free_block;
    if (likely(tnext >= 0)) {
      set_shm_unit(tpb->next_block, pinfo->area_id, -1, tnext);
      pinfo->free_block = tpb->block_id;
      pinfo->free_block_num++;
    } else {
      init_shm_unit(tpb->next_block);
      pinfo->free_block = tpb->block_id;
      pinfo->free_block_num = 1;
      pinfo->next_free = area0->first_free;
      area0->first_free = pinfo->area_id;
      free_area = pinfo;
    }
  }

  /**
   * @brief 计算区域可容纳的块数量
   *
   * @return 块数量
   */
  int32 calc_area_block_num();
  /**
   * @brief 初始化块信息
   *
   * @param[in] pnext 下一个块指针
   * @param[out] pb 当前块指针
   * @param[in] area_id 区域ID
   * @param[in] block_id 块ID
   */
  void init_block(shmem_block_info *pnext, shmem_block_info *pb, int16 area_id, int32 block_id);
  /**
   * @brief 初始化区域信息
   *
   * @param[out] pinfo 区域信息指针
   * @param[in] area_id 区域ID
   */
  void init_area(shmem_area_info *pinfo, int16 area_id);
  /**
   * @brief 恢复区域信息
   *
   * @param[in,out] pinfo 区域信息指针
   * @param[in] area_id 区域ID
   */
  void recove_area(shmem_area_info *pinfo, int16 area_id);
  /**
   * @brief 分配共享内存区域
   *
   * @param[out] o_area 输出参数，返回分配的区域指针
   * @param[in] area_id 区域ID
   * @return 成功返回共享内存存在状态，失败返回错误码
   */
  int32 alloc_area(shmem_area_info *&o_area, int16 area_id);
  /**
   * @brief 初始化内存
   *
   * @return 成功返回0，失败返回错误码
   */
  int32 init_mem();

  /**
   * @brief 恢复块数据
   *
   * @param[in] pfunc 恢复接口指针
   * @param[in] tpb 块指针
   */
  void recove_block_data(shmem_pool_recover *pfunc, shmem_block_info *tpb);
  /**
   * @brief 恢复数据
   *
   * @param[in] pfunc 恢复接口指针
   */
  void recove_data(shmem_pool_recover *pfunc);
};

/**
 * @brief 共享内存块一级信息结构体
 *
 * 用于管理特定数据类型的空闲块链表头部信息。
 */
struct alignas(8) shmem_block_c1_info {
  /** @brief 原子锁，用于保护空闲块链表 */
  atomic_lock mlock;
  /** @brief 数据类型标识 */
  int32 data_type;
  /** @brief 第一个空闲块地址 */
  shm_addr_unit first_addr;
};

/**
 * @brief 特定数据类型的共享内存句柄类
 *
 * 类似dync_pool_c1h，提供特定数据类型的私有缓存管理。
 * 支持单线程和多线程操作，维护空闲块链表以提高性能。
 *
 * @tparam D 数据类型
 */
template <typename D> class shmem_block_c1h {
protected:
  /** @brief 第一个空闲块指针 */
  shmem_block_info *first_free;
  /** @brief 共享内存池指针 */
  shm_block_pool *pool;
  /** @brief 池方法标志，0表示单线程，1表示多线程 */
  int32 pool_mth;
  /** @brief 数据类型 */
  int32 data_type;
  /** @brief 空闲块链表信息指针 */
  shmem_block_c1_info *free_list;

  /**
   * @brief 添加块到空闲链表
   *
   * @param[in,out] tpb 要添加的块指针
   */
  FORCE_INLINE void add_free(shmem_block_info *tpb) {
    if (NULL == first_free) {
      tpb->flag = 2;
      set_shm_unit(free_list->first_addr, tpb->area_id, -1, tpb->block_id);
    } else {
      tpb->flag = 2;
      set_shm_unit(tpb->next_block, free_list->first_addr);
      init_shm_unit(tpb->prev_block);
      set_shm_unit(free_list->first_addr, tpb->area_id, -1, tpb->block_id);
      set_shm_unit(first_free->prev_block, free_list->first_addr);
    }
    first_free = tpb;
  }
  /**
   * @brief 从空闲链表中弹出块
   */
  FORCE_INLINE void pop_free() {
    if (shm_addr_valid(first_free->next_block.addr)) {
      set_shm_unit(free_list->first_addr, first_free->next_block);
      first_free = pool->addr2block(first_free->next_block.addr);
      init_shm_unit(first_free->prev_block);
    } else {
      init_shm_unit(free_list->first_addr);
      first_free = NULL;
    }
  }
  /**
   * @brief 从空闲链表中删除块
   *
   * @param[in] tpb 要删除的块指针
   */
  FORCE_INLINE void del_free(shmem_block_info *tpb) {
    if (tpb == first_free) {
      pop_free();
    } else {
      shmem_block_info *tpb_prev = pool->addr2block(tpb->prev_block.addr);
      set_shm_unit(tpb_prev->next_block, tpb->next_block);
      if (shm_addr_valid(tpb->next_block.addr)) {
        shmem_block_info *tpb_next = pool->addr2block(tpb->next_block.addr);
        set_shm_unit(tpb_next->prev_block, tpb->prev_block);
      }
    }
  }
  /**
   * @brief 重建空闲块链表
   *
   * 重建双向链表，插入删除时，优先设置next，依据next来重建
   */
  void recove_list() {
    // 重建双向链表，插入删除时，优先设置next，依据next来重建
    shmem_block_info *tpb = NULL;
    shm_addr_unit tua;
    set_shm_unit(tua, free_list->first_addr);
    while (shm_addr_valid(tua.addr)) {
      tpb = pool->addr2block(tua.addr);
      int32 tfree_num = shm_block_pool::block_free_data_num(*tpb);
      if (tfree_num == tpb->data_num) {
        tpb->flag = 3;
        set_shm_unit(tua, tpb->next_block);
        continue;
      } else if (tfree_num == 0) {
        tpb->flag = 1;
        set_shm_unit(tua, tpb->next_block);
        continue;
      } else {
        tpb->flag = 2;
        set_shm_unit(free_list->first_addr, tua);
        first_free = tpb;
        break;
      }
    }

    if (NULL == first_free) {
      init_shm_unit(free_list->first_addr);
      return;
    }
    init_shm_unit(first_free->prev_block);

    shmem_block_info *tpb_prev = first_free;
    set_shm_unit(tua, first_free->next_block);
    while (shm_addr_valid(tua.addr)) {
      tpb = pool->addr2block(tua.addr);
      int32 tfree_num = shm_block_pool::block_free_data_num(*tpb);
      if (tfree_num == tpb->data_num) {
        tpb->flag = 3;
        set_shm_unit(tpb_prev->next_block, tpb->next_block);
      } else if (tfree_num == 0) {
        tpb->flag = 1;
        set_shm_unit(tpb_prev->next_block, tpb->next_block);
      } else {
        tpb->flag = 2;
        set_shm_unit(tpb->prev_block, tpb_prev->area_id, -1, tpb_prev->block_id);
        tpb_prev = tpb;
      }
      set_shm_unit(tua, tpb->next_block);
    }
    init_shm_unit(tpb_prev->next_block);
  }

public:
  /**
   * @brief 根据地址获取数据指针
   *
   * @param[in] addr 共享内存地址
   * @return 类型化的数据指针
   */
  FORCE_INLINE D *addr2data(shm_data_addr &addr) {
    char *tpm = pool->addr2data(addr);
    return static_cast<D *>((void *)tpm);
  }
  /**
   * @brief 单线程获取对象
   *
   * @param[out] o_data 输出参数，返回获取到的对象指针
   * @param[out] o_addr 输出参数，返回对象的共享内存地址
   * @return 成功返回0，失败返回错误码
   */
  int32 get(D *&o_data, shm_data_addr &o_addr) {
    char *tpd = NULL;
    int16 tdid = -1;

    while (NULL != first_free) {
      tdid = shm_block_pool::get_data(tpd, first_free);
      if (likely(tdid >= 0)) {
        o_data = static_cast<D *>((void *)tpd);
        o_addr.area_id = first_free->area_id;
        o_addr.data_id = tdid;
        o_addr.block_id = first_free->block_id;
        return 0;
      }
      first_free->first_pos = first_free->data_num;
      first_free->flag = 3;
      pop_free();
    }

    int32 ret = 0;
    shmem_block_info *tpb = NULL;
    if (pool_mth == 0)
      ret = pool->get_block(tpb, data_type, sizeof(D));
    else
      ret = pool->take_block(tpb, data_type, sizeof(D));
    if (unlikely(ret < 0)) {
      return ret;
    }

    add_free(tpb);
    tdid = shm_block_pool::get_data(tpd, tpb);
    // assert(tdid >= 0);
    o_data = static_cast<D *>((void *)tpd);
    o_addr.area_id = tpb->area_id;
    o_addr.data_id = tdid;
    o_addr.block_id = tpb->block_id;
    return 0;
  }
  /**
   * @brief 单线程释放对象
   *
   * @param[in] data_addr 要释放的对象地址
   */
  void back(shm_data_addr &data_addr) {
    shmem_block_info *tpb = pool->addr2block(data_addr);
    pool->back_data(tpb, data_addr.data_id);
    int32 tfree_num = shm_block_pool::block_free_data_num(*tpb);
    if (tfree_num == tpb->data_num) {
      del_free(tpb);
      if (pool_mth == 0)
        pool->back_block(tpb);
      else
        pool->release_block(tpb);
    } else if (tfree_num == 1) {
      add_free(tpb);
    }
  }
  /**
   * @brief 多线程获取对象
   *
   * @param[out] o_data 输出参数，返回获取到的对象指针
   * @param[out] o_addr 输出参数，返回对象的共享内存地址
   * @return 成功返回0，失败返回错误码
   */
  int32 take(D *&o_data, shm_data_addr &o_addr) {
    char *tpd = NULL;
    int16 tdid = -1;

    free_list->mlock.lock();
    while (NULL != first_free) {
      tdid = shm_block_pool::get_data(tpd, first_free);
      if (likely(tdid >= 0)) {
        o_data = static_cast<D *>((void *)tpd);
        o_addr.area_id = first_free->area_id;
        o_addr.data_id = tdid;
        o_addr.block_id = first_free->block_id;
        free_list->mlock.unlock();
        return 0;
      }
      first_free->first_pos = first_free->data_num;
      first_free->flag = 3;
      pop_free();
    }
    free_list->mlock.unlock();

    int32 ret = 0;
    shmem_block_info *tpb = NULL;
    ret = pool->take_block(tpb, data_type, sizeof(D));
    if (unlikely(ret < 0)) {
      return ret;
    }

    free_list->mlock.lock();
    add_free(tpb);
    tdid = shm_block_pool::get_data(tpd, tpb);
    // assert(tdid >= 0);
    o_data = static_cast<D *>((void *)tpd);
    o_addr.area_id = tpb->area_id;
    o_addr.data_id = tdid;
    o_addr.block_id = tpb->block_id;
    free_list->mlock.unlock();
    return 0;
  }
  /**
   * @brief 多线程释放对象
   *
   * @param[in] data_addr 要释放的对象地址
   */
  void release(shm_data_addr &data_addr) {
    shmem_block_info *tpb = pool->addr2block(data_addr);

    free_list->mlock.lock();
    pool->back_data(tpb, data_addr.data_id);
    int32 tfree_num = shm_block_pool::block_free_data_num(*tpb);
    if (tfree_num == tpb->data_num) {
      del_free(tpb);
      free_list->mlock.unlock();

      pool->release_block(tpb);
      return;
    } else if (tfree_num == 1) {
      add_free(tpb);
    }
    free_list->mlock.unlock();
  }

  /**
   * @brief 初始化共享内存句柄
   *
   * @param[in] p_pool 共享内存池指针
   * @param[in] is_pool_mth 池方法标志，0表示单线程，1表示多线程
   * @param[in] tdata_type 数据类型
   * @param[in] p_first_addr 空闲块链表信息指针
   */
  void init(shm_block_pool *p_pool, int32 is_pool_mth, int32 tdata_type, shmem_block_c1_info *p_first_addr) {
    first_free = NULL;
    pool = p_pool;
    pool_mth = is_pool_mth;
    data_type = tdata_type;
    free_list = p_first_addr;

    free_list->mlock.init();
    free_list->data_type = data_type;
    if (pool->is_shm_exist()) {
      init_shm_unit(free_list->first_addr);
    } else {
      free_list->mlock.lock();
      recove_list();
      free_list->mlock.unlock();
    }
  }

  /**
   * @brief 关闭共享内存句柄
   */
  void close() {
    if (NULL == free_list)
      return;
    free_list->mlock.lock();
    first_free = NULL;
    init_shm_unit(free_list->first_addr);
    free_list->mlock.unlock();
  }

  /**
   * @brief 构造函数
   */
  shmem_block_c1h() : first_free(NULL), free_list(NULL) {}
  /**
   * @brief 析构函数
   */
  ~shmem_block_c1h(){};
};

} // namespace lb_common
