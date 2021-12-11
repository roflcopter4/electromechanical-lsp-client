#include "msgpack/dumper.hh"

inline namespace emlsp {
namespace ipc::mpack {

void dumper::put_nl()
{
      if (put_nl_)
            strm_ << ",\n";
      put_nl_ = true;
}

void dumper::put_indent()
{
      if (put_indent_)
            for (unsigned i = 0; i < depth_ * width; ++i)
                  strm_ << ' ';
      put_indent_ = true;
}

void dumper::dump_object(msgpack::object const *val)
{
      switch (val->type) {
      case msgpack::type::NIL:
            put_indent();
            strm_ << "nil";
            put_nl();
            break;
      case msgpack::type::POSITIVE_INTEGER:
            put_indent();
            strm_ << std::to_string(val->via.u64);
            put_nl();
            break;
      case msgpack::type::NEGATIVE_INTEGER:
            put_indent();
            strm_ << std::to_string(val->via.i64);
            put_nl();
            break;
      case msgpack::type::FLOAT:
            put_indent();
            strm_ << std::to_string(val->via.f64);
            put_nl();
            break;
      case msgpack::type::BOOLEAN:
            put_indent();
            strm_ << (val->via.boolean ? "true" : "false");
            put_nl();
            break;
      case msgpack::type::STR:
            put_indent();
            strm_ << '"' << std::string{val->via.str.ptr, val->via.str.size} << '"';
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
            throw emlsp::except::not_implemented(std::to_string(val->type));
      }
}

void dumper::dump_array(msgpack::object_array const *arr)
{
      if (arr->size > 0) {
            strm_ << "[\n";
            ++depth_;
            for (unsigned i = 0; i < arr->size; ++i)
                  dump_object(arr->ptr + i);
            --depth_;
            put_indent();
            strm_ << "]";
      } else {
            strm_ << "[]";
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
                  strm_ << ":  ";
                  put_indent_ = put_nl_ = false;
                  dump_object(&ptr->val);
                  std::cout << ",\n";
            }
            --depth_;
            put_indent();
            strm_ << '}';
      } else {
            strm_ << "{}";
      }
}

} // namespace ipc::mpack
} // namespace emlsp
