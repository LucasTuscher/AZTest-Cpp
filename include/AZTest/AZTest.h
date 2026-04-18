#pragma once

/**
 * @file AZTest.h
 * @brief Main header file for Engine Test Framework
 *
 * Include this file to use the test framework.
 */

#include "Core/TestResult.h"
#include "Core/TestFixture.h"
#include "Core/TestRegistry.h"
#include "Reporters/IReporter.h"
#include "Reporters/ConsoleReporter.h"
#include "Reporters/XMLReporter.h"
#include "Reporters/JSONReporter.h"
#include "Reporters/CallbackReporter.h"
#include "Utils/Benchmark.h"
#include "Matchers.h"
#include "Mock.h"
#include "PropertyTest.h"
#include "Environment.h"

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <iterator>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <cmath>
#include <limits>
#include <csignal>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// ============================================================================
// HELPER MACROS
// ============================================================================

#define AZTEST_CONCAT_IMPL(a, b) a##b
#define AZTEST_CONCAT(a, b) AZTEST_CONCAT_IMPL(a, b)
#define AZTEST_CONCAT3(a, b, c) AZTEST_CONCAT(a, AZTEST_CONCAT(b, c))
#define AZTEST_UNIQUE_NAME(base) AZTEST_CONCAT(base, __LINE__)

namespace AZTest {
namespace Detail {

/**
 * @brief Derive a suite name from the current file path.
 *
 * This keeps per-file suites without requiring boilerplate.
 */
inline std::string SuiteFromFile(const char* file) {
    if (!file) {
        return "DefaultSuite";
    }

    const char* slash = std::strrchr(file, '/');
    const char* backslash = std::strrchr(file, '\\');
    const char* start = file;

    if (slash && backslash) {
        start = (slash > backslash) ? slash + 1 : backslash + 1;
    } else if (slash) {
        start = slash + 1;
    } else if (backslash) {
        start = backslash + 1;
    }

    std::string name(start);
    auto dotPos = name.find_last_of('.');
    if (dotPos != std::string::npos) {
        name = name.substr(0, dotPos);
    }

    return name.empty() ? "DefaultSuite" : name;
}

template <typename, typename = void>
struct HasBegin : std::false_type {};

template <typename T>
struct HasBegin<T, std::void_t<decltype(std::begin(std::declval<T>())),
                               decltype(std::end(std::declval<T>()))>> : std::true_type {};

template <typename, typename = void>
struct HasSize : std::false_type {};

template <typename T>
struct HasSize<T, std::void_t<decltype(std::declval<T>().size())>> : std::true_type {};

template <typename T> struct IsPair : std::false_type {};
template <typename T1, typename T2> struct IsPair<std::pair<T1, T2>> : std::true_type {};

template <typename T> struct IsTuple : std::false_type {};
template <typename... Args> struct IsTuple<std::tuple<Args...>> : std::true_type {};

template <typename Tuple, size_t... Is>
inline std::string TupleToStringImpl(const Tuple& t, std::index_sequence<Is...>) {
    std::ostringstream oss;
    oss << "(";
    ((oss << (Is == 0 ? "" : ", ") << AZTest::Detail::ToString(std::get<Is>(t))), ...);
    oss << ")";
    return oss.str();
}

template <typename... Args>
inline std::string TupleToString(const std::tuple<Args...>& t) {
    return TupleToStringImpl(t, std::make_index_sequence<sizeof...(Args)>{});
}

template <typename T>
std::string ToString(const T& value);

template <typename T>
inline std::string JoinContainer(const T& container, const char* sep = ", ") {
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    for (const auto& v : container) {
        if (!first) oss << sep;
        first = false;
        oss << AZTest::Detail::ToString(v);
    }
    oss << "]";
    return oss.str();
}

template <typename T>
inline std::string ToString(const T& value) {
    using Decayed = std::decay_t<T>;
    if constexpr (std::is_same_v<Decayed, std::string>) {
        return "\"" + value + "\"";
    } else if constexpr (std::is_same_v<Decayed, const char*> ||
                         std::is_same_v<Decayed, char*> ||
                         std::is_same_v<Decayed, const unsigned char*> ||
                         std::is_same_v<Decayed, unsigned char*>) {
        return "\"" + std::string(value) + "\"";
    } else if constexpr (std::is_same_v<Decayed, char>) {
        return std::string("'") + value + "'";
    } else if constexpr (std::is_same_v<Decayed, bool>) {
        return value ? "true" : "false";
    } else if constexpr (HasBegin<Decayed>::value && !std::is_same_v<Decayed, std::string>) {
        return JoinContainer(value);
    } else if constexpr (Detail::IsPair<Decayed>::value) {
        return "(" + AZTest::Detail::ToString(value.first) + ", " + AZTest::Detail::ToString(value.second) + ")";
    } else if constexpr (Detail::IsTuple<Decayed>::value) {
        return Detail::TupleToString(value);
    } else {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }
}

template <typename Range>
inline size_t RangeSize(const Range& r) {
    using R = std::decay_t<Range>;
    if constexpr (HasSize<R>::value) {
        return static_cast<size_t>(r.size());
    } else {
        return static_cast<size_t>(std::distance(std::begin(r), std::end(r)));
    }
}

template <typename Haystack, typename Needle>
inline bool Contains(const Haystack& haystack, const Needle& needle) {
    if constexpr (std::is_convertible_v<Haystack, std::string>) {
        return std::string(haystack).find(needle) != std::string::npos;
    } else if constexpr (HasBegin<Haystack>::value) {
        for (const auto& v : haystack) {
            if (v == needle) return true;
        }
        return false;
    } else {
        return false;
    }
}

inline bool StartsWith(const std::string& value, const std::string& prefix) {
    if (prefix.size() > value.size()) return false;
    return std::equal(prefix.begin(), prefix.end(), value.begin());
}

inline bool EndsWith(const std::string& value, const std::string& suffix) {
    if (suffix.size() > value.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
}

inline int Strcasecmp(const char* s1, const char* s2) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    int result;
    if (p1 == p2)
        return 0;
    while ((result = tolower(*p1) - tolower(*p2++)) == 0)
        if (*p1++ == '\0')
            break;
    return result;
}

} // namespace Detail

namespace Core {
// Manages a trace message for the current scope
class ScopedTrace {
public:
    ScopedTrace(const char* file, int line, const std::string& message) {
        std::stringstream ss;
        ss << file << ":" << line << ": " << message;
        TestRegistry::Instance().PushTrace(ss.str());
    }
    ~ScopedTrace() {
        TestRegistry::Instance().PopTrace();
    }
};
} // namespace Core

} // namespace AZTest

#define SCOPED_TRACE(message) \
    AZTest::Core::ScopedTrace AZTEST_UNIQUE_NAME(scoped_trace_)(__FILE__, __LINE__, (message))

// ============================================================================
// TEST MACROS
// ============================================================================

/**
 * @brief Define a simple test
 *
 * Usage:
 * TEST(SuiteName, TestName) {
 *     EXPECT_EQ(1, 1);
 * }
 */
#define TEST(suite_name, test_name) \
    static void AZTEST_UNIQUE_NAME(AZTestFunc_)(); \
    namespace { \
        struct AZTEST_UNIQUE_NAME(AZTestRegistrar_) { \
            AZTEST_UNIQUE_NAME(AZTestRegistrar_)() { \
                AZTest::Core::TestRegistry::Instance().RegisterTest( \
                    #test_name, #suite_name, \
                    AZTEST_UNIQUE_NAME(AZTestFunc_), \
                    __FILE__, __LINE__); \
            } \
        }; \
        static AZTEST_UNIQUE_NAME(AZTestRegistrar_) \
            AZTEST_UNIQUE_NAME(engineTestRegistrarInstance_); \
    } \
    static void AZTEST_UNIQUE_NAME(AZTestFunc_)()

/**
 * @brief Define a test with a fixture
 *
 * Usage:
 * class MyFixture : public TestFixture {
 * protected:
 *     void SetUp() override { ... }
 *     void TearDown() override { ... }
 * };
 *
 * TEST_F(MyFixture, TestName) {
 *     EXPECT_TRUE(true);
 * }
 */
#define TEST_F(fixture_class, test_name) \
    class AZTEST_UNIQUE_NAME(FixtureTest_) : public fixture_class { \
    public: \
        void RunTest(); \
        void RunWithLifecycle() { \
            bool setUpDone = false; \
            try { \
                SetUp(); \
                setUpDone = true; \
                RunTest(); \
                TearDown(); \
            } catch (...) { \
                if (setUpDone) { \
                    try { TearDown(); } catch (...) {} \
                } \
                throw; \
            } \
        } \
    }; \
    namespace { \
        struct AZTEST_UNIQUE_NAME(FixtureTestRegistrar_) { \
            AZTEST_UNIQUE_NAME(FixtureTestRegistrar_)() { \
                auto runFunc = []() { \
                    AZTEST_UNIQUE_NAME(FixtureTest_) fixture; \
                    fixture.RunWithLifecycle(); \
                }; \
                auto setUpFunc = []() { \
                    fixture_class::SetUpTestSuite(); \
                }; \
                auto tearDownFunc = []() { \
                    fixture_class::TearDownTestSuite(); \
                }; \
                AZTest::Core::TestRegistry::Instance().RegisterFixtureTest( \
                    #test_name, #fixture_class, \
                    runFunc, setUpFunc, tearDownFunc, \
                    __FILE__, __LINE__); \
            } \
        }; \
        static AZTEST_UNIQUE_NAME(FixtureTestRegistrar_) \
            AZTEST_UNIQUE_NAME(fixtureTestRegistrarInstance_); \
    } \
    void AZTEST_UNIQUE_NAME(FixtureTest_)::RunTest()

/**
 * @brief Define a test using the current file name as suite.
 *
 * Usage:
 * TEST_CASE(TestName) { ... }
 */
#define TEST_CASE(test_name) \
    static void AZTEST_UNIQUE_NAME(AZTestFunc_)(); \
    namespace { \
        struct AZTEST_UNIQUE_NAME(AZTestRegistrar_) { \
            AZTEST_UNIQUE_NAME(AZTestRegistrar_)() { \
                AZTest::Core::TestRegistry::Instance().RegisterTest( \
                    #test_name, AZTest::Detail::SuiteFromFile(__FILE__), \
                    AZTEST_UNIQUE_NAME(AZTestFunc_), \
                    __FILE__, __LINE__); \
            } \
        }; \
        static AZTEST_UNIQUE_NAME(AZTestRegistrar_) \
            AZTEST_UNIQUE_NAME(engineTestRegistrarInstance_); \
    } \
    static void AZTEST_UNIQUE_NAME(AZTestFunc_)()

// Parametrized tests (value-based, no fixture)
#define AZTEST_PARAM_FUNC_NAME(suite_name, test_name) AZTEST_CONCAT3(AZTestParamFunc_, suite_name, _##test_name)
#define AZTEST_PARAM_REG_NAME(suite_name, test_name) AZTEST_CONCAT3(AZTestParamRegistrar_, suite_name, _##test_name)
#define AZTEST_PARAM_TYPE_NAME(suite_name, test_name) AZTEST_CONCAT3(AZTestParamType_, suite_name, _##test_name)

#define TEST_P(suite_name, test_name, param_type) \
    static void AZTEST_PARAM_FUNC_NAME(suite_name, test_name)(const param_type& param); \
    namespace { \
        using AZTEST_PARAM_TYPE_NAME(suite_name, test_name) = param_type; \
        struct AZTEST_PARAM_REG_NAME(suite_name, test_name) { \
            static void Register(const char* instanceName, const std::vector<param_type>& params) { \
                for (size_t idx = 0; idx < params.size(); ++idx) { \
                    param_type paramCopy = params[idx]; \
                    std::string name = std::string(#test_name) + "/" + instanceName + "/" + std::to_string(idx + 1) + ":" + AZTest::Detail::ToString(paramCopy); \
                    AZTest::Core::TestRegistry::Instance().RegisterTest( \
                        name, #suite_name, \
                        [paramCopy]() { AZTEST_PARAM_FUNC_NAME(suite_name, test_name)(paramCopy); }, \
                        __FILE__, __LINE__); \
                } \
            } \
        }; \
    } \
    static void AZTEST_PARAM_FUNC_NAME(suite_name, test_name)(const param_type& param)

#define INSTANTIATE_TEST_SUITE_P(prefix, suite_name, test_name, ...) \
    namespace { \
        struct AZTEST_UNIQUE_NAME(AZTestParamInstantiation_) { \
            AZTEST_UNIQUE_NAME(AZTestParamInstantiation_)() { \
                using ParamType = AZTEST_PARAM_TYPE_NAME(suite_name, test_name); \
                std::vector<ParamType> params = { __VA_ARGS__ }; \
                AZTEST_PARAM_REG_NAME(suite_name, test_name)::Register(#prefix, params); \
            } \
        }; \
        static AZTEST_UNIQUE_NAME(AZTestParamInstantiation_) AZTEST_UNIQUE_NAME(engineTestParamInstance_); \
    }

// Type-parametrized tests
#define TYPED_TEST_SUITE(suite_name, ...) \
    using AZTEST_CONCAT(AZTestTypeSuite_, suite_name) = std::tuple<__VA_ARGS__>; \
    template <typename T> class suite_name : public AZTest::Core::TestFixture {}

#define TYPED_TEST(suite_name, test_name) \
    template <typename T> \
    class AZTEST_PARAM_FUNC_NAME(suite_name, test_name) : public suite_name<T> { \
    public: \
        void RunTest(); \
        void RunWithLifecycle() { \
            bool setUpDone = false; \
            try { this->SetUp(); setUpDone = true; RunTest(); this->TearDown(); } \
            catch (...) { if (setUpDone) { try { this->TearDown(); } catch(...) {} } throw; } \
        } \
    }; \
    namespace { \
        template<typename Tuple, std::size_t... I> \
        void AZTEST_CONCAT3(AZTestTypeReg_, suite_name, _##test_name)(std::index_sequence<I...>) { \
            (AZTest::Core::TestRegistry::Instance().RegisterTest( \
                std::string(#test_name) + "<" + typeid(std::tuple_element_t<I, Tuple>).name() + ">", \
                #suite_name, \
                []() { \
                    AZTEST_PARAM_FUNC_NAME(suite_name, test_name)<std::tuple_element_t<I, Tuple>> fixture; \
                    fixture.RunWithLifecycle(); \
                }, \
                __FILE__, __LINE__ \
            ), ...); \
        } \
        struct AZTEST_PARAM_REG_NAME(suite_name, test_name) { \
            AZTEST_PARAM_REG_NAME(suite_name, test_name)() { \
                using Types = AZTEST_CONCAT(AZTestTypeSuite_, suite_name); \
                AZTEST_CONCAT3(AZTestTypeReg_, suite_name, _##test_name)<Types>( \
                    std::make_index_sequence<std::tuple_size_v<Types>>{} \
                ); \
            } \
        }; \
        static AZTEST_PARAM_REG_NAME(suite_name, test_name) AZTEST_UNIQUE_NAME(typeTestInst_); \
    } \
    template <typename TypeParam> \
    void AZTEST_PARAM_FUNC_NAME(suite_name, test_name)<TypeParam>::RunTest()

// ============================================================================
// EXPECT MACROS (Continue on failure)
// ============================================================================

#define TEST_SKIP(reason) \
    do { \
        AZTest::Core::TestRegistry::Instance().RecordSkip((reason), __FILE__, __LINE__); \
        return; \
    } while(0)

#define EXPECT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::stringstream ss; \
            ss << "Expected TRUE but got FALSE: " #condition; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_FALSE(condition) \
    do { \
        if (condition) { \
            std::stringstream ss; \
            ss << "Expected FALSE but got TRUE: " #condition; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_EQ(val1, val2) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val1_) = (val1); \
        auto AZTEST_UNIQUE_NAME(et_val2_) = (val2); \
        if (!(AZTEST_UNIQUE_NAME(et_val1_) == AZTEST_UNIQUE_NAME(et_val2_))) { \
            std::stringstream ss; \
            ss << "Expected equality: " #val1 " == " #val2 "\n" \
               << "      Actual: " << AZTEST_UNIQUE_NAME(et_val1_) << " != " << AZTEST_UNIQUE_NAME(et_val2_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_NE(val1, val2) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val1_) = (val1); \
        auto AZTEST_UNIQUE_NAME(et_val2_) = (val2); \
        if (AZTEST_UNIQUE_NAME(et_val1_) == AZTEST_UNIQUE_NAME(et_val2_)) { \
            std::stringstream ss; \
            ss << "Expected inequality: " #val1 " != " #val2 "\n" \
               << "      Actual: " << AZTEST_UNIQUE_NAME(et_val1_) << " == " << AZTEST_UNIQUE_NAME(et_val2_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_LT(val1, val2) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val1_) = (val1); \
        auto AZTEST_UNIQUE_NAME(et_val2_) = (val2); \
        if (!(AZTEST_UNIQUE_NAME(et_val1_) < AZTEST_UNIQUE_NAME(et_val2_))) { \
            std::stringstream ss; \
            ss << "Expected: " #val1 " < " #val2 "\n" \
               << "      Actual: " << AZTEST_UNIQUE_NAME(et_val1_) << " >= " << AZTEST_UNIQUE_NAME(et_val2_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_LE(val1, val2) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val1_) = (val1); \
        auto AZTEST_UNIQUE_NAME(et_val2_) = (val2); \
        if (!(AZTEST_UNIQUE_NAME(et_val1_) <= AZTEST_UNIQUE_NAME(et_val2_))) { \
            std::stringstream ss; \
            ss << "Expected: " #val1 " <= " #val2 "\n" \
               << "      Actual: " << AZTEST_UNIQUE_NAME(et_val1_) << " > " << AZTEST_UNIQUE_NAME(et_val2_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_GT(val1, val2) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val1_) = (val1); \
        auto AZTEST_UNIQUE_NAME(et_val2_) = (val2); \
        if (!(AZTEST_UNIQUE_NAME(et_val1_) > AZTEST_UNIQUE_NAME(et_val2_))) { \
            std::stringstream ss; \
            ss << "Expected: " #val1 " > " #val2 "\n" \
               << "      Actual: " << AZTEST_UNIQUE_NAME(et_val1_) << " <= " << AZTEST_UNIQUE_NAME(et_val2_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_GE(val1, val2) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val1_) = (val1); \
        auto AZTEST_UNIQUE_NAME(et_val2_) = (val2); \
        if (!(AZTEST_UNIQUE_NAME(et_val1_) >= AZTEST_UNIQUE_NAME(et_val2_))) { \
            std::stringstream ss; \
            ss << "Expected: " #val1 " >= " #val2 "\n" \
               << "      Actual: " << AZTEST_UNIQUE_NAME(et_val1_) << " < " << AZTEST_UNIQUE_NAME(et_val2_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_STREQ(str1, str2) \
    do { \
        std::string AZTEST_UNIQUE_NAME(et_str1_) = (str1); \
        std::string AZTEST_UNIQUE_NAME(et_str2_) = (str2); \
        if (AZTEST_UNIQUE_NAME(et_str1_) != AZTEST_UNIQUE_NAME(et_str2_)) { \
            std::stringstream ss; \
            ss << "Expected string equality:\n" \
               << "      Expected: \"" << AZTEST_UNIQUE_NAME(et_str2_) << "\"\n" \
               << "      Actual:   \"" << AZTEST_UNIQUE_NAME(et_str1_) << "\""; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_STRNE(str1, str2) \
    do { \
        if (0 == strcmp((str1), (str2))) { \
            std::stringstream ss; \
            ss << "Expected string inequality:\n" \
               << "      Expected: \"" << (str2) << "\"\n" \
               << "      Actual:   \"" << (str1) << "\""; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_STRCASEEQ(str1, str2) \
    do { \
        if (0 != AZTest::Detail::Strcasecmp((str1), (str2))) { \
            std::stringstream ss; \
            ss << "Expected case-insensitive string equality:\n" \
               << "      Expected: \"" << (str2) << "\"\n" \
               << "      Actual:   \"" << (str1) << "\""; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_STRCASENE(str1, str2) \
    do { \
        if (0 == AZTest::Detail::Strcasecmp((str1), (str2))) { \
            std::stringstream ss; \
            ss << "Expected case-insensitive string inequality:\n" \
               << "      Expected: \"" << (str2) << "\"\n" \
               << "      Actual:   \"" << (str1) << "\""; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_NEAR(val1, val2, abs_error) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val1_) = (val1); \
        auto AZTEST_UNIQUE_NAME(et_val2_) = (val2); \
        auto AZTEST_UNIQUE_NAME(et_diff_) = (AZTEST_UNIQUE_NAME(et_val1_) > AZTEST_UNIQUE_NAME(et_val2_)) ? (AZTEST_UNIQUE_NAME(et_val1_) - AZTEST_UNIQUE_NAME(et_val2_)) : (AZTEST_UNIQUE_NAME(et_val2_) - AZTEST_UNIQUE_NAME(et_val1_)); \
        if (AZTEST_UNIQUE_NAME(et_diff_) > (abs_error)) { \
            std::stringstream ss; \
            ss << "Expected: " #val1 " near " #val2 " (within " << (abs_error) << ")\n" \
               << "      Actual: " << AZTEST_UNIQUE_NAME(et_val1_) << " vs " << AZTEST_UNIQUE_NAME(et_val2_) << " (diff: " << AZTEST_UNIQUE_NAME(et_diff_) << ")"; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_FLOAT_EQ(val1, val2) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val1_) = static_cast<float>(val1); \
        auto AZTEST_UNIQUE_NAME(et_val2_) = static_cast<float>(val2); \
        auto AZTEST_UNIQUE_NAME(et_diff_) = std::fabs(AZTEST_UNIQUE_NAME(et_val1_) - AZTEST_UNIQUE_NAME(et_val2_)); \
        float AZTEST_UNIQUE_NAME(et_max_) = std::max(std::fabs(AZTEST_UNIQUE_NAME(et_val1_)), std::fabs(AZTEST_UNIQUE_NAME(et_val2_))); \
        float AZTEST_UNIQUE_NAME(et_tol_) = 4.0f * std::numeric_limits<float>::epsilon() * AZTEST_UNIQUE_NAME(et_max_); \
        if (AZTEST_UNIQUE_NAME(et_tol_) < std::numeric_limits<float>::epsilon()) AZTEST_UNIQUE_NAME(et_tol_) = std::numeric_limits<float>::epsilon(); \
        if (AZTEST_UNIQUE_NAME(et_diff_) > AZTEST_UNIQUE_NAME(et_tol_)) { \
            std::stringstream ss; \
            ss << "Expected float equality: " #val1 " == " #val2 "\n" \
               << "      Actual: " << AZTEST_UNIQUE_NAME(et_val1_) << " vs " << AZTEST_UNIQUE_NAME(et_val2_) << " (diff: " << AZTEST_UNIQUE_NAME(et_diff_) << ", tol: " << AZTEST_UNIQUE_NAME(et_tol_) << ")"; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_DOUBLE_EQ(val1, val2) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val1_) = static_cast<double>(val1); \
        auto AZTEST_UNIQUE_NAME(et_val2_) = static_cast<double>(val2); \
        auto AZTEST_UNIQUE_NAME(et_diff_) = std::fabs(AZTEST_UNIQUE_NAME(et_val1_) - AZTEST_UNIQUE_NAME(et_val2_)); \
        double AZTEST_UNIQUE_NAME(et_max_) = std::max(std::fabs(AZTEST_UNIQUE_NAME(et_val1_)), std::fabs(AZTEST_UNIQUE_NAME(et_val2_))); \
        double AZTEST_UNIQUE_NAME(et_tol_) = 4.0 * std::numeric_limits<double>::epsilon() * AZTEST_UNIQUE_NAME(et_max_); \
        if (AZTEST_UNIQUE_NAME(et_tol_) < std::numeric_limits<double>::epsilon()) AZTEST_UNIQUE_NAME(et_tol_) = std::numeric_limits<double>::epsilon(); \
        if (AZTEST_UNIQUE_NAME(et_diff_) > AZTEST_UNIQUE_NAME(et_tol_)) { \
            std::stringstream ss; \
            ss << "Expected double equality: " #val1 " == " #val2 "\n" \
               << "      Actual: " << AZTEST_UNIQUE_NAME(et_val1_) << " vs " << AZTEST_UNIQUE_NAME(et_val2_) << " (diff: " << AZTEST_UNIQUE_NAME(et_diff_) << ", tol: " << AZTEST_UNIQUE_NAME(et_tol_) << ")"; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_NAN(val) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val_) = (val); \
        if (!std::isnan(AZTEST_UNIQUE_NAME(et_val_))) { \
            std::stringstream ss; \
            ss << "Expected NaN, got: " << AZTEST_UNIQUE_NAME(et_val_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_INF(val) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val_) = (val); \
        if (!std::isinf(AZTEST_UNIQUE_NAME(et_val_)) || AZTEST_UNIQUE_NAME(et_val_) < 0) { \
            std::stringstream ss; \
            ss << "Expected positive infinity, got: " << AZTEST_UNIQUE_NAME(et_val_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_NINF(val) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val_) = (val); \
        if (!std::isinf(AZTEST_UNIQUE_NAME(et_val_)) || AZTEST_UNIQUE_NAME(et_val_) > 0) { \
            std::stringstream ss; \
            ss << "Expected negative infinity, got: " << AZTEST_UNIQUE_NAME(et_val_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_FINITE(val) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val_) = (val); \
        if (std::isinf(AZTEST_UNIQUE_NAME(et_val_)) || std::isnan(AZTEST_UNIQUE_NAME(et_val_))) { \
            std::stringstream ss; \
            ss << "Expected finite value, got: " << AZTEST_UNIQUE_NAME(et_val_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_CONTAINS(haystack, needle) \
    do { \
        auto&& AZTEST_UNIQUE_NAME(et_h_) = (haystack); \
        auto&& AZTEST_UNIQUE_NAME(et_n_) = (needle); \
        if (!AZTest::Detail::Contains(AZTEST_UNIQUE_NAME(et_h_), AZTEST_UNIQUE_NAME(et_n_))) { \
            std::stringstream ss; \
            ss << "Expected container/string to contain element\n" \
               << "      Haystack: " << AZTest::Detail::ToString(AZTEST_UNIQUE_NAME(et_h_)) << "\n" \
               << "      Needle:   " << AZTest::Detail::ToString(AZTEST_UNIQUE_NAME(et_n_)); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_STARTS_WITH(str, prefix) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_str_) = std::string(str); \
        auto AZTEST_UNIQUE_NAME(et_pre_) = std::string(prefix); \
        if (!AZTest::Detail::StartsWith(AZTEST_UNIQUE_NAME(et_str_), AZTEST_UNIQUE_NAME(et_pre_))) { \
            std::stringstream ss; \
            ss << "Expected string to start with prefix\n" \
               << "      String:  " << AZTest::Detail::ToString(AZTEST_UNIQUE_NAME(et_str_)) << "\n" \
               << "      Prefix:  " << AZTest::Detail::ToString(AZTEST_UNIQUE_NAME(et_pre_)); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_ENDS_WITH(str, suffix) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_str_) = std::string(str); \
        auto AZTEST_UNIQUE_NAME(et_suf_) = std::string(suffix); \
        if (!AZTest::Detail::EndsWith(AZTEST_UNIQUE_NAME(et_str_), AZTEST_UNIQUE_NAME(et_suf_))) { \
            std::stringstream ss; \
            ss << "Expected string to end with suffix\n" \
               << "      String:  " << AZTest::Detail::ToString(AZTEST_UNIQUE_NAME(et_str_)) << "\n" \
               << "      Suffix:  " << AZTest::Detail::ToString(AZTEST_UNIQUE_NAME(et_suf_)); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_RANGE_EQ(range1, range2) \
    do { \
        auto&& AZTEST_UNIQUE_NAME(et_r1_) = (range1); \
        auto&& AZTEST_UNIQUE_NAME(et_r2_) = (range2); \
        auto AZTEST_UNIQUE_NAME(et_size1_) = AZTest::Detail::RangeSize(AZTEST_UNIQUE_NAME(et_r1_)); \
        auto AZTEST_UNIQUE_NAME(et_size2_) = AZTest::Detail::RangeSize(AZTEST_UNIQUE_NAME(et_r2_)); \
        if (AZTEST_UNIQUE_NAME(et_size1_) != AZTEST_UNIQUE_NAME(et_size2_)) { \
            std::stringstream ss; \
            ss << "Range sizes differ: " << AZTEST_UNIQUE_NAME(et_size1_) << " vs " << AZTEST_UNIQUE_NAME(et_size2_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } else { \
            size_t AZTEST_UNIQUE_NAME(et_index_) = 0; \
            auto AZTEST_UNIQUE_NAME(et_it1_) = std::begin(AZTEST_UNIQUE_NAME(et_r1_)); \
            auto AZTEST_UNIQUE_NAME(et_it2_) = std::begin(AZTEST_UNIQUE_NAME(et_r2_)); \
            for (; AZTEST_UNIQUE_NAME(et_it1_) != std::end(AZTEST_UNIQUE_NAME(et_r1_)); ++AZTEST_UNIQUE_NAME(et_it1_), ++AZTEST_UNIQUE_NAME(et_it2_), ++AZTEST_UNIQUE_NAME(et_index_)) { \
                if (!(*AZTEST_UNIQUE_NAME(et_it1_) == *AZTEST_UNIQUE_NAME(et_it2_))) { \
                    std::stringstream ss; \
                    ss << "Range elements differ at index " << AZTEST_UNIQUE_NAME(et_index_) << "\n" \
                       << "      Expected: " << AZTest::Detail::ToString(*AZTEST_UNIQUE_NAME(et_it2_)) << "\n" \
                       << "      Actual:   " << AZTest::Detail::ToString(*AZTEST_UNIQUE_NAME(et_it1_)); \
                    AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
                    break; \
                } \
            } \
        } \
    } while(0)

// Alias for EXPECT_RANGE_EQ — works with any pair of containers
#define EXPECT_CONTAINER_EQ(a, b) EXPECT_RANGE_EQ(a, b)
#define ASSERT_CONTAINER_EQ(a, b) ASSERT_RANGE_EQ(a, b)

#define EXPECT_THROW(statement, exception_type) \
    do { \
        bool AZTEST_UNIQUE_NAME(caughtKind_) = false; \
        bool AZTEST_UNIQUE_NAME(caughtAny_) = false; \
        std::string AZTEST_UNIQUE_NAME(caughtWhat_); \
        try { \
            statement; \
        } catch (const exception_type&) { \
            AZTEST_UNIQUE_NAME(caughtKind_) = true; \
            AZTEST_UNIQUE_NAME(caughtAny_) = true; \
        } catch (const std::exception& e) { \
            AZTEST_UNIQUE_NAME(caughtAny_) = true; \
            AZTEST_UNIQUE_NAME(caughtWhat_) = e.what(); \
        } catch (...) { \
            AZTEST_UNIQUE_NAME(caughtAny_) = true; \
            AZTEST_UNIQUE_NAME(caughtWhat_) = "unknown exception type"; \
        } \
        if (!AZTEST_UNIQUE_NAME(caughtKind_)) { \
            std::stringstream ss; \
            if (!AZTEST_UNIQUE_NAME(caughtAny_)) ss << "Expected exception of type " #exception_type " but none was thrown."; \
            else ss << "Expected exception of type " #exception_type " but caught a different one: " << AZTEST_UNIQUE_NAME(caughtWhat_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_ANY_THROW(statement) \
    do { \
        bool thrown = false; \
        try { \
            statement; \
        } catch (...) { \
            thrown = true; \
        } \
        if (!thrown) { \
            std::stringstream ss; \
            ss << "Expected any exception but none was thrown"; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_NO_THROW(statement) \
    do { \
        bool thrown = false; \
        try { \
            statement; \
        } catch (const std::exception& e) { \
            thrown = true; \
            std::stringstream ss; \
            ss << "Expected no exception but caught std::exception: " << e.what(); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } catch (...) { \
            thrown = true; \
            AZTest::Core::TestRegistry::Instance().RecordFailure("Expected no exception but caught unknown exception", __FILE__, __LINE__); \
        } \
        (void)thrown; \
    } while(0)

#define EXPECT_PRED1(pred, val1) \
    do { \
        if (!pred(val1)) { \
            std::stringstream ss; \
            ss << "Predicate " #pred " failed with value: " << AZTest::Detail::ToString(val1); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_PRED2(pred, val1, val2) \
    do { \
        if (!pred(val1, val2)) { \
            std::stringstream ss; \
            ss << "Predicate " #pred " failed with values: " << AZTest::Detail::ToString(val1) \
               << ", " << AZTest::Detail::ToString(val2); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

// ============================================================================
// ASSERT MACROS (Abort test on failure)
// ============================================================================

namespace AZTest {
class AssertionFailure : public std::runtime_error {
public:
    explicit AssertionFailure(const std::string& msg) : std::runtime_error(msg) {}
};

// Helper for streamable ADD_FAILURE
class FailureRecorder {
public:
    FailureRecorder(const char* file, int line) : file_(file), line_(line) {}
    ~FailureRecorder() {
        AZTest::Core::TestRegistry::Instance().RecordFailure(stream_.str(), file_, line_);
    }

    template <typename T>
    FailureRecorder& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

private:
    std::stringstream stream_;
    const char* file_;
    int line_;
};

} // namespace AZTest

#define ADD_FAILURE() AZTest::FailureRecorder(__FILE__, __LINE__)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::stringstream ss; \
            ss << "Assertion failed: " #condition; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            std::stringstream ss; \
            ss << "Assertion failed (expected false): " #condition; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_EQ(val1, val2) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val1_) = (val1); \
        auto AZTEST_UNIQUE_NAME(et_val2_) = (val2); \
        if (!(AZTEST_UNIQUE_NAME(et_val1_) == AZTEST_UNIQUE_NAME(et_val2_))) { \
            std::stringstream ss; \
            ss << "Assertion failed: " #val1 " == " #val2 "\n" \
               << "      Actual: " << AZTEST_UNIQUE_NAME(et_val1_) << " != " << AZTEST_UNIQUE_NAME(et_val2_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_NE(val1, val2) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val1_) = (val1); \
        auto AZTEST_UNIQUE_NAME(et_val2_) = (val2); \
        if (AZTEST_UNIQUE_NAME(et_val1_) == AZTEST_UNIQUE_NAME(et_val2_)) { \
            std::stringstream ss; \
            ss << "Assertion failed: " #val1 " != " #val2; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_STREQ(str1, str2) \
    do { \
        if (0 != strcmp((str1), (str2))) { \
            std::stringstream ss; \
            ss << "Assertion failed: string equality:\n" \
               << "      Expected: \"" << (str2) << "\"\n" \
               << "      Actual:   \"" << (str1) << "\""; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_STRNE(str1, str2) \
    do { \
        if (0 == strcmp((str1), (str2))) { \
            std::stringstream ss; \
            ss << "Assertion failed: string inequality:\n" \
               << "      Expected: \"" << (str2) << "\"\n" \
               << "      Actual:   \"" << (str1) << "\""; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_STRCASEEQ(str1, str2) \
    do { \
        if (0 != AZTest::Detail::Strcasecmp((str1), (str2))) { \
            std::stringstream ss; \
            ss << "Assertion failed: case-insensitive string equality:\n" \
               << "      Expected: \"" << (str2) << "\"\n" \
               << "      Actual:   \"" << (str1) << "\""; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_STRCASENE(str1, str2) \
    do { \
        if (0 == AZTest::Detail::Strcasecmp((str1), (str2))) { \
            std::stringstream ss; \
            ss << "Assertion failed: case-insensitive string inequality:\n" \
               << "      Expected: \"" << (str2) << "\"\n" \
               << "      Actual:   \"" << (str1) << "\""; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_NEAR(val1, val2, abs_error) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val1_) = (val1); \
        auto AZTEST_UNIQUE_NAME(et_val2_) = (val2); \
        auto AZTEST_UNIQUE_NAME(et_diff_) = (AZTEST_UNIQUE_NAME(et_val1_) > AZTEST_UNIQUE_NAME(et_val2_)) ? (AZTEST_UNIQUE_NAME(et_val1_) - AZTEST_UNIQUE_NAME(et_val2_)) : (AZTEST_UNIQUE_NAME(et_val2_) - AZTEST_UNIQUE_NAME(et_val1_)); \
        if (AZTEST_UNIQUE_NAME(et_diff_) > (abs_error)) { \
            std::stringstream ss; \
            ss << "Assertion failed: " #val1 " near " #val2 " (within " << (abs_error) << ")\n" \
               << "      Actual: " << AZTEST_UNIQUE_NAME(et_val1_) << " vs " << AZTEST_UNIQUE_NAME(et_val2_) << " (diff: " << AZTEST_UNIQUE_NAME(et_diff_) << ")"; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_FLOAT_EQ(val1, val2) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val1_) = static_cast<float>(val1); \
        auto AZTEST_UNIQUE_NAME(et_val2_) = static_cast<float>(val2); \
        auto AZTEST_UNIQUE_NAME(et_diff_) = std::fabs(AZTEST_UNIQUE_NAME(et_val1_) - AZTEST_UNIQUE_NAME(et_val2_)); \
        float AZTEST_UNIQUE_NAME(et_max_) = std::max(std::fabs(AZTEST_UNIQUE_NAME(et_val1_)), std::fabs(AZTEST_UNIQUE_NAME(et_val2_))); \
        float AZTEST_UNIQUE_NAME(et_tol_) = 4.0f * std::numeric_limits<float>::epsilon() * AZTEST_UNIQUE_NAME(et_max_); \
        if (AZTEST_UNIQUE_NAME(et_tol_) < std::numeric_limits<float>::epsilon()) AZTEST_UNIQUE_NAME(et_tol_) = std::numeric_limits<float>::epsilon(); \
        if (AZTEST_UNIQUE_NAME(et_diff_) > AZTEST_UNIQUE_NAME(et_tol_)) { \
            std::stringstream ss; \
            ss << "Assertion failed: float equality: " #val1 " == " #val2 "\n" \
               << "      Actual: " << AZTEST_UNIQUE_NAME(et_val1_) << " vs " << AZTEST_UNIQUE_NAME(et_val2_) << " (diff: " << AZTEST_UNIQUE_NAME(et_diff_) << ", tol: " << AZTEST_UNIQUE_NAME(et_tol_) << ")"; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_DOUBLE_EQ(val1, val2) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val1_) = static_cast<double>(val1); \
        auto AZTEST_UNIQUE_NAME(et_val2_) = static_cast<double>(val2); \
        auto AZTEST_UNIQUE_NAME(et_diff_) = std::fabs(AZTEST_UNIQUE_NAME(et_val1_) - AZTEST_UNIQUE_NAME(et_val2_)); \
        double AZTEST_UNIQUE_NAME(et_max_) = std::max(std::fabs(AZTEST_UNIQUE_NAME(et_val1_)), std::fabs(AZTEST_UNIQUE_NAME(et_val2_))); \
        double AZTEST_UNIQUE_NAME(et_tol_) = 4.0 * std::numeric_limits<double>::epsilon() * AZTEST_UNIQUE_NAME(et_max_); \
        if (AZTEST_UNIQUE_NAME(et_tol_) < std::numeric_limits<double>::epsilon()) AZTEST_UNIQUE_NAME(et_tol_) = std::numeric_limits<double>::epsilon(); \
        if (AZTEST_UNIQUE_NAME(et_diff_) > AZTEST_UNIQUE_NAME(et_tol_)) { \
            std::stringstream ss; \
            ss << "Assertion failed: double equality: " #val1 " == " #val2 "\n" \
               << "      Actual: " << AZTEST_UNIQUE_NAME(et_val1_) << " vs " << AZTEST_UNIQUE_NAME(et_val2_) << " (diff: " << AZTEST_UNIQUE_NAME(et_diff_) << ", tol: " << AZTEST_UNIQUE_NAME(et_tol_) << ")"; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_NAN(val) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val_) = (val); \
        if (!std::isnan(AZTEST_UNIQUE_NAME(et_val_))) { \
            std::stringstream ss; \
            ss << "Assertion failed: expected NaN, got: " << AZTEST_UNIQUE_NAME(et_val_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_INF(val) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val_) = (val); \
        if (!std::isinf(AZTEST_UNIQUE_NAME(et_val_)) || AZTEST_UNIQUE_NAME(et_val_) < 0) { \
            std::stringstream ss; \
            ss << "Assertion failed: expected positive infinity, got: " << AZTEST_UNIQUE_NAME(et_val_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_NINF(val) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val_) = (val); \
        if (!std::isinf(AZTEST_UNIQUE_NAME(et_val_)) || AZTEST_UNIQUE_NAME(et_val_) > 0) { \
            std::stringstream ss; \
            ss << "Assertion failed: expected negative infinity, got: " << AZTEST_UNIQUE_NAME(et_val_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_FINITE(val) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_val_) = (val); \
        if (std::isinf(AZTEST_UNIQUE_NAME(et_val_)) || std::isnan(AZTEST_UNIQUE_NAME(et_val_))) { \
            std::stringstream ss; \
            ss << "Assertion failed: expected finite value, got: " << AZTEST_UNIQUE_NAME(et_val_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_THROW(statement, exception_type) \
    do { \
        bool AZTEST_UNIQUE_NAME(caughtKind_) = false; \
        bool AZTEST_UNIQUE_NAME(caughtAny_) = false; \
        std::string AZTEST_UNIQUE_NAME(caughtWhat_); \
        try { \
            statement; \
        } catch (const exception_type&) { \
            AZTEST_UNIQUE_NAME(caughtKind_) = true; \
            AZTEST_UNIQUE_NAME(caughtAny_) = true; \
        } catch (const std::exception& e) { \
            AZTEST_UNIQUE_NAME(caughtAny_) = true; \
            AZTEST_UNIQUE_NAME(caughtWhat_) = e.what(); \
        } catch (...) { \
            AZTEST_UNIQUE_NAME(caughtAny_) = true; \
            AZTEST_UNIQUE_NAME(caughtWhat_) = "unknown exception type"; \
        } \
        if (!AZTEST_UNIQUE_NAME(caughtKind_)) { \
            std::stringstream ss; \
            if (!AZTEST_UNIQUE_NAME(caughtAny_)) ss << "Assertion failed: expected exception of type " #exception_type " but none was thrown."; \
            else ss << "Assertion failed: expected exception of type " #exception_type " but caught a different one: " << AZTEST_UNIQUE_NAME(caughtWhat_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_ANY_THROW(statement) \
    do { \
        bool thrown = false; \
        try { \
            statement; \
        } catch (...) { \
            thrown = true; \
        } \
        if (!thrown) { \
            std::stringstream ss; \
            ss << "Assertion failed: expected any exception but none was thrown"; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_NO_THROW(statement) \
    do { \
        try { \
            statement; \
        } catch (const std::exception& e) { \
            std::stringstream ss; \
            ss << "Assertion failed: expected no exception but caught std::exception: " << e.what(); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } catch (...) { \
            std::stringstream ss; \
            ss << "Assertion failed: expected no exception but caught unknown exception"; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_CONTAINS(haystack, needle) \
    do { \
        auto&& AZTEST_UNIQUE_NAME(et_h_) = (haystack); \
        auto&& AZTEST_UNIQUE_NAME(et_n_) = (needle); \
        if (!AZTest::Detail::Contains(AZTEST_UNIQUE_NAME(et_h_), AZTEST_UNIQUE_NAME(et_n_))) { \
            std::stringstream ss; \
            ss << "Assertion failed: expected container/string to contain element\n" \
               << "      Haystack: " << AZTest::Detail::ToString(AZTEST_UNIQUE_NAME(et_h_)) << "\n" \
               << "      Needle:   " << AZTest::Detail::ToString(AZTEST_UNIQUE_NAME(et_n_)); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_RANGE_EQ(range1, range2) \
    do { \
        auto&& AZTEST_UNIQUE_NAME(et_r1_) = (range1); \
        auto&& AZTEST_UNIQUE_NAME(et_r2_) = (range2); \
        auto AZTEST_UNIQUE_NAME(et_size1_) = AZTest::Detail::RangeSize(AZTEST_UNIQUE_NAME(et_r1_)); \
        auto AZTEST_UNIQUE_NAME(et_size2_) = AZTest::Detail::RangeSize(AZTEST_UNIQUE_NAME(et_r2_)); \
        if (AZTEST_UNIQUE_NAME(et_size1_) != AZTEST_UNIQUE_NAME(et_size2_)) { \
            std::stringstream ss; \
            ss << "Assertion failed: range sizes differ: " << AZTEST_UNIQUE_NAME(et_size1_) << " vs " << AZTEST_UNIQUE_NAME(et_size2_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } else { \
            size_t AZTEST_UNIQUE_NAME(et_index_) = 0; \
            auto AZTEST_UNIQUE_NAME(et_it1_) = std::begin(AZTEST_UNIQUE_NAME(et_r1_)); \
            auto AZTEST_UNIQUE_NAME(et_it2_) = std::begin(AZTEST_UNIQUE_NAME(et_r2_)); \
            for (; AZTEST_UNIQUE_NAME(et_it1_) != std::end(AZTEST_UNIQUE_NAME(et_r1_)); ++AZTEST_UNIQUE_NAME(et_it1_), ++AZTEST_UNIQUE_NAME(et_it2_), ++AZTEST_UNIQUE_NAME(et_index_)) { \
                if (!(*AZTEST_UNIQUE_NAME(et_it1_) == *AZTEST_UNIQUE_NAME(et_it2_))) { \
                    std::stringstream ss; \
                    ss << "Assertion failed: range elements differ at index " << AZTEST_UNIQUE_NAME(et_index_) << "\n" \
                       << "      Expected: " << AZTest::Detail::ToString(*AZTEST_UNIQUE_NAME(et_it2_)) << "\n" \
                       << "      Actual:   " << AZTest::Detail::ToString(*AZTEST_UNIQUE_NAME(et_it1_)); \
                    AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
                    throw AZTest::AssertionFailure(ss.str()); \
                } \
            } \
        } \
    } while(0)

#define ASSERT_STARTS_WITH(str, prefix) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_str_) = std::string(str); \
        auto AZTEST_UNIQUE_NAME(et_pre_) = std::string(prefix); \
        if (!AZTest::Detail::StartsWith(AZTEST_UNIQUE_NAME(et_str_), AZTEST_UNIQUE_NAME(et_pre_))) { \
            std::stringstream ss; \
            ss << "Assertion failed: string should start with prefix\n" \
               << "      String:  " << AZTest::Detail::ToString(AZTEST_UNIQUE_NAME(et_str_)) << "\n" \
               << "      Prefix:  " << AZTest::Detail::ToString(AZTEST_UNIQUE_NAME(et_pre_)); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_ENDS_WITH(str, suffix) \
    do { \
        auto AZTEST_UNIQUE_NAME(et_str_) = std::string(str); \
        auto AZTEST_UNIQUE_NAME(et_suf_) = std::string(suffix); \
        if (!AZTest::Detail::EndsWith(AZTEST_UNIQUE_NAME(et_str_), AZTEST_UNIQUE_NAME(et_suf_))) { \
            std::stringstream ss; \
            ss << "Assertion failed: string should end with suffix\n" \
               << "      String:  " << AZTest::Detail::ToString(AZTEST_UNIQUE_NAME(et_str_)) << "\n" \
               << "      Suffix:  " << AZTest::Detail::ToString(AZTEST_UNIQUE_NAME(et_suf_)); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_PRED1(pred, val1) \
    do { \
        if (!pred(val1)) { \
            std::stringstream ss; \
            ss << "Predicate " #pred " failed with value: " << AZTest::Detail::ToString(val1); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_PRED2(pred, val1, val2) \
    do { \
        if (!pred(val1, val2)) { \
            std::stringstream ss; \
            ss << "Predicate " #pred " failed with values: " << AZTest::Detail::ToString(val1) \
               << ", " << AZTest::Detail::ToString(val2); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

// ============================================================================
// DEATH TESTS
// ============================================================================

namespace AZTest {
namespace Detail {

#ifdef _WIN32
// Windows SEH-based death test
template<typename F>
inline bool ExecuteDeathTest(F&& f, std::string& deathMessage) {
    __try {
        f();
        // If we get here, no exception occurred
        deathMessage = "Function did not die";
        return false;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        DWORD code = GetExceptionCode();
        switch (code) {
            case EXCEPTION_ACCESS_VIOLATION:
                deathMessage = "Access violation";
                break;
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
                deathMessage = "Integer divide by zero";
                break;
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                deathMessage = "Floating point divide by zero";
                break;
            case EXCEPTION_STACK_OVERFLOW:
                deathMessage = "Stack overflow";
                break;
            case EXCEPTION_ILLEGAL_INSTRUCTION:
                deathMessage = "Illegal instruction";
                break;
            default:
                deathMessage = "SEH exception (code: " + std::to_string(code) + ")";
                break;
        }
        return true;
    }
}
#else
// POSIX signal-based death test
volatile sig_atomic_t deathTestSignal = 0;

void DeathTestSignalHandler(int sig) {
    deathTestSignal = sig;
    // Jump back to the test - use longjmp
    // Note: This is a simplified version. For production, use siglongjmp
    std::abort();
}

template<typename F>
inline bool ExecuteDeathTest(F&& f, std::string& deathMessage) {
    // Save original handlers
    void (*oldAbortHandler)(int) = std::signal(SIGABRT, DeathTestSignalHandler);
    void (*oldSegvHandler)(int) = std::signal(SIGSEGV, DeathTestSignalHandler);
    void (*oldFpeHandler)(int) = std::signal(SIGFPE, DeathTestSignalHandler);
    void (*oldIllHandler)(int) = std::signal(SIGILL, DeathTestSignalHandler);
    
    deathTestSignal = 0;
    bool died = false;
    
    // Note: In a real implementation, we'd use sigsetjmp/siglongjmp
    // For now, we catch signals that don't crash the process
    try {
        f();
        // If we get here, no immediate crash
        if (deathTestSignal != 0) {
            died = true;
            switch (deathTestSignal) {
                case SIGABRT: deathMessage = "SIGABRT"; break;
                case SIGSEGV: deathMessage = "SIGSEGV"; break;
                case SIGFPE: deathMessage = "SIGFPE"; break;
                case SIGILL: deathMessage = "SIGILL"; break;
                default: deathMessage = "Signal " + std::to_string(deathTestSignal); break;
            }
        } else {
            deathMessage = "Function did not die";
        }
    } catch (...) {
        // C++ exceptions don't count as "death" for death tests
        deathMessage = "Function did not die (threw exception)";
    }
    
    // Restore original handlers
    std::signal(SIGABRT, oldAbortHandler);
    std::signal(SIGSEGV, oldSegvHandler);
    std::signal(SIGFPE, oldFpeHandler);
    std::signal(SIGILL, oldIllHandler);
    
    return died;
}
#endif

} // namespace Detail
} // namespace AZTest

#define EXPECT_DEATH(statement, expected_message_substring) \
    do { \
        std::string AZTEST_UNIQUE_NAME(deathMsg_); \
        bool AZTEST_UNIQUE_NAME(died_) = AZTest::Detail::ExecuteDeathTest([&]() { statement; }, AZTEST_UNIQUE_NAME(deathMsg_)); \
        if (!AZTEST_UNIQUE_NAME(died_)) { \
            std::stringstream ss; \
            ss << "Expected death, but function did not die"; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } else if (AZTEST_UNIQUE_NAME(deathMsg_).find(expected_message_substring) == std::string::npos && std::string(expected_message_substring) != "*") { \
            std::stringstream ss; \
            ss << "Expected death with message containing: " << (expected_message_substring) << "\n" \
               << "      Actual death: " << AZTEST_UNIQUE_NAME(deathMsg_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_DEATH(statement, expected_message_substring) \
    do { \
        std::string AZTEST_UNIQUE_NAME(deathMsg_); \
        bool AZTEST_UNIQUE_NAME(died_) = AZTest::Detail::ExecuteDeathTest([&]() { statement; }, AZTEST_UNIQUE_NAME(deathMsg_)); \
        if (!AZTEST_UNIQUE_NAME(died_)) { \
            std::stringstream ss; \
            ss << "Assertion failed: Expected death, but function did not die"; \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } else if (AZTEST_UNIQUE_NAME(deathMsg_).find(expected_message_substring) == std::string::npos && std::string(expected_message_substring) != "*") { \
            std::stringstream ss; \
            ss << "Assertion failed: Expected death with message containing: " << (expected_message_substring) << "\n" \
               << "      Actual death: " << AZTEST_UNIQUE_NAME(deathMsg_); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

// ============================================================================
// BENCHMARKING
// ============================================================================

#define ENGINE_BENCHMARK(name) AZTest::Utils::Benchmark AZTEST_UNIQUE_NAME(bench_)(name)
#define AZTEST_BENCHMARK(name)  ENGINE_BENCHMARK(name)

// ============================================================================
// TEST TAGS
// ============================================================================
//
// Attach searchable tags to a test so you can run subsets via --include_tag /
// --exclude_tag without changing filter patterns.
//
// Usage:
//   AZTEST_TAG(MathSuite, Addition, "fast", "unit");
//
// Must be placed at namespace scope (outside any function), in the same
// translation unit as the test.
//
#define AZTEST_TAG(Suite, Name, ...) \
    namespace { \
    struct AZTEST_CONCAT3(AZTagReg_, Suite, AZTEST_CONCAT(_, Name)) { \
        AZTEST_CONCAT3(AZTagReg_, Suite, AZTEST_CONCAT(_, Name))() { \
            AZTest::Core::TestRegistry::Instance().AddTags( \
                #Suite, #Name, std::vector<std::string>{__VA_ARGS__}); \
        } \
    } AZTEST_CONCAT3(azTagReg_, Suite, AZTEST_CONCAT(_, Name)); \
    } // namespace

#define PERFORMANCE_TEST(suite_name, test_name, iterations) \
    TEST(suite_name, test_name) { \
        ENGINE_BENCHMARK(#test_name); \
        for (int i = 0; i < iterations; ++i)

#define END_PERFORMANCE_TEST }

// ============================================================================
// TEST RUNNER
// ============================================================================

#define RUN_ALL_TESTS() AZTest::Core::TestRegistry::Instance().RunAllTests()

// Provide a zero-boilerplate main() for standalone test binaries
#define AZTEST_MAIN() \
    int main(int argc, char** argv) { \
        AZTest::InitializeDefaultReporter(); \
        return AZTest::RunWithArgs(argc, argv); \
    }

namespace AZTest {

/**
 * @brief Initialize the test framework with a console reporter
 */
inline void InitializeDefaultReporter() {
    auto consoleReporter = std::make_shared<Reporters::ConsoleReporter>(Core::TestRegistry::Instance().UseColors());
    Core::TestRegistry::Instance().AddReporter(consoleReporter);
}

/**
 * @brief Initialize the test framework with custom reporters
 */
inline void InitializeWithReporters(std::vector<std::shared_ptr<Core::IReporter>> reporters) {
    Core::TestRegistry::Instance().ClearReporters();
    for (auto& reporter : reporters) {
        Core::TestRegistry::Instance().AddReporter(reporter);
    }
}

/**
 * @brief Add a global test environment. 
 * The framework takes ownership of the pointer and deletes it when finished.
 */
inline void AddGlobalTestEnvironment(AZTest::Environment* env) {
    AZTest::Core::TestRegistry::Instance().AddEnvironment(env);
}


inline const char* VersionString() {
    return "2.0";
}

namespace Detail {

/// Determine if stdout appears to be a terminal (Windows-safe).
/// Used for --color=auto heuristics.
inline bool IsStdoutTty() {
#ifdef _WIN32
    DWORD mode = 0;
    HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return false;
    return ::GetConsoleMode(hOut, &mode) != 0;
#else
    return isatty(STDOUT_FILENO) != 0;
#endif
}

/// Determine if colors should be used with --color=auto.
/// Checks TTY status and common CI environment variables.
inline bool ShouldUseColorAuto() {
    // Respect NO_COLOR (https://no-color.org/)
    if (const char* noColor = std::getenv("NO_COLOR"); noColor && *noColor) {
        return false;
    }
    // Respect CLICOLOR_FORCE (force colors)
    if (const char* force = std::getenv("CLICOLOR_FORCE"); force && *force) {
        return true;
    }
    // Respect CLICOLOR (disable if set to 0)
    if (const char* cliColor = std::getenv("CLICOLOR"); cliColor && std::strcmp(cliColor, "0") == 0) {
        return false;
    }
    // CI environments usually want colors even if not a TTY
    const char* ciVars[] = {"CI", "GITHUB_ACTIONS", "GITLAB_CI", "TRAVIS", "CIRCLECI", "APPVEYOR", "TF_BUILD", "AGENT_NAME"};
    for (const char* var : ciVars) {
        if (const char* val = std::getenv(var); val && *val) {
            return true;
        }
    }
    // Otherwise, use TTY detection
    return IsStdoutTty();
}

} // namespace Detail

/**
 * @brief Run tests with simple command line support.
 *
 * Supported flags:
 *   --filter=<pattern>   Wildcard pattern (e.g. Physics.*, *.SlowTest)
 *   --filter <pattern>   Same as above
 *   --list               List tests and exit
 *   --shuffle            Shuffle test order (seeded)
 *   --seed=<n>           Set seed for shuffle
 *   --repeat=<n>         Repeat all tests n times
 *   --fail_fast          Stop on first failure
 *   --slow=<ms>          Warn for tests slower than this threshold
 *   --json=<path>        Write results as JSON to path
 *   --xml=<path>         Write results as JUnit XML to path
 *   --no_color           Disable ANSI colors in console output
 *   --timeout=<ms>       Fail tests that exceed this runtime
 */
inline int RunWithArgs(int argc, char** argv) {
    std::string filter;
    bool listOnly = false;
    bool listSuitesOnly = false;
    bool shuffle = false;
    uint64_t seed = 0;
    int repeat = 1;
    bool failFast = false;
    bool runDisabled = false;
    double slowWarnMs = 0.0;
    double timeoutMs = 0.0;
    int parallelWorkers = 1;
    std::vector<std::string> jsonOutputs;
    std::vector<std::string> xmlOutputs;
    std::vector<std::string> includeTags;
    std::vector<std::string> excludeTags;
    enum class ColorMode { Auto, Always, Never };
    ColorMode colorMode = ColorMode::Auto;

    auto PrintHelp = []() {
        std::cout
            << "AZTest " << VersionString() << "\n"
            << "Usage: <test-binary> [options]\n\n"
            << "Options:\n"
            << "  --help                     Show this help and exit\n"
            << "  --version                  Show version and exit\n"
            << "  --list                     List tests (grouped by suite) and exit\n"
            << "  --list_suites              List unique suites and exit\n"
            << "  --filter <pattern>         Wildcard filter; separate with ':' for multiple\n"
            << "  --filter=<pattern>         (e.g. Physics.*  or  Math.*:Physics.*)\n"
            << "  --include_tag <tag>        Only run tests with this tag (repeatable)\n"
            << "  --include_tag=<tag>        Same as above\n"
            << "  --exclude_tag <tag>        Skip tests with this tag (repeatable)\n"
            << "  --exclude_tag=<tag>        Same as above\n"
            << "  --parallel <n>             Run up to N tests concurrently per suite\n"
            << "  --parallel=<n>             Same as above\n"
            << "  --shuffle                  Shuffle test order\n"
            << "  --seed <n>                 Seed for shuffling\n"
            << "  --seed=<n>                 Same as above\n"
            << "  --repeat <n>               Repeat all tests n times\n"
            << "  --repeat=<n>               Same as above\n"
            << "  --fail_fast                Stop on first failure\n"
            << "  --run_disabled             Run DISABLED_ tests instead of skipping\n"
            << "  --slow <ms>                Warn for tests slower than this threshold\n"
            << "  --slow=<ms>                Same as above\n"
            << "  --timeout <ms>             Fail tests that exceed this runtime\n"
            << "  --timeout=<ms>             Same as above\n"
            << "  --json <path>              Write results as JSON to path\n"
            << "  --json=<path>              Same as above\n"
            << "  --xml <path>               Write results as JUnit XML to path\n"
            << "  --xml=<path>               Same as above\n"
            << "  --color <mode>             auto|always|never (default: auto)\n"
            << "  --color=<mode>             Same as above\n";
    };

    auto ParseColorMode = [](const std::string& value, ColorMode& out) -> bool {
        if (value == "auto") {
            out = ColorMode::Auto;
            return true;
        }
        if (value == "always") {
            out = ColorMode::Always;
            return true;
        }
        if (value == "never") {
            out = ColorMode::Never;
            return true;
        }
        return false;
    };

    auto RequireValue = [&](int& i, const std::string& flag) -> const char* {
        if (i + 1 >= argc) {
            std::cerr << "Missing value for " << flag << "\n";
            return nullptr;
        }
        return argv[++i];
    };

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            PrintHelp();
            return 0;
        } else if (arg == "--version") {
            std::cout << "AZTest " << VersionString() << "\n";
            return 0;
        } else if (arg.rfind("--filter=", 0) == 0) {
            filter = arg.substr(std::string("--filter=").size());
        } else if (arg == "--filter" && i + 1 < argc) {
            filter = argv[++i];
        } else if (arg == "--list") {
            listOnly = true;
        } else if (arg == "--list_suites") {
            listSuitesOnly = true;
        } else if (arg == "--shuffle") {
            shuffle = true;
        } else if (arg == "--seed") {
            const char* v = RequireValue(i, "--seed");
            if (!v) return 2;
            seed = static_cast<uint64_t>(std::stoull(v));
        } else if (arg.rfind("--seed=", 0) == 0) {
            seed = static_cast<uint64_t>(std::stoull(arg.substr(std::string("--seed=").size())));
        } else if (arg == "--repeat") {
            const char* v = RequireValue(i, "--repeat");
            if (!v) return 2;
            repeat = std::max(1, std::stoi(v));
        } else if (arg.rfind("--repeat=", 0) == 0) {
            repeat = std::max(1, std::stoi(arg.substr(std::string("--repeat=").size())));
        } else if (arg == "--fail_fast") {
            failFast = true;
        } else if (arg == "--run_disabled") {
            runDisabled = true;
        } else if (arg == "--slow") {
            const char* v = RequireValue(i, "--slow");
            if (!v) return 2;
            slowWarnMs = std::stod(v);
        } else if (arg.rfind("--slow=", 0) == 0) {
            slowWarnMs = std::stod(arg.substr(std::string("--slow=").size()));
        } else if (arg == "--timeout") {
            const char* v = RequireValue(i, "--timeout");
            if (!v) return 2;
            timeoutMs = std::stod(v);
        } else if (arg.rfind("--timeout=", 0) == 0) {
            timeoutMs = std::stod(arg.substr(std::string("--timeout=").size()));
        } else if (arg == "--json") {
            const char* v = RequireValue(i, "--json");
            if (!v) return 2;
            jsonOutputs.push_back(v);
        } else if (arg.rfind("--json=", 0) == 0) {
            jsonOutputs.push_back(arg.substr(std::string("--json=").size()));
        } else if (arg == "--xml") {
            const char* v = RequireValue(i, "--xml");
            if (!v) return 2;
            xmlOutputs.push_back(v);
        } else if (arg.rfind("--xml=", 0) == 0) {
            xmlOutputs.push_back(arg.substr(std::string("--xml=").size()));
        } else if (arg.rfind("--include_tag=", 0) == 0) {
            includeTags.push_back(arg.substr(std::string("--include_tag=").size()));
        } else if (arg == "--include_tag") {
            const char* v = RequireValue(i, "--include_tag");
            if (!v) return 2;
            includeTags.push_back(v);
        } else if (arg.rfind("--exclude_tag=", 0) == 0) {
            excludeTags.push_back(arg.substr(std::string("--exclude_tag=").size()));
        } else if (arg == "--exclude_tag") {
            const char* v = RequireValue(i, "--exclude_tag");
            if (!v) return 2;
            excludeTags.push_back(v);
        } else if (arg.rfind("--parallel=", 0) == 0) {
            parallelWorkers = std::max(1, std::stoi(arg.substr(std::string("--parallel=").size())));
        } else if (arg == "--parallel") {
            const char* v = RequireValue(i, "--parallel");
            if (!v) return 2;
            parallelWorkers = std::max(1, std::stoi(v));
        } else if (arg == "--no_color") {
            colorMode = ColorMode::Never;
        } else if (arg == "--color") {
            const char* v = RequireValue(i, "--color");
            if (!v) return 2;
            if (!ParseColorMode(v, colorMode)) {
                std::cerr << "Invalid value for --color: " << v << "\n";
                return 2;
            }
        } else if (arg.rfind("--color=", 0) == 0) {
            std::string value = arg.substr(std::string("--color=").size());
            if (!ParseColorMode(value, colorMode)) {
                std::cerr << "Invalid value for --color: " << value << "\n";
                return 2;
            }
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            std::cerr << "Run with --help to see supported options.\n";
            return 2;
        }
    }

    bool useColors = true;
    if (colorMode == ColorMode::Never) {
        useColors = false;
    } else if (colorMode == ColorMode::Always) {
        useColors = true;
    } else {
        useColors = Detail::ShouldUseColorAuto();
    }

    if (!filter.empty()) {
        Core::TestRegistry::Instance().SetFilter(filter);
    }
    Core::TestRegistry::Instance().EnableShuffle(shuffle);
    Core::TestRegistry::Instance().SetSeed(seed);
    Core::TestRegistry::Instance().SetRepeat(repeat);
    Core::TestRegistry::Instance().EnableFailFast(failFast);
    Core::TestRegistry::Instance().SetRunDisabled(runDisabled);
    Core::TestRegistry::Instance().SetSlowThreshold(slowWarnMs);
    Core::TestRegistry::Instance().SetUseColors(useColors);
    Core::TestRegistry::Instance().SetTimeout(timeoutMs);
    Core::TestRegistry::Instance().SetParallelWorkers(parallelWorkers);
    if (!includeTags.empty()) Core::TestRegistry::Instance().SetIncludeTags(includeTags);
    if (!excludeTags.empty()) Core::TestRegistry::Instance().SetExcludeTags(excludeTags);

    for (const auto& path : jsonOutputs) {
        Core::TestRegistry::Instance().AddReporter(std::make_shared<Reporters::JSONReporter>(path));
    }
    for (const auto& path : xmlOutputs) {
        Core::TestRegistry::Instance().AddReporter(std::make_shared<Reporters::XMLReporter>(path));
    }

    if (listSuitesOnly) {
        const auto& tests = Core::TestRegistry::Instance().GetTests();
        std::vector<std::string> suites;
        suites.reserve(tests.size());
        for (const auto& t : tests) {
            if (std::find(suites.begin(), suites.end(), t.suite) == suites.end()) {
                suites.push_back(t.suite);
            }
        }
        std::cout << "Suites (" << suites.size() << "):\n";
        for (const auto& s : suites) {
            std::cout << "  " << s << "\n";
        }
        return 0;
    }

    if (listOnly) {
        const auto& tests = Core::TestRegistry::Instance().GetTests();
        std::cout << "Tests (" << tests.size() << "):\n";
        std::string currentSuite;
        for (const auto& t : tests) {
            if (t.suite != currentSuite) {
                currentSuite = t.suite;
                std::cout << "\n[" << currentSuite << "]\n";
            }
            std::cout << "  " << t.name;
            if (t.disabled) {
                std::cout << " [DISABLED]";
            }
            std::cout << "\n";
        }
        return 0;
    }

    return RUN_ALL_TESTS();
}

} // namespace AZTest
#include "UEIntegration.h"
