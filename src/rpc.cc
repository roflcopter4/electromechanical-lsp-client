#include "Common.hh"
#include "rapid.hh"

#include <fmt/core.h>
#include <glib.h>
#include <nlohmann/json.hpp>
#include <stack>
#include "util/myerr.h"

#ifdef true
#  warning "true was a macro!"
#  undef true
#endif
#ifdef false
#  warning "false was a macro!"
#  undef false
#endif

#define PRINT(FMT, ...)  fmt::print(FMT_COMPILE(FMT) __VA_OPT__(,) __VA_ARGS__)
#define FORMAT(FMT, ...) fmt::format(FMT_COMPILE(FMT) __VA_OPT__(,) __VA_ARGS__)

namespace emlsp::rpc::json
{

static int attempt_clangd_pipe(int *writefd, int *readfd);
static void test_read(GPid pid, int writefd, int readfd);

__attribute__((__noinline__))
void test2()
{
      using nlohmann::json;

      int writefd = -1;
      int readfd  = -1;
      GPid pid    = attempt_clangd_pipe(&writefd, &readfd);
      auto obj    = json::object({
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
      });

      {
            std::stringstream ss;
            ss << obj;
            std::string msg = ss.str();
            std::string header = fmt::format(FMT_COMPILE("Content-Length: {}\r\n\r\n"), msg.length());
            fmt::print(FMT_COMPILE("Writing string:\n{}\n"), msg);
            write(writefd, msg.data(), msg.length());
      }

      test_read(pid, writefd, readfd);
}

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

      ND inline Allocator &get_allocator() { return allocator_; }

      template <typename T>
      inline void addmember(StringRefType name, T &&value)
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

      ND inline Allocator &get_allocator() { return allocator_; }

      template <typename T>
      inline void addmember(StringRefType &&name, T &&value)
      {
            (void)AddMember(std::move(name), std::move(value), allocator_);
      }
};

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

      ND inline Allocator &get_allocator() { return allocator_; }
      ND inline ValueType &&yield() { return std::move(obj_); }
      ND inline ValueType &get()    { return obj_; }
      ND inline ValueType &operator() () { return obj_; }

      template <typename T>
      __attribute__((__always_inline__))
      inline void addmember(rapidjson::GenericStringRef<rapidjson::UTF8<>::Ch> name, T &&value)
      {
            // rapidjson::Value n(std::move(name));
            // rapidjson::Value v(std::forward<T &>(value));
            (void)obj_.AddMember(std::move(name), std::move(value), allocator_);
      }
};

__attribute__((__always_inline__))
inline Wrapper<rapidjson::Value>
wrap_new_object(rapidjson::MemoryPoolAllocator<> &allocator)
{
      return {allocator, rapidjson::Value(rapidjson::Type::kObjectType)};
}

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
      rapidjson::Writer       writer(ss);
      // doc->get().Accept(writer);
      doc->get().Accept(writer);

      {
            std::string msg    = ss.GetString();
            std::string header = fmt::format(FMT_COMPILE("Content-Length: {}\r\n\r\n"), msg.length());
            fmt::print(FMT_COMPILE("Writing string:\n{}{}\n"), header, msg);

            write(writefd, header.data(), header.length());
            write(writefd, msg.data(), msg.length());
      }

      test_read(pid, writefd, readfd);
      delete doc;
}

namespace
{

class Wrapper
{
    private:
      using Document  = rapidjson::Document;
      using Value     = rapidjson::Value;
      using Type      = rapidjson::Type;
      using StringRef = rapidjson::GenericStringRef<rapidjson::UTF8<char>::Ch>;

      Document doc_;
      Value   *cur_;
      std::stack<Value *> stack_{};

    public:
      explicit Wrapper(Type const ty = Type::kObjectType) : doc_(ty), cur_(&doc_)
      {}
      ~Wrapper() = default;

      template <typename T>
      inline void add_member(StringRef name, T &&value)
      {
            (void)cur_->AddMember(std::move(name), std::move(value), doc_.GetAllocator());
      }

      template <typename T>
      inline void add_value(T &&value)
      {
            (void)cur_->PushBack(std::move(value));
      }

      void push(Type const ty = Type::kObjectType)
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

      void pop() 
      {
            assert(!stack_.empty());
            cur_ = stack_.top();
            stack_.pop();
      }

      Document &doc()        { return doc_; }
      Value    *get()        { return cur_; }
      Value    *operator()() { return cur_; }

    private:
      DELETE_COPY_CTORS(Wrapper);
};

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
      wrp->pop();
      wrp->pop();


      rapidjson::StringBuffer ss;
      rapidjson::Writer       writer(ss);
      wrp->doc().Accept(writer);

      std::string msg    = ss.GetString();
      std::string header = fmt::format(FMT_COMPILE("Content-Length: {}\r\n\r\n"), msg.length());
      fmt::print(FMT_COMPILE("({}) Writing string:\n{}{}\n"), __PRETTY_FUNCTION__, header, msg);

      {
            int    writefd = -1;
            int    readfd  = -1;
            GPid   pid     = attempt_clangd_pipe(&writefd, &readfd);
            write(writefd, header.data(), header.length());
            write(writefd, msg.data(), msg.length());
            test_read(pid, writefd, readfd);
            close(writefd);
            waitpid(pid, nullptr, 0);
      }

      delete wrp;
}

} // namespace


__attribute__((__noinline__))
void test3()
{
      test3_();
}


static void
test_read(GPid const pid, int const writefd, int const readfd)
{
      size_t msglen;
      {
            char ch;
            char buf[128];
            char *ptr = buf;
            read(readfd, buf, 16); // Discard
            read(readfd, &ch, 1);  // Read 1st potential digit

            while (isdigit(ch)) {
                  *ptr++ = ch;
                  read(readfd, &ch, 1);
            }

            if (ptr == buf)
                  throw std::runtime_error("No message length specified in jsonrpc header.");

            *ptr = '\0';
            msglen = strtoull(buf, nullptr, 10);
            read(readfd, buf, 3);
      }

      char *msg = new char[msglen + 1];
      read(readfd, msg, msglen);
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
      close(writefd);
      close(readfd);
      waitpid(pid, nullptr, 0);
}

static int
attempt_clangd_pipe(int *writefd, int *readfd)
{
      GError *gerr = nullptr;
      GPid pid     = -1;

      static constexpr char const *const argv[] = { 
            "clangd", "--log=verbose", "--pch-storage=memory", "--pretty", nullptr
      };

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
      
#if 0
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
#endif

      if (gerr != nullptr)
            throw std::runtime_error(FORMAT("glib error: {}", gerr->message));

      return pid;
}

} // namespace emlsp::rpc::json
