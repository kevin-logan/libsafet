#pragma once

#include <cstddef>
#include <type_traits>

namespace safet {

// predeclare - some impl items need this declaration
template <typename... PackArgs>
struct pack;

namespace pack_impl {
    template <template <typename...> typename DestinationType, typename SourceType>
    struct copy_template_args {
        using type = DestinationType<>;
    };

    template <template <typename...> typename DestinationType, template <typename...> typename SourceType, typename... SourceArgs>
    struct copy_template_args<DestinationType, SourceType<SourceArgs...>> {
        using type = DestinationType<SourceArgs...>;
    };

    template <typename... Ts>
    struct first_helper {
        static_assert(sizeof...(Ts) > 0, "Cannot get the first type of an empty parameter pack");
    };

    template <typename T, typename... Ts>
    struct first_helper<T, Ts...> {
        using type = T;
    };

    template <typename... T>
    struct last_helper {
        static_assert(sizeof...(T) > 0, "Cannot get the last type of an empty parameter pack");
    };

    template <typename T, typename T2, typename... Ts>
    struct last_helper<T, T2, Ts...> {
        using type = typename last_helper<T2, Ts...>::type;
    };

    template <typename T>
    struct last_helper<T> {
        using type = T;
    };

    template <size_t I, typename... Args>
    struct ith_helper {
        static_assert(I < sizeof...(Args), "Too few types in parameter pack to get the Ith type");
    };

    template <typename Arg, typename... Args>
    struct ith_helper<0, Arg, Args...> {
        using type = Arg;
    };

    template <size_t I, typename Arg, typename... Args>
    struct ith_helper<I, Arg, Args...> {
        using type = typename ith_helper<I - 1, Args...>::type;
    };

    template <typename NeedleType, size_t Count, typename... HaystackTypes>
    struct count_of_helper : std::integral_constant<size_t, Count> {
    };

    template <typename NeedleType, size_t Count, typename HaystackFirstType, typename... HaystackTypes>
    struct count_of_helper<NeedleType, Count, HaystackFirstType, HaystackTypes...>
        : std::integral_constant<
              size_t,
              count_of_helper<
                  NeedleType,
                  Count + std::conditional_t<std::is_same<NeedleType, HaystackFirstType>::value, std::integral_constant<size_t, 1>, std::integral_constant<size_t, 0>>::value,
                  HaystackTypes...>::value> {
    };

    template <size_t NumToRemove, typename... Types>
    struct remove_prefix_types {
    };

    template <size_t NumToRemove, typename FirstType, typename... Types>
    struct remove_prefix_types<NumToRemove, FirstType, Types...> {
        using type = typename remove_prefix_types<NumToRemove - 1, Types...>::type;
    };

    template <typename FirstType, typename... Types>
    struct remove_prefix_types<0, FirstType, Types...> {
        using type = pack<FirstType, Types...>;
    };

    template <size_t NumToKeep, typename... Types>
    struct remove_suffix_helper {
    };

    template <size_t NumToKeep, typename FirstType, typename... Types>
    struct remove_suffix_helper<NumToKeep, FirstType, Types...> {
        using type =
            typename remove_suffix_helper<NumToKeep - 1, Types...>::type::template apply<pack<FirstType>::template append>;
    };

    template <typename FirstType, typename... Types>
    struct remove_suffix_helper<1, FirstType, Types...> {
        using type = pack<FirstType>;
    };

    template <size_t NumToremove, typename... Types>
    struct remove_suffix_types {
        using type = typename remove_suffix_helper<sizeof...(Types) - NumToremove, Types...>::type;
    };

    template <template <typename...> typename Filter, typename... Types>
    struct filter_helper {
        using type = pack<>;
    };

    template <template <typename...> typename Filterer, typename FirstType, typename... Types>
    struct filter_helper<Filterer, FirstType, Types...> {
        using type = typename std::conditional<
            Filterer<FirstType>::value,
            typename filter_helper<Filterer, Types...>::type::template apply<pack<FirstType>::template append>,
            typename filter_helper<Filterer, Types...>::type>::type;
    };
} // namespace pack_impl

template <typename... PackArgs>
struct pack {
    using first_type = pack_impl::first_helper<PackArgs...>;
    using last_type = pack_impl::last_helper<PackArgs...>;
    template <size_t I>
    using ith_type = pack_impl::ith_helper<I, PackArgs...>;

    using size = std::integral_constant<size_t, sizeof...(PackArgs)>;

    using index_sequence_for = std::index_sequence_for<PackArgs...>;

    template <template <typename...> typename ApplyType>
    using apply = ApplyType<PackArgs...>;

    template <typename... AppendTs>
    using append = pack<PackArgs..., AppendTs...>;

    template <typename OtherVariadic>
    using append_variadic_args = typename pack_impl::copy_template_args<pack, OtherVariadic>::type::template apply<append>;

    template <typename TestType>
    using contains = std::disjunction<std::is_same<TestType, PackArgs>...>;
    template <typename... TestTypes>
    using is_equal = std::conjunction<std::is_same<TestTypes, PackArgs>...>;

    template <typename NeedleType>
    using count_of = pack_impl::count_of_helper<NeedleType, 0, PackArgs...>;

    template <typename... TestTypes>
    using is_superset_of = std::conjunction<contains<TestTypes>...>;
    template <typename... TestTypes>
    using equivalent = std::conjunction<std::bool_constant<sizeof...(TestTypes) == sizeof...(PackArgs)>,
        is_superset_of<TestTypes...>,
        typename pack<TestTypes...>::template is_superset_of<PackArgs...>,
        std::bool_constant<count_of<TestTypes>::value == pack<TestTypes...>::template count_of<TestTypes>::value>...,
        std::bool_constant<count_of<PackArgs>::value == pack<TestTypes...>::template count_of<PackArgs>::value>...>;

    template <size_t NumToRemove>
    using remove_prefix = typename pack_impl::remove_prefix_types<NumToRemove, PackArgs...>::type;

    template <size_t NumToRemove>
    using remove_suffix = typename pack_impl::remove_suffix_types<NumToRemove, PackArgs...>::type;

    template <size_t NewSize>
    using downsize = typename pack_impl::remove_suffix_types<sizeof...(PackArgs) - NewSize, PackArgs...>::type;

    template <size_t StartIndex, size_t Count>
    using subpack = typename remove_prefix<StartIndex>::template downsize<Count>;

    template <template <typename...> typename Filterer>
    using filter = typename pack_impl::filter_helper<Filterer, PackArgs...>::type;

    template <typename ReturnType>
    static constexpr auto resolve_overload(ReturnType (*function_ptr)(PackArgs...)) -> ReturnType (*)(PackArgs...)
    {
        return function_ptr;
    }

    template <typename ReturnType, typename ClassType>
    static constexpr auto resolve_overload(ReturnType (ClassType::*method_ptr)(PackArgs...))
        -> ReturnType (ClassType::*)(PackArgs...)
    {
        return method_ptr;
    }

    static constexpr auto make_tuple(PackArgs... args) -> std::tuple<PackArgs...>
    {
        return std::tuple<PackArgs...>{ std::move(args)... };
    }
};

template <typename ParentType>
using pack_for = typename pack_impl::copy_template_args<pack, ParentType>::type;
} // namespace safet
