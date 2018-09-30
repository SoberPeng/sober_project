/************************************************************************
*Author      : Sober.Peng 17-10-27
*Description :
************************************************************************/
#include <stdio.h>
#ifndef WIN32
#include <sys/statfs.h>
#include <sys/stat.h>
#endif
#include "filecache/base/diskhelper.h"

#ifdef __cplusplus
namespace vzes {
extern "C" {
#endif // __cplusplus

FLASH_SIZE FlashTotalSize() {
#ifndef WIN32
  struct statfs disk_statfs;
  if (statfs("/mnt/log", &disk_statfs) == 0) {
    long long int blocks = (long long int)disk_statfs.f_blocks;
    long long int disk_size = blocks * (long long int)disk_statfs.f_bsize;
    if (disk_size > (5 * 1024 * 1024 + 1)) {
      return FLASH_256M;
    }
  }
#endif //  WIN32
  return FLASH_128M;
}

unsigned int GetDiskSpaceSize(char *path) {
#ifndef WIN32
  struct statfs disk_statfs;
  if (statfs(path, &disk_statfs) == 0) {
    long long int blocks = (long long int)disk_statfs.f_blocks;
    long long int disk_size = blocks * (long long int)disk_statfs.f_bsize;
    if (disk_size != 0) {
      return (unsigned int)disk_size/(1024*1024);
    }
  }
#endif  // WIN32
  return 0;
}

#ifdef __cplusplus
};
}
#endif // __cplusplus
