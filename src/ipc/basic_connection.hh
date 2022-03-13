#pragma once
#ifndef HGUARD__IPC__BASIC_CONNECTION_HH_
#define HGUARD__IPC__BASIC_CONNECTION_HH_ //NOLINT

#include "Common.hh"
#include "ipc/connection_impl.hh"
#include "ipc/dialer.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


/**
 * TODO: Documentation of some kind...
 */
template <typename DialerVariant>
      REQUIRES (IsDialerVariant<DialerVariant>;)
class basic_connection : public DialerVariant
{
      using cint    = int const;
      using csize_t = size_t const;

      using this_type = basic_connection<DialerVariant>;
      using base_type = DialerVariant;

    public:
      using dialer_type          = DialerVariant;
      using connection_impl_type = typename dialer_type::connection_impl_type;
      // FIXME This should probably be private
      using base_type::impl;

      basic_connection()           = default;
      ~basic_connection() override = default;

      DELETE_COPY_CTORS(basic_connection);
      DEFAULT_MOVE_CTORS(basic_connection);

      static auto new_unique() { return std::unique_ptr<this_type>(new this_type); }
      static auto new_shared() { return std::unique_ptr<this_type>(new this_type); }

      //--------------------------------------------------------------------------------

      ssize_t raw_read(void *buf, csize_t nbytes)             { return impl().read(buf, static_cast<ssize_t>(nbytes)); }
      ssize_t raw_read(void *buf, csize_t nbytes, cint flags) { return impl().read(buf, static_cast<ssize_t>(nbytes), flags); }

      ssize_t raw_write(void const *buf, csize_t nbytes)
      {
            fmt::print(stderr, FC("\033[1;33mwriting {} bytes <<'_eof_'\n\033[0;32m{}\033[1;33m\n_eof_\033[0m\n"),
                       nbytes, std::string_view(static_cast<char const *>(buf), nbytes));
            return impl().write(buf, static_cast<ssize_t>(nbytes));
      }
      ssize_t raw_write(void const *buf, csize_t nbytes, cint flags)
      {
            fmt::print(stderr, FC("\033[1;33mwriting {} bytes <<'_eof_'\n\033[0;32m{}\033[1;33m\n_eof_\033[0m\n"),
                       nbytes, std::string_view(static_cast<char const *>(buf), nbytes));
            return impl().write(buf, static_cast<ssize_t>(nbytes), flags);
      }

      ssize_t raw_write(std::string const &buf)                  { return raw_write(buf.data(), buf.size()); }
      ssize_t raw_write(std::string const &buf, cint flags)      { return raw_write(buf.data(), buf.size(), flags); }
      ssize_t raw_write(std::string_view const &buf)             { return raw_write(buf.data(), buf.size()); }
      ssize_t raw_write(std::string_view const &buf, cint flags) { return raw_write(buf.data(), buf.size(), flags); }

      ND auto raw_descriptor() const { return this->impl().fd(); }
      ND auto available()      const { return this->impl().available(); }
};


namespace connections {

using pipe             = basic_connection<dialers::pipe>;
using std_streams      = basic_connection<dialers::std_streams>;
using unix_socket      = basic_connection<dialers::unix_socket>;
using inet_ipv4_socket = basic_connection<dialers::inet_ipv4_socket>;
using inet_ipv6_socket = basic_connection<dialers::inet_ipv6_socket>;
using inet_socket      = basic_connection<dialers::inet_socket>;
#if defined _WIN32 && defined WIN32_USE_PIPE_IMPL
using win32_named_pipe  = basic_connection<dialers::win32_named_pipe>;
using win32_handle_pipe = basic_connection<dialers::win32_handle_pipe>;
using dual_unix_socket  = basic_connection<dialers::dual_unix_socket>;
#endif
#ifdef HAVE_SOCKETPAIR
using socketpair       = basic_connection<dialers::socketpair>;
using Default          = socketpair;
#elif !defined _WIN32
using Default          = unix_socket;
#else
using Default          = dual_unix_socket;
#endif


} // namespace connections


template <typename T>
concept IsBasicConnectionVariant =
    std::derived_from<T, basic_connection<typename T::dialer_type>>;


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif // base_connection.hh
