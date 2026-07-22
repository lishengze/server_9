#include "wait_wake.h"

#include <fcntl.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace lb_common {

int32 event_wake::init(int32 need_multicast) {
  destroy();
  int32 rc = 0;
  int32 efd = -1;
  if (need_multicast != 0)
    efd = eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE);
  else
    efd = eventfd(0, EFD_CLOEXEC);
  if (-1 == efd) {
    return LBERR_OBJ_OPEN_FAIL;
  }
  if ((rc = fcntl(efd, F_GETFL, 0)) < 0) {
    close(efd);
    return LBERR_ATTR_GET_FAIL;
  }
  rc |= O_NONBLOCK;
  if ((rc = fcntl(efd, F_SETFL, rc)) < 0) {
    close(efd);
    return LBERR_ATTR_SET_FAIL;
  }

  ev_fd = efd;
  return 0;
}
void event_wake::destroy() {
  if (ev_fd < 0)
    return;
  close(ev_fd);
  ev_fd = -1;
}

} // namespace lb_common
