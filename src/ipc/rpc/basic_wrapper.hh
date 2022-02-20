#pragma once
#ifndef HGUARD__IPC__RPC__BASIC_WRAPPER_HH_
#define HGUARD__IPC__RPC__BASIC_WRAPPER_HH_ //NOLINT

#include "Common.hh"
#include "ipc/base_connection.hh"

#include "rapid.hh"

inline namespace emlsp {
namespace ipc::rpc {
/****************************************************************************************/

// template <typename ConnectionType>
//       REQUIRES(ipc::IsBasicConnectionVariant<ConnectionType>)
// class msgpack_wrapper;

// template <typename ConnectionType>
//class msgpack_connection;


template <typename ConnectionType, typename Packer, typename Unpacker>
      REQUIRES(ipc::IsBasicConnectionVariant<ConnectionType>)
class basic_wrapper
{
      using this_type     = basic_wrapper;
      using conn_type     = ConnectionType;
      using packer_type   = Packer;
      using value_type    = typename packer_type::value_type;
      using buffer_type   = typename packer_type::buffer_type;
      using unpacker_type = Unpacker;

      friend Packer;

    public:
      explicit basic_wrapper(conn_type &con) : con_(con) {}
      virtual ~basic_wrapper() = default;

      DELETE_COPY_CTORS(basic_wrapper);
      DELETE_MOVE_CTORS(basic_wrapper);

      class packer_ptr
      {
            friend Packer;
            Packer *pack_ = nullptr;

          public:
            packer_ptr() = default;
            explicit packer_ptr(packer_type *pack) : pack_(pack) {}

            virtual ~packer_ptr()
            {
                  pack_->clear();
            }

            auto *operator->()   noexcept { return pack_; }
            auto &operator* () & noexcept { return *pack_; }
            auto &operator()() & noexcept { return pack_->pk; }

            auto const *operator->() const   noexcept { return pack_; }
            auto const &operator* () const & noexcept { return *pack_; }
            auto const &operator()() const & noexcept { return pack_->pk; }

            packer_ptr(packer_ptr &&other) noexcept
                : pack_(other.pack_)
            {
                  other.pack_ = nullptr;
            }

            packer_ptr &operator=(packer_ptr &&other) noexcept
            {
                  this->pack_ = other.pack_;
                  other.pack_ = nullptr;
                  return *this;
            }

            DELETE_COPY_CTORS(packer_ptr);
      };

    /*----------------------------------------------------------------------------------*/

      ND auto get_packer() noexcept 
      {
            for (;;) {
                  for (auto &obj : packs_) {
                        auto *ret = obj.get_packer();
                        if (ret)
                              return packer_ptr{ret};
                  }
                  auto lock = std::unique_lock<std::mutex>{packing_mtx_};
                  packing_cond_.wait_for(lock, 5ms);
            }

      }

    /*----------------------------------------------------------------------------------*/

      ND conn_type &con() & { return con_; }

      virtual value_type read_object ()                    = 0;
      virtual size_t     write_object(packer_type const &) = 0;
      virtual size_t     write_object(buffer_type const &) = 0;

    protected:
      conn_type &con_;

      unpacker_type           unpacker_     = {};
      std::mutex              io_mtx_       = {};
      std::mutex              packing_mtx_  = {};
      std::condition_variable packing_cond_ = {};

      std::array<packer_type, 8> packs_ = {
          packer_type{packing_cond_}, packer_type{packing_cond_},
          packer_type{packing_cond_}, packer_type{packing_cond_},
          packer_type{packing_cond_}, packer_type{packing_cond_},
          packer_type{packing_cond_}, packer_type{packing_cond_}};
};


/****************************************************************************************/

namespace detail {

class msgpack_packer
{
      std::condition_variable &cond_;
      std::mutex               mtx_     = {};
      std::atomic_bool         is_free_ = true;

    public:
      using buffer_type     = msgpack::vrefbuffer;
      using value_type      = msgpack::object_handle;
      using marshaller_type = msgpack::packer<buffer_type>;

      msgpack_packer *get_packer() noexcept 
      {
            mtx_.lock();
            bool expect = true;
            is_free_.compare_exchange_strong(expect, false,
                                             std::memory_order_seq_cst,
                                             std::memory_order_acquire);
            auto *ret = expect ? this : nullptr;
            mtx_.unlock();
            return ret;
      }

      void clear()
      {
            mtx_.lock();
            vbuf.clear();
            is_free_.store(true, std::memory_order_seq_cst);
            cond_.notify_all();
            mtx_.unlock();
      }

      explicit msgpack_packer(std::condition_variable &cond) : cond_(cond) {}
      ~msgpack_packer() = default;
      DELETE_MOVE_CTORS(msgpack_packer);
      DELETE_COPY_CTORS(msgpack_packer);

      buffer_type     vbuf = {};
      marshaller_type pk   = {vbuf};
};

} // namespace detail


template <typename ConnectionType>
class msgpack_wrapper
      : public basic_wrapper<ConnectionType, detail::msgpack_packer, msgpack::unpacker>
{
      static constexpr size_t  read_buffer_size = SIZE_C(8'388'608);

      using base_type = basic_wrapper<ConnectionType,
                                      detail::msgpack_packer, msgpack::unpacker>;
      using this_type = msgpack_wrapper<ConnectionType>;
      using conn_type = ConnectionType;

    public:
      using unpacker_type = msgpack::unpacker;
      using packer_type   = detail::msgpack_packer;
      using value_type    = typename packer_type::value_type;
      using buffer_type   = typename packer_type::buffer_type;

      explicit msgpack_wrapper(conn_type &con) : base_type(con)
      {
            this->unpacker_.m_user_data = this;
            this->unpacker_.reserve_buffer(read_buffer_size);
      }

      ~msgpack_wrapper() override = default;

      DELETE_COPY_CTORS(msgpack_wrapper);
      DELETE_MOVE_CTORS(msgpack_wrapper);

    /*----------------------------------------------------------------------------------*/

      using base_type::io_mtx_;
      using base_type::unpacker_;
      using base_type::con_;

      msgpack::object_handle read_object() override
      {
            io_mtx_.lock();
            auto    obj    = msgpack::object_handle{};
            ssize_t nread;

            while (!unpacker_.next(obj)) {
                  if (unpacker_.buffer_capacity() == 0)
                        unpacker_.reserve_buffer();

                  do {
                        nread = con_.raw_read(unpacker_.buffer(),
                                              unpacker_.buffer_capacity(), 0);
                        if (nread == (-1) && errno != EAGAIN)
                              err(1, "read()");
                  } while (nread <= 0);

                  unpacker_.buffer_consumed(nread);
            }

            if (unpacker_.nonparsed_size() > 0)
                  fmt::print(stderr, FC("Warning: {} bytes still unparsed.\n"),
                             unpacker_.nonparsed_size());

            io_mtx_.unlock();
            return obj;
      }

      size_t write_object(msgpack::vrefbuffer const &vec) override
      {
            io_mtx_.lock();
            size_t total = 0;

            for (size_t i = 0; i < vec.vector_size(); ++i) {
                  auto const &iov = vec.vector()[i];
                  total += con_.raw_write(iov.iov_base, iov.iov_len);
            }

            io_mtx_.unlock();
            return total;
      }

      size_t write_object(packer_type const &pack) override
      {
            return write_object(pack.vbuf);
      }

    /*----------------------------------------------------------------------------------*/
};



/****************************************************************************************/
/****************************************************************************************/


namespace detail {

class jsonrpc_packer
{
      std::condition_variable &cond_;
      std::mutex               mtx_     = {};
      std::atomic_bool         is_free_ = true;

    public:
      using buffer_type     = rapidjson::Document;
      using value_type      = rapidjson::Document;
      using marshaller_type = ipc::json::rapid_doc<>;

      jsonrpc_packer *get_packer() noexcept 
      {
            mtx_.lock();
            bool expect = true;
            is_free_.compare_exchange_strong(expect, false,
                                             std::memory_order_seq_cst,
                                             std::memory_order_acquire);
            auto *ret = expect ? this : nullptr;
            mtx_.unlock();
            return ret;
      }

      void clear()
      {
            mtx_.lock();
            pk.doc().Clear();
            pk.clear();
            is_free_.store(true, std::memory_order_seq_cst);
            cond_.notify_all();
            mtx_.unlock();
      }

      explicit jsonrpc_packer(std::condition_variable &cond) : cond_(cond) {}
      ~jsonrpc_packer() = default;
      DELETE_MOVE_CTORS(jsonrpc_packer);
      DELETE_COPY_CTORS(jsonrpc_packer);

      marshaller_type pk = {};
};

} // namespace detail



template <typename ConnectionType>
class lsp_jsonrpc_packer
      : public basic_wrapper<ConnectionType, detail::jsonrpc_packer, msgpack::unpacker>
{

};


/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/


#if 0
template <typename ConnectionType>
      REQUIRES(ipc::IsBasicConnectionVariant<ConnectionType>)
class msgpack_wrapper
{
      // 1 << 23 aka 8 MiB;
      static constexpr size_t  read_buffer_size = SIZE_C(8'388'608);

    public:
      msgpack_wrapper()
      {
            unpacker_.m_user_data = this;
            unpacker_.reserve_buffer(read_buffer_size);
      }

      ~msgpack_wrapper() = default;

      DELETE_COPY_CTORS(msgpack_wrapper);
      DELETE_MOVE_CTORS(msgpack_wrapper);

    /*----------------------------------------------------------------------------------*/

      msgpack::object_handle read_object(ConnectionType &con)
      {
      }

      size_t write_object(ConnectionType &con, msgpack::vrefbuffer const &vec)
      {
      }

    /*----------------------------------------------------------------------------------*/

      class packer_ptr;

    protected:
      class packer
      {
            using owner_type = msgpack_wrapper;
            friend owner_type;
            friend class packer_ptr;

            std::condition_variable &cond_;
            std::mutex               mtx_     = {};
            std::atomic_bool         is_free_ = true;

            packer *get_packer() noexcept 
            {
                  auto lock = std::unique_lock<std::mutex>{mtx_};
                  lock.lock();
                  bool expect = true;
                  is_free_.compare_exchange_strong(expect, false,
                                                   std::memory_order_seq_cst,
                                                   std::memory_order_acquire);
                  return expect ? this : nullptr;
            }

            void clear()
            {
                  auto lock = std::unique_lock<std::mutex>{mtx_};
                  lock.lock();
                  vbuf.clear();
                  is_free_.store(true, std::memory_order_seq_cst);
                  cond_.notify_all();
            }

          public:
            explicit packer(std::condition_variable &cond) : cond_(cond) {}
            ~packer() = default;
            DELETE_MOVE_CTORS(packer);
            DELETE_COPY_CTORS(packer);

            using buffer_type = msgpack::vrefbuffer;
            using packer_type = msgpack::packer<buffer_type>;

            buffer_type vbuf = {};
            packer_type pk   = {vbuf};
      };

    /*----------------------------------------------------------------------------------*/

    public:
      class packer_ptr
      {
            packer *pack_ = nullptr;

          public:
            packer_ptr() = default;
            explicit packer_ptr(packer *pack) : pack_(pack) {}

            ~packer_ptr()
            {
                  pack_->clear();
            }

            auto *operator->()   noexcept { return pack_; }
            auto &operator* () & noexcept { return *pack_; }
            auto &operator()() & noexcept { return pack_->pk; }

            auto const *operator->() const   noexcept { return pack_; }
            auto const &operator* () const & noexcept { return *pack_; }
            auto const &operator()() const & noexcept { return pack_->pk; }

            packer_ptr(packer_ptr &&other) noexcept
                : pack_(other.pack_)
            {
                  other.pack_ = nullptr;
            }

            packer_ptr &operator=(packer_ptr &&other) noexcept
            {
                  this->pack_ = other.pack_;
                  other.pack_ = nullptr;
                  return *this;
            }

            DELETE_COPY_CTORS(packer_ptr);
      };

    /*----------------------------------------------------------------------------------*/

      ND auto get_packer() noexcept 
      {
            for (;;) {
                  for (auto &obj : packs_) {
                        auto *ret = obj.get_packer();
                        if (ret)
                              return packer_ptr{ret};
                  }
                  auto lock = std::unique_lock<std::mutex>{packing_mtx_};
                  packing_cond_.wait_for(lock, 5ms);
            }

      }

      size_t write_object(ConnectionType &con, packer const &pack)
      {
            return write_object(std::forward<ConnectionType &>(con), pack.vbuf);
      }

    private:
      msgpack::unpacker       unpacker_{};
      std::mutex              io_mtx_;
      std::mutex              packing_mtx_  = {};
      std::condition_variable packing_cond_ = {};

      std::array<packer, 8> packs_ = {packer{packing_cond_}, packer{packing_cond_},
                                      packer{packing_cond_}, packer{packing_cond_},
                                      packer{packing_cond_}, packer{packing_cond_},
                                      packer{packing_cond_}, packer{packing_cond_}};
};


/****************************************************************************************/


// using ConnectionType = ipc::connections::unix_socket;
template <typename ConnectionType>
class msgpack_connection
      : public ConnectionType, public msgpack_wrapper<ConnectionType>
{
      using this_type       = msgpack_connection; // <ConnectionType>;
      using connection_type = ConnectionType;
      using wrapper_type    = msgpack_wrapper<ConnectionType>;
      using packer_type     = typename wrapper_type::packer;

    protected:
      msgpack_connection() = default;

    public:
      ~msgpack_connection() override = default;

      static auto new_unique() { return std::unique_ptr<this_type>(new this_type); }
      static auto new_shared() { return std::shared_ptr<this_type>(new this_type); }

      DELETE_MOVE_CTORS(msgpack_connection);
      DELETE_COPY_CTORS(msgpack_connection);


    /*----------------------------------------------------------------------------------*/

      using typename wrapper_type::packer_ptr;
      using wrapper_type::get_packer;

      auto read_object()                                { return wrapper_type::read_object(*this); }
      auto write_object(msgpack::vrefbuffer const &buf) { return wrapper_type::write_object(*this, buf); }
      auto write_object(packer_type const &pack)        { return wrapper_type::write_object(*this, pack); }

    private:
      // TODO FIX THIS XXX
      // msgpack_wrapper<base_type> wrapper_;
};
#endif


/****************************************************************************************/
} // namespace ipc::rpc
} // namespace emlsp
#endif
