#pragma once
#include "Common.hh"
#include "util/util.hh"

inline namespace emlsp {
namespace except {

class not_implemented final : public std::logic_error
{
    private:
      std::string text_;

      not_implemented(std::string const &message, char const *function)
          : std::logic_error("Not Implemented")
      {
            text_ = message + " : " + function;
      };

      not_implemented(const char *message, const char *function)
          : not_implemented(std::string(message), function)
      {}

    public:
      not_implemented() : not_implemented("Feature not yet implememented", FUNCTION_NAME)
      {}

      explicit not_implemented(const char *message)
          : not_implemented(message, FUNCTION_NAME)
      {}

      explicit not_implemented(const std::string &message)
          : not_implemented(message, FUNCTION_NAME)
      {}

      ND char const *what() const noexcept override { return text_.c_str(); }
};

} // namespace except
} // namespace emlsp
