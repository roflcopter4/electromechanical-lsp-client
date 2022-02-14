#pragma once
#include "Common.hh"
#include "util/util.hh"

inline namespace emlsp {
namespace util::concepts {

template <typename T> concept IsConst    = std::is_const_v<std::remove_reference_t<T>>;
template <typename T> concept IsTrivial  = std::is_trivial_v<std::remove_reference_t<T>>;
template <typename T> concept IsPointer  = std::is_pointer_v<std::remove_reference_t<T>>;
template <typename T> concept IsArray    = std::is_array_v<std::remove_reference_t<T>>;
template <typename T> concept IsIntegral = std::is_integral_v<T>;
template <typename T> concept IsFunction = std::is_function_v<T>;

template <typename T> concept IsUniquePtr =
    std::same_as<std::remove_cvref_t<T>, std::unique_ptr<typename T::pointer>> ||
    std::same_as<std::remove_cvref_t<T>, std::unique_ptr<typename T::element_type[]>>;

template <typename T> concept IsSharedPtr =
    std::same_as<std::remove_cvref_t<T>, std::shared_ptr<typename T::pointer>> ||
    std::same_as<std::remove_cvref_t<T>, std::shared_ptr<typename T::element_type[]>>;

template <typename T> concept IsAnyPointer =
    IsPointer<T> || IsUniquePtr<T> || IsSharedPtr<T>;

template <typename T> concept IsStringVariant =
    std::same_as<std::remove_cvref_t<T>, std::string> ||
    std::same_as<std::remove_cvref_t<T>, std::string_view>;

template <typename T> concept IsNotConst      = !IsConst<T>;
template <typename T> concept IsNotTrivial    = !IsTrivial<T>;
template <typename T> concept IsNotPointer    = !IsPointer<T>;
template <typename T> concept IsNotArray      = !IsArray<T>;
template <typename T> concept IsNotIntegral   = !IsIntegral<T>;
template <typename T> concept IsNotUniquePtr  = !IsUniquePtr<T>;
template <typename T> concept IsNotAnyPointer = !IsAnyPointer<T>;

template <typename T> concept Stringable =
    std::convertible_to<T, std::string> ||
    std::same_as<std::remove_cvref_t<T>, std::string> ||
    std::same_as<std::remove_cvref_t<T>, std::string_view>;

template <typename T> concept StringLiteral =
    std::is_reference_v<T> &&
    std::is_array_v<std::remove_reference_t<T>> &&
    ( std::convertible_to<std::remove_reference_t<T>, char const *> ||
      std::convertible_to<T, char const *(&)> ||
      std::convertible_to<T, char const *(&&)> );

template <typename T> concept NonStringable    = !Stringable<T>;
template <typename T> concept NonStringLiteral = requires(T x) {
    !StringLiteral<T>;
    IsNotConst<T>;
    { *x } -> IsNotConst<>;
};

} // namespace util::concepts
} // namespace emlsp
