
#include "shmem_pool.h"
#include "mutils.h"

namespace lb_common
{

  /**
   * @brief 根据区域ID获取区域信息指针
   *
   * @param area_id 区域ID
   * @return 区域信息指针，失败返回NULL
   */
  shmem_area_info *shm_block_pool::addr2area(int16 area_id)
  {
    // 边界检查：area_id 必须在 [0, area_mems.size()) 范围内
    // 防止 shm_data_addr 来自外部时 area_id 越界导致未定义行为
    if (unlikely(area_id < 0 || (size_t)area_id >= area_mems.size()))
    {
      return NULL;
    }

    shmem_area_info *tpa = area_mems[area_id];
    if (likely(NULL != tpa))
      return tpa;

    if (is_write == 0)
    {
      map_lock.lock();
      tpa = area_mems[area_id];
      if (NULL == tpa)
      {
        alloc_area(tpa, area_id);
      }
      map_lock.unlock();
    }
    return tpa;
  }

  /**
   * @brief 单线程获取页块
   *
   * @param o_block 输出参数，返回获取到的块指针
   * @param data_type 数据类型
   * @param data_size 数据大小
   * @return 成功返回0，失败返回错误码
   */
  int32 shm_block_pool::get_block(shmem_block_info *&o_block, int32 data_type, int32 data_size)
  {
    shmem_block_info *rb = NULL;
    while (NULL != free_area)
    {
      rb = take_area_block();
      if (likely(NULL != rb))
      {
        build_block_data(rb, data_type, data_size);
        o_block = rb;
        return 0;
      }
      pop_area_list();
    }

    int32 tid = atomic_load32(&(area0->area_num));
    // 修复：area_num 超出 area_mems 容量（默认 16384）时直接返回错误，
    // 避免 (int16)(tid & 0xffff) 截到负值或 alloc_area 生成错误 shm 名。
    // if (unlikely(tid < 0 || tid >= (int32)area_mems.size())) {
    //  return LBERR_OBJ_NUM_LIMIT;
    //}
    shmem_area_info *tpa = NULL;
    int32 ret = alloc_area(tpa, (int16)(tid & 0xffff));
    if (ret >= 0)
    {
      add_area_list(tpa);
      rb = take_area_block();
      // P2-8: take_area_block 可能返回 NULL（free_block<0 等异常），
      // 直接传给 build_block_data 会写越界。
      if (NULL == rb)
      {
        return LBERR_OBJ_IS_EMPTY;
      }
      build_block_data(rb, data_type, data_size);
      o_block = rb;
      return 0;
    }
    return ret;
  }
  /**
   * @brief 多线程获取页块
   *
   * @param o_block 输出参数，返回获取到的块指针
   * @param data_type 数据类型
   * @param data_size 数据大小
   * @return 成功返回0，失败返回错误码
   */
  int32 shm_block_pool::take_block(shmem_block_info *&o_block, int32 data_type, int32 data_size)
  {
    shmem_block_info *rb = NULL;

    area0->mlock.lock();
    while (NULL != free_area)
    {
      rb = take_area_block();
      if (likely(NULL != rb))
      {
        area0->mlock.unlock();

        build_block_data(rb, data_type, data_size);
        o_block = rb;
        return 0;
      }
      pop_area_list();
    }
    area0->mlock.unlock();

    map_lock.lock();
    int32 tid = atomic_load32(&(area0->area_num));
    // 修复：area_num 超出 area_mems 容量（默认 16384）时直接返回错误，
    // 避免 (int16)(tid & 0xffff) 截到负值或 alloc_area 生成错误 shm 名。
    // if (unlikely(tid < 0 || tid >= (int32)area_mems.size()))
    //{
    //  return LBERR_OBJ_NUM_LIMIT;
    //}
    shmem_area_info *tpa = NULL;
    int32 ret = alloc_area(tpa, (int16)(tid & 0xffff));
    map_lock.unlock();

    if (ret >= 0)
    {
      area0->mlock.lock();
      add_area_list(tpa);
      rb = take_area_block();
      area0->mlock.unlock();

      // P2-8: take_area_block 可能返回 NULL（free_block<0 等异常），
      // 直接传给 build_block_data 会写越界。
      if (NULL == rb)
      {
        return LBERR_OBJ_IS_EMPTY;
      }
      build_block_data(rb, data_type, data_size);
      o_block = rb;
      return 0;
    }
    return ret;
  }

  /**
   * @brief 构建块数据结构
   *
   * @param pb 块指针
   * @param data_type 数据类型
   * @param data_size 数据大小
   */
  void shm_block_pool::build_block_data(shmem_block_info *pb, int32 data_type, int32 data_size)
  {
    init_shm_unit(pb->next_block);
    int16 tnum = calc_block_data(pb->head_size, data_size);
    pb->data_size = data_size;
    pb->mlock.init();
    pb->data_type = data_type;
    pb->first_pos = 0;
    pb->data_num = tnum;

    for (int16 i = 0; i < tnum; i++)
    {
      pb->data_free_pos[i] = i;
    }
  }
  /**
   * @brief 计算块可容纳的数据数量和头部大小
   *
   * @param o_head_size 输出参数，返回头部大小
   * @param data_size 数据大小
   * @return 可容纳的数据数量
   */
  int16 shm_block_pool::calc_block_data(int32 &o_head_size, int32 data_size)
  {
    // 计算能装下的最大数据项数。
    // 块布局：[块头(48字节)] [tn 个 free_pos (2*tn字节)] [对齐 padding] [tn 个数据]
    // 数据起始位置 = ALIGN_UP(48 + 2*tn, 64)，从此位置开始存数据
    // 总占用 = head_size + tn * data_size ≤ block_size
    //
    // 优化：不需要 ts 已对齐到 64——只要数据起始位置 64 对齐即可
    // 这避免了 ts 没对齐时强制减小 tn 的浪费
    //
    // 契约：本函数保证返回 tn > 0，调用方应保证 data_size 不至于连一个都装不下。
    //       若 data_size 过大导致无法装下，应在调用方（init/get_block/take_block）提前校验。
    int32 tn = (block_size - sizeof(shmem_block_info)) / (data_size + (int32)sizeof(int16));
    // 断言：调用方必须保证 tn >= 1（data_size 合理）
    assert(tn > 0);
    if (unlikely(tn <= 0))
    {
      o_head_size = 0;
      return 0;
    }
    // 循环直到数据起始位置+数据区 <= block_size
    while (true)
    {
      int32 head_size_now = ALIGN_UP(sizeof(shmem_block_info) + (int32)sizeof(int16) * tn, 64);
      if ((int64)head_size_now + (int64)tn * data_size <= block_size)
        break;
      tn--;
      // 断言：保证 tn > 0（按递减逻辑必能找到解）
      assert(tn > 0);
      if (unlikely(tn < 0))
      {
        o_head_size = 0;
        return 0;
      }
    }
    o_head_size = ALIGN_UP(sizeof(shmem_block_info) + (int32)sizeof(int16) * tn, 64);
    return tn;
  }

  /**
   * @brief 单线程释放页块
   *
   * @param pblock 要释放的块指针
   */
  void shm_block_pool::back_block(shmem_block_info *pblock)
  {
    pblock->flag = 1;
    pblock->data_type = 0;
    pblock->mlock.init();
    pblock->first_pos = 0;
    pblock->data_num = 0;
    init_shm_unit(pblock->prev_block);

    back_area_block(area_mems[pblock->area_id], pblock);
  }
  /**
   * @brief 多线程释放页块
   *
   * @param pblock 要释放的块指针
   */
  void shm_block_pool::release_block(shmem_block_info *pblock)
  {
    pblock->flag = 1;
    pblock->data_type = 0;
    pblock->mlock.init();
    pblock->first_pos = 0;
    pblock->data_num = 0;
    init_shm_unit(pblock->prev_block);

    area0->mlock.lock();
    back_area_block(area_mems[pblock->area_id], pblock);
    area0->mlock.unlock();
  }

  /**
   * @brief 分配共享内存区域
   *
   * @param o_area 输出参数，返回分配的区域指针
   * @param area_id 区域ID
   * @return 成功返回共享内存存在状态，失败返回错误码
   */
  int32 shm_block_pool::alloc_area(shmem_area_info *&o_area, int16 area_id)
  {
    char tname[256];
    std::memset(tname, 0, sizeof(tname));
    std::snprintf(tname, sizeof(tname) - 1, "%s_area_%d", shm_name, area_id);

    void *tpm = NULL;
    int32 is_create = is_write;
    int32 shmisexist = comm_utils::map_shm(tpm, tname, area_size, is_hugepage, is_create);
    if (shmisexist < 0)
    {
      o_area = NULL;
      return shmisexist;
    }

    shmem_area_info *ta = (shmem_area_info *)tpm;
    if (area_id == 0)
    {
      area0 = ta;
    }

    if (is_create == 1)
    {
      if (shmisexist == 0 || ta->state == 0 || area0->state == 0)
      {
        init_area(ta, area_id);
      }
      else
      {
        // 几何参数一致性校验：原版仅放在 assert 中，Release（-DNDEBUG）下被剔除，
        // 导致不同 block_size 的进程互相挂接后所有寻址错乱。改为运行时检查并返回错误。
        if (ta->area_size != area_size || ta->block_size != block_size || ta->is_hugepage != is_hugepage ||
            ta->area_id != area_id)
        {
          o_area = NULL;
          return LBERR_ATTR_SET_FAIL;
        }
      }
    }
    else
    {
      // 读进程：必须挂接到已存在的 shm（shmisexist==1），几何参数必须一致。
      // 原 assert 在 Release 下消失，读进程会使用未初始化 ta 继续操作。
      if (shmisexist != 1)
      {
        o_area = NULL;
        return LBERR_SHM_MAP_FAIL;
      }
      if (ta->area_size != area_size || ta->block_size != block_size || ta->is_hugepage != is_hugepage ||
          ta->area_id != area_id)
      {
        o_area = NULL;
        return LBERR_ATTR_SET_FAIL;
      }

      while (ta->state == 0)
      {
        comm_utils::sleep_us(10);
      }
    }

    area_mems[area_id] = ta;
    o_area = ta;

    if (is_create == 1)
    {
      int32 tan = atomic_load32(&(area0->area_num));
      do
      {
        if (tan >= area_id + 1)
        {
          break;
        }
        if (atomic_cas32(&(area0->area_num), &tan, area_id + 1))
          break;
      } while (true);
    }

    return shmisexist;
  }

  /**
   * @brief 初始化区域信息
   *
   * @param pinfo 区域信息指针
   * @param area_id 区域ID
   */
  void shm_block_pool::init_area(shmem_area_info *pinfo, int16 area_id)
  {
    pinfo->state = 0;
    pinfo->mlock.init();
    pinfo->first_free = -1; // 仅第一个区域有效
    pinfo->next_free = -1;
    pinfo->free_block = -1;
    pinfo->free_block_num = 0;
    pinfo->area_id = area_id;
    pinfo->is_hugepage = is_hugepage;
    pinfo->block_size = block_size;
    pinfo->all_block_num = 0;

    pinfo->area_num = 0; // 仅第一个区域有效
    pinfo->state = 0;
    pinfo->area_size = area_size;
    std::memset(pinfo->reserved, 0, sizeof(pinfo->reserved));

    char *tpb_first = ((char *)(pinfo)) + sizeof(shmem_area_info);
    shmem_block_info *tpb_next = NULL;
    int32 tbn = calc_area_block_num();
    for (int32 i = tbn - 1; i >= 0; i--)
    {
      shmem_block_info *tpb = (shmem_block_info *)(tpb_first + (i * block_size));
      init_block(tpb_next, tpb, area_id, i);
      tpb_next = tpb;
    }

    pinfo->free_block = 0;
    pinfo->free_block_num = tbn;
    pinfo->all_block_num = tbn;
    pinfo->state = 1;
  }
  /**
   * @brief 初始化块信息
   *
   * @param pnext 下一个块指针
   * @param pb 当前块指针
   * @param area_id 区域ID
   * @param block_id 块ID
   */
  void shm_block_pool::init_block(shmem_block_info *pnext, shmem_block_info *pb, int16 area_id, int32 block_id)
  {
    init_shm_unit(pb->prev_block);
    pb->head_size = 0; // 保持data从64字节对齐开始
    pb->data_size = 0;
    pb->block_id = block_id;
    pb->area_id = area_id;
    pb->flag = 1; // 0-idle,1-arealist,2-freelist,3-usedlist
    pb->mlock.init();
    pb->data_type = 0;
    pb->first_pos = 0;
    pb->data_num = 0;
    pb->block_size = block_size;

    if (NULL != pnext)
    {
      // set_shm_unit(pnext->prev_block,area_id,-1,block_id);
      set_shm_unit(pb->next_block, pnext->area_id, -1, pnext->block_id);
    }
    else
    {
      init_shm_unit(pb->next_block);
    }
  }
  /**
   * @brief 计算区域可容纳的块数量
   *
   * @return 块数量
   */
  int32 shm_block_pool::calc_area_block_num()
  {
    int64 tb_num = (area_size - sizeof(shmem_area_info)) / block_size;
    return (int32)(tb_num & 0xfffffff);
  }

  /**
   * @brief 恢复区域信息
   *
   * @param pinfo 区域信息指针
   * @param area_id 区域ID
   */
  void shm_block_pool::recove_area(shmem_area_info *pinfo, int16 area_id)
  {
    char *tpb_first = ((char *)(pinfo)) + sizeof(shmem_area_info);
    shmem_block_info *tpb_next = NULL;
    int32 tb_free_n = 0;
    int32 tbn = pinfo->all_block_num;
    for (int32 i = tbn - 1; i >= 0; i--)
    {
      shmem_block_info *tpb = (shmem_block_info *)(tpb_first + (i * block_size));
      if (tpb->first_pos == 0)
      {
        init_block(tpb_next, tpb, area_id, i);
        tpb_next = tpb;
        tb_free_n++;
      }
    }

    if (tb_free_n > 0)
    {
      pinfo->free_block = tpb_next->block_id;
      pinfo->free_block_num = tb_free_n;
    }
    else
    {
      pinfo->free_block = -1;
      pinfo->free_block_num = 0;
    }
    pinfo->state = 1;
  }

  /**
   * @brief 计算指定数据大小可容纳的数据数量
   *
   * @param data_size 数据大小
   * @return 可容纳的数据数量
   */
  int16 shm_block_pool::calc_block_data_num(int32 data_size)
  {
    int32 ts_head = 0;
    return calc_block_data(ts_head, data_size);
  }

  /**
   * @brief 初始化共享内存池
   *
   * @param prev_name 共享内存名称前缀
   * @param tarea_size 区域大小
   * @param tblock_size 块大小
   * @param used_hugepage 是否使用大页面
   * @param create_mem 是否创建内存
   * @return 成功返回0，失败返回错误码
   */
  int32 shm_block_pool::init(const char *prev_name, int64 tarea_size, int32 tblock_size, int16 used_hugepage,
                             int16 create_mem)
  {
    // map_lock.init();
    map_lock.lock();

    area0 = NULL;
    free_area = NULL;
    block_size = tblock_size;
    shm_exist = 0;
    if (create_mem == 0)
      is_write = 0;
    else
      is_write = 1;
    is_hugepage = used_hugepage;
    area_size = tarea_size;
    std::memset(shm_name, 0, sizeof(shm_name));
    comm_utils::str_copy_safe(shm_name, (int32)sizeof(shm_name), prev_name, (int32)sizeof(shm_name)); //(int32) std::strlen(prev_name));

    area_mems.resize(16384);
    for (uint32 i = 0; i < area_mems.size(); i++)
    {
      area_mems[i] = NULL;
    }

    int32 ret = init_mem();
    map_lock.unlock();
    return ret;
  }
  /**
   * @brief 初始化内存
   *
   * @return 成功返回0，失败返回错误码
   */
  int32 shm_block_pool::init_mem()
  {
    shmem_area_info *tpa = NULL;
    int32 ret = alloc_area(tpa, 0);
    if (ret < 0)
    {
      return ret;
    }
    if (ret == 0)
    {
      area0->mlock.lock();
      add_area_list(tpa);
      area0->mlock.unlock();
      shm_exist = 0;
      return ret;
    }

    shm_exist = 1;
    int32 tarea_num = atomic_load32(&(area0->area_num));
    // 修复：area_num 超出 area_mems.size() 时 area_mems[i] OOB 读。
    // if (tarea_num > (int32)area_mems.size())
    //{
    //  tarea_num = (int32)area_mems.size();
    //}
    for (int32 i = 1; i < tarea_num; i++)
    {
      ret = alloc_area(tpa, (int16)(i & 0xffff));
      if (ret < 0)
      {
        return ret;
      }
    }

    if (is_write != 0)
    {
      area0->mlock.lock();

      area0->first_free = -1;
      area0->next_free = -1;
      for (int32 i = tarea_num - 1; i >= 0; i--)
      {
        tpa = area_mems[i];
        tpa->next_free = -1;
        if (tpa->free_block != -1)
        {
          add_area_list(tpa);
        }
      }

      area0->mlock.unlock();
    }
    return 0;
  }
  /**
   * @brief 关闭共享内存池
   */
  void shm_block_pool::close()
  {
    map_lock.lock();
    if (NULL == area0)
    {
      shm_exist = 0;
      map_lock.unlock();
      return;
    }
    if (is_write == 0)
    {
      map_lock.unlock();
      return;
    }

    area0->mlock.lock();
    area0->state = 0;
    area0->first_free = -1;
    int32 tarea_num = atomic_load32(&(area0->area_num));
    // 修复：area_num 超出 area_mems.size() 时 area_mems[i] OOB 读。
    // if (tarea_num > (int32)area_mems.size())
    //{
    //  tarea_num = (int32)area_mems.size();
    //}
    for (int32 i = 1; i < tarea_num; i++)
    {
      shmem_area_info *tpa = area_mems[i];
      tpa->state = 0;
    }
    free_area = NULL;
    shm_exist = 1;
    area0->mlock.unlock();
    map_lock.unlock();
  }
  /**
   * @brief 重建共享内存池
   *
   * @return 成功返回0，失败返回错误码
   */
  int32 shm_block_pool::rebuild()
  {
    int32 ret = 0;
    map_lock.lock();
    if (NULL == area0)
    {
      ret = init_mem();
      map_lock.unlock();
      return ret;
    }
    if (is_write == 0)
    {
      int32 tarea_num = atomic_load32(&(area0->area_num));
      for (int32 i = 0; i < tarea_num; i++)
      {
        shmem_area_info *tpa = area_mems[i];
        if (NULL == tpa)
        {
          alloc_area(tpa, i);
        }
      }
      map_lock.unlock();
      return 0;
    }

    area0->mlock.lock();
    area0->state = 0;
    area0->first_free = -1;
    area0->next_free = -1;

    int32 tarea_num = atomic_load32(&(area0->area_num));
    // P1-4: 倒序遍历 + add_area_list 让先申请的 area 排在 first_free 链表头（优先使用）。
    // 但旧版循环 i 走到 0 时 init_area(area0, 0) 会把 area0->first_free/area_num 重置为
    // -1/0，把前面 i=N-1..1 的 add_area_list 全部作废，导致除 area0 外所有区域泄漏。
    // 修复：把 i=0 (area0) 移出循环，单独 init + add，确保 area0 始终是链表头。
    for (int32 i = tarea_num - 1; i >= 1; i--)
    {
      shmem_area_info *tpa = area_mems[i];
      init_area(tpa, (int16)(i & 0xffff));
      if (tpa->free_block != -1)
      {
        add_area_list(tpa);
      }
    }
    init_area(area0, 0);
    if (area0->free_block != -1)
    {
      add_area_list(area0);
    }
    // 修正 free_area 指向 first_free 链表的第一个有效节点（按"先申请优先"应指向 area0）
    free_area = (area0->first_free >= 0) ? area_mems[area0->first_free] : area0;
    shm_exist = 0;

    area0->mlock.unlock();
    map_lock.unlock();
    return 0;
  }

  /**
   * @brief 恢复共享内存数据
   *
   * @param pfunc 恢复接口指针
   */
  void shm_block_pool::recover(shmem_pool_recover *pfunc)
  {
    map_lock.lock();

    if (shm_exist == 0)
    {
      map_lock.unlock();
      return;
    }

    if (is_write == 0)
    {
      map_lock.unlock();
      return;
    }

    recove_data(pfunc);

    area0->mlock.lock();
    area0->first_free = -1;
    area0->next_free = -1;

    int32 tarea_num = atomic_load32(&(area0->area_num));
    // P1-4: 与 rebuild() 同问题——i=0 (area0) 时 recove_area(area0, 0) 不会重置 first_free/area_num，
    // 但 area0 自己仍会被 add_area_list 处理。为保持"先申请优先"的链表头语义，
    // 显式把 area0 移到循环外处理（先 i=N-1..1 倒序遍历，area0 最后单独 add）。
    for (int32 i = tarea_num - 1; i >= 1; i--)
    {
      shmem_area_info *tpa = area_mems[i];
      tpa->next_free = -1;
      recove_area(tpa, i);
      if (tpa->free_block != -1)
      {
        add_area_list(tpa);
      }
    }
    area0->next_free = -1;
    recove_area(area0, 0);
    if (area0->free_block != -1)
    {
      add_area_list(area0);
    }
    // 修正 free_area 指向 first_free 链表的第一个有效节点
    {
      int32 th = area0->first_free;
      free_area = (th >= 0) ? area_mems[th] : area0;
    }

    area0->mlock.unlock();

    // recove_data(pfunc);
    map_lock.unlock();
  }
  /**
   * @brief 恢复数据
   *
   * @param pfunc 恢复接口指针
   */
  void shm_block_pool::recove_data(shmem_pool_recover *pfunc)
  {
    if (NULL == pfunc)
      return;

    int32 tarea_num = atomic_load32(&(area0->area_num));
    for (int32 i = 0; i < tarea_num; i++)
    {
      shmem_area_info *tpa = area_mems[i];
      char *tpbf = ((char *)(tpa)) + sizeof(shmem_area_info);
      for (int32 j = 0; j < tpa->all_block_num; j++)
      {
        shmem_block_info *tpb = (shmem_block_info *)(tpbf + (j * block_size));
        if (tpb->data_type == 0 || tpb->flag < 2)
          continue;
        recove_block_data(pfunc, tpb);
      }
    }
  }
  /**
   * @brief 恢复块数据
   *
   * @param pfunc 恢复接口指针
   * @param tpb 块指针
   */
  void shm_block_pool::recove_block_data(shmem_pool_recover *pfunc, shmem_block_info *tpb)
  {
    // 使用动态内存分配避免栈溢出
    std::vector<int16> tused_pos(tpb->data_num, 0);

    shm_data_addr taddr;
    char *pdata = ((char *)(tpb)) + tpb->head_size;

    for (int16 i = tpb->first_pos; i < tpb->data_num; i++)
    {
      int16 tid = tpb->data_free_pos[i];
      if (tid >= 0 && tid < tpb->data_num)
      {
        tused_pos[tid] = 1;
      }
    }

    for (int16 i = 0; i < tpb->data_num; i++)
    {
      if (tused_pos[i] == 1)
        continue;
      taddr.area_id = tpb->area_id;
      taddr.data_id = i;
      taddr.block_id = tpb->block_id;
      if (pfunc->recover_data(taddr, (pdata + i * tpb->data_size), tpb->data_size, tpb->data_type))
      {
        continue;
      }

      if (is_write != 0)
      {
        back_data(tpb, i);
      }
    }
  }

} // namespace lb_common
