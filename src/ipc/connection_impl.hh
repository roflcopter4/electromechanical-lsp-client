// ReSharper disable CppLocalVariableMayBeConst
#pragma once
#ifndef HGUARD__IPC__CONNECTION_IMPL_HH_
#define HGUARD__IPC__CONNECTION_IMPL_HH_ //NOLINT

#include "Common.hh"
#include "util/exceptions.hh"

#define WIN32_USE_PIPE_IMPL

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


namespace except {

class connection_closed final : public std::runtime_error
{
      std::string text_;
      connection_closed(std::string const &message, char const *function)
          : std::runtime_error("Connection closed")
      {
            text_ = message + " : "s + function;
      }
      connection_closed(char const *message, char const *function)
          : connection_closed(std::string(message), function)
      {}
    public:
      connection_closed() : connection_closed("Connection closed"s, FUNCTION_NAME)
      {}
      explicit connection_closed(char const *message)
          : connection_closed(message, FUNCTION_NAME)
      {}
      explicit connection_closed(std::string const &message)
          : connection_closed(message, FUNCTION_NAME)
      {}
      ND char const *what() const noexcept override { return text_.c_str(); }
};

} // namespace except


/****************************************************************************************/


namespace detail {

#ifdef _WIN32
using procinfo_t = PROCESS_INFORMATION;
#else
using procinfo_t = pid_t;
#endif

/*--------------------------------------------------------------------------------------*/

template <typename DescriptorType>
class base_connection_interface
{
#ifdef _WIN32
      using err_descriptor_type = HANDLE;
      static constexpr HANDLE invalidate_err_handle() { return reinterpret_cast<HANDLE>(-1); }
#else
      using err_descriptor_type = int;
      static constexpr int invalidate_err_handle() { return (-1); }
#endif

    public:
      using descriptor_type = DescriptorType;

    protected:
      enum class sink_type {
            DEFAULT,
            DEVNULL,
            FILENAME,
            FD
      } err_sink_type_ = sink_type::DEFAULT;

      std::string         fname_  = {};
      err_descriptor_type err_fd_ = invalidate_err_handle();

#ifdef _WIN32
      static constexpr char dev_null[] = "NUL";
#else
      static constexpr char dev_null[] = "/dev/null";
#endif
      static constexpr int default_read_flags  = MSG_WAITALL;
      static constexpr int default_write_flags = 0;

    private:
      bool initialized_ = false;

    public:
      base_connection_interface() = default;

      virtual ~base_connection_interface()
      {
            if (err_sink_type_ == sink_type::FILENAME)
                  util::close_descriptor(err_fd_);
      }

      DEFAULT_COPY_CTORS(base_connection_interface);
      base_connection_interface &
      operator=(base_connection_interface &&) noexcept = default;

      base_connection_interface(base_connection_interface &&other) noexcept
          : err_sink_type_ (other.err_sink_type_)
          , fname_ (std::move(other.fname_))
          , err_fd_ (other.err_fd_)
          , initialized_ (other.initialized_)
      {
            other.err_sink_type_ = sink_type::DEFAULT;
            other.err_fd_        = invalidate_err_handle();
      }

      //--------------------------------------------------------------------------------

      void set_stderr_default()
      {
            err_sink_type_ = sink_type::DEFAULT;
      }

      void set_stderr_devnull()
      {
            err_sink_type_ = sink_type::DEVNULL;
      }

      void set_stderr_fd(err_descriptor_type const fd)
      {
            err_sink_type_ = sink_type::FD;
            err_fd_ = fd;
      }

      void set_stderr_filename(std::string const &fname)
      {
            if (err_sink_type_ == sink_type::FILENAME || err_sink_type_ == sink_type::FD)
                  util::close_descriptor(err_fd_);
            fname_         = fname;
            err_sink_type_ = sink_type::FILENAME;
      }

      virtual ssize_t read(void *buf, ssize_t nbytes)        { return read (buf, nbytes, default_read_flags); }
      virtual ssize_t write(void const *buf, ssize_t nbytes) { return write(buf, nbytes, default_write_flags); }

      virtual ssize_t    read (void       *buf, ssize_t nbytes, int flags) = 0;
      virtual ssize_t    write(void const *buf, ssize_t nbytes, int flags) = 0;
      virtual void       close()                                           = 0;
      virtual procinfo_t do_spawn_connection(size_t argc, char **argv)     = 0;

      ND virtual DescriptorType fd()        const noexcept                 = 0;
      ND virtual size_t         available() const noexcept(false)          = 0;

      //--------------------------------------------------------------------------------

    protected:
      ND err_descriptor_type get_err_handle();

      void set_to_initialized() { initialized_ = true; }

      void throw_if_initialized(std::type_info const &id) const noexcept(false)
      {
            if (!initialized_)
                  return;
            throw std::logic_error{
                fmt::format(FC("Error: Attempt to initialize already "
                               "initialized instance of class \"{}\"."),
                            util::demangle(id))};
      }
};

template <typename T>
concept IsConnectionImplVariant =
    std::derived_from<T, detail::base_connection_interface<typename T::descriptor_type>>;

/*--------------------------------------------------------------------------------------*/

template <typename AddrType>
class socket_connection_base_impl : public base_connection_interface<socket_t>
{
      using this_type = socket_connection_base_impl;
      using base_type = base_connection_interface<socket_t>;
#ifdef _WIN32
      using sock_int_type = int;
#else
      using sock_int_type = size_t;
#endif
      bool should_close_listener_ = true;

    protected:
      socket_t listener_ = static_cast<socket_t>(-1);
      socket_t acceptor_ = static_cast<socket_t>(-1);
      AddrType addr_     = {};

      virtual socket_t open_new_socket()   = 0;
      virtual socket_t connect_to_socket() = 0;

    public:
      socket_connection_base_impl() = default;
      explicit socket_connection_base_impl(socket_t const sock)
            : acceptor_(sock)
      {}

      ~socket_connection_base_impl() override
      {
            close();
      }

      virtual void set_listener(socket_t sock, AddrType const &addr) = 0;
      bool        &should_close_listnener() &       { return should_close_listener_; }
      bool const  &should_close_listnener() const & { return should_close_listener_; }

      DELETE_COPY_CTORS(socket_connection_base_impl);
      socket_connection_base_impl &
      operator=(socket_connection_base_impl &&) noexcept = default;

      socket_connection_base_impl(socket_connection_base_impl &&other) noexcept
          : base_type(std::move(other)),
            listener_(other.listener_),
            acceptor_(other.acceptor_),
            addr_(std::move(other.addr_))
      {
            other.listener_ = static_cast<socket_t>(-1);
            other.acceptor_ = static_cast<socket_t>(-1);
      }

      //--------------------------------------------------------------------------------

      using base_type::read;
      using base_type::write;

      ssize_t read (void       *buf, ssize_t nbytes, int flags) final;
      ssize_t write(void const *buf, ssize_t nbytes, int flags) final;
      void    close() final;

      ND descriptor_type fd() const noexcept final
      {
            return acceptor_;
      }

      ND size_t available() const noexcept(false) override
      {
            return util::available_in_fd(acceptor_);
      }

      ND virtual AddrType const &addr() const & { return addr_; }
};

class unix_socket_connection_impl final : public socket_connection_base_impl<sockaddr_un>
{
    public:
      using this_type = unix_socket_connection_impl;
      using base_type = socket_connection_base_impl<sockaddr_un>;

    private:
      std::string path_;

    public:
      unix_socket_connection_impl()           = default;
      ~unix_socket_connection_impl() override = default;

      void set_listener(socket_t const sock, sockaddr_un const &addr) override
      {
            should_close_listnener() = false;
            this->listener_ = sock;
            this->addr_     = addr;
      }

      DELETE_COPY_CTORS(unix_socket_connection_impl);
      unix_socket_connection_impl &
      operator=(this_type &&) noexcept = default;

      unix_socket_connection_impl(this_type &&other) noexcept
            : base_type(std::move(other))
            , path_(std::move(other.path_))
      {}

      //--------------------------------------------------------------------------------

      procinfo_t do_spawn_connection(size_t argc, char **argv) override;

    private:
      socket_t open_new_socket()   override;
      socket_t connect_to_socket() override;
};

#ifdef HAVE_SOCKETPAIR
class socketpair_connection_impl final : public socket_connection_base_impl<sockaddr_un>
{
    public:
      using this_type = socketpair_connection_impl;
      using base_type = socket_connection_base_impl<sockaddr_un>;

    private:
      socket_t fds_[2] = {-1, -1};

    public:
      socketpair_connection_impl()           = default;
      ~socketpair_connection_impl() override = default;

      [[deprecated("Don't use this function. It will just crash. On purpose.")]]
      void set_listener(socket_t sock, sockaddr_un const &addr) override
      {
            throw std::logic_error(
                "This function makes no sense at all for this type of socket.");
      }

      DELETE_COPY_CTORS(socketpair_connection_impl);
      socketpair_connection_impl &
      operator=(socketpair_connection_impl &&) noexcept = default;

      socketpair_connection_impl(socketpair_connection_impl &&other) noexcept
          : base_type(std::move(other))
      {
            ::memcpy(fds_, other.fds_, sizeof(fds_));
            other.fds_[0] = (-1);
            other.fds_[1] = (-1);
      }

      //--------------------------------------------------------------------------------

      procinfo_t do_spawn_connection(size_t argc, char **argv) override;

    private:
      socket_t open_new_socket()   override;
      socket_t connect_to_socket() override;
};
#endif

/*--------------------------------------------------------------------------------------*/

class fd_connection_impl;

class pipe_connection_impl : public base_connection_interface<int>
{
    public:
      using this_type = pipe_connection_impl;
      using base_type = base_connection_interface<int>;

    private:
      int volatile read_  = (-1);
      int volatile write_ = (-1);

      friend class fd_connection_impl;

    public:
      pipe_connection_impl() = default;
      ~pipe_connection_impl() override
      {
            close();
      }

      DELETE_COPY_CTORS(pipe_connection_impl);
      pipe_connection_impl &operator=(pipe_connection_impl &&) noexcept = default;
      pipe_connection_impl(pipe_connection_impl &&other) noexcept
          : base_type(std::move(other))
          , read_(other.read_)
          , write_(other.write_)
      {
            other.read_  = (-1);
            other.write_ = (-1);
      }

      //--------------------------------------------------------------------------------

      using base_type::read;
      using base_type::write;

      ssize_t read (void       *buf, ssize_t nbytes, int flags) final;
      ssize_t write(void const *buf, ssize_t nbytes, int flags) final;

      void       close() final;
      procinfo_t do_spawn_connection(size_t argc, char **argv) override;

      ND descriptor_type fd()        const noexcept final { return read_; }
      ND size_t          available() const noexcept final { return util::available_in_fd(read_); }

      ND virtual int read_fd()  const noexcept { return read_; }
      ND virtual int write_fd() const noexcept { return write_; }

};


class fd_connection_impl final : public pipe_connection_impl
{
#define ERRMSG "The fd_connection class is intended to be attached to stdin and stdout. It cannot spawn a process."

    public:
      using this_type = fd_connection_impl;
      using base_type = pipe_connection_impl;

      fd_connection_impl()           = default;
      ~fd_connection_impl() override = default;

      fd_connection_impl(int const readfd, int const writefd)
      {
            set_descriptors(readfd, writefd);
            set_to_initialized();
      }

      DELETE_COPY_CTORS(fd_connection_impl);
      DEFAULT_MOVE_CTORS(fd_connection_impl);

      //--------------------------------------------------------------------------------

#if defined __GNUC__ && !defined __clang__
      __attribute__((__error__(ERRMSG)))
#endif
      [[deprecated(ERRMSG)]]
      procinfo_t do_spawn_connection(size_t, char **) override
      {
            throw std::logic_error(ERRMSG);
      }

#if !defined __GNUC__ || defined __clang__
      /* HACK: Stupid ugly hack to make any attempt to call this function fail. */
      template <typename ...Types>
      procinfo_t do_spawn_connection(Types...)
      {
            static_assert(std::is_constructible_v<Types...>, ERRMSG);
            throw std::logic_error(ERRMSG);
      }
#endif

      void set_descriptors(int const readfd, int const writefd)
      {
            read_  = readfd;
            write_ = writefd;
      }

      static fd_connection_impl make_from_std_handles() { return {0, 1}; }

#undef ERRMSG
};

/*--------------------------------------------------------------------------------------*/

#if defined _WIN32 && defined WIN32_USE_PIPE_IMPL

/**
 * XXX BUG: Totally broken.
 */
class win32_named_pipe_impl final : public base_connection_interface<HANDLE>
{
      HANDLE      pipe_ = nullptr;
      std::string pipe_fname_;

      OVERLAPPED  overlapped_ = {};
      std::mutex  mtx_ = {};

    public:
      win32_named_pipe_impl() = default;
      ~win32_named_pipe_impl() override
      {
            close();
      }

      void close() override
      {
            if (pipe_)
                  CloseHandle(pipe_);
            pipe_ = nullptr;
            pipe_fname_ = {};
      }

      procinfo_t do_spawn_connection(size_t argc, char **argv) override;

      ssize_t read (void       *buf, ssize_t const nbytes) override { return read(buf, nbytes, 0); }
      ssize_t write(void const *buf, ssize_t const nbytes) override { return write(buf, nbytes, 0); }
      ssize_t read (void       *buf, ssize_t nbytes, int flags) override;
      ssize_t write(void const *buf, ssize_t nbytes, int flags) override;

      ND HANDLE fd() const noexcept override { return pipe_; }
      ND size_t available() const noexcept(false) override { throw util::except::not_implemented(); }

      DELETE_COPY_CTORS(win32_named_pipe_impl);
      DEFAULT_MOVE_CTORS(win32_named_pipe_impl);
};

#endif

/****************************************************************************************/

#ifdef _WIN32
template <typename DescriptorType>
HANDLE
base_connection_interface<DescriptorType>::get_err_handle()
{
      HANDLE err_handle;

      switch (err_sink_type_) {
      case sink_type::DEFAULT:
            err_handle = GetStdHandle(STD_ERROR_HANDLE);
            break;
      case sink_type::DEVNULL:
            err_handle = CreateFileA(dev_null, GENERIC_WRITE, 0, nullptr,
                                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            err_fd_ = err_handle;
            break;
      case sink_type::FD:
            err_handle = err_fd_;
            break;
      case sink_type::FILENAME:
            err_handle = CreateFileA(fname_.c_str(), GENERIC_WRITE, 0, nullptr,
                                     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            err_fd_ = err_handle;
            break;
      default:
            throw std::logic_error(fmt::format(FC("Invalid value for err_sink_type_: {}"),
                                               static_cast<int>(err_sink_type_)));
      }

      return err_handle;
}

#else

template <typename DescriptorType>
DescriptorType
base_connection_interface<DescriptorType>::get_err_handle() const
{
      int fd;

      switch (err_sink_type_) {
      case sink_type::DEVNULL:
            fd = ::open(dev_null, O_WRONLY|O_APPEND);
            err_fd_ = fd;
            break;
      case sink_type::FILENAME:
            fd = ::open(fname_.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0600);
            err_fd_ = fd;
            break;
      case sink_type::FD:
            fd = err_fd_;
            break;
      case sink_type::DEFAULT:
            fd = STDERR_FILENO;
            break;
      default:
            fd = static_cast<DescriptorType>(-1);
            break;
      }

      return fd;
}
#endif

/*--------------------------------------------------------------------------------------*/

template <typename AddrType>
void socket_connection_base_impl<AddrType>::close()
{
      if (acceptor_ != static_cast<socket_t>(-1)) {
            shutdown(acceptor_, 2);
            util::close_descriptor(acceptor_);
      }
      if (listener_ != static_cast<socket_t>(-1) && should_close_listener_) {
            shutdown(listener_, 2);
            util::close_descriptor(listener_);
      }
}

template <typename AddrType>
ssize_t
socket_connection_base_impl<AddrType>::read(void         *buf,
                                            ssize_t const nbytes,
                                            int     const flags)
{
      ssize_t total = 0;
      ssize_t n     = 0;

      if (flags & MSG_WAITALL) {
            // Put it in a loop just in case...
            do {
                  n = ::recv(acceptor_, static_cast<char *>(buf) + total,
                             static_cast<sock_int_type>(nbytes - total), flags);
            } while ((n != (-1)) && (total += n) < static_cast<ssize_t>(nbytes));
      } else {
            while (available() == 0)
                  std::this_thread::sleep_for(10ms);
            n = ::recv(acceptor_, static_cast<char *>(buf), static_cast<sock_int_type>(nbytes), flags);
            total = n;
      }

      if (n == (-1)) [[unlikely]] {
            if (errno == EBADF)
                  throw except::connection_closed();
            err(1, "recv() failed");
      }

      return total;
}

template <typename AddrType>
ssize_t
socket_connection_base_impl<AddrType>::write(void    const *buf,
                                             ssize_t const  nbytes,
                                             int     const  flags)
{
      ssize_t total = 0;
      ssize_t n;

      do {
            n = ::send(acceptor_, static_cast<char const *>(buf) + total,
                       static_cast<sock_int_type>(nbytes - total), flags);
      } while (n != (-1) && (total += n) < static_cast<ssize_t>(nbytes));

      if (n == (-1)) [[unlikely]] {
            if (errno == EBADF)
                  throw except::connection_closed();
            err(1, "send() failed");
      }

      return total;
}


/****************************************************************************************/
} // namespace detail
} // namespace ipc
} // namespace emlsp
#endif
