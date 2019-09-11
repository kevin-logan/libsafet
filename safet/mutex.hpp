#include <mutex>

namespace safet {
template <typename T>
class mutex {
public:
    mutex(T value)
        : m_value(value)
    {
    }

    template <typename... Params, typename = std::enable_if_t<std::is_constructible_v<T, Params...>>>
    mutex(Params&&... params)
        : m_value(std::forward<Params>(params)...)
    {
    }

    mutex(const mutex<T>& copy) = delete;
    mutex(mutex<T>&& move) = delete;

    template <typename Functor>
    auto acquire(Functor&& f) & -> void
    {
        std::scoped_lock guard{ m_mutex };
        std::forward<Functor>(f)(m_value);
    }

    template <typename Functor>
    auto acquire(Functor&& f) && -> void
    {
        std::scoped_lock guard{ m_mutex };
        std::forward<Functor>(f)(std::move(m_value));
    }

    template <typename Functor>
    auto acquire(Functor&& f) const& -> void
    {
        std::scoped_lock guard{ m_mutex };
        std::forward<Functor>(f)(m_value);
    }

    template <typename Functor>
    auto try_acquire(Functor&& f) & -> bool
    {
        std::unique_lock guard{ m_mutex, std::try_to_lock };
        if (guard.owns_lock()) {
            std::forward<Functor>(f)(m_value);
            return true;
        }

        return false;
    }

    template <typename Functor>
    auto try_acquire(Functor&& f) && -> bool
    {
        std::unique_lock guard{ m_mutex, std::try_to_lock };
        if (guard.owns_lock()) {
            std::forward<Functor>(f)(std::move(m_value));
            return true;
        }

        return false;
    }

    template <typename Functor>
    auto try_acquire(Functor&& f) const& -> bool
    {
        std::unique_lock guard{ m_mutex, std::try_to_lock };
        if (guard.owns_lock()) {
            std::forward<Functor>(f)(m_value);
            return true;
        }

        return false;
    }

    template <typename LockedFunctor, typename NotLockedFunctor>
    auto try_acquire(LockedFunctor&& if_locked, NotLockedFunctor&& if_not_locked) & -> void
    {
        std::unique_lock guard{ m_mutex, std::try_to_lock };
        if (guard.owns_lock()) {
            std::forward<LockedFunctor>(if_locked)(m_value);
        } else {
            std::forward<NotLockedFunctor>(if_not_locked)();
        }
    }

    template <typename LockedFunctor, typename NotLockedFunctor>
    auto try_acquire(LockedFunctor&& if_locked, NotLockedFunctor&& if_not_locked) && -> void
    {
        std::unique_lock guard{ m_mutex, std::try_to_lock };
        if (guard.owns_lock()) {
            std::forward<LockedFunctor>(if_locked)(std::move(m_value));
        } else {
            std::forward<NotLockedFunctor>(if_not_locked)();
        }
    }

    template <typename LockedFunctor, typename NotLockedFunctor>
    auto try_acquire(LockedFunctor&& if_locked, NotLockedFunctor&& if_not_locked) const& -> void
    {
        std::unique_lock guard{ m_mutex, std::try_to_lock };
        if (guard.owns_lock()) {
            std::forward<LockedFunctor>(if_locked)(m_value);
        } else {
            std::forward<NotLockedFunctor>(if_not_locked)();
        }
    }

private:
    mutable std::mutex m_mutex;
    T m_value;
};
}
