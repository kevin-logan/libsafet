#include <catch2/catch.hpp>

#include <safet/finally.hpp>
#include <safet/optional.hpp>

using namespace safet;

TEST_CASE("optional example use cases", "[optional]")
{
    optional<int> o { 0 };
    optional<int> o_empty;

    SECTION("use value_or to get a guaranteed value")
    {
        int value = o_empty.value_or([]() { return 1; });

        REQUIRE(value == 1);
    }

    SECTION("get a guaranteed value through instantiation")
    {
        SECTION("using emplace")
        {
            int& value = o_empty.emplace(1);

            REQUIRE(value == 1);
        }

        SECTION("using emplace_if_empty")
        {
            int& value = o_empty.emplace_if_empty([]() { return 1; });

            REQUIRE(value == 1);
        }
    }

    SECTION("combine if_set and value_or to map the optional to another type")
    {
        auto s = o.if_set([](int value) { return std::to_string(value); }).value_or([]() { return std::string { "empty" }; });
        auto s_empty = o_empty.if_set([](int value) { return std::to_string(value); }).value_or([]() { return std::string { "empty" }; });

        REQUIRE(s == "0");
        REQUIRE(s_empty == "empty");

        SECTION("this can also be used to map a safet::optional into an std::optional")
        {
            std::optional<int> std_o = std::move(o).if_set([](int value) -> std::optional<int> { return { std::move(value) }; }).value_or([]() -> std::optional<int> { return std::nullopt; });

            REQUIRE(std_o.has_value());
            REQUIRE(std_o.value() == 0);
        }
    }

    SECTION("use value_or and exceptions to act like an std::optional")
    {
        // if you really know the optional must be engaged and you can't rework the code to prevent this situation.
        // The syntax is verbose compared to std::optional::value, and given this pattern is exactly the sort of
        // thing safet::optional is trying to avoid, it won't get any first-class improvements
        try {
            int value = o.value_or([]() -> int { throw std::bad_optional_access {}; });

            REQUIRE(value == 0);
        } catch (...) {
            FAIL("o must be engaged, so no exception should be raised");
        }
    }
}

TEST_CASE("optional storing references", "[optional]")
{
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

    // show type deduction prefers value types:
    int x { 0 };
    optional o { x };

    REQUIRE(std::is_same<decltype(o), optional<int>>::value);
    // prove the memory location of the stored value is different than that of x, because o was deduced as optional<int>, not optional<int&>
    o.if_set([&](int& value) { REQUIRE(&value != &x); }).if_unset([]() { FAIL("optional explicitly initialized with a value must be engaged"); });

    optional<int&> o_ref { x };
    o_ref.if_set([&](int& value) { REQUIRE(&value == &x); }).if_unset([]() { FAIL("optional explicitly initialized with a value must be engaged"); });

    // a (or the?) reason std::optional doesn't support references is the question of reassignment. Should it rebind the reference or
    // should the value referenced be reassigned? safet::optional has chosen the prior, as assigning a safet::optional will always
    // result in a 'new' value being stored in the optional, rather than forwarding that assignment to the contained value.
    int y { 1 };
    o_ref = y;
    o_ref.if_set([&](int& value) { REQUIRE(&value == &y); }).if_unset([]() { FAIL("optional explicitly initialized with a value must be engaged"); });

    o_ref.if_set([](int& value) { value = 2; });

    // ensure changing the optional's value changes the variable to which it references
    REQUIRE(y == 2);

    // ensure x never changed as we rebound the reference before changing anything
    REQUIRE(x == 0);
}

TEST_CASE("optional constructors", "[optional]")
{
    // default constructs to empty
    {
        optional<int> o;
        REQUIRE(o.empty());
    }

    // constructed with value, including type deduction
    {
        optional o { 1 };

        // value must be set and equal to 1
        o.if_set([](int value) { REQUIRE(value == 1); }).if_unset([]() { FAIL("if_unset must not be called on an engaged optional"); });
    }

    // construct in-place
    {
        optional<std::string> o { std::in_place_t {}, 5u, 'z' };

        // value must be set and equal to "zzzzz"
        o.if_set([](std::string& value) { REQUIRE(value == "zzzzz"); }).if_unset([]() { FAIL("if_unset must not be called on an engaged optional"); });
    }

    // construct as explicitly nullopt
    {
        optional<std::string> o { std::nullopt };
        REQUIRE(o.empty());
    }

    // copy constructor
    {
        optional<std::string> o { std::in_place_t {}, 5u, 'z' };
        optional copy { o };

        // value must be set and equal to "zzzzz"
        copy.if_set([](std::string& value) { REQUIRE(value == "zzzzz"); }).if_unset([]() { FAIL("if_unset must not be called on an engaged optional"); });

        // and copied-from value should be untouched
        o.if_set([](std::string& value) { REQUIRE(value == "zzzzz"); }).if_unset([]() { FAIL("if_unset must not be called on an engaged optional"); });

        // copying an empty optional gives you an empty optional
        optional<int> empty;
        optional empty_copy { empty };

        REQUIRE(empty.empty());
        REQUIRE(empty_copy.empty());
    }

    // move constructor
    {
        optional<std::unique_ptr<std::string>> o { std::make_unique<std::string>(5u, 'z') };
        optional move { std::move(o) };

        // value must be set and equal to "zzzzz"
        move.if_set([](std::unique_ptr<std::string>& value) { REQUIRE(*value == "zzzzz"); }).if_unset([]() { FAIL("if_unset must not be called on an engaged optional"); });

        // and moved-from value should be moved from, but still engaged
        o.if_set([](std::unique_ptr<std::string>& value) { REQUIRE(value == nullptr); }).if_unset([]() { FAIL("if_unset must not be called on an engaged optional"); });

        // copying an empty optional gives you an empty optional
        optional<int> empty;
        optional empty_move { std::move(empty) };

        REQUIRE(empty.empty()); // moved from will still report as disengaged
        REQUIRE(empty_move.empty());
    }
}

TEST_CASE("optional destructor", "[optional]")
{
    bool destructed = false;
    {
        optional o { finally { [&]() { destructed = true; } } };

        // still in scope
        REQUIRE_FALSE(destructed);
    }

    // o out of scope, should have destructed
    REQUIRE(destructed);

    size_t destruct_count { 0 };
    {
        finally destruction_counter { [&]() { ++destruct_count; } };

        optional o { destruction_counter };

        // no destructions yet
        REQUIRE(destruct_count == 0);

        // makes another copy of the counter, original should destruct
        o = destruction_counter;
        REQUIRE(destruct_count == 1);

        // makes yet another copy
        o.emplace(destruction_counter);
        REQUIRE(destruct_count == 2);

        // makes yet another copy
        {
            optional o2 { o };
            o = o2; // destruct on copying value from o2
            REQUIRE(destruct_count == 3);
        }

        // o2 has destructed
        REQUIRE(destruct_count == 4);

        // reset to nullopt, destructs current value
        o = std::nullopt;
        REQUIRE(destruct_count == 5);
    }

    // last value was nullopt, no additional destruction from o
    // however destruction_counter will trigger once more when the
    // original finally leaves scope
    REQUIRE(destruct_count == 6);
}

TEST_CASE("optional assignment", "[optional]")
{
    optional<std::string> s;

    s = "Hello, World!";
    s.if_set([](const std::string& val) { REQUIRE(val == "Hello, World!"); }).if_unset([]() { FAIL("if_unset must not be called on an engaged optional"); });

    optional<std::string> s2;

    s2 = s;
    s2.if_set([](const std::string& val) { REQUIRE(val == "Hello, World!"); }).if_unset([]() { FAIL("if_unset must not be called on an engaged optional"); });

    s2 = std::move(s);
    s2.if_set([](const std::string& val) { REQUIRE(val == "Hello, World!"); }).if_unset([]() { FAIL("if_unset must not be called on an engaged optional"); });

    s2 = std::nullopt;
    REQUIRE(s2.empty());
}

TEST_CASE("optional::operator&&", "[optional]")
{
    // && operator on an optional 'forwards' the value of that optional
    // into a new optional, which is only engaged if the condition was true
    optional<std::string> s { "1" };

    // make sure the value is properly set
    (s && true).if_set([](std::string& value) { REQUIRE(value == "1"); }).if_unset([]() { FAIL("if_unset must not be called on an engaged optional"); });

    // because the value is 'forwarded', if used on an l-value we should have an optional
    // l-value reference if the condition is true:
    (s && true).if_set([](std::string& value) { value = "2"; });

    // so original s should be "2" now
    REQUIRE(s == "2");

    optional unique_s { std::make_unique<std::string>("3") };

    // values can be moved out with the operator
    (std::move(unique_s) && true).if_set([](std::unique_ptr<std::string> value) { REQUIRE(*value == "3"); }).if_unset([]() { FAIL("if_unset must not be called on an engaged optional"); });

    // false condition should always result in disengaged optional
    REQUIRE((s && false).empty());
}

TEST_CASE("optional::if_set()", "[optional]")
{
    optional o { std::make_unique<std::string>("1") };

    auto& result = o.if_set([](auto& value_ptr) { REQUIRE(*value_ptr == "1"); });

    // if_set returns an optional - either the return value of the functor
    // or, if the functor returns void, it will return itself
    REQUIRE(&result == &o);

    // move semantics allow us to move the value out
    auto new_o = std::move(o).if_set([](auto value_ptr) { return std::make_unique<std::string>(*value_ptr + "_2"); });

    // o must have been set so new_o also
    REQUIRE_FALSE(new_o.empty());
    new_o.if_set([](auto& value_ptr) { REQUIRE(*value_ptr == "1_2"); });

    // functor must not be called if the optional is disengaged
    o = std::nullopt;
    o.if_set([](auto&&) { FAIL("if_set must not be called on a disengaged optional"); });

    SECTION("mutation of value via generic lambda")
    {
        optional i { 1 };

        i.if_set([](auto& val) { val = 2; });
        REQUIRE(i == 2);
        auto i2 = i.and_then([](auto& val) {
            val = 3;
            return optional { 4 }; });
        REQUIRE(i == 3);
        REQUIRE(i2 == 4);
    }

    SECTION("argument forwarding")
    {
        optional o { std::make_unique<std::string>("1") };
        auto test_argument = 2;

        auto& result = o.if_set([](auto& value_ptr, auto& test_argument) 
        { 
            REQUIRE(*value_ptr == "1");
            REQUIRE(test_argument == 2); 
        }, test_argument);

        // if_set returns an optional - either the return value of the functor
        // or, if the functor returns void, it will return itself
        REQUIRE(&result == &o);

        // move semantics allow us to move the value out
        auto new_o = std::move(o).if_set([](auto value_ptr, auto& test_argument) 
        { 
            REQUIRE(test_argument == 2);
            return std::make_unique<std::string>(*value_ptr + "_2"); 
        }, test_argument);

        // o must have been set so new_o also
        REQUIRE_FALSE(new_o.empty());
        new_o.if_set([](auto& value_ptr, auto& test_argument) 
        { 
            REQUIRE(*value_ptr == "1_2"); 
            REQUIRE(test_argument == 2);
        }, test_argument);

        // check the const method of if_set works, also check that we can pass move only parameters
        const optional const_o { 4 };
        const auto& new_const_o = const_o.if_set([](auto& value, auto test_pointer)
        {
            REQUIRE(value == 4);
            REQUIRE(*test_pointer == 10);
            return value;
        }, std::move(std::make_unique<int>(10)));
        REQUIRE_FALSE(new_const_o.empty());

    }

    SECTION("mutation of value via generic lambda and argument forwarding")
    {
        // Mutation with argument forwarding
        optional i { 1 };
        auto test_argument = 2;

        i.if_set([](auto& val, auto& test_argument) { val += test_argument; }, test_argument) ;
        REQUIRE(i == 3);
        auto i2 = i.and_then([](auto& val, auto& test_argument) {
            val += test_argument;
            return optional { 4 }; }, test_argument);
        REQUIRE(i == 5);
        REQUIRE(i2 == 4);
    }

}

TEST_CASE("optional::if_unset()", "[optional]")
{
    optional o { 1 };

    // functor must not be called if optional is engaged
    auto& self_result = o.if_unset([]() { FAIL("if_unset must not be called on an engaged optional"); });

    // functor which returns nothing then if_unset returns self
    REQUIRE(&self_result == &o);

    // functor must be called if optional is disengaged
    o = std::nullopt;
    auto result = o.if_unset([]() { return 2; });

    // result must be set now, and should be 2
    REQUIRE(result == 2);

    SECTION("argument forwarding")
    {
        optional o { 1 };
        auto test_argument = 2;

        // functor must be called if optional is disengaged
        o = std::nullopt;
        auto result = o.if_unset([](auto& test_argument) { return 2 + test_argument; }, test_argument);
        REQUIRE(result == 4);
    }
}

TEST_CASE("optional::value_or()", "[optional]")
{
    optional o { 1 };

    // optional is engaged, value must be engaged value
    REQUIRE(o.value_or([]() { return 2; }) == 1);

    // functor must not be called if optional is engaged
    o.value_or([]() { FAIL("value_or must not be called on an engaged optional"); return 2; });

    // optional is disengaged, value must be what is returned from the functor
    o = std::nullopt;
    REQUIRE(o.value_or([]() { return 2; }) == 2);

    optional move_only { std::make_unique<int>(3) };

    // move semantics work
    std::unique_ptr<int> ptr_out = std::move(move_only).value_or([]() { return std::make_unique<int>(4); });

    REQUIRE(*ptr_out == 3);

    SECTION("argument forwarding")
    {
        optional o { 1 };
        auto test_argument = 2;

        // optional is engaged, value must be engaged value
        REQUIRE(o.value_or([](auto& test_argument) { return test_argument; }, test_argument)  == 1);

        // optional is disengaged, value must be what is returned from the functor
        o = std::nullopt;
        REQUIRE(o.value_or([](auto& test_argument) { return test_argument; }, test_argument) == 2);

        optional move_only { std::make_unique<int>(3) };

        // move semantics work
        std::unique_ptr<int> ptr_out = std::move(move_only).value_or([](auto& test_argument) { return std::make_unique<int>(4); }, test_argument);

        REQUIRE(*ptr_out == 3);
    }
}

TEST_CASE("optional::and_then()", "[optional]")
{
    // converts a string into a number from 0-9, or an empty optional if the
    // string wasn't a number from 0-9
    auto string_to_digit = [](const std::string& str) -> optional<int> {
        if (str.size() == 1 && str[0] >= '0' && str[0] <= '9') {
            return str[0] - '0';
        } else {
            return std::nullopt;
        }
    };

    optional<std::string> s;

    // and_then on an empty optional will always return an empty optional
    // of the functor's result's type
    optional<int> i = s.and_then(string_to_digit);
    REQUIRE(i.empty());

    // empty optional if the functor returns nullopt
    s = "not a number";
    i = s.and_then(string_to_digit);
    REQUIRE(i.empty());

    // "5" will succeed in string_to_digit, so the conversion must succeed
    s = "5";
    i = s.and_then(string_to_digit);
    REQUIRE(i == 5);

    optional move_only { std::make_unique<std::string>("3") };

    // move semantics work
    i = std::move(move_only).and_then([&](auto value_ptr) { return string_to_digit(*value_ptr); });
    REQUIRE(i == 3);

    SECTION("argument forwarding")
    {
        // converts a string into a number from 0-9, or an empty optional if the
        // string wasn't a number from 0-9 then adds the second int value to the string value.
        auto sum_string_and_digit = [](const std::string& str, const int& value) -> optional<int> {
            if (str.size() == 1 && str[0] >= '0' && str[0] <= '9') {
                return (str[0] - '0') + value;
            } else {
                return std::nullopt;
            }
        };

        optional<std::string> s;
        int test_argument = 2;
        // and_then on an empty optional will always return an empty optional
        // of the functor's result's type
        optional<int> i = s.and_then([&](auto value, auto& test_argument) { return sum_string_and_digit(value, test_argument); }, test_argument);
        REQUIRE(i.empty());

        // empty optional if the functor returns nullopt
        s = "not a number";
        i = s.and_then([&](auto value, auto& test_argument) { return sum_string_and_digit(value, test_argument); }, test_argument);
        REQUIRE(i.empty());

        // "5" will succeed in string_to_digit, so the conversion must succeed
        s = "5";
        i = s.and_then([&](auto value, auto& test_argument) { return sum_string_and_digit(value, test_argument); }, test_argument);
        REQUIRE(i == 7);

        optional move_only { std::make_unique<std::string>("3") };

        // move semantics work
        i = std::move(move_only).and_then([&](auto value_ptr, auto& test_argument) { return sum_string_and_digit(*value_ptr, test_argument); }, test_argument);
        REQUIRE(i == 5);

    }
}

TEST_CASE("optional::emplace()", "[optional]")
{
    optional<std::string> s;
    auto& emplaced_value = s.emplace(5, 'z');

    REQUIRE(emplaced_value == "zzzzz");
    s.if_set([&](const std::string& value) { REQUIRE(value == "zzzzz"); REQUIRE(&emplaced_value == &value); }).if_unset([]() { FAIL("emplace must result in an engaged optional"); });
}

TEST_CASE("optional::emplace_if_empty()", "[optional]")
{
    optional<std::string> s;
    auto& emplaced_value = s.emplace_if_empty([]() { return std::string(5, 'z'); });

    REQUIRE(emplaced_value == "zzzzz");
    s.if_set([&](const std::string& value) { REQUIRE(value == "zzzzz"); REQUIRE(&emplaced_value == &value); }).if_unset([]() { FAIL("emplace_if_empty must result in an engaged optional"); });

    // this emplace must not occur, as s is already engaged
    auto& engaged_value = s.emplace_if_empty([]() { FAIL("emplace_if_empty functor must not be called on engaged optional"); return std::string(5, 'a'); });

    // ensure the values are still the same
    REQUIRE(emplaced_value == "zzzzz");
    s.if_set([&](const std::string& value) { REQUIRE(value == "zzzzz"); REQUIRE(&emplaced_value == &value); }).if_unset([]() { FAIL("emplace_if_empty must result in an engaged optional"); });
}

TEST_CASE("optional::collapse()", "[optional]")
{
    // converts a string into a number from 0-9, or an empty optional if the
    // string wasn't a number from 0-9
    auto string_to_digit = [](const std::string& str) -> optional<int> {
        if (str.size() == 1 && str[0] >= '0' && str[0] <= '9') {
            return str[0] - '0';
        } else {
            return std::nullopt;
        }
    };

    // unless you have a specific use case for this, you should probably use and_then to
    // get a more ergonomic experience, but nested optionals can also be handled
    optional<std::string> s { "5" };
    optional<optional<int>> o_i = s.if_set([&](const std::string& value) { return string_to_digit(value); });

    // engaged inner and outer optional<optional<int>> must collapse to engaged optional<int>
    optional<int> i = std::move(o_i).collapse();
    i.if_set([](int value) { REQUIRE(value == 5); }).if_unset([]() { FAIL("engaged inner and outer optional<optional<int>> must collapse to engaged optional<int>"); });

    // engaged outer but disengaged inner optional<optional<int>> must collapse to an empty optional
    o_i = optional<int> {};
    REQUIRE(std::move(o_i).collapse().empty());

    // disengaged outer optional<optional<int>> must collapse to an empty optional
    o_i = std::nullopt;
    REQUIRE(std::move(o_i).collapse().empty());

    // multiple levels can be collapsed at once
    optional<optional<optional<optional<optional<optional<optional<optional<optional<optional<int>>>>>>>>>> super_optional { std::in_place_t {}, std::in_place_t {}, std::in_place_t {}, std::in_place_t {}, std::in_place_t {}, std::in_place_t {}, std::in_place_t {}, std::in_place_t {}, std::in_place_t {}, 1 };
    optional<int> super_collapsed = std::move(super_optional).collapse();

    super_collapsed.if_set([](int value) { REQUIRE(value == 1); }).if_unset([]() { FAIL("fully engaged deeply nested optional must collapse to engaged optional<int>"); });
}

TEST_CASE("optional::empty()", "[optional]")
{
    optional<int> i;

    // default constructed optional must be empty
    REQUIRE(i.empty());

    // optional given a value must not be empty
    i = 1;
    REQUIRE_FALSE(i.empty());

    // previously engaged optional assigned std::nullopt must be empty again
    i = std::nullopt;
    REQUIRE(i.empty());

    // optional constructed with a value must not be empty
    optional i2 { 2 };
    REQUIRE_FALSE(i2.empty());
}

TEST_CASE("optional comparisons", "[optional]")
{
    optional<int> empty_1, empty_2;
    optional<int> engaged_1 { 0 }, engaged_2 { 0 };

    // empty optional is always less than an engaged optional, regardless of the value
    REQUIRE(empty_1 < engaged_1);
    REQUIRE(empty_1 <= engaged_1);
    REQUIRE_FALSE(empty_1 > engaged_1);
    REQUIRE_FALSE(empty_1 >= engaged_1);

    // engaged optional is always greater than an empty optional, regardless of the value
    REQUIRE(engaged_1 > empty_1);
    REQUIRE(engaged_1 >= empty_1);
    REQUIRE_FALSE(engaged_1 < empty_1);
    REQUIRE_FALSE(engaged_1 <= empty_1);

    // optionals initialized identically should evaluate as equivalent
    REQUIRE(empty_1 == empty_2);
    REQUIRE(engaged_1 == engaged_2);
    REQUIRE_FALSE(empty_1 != empty_2);
    REQUIRE_FALSE(engaged_1 != engaged_2);

    // empty optional is always less than a comparable value type
    REQUIRE(empty_1 < 0);
    REQUIRE(empty_1 <= 0);
    REQUIRE_FALSE(empty_1 > 0);
    REQUIRE_FALSE(empty_1 >= 0);

    // empty optional never equals a value type
    REQUIRE_FALSE(empty_1 == 0);

    // comparisons passes through to engaged optional's value
    REQUIRE(engaged_1 == 0);
    REQUIRE(engaged_1 != 1);
    REQUIRE_FALSE(engaged_1 != 0);
    REQUIRE_FALSE(engaged_1 == 1);

    REQUIRE(engaged_1 > -1);
    REQUIRE(engaged_1 >= -1);
    REQUIRE(engaged_1 >= 0);
    REQUIRE_FALSE(engaged_1 > 1);
    REQUIRE_FALSE(engaged_1 >= 1);

    REQUIRE(engaged_1 < 1);
    REQUIRE(engaged_1 <= 1);
    REQUIRE(engaged_1 <= 0);
    REQUIRE_FALSE(engaged_1 < -1);
    REQUIRE_FALSE(engaged_1 <= -1);
}