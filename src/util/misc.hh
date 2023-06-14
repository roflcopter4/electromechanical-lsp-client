#pragma once
#ifndef HGUARD__UTIL__MISC_HH_
#define HGUARD__UTIL__MISC_HH_

#include "Common.hh"

#ifdef WANT_GLIB
# include <glib.h> //NOLINT
# include <gio/gio.h>
#endif

/*
 * This file is for stuff that is pretty much overtly awful garbage.
 */

inline namespace MAIN_PACKAGE_NAMESPACE {
using namespace std::literals;
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
            requires std::convertible_to<T2 *, T1 *>
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

/*-------------------------------------------------------------------------------------*/

/* I wouldn't normally check the return of malloc & friends, but MSVC loudly complains
 * about possible null pointers. So I'll make an exception. Damn it Microsoft. */
template <typename T>
ND __forceinline T *
xmalloc(size_t const size = sizeof(T))
{
      //NOLINTNEXTLINE(hicpp-no-malloc, cppcoreguidelines-no-malloc)
      void *ret = malloc(size);
      if (ret == nullptr) [[unlikely]]
            throw std::system_error(std::make_error_code(std::errc::not_enough_memory));
      return static_cast<T *>(ret);
}

template <typename T>
ND __forceinline T *
xcalloc(size_t const nmemb = SIZE_C(1),
        size_t const size  = sizeof(T))
{
      //NOLINTNEXTLINE(hicpp-no-malloc, cppcoreguidelines-no-malloc)
      void *ret = calloc(nmemb, size);
      if (ret == nullptr) [[unlikely]]
            throw std::system_error(std::make_error_code(std::errc::not_enough_memory));
      return static_cast<T *>(ret);
}

constexpr void free_all() {}

/* I can't lie; the primary reason this exists is to silence clang's whining about
 * calling free. */
template <typename T, typename ...Pack>
      requires util::concepts::GenericPointer<T>
constexpr void free_all(T arg, Pack ...pack)
{
      // NOLINTNEXTLINE(hicpp-no-malloc,cppcoreguidelines-no-malloc)
      ::free(arg);
      free_all(pack...);
}

/*-------------------------------------------------------------------------------------*/

#ifdef WANT_GLIB
namespace glib {

template <typename T>
using unique_ptr_glib = std::unique_ptr<T, libc_free_deleter<T, ::g_free>>;

inline unique_ptr_glib<gchar> filename_to_uri(char const *fname)
{
      return unique_ptr_glib<gchar>(g_filename_to_uri(fname, "", nullptr));
}

inline unique_ptr_glib<gchar> filename_to_uri(std::string const &fname)
{
      GError *e = nullptr;
      auto ret = unique_ptr_glib<gchar>(g_filename_to_uri(fname.c_str(), "", &e));
      if (e)
            throw std::runtime_error("glib error: "s + e->message);
      return ret;
}

inline unique_ptr_glib<gchar> filename_to_uri(std::string_view const &fname)
{
      return unique_ptr_glib<gchar>(g_filename_to_uri(fname.data(), "", nullptr));
}

} // namespace glib
#endif

/*-------------------------------------------------------------------------------------*/

namespace except {

class not_implemented final : public std::logic_error
{
      std::string text_;

      not_implemented(std::string const &message, char const *function)
          : std::logic_error("Not Implemented")
      {
            text_ = message + " : " + function;
      }

      not_implemented(char const *message, char const *function)
          : not_implemented(std::string(message), function)
      {}

    public:
      not_implemented() noexcept : not_implemented("Feature not yet implemented", FUNCTION_NAME)
      {}

      explicit not_implemented(char const *message)
          : not_implemented(message, FUNCTION_NAME)
      {}

      explicit not_implemented(std::string const &message)
          : not_implemented(message, FUNCTION_NAME)
      {}

      explicit not_implemented(std::string_view const &message)
          : not_implemented(std::string{message.data(), message.size()}, FUNCTION_NAME)
      {}

      ND char const *what() const noexcept override
      {
            return text_.c_str();
      }
};

} // namespace except

/****************************************************************************************/
} // namespace util
} // namespace MAIN_PACKAGE_NAMESPACE
#endif
