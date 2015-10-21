#ifndef __BASE_UTIL_INC_SEQGENHELPER_H__
#define __BASE_UTIL_INC_SEQGENHELPER_H__

namespace parrot
{
template <std::size_t... Ts> struct seqIndex
{
};

template <std::size_t N, std::size_t... Ts>
struct genSeq : genSeq<N - 1, N - 1, Ts...>
{
};

template <std::size_t... Ts> struct genSeq<0, Ts...> : seqIndex<Ts...>
{
};
}

#endif
