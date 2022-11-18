#pragma once
#ifndef HGUARD__UTIL__DELETERS_HH_
#define HGUARD__UTIL__DELETERS_HH_

#include "Common.hh"

inline namespace emlsp {
namespace util {
/****************************************************************************************/


namespace detail {
using free_fn_t = void (*)(void *);
} // namespace detail


template <typename T1, detail::free_fn_t Free = ::free>
class libc_free_deleter
{
    public:
      constexpr libc_free_deleter() noexcept = default;

      template <typename T2>
            REQUIRES (std::convertible_to<T2 *, T1 *>)
      libc_free_deleter(libc_free_deleter<T2> const &) noexcept
      {}

      void operator()(T1 *ptr) const noexcept
      {
            // NOLINTNEXTLINE(bugprone-sizeof-expression)
            static_assert(sizeof(T1) > 0, "Can't delete an incomplete type");
            Free(ptr);  // NOLINT(hicpp-no-malloc, cppcoreguidelines-no-malloc)
      }
};


template <typename T>
using unique_ptr_malloc = std::unique_ptr<T, libc_free_deleter<T>>;


/****************************************************************************************/
} // namespace util
} // namespace emlsp
#endif
