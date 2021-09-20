#include <catch2/catch.hpp>

#include <safet/cow.hpp>

#include <string>

using namespace safet;

TEST_CASE("cow example use cases", "[cow]")
{
    SECTION("cow can be used to efficiently return a value or a reference")
    {
        std::string s { "Hello, World!" };

        auto string_replacer = [](const std::string& str, const std::string& find_value, const std::string& replace_value) -> cow<std::string> {
            cow result { std::cref(str) };

            auto pos = str.find(find_value);

            if (pos != std::string::npos) {
                result.get_mutable() = str.substr(0, pos) + replace_value + str.substr(pos + find_value.size());
            }

            return result;
        };

        auto result = string_replacer(s, "this isn't in the original string", "we won't do any replacing");

        // because nothing should have been replaced, we should have received a cow that references the original data, so no copy was made
        REQUIRE(&result.get_const() == &s);

        result = string_replacer(s, "Hello", "Goodbye");

        // because this time it should have replaced Hello with Goodbye, we expect a new value AND the original value to be unchanged
        REQUIRE(&result.get_const() != &s);
        REQUIRE(result.get_const() == "Goodbye, World!");
        REQUIRE(s == "Hello, World!");
    }
}

TEST_CASE("cow constructors", "[cow]")
{
    std::string base_value { "Hello, World!" };

    SECTION("construct with value")
    {
        cow c { base_value };

        REQUIRE(c.get_const() == base_value);
        // we started with our own copy, should not reference base value
        REQUIRE(&c.get_const() != &base_value);
    }

    SECTION("construct with reference")
    {
        cow c { std::cref(base_value) };

        REQUIRE(c.get_const() == base_value);
        REQUIRE(&c.get_const() == &base_value);
    }

    SECTION("copy construct")
    {
        cow owned { base_value };
        cow refed { std::cref(base_value) };

        cow copy_owned { owned };
        cow copy_refed { refed };

        // copies should never hold by-value, but always reference the value of the cow they copied
        REQUIRE(copy_owned.get_const() == base_value);
        REQUIRE(copy_refed.get_const() == base_value);
        REQUIRE(&copy_owned.get_const() == &owned.get_const());
        REQUIRE(&copy_refed.get_const() == &refed.get_const());
    }

    SECTION("move construct")
    {
        cow owned { base_value };
        cow refed { std::cref(base_value) };

        cow move_owned { std::move(owned) };
        cow move_refed { std::move(refed) };

        // moves should act exactly like what was moved out, holding a value or ref just like the moved-from value
        REQUIRE(move_owned.get_const() == base_value);
        REQUIRE(move_refed.get_const() == base_value);
        REQUIRE(&move_owned.get_const() != &base_value);
        REQUIRE(&move_refed.get_const() == &base_value);
    }
}

TEST_CASE("cow assignment operators")
{
    std::string base_value { "Hello, World!" };
    std::string modify_value { "And, Goodbye!" };
    cow c { std::cref(base_value) };

    SECTION("assign with value")
    {
        c = modify_value;

        // prove value was mutated
        REQUIRE(c.get_const() == modify_value);

        // owned by value, must not reference modify_value
        REQUIRE(&c.get_const() != &modify_value);
    }

    SECTION("assign with reference")
    {
        c = std::cref(modify_value);

        // prove value was mutated
        REQUIRE(c.get_const() == modify_value);

        // prove cow is referencing modify_value, not a copy
        REQUIRE(&c.get_const() == &modify_value);
    }

    SECTION("copy assign")
    {
        cow owned { base_value };
        cow refed { std::cref(base_value) };

        c = owned;
        // copies should never hold by-value, but always reference the value of the cow they copied
        REQUIRE(&c.get_const() == &owned.get_const());

        c = refed;
        // copies should never hold by-value, but always reference the value of the cow they copied
        REQUIRE(&c.get_const() == &refed.get_const());
    }

    SECTION("move assign")
    {
        cow owned { base_value };
        cow refed { std::cref(base_value) };

        c = std::move(owned);
        // moves should act exactly like what was moved out, so moving a value means we also hold by value
        REQUIRE(&c.get_const() != &base_value);

        c = std::move(refed);
        // moves should act exactly like what was moved out, so moving a ref means we also hold a ref
        REQUIRE(&c.get_const() == &base_value);
    }
}

TEST_CASE("cow::get_const()", "[cow]")
{
    std::string base_value { "Hello, World!" };
    cow owned { base_value };
    cow refed { std::cref(base_value) };

    auto* owned_value_ptr = &owned.get_const();

    // make sure multiple calls to get_const doesn't change the value ptr
    REQUIRE(owned.get_const() == base_value);
    REQUIRE(&owned.get_const() == owned_value_ptr);
    REQUIRE(owned_value_ptr != &base_value);

    // make sure that after multiple calls to get_const we still reference the original data
    REQUIRE(refed.get_const() == base_value);
    REQUIRE(&refed.get_const() == &base_value);
}

TEST_CASE("cow::get_mutable()", "[cow]")
{
    std::string base_value { "Hello, World!" };

    SECTION("on an owned cow")
    {
        cow owned { base_value };

        auto* owned_value_ptr = &owned.get_const();

        owned.get_mutable() += " And, Goodbye!";

        // make sure value was updated
        REQUIRE(owned.get_const() == "Hello, World! And, Goodbye!");

        // make sure no copy was made as we already owned the value
        REQUIRE(&owned.get_const() == owned_value_ptr);

        // make sure we're not referencing the original value
        REQUIRE(owned_value_ptr != &base_value);
    }

    SECTION("on a refed cow")
    {
        cow refed { std::cref(base_value) };

        auto* refed_value_ptr = &refed.get_const();

        refed.get_mutable() += " And, Goodbye!";

        // make sure value was updated
        REQUIRE(refed.get_const() == "Hello, World! And, Goodbye!");

        // make sure we copied our own value on write
        REQUIRE(&refed.get_const() != refed_value_ptr);

        // make sure nothing touched the original value
        REQUIRE(base_value == "Hello, World!");
    }
}

TEST_CASE("cow::set()", "[cow]")
{
    std::string base_value { "Hello, World!" };
    std::string modify_value { "And, Goodbye!" };
    cow c { std::cref(base_value) };

    SECTION("set with value")
    {
        c.set(modify_value);

        // prove value was mutated
        REQUIRE(c.get_const() == modify_value);

        // owned by value, must not reference modify_value
        REQUIRE(&c.get_const() != &modify_value);
    }

    SECTION("set with reference")
    {
        c.set(std::cref(modify_value));

        // prove value was mutated
        REQUIRE(c.get_const() == modify_value);

        // prove cow is referencing modify_value, not a copy
        REQUIRE(&c.get_const() == &modify_value);
    }
}

TEST_CASE("cow::operator->()", "[cow]")
{
    std::string base_value { "Hello, World!" };
    cow owned { base_value };
    cow refed { std::cref(base_value) };

    // owned cow should not reference the base_value but it should be identical
    REQUIRE(owned.operator->() != &base_value);
    REQUIRE(*owned.operator->() == base_value);

    // refed cow should reference base_value
    REQUIRE(refed.operator->() == &base_value);

    REQUIRE(owned->size() == base_value.size());
    REQUIRE(refed->size() == base_value.size());
}