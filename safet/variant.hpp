#pragma once

#include <safet/optional.hpp>
#include <safet/pack.hpp>

#include <functional>
#include <variant>

namespace safet {

template <typename... Args>
class variant;

namespace variant_impl {
    // acts as a very simple wrapper over a reference (even simpler than `std::reference_wrapper`). The only reason we don't
    // directly use a pointer in the variant class (nor use `std::reference_wrapper`) is because both those types could also
    // exist in the variant (e.g. `variant<int&, int*, std::refrence_wrapper<int>>`) and that would cause many ambiguity
    // issues, especially during calls to `visit`, so we use this type that should never be used otherwise, which guarantees
    // there will not be any unintentional type intersections
    template <typename T>
    struct wrapped_reference {
        T* m_value;
    };

    template <typename T, typename... Args>
    concept one_of_as_reference = impl::one_of<T&, Args...>;
}

template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename... Args>
class variant {
public:
    // store references as pointers
    template <typename T>
    using storage_type = std::conditional<std::is_reference<T>::value, variant_impl::wrapped_reference<std::remove_reference_t<T>>, T>::type;

    template <typename T = typename pack<Args...>::template ith_type<0>::type, typename = std::enable_if<std::is_constructible_v<T>>>
    variant()
        : variant(std::in_place_index_t<0> {}, T {})
    {
    }

    template <impl::value T, typename... ConstructArgs>
    explicit variant(std::in_place_type_t<T>, ConstructArgs&&... args)
        : m_variant(std::in_place_type_t<storage_type<T>> {}, std::forward<ConstructArgs>(args)...)
    {
    }

    template <impl::reference T>
    explicit variant(std::in_place_type_t<T>, std::type_identity_t<T> ref)
        : m_variant(std::in_place_type_t<storage_type<T>> {}, &ref)
    {
    }

    template <size_t I, typename... ConstructArgs, impl::value = typename pack<Args...>::template ith_type<I>::type>
    explicit variant(std::in_place_index_t<I>, ConstructArgs&&... args)
        : m_variant(std::in_place_index_t<I> {}, std::forward<ConstructArgs>(args)...)
    {
    }

    template <size_t I, impl::reference = typename pack<Args...>::template ith_type<I>::type>
    explicit variant(std::in_place_index_t<I>, typename pack<Args...>::template ith_type<I>::type ref)
        : m_variant(std::in_place_index_t<I> {}, &ref)
    {
    }

    template <typename T, typename = std::enable_if_t<std::is_constructible_v<std::variant<storage_type<Args>...>, T&&>>>
    variant(T&& arg)
        : m_variant(std::forward<T>(arg))
    {
    }

    template <variant_impl::one_of_as_reference<Args...> T>
    variant(std::reference_wrapper<T> ref)
        : m_variant(variant_impl::wrapped_reference<T> { &ref.get() })
    {
    }

    ~variant() = default;

    variant(const variant<Args...>&) = default;
    variant(variant<Args...>&&) = default;

    auto operator=(const variant<Args...>&) -> variant<Args...>& = default;
    auto operator=(variant<Args...>&&) -> variant<Args...>& = default;

    template <impl::one_of<Args...> T>
    auto operator=(T value) -> variant<Args...>&
    {
        m_variant = transform_in(std::move(value));

        return *this;
    }

    template <variant_impl::one_of_as_reference<Args...> T>
    auto operator=(std::reference_wrapper<T> ref) -> variant<Args...>&
    {
        m_variant = variant_impl::wrapped_reference<T> { &ref.get() };

        return *this;
    }

    template <impl::value Type, typename... EmplaceArgs>
    auto emplace(EmplaceArgs&&... emplace_args) -> Type&
    {
        return m_variant.template emplace<storage_type<Type>>(std::forward<EmplaceArgs>(emplace_args)...);
    }

    template <impl::reference Type>
    auto emplace(Type ref) -> Type&
    {
        return *m_variant.template emplace<storage_type<Type>>(&ref).m_value;
    }

    template <size_t I, typename... EmplaceArgs>
    auto emplace(EmplaceArgs&&... emplace_args) -> typename pack<Args...>::template ith_type<I>::type&
    {
        return m_variant.template emplace<I>(std::forward<EmplaceArgs>(emplace_args)...);
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& v) &
    {
        return visit_helper(std::forward<Visitor>(v), typename pack<Args...>::remove_duplicates {});
    }
    template <typename Visitor>
    decltype(auto) visit(Visitor&& v) const&
    {
        return visit_helper(std::forward<Visitor>(v), typename pack<Args...>::remove_duplicates {});
    }
    template <typename Visitor>
    decltype(auto) visit(Visitor&& v) &&
    {
        return std::move(*this).visit_helper(std::forward<Visitor>(v), typename pack<Args...>::remove_duplicates {});
    }

    template <impl::one_of<Args...> T>
    auto get() & -> optional<T&>
    {
        if (std::holds_alternative<storage_type<T>>(m_variant)) {
            auto& val = std::get<storage_type<T>>(m_variant);

            return transform_out<T>(val);
        } else {
            return std::nullopt;
        }
    }

    template <impl::one_of<Args...> T>
    auto get() const& -> optional<const T&>
    {
        if (std::holds_alternative<storage_type<T>>(m_variant)) {
            auto& val = std::get<storage_type<T>>(m_variant);

            return transform_out<T>(val);
        } else {
            return std::nullopt;
        }
    }

    template <impl::one_of<Args...> T>
    auto get() && -> optional<T>
    {
        if (std::holds_alternative<storage_type<T>>(m_variant)) {
            auto& val = std::get<storage_type<T>>(m_variant);

            return transform_out<T>(std::move(val));
        } else {
            return std::nullopt;
        }
    }

    template <size_t I>
    auto get() & -> optional<typename pack<Args...>::template ith_type<I>::type&>
    {
        if (m_variant.index() == I) {
            auto& val = std::get<I>(m_variant);

            return transform_out<typename pack<Args...>::template ith_type<I>::type>(val);
        } else {
            return std::nullopt;
        }
    }

    template <size_t I>
    auto get() const& -> optional<const typename pack<Args...>::template ith_type<I>::type&>
    {
        if (m_variant.index() == I) {
            auto& val = std::get<I>(m_variant);

            return transform_out<typename pack<Args...>::template ith_type<I>::type>(val);
        } else {
            return std::nullopt;
        }
    }

    template <size_t I>
    auto get() && -> optional<typename pack<Args...>::template ith_type<I>::type>
    {
        if (m_variant.index() == I) {
            auto& val = std::get<I>(m_variant);

            return transform_out<typename pack<Args...>::template ith_type<I>::type>(std::move(val));
        } else {
            return std::nullopt;
        }
    }

    auto index() const -> size_t
    {
        return m_variant.index();
    }

    template <typename Covisitor>
    decltype(auto) covisit(impl::decays_to<variant<Args...>> auto&& other, Covisitor&& covisitor) &
    {
        return visit([&](impl::one_of<Args...> auto& first) {
            return std::forward<decltype(other)>(other).visit([&](auto&& second) {
                return std::forward<Covisitor>(covisitor)(first, std::forward<decltype(second)>(second));
            });
        });
    }

    template <typename Covisitor>
    decltype(auto) covisit(impl::decays_to<variant<Args...>> auto&& other, Covisitor&& covisitor) const&
    {
        return visit([&](const impl::one_of<Args...> auto& first) {
            return std::forward<decltype(other)>(other).visit([&](auto&& second) {
                return std::forward<Covisitor>(covisitor)(first, std::forward<decltype(second)>(second));
            });
        });
    }

    template <typename Covisitor>
    decltype(auto) covisit(impl::decays_to<variant<Args...>> auto&& other, Covisitor&& covisitor) &&
    {
        return visit([&](impl::one_of<Args...> auto&& first) {
            return std::forward<decltype(other)>(other).visit([&](auto&& second) {
                return std::forward<Covisitor>(covisitor)(std::move(first), std::forward<decltype(second)>(second));
            });
        });
    }

private:
    template <typename Visitor, typename... UniqueArgs>
    decltype(auto) visit_helper(Visitor&& v, pack<UniqueArgs...>) &
    {
        return std::visit(
            overloaded { [&](storage_type<UniqueArgs>& val) -> decltype(auto) {
                return std::forward<Visitor>(v)(transform_out<UniqueArgs>(val));
            }... },
            m_variant);
    }
    template <typename Visitor, typename... UniqueArgs>
    decltype(auto) visit_helper(Visitor&& v, pack<UniqueArgs...>) const&
    {
        return std::visit(
            overloaded { [&](const storage_type<UniqueArgs>& val) -> decltype(auto) {
                return std::forward<Visitor>(v)(transform_out<UniqueArgs>(val));
            }... },
            m_variant);
    }
    template <typename Visitor, typename... UniqueArgs>
    decltype(auto) visit_helper(Visitor&& v, pack<UniqueArgs...>) &&
    {
        return std::visit(
            overloaded { [&](storage_type<UniqueArgs>&& val) -> decltype(auto) {
                return std::forward<Visitor>(v)(transform_out<UniqueArgs>(std::move(val)));
            }... },
            std::move(m_variant));
    }

    template <typename T>
    static auto transform_out(storage_type<T>& val) -> T&
    {
        if constexpr (std::is_reference<T>::value) {
            return *val.m_value;
        } else {
            return val;
        }
    }
    template <typename T>
    static auto transform_out(const storage_type<T>& val) -> const T&
    {
        if constexpr (std::is_reference<T>::value) {
            return *val.m_value;
        } else {
            return val;
        }
    }
    template <typename T>
    static auto transform_out(storage_type<T>&& val) -> T&&
    {
        if constexpr (std::is_reference<T>::value) {
            return *val.m_value;
        } else {
            return std::move(val);
        }
    }
    template <typename T>
    static auto transform_in(T val) -> storage_type<T>
    {
        if constexpr (std::is_reference<T>::value) {
            return variant_impl::wrapped_reference<std::remove_reference_t<T>> { &val };
        } else {
            return val;
        }
    }
    std::variant<storage_type<Args>...> m_variant;
};

template <typename VariantType, typename... Args>
auto three_way_comparison_helper(const VariantType& lhs, const VariantType& rhs, pack<Args...>) -> std::common_comparison_category_t<std::compare_three_way_result_t<Args>...>
{

    // covisit with all overloads when types match implemented to call <=> operator, and fallback which returns <=> over the indices.
    // the fallback will never be used, actually, due to the runtime index check in the operator prior to calling this helper
    return lhs.covisit(rhs,
        overloaded {
            [&](const Args& lhs_value, const Args& rhs_value) -> std::common_comparison_category_t<std::compare_three_way_result_t<Args>...> { return lhs_value <=> rhs_value; }..., [&](auto&, auto&) -> std::common_comparison_category_t<std::compare_three_way_result_t<Args>...> { return lhs.index() <=> rhs.index(); } });
}

template <typename... Args>
auto operator<=>(const variant<Args...>& lhs, const variant<Args...>& rhs) -> std::common_comparison_category_t<std::compare_three_way_result_t<Args>...>
{
    const auto lhs_index = lhs.index();
    const auto rhs_index = rhs.index();

    if (lhs_index != rhs_index) {
        return lhs_index <=> rhs_index;
    }

    // we only want const T& overloads, so add const and & to each element in the pack
    // and then remove duplicates (so originally `int` and `const int&` would not
    // create ambiguous overloads)
    using constified_pack = typename pack<Args...>::transform<std::add_const>;
    using reference_pack = typename constified_pack::transform<std::add_lvalue_reference>;
    using no_duplicates_pack = typename reference_pack::remove_duplicates;

    return three_way_comparison_helper(lhs, rhs, no_duplicates_pack {});
}

template <typename VariantType, typename... Args>
auto equality_helper(const VariantType& lhs, const VariantType& rhs, pack<Args...>) -> bool
{
    // covisit with all overloads when types match implemented to call == operator, and fallback which returns false.
    // the fallback will never be used, actually, due to the runtime index check in the operator prior to calling this helper
    return lhs.covisit(rhs,
        overloaded {
            [&](const Args& lhs_value, const Args& rhs_value) { return lhs_value == rhs_value; }..., [&](auto&, auto&) { return false; } });
}

template <typename... Args>
auto operator==(const variant<Args...>& lhs, const variant<Args...>& rhs) -> bool
{
    const auto lhs_index = lhs.index();
    const auto rhs_index = rhs.index();

    if (lhs_index != rhs_index) {
        return false;
    }

    // we only want const T& overloads, so add const and & to each element in the pack
    // and then remove duplicates (so originally `int` and `const int&` would not
    // create ambiguous overloads)
    using constified_pack = typename pack<Args...>::transform<std::add_const>;
    using reference_pack = typename constified_pack::transform<std::add_lvalue_reference>;
    using no_duplicates_pack = typename reference_pack::remove_duplicates;

    return equality_helper(lhs, rhs, no_duplicates_pack {});
}
} // namespace safet
