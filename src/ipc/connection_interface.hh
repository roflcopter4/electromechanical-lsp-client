#pragma once
#ifndef HGUARD__IPC__CONNECTION_INTERFACE_HH_
#define HGUARD__IPC__CONNECTION_INTERFACE_HH_ //NOLINT

#include "Common.hh"
#include "ipc/connection_impl.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


namespace detail {
#ifdef _WIN32
using descriptor_type = intptr_t;
#else
using descriptor_type = int;
#endif
} // namespace detail


class connection_interface
{
      using this_type = connection_interface;

    protected:
      using procinfo_t   = util::detail::procinfo_t;
      using descriptor_t = detail::descriptor_type;

    public:
      connection_interface()          = default;
      virtual ~connection_interface() = default;

      DEFAULT_ALL_CTORS(connection_interface);

      //--------------------------------------------------------------------------------

      virtual ssize_t   raw_read(void *buf, size_t nbytes)                    = 0;
      virtual ssize_t   raw_read(void *buf, size_t nbytes, int flags)         = 0;
      virtual ssize_t   raw_write(void const *buf, size_t nbytes)             = 0;
      virtual ssize_t   raw_write(void const *buf, size_t nbytes, int flags)  = 0;
      virtual ssize_t   raw_writev(void const *buf, size_t nbytes, int flags) = 0;
      ND virtual size_t available() const                                     = 0;
      ND virtual intptr_t raw_descriptor() const                          = 0;

      virtual ssize_t raw_write(std::string const &buf)                 { return raw_write(buf.data(), buf.size()); }
      virtual ssize_t raw_write(std::string const &buf, int flags)      { return raw_write(buf.data(), buf.size(), flags); }
      virtual ssize_t raw_write(std::string_view const &buf)            { return raw_write(buf.data(), buf.size()); }
      virtual ssize_t raw_write(std::string_view const &buf, int flags) { return raw_write(buf.data(), buf.size(), flags); }

      //--------------------------------------------------------------------------------

      virtual void close()                                               = 0;
      virtual int  waitpid() noexcept                                    = 0;
      virtual void redirect_stderr_to_default()                          = 0;
      virtual void redirect_stderr_to_devnull()                          = 0;
      virtual void redirect_stderr_to_fd(descriptor_t fd)                = 0;
      virtual void redirect_stderr_to_filename(std::string const &fname) = 0;
      virtual procinfo_t spawn_connection(size_t argc, char **argv)      = 0;
      ND virtual procinfo_t const &pid() const &                         = 0;

      virtual procinfo_t spawn_connection(char **const argv)
      {
            char **p = argv;
            while (*p++)
                  ;
            return spawn_connection(util::ptr_diff(p, argv), argv);
      }

      virtual procinfo_t spawn_connection(std::vector<char const *> &&vec)
      {
            if (vec[vec.size() - 1] != nullptr)
                  vec.emplace_back(nullptr);
            return spawn_connection(vec.size() - 1, const_cast<char **>(vec.data()));
      }

      virtual procinfo_t spawn_connection(size_t argc, char const **argv) { return spawn_connection(argc, const_cast<char **>(argv)); }
      virtual procinfo_t spawn_connection(char const **argv)              { return spawn_connection(const_cast<char **>(argv)); }
      virtual procinfo_t spawn_connection(char const *const *argv)        { return spawn_connection(const_cast<char **>(argv)); }
      virtual procinfo_t spawn_connection(std::vector<char *> &vec)       { return spawn_connection(reinterpret_cast<std::vector<char const *> &>(vec)); }
      virtual procinfo_t spawn_connection(std::vector<char *> &&vec)      { return spawn_connection(reinterpret_cast<std::vector<char const *> &&>(vec)); }
      virtual procinfo_t spawn_connection(std::vector<char const *> &vec) { return spawn_connection(std::forward<std::vector<char const *> &&>(vec)); }

      /**
       * \brief Spawn a process. This method is To be used much like execl. Unlike execl,
       * no null pointer is required as a sentinel.
       * \tparam Types Must be char const (&)[].
       * \param args  All arguments must be string literals.
       * \return Process id (either pid_t or PROCESS_INFORMATION).
       */
      template <util::concepts::StringLiteral ...Types>
      procinfo_t spawn_connection_l(Types &&...args)
      {
            char const *const pack[] = {args..., nullptr};
            constexpr size_t  argc   = (sizeof(pack) / sizeof(pack[0])) - SIZE_C(1);
            return spawn_connection(argc, const_cast<char **>(pack));
      }
};


template <typename ConnectionImpl>
      REQUIRES (detail::ConnectionImplVariant<ConnectionImpl>)
class connection_impl_wrapper
{
      ConnectionImpl impl_;

    public:
      using connection_impl_type = ConnectionImpl;

      connection_impl_wrapper()          = default;
      virtual ~connection_impl_wrapper() = default;

      DEFAULT_ALL_CTORS(connection_impl_wrapper);

      //--------------------------------------------------------------------------------

      // FIXME This should probably be protected
      ND virtual ConnectionImpl       &impl()       & { return impl_; }
      ND virtual ConnectionImpl const &impl() const & { return impl_; }
};


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif
// vim: ft=cpp
