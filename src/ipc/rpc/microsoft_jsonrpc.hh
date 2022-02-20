#pragma once
#ifndef HGUARD__IPC__RPC__MICROSOFT_JSONRPC_HH_
#define HGUARD__IPC__RPC__MICROSOFT_JSONRPC_HH_ //NOLINT

#include "Common.hh"
#include "ipc/base_connection.hh"

inline namespace emlsp {
namespace ipc::rpc {
/****************************************************************************************/


class stupid_loop
{
      using this_type = stupid_loop;

    public:
      using conn_type = ipc::connections::Default;

    private:
      event_base *base;

      std::atomic_flag start_flag_ = {};
      std::atomic_bool started_    = false;
      std::thread thrd_;

      std::unique_ptr<conn_type> con_ = conn_type::new_unique();

    public:
      explicit stupid_loop()
      {
            event_config *cfg = ::event_config_new();
            // ::event_config_require_features(cfg, EV_FEATURE_EARLY_CLOSE);
            base = ::event_base_new_with_config(cfg);
            ::event_config_free(cfg);
      }

      ~stupid_loop()
      {
            ::event_base_free(base);
            this->base     = nullptr;
            this->started_ = false;
      }

      void start(event_callback_fn const cb)
      {
            this->thrd_    = std::thread{startup_wrapper, this};
            this->started_ = true;
      }

      void loopbreak()
      {
            ::event_base_loopbreak(base);
            this->thrd_.join();
      }

    private:
      static void
      startup_wrapper(this_type *client) noexcept
      {
            try {
                  client->main_loop(nullptr);
            } catch (std::exception &e) {
                  DUMP_EXCEPTION(e);
                  ::exit(1);
            }
      }

      void main_loop(event_callback_fn const cb)
      {
            static constexpr timeval    tv   = {.tv_sec = 0, .tv_usec = 100'000} /* 1/10 seconds */;
            static std::atomic_uint64_t iter = UINT64_C(0);

            if (start_flag_.test_and_set())
                  return;

            event *rd_handle = ::event_new(base, con_->raw_descriptor(),
                                           EV_READ | EV_PERSIST, cb, this);
            ::event_add(rd_handle, &tv);

            for (;;) {
                  uint64_t const n = iter.fetch_add(1);
                  fflush(stderr);
                  fmt::print(stderr, FC("\033[1;35mHERE IN {}, iter number {}\033[0m\n"),
                             FUNCTION_NAME, n);
                  fflush(stderr);

                  if (::event_base_loop(base, 0) == 1) [[unlikely]] {
                        fmt::print(stderr, "\033[1;31mERROR: No registered events!\033[0m\n");
                        break;
                  }
                  if (::event_base_got_break(base)) [[unlikely]]
                        break;
            }

            ::event_free(rd_handle);
      }

      ND conn_type       &con()       & { return *con_; }
      ND conn_type const &con() const & { return *con_; }

      DELETE_COPY_CTORS(stupid_loop);
      DELETE_MOVE_CTORS(stupid_loop);
};



/****************************************************************************************/
} // namespace ipc::rpc
} // namespace emlsp
#endif
