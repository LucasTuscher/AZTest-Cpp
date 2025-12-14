#pragma once

#include "TestResult.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace AZTest {
namespace Core {

class IReporter;

struct TestInfo {
    std::string name;
    std::string suite;
    std::function<void()> testFunc;
    std::string file;
    int line;
    bool isFixture;
    std::function<void()> setUpFunc;
    std::function<void()> tearDownFunc;
    bool disabled;

    TestInfo()
        : line(0)
        , isFixture(false)
        , disabled(false) {}
};

class TestRegistry {
private:
    std::vector<TestInfo> tests_;
    std::vector<TestResult> results_;
    std::vector<std::shared_ptr<IReporter>> reporters_;
    std::string filterPattern_;
    bool shuffle_;
    uint64_t seed_;
    int repeatCount_;
    bool failFast_;
    double slowThresholdMs_;
    bool useColors_;
    double timeoutMs_;
    std::vector<std::string> traceStack_;

    bool currentTestFailed_;
    bool currentTestSkipped_;
    std::string currentFailureMsg_;
    std::string currentSkipMsg_;
    std::string currentFile_;
    int currentLine_;

    TestRegistry();

public:
    static TestRegistry& Instance();

    // Test registration
    void RegisterTest(const std::string& name,
                     const std::string& suite,
                     std::function<void()> func,
                     const char* file,
                     int line);

    void RegisterFixtureTest(const std::string& name,
                            const std::string& suite,
                            std::function<void()> func,
                            std::function<void()> setUp,
                            std::function<void()> tearDown,
                            const char* file,
                            int line);

    // Failure recording
    void RecordFailure(const std::string& msg, const char* file, int line);
    void RecordSkip(const std::string& msg, const char* file, int line);

    // Trace stack management
    void PushTrace(const std::string& trace);
    void PopTrace();
    std::string GetTraceStack() const;

    // Test execution
    int RunAllTests();
    int RunTestSuite(const std::string& suiteName);

    // Reporter management
    void AddReporter(std::shared_ptr<IReporter> reporter);
    void ClearReporters();

    // Results
    const std::vector<TestResult>& GetResults() const { return results_; }
    const std::vector<TestInfo>& GetTests() const { return tests_; }

    // Internal state
    bool CurrentTestFailed() const { return currentTestFailed_; }
    void ResetTestState();

    // Filtering
    void SetFilter(const std::string& pattern);
    void ClearFilter();

    // Execution options
    void EnableShuffle(bool enable);
    void SetSeed(uint64_t seed);
    void SetRepeat(int count);
    void EnableFailFast(bool enable);
    void SetSlowThreshold(double ms);
    void SetUseColors(bool enable);
    bool UseColors() const { return useColors_; }
    void SetTimeout(double ms);
    double TimeoutMs() const { return timeoutMs_; }
};

} // namespace Core
} // namespace AZTest
