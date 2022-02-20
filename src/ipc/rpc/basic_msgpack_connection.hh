#pragma once
#ifndef HGUARD__IPC__RPC__BASIC_MSGPACK_CONNECTION_HH_
#define HGUARD__IPC__RPC__BASIC_MSGPACK_CONNECTION_HH_ //NOLINT

#include "Common.hh"
#include "ipc/base_connection.hh"

inline namespace emlsp {
namespace ipc::rpc {
/****************************************************************************************/

// template <typename ConnectionType>
//       REQUIRES(ipc::IsBasicConnectionVariant<ConnectionType>)
// class msgpack_wrapper;

// template <typename ConnectionType>
//class msgpack_connection;


/****************************************************************************************/


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

      template <typename ConnectionType>
            REQUIRES(ipc::IsBasicConnectionVariant<ConnectionType>)
      msgpack::object_handle read_object(ConnectionType &con)
      {
            io_mtx_.lock();

            // auto    shared = std::shared_ptr<conn_type>(con_);
            auto    obj    = msgpack::object_handle{};
            ssize_t nread;

            while (!unpacker_.next(obj)) {
                  if (unpacker_.buffer_capacity() == 0)
                        unpacker_.reserve_buffer();

                  do {
                        nread = con.raw_read(unpacker_.buffer(),
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

      template <typename ConnectionType>
            REQUIRES(ipc::IsBasicConnectionVariant<ConnectionType>)
      size_t write_object(ConnectionType &con, msgpack::vrefbuffer const &vec)
      {
            io_mtx_.lock();

            // auto   shared = std::shared_ptr<conn_type>{con_};
            size_t total  = 0;

            for (size_t i = 0; i < vec.vector_size(); ++i) {
                  auto const &iov = vec.vector()[i];
                  total += con.raw_write(iov.iov_base, iov.iov_len);
            }

            io_mtx_.unlock();
            return total;
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
                  mtx_.lock();
                  bool expect = true;
                  is_free_.compare_exchange_strong(expect, false,
                                                   std::memory_order_seq_cst,
                                                   std::memory_order_acquire);
                  auto *const ret = expect ? this : nullptr;
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

      template <typename ConnectionType>
            REQUIRES(ipc::IsBasicConnectionVariant<ConnectionType>)
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
class msgpack_connection : public ConnectionType, protected msgpack_wrapper
{
      using this_type       = msgpack_connection; // <ConnectionType>;
      using base_type       = ConnectionType;
      using connection_type = ConnectionType;

    protected:
      msgpack_connection() = default;

    public:
      ~msgpack_connection() override = default;

      static auto new_unique() { return std::unique_ptr<this_type>(new this_type); }
      static auto new_shared() { return std::shared_ptr<this_type>(new this_type); }

      DELETE_MOVE_CTORS(msgpack_connection);
      DELETE_COPY_CTORS(msgpack_connection);


    /*----------------------------------------------------------------------------------*/

      using msgpack_wrapper::get_packer;
      using msgpack_wrapper::packer_ptr;
      auto read_object()                                     { return msgpack_wrapper::read_object(*this); }
      auto write_object(msgpack::vrefbuffer const &buf)      { return msgpack_wrapper::write_object(*this, buf); }
      auto write_object(msgpack_wrapper::packer const &pack) { return msgpack_wrapper::write_object(*this, pack); }

    private:
      // TODO FIX THIS XXX
      // msgpack_wrapper<base_type> wrapper_;
};


/****************************************************************************************/
} // namespace ipc::rpc
} // namespace emlsp
#endif // basic_msgpack_connection.hh
