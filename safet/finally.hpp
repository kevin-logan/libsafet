#pragma once

#include <safet/impl/concepts.hpp>
#include <safet/optional.hpp>

namespace safet {
template <impl::invocable Functor>
class finally {
public:
    finally(Functor f)
        : m_finally(std::move(f))
    {
    }

    finally(const finally&) = default;
    finally(finally&& move)
        : m_finally(std::move(move.m_finally))
    {
        move.m_finally = std::nullopt;
    }

    auto operator=(const finally&) -> finally& = default;
    auto operator=(finally&& move) -> finally&
    {
        m_finally = std::move(move.m_finally);
        move.m_finally = std::nullopt;
    }

    ~finally()
    {
        std::move(m_finally).if_set([](Functor&& finally) {
            std::move(finally)();
        });
    }

private:
    optional<Functor> m_finally;
};
}