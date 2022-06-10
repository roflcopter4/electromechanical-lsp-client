#pragma once
#ifndef HGUARD__IPC__BASIC_CONNECTION_HH_
#define HGUARD__IPC__BASIC_CONNECTION_HH_ //NOLINT

#include "Common.hh"
// #include "ipc/basic_dialer.hh"

#include "ipc/connection_impl.hh"
#include "ipc/connection_interface.hh"
#include "ipc/basic_dialer.hh"
#include "ipc/ipc_connection.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


#if 0
class basic_connection_interface
{
      using this_type = basic_connection_interface;

     protected:
#ifdef _WIN32
      using descriptor_type = intptr_t;
#else
      using descriptor_type = int;
#endif

    public:
      basic_connection_interface() = default;
      virtual ~basic_connection_interface() = default;

      DEFAULT_ALL_CTORS(basic_connection_interface);

      //--------------------------------------------------------------------------------

      virtual ssize_t   raw_read(void *buf, size_t nbytes)                    = 0;
      virtual ssize_t   raw_read(void *buf, size_t nbytes, int flags)         = 0;
      virtual ssize_t   raw_write(void const *buf, size_t nbytes)             = 0;
      virtual ssize_t   raw_write(void const *buf, size_t nbytes, int flags)  = 0;
      virtual ssize_t   raw_writev(void const *buf, size_t nbytes, int flags) = 0;
      ND virtual size_t available() const                                     = 0;
      ND virtual descriptor_type raw_descriptor() const                       = 0;

      virtual ssize_t raw_write(std::string const &buf)                 { return raw_write(buf.data(), buf.size()); }
      virtual ssize_t raw_write(std::string const &buf, int flags)      { return raw_write(buf.data(), buf.size(), flags); }
      virtual ssize_t raw_write(std::string_view const &buf)            { return raw_write(buf.data(), buf.size()); }
      virtual ssize_t raw_write(std::string_view const &buf, int flags) { return raw_write(buf.data(), buf.size(), flags); }

};
#endif

template <typename DialerType>
      // REQUIRES( BasicDialerVariant<DialerType> )
      // REQUIRES (detail::ConnectionImplVariant<ConnectionImpl>)
      REQUIRES (false)
// class basic_io_impl : public connection_interface, public connection_impl_wrapper<ConnectionImpl>
class connection_io_impl : public DialerType
{
      using this_type  = connection_io_impl<DialerType>;
      using base_type  = DialerType;
      using base_interface = connection_interface;

    protected:
      using descriptor_t = typename DialerType::descriptor_t;
      using procinfo_t = typename DialerType::procinfo_t;

    public:
      using connection_impl_type = typename DialerType::connection_impl_type;
      using dialer_type          = DialerType;

      using base_type::raw_read;
      using base_type::raw_write;
      using base_type::raw_writev;
      using base_type::available;
      using base_type::raw_descriptor;
      using base_type::close;
      using base_type::waitpid;
      using base_type::redirect_stderr_to_default;
      using base_type::redirect_stderr_to_devnull;
      using base_type::redirect_stderr_to_fd;
      using base_type::redirect_stderr_to_filename;
      using base_type::spawn_connection;
      using base_type::pid;

      connection_io_impl()           = default;
      ~connection_io_impl() override = default;

      DELETE_COPY_CTORS(connection_io_impl);
      DEFAULT_MOVE_CTORS(connection_io_impl);

      //static auto new_unique() { return std::unique_ptr<this_type>(new this_type); }
      //static auto new_shared() { return std::unique_ptr<this_type>(new this_type); }

      //--------------------------------------------------------------------------------

      ssize_t raw_read(void *buf, size_t const nbytes) final
      {
            return this->impl().read(buf, static_cast<ssize_t>(nbytes));
      }

      ssize_t raw_read(void *buf, size_t const nbytes, int const flags) final
      {
            return this->impl().read(buf, static_cast<ssize_t>(nbytes), flags);
      }

      ssize_t raw_write(void const *buf, size_t const nbytes) final
      {
            util::eprint(FC("\033[1;33mwriting {} bytes "
                            "<<'_EOF_'\n\033[0;32m{}\033[1;33m\n_EOF_\033[0m\n"),
                         nbytes,
                         std::string_view(static_cast<char const *>(buf), nbytes));
            return this->impl().write(buf, static_cast<ssize_t>(nbytes));
      }

      ssize_t raw_write(void const *buf, size_t const nbytes, int const flags) final
      {
            return this->impl().write(buf, static_cast<ssize_t>(nbytes), flags);
      }

      ssize_t raw_writev(void const *buf, size_t const nbytes, int const flags) final
      {
            return this->impl().write(buf, static_cast<ssize_t>(nbytes), flags);
      }

      ND size_t available() const final
      {
            return this->impl().available();
      }

      ND intptr_t raw_descriptor() const final
      {
            auto const fd = this->impl().fd();
#ifdef _WIN32
            if constexpr (std::is_integral_v<decltype(fd)>)
                  return static_cast<descriptor_type>(this->impl().fd());
            else
                  return reinterpret_cast<descriptor_type>(this->impl().fd());
#else
            return fd;
#endif
      }

#if 0
    protected:
      
         void close()                                          override {};
         int  waitpid() noexcept                               override {return 0;};
         void redirect_stderr_to_default()                     override {};
         void redirect_stderr_to_devnull()                     override {};
         void redirect_stderr_to_fd(descriptor_t)              override {};
         void redirect_stderr_to_filename(std::string const &) override {};
         procinfo_t spawn_connection(size_t, char **)          override {return 0;};
      ND procinfo_t const &pid() const &                       override {return pointless_;};

    private:
      static constexpr procinfo_t pointless_{};
#endif
};




// namespace detail {

#if 0
template <typename ConnectionImpl, /* template <typename> class Connection, */ template <typename> class Dialer>
//template <typename ConnectionImpl, /* template <typename> class Connection, */ typename Dialer>
      // REQUIRES( [>BasicConnectionVariant<Connection<ConnectionImpl>> &&<] BasicDialerVariant<Dialer> )
class basic_connection : public basic_io_impl<ConnectionImpl>, 
                         public Dialer<ConnectionImpl>
{
      using this_type = basic_connection<ConnectionImpl, Dialer>;

    public:
      using connection_impl_type = ConnectionImpl;
      using io_impl_type         = basic_io_impl<ConnectionImpl>;
      using dialer_type          = Dialer<ConnectionImpl>;

      using io_impl_type::raw_read;
      using io_impl_type::raw_write;
      using io_impl_type::raw_writev;
      using io_impl_type::available;
      using io_impl_type::raw_descriptor;

      using dialer_type::close;
      using dialer_type::waitpid;
      using dialer_type::redirect_stderr_to_default;
      using dialer_type::redirect_stderr_to_devnull;
      using dialer_type::redirect_stderr_to_fd;
      using dialer_type::redirect_stderr_to_filename;
      using dialer_type::spawn_connection;
      using dialer_type::pid;
};
#endif

// } // namespace detail
//

#if 0
template <typename ConnectionImpl          = detail::unix_socket_connection_impl,
          template <typename> class Dialer = spawn_dialer>
using basic_connection = connection_io_impl<Dialer<ConnectionImpl>>;

namespace connections {

using pipe              = basic_connection<detail::pipe_connection_impl, spawn_dialer              > ;
using std_streams       = basic_connection<detail::fd_connection_impl, std_streams_dialer> ;
using unix_socket       = basic_connection<detail::unix_socket_connection_impl, spawn_dialer       > ;
using inet_ipv4_socket  = basic_connection<detail::inet_ipv4_socket_connection_impl, spawn_dialer  > ;
using inet_ipv6_socket  = basic_connection<detail::inet_ipv6_socket_connection_impl, spawn_dialer  > ;
using inet_socket       = basic_connection<detail::inet_any_socket_connection_impl, spawn_dialer       > ;
using libuv_pipe_handle = basic_connection<detail::libuv_pipe_handle_impl, spawn_dialer > ;
#if defined _WIN32 && defined WIN32_USE_PIPE_IMPL
using win32_named_pipe  = basic_connection<dialers::win32_named_pipe>;
using win32_handle_pipe = basic_connection<dialers::win32_handle_pipe>;
#endif
#ifdef HAVE_SOCKETPAIR
using socketpair        = basic_connection<detail::socketpair_connection_impl, spawn_dialer>;
#endif
using Default           = libuv_pipe_handle;

} // namespace connections
#endif


#if 0
template <typename T>
concept BasicConnectionVariant =
    // std::derived_from<T, detail::basic_connection<typename T::connection_impl_type, typename T::dialer_type>> &&
    std::derived_from<T, typename T::dialer_type> &&
    BasicDialerVariant<typename T::dialer_type>;
#endif


#if 0
/**
 * TODO: Documentation of some kind...
 */
template <typename DialerVariant>
      REQUIRES (BasicDialerVariant<DialerVariant>)
class basic_connection
      : public basic_connection_interface, public DialerVariant
{
      using this_type  = basic_connection<DialerVariant>;
      using base_type  = DialerVariant;
      using base_iface = basic_connection_interface;

    public:
      using dialer_type          = DialerVariant;
      using connection_impl_type = typename dialer_type::connection_impl_type;
      using base_type::impl;

      basic_connection()           = default;
      ~basic_connection() override = default;

      DELETE_COPY_CTORS(basic_connection);
      DEFAULT_MOVE_CTORS(basic_connection);

      //static auto new_unique() { return std::unique_ptr<this_type>(new this_type); }
      //static auto new_shared() { return std::unique_ptr<this_type>(new this_type); }

      //--------------------------------------------------------------------------------

      ssize_t raw_read(void *buf, size_t const nbytes) override
      {
            return impl().read(buf, static_cast<ssize_t>(nbytes));
      }

      ssize_t raw_read(void *buf, size_t const nbytes, int const flags) override
      {
            return impl().read(buf, static_cast<ssize_t>(nbytes), flags);
      }

      ssize_t raw_write(void const *buf, size_t const nbytes) override
      {
            util::eprint(FC("\033[1;33mwriting {} bytes "
                            "<<'_EOF_'\n\033[0;32m{}\033[1;33m\n_EOF_\033[0m\n"),
                         nbytes,
                         std::string_view(static_cast<char const *>(buf), nbytes));
            return impl().write(buf, static_cast<ssize_t>(nbytes));
      }

      ssize_t raw_write(void const *buf, size_t const nbytes, int const flags) override
      {
            return impl().write(buf, static_cast<ssize_t>(nbytes), flags);
      }

      ssize_t raw_writev(void const *buf, size_t const nbytes, int const flags) override
      {
            return impl().write(buf, static_cast<ssize_t>(nbytes), flags);
      }

      ND size_t available() const override
      {
            return impl().available();
      }

      ND descriptor_type raw_descriptor() const override
      {
            auto const fd = this->impl().fd();
#ifdef _WIN32
            if constexpr (std::is_integral_v<decltype(fd)>)
                  return static_cast<descriptor_type>(impl().fd());
            else
                  return reinterpret_cast<descriptor_type>(impl().fd());
#else
            return fd;
#endif
      }
};


namespace connections {

using pipe              = basic_connection<dialers::pipe>;
using std_streams       = basic_connection<dialers::std_streams>;
using unix_socket       = basic_connection<dialers::unix_socket>;
using inet_ipv4_socket  = basic_connection<dialers::inet_ipv4_socket>;
using inet_ipv6_socket  = basic_connection<dialers::inet_ipv6_socket>;
using inet_socket       = basic_connection<dialers::inet_socket>;
using libuv_pipe_handle = basic_connection<dialers::libuv_pipe_handle>;
#if defined _WIN32 && defined WIN32_USE_PIPE_IMPL
using win32_named_pipe  = basic_connection<dialers::win32_named_pipe>;
using win32_handle_pipe = basic_connection<dialers::win32_handle_pipe>;
#endif
#ifdef HAVE_SOCKETPAIR
using socketpair        = basic_connection<dialers::socketpair>;
#endif
using Default           = libuv_pipe_handle;

} // namespace connections


template <typename T>
concept BasicConnectionVariant =
    std::derived_from<T, basic_connection<typename T::dialer_type>>;
#endif


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif
// vim: ft=cpp
