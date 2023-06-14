#pragma once
#ifndef HGUARD__IPC__BASIC_SYNC_THING_HH_
#define HGUARD__IPC__BASIC_SYNC_THING_HH_ //NOLINT
/****************************************************************************************/
#include "Common.hh"

#include "ipc/io/basic_io_wrapper.hh"
#include "ipc/io/ms_jsonrpc_io_wrapper.hh"
#include "ipc/io/msgpack_io_wrapper.hh"

inline namespace MAIN_PACKAGE_NAMESPACE {
namespace ipc {
/****************************************************************************************/


class basic_sync_thing
{
    public:
      basic_sync_thing()          = default;
      virtual ~basic_sync_thing() = default;

      DELETE_ALL_CTORS(basic_sync_thing);

    private:
      class my_cond
      {
            std::mutex              mtx_{};
            std::condition_variable cnd_{};
            std::atomic_uint        cnt_ = 0;
            std::atomic_flag        flag_{};

          public:
            my_cond() = default;

            bool wait()
            {
                  std::unique_lock lock(mtx_);
                  cnd_.wait(lock);
                  return false;
            }

            template <typename Rep, typename Period>
            bool wait(std::chrono::duration<Rep, Period> const &dur)
            {
                  std::unique_lock lock(mtx_);
                  return cnd_.wait_for(lock, dur) == std::cv_status::timeout;
            }

            void notify_one() noexcept { cnd_.notify_one(); }
            void notify_all() noexcept { cnd_.notify_all(); }
      };

      my_cond cond_{};

    public:
      void notify_one() noexcept { cond_.notify_one(); }
      void notify_all() noexcept { cond_.notify_all(); }
      bool wait()                { return cond_.wait(); }

      template <typename Rep, typename Period>
      bool wait(std::chrono::duration<Rep, Period> const &dur)
      {
            return cond_.wait(dur);
      }
};


/****************************************************************************************/
} // namespace ipc
} // namespace MAIN_PACKAGE_NAMESPACE
#endif
