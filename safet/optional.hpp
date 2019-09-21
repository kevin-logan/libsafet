#pragma once

#include <optional>

namespace safet {
template <typename T>
class optional {
public:
    optional() {}

    optional(T value)
        : m_value(std::move(value))
    {
    }

    optional(const optional<T>&) = default;
    optional(optional<T>&&) = default;

    auto operator=(const optional<T>&) -> optional<T>& = default;
    auto operator=(optional<T> &&) -> optional<T>& = default;

    auto operator=(T value) -> optional<T>&
    {
        m_value = std::move(value);

        return *this;
    }

    auto operator=(std::nullopt_t) -> optional<T>&
    {
        m_value = std::nullopt;

        return *this;
    }

    template <typename... Params>
    auto emplace(Params&&... params) -> T&
    {
        return m_value.emplace(std::forward<Params>(params)...);
    }

    template <typename Functor>
    auto if_set(Functor&& f) -> optional<T>&
    {
        if (m_value.has_value()) {
            std::forward<Functor>(f)(m_value.value());
        }

        return *this;
    }

    template <typename Functor>
    auto if_set(Functor&& f) const -> const optional<T>&
    {
        if (m_value.has_value()) {
            std::forward<Functor>(f)(m_value.value());
        }

        return *this;
    }

    template <typename Functor>
    auto if_empty(Functor&& f) -> optional<T>&
    {
        if (!m_value.has_value()) {
            std::forward<Functor>(f)();
        }

        return *this;
    }

    template <typename Functor>
    auto if_empty(Functor&& f) const -> const optional<T>&
    {
        if (!m_value.has_value()) {
            std::forward<Functor>(f)();
        }

        return *this;
    }

    template <typename SetFunctor, typename EmptyFunctor>
    auto handle(SetFunctor&& if_set, EmptyFunctor&& if_empty) & -> std::enable_if_t<std::is_same<std::invoke_result_t<SetFunctor, T&>, std::invoke_result_t<EmptyFunctor>>::value, std::invoke_result_t<EmptyFunctor>>
    {
        if (m_value.has_value()) {
            return std::forward<SetFunctor>(if_set)(m_value.value());
        } else {
            return std::forward<EmptyFunctor>(if_empty)();
        }
    }

    template <typename SetFunctor, typename EmptyFunctor>
    auto handle(SetFunctor&& if_set, EmptyFunctor&& if_empty) && -> std::enable_if_t<std::is_same<std::invoke_result_t<SetFunctor, T>, std::invoke_result_t<EmptyFunctor>>::value, std::invoke_result_t<EmptyFunctor>>
    {
        if (m_value.has_value()) {
            return std::forward<SetFunctor>(if_set)(std::move(m_value.value()));
        } else {
            return std::forward<EmptyFunctor>(if_empty)();
        }
    }

    template <typename SetFunctor, typename EmptyFunctor>
    auto handle(SetFunctor&& if_set, EmptyFunctor&& if_empty) const& -> std::enable_if_t<std::is_same<std::invoke_result_t<SetFunctor, const T&>, std::invoke_result_t<EmptyFunctor>>::value, std::invoke_result_t<EmptyFunctor>>
    {
        if (m_value.has_value()) {
            return std::forward<SetFunctor>(if_set)(m_value.value());
        } else {
            return std::forward<EmptyFunctor>(if_empty)();
        }
    }

    template <typename InstantiateFunctor>
    auto get_or_instantiate(InstantiateFunctor&& functor) -> T&
    {
        if (!m_value.has_value()) {
            m_value.emplace(functor());
        }

        return m_value.value();
    }

private:
    std::optional<T> m_value;
};
}
