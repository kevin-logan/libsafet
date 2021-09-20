#pragma once

#include <safet/impl/concepts.hpp>
#include <safet/optional.hpp>

#include <mutex>

namespace safet {
template <typename T>
class critical_section {
public:
    template <typename... Params, typename = std::enable_if_t<std::is_constructible_v<T, Params&&...>>>
    critical_section(Params&&... params)
        : m_value(std::forward<Params>(params)...)
    {
    }

    critical_section(const critical_section<T>& copy) = delete;
    critical_section(critical_section<T>&& move) = delete;

    ~critical_section() = default;

    auto operator=(const critical_section<T>& copy) -> critical_section<T>& = delete;
    auto operator=(critical_section<T>&& move) -> critical_section<T>& = delete;

    auto operator=(T new_value) -> critical_section<T>&
    {
        enter([new_value = std::move(new_value)](T& value) {
            value = std::move(new_value);
        });
    }

    template <typename Functor>
    decltype(auto) enter(Functor&& f) &
    {
        static_assert(impl::invocable<Functor&&, T&>, "enter functor must be invocable with T&");

        std::unique_lock guard { m_mutex };
        if constexpr (impl::invocable_and_returns_something<Functor&&, T&>) {
            return std::forward<Functor>(f)(m_value);
        } else {
            std::forward<Functor>(f)(m_value);
        }
    }

    template <typename Functor>
    decltype(auto) enter(Functor&& f) const&
    {
        static_assert(impl::invocable<Functor&&, const T&>, "enter functor must be invocable with const T&");

        std::unique_lock guard { m_mutex };
        if constexpr (impl::invocable_and_returns_something<Functor&&, const T&>) {
            return std::forward<Functor>(f)(m_value);
        } else {
            std::forward<Functor>(f)(m_value);
        }
    }

    template <typename Functor>
    decltype(auto) enter(Functor&& f) &&
    {
        static_assert(impl::invocable<Functor&&, T&&>, "enter functor must be invocable with T&&");

        std::unique_lock guard { m_mutex };
        if constexpr (impl::invocable_and_returns_something<Functor&&, T&&>) {
            return std::forward<Functor>(f)(std::move(m_value));
        } else {
            std::forward<Functor>(f)(std::move(m_value));
        }
    }

    template <typename Functor>
    decltype(auto) try_enter(Functor&& f) &
    {
        static_assert(impl::invocable<Functor&&, T&>, "enter functor must be invocable with T&");

        std::unique_lock guard { m_mutex, std::try_to_lock };
        if constexpr (impl::invocable_and_returns_something<Functor&&, T&>) {
            return [&]() -> optional<std::invoke_result_t<Functor&&, T&>> {
                if (guard.owns_lock()) {
                    return std::forward<Functor>(f)(m_value);
                } else {
                    return std::nullopt;
                }
            }();
        } else {
            if (guard.owns_lock()) {
                std::forward<Functor>(f)(m_value);

                return true;
            }

            return false;
        }
    }

    template <typename Functor>
    decltype(auto) try_enter(Functor&& f) const&
    {
        static_assert(impl::invocable<Functor&&, const T&>, "enter functor must be invocable with const T&");

        std::unique_lock guard { m_mutex, std::try_to_lock };
        if constexpr (impl::invocable_and_returns_something<Functor&&, const T&>) {
            return [&]() -> optional<std::invoke_result_t<Functor&&, const T&>> {
                if (guard.owns_lock()) {
                    return std::forward<Functor>(f)(m_value);
                } else {
                    return std::nullopt;
                }
            }();
        } else {
            if (guard.owns_lock()) {
                std::forward<Functor>(f)(m_value);

                return true;
            }

            return false;
        }
    }

    template <typename Functor>
    decltype(auto) try_enter(Functor&& f) &&
    {
        static_assert(impl::invocable<Functor&&, T&&>, "enter functor must be invocable with T&&");

        std::unique_lock guard { m_mutex, std::try_to_lock };
        if constexpr (impl::invocable_and_returns_something<Functor&&, T&&>) {
            return [&]() -> optional<std::invoke_result_t<Functor&&, T&&>> {
                if (guard.owns_lock()) {
                    return std::forward<Functor>(f)(std::move(m_value));
                } else {
                    return std::nullopt;
                }
            }();
        } else {
            if (guard.owns_lock()) {
                std::forward<Functor>(f)(std::move(m_value));

                return true;
            }

            return false;
        }
    }

private:
    mutable std::mutex m_mutex;
    T m_value;
};
}
