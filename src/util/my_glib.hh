#pragma once
#ifndef HGUARD__UTIL__MY_GLIB_HH_
#define HGUARD__UTIL__MY_GLIB_HH_

#include "Common.hh"

inline namespace emlsp {
using namespace std::literals;
namespace util::glib {
/****************************************************************************************/


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


/****************************************************************************************/
} // namespace util::glib
} // namespace emlsp
#endif
