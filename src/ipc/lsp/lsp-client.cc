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
      } tag = {NIL};

    private:
      std::variant<std::string, std::u16string, uintmax_t> obj_;

    public:
      tagged_union() { obj_.emplace<uintmax_t>(0); }

#if 0
      tagged_union(tagged_union &&) noexcept            = default;
      tagged_union &operator=(tagged_union &&) noexcept = default;
      tagged_union(tagged_union const &)            = default;
      tagged_union &operator=(tagged_union const &) = default;
#endif

      explicit tagged_union(std::string const &arg)  : tag(STRING)  { obj_.emplace<std::string>(arg); }
      explicit tagged_union(std::string const &&arg) : tag(STRING)  { obj_.emplace<std::string>(std::forward<std::string const &&>(arg)); }
      explicit tagged_union(std::string &arg)        : tag(STRING)  { obj_.emplace<std::string>(std::move(arg)); }
      explicit tagged_union(std::string &&arg)       : tag(STRING)  { obj_.emplace<std::string>(std::forward<std::string &&>(arg)); }
      explicit tagged_union(intmax_t const i)        : tag(INTEGER) { obj_.emplace<uintmax_t>(i); }
      explicit tagged_union(uintmax_t const i)       : tag(INTEGER) { obj_.emplace<uintmax_t>(i); }

      template <typename Ty>
      ND Ty const &get() const { return std::get<Ty>(obj_); }

      template <typename Ty>
      ND Ty &get() { return std::get<Ty>(obj_); }
};

template <typename T>
class contanister {
      T obj_;
    public:
      explicit contanister(T &obj)  : obj_(std::move(obj)) {}
      explicit contanister(T &&obj) : obj_(obj) {}

      ND T const &get() const { return obj_; }
};

void test10()
{
      auto const u1 = tagged_union{UINTMAX_C(50'000)};
      auto const u2 = tagged_union{UINTMAX_C(100'000)};
      auto const u3 = tagged_union{"hello"s};

      contanister const c1{"hi"sv};

      std::cout << 50'000  << '\t' << u1.get<uintmax_t>() << '\n';
      std::cout << 100'000 << '\t' << u2.get<uintmax_t>() << '\n';
      std::cout << c1.get() << '\n';
}

} // namespace ipc::lsp
} // namespace emlsp
