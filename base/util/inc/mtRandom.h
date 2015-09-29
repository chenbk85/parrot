#ifndef __BASE_UTIL_INC_MTRANDOM_H__
#define __BASE_UTIL_INC_MTRANDOM_H__

#include <random>

namespace parrot
{
class MtRandom
{
  public:
    MtRandom();
    
  public:
    void setSeed(uint64_t);
    uint64_t getSeed() const;
    uint64_t random();
    uint32_t random(uint32_t max);

  private:
    std::mt19937_64 _mt;
    uint64_t _seed;
};
}

#endif
