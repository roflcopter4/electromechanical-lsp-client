#pragma once
#include "Common.hh"
#include "util/util.hh"

inline namespace emlsp {
namespace util::except {

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
      not_implemented() noexcept : not_implemented("Feature not yet implememented", FUNCTION_NAME)
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

} // namespace util::except
} // namespace emlsp
