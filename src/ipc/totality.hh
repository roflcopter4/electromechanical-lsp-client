#pragma once
#ifndef HGUARD__IPC__TOTALITY_HH_
#define HGUARD__IPC__TOTALITY_HH_ //NOLINT
/****************************************************************************************/
#include "Common.hh"

#include "ipc/basic_connection.hh"
#include "ipc/basic_wrapper.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


namespace event_tags {

struct base_event_tag {};
struct libevent : base_event_tag {};
struct libuv : base_event_tag {};
struct asio : base_event_tag {};

template <typename T>
concept IsEventTag = requires
{
      requires std::derived_from<T, base_event_tag>;
      requires !std::same_as<T, base_event_tag>;
};

} // namespace event_tags


/****************************************************************************************/
/****************************************************************************************/

template <typename ConnectionType,
          template <typename>
          typename WrapperType,
          typename EventTag>
    REQUIRES (
          requires IsBasicConnectionVariant<ConnectionType>;
          requires rpc::IsIOWrapperVariant<WrapperType<ConnectionType>>;
          requires event_tags::IsEventTag<EventTag>;
    )
class base_totality_iface : public ConnectionType
{
    public:
      base_totality_iface() : wrapper_(this) {}

      ~base_totality_iface() override = default;

      DELETE_COPY_CTORS(base_totality_iface);
      DEFAULT_MOVE_CTORS(base_totality_iface);

      using wrapper_type = WrapperType<ConnectionType>;
      wrapper_type &wrapper() & { return wrapper_; }

    protected:
      WrapperType<ConnectionType> wrapper_;
};


/*--------------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------------*/


template <typename ConnectionType,
          template <typename>
          typename WrapperType,
          typename EventTag>
class totality;


/*--------------------------------------------------------------------------------------*/


template <typename ConnectionType, template <typename> typename WrapperType>
class totality<ConnectionType, WrapperType, event_tags::libevent>
    : public base_totality_iface<ConnectionType, WrapperType, event_tags::libevent>
{
      using base_type       = base_totality_iface<ConnectionType, WrapperType, event_tags::libevent>;
      using this_type       = totality<ConnectionType, WrapperType, event_tags::libevent>;
      using connection_type = ConnectionType;
      using wrapper_type    = WrapperType<ConnectionType>;
      using value_type      = typename WrapperType<ConnectionType>::value_type;

    public:
      totality()
      {
            //event_config *cfg = ::event_config_new();
            //::event_config_require_features(cfg, EV_FEATURE_EARLY_CLOSE);
            //::event_config_set_flag(cfg, EVENT_BASE_FLAG_STARTUP_IOCP );
            //base_ = ::event_base_new_with_config(cfg);
            //::event_config_free(cfg);
            base_ = event_base_new();
            if (!base_)
                  throw std::runtime_error("BAD BAD BAD");
      }

      ~totality() override
      {
            try {
                  stop();
            } catch (...) {}
            if (base_)
                  event_base_free(base_);
      }

      DELETE_COPY_CTORS(totality);
      DELETE_MOVE_CTORS(totality);

      static auto new_unique() { return std::unique_ptr<this_type>{new this_type}; }
      static auto new_shared() { return std::shared_ptr<this_type>{new this_type}; }

    //----------------------------------------------------------------------------------

    private:
      using base_type::raw_descriptor;

      event_base      *base_       = nullptr;
      std::atomic_flag start_flag_ = {};
      std::thread      thrd_;
      std::mutex       init_cond_mtx_{};
      std::condition_variable init_cond_{};

      NOINLINE
      void main_loop()
      {
            for (;;) {
                  init_cond_.notify_all();
                  auto const ret = ::event_base_loop(base_, 0);

                  if (::event_base_got_break(base_)) [[unlikely]]
                        break;
                  if (ret == -1) [[unlikely]]
                        errx(1, "error in libevent...");
                  if (ret == 1) [[unlikely]] {
                        fmt::print(stderr, "\033[1;31mERROR: No registered events!\033[0m\n");
                        break;
                  }
            }
      }

      static void startup_wrapper_cleanup(void **arg)
      {
            if (!arg || !*arg)
                  return;
            auto *handle_p = reinterpret_cast<event **>(arg);
            ::event_del(*handle_p);
            ::event_free(*handle_p);
            *handle_p = nullptr;
      }

      static void
      startup_wrapper(this_type *self, event_callback_fn cb)
      {
            static constexpr timeval tv = {0U, 500'000U};

            event *rd_handle = ::event_new(self->base_, self->raw_descriptor(),
                                           EV_READ | EV_PERSIST, cb, self);

            //pthread_cleanup_push(reinterpret_cast<void(*)(void *)>(&startup_wrapper_cleanup), &rd_handle);

            try {
                  ::event_add(rd_handle, &tv);
                  self->main_loop();
            } catch (std::exception &e) {
                  startup_wrapper_cleanup(reinterpret_cast<void **>(&rd_handle));
                  throw;
            }

            startup_wrapper_cleanup(reinterpret_cast<void **>(&rd_handle));

            //pthread_cleanup_pop(true);
      }

    public:
      void start()
      {
            if (start_flag_.test_and_set()) [[unlikely]]
                  return;

            thrd_ = std::thread { startup_wrapper, this, reinterpret_cast<event_callback_fn>(&event_callback_wrapper) };
            std::unique_lock lock(init_cond_mtx_);
            init_cond_.wait(lock);
      }

      void loopbreak()
      {
            static std::mutex mtx;
            std::lock_guard   lock(mtx);
            if (!start_flag_.test()) [[unlikely]]
                  throw std::logic_error("Attempt to break uninitialized event loop.");
            ::event_base_loopbreak(base_);
            thrd_.join();
            start_flag_.clear();
      }

      void stop() { loopbreak(); }

    /*----------------------------------------------------------------------------------*/

      std::deque<std::atomic<value_type *>> queue_;
      std::atomic<std::deque<std::atomic<value_type *>> *> r{&queue_};

      void read_callback()
      {
            auto obj = this->wrapper().read_object();
      }

      void event_callback(UNUSED socket_t const fd, uint16_t const flags)
      {
            if (flags & EV_CLOSED)
                  loopbreak();
            if (flags & EV_READ)
                  read_callback();
            if (flags & EV_TIMEOUT)
                  (void)0;
      }

      static void event_callback_wrapper(socket_t fd, int16_t flags, void *data)
      {
            auto *self = static_cast<this_type *>(data);
            //self->read_callback(fd, flags);
      }
};


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif
