// ReSharper disable CppLocalVariableMayBeConst
// ReSharper disable CppClangTidyPerformanceNoIntToPtr
#pragma once
#ifndef HGUARD__IPC__CONNECTION_IMPL_HH_
#define HGUARD__IPC__CONNECTION_IMPL_HH_ //NOLINT

#include "Common.hh"
#include "ipc/misc.hh"

#define WIN32_USE_PIPE_IMPL

inline namespace emlsp {
namespace ipc::detail {
/****************************************************************************************/

#ifdef _WIN32
using procinfo_t = PROCESS_INFORMATION;
#else
using procinfo_t = pid_t;
#endif

/****************************************************************************************/
/* ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
   ┃  ┌───────────────────────────────────────────────┐                               ┃
   ┃  │Base interface for an automatic ipc connection.│                               ┃
   ┃  └───────────────────────────────────────────────┘                               ┃
   ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛ */

template <typename DescriptorType>
class base_connection_interface
{
#ifdef _WIN32
      using err_descriptor_type = HANDLE;
      static constexpr HANDLE invalid_err_handle() { return reinterpret_cast<HANDLE>(-1); }
      static constexpr char dev_null[] = "NUL";
#else
      using err_descriptor_type = int;
      static constexpr int invalid_err_handle() { return (-1); }
      static constexpr char dev_null[] = "/dev/null";
#endif

      bool open_listener_  = true;
      uint8_t initialized_ = 0;

    protected:
      enum class sink_type : uint8_t {
            DEFAULT,
            DEVNULL,
            FILENAME,
            FD
      };

      sink_type           err_sink_type_ = sink_type::DEFAULT;
      err_descriptor_type err_fd_        = invalid_err_handle();
      std::string         fname_         = {};
      std::mutex          read_mtx_      = {};
      std::mutex          write_mtx_     = {};

      static constexpr int default_read_flags  = MSG_WAITALL;
      static constexpr int default_write_flags = 0;

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
          : initialized_ (other.initialized_)
          , err_sink_type_ (other.err_sink_type_)
          , err_fd_ (other.err_fd_)
          , fname_ (std::move(other.fname_))
      {
            other.err_sink_type_ = sink_type::DEFAULT;
            other.err_fd_        = invalid_err_handle();
      }

      using descriptor_type = DescriptorType;

      //--------------------------------------------------------------------------------

      void set_stderr_default()
      {
            if (err_sink_type_ == sink_type::FILENAME)
                  util::close_descriptor(err_fd_);
            err_sink_type_ = sink_type::DEFAULT;
      }

      void set_stderr_devnull()
      {
            if (err_sink_type_ == sink_type::FILENAME)
                  util::close_descriptor(err_fd_);
            err_sink_type_ = sink_type::DEVNULL;
      }

      void set_stderr_fd(err_descriptor_type const fd)
      {
            if (err_sink_type_ == sink_type::FILENAME)
                  util::close_descriptor(err_fd_);
            err_sink_type_ = sink_type::FD;
            err_fd_        = fd;
      }

      void set_stderr_filename(std::string const &fname)
      {
            if (err_sink_type_ == sink_type::FILENAME)
                  util::close_descriptor(err_fd_);
            err_sink_type_ = sink_type::FILENAME;
            fname_         = fname;
      }

      void should_open_listener(bool const val) { open_listener_ = val; }

      virtual ssize_t read(void *buf, ssize_t nbytes)        { return read (buf, nbytes, default_read_flags); }
      virtual ssize_t write(void const *buf, ssize_t nbytes) { return write(buf, nbytes, default_write_flags); }

      virtual ssize_t    read (void       *buf, ssize_t nbytes, int flags) = 0;
      virtual ssize_t    write(void const *buf, ssize_t nbytes, int flags) = 0;
      virtual void       open()                                            = 0;
      virtual void       close()                                           = 0;
      virtual procinfo_t do_spawn_connection(size_t argc, char **argv)     = 0;

      ND virtual DescriptorType fd()        const noexcept                 = 0;
      ND virtual size_t         available() const noexcept(false)          = 0;

      //--------------------------------------------------------------------------------

    protected:
      ND err_descriptor_type get_err_handle();
      ND bool should_open_listener() const { return open_listener_; }
      ND bool check_initialized(uint8_t const val) const { return initialized_ >= val; }
      void    set_initialized(uint8_t const val = 1) { initialized_ = val; }

      void throw_if_initialized(std::type_info const &id, uint8_t const val = 1) const noexcept(false)
      {
            if (check_initialized(val))
                  throw std::logic_error{
                      fmt::format(FC("Error: Attempt to initialize already initialized "
                                     "instance of class \"{}\", to level {}."),
                                  util::demangle(id), val)};
      }
};


template <typename T>
concept IsConnectionImplVariant =
    std::derived_from<T, base_connection_interface<typename T::descriptor_type>>;


/****************************************************************************************/
/* ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
   ┃  ┌──────────────────────────────────────────────────┐                            ┃
   ┃  │Derivative interface for socket based connections.│                            ┃
   ┃  └──────────────────────────────────────────────────┘                            ┃
   ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛ */


#ifdef _WIN32
# define ERRNO WSAGetLastError()
# define ERR(n, t) util::win32::error_exit_wsa(L ## t)
#else
# define ERRNO errno
# define ERR(n, t) err(n, t)
#endif


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
      bool should_open_listener_  = true;
      bool should_close_listener_ = true;
      bool should_connect_        = true;

    protected:
      socket_t  listener_  = static_cast<socket_t>(-1);
      socket_t  connector_ = static_cast<socket_t>(-1);
      socket_t  acceptor_  = static_cast<socket_t>(-1);
      socket_t &main_fd_   = acceptor_;
      AddrType  addr_      = {};

    public:
      using addr_type = AddrType;

      socket_connection_base_impl() = default;
      explicit socket_connection_base_impl(socket_t const sock)
            : acceptor_(sock)
      {}

      ~socket_connection_base_impl() override
      {
            close();
      }

      DELETE_COPY_CTORS(socket_connection_base_impl);
      socket_connection_base_impl &
      operator=(socket_connection_base_impl &&) noexcept = default;

      socket_connection_base_impl(socket_connection_base_impl &&other) noexcept
          : base_type (std::move(other)),
            listener_ (other.listener_),
            acceptor_ (other.acceptor_),
            addr_ (std::move(other.addr_))
      {
            other.listener_ = static_cast<socket_t>(-1);
            other.acceptor_ = static_cast<socket_t>(-1);
      }

      //--------------------------------------------------------------------------------


      bool &should_open_listener()   & { return should_open_listener_; }
      bool &should_close_listnener() & { return should_close_listener_; }
      bool &should_connect()         & { return should_connect_; }

      using base_type::read;
      using base_type::write;

      ssize_t read (void       *buf, ssize_t nbytes, int flags) final;
      ssize_t write(void const *buf, ssize_t nbytes, int flags) final;
      void    close() final;

      ND size_t available() const noexcept(false) override
      {
            return util::available_in_fd(main_fd_);
      }

      ND socket_t fd() const noexcept final { return main_fd_; }
      ND socket_t listener() const { return listener_; }
      ND socket_t peer() const { return connector_; }
      ND socket_t acceptor() const { return acceptor_; }

      ND virtual AddrType &addr() & { return addr_; }

      virtual socket_t accept()  = 0;
      virtual socket_t connect() = 0;
      virtual void set_listener(socket_t sock, AddrType const &addr) = 0;

    protected:
      virtual socket_t open_new_socket()      = 0;
      virtual socket_t connect_to_socket()    = 0;
      virtual socket_t get_connected_socket() = 0;
};


/*======================================================================================*/


class unix_socket_connection_impl final : public socket_connection_base_impl<sockaddr_un>
{
      using this_type = unix_socket_connection_impl;
      using base_type = socket_connection_base_impl<sockaddr_un>;

      std::string path_;

    public:
      unix_socket_connection_impl()           = default;
      ~unix_socket_connection_impl() override = default;

      DELETE_COPY_CTORS(unix_socket_connection_impl);
      unix_socket_connection_impl &
      operator=(this_type &&) noexcept = default;

      unix_socket_connection_impl(this_type &&other) noexcept
            : base_type(std::move(other))
            , path_(std::move(other.path_))
      {}

      //--------------------------------------------------------------------------------

      void set_address(std::string path);

      socket_t   connect() override;
      socket_t   accept() override;
      void       open() override;
      procinfo_t do_spawn_connection(size_t argc, char **argv) override;
      void       set_listener(socket_t sock, sockaddr_un const &addr) override;

      ND auto const &path() const & { return path_; }

    protected:
      socket_t open_new_socket()   override;
      socket_t connect_to_socket() override;
      socket_t get_connected_socket() override { return connect_to_socket(); }
};


/*======================================================================================*/


#ifdef HAVE_SOCKETPAIR
class socketpair_connection_impl final : public socket_connection_base_impl<sockaddr_un>
{
      using this_type = socketpair_connection_impl;
      using base_type = socket_connection_base_impl<sockaddr_un>;

      socket_t fds_[2] = {-1, -1};

    public:
      socketpair_connection_impl()           = default;
      ~socketpair_connection_impl() override = default;

      [[deprecated("Don't use this function. It will just crash. On purpose.")]]
      void set_listener(socket_t /*sock*/, sockaddr_un const & /*addr*/) override
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
            memcpy(fds_, other.fds_, sizeof(fds_));
            other.fds_[0] = (-1);
            other.fds_[1] = (-1);
      }

      //--------------------------------------------------------------------------------

      procinfo_t do_spawn_connection(size_t argc, char **argv) override;

    protected:
      socket_t open_new_socket()   override;
      socket_t connect_to_socket() override;
      socket_t get_connected_socket() override { return connect_to_socket(); }
};
#endif


/*======================================================================================*/
/* TCP/IP */


extern socket_t connect_to_inet_socket(sockaddr const *addr, socklen_t size, int type);


template <typename AddrType>
      REQUIRES (IsInetSockaddr<AddrType>;)
class inet_socket_connection_base_impl : public socket_connection_base_impl<AddrType>
{
      using this_type = inet_socket_connection_base_impl;
      using base_type = socket_connection_base_impl<AddrType>;

    protected:
      bool addr_init_ = false;

    public:
      inet_socket_connection_base_impl()           = default;
      ~inet_socket_connection_base_impl() override = default;

      DELETE_COPY_CTORS(inet_socket_connection_base_impl);
      DEFAULT_MOVE_CTORS(inet_socket_connection_base_impl);

      //--------------------------------------------------------------------------------

      void set_listener(socket_t const sock, AddrType const &addr) override
      {
            this->should_close_listnener() = true;
            this->listener_ = sock;
            this->addr_     = addr;
      }

      socket_t connect() override
      {
            if (!this->check_initialized(1))
                  throw std::logic_error("Cannot connect to uninitialized address!");

            this->connector_ = connect_to_inet_socket(get_addr_generic(), get_socklen(), get_type());
            this->main_fd_   = this->connector_;
            return this->connector_;
      }

      socket_t accept() override
      {
            if (!this->check_initialized(1))
                  throw std::logic_error("Cannot accept from uninitialized address!");
            socklen_t size  = get_socklen();
            this->acceptor_ = ::accept(this->listener_, const_cast<sockaddr *>(get_addr_generic()), &size);
            if (static_cast<intptr_t>(this->acceptor_) < 0 || size != get_socklen())
                  ERR(1, "accept");

            this->main_fd_ = this->acceptor_;
            return this->acceptor_;
      }

    protected:
      socket_t get_connected_socket() override { return this->connector_; }
      ND virtual sockaddr const *get_addr_generic() const = 0;
      ND virtual socklen_t       get_socklen() const      = 0;
      ND virtual int             get_type() const         = 0;
};

/*--------------------------------------------------------------------------------------*/
/* Either */

class inet_any_socket_connection_impl final
    : public inet_socket_connection_base_impl<sockaddr *>
{
      using this_type = inet_any_socket_connection_impl;
      using base_type = socket_connection_base_impl<addr_type>;

      socklen_t addr_length_ = 0;
      int       type_        = AF_UNSPEC;

    public:
      inet_any_socket_connection_impl() 
      {
            addr_ = nullptr;
      }

      ~inet_any_socket_connection_impl() override
      {
            free(this->addr_); //NOLINT
      }

      DEFAULT_MOVE_CTORS(inet_any_socket_connection_impl);
      DELETE_COPY_CTORS(inet_any_socket_connection_impl);

      //--------------------------------------------------------------------------------

      int resolve(char const *server_name, char const *port);
      int resolve(char const *server_and_port);

      void assign_from_addrinfo(addrinfo const *info);

      void       open() override;
      procinfo_t do_spawn_connection(size_t argc, char **argv) override;

    protected:
      socket_t open_new_socket()   override;
      socket_t connect_to_socket() override;

      ND sockaddr const *get_addr_generic() const override { return addr_; }
      ND socklen_t       get_socklen() const override { return addr_length_; }
      ND int             get_type() const override { return type_; }

    private:
      void set_listener(socket_t const /*sock*/, sockaddr *const & /*addr*/) override
      {}
};

/*--------------------------------------------------------------------------------------*/
/* IPv4 only */

class inet_ipv4_socket_connection_impl final
    : public inet_socket_connection_base_impl<sockaddr_in>
{
      using this_type = inet_ipv4_socket_connection_impl;
      using base_type = socket_connection_base_impl<addr_type>;

    public:
      inet_ipv4_socket_connection_impl()           = default;
      ~inet_ipv4_socket_connection_impl() override = default;
      DEFAULT_MOVE_CTORS(inet_ipv4_socket_connection_impl);
      DELETE_COPY_CTORS(inet_ipv4_socket_connection_impl);

      //--------------------------------------------------------------------------------

      void       open() override;
      procinfo_t do_spawn_connection(size_t argc, char **argv) override;

    protected:
      socket_t open_new_socket()   override;
      socket_t connect_to_socket() override;

      ND sockaddr const *get_addr_generic() const override { return reinterpret_cast<sockaddr const *>(&addr_); }
      ND socklen_t       get_socklen() const override { return sizeof(sockaddr_in); }
      ND int             get_type() const override { return AF_INET; }
};

/*--------------------------------------------------------------------------------------*/
/* IPv6 only */

class inet_ipv6_socket_connection_impl final
    : public inet_socket_connection_base_impl<sockaddr_in6>
{
      using this_type = inet_ipv6_socket_connection_impl;
      using base_type = socket_connection_base_impl<addr_type>;

    public:
      inet_ipv6_socket_connection_impl()           = default;
      ~inet_ipv6_socket_connection_impl() override = default;
      DEFAULT_MOVE_CTORS(inet_ipv6_socket_connection_impl);
      DELETE_COPY_CTORS(inet_ipv6_socket_connection_impl);

      //--------------------------------------------------------------------------------

      void       open() override;
      procinfo_t do_spawn_connection(size_t argc, char **argv) override;

    protected:
      socket_t open_new_socket()   override;
      socket_t connect_to_socket() override;

      ND sockaddr const *get_addr_generic() const override { return reinterpret_cast<sockaddr const *>(&addr_); }
      ND socklen_t       get_socklen() const override { return sizeof(sockaddr_in6); }
      ND int             get_type() const override { return AF_INET6; }
};


#undef ERR
#undef ERRNO


/****************************************************************************************/
/* ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
   ┃  ┌────────────────────────────────────────────────────────┐                      ┃
   ┃  │Derivative interface for fd or HANDLE based connections.│                      ┃
   ┃  └────────────────────────────────────────────────────────┘                      ┃
   ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛ */

class fd_connection_impl;

/*
 * For regular pipes.
 */
class pipe_connection_impl : public base_connection_interface<int>
{
      using this_type = pipe_connection_impl;
      using base_type = base_connection_interface<int>;
      friend class fd_connection_impl;

      int volatile read_  = (-1);
      int volatile write_ = (-1);

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

      void       open() final { /* Do nothing. This method makes no sense for pipes. */ }
      void       close() final;
      procinfo_t do_spawn_connection(size_t argc, char **argv) override;

      ND descriptor_type fd()        const noexcept final { return read_; }
      ND size_t          available() const noexcept final { return util::available_in_fd(read_); }

      ND virtual int read_fd()  const noexcept { return read_; }
      ND virtual int write_fd() const noexcept { return write_; }
};

/*--------------------------------------------------------------------------------------*/

/*
 * For any arbitrary file descriptors, including stdin/stdout.
 */
class fd_connection_impl final : public pipe_connection_impl
{
#define ERRMSG "The fd_connection class is intended to be attached to stdin and stdout. It cannot spawn a process."

      using this_type = fd_connection_impl;
      using base_type = pipe_connection_impl;

    public:
      fd_connection_impl()           = default;
      ~fd_connection_impl() override = default;

      fd_connection_impl(int const readfd, int const writefd)
      {
            set_descriptors(readfd, writefd);
            set_initialized();
      }

#ifdef _WIN32
      fd_connection_impl(HANDLE readfd, HANDLE writefd)
      {
            set_descriptors(_open_osfhandle(reinterpret_cast<intptr_t>(readfd),
                                            _O_RDONLY | _O_BINARY),
                            _open_osfhandle(reinterpret_cast<intptr_t>(writefd),
                                            _O_WRONLY | _O_BINARY));
            set_initialized();
      }
#endif

      DELETE_COPY_CTORS(fd_connection_impl);
      DEFAULT_MOVE_CTORS(fd_connection_impl);

      //--------------------------------------------------------------------------------

    private:
      procinfo_t do_spawn_connection(size_t /*argc*/, char ** /*argv*/) override
      {
            throw std::logic_error(ERRMSG);
      }

    public:
      void set_descriptors(int const readfd, int const writefd)
      {
            read_  = readfd;
            write_ = writefd;
      }

      static fd_connection_impl make_from_std_handles() { return {0, 1}; }

#undef ERRMSG
};


/****************************************************************************************/


#ifdef _WIN32

class pipe_handle_connection_impl : public base_connection_interface<HANDLE>
{
      using this_type = pipe_connection_impl;
      using base_type = base_connection_interface<HANDLE>;
      friend class fd_connection_impl;

      HANDLE volatile read_  = reinterpret_cast<HANDLE>(-1);
      HANDLE volatile write_ = reinterpret_cast<HANDLE>(-1);

    public:
      pipe_handle_connection_impl() = default;
      ~pipe_handle_connection_impl() override
      {
            close();
      }

      DELETE_COPY_CTORS(pipe_handle_connection_impl);
      pipe_handle_connection_impl &operator=(pipe_handle_connection_impl &&) noexcept = default;
      pipe_handle_connection_impl(pipe_handle_connection_impl &&other) noexcept
          : base_type(std::move(other))
          , read_(other.read_)
          , write_(other.write_)
      {
            other.read_  = reinterpret_cast<HANDLE>(-1);
            other.write_ = reinterpret_cast<HANDLE>(-1);
      }

      //--------------------------------------------------------------------------------

      using base_type::read;
      using base_type::write;

      ssize_t read (void       *buf, ssize_t nbytes, int flags) final;
      ssize_t write(void const *buf, ssize_t nbytes, int flags) final;

      void       open() final { /* Do nothing. This method makes no sense for pipes. */ }
      void       close() final;
      procinfo_t do_spawn_connection(size_t argc, char **argv) override;

      ND descriptor_type fd()        const noexcept final { return read_; }
      ND size_t          available() const noexcept final { return util::available_in_fd(read_); }

      ND virtual HANDLE read_fd()  const noexcept { return read_; }
      ND virtual HANDLE write_fd() const noexcept { return write_; }

      void set_descriptors(HANDLE const readfd, HANDLE const writefd)
      {
            set_initialized(1);
            read_  = readfd;
            write_ = writefd;
      }
};

#endif


/****************************************************************************************/
/* ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
   ┃  ┌──────────────────────────────────────────────────────────┐                    ┃
   ┃  │Broken interface for Windows named pipe based connections.│                    ┃
   ┃  └──────────────────────────────────────────────────────────┘                    ┃
   ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛ */

#if defined _WIN32 && defined WIN32_USE_PIPE_IMPL

/**
 * XXX BUG: Totally broken.
 */
class win32_named_pipe_impl final : public base_connection_interface<HANDLE>
{
      HANDLE       pipe_ = nullptr;
      std::wstring pipe_fname_;

      OVERLAPPED  overlapped_ = {};

    public:
      win32_named_pipe_impl()
      {
            overlapped_.hEvent = CreateEvent(nullptr, true, true, nullptr);
      }
      ~win32_named_pipe_impl() override
      {
            close();
            CloseHandle(overlapped_.hEvent);
      }

      void close() override
      {
            if (pipe_) {
                  CloseHandle(pipe_);
                  DeleteFileW(pipe_fname_.c_str());
            }
            pipe_       = nullptr;
            pipe_fname_ = {};
      }

      void       open() override;
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
/****************************************************************************************/
/* Out of line implementations for the base interface. */

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
int
base_connection_interface<DescriptorType>::get_err_handle()
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
/* Out of line implementations for socket connections. */

template <typename AddrType>
void socket_connection_base_impl<AddrType>::close()
{
      if (acceptor_ != static_cast<socket_t>(-1)) {
            ::shutdown(acceptor_, 2);
            util::close_descriptor(acceptor_);
      }
      if (connector_ != static_cast<socket_t>(-1)) {
            ::shutdown(acceptor_, 2);
            util::close_descriptor(connector_);
      }
      if (listener_ != static_cast<socket_t>(-1) && should_close_listener_) {
            ::shutdown(listener_, 2);
            util::close_descriptor(listener_);
      }
}

template <typename AddrType>
ssize_t
socket_connection_base_impl<AddrType>::read(void         *buf,
                                            ssize_t const nbytes,
                                            int           flags)
{
      auto    lock  = std::lock_guard{read_mtx_};
      ssize_t total = 0;
      ssize_t n     = 0;

      if (flags & MSG_WAITALL) {
            // Put it in a loop just in case...
            flags &= ~MSG_WAITALL;
            do {
                  n = ::recv(main_fd_, static_cast<char *>(buf) + total,
                             static_cast<sock_int_type>(nbytes - total), flags);
            } while ((n != (-1)) && (total += n) < static_cast<ssize_t>(nbytes));
      } else {
            //while (available() == 0)
            for (;;) {
                  n = ::recv(main_fd_, static_cast<char *>(buf), static_cast<sock_int_type>(nbytes), flags);
                  if (n != 0)
                        break;
                  std::this_thread::sleep_for(10ms);
            }
            total = n;
      }

      if (n == (-1)) [[unlikely]] {
            if (errno == EBADF)
                  throw except::connection_closed();
            util::win32::error_exit_wsa(L"recv");
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
      auto    lock  = std::lock_guard{write_mtx_};
      ssize_t total = 0;
      ssize_t n;

      do {
            n = ::send(main_fd_, static_cast<char const *>(buf) + total,
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
} // namespace ipc::detail
} // namespace emlsp
#endif
