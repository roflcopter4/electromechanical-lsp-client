#pragma once
#ifndef HGUARD__IPC__MAIN_LOOP_HH_
#define HGUARD__IPC__MAIN_LOOP_HH_ //NOLINT

#include "Common.hh"
#include "ipc/forward.hh"

#include "ipc/base_connection.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


class base_loop_interface
{
      using this_type = base_loop_interface;

    public:
      base_loop_interface()  = default;
      ~base_loop_interface() = default;

      DELETE_COPY_CTORS(base_loop_interface);
      DELETE_MOVE_CTORS(base_loop_interface);

    private:
      class wrapper
      {
      };

      std::vector<std::shared_ptr<std::atomic<wrapper *>>> v_ {};
};


class dumb_loop_impl
{

};



namespace framework {

class actor;

/* Used to identify the sender and recipient of messages. */


/* Base class for all registered message handlers. */
class message_handler_base
{
    public:
      message_handler_base()          = default;
      virtual ~message_handler_base() = default;

      /* Used to determine which message handlers receive an incoming message. */
      ND virtual const std::type_info &message_id() const = 0;

      DEFAULT_COPY_CTORS(message_handler_base);
      DEFAULT_MOVE_CTORS(message_handler_base);
};


/* Base class for a handler for a specific message type. */
template <typename Message>
class message_handler : public message_handler_base
{
    public:
      /* NOTE: This commment was in the original code I "borrowed" this from and I'll
       *       leave it here to properly shame it. It's... painful...
       * Handle an incoming message. */
      virtual void handle_message(Message msg, actor *from) = 0;

      message_handler()           = default;
      ~message_handler() override = default;

      DEFAULT_COPY_CTORS(message_handler);
      DEFAULT_MOVE_CTORS(message_handler);
};


/* Concrete message handler for a specific message type. */
template <typename Actor, typename Message>
class method_ptr_message_handler : public message_handler<Message>
{
    public:
      /* Construct a message handler to invoke the specified member function. */
      method_ptr_message_handler(void (Actor::*methodp)(Message, actor *), Actor *a)
          : function_(methodp),
            actor_(a)
      {}

      /* Used to determine which message handlers receive an incoming message. */
      ND std::type_info const &message_id() const override
      {
            return typeid(Message);
      }

      /* Handle an incoming message. */
      void handle_message(Message msg, actor *from) override
      {
            (actor_->*function_)(std::move(msg), from);
      }

      /* Determine whether the message handler represents the specified function. */
      ND bool is_function(void (Actor::*methodp)(Message, actor *)) const
      {
            return methodp == function_;
      }

    private:
      void (Actor::*function_)(Message, actor *);
      Actor *actor_;
};


// Base class for all actors.
class actor
{
      /* All messages associated with a single actor object should be processed
       * non-concurrently. We use a strand to ensure non-concurrent execution even
       * if the underlying executor may use multiple threads. */
      asio::strand<asio::any_io_executor>                executor_;
      std::vector<std::shared_ptr<message_handler_base>> handlers_;

    public:
      virtual ~actor() = default;

      /* Obtain the actor's address for use as a message sender or recipient. */
      actor *address()
      {
            return this;
      }

      /* Send a message from one actor to another. */
      template <typename Message>
      void send(Message msg, actor *from)
      {
            // Execute the message handler in the context of the target's executor.
            asio::post(executor_,
                       [to = this, from, msg = std::move(msg)] () mutable -> void
                       {
                             to->call_handler(std::move(msg), from);
                       }
            );
      }

    protected:
      /* Construct the actor to use the specified executor for all message handlers. */
      explicit actor(asio::any_io_executor const &e) : executor_(e)
      {}

      /* Register a handler for a specific message type. Duplicates are permitted. */
      template <typename Actor, typename Message>
      void register_handler(void (Actor::*methodp)(Message, actor *))
      {
            using mh_type = method_ptr_message_handler<Actor, Message>;
            handlers_.emplace_back(
                std::make_shared<mh_type>(methodp, dynamic_cast<Actor *>(this)));
      }

      /* Deregister a handler. Removes only the first matching handler. */
      template <typename Actor, typename Message>
      void deregister_handler(void (Actor::*methodp)(Message, actor *))
      {
            const std::type_info &id = typeid(Message);

            for (auto iter = handlers_.begin(); iter != handlers_.end(); ++iter)
            {
                  using mh_type = method_ptr_message_handler<Actor, Message>;

                  if ((*iter)->message_id() == id) {
                        auto mh = dynamic_cast<mh_type *>((*iter).get());
                        if (mh->is_function(methodp)) {
                              handlers_.erase(iter);
                              return;
                        }
                  }
            }
      }

      // Send a message from within a message handler.
      template <typename Message>
      void tail_send(Message msg, actor *to)
      {
            // Execute the message handler in the context of the target's executor.
            asio::defer(to->executor_,
                        [to, from = this, msg = std::move(msg)]
                        {
                              to->call_handler(std::move(msg), from);
                        }
            );
      }

    private:
      /* Find the matching message handlers, if any, and call them. */
      template <typename Message>
      void call_handler(Message msg, actor *from)
      {
            const std::type_info &message_id = typeid(Message);

            for (auto &hand : handlers_) {
                  if (hand->message_id() == message_id) {
                        auto *hand_ptr = dynamic_cast<message_handler<Message> *>(hand.get());
                        hand_ptr->handle_message(msg, from);
                  }
            }
      }

    public:
      DEFAULT_COPY_CTORS(actor);
      DEFAULT_MOVE_CTORS(actor);
};


//------------------------------------------------------------------------------


/* A concrete actor that allows synchronous message retrieval. */
template <typename Message>
class receiver : public actor
{
      std::mutex              mutex_;
      std::condition_variable condition_;
      std::deque<Message>     message_queue_;

    public:
      receiver() : actor(asio::system_executor())
      {
            register_handler(&receiver::message_handler);
      }

      /* Block until a message has been received. */
      Message wait()
      {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this] { return !message_queue_.empty(); });

            Message msg(std::move(message_queue_.front()));
            message_queue_.pop_front();
            return msg;
      }

    private:
      /* Handle a new message by adding it to the queue and waking a waiter. */
      void message_handler(Message msg, UNUSED actor *from)
      {
            std::lock_guard<std::mutex> lock(mutex_);
            message_queue_.emplace_back(std::move(msg));
            condition_.notify_one();
      }
};


class member : public actor
{
    public:
      explicit member(asio::any_io_executor const &e) : actor(e)
      {
            register_handler(&member::init_handler);
      }

    private:
      void init_handler(actor *next, actor *from)
      {
            next_   = next;
            caller_ = from;

            register_handler(&member::token_handler);
            deregister_handler(&member::init_handler);
      }

      void token_handler(int token, UNUSED actor *from)
      {
            int    msg(token);
            actor *to(caller_);

            if (token > 0) {
                  msg = token - 1;
                  to  = next_;
            }

            tail_send(msg, to);
      }

      actor *next_{};
      actor *caller_{};
};



} // namespace framework


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif // main_loop.hh
