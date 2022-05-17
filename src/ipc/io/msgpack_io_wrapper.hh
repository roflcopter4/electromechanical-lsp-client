// ReSharper disable CppClassCanBeFinal
#pragma once
#ifndef HGUARD__IPC__IO__MSGPACK_IO_WRAPPER_HH_
#define HGUARD__IPC__IO__MSGPACK_IO_WRAPPER_HH_ //NOLINT

#include "Common.hh"
#include "ipc/io/basic_io_wrapper.hh"

#define AUTOC auto const

inline namespace emlsp {
namespace ipc::io {
/****************************************************************************************/


namespace detail {

class msgpack_packer
{
      std::condition_variable &cond_;
      std::mutex               mtx_     = {};
      std::atomic_bool         is_free_ = true;

    public:
      using value_type      = msgpack::object_handle;
      using buffer_type     = msgpack::vrefbuffer;
      using marshaller_type = msgpack::packer<buffer_type>;

      msgpack_packer *get_new_packer()
      {
            std::lock_guard lock(mtx_);
            bool expect = true;
            is_free_.compare_exchange_strong(expect, false,
                                             std::memory_order::seq_cst,
                                             std::memory_order::acquire);
            auto *ret = expect ? this : nullptr;
            return ret;
      }

      void clear()
      {
            std::lock_guard lock(mtx_);
            vbuf.clear();
            is_free_.store(true, std::memory_order::seq_cst);
            cond_.notify_all();
      }

      explicit msgpack_packer(std::condition_variable &cond) : cond_(cond) {}
      ~msgpack_packer() = default;

      msgpack_packer(msgpack_packer const &)                = delete;
      msgpack_packer(msgpack_packer &&) noexcept            = delete;
      msgpack_packer &operator=(msgpack_packer const &)     = delete;
      msgpack_packer &operator=(msgpack_packer &&) noexcept = delete;

      buffer_type     vbuf = {};
      marshaller_type pk   = {vbuf};
};

} // namespace detail


template <typename Connection>
      REQUIRES (BasicConnectionVariant<Connection>)
class msgpack_wrapper
      : public basic_wrapper<Connection, detail::msgpack_packer, msgpack::unpacker>
{
      static constexpr size_t  read_buffer_size = SIZE_C(8'388'608);

      using this_type = msgpack_wrapper<Connection>;
      using base_type = basic_wrapper<Connection,
                                         detail::msgpack_packer, msgpack::unpacker>;

    public:
      using connection_type = Connection;
      using packer_type     = detail::msgpack_packer;
      using unpacker_type   = msgpack::unpacker;

      using value_type  = msgpack::object_handle;
      using buffer_type = msgpack::vrefbuffer;

    /*----------------------------------------------------------------------------------*/

      explicit msgpack_wrapper(connection_type *con) : base_type(con)
      {
            this->unpacker_.m_user_data = this;
            this->unpacker_.reserve_buffer(read_buffer_size);
      }

      ~msgpack_wrapper() override = default;

      msgpack_wrapper(msgpack_wrapper const &)                = delete;
      msgpack_wrapper(msgpack_wrapper &&) noexcept            = default;
      msgpack_wrapper &operator=(msgpack_wrapper const &)     = delete;
      msgpack_wrapper &operator=(msgpack_wrapper &&) noexcept = default;

    /*----------------------------------------------------------------------------------*/

    private:
      using base_type::unpacker_;
      using base_type::con_;

    public:
      using base_type::write_object;

      size_t just_read()
      {
            size_t total = 0;
            while (con_->available() > 0) {
                  if (unpacker_.buffer_capacity() == 0)
                        unpacker_.reserve_buffer();
                  size_t const nread = con_->raw_read(unpacker_.buffer(), unpacker_.buffer_capacity(), 0);
                  unpacker_.buffer_consumed(nread);
                  total += nread;
            }
            return total;
      }

      __attribute_error__("Don't use this function. Use `just_read()'.")
      msgpack::object_handle read_object() override
      {
            assert(0 && "I told you not to use this function.");
            std::lock_guard lock{this->read_mtx_};

            auto  obj = msgpack::object_handle{};
            AUTOC nread = con_->raw_read(unpacker_.buffer(), unpacker_.buffer_capacity(), 0);
            unpacker_.buffer_consumed(nread);

#if 0
            while (!unpacker_.next(obj)) {
                  if (unpacker_.buffer_capacity() == 0)
                        unpacker_.reserve_buffer();

                  do {
                        // errno = 0;
                        nread = con_->raw_read(unpacker_.buffer(),
                                               unpacker_.buffer_capacity(), 0);
                        if (nread == (-1) || (errno && errno != EAGAIN)) {
                              perror("uhhhhh");
                              fflush(stdout);
                              fflush(stderr);
                              err_nothrow("read()");
                        }
                  } while (nread <= 0);

                  unpacker_.buffer_consumed(nread);
            }

            if (unpacker_.nonparsed_size() > 0)
                  fmt::print(stderr, FC("Warning: {} bytes still unparsed.\n"),
                             unpacker_.nonparsed_size());
#endif

            return obj;
      }

      size_t write_object(msgpack::vrefbuffer const &vec)
      {
            std::lock_guard lock(this->write_mtx_);

#ifdef _WIN32
            auto winvec = std::make_unique<WSABUF[]>(vec.vector_size());
            for (unsigned i = 0; i < vec.vector_size(); ++i)
                  winvec[i] = WSABUF{static_cast<DWORD>(vec.vector()[i].iov_len),
                                     static_cast<char *>(vec.vector()[i].iov_base)};
            return con_->impl().writev(winvec.get(), vec.vector_size());
#else
            return con_->impl().writev(vec.vector(), vec.vector_size());
#endif
      }

      size_t write_object(packer_type &pack) override
      {
            return write_object(pack.vbuf);
      }

      size_t write_object(msgpack::object_handle &obj) override
      {
            msgpack::vrefbuffer rbuf;
            msgpack::pack(rbuf, obj.get());
            return write_object(rbuf);
      }

      auto &get_unpacker() & noexcept { return unpacker_; }
};


/****************************************************************************************/
} // namespace ipc::io
} // namespace emlsp
#endif
