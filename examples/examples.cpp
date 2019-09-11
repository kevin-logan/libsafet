#include <safet/safet.hpp>

#include <iostream>

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

auto main(int argc, char* argv[], char* envp[]) -> int
{
    (void)argc;
    (void)argv;
    (void)envp;

    use_memory();
    use_mutex();
    use_optional();

    return 0;
}