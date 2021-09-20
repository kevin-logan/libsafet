#include <catch2/catch.hpp>

#include <safet/finally.hpp>
#include <safet/memory.hpp>

using namespace safet;

template <typename T>
struct copy_only_functor {
    copy_only_functor(T functor)
        : functor(std::move(functor))
    {
    }

    copy_only_functor(const copy_only_functor&) = default;
    copy_only_functor(copy_only_functor&&) = delete;

    template <typename... Ts>
    decltype(auto) operator()(Ts&&... params)
    {
        return functor(std::forward<Ts>(params)...);
    }
    T functor;
};

template <typename T>
struct move_only_functor {
    move_only_functor(T functor)
        : functor(std::move(functor))
    {
    }

    move_only_functor(const move_only_functor&) = delete;
    move_only_functor(move_only_functor&&) = default;

    template <typename... Ts>
    decltype(auto) operator()(Ts&&... params)
    {
        return functor(std::forward<Ts>(params)...);
    }
    T functor;
};

template <typename T>
struct no_move_nor_copy_functor {
    no_move_nor_copy_functor(T functor)
        : functor(std::move(functor))
    {
    }

    no_move_nor_copy_functor(const no_move_nor_copy_functor&) = delete;
    no_move_nor_copy_functor(no_move_nor_copy_functor&&) = delete;

    template <typename... Ts>
    decltype(auto) operator()(Ts&&... params)
    {
        return functor(std::forward<Ts>(params)...);
    }
    T functor;
};

TEST_CASE("unique_ptr constructors", "[unique_ptr]")
{
    SECTION("Default constructor")
    {
        unique_ptr<int> p {};

        REQUIRE(p.deref().empty());
    }
    SECTION("Constructed explicitely nullptr")
    {
        unique_ptr<int> p { nullptr };

        REQUIRE(p.deref().empty());
    }
    SECTION("Constructed via raw pointer")
    {
        bool destructed = false;
        {
            unique_ptr p { new finally { [&]() { destructed = true; } } };
            REQUIRE_FALSE(destructed);
        }
        REQUIRE(destructed);
    }
    SECTION("Constructed via raw pointer with special destructor")
    {
        SECTION("Special destructor is a value type")
        {
            SECTION("Special destructor by copy")
            {
                bool destructed = false;
                auto destructor = copy_only_functor { [&](int* p) { std::destroy_at(p); destructed = true; } };

                {
                    unique_ptr<int, decltype(destructor)> p { new int { 1337 }, destructor };

                    REQUIRE_FALSE(destructed);
                    REQUIRE(p.deref() == 1337);
                }
                REQUIRE(destructed);
            }

            SECTION("Special destructor by move")
            {
                bool destructed = false;
                auto destructor = move_only_functor { [&](int* p) { std::destroy_at(p); destructed = true; } };

                {
                    unique_ptr<int, decltype(destructor)> p { new int { 1337 }, std::move(destructor) };

                    REQUIRE_FALSE(destructed);
                    REQUIRE(p.deref() == 1337);
                }
                REQUIRE(destructed);
            }
        }
        SECTION("Special destructor is a reference type")
        {
            bool destructed = false;
            auto destructor = no_move_nor_copy_functor { [&](int* p) { std::destroy_at(p); destructed = true; } };

            // std::unique_ptr supports reference-type destructors directly, while safet does not because
            // std::reference_wrapper already perfectly accomplishes (as they provide the call operator)
            // and force the user code to be even more explicit about using a reference.
            auto destructor_ref = std::ref(destructor);

            {
                unique_ptr<int, decltype(destructor_ref)> p { new int { 1337 }, destructor_ref };

                REQUIRE_FALSE(destructed);
                REQUIRE(p.deref() == 1337);
            }
            REQUIRE(destructed);
        }
    }
    SECTION("Constructed by move")
    {
        auto a = make_unique<int>(1337);
        unique_ptr b { std::move(a) };

        REQUIRE(a.deref().empty());
        REQUIRE(b.deref() == 1337);
    }
}

TEST_CASE("unique_ptr assignment", "[unique_ptr]")
{
    SECTION("Move assignment")
    {
        auto a = make_unique<int>(1337);
        auto b = make_unique<int>(0);

        REQUIRE(a.deref() == 1337);
        REQUIRE(b.deref() == 0);

        b = std::move(a);

        REQUIRE(a.deref().empty());
        REQUIRE(b.deref() == 1337);
    }
}

TEST_CASE("unique_ptr::deref()", "[unique_ptr]")
{
    auto a = make_unique<int>(5);
    unique_ptr<int> b;

    REQUIRE(a.deref() == 5);
    REQUIRE(b.deref().empty());
}

TEST_CASE("unique_ptr::empty()", "[unique_ptr]")
{
    auto a = make_unique<int>(5);
    unique_ptr<int> b;

    REQUIRE_FALSE(a.empty());
    REQUIRE(b.empty());
}

TEST_CASE("unique_ptr::clear()", "[unique_ptr]")
{
    auto a = make_unique<int>(5);
    unique_ptr<int> b;

    REQUIRE_FALSE(a.empty());
    REQUIRE(b.empty());

    a.clear();
    b.clear();

    REQUIRE(a.empty());
    REQUIRE(b.empty());
}

TEST_CASE("shared_ptr constructors", "[shared_ptr]")
{
    SECTION("Default constructor")
    {
        shared_ptr<int> p {};

        REQUIRE(p.deref().empty());
    }
    SECTION("Constructed explicitely nullptr")
    {
        shared_ptr<int> p { nullptr };

        REQUIRE(p.deref().empty());
    }
    SECTION("Constructed via raw pointer")
    {
        bool destructed = false;
        {
            shared_ptr p { new finally { [&]() { destructed = true; } } };
            REQUIRE_FALSE(destructed);
        }
        REQUIRE(destructed);
    }
    SECTION("Constructed by copy")
    {
        auto a = make_shared<int>(1337);
        shared_ptr b { a };

        REQUIRE(a.deref() == 1337);
        REQUIRE(b.deref() == 1337);
    }
    SECTION("Constructed by move")
    {
        auto a = make_shared<int>(1337);
        shared_ptr b { std::move(a) };

        REQUIRE(a.deref().empty());
        REQUIRE(b.deref() == 1337);
    }
}

TEST_CASE("shared_ptr assignment", "[shared_ptr]")
{
    SECTION("Copy assignment")
    {
        auto a = make_shared<int>(1337);
        auto b = make_shared<int>(0);

        REQUIRE(a.deref() == 1337);
        REQUIRE(b.deref() == 0);

        b = a;

        REQUIRE(a.deref() == 1337);
        REQUIRE(b.deref() == 1337);
    }
    SECTION("Move assignment")
    {
        auto a = make_shared<int>(1337);
        auto b = make_shared<int>(0);

        REQUIRE(a.deref() == 1337);
        REQUIRE(b.deref() == 0);

        b = std::move(a);

        REQUIRE(a.deref().empty());
        REQUIRE(b.deref() == 1337);
    }
}

TEST_CASE("shared_ptr::deref()", "[shared_ptr]")
{
    auto a = make_shared<int>(5);
    shared_ptr<int> b;

    REQUIRE(a.deref() == 5);
    REQUIRE(b.deref().empty());
}

TEST_CASE("shared_ptr::empty()", "[shared_ptr]")
{
    auto a = make_shared<int>(5);
    shared_ptr<int> b;

    REQUIRE_FALSE(a.empty());
    REQUIRE(b.empty());
}

TEST_CASE("shared_ptr::clear()", "[shared_ptr]")
{
    auto a = make_shared<int>(5);
    shared_ptr<int> b;

    REQUIRE_FALSE(a.empty());
    REQUIRE(b.empty());

    a.clear();
    b.clear();

    REQUIRE(a.empty());
    REQUIRE(b.empty());
}

TEST_CASE("weak_ptr constructors", "[weak_ptr]")
{
    SECTION("Default constructor")
    {
        weak_ptr<int> p {};

        REQUIRE(p.lock().deref().empty());
    }
    SECTION("Constructed from shared_ptr")
    {
        auto shared = make_shared<int>(1337);
        weak_ptr a { shared };

        REQUIRE(a.lock().deref() == 1337);
    }
    SECTION("Constructed by copy")
    {
        auto shared = make_shared<int>(1337);
        weak_ptr a { shared };
        weak_ptr b { a };

        REQUIRE(a.lock().deref() == 1337);
        REQUIRE(b.lock().deref() == 1337);
    }
    SECTION("Constructed by move")
    {
        auto shared = make_shared<int>(1337);
        weak_ptr a { shared };
        weak_ptr b { std::move(a) };

        REQUIRE(a.lock().deref().empty());
        REQUIRE(b.lock().deref() == 1337);
    }
}

TEST_CASE("weak_ptr assignment", "[weak_ptr]")
{
    auto shared = make_shared<int>(1337);
    SECTION("Copy assignment")
    {
        weak_ptr a { shared };
        weak_ptr<int> b {};

        REQUIRE(a.lock().deref() == 1337);
        REQUIRE(b.lock().deref().empty());

        b = a;

        REQUIRE(a.lock().deref() == 1337);
        REQUIRE(b.lock().deref() == 1337);
    }
    SECTION("Move assignment")
    {
        weak_ptr a { shared };
        weak_ptr<int> b {};

        REQUIRE(a.lock().deref() == 1337);
        REQUIRE(b.lock().deref().empty());

        b = std::move(a);

        REQUIRE(a.lock().deref().empty());
        REQUIRE(b.lock().deref() == 1337);
    }
    SECTION("Assignment from shared_ptr")
    {
        weak_ptr<int> a {};

        REQUIRE(a.lock().deref().empty());

        a = shared;

        REQUIRE(a.lock().deref() == 1337);
    }
}

TEST_CASE("weak_ptr::lock()", "[weak_ptr]")
{
    weak_ptr<int> a {};

    // nothing set so only locking an empty shared_ptr
    REQUIRE(a.lock().empty());

    {
        auto shared = make_shared<int>(5);
        a = shared;

        // value was set to a shared_ptr that still lives, should lock shared_ptr pointing to that value
        REQUIRE(a.lock().deref() == 5);
    }

    // shared_ptr in scope above has destructed, weak_ptr should now lock an empty shared_ptr
    REQUIRE(a.lock().empty());
}

TEST_CASE("weak_ptr::clear()", "[weak_ptr]")
{
    auto shared = make_shared<int>(5);
    weak_ptr<int> a { shared };
    weak_ptr<int> b;

    REQUIRE_FALSE(a.lock().empty());
    REQUIRE(b.lock().empty());

    a.clear();
    b.clear();

    REQUIRE(a.lock().empty());
    REQUIRE(b.lock().empty());
}