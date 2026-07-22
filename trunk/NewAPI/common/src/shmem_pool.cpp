
#include "shmem_pool.h"
#include "mutils.h"

namespace lb_common {

/**
 * @brief 根据区域ID获取区域信息指针
 * 
 * @param area_id 区域ID
 * @return 区域信息指针，失败返回NULL
 */
shmem_area_info *shm_block_pool::addr2area(int16 area_id) {
  shmem_area_info *tpa = area_mems[area_id];
  if (likely(NULL != tpa))
    return tpa;

  if (is_write == 0) {
    map_lock.lock();
    tpa = area_mems[area_id];
    if (NULL == tpa) {
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
int32 shm_block_pool::get_block(shmem_block_info *&o_block, int32 data_type, int32 data_size) {
  shmem_block_info *rb = NULL;
  while (NULL != free_area) {
    rb = take_area_block();
    if (likely(NULL != rb)) {
      build_block_data(rb, data_type, data_size);
      o_block = rb;
      return 0;
    }
    pop_area_list();
  }

  int32 tid = atomic_load32(&(area0->area_num)) + 1;
  shmem_area_info *tpa = NULL;
  int32 ret = alloc_area(tpa, (int16)(tid & 0xffff));
  if (ret >= 0) {
    add_area_list(tpa);
    rb = take_area_block();
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
int32 shm_block_pool::take_block(shmem_block_info *&o_block, int32 data_type, int32 data_size) {
  shmem_block_info *rb = NULL;

  area0->mlock.lock();
  while (NULL != free_area) {
    rb = take_area_block();
    if (likely(NULL != rb)) {
      area0->mlock.unlock();

      build_block_data(rb, data_type, data_size);
      o_block = rb;
      return 0;
    }
    pop_area_list();
  }
  area0->mlock.unlock();

  map_lock.lock();
  int32 tid = atomic_load32(&(area0->area_num)) + 1;
  shmem_area_info *tpa = NULL;
  int32 ret = alloc_area(tpa, (int16)(tid & 0xffff));
  map_lock.unlock();

  if (ret >= 0) {
    area0->mlock.lock();
    add_area_list(tpa);
    rb = take_area_block();
    area0->mlock.unlock();

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
void shm_block_pool::build_block_data(shmem_block_info *pb, int32 data_type, int32 data_size) {
  init_shm_unit(pb->next_block);
  int16 tnum = calc_block_data(pb->head_size, data_size);
  pb->data_size = data_size;
  pb->mlock.init();
  pb->data_type = data_type;
  pb->first_pos = 0;
  pb->data_num = tnum;

  for (int16 i = 0; i < tnum; i++) {
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
int16 shm_block_pool::calc_block_data(int32 &o_head_size, int32 data_size) {
  int32 tn = (block_size - sizeof(shmem_block_info)) / data_size;
  int32 ts = sizeof(shmem_block_info) + sizeof(int16) * tn;
  int32 ts_align = ALIGN_UP(ts, 64);
  while (ts != ts_align) {
    if (ts_align + data_size * tn <= block_size)
      break;
    tn--;
    ts = sizeof(shmem_block_info) + sizeof(int16) * tn;
    ts_align = ALIGN_UP(ts, 64);
  }
  o_head_size = ts_align;
  return tn;
}

/**
 * @brief 单线程释放页块
 * 
 * @param pblock 要释放的块指针
 */
void shm_block_pool::back_block(shmem_block_info *pblock) {
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
void shm_block_pool::release_block(shmem_block_info *pblock) {
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
int32 shm_block_pool::alloc_area(shmem_area_info *&o_area, int16 area_id) {
  char tname[256];
  std::memset(tname, 0, sizeof(tname));
  std::snprintf(tname, sizeof(tname) - 1, "%s_area_%d", shm_name, area_id);

  void *tpm = NULL;
  int32 is_create = is_write;
  int32 shmisexist = comm_utils::map_shm(tpm, tname, area_size, is_hugepage, is_create);
  if (shmisexist < 0) {
    o_area = NULL;
    return shmisexist;
  }

  shmem_area_info *ta = (shmem_area_info *)tpm;
  if (area_id == 0) {
    area0 = ta;
  }

  if (is_create == 1) {
    if (shmisexist == 0 || ta->state == 0 || area0->state == 0) {
      init_area(ta, area_id);
    } else {
      assert(ta->area_size == area_size && ta->block_size == block_size && ta->is_hugepage == is_hugepage &&
             ta->area_id == area_id);
    }
  } else {
    assert(1 == shmisexist);
    assert(ta->area_size == area_size && ta->block_size == block_size && ta->is_hugepage == is_hugepage &&
           ta->area_id == area_id);

    while (ta->state == 0) {
      comm_utils::sleep_us(10);
    }
  }

  area_mems[area_id] = ta;
  o_area = ta;

  if (is_create == 1) {
    int32 tan = atomic_load32(&(area0->area_num));
    do {
      if (tan >= area_id + 1) {
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
void shm_block_pool::init_area(shmem_area_info *pinfo, int16 area_id) {
  pinfo->state = 0;
  pinfo->mlock.init();
  pinfo->first_free = -1; //仅第一个区域有效
  pinfo->next_free = -1;
  pinfo->free_block = -1;
  pinfo->free_block_num = 0;
  pinfo->area_id = area_id;
  pinfo->is_hugepage = is_hugepage;
  pinfo->block_size = block_size;
  pinfo->all_block_num = 0;

  pinfo->area_num = 0; //仅第一个区域有效
  pinfo->state = 0;
  pinfo->area_size = area_size;
  std::memset(pinfo->reserved, 0, sizeof(pinfo->reserved));

  char *tpb_first = ((char *)(pinfo)) + sizeof(shmem_area_info);
  shmem_block_info *tpb_next = NULL;
  int32 tbn = calc_area_block_num();
  for (int32 i = tbn - 1; i >= 0; i--) {
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
void shm_block_pool::init_block(shmem_block_info *pnext, shmem_block_info *pb, int16 area_id, int32 block_id) {
  init_shm_unit(pb->prev_block);
  pb->head_size = 0; //保持data从64字节对齐开始
  pb->data_size = 0;
  pb->block_id = block_id;
  pb->area_id = area_id;
  pb->flag = 1; //0-idle,1-arealist,2-freelist,3-usedlist
  pb->mlock.init();
  pb->data_type = 0;
  pb->first_pos = 0;
  pb->data_num = 0;
  pb->block_size = block_size;

  if (NULL != pnext) {
    //set_shm_unit(pnext->prev_block,area_id,-1,block_id);
    set_shm_unit(pb->next_block, pnext->area_id, -1, pnext->block_id);
  } else {
    init_shm_unit(pb->next_block);
  }
}
/**
 * @brief 计算区域可容纳的块数量
 * 
 * @return 块数量
 */
int32 shm_block_pool::calc_area_block_num() {
  int64 tb_num = (area_size - sizeof(shmem_area_info)) / block_size;
  return (int32)(tb_num & 0xfffffff);
}

/**
 * @brief 恢复区域信息
 * 
 * @param pinfo 区域信息指针
 * @param area_id 区域ID
 */
void shm_block_pool::recove_area(shmem_area_info *pinfo, int16 area_id) {
  char *tpb_first = ((char *)(pinfo)) + sizeof(shmem_area_info);
  shmem_block_info *tpb_next = NULL;
  int32 tb_free_n = 0;
  int32 tbn = pinfo->all_block_num;
  for (int32 i = tbn - 1; i >= 0; i--) {
    shmem_block_info *tpb = (shmem_block_info *)(tpb_first + (i * block_size));
    if (tpb->first_pos == 0) {
      init_block(tpb_next, tpb, area_id, i);
      tpb_next = tpb;
      tb_free_n++;
    }
  }

  if (tb_free_n > 0) {
    pinfo->free_block = tpb_next->block_id;
    pinfo->free_block_num = tb_free_n;
  } else {
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
int16 shm_block_pool::calc_block_data_num(int32 data_size) {
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
                           int16 create_mem) {
  //map_lock.init();
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
  comm_utils::str_copy_format(shm_name, prev_name, sizeof(shm_name));

  area_mems.resize(16384);
  for (uint32 i = 0; i < area_mems.size(); i++) {
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
int32 shm_block_pool::init_mem() {
  shmem_area_info *tpa = NULL;
  int32 ret = alloc_area(tpa, 0);
  if (ret < 0) {
    return ret;
  }
  if (ret == 0) {
    area0->mlock.lock();
    add_area_list(tpa);
    area0->mlock.unlock();
    shm_exist = 0;
    return ret;
  }

  shm_exist = 1;
  int32 tarea_num = atomic_load32(&(area0->area_num));
  for (int32 i = 1; i < tarea_num; i++) {
    ret = alloc_area(tpa, (int16)(i & 0xffff));
    if (ret < 0) {
      return ret;
    }
  }

  if (is_write != 0) {
    area0->mlock.lock();

    area0->first_free = -1;
    area0->next_free = -1;
    for (int32 i = tarea_num - 1; i >= 0; i--) {
      tpa = area_mems[i];
      tpa->next_free = -1;
      if (tpa->free_block != -1) {
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
void shm_block_pool::close() {
  map_lock.lock();
  if (NULL == area0) {
    shm_exist = 0;
    map_lock.unlock();
    return;
  }
  if (is_write == 0) {
    map_lock.unlock();
    return;
  }

  area0->mlock.lock();
  area0->state = 0;
  area0->first_free = -1;
  int32 tarea_num = atomic_load32(&(area0->area_num));
  for (int32 i = 1; i < tarea_num; i++) {
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
int32 shm_block_pool::rebuild() {
  int32 ret = 0;
  map_lock.lock();
  if (NULL == area0) {
    ret = init_mem();
    map_lock.unlock();
    return ret;
  }
  if (is_write == 0) {
    int32 tarea_num = atomic_load32(&(area0->area_num));
    for (int32 i = 0; i < tarea_num; i++) {
      shmem_area_info *tpa = area_mems[i];
      if (NULL == tpa) {
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
  for (int32 i = tarea_num - 1; i >= 0; i--) {
    shmem_area_info *tpa = area_mems[i];
    init_area(tpa, (int16)(i & 0xffff));
    if (tpa->free_block != -1) {
      add_area_list(tpa);
    }
  }
  free_area = area0;
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
void shm_block_pool::recover(shmem_pool_recover *pfunc) {
  map_lock.lock();

  if (shm_exist == 0) {
    map_lock.unlock();
    return;
  }

  if (is_write == 0) {
    map_lock.unlock();
    return;
  }

  recove_data(pfunc);

  area0->mlock.lock();
  area0->first_free = -1;
  area0->next_free = -1;

  int32 tarea_num = atomic_load32(&(area0->area_num));
  for (int32 i = tarea_num - 1; i >= 0; i--) {
    shmem_area_info *tpa = area_mems[i];
    tpa->next_free = -1;
    recove_area(tpa, i);
    if (tpa->free_block != -1) {
      add_area_list(tpa);
    }
  }

  area0->mlock.unlock();

  //recove_data(pfunc);
  map_lock.unlock();
}
/**
 * @brief 恢复数据
 * 
 * @param pfunc 恢复接口指针
 */
void shm_block_pool::recove_data(shmem_pool_recover *pfunc) {
  if (NULL == pfunc)
    return;

  int32 tarea_num = atomic_load32(&(area0->area_num));
  for (int32 i = 0; i < tarea_num; i++) {
    shmem_area_info *tpa = area_mems[i];
    char *tpbf = ((char *)(tpa)) + sizeof(shmem_area_info);
    for (int32 j = 0; j < tpa->all_block_num; j++) {
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
void shm_block_pool::recove_block_data(shmem_pool_recover *pfunc, shmem_block_info *tpb) {
  //使用动态内存分配避免栈溢出
  std::vector<int16> tused_pos(tpb->data_num, 0);

  shm_data_addr taddr;
  char *pdata = ((char *)(tpb)) + tpb->head_size;

  for (int16 i = tpb->first_pos; i < tpb->data_num; i++) {
    int16 tid = tpb->data_free_pos[i];
    if (tid >= 0 && tid < tpb->data_num) {
      tused_pos[tid] = 1;
    }
  }

  for (int16 i = 0; i < tpb->data_num; i++) {
    if (tused_pos[i] == 1)
      continue;
    taddr.area_id = tpb->area_id;
    taddr.data_id = i;
    taddr.block_id = tpb->block_id;
    if (pfunc->recover_data(taddr, (pdata + i * tpb->data_size), tpb->data_size, tpb->data_type)) {
      continue;
    }

    if (is_write != 0) {
      back_data(tpb, i);
    }
  }
}

} // namespace lb_common
