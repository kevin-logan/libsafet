#pragma once

#include <safet/optional.hpp>

#include <algorithm>
#include <iterator>

namespace safet {
template <typename BaseIteratorType, typename Filter>
class filtered_iterator {
public:
    using difference_type = typename std::iterator_traits<BaseIteratorType>::difference_type;
    using value_type = typename std::iterator_traits<BaseIteratorType>::value_type;
    using pointer = typename std::iterator_traits<BaseIteratorType>::pointer;
    using reference = typename std::iterator_traits<BaseIteratorType>::reference;
    using iterator_category = typename std::iterator_traits<BaseIteratorType>::iterator_category;

    struct Wrapper {
        reference m_value;
    };

    filtered_iterator(BaseIteratorType pos, const BaseIteratorType& end, Filter filter)
        : m_base_iterator(std::move(pos))
        , m_base_end_iterator(std::move(end))
        , m_filter(std::move(filter))
    {
        // start iterator could need to be skipped
        skip_invalid_items();
    }

    filtered_iterator(const filtered_iterator& copy) = default;
    filtered_iterator(filtered_iterator&&) = default;

    auto operator=(const filtered_iterator& copy) -> filtered_iterator& = default;
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
        if constexpr (std::is_rvalue_reference_v<reference> || !std::is_reference_v<reference>) {
            return std::move(m_last_value.get_or_instantiate([this]() -> Wrapper { throw - 1; }).m_value);
        } else {
            return m_last_value.get_or_instantiate([this]() -> Wrapper { throw - 1; }).m_value;
        }
    }
    auto operator-> () const -> pointer
    {
        return m_base_iterator.operator->();
    }

private:
    auto skip_invalid_items() -> void
    {
        while (m_base_iterator != m_base_end_iterator && !m_filter(m_last_value.emplace(Wrapper{ *m_base_iterator }).m_value)) {
            ++m_base_iterator;
        }
    }

    BaseIteratorType m_base_iterator;
    const BaseIteratorType& m_base_end_iterator;
    Filter m_filter;
    mutable optional<Wrapper> m_last_value;
};

template <typename BaseIteratorType, typename Mapper>
class mapped_iterator {
public:
    using original_value_type = typename std::iterator_traits<BaseIteratorType>::value_type;

    using difference_type = typename std::iterator_traits<BaseIteratorType>::difference_type;
    using value_type = std::invoke_result_t<Mapper, original_value_type>;
    using pointer = void; //std::add_pointer_t<value_type>;
    using reference = value_type;
    using iterator_category = typename std::iterator_traits<BaseIteratorType>::iterator_category;

    //using pointer = std::add_pointer_t<value_type>;
    //using reference = value_type&;

    mapped_iterator(BaseIteratorType pos, Mapper mapper)
        : m_base_iterator(std::move(pos))
        , m_mapper(std::move(mapper))
    {
    }

    mapped_iterator(const mapped_iterator& copy) = default;
    mapped_iterator(mapped_iterator&&) = default;

    auto operator=(const mapped_iterator& copy) -> mapped_iterator& = default;
    auto operator=(mapped_iterator &&) -> mapped_iterator& = default;

    mapped_iterator& operator++()
    {
        ++m_base_iterator;
        m_mapped_value = std::nullopt;
        return *this;
    }

    mapped_iterator operator++(int)
    {
        mapped_iterator copy{ m_base_iterator++, m_mapper };
        m_mapped_value = std::nullopt;
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
            return std::move(m_mapped_value.get_or_instantiate([this]() { return m_mapper(*m_base_iterator); }));
        } else {
            return m_mapped_value.get_or_instantiate([this]() { return m_mapper(*m_base_iterator); });
        }
    }

    auto operator-> () const -> pointer
    {
        // result is a temporary, can't return a pointer, but pointer is defined as void so we needn't return anything
        return &m_mapped_value.get_or_instantiate([this]() { return m_mapper(*m_base_iterator); });
    }

private:
    BaseIteratorType m_base_iterator;
    Mapper m_mapper; // reference wrapper so we can use operator =
    mutable optional<value_type> m_mapped_value;
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

private:
    IteratorType m_begin;
    IteratorType m_end;
};

template <typename ContainerType>
range(ContainerType &&)->range<typename IteratorTypeFor<ContainerType>::type>;
}
