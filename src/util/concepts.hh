#pragma once
#include "Common.hh"
#include "util/util.hh"

inline namespace emlsp {
namespace util::concepts {

template <typename T> concept Const     = std::is_const_v<std::remove_reference_t<T>>;
template <typename T> concept Trivial   = std::is_trivial_v<std::remove_reference_t<T>>;
template <typename T> concept Pointer   = std::is_pointer_v<T>;
template <typename T> concept Array     = std::is_array_v<std::remove_reference_t<T>>;
template <typename T> concept Integral  = std::is_integral_v<std::remove_reference_t<T>>;
template <typename T> concept Function  = std::is_function_v<T>;
template <typename T> concept Reference = std::is_reference_v<T>;

template <typename T> concept UniquePtr =
    std::same_as<std::remove_cvref_t<T>, std::unique_ptr<typename T::pointer>> ||
    std::same_as<std::remove_cvref_t<T>, std::unique_ptr<std::remove_pointer_t<typename T::pointer>>> ||
    std::same_as<std::remove_cvref_t<T>, std::unique_ptr<typename T::element_type>> ||
    std::same_as<std::remove_cvref_t<T>, std::unique_ptr<typename T::element_type[]>>;

template <typename T> concept SharedPtr =
    std::same_as<std::remove_cvref_t<T>, std::shared_ptr<typename T::pointer>> ||
    std::same_as<std::remove_cvref_t<T>, std::shared_ptr<std::remove_pointer_t<typename T::pointer>>> ||
    std::same_as<std::remove_cvref_t<T>, std::shared_ptr<typename T::element_type>> ||
    std::same_as<std::remove_cvref_t<T>, std::shared_ptr<typename T::element_type[]>>;

template <typename T> concept AnyPointer = Pointer<T> || UniquePtr<T> || SharedPtr<T>;

template <typename T> concept StringVariant =
    std::same_as<std::remove_cvref_t<T>, std::string> ||
    std::same_as<std::remove_cvref_t<T>, std::string_view>;

template <typename T> concept NotConst      = !Const<T>;
template <typename T> concept NotReference  = !Reference<T>;
template <typename T> concept NotPointer    = !Pointer<T>;
template <typename T> concept NotArray      = !Array<T>;
template <typename T> concept NotIntegral   = !Integral<T>;
template <typename T> concept NotUniquePtr  = !UniquePtr<T>;
template <typename T> concept NotAnyPointer = !AnyPointer<T>;
template <typename T> concept NonTrivial    = !Trivial<T>;

template <typename T> concept Stringable =
    std::convertible_to<T, std::string> ||
    std::same_as<std::remove_cvref_t<T>, std::string> ||
    std::same_as<std::remove_cvref_t<T>, std::string_view>;

template <typename T> concept StringLiteral = requires {
    requires std::is_reference_v<T>;
    requires std::is_array_v<std::remove_reference_t<T>>;
    requires std::convertible_to<std::remove_reference_t<T>, char const *> ||
      std::convertible_to<T, char const *(&)> ||
      std::convertible_to<T, char const *(&&)>;
};

template <typename T> concept NonStringable    = !Stringable<T>;
template <typename T> concept NonStringLiteral = requires(T x) {
    requires !StringLiteral<T>;
    requires NotConst<T>;
    { *x } -> NotConst<>;
};

} // namespace util::concepts
} // namespace emlsp
