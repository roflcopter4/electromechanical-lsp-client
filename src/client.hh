#pragma once
#ifndef HGUARD_d_CLIENT_HH_
#define HGUARD_d_CLIENT_HH_
/****************************************************************************************/

#include "Common.hh"
#include <map>
#include <vector>

namespace emlsp {

namespace detail {} // namespace detail

/**
 * TODO: Documentation of some kind...
 *
 * Main class for the client. Ideally this should be more like a struct than a class.
 */
class client
{
    public:
      /* This "structure" is intended to be used statically, in which case there
       * won't be a structure at all. Shut up about alignment, clang.
       */
      static constexpr struct info_s { //NOLINT(altera-struct-pack-align)
            static constexpr uint8_t major    = MAIN_PROJECT_VERSION_MAJOR;
            static constexpr uint8_t minor    = MAIN_PROJECT_VERSION_MINOR;
            static constexpr uint8_t patch    = MAIN_PROJECT_VERSION_PATCH;
            static constexpr char    string[] = MAIN_PROJECT_VERSION_STRING;
            static constexpr char    name[]   = MAIN_PROJECT_NAME;
      } info;

      enum class Type {
            NONE, LSP,
      };

    private:
      Type type_;

    public:

      explicit client(Type ty) : type_(ty) {}

      ND Type type() const { return type_; }

#if 0
    private:
      std::string lang_{};
      std::string language_server_{};

      std::vector<std::string>           language_server_argv_{};
      std::map<std::string, std::string> capabilities_{};

    public:
      client() = default;

      ND std::string const &language() const { return lang_; }

      virtual ssize_t write(std::string const &str) = 0;
      virtual std::string read(ssize_t nbytes) = 0;
#endif
};

} // namespace emlsp

/****************************************************************************************/
#endif // client.hh
