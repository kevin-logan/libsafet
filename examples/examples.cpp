#include <safet/safet.hpp>

#include <iostream>
#include <list>
#include <vector>

auto stringify_variant(const safet::variant<std::string, int64_t>& v) -> safet::cow<std::string>
{
    return v.visit([](const auto& val) {
        using type = std::decay_t<decltype(val)>;

        if constexpr (std::is_same<type, std::string>::value) {
            return safet::cow<std::string> { std::cref(val) };
        } else {
            // must be an int, store to_string conversion by value
            return safet::cow<std::string> { std::to_string(val) };
        }
    });
}

auto use_cow() -> void
{
    safet::variant<std::string, int64_t> v { 1337 };
    std::cout << "stringified 1337: " << stringify_variant(v).get_const() << std::endl;
    v.emplace<std::string>("derp");
    auto cow_str = stringify_variant(v);

    v.get<std::string>().if_set([&](const std::string& str) { std::cout << "variant address: " << &str << ", vs cow before change: " << &cow_str.get_const() << std::endl; });

    // now change it
    cow_str.get_mutable() += "_changed";
    v.get<std::string>().if_set([&](const std::string& str) { std::cout << "variant address: " << &str << ", vs cow after change: " << &cow_str.get_const() << std::endl; });

    std::cout << "stringified \"derp\" after change: " << cow_str.get_const() << std::endl;
    v.emplace<int64_t>(10101);
    std::cout << "stringified 10101: " << stringify_variant(v).get_const() << std::endl;
}

auto use_memory() -> void
{
    safet::weak_ptr<int> w_ptr;
    {
        auto u_ptr = safet::make_unique<int>(5);
        auto s_ptr = safet::make_shared<int>(1337);
        w_ptr = s_ptr;

        u_ptr.deref().if_set([](int val) { std::cout << "u_ptr expectedly had value: " << val << std::endl; }).if_unset([]() { std::cout << "u_ptr unexpectedly was nullptr" << std::endl; });
        s_ptr.deref().if_set([](int val) { std::cout << "s_ptr expectedly had value: " << val << std::endl; }).if_unset([]() { std::cout << "s_ptr unexpectedly was nullptr" << std::endl; });
        w_ptr.lock().deref().if_set([](int val) { std::cout << "w_ptr (pre) expectedly had value: " << val << std::endl; }).if_unset([]() { std::cout << "w_ptr (pre) unexpectedly was nullptr" << std::endl; });
    }

    w_ptr.lock().deref().if_set([](int val) { std::cout << "w_ptr (post) unexpectedly had value: " << val << std::endl; }).if_unset([]() { std::cout << "w_ptr (post) expectedly was nullptr" << std::endl; });
}

auto use_critical_section() -> void
{
    safet::critical_section<int> data { 1337 };

    data.enter([&data](int& val) {
        std::cout << "enter expectedly locked first with value: " << val << std::endl;

        // try to lock again, this should fail
        if (!data.try_enter(
                [](int& val_2) {
                    std::cout << "try_enter unexpectedly locked again with value: " << val_2 << std::endl;
                })) {
            std::cout << "try_enter expectedly failed to lock" << std::endl;
        };
    });

    if (!data.try_enter(
            [](int& val) {
                std::cout << "try_enter expectedly locked with value: " << val << std::endl;
            })) {
        std::cout << "try_enter unexpectedly failed to lock" << std::endl;
    }
}

auto use_optional() -> void
{
    safet::optional<int> opt;

    opt.if_set([](int val) { std::cout << "optional had unexpected value: " << val << std::endl; }).if_unset([]() { std::cout << "optional expectedly had no value" << std::endl; });

    opt = 5;

    opt.if_set([](int val) { std::cout << "optional had expected value: " << val << std::endl; }).if_unset([]() { std::cout << "optional unexpectedly had no value" << std::endl; });

    /*
     * safet::optionals can hold references
     * 
     * With the safet::optional assigning an optional always resets the value, so no confusion betwene reassigning the reference
     * vs. assigning the referenced to value.
     * 
     * However type deduction will always favor a value type in an instance like this:
     * 
     * int x{0};
     * optional o{x};
     * 
     * so you always must explicitly specify you want a reference:
     * 
     * int x{0};
     * optional<int&> o{x};
     * 
     * the equals operator will always reassign the reference:
     * 
     * int x{0};
     * optional<int&> o{x};
     * int y{1};
     * o = y; // o now references y, x's value is still 0
     */

    int x { 0 };
    safet::optional o_value { x }; // deduces as a value type:
    o_value.if_set([](auto&& val) { std::cout << "o_value has value: " << val << ", expecting 0" << std::endl; }).if_unset([]() { std::cout << "o_value is unexpectedly empty!" << std::endl; });

    x += 1;
    o_value.if_set([](auto&& val) { std::cout << "o_value has value: " << val << ", expecting 0" << std::endl; }).if_unset([]() { std::cout << "o_value is unexpectedly empty!" << std::endl; });

    safet::optional<int&> o_ref { x }; // forced as a reference
    o_ref.if_set([](auto&& val) { std::cout << "o_value has value: " << val << ", expecting 1" << std::endl; }).if_unset([]() { std::cout << "o_value is unexpectedly empty!" << std::endl; });

    x += 1;
    o_ref.if_set([](auto&& val) { std::cout << "o_value has value: " << val << ", expecting 2" << std::endl; }).if_unset([]() { std::cout << "o_value is unexpectedly empty!" << std::endl; });

    int y { 0 };
    o_ref = y; // o_ref now refers to y
    o_ref.if_set([](auto&& val) { std::cout << "o_value has value: " << val << ", expecting 0" << std::endl; }).if_unset([]() { std::cout << "o_value is unexpectedly empty!" << std::endl; });

    // x is still 2
    if (x == 2) {
        std::cout << "x unchanged with value 2" << std::endl;
    }

    // can check additional conditions and wrap them into the optional
    (o_ref && x == 2).if_set([](auto&& val) { std::cout << "o_ref was set and x was 2 as expected" << std::endl; });
    (o_ref && x == 3).if_unset([]() { std::cout << "either o_ref was unset or x wasn't 3, which is expected" << std::endl; });

    // conditions on optionals give optionals with references (or the original value if used on an r-value)
    (o_value && x == 2).if_set([](auto&& val) { val = 1337; });

    // original o_value has been set to 1337 by the condition above
    o_value.if_set([](auto&& val) { std::cout << "o_value has value: " << val << ", expecting 1337" << std::endl; });

    // you can use value_or to get a guaranteed value
    safet::optional<int> o_empty;
    std::cout << "o_empty value_or = " << o_empty.value_or([]() { return 42069; }) << ", expecting 42069" << std::endl;

    // alternatively you can get a guaranteed value through instantiation:
    int& emplace_value = o_empty.emplace(123);
    std::cout << "o_empty's emplaced value: " << emplace_value << ", expecting 123" << std::endl;

    o_empty = std::nullopt;
    int& emplace_if_empty_value = o_empty.emplace_if_empty([]() { return 321; });
    std::cout << "o_empty's emplace_if_empty value: " << emplace_if_empty_value << ", expecting 321" << std::endl;

    // you can also combine `if_set` and `value_or` to effectively map the optional to another type:
    o_empty = std::nullopt;
    auto mapped_value = o_empty.if_set([](auto&& val) { return std::to_string(val); }).value_or([]() { return std::string { "empty" }; });

    std::cout << "o_empty's 'mapped' value: " << mapped_value << ", expecting \"empty\"" << std::endl;

    // this can also be used to throw an exception, acting like a traditional std::optional, if you really know the optional must be engaged
    // and you can't rework the code to prevent this situation. The syntax is verbose compared to std::optional::value, and given this
    // pattern is exactly the sort of thing safet::optional is trying to avoid, it won't get any first-class improvements
    int o_value_contained = o_value.value_or([]() -> int { throw std::bad_optional_access {}; });
    std::cout << "o_value had value: " << o_value_contained << ", expecting 1337" << std::endl;

    // also this can be used to convert the safet::optional to an std::optional if, for example, an API requires it
    std::optional<int> o_std = std::move(o_value).if_set([](auto val) -> std::optional<int> { return std::move(val); }).value_or([]() -> std::optional<int> { return std::nullopt; });
    if (o_std.has_value()) {
        std::cout << "o_std has value: " << o_std.value() << ", expecting 1337" << std::endl;
    } else {
        std::cout << "o_std has no value, which is unexpected" << std::endl;
    }
}

template <typename P1, typename P2, typename Sequence>
struct PairifyHelper {
};

template <typename P1, typename P2, size_t I>
struct PairifyHelper<P1, P2, std::index_sequence<I>> {
    using type = safet::pack<std::pair<typename P1::template ith_type<I>::type, typename P2::template ith_type<I>::type>>;
};

template <typename P1, typename P2, size_t I1, size_t I2, size_t... Is>
struct PairifyHelper<P1, P2, std::index_sequence<I1, I2, Is...>> {
    using type = typename PairifyHelper<P1, P2, std::index_sequence<I2, Is...>>::type::template apply<safet::pack<std::pair<typename P1::template ith_type<I1>::type, typename P2::template ith_type<I1>::type>>::template append>;
};

template <typename P1, typename P2, typename = void>
struct Pairify {
};

template <typename P1, typename P2>
struct Pairify<P1, P2, std::enable_if_t<P1::size::value == P2::size::value>> {
    using type = typename PairifyHelper<P1, P2, typename P1::index_sequence_for>::type;
};

auto use_pack() -> void
{
    using P1 = safet::pack<int, std::string, double>;
    using P2 = safet::pack<bool, void*, std::vector<std::string>>;

    // normally you cannot use multiple parameter packs in a single struct/function/etc, this allows us to do it by encapsulating each in a safet::pack

    using PairPack = typename Pairify<P1, P2>::type;

    std::cout << "PairPack: " << typeid(PairPack).name() << std::endl;

    using ExpectedPairs = safet::pack<std::pair<int, bool>, std::pair<std::string, void*>, std::pair<double, std::vector<std::string>>>;

    std::cout << "PairPack deduced as expected: " << ExpectedPairs::apply<PairPack::is_equal>::value << std::endl;
}

auto use_variant() -> void
{
    safet::variant<int, std::string, safet::optional<double>> var { 1337 };

    auto copy = var;
    auto move { std::move(copy) };

    move.get<int>().if_set([&var](int val1) {
        var.get<int>().if_set([val1](int val2) {
            if (val1 == val2) {
                std::cout << "both values equal as expected: " << val1 << std::endl;
            } else {
                std::cout << val1 << " was unexpectedly not equal to " << val2 << std::endl;
            } }).if_unset([val1]() { std::cout << "var unexpectedly did not contain an int" << std::endl; });
    });

    var = safet::optional { 13.37 };
    var.get<safet::optional<double>>().if_set(
        [](auto& opt_double) {
            opt_double.if_set([](double val) { std::cout << "var had optional double with value " << val << ", expected 13.37" << std::endl; });
        });
}

auto main(int argc, char* argv[], char* envp[]) -> int
{
    (void)argc;
    (void)argv;
    (void)envp;

    use_cow();
    //use_memory();
    //use_critical_section();
    //use_optional();
    //use_pack();
    use_variant();

    return 0;
}