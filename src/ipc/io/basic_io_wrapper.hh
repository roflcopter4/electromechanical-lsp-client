#pragma once
#ifndef HGUARD__IPC__BASIC_WRAPPER_HH_
#define HGUARD__IPC__BASIC_WRAPPER_HH_ //NOLINT

#include "Common.hh"
#include "ipc/ipc_connection.hh"

inline namespace emlsp {
namespace ipc::io {
/****************************************************************************************/


template <typename Connection, typename Packer, typename Unpacker>
      REQUIRES (BasicConnectionVariant<Connection>)
class basic_wrapper
{
      using this_type       = basic_wrapper;
      using connection_type = Connection;
      using packer_type     = Packer;
      using unpacker_type   = Unpacker;
      using value_type      = typename packer_type::value_type;

      friend Packer;

    public:
      explicit basic_wrapper(connection_type *con) : con_(con) {}
      virtual ~basic_wrapper() = default;

      basic_wrapper(basic_wrapper const &)                = delete;
      basic_wrapper(basic_wrapper &&) noexcept            = delete;
      basic_wrapper &operator=(basic_wrapper const &)     = delete;
      basic_wrapper &operator=(basic_wrapper &&) noexcept = delete;

      /*----------------------------------------------------------------------------------*/

      class packer_container
      {
            friend Packer;
            Packer *pack_ = nullptr;

          public:
            packer_container() = default;
            explicit packer_container(packer_type *pack) : pack_(pack) {}

            ~packer_container()
            {
                  try {
                        pack_->clear();
                  } catch (...) {}
            }

            auto *operator->()   noexcept { return pack_; }
            auto &operator* () & noexcept { return *pack_; }
            auto &operator()() & noexcept { return pack_->pk; }

            auto const *operator->() const   noexcept { return pack_; }
            auto const &operator* () const & noexcept { return *pack_; }
            auto const &operator()() const & noexcept { return pack_->pk; }

            //packer_container(packer_container &&other) noexcept
            //    : pack_(other.pack_)
            //{
            //      other.pack_ = nullptr;
            //}

            //packer_container &operator=(packer_container &&other) noexcept
            //{
            //      this->pack_ = other.pack_;
            //      other.pack_ = nullptr;
            //      return *this;
            //}

            packer_container(packer_container const &)                     = delete;
            packer_container(packer_container &&other) noexcept            = delete;
            packer_container &operator=(packer_container const &)          = delete;
            packer_container &operator=(packer_container &&other) noexcept = delete;
      };

    /*----------------------------------------------------------------------------------*/

      virtual Packer *get_packer_if_available(Packer &pack) = 0;

      ND packer_container get_new_packer()
      {
            for (;;) {
                  for (auto &obj : packs_)
                        if (auto *ret = get_packer_if_available(obj))
                              return packer_container{ret};

                  std::unique_lock lock{packing_mtx_};
                  packing_cond_.wait_for(lock, 50ms);
            }
      }

      ND connection_type *con() { return con_; }

    /*----------------------------------------------------------------------------------*/

      virtual value_type read_object ()              = 0;
      virtual size_t     write_object(packer_type &) = 0;
      virtual size_t     write_object(value_type &)  = 0;

      virtual size_t write_object(packer_type *packer)
      {
            return write_object(*packer);
      }

      virtual size_t write_object(packer_container &ptr)
      {
            return write_object(*ptr);
      }

    /*----------------------------------------------------------------------------------*/

    protected:
      connection_type *con_;

      unpacker_type           unpacker_     = {};
      std::recursive_mutex    io_mtx_       = {};
      std::recursive_mutex    write_mtx_    = {};
      std::recursive_mutex    read_mtx_     = {};
      std::mutex              packing_mtx_  = {};
      std::condition_variable packing_cond_ = {};

#define PACK packer_type{packing_cond_}
      std::array<packer_type, 4> packs_ = std::array<packer_type, 4>{PACK, PACK, PACK, PACK};
#if 0
      std::array<packer_type, 64> packs_ = std::array<packer_type, 64>{
          PACK, PACK, PACK, PACK, PACK, PACK, PACK, PACK,
          PACK, PACK, PACK, PACK, PACK, PACK, PACK, PACK,
          PACK, PACK, PACK, PACK, PACK, PACK, PACK, PACK,
          PACK, PACK, PACK, PACK, PACK, PACK, PACK, PACK,
          PACK, PACK, PACK, PACK, PACK, PACK, PACK, PACK,
          PACK, PACK, PACK, PACK, PACK, PACK, PACK, PACK,
          PACK, PACK, PACK, PACK, PACK, PACK, PACK, PACK,
          PACK, PACK, PACK, PACK, PACK, PACK, PACK, PACK,
      };
#endif
#undef PACK
};


template <typename T>
concept WrapperVariant = std::derived_from<T,
                                           basic_wrapper<typename T::connection_type,
                                                         typename T::packer_type,
                                                         typename T::unpacker_type>>;


/****************************************************************************************/
} // namespace ipc::io
} // namespace emlsp
#endif
