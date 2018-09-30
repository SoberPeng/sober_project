#ifndef PRIME_KEY_PRIME_H_
#define PRIME_KEY_PRIME_H_

#include <string>

namespace prime{
  // The digest array size must be big than 16 unsigned char
  // 
  class PrimeGenerator{
    public:
    static void HmacMd5(const unsigned char *data, int data_size,
      const unsigned char *key, int key_size, unsigned char *digest);

    static const std::string GenerateVzenithPrimeKey(
      const unsigned char *data, int data_size);
  };
}


#endif // PRIME_KEY_PRIME_H_