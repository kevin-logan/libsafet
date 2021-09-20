#include <catch2/catch.hpp>

#include <safet/finally.hpp>

using namespace safet;

TEST_CASE("finally example use cases", "[finally]")
{
    // finally is a general purpose RAII concept, and whenever it is used one
    // should first consider if there's a more specific object that should be
    // used instead (e.g. safet::critical_section, unique_ptr, shared_ptr, etc)
    // but sometimes a use case is so niche building a new RAII object for it
    // might feel like overkill, and finally is a good general purpose utility
    // for those use cases.

    // for example, say you want to time a function that has multiple exit
    // conditions, perhaps in a bit of code that is frequently changing with
    // new exit conditions added regularly ... for some reason.

    auto timing_logged = false;
    auto complicated_exit_conditions = [&](int arg) {
        finally log_time { [&]() { timing_logged = true; } };
        if (arg == 0) {
            return 0;
        }

        if (arg == 1) {
            return arg * arg;
        }

        if (arg == 2) {
            return arg + arg;
        }

        if (arg == 3) {
            return arg % 2;
        }

        return arg;
    };

    SECTION("first exit condition triggered")
    {
        complicated_exit_conditions(0);
        REQUIRE(timing_logged);
    }

    SECTION("second exit condition triggered")
    {
        complicated_exit_conditions(1);
        REQUIRE(timing_logged);
    }

    SECTION("third exit condition triggered")
    {
        complicated_exit_conditions(2);
        REQUIRE(timing_logged);
    }

    SECTION("fourth exit condition triggered")
    {
        complicated_exit_conditions(3);
        REQUIRE(timing_logged);
    }

    SECTION("fifth exit condition triggered")
    {
        complicated_exit_conditions(4);
        REQUIRE(timing_logged);
    }
}

TEST_CASE("finally construtors", "[finally]")
{
    size_t call_count { 0 };

    {
        // type deduced from functor
        finally f { [&]() { ++call_count; } };

        REQUIRE(call_count == 0);

        // copy construct
        finally f_copy { f };

        REQUIRE(call_count == 0);

        // move construct
        finally f_move { std::move(f) };

        REQUIRE(call_count == 0);
    }

    // f_copy and f_move both destruct, f no longer should hold any functor
    // and should not result in a call
    REQUIRE(call_count == 2);
}