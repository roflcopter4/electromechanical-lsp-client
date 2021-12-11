#include "Common.hh"
#include "ipc/lsp/lsp-client.hh"

inline namespace emlsp {
namespace ipc::lsp {

#if 0
std::map<std::string, std::any> protocolmap = {
      { "foo", "bar" }
};
#endif

class tagged_union
{

    public:
      enum types {
            NIL, INTEGER, STRING,
      } tag;

    private:
      std::variant<std::string, uintmax_t> obj_;

    public:
      tagged_union() = default;

      tagged_union(tagged_union &&) noexcept            = default;
      tagged_union &operator=(tagged_union &&) noexcept = default;

      tagged_union(tagged_union const &)            = delete;
      tagged_union &operator=(tagged_union const &) = delete;

      tagged_union(std::string &arg)  : tag(STRING)  { obj_.emplace<std::string>(std::move(arg)); }
      tagged_union(std::string &&arg) : tag(STRING)  { obj_.emplace<std::string>(std::move(arg)); }
      tagged_union(intmax_t const i)  : tag(INTEGER) { obj_.emplace<uintmax_t>(i); }
      tagged_union(uintmax_t const i) : tag(INTEGER) { obj_.emplace<uintmax_t>(i); }

      template <typename Ty>
      Ty &get()
      {
            return std::get<Ty>(obj_);
      }
};

void test10()
{
      tagged_union u1 = UINTMAX_C(50'000);
      tagged_union u2 = UINTMAX_C(100'000);

      std::cout << 50'000  << '\t' << u1.get<uintmax_t>() << '\n';
      std::cout << 100'000 << '\t' << u2.get<uintmax_t>() << '\n';
}

} // namespace ipc::lsp 
} // namespace emlsp
