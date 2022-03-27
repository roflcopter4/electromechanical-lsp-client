#pragma once
#ifndef HGUARD__UTIL__TYPES_HH_
#define HGUARD__UTIL__TYPES_HH_ //NOLINT
/*--------------------------------------------------------------------------------------*/

#include "Common.hh"

inline namespace emlsp {
namespace util {
/****************************************************************************************/


class string_ref : public std::string_view
{
    public:
      template <size_t N>
      explicit string_ref(char const (&str)[N]) : std::string_view(str, N)
      {}
      explicit string_ref(std::string const &str) : std::string_view(str.data(), str.size())
      {}
      explicit string_ref(std::string_view str) : std::string_view(str)
      {}
      explicit string_ref(char const *str) : std::string_view(str, __builtin_strlen(str))
      {}
};

class string_literal : public std::string_view
{
    public:
      template <size_t N>
      explicit string_literal(char const (&str)[N]) : std::string_view(str, N)
      {}
};


extern std::string_view const &get_signal_name(int signum);
extern std::string_view const &get_signal_explanation(int signum);


/****************************************************************************************/
} // namespace util
} // namespace emlsp
#endif
// vim: ft=cpp
