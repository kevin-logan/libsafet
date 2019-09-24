#pragma once

#include <atomic>
#include <memory>

namespace safet {
template <typename T>
class unique_ptr {
public:
    unique_ptr() {}
    unique_ptr(T* ptr)
        : m_ptr(ptr)
    {
    }
    unique_ptr(std::unique_ptr<T> ptr)
        : m_ptr(std::move(ptr))
    {
    }
    unique_ptr(const unique_ptr<T>&) = delete;
    unique_ptr(unique_ptr<T>&&) = default;

    auto operator=(const unique_ptr<T>&) -> unique_ptr<T>& = delete;
    auto operator=(unique_ptr<T> &&) -> unique_ptr<T>& = default;

    template <typename Functor>
    auto deref(Functor&& f) const -> void
    {
        if (m_ptr != nullptr) {
            std::forward<Functor>(f)(*m_ptr);
        }
    }

    template <typename HasValueFunctor, typename NullptrFunctor>
    auto deref_or(HasValueFunctor&& has_value_functor, NullptrFunctor&& nullptr_functor) const -> std::enable_if_t<std::is_same<std::invoke_result_t<HasValueFunctor, T&>, std::invoke_result_t<NullptrFunctor>>::value, std::invoke_result_t<NullptrFunctor>>
    {
        if (m_ptr != nullptr) {
            return std::forward<HasValueFunctor>(has_value_functor)(*m_ptr);
        } else {
            return std::forward<NullptrFunctor>(nullptr_functor)();
        }
    }

    auto empty() const -> bool
    {
        return m_ptr == nullptr;
    }

    auto clear() -> void
    {
        m_ptr = nullptr;
    }

private:
    std::unique_ptr<T> m_ptr;
};

template <typename T, typename... Params>
auto make_unique(Params&&... params) -> unique_ptr<T>
{
    return unique_ptr<T>{ new T{ std::forward<Params>(params)... } };
}

template <typename T>
class shared_ptr {
public:
    shared_ptr() {}
    shared_ptr(T* ptr)
        : m_ptr(ptr)
    {
    }
    shared_ptr(std::shared_ptr<T> ptr)
        : m_ptr(std::move(ptr))
    {
    }
    shared_ptr(const shared_ptr<T>&) = default;
    shared_ptr(shared_ptr<T>&&) = default;

    auto operator=(const shared_ptr<T>&) -> shared_ptr<T>& = default;
    auto operator=(shared_ptr<T> &&) -> shared_ptr<T>& = default;

    template <typename Functor>
    auto deref(Functor&& f) const -> void
    {
        if (m_ptr != nullptr) {
            std::forward<Functor>(f)(*m_ptr);
        }
    }

    template <typename HasValueFunctor, typename NullptrFunctor>
    auto deref_or(HasValueFunctor&& has_value_functor, NullptrFunctor&& nullptr_functor) const -> std::enable_if_t<std::is_same<std::invoke_result_t<HasValueFunctor, T&>, std::invoke_result_t<NullptrFunctor>>::value, std::invoke_result_t<NullptrFunctor>>
    {
        if (m_ptr != nullptr) {
            return std::forward<HasValueFunctor>(has_value_functor)(*m_ptr);
        } else {
            return std::forward<NullptrFunctor>(nullptr_functor)();
        }
    }

    auto empty() const -> bool
    {
        return m_ptr == nullptr;
    }

    auto clear() -> void
    {
        m_ptr = nullptr;
    }

private:
    std::shared_ptr<T> m_ptr;

    template <typename>
    friend class weak_ptr;
};

template <typename T, typename... Params>
auto make_shared(Params&&... params) -> shared_ptr<T>
{
    return shared_ptr<T>{ new T{ std::forward<Params>(params)... } };
}

template <typename T>
class weak_ptr {
public:
    weak_ptr() {}
    weak_ptr(T* ptr)
        : m_ptr(ptr)
    {
    }
    weak_ptr(std::weak_ptr<T> ptr)
        : m_ptr(std::move(ptr))
    {
    }
    weak_ptr(const shared_ptr<T>& s_ptr)
        : m_ptr(s_ptr.m_ptr)
    {
    }
    weak_ptr(const weak_ptr<T>&) = default;
    weak_ptr(weak_ptr<T>&&) = default;

    auto operator=(const weak_ptr<T>&) -> weak_ptr<T>& = default;
    auto operator=(weak_ptr<T> &&) -> weak_ptr<T>& = default;

    auto operator=(const shared_ptr<T>& s_ptr) -> weak_ptr<T>
    {
        m_ptr = s_ptr.m_ptr;

        return *this;
    }

    auto lock() -> shared_ptr<T>
    {
        return shared_ptr<T>{ m_ptr.lock() };
    }

    auto clear() -> void
    {
        m_ptr = nullptr;
    }

private:
    std::weak_ptr<T> m_ptr;
};

template <typename T>
class strong_ptr {
public:
    strong_ptr()
        : m_ptr(nullptr)
    {
    }

    template <typename... Params, typename = std::enable_if_t<std::is_constructible_v<T, Params...>>>
    strong_ptr(Params&&... params)
        : m_ptr(new Implementation{ std::forward<Params>(params)... })
    {
    }

    strong_ptr(const strong_ptr& copy)
        : m_ptr(copy.m_ptr)
    {
        // no need to lock because `copy` can't go out of scope yet, so we just increment
        if (m_ptr) {
            ++m_ptr->m_rc;
        }
    }

    strong_ptr(strong_ptr&& move)
        : m_ptr(std::move(move.m_ptr))
    {
        move.m_ptr = nullptr;
    }

    auto operator=(const strong_ptr& copy) -> strong_ptr&
    {
        clean_value();
        m_ptr = copy.m_ptr;

        // `copy` is holding m_ptr alive already, so safely increment value without worry
        // rc could reach 0 between assigning m_ptr and here
        if (m_ptr) {
            ++m_ptr->m_rc;
        }
    }
    auto operator=(strong_ptr&& move) -> strong_ptr&
    {
        clean_value();
        m_ptr = std::move(move.m_ptr);
        move.m_ptr = nullptr;
    }

    ~strong_ptr()
    {
        clean_value();
    }

    template <typename Functor>
    auto deref(Functor&& f) const -> void
    {
        if (m_ptr != nullptr) {
            std::forward<Functor>(f)(m_ptr->m_value);
        }
    }

    template <typename HasValueFunctor, typename NullptrFunctor>
    auto deref_or(HasValueFunctor&& has_value_functor, NullptrFunctor&& nullptr_functor) const -> std::enable_if_t<std::is_same<std::invoke_result_t<HasValueFunctor, T&>, std::invoke_result_t<NullptrFunctor>>::value, std::invoke_result_t<NullptrFunctor>>
    {
        if (m_ptr == nullptr) {
            return std::forward<NullptrFunctor>(nullptr_functor)();
        } else {
            return std::forward<HasValueFunctor>(has_value_functor)(m_ptr->m_value);
        }
    }

    auto empty() const -> bool
    {
        return m_ptr == nullptr;
    }

    template <typename... Params, typename = std::enable_if_t<std::is_constructible_v<T, Params...>>>
    auto emplace(Params&&... params) -> T&
    {
        clean_value();
        m_ptr = new Implementation{ std::forward<Params>(params)... };

        return m_ptr->m_value;
    }

    auto clear() -> void
    {
        clean_value();
    }

private:
    auto clean_value()
    {
        if (m_ptr) {
            if (m_ptr->m_rc.fetch_sub(1) == 1) {
                delete m_ptr;
            }
        }
        m_ptr = nullptr;
    }

    struct Implementation {
        template <typename... Params, typename = std::enable_if_t<std::is_constructible_v<T, Params...>>>
        Implementation(Params&&... params)
            : m_value(std::forward<Params>(params)...)
            , m_rc(0)
        {
        }

        T m_value;
        std::atomic<uint64_t> m_rc;
    };

    Implementation* m_ptr;
};

template <typename T, typename... Params>
auto make_strong(Params&&... params) -> strong_ptr<T>
{
    return strong_ptr<T>{ std::forward<Params>(params)... };
}
}
