#pragma once
#ifndef HGUARD__IPC__BASE_CONNECTION_HH_
#define HGUARD__IPC__BASE_CONNECTION_HH_ //NOLINT

#include "Common.hh"
#include "ipc/forward.hh"

#include "ipc/connection_impl.hh"
#include "ipc/dialer.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


/**
 * TODO: Documentation of some kind...
 */
template <typename ConnectionImpl>
      REQUIRES (detail::IsConnectionImplVariant<ConnectionImpl>)
class base_connection
{
      using procinfo_t   = detail::procinfo_t;
    public:
      using connection_impl_type = ConnectionImpl;
      using this_type    = base_connection<ConnectionImpl>;
      using cstring_uptr = std::unique_ptr<char[]>;
      using cstring_sptr = std::shared_ptr<char[]>;

    private:
      procinfo_t pid_ = {};
      std::unique_ptr<ConnectionImpl> connection_ = {};

    protected:
      ND ConnectionImpl       *connection()       { return connection_.get(); }
      ND ConnectionImpl const *connection() const { return connection_.get(); }

      void kill(bool const in_destructor)
      {
            connection_->close();
            detail::kill_impl(pid_);
            if (!in_destructor)
                  memset(&pid_, 0, sizeof(pid_));
      }

      void wipe_connection() { connection_ = {}; }

    public:
      base_connection() : connection_(std::make_unique<ConnectionImpl>())
      {}

      explicit base_connection(ConnectionImpl &con) : connection_(std::move(con)) {}

      virtual ~base_connection() noexcept
      {
            try {
                  this->kill(true);
            } catch (...) {}
      }

      base_connection(base_connection const &)                = delete;
      base_connection &operator=(base_connection const &)     = delete;
      base_connection(base_connection &&) noexcept            = default;
      base_connection &operator=(base_connection &&) noexcept = default;

      //--------------------------------------------------------------------------------


      procinfo_t spawn_connection(size_t argc, char **argv)
      {
            pid_ = connection_->do_spawn_connection(argc, argv);
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

      ssize_t raw_read(void *buf, size_t const nbytes)                  { return connection_->read(buf, static_cast<ssize_t>(nbytes)); }
      ssize_t raw_read(void *buf, size_t const nbytes, int const flags) { return connection_->read(buf, static_cast<ssize_t>(nbytes), flags); }

      ssize_t raw_write(void const *buf, size_t const nbytes)                  { return connection_->write(buf, static_cast<ssize_t>(nbytes)); }
      ssize_t raw_write(void const *buf, size_t const nbytes, int const flags) { return connection_->write(buf, static_cast<ssize_t>(nbytes), flags); }

      template <size_t N>
      void write_message_l(char const (&msg)[N])
      {
            write_message(msg, sizeof(msg) - SIZE_C(1));
      }

      void kill() { this->kill(false); }

      void set_stderr_default() { connection_->set_stderr_default(); }
      void set_stderr_devnull() { connection_->set_stderr_devnull(); }
      void set_stderr_filename(std::string const &fname)
      {
            connection_->set_stderr_filename(fname);
      }

      ND procinfo_t const &pid() const & { return pid_; }

      ND typename ConnectionImpl::descriptor_type raw_descriptor() const
      {
            return connection_->fd();
      }

    private:
      [[noreturn]] static void nil_impl()
      {
            throw util::except::not_implemented(
                fmt::format(FC("Function \"{}\" is not implemented"), __func__));
      }

    public:
      virtual size_t discard_message()                                         {nil_impl();}
      virtual void   write_message(char const * /*unused*/)                    {nil_impl();}
      virtual void   write_message(void const * /*unused*/, size_t /*unused*/) {nil_impl();}
      virtual void   write_message(std::string const & /*unused*/)             {nil_impl();}
      virtual void   write_message(std::vector<char> const & /*unused*/)       {nil_impl();}
      virtual size_t            read_message(void ** /*unused*/)               {nil_impl();}
      virtual size_t            read_message(char ** /*unused*/)               {nil_impl();}
      virtual size_t            read_message(char *& /*unused*/)               {nil_impl();}
      virtual std::vector<char> read_message()                                 {nil_impl();}
      virtual std::string       read_message_string()                          {nil_impl();}
      virtual size_t            read_message(cstring_uptr & /*unused*/)        {nil_impl();}
      virtual size_t            read_message(cstring_sptr & /*unused*/)        {nil_impl();}
};


/*--------------------------------------------------------------------------------------*/


using base_unix_socket_connection = base_connection<detail::unix_socket_connection_impl>;
using base_pipe_connection        = base_connection<detail::pipe_connection_impl>;
#ifdef HAVE_SOCKETPAIR
using base_socketpair_connection  = base_connection<detail::socketpair_connection_impl>;
#endif
#if defined _WIN32 && defined WIN32_USE_PIPE_IMPL
using base_named_pipe_connection  = base_connection<detail::win32_named_pipe_impl>;
#endif


/****************************************************************************************/

/*──────────────────────────────────────────────────────────────────────────────────────*
 *  ┌────────────────┐                                                                  *
 *  │Attempt number 2│                                                                  *
 *  └────────────────┘                                                                  *
 *──────────────────────────────────────────────────────────────────────────────────────*/


/**
 * TODO: Documentation of some kind...
 */
template <typename DialerVariant>
      REQUIRES (IsDialerVariant<DialerVariant>)
class basic_connection : public DialerVariant
{
      typedef int const    cint;
      typedef size_t const csize_t;

      using this_type = basic_connection<DialerVariant>;
      using base_type = DialerVariant;
      using base_type::impl;

      basic_connection() = default;

    public:
      using dialer_type          = DialerVariant;
      using connection_impl_type = typename dialer_type::connection_impl_type;

      ~basic_connection() override = default;

      DELETE_COPY_CTORS(basic_connection);
      DEFAULT_MOVE_CTORS(basic_connection);

      static auto new_unique() { return std::unique_ptr<this_type>(new this_type); }
      static auto new_shared() { return std::shared_ptr<this_type>(new this_type); }

      //--------------------------------------------------------------------------------


      ssize_t raw_read(void *buf, csize_t nbytes)             { return impl().read(buf, static_cast<ssize_t>(nbytes)); }
      ssize_t raw_read(void *buf, csize_t nbytes, cint flags) { return impl().read(buf, static_cast<ssize_t>(nbytes), flags); }

      ssize_t raw_write(void const *buf, csize_t nbytes)             { return impl().write(buf, static_cast<ssize_t>(nbytes)); }
      ssize_t raw_write(void const *buf, csize_t nbytes, cint flags) { return impl().write(buf, static_cast<ssize_t>(nbytes), flags); }

      ssize_t raw_write(std::string const &buf)                  { return raw_write(buf.data(), buf.size()); }
      ssize_t raw_write(std::string const &buf, cint flags)      { return raw_write(buf.data(), buf.size(), flags); }
      ssize_t raw_write(std::string_view const &buf)             { return raw_write(buf.data(), buf.size()); }
      ssize_t raw_write(std::string_view const &buf, cint flags) { return raw_write(buf.data(), buf.size(), flags); }

      ND auto raw_descriptor() const { return this->impl().fd(); }
};


namespace connections {

using spawn_unix_socket = basic_connection<dialers::spawn_unix_socket>;
using spawn_pipe        = basic_connection<dialers::spawn_pipe>;
using std_streams       = basic_connection<dialers::std_streams>;
#ifdef HAVE_SOCKETPAIR
using spawn_socketpair  = basic_connection<dialers::spawn_socketpair>;
using spawn_default     = spawn_socketpair;
#else
using spawn_default = spawn_unix_socket;
#endif
#if defined _WIN32 && defined WIN32_USE_PIPE_IMPL
using spawn_win32_named_pipe  = basic_connection<dialers::spawn_win32_named_pipe>;
#endif

} // namespace connections


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif // connection.hh
