#pragma once

/**
 * @file dync_pool.h
 * @brief 动态内存池模块
 *
 * 这是一个可动态扩展的对象内存池。
 *
 * 设计想法：
 *    将内存池分两级：
 *    子级：为N个对象的块缓存，用于私有，以减少锁开销。
 *          比如，委托缓存池，每个客户私有块缓存，该客户在唯一线程运行时，
 *          其优先从私有块缓存获取委托内存，并无需加锁。
 *          子级块私有时，若该块全部空闲，方会返还主级。
 *    主级：为块缓存链表，维护所有申请的块缓存链表和空闲的块缓存链表
 *
 * 限制：
 *    1. 一般用于全局性缓存池，而非临时
 *    2. 初始化尽量单线程
 *
 * @note 使用模板实现，N为期望取值：8*n - 2
 */

#include "comm_errno.h"
#include "comm_sys.h"
#include "mlock.h"
#include "mutils.h"

#include <assert.h>

namespace lb_common
{

  template <int8 N, class T>
  class dync_pool;

  template <int8 N, class T>
  class dync_pool_c1h;

  /**
   * @brief 动态内存池块类
   *
   * 每个块包含N个T类型对象，是动态内存池的基本管理单元。
   * 支持单线程操作，通过空闲位置数组管理对象分配。
   *
   * @tparam N 块中对象数量，期望取值：8*n - 2
   * @tparam T 对象类型
   */
  template <int8 N, class T>
  class dync_pool_block
  {
  protected:
    int8 first_free;  ///< 第一个空闲位置索引
    int8 num;         ///< 块中对象总数
    int8 free_pos[N]; ///< 空闲位置数组，存储可用对象位置

    T datas[N]; ///< 对象数据数组

    dync_pool_block *next;     ///< 下一空闲块指针
    dync_pool_block *prev;     ///< 前一空闲块指针
    dync_pool_block *all_next; ///< 所有块链表的下一节点指针
    dync_pool_block *all_prev; ///< 所有块链表的前一节点指针

    friend class dync_pool<N, T>;
    friend class dync_pool_c1h<N, T>;

  public:
    /**
     * @brief 根据位置获取对象指针
     * @param[in] pos 对象在块中的位置
     * @return 对象指针
     */
    FORCE_INLINE T *pos2buf(int8 pos)
    {
      assert(pos >= 0 && pos < N);
      return &(datas[pos]);
    }
    /**
     * @brief 单线程获取对象
     * @param[out] o_buf 输出参数，返回获取到的对象指针
     * @return 对象位置，失败返回LBERR_OBJ_IS_EMPTY
     */
    FORCE_INLINE int8 get(T *&o_buf)
    {
      if (likely(first_free < N))
      {
        int8 tpos = free_pos[first_free];
        first_free++;
        o_buf = &(datas[tpos]);
        return tpos;
      }
      return (int8)LBERR_OBJ_IS_EMPTY;
    }
    /**
     * @brief 单线程释放对象
     * @param[in] pos 输入参数，上次获取的位置
     */
    FORCE_INLINE void back(int8 pos)
    {
      assert(pos >= 0 && pos < N);
      if (likely(first_free > 0))
      {
        first_free--;
        free_pos[first_free] = pos;
      }
    }

    /** @brief 获取空闲对象数量 */
    FORCE_INLINE int8 free_num() const { return N - first_free; }
    /** @brief 获取已使用对象数量 */
    FORCE_INLINE int8 used_num() const { return first_free; }

    /**
     * @brief 初始化块
     * 重置所有状态，设置空闲位置数组
     */
    FORCE_INLINE void init()
    {
      first_free = 0;
      num = N;
      for (int8 i = 0; i < N; ++i)
      {
        free_pos[i] = i;
      }

      next = NULL;
      prev = NULL;
      all_next = NULL;
      all_prev = NULL;
    }
    /**
     * @brief 重置块状态
     * 仅重置使用状态，保持链表关系
     */
    FORCE_INLINE void reset()
    {
      first_free = 0;
      num = N;
      for (int8 i = 0; i < N; ++i)
      {
        free_pos[i] = i;
      }

      next = NULL;
      prev = NULL;
    }
  };

  /**
   * @brief 动态内存池主类
   *
   * 可动态扩展的对象内存池，采用两级管理架构：
   * - 主级：管理所有块缓存链表和空闲块缓存链表
   * - 子级：每个线程的私有块缓存，减少锁开销
   *
   * 支持单线程和多线程操作，可根据配置保持或释放内存。
   *
   * @tparam N 块中对象数量，期望取值：8*n - 2
   * @tparam T 对象类型
   */
  template <int8 N, class T>
  class dync_pool
  {
  private:
    using m_it = dync_pool_block<N, T>;

    atomic_lock mlock; ///< 原子锁，用于多线程同步
    m_it *first;       ///< 空闲块链表头指针
    int32 free_num;    ///< 空闲块数量
    int32 init_num;    ///< 初始块数量
    int16 expand_num;  ///< 扩展时的块数量
    int16 keep_mem;    ///< 是否保持内存标志
    int32 all_num;     ///< 所有块的总数量
    m_it *all_list;    ///< 所有块链表头指针

    /**
     * @brief 分配多个内存块
     * @param[out] o_last 输出参数，返回最后一个分配的块
     * @param[in] io_num 输入输出参数，输入要分配的数量，输出实际分配的数量
     * @return 第一个分配的块指针，失败返回NULL
     */
    m_it *alloc_blk(m_it *&o_last, int32 &io_num)
    {
      // 导致 init(0,0,0) 内部 state 不一致（all_num=0 但实际有 1 块）+ get 后 free_num 下溢。
      // 改为入口检查：io_num <= 0 直接返回 NULL，不分配。
      if (unlikely(io_num <= 0))
      {
        o_last = NULL;
        return NULL;
      }
      // 上层调用方仍按「成功」处理，把已分配块当作整组推进 free_pos，
      // 并把 o_last->next 指向 first，可能造成链表中只剩部分块。
      // 现在失败时回收所有已分配块并返回 NULL。
      size_t ts = sizeof(m_it);
      void *tp = comm_utils::aligned_malloc(ts, SIMD_ALIGN_SIZE);
      if (NULL == tp)
      {
        io_num = 0;
        return NULL;
      }

      m_it *pi = new (tp) m_it;
      pi->init();
      m_it *tret = pi;
      m_it *tprev = pi;
      int32 i = 1;
      int32 tnum = io_num;
      while (i < tnum)
      {
        tp = comm_utils::aligned_malloc(ts, SIMD_ALIGN_SIZE);
        if (NULL == tp)
        {
          // 部分分配失败：回滚所有已分配块。
          io_num = 0;
          o_last = NULL;
          m_it *tcur = tret;
          while (tcur != NULL)
          {
            m_it *tnext = tcur->all_next;
            tcur->~m_it();
            comm_utils::aligned_free(static_cast<void *>(tcur));
            tcur = tnext;
          }
          return NULL;
        }
        pi = new (tp) m_it;
        pi->init();
        pi->prev = tprev;
        // get/take 用 first->next 取下一块，导致除首块外其他块不可达。
        // 这里同时设 tprev->next = pi 链接空闲链表。
        pi->next = NULL;
        tprev->next = pi;
        pi->all_prev = tprev;
        tprev->all_next = pi;
        tprev = pi;
        i++;
      }
      o_last = pi;
      io_num = tnum;
      return tret;
    }
    /**
     * @brief 释放单个内存块
     * @param[in] pblk 要释放的块指针
     */
    static void free_blk(m_it *pblk) { comm_utils::aligned_free(static_cast<void *>(pblk)); }

  public:
    /**
     * @brief 单线程获取内存块
     * @param[out] o_block 输出参数，返回获取到的块指针
     * @return 0表示成功，负值表示失败
     */
    int32 get(dync_pool_block<N, T> *&o_block)
    {
      m_it *tp = first;
      if (likely(NULL != tp))
      {
        // CACHE_PREFETCH_L2(reinterpret_cast<void *>(tp),1);

        first = tp->next;
        tp->next = NULL;
        free_num--;
      }
      else
      {
        int32 new_num = expand_num;
        m_it *tn_last = NULL;
        tp = alloc_blk(tn_last, new_num);
        if (NULL != tp)
        {
          // CACHE_PREFETCH_L2(reinterpret_cast<void *>(tp),1);

          tn_last->next = first;
          tn_last->all_next = all_list;
          first = tp->next;
          tp->next = NULL;
          free_num = new_num - 1;
          all_num += new_num;
          if (NULL != all_list)
          {
            all_list->all_prev = tn_last;
          }
          all_list = tp;
        }
        else
        {
          return LBERR_MEM_ALLOC_FAIL;
        }
      }
      o_block = tp;
      return 0;
    }
    /**
     * @brief 单线程释放内存块
     * @param[in] get_block 要释放的块指针
     */
    void back(dync_pool_block<N, T> *get_block)
    {
      assert(NULL != get_block);
      if (free_num < init_num || keep_mem == 1)
      {
        get_block->reset();
        get_block->next = first;
        get_block->prev = NULL;
        first = get_block;
        free_num++;
      }
      else
      {
        m_it *tprev = get_block->all_prev;
        m_it *tnext = get_block->all_next;
        if (NULL != tnext)
          tnext->all_prev = tprev;
        if (NULL != tprev)
        {
          tprev->all_next = tnext;
        }
        else
        {
          all_list = tnext;
        }
        all_num--;
        free_blk(get_block);
      }
    }

    /**
     * @brief 多线程获取内存块
     * @param[out] o_block 输出参数，返回获取到的块指针
     * @return 0表示成功，负值表示失败
     */
    int32 take(dync_pool_block<N, T> *&o_block)
    {
      mlock.lock();
      m_it *tp = first;
      if (likely(NULL != tp))
      {
        // CACHE_PREFETCH_L2(reinterpret_cast<void *>(tp),1);

        first = tp->next;
        tp->next = NULL;
        free_num--;
      }
      else
      {
        // 原因：解锁去分配期间，其他线程可能通过 release() 把 block 还回 free 链表，
        // free_num 会增加。回退路径 free_num-- 应基于当前真实值（包含其他线程的贡献），
        // 否则会下溢成负数，让 get_used() 算出错误的"已用数"。
        // 正确语义：first == NULL 本身已足够判断空闲链表为空，无需修改 free_num。
        mlock.unlock();

        int32 new_num = expand_num;
        m_it *tn_last = NULL;
        tp = alloc_blk(tn_last, new_num);

        mlock.lock();

        if (NULL != tp)
        {
          // CACHE_PREFETCH_L2(reinterpret_cast<void *>(tp),1);

          tn_last->next = first;
          tn_last->all_next = all_list;
          first = tp->next;
          tp->next = NULL;
          free_num += new_num - 1;
          all_num += new_num;
          if (NULL != all_list)
          {
            all_list->all_prev = tn_last;
          }
          all_list = tp;
        }
        else
        {
          // 分配失败回退路径：从 free 链表取一个（其他线程可能在我们 unlock 期间已还 block）
          tp = first;
          if (NULL != tp)
          {
            first = tp->next;
            tp->next = NULL;
            // free_num 直接基于当前真实值递减（已不再被人为置 0）
            free_num--;
          }
          else
          {
            // 避免调用方误用未初始化的 o_block 值
            o_block = NULL;
            mlock.unlock();
            return LBERR_MEM_ALLOC_FAIL;
          }
        }
      }
      o_block = tp;
      mlock.unlock();
      return 0;
    }
    /**
     * @brief 多线程释放内存块
     * @param[in] get_block 要释放的块指针
     */
    void release(dync_pool_block<N, T> *get_block)
    {
      int32 tctl = 0;
      mlock.lock();
      if (free_num < init_num || keep_mem == 1)
      {
        get_block->reset();
        get_block->next = first;
        get_block->prev = NULL;
        first = get_block;
        free_num++;
      }
      else
      {
        get_block->prev = NULL;
        m_it *tprev = get_block->all_prev;
        m_it *tnext = get_block->all_next;
        if (NULL != tnext)
          tnext->all_prev = tprev;
        if (NULL != tprev)
        {
          tprev->all_next = tnext;
        }
        else
        {
          all_list = tnext;
        }
        all_num--;
        tctl = 1;
      }
      mlock.unlock();

      if (tctl == 1)
      {
        free_blk(get_block);
      }
    }

    /** @brief 获取空闲块数量 */
    FORCE_INLINE int32 get_free() const { return free_num; }
    /** @brief 获取已使用块数量 */
    FORCE_INLINE int32 get_used() const { return all_num - free_num; }
    /** @brief 获取总块数量 */
    FORCE_INLINE int32 size() const { return all_num; }

    /**
     * @brief 初始化动态内存池
     * @param[in] init_block_num 初始块数量
     * @param[in] is_keep_mem 是否保持内存标志，1表示保持，0表示释放
     * @param[in] expand_block_num 扩展时的块数量，默认为4
     * @return 0表示成功（包括重复调用静默成功），负值表示失败
     */
    int32 init(int32 init_block_num, int16 is_keep_mem, int16 expand_block_num = 4)
    {
      mlock.lock();
      if (all_num > 0)
      {
        // 已初始化：与 hash_map_mth::init 一致，静默返回 0
        mlock.unlock();
        return 0;
      }

      int32 new_num = init_block_num;
      m_it *tn_last = NULL;
      m_it *tp = alloc_blk(tn_last, new_num);
      if (NULL == tp)
      {
        mlock.unlock();
        // 区分：init_block_num==0 是参数错，其他才是 alloc 失败。
        return (init_block_num <= 0) ? LBERR_ARGV_WRONG : LBERR_MEM_ALLOC_FAIL;
      }

      tn_last->next = NULL;
      tn_last->all_next = NULL;
      tp->all_prev = NULL;

      first = tp;
      free_num = new_num;
      init_num = init_block_num;
      expand_num = expand_block_num;
      keep_mem = is_keep_mem;
      all_num = new_num;
      all_list = tp;

      mlock.unlock();
      return 0;
    }
    /**
     * @brief 关闭动态内存池
     * 释放所有分配的内存块
     */
    void close()
    {
      mlock.lock();
      m_it *tp = all_list;
      // 已归还，tp->all_next 是 use-after-free。这里先缓存 next 再释放。
      while (NULL != tp)
      {
        m_it *tnext = tp->all_next;
        free_blk(tp);
        tp = tnext;
      }

      first = NULL;
      free_num = 0;
      all_num = 0;
      all_list = NULL;
      mlock.unlock();
    }
    /**
     * @brief 重置动态内存池状态
     * 清理所有状态信息
     */
    void reset()
    {
      mlock.init();
      first = NULL;
      free_num = 0;
      all_num = 0;
      all_list = NULL;
    }

    /**
     * @brief 构造函数
     */
    dync_pool() { reset(); }
    /**
     * @brief 析构函数
     */
    ~dync_pool() { close(); }

    dync_pool(const dync_pool &) = delete;
    dync_pool &operator=(const dync_pool &) = delete;
    dync_pool(dync_pool &) = delete;
    dync_pool &operator=(dync_pool &) = delete;
  };

  /**
   * @brief 私有空闲块句柄类
   *
   * 每个线程的私有块缓存，用于减少锁开销。
   * 当线程在唯一线程运行时，优先从私有块缓存获取内存，无需加锁。
   * 当块全部空闲时，会返还主级内存池。
   *
   * @tparam N 块中对象数量，期望取值：8*n - 2
   * @tparam T 对象类型
   */
  template <int8 N, class T>
  class dync_pool_c1h
  {
  private:
    using m_blk = dync_pool_block<N, T>;
    using m_pool = dync_pool<N, T>;

    m_blk *head;       ///< 空闲块链表头指针
    atomic_lock mlock; ///< 原子锁，用于多线程同步
    int32 pool_mth;    ///< 池方法标志，0=单线程，1=多线程
    m_pool *pool;      ///< 关联的动态内存池指针

    /**
     * @brief 根据数据指针和位置计算块指针
     * @param[in] pd 数据指针
     * @param[in] pos 数据在块中的位置
     * @return 对应的块指针
     */
    STATIC_FORCE_INLINE dync_pool_block<N, T> *pos2blk(T *pd, int8 pos)
    {
      // 使用offsetof计算datas的偏移量（编译期计算）
      constexpr size_t toff = offsetof(m_blk, datas);
      char *datas_addr = reinterpret_cast<char *>(pd - pos);
      return reinterpret_cast<dync_pool_block<N, T> *>(datas_addr - toff);
    }

  public:
    /**
     * @brief 单线程获取对象
     * @param[out] o_buf 输出参数，返回获取到的对象指针
     * @return 对象位置，失败返回LBERR_MEM_ALLOC_FAIL
     */
    int8 get(T *&o_buf)
    {
      int8 rpos = 0;
      while (likely(NULL != head))
      {
        rpos = head->get(o_buf);
        if (likely(rpos >= 0))
        {
          // P0-4: 块被取走一个对象后若变满（free_num==0），需从链表脱离以避免
          // c1h::back tn==1 分支的 else 把"刚变满的 head 块"重新链入而自环。
          // 简化：仅移 head 指针 + 清新 head 的 prev，不再显式清空旧 head 块的 next/prev。
          // 隐含约定：c1h::back 的 tn==1 分支第一行 tl.next = head 会覆盖
          // 旧 head 块的 next，所以脏 next 不会残留。若修改 c1h::back 路径
          // （如增加"块已在链表中则跳过"判断），必须重新审视此处。
          if (head->free_num() == 0)
          {
            head = head->next;
            if (NULL != head)
              head->prev = NULL;
          }
          return rpos;
        }
        // head 块已满（get 返回 -1），同样仅移 head 不清旧块（理由同上）。
        head = head->next;
        if (NULL != head)
          head->prev = NULL;
      }

      int32 ret = 0;
      m_blk *tb = NULL;
      if (pool_mth == 0)
        ret = pool->get(tb);
      else
        ret = pool->take(tb);

      if (ret == 0)
      {
        tb->next = NULL;
        tb->prev = NULL;
        head = tb;
        rpos = tb->get(o_buf);
        // assert(rpos >= 0);
        return rpos;
      }
      return (int8)(LBERR_MEM_ALLOC_FAIL);
    }
    /**
     * @brief 单线程释放对象
     * @param[in] get_buf 要释放的对象指针
     * @param[in] pos 对象在块中的位置
     * @note 业务契约：调用方须保证同一个 get_buf/pos 仅 back 一次。重复 back
     *       会让 dync_pool_block::back 的 free_pos 中出现重复 pos，但库不检测；
     *       在 tn==N 路径，块已归还主池后再 back 会因 tl->prev=NULL 而在
     *       else 分支解引用 NULL（崩溃路径仅当 P0-4 修复被回滚时才可达，
     *       当前实现已保证满块物理脱离，重复 back 仍属于业务误用）。
     */
    void back(T *get_buf, int8 pos)
    {
      m_blk *tl = pos2blk(get_buf, pos);
      tl->back(pos);

      int8 tn = tl->free_num();
      if (tn == 1)
      {
        if (NULL == head)
        {
          tl->next = NULL;
          tl->prev = NULL;
          head = tl;
        }
        else
        {
          tl->next = head;
          tl->prev = NULL;
          head->prev = tl;
          head = tl;
        }
      }
      else if (tn == N)
      {
        if (tl == head)
        {
          head = tl->next;
          if (NULL != head)
            head->prev = NULL;
        }
        else
        {
          tl->prev->next = tl->next;
          if (NULL != tl->next)
          {
            tl->next->prev = tl->prev;
          }
        }
        tl->next = NULL;
        tl->prev = NULL;

        if (pool_mth == 0)
          pool->back(tl);
        else
          pool->release(tl);
      }
    }
    /**
     * @brief 多线程获取对象
     * @param[out] o_buf 输出参数，返回获取到的对象指针
     * @return 对象位置，失败返回LBERR_MEM_ALLOC_FAIL
     */
    int8 take(T *&o_buf)
    {
      int8 rpos = 0;
      mlock.lock();
      while (likely(NULL != head))
      {
        rpos = head->get(o_buf);
        if (likely(rpos >= 0))
        {
          // P0-4: 同 get()，块变满立即脱离 head。仅移 head + 清新 head 的 prev，
          // 不显式清空旧 head 块的 next/prev。隐含约定：c1h::release 的
          // tn==1 分支第一行 tl.next = head 会覆盖旧 head 块的 next，
          // 所以脏 next 不会残留。若修改 c1h::release 路径必须重新审视此处。
          if (head->free_num() == 0)
          {
            head = head->next;
            if (NULL != head)
              head->prev = NULL;
          }
          mlock.unlock();
          return rpos;
        }
        // head 块已满（get 返回 -1），同样仅移 head 不清旧块（理由同上）。
        head = head->next;
        if (NULL != head)
          head->prev = NULL;
      }
      mlock.unlock();

      int32 ret = 0;
      m_blk *tb = NULL;
      if (pool_mth == 0)
        ret = pool->get(tb);
      else
        ret = pool->take(tb);

      if (ret == 0)
      {
        mlock.lock();
        // P1-7: 不能直接 head = tb 覆盖——解锁去主池取块期间，其他线程可能通过
        // release() 把块还回 free 链表（pool->take 内部也会新增 free 链表条目），
        // 直接覆盖会丢失这些块，导致归还的块既不在 head 链表也不在主池 free 链表中。
        // 正确做法是按"原 head 前插入新块"的方式链接，保留其他线程的贡献。
        tb->next = head;
        tb->prev = NULL;
        if (NULL != head)
          head->prev = tb;
        head = tb;
        rpos = tb->get(o_buf);
        // assert(rpos >= 0);
        mlock.unlock();
        return rpos;
      }
      return (int8)(LBERR_MEM_ALLOC_FAIL);
    }
    /**
     * @brief 多线程释放对象
     * @param[in] get_buf 要释放的对象指针
     * @param[in] pos 对象在块中的位置
     * @note 业务契约：调用方须保证同一个 get_buf/pos 仅 release 一次。重复 release
     *       与单线程 back 同问题：在 tn==N 路径块已归还主池后再 release 会触发
     *       else 分支 NULL 解引用。P0-4 修复后此路径已被满块物理脱离避免重复触发，
     *       但重复释放仍是业务误用，库不强制检测。
     */
    void release(T *get_buf, int8 pos)
    {
      m_blk *tl = pos2blk(get_buf, pos);

      mlock.lock();
      tl->back(pos);

      int8 tn = tl->free_num();
      if (tn == 1)
      {
        if (NULL == head)
        {
          tl->next = NULL;
          tl->prev = NULL;
          head = tl;
        }
        else
        {
          tl->next = head;
          tl->prev = NULL;
          head->prev = tl;
          head = tl;
        }
      }
      else if (tn == N)
      {
        if (tl == head)
        {
          head = tl->next;
          if (NULL != head)
            head->prev = NULL;
        }
        else
        {
          tl->prev->next = tl->next;
          if (NULL != tl->next)
          {
            tl->next->prev = tl->prev;
          }
        }
        tl->next = NULL;
        tl->prev = NULL;
        mlock.unlock();

        if (pool_mth == 0)
          pool->back(tl);
        else
          pool->release(tl);
        return;
      }

      mlock.unlock();
    }
    /**
     * @brief 初始化私有空闲块句柄
     * @param[in] p_pool 关联的动态内存池指针
     * @param[in] pool_is_mth 池方法标志，0表示单线程，1表示多线程
     */
    void init(dync_pool<N, T> *p_pool, int32 pool_is_mth)
    {
      head = NULL;
      mlock.init();
      pool_mth = pool_is_mth;
      pool = p_pool;
    }

    /**
     * @brief 关闭私有空闲块句柄
     * 清理状态信息
     */
    void close()
    {
      mlock.lock();
      head = NULL;
      mlock.unlock();
    }

    /**
     * @brief 默认构造函数
     */
    dync_pool_c1h() : head(NULL), pool_mth(1), pool(NULL) {};
    /**
     * @brief 带参数的构造函数
     * @param[in] p_pool 关联的动态内存池指针
     * @param[in] pool_is_mth 池方法标志，0表示单线程，1表示多线程
     */
    dync_pool_c1h(dync_pool<N, T> *p_pool, int32 pool_is_mth) : head(NULL), pool_mth(pool_is_mth), pool(p_pool) {};
    /**
     * @brief 析构函数
     */
    ~dync_pool_c1h() {};
  };

} // namespace lb_common
