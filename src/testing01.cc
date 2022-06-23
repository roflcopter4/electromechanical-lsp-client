// ReSharper disable CppClangTidyCppcoreguidelinesNarrowingConversions
#include "Common.hh"
#include "ipc/basic_protocol_connection.hh"

#include "ipc/rpc/lsp.hh"
#include "ipc/rpc/lsp/static-data.hh"
#include "ipc/rpc/neovim.hh"

#include "msgpack/dumper.hh"

#include "toploop.hh"

#include "lazy.hh"

#include <nlohmann/json.hpp>

#define AUTOC auto const

inline namespace emlsp {
namespace testing {
/****************************************************************************************/


#ifdef _WIN32
using con_type   = ipc::connections::libuv_pipe_handle;
//using con_type   = ipc::connections::unix_socket;
#else
// using con_type   = ipc::connections::libuv_pipe_handle;
// using con_type   = ipc::connections::unix_socket;
using con_type   = ipc::connections::inet_ipv6_socket;
#endif
using nvim_type  = ipc::protocols::Msgpack::connection<con_type>;
using clang_type = ipc::protocols::MsJsonrpc::connection<con_type>;

#define USE_PIPS


WHAT_THE_FUCK()
NOINLINE void foo02()
{
      AUTOC loop = loops::main_loop::create();

#ifdef USE_PIPES
      AUTOC nvim = nvim_type::new_unique();
      nvim->impl().set_loop(loop->base());
      nvim->impl().open();
      AUTOC clangd = clang_type::new_unique();
      clangd->impl().set_loop(loop->base());
      clangd->impl().open();
#else
      auto nvim   = nvim_type::new_unique();
      auto clangd = clang_type::new_unique();
      //nvim->impl().should_connect(false);
      //clangd->impl().use_dual_sockets(true);
      //clangd->impl().use_dual_sockets(true);
# ifdef _WIN32
      clangd->impl().should_connect(false);
      clangd->impl().open();
# endif
#endif

      clangd->redirect_stderr_to_devnull();

#if 1
      nvim->spawn_connection_l(
         //R"(C:\Program Files (x86)\Microsoft Visual Studio\Shared\Python39_64\python.exe)",
         "python"
#ifdef _WIN32
         ".exe"
#endif
         , "-c",
         R"(
import msgpack, sys, time
sys.stdout.mode = 'wb'
sys.stdout.buffer.write(msgpack.packb([0.1231, 5/3, 10/3, 9/3, {'a': '\r\n'}, True, 'l'*64, '23', ~0, -1, 18446744073709551615, []]))
sys.stdout.buffer.write(msgpack.packb([0.1231, 10, [[], [], {}], 10]))
sys.stdout.buffer.flush();
time.sleep(1)
)"
      );
#endif

#if 0
      clangd->spawn_connection_l(
         "python.exe",
         "-c",
         R"(
import json, sys, time
x = json.dumps([0.1231, 5/3, 10/3, 9/3, {'a': 7}, True, 43, '23', ~0, -1, 18446744073709551615, []]).encode('utf8')
sys.stdout.buffer.write(b'Content-Length: %d\r\n\r\n%s' % (len(x), x))
x = json.dumps([0.1231, '\r\n', 10, [[], [], {}], 10]).encode('utf8')
sys.stdout.buffer.write(b'Content-Length: %d\r\n\r\n%s' % (len(x), x))
sys.stdout.flush()
time.sleep(20)
)"
      );
#endif

      clangd->spawn_connection_l("clangd", "--log=verbose");

#ifdef USE_PIPES
      //loop->use_pipe_handle("nvim", nvim->impl().get_uv_handle(), nvim.get());
      loop->use_pipe_handle  ("clangd", clangd);
      loop->start_pipe_handle("clangd", clangd);
      loop->use_pipe_handle  ("nvim",   nvim);
      loop->start_pipe_handle("nvim",   nvim);
      //assert (nvim->impl().get_uv_handle()->type == UV_NAMED_PIPE);
#else
      //ResumeThread(nvim->pid().hThread);
      //nvim->impl().accept();
      //loop->open_poll_handle ("nvim", nvim);
      //loop->start_poll_handle("nvim", nvim);

# ifdef _WIN32
      ResumeThread(clangd->pid().hThread);
      clangd->impl().accept();
# endif
      loop->open_poll_handle ("clangd", clangd);
      loop->start_poll_handle("clangd", clangd);
#endif

      //std::this_thread::sleep_for(2s);
      //auto thrd = std::thread{[&loop]() { loop->loop_start(); }};
      //std::this_thread::sleep_for(2s);

      loop->loop_start_async();

      auto fname_uri_ptr = util::glib::filename_to_uri(util::recode<char>(lazy::fname_raw));
      auto fname_uri     = std::string_view{fname_uri_ptr.get()};

      {
            auto path_uri = util::glib::filename_to_uri(util::recode<char>(lazy::path_raw));
            auto init     = ipc::lsp::data::init_msg(path_uri.get());
            auto content  = util::slurp_file(lazy::fname_raw);

            clangd->write_string(init);
            std::this_thread::sleep_for(500ms);

            auto wrap = clangd->get_new_packer();

            wrap().add_member("jsonrpc", "2.0");
            wrap().add_member("method", "textDocument/didOpen"sv);
            wrap().set_member("params");
            wrap().set_member("textDocument");
            wrap().add_member("uri", fname_uri);
            wrap().add_member("text", content);
            wrap().add_member("version", 1);
            wrap().add_member("languageId", "cpp");

            clangd->write_object(wrap);
            //clangd->wait();
      }
      {
            auto wrap = clangd->get_new_packer();
            wrap().add_member("jsonrpc", "2.0");
            wrap().add_member("id", 1);
            wrap().add_member("method", "textDocument/semanticTokens/full");
            wrap().set_member("params");
            wrap().set_member("textDocument");
            wrap().add_member("uri", fname_uri);

            clangd->write_object(wrap);
            //clangd->wait();
      }

      std::this_thread::sleep_for(5s);
      loop->loop_stop();
      //loop->wait();
      //thrd.join();
}


#if 0
NOINLINE void foo03()
{
      //AUTOC con  = nvim_type::new_unique();
      //AUTOC loop = loops::main_loop::create();

      DWORD status = 0;
      //auto  con    = std::make_unique<ipc::connections::unix_socket>();

      AUTOC loop = loops::main_loop::create();
      AUTOC con  = ipc::protocols::MsJsonrpc::connection<ipc::connections::unix_socket>::new_unique();

#ifdef _WIN32
      con->impl().should_connect(false);
      con->impl().use_dual_sockets(false);
      con->impl().open();

      //SetEnvironmentVariableA("EMLSP_SERVER_NAME", con->impl().path().c_str());
      //{
      //      wchar_t    buf[128];
      //      auto const ret = GetEnvironmentVariableW(L"EMLSP_SERVER_NAME", buf, (DWORD)std::size(buf));
      //      auto const tmp = util::recode<wchar_t>(con->impl().path());
      //      if (ret == 0 || buf != tmp)
      //            util::win32::error_exit_message(L"Failed to set environment variable: "s + tmp);
      //}
#endif

      con->spawn_connection_l(
#ifdef _WIN32
                              //R"(D:\LLVM2\bin\clangd.exe)",
                              R"(C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clangd.exe)",
#else
                              "clangd",
#endif
                              "--log=verbose", "--pch-storage=memory",
                              "--print-options");

      //con->spawn_connection_l(R"(D:\ass\VisualStudio\Repos\dumb_injector.dll\x64\Debug\Test_dumb_injector.exe)");

      //con->spawn_connection_l(R"(D:\new_msys64\ucrt64\bin\perl.exe)", "-E", R"(while (<>) { print STDERR "PERL RULZ:\t$_"; })");;

//      con->spawn_connection_l(
//         //R"(D:\ass\VisualStudio\Repos\DetourTest\x64\Debug\DetourExe.exe)", R"(/d:D:\ass\VisualStudio\Repos\dumb_injector.dll\x64\Debug\dumb_injector.dll.dll)",
//         //"python.exe",
//         R"(C:\Program Files (x86)\Microsoft Visual Studio\Shared\Python39_64\python.exe)",
//         //"python3.10",
//         //"python3.9",
//         "-c",
//         R"(
//import json, sys, os
//if sys.stdout is None:
//    sys.exit(1)
//# sys.stdin = os.fdopen(0, 'r')
//# sys.stdout = os.fdopen(114, 'w')
//x = json.dumps([0.1231, 5/3, 10/3, 9/3, {'a': 7}, True, 43, '23', ~0, -1, 18446744073709551615, []]).encode('utf8')
//sys.stdout.buffer.write(b'Content-Length: %d\r\n\r\n%s' % (len(x), x))
//x = json.dumps([0.1231, '\r\n', 10, [[], [], {}], 10]).encode('utf8')
//sys.stdout.buffer.write(b'Content-Length: %d\r\n\r\n%s' % (len(x), x))
//sys.stdout.flush()
//)"
//      );


//      con->spawn_connection_l("python", "-c", R"(
//import sys
//print("hello", file=sys.stderr)
//print("hello")
//)"
//      );

#if 0
      HANDLE process_handle = OpenProcess(
               PROCESS_CREATE_THREAD  |  // For CreateRemoteThread
               PROCESS_VM_OPERATION   |  // For VirtualAllocEx/VirtualFreeEx
               PROCESS_VM_WRITE       |  // For WriteProcessMemory
               PROCESS_DUP_HANDLE     |
               PROCESS_SUSPEND_RESUME,
               FALSE,                    // Don't inherit handles
               con->pid().dwProcessId);

      if (process_handle == INVALID_HANDLE_VALUE)
            util::win32::error_exit(L"OpenProcess()");

      auto *remote_buffer = static_cast<LPWSTR>(VirtualAllocEx(
          process_handle, nullptr, remote_buffer_size,
          MEM_COMMIT,
          PAGE_READWRITE
      ));
      if (!remote_buffer)
            util::win32::error_exit(L"VirtualAllocEx()");

      if (!WriteProcessMemory(process_handle, remote_buffer, dll_filename,
                              remote_buffer_size, nullptr))
            util::win32::error_exit(L"WriteProcessMemory()");

      // Get the real address of LoadLibraryW in Kernel32.dll
      LPTHREAD_START_ROUTINE LoadLibraryW_p;
      {
            auto *mod_hand = GetModuleHandleW(L"Kernel32");
            if (!mod_hand || mod_hand == INVALID_HANDLE_VALUE)
                  util::win32::error_exit(L"GetModuleHandleW()");

            // NOLINTNEXTLINE(clang-diagnostic-cast-function-type)
            LoadLibraryW_p = reinterpret_cast<LPTHREAD_START_ROUTINE>(
                GetProcAddress(mod_hand, "LoadLibraryW"));
            if (!LoadLibraryW_p)
                  util::win32::error_exit(L"GetProcAddress(GetModuleHandleW()");
      }

      void *const remote_thread_handle = CreateRemoteThread(
          process_handle, nullptr, 0,
          LoadLibraryW_p,
          remote_buffer, 0, nullptr);

      if (!remote_thread_handle || remote_thread_handle == INVALID_HANDLE_VALUE)
            util::win32::error_exit(L"CreateRemoteThread()");
#endif

#ifdef _WIN32
      ResumeThread(con->pid().hThread);
      try {
            con->impl().accept();
      } catch (std::exception const &e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            throw;
      }
#endif

      loop->open_poll_handle("clangd", con->raw_descriptor(), con.get());
      loop->start_poll_handle("clangd", con->poll_callback());

      //loop->open_pipe_handle("clangd", uv_open_osfhandle((uv_os_fd_t)con->raw_descriptor()), con.get());
      //loop->use_pipe_handle("clangd",   con->impl().get_uv_handle(), clangd.get());
      //loop->start_pipe_handle("clangd", con->pipe_alloc_callback(), con->pipe_read_callback());

      loop->loop_start_async();

#if 0
      WaitForSingleObject(remote_thread_handle, INFINITE);
      util::eprint(FC("Injected thread has terminated.\n"));
      CloseHandle(remote_thread_handle);

      if (!VirtualFreeEx(process_handle, remote_buffer, 0, MEM_RELEASE))
            util::eprint(
                FC("Unable to free memory via VirtualFreeEx(): {}\n"),
                std::error_code(GetLastError(), std::system_category()).message());

      CloseHandle(process_handle);
#endif
      std::this_thread::sleep_for(1s);

      AUTOC fname_uri = util::glib::unique_ptr_glib<gchar>{g_filename_to_uri((gchar const *)lazy::fname_raw.data(), "", nullptr)};
      AUTOC fname_len = static_cast<rapidjson::SizeType>(strlen(fname_uri.get()));
      {
            AUTOC init    = ipc::lsp::data::init_msg(lazy::path_raw.data());
            AUTOC content = util::slurp_file(lazy::fname_raw);
            auto  wrap    = con->get_new_packer();

            con->write_string(init);

            wrap().add_member("jsonrpc", "2.0");
            wrap().add_member("method", "textDocument/didOpen");
            wrap().set_member("params");
            wrap().set_member("textDocument");
            wrap().add_member("uri", fname_uri.get(), fname_len);
            wrap().add_member("text", content.data(), content.size());
            wrap().add_member("version", 1);
            wrap().add_member("languageId", "cpp");

            con->write_object(wrap());
      }
      {
            auto wrap = con->get_new_packer();
            wrap().add_member("jsonrpc", "2.0");
            wrap().add_member("id", 1);
            wrap().add_member("method", "textDocument/semanticTokens/full");
            wrap().set_member("params");
            wrap().set_member("textDocument");
            wrap().add_member("uri", fname_uri.get(), fname_len);

            con->write_object(wrap);
      }
      {
            auto wrap = con->get_new_packer();
            wrap().add_member ("jsonrpc", "2.0");
            wrap().add_member ("id", 2);
            wrap().add_member ("method", "textDocument/semanticTokens/full/delta");
            wrap().set_member ("params");
            wrap().add_member ("previousResultId", "1");
            wrap().set_member ("textDocument");
            wrap().add_member ("uri", fname_uri.get(), fname_len);

            con->write_object(wrap);
      }

      std::this_thread::sleep_for(100ms);
      //loop->loop_start();

      //loop->wait();
#ifdef _WIN32
      WaitForSingleObject(con->pid().hProcess, INFINITE);
      GetExitCodeProcess(con->pid().hProcess, &status);
#else
      waitpid(con->pid(), reinterpret_cast<int *>(&status), 0);
      con->wait();
#endif
      con->waitpid();

      util::eprint(FC("Process has terminated with status {}.\n"), status);
      std::this_thread::sleep_for(10min);
}
#endif


NOINLINE static void donloh()
{
#define O(...) nlohmann::json::object( {__VA_ARGS__} ) //NOLINT(cppcoreguidelines-macro-usage)

      auto foo = O(
          {"jsonrpc", "2.0"},
          {"method",  "textDocument/didOpen"},
          {"params",  O({"textDocument",
                         O({"languageId", "cpp"},
                           {"version", 1},
                           {"uri",  util::recode<char8_t>(lazy::fname_raw)},
                           {"text", util::recode<char8_t>(lazy::path_raw)})
                         })
          }
      );

      std::cout << std::setw(4) << foo << std::setw(0) << std::endl;

#undef O
}

NOINLINE static void dorapid()
{
      rapidjson::Document doc(rapidjson::kObjectType);
      rapidjson::Value ob1(rapidjson::kObjectType);
      rapidjson::Value ob2(rapidjson::kObjectType);
      doc.AddMember("jsonrpc",      "2.0",                               doc.GetAllocator());
      doc.AddMember("method",       "textDocument/didOpen",              doc.GetAllocator());
      ob2.AddMember("languageId",   "cpp",                               doc.GetAllocator());
      ob2.AddMember("text",         util::recode<char>(lazy::path_raw),  doc.GetAllocator());
      ob2.AddMember("uri",          util::recode<char>(lazy::fname_raw), doc.GetAllocator());
      ob2.AddMember("version",      1,                                   doc.GetAllocator());
      ob1.AddMember("textDocument", std::move(ob2),                      doc.GetAllocator());
      doc.AddMember("params",       std::move(ob1),                      doc.GetAllocator());

      rapidjson::StringBuffer sb;
      rapidjson::PrettyWriter wr{sb, &doc.GetAllocator()};
      doc.Accept(wr);
      std::cout << std::string_view(sb.GetString(), sb.GetLength()) << std::endl;
}

NOINLINE static void dodumbrapid()
{
#if 0
      rapidjson::Document doc(rapidjson::kObjectType);
      rapidjson::Value ob1(rapidjson::kObjectType);
      rapidjson::Value ob2(rapidjson::kObjectType);
      doc.AddMember("jsonrpc",      "2.0",                  doc.GetAllocator());
      doc.AddMember("method",       "textDocument/didOpen", doc.GetAllocator());
      ob2.AddMember("languageId",   "cpp",                  doc.GetAllocator());
      ob2.AddMember("text",         lazy::path_raw,         doc.GetAllocator());
      ob2.AddMember("uri",          lazy::fname_raw,        doc.GetAllocator());
      ob2.AddMember("version",      1,                      doc.GetAllocator());
      ob1.AddMember("textDocument", std::move(ob2),         doc.GetAllocator());
      doc.AddMember("params",       std::move(ob1),         doc.GetAllocator());
#endif

      ipc::json::rapid_doc wrap;
      wrap.add_member("jsonrpc", "2.0");
      wrap.add_member("method", "textDocument/didOpen");
      wrap.set_member("params");
      wrap.set_member("textDocument");
      wrap.add_member("languageId", "cpp");
      wrap.add_member("text", util::recode<char>(lazy::path_raw));
      wrap.add_member("uri", util::recode<char>(lazy::fname_raw));
      wrap.add_member("version", 1);

      rapidjson::StringBuffer sb;
      rapidjson::Writer       wr{sb, &wrap.alloc()};
      wrap.doc().Accept(wr);
      std::cout << std::string_view(sb.GetString(), sb.GetLength()) << std::endl;
}

NOINLINE void foo04()
{
      donloh();
      dorapid();
      dodumbrapid();
}


NOINLINE void foo10(char const *addr)
{
      using con_type   = ipc::connections::inet_socket;
      using proto_type = ipc::protocols::Msgpack::connection<con_type>;

      auto nvim  = proto_type::new_shared();
      auto &impl = dynamic_cast<con_type::connection_impl_type &>(nvim->impl());
      impl.should_open_listener(false);
      impl.resolve(addr);
      impl.connect();

      util::eprint(FC("Connected to {}!\n"), addr);
      nvim->close();
}


/****************************************************************************************/
} // namespace testing
} // namespace emlsp
