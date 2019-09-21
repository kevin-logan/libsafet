#pragma once

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
    auto deref_or(HasValueFunctor&& has_value_functor, NullptrFunctor&& nullptr_functor) const -> void
    {
        if (m_ptr != nullptr) {
            std::forward<HasValueFunctor>(has_value_functor)(*m_ptr);
        } else {
            std::forward<NullptrFunctor>(nullptr_functor)();
        }
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
    auto deref_or(HasValueFunctor&& has_value_functor, NullptrFunctor&& nullptr_functor) const -> void
    {
        if (m_ptr != nullptr) {
            std::forward<HasValueFunctor>(has_value_functor)(*m_ptr);
        } else {
            std::forward<NullptrFunctor>(nullptr_functor)();
        }
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

private:
    std::weak_ptr<T> m_ptr;
};
}
