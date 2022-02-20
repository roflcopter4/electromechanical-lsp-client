#include "Common.hh"
#include "ipc/lsp/lsp-client.hh"
#include "ipc/lsp/lsp-connection.hh"
#include "ipc/lsp/static-data.hh"
#include "libevent.hh"

// #include "ipc/rpc/basic_msgpack_connection.hh"
#include "ipc/rpc/basic_wrapper.hh"

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
      using conn_type = ipc::connections::std_streams;
      using wrapper   = ipc::rpc::msgpack_wrapper<conn_type>;
      // using wrapper   = rpc::msgpack_connection;
      // using wrapper   = ipc::rpc::msgpack_connection<conn_type>;

      auto con = conn_type::new_shared();
      auto rd  = std::make_unique<wrapper>(*con);

#if 0
      int orig = fcntl(0, F_GETFL);
      fcntl(0, F_SETFL, orig | O_NONBLOCK);
      std::string in;

      while (!feof(stdin)) {
            char buf[8192];
            size_t const nread = fread(buf, 1, sizeof buf, stdin);
            in.append(buf, nread);
      }
#endif

#if 0
      std::this_thread::sleep_for(2000ms);
      auto obj1 = rd->read_object();
      fmt::print(stderr, "Read obj... uh... {}\n", obj1->type);
      auto obj2 = rd->read_object();
      fmt::print(stderr, "Read obj... uh... {}\n", obj2->type);
      auto obj3 = rd->read_object();
      fmt::print(stderr, "Read obj... uh... {}\n", obj2->type);
#endif

      auto pack1 = rd->get_packer();

      double x = 2.0;

      pack1->pk.pack_array(3);
      pack1->pk << "hello";
      pack1->pk << M_PI;

      __asm__ volatile (
            "vsqrtsd %[in], %[in], %[out]\n\t"
            : [out] "=v" (x)
            : [in] "v" (x)
            :
      );

      pack1->pk.pack_array(4);
      pack1->pk << x;
      pack1->pk << 'q';
      pack1->pk << "hello"sv;
      pack1->pk << ~0LLU;


      {
            wrapper::packer_ptr arr[7] = {
                rd->get_packer(), rd->get_packer(), rd->get_packer(), rd->get_packer(),
                rd->get_packer(), rd->get_packer(), rd->get_packer(),
            };

            for (auto &obj : arr) {
                  obj->pk.pack_array(1);
                  obj->pk<< "hi";
            }

            for (auto &obj : arr) {
                  rd->write_object(obj->vbuf);
                  rd->con().raw_write("\n"sv);
            }
      }
      {
            wrapper::packer_ptr arr[7] = {
                rd->get_packer(), rd->get_packer(), rd->get_packer(), rd->get_packer(),
                rd->get_packer(), rd->get_packer(), rd->get_packer(),
            };

            for (auto &obj : arr) {
                  obj->pk.pack_array(3);
                  obj->pk << true << -1LL;
                  obj->pk.pack_nil();
            }

            for (auto &obj : arr) {
                  rd->write_object(obj->vbuf);
                  rd->con().raw_write("\n"sv);
            }
      }

#define PACK(N) pack ## N

      rd->write_object(pack1->vbuf);
      rd->con().raw_write("\n"sv);

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
}


/****************************************************************************************/
} // namespace rpc::mpack
} // namespace emlsp
