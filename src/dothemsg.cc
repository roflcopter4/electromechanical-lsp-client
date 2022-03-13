#include "Common.hh"
#include "ipc/lsp/static-data.hh"
#include "ipc/rpc/basic_wrapper.hh"
#include "ipc/totality.hh"
#include "ipc/rpc/basic_io_connection.hh"

//#include <nlohmann/json.hpp>

// #include <msgpack.h>

#define AUTOC auto const


inline namespace emlsp {
namespace rpc::mpack {
/****************************************************************************************/


#define ME_VERY_LAZY(T) std::remove_reference_t<decltype(T)>


#if 0
template <typename ConnectionType>
      REQUIRES(ipc::IsBasicConnectionVariant<ConnectionType>)
class msgpack_wrapper
{
    public:
      using conn_type = ConnectionType;
      using this_type = msgpack_wrapper<ConnectionType>;

    private:
      // 1 << 23 aka 8 MiB;
      static constexpr size_t    read_buffer_size = SIZE_C(8'388'608);
      std::shared_ptr<conn_type> con_;
      msgpack::unpacker          unpacker_{};
      std::mutex                 mtx_;

    public:
      explicit msgpack_wrapper(std::shared_ptr<conn_type> con)
            : con_(std::move(con))
      {
            unpacker_.m_user_data = this;
            unpacker_.reserve_buffer(read_buffer_size);
      }

      ~msgpack_wrapper() = default;
      DELETE_COPY_CTORS(msgpack_wrapper);
      DELETE_MOVE_CTORS(msgpack_wrapper);

      static std::unique_ptr<msgpack_wrapper> new_unique(std::shared_ptr<conn_type> con)
      {
            return std::make_unique<msgpack_wrapper>(std::move(con));
      }

      /*--------------------------------------------------------------------------------*/

      msgpack::object_handle read_object()
      {
            msgpack::object_handle obj;
            ssize_t                nread;

            while (!unpacker_.next(obj)) {
                  if (unpacker_.buffer_capacity() == 0)
                        unpacker_.reserve_buffer();

                  do {
                        nread = con_->raw_read(unpacker_.buffer(),
                                               unpacker_.buffer_capacity(), 0);
                        if (nread == (-1) && errno != EAGAIN)
                              err(1, "read()");
                  } while (nread <= 0);

                  unpacker_.buffer_consumed(nread);
            }

            if (unpacker_.nonparsed_size() > 0)
                  fmt::print(stderr, FC("Warning: {} bytes still unparsed.\n"),
                             unpacker_.nonparsed_size());

            return obj;
      }

      size_t write_object(msgpack::v3::vrefbuffer const &vec)
      {
            size_t total = 0;
            for (size_t i = 0; i < vec.vector_size(); ++i) {
                  auto const &iov = vec.vector()[i];
                  total += con_->raw_write(iov.iov_base, iov.iov_len);
            }
            return total;
      }

      /*--------------------------------------------------------------------------------*/

    private:
      class packer
      {
            std::atomic_bool in_use_ = false;

            std::mutex              mtx_  = {};
            std::condition_variable cond_ = {};

            static void deleter(packer *ptr)
            {
                  ptr->vbuf.clear();
                  ptr->in_use_.store(false, std::memory_order::release);
                  ptr->in_use_.notify_one();
            }

            // std::unique_ptr<packer, decltype(&deleter)> get_packer()
            packer & get_packer() & noexcept 
            {
                  bool expect = false;
                  if (!in_use_.compare_exchange_weak(expect, true,
                                                     std::memory_order::seq_cst,
                                                     std::memory_order::acquire))
                  {
                        auto lock = std::unique_lock<std::mutex>{mtx_};
                        cond_.wait_for(lock, 5ms);
                        expect = false;
                  }

                  return *this;
            }

          public:
            packer()  = default;
            ~packer() = default;
            DEFAULT_MOVE_CTORS(packer);
            DELETE_COPY_CTORS(packer);
            friend this_type;

            msgpack::vrefbuffer                  vbuf{};
            msgpack::packer<msgpack::vrefbuffer> pk{vbuf};
      };

      std::array<packer, 8> packs_       = {};
      std::atomic_uint      packs_index_ = 0;

    public:
      ND auto &get_packer() & noexcept 
      {
            unsigned n = packs_index_.fetch_add(1) % std::size(packs_);
            return packs_[n].get_packer();
      }

      size_t write_object(packer const &pack) { return write_object(pack.vbuf); }
};
#endif


/****************************************************************************************/


NOINLINE void
test01()
{
#if 0
      using conn_type = ipc::connections::std_streams;
      using wrapper   = ipc::rpc::msgpack_io_wrapper<conn_type>;

      auto con       = conn_type::new_unique();
      auto iowrapper = std::make_unique<wrapper>(con.get());
      auto pack1     = iowrapper->get_packer();

      {
            auto &pk = pack1->pk;
            double x = 2.0;

            pk.pack_array(3);
            pk << "hello";
            pk << 3.1415926535897932384626433832795;

#if 0
            __asm__ volatile (
                  "vsqrtsd %[in], %[in], %[out]\n\t"
                  : [out] "=v" (x)
                  : [in] "v" (x)
                  :
            );
#endif

            pk.pack_array(4);
            pk << x;
            pk << 'q';
            pk << "hello"sv;
            pk << ~0LLU;
      }


      {
            wrapper::packer_container arr[7] = {
                iowrapper->get_packer(), iowrapper->get_packer(), iowrapper->get_packer(),
                iowrapper->get_packer(), iowrapper->get_packer(), iowrapper->get_packer(),
                iowrapper->get_packer(),
            };

            for (auto &obj : arr) {
                  obj->pk.pack_array(1);
                  obj->pk<< "hi";
            }

            for (auto &obj : arr) {
                  iowrapper->write_object(obj);
                  iowrapper->con()->raw_write("\n"sv);
            }
      }
      {
            wrapper::packer_container arr[7] = {
                iowrapper->get_packer(), iowrapper->get_packer(), iowrapper->get_packer(),
                iowrapper->get_packer(), iowrapper->get_packer(), iowrapper->get_packer(),
                iowrapper->get_packer(),
            };

            for (auto &obj : arr) {
                  obj->pk.pack_array(3);
                  obj->pk << true << -1LL;
                  obj->pk.pack_nil();
            }

            for (auto &obj : arr) {
                  iowrapper->write_object(obj);
                  iowrapper->con()->raw_write("\n"sv);
            }
      }

      iowrapper->write_object(pack1);
      iowrapper->con()->raw_write("\n"sv);
#endif

#if 0

      {
            auto reet = ipc::totality<ipc::connections::unix_socket,
                                      ipc::rpc::ms_jsonrpc_io_wrapper,
                                      ipc::event_tags::libevent
                        >::new_unique();

            reet->spawn_connection_l(R"(C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clangd.exe)", "--pch-storage=memory", "--log=verbose", "--clang-tidy");
            reet->start();
            auto pk = reet->wrapper().get_packer();
            pk().add_member("jsonrpc", "2");
            reet->stop();
      }

#endif

#if 0
      ssize_t           len;
      msgpack::unpacker upkr{};

      len = readall();
      msgpack::object_handle hand = msgpack::unpack(read_buffer_.get(), len);
      bool fin = upkr.next(hand);

      if (!fin) {
            if (len == read_buffer_size)
                  throw util::except::not_implemented("Sheesh, this is unlikely.");
      }
#endif

#if 1
      {
            //auto con = ipc::connections::pipe::new_shared();
            //auto con = std::make_unique<ipc::connections::pipe>();
            //auto x   = std::make_unique<ipc::rpc::ms_jsonrpc_io_wrapper<ipc::connections::pipe>>(con.get());

            auto reet = ipc::totality<ipc::connections::unix_socket,
                                      ipc::rpc::ms_jsonrpc_io_wrapper,
                                      ipc::event_tags::libevent
                        >::new_unique();

            reet->spawn_connection_l(R"(C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clangd.exe)", "--pch-storage=memory", "--log=verbose", "--clang-tidy");
            reet->start();
            auto pack = reet->wrapper().get_packer();
            pack().add_member("moran", 34);
            reet->wrapper().write_object(*pack);
            std::this_thread::sleep_for(5s);
      }
#endif
            std::this_thread::sleep_for(1s);

#if 0
      {
            // auto again = std::make_unique<ipc::rpc::basic_io_connection<
            //     ipc::connections::socketpair, ipc::rpc::ms_jsonrpc_io_wrapper>>();
            // auto cont = again->get_packer();

            // auto foo = nlohmann::json::object();
            // auto foo = nlohmann::json({{"hi", "there"}, {"do", 34}}, false, nlohmann::json::value_t::object);
            // foo.insert(foo.cbegin(), );

            auto foo = nlohmann::json::object();
            foo.emplace("jsonrpc", "2.0");
            foo.emplace("method", "textDocument/didOpen");
            auto obj2 = nlohmann::json::object();
            auto obj3 = nlohmann::json::object();
            obj3.emplace("uri", "THE QUAK BROON");
            obj3.emplace("text", "insert texate herr");
            obj3.emplace("version", 1);
            obj3.emplace("languageId", M_PI);
            obj2.emplace("textDocument", std::move(obj3));
            foo.emplace("params", std::move(obj2));
            foo["cunt"] = 34;

            std::cout << foo << std::endl;
      }
#endif

#if 0
      {
            ipc::json::rapid_doc wrap;
            wrap.add_member("jsonrpc", "2.0");
            wrap.add_member("method", "textDocument/didOpen");
            wrap.set_member("params");
            wrap.set_member("textDocument");
            wrap.add_member("uri", "THE QUAK BROON");
            wrap.add_member("text", "insert texate herr");
            wrap.add_member("version", 1);
            wrap.add_member("languageId", "cpp");
            wrap.add_member("cunt", 34);
            wrap.add_member("reet", 342819);

            rapidjson::MemoryBuffer ss;
            rapidjson::Writer       writer(ss);
            wrap.doc().Accept(writer);
            std::cout << std::string_view{ss.GetBuffer(), ss.GetSize()} << std::endl;
      }
#endif
#if 0
      {
            rapidjson::Document doc(rapidjson::kObjectType);
            doc.AddMember("jsonrpc", "2.0", doc.GetAllocator());
            doc.AddMember("method", "textDocument/didOpen", doc.GetAllocator());
            auto obj2 = rapidjson::Value(rapidjson::kObjectType);
            auto obj3 = rapidjson::Value(rapidjson::kObjectType);
            obj3.AddMember("uri", "THE QUAK BROON", doc.GetAllocator());
            obj3.AddMember("text", "insert texate herr", doc.GetAllocator());
            obj3.AddMember("version", 1, doc.GetAllocator());
            obj3.AddMember("languageId", M_PI, doc.GetAllocator());
            obj2.AddMember("params", std::move(obj3), doc.GetAllocator());
            doc.AddMember("textDocument", std::move(obj2), doc.GetAllocator());
            doc.AddMember("cunt", rapidjson::Value(rapidjson::kNumberType).Set(34), doc.GetAllocator());

            rapidjson::MemoryBuffer ss;
            rapidjson::Writer       writer(ss);
            doc.Accept(writer);
            std::cout << std::string_view{ss.GetBuffer(), ss.GetSize()} << std::endl;
      }
#endif
}


/****************************************************************************************/
} // namespace rpc::mpack
} // namespace emlsp
