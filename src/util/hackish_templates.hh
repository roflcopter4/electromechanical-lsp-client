#pragma once
#ifndef HGUARD__UTIL__HACKISH_TEMPLATES_HH_
#define HGUARD__UTIL__HACKISH_TEMPLATES_HH_ //NOLINT
/*--------------------------------------------------------------------------------------*/

#include "Common.hh"
#include "util/util.hh"

inline namespace MAIN_PACKAGE_NAMESPACE {
namespace util {
/****************************************************************************************/


template <typename V>
      requires concepts::NotAnyPointer<V>
void resize_vector_hack(V &vec, size_t const new_size)
{
      struct do_nothing {
            typename V::value_type v;
            /* The constructor must be an empty function for this to work. Setting to
             * `default' results in allocated memory being initialized anyway. */
            do_nothing() {} // NOLINT
      };
      static_assert(sizeof(do_nothing[10]) == sizeof(typename V::value_type[10]),
                    "alignment error");

      using alloc_traits = std::allocator_traits<typename V::allocator_type>;
      using rebind_type  = typename alloc_traits::template rebind_alloc<do_nothing>;
      using hack_vector  = std::vector<do_nothing, rebind_type>;

      reinterpret_cast<hack_vector &>(vec).resize(new_size);
}

/*--------------------------------------------------------------------------------------*/

namespace impl {

template <typename Elem>
class my_char_traits : public std::char_traits<Elem>
{
      using base_type = std::char_traits<Elem>;

    public:
      using base_type::assign;

      /* This function is supposed to set `count' elements from `first' to the value
       * `ch'. Typically `ch' is 0. In other words, this function is meant to zero the
       * memory. Instead, this function does nothing. Much better. */
      static constexpr Elem *assign(Elem *const first, size_t, Elem const) noexcept
      {
            /* Do nothing. */
            return first;
      }
};

template <typename T>
concept StdStringDerived =
    std::derived_from<T,
                      std::basic_string<typename T::value_type,
                                        std::char_traits<typename T::value_type>,
                                        std::allocator<typename T::value_type>>>;
} // namespace impl


/* Resize a standard string without zeroing any memory. */
template <typename T>
      requires impl::StdStringDerived<T>
void resize_string_hack(T &str, size_t const new_size)
{
      using value_type   = typename T::value_type;
      using alloc_traits = std::allocator_traits<typename T::allocator_type>;
      using allocator    = typename alloc_traits::template rebind_alloc<value_type>;
      using char_traits  = impl::my_char_traits<value_type>;

      reinterpret_cast<std::basic_string<value_type, char_traits, allocator> &>(str).resize(new_size);
}


/****************************************************************************************/
} // namespace util
} // namespace MAIN_PACKAGE_NAMESPACE
#endif
// vim: ft=cpp
