#pragma once

#include <safet/pack.hpp>

#include <variant>

namespace safet {
template <typename... Args>
class variant {
public:
    template <typename... ConstructArgs, typename = std::enable_if_t<std::is_constructible_v<std::variant<Args...>, ConstructArgs...>>>
    variant(ConstructArgs&&... args)
        : m_variant(std::forward<ConstructArgs>(args)...)
    {
    }

    variant(const variant<Args...>&) = default;
    variant(variant<Args...>&&) = default;

    auto operator=(const variant<Args...>&) -> variant<Args...>& = default;
    auto operator=(variant<Args...> &&) -> variant<Args...>& = default;

    template <typename Type, typename... EmplaceArgs>
    auto emplace(EmplaceArgs&&... emplace_args)
    {
        return m_variant.template emplace<Type>(std::forward<EmplaceArgs>(emplace_args)...);
    }

    template <typename ArgType, typename Functor>
    auto if_set(Functor&& f) & -> std::invoke_result_t<Functor, ArgType&>
    {
        static_assert(pack<Args...>::template contains<ArgType>::value, "is_set type must be a type held by the variant");

        if (std::holds_alternative<ArgType>(m_variant)) {
            return std::forward<Functor>(f)(std::get<ArgType>(m_variant));
        }
    }
    template <typename ArgType, typename Functor>
    auto if_set(Functor&& f) const& -> std::invoke_result_t<Functor, const ArgType&>
    {
        static_assert(pack<Args...>::template contains<ArgType>::value, "is_set type must be a type held by the variant");

        if (std::holds_alternative<ArgType>(m_variant)) {
            return std::forward<Functor>(f)(std::get<ArgType>(m_variant));
        }
    }
    template <typename ArgType, typename Functor>
    auto if_set(Functor&& f) && -> std::invoke_result_t<Functor, ArgType>
    {
        static_assert(pack<Args...>::template contains<ArgType>::value, "is_set type must be a type held by the variant");

        if (std::holds_alternative<ArgType>(m_variant)) {
            return std::forward<Functor>(f)(std::get<ArgType>(std::move(m_variant)));
        }
    }

    template <typename ArgType, typename IfSetFunctor, typename ElseFunctor>
    auto if_set_else(IfSetFunctor&& if_set_f, ElseFunctor&& else_f) & -> std::enable_if_t<std::is_same<std::invoke_result_t<IfSetFunctor, ArgType&>, std::invoke_result_t<ElseFunctor>>::value, std::invoke_result_t<ElseFunctor>>
    {
        static_assert(pack<Args...>::template contains<ArgType>::value, "if_set_else type must be a type held by the variant");

        if (std::holds_alternative<ArgType>(m_variant)) {
            return std::forward<IfSetFunctor>(if_set_f)(std::get<ArgType>(m_variant));
        } else {
            return std::forward<ElseFunctor>(else_f)();
        }
    }
    template <typename ArgType, typename IfSetFunctor, typename ElseFunctor>
    auto if_set_else(IfSetFunctor&& if_set_f, ElseFunctor&& else_f) const& -> std::enable_if_t<std::is_same<std::invoke_result_t<IfSetFunctor, const ArgType&>, std::invoke_result_t<ElseFunctor>>::value, std::invoke_result_t<ElseFunctor>>
    {
        static_assert(pack<Args...>::template contains<ArgType>::value, "if_set_else type must be a type held by the variant");

        if (std::holds_alternative<ArgType>(m_variant)) {
            return std::forward<IfSetFunctor>(if_set_f)(std::get<ArgType>(m_variant));
        } else {
            return std::forward<ElseFunctor>(else_f)();
        }
    }
    template <typename ArgType, typename IfSetFunctor, typename ElseFunctor>
    auto if_set_else(IfSetFunctor&& if_set_f, ElseFunctor&& else_f) && -> std::enable_if_t<std::is_same<std::invoke_result_t<IfSetFunctor, ArgType>, std::invoke_result_t<ElseFunctor>>::value, std::invoke_result_t<ElseFunctor>>
    {
        static_assert(pack<Args...>::template contains<ArgType>::value, "if_set_else type must be a type held by the variant");

        if (std::holds_alternative<ArgType>(m_variant)) {
            return std::forward<IfSetFunctor>(if_set_f)(std::get<ArgType>(std::move(m_variant)));
        } else {
            return std::forward<ElseFunctor>(else_f)();
        }
    }

    template <typename ArgType, typename Functor>
    auto if_unset(Functor&& f) const& -> std::invoke_result_t<Functor>
    {
        static_assert(pack<Args...>::template contains<ArgType>::value, "is_set type must be a type held by the variant");

        if (!std::holds_alternative<ArgType>(m_variant)) {
            return std::forward<Functor>(f)();
        }
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& v) &
    {
        return std::visit(std::forward<Visitor>(v), m_variant);
    }
    template <typename Visitor>
    decltype(auto) visit(Visitor&& v) const&
    {
        return std::visit(std::forward<Visitor>(v), m_variant);
    }
    template <typename Visitor>
    decltype(auto) visit(Visitor&& v) &&
    {
        return std::visit(std::forward<Visitor>(v), std::move(m_variant));
    }

private:
    std::variant<Args...> m_variant;
};
} // namespace safet
