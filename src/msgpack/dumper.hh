#pragma once

#include "Common.hh"

#include <msgpack.hpp>

inline namespace MAIN_PACKAGE_NAMESPACE {
namespace util::mpack {

namespace concepts {
template <typename T>
concept ValidMsgpack =
    std::same_as<std::remove_cvref_t<T>, msgpack::object> ||
    std::same_as<std::remove_cvref_t<T>, msgpack::object_map> ||
    std::same_as<std::remove_cvref_t<T>, msgpack::object_array>;
}

class dumper
{
      static constexpr unsigned width = 4;

      std::ostream &strm_;
      unsigned      depth_      = 0;
      bool          put_nl_     = true;
      bool          put_indent_ = true;

      void put_nl();
      void put_indent();

      //void dump_object(msgpack::object const *val);
      //void dump_array(msgpack::object_array const *arr);
      //void dump_dict(msgpack::object_map const *dict);

      void dump(msgpack::object const *val);
      void dump(msgpack::object_array const *arr);
      void dump(msgpack::object_map const *dict);

    public:
      template <typename Object> requires concepts::ValidMsgpack<Object>
      explicit dumper(Object const *value, std::ostream &out = std::cout)
            : strm_(out)
      {
            dump(value);
      }

      template <typename Object> requires concepts::ValidMsgpack<Object>
      explicit dumper(Object const &value, std::ostream &out = std::cout)
            : strm_(out)
      {
            dump(&value);
      }
};

} // namespace util::mpack
} // namespace MAIN_PACKAGE_NAMESPACE
