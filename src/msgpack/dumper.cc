#include "Common.hh"
#include "msgpack/dumper.hh"

inline namespace emlsp {
namespace ipc::mpack {

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

void dumper::dump_object(msgpack::object const *val)
{
      switch (val->type) {
      case msgpack::type::NIL:
            put_indent();
            strm_ << "nil"sv;
            put_nl();
            break;
      case msgpack::type::POSITIVE_INTEGER:
            put_indent();
            strm_ << val->via.u64;
            put_nl();
            break;
      case msgpack::type::NEGATIVE_INTEGER:
            put_indent();
            strm_ << val->via.i64;
            put_nl();
            break;
      case msgpack::type::FLOAT:
            put_indent();
            strm_ << val->via.f64;
            put_nl();
            break;
      case msgpack::type::BOOLEAN:
            put_indent();
            strm_ << (val->via.boolean ? "true"sv : "false"sv);
            put_nl();
            break;
      case msgpack::type::STR:
            put_indent();
            strm_ << '"' << std::string_view{val->via.str.ptr, val->via.str.size} << '"';
            put_nl();
            break;
      case msgpack::type::ARRAY:
            put_indent();
            dump_array(&val->via.array);
            put_nl();
            break;
      case msgpack::type::MAP:
            put_indent();
            dump_dict(&val->via.map);
            put_nl();
            break;
      default:
            throw util::except::not_implemented(std::to_string(val->type));
      }
}

void dumper::dump_array(msgpack::object_array const *arr)
{
      if (arr->size > 0) {
            strm_ << "[\n"sv;
            ++depth_;
            for (unsigned i = 0; i < arr->size; ++i)
                  dump_object(arr->ptr + i);
            --depth_;
            put_indent();
            strm_ << ']';
      } else {
            strm_ << "[]"sv;
      }
}

void dumper::dump_dict(msgpack::object_map const *dict)
{
      if (dict->size > 0) {
            strm_ << "{\n";
            ++depth_;
            for (unsigned i = 0; i < dict->size; ++i) {
                  auto const *ptr = dict->ptr + i;
                  put_nl_ = false;
                  dump_object(&ptr->key);
                  strm_ << ":  "sv;
                  put_indent_ = put_nl_ = false;
                  dump_object(&ptr->val);
                  std::cout << ",\n"sv;
            }
            --depth_;
            put_indent();
            strm_ << '}';
      } else {
            strm_ << "{}"sv;
      }
}

} // namespace ipc::mpack
} // namespace emlsp
