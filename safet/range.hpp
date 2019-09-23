#pragma once

#include <safet/optional.hpp>
#include <safet/variant.hpp>

#include <algorithm>
#include <iterator>
#include <numeric>

namespace safet {
namespace range_impl {
    // wrapper so references can be stored in an optional/shared_ptr if needed
    template <typename ValueType>
    struct iterator_value_wrapper {
        ValueType m_value;
    };

    template <typename ValueType, typename = void>
    class iterator_value_cache {
    public:
        template <typename InstantiateFunctor>
        auto get_or_instantiate(InstantiateFunctor&& f) const -> ValueType&
        {
            return m_value_ptr.get_or_instantiate([f = std::forward<InstantiateFunctor>(f)]() mutable { return std::make_shared<iterator_value_wrapper<ValueType>>(iterator_value_wrapper<ValueType>{ std::forward<InstantiateFunctor>(f)() }); })->m_value;
        }

        auto emplace(iterator_value_wrapper<ValueType> value_wrapper) -> ValueType&
        {
            return m_value_ptr.emplace(std::make_shared<iterator_value_wrapper<ValueType>>(std::move(value_wrapper)))->m_value;
        }

        auto clear() -> void
        {
            m_value_ptr = std::nullopt;
        }

    private:
        mutable optional<std::shared_ptr<iterator_value_wrapper<ValueType>>> m_value_ptr;
    };

    template <typename ValueType>
    class iterator_value_cache<ValueType,
        std::enable_if_t<std::is_copy_constructible<ValueType>::value && std::is_copy_assignable<ValueType>::value && std::is_move_constructible<ValueType>::value && std::is_move_assignable<ValueType>::value>> {
    public:
        template <typename InstantiateFunctor>
        auto get_or_instantiate(InstantiateFunctor&& f) const -> ValueType&
        {
            return m_value.get_or_instantiate([f = std::forward<InstantiateFunctor>(f)]() mutable { return iterator_value_wrapper<ValueType>{ std::forward<InstantiateFunctor>(f)() }; }).m_value;
        }

        auto emplace(iterator_value_wrapper<ValueType> value_wrapper) -> ValueType&
        {
            return m_value.emplace(std::move(value_wrapper)).m_value;
        }

        auto clear() -> void
        {
            m_value = std::nullopt;
        }

    private:
        mutable optional<iterator_value_wrapper<ValueType>> m_value;
    };
}
template <typename BaseIteratorType, typename Filter>
class filtered_iterator {
public:
    using difference_type = typename std::iterator_traits<BaseIteratorType>::difference_type;
    using value_type = typename std::iterator_traits<BaseIteratorType>::value_type;
    using pointer = typename std::iterator_traits<BaseIteratorType>::pointer;
    using reference = typename std::iterator_traits<BaseIteratorType>::reference;
    using iterator_category = std::forward_iterator_tag; // typename std::iterator_traits<BaseIteratorType>::iterator_category;

    filtered_iterator(BaseIteratorType pos, BaseIteratorType end, Filter filter)
        : m_base_iterator(std::move(pos))
        , m_base_end_iterator(std::move(end))
        , m_filter(std::move(filter))
    {
        // start iterator could need to be skipped
        skip_invalid_items();
    }

    filtered_iterator(const filtered_iterator&) = default;
    filtered_iterator(filtered_iterator&&) = default;

    auto operator=(const filtered_iterator&) -> filtered_iterator& = default;
    auto operator=(filtered_iterator &&) -> filtered_iterator& = default;

    filtered_iterator& operator++()
    {
        ++m_base_iterator;
        skip_invalid_items();
        return *this;
    }

    filtered_iterator operator++(int)
    {
        filtered_iterator copy{ m_base_iterator++, m_base_end_iterator, m_filter };
        skip_invalid_items();
        return copy;
    }

    auto operator==(const filtered_iterator& other) const -> bool
    {
        return m_base_iterator == other.m_base_iterator;
    }
    auto operator!=(const filtered_iterator& other) const -> bool
    {
        return m_base_iterator != other.m_base_iterator;
    }
    auto operator*() const -> reference
    {
        // the iterator_value_cache (m_last_value) will always be set here
        if constexpr (std::is_rvalue_reference_v<reference> || !std::is_reference_v<reference>) {
            return std::move(m_last_value.get_or_instantiate([this]() { return range_impl::iterator_value_wrapper<reference>{ *m_base_iterator }; }));
        } else {
            return m_last_value.get_or_instantiate([this]() { return range_impl::iterator_value_wrapper<reference>{ *m_base_iterator }; });
        }
    }
    auto operator-> () const -> pointer
    {
        return m_base_iterator.operator->();
    }

private:
    auto skip_invalid_items() -> void
    {
        while (m_base_iterator != m_base_end_iterator && !m_filter(m_last_value.emplace(range_impl::iterator_value_wrapper<reference>{ *m_base_iterator }))) {
            ++m_base_iterator;
        }
    }

    BaseIteratorType m_base_iterator;
    BaseIteratorType m_base_end_iterator;
    Filter m_filter;
    range_impl::iterator_value_cache<reference> m_last_value;
};

template <typename BaseIteratorType, typename Mapper>
class mapped_iterator {
public:
    using original_value_type = typename std::iterator_traits<BaseIteratorType>::value_type;

    using difference_type = typename std::iterator_traits<BaseIteratorType>::difference_type;
    using value_type = std::invoke_result_t<Mapper, original_value_type>;
    using pointer = std::add_pointer_t<value_type>;
    using reference = value_type;
    using iterator_category = typename std::iterator_traits<BaseIteratorType>::iterator_category;

    mapped_iterator(BaseIteratorType pos, Mapper mapper)
        : m_base_iterator(std::move(pos))
        , m_mapper(std::move(mapper))
    {
    }

    mapped_iterator(const mapped_iterator&) = default;
    mapped_iterator(mapped_iterator&&) = default;

    auto operator=(const mapped_iterator&) -> mapped_iterator& = default;
    auto operator=(mapped_iterator &&) -> mapped_iterator& = default;

    mapped_iterator& operator++()
    {
        ++m_base_iterator;
        m_mapped_value.clear();
        return *this;
    }

    mapped_iterator operator++(int)
    {
        mapped_iterator copy{ m_base_iterator++, m_mapper };
        m_mapped_value.clear();
        return copy;
    }

    auto operator==(const mapped_iterator& other) const -> bool
    {
        return m_base_iterator == other.m_base_iterator;
    }
    auto operator!=(const mapped_iterator& other) const -> bool
    {
        return m_base_iterator != other.m_base_iterator;
    }
    auto operator*() const -> reference
    {
        if constexpr (std::is_rvalue_reference_v<reference> || !std::is_reference_v<reference>) {
            return std::move(m_mapped_value.get_or_instantiate([this]() { return range_impl::iterator_value_wrapper<value_type>{ m_mapper(*m_base_iterator) }; }));
        } else {
            return m_mapped_value.get_or_instantiate([this]() { return range_impl::iterator_value_wrapper<value_type>{ m_mapper(*m_base_iterator) }; });
        }
    }

    auto operator-> () const -> pointer
    {
        // result is a temporary, can't return a pointer, but pointer is defined as void so we needn't return anything
        return &m_mapped_value.get_or_instantiate([this]() { return range_impl::iterator_value_wrapper<value_type>{ m_mapper(*m_base_iterator) }; });
    }

private:
    BaseIteratorType m_base_iterator;
    Mapper m_mapper;
    range_impl::iterator_value_cache<value_type> m_mapped_value;
};

template <typename FirstIterType, typename SecondIterType, typename = void>
class joined_iterator {
public:
    using difference_type = typename std::iterator_traits<FirstIterType>::difference_type;
    using value_type = typename std::iterator_traits<FirstIterType>::value_type;
    using pointer = typename std::iterator_traits<FirstIterType>::pointer;
    using reference = typename std::iterator_traits<FirstIterType>::reference;
    using iterator_category = typename std::iterator_traits<FirstIterType>::iterator_category;

    using difference_type_second = typename std::iterator_traits<SecondIterType>::difference_type;
    using value_type_second = typename std::iterator_traits<SecondIterType>::value_type;
    using pointer_second = typename std::iterator_traits<SecondIterType>::pointer;
    using reference_second = typename std::iterator_traits<SecondIterType>::reference;
    using iterator_category_second = typename std::iterator_traits<SecondIterType>::iterator_category;

    static_assert(std::is_same<difference_type, difference_type_second>::value, "Joined iterators must share the same traits");
    static_assert(std::is_same<value_type, value_type_second>::value, "Joined iterators must share the same traits");
    static_assert(std::is_same<pointer, pointer_second>::value, "Joined iterators must share the same traits");
    static_assert(std::is_same<reference, reference_second>::value, "Joined iterators must share the same traits");
    static_assert(std::is_same<iterator_category, iterator_category_second>::value, "Joined iterators must share the same traits");

    joined_iterator(FirstIterType pos, FirstIterType first_range_end, SecondIterType second_range_begin)
        : m_base_iterator(std::move(pos))
        , m_first_range_end(std::move(first_range_end))
        , m_second_range_begin(std::move(second_range_begin))
    {
    }

    joined_iterator(SecondIterType pos, FirstIterType first_range_end, SecondIterType second_range_begin)
        : m_base_iterator(std::move(pos))
        , m_first_range_end(std::move(first_range_end))
        , m_second_range_begin(std::move(second_range_begin))
    {
    }

    joined_iterator(variant<FirstIterType, SecondIterType> pos_var, FirstIterType first_range_end, SecondIterType second_range_begin)
        : m_base_iterator(std::move(pos_var))
        , m_first_range_end(std::move(first_range_end))
        , m_second_range_begin(std::move(second_range_begin))
    {
    }

    joined_iterator(const joined_iterator&) = default;
    joined_iterator(joined_iterator&&) = default;

    auto operator=(const joined_iterator&) -> joined_iterator& = default;
    auto operator=(joined_iterator &&) -> joined_iterator& = default;

    joined_iterator& operator++()
    {
        m_base_iterator.visit([](auto& iter) { ++iter; });
        reconcile();
        return *this;
    }

    joined_iterator operator++(int)
    {
        auto copy = *this;
        m_base_iterator.visit([](auto& iter) { ++iter; });
        reconcile();
        return copy;
    }

    auto operator==(const joined_iterator& other) const -> bool
    {
        return m_base_iterator.visit([&other](const auto& ours) {
            return other.m_base_iterator.template if_set_else<std::decay_t<decltype(ours)>>([&ours](const auto& theirs) { return ours == theirs; }, []() { return false; });
        });
    }
    auto operator!=(const joined_iterator& other) const -> bool
    {
        return m_base_iterator.visit([&other](const auto& ours) {
            return other.m_base_iterator.template if_set_else<std::decay_t<decltype(ours)>>([&ours](const auto& theirs) { return ours != theirs; }, []() { return true; });
        });
    }
    auto operator*() const -> reference
    {
        if constexpr (std::is_rvalue_reference_v<reference> || !std::is_reference_v<reference>) {
            return std::move(m_last_value.get_or_instantiate([this]() { return m_base_iterator.visit([](auto& iter) { return range_impl::iterator_value_wrapper<value_type>{ *iter }; }); }));
        } else {
            return m_last_value.get_or_instantiate([this]() { return m_base_iterator.visit([](auto& iter) { return range_impl::iterator_value_wrapper<value_type>{ *iter }; }); });
        }
    }
    auto operator-> () const -> pointer
    {
        return &m_last_value.get_or_instantiate([this]() { return m_base_iterator.visit([](auto& iter) { return range_impl::iterator_value_wrapper<value_type>{ *iter }; }); });
    }

private:
    auto reconcile() -> void
    {
        m_last_value.clear();
        m_base_iterator.template if_set<FirstIterType>([this](const FirstIterType& pos) { if (pos == m_first_range_end) { m_base_iterator.template emplace<SecondIterType>(m_second_range_begin);} });
    }

    variant<FirstIterType, SecondIterType> m_base_iterator;
    FirstIterType m_first_range_end;
    SecondIterType m_second_range_begin;
    range_impl::iterator_value_cache<value_type> m_last_value;
};

template <typename FirstIterType, typename SecondIterType>
class joined_iterator<FirstIterType, SecondIterType, std::enable_if_t<std::is_same<FirstIterType, SecondIterType>::value>> {
public:
    using difference_type = typename std::iterator_traits<FirstIterType>::difference_type;
    using value_type = typename std::iterator_traits<FirstIterType>::value_type;
    using pointer = typename std::iterator_traits<FirstIterType>::pointer;
    using reference = typename std::iterator_traits<FirstIterType>::reference;
    using iterator_category = typename std::iterator_traits<FirstIterType>::iterator_category;

    joined_iterator(FirstIterType pos, optional<FirstIterType> first_range_end, SecondIterType second_range_begin)
        : m_base_iterator(std::move(pos))
        , m_first_range_end(std::move(first_range_end))
        , m_second_range_begin(std::move(second_range_begin))
    {
    }

    joined_iterator(variant<FirstIterType, SecondIterType> pos_var, FirstIterType first_range_end, SecondIterType second_range_begin)
        : m_base_iterator(std::move(pos_var))
        , m_first_range_end(std::move(first_range_end))
        , m_second_range_begin(std::move(second_range_begin))
    {
    }

    joined_iterator(const joined_iterator&) = default;
    joined_iterator(joined_iterator&&) = default;

    auto operator=(const joined_iterator&) -> joined_iterator& = default;
    auto operator=(joined_iterator &&) -> joined_iterator& = default;

    joined_iterator& operator++()
    {
        ++m_base_iterator;
        reconcile();
        return *this;
    }

    joined_iterator operator++(int)
    {
        joined_iterator copy{ m_base_iterator++, m_first_range_end, m_second_range_begin };
        reconcile();
        return copy;
    }

    auto operator==(const joined_iterator& other) const -> bool
    {
        return m_base_iterator == other.m_base_iterator;
    }
    auto operator!=(const joined_iterator& other) const -> bool
    {
        return m_base_iterator != other.m_base_iterator;
    }
    auto operator*() const -> reference
    {
        if constexpr (std::is_rvalue_reference_v<reference> || !std::is_reference_v<reference>) {
            return std::move(m_last_value.get_or_instantiate([this]() { return range_impl::iterator_value_wrapper<value_type>{ *m_base_iterator }; }));
        } else {
            return m_last_value.get_or_instantiate([this]() { return range_impl::iterator_value_wrapper<value_type>{ *m_base_iterator }; });
        }
    }
    auto operator-> () const -> pointer
    {
        return &m_last_value.get_or_instantiate([this]() { return range_impl::iterator_value_wrapper<value_type>{ *m_base_iterator }; });
    }

private:
    auto reconcile() -> void
    {
        m_last_value.clear();
        m_first_range_end.if_set([this](const FirstIterType& first_range_end) {
            if (m_base_iterator == first_range_end) {
                m_base_iterator = m_second_range_begin;
                m_first_range_end = std::nullopt;
            }
        });
    }

    FirstIterType m_base_iterator;
    // if both iterators are the same type, they could refer to the same range and have the same end, only go to m_second_range_begin once
    optional<FirstIterType> m_first_range_end;
    SecondIterType m_second_range_begin;
    range_impl::iterator_value_cache<value_type> m_last_value;
};

template <typename ContainerType, typename = void>
struct IteratorTypeFor {
};

template <typename ContainerType>
struct IteratorTypeFor<const ContainerType&, std::void_t<typename ContainerType::iterator>> {
    using type = typename ContainerType::const_iterator;
};
template <typename ContainerType>
struct IteratorTypeFor<ContainerType&, std::void_t<typename ContainerType::iterator>> {
    using type = typename ContainerType::iterator;
};
template <typename ContainerType>
struct IteratorTypeFor<ContainerType, std::void_t<typename ContainerType::iterator>> {
    using type = std::move_iterator<typename ContainerType::iterator>;
};

template <typename IteratorType>
class range {
public:
    using iterator = IteratorType;
    class const_iterator {
    public:
        using difference_type = typename std::iterator_traits<iterator>::difference_type;
        using value_type = typename std::iterator_traits<iterator>::value_type;
        using pointer = std::add_pointer_t<std::add_const_t<std::remove_pointer_t<typename std::iterator_traits<iterator>::pointer>>>;
        using reference = std::add_const_t<std::remove_reference_t<typename std::iterator_traits<iterator>::reference>>&;
        using iterator_category = typename std::iterator_traits<iterator>::iterator_category;

        const_iterator(iterator base_iterator)
            : m_base_iterator(std::move(base_iterator))
        {
        }

        const_iterator(const const_iterator&) = default;
        const_iterator(const_iterator&&) = default;

        auto operator=(const const_iterator&) -> const_iterator& = default;
        auto operator=(const_iterator &&) -> const_iterator& = default;

        const_iterator& operator++()
        {
            ++m_base_iterator;
            return *this;
        }

        const_iterator operator++(int)
        {
            const_iterator copy{ m_base_iterator++ };
            return copy;
        }

        auto operator==(const const_iterator& other) const -> bool
        {
            return m_base_iterator == other.m_base_iterator;
        }
        auto operator!=(const const_iterator& other) const -> bool
        {
            return m_base_iterator != other.m_base_iterator;
        }
        auto operator*() const -> reference
        {
            return *m_base_iterator;
        }
        auto operator-> () const -> pointer
        {
            return m_base_iterator.operator->();
        }

    private:
        iterator m_base_iterator;
    };

    template <typename ContainerType>
    range(ContainerType&& container)
        : m_begin(typename IteratorTypeFor<ContainerType>::type{ container.begin() })
        , m_end(typename IteratorTypeFor<ContainerType>::type{ container.end() })
    {
    }

    range(IteratorType begin, IteratorType end)
        : m_begin(std::move(begin))
        , m_end(std::move(end))
    {
    }

    range(const range&) = default;
    range(range&&) = default;

    auto operator=(const range&) -> range& = default;
    auto operator=(range &&) -> range& = default;

    auto begin() const& -> const_iterator { return const_iterator{ m_begin }; }
    auto begin() & -> iterator { return m_begin; }
    auto begin() && -> iterator { return std::move(m_begin); }
    auto end() const& -> const_iterator { return const_iterator{ m_end }; }
    auto end() & -> iterator { return m_end; }
    auto end() && -> iterator { return std::move(m_end); }

    auto empty() const -> bool
    {
        return m_begin == m_end;
    }

    template <typename FilterFunctor>
    auto filter(FilterFunctor&& f) & -> range<filtered_iterator<iterator, FilterFunctor>>
    {
        return range<filtered_iterator<iterator, FilterFunctor>>{
            filtered_iterator<iterator, FilterFunctor>{ m_begin, m_end, f },
            filtered_iterator<iterator, FilterFunctor>{ m_end, m_end, f }
        };
    }
    template <typename FilterFunctor>
    auto filter(FilterFunctor&& f) && -> range<filtered_iterator<iterator, FilterFunctor>>
    {
        return range<filtered_iterator<iterator, FilterFunctor>>{
            filtered_iterator<iterator, FilterFunctor>{ std::move(m_begin), m_end, f },
            filtered_iterator<iterator, FilterFunctor>{ std::move(m_end), m_end, f }
        };
    }
    template <typename FilterFunctor>
    auto filter(FilterFunctor&& f) const& -> range<filtered_iterator<const_iterator, FilterFunctor>>
    {
        return range<filtered_iterator<const_iterator, FilterFunctor>>{
            filtered_iterator<const_iterator, FilterFunctor>{ const_iterator{ m_begin }, const_iterator{ m_end }, f },
            filtered_iterator<const_iterator, FilterFunctor>{ const_iterator{ m_end }, const_iterator{ m_end }, f }
        };
    }

    template <typename MapperFunctor>
    auto map(MapperFunctor&& f) & -> range<mapped_iterator<iterator, MapperFunctor>>
    {
        return range<mapped_iterator<iterator, MapperFunctor>>{
            mapped_iterator<iterator, MapperFunctor>{ m_begin, f },
            mapped_iterator<iterator, MapperFunctor>{ m_end, f }
        };
    }
    template <typename MapperFunctor>
    auto map(MapperFunctor&& f) && -> range<mapped_iterator<iterator, MapperFunctor>>
    {
        return range<mapped_iterator<iterator, MapperFunctor>>{
            mapped_iterator<iterator, MapperFunctor>{ std::move(m_begin), f },
            mapped_iterator<iterator, MapperFunctor>{ std::move(m_end), f }
        };
    }
    template <typename MapperFunctor>
    auto map(MapperFunctor&& f) const& -> range<mapped_iterator<const_iterator, MapperFunctor>>
    {
        return range<mapped_iterator<const_iterator, MapperFunctor>>{
            mapped_iterator<const_iterator, MapperFunctor>{ const_iterator{ m_begin }, f },
            mapped_iterator<const_iterator, MapperFunctor>{ const_iterator{ m_end }, f }
        };
    }

    template <template <typename...> typename CollectionType>
    auto collect() & -> CollectionType<typename std::iterator_traits<iterator>::value_type>
    {
        return CollectionType<typename std::iterator_traits<iterator>::value_type>{
            m_begin, m_end
        };
    }
    template <template <typename...> typename CollectionType>
    auto collect() && -> CollectionType<typename std::iterator_traits<iterator>::value_type>
    {
        return CollectionType<typename std::iterator_traits<iterator>::value_type>{
            std::move(m_begin), std::move(m_end)
        };
    }
    template <template <typename...> typename CollectionType>
    auto collect() const& -> CollectionType<typename std::iterator_traits<const_iterator>::value_type>
    {
        return CollectionType<typename std::iterator_traits<const_iterator>::value_type>{
            const_iterator{ m_begin }, const_iterator{ m_end }
        };
    }

    template <typename Functor>
    auto each(Functor&& f) & -> void
    {
        std::for_each(m_begin, m_end, std::forward<Functor>(f));
    }
    template <typename Functor>
    auto each(Functor&& f) && -> void
    {
        std::for_each(std::move(m_begin), std::move(m_end), std::forward<Functor>(f));
    }
    template <typename Functor>
    auto each(Functor&& f) const& -> void
    {
        std::for_each(const_iterator{ m_begin }, const_iterator{ m_end }, std::forward<Functor>(f));
    }

    template <typename OtherRange>
    auto join(OtherRange&& other) & -> range<joined_iterator<iterator, typename OtherRange::iterator>>
    {
        return range<joined_iterator<iterator, typename OtherRange::iterator>>{
            joined_iterator<iterator, typename OtherRange::iterator>{ m_begin, m_end, other.begin() },
            joined_iterator<iterator, typename OtherRange::iterator>{ other.end(), m_end, other.begin() }
        };
    }
    template <typename OtherRange>
    auto join(OtherRange&& other) && -> range<joined_iterator<iterator, typename OtherRange::iterator>>
    {
        return range<joined_iterator<iterator, typename OtherRange::iterator>>{
            joined_iterator<iterator, typename OtherRange::iterator>{ std::move(m_begin), m_end, other.begin() },
            joined_iterator<iterator, typename OtherRange::iterator>{ other.end(), std::move(m_end), other.begin() }
        };
    }
    template <typename OtherRange>
    auto join(const OtherRange& other) const& -> range<joined_iterator<iterator, typename OtherRange::iterator>>
    {
        using other_const_iterator = typename OtherRange::const_iterator;
        return range<joined_iterator<iterator, typename OtherRange::iterator>>{
            joined_iterator<iterator, typename OtherRange::iterator>{ const_iterator{ m_begin }, const_iterator{ m_end }, other.begin() },
            joined_iterator<iterator, typename OtherRange::iterator>{ other.end(), const_iterator{ m_end }, other.begin() }
        };
    }

    template <typename Functor, typename ValueType>
    auto fold(Functor&& f, ValueType start_value) & -> ValueType
    {
        return std::accumulate(m_begin, m_end, std::move(start_value), std::forward<Functor>(f));
    }
    template <typename Functor, typename ValueType>
    auto fold(Functor&& f, ValueType start_value) && -> ValueType
    {
        return std::accumulate(std::move(m_begin), std::move(m_end), std::move(start_value), std::forward<Functor>(f));
    }
    template <typename Functor, typename ValueType>
    auto fold(Functor&& f, ValueType start_value) const& -> ValueType
    {
        return std::accumulate(const_iterator{ m_begin }, const_iterator{ m_end }, std::move(start_value), std::forward<Functor>(f));
    }

private:
    iterator m_begin;
    iterator m_end;
};

template <typename ContainerType>
range(ContainerType &&)->range<typename IteratorTypeFor<ContainerType>::type>;
}
