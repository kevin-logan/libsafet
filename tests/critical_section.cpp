#include <catch2/catch.hpp>

#include <safet/critical_section.hpp>

#include <chrono>
#include <thread>

using namespace safet;

TEST_CASE("critical_section example use cases", "[critical_section]")
{
    critical_section<int> cs;
    SECTION("read/write a value on multiple threads")
    {
        auto double_and_test = [](critical_section<int>& cs) {
            size_t test_count = 100;

            while (test_count-- != 0) {
                cs.enter([](int& i) {
                    int copy = i;
                    copy *= 2;

                    // give enough time here for another thread to theoretically mutate the value before
                    // we test to make sure our copied value is indeed still double the original value
                    std::this_thread::sleep_for(std::chrono::milliseconds { 1 });

                    // make sure nothing has modified our value since we copied it note - this failing can prove
                    // there is a bug, but it passing wouldn't prove there is no bug. This is technically always
                    // true, but especially true on tests around synchronization primitives
                    // "testing can be used to show the presence of bugs but never to show their absence." - Dijkstra
                    REQUIRE(copy == i * 2);

                    // update the master value so we could prove a bug exists if these operations aren't properly guarded
                    i = copy;
                });
            }
        };

        std::vector<std::thread> v;
        constexpr size_t thread_count = 10;

        for (size_t i = 0; i < thread_count; ++i) {
            v.emplace_back(double_and_test, std::ref(cs));
        }

        // join the threads
        for (auto& t : v) {
            t.join();
        }
    }
}

TEST_CASE("critical_section constructor", "[critical_section]")
{
    // only constructor is a forwarding constructor
    critical_section<std::string> s { 5, 'a' };

    s.enter([](const std::string& value) { REQUIRE(value == "aaaaa"); });
}

TEST_CASE("critical_section::enter()", "[critical_section]")
{
    critical_section<std::string> s { 5, 'a' };

    SECTION("when returning a value")
    {
        SECTION("with l-value reference")
        {
            auto retval = s.enter([](std::string& value) {
                REQUIRE(value == "aaaaa");
                value += "bbbbb";

                return 1;
            });

            REQUIRE(retval == 1);

            s.enter([](std::string& value) {
                REQUIRE(value == "aaaaabbbbb");
            });
        }

        SECTION("with const l-value reference")
        {
            const auto& c_s = s;

            auto retval = s.enter([](const std::string& value) {
                REQUIRE(value == "aaaaa");

                return 1;
            });

            REQUIRE(retval == 1);
        }

        SECTION("with r-value reference")
        {
            auto retval = std::move(s).enter([](std::string&& value) {
                REQUIRE(value == "aaaaa");
                value += "bbbbb";

                return std::move(value);
            });

            REQUIRE(retval == "aaaaabbbbb");
        }
    }

    SECTION("when returning void")
    {
        SECTION("with l-value reference")
        {
            s.enter([](std::string& value) {
                REQUIRE(value == "aaaaa");
                value += "bbbbb";
            });

            s.enter([](std::string& value) {
                REQUIRE(value == "aaaaabbbbb");
            });
        }

        SECTION("with const l-value reference")
        {
            const auto& c_s = s;

            s.enter([](const std::string& value) {
                REQUIRE(value == "aaaaa");
            });
        }

        SECTION("with r-value reference")
        {
            std::move(s).enter([](std::string&& value) {
                REQUIRE(value == "aaaaa");
            });
        }
    }
}

TEST_CASE("critical_section::try_enter()", "[critical_section]")
{
    critical_section<std::string> s { 5, 'a' };

    SECTION("when exclusive access fails")
    {
        s.enter([&](std::string&) {
            SECTION("when returning a value")
            {
                SECTION("with l-value reference")
                {
                    auto retval = s.try_enter([](std::string& value) {
                        FAIL("try_enter must not call the functor on an already entered critical_section");

                        return 1;
                    });

                    REQUIRE(retval.empty());

                    s.try_enter([](std::string& value) {
                        FAIL("try_enter must not call the functor on an already entered critical_section");
                    });
                }

                SECTION("with const l-value reference")
                {
                    const auto& c_s = s;

                    auto retval = s.try_enter([](const std::string& value) {
                        FAIL("try_enter must not call the functor on an already entered critical_section");

                        return 1;
                    });

                    REQUIRE(retval.empty());
                }

                SECTION("with r-value reference")
                {
                    auto retval = std::move(s).try_enter([](std::string&& value) {
                        FAIL("try_enter must not call the functor on an already entered critical_section");

                        return 1;
                    });

                    REQUIRE(retval.empty());
                }
            }

            SECTION("when returning void")
            {
                SECTION("with l-value reference")
                {
                    auto called = s.try_enter([](std::string&) {
                        FAIL("try_enter must not call the functor on an already entered critical_section");
                    });

                    REQUIRE_FALSE(called);
                }

                SECTION("with const l-value reference")
                {
                    const auto& c_s = s;

                    auto called = s.try_enter([](const std::string&) {
                        FAIL("try_enter must not call the functor on an already entered critical_section");
                    });

                    REQUIRE_FALSE(called);
                }

                SECTION("with r-value reference")
                {
                    auto called = std::move(s).try_enter([](std::string&&) {
                        FAIL("try_enter must not call the functor on an already entered critical_section");
                    });

                    REQUIRE_FALSE(called);
                }
            }
        });
    }

    SECTION("when exclusive access succeeds")
    {
        SECTION("when returning a value")
        {
            SECTION("with l-value reference")
            {
                auto retval = s.try_enter([](std::string& value) {
                    REQUIRE(value == "aaaaa");
                    value += "bbbbb";

                    return 1;
                });

                REQUIRE(retval == 1);

                s.try_enter([](std::string& value) {
                    REQUIRE(value == "aaaaabbbbb");
                });
            }

            SECTION("with const l-value reference")
            {
                const auto& c_s = s;

                auto retval = s.try_enter([](const std::string& value) {
                    REQUIRE(value == "aaaaa");

                    return 1;
                });

                REQUIRE(retval == 1);
            }

            SECTION("with r-value reference")
            {
                auto retval = std::move(s).try_enter([](std::string&& value) {
                    REQUIRE(value == "aaaaa");
                    value += "bbbbb";

                    return std::move(value);
                });

                REQUIRE(retval == "aaaaabbbbb");
            }
        }

        SECTION("when returning void")
        {
            SECTION("with l-value reference")
            {
                auto called = s.try_enter([](std::string& value) {
                    REQUIRE(value == "aaaaa");
                    value += "bbbbb";
                });

                REQUIRE(called);

                called = s.try_enter([](std::string& value) {
                    REQUIRE(value == "aaaaabbbbb");
                });

                REQUIRE(called);
            }

            SECTION("with const l-value reference")
            {
                const auto& c_s = s;

                auto called = s.try_enter([](const std::string& value) {
                    REQUIRE(value == "aaaaa");
                });

                REQUIRE(called);
            }

            SECTION("with r-value reference")
            {
                auto called = std::move(s).try_enter([](std::string&& value) {
                    REQUIRE(value == "aaaaa");
                });

                REQUIRE(called);
            }
        }
    }
}