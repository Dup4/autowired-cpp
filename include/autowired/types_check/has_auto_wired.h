#ifndef AUTO_WIRED_TYPES_CHECK_HAS_AUTO_WIRED_H
#define AUTO_WIRED_TYPES_CHECK_HAS_AUTO_WIRED_H

#include <type_traits>

namespace auto_wired::internal {

template <typename T>
class has_auto_wired {
private:
    template <typename U>
    static auto check(int) -> decltype(std::declval<U>().AutoWired(), std::true_type());

    template <typename U>
    static std::false_type check(...);

public:
    enum { value = std::is_same<decltype(check<T>(0)), std::true_type>::value };
};

template <typename T>
inline constexpr bool has_auto_wired_v = has_auto_wired<T>::value;

}  // namespace auto_wired::internal

#endif  // AUTO_WIRED_TYPES_CHECK_HAS_AUTO_WIRED_H
