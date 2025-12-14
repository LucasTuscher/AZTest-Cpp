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
#include "Utils/Benchmark.h"

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

#define EXPECT_THROW(statement, exception_type) \
    do { \
        bool thrown = false; \
        try { \
            statement; \
        } catch (const exception_type&) { \
            thrown = true; \
        } catch (...) { \
            thrown = true; \
        } \
        if (!thrown) { \
            std::stringstream ss; \
            ss << "Expected exception: " #exception_type " was not thrown"; \
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

#define ASSERT_THROW(statement, exception_type) \
    do { \
        bool thrown = false; \
        try { \
            statement; \
        } catch (const exception_type&) { \
            thrown = true; \
        } catch (...) { \
            thrown = true; \
        } \
        if (!thrown) { \
            std::stringstream ss; \
            ss << "Assertion failed: expected exception " #exception_type " was not thrown"; \
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
// BENCHMARKING
// ============================================================================

#define ENGINE_BENCHMARK(name) AZTest::Utils::Benchmark AZTEST_UNIQUE_NAME(bench_)(name)

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
    bool shuffle = false;
    uint64_t seed = 0;
    int repeat = 1;
    bool failFast = false;
    double slowWarnMs = 0.0;
    double timeoutMs = 0.0;
    std::vector<std::string> jsonOutputs;
    std::vector<std::string> xmlOutputs;
    bool useColors = true;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--filter=", 0) == 0) {
            filter = arg.substr(std::string("--filter=").size());
        } else if (arg == "--filter" && i + 1 < argc) {
            filter = argv[++i];
        } else if (arg == "--list") {
            listOnly = true;
        } else if (arg == "--shuffle") {
            shuffle = true;
        } else if (arg.rfind("--seed=", 0) == 0) {
            seed = static_cast<uint64_t>(std::stoull(arg.substr(std::string("--seed=").size())));
        } else if (arg.rfind("--repeat=", 0) == 0) {
            repeat = std::max(1, std::stoi(arg.substr(std::string("--repeat=").size())));
        } else if (arg == "--fail_fast") {
            failFast = true;
        } else if (arg.rfind("--slow=", 0) == 0) {
            slowWarnMs = std::stod(arg.substr(std::string("--slow=").size()));
        } else if (arg.rfind("--timeout=", 0) == 0) {
            timeoutMs = std::stod(arg.substr(std::string("--timeout=").size()));
        } else if (arg.rfind("--json=", 0) == 0) {
            jsonOutputs.push_back(arg.substr(std::string("--json=").size()));
        } else if (arg.rfind("--xml=", 0) == 0) {
            xmlOutputs.push_back(arg.substr(std::string("--xml=").size()));
        } else if (arg == "--no_color") {
            useColors = false;
        }
    }

    if (!filter.empty()) {
        Core::TestRegistry::Instance().SetFilter(filter);
    }
    Core::TestRegistry::Instance().EnableShuffle(shuffle);
    Core::TestRegistry::Instance().SetSeed(seed);
    Core::TestRegistry::Instance().SetRepeat(repeat);
    Core::TestRegistry::Instance().EnableFailFast(failFast);
    Core::TestRegistry::Instance().SetSlowThreshold(slowWarnMs);
    Core::TestRegistry::Instance().SetUseColors(useColors);
    Core::TestRegistry::Instance().SetTimeout(timeoutMs);

    for (const auto& path : jsonOutputs) {
        Core::TestRegistry::Instance().AddReporter(std::make_shared<Reporters::JSONReporter>(path));
    }
    for (const auto& path : xmlOutputs) {
        Core::TestRegistry::Instance().AddReporter(std::make_shared<Reporters::XMLReporter>(path));
    }

    if (listOnly) {
        const auto& tests = Core::TestRegistry::Instance().GetTests();
        std::cout << "Listing tests (" << tests.size() << "):" << std::endl;
        for (const auto& t : tests) {
            std::cout << "  " << t.suite << "." << t.name;
            if (t.disabled) {
                std::cout << " [DISABLED]";
            }
            std::cout << std::endl;
        }
        return 0;
    }

    return RUN_ALL_TESTS();
}

} // namespace AZTest
