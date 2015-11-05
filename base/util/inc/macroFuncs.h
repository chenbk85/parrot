#ifndef __BASE_UTIL_INC_MACROFUNCS_H__
#define __BASE_UTIL_INC_MACROFUNCS_H__

#include <thread>
#include <chrono>
#include <cassert>

/**
 * PARROT_ASSERT
 *
 * Sleep 1 second to let logger thread finish logging before assert.
 */
#define PARROT_ASSERT(x)                                                       \
    do                                                                         \
    {                                                                          \
        if (!(x))                                                              \
        {                                                                      \
            assert(0);                                                         \
        }                                                                      \
    } while (false)

#endif
