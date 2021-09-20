#include <catch2/catch.hpp>

#include <safet/variant.hpp>

#include <charconv>
#include <string>

using namespace safet;

enum class error_code {
    PRETTY_BROKE,
    REALLY_BROKE,
    SURPRISINGLY_BROKE,
};

template <typename T>
using result = variant<T, error_code>;

static constexpr size_t result_value_index = 0;
static constexpr size_t result_error_index = 1;

TEST_CASE("variant example use cases", "[variant]")
{
    SECTION("variant can be used like a result type, in lieu of exceptions")
    {
        auto string_to_number = [](const std::string& str) -> result<int64_t> {
            int64_t result;
            auto [dummy, ec] = std::from_chars(str.data(), str.data() + str.size(), result);

            if (ec == std::errc()) {
                return result;
            } else {
                return error_code::PRETTY_BROKE;
            }
        };

        auto i = string_to_number("123");

        REQUIRE(i.get<result_value_index>() == 123);
        REQUIRE(i.get<result_error_index>().empty());

        i = string_to_number("this isn't a number, actually");

        REQUIRE(i.get<result_value_index>().empty());
        REQUIRE(i.get<result_error_index>() == error_code::PRETTY_BROKE);
    }

    SECTION("variant can provide storage to support static polymorphism")
    {
        class pet_animal {
        public:
            pet_animal(std::string name)
                : name(std::move(name))
            {
            }

            auto pet() const -> void { has_been_petted = true; }

            std::string name;
            mutable bool has_been_petted { false };
        };

        class dog : public pet_animal {
        public:
            dog(std::string name)
                : pet_animal(std::move(name))
            {
            }

            auto bark() const -> void { has_barked = true; }

            mutable bool has_barked { false };
        };

        class cat : public pet_animal {
        public:
            cat(std::string name)
                : pet_animal(std::move(name))
            {
            }

            auto meow() const -> void { has_meowed = true; }

            mutable bool has_meowed { false };
        };

        variant<dog, cat> v_d { dog { "otto" } };
        variant<dog, cat> v_c { cat { "teacup" } };

        auto animal_handler = overloaded {
            [](const dog& d) -> void { d.bark(); },
            [](const cat& c) -> void { c.meow(); }
        };

        auto visitor = [&](const auto& value) {
            // pet the dog or cat
            value.pet();

            // call an overloaded function with the dog or cat (with type information intact)
            animal_handler(value);
        };

        v_d.visit(visitor);
        v_c.visit(visitor);

        REQUIRE(v_d.get<dog>().if_set([](const dog& d) { return d.name == "otto"; }) == true);
        REQUIRE(v_d.get<dog>().if_set([](const dog& d) { return d.has_been_petted; }) == true);
        REQUIRE(v_d.get<dog>().if_set([](const dog& d) { return d.has_barked; }) == true);

        REQUIRE(v_c.get<cat>().if_set([](const cat& c) { return c.name == "teacup"; }) == true);
        REQUIRE(v_c.get<cat>().if_set([](const cat& c) { return c.has_been_petted; }) == true);
        REQUIRE(v_c.get<cat>().if_set([](const cat& c) { return c.has_meowed; }) == true);
    }
}

TEST_CASE("variant constructors", "[variant]")
{
    SECTION("default construction")
    {
        variant<int, double, std::string> v_i;
        variant<double, std::string, int> v_d;
        variant<std::string, int, double> v_s;

        // default constructor will value-initialize the first type in the variant
        REQUIRE(v_i.get<int>() == 0);
        REQUIRE(v_i.get<double>().empty());
        REQUIRE(v_i.get<std::string>().empty());

        REQUIRE(v_d.get<int>().empty());
        REQUIRE(v_d.get<double>() == 0.0);
        REQUIRE(v_d.get<std::string>().empty());

        REQUIRE(v_s.get<int>().empty());
        REQUIRE(v_s.get<double>().empty());
        REQUIRE(v_s.get<std::string>() == "");
    }

    SECTION("in_place_type construction")
    {
        SECTION("with values types")
        {
            variant<int, double, std::string> v_i { std::in_place_type_t<int> {}, 1 };
            variant<int, double, std::string> v_d { std::in_place_type_t<double> {}, 1.0 };
            variant<int, double, std::string> v_s { std::in_place_type_t<std::string> {}, 5u, 'z' };

            REQUIRE(v_i.get<int>() == 1);
            REQUIRE(v_i.get<double>().empty());
            REQUIRE(v_i.get<std::string>().empty());

            REQUIRE(v_d.get<int>().empty());
            REQUIRE(v_d.get<double>() == 1.0);
            REQUIRE(v_d.get<std::string>().empty());

            REQUIRE(v_s.get<int>().empty());
            REQUIRE(v_s.get<double>().empty());
            REQUIRE(v_s.get<std::string>() == "zzzzz");
        }

        SECTION("with references")
        {
            int i = 10;
            double d = 10.0;
            std::string s = "yyyyy";

            variant<int&, double&, std::string&> v_i { std::in_place_type_t<int&> {}, i };
            variant<int&, double&, std::string&> v_d { std::in_place_type_t<double&> {}, d };
            variant<int&, double&, std::string&> v_s { std::in_place_type_t<std::string&> {}, s };

            REQUIRE(v_i.get<int&>() == 10);
            REQUIRE(v_i.get<double&>().empty());
            REQUIRE(v_i.get<std::string&>().empty());

            REQUIRE(v_d.get<int&>().empty());
            REQUIRE(v_d.get<double&>() == 10.0);
            REQUIRE(v_d.get<std::string&>().empty());

            REQUIRE(v_s.get<int&>().empty());
            REQUIRE(v_s.get<double&>().empty());
            REQUIRE(v_s.get<std::string&>() == "yyyyy");
        }
    }

    SECTION("in_place_index construction")
    {
        SECTION("with values types")
        {
            variant<int, double, std::string> v_i { std::in_place_index_t<0> {}, 2 };
            variant<int, double, std::string> v_d { std::in_place_index_t<1> {}, 2.0 };
            variant<int, double, std::string> v_s { std::in_place_index_t<2> {}, 5u, 'a' };

            REQUIRE(v_i.get<int>() == 2);
            REQUIRE(v_i.get<double>().empty());
            REQUIRE(v_i.get<std::string>().empty());

            REQUIRE(v_d.get<int>().empty());
            REQUIRE(v_d.get<double>() == 2.0);
            REQUIRE(v_d.get<std::string>().empty());

            REQUIRE(v_s.get<int>().empty());
            REQUIRE(v_s.get<double>().empty());
            REQUIRE(v_s.get<std::string>() == "aaaaa");
        }

        SECTION("with references")
        {
            int i = 20;
            double d = 20.0;
            std::string s = "bbbbb";

            variant<int&, double&, std::string&> v_i { std::in_place_index_t<0> {}, i };
            variant<int&, double&, std::string&> v_d { std::in_place_index_t<1> {}, d };
            variant<int&, double&, std::string&> v_s { std::in_place_index_t<2> {}, s };

            REQUIRE(v_i.get<int&>() == 20);
            REQUIRE(v_i.get<double&>().empty());
            REQUIRE(v_i.get<std::string&>().empty());

            REQUIRE(v_d.get<int&>().empty());
            REQUIRE(v_d.get<double&>() == 20.0);
            REQUIRE(v_d.get<std::string&>().empty());

            REQUIRE(v_s.get<int&>().empty());
            REQUIRE(v_s.get<double&>().empty());
            REQUIRE(v_s.get<std::string&>() == "bbbbb");
        }
    }

    SECTION("construct with value")
    {
        SECTION("with value types")
        {
            variant<int, double, std::string> v_i { 3 };
            variant<int, double, std::string> v_d { 3.0 };
            variant<int, double, std::string> v_s { "ccccc" };

            REQUIRE(v_i.get<int>() == 3);
            REQUIRE(v_i.get<double>().empty());
            REQUIRE(v_i.get<std::string>().empty());

            REQUIRE(v_d.get<int>().empty());
            REQUIRE(v_d.get<double>() == 3.0);
            REQUIRE(v_d.get<std::string>().empty());

            REQUIRE(v_s.get<int>().empty());
            REQUIRE(v_s.get<double>().empty());
            REQUIRE(v_s.get<std::string>() == "ccccc");
        }

        SECTION("with references")
        {
            int i = 30;
            double d = 30.0;
            std::string s = "xxxxx";

            variant<int&, double&, std::string&> v_i { std::ref(i) };
            variant<int&, double&, std::string&> v_d { std::ref(d) };
            variant<int&, double&, std::string&> v_s { std::ref(s) };

            REQUIRE(v_i.get<int&>() == 30);
            REQUIRE(v_i.get<double&>().empty());
            REQUIRE(v_i.get<std::string&>().empty());

            REQUIRE(v_d.get<int&>().empty());
            REQUIRE(v_d.get<double&>() == 30.0);
            REQUIRE(v_d.get<std::string&>().empty());

            REQUIRE(v_s.get<int&>().empty());
            REQUIRE(v_s.get<double&>().empty());
            REQUIRE(v_s.get<std::string&>() == "xxxxx");
        }
    }

    SECTION("copy construction")
    {
        variant<int, double, std::string> v { 4 };
        auto v2 = v;
        auto v3 = v;
        variant<int, double, std::string> v4 { v };
        variant<int, double, std::string> v5 { v };

        REQUIRE(v2.get<int>() == 4);
        REQUIRE(v3.get<int>() == 4);
        REQUIRE(v4.get<int>() == 4);
        REQUIRE(v5.get<int>() == 4);
    }

    SECTION("move construction")
    {
        // variants with non-copiable types, like a unique_ptr, are only movable
        variant<std::unique_ptr<int>, double, std::string> v { std::make_unique<int>(5) };

        auto v2 = std::move(v);
        REQUIRE(v2.get<std::unique_ptr<int>>().if_set([](auto& value_ptr) { return *value_ptr == 5; }) == true);

        auto v3 = std::move(v2);
        REQUIRE(v3.get<std::unique_ptr<int>>().if_set([](auto& value_ptr) { return *value_ptr == 5; }) == true);

        variant<std::unique_ptr<int>, double, std::string> v4 { std::move(v3) };
        REQUIRE(v4.get<std::unique_ptr<int>>().if_set([](auto& value_ptr) { return *value_ptr == 5; }) == true);

        variant<std::unique_ptr<int>, double, std::string> v5 { std::move(v4) };
        REQUIRE(v5.get<std::unique_ptr<int>>().if_set([](auto& value_ptr) { return *value_ptr == 5; }) == true);
    }
}

TEST_CASE("variant assignment operators", "[variant]")
{
    variant<int, double, std::string, int&> v { -1000 };

    SECTION("copy assignment")
    {
        variant<int, double, std::string, int&> v2 { 1 };

        v = v2;

        REQUIRE(v.get<int>() == 1);
    }

    SECTION("move assignment")
    {
        variant<std::unique_ptr<int>, double, std::string> movable_only;
        variant<std::unique_ptr<int>, double, std::string> movable_only_2 { std::make_unique<int>(2) };

        movable_only = std::move(movable_only_2);

        REQUIRE(movable_only.get<std::unique_ptr<int>>().if_set([](auto& value_ptr) { return *value_ptr == 2; }) == true);
    }

    SECTION("value assignment")
    {
        SECTION("int")
        {
            v = 3;

            REQUIRE(v.get<int>() == 3);
        }

        SECTION("double")
        {
            v = 3.0;

            REQUIRE(v.get<double>() == 3.0);
        }

        SECTION("string")
        {
            v = std::string { "hello, world!" };

            REQUIRE(v.get<std::string>() == "hello, world!");
        }

        SECTION("int&")
        {
            int x = 3;

            // this should assign the `int`, not the `int&` as we didn't explicitly use std::ref
            v = x;
            REQUIRE(v.get<int&>().empty());
            REQUIRE(v.get<int>() == 3);
            x = 4;
            REQUIRE(v.get<int>() == 3);

            // now this should assign the `int&` value instead, because we used std::ref
            v = std::ref(x);
            REQUIRE(v.get<int>().empty());
            REQUIRE(v.get<int&>() == 4);
            x = 5;
            REQUIRE(v.get<int&>() == 5);
        }
    }
}

TEST_CASE("variant emplace", "[variant]")
{
    SECTION("value types")
    {
        variant<int, double, std::string> v { -2000 };

        SECTION("int")
        {
            v.emplace<int>(1);

            REQUIRE(v.get<int>() == 1);
        }

        SECTION("double")
        {
            v.emplace<double>(2.0);

            REQUIRE(v.get<double>() == 2.0);
        }

        SECTION("string")
        {
            v.emplace<std::string>(5, 'a');

            REQUIRE(v.get<std::string>() == "aaaaa");
        }
    }

    SECTION("reference types")
    {
        int dummy { -2500 };
        variant<int&, double&, std::string&> v { std::ref(dummy) };

        SECTION("int&")
        {
            int i = 4;
            v.emplace<int&>(i);

            REQUIRE(v.get<int&>().if_set([&](int& ref) { return &ref == &i; }) == true);
        }

        SECTION("double&")
        {
            double d = 5.0;
            v.emplace<double&>(d);

            REQUIRE(v.get<double&>().if_set([&](double& ref) { return &ref == &d; }) == true);
        }

        SECTION("string&")
        {
            std::string s { 5, 'b' };
            v.emplace<std::string&>(s);

            REQUIRE(v.get<std::string&>().if_set([&](std::string& ref) { return &ref == &s; }) == true);
        }
    }
}

TEST_CASE("variant visit", "[variant]")
{
    int i = 1;
    auto u = std::make_unique<std::string>(5, 'c');
    variant<int&, double, std::unique_ptr<std::string>&> v_i { std::ref(i) };
    variant<int&, double, std::unique_ptr<std::string>&> v_d { 2.0 };
    variant<int&, double, std::unique_ptr<std::string>&> v_s { std::ref(u) };

    SECTION("mutable l-value reference")
    {
        auto visitor = safet::overloaded {
            [](int&) {
                return 0;
            },
            [](double&) {
                return 1;
            },
            [](std::unique_ptr<std::string>&) {
                return 2;
            }
        };

        REQUIRE(v_i.visit(visitor) == 0);
        REQUIRE(v_d.visit(visitor) == 1);
        REQUIRE(v_s.visit(visitor) == 2);
    }

    SECTION("const l-value reference")
    {
        const auto& c_v_i = v_i;
        const auto& c_v_d = v_d;
        const auto& c_v_s = v_s;

        auto visitor = safet::overloaded {
            [](const int&) {
                return 0;
            },
            [](const double&) {
                return 1;
            },
            [](const std::unique_ptr<std::string>&) {
                return 2;
            }
        };

        REQUIRE(c_v_i.visit(visitor) == 0);
        REQUIRE(c_v_d.visit(visitor) == 1);
        REQUIRE(c_v_s.visit(visitor) == 2);
    }

    SECTION("r-value reference")
    {
        auto visitor = safet::overloaded {
            [](int&) {
                return 0;
            },
            [](double&&) {
                return 1;
            },
            [](std::unique_ptr<std::string>&) {
                return 2;
            }
        };

        REQUIRE(std::move(v_i).visit(visitor) == 0);
        REQUIRE(std::move(v_d).visit(visitor) == 1);
        REQUIRE(std::move(v_s).visit(visitor) == 2);
    }
}

TEST_CASE("variant get", "[variant]")
{
    variant<int, double, std::string> v_i { 1 };
    variant<int, double, std::string> v_d { 2.0 };
    variant<int, double, std::string> v_s { "ccccc" };

    SECTION("by type")
    {
        SECTION("mutable l-value")
        {
            REQUIRE(v_i.get<int>() == 1);
            REQUIRE(v_d.get<int>().empty());
            REQUIRE(v_s.get<int>().empty());

            REQUIRE(v_i.get<double>().empty());
            REQUIRE(v_d.get<double>() == 2.0);
            REQUIRE(v_s.get<double>().empty());

            REQUIRE(v_i.get<std::string>().empty());
            REQUIRE(v_d.get<std::string>().empty());
            REQUIRE(v_s.get<std::string>() == "ccccc");

            SECTION("prove mutability")
            {
                v_i.get<int>().if_set([](int& i) { i = 10; });
                v_d.get<double>().if_set([](double& d) { d = 20.0; });
                v_s.get<std::string>().if_set([](std::string& s) { s = "CCCCC"; });

                REQUIRE(v_i.get<int>() == 10);
                REQUIRE(v_d.get<double>() == 20.0);
                REQUIRE(v_s.get<std::string>() == "CCCCC");
            }
        }

        SECTION("const  l-value")
        {
            const auto& c_v_i = v_i;
            const auto& c_v_d = v_d;
            const auto& c_v_s = v_s;

            REQUIRE(c_v_i.get<int>() == 1);
            REQUIRE(c_v_d.get<int>().empty());
            REQUIRE(c_v_s.get<int>().empty());

            REQUIRE(c_v_i.get<double>().empty());
            REQUIRE(c_v_d.get<double>() == 2.0);
            REQUIRE(c_v_s.get<double>().empty());

            REQUIRE(c_v_i.get<std::string>().empty());
            REQUIRE(c_v_d.get<std::string>().empty());
            REQUIRE(c_v_s.get<std::string>() == "ccccc");
        }

        SECTION("r-value")
        {
            REQUIRE(std::move(v_i).get<int>() == 1);
            REQUIRE(std::move(v_d).get<int>().empty());
            REQUIRE(std::move(v_s).get<int>().empty());

            REQUIRE(std::move(v_i).get<double>().empty());
            REQUIRE(std::move(v_d).get<double>() == 2.0);
            REQUIRE(std::move(v_s).get<double>().empty());

            REQUIRE(std::move(v_i).get<std::string>().empty());
            REQUIRE(std::move(v_d).get<std::string>().empty());
            REQUIRE(std::move(v_s).get<std::string>() == "ccccc");

            SECTION("prove r-value-ness")
            {
                variant<std::unique_ptr<int>, std::unique_ptr<double>, std::unique_ptr<std::string>> u_v_i { std::make_unique<int>(2) };
                variant<std::unique_ptr<int>, std::unique_ptr<double>, std::unique_ptr<std::string>> u_v_d { std::make_unique<double>(2.0) };
                variant<std::unique_ptr<int>, std::unique_ptr<double>, std::unique_ptr<std::string>> u_v_s { std::make_unique<std::string>(5, 'z') };

                REQUIRE(std::move(u_v_i).get<std::unique_ptr<int>>().if_set([](auto ptr) { return *ptr == 2; }) == true);
                REQUIRE(std::move(u_v_d).get<std::unique_ptr<int>>().empty());
                REQUIRE(std::move(u_v_s).get<std::unique_ptr<int>>().empty());

                REQUIRE(std::move(u_v_i).get<std::unique_ptr<double>>().empty());
                REQUIRE(std::move(u_v_d).get<std::unique_ptr<double>>().if_set([](auto ptr) { return *ptr == 2.0; }) == true);
                REQUIRE(std::move(u_v_s).get<std::unique_ptr<double>>().empty());

                REQUIRE(std::move(u_v_i).get<std::unique_ptr<std::string>>().empty());
                REQUIRE(std::move(u_v_d).get<std::unique_ptr<std::string>>().empty());
                REQUIRE(std::move(u_v_s).get<std::unique_ptr<std::string>>().if_set([](auto ptr) { return *ptr == "zzzzz"; }) == true);
            }
        }
    }

    SECTION("by index")
    {
        SECTION("mutable l-value")
        {
            REQUIRE(v_i.get<0>() == 1);
            REQUIRE(v_d.get<0>().empty());
            REQUIRE(v_s.get<0>().empty());

            REQUIRE(v_i.get<1>().empty());
            REQUIRE(v_d.get<1>() == 2.0);
            REQUIRE(v_s.get<1>().empty());

            REQUIRE(v_i.get<2>().empty());
            REQUIRE(v_d.get<2>().empty());
            REQUIRE(v_s.get<2>() == "ccccc");

            SECTION("prove mutability")
            {
                v_i.get<0>().if_set([](int& i) { i = 10; });
                v_d.get<1>().if_set([](double& d) { d = 20.0; });
                v_s.get<2>().if_set([](std::string& s) { s = "CCCCC"; });

                REQUIRE(v_i.get<0>() == 10);
                REQUIRE(v_d.get<1>() == 20.0);
                REQUIRE(v_s.get<2>() == "CCCCC");
            }
        }

        SECTION("const  l-value")
        {
            const auto& c_v_i = v_i;
            const auto& c_v_d = v_d;
            const auto& c_v_s = v_s;

            REQUIRE(c_v_i.get<0>() == 1);
            REQUIRE(c_v_d.get<0>().empty());
            REQUIRE(c_v_s.get<0>().empty());

            REQUIRE(c_v_i.get<1>().empty());
            REQUIRE(c_v_d.get<1>() == 2.0);
            REQUIRE(c_v_s.get<1>().empty());

            REQUIRE(c_v_i.get<2>().empty());
            REQUIRE(c_v_d.get<2>().empty());
            REQUIRE(c_v_s.get<2>() == "ccccc");
        }

        SECTION("r-value")
        {
            REQUIRE(std::move(v_i).get<0>() == 1);
            REQUIRE(std::move(v_d).get<0>().empty());
            REQUIRE(std::move(v_s).get<0>().empty());

            REQUIRE(std::move(v_i).get<1>().empty());
            REQUIRE(std::move(v_d).get<1>() == 2.0);
            REQUIRE(std::move(v_s).get<1>().empty());

            REQUIRE(std::move(v_i).get<2>().empty());
            REQUIRE(std::move(v_d).get<2>().empty());
            REQUIRE(std::move(v_s).get<2>() == "ccccc");

            SECTION("prove r-value-ness")
            {
                variant<std::unique_ptr<int>, std::unique_ptr<double>, std::unique_ptr<std::string>> u_v_i { std::make_unique<int>(2) };
                variant<std::unique_ptr<int>, std::unique_ptr<double>, std::unique_ptr<std::string>> u_v_d { std::make_unique<double>(2.0) };
                variant<std::unique_ptr<int>, std::unique_ptr<double>, std::unique_ptr<std::string>> u_v_s { std::make_unique<std::string>(5, 'z') };

                REQUIRE(std::move(u_v_i).get<0>().if_set([](auto ptr) { return *ptr == 2; }) == true);
                REQUIRE(std::move(u_v_d).get<0>().empty());
                REQUIRE(std::move(u_v_s).get<0>().empty());

                REQUIRE(std::move(u_v_i).get<1>().empty());
                REQUIRE(std::move(u_v_d).get<1>().if_set([](auto ptr) { return *ptr == 2.0; }) == true);
                REQUIRE(std::move(u_v_s).get<1>().empty());

                REQUIRE(std::move(u_v_i).get<2>().empty());
                REQUIRE(std::move(u_v_d).get<2>().empty());
                REQUIRE(std::move(u_v_s).get<2>().if_set([](auto ptr) { return *ptr == "zzzzz"; }) == true);
            }
        }
    }
}

TEST_CASE("variant comparisons", "[variant]")
{
    // using many types, some which are nearly-conflicting in visit cases
    // (e.g. int& and int* are similar, int and const int& would be ambiguous overloads)
    variant<int, std::string, int, int&, const int&, int*> a, b;

    SECTION("a is less than b")
    {
        a.emplace<0>(1);
        b.emplace<0>(2);

        REQUIRE(a < b);
        REQUIRE(a <= b);
        REQUIRE_FALSE(a > b);
        REQUIRE_FALSE(a >= b);
        REQUIRE_FALSE(a == b);
        REQUIRE(a != b);
    }

    SECTION("a is equal to b")
    {
        a.emplace<0>(1);
        b.emplace<0>(1);

        REQUIRE_FALSE(a < b);
        REQUIRE(a <= b);
        REQUIRE_FALSE(a > b);
        REQUIRE(a >= b);
        REQUIRE(a == b);
        REQUIRE_FALSE(a != b);
    }

    SECTION("a is greater than b")
    {
        a.emplace<0>(2);
        b.emplace<0>(1);

        REQUIRE_FALSE(a < b);
        REQUIRE_FALSE(a <= b);
        REQUIRE(a > b);
        REQUIRE(a >= b);
        REQUIRE_FALSE(a == b);
        REQUIRE(a != b);
    }

    SECTION("a is a different type with a lower index")
    {
        a.emplace<0>(1);
        b = "Hello, World!";

        REQUIRE(a < b);
        REQUIRE(a <= b);
        REQUIRE_FALSE(a > b);
        REQUIRE_FALSE(a >= b);
        REQUIRE_FALSE(a == b);
        REQUIRE(a != b);
    }

    SECTION("a is a different type with a higher index")
    {
        a = "Hello, World!";
        b.emplace<0>(1);

        REQUIRE_FALSE(a < b);
        REQUIRE_FALSE(a <= b);
        REQUIRE(a > b);
        REQUIRE(a >= b);
        REQUIRE_FALSE(a == b);
        REQUIRE(a != b);
    }

    SECTION("a is the same type but with a lower index")
    {
        a.emplace<0>(1);
        b.emplace<2>(1);

        REQUIRE(a < b);
        REQUIRE(a <= b);
        REQUIRE_FALSE(a > b);
        REQUIRE_FALSE(a >= b);
        REQUIRE_FALSE(a == b);
        REQUIRE(a != b);
    }

    SECTION("a is the same type but with a higher index")
    {
        a.emplace<2>(1);
        b.emplace<0>(1);

        REQUIRE_FALSE(a < b);
        REQUIRE_FALSE(a <= b);
        REQUIRE(a > b);
        REQUIRE(a >= b);
        REQUIRE_FALSE(a == b);
        REQUIRE(a != b);
    }
}