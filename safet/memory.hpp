#pragma once

#include <atomic>
#include <memory>

namespace safet {

template <typename T, typename D = std::default_delete<T>>
class unique_ptr {
public:
    static_assert(!std::is_reference_v<D>, "unique_ptr deleter D musn't be a reference, use std::reference_wrapper if desired");

    constexpr unique_ptr() noexcept { }
    constexpr unique_ptr(std::nullptr_t) noexcept { }
    explicit unique_ptr(T* ptr) noexcept
        : m_ptr(ptr)
    {
    }
    unique_ptr(T* ptr, const D& deleter) noexcept
        : m_ptr(ptr, deleter)
    {
    }
    unique_ptr(T* ptr, D&& deleter) noexcept
        : m_ptr(ptr, std::move(deleter))
    {
    }
    unique_ptr(std::unique_ptr<T, D> ptr) noexcept
        : m_ptr(std::move(ptr))
    {
    }
    unique_ptr(const unique_ptr<T, D>&) = delete;
    unique_ptr(unique_ptr<T, D>&&) noexcept = default;

    ~unique_ptr() = default;

    auto operator=(const unique_ptr<T, D>&) noexcept -> unique_ptr<T, D>& = delete;
    auto operator=(unique_ptr<T, D>&&) noexcept -> unique_ptr<T, D>& = default;

    auto deref() const -> optional<T&>
    {
        if (m_ptr != nullptr) {
            return *m_ptr;
        } else {
            return std::nullopt;
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
    std::unique_ptr<T, D> m_ptr;
};

template <typename T, typename... Params>
auto make_unique(Params&&... params) -> unique_ptr<T>
{
    return unique_ptr<T> { std::make_unique<T>(std::forward<Params>(params)...) };
}

template <typename T>
class shared_ptr {
public:
    constexpr shared_ptr() noexcept { }
    constexpr shared_ptr(std::nullptr_t) noexcept { }
    explicit shared_ptr(T* ptr)
        : m_ptr(ptr)
    {
    }
    shared_ptr(std::shared_ptr<T> ptr) noexcept
        : m_ptr(std::move(ptr))
    {
    }
    shared_ptr(const shared_ptr<T>&) noexcept = default;
    shared_ptr(shared_ptr<T>&&) noexcept = default;

    ~shared_ptr() = default;

    auto operator=(const shared_ptr<T>&) noexcept -> shared_ptr<T>& = default;
    auto operator=(shared_ptr<T>&&) noexcept -> shared_ptr<T>& = default;

    auto deref() const -> optional<T&>
    {
        if (m_ptr != nullptr) {
            return *m_ptr;
        } else {
            return std::nullopt;
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
    return shared_ptr<T> { std::make_shared<T>(std::forward<Params>(params)...) };
}

template <typename T>
class weak_ptr {
public:
    weak_ptr() noexcept { }
    weak_ptr(std::weak_ptr<T> ptr) noexcept
        : m_ptr(std::move(ptr))
    {
    }
    weak_ptr(const shared_ptr<T>& s_ptr) noexcept
        : m_ptr(s_ptr.m_ptr)
    {
    }
    weak_ptr(const weak_ptr<T>&) noexcept = default;
    weak_ptr(weak_ptr<T>&&) noexcept = default;

    ~weak_ptr() = default;

    auto operator=(const weak_ptr<T>&) noexcept -> weak_ptr<T>& = default;
    auto operator=(weak_ptr<T>&&) noexcept -> weak_ptr<T>& = default;

    auto operator=(const shared_ptr<T>& s_ptr) -> weak_ptr<T>
    {
        m_ptr = s_ptr.m_ptr;

        return *this;
    }

    auto lock() -> shared_ptr<T>
    {
        return shared_ptr<T> { m_ptr.lock() };
    }

    auto clear() -> void
    {
        m_ptr.reset();
    }

private:
    std::weak_ptr<T> m_ptr;
};
}
