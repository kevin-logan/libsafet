#include <safet/safet.hpp>

#include <iostream>
#include <list>
#include <vector>

static size_t stol_mine_counter{ 0 };
auto stol_mine(const std::string& str) -> int64_t
{
    ++stol_mine_counter;
    return std::stol(str);
}

auto test_join() -> void
{
    std::vector<int64_t> vec;

    for (size_t i = 0; i < 100000; ++i) {
        vec.emplace_back(rand() % 1000);
    }

    auto vec_2 = vec;

    auto start = std::chrono::high_resolution_clock::now();
    std::partition(vec.begin(), vec.end(), [](const auto& val) { return val > 500; });
    auto middle = std::chrono::high_resolution_clock::now();
    vec = safet::range{ std::move(vec) }.filter([](const auto& val) { return val <= 500; }).join(safet::range{ std::move(vec) }.filter([](const auto& val) { return val > 500; })).collect<std::vector>();
    auto end = std::chrono::high_resolution_clock::now();

    auto stl_us = std::chrono::duration_cast<std::chrono::microseconds>(middle - start).count();
    auto safet_us = std::chrono::duration_cast<std::chrono::microseconds>(end - middle).count();

    std::cout << "stl partition took " << stl_us << "us" << std::endl;
    std::cout << "safet partition took " << safet_us << "us" << std::endl;
    std::cout << static_cast<double>(safet_us) / static_cast<double>(stl_us) << std::endl;
}

auto container_test_stl_way(const std::vector<std::string>& vec) -> std::pair<std::chrono::nanoseconds, std::list<int64_t>>
{

    auto start = std::chrono::high_resolution_clock::now();

    // goal is to convert "item_XXX" to the number XXX, then remove odd numbers, square the results, and put them into an std::list
    std::list<int64_t> output;
    for (const auto& item : vec) {
        int64_t number = stol_mine(item.substr(5));

        if ((number % 2) == 0) {
            output.push_back(number * number);
        }
    }

    auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start);
    return std::make_pair(time, std::move(output));
}

auto container_test_safet_way(const std::vector<std::string>& stl_vec) -> std::pair<std::chrono::nanoseconds, std::list<int64_t>>
{
    safet::range r{ stl_vec };
    auto start = std::chrono::high_resolution_clock::now();

    auto list = r.map([](const auto& str) { return stol_mine(str.substr(5)); }).filter([](int64_t item) { return (item % 2) == 0; }).map([](int64_t item) { return item * item; }).collect<std::list>();

    auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start);
    return std::make_pair(time, std::move(list));
}

auto use_container() -> void
{
    test_join();

    size_t stl_stol_calls{ 0 };
    size_t safet_stol_calls{ 0 };
    std::chrono::nanoseconds stl_total{ 0 };
    std::chrono::nanoseconds safet_total{ 0 };
    for (size_t i = 0; i < 500; ++i) {
        std::vector<std::string> vec;

        for (size_t i = 0; i < 10000; ++i) {
            vec.push_back("item_" + std::to_string(rand() % 100));
        }

        auto stl_result = container_test_stl_way(vec);
        stl_stol_calls += stol_mine_counter;
        stol_mine_counter = 0;
        auto safet_result = container_test_safet_way(vec);
        safet_stol_calls += stol_mine_counter;
        stol_mine_counter = 0;

        stl_total += stl_result.first;
        safet_total += safet_result.first;

        if (stl_result.second != safet_result.second) {
            std::cout << "ERROR IN TEST " << i << ", RESULTS DID NOT MATCH" << std::endl;
        }
    }

    std::cout << "stl_total: " << std::chrono::duration_cast<std::chrono::microseconds>(stl_total).count() << "us with " << stl_stol_calls << " stol calls" << std::endl;
    std::cout << "safet_total: " << std::chrono::duration_cast<std::chrono::microseconds>(safet_total).count() << "us with " << safet_stol_calls << " stol calls" << std::endl;
    std::cout << (static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(safet_total).count()) / static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(stl_total).count())) * 100.0 << "%" << std::endl;

    // let's test uncopiable types
    std::vector<std::unique_ptr<int64_t>>
        vec_unique;
    vec_unique.reserve(100);
    for (size_t i = 0; i < 100; ++i) {
        vec_unique.push_back(std::make_unique<int64_t>(rand() % 100));
    }

    auto fold_result = safet::range{ std::move(vec_unique) }.filter([](const std::unique_ptr<int64_t>& int_ptr) { return *int_ptr > 50; }).map([](std::unique_ptr<int64_t> int_ptr) { return std::make_unique<std::string>("random_" + std::to_string(*int_ptr)); }).filter([](const std::unique_ptr<std::string>& str) { return str->back() != '3'; }).fold([](std::string complete_value, auto&& incoming_value) { complete_value.append(*incoming_value); return complete_value; }, std::string{});
    std::cout << "unique_ptr range folded to a string of length: " << fold_result.size() << std::endl;
}

auto use_memory() -> void
{
    safet::weak_ptr<int> w_ptr;
    {
        auto u_ptr = safet::make_unique<int>(5);
        auto s_ptr = safet::make_shared<int>(1337);
        w_ptr = s_ptr;

        u_ptr.deref_or([](int val) { std::cout << "u_ptr expectedly had value: " << val << std::endl; }, []() { std::cout << "u_ptr unexpectedly was nullptr" << std::endl; });
        s_ptr.deref_or([](int val) { std::cout << "s_ptr expectedly had value: " << val << std::endl; }, []() { std::cout << "s_ptr unexpectedly was nullptr" << std::endl; });
        w_ptr.lock().deref_or([](int val) { std::cout << "w_ptr (pre) expectedly had value: " << val << std::endl; }, []() { std::cout << "w_ptr (pre) unexpectedly was nullptr" << std::endl; });
    }

    w_ptr.lock().deref_or([](int val) { std::cout << "w_ptr (post) unexpectedly had value: " << val << std::endl; }, []() { std::cout << "w_ptr (post) expectedly was nullptr" << std::endl; });
}

auto use_mutex() -> void
{
    safet::mutex<int> mutex{ 1337 };

    mutex.acquire([&mutex](int& val) {
        std::cout << "acquire expectedly locked first with value: " << val << std::endl;

        // try to lock again, this should fail
        mutex.try_acquire(
            [](int& val_2) {
                std::cout << "try_acquire unexpectedly locked again with value: " << val_2 << std::endl;
            },
            []() {
                std::cout << "try_acquire expectedly failed to lock" << std::endl;
            });
    });

    mutex.try_acquire(
        [](int& val) {
            std::cout << "try_acquire expectedly locked with value: " << val << std::endl;
        },
        []() {
            std::cout << "try_acquire unexpectedly failed to lock" << std::endl;
        });
}

auto use_optional() -> void
{
    safet::optional<int> opt;

    opt.handle([](int val) { std::cout << "optional had unexpected value: " << val << std::endl; }, []() { std::cout << "optional expectedly had no value" << std::endl; });

    opt = 5;

    opt.handle([](int val) { std::cout << "optional had expected value: " << val << std::endl; }, []() { std::cout << "optional unexpectedly had no value" << std::endl; });
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
    safet::variant<int, std::string, safet::optional<double>> var{ 1337 };

    auto copy = var;
    auto move{ std::move(copy) };

    move.if_set<int>([&var](int val1) {
        var.if_set_else<int>([val1](int val2) {
            if (val1 == val2) {
                std::cout << "both values equal as expected: " << val1 << std::endl;
            } else {
                std::cout << val1 << " was unexpectedly not equal to " << val2 << std::endl;
            } }, [val1]() { std::cout << "var unexpectedly did not contain an int" << std::endl; });
    });

    var = safet::optional{ 13.37 };
    var.if_set<safet::optional<double>>(
        [](auto& opt_double) {
            opt_double.if_set([](double val) { std::cout << "var had optional double with value " << val << ", expected 13.37" << std::endl; });
        });
}

auto main(int argc, char* argv[], char* envp[]) -> int
{
    (void)argc;
    (void)argv;
    (void)envp;

    use_container();
    //use_memory();
    //use_mutex();
    //use_optional();
    //use_pack();
    //use_variant();

    return 0;
}