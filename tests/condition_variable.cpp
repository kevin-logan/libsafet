#include <catch2/catch.hpp>

#include <safet/condition_variable.hpp>

#include <string>
#include <thread>

using namespace safet;

TEST_CASE("condition_variable example use cases", "[condition_variable]")
{
}

TEST_CASE("condition_variable constructors", "[condition_variable]")
{
    // only constructor is a forwarding constructor
    condition_variable<std::string> s { 5, 'z' };

    s.inspect([](const std::string& value) { REQUIRE(value == "zzzzz"); });
}

TEST_CASE("condition_variable::notify() and condition_variable::wait()", "[condition_variable]")
{
    condition_variable<int> s { 1 };

    SECTION("notify can be used to wake up waiters that have external conditions")
    {
        enum class state {
            THREAD_STARTING,
            THREAD_WAITING,
            THREAD_SHOULD_WAKE,
            THREAD_FINISHED,
        };

        std::atomic<state> external_condition { state::THREAD_STARTING };

        std::thread wait_until_external_condition {
            [&]() {
                external_condition.exchange(state::THREAD_WAITING);
                s.notify(); // notify main thread we've updated value

                s.wait(
                    [&](const int&) { return external_condition.load() == state::THREAD_SHOULD_WAKE; },
                    [&](int&) {
                        REQUIRE(external_condition.load() == state::THREAD_SHOULD_WAKE);
                        // set condition back to false once we're done
                        external_condition.exchange(state::THREAD_FINISHED);
                        s.notify();
                    });
            }
        };

        // wait for thread waiting signal
        s.wait(
            [&](const int&) { return external_condition.load() == state::THREAD_WAITING; },
            [&](int&) {
                REQUIRE(external_condition.load() == state::THREAD_WAITING);
                external_condition.exchange(state::THREAD_SHOULD_WAKE);
                s.notify();
            });
        s.wait(
            [&](const int&) { return external_condition.load() == state::THREAD_FINISHED; },
            [&](int&) {
                REQUIRE(external_condition.load() == state::THREAD_FINISHED);
                wait_until_external_condition.join();
            });

        s.inspect([](const int& value) { REQUIRE(value == 1); });
    }
}