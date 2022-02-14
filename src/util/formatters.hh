#pragma once
#ifndef HGUARD__UTIL__FORMATTERS_HH_
#define HGUARD__UTIL__FORMATTERS_HH_
#include "Common.hh"
/****************************************************************************************/

namespace fmt {
inline namespace v8 {


template <>
struct formatter<::timespec>
{
    private:
      static constexpr double nsec_to_second = 1'000'000'000.0;

    public:
      constexpr auto parse(format_parse_context const &ctx) -> decltype(ctx.begin())
      {
            auto const it = ctx.begin();
            if (it + 1 != ctx.end() && *it != '}')
                  throw format_error("invalid format");

            return it;
      }

      template <typename FormatContext>
      constexpr auto format(::timespec const &p, FormatContext &ctx) -> decltype(ctx.out())
      {
            auto const   sec  = static_cast<double>(p.tv_sec);
            auto const   nsec = static_cast<double>(p.tv_nsec);
            double const val  = sec + (nsec / nsec_to_second);

            return format_to(ctx.out(), FMT_COMPILE("{}s"), val);
      }
};


template <>
struct formatter<std::filesystem::path>
{
    private:
      static constexpr double nsec_to_second = 1'000'000'000.0;

    public:
      constexpr auto parse(format_parse_context const &ctx) -> decltype(ctx.begin())
      {
            auto const it = ctx.begin();
            if (it + 1 != ctx.end() && *it != '}')
                  throw format_error("invalid format");

            return it;
      }

      template <typename FormatContext>
      constexpr auto format(std::filesystem::path const &p, FormatContext &ctx) -> decltype(ctx.out())
      {
            return format_to(ctx.out(), FMT_COMPILE("{}"), p.string());
      }
};


template <>
struct formatter<std::error_code>
{
    private:
      static constexpr double nsec_to_second = 1'000'000'000.0;

    public:
      constexpr auto parse(format_parse_context const &ctx) -> decltype(ctx.begin())
      {
            auto const it = ctx.begin();
            if (it + 1 != ctx.end() && *it != '}')
                  throw format_error("invalid format");

            return it;
      }

      template <typename FormatContext>
      constexpr auto format(std::error_code const &p, FormatContext &ctx) -> decltype(ctx.out())
      {
            return format_to(ctx.out(), FMT_COMPILE("({}: '{}`)"), p.value(), p.message());
      }
};


} // namespace v8
} // namespace fmt


///****************************************************************************************/
#endif
