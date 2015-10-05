#include "poolThread.h"

namespace parrot
{
void PoolThread::beforeRun()
{
    ThreadBase::sleep(-1);
}
}
