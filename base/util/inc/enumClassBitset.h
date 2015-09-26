#include <bitset>
#include <type_traits>

template <typename T> class EnumClassBitset {
  private:
    typename std::underlying_type<T>::type getValue(T v) const {
        return static_cast<typename std::underlying_type<T>::type>(v);
    }

  public:
    EnumClassBitset() : _enumBitset() {
    }

    bool test(T pos) const {
        return _enumBitset.test(getValue(pos));
    }

    constexpr bool operator[](T pos) const {
        return _enumBitset[getValue(pos)];
    }

    typename std::bitset<static_cast<typename std::underlying_type<T>::type>(
        T::TotalFlags)>::reference
    operator[](T pos) {
        return _enumBitset[getValue(pos)];
    }

    EnumClassBitset &reset(T pos) {
        _enumBitset.reset(getValue(pos));
        return *this;
    }

    EnumClassBitset &flip(T pos) {
        _enumBitset.flip(getValue(pos));
        return *this;
    }

  private:
    std::bitset<static_cast<typename std::underlying_type<T>::type>(
        T::TotalFlags)> _enumBitset;
};
