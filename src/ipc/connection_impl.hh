// ReSharper disable CppLocalVariableMayBeConst
// ReSharper disable CppFinalFunctionInFinalClass
#pragma once
#ifndef HGUARD__IPC__CONNECTION_IMPL_HH_
#define HGUARD__IPC__CONNECTION_IMPL_HH_ //NOLINT

#include "Common.hh"
#include "ipc/misc.hh"
#include "util/debug_trap.h"

/* #undef WIN32_USE_PIPE_IMPL */
#define WHAT_THE_FUCK() [[maybe_unused]] static constexpr int P99_PASTE(z_VARIABLE_that_IS_not_USED_, __LINE__, _, __COUNTER__, _) = 0

inline namespace emlsp {
namespace ipc::detail::base {
/****************************************************************************************/
/* ┏----------------------------------------------------------------------------------┓
   ┃  ┌───────────────────────────────────────────────┐                               ┃
   ┃  │Base interface for an automatic ipc connection.│                               ┃
   ┃  └───────────────────────────────────────────────┘                               ┃
   ┗----------------------------------------------------------------------------------┛ */


/*
 * TODO: Document
 */
class base_connection_impl_interface
{
      using DescriptorType = intptr_t;

#ifdef _WIN32
      static constexpr wchar_t dev_null[] = L"NUL";
#else
      static constexpr char dev_null[] = "/dev/null";
#endif

    protected:
      enum class sink_type : uint8_t {
            DEFAULT,
            DEVNULL,
            FILENAME,
            FD,
      };

    public:
#ifdef _WIN32
      using err_descriptor_type = HANDLE;
#else
      using err_descriptor_type = int;
#endif

      static constexpr int default_read_flags  = MSG_WAITALL;
      static constexpr int default_write_flags = 0;

      std::recursive_mutex read_mtx_  = {};
      std::recursive_mutex write_mtx_ = {};
      std::string          err_fname_ = {};
      err_descriptor_type  err_fd_    = err_descriptor_type(-1); // NOLINT(*cast*)
      sink_type            err_sink_type_ : 2 = sink_type::DEFAULT;

    private:
      uint8_t initialized_ : 3 = 0;

    public:
      base_connection_impl_interface() = default;

      virtual ~base_connection_impl_interface()
      {
            if (err_sink_type_ == sink_type::FILENAME)
                  util::close_descriptor(err_fd_);
      }

      DELETE_ALL_CTORS(base_connection_impl_interface);

      using this_type       = base_connection_impl_interface;
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
            err_fname_     = fname;
      }

      virtual ssize_t read(void *buf, ssize_t const nbytes)
      {
            return read(buf, nbytes, default_read_flags);
      }

      virtual ssize_t write(void const *buf, ssize_t const nbytes)
      {
            return write(buf, nbytes, default_write_flags);
      }

      virtual ssize_t writev(iovec *buf, size_t const nbytes)
      {
            return writev(buf, nbytes, default_write_flags);
      }

      ND virtual constexpr bool is_socket() const noexcept
      {
            return false;
      }

         virtual ssize_t    read (void       *buf, ssize_t nbytes, int flags) = 0;
         virtual ssize_t    write(void const *buf, ssize_t nbytes, int flags) = 0;
         virtual ssize_t    writev(iovec *bufs, size_t nbufs, int flags)      = 0;
         virtual void       open()                                            = 0;
         virtual void       close() noexcept                                  = 0;
         virtual procinfo_t do_spawn_connection(size_t argc, char **argv)     = 0;
      ND virtual DescriptorType fd()        const noexcept                    = 0;
      ND virtual size_t         available() const                             = 0;

      //--------------------------------------------------------------------------------

    protected:
      ND err_descriptor_type get_err_handle();
      ND bool check_initialized(uint8_t const val) const noexcept { return initialized_ >= val; }
      void    set_initialized(uint8_t const val = 1) noexcept     { initialized_ = val; }

      void throw_if_initialized(std::type_info const &id, uint8_t const val = 1) const
      {
            if (check_initialized(val))
                  throw std::logic_error{
                      fmt::format(FC("Error: Attempt to initialize already initialized "
                                     "instance of class \"{}\", to level {}."),
                                  util::demangle(id), val)};
      }

#ifdef _WIN32
    private:
      std::thread child_watcher_thread_{};

    protected:
      std::thread *get_child_watcher_thread() { return &child_watcher_thread_; }
#else
      std::thread *get_child_watcher_thread() { (void)this; return nullptr; }
#endif

#ifdef _WIN32
    public:
      ND virtual bool spawn_with_shim() const noexcept { return false; }
         virtual void spawn_with_shim(bool) noexcept {}
#endif
};


template <typename T>
concept ConnectionImplVariant =
    std::derived_from<T, base_connection_impl_interface>;


/****************************************************************************************/
/* ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
   ┃  ┌──────────────────────────────────────────────────┐                            ┃
   ┃  │Derivative interface for socket based connections.│                            ┃
   ┃  └──────────────────────────────────────────────────┘                            ┃
   ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛ */


#pragma push_macro("SETERRNO")
#undef SETERRNO
#ifdef _WIN32
# define ERRNO WSAGetLastError()
# define SETERRNO(v) WSASetLastError(v)
# define ERR(t) util::win32::error_exit_wsa(L ## t)
#else
# define ERRNO errno
# define SETERRNO(v) (errno = (v))
# define ERR(t) err(t)
#endif


class socket_connection_base_impl
      : public base_connection_impl_interface
{
      using this_type = socket_connection_base_impl;
      using base_type = base_connection_impl_interface;
      // using AddrType  = sockaddr_storage;

#ifdef _WIN32
      using sock_int_type = int;
#else
      using sock_int_type = size_t;
#endif
      using cbool = bool const;

      bool should_open_listener_  : 1 = true;
      bool should_close_listener_ : 1 = true;
      bool should_connect_        : 1 = true;
      bool use_dual_sockets_      : 1 = false;

    protected:
      socket_t  listener_  = invalid_socket;
      socket_t  con_read_  = invalid_socket;
      socket_t  con_write_ = invalid_socket;
      socket_t  acc_read_  = invalid_socket;
      socket_t  acc_write_ = invalid_socket;
      // AddrType  addr_      = {};

    public:
      using addr_type = sockaddr;

      socket_connection_base_impl() = default;
      explicit socket_connection_base_impl(socket_t const sock)
            : acc_read_(sock), acc_write_(sock)
      {}

      ~socket_connection_base_impl() override
      {
            close();
      }

      DELETE_ALL_CTORS(socket_connection_base_impl);

      //--------------------------------------------------------------------------------

      ND bool should_open_listener() const noexcept   { return should_open_listener_; }
      ND bool should_close_listnener() const noexcept { return should_close_listener_; }
      ND bool should_connect() const noexcept         { return should_connect_; }
      ND bool use_dual_sockets() const noexcept       { return use_dual_sockets_; }
      void should_open_listener(cbool val)  noexcept  { should_open_listener_  = val; }
      void should_close_listnener(cbool val) noexcept { should_close_listener_ = val; }
      void should_connect(cbool val) noexcept         { should_connect_        = val; }
      void use_dual_sockets(cbool val) noexcept       { use_dual_sockets_      = val; }

      using base_type::read;
      using base_type::write;
      using base_type::writev;

      ssize_t read  (void       *buf, ssize_t nbytes, int flags) final;
      ssize_t write (void const *buf, ssize_t nbytes, int flags) final;
      ssize_t writev(iovec *bufs, size_t nbufs, int flags) final;
      void    close() noexcept final;

      ND socket_t listener() const { return listener_; }
      ND socket_t peer()     const { return con_read_; }
      ND socket_t acceptor() const { return acc_read_; }

      virtual socket_t accept()  = 0;
      virtual socket_t connect() = 0;

      ND virtual sockaddr       &addr() &       = 0;
      ND virtual sockaddr const &addr() const & = 0;

      ND constexpr bool is_socket() const noexcept final
      {
            return true;
      }

      ND size_t available() const final
      {
            return util::available_in_fd(acc_read_);
      }

      ND descriptor_type fd() const noexcept final
      {
            return static_cast<descriptor_type>(acc_read_);
      }

      virtual void
      set_listener(socket_t const sock, sockaddr const &newaddr) noexcept
      {
            should_close_listnener(false);
            listener_ = sock;
            memcpy(&addr(), &newaddr, get_socklen());
      }

    protected:
      virtual socket_t open_new_socket()      = 0;
      virtual socket_t connect_internally()   = 0;
      virtual socket_t get_connected_socket() = 0;

      ND constexpr virtual socklen_t get_socklen() const = 0;
};


/*======================================================================================*/


class unix_socket_connection_impl final
      : public socket_connection_base_impl
{
      using this_type = unix_socket_connection_impl;
      using base_type = socket_connection_base_impl;

      std::string path_{};
      sockaddr_un addr_{};

    public:
      unix_socket_connection_impl()        = default;
      ~unix_socket_connection_impl() final = default;

      DELETE_ALL_CTORS(unix_socket_connection_impl);

      //--------------------------------------------------------------------------------

      void set_address(std::string path);

      socket_t   connect() final;
      socket_t   accept() final;
      void       open() final;
      procinfo_t do_spawn_connection(size_t argc, char **argv) final;

      ND sockaddr const &addr() const & final { return reinterpret_cast<sockaddr const &>(addr_); }
      ND sockaddr       &addr()       & final { return reinterpret_cast<sockaddr &>(addr_); }
      ND auto const     &path() const &       { return path_; }

#ifdef _WIN32
      ND bool spawn_with_shim() const noexcept final { return use_shim_; }
      void spawn_with_shim(bool val) noexcept final;

    private:
      bool use_shim_ = false;
#endif

    protected:
      socket_t open_new_socket()    final;
      socket_t connect_internally() final;
      socket_t get_connected_socket() noexcept final { return con_read_; }

      ND constexpr socklen_t get_socklen() const final { return sizeof(sockaddr_un); }
};


/*======================================================================================*/


#ifdef HAVE_SOCKETPAIR
class socketpair_connection_impl final
      : public socket_connection_base_impl//<sockaddr_un>
{
      using this_type = socketpair_connection_impl;
      using base_type = socket_connection_base_impl; //<sockaddr_un>;

    public:
      socketpair_connection_impl()           = default;
      ~socketpair_connection_impl() override = default;

      DELETE_ALL_CTORS(socketpair_connection_impl);

    private:
      [[deprecated("Don't use this function. It will just crash. On purpose.")]]
      void set_listener(socket_t /*sock*/, sockaddr const & /*addr*/) noexcept override
      {
            // throw std::logic_error(
            //     "This function makes no sense at all for this type of socket.");
            err_nothrow("This function makes no sense at all for this type of socket.");
      }

    public:
      void       open() override;
      procinfo_t do_spawn_connection(size_t argc, char **argv) override;

    protected:
      socket_t open_new_socket()   override;
      socket_t connect_internally() override __attribute__((__pure__));
      socket_t get_connected_socket() override { return connect_internally(); }

    private:
      socket_t connect() override { throw util::except::not_implemented(""); }
      socket_t accept() override { throw util::except::not_implemented(""); }

      ND sockaddr       &addr()       & override { throw util::except::not_implemented(""); }
      ND sockaddr const &addr() const & override { throw util::except::not_implemented(""); }
      ND socklen_t get_socklen() const override { return 0; }
};
#endif


/*======================================================================================*/
/* TCP/IP */


extern socket_t
connect_to_inet_socket(sockaddr const *addr, socklen_t size, int type, int protocol = 0);


template <typename AddrType>
      REQUIRES (IsInetSockaddr<AddrType> ||
                std::same_as<AddrType, sockaddr *>)
class inet_socket_connection_base_impl : public socket_connection_base_impl
{
      using this_type = inet_socket_connection_base_impl;
      using base_type = socket_connection_base_impl;

    protected:
      bool        addr_init_  = false;
      uint16_t    hport_      = 0;
      std::string addr_string_{};
      std::string addr_string_with_port_{};
      AddrType    addr_{};

    public:
      inet_socket_connection_base_impl()           = default;
      ~inet_socket_connection_base_impl() override = default;

      DELETE_ALL_CTORS(inet_socket_connection_base_impl);

      //--------------------------------------------------------------------------------

#if 0
      void set_listener(socket_t const sock, sockaddr const &addr) noexcept override
      {
            this->should_close_listnener(true);
            this->listener_ = sock;
            this->addr_     = addr;
      }
#endif

      socket_t connect() final
      {
            if (!check_initialized(1))
                  throw std::logic_error("Cannot connect to uninitialized address!");

            /* XXX Code implicitly assumes that we read and write to and from the
             *     acc_* sockets, so use those to connect. Ugh. */
            acc_read_ = connect_to_inet_socket(get_addr_generic(),
                                               get_socklen(), get_type());
            acc_write_ = acc_read_;
            return con_read_;
      }

      socket_t accept() final
      {
            if (!this->check_initialized(1))
                  throw std::logic_error("Cannot accept from uninitialized address!");

            socklen_t const mylen = get_socklen();
            socklen_t size  = mylen;
            this->acc_read_ = ::accept(this->listener_,
                                       const_cast<sockaddr *>(get_addr_generic()), &size);
            if (this->acc_read_ == invalid_socket || size != mylen)
                  ERR("accept");

            if (this->use_dual_sockets()) {
                  this->acc_write_ = ::accept(
                      this->listener_, const_cast<sockaddr *>(get_addr_generic()), &size);
                  if (this->acc_write_ == invalid_socket || size != mylen)
                        ERR("accept");
            } else {
                  this->acc_write_ = this->acc_read_;
            }

            return this->acc_read_;
      }

      ND sockaddr &addr() & final
      {
            if constexpr (std::is_pointer_v<AddrType>)
                  return *addr_;
            else
                  return reinterpret_cast<sockaddr &>(addr_);
      }

      ND sockaddr const &addr() const & final
      {
            if constexpr (std::is_pointer_v<AddrType>)
                  return *addr_;
            else
                  return reinterpret_cast<sockaddr const &>(addr_);
      }

      ND std::string const &get_addr_string() const & { return addr_string_; }
      ND uint16_t           get_port()        const   { return hport_; }
      ND std::string const &get_addr_and_port_string() const & { return addr_string_with_port_; }

    protected:
      ND virtual sockaddr const *get_addr_generic() const = 0;
      ND virtual int             get_type()         const = 0;

      socket_t get_connected_socket() noexcept final { return con_read_; }

      ND constexpr socklen_t get_socklen() const override
      {
            /* This redundent check shuts clang up. */
            if constexpr (!std::is_pointer_v<AddrType>)
                  return sizeof(AddrType);
            else
                  throw util::except::not_implemented();
      }
};

/*--------------------------------------------------------------------------------------*/
/* Either */

class inet_any_socket_connection_impl final
    : public inet_socket_connection_base_impl<sockaddr *>
{
      using this_type = inet_any_socket_connection_impl;
      using base_type = socket_connection_base_impl;

      int       type_        = AF_UNSPEC;
      socklen_t addr_length_ = 0;

    public:
      inet_any_socket_connection_impl() noexcept;
      ~inet_any_socket_connection_impl() final;

      DELETE_ALL_CTORS(inet_any_socket_connection_impl);

      //--------------------------------------------------------------------------------

      int resolve(char const *server_name, char const *port);
      int resolve(char const *server_and_port);

      void assign_from_addrinfo(addrinfo const *info);

      void       open() final;
      procinfo_t do_spawn_connection(size_t argc, char **argv) final;

    protected:
      socket_t open_new_socket()    final;
      socket_t connect_internally() final;

      ND constexpr socklen_t get_socklen()  const final { return addr_length_; }
      ND sockaddr const *get_addr_generic() const final { return addr_; }
      ND int get_type() const final { return type_; }

    private:
      void set_listener(socket_t const/*sock*/, sockaddr const & /*addr*/) noexcept final
      {}
      void init_strings();
};

/*--------------------------------------------------------------------------------------*/
/* IPv4 only */

class inet_ipv4_socket_connection_impl final
    : public inet_socket_connection_base_impl<sockaddr_in>
{
      using this_type = inet_ipv4_socket_connection_impl;
      using base_type = socket_connection_base_impl;

    public:
      inet_ipv4_socket_connection_impl()        = default;
      ~inet_ipv4_socket_connection_impl() final = default;

      DELETE_ALL_CTORS(inet_ipv4_socket_connection_impl);

      //--------------------------------------------------------------------------------

      void       open() final;
      procinfo_t do_spawn_connection(size_t argc, char **argv) final;

    protected:
      socket_t open_new_socket()    final;
      socket_t connect_internally() final;

      ND sockaddr const *get_addr_generic() const final
      {
            return reinterpret_cast<sockaddr const *>(&addr_);
      }
      // ND socklen_t get_socklen() const final { return sizeof(sockaddr_in); }

      ND constexpr int get_type() const final { return AF_INET; }
};

/*--------------------------------------------------------------------------------------*/
/* IPv6 only */

class inet_ipv6_socket_connection_impl final
    : public inet_socket_connection_base_impl<sockaddr_in6>
{
      using this_type = inet_ipv6_socket_connection_impl;
      using base_type = socket_connection_base_impl; //<addr_type>;

    public:
      inet_ipv6_socket_connection_impl()        = default;
      ~inet_ipv6_socket_connection_impl() final = default;
      DELETE_MOVE_CTORS(inet_ipv6_socket_connection_impl);
      DELETE_COPY_CTORS(inet_ipv6_socket_connection_impl);

      //--------------------------------------------------------------------------------

      void       open() final;
      procinfo_t do_spawn_connection(size_t argc, char **argv) final;

    protected:
      socket_t open_new_socket()    final;
      socket_t connect_internally() final;

      ND sockaddr const *get_addr_generic() const final
      {
            return reinterpret_cast<sockaddr const *>(&addr_);
      }
      // ND socklen_t get_socklen() const final { return sizeof(sockaddr_in6); }

      ND constexpr int get_type() const final { return AF_INET6; }
};


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
class pipe_connection_impl : public base_connection_impl_interface
{
      using this_type = pipe_connection_impl;
      using base_type = base_connection_impl_interface;
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
      DELETE_MOVE_CTORS(pipe_connection_impl);

      //--------------------------------------------------------------------------------

      using base_type::read;
      using base_type::write;
      using base_type::writev;

      ssize_t read (void       *buf, ssize_t nbytes, int flags) final;
      ssize_t write(void const *buf, ssize_t nbytes, int flags) final;
      ssize_t writev(iovec *vec, size_t nbufs, int flags) final;

      void       open() final { /* Do nothing. This method makes no sense for pipes. */ }
      void       close() noexcept final;
      procinfo_t do_spawn_connection(size_t argc, char **argv) override;
      ND size_t  available() const final;

      ND descriptor_type fd()   const noexcept final { return read_; }
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
      fd_connection_impl()        = default;
      ~fd_connection_impl() final = default;

      fd_connection_impl(int readfd, int writefd);

#ifdef _WIN32
      fd_connection_impl(HANDLE readfd, HANDLE writefd);
#endif

      DELETE_COPY_CTORS(fd_connection_impl);
      DELETE_MOVE_CTORS(fd_connection_impl);

      //--------------------------------------------------------------------------------

    private:
      __attribute_error__(ERRMSG)
      procinfo_t do_spawn_connection(size_t /*argc*/, char ** /*argv*/) final
      {
            throw std::logic_error(ERRMSG);
      }

    public:
      void set_descriptors(int readfd, int writefd);

      static fd_connection_impl make_from_std_handles() { return {0, 1}; }

#undef ERRMSG
};


/****************************************************************************************/


#ifdef _WIN32

class pipe_handle_connection_impl final : public base_connection_impl_interface
{
      using this_type = pipe_connection_impl;
      using base_type = base_connection_impl_interface;
      friend class fd_connection_impl;

      HANDLE volatile read_  = INVALID_HANDLE_VALUE;
      HANDLE volatile write_ = INVALID_HANDLE_VALUE;

    public:
      pipe_handle_connection_impl() = default;
      ~pipe_handle_connection_impl() final
      {
            close();
      }

      DELETE_COPY_CTORS(pipe_handle_connection_impl);
      DELETE_MOVE_CTORS(pipe_handle_connection_impl);

      //--------------------------------------------------------------------------------

      using base_type::read;
      using base_type::write;
      using base_type::writev;

      ssize_t read (void       *buf, ssize_t nbytes, int flags) final;
      ssize_t write(void const *buf, ssize_t nbytes, int flags) final;
      ssize_t writev(iovec *vec, size_t nbufs, int flags) final;

      void       open() final { /* Do nothing. This method makes no sense for pipes. */ }
      void       close() noexcept final;
      procinfo_t do_spawn_connection(size_t argc, char **argv) final;

      ND descriptor_type fd() const noexcept      final { return reinterpret_cast<descriptor_type>(read_); }
      ND size_t available() const final { return util::available_in_fd(read_); }

      ND HANDLE read_fd()  const noexcept { return read_; }
      ND HANDLE write_fd() const noexcept { return write_; }

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
class win32_named_pipe_impl final : public base_connection_impl_interface
{
      std::string  pipe_narrow_name_;
      std::wstring pipe_fname_;
      HANDLE       pipe_             = INVALID_HANDLE_VALUE;
      int          crt_fd_           = -1;
      int          file_mode_        = _O_BINARY;
      bool         use_dual_sockets_ = false;

      std::condition_variable write_cond_{};

    public:
      win32_named_pipe_impl() = default;
      ~win32_named_pipe_impl() override
      {
            close();
      }

      void       open() override;
      void       close() noexcept override;
      procinfo_t do_spawn_connection(size_t argc, char **argv) override;

      ssize_t read  (void       *buf, ssize_t const nbytes) override { return read(buf, nbytes, 0); }
      ssize_t write (void const *buf, ssize_t const nbytes) override { return write(buf, nbytes, 0); }
      ssize_t writev(iovec *vec, size_t const nbufs)  override { return writev(vec, nbufs, 0); }

      ssize_t read  (void       *buf, ssize_t nbytes, int flags) override;
      ssize_t write (void const *buf, ssize_t nbytes, int flags) override;
      ssize_t writev(iovec *vec, size_t nbufs,  int flags) override;

      ND descriptor_type fd() const noexcept override { return crt_fd_; }
      ND HANDLE handle() const noexcept { return pipe_; }
      ND size_t available() const override;

      ND bool use_dual_sockets() const noexcept { return use_dual_sockets_; }
      void use_dual_sockets(bool val) noexcept  { use_dual_sockets_ = val; }
      void set_file_mode(int val) noexcept      { file_mode_ = val; }

      DELETE_COPY_CTORS(win32_named_pipe_impl);
      DEFAULT_MOVE_CTORS(win32_named_pipe_impl);

    private:
      static void __stdcall write_callback(_In_ DWORD           dwErrorCode,
                                           _In_ DWORD dwNumberOfBytesTransfered,
                                           _Inout_ LPOVERLAPPED lpOverlapped);
};

#endif


/****************************************************************************************/


class libuv_pipe_handle_impl final
    : public base_connection_impl_interface
{
      using this_type = libuv_pipe_handle_impl;

#ifdef _WIN32
      int crt_err_fd_ = 2;
#endif

      uv_loop_t   *loop_         = nullptr;
      uv_pipe_t    read_handle_  = {};
      uv_pipe_t    write_handle_ = {};
      uv_process_t proc_handle_  = {};

      std::condition_variable write_cond_{};

    public:
      libuv_pipe_handle_impl() = default;
      ~libuv_pipe_handle_impl() final
      {
#ifdef _WIN32
            if (crt_err_fd_ > 2) {
                  _close(crt_err_fd_);
                  crt_err_fd_ = 2;
                  err_fd_     = INVALID_HANDLE_VALUE;
            }
#endif
            close();
      }

      void       close() noexcept final;
      void       open() final;
      procinfo_t do_spawn_connection(size_t argc, char **argv) final;

      ssize_t write(void const *buf, ssize_t const nbytes)      final { return write(buf, nbytes, 0); }
      ssize_t write(void const *buf, ssize_t nbytes, int flags) final;

    private:
      [[noreturn, gnu::__error__("This implementation can't read")]]
      ssize_t read(void * /*buf*/, ssize_t /*nbytes*/, int /*flags*/) final
      {
            throw std::logic_error("This implementation can't read");
      }

      [[noreturn, gnu::__error__("This implementation can't read")]]
      ssize_t read(void * /*buf*/, UU ssize_t const /*nbytes*/) final
      {
            throw std::logic_error("This implementation can't read.");
      }

    public:
      ssize_t writev(iovec *vec, size_t const nbufs) final { return writev(vec, nbufs, 0); };
      ssize_t writev(iovec *vec, size_t nbufs, int flags) final;

      ND descriptor_type fd() const noexcept final __attribute__((__pure__));
      ND size_t        available() const final;
      ND uv_pipe_t    *get_uv_handle()         noexcept { return &read_handle_; }
      ND uv_process_t *get_uv_process_handle() noexcept { return &proc_handle_; }

      ND uv_pipe_t    const *get_uv_handle()         const noexcept { return &read_handle_; }
      ND uv_process_t const *get_uv_process_handle() const noexcept { return &proc_handle_; }

      void set_loop(uv_loop_t *loop);

      DELETE_COPY_CTORS(libuv_pipe_handle_impl);
      DELETE_MOVE_CTORS(libuv_pipe_handle_impl);

    private:
      static void uvwrite_callback(uv_write_t *req, int status);
};


/****************************************************************************************/
/****************************************************************************************/
/* Out of line implementations for the base interface. */


#ifdef _WIN32
inline base_connection_impl_interface::err_descriptor_type
base_connection_impl_interface::get_err_handle()
{
      HANDLE err_handle = INVALID_HANDLE_VALUE;

      switch (err_sink_type_) {
      case sink_type::DEFAULT:
            err_handle = GetStdHandle(STD_ERROR_HANDLE);
            break;
      case sink_type::DEVNULL:
            err_handle = CreateFileW(dev_null, GENERIC_WRITE, 0, nullptr,
                                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            err_fd_ = err_handle;
            break;
      case sink_type::FD:
            err_handle = err_fd_;
            break;
      case sink_type::FILENAME:
            err_handle = CreateFileW(util::recode<wchar_t>(err_fname_).c_str(),
                                     GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL, nullptr);
            err_fd_ = err_handle;
            break;
      }

      return err_handle;
}

#else

inline base_connection_impl_interface::err_descriptor_type
base_connection_impl_interface::get_err_handle()
{
      int fd;

      switch (err_sink_type_) {
      case sink_type::DEVNULL:
            fd = ::open(dev_null, O_WRONLY|O_APPEND);
            err_fd_ = fd;
            break;
      case sink_type::FILENAME:
            fd = ::open(err_fname_.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0600);
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

#ifdef _WIN32
# define XLATE_ERR(e) WSA##e
#else
# define XLATE_ERR(e) e
#endif

WHAT_THE_FUCK();

inline void close_socket(socket_t &sock) noexcept
{
      if (sock != invalid_socket) {
            ::shutdown(sock, 2);
            util::close_descriptor(sock);
      }
}

inline void socket_connection_base_impl::close() noexcept
{
      if (acc_write_ != acc_read_)
            close_socket(acc_write_);
      if (con_write_ != con_read_)
            close_socket(con_write_);
      if (should_close_listener_)
            close_socket(listener_);
      close_socket(acc_read_);
      close_socket(con_read_);
}


WHAT_THE_FUCK();
extern size_t socket_recv_impl(socket_t sock, void       *buf, size_t nbytes, int flags);
extern size_t socket_send_impl(socket_t sock, void const *buf, size_t nbytes, int flags);
extern size_t socket_writev_impl(socket_t sock, iovec const *, size_t nbufs, int flags);


template <typename File, size_t (*WriteFn)(File, iovec const *, size_t, int)>
size_t
writev_all(File fd, iovec *buf_vec, size_t nbufs, int const flags)
{
#ifdef _WIN32
# define iov_len  len
# define iov_base buf
#endif

//#if defined __GNUC__ || defined __clang__
//      iovec vec_cpy_vla[nbufs + SIZE_C(1)];
//      iovec *vec_cpy = vec_cpy_vla;
//#else
//      auto *vec_cpy  = static_cast<iovec *>(alloca((nbufs + SIZE_C(1)) * sizeof(iovec)));
//#endif
//      auto  *vec_cpy = static_cast<iovec *>(::malloc((nbufs + SIZE_C(1)) * sizeof(iovec)));

      //static constexpr size_t max_iovecs = 32;
      //if (nbufs > max_iovecs)
      //      throw std::invalid_argument("Too many iovecs");
      //iovec vec_cpy_array[max_iovecs];
      //iovec *vec_cpy = vec_cpy_array;

      //size_t total   = 0;
      //init_iovec(vec_cpy[nbufs], nullptr, 0);
      //memcpy(vec_cpy, buf_vec, nbufs * sizeof(iovec));

      size_t total = 0;

      while (nbufs > 0 && buf_vec[0].iov_len > 0) {
            size_t delta = WriteFn(fd, buf_vec, nbufs, flags);
            assert(delta > 0);
            total += delta;

            for (iovec *vec; (vec = buf_vec)->iov_len > 0; ++buf_vec, --nbufs) {
                  if (vec->iov_len > delta) {
                        vec->iov_len -= static_cast<decltype(vec->iov_len)>(delta);
                        vec->iov_base = (static_cast<char *>(vec->iov_base)) + delta;
                        break;
                  }
                  delta -= vec->iov_len;
            }
      }

      //::free(vec_cpy);
      return total;

#undef iov_len
#undef iov_base
}


inline ssize_t
socket_connection_base_impl::read(void         *buf,
                                  ssize_t const nbytes,
                                  int     const flags)
{
      std::lock_guard<std::recursive_mutex> lock{read_mtx_};
      SETERRNO(0);
      return static_cast<ssize_t>(socket_recv_impl(acc_read_, buf, nbytes, flags));
}

inline ssize_t
socket_connection_base_impl::write(void    const *buf,
                                   ssize_t const  nbytes,
                                   int     const  flags)
{
      std::lock_guard<std::recursive_mutex> lock{write_mtx_};
      SETERRNO(0);
      return static_cast<ssize_t>(socket_send_impl(acc_write_, buf, nbytes, flags));
}


inline ssize_t
socket_connection_base_impl::writev(iovec  *bufs,
                                    size_t const  nbufs,
                                    int    const  flags)
{
      std::lock_guard<std::recursive_mutex> lock{write_mtx_};
      SETERRNO(0);
      return static_cast<ssize_t>(
          writev_all<socket_t, socket_writev_impl>(acc_write_, bufs, nbufs, flags));
}


#undef ERR
#undef ERRNO
#undef XLATE_ERR
#pragma pop_macro("SETERRNO")


/****************************************************************************************/
} // namespace ipc::detail::base
} // namespace emlsp
#endif
