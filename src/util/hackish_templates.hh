#pragma once
#ifndef HGUARD__UTIL__HACKISH_TEMPLATES_HH_
#define HGUARD__UTIL__HACKISH_TEMPLATES_HH_ //NOLINT
/*--------------------------------------------------------------------------------------*/

#include "Common.hh"
#include "util/util.hh"

inline namespace emlsp {
namespace util {
/****************************************************************************************/


template <typename V>
      REQUIRES (concepts::IsNotAnyPointer<V>;)
void resize_vector_hack(V &vec, size_t const new_size)
{
      struct nothing {
            typename V::value_type v;
            /* The constructor must be an empty function for this to work. Setting to
             * `default' results in allocated memory being initialized anyway. */
            nothing() {} // NOLINT
      };
      static_assert(sizeof(nothing[10]) == sizeof(typename V::value_type[10]),
                    "alignment error");

      using alloc_traits = std::allocator_traits<typename V::allocator_type>;
      using rebind_type  = typename alloc_traits::template rebind_alloc<nothing>;
      using hack_type    = std::vector<nothing, rebind_type>;

      reinterpret_cast<hack_type &>(vec).resize(new_size);
}

/*--------------------------------------------------------------------------------------*/

namespace impl {

template <typename Elem>
class my_char_traits : public std::char_traits<Elem>
{
      using base_type = std::char_traits<Elem>;

    public:
      using base_type::assign;

      static constexpr Elem *
      assign(Elem *const first, UNUSED size_t count, UNUSED Elem const ch) noexcept
      {
            /* Do nothing. */
            return first;
      }
};

} // namespace impl

template <typename T>
      REQUIRES (concepts::IsNotAnyPointer<T>;)
void resize_string_hack(T &str, size_t const new_size)
{
      using alloc_traits = std::allocator_traits<typename T::allocator_type>;
      using value_t      = typename T::value_type;
      using rebind_type  = typename alloc_traits::template rebind_alloc<value_t>;
      using char_traits  = impl::my_char_traits<value_t>;

      using hackstr = std::basic_string<value_t, char_traits, rebind_type>;

      reinterpret_cast<hackstr &>(str).resize(new_size);
}


/****************************************************************************************/
} // namespace util
} // namespace emlsp
#endif
// vim: ft=cpp
