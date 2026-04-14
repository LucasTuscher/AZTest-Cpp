#pragma once

#include <random>
#include <string>
#include <type_traits>
#include <tuple>
#include <sstream>
#include "AZTest.h"

namespace AZTest {
namespace Property {

// Base generator
template <typename T, typename Enable = void>
struct Generator;

// Int generator
template <typename T>
struct Generator<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> {
    static T Generate(std::mt19937& rng) {
        std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
        return dist(rng);
    }
};

// Float generator
template <typename T>
struct Generator<T, std::enable_if_t<std::is_floating_point_v<T>>> {
    static T Generate(std::mt19937& rng) {
        std::uniform_real_distribution<T> dist(-1000.0, 1000.0);
        return dist(rng);
    }
};

// Boolean generator
template <>
struct Generator<bool> {
    static bool Generate(std::mt19937& rng) {
        std::uniform_int_distribution<int> dist(0, 1);
        return dist(rng) != 0;
    }
};

// String generator
template <>
struct Generator<std::string> {
    static std::string Generate(std::mt19937& rng) {
        std::uniform_int_distribution<int> lenDist(0, 100);
        std::uniform_int_distribution<int> charDist(32, 126);
        int len = lenDist(rng);
        std::string res;
        for (int i = 0; i < len; ++i) res += static_cast<char>(charDist(rng));
        return res;
    }
};

} // namespace Property
} // namespace AZTest

// PROPERTY_TEST Macro
#define AZProperty_TEST_CLASS_NAME(suite_name, test_name) \
  AZTEST_CONCAT3(AZTestPropReg_, suite_name, _##test_name)

#define PROPERTY_TEST(suite_name, test_name, runs, ...) \
    static void AZTEST_CONCAT3(AZTestPropFunc_, suite_name, _##test_name)(__VA_ARGS__); \
    namespace { \
        template<typename... Args> \
        struct RunPropTest { \
            static void Run(void(*func)(Args...), int iters) { \
                std::mt19937 rng(1337); \
                for (int i = 0; i < iters; ++i) { \
                    if (AZTest::Core::TestRegistry::Instance().CurrentTestFailed()) break; \
                    auto args = std::make_tuple(AZTest::Property::Generator<std::decay_t<Args>>::Generate(rng)...); \
                    std::string trace = "Property Test Iteration " + std::to_string(i+1) + " with args: " + AZTest::Detail::ToString(args); \
                    AZTest::Core::TestRegistry::Instance().PushTrace(trace); \
                    std::apply(func, args); \
                    AZTest::Core::TestRegistry::Instance().PopTrace(); \
                } \
            } \
        }; \
        template<typename R, typename... Args> \
        RunPropTest<Args...> DeduceArgs(R(*)(Args...)); \
        \
        struct AZProperty_TEST_CLASS_NAME(suite_name, test_name) { \
            AZProperty_TEST_CLASS_NAME(suite_name, test_name)() { \
                AZTest::Core::TestRegistry::Instance().RegisterTest( \
                    #test_name, #suite_name, \
                    []() { \
                        decltype(DeduceArgs(AZTEST_CONCAT3(AZTestPropFunc_, suite_name, _##test_name)))::Run(AZTEST_CONCAT3(AZTestPropFunc_, suite_name, _##test_name), runs); \
                    }, \
                    __FILE__, __LINE__); \
            } \
        }; \
        static AZProperty_TEST_CLASS_NAME(suite_name, test_name) \
            AZTEST_UNIQUE_NAME(engineTestPropRegInst_); \
    } \
    static void AZTEST_CONCAT3(AZTestPropFunc_, suite_name, _##test_name)(__VA_ARGS__)
