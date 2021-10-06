// ReSharper disable CppInconsistentNaming
#include "Common.hh"
#include "rapid.hh"
#include "util/myerr.h"

#include <glib.h>
#include <nlohmann/json.hpp>
#include <stack>

#ifdef HAVE_THREADS_H
#  include <threads.h>
#endif

#ifdef true
#  warning "true was a macro!"
#  undef true
#endif
#ifdef false
#  warning "false was a macro!"
#  undef false
#endif

#define PRINT(FMT, ...)  fmt::print(FMT_COMPILE(FMT),  ##__VA_ARGS__)
#define FORMAT(FMT, ...) fmt::format(FMT_COMPILE(FMT), ##__VA_ARGS__)

namespace emlsp::rpc::json
{
struct socket_info {
      std::string path;
      socket_t    main_sock;
      socket_t    connected_sock;
      socket_t    accepted_sock;

#ifdef _WIN32
      PROCESS_INFORMATION pid;
#else
      pid_t pid;
#endif
};

#undef MSG_WAITALL

UNUSED static socket_info *attempt_clangd_sock();
UNUSED static void         test_recv(socket_t readfd);

static GPid attempt_clangd_pipe(int *writefd, int *readfd);
static void test_read(int readfd);

/****************************************************************************************/

__attribute__((__noinline__))
void test2()
{
      using nlohmann::json;

      int writefd = -1;
      int readfd  = -1;
      GPid pid    = attempt_clangd_pipe(&writefd, &readfd);
      auto obj    = json::object( {
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", "initialize"},
            {"params", {
                  {"clientInfo", {
                        {"name", "retard"},
                        {"version", "0.0.1"},
                  }},
                  {"capabilities", {
                        {"depression", true},
                        {"failure", true},
                        {"confidence", false}
                  }}
            }}
      } );

      {
            std::stringstream ss;
            ss << obj;
            std::string msg = ss.str();
            std::string header = FORMAT("Content-Length: {}\r\n\r\n", msg.length());
            PRINT("Writing string:\n{}\n", msg);
            (void)write(writefd, msg.data(), msg.length());
      }

      test_read(readfd);
      close(readfd);
      close(writefd);
}

/****************************************************************************************/

template <typename Allocator> class Document;
template <typename Allocator> class Object;

template <typename Allocator>
class Object : public rapidjson::Value
{
      Allocator &allocator_;
    public:
      friend class emlsp::rpc::json::Document<Allocator>;

      explicit Object(Allocator &alloc)
          : rapidjson::Value(rapidjson::Type::kObjectType), allocator_(alloc)
      {}

      ND Allocator &get_allocator() { return allocator_; }

      template <typename T>
      void addmember(StringRefType name, T &&value)
      {
            (void)AddMember((name), (value), allocator_);
      }

};

template <typename Allocator>
class Document : public rapidjson::Document
{
      Allocator &allocator_;
    public:
      friend class emlsp::rpc::json::Object<Allocator>;

      explicit Document(rapidjson::Type const ty) : rapidjson::Document(ty), allocator_(GetAllocator()) { }

      ND Allocator &get_allocator() { return allocator_; }

      template <typename T>
      void addmember(StringRefType &&name, T &&value)
      {
            (void)AddMember(std::move(name), std::move(value), allocator_);
      }
};

/*--------------------------------------------------------------------------------------*/

template <typename ValueType>
class Wrapper
{
      using Allocator = rapidjson::MemoryPoolAllocator<>;
      Allocator &allocator_;
      ValueType obj_;

    public:
      explicit Wrapper(ValueType &&obj)
          : allocator_(obj.GetAllocator()), obj_(std::move(obj))
      {}

      Wrapper(Allocator &allocator, ValueType &&obj)
          : allocator_(allocator), obj_(std::move(obj))
      {}

      ND Allocator  &get_allocator() const { return allocator_; }
      ND ValueType &&yield()      { return std::move(obj_); }
      ND ValueType  &get()        { return obj_; }
      ND ValueType  &operator()() { return obj_; }

      template <typename T>
      __attribute__((__always_inline__))
      void addmember(rapidjson::GenericStringRef<rapidjson::UTF8<>::Ch> name, T &&value)
      {
            (void)obj_.AddMember(std::move(name), std::move(value), allocator_);
      }
};

__attribute__((__always_inline__))
inline Wrapper<rapidjson::Value>
wrap_new_object(rapidjson::MemoryPoolAllocator<> &allocator)
{
      return {allocator, rapidjson::Value(rapidjson::Type::kObjectType)};
}

/*--------------------------------------------------------------------------------------*/

__attribute__((__noinline__))
void test1()
{
#define ADDMEMBER(a, b) AddMember((a), (b), allocator)
#define PUSH(x) PushBack((x), allocator)

      // using rapidjson::Document;
      // using rapidjson::Type;
      // using rapidjson::Value;

      int    writefd = -1;
      int    readfd  = -1;
      GPid   pid     = attempt_clangd_pipe(&writefd, &readfd);
      // auto   doc     = std::make_unique<rapidjson::Document>(rapidjson::Type::kObjectType);
      // auto docinst = Document{rapidjson::Type::kObjectType};
      // auto doc = &docinst;


#if 0
      auto doc = std::make_unique<Document<rapidjson::MemoryPoolAllocator<>>
                                 >(rapidjson::Type::kObjectType);
#endif

      auto doc = new Wrapper<rapidjson::Document>(rapidjson::Document{rapidjson::kObjectType});

      // auto &&allocator = doc->GetAllocator();

      doc->addmember("jsonrpc", "2.0");
      doc->addmember("id", 1);
      doc->addmember("method", "initialize");

      {
            // Object obj(doc->get_allocator());
            // auto obj = rapidjson::Value(rapidjson::Type::kObjectType);
            // auto obj = Wrapper{rapidjson::Value{rapidjson::kObjectType}, doc->get_allocator()};
            auto obj = wrap_new_object(doc->get_allocator());
            // obj.SetObject();

            {
                  auto subobj = wrap_new_object(doc->get_allocator());
                  // Object subobj(doc->get_allocator());
                  // auto subobj = rapidjson::Value(rapidjson::Type::kObjectType);
                  subobj.addmember("depression", true);
                  subobj.addmember("failure", true);
                  subobj.addmember("confidence", false);
                  obj.addmember("capabilities", std::move(subobj.yield()));
            }
            {
                  auto subobj = wrap_new_object(doc->get_allocator());
                  // Object subobj(doc->get_allocator());
                  // auto subobj = rapidjson::Value(rapidjson::Type::kObjectType);
                  subobj.addmember("name", "retard");
                  subobj.addmember("version", rapidjson::StringRef("0.0.1"));
                  obj.addmember("clientInfo", std::move(subobj.yield()));
            }

            doc->addmember("params", std::move(obj.yield()));
      }

      rapidjson::StringBuffer ss;
      rapidjson::Writer       writer{ss};
      // doc->get().Accept(writer);
      doc->get().Accept(writer);

      {
            std::string msg    = ss.GetString();
            std::string header = FORMAT("Content-Length: {}\r\n\r\n", msg.length());
            PRINT("Writing string:\n{}{}\n", header, msg);

            (void)write(writefd, header.data(), header.length());
            (void)write(writefd, msg.data(), msg.length());
      }

      test_read(readfd);
      close(readfd);
      close(writefd);
      delete doc;
}

/****************************************************************************************/

namespace
{

class Wrapper
{
      using Document  = rapidjson::Document;
      using Value     = rapidjson::Value;
      using Type      = rapidjson::Type;
      using StringRef = rapidjson::GenericStringRef<rapidjson::UTF8<char>::Ch>;

      Document doc_;
      Value   *cur_;
      std::stack<Value *> stack_{};

    public:
      explicit Wrapper(Type const ty = Type::kObjectType) : doc_(ty), cur_(&doc_) {}
      ~Wrapper() = default;

      template <typename T>
      void add_member(StringRef name, T &&value)
      {
            (void)cur_->AddMember(std::move(name), std::move(value), doc_.GetAllocator());
      }

      template <typename T>
      void add_value(T &&value)
      {
            (void)cur_->PushBack(std::move(value), doc_.GetAllocator());
      }

      UNUSED void push(Type const ty = Type::kObjectType)
      {
            cur_->PushBack(rapidjson::Value(ty), doc_.GetAllocator());
            stack_.push(cur_);
            cur_ = cur_->End();
      }

      void push_member(StringRef &&name, Type const ty = Type::kObjectType)
      {
            cur_->AddMember(name, rapidjson::Value(ty), doc_.GetAllocator());
            stack_.push(cur_);
            cur_ = &cur_->FindMember(std::forward<StringRef &>(name))->value;
      }

      void pop(int const n = 1)
      {
            for (int i = 0; i < (n - 1); ++i)
                  stack_.pop();
            assert(!stack_.empty());
            cur_ = stack_.top();
            stack_.pop();
      }

      Document &doc() { return doc_; }
      Value    *operator()() const { return cur_; }

      UNUSED ND Value *get() const { return cur_; }

      Wrapper(Wrapper const &)                = delete;
      Wrapper(Wrapper &&) noexcept            = delete;
      Wrapper &operator=(Wrapper const &)     = delete;
      Wrapper &operator=(Wrapper &&) noexcept = delete;
};

/*--------------------------------------------------------------------------------------*/

__attribute__((__always_inline__))
void test3_()
{
      auto *wrp = new Wrapper();
      wrp->add_member("jsonrpc", "2.0");
      wrp->add_member("id", 1);
      wrp->add_member("method", "initialize");
      wrp->push_member("params");
      wrp->push_member("capabilities");
      wrp->add_member("depression", true);
      wrp->add_member("failure", true);
      wrp->add_member("confidence", false);
      wrp->pop();
      wrp->push_member("clientInfo");
      wrp->add_member("name", "retard");
      wrp->add_member("version", "0.0.1");
      wrp->push_member("test", rapidjson::Type::kArrayType);
      wrp->add_value("test");
      wrp->pop(3);


      rapidjson::StringBuffer ss;
      rapidjson::Writer       writer(ss);
      wrp->doc().Accept(writer);

      std::string const &msg   = ss.GetString();
      std::string const header = FORMAT("Content-Length: {}\r\n\r\n", msg.length());
      PRINT("({}) Writing string:\n{}{}\n", __FUNCTION__, header, msg);

      {
#if 0
            socket_info const *info = attempt_clangd_sock();

            (void)send(info->accepted_sock, header.data(), (int)header.length(), 0);
            (void)send(info->accepted_sock, msg.data(), (int)msg.length(), 0);

            test_recv(info->accepted_sock);

            closesocket(info->accepted_sock);
            closesocket(info->connected_sock);
            closesocket(info->main_sock);
            CloseHandle(info->pid.hThread);
            CloseHandle(info->pid.hProcess);
            delete info;
#endif

            int    writefd = -1;
            int    readfd  = -1;
            GPid   pid     = attempt_clangd_pipe(&writefd, &readfd);
            (void)write(writefd, header.data(), header.length());
            (void)write(writefd, msg.data(), msg.length());
            test_read(readfd);

            close(writefd);
            close(readfd);

#ifdef _WIN32
            g_spawn_close_pid(pid);
#else
            waitpid(pid, nullptr, 0);
#endif
      }

      delete wrp;
}

} // namespace


__attribute__((__noinline__))
void test3()
{
      test3_();
}

/****************************************************************************************/

static void
test_read(int const readfd)
{
      size_t msglen;
      {
            char ch;
            char buf[128];
            char *ptr = buf;
            (void)read(readfd, buf, 16); // Discard
            (void)read(readfd, &ch, 1);  // Read 1st potential digit

            while (isdigit(ch)) {
                  *ptr++ = ch;
                  (void)read(readfd, &ch, 1);
            }

            if (ptr == buf)
                  throw std::runtime_error("No message length specified in jsonrpc header.");

            *ptr = '\0';
            msglen = strtoull(buf, nullptr, 10);
            (void)read(readfd, buf, 3);
      }

      auto *msg = new char[msglen + 1];
      (void)read(readfd, msg, msglen);
      msg[msglen] = '\0';
      {
            auto obj = rapidjson::Document();
            obj.ParseInsitu<rapidjson::kParseInsituFlag>(msg);

            rapidjson::StringBuffer ss;
            rapidjson::PrettyWriter writer(ss);
            obj.Accept(writer);

            std::cout << "\n\n----- Object (rapidjson):\n" << ss.GetString() << std::endl;
      }

      delete[] msg;
}

static void
test_recv(socket_t const readfd)
{
      size_t msglen;
      {
            char ch;
            char buf[128];
            char *ptr = buf;
            (void)recv(readfd, buf, 16, MSG_WAITALL); // Discard
            (void)recv(readfd, &ch, 1, MSG_WAITALL);  // Read 1st potential digit

            while (isdigit(ch)) {
                  *ptr++ = ch;
                  (void)recv(readfd, &ch, 1, MSG_WAITALL);
            }

            if (ptr == buf)
                  throw std::runtime_error("No message length specified in jsonrpc header.");

            *ptr = '\0';
            msglen = strtoull(buf, nullptr, 10);
            (void)recv(readfd, buf, 3, MSG_WAITALL);
      }

      auto *msg = new char[msglen + 1];
      (void)recv(readfd, msg, msglen, MSG_WAITALL);
      msg[msglen] = '\0';
      {
            auto obj = rapidjson::Document();
            obj.ParseInsitu<rapidjson::kParseInsituFlag>(msg);

            rapidjson::StringBuffer ss;
            rapidjson::PrettyWriter writer(ss);
            obj.Accept(writer);

            std::cout << "\n\n----- Object (rapidjson):\n" << ss.GetString() << std::endl;
      }

      delete[] msg;
}

/*--------------------------------------------------------------------------------------*/

#ifdef _WIN32
NORETURN static int
process_startup_thread(void *varg)
{
      auto *info = static_cast<socket_info *>(varg);

      try {
            info->connected_sock = util::rpc::connect_to_socket(info->path.c_str());
      } catch (std::exception &e) {
            std::cerr << "Caught exception \"" << e.what()
                      << "\" while attempting to connect to socket at "
                      << info->path << std::endl;
            thrd_exit(WSAGetLastError());
      }

      char cmdline[256];
      strcpy_s(cmdline, "clangd --pch-storage=memory --log=verbose --pretty");

      STARTUPINFOA startinfo;
      memset(&startinfo, 0, sizeof(startinfo));
      startinfo.dwFlags    = STARTF_USESTDHANDLES;
      startinfo.hStdInput  = reinterpret_cast<HANDLE>(info->connected_sock); //NOLINT
      startinfo.hStdOutput = reinterpret_cast<HANDLE>(info->connected_sock); //NOLINT

      bool b = CreateProcessA(nullptr, cmdline, nullptr, nullptr, true,
                              CREATE_NEW_CONSOLE, nullptr, nullptr,
                              &startinfo, &info->pid);
      auto const error = GetLastError();
      thrd_exit(static_cast<int>(b ? 0LU : error));
}
#endif

static socket_info *
attempt_clangd_sock()
{
      auto const tmppath = util::get_temporary_filename();
      auto const sock    = util::rpc::open_new_socket(tmppath.string().c_str());
      auto      *info    = new socket_info{tmppath.string(), sock, {}, {}, {}};

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

      info->accepted_sock = connected_sock;
      return info;
}

static GPid
attempt_clangd_pipe(int *writefd, int *readfd)
{
      static constexpr char const *const argv[] = {
            "clangd", "--log=verbose", "--pch-storage=memory", "--pretty", nullptr
      };

      GError *gerr = nullptr;
      GPid pid;

      g_spawn_async_with_pipes(
             nullptr,
             const_cast<char **>(argv),
             environ,
             GSpawnFlags(G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD),
             nullptr,
             nullptr,
             &pid,
             writefd,
             readfd,
             nullptr,
             &gerr
      );

      if (gerr != nullptr)
            throw std::runtime_error(FORMAT("glib error: {}", gerr->message));

      return pid;
}

} // namespace emlsp::rpc::json
