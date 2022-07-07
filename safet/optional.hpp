#pragma once

#include <safet/impl/concepts.hpp>

#include <compare>
#include <cstdint>
#include <memory>
#include <optional>

namespace safet {

template <typename T>
class optional;

namespace optional_impl {
    template <typename T>
    struct is_optional : std::false_type {
    };

    template <typename T>
    struct is_optional<optional<T>> : std::true_type {
    };

    template <typename T>
    struct innermost_type {
        using type = T;
    };

    template <typename T>
    struct innermost_type<optional<T>> {
        using type = innermost_type<T>::type;
    };

    template <typename T, typename... Args>
    concept invocable_and_returns_optional = impl::invocable<T, Args...> && is_optional<std::invoke_result_t<T, Args...>>::value;

    template <typename T, typename OptionalType>
    concept three_way_comparable_with_optional = !std::same_as<T, optional<OptionalType>> && std::three_way_comparable_with<T, OptionalType>;

    template <typename T, typename OptionalType>
    concept equality_comparable_with_optional = !std::same_as<T, optional<OptionalType>> && std::equality_comparable_with<T, OptionalType>;

}

template <typename T>
class optional {
public:
    using value_type = T;

    optional() noexcept
        : m_engaged(false)
    {
    }

    optional(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
        : m_engaged(false) // will be true upon `construct` being called below
    {
        construct(std::move(value));
    }

    template <typename... Args>
    optional(std::in_place_t, Args&&... args) noexcept(noexcept(T(std::declval<Args&&>()...)))
        : m_engaged(false) // will be true upon `construct` being called below
    {
        construct(std::forward<Args>(args)...);
    }

    optional(std::nullopt_t) noexcept
        : m_engaged(false)
    {
    }

    optional(const optional& copy) noexcept(std::is_nothrow_copy_constructible_v<T>)
        : m_engaged(false) // will be true upon `construct` being called below (if copy is engaged)
    {
        if (copy.m_engaged) {
            // copy construct value, engaged if we are since we just copied that value
            construct(copy.value());
        }
    }

    optional(optional&& move) noexcept(std::is_nothrow_move_constructible_v<T>)
        : m_engaged(false) // will be true upon `construct` being called below (if move is engaged)
    {
        if (move.m_engaged) {
            // move construct value, engaged if we are since we just copied that value
            construct(std::move(move).value());
        }
    }

    ~optional() noexcept(std::is_nothrow_destructible_v<T>)
    {
        destroy();
    }

    auto operator=(const optional& copy) noexcept(std::is_nothrow_copy_assignable_v<T>) -> optional&
    {
        if (&copy != this) {
            if (copy.m_engaged) {
                construct(copy.value());
            } else {
                // destroy a value if we have one
                destroy();
            }
        }

        return *this;
    }

    auto operator=(optional&& move) noexcept(std::is_nothrow_move_assignable_v<T>) -> optional&
    {
        if (&move != this) {
            if (move.m_engaged) {
                construct(std::move(move).value());
            } else {
                // destroy a value if we have one
                destroy();
            }
        }

        return *this;
    }

    auto operator=(T new_value) noexcept(std::is_nothrow_move_assignable_v<T>) -> optional&
    {
        construct(std::move(new_value));

        return *this;
    }

    auto operator=(std::nullopt_t) noexcept(std::is_nothrow_destructible_v<T>) -> optional&
    {
        destroy();

        return *this;
    }

    auto operator&&(bool condition) & noexcept -> optional<T&>
    {
        if (m_engaged && condition) {
            return optional<T&> { value() };
        } else {
            return std::nullopt;
        }
    }

    auto operator&&(bool condition) const& noexcept -> optional<const T&>
    {
        if (m_engaged && condition) {
            return optional<const T&> { value() };
        } else {
            return std::nullopt;
        }
    }

    auto operator&&(bool condition) && noexcept(std::is_nothrow_move_constructible_v<T>) -> optional<T>
    {
        if (m_engaged && condition) {
            return optional<T> { std::move(*this).value() };
        } else {
            return std::nullopt;
        }
    }

   template <typename Functor, typename... AdditionalArgs>
    decltype(auto) if_set(Functor&& f, AdditionalArgs&&... additional_args) &
    {
        static_assert(impl::invocable<Functor&&, T&, AdditionalArgs&&...>, "if_set functor must be invocable with T&");

        if constexpr (impl::invocable_and_returns_something<Functor&&, T&, AdditionalArgs&&...>) {
            return [&]() -> optional<std::invoke_result_t<Functor&&, T&, AdditionalArgs&&...>> {
                if (m_engaged) {
                    return std::forward<Functor>(f)(value(), std::forward<AdditionalArgs>(additional_args)...);
                }

                return std::nullopt;
            }();
        } else {
            if (m_engaged) {
                std::forward<Functor>(f)(value(), std::forward<AdditionalArgs>(additional_args)...);
            }

            return *this;
        }
    }

   template <typename Functor, typename... AdditionalArgs>
    decltype(auto) if_set(Functor&& f, AdditionalArgs&&... additional_args) const&
    {
        static_assert(impl::invocable<Functor&&, const T&, AdditionalArgs&&...>, "if_set functor on const optional must be invocable with const T&");

        if constexpr (impl::invocable_and_returns_something<Functor&&, const T&, AdditionalArgs&&...>) {
            return [&]() -> optional<std::invoke_result_t<Functor&&, const T&, AdditionalArgs&&...>> {
                if (m_engaged) {
                    return std::forward<Functor>(f)(value(), std::forward<AdditionalArgs>(additional_args)...);
                }

                return std::nullopt;
            }();
        } else {
            if (m_engaged) {
                std::forward<Functor>(f)(value(), std::forward<AdditionalArgs>(additional_args)...);
            }

            return *this;
        }
    }

    template <typename Functor, typename... AdditionalArgs>
    decltype(auto) if_set(Functor&& f, AdditionalArgs&&... additional_args) &&
    {
        static_assert(impl::invocable<Functor&&, T&&, AdditionalArgs&&...>, "if_set functor on r-value optional must be invocable with T&&");

        if constexpr (impl::invocable_and_returns_something<Functor&&, T&&, AdditionalArgs&&...>) {
            return [&]() -> optional<std::invoke_result_t<Functor&&, T&&, AdditionalArgs&&...>> {
                if (m_engaged) {
                    return std::forward<Functor>(f)(std::move(*this).value(), std::forward<AdditionalArgs>(additional_args)...);
                }

                return std::nullopt;
            }();
        } else {
            if (m_engaged) {
                std::forward<Functor>(f)(std::move(*this).value(), std::forward<AdditionalArgs>(additional_args)...);
            }

            return std::move(*this);
        }
    }

    template <typename Functor, typename... AdditionalArgs>
    requires(impl::invocable_and_returns<Functor&&, T, AdditionalArgs&&...>)
    auto value_or(Functor&& f, AdditionalArgs&&... additional_args) & -> T
    {
        if (m_engaged) {
            return value();
        } else {
            return std::forward<Functor>(f)(std::forward<AdditionalArgs>(additional_args)...);
        }
    }

    template <typename Functor, typename... AdditionalArgs>
    requires(impl::invocable_and_returns<Functor&&, T, AdditionalArgs&&...>)
    auto value_or(Functor&& f, AdditionalArgs&&... additional_args) const& -> T
    {
        if (m_engaged) {
            return value();
        } else {
            return std::forward<Functor>(f)(std::forward<AdditionalArgs>(additional_args)...);
        }
    }

    template <typename Functor, typename... AdditionalArgs>
    requires(impl::invocable_and_returns<Functor&&, T, AdditionalArgs&&...>)
    auto value_or(Functor&& f, AdditionalArgs&&... additional_args) && -> T
    {
        if (m_engaged) {
            return std::move(*this).value();
        } else {
            return std::forward<Functor>(f)(std::forward<AdditionalArgs>(additional_args)...);
        }
    }

    template <typename Functor, typename... AdditionalArgs>
    requires(impl::invocable_and_returns_something<Functor&&, AdditionalArgs&&...>)
    auto if_unset(Functor&& f, AdditionalArgs&&... additional_args) const -> optional<std::invoke_result_t<Functor&&, AdditionalArgs&&...>>
    {
        // const overload only, as we only ever inspect `m_engaged`
        if (m_engaged) {
            return std::nullopt;
        } else {
            return std::forward<Functor>(f)(std::forward<AdditionalArgs>(additional_args)...);
        }
    }

    template <typename Functor, typename... AdditionalArgs>
    requires(impl::invocable_and_returns_nothing<Functor&&, AdditionalArgs&&...>)
    auto if_unset(Functor&& f, AdditionalArgs&&... additional_args) & -> optional&
    {
        // const overload only, as we only ever inspect `m_engaged`
        if (!m_engaged) {
            std::forward<Functor>(f)(std::forward<AdditionalArgs>(additional_args)...);
        }

        return *this;
    }

    template <typename Functor, typename... AdditionalArgs>
    requires(impl::invocable_and_returns_nothing<Functor&&, AdditionalArgs&&...>)
    auto if_unset(Functor&& f, AdditionalArgs&&... additional_args) const& -> const optional&
    {
        // const overload only, as we only ever inspect `m_engaged`
        if (!m_engaged) {
            std::forward<Functor>(f)(std::forward<AdditionalArgs>(additional_args)...);
        }

        return *this;
    }

    template <typename Functor, typename... AdditionalArgs>
    requires(impl::invocable_and_returns_nothing<Functor&&, AdditionalArgs&&...>)
    auto if_unset(Functor&& f, AdditionalArgs&&... additional_args) && -> optional
    {
        // const overload only, as we only ever inspect `m_engaged`
        if (!m_engaged) {
            std::forward<Functor>(f)(std::forward<AdditionalArgs>(additional_args)...);
        }

        return std::move(*this);
    }

    template <typename Functor, typename... AdditionalArgs>
    decltype(auto) and_then(Functor&& f, AdditionalArgs&&... additional_args) &
    {
        static_assert(optional_impl::invocable_and_returns_optional<Functor&&, T&, AdditionalArgs&&...>, "and_then functor on optional must return an optional");

        return [&]() -> std::invoke_result_t<Functor&&, T&, AdditionalArgs&&...> {
            if (m_engaged) {
                return std::forward<Functor>(f)(value(), std::forward<AdditionalArgs>(additional_args)...);
            } else {
                return std::nullopt;
            }
        }();
    }

    template <typename Functor, typename... AdditionalArgs>
    decltype(auto) and_then(Functor&& f, AdditionalArgs&&... additional_args) const&
    {
        static_assert(optional_impl::invocable_and_returns_optional<Functor&&, const T&, AdditionalArgs&&...>, "and_then functor on optional must return an optional");

        return [&]() -> std::invoke_result_t<Functor&&, const T&, AdditionalArgs&&...> {
            if (m_engaged) {
                return std::forward<Functor>(f)(value(), std::forward<AdditionalArgs>(additional_args)...);
            } else {
                return std::nullopt;
            }
        }();
    }

    template <typename Functor, typename... AdditionalArgs>
    decltype(auto) and_then(Functor&& f, AdditionalArgs&&... additional_args) &&
    {
        static_assert(optional_impl::invocable_and_returns_optional<Functor&&, T&&, AdditionalArgs&&...>, "and_then functor on optional must return an optional");

        return [&]() -> std::invoke_result_t<Functor&&, T&&, AdditionalArgs&&...> {
            if (m_engaged) {
                return std::forward<Functor>(f)(std::move(*this).value(), std::forward<AdditionalArgs>(additional_args)...);
            } else {
                return std::nullopt;
            }
        }();
    }

    template <typename... Args>
    auto emplace(Args&&... args) -> T&
    {
        return construct(std::forward<Args>(args)...);
    }

    template <impl::invocable_and_returns<T> Functor>
    auto emplace_if_empty(Functor&& f) -> T&
    {
        if (m_engaged) {
            return value();
        } else {
            return construct(std::forward<Functor>(f)());
        }
    }

    auto collapse() && -> optional<typename optional_impl::innermost_type<T>::type>
    {
        if constexpr (optional_impl::is_optional<T>::value) {
            return std::move(*this).if_set([&](auto&& value) -> optional<typename optional_impl::innermost_type<T>::type> {
                                       return std::move(value).collapse();
                                   })
                .value_or([&]() -> optional<typename optional_impl::innermost_type<T>::type> { return std::nullopt; });
        } else {
            return std::move(*this);
        }
    }

    auto empty() const -> bool
    {
        return !m_engaged;
    }

private:
    // store references as pointers
    static constexpr auto is_reference = std::is_reference<T>::value;
    using storage_type = std::conditional<is_reference, std::add_pointer_t<std::remove_reference_t<T>>, T>::type;

    auto value() & -> T&
    {
        if constexpr (is_reference) {
            // stored as pointer, double deref
            return **reinterpret_cast<storage_type*>(m_buffer);
        } else {
            return *reinterpret_cast<storage_type*>(m_buffer);
        }
    }
    auto value() const& -> const T&
    {
        if constexpr (is_reference) {
            // stored as pointer, double deref
            return **reinterpret_cast<const storage_type*>(m_buffer);
        } else {
            return *reinterpret_cast<const storage_type*>(m_buffer);
        }
    }
    auto value() && -> T
    {
        if constexpr (is_reference) {
            // stored as pointer, double deref (no need to move a reference)
            return **reinterpret_cast<storage_type*>(m_buffer);
        } else {
            return std::move(*reinterpret_cast<storage_type*>(m_buffer));
        }
    }
    template <typename... Args>
    auto construct(Args&&... args) -> T&
    {
        if (m_engaged) {
            destroy();
        }

        m_engaged = true;

        if constexpr (is_reference) {
            // stored as pointer, double deref (no need to forward args, should always be 1 arg which is a reference)
            return **(new (m_buffer) storage_type(std::addressof(args)...));
        } else {
            return *(new (m_buffer) storage_type(std::forward<Args>(args)...));
        }
    }
    auto destroy() -> void
    {
        if (m_engaged) {
            // no need to destroy a reference
            if constexpr (!is_reference) {
                std::destroy_at(&value());
            }

            m_engaged = false;
        }
    }

    bool m_engaged { false };
    uint8_t m_buffer[sizeof(storage_type)];

};

template <typename T>
auto operator<=>(const optional<T>& lhs, const optional<T>& rhs) -> std::compare_three_way_result_t<T>
{
    return lhs.and_then([&](const T& lhs_value) {
                  return rhs.if_set([&](const T& rhs_value) {
                      return lhs_value <=> rhs_value;
                  });
              })
        .value_or([&]() {
            return !lhs.empty() <=> !rhs.empty();
        });
}

template <typename T>
auto operator==(const optional<T>& lhs, const optional<T>& rhs) -> bool
{
    return lhs.and_then([&](const T& lhs_value) {
                  return rhs.if_set([&](const T& rhs_value) {
                      // equal if both engaged and equal
                      return lhs_value == rhs_value;
                  });
              })
        .value_or([&]() {
            // equal if both empty
            return lhs.empty() && rhs.empty();
        });
}

template <typename T, optional_impl::three_way_comparable_with_optional<T> U>
auto operator<=>(const optional<T>& lhs, const U& rhs) -> std::compare_three_way_result_t<T, U>
{
    return lhs.if_set([&](const T& lhs_value) -> std::compare_three_way_result_t<T, U> {
                  return lhs_value <=> rhs;
              })
        .value_or([]() -> std::compare_three_way_result_t<T, U> { return std::strong_ordering::less; });
}

template <typename T, optional_impl::equality_comparable_with_optional<T> U>
auto operator==(const optional<T>& lhs, const U& rhs) -> bool
{
    return lhs.if_set([&](const T& lhs_value) {
                  return lhs_value == rhs;
              })
        .value_or([]() { return false; });
}

}
