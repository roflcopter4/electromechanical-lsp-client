#pragma once
#ifndef HGUARD__IPC__DIALER_HH_
#define HGUARD__IPC__DIALER_HH_ //NOLINT

#include "Common.hh"
#include "ipc/connection_impl.hh"
#include "ipc/forward.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


namespace detail {

extern int kill_impl(procinfo_t const &pid);

} // namespace detail


/*--------------------------------------------------------------------------------------*/


template <typename ConnectionImpl>
      REQUIRES (detail::IsConnectionImplVariant<ConnectionImpl>;)
class basic_dialer
{
      ConnectionImpl impl_;

    public:
      using connection_impl_type = ConnectionImpl;

      basic_dialer() = default;
      virtual ~basic_dialer() noexcept = default;

      DELETE_COPY_CTORS(basic_dialer);
      basic_dialer &operator=(basic_dialer &&) noexcept = default;

      basic_dialer(basic_dialer &&other) noexcept
            : impl_(std::move(other.impl_))
      {}

      //--------------------------------------------------------------------------------

      virtual void close() = 0;

      // FIXME This should probably be protected
      ND ConnectionImpl       &impl()       & { return impl_; }
      ND ConnectionImpl const &impl() const & { return impl_; }
};


template <typename ConnectionImpl>
      REQUIRES (detail::IsConnectionImplVariant<ConnectionImpl>;)
class spawn_dialer : public basic_dialer<ConnectionImpl>
{
      using procinfo_t = detail::procinfo_t;

    public:
      using this_type            = spawn_dialer<ConnectionImpl>;
      using base_type            = basic_dialer<ConnectionImpl>;
      using connection_impl_type = typename base_type::connection_impl_type;

    private:
      procinfo_t pid_ = {};

    public:
      spawn_dialer() = default;

      ~spawn_dialer() noexcept override
      {
            try {
                  this->kill(true);
            } catch (...) {}
      }

      DELETE_COPY_CTORS(spawn_dialer);
      spawn_dialer(spawn_dialer &&other) noexcept
          : pid_(std::move(other.pid_)),
            base_type(std::move(other))
      {
            other.pid_ = {};
      }
      spawn_dialer &operator=(spawn_dialer &&other) noexcept = default;

      //--------------------------------------------------------------------------------

      void close() override
      {
            this->kill(false);
      }

      template <typename T>
      void set_stderr_fd(T const fd) { this->impl().set_stderr_fd(fd); }

      void set_stderr_default() { this->impl().set_stderr_default(); }
      void set_stderr_devnull() { this->impl().set_stderr_devnull(); }
      void set_stderr_filename(std::string const &fname)
      {
            this->impl().set_stderr_filename(fname);
      }

      procinfo_t spawn_connection(size_t argc, char **argv)
      {
            pid_ = this->impl().do_spawn_connection(argc, argv);
            return pid_;
      }

      procinfo_t spawn_connection(size_t const argc, char const **argv)
      {
            return spawn_connection(argc, const_cast<char **>(argv));
      }

      procinfo_t spawn_connection(char **const argv)
      {
            char **p = argv;
            while (*p++)
                  ;
            return spawn_connection(p - argv, argv);
      }

      procinfo_t spawn_connection(char const **const argv)
      {
            return spawn_connection(const_cast<char **>(argv));
      }

      procinfo_t spawn_connection(char const *const *const argv)
      {
            return spawn_connection(const_cast<char **>(argv));
      }

      procinfo_t spawn_connection(std::vector<char *> &vec)
      {
            if (vec[vec.size() - 1] != nullptr)
                  vec.emplace_back(nullptr);
            return spawn_connection(vec.size(), vec.data());
      }

      procinfo_t spawn_connection(std::vector<char const *> &vec)
      {
            if (vec[vec.size() - 1] != nullptr)
                  vec.emplace_back(nullptr);
            return spawn_connection(vec.size(), const_cast<char **>(vec.data()));
      }

      /*
       * To be used much like execl. All arguments must be `char const *`. Unlike execl,
       * no null pointer is required as a sentinel.
       */
#ifdef __TAG_HIGHLIGHT__
      template <typename ...Types>
#else
      template <util::concepts::StringLiteral ...Types>
#endif
      procinfo_t spawn_connection_l(Types &&...args)
      {
            char const *const pack[] = {args..., nullptr};
            constexpr size_t  argc   = (sizeof(pack) / sizeof(pack[0])) - SIZE_C(1);
            return spawn_connection(argc, const_cast<char **>(pack));
      }

      ND procinfo_t const &pid() const & { return pid_; }

    private:
      void kill(bool const in_destructor)
      {
            detail::kill_impl(pid_);
            this->impl().close();
            if (!in_destructor)
                  pid_ = {};
      }
};


class std_streams_dialer : public basic_dialer<detail::fd_connection_impl>
{
    public:
      using this_type = std_streams_dialer;
      using base_type = basic_dialer<connection_impl_type>;

      std_streams_dialer()
      {
            impl().set_descriptors(0, 1);
      }

      ~std_streams_dialer() noexcept override = default;

      DELETE_COPY_CTORS(std_streams_dialer);
      DEFAULT_MOVE_CTORS(std_streams_dialer);

      //--------------------------------------------------------------------------------

      void close() override {}
};


/*--------------------------------------------------------------------------------------*/


namespace dialers {

using pipe             = ipc::spawn_dialer<ipc::detail::pipe_connection_impl>;
using std_streams      = ipc::std_streams_dialer;
using inet_ipv4_socket = ipc::spawn_dialer<ipc::detail::inet_ipv4_socket_connection_impl>;
using inet_ipv6_socket = ipc::spawn_dialer<ipc::detail::inet_ipv6_socket_connection_impl>;
using inet_socket      = ipc::spawn_dialer<ipc::detail::inet_any_socket_connection_impl>;
using unix_socket      = ipc::spawn_dialer<ipc::detail::unix_socket_connection_impl>;
#ifdef HAVE_SOCKETPAIR
using socketpair       = ipc::spawn_dialer<ipc::detail::socketpair_connection_impl>;
#endif
#if defined _WIN32 && defined WIN32_USE_PIPE_IMPL
using win32_named_pipe   = ipc::spawn_dialer<ipc::detail::win32_named_pipe_impl>;
using win32_handle_pipe  = ipc::spawn_dialer<ipc::detail::pipe_handle_connection_impl>;
#endif

} // namespace dialers


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif // dialer.hh
