#pragma once

#include <safet/variant.hpp>

namespace safet {
template <typename T>
class cow {
public:
    static_assert(std::is_copy_constructible_v<T>, "cow type must be copy constructible");

    cow(std::remove_const_t<T> value)
        : m_value(std::move(value))
    {
    }
    cow(std::reference_wrapper<std::add_const_t<T>> ref)
        : m_value(std::move(ref))
    {
    }

    cow(const cow&) = default;
    cow(cow&&) = default;

    auto operator=(const cow&) -> cow& = default;
    auto operator=(cow &&) -> cow& = default;

    auto operator=(T value) -> cow&
    {
        m_value.template emplace<T>(std::move(value));

        return *this;
    }

    auto operator=(std::reference_wrapper<std::add_const_t<T>> ref) -> cow&
    {
        m_value.template emplace<std::reference_wrapper<std::add_const_t<T>>>(std::move(ref));

        return *this;
    }

    auto get_const() const -> const T&
    {
        return m_value.visit([](auto& value) -> const T& {
            using type = std::decay_t<decltype(value)>;

            if constexpr (std::is_same<type, T>::value) {
                return value;
            } else {
                // must be reference wrapper
                return value.get();
            }
        });
    }

    auto get_mutable() -> T&
    {
        return m_value.visit([this](auto& value) -> T& {
            using type = std::decay_t<decltype(value)>;

            if constexpr (std::is_same<type, T>::value) {
                return value;
            } else {
                // must be reference wrapper
                return m_value.template emplace<T>(value.get());
            }
        });
    }

    auto set(T value) -> T&
    {
        return m_value.template emplace<T>(std::move(value));
    }

    auto set(std::reference_wrapper<std::add_const_t<T>> ref) -> const T&
    {
        return m_value.template emplace<std::reference_wrapper<std::add_const_t<T>>>(std::move(ref));
    }

    auto operator-> () const -> const T*
    {
        m_value.visit([](auto& value) -> const T* {
            using type = std::decay_t<decltype(value)>;

            if constexpr (std::is_same<type, T>::value) {
                return &value;
            } else {
                // must be reference wrapper
                return &value.get();
            }
        });
    }

private:
    variant<T, std::reference_wrapper<std::add_const_t<T>>> m_value;
};
}