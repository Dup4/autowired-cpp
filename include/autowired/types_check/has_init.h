#ifndef AUTO_WIRED_TYPES_CHECK_HAS_INIT_H
#define AUTO_WIRED_TYPES_CHECK_HAS_INIT_H

#include <type_traits>

namespace auto_wired::internal {

template <typename T>
class has_init {
private:
    template <typename U>
    static auto check(int) -> decltype(std::declval<U>().Init(), std::true_type());

    template <typename U>
    static std::false_type check(...);

public:
    enum { value = std::is_same<decltype(check<T>(0)), std::true_type>::value };
};

template <typename T>
inline constexpr bool has_init_v = has_init<T>::value;

}  // namespace auto_wired::internal

#endif  // AUTO_WIRED_TYPES_CHECK_HAS_INIT_H
