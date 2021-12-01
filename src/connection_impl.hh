#pragma once
#ifndef HGUARD_d_CONNECTION_IMPL_HH_
#define HGUARD_d_CONNECTION_IMPL_HH_

#include "Common.hh"
#include "client.hh"
#include "util/exceptions.hh"
/****************************************************************************************/

namespace emlsp::ipc::detail {

#ifdef _WIN32
using procinfo_t = PROCESS_INFORMATION;
#else
using procinfo_t = pid_t;
#endif

class base_connection_interface
{
    protected:
      enum class sink_type {
            DEFAULT,
            DEVNULL,
            FD,
            FILENAME
      } err_sink_type_ = sink_type::DEFAULT;

      std::string fname_ = {};
      int         fd_     = (-1);

#ifdef _WIN32
      ND HANDLE get_err_handle() const;
#endif

    public:
      base_connection_interface()  = default;
      virtual ~base_connection_interface()
      {
            switch (err_sink_type_)
            {
                  case sink_type::FILENAME:
                  case sink_type::FD:
                  case sink_type::DEVNULL:
                  case sink_type::DEFAULT:
                        break;
            }
      }

      virtual procinfo_t do_spawn_connection(size_t argc, char **argv) = 0;
      virtual ssize_t read(void *buf, size_t nbytes)                   = 0;
      virtual ssize_t read(void *buf, size_t nbytes, int flags)        = 0;
      virtual ssize_t write(void const *buf, size_t nbytes)            = 0;
      virtual ssize_t write(void const *buf, size_t nbytes, int flags) = 0;

      void set_stderr_default() { err_sink_type_ = sink_type::DEFAULT; }
      void set_stderr_devnull() { err_sink_type_ = sink_type::DEVNULL; }
      void set_stderr_filename(std::string const &fname)
      {
            fname_         = fname;
            err_sink_type_ = sink_type::FILENAME;
      }

      DEFAULT_MOVE_CTORS(base_connection_interface);
      DEFAULT_COPY_CTORS(base_connection_interface);
};

/*--------------------------------------------------------------------------------------*/

template <typename AddrType>
class socket_connection_base_impl : public base_connection_interface
{
#ifdef _WIN32
      static int close_socket(socket_t const sock) { return closesocket(sock); }
#else
      static int close_socket(socket_t const sock) { return close(sock); }
#endif

    protected:
      socket_t root_      = static_cast<socket_t>(-1);
      socket_t accepted_  = static_cast<socket_t>(-1);
      AddrType addr_      = {};

      virtual socket_t open_new_socket()   = 0;
      virtual socket_t connect_to_socket() = 0;

    public:
      socket_connection_base_impl() = default;
      ~socket_connection_base_impl() override;

      explicit socket_connection_base_impl(socket_t const sock)
            : accepted_(sock)
      {}

      ssize_t read (void       *buf, size_t const nbytes) final { return read(buf, nbytes, MSG_WAITALL); }
      ssize_t write(void const *buf, size_t const nbytes) final { return write(buf, nbytes, 0); }
      ssize_t read (void       *buf, size_t nbytes, int flags) final;
      ssize_t write(void const *buf, size_t nbytes, int flags) final;

      ND socket_t const &operator()() const { return accepted_; }
      ND socket_t const &base()       const { return root_; }
      ND socket_t const &accepted()   const { return accepted_; }
      ND AddrType const &addr()       const { return addr_; }

      DELETE_COPY_CTORS(socket_connection_base_impl);
      DEFAULT_MOVE_CTORS(socket_connection_base_impl);
};

class unix_socket_connection_impl final : public socket_connection_base_impl<sockaddr_un>
{
      std::string path_;

    protected:
      socket_t open_new_socket()   final;
      socket_t connect_to_socket() final;

    public:
      unix_socket_connection_impl()        = default;
      ~unix_socket_connection_impl() final = default;

      procinfo_t do_spawn_connection(size_t argc, char **argv) final;

      DELETE_COPY_CTORS(unix_socket_connection_impl);
      DEFAULT_MOVE_CTORS(unix_socket_connection_impl);
};

/*--------------------------------------------------------------------------------------*/

class pipe_connection_impl final : public base_connection_interface
{
    private:
      int read_  = (-1);
      int write_ = (-1);

    public:
      pipe_connection_impl() = default;
      ~pipe_connection_impl() final;

      pipe_connection_impl(int const readfd, int const writefd)
            : read_(readfd), write_(writefd)
      {}

      static pipe_connection_impl std_handles() { return {0, 1}; }

      procinfo_t do_spawn_connection(size_t argc, char **argv) final;

      ssize_t read (void       *buf, size_t const nbytes) final { return read(buf, nbytes, MSG_WAITALL); }
      ssize_t write(void const *buf, size_t const nbytes) final { return write(buf, nbytes, 0); }
      ssize_t read (void       *buf, size_t nbytes, int flags) final;
      ssize_t write(void const *buf, size_t nbytes, int flags) final;

      ND int read_fd() const { return read_; }
      ND int write_fd() const { return write_; }

      DELETE_COPY_CTORS(pipe_connection_impl);
      DEFAULT_MOVE_CTORS(pipe_connection_impl);
};

/*--------------------------------------------------------------------------------------*/

#ifdef _WIN32

/**
 * BUG: Totally broken.
 */
class win32_named_pipe_impl final : public base_connection_interface
{
    private:
      HANDLE      pipe_ = nullptr;
      std::string fname_;

    public:
      //win32_named_pipe_impl() = default;
      win32_named_pipe_impl()
      {
            throw emlsp::except::not_implemented("FIXME: Not functional.");
      }
      ~win32_named_pipe_impl() final
      {
            if (pipe_)
                  CloseHandle(pipe_);
      }

      procinfo_t do_spawn_connection(size_t argc, char **argv) final;

      ssize_t read (void       *buf, size_t const nbytes) final { return read(buf, nbytes, 0); }
      ssize_t write(void const *buf, size_t const nbytes) final { return write(buf, nbytes, 0); }
      ssize_t read (void       *buf, size_t nbytes, int flags) final;
      ssize_t write(void const *buf, size_t nbytes, int flags) final;

      DELETE_COPY_CTORS(win32_named_pipe_impl);
      DEFAULT_MOVE_CTORS(win32_named_pipe_impl);
};

#endif

/****************************************************************************************/

template <typename AddrType>
socket_connection_base_impl<AddrType>::~socket_connection_base_impl()
{
      if (accepted_ != static_cast<socket_t>(-1)) {
            shutdown(accepted_, 2);
            close_socket(accepted_);
      }
      if (root_ != static_cast<socket_t>(-1)) {
            shutdown(root_, 2);
            close_socket(root_);
      }
}

template <typename AddrType>
ssize_t
socket_connection_base_impl<AddrType>::read(void *buf, size_t const nbytes, UNUSED int flags)
{
      ssize_t total = 0, n;

#if 0
      do {
            n = ::read(accepted_, static_cast<char *>(buf) + total, nbytes - total);
      } while ((n != (-1)) && (total += n) < static_cast<ssize_t>(nbytes));
#endif
      if (flags & MSG_WAITALL) {
            flags &= ~MSG_WAITALL;
            do {
                  n = ::recv(accepted_, static_cast<char *>(buf) + total, nbytes - total, flags);
            } while ((n != (-1)) && (total += n) < static_cast<ssize_t>(nbytes));
      } else {
            n = ::recv(accepted_, static_cast<char *>(buf), nbytes, flags);
      }

      if (n == (-1))
#if defined _WIN32
            util::win32::error_exit(L"recv()");
#else
            err(1, "send()");
#endif

      return total;
}

template <typename AddrType>
ssize_t
socket_connection_base_impl<AddrType>::write(void const *buf, size_t const nbytes, int const flags)
{
      ssize_t total = 0, n;
      do {
            n = ::send(accepted_, static_cast<char const *>(buf) + total, nbytes - total, flags);
      } while (n != (-1) && (total += n) < static_cast<ssize_t>(nbytes));

      if (n == (-1))
#if defined _WIN32
            util::win32::error_exit(L"send()");
#else
            err(1, "send()");
#endif

      return total;
}

} // namespace emlsp::ipc::detail
/****************************************************************************************/
#endif
