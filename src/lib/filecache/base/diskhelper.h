/************************************************************************
*Author      : Sober.Peng 17-10-27
*Description :
************************************************************************/
#ifndef LIBVZBASE_DISKHELPER_H_
#define LIBVZBASE_DISKHELPER_H_

#ifdef __cplusplus
#include <string>

namespace vzes {
extern "C" {
#endif // __cplusplus

enum FLASH_SIZE {
  FLASH_256M = 1,
  FLASH_128M = 2,
};

FLASH_SIZE FlashTotalSize();

unsigned int GetDiskSpaceSize(char *path);

#ifdef __cplusplus
};

}
#endif // __cplusplus

#endif  // LIBVZBASE_DISKHELPER_H_
