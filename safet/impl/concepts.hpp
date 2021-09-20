#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

//namespace updated {
namespace safet::impl {
template <typename T, typename... Args>
concept invocable = requires(T t, Args... a)
{
    { std::forward<T>(t)(std::forward<Args>(a)...) };
};

template <typename T, typename Return, typename... Args>
concept invocable_and_returns = requires(T t, Args... a)
{
    {
        std::forward<T>(t)(std::forward<Args>(a)...)
        }
        -> std::same_as<Return>;
};

template <typename T, typename... Args>
concept invocable_and_returns_nothing = invocable_and_returns<T, void, Args...>;

template <typename T, typename... Args>
concept invocable_and_returns_something = invocable<T, Args...> && !invocable_and_returns_nothing<T, Args...>;

template <typename T>
concept reference = std::is_reference<T>::value;

template <typename T>
concept value = !reference<T>;

template <typename T, typename... Args>
concept one_of = std::disjunction_v<std::is_same<T, Args>...>;

template <typename T, typename U>
concept decays_to = std::is_same_v<std::decay_t<T>, U>;
}