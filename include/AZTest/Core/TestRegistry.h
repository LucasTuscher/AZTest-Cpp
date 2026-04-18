#pragma once

#include "TestResult.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <map>
#include <mutex>
#include <utility>
#include "../Environment.h"

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
    std::vector<AZTest::Environment*> environments_;
    std::vector<std::string> filterPatterns_;
    bool shuffle_;
    uint64_t seed_;
    int repeatCount_;
    bool failFast_;
    bool runDisabled_;
    double slowThresholdMs_;
    bool useColors_;
    double timeoutMs_;
    int parallelWorkers_;

    // Tag filtering
    std::map<std::pair<std::string,std::string>, std::vector<std::string>> tagMap_;
    std::vector<std::string> includeTags_;
    std::vector<std::string> excludeTags_;

    // Mutex for reporter calls (used in parallel mode)
    mutable std::mutex reporterMutex_;

    TestRegistry();
    ~TestRegistry();

    TestResult RunSingleTest(const TestInfo& test);

public:
    static TestRegistry& Instance();

    // Global Environments
    void AddEnvironment(AZTest::Environment* env);
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

    // Internal state (thread-local — see TestRegistry.cpp)
    bool CurrentTestFailed() const;
    void ResetTestState();
    size_t GetEnvironmentCount() const { return environments_.size(); }

    // Filtering
    void SetFilter(const std::string& pattern);
    void ClearFilter();

    // Tag management
    void AddTags(const std::string& suite, const std::string& name, std::vector<std::string> tags);
    void SetIncludeTags(std::vector<std::string> tags);
    void SetExcludeTags(std::vector<std::string> tags);

    // Execution options
    void EnableShuffle(bool enable);
    void SetSeed(uint64_t seed);
    void SetRepeat(int count);
    void EnableFailFast(bool enable);
    void SetRunDisabled(bool enable);
    bool RunDisabled() const { return runDisabled_; }
    void SetSlowThreshold(double ms);
    void SetUseColors(bool enable);
    bool UseColors() const { return useColors_; }
    void SetTimeout(double ms);
    double TimeoutMs() const { return timeoutMs_; }
    void SetParallelWorkers(int n);
    int ParallelWorkers() const { return parallelWorkers_; }
};

} // namespace Core
} // namespace AZTest
