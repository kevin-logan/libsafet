#pragma once

#include <safet/variant.hpp>

namespace safet {
template <typename T>
class cow {
public:
    static_assert(std::is_copy_constructible_v<T>, "cow type must be copy constructible");

    cow(T value)
        : m_value(std::in_place_type_t<T> {}, std::move(value))
    {
    }
    cow(std::reference_wrapper<const T> ref)
        : m_value(std::in_place_type_t<const T&> {}, ref.get())
    {
    }

    cow(const cow& copy)
        : m_value(std::in_place_type_t<const T&> {}, std::ref(copy.get_const()))
    {
    }
    cow(cow&&) = default;

    ~cow() = default;

    auto operator=(const cow& copy) -> cow&
    {
        m_value.template emplace<const T&>(copy.get_const());
        return *this;
    }
    auto operator=(cow&&) -> cow& = default;

    auto operator=(T value) -> cow&
    {
        m_value.template emplace<T>(std::move(value));

        return *this;
    }

    auto operator=(std::reference_wrapper<const T> ref) -> cow&
    {
        m_value.template emplace<const T&>(ref.get());

        return *this;
    }

    auto get_const() const -> const T&
    {
        return m_value.visit([](const auto& value) -> const T& {
            return value;
        });
    }

    auto get_mutable() -> T&
    {
        return m_value.visit(
            overloaded {
                [this](const T& value) -> T& {
                    // must be reference, so emplace on the variant to copy the value
                    return m_value.template emplace<T>(value);
                },
                [](T& value) -> T& {
                    return value;
                } });
    }

    auto set(T value) -> T&
    {
        return m_value.template emplace<T>(std::move(value));
    }

    auto set(std::reference_wrapper<const T> ref) -> const T&
    {
        return m_value.template emplace<const T&>(ref.get());
    }

    auto operator->() const -> const T*
    {
        return m_value.visit([](const auto& value) -> const T* {
            return &value;
        });
    }

private:
    variant<T, const T&> m_value;
};
}