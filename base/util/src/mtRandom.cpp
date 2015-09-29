#include "mtRandom.h"

namespace parrot
{
MtRandom::MtRandom() : _mt(), _seed(0)
{
    std::random_device rd;
    _seed = rd();
    setSeed(_seed);
}

void MtRandom::setSeed(uint64_t s)
{
    _seed = s;
    _mt.seed(_seed);
}

uint64_t MtRandom::getSeed() const
{
    return _seed;
}

uint64_t MtRandom::random()
{
    return _mt();
}

uint32_t MtRandom::random(uint32_t max)
{
    return _mt() % max;
}
}
