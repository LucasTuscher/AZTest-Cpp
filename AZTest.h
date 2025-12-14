#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <sstream>
#include <iomanip>

// ============================================================================
// COLOR OUTPUT
// ============================================================================
namespace AZTest {
namespace Color {
    inline const char* Reset()  { return "\033[0m"; }
    inline const char* Red()    { return "\033[31m"; }
    inline const char* Green()  { return "\033[32m"; }
    inline const char* Yellow() { return "\033[33m"; }
    inline const char* Blue()   { return "\033[34m"; }
    inline const char* Cyan()   { return "\033[36m"; }
    inline const char* Bold()   { return "\033[1m"; }
}

// ============================================================================
// TEST RESULT & REGISTRY
// ============================================================================
struct TestResult {
    std::string testName;
    std::string suiteName;
    bool passed;
    std::string failureMessage;
    std::string file;
    int line;
    double executionTimeMs;
};

class TestRegistry {
private:
    struct TestInfo {
        std::string name;
        std::string suite;
        std::function<void()> testFunc;
        std::string file;
        int line;
    };

    std::vector<TestInfo> tests;
    std::vector<TestResult> results;
    bool currentTestFailed = false;
    std::string currentFailureMsg;
    std::string currentFile;
    int currentLine = 0;

    TestRegistry() = default;

public:
    static TestRegistry& Instance() {
        static TestRegistry instance;
        return instance;
    }

    void RegisterTest(const std::string& name, const std::string& suite,
                     std::function<void()> func, const char* file, int line) {
        tests.push_back({name, suite, func, file, line});
    }

    void RecordFailure(const std::string& msg, const char* file, int line) {
        currentTestFailed = true;
        currentFailureMsg = msg;
        currentFile = file;
        currentLine = line;
    }

    int RunAllTests() {
        std::cout << Color::Bold() << Color::Cyan()
                  << "\n========================================\n"
                  << "  AZTEST FRAMEWORK\n"
                  << "========================================\n"
                  << Color::Reset() << std::endl;

        std::cout << "Running " << tests.size() << " test(s)...\n" << std::endl;

        int passed = 0;
        int failed = 0;
        double totalTime = 0.0;

        std::string currentSuite = "";

        for (const auto& test : tests) {
            // Print suite header if changed
            if (test.suite != currentSuite) {
                currentSuite = test.suite;
                std::cout << Color::Bold() << "\n[" << currentSuite << "]\n"
                         << Color::Reset();
            }

            currentTestFailed = false;
            currentFailureMsg.clear();

            // Run test with timing
            auto start = std::chrono::high_resolution_clock::now();

            try {
                test.testFunc();
            } catch (const std::exception& e) {
                currentTestFailed = true;
                currentFailureMsg = std::string("Exception: ") + e.what();
            } catch (...) {
                currentTestFailed = true;
                currentFailureMsg = "Unknown exception thrown";
            }

            auto end = std::chrono::high_resolution_clock::now();
            double timeMs = std::chrono::duration<double, std::milli>(end - start).count();
            totalTime += timeMs;

            // Record result
            TestResult result;
            result.testName = test.name;
            result.suiteName = test.suite;
            result.passed = !currentTestFailed;
            result.failureMessage = currentFailureMsg;
            result.file = currentTestFailed ? currentFile : test.file;
            result.line = currentTestFailed ? currentLine : test.line;
            result.executionTimeMs = timeMs;
            results.push_back(result);

            // Print result
            if (result.passed) {
                std::cout << Color::Green() << "  [PASS] " << Color::Reset()
                         << test.name;
                passed++;
            } else {
                std::cout << Color::Red() << "  [FAIL] " << Color::Reset()
                         << test.name;
                failed++;
            }

            // Show timing for slow tests
            if (timeMs > 10.0) {
                std::cout << Color::Yellow() << " (" << std::fixed
                         << std::setprecision(2) << timeMs << " ms)"
                         << Color::Reset();
            }
            std::cout << std::endl;

            // Show failure details
            if (!result.passed) {
                std::cout << Color::Red()
                         << "      " << result.file << ":" << result.line << "\n"
                         << "      " << result.failureMessage << "\n"
                         << Color::Reset();
            }
        }

        // Summary
        std::cout << Color::Bold() << Color::Cyan()
                  << "\n========================================\n"
                  << Color::Reset();
        std::cout << "Total: " << tests.size() << " tests\n";
        std::cout << Color::Green() << "Passed: " << passed << Color::Reset() << "\n";

        if (failed > 0) {
            std::cout << Color::Red() << "Failed: " << failed << Color::Reset() << "\n";
        }

        std::cout << "Time: " << std::fixed << std::setprecision(2)
                  << totalTime << " ms\n";
        std::cout << Color::Bold() << Color::Cyan()
                  << "========================================\n"
                  << Color::Reset() << std::endl;

        return failed;
    }

    const std::vector<TestResult>& GetResults() const { return results; }
};

// ============================================================================
// TEST MACROS
// ============================================================================

#define AZTEST_CONCAT_IMPL(a, b) a##b
#define AZTEST_CONCAT(a, b) AZTEST_CONCAT_IMPL(a, b)

// Main TEST macro
#define TEST(suite_name, test_name) \
    static void AZTEST_CONCAT(AZTestFunc_, __LINE__)(); \
    namespace { \
        struct AZTEST_CONCAT(AZTestRegistrar_, __LINE__) { \
            AZTEST_CONCAT(AZTestRegistrar_, __LINE__)() { \
                AZTest::TestRegistry::Instance().RegisterTest( \
                    #test_name, #suite_name, \
                    AZTEST_CONCAT(AZTestFunc_, __LINE__), \
                    __FILE__, __LINE__); \
            } \
        }; \
        static AZTEST_CONCAT(AZTestRegistrar_, __LINE__) \
            AZTEST_CONCAT(azTestRegistrarInstance_, __LINE__); \
    } \
    static void AZTEST_CONCAT(AZTestFunc_, __LINE__)()

// EXPECT macros (continue on failure)
#define EXPECT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::stringstream ss; \
            ss << "Expected TRUE but got FALSE: " #condition; \
            AZTest::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_FALSE(condition) \
    do { \
        if (condition) { \
            std::stringstream ss; \
            ss << "Expected FALSE but got TRUE: " #condition; \
            AZTest::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_EQ(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (v1 != v2) { \
            std::stringstream ss; \
            ss << "Expected equality: " #val1 " == " #val2 "\n" \
               << "      Actual: " << v1 << " != " << v2; \
            AZTest::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_NE(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (v1 == v2) { \
            std::stringstream ss; \
            ss << "Expected inequality: " #val1 " != " #val2 "\n" \
               << "      Actual: " << v1 << " == " << v2; \
            AZTest::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_LT(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (!(v1 < v2)) { \
            std::stringstream ss; \
            ss << "Expected: " #val1 " < " #val2 "\n" \
               << "      Actual: " << v1 << " >= " << v2; \
            AZTest::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_LE(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (!(v1 <= v2)) { \
            std::stringstream ss; \
            ss << "Expected: " #val1 " <= " #val2 "\n" \
               << "      Actual: " << v1 << " > " << v2; \
            AZTest::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_GT(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (!(v1 > v2)) { \
            std::stringstream ss; \
            ss << "Expected: " #val1 " > " #val2 "\n" \
               << "      Actual: " << v1 << " <= " << v2; \
            AZTest::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define EXPECT_GE(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (!(v1 >= v2)) { \
            std::stringstream ss; \
            ss << "Expected: " #val1 " >= " #val2 "\n" \
               << "      Actual: " << v1 << " < " << v2; \
            AZTest::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

// ASSERT macros (abort test on failure)
class AssertionFailure : public std::runtime_error {
public:
    explicit AssertionFailure(const std::string& msg) : std::runtime_error(msg) {}
};

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::stringstream ss; \
            ss << "Assertion failed: " #condition; \
            AZTest::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            std::stringstream ss; \
            ss << "Assertion failed (expected false): " #condition; \
            AZTest::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_EQ(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (v1 != v2) { \
            std::stringstream ss; \
            ss << "Assertion failed: " #val1 " == " #val2 "\n" \
               << "      Actual: " << v1 << " != " << v2; \
            AZTest::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

#define ASSERT_NE(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (v1 == v2) { \
            std::stringstream ss; \
            ss << "Assertion failed: " #val1 " != " #val2; \
            AZTest::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)

// ============================================================================
// BENCHMARKING
// ============================================================================
class Benchmark {
public:
    explicit Benchmark(const std::string& name) : name_(name) {
        start_ = std::chrono::high_resolution_clock::now();
        std::cout << Color::Cyan() << "[BENCH] Starting: " << name_
                  << Color::Reset() << std::endl;
    }

    ~Benchmark() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - start_).count();

        std::cout << Color::Cyan() << "[BENCH] " << name_ << ": "
                  << Color::Yellow() << std::fixed << std::setprecision(3)
                  << duration << " ms" << Color::Reset() << std::endl;
    }

private:
    std::string name_;
    std::chrono::high_resolution_clock::time_point start_;
};

#define AZTEST_BENCHMARK(name) AZTest::Benchmark AZTEST_CONCAT(bench_, __LINE__)(name)

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================
#define PERFORMANCE_TEST(suite_name, test_name, iterations) \
    TEST(suite_name, test_name) { \
        AZTEST_BENCHMARK(#test_name); \
        for (int i = 0; i < iterations; ++i)

#define END_PERFORMANCE_TEST }

// ============================================================================
// MAIN RUNNER
// ============================================================================
#define RUN_ALL_TESTS() AZTest::TestRegistry::Instance().RunAllTests()

} // namespace AZTest
