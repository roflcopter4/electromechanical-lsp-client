#pragma once
#ifndef HGUARD__UTIL__TYPES_HH_
#define HGUARD__UTIL__TYPES_HH_ //NOLINT
/*--------------------------------------------------------------------------------------*/

#include "Common.hh"

inline namespace emlsp {
namespace util {
/****************************************************************************************/


template <typename Elem, template <typename> typename Traits = std::char_traits>
class string_ref : public std::basic_string_view<Elem, Traits<Elem>>
{
    public:
      template <size_t N>
      explicit string_ref(char const (&str)[N]) noexcept : std::string_view(str, N - 1LLU)
      {}
      explicit string_ref(std::string const &str) noexcept : std::string_view(str.data(), str.size())
      {}
      explicit string_ref(std::string_view str) noexcept : std::string_view(str)
      {}
      explicit string_ref(char const *str) noexcept : std::string_view(str, __builtin_strlen(str))
      {}
      explicit string_ref(char const *str, size_t len) noexcept : std::string_view(str, len)
      {}
};

template <typename Elem, template <typename> typename Traits = std::char_traits>
class string_literal : public std::basic_string_view<Elem, Traits<Elem>>
{
    public:
      template <size_t N>
      explicit string_literal(Elem const (&str)[N]) : std::string_view(str, N - 1LLU)
      {}
};


extern std::string_view const &get_signal_name(int signum);
extern std::string_view const &get_signal_explanation(int signum);


/****************************************************************************************/
} // namespace util
} // namespace emlsp
#endif
// vim: ft=cpp
