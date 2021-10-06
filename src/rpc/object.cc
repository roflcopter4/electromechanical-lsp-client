#include "rpc/object.hh"

//#include <anyrpc/anyrpc.h>
//#include <grpcpp/grpcpp.h>
//#include <rest_rpc.hpp>

#if 0
#include <boost/asio.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/basic_socket_iostream.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#endif

// #include <rest_rpc.hpp>

[[maybe_unused]] static constexpr
char const initial_msg[] =
R"({
      "jsonrpc": "2.0",
      "id": 1,
      "method": "initialize",
      "params": {
            "processId": %d,
            "rootUri":   "%s",
            "trace":     "off",
            "locale":    "en_US.UTF8",
            "clientInfo": {
                  "name": ")" MAIN_PROJECT_NAME R"(",
                  "version": ")" MAIN_PROJECT_VERSION_STRING R"("
            },
            "capabilities": {
            }
      }
})";


namespace emlsp::rpc {
namespace socket {

struct socket_info {
      struct socket_container_s {
            socket_t main;
            socket_t connected;
            socket_t accepted;
            socket_t &operator()() { return accepted; }
      } sock;

      std::string path;
#ifdef _WIN32
      PROCESS_INFORMATION pid;
#else
      pid_t pid;
#endif
};

#ifdef _WIN32
[[noreturn]] static int
process_startup_thread(void *varg)
{
      auto *info = static_cast<socket_info *>(varg);

      try {
            info->sock.connected = util::rpc::connect_to_socket(info->path.c_str());
      } catch (std::exception &e) {
            std::cerr << "Caught exception \"" << e.what()
                      << "\" while attempting to connect to socket at " << info->path << '\n';
            thrd_exit(WSAGetLastError());
      }

      char cmdline[256];
      strcpy(cmdline, "clangd --pch-storage=memory --log=verbose --pretty");

      STARTUPINFOA startinfo = {};
      startinfo.dwFlags      = STARTF_USESTDHANDLES;
      startinfo.hStdInput    = reinterpret_cast<HANDLE>(info->sock.connected); //NOLINT
      startinfo.hStdOutput   = reinterpret_cast<HANDLE>(info->sock.connected); //NOLINT

      bool b = CreateProcessA(nullptr, cmdline, nullptr, nullptr, true,
                              CREATE_NEW_CONSOLE, nullptr, nullptr,
                              &startinfo, &info->pid);
      auto const error = GetLastError();
      thrd_exit(static_cast<int>(b ? 0LU : error));
}
#endif

socket_info *
attempt_clangd_sock()
{
      auto const tmppath = util::get_temporary_filename();
      auto const sock    = util::rpc::open_new_socket(tmppath.string().c_str());
      auto      *info    = new socket_info{
          .sock = {sock, {}, {}},
          .path = tmppath.string(),
          .pid = {}
      };

      std::cerr << FORMAT("(Socket bound to \"{}\" with raw value ({}).\n", tmppath.string(), sock);

#ifdef _WIN32
      thrd_t start_thread{};
      thrd_create(&start_thread, process_startup_thread, info);

      auto const connected_sock = accept(sock, nullptr, nullptr);

      int e;
      thrd_join(start_thread, &e);
      if (e != 0)
            err(e, "Process creation failed.");
#else
      static constexpr char const *const argv[] = {
            "clangd", "--log=verbose", "--pch-storage=memory", "--pretty", nullptr
      };

      if ((info->pid = fork()) == 0) {
            socket_t dsock = util::rpc::connect_to_socket(tmppath.c_str());
            dup2(dsock, 0);
            dup2(dsock, 1);
            close(dsock);
            execvpe("clangd", const_cast<char **>(argv), environ);
      }

      auto const connected_sock = accept(sock, nullptr, nullptr);
#endif

      info->sock.accepted = connected_sock;
      return info;

#if 0
      auto tmppath = util::get_temporary_filename();
      socket_t sock = util::rpc::open_new_socket(tmppath.c_str());

      if ((pid = fork()) == 0) {
            socket_t dsock = util::rpc::connect_to_socket(tmppath.c_str());
            dup2(dsock, 0);
            dup2(dsock, 1);
            close(dsock);
            execvpe("clangd", const_cast<char **>(argv), environ);
      }

      socket_t data = accept(sock, nullptr, nullptr);
      if (data == (-1))
            err(1, "accept");

      *writefd = *readfd = data;
#endif
}

} // namespace socket


namespace any {



} // namespace any


namespace test {

static void
test_wrapper(void (*fn)(socket::socket_info *))
{
      socket::socket_info *info = socket::attempt_clangd_sock();

      fn(info);

#ifdef _WIN32
      closesocket(info->sock.connected);
      closesocket(info->sock.accepted);
      closesocket(info->sock.main);
      TerminateProcess(info->pid.hProcess, 1);
      CloseHandle(info->pid.hThread);
      CloseHandle(info->pid.hProcess);
#else
      close(info->sock.connected);
      close(info->sock.accepted);
      close(info->sock.main);
      kill(info->pid, SIGTERM);
#endif
      delete info;
}

static void do_test01(socket::socket_info *info)
{
      char buf[8192];
      ssize_t buf_size = snprintf(buf, 8192, initial_msg, getpid(), "");
      std::string const head = fmt::format(FMT_COMPILE("Content-Length: {}\r\n\r\n"), buf_size);
      send(info->sock(), head.data(), head.size(), MSG_MORE);
      send(info->sock(), buf, buf_size, MSG_EOR);

      buf_size    = recv(info->sock(), buf, sizeof(buf), MSG_WAITFORONE);
      buf_size   += recv(info->sock(), buf+buf_size, sizeof(buf)-buf_size, MSG_DONTWAIT);
      auto foobar = std::string(buf, buf_size);
      std::cout << "got: " << foobar << '\n';
}

static void
do_test02(UNUSED socket::socket_info *info)
{

}

#if 0
namespace asio = boost::asio;
//using boost::asio;

static void do_test02(socket::socket_info *info)
{
      auto sock = asio::basic_stream_socket(asio::system_executor{},
                                            asio::generic::stream_protocol(AF_UNIX, SOCK_STREAM),
                                            info->sock.accepted);

      char buf[8192];
      int buf_size = snprintf(buf, 8192, initial_msg, GetCurrentProcessId(), "");

      //std::string init = fmt::sprintf(initial_msg, GetCurrentProcessId(), "");
      std::string const head = fmt::format(FMT_COMPILE("Content-Length: {}\r\n\r\n"), buf_size);

      WriteFile(reinterpret_cast<HANDLE>(STD_ERROR_HANDLE), head.data(), head.size(), nullptr, nullptr);
      WriteFile(reinterpret_cast<HANDLE>(STD_ERROR_HANDLE), buf, buf_size, nullptr, nullptr);
      std::cerr << std::endl;

      WriteFile(reinterpret_cast<HANDLE>(info->sock.accepted), head.data(), head.size(), nullptr, nullptr);
      WriteFile(reinterpret_cast<HANDLE>(info->sock.accepted), buf, buf_size, nullptr, nullptr);

      //asio::mutable_buffer const mutbuf{buf, 8192};
      //sock.receive(std::vector{mutbuf}, MSG_WAITALL);
      //auto const foobar = std::string{buf, mutbuf.size()};

      buf_size = recv(info->sock(), buf, 8192, 0);
      auto foobar = std::string(buf, buf_size);
      std::cout << "got: " << foobar << '\n';

      //asio::const_buffer buf {static_cast<void *>(head.data()), head.size()};
      //asio::buffered_write_stream<char> foo{sock};
      //auto bar = asio::basic_socket_streambuf{sock};
}

#endif

void test01() { test_wrapper(do_test01); }
void test02() { test_wrapper(do_test02); }

} // namespace test
} // namespace emlsp::rpc
