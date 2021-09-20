#pragma once

#include <safet/impl/concepts.hpp>

#include <condition_variable>
#include <mutex>

namespace safet {
template <typename T>
class condition_variable {
public:
    enum class notification_type {
        NO_NOTIFY,
        NOTIFY_ONE,
        NOTIFY_ALL,
    };

    template <typename... Args>
    condition_variable(Args&&... args)
        : m_value(std::forward<Args>(args)...)
    {
    }

    condition_variable(const condition_variable&) = delete;
    condition_variable(condition_variable&&) = delete;

    ~condition_variable() = default;

    auto operator=(const condition_variable&) -> condition_variable& = delete;
    auto operator=(condition_variable&&) -> condition_variable& = delete;

    auto notify(notification_type n = notification_type::NOTIFY_ALL) -> void
    {
        switch (n) {
        case notification_type::NOTIFY_ALL:
            m_cv.notify_all();
            break;
        case notification_type::NOTIFY_ONE:
            m_cv.notify_one();
            break;
        }
    }

    template <impl::invocable<T&> ModifyFunctor>
    auto modify(ModifyFunctor&& f, notification_type n = notification_type::NOTIFY_ALL) -> std::invoke_result_t<ModifyFunctor&&, T&>
    {
        decltype(auto) ret_val = [&]() -> decltype(auto) {
            std::unique_lock l { m_mutex };
            return std::forward<ModifyFunctor>(f)(m_value);
        }();

        notify(n);

        return ret_val;
    }

    template <impl::invocable<const T&> InspectFunctor>
    auto inspect(InspectFunctor&& f) const -> std::invoke_result_t<InspectFunctor&&, const T&>
    {
        std::unique_lock l { m_mutex };
        return std::forward<InspectFunctor>(f)(m_value);
    }

    template <typename WaitCondFunctor, typename ReadyFunctor>
    decltype(auto) wait(WaitCondFunctor&& wait_functor, ReadyFunctor&& ready_functor) &
    {
        static_assert(impl::invocable<WaitCondFunctor&, const T&>, "wait WaitCondFunctor must be invocable with const T&");
        static_assert(impl::invocable<ReadyFunctor&&, T&>, "wait ReadyFunctor must be invocable with T&");

        std::unique_lock l { m_mutex };
        m_cv.wait(l, [&]() {
            // cannot forward wait_functor as it can be called multiple times. Still, we take it as a
            // forwarding reference so that we can accept const and non-const arguments, so long as
            // they are callable
            return wait_functor(static_cast<const T&>(m_value));
        });

        return std::forward<ReadyFunctor>(ready_functor)(m_value);
    }

    template <typename WaitCondFunctor, typename ReadyFunctor>
    decltype(auto) wait(WaitCondFunctor&& wait_functor, ReadyFunctor&& ready_functor) const&
    {
        static_assert(impl::invocable<WaitCondFunctor&, const T&>, "wait WaitCondFunctor must be invocable with const T&");
        static_assert(impl::invocable<ReadyFunctor&&, const T&>, "wait ReadyFunctor must be invocable with const T&");

        std::unique_lock l { m_mutex };
        m_cv.wait(l, [&]() {
            // cannot forward wait_functor as it can be called multiple times. Still, we take it as a
            // forwarding reference so that we can accept const and non-const arguments, so long as
            // they are callable
            return wait_functor(m_value);
        });

        return std::forward<ReadyFunctor>(ready_functor)(m_value);
    }

    template <typename WaitCondFunctor, typename ReadyFunctor>
    decltype(auto) wait(WaitCondFunctor&& wait_functor, ReadyFunctor&& ready_functor) &&
    {
        static_assert(impl::invocable<WaitCondFunctor&, const T&>, "wait WaitCondFunctor must be invocable with const T&");
        static_assert(impl::invocable<ReadyFunctor&&, T&&>, "wait ReadyFunctor must be invocable with T&&");

        std::unique_lock l { m_mutex };
        m_cv.wait(l, [&]() {
            // cannot forward wait_functor as it can be called multiple times. Still, we take it as a
            // forwarding reference so that we can accept const and non-const arguments, so long as
            // they are callable
            return wait_functor(static_cast<const T&>(m_value));
        });

        return std::forward<ReadyFunctor>(ready_functor)(std::move(m_value));
    }

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    T m_value;
};
}