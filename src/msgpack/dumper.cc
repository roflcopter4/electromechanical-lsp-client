#include "Common.hh"
#include "msgpack/dumper.hh"

inline namespace emlsp {
namespace util::mpack {

using namespace std::string_view_literals;

void dumper::put_nl()
{
      if (put_nl_)
            strm_ << ",\n"sv;
      put_nl_ = true;
}

void dumper::put_indent()
{
      if (put_indent_) {
            auto const nchars = depth_ * width;
            strm_ << std::string(nchars, ' ');
      }
      put_indent_ = true;
}

void dumper::dump(msgpack::object const *val)
{
      put_indent();

      switch (val->type) {
      case msgpack::type::NIL:
            strm_ << "nil"sv;
            break;
      case msgpack::type::POSITIVE_INTEGER:
            strm_ << val->via.u64;
            break;
      case msgpack::type::NEGATIVE_INTEGER:
            strm_ << val->via.i64;
            break;
      case msgpack::type::FLOAT64:
      case msgpack::type::FLOAT32:
            strm_ << val->via.f64;
            break;
      case msgpack::type::BOOLEAN:
            strm_ << (val->via.boolean ? "true"sv : "false"sv);
            break;
      case msgpack::type::STR:
            strm_ << '"' << std::string_view{val->via.str.ptr, val->via.str.size} << '"';
            break;
      case msgpack::type::ARRAY:
            dump(&val->via.array);
            break;
      case msgpack::type::MAP:
            dump(&val->via.map);
            break;
      case msgpack::type::BIN:
      case msgpack::type::EXT:
      default:
            throw util::except::not_implemented(std::to_string(val->type));
      }

      put_nl();
}

void dumper::dump(msgpack::object_array const *arr)
{
      if (arr->size > 0) {
            strm_ << "[\n"sv;
            ++depth_;
            for (unsigned i = 0; i < arr->size; ++i)
                  dump(arr->ptr + i);
            --depth_;
            put_indent();
            strm_ << ']';
      } else {
            strm_ << "[]"sv;
      }
}

void dumper::dump(msgpack::object_map const *dict)
{
      if (dict->size > 0) {
            strm_ << "{\n";
            ++depth_;
            for (unsigned i = 0; i < dict->size; ++i) {
                  auto const *ptr = dict->ptr + i;
                  put_nl_ = false;
                  dump(&ptr->key);
                  strm_ << ":  "sv;
                  put_indent_ = false;
                  dump(&ptr->val);
            }
            --depth_;
            put_indent();
            strm_ << '}';
      } else {
            strm_ << "{}"sv;
      }
}

} // namespace util::mpack
} // namespace emlsp
