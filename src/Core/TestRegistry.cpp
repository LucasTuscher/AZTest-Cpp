#include "../../include/AZTest/Core/TestRegistry.h"
#include "../../include/AZTest/Reporters/IReporter.h"
#include "../../include/AZTest/AZTest.h"
#include "../../include/AZTest/Environment.h"
#include "../../include/AZTest/Mock.h"
#include <chrono>
#include <algorithm>
#include <random>
#include <future>
#include <thread>
#include <sstream>
#include <unordered_map>
#include <queue>
#include <condition_variable>
#include <functional>

namespace AZTest {
namespace Core {

// ============================================================================
// Thread-local test state — one copy per thread, safe for parallel execution
// ============================================================================

namespace {

thread_local bool     tl_failed   = false;
thread_local bool     tl_skipped  = false;
thread_local std::string             tl_failureMsg;
thread_local std::vector<TestFailure> tl_failures;
thread_local std::string             tl_skipMsg;
thread_local std::string             tl_file;
thread_local int      tl_line     = 0;
thread_local std::vector<std::string> tl_traceStack;

// Simple thread pool used for parallel test execution
class ThreadPool {
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool stop_ = false;
public:
    explicit ThreadPool(int n) {
        for (int i = 0; i < n; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mtx_);
                        cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                        if (stop_ && tasks_.empty()) return;
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    void Submit(std::function<void()> f) {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            tasks_.push(std::move(f));
        }
        cv_.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& w : workers_) w.join();
    }
};

std::string JoinFailureMessages(const std::vector<TestFailure>& failures) {
    std::ostringstream oss;
    for (size_t i = 0; i < failures.size(); ++i) {
        if (i > 0) oss << "\n";
        oss << failures[i].message;
    }
    return oss.str();
}

bool WildcardMatch(const std::string& pattern, const std::string& value) {
    size_t p = 0, v = 0, star = std::string::npos, match = 0;
    while (v < value.size()) {
        if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == value[v])) {
            ++p; ++v;
        } else if (p < pattern.size() && pattern[p] == '*') {
            star = p++;
            match = v;
        } else if (star != std::string::npos) {
            p = star + 1;
            v = ++match;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*') ++p;
    return p == pattern.size();
}

} // anonymous namespace

// ============================================================================
// TestRegistry constructor / destructor
// ============================================================================

TestRegistry::TestRegistry()
    : shuffle_(false)
    , seed_(0)
    , repeatCount_(1)
    , failFast_(false)
    , runDisabled_(false)
    , slowThresholdMs_(0.0)
    , useColors_(true)
    , timeoutMs_(0.0)
    , parallelWorkers_(1) {
}

TestRegistry::~TestRegistry() {
    for (auto* env : environments_) delete env;
    environments_.clear();
}

TestRegistry& TestRegistry::Instance() {
    static TestRegistry instance;
    return instance;
}

// ============================================================================
// Registration
// ============================================================================

void TestRegistry::AddEnvironment(AZTest::Environment* env) {
    if (env) environments_.push_back(env);
}

void TestRegistry::RegisterTest(const std::string& name,
                                const std::string& suite,
                                std::function<void()> func,
                                const char* file,
                                int line) {
    TestInfo info;
    info.name = name;
    info.suite = suite;
    info.testFunc = func;
    info.file = file;
    info.line = line;
    info.isFixture = false;
    info.disabled = (suite.rfind("DISABLED_", 0) == 0) || (name.rfind("DISABLED_", 0) == 0);
    tests_.push_back(info);
}

void TestRegistry::RegisterFixtureTest(const std::string& name,
                                      const std::string& suite,
                                      std::function<void()> func,
                                      std::function<void()> setUp,
                                      std::function<void()> tearDown,
                                      const char* file,
                                      int line) {
    TestInfo info;
    info.name = name;
    info.suite = suite;
    info.testFunc = func;
    info.file = file;
    info.line = line;
    info.isFixture = true;
    info.setUpFunc = setUp;
    info.tearDownFunc = tearDown;
    info.disabled = (suite.rfind("DISABLED_", 0) == 0) || (name.rfind("DISABLED_", 0) == 0);
    tests_.push_back(info);
}

// ============================================================================
// Thread-local state helpers
// ============================================================================

void TestRegistry::RecordFailure(const std::string& msg, const char* file, int line) {
    tl_failed = true;
    TestFailure failure;
    failure.message = GetTraceStack() + msg;
    failure.fileName = file ? file : "";
    failure.lineNumber = line;
    tl_failures.push_back(failure);
    tl_failureMsg = failure.message;
    tl_file = failure.fileName;
    tl_line = failure.lineNumber;
}

void TestRegistry::RecordSkip(const std::string& msg, const char* file, int line) {
    tl_skipped = true;
    tl_skipMsg = msg;
    if (file) tl_file = file;
    tl_line = line;
}

void TestRegistry::PushTrace(const std::string& trace) {
    tl_traceStack.push_back(trace);
}

void TestRegistry::PopTrace() {
    if (!tl_traceStack.empty()) tl_traceStack.pop_back();
}

std::string TestRegistry::GetTraceStack() const {
    if (tl_traceStack.empty()) return "";
    std::stringstream ss;
    ss << "Trace:\n";
    for (const auto& t : tl_traceStack) ss << "  " << t << "\n";
    return ss.str();
}

bool TestRegistry::CurrentTestFailed() const { return tl_failed; }

void TestRegistry::ResetTestState() {
    tl_failed   = false;
    tl_skipped  = false;
    tl_failureMsg.clear();
    tl_failures.clear();
    tl_skipMsg.clear();
    tl_file.clear();
    tl_line = 0;
    tl_traceStack.clear();
    // Auto-reset global mock registry (serial path only; parallel tests manage mocks locally)
    if (parallelWorkers_ <= 1) {
        AZTest::Mock::MockRegistry::Instance().Clear();
    }
}

// ============================================================================
// Single-test runner — returns a complete TestResult, safe to call from any thread
// ============================================================================

TestResult TestRegistry::RunSingleTest(const TestInfo& test) {
    // Reset this thread's state
    tl_failed   = false;
    tl_skipped  = false;
    tl_failureMsg.clear();
    tl_failures.clear();
    tl_skipMsg.clear();
    tl_file.clear();
    tl_line = 0;
    tl_traceStack.clear();

    {
        std::lock_guard<std::mutex> lock(reporterMutex_);
        for (auto& rep : reporters_) rep->OnTestStart(test.name);
    }

    auto start = std::chrono::high_resolution_clock::now();

    if (test.disabled) {
        if (!runDisabled_) {
            tl_skipped = true;
            tl_skipMsg = "Disabled test (name or suite starts with DISABLED_)";
            tl_file = test.file;
            tl_line = test.line;
        } else {
            try { test.testFunc(); }
            catch (const AssertionFailure&) {}
            catch (const std::exception& e) {
                tl_failed = true; tl_failureMsg = std::string("Uncaught exception: ") + e.what();
                tl_file = test.file; tl_line = test.line;
            } catch (...) {
                tl_failed = true; tl_failureMsg = "Unknown exception thrown";
                tl_file = test.file; tl_line = test.line;
            }
        }
    } else if (timeoutMs_ > 0.0) {
        std::packaged_task<void()> task([&test]() { test.testFunc(); });
        auto fut = task.get_future();
        std::thread runner(std::move(task));
        if (fut.wait_for(std::chrono::milliseconds(static_cast<long long>(timeoutMs_))) == std::future_status::ready) {
            try { fut.get(); }
            catch (const AssertionFailure&) {}
            catch (const std::exception& e) {
                tl_failed = true; tl_failureMsg = std::string("Uncaught exception: ") + e.what();
                tl_file = test.file; tl_line = test.line;
            } catch (...) {
                tl_failed = true; tl_failureMsg = "Unknown exception thrown";
                tl_file = test.file; tl_line = test.line;
            }
            if (runner.joinable()) runner.join();
        } else {
            tl_failed = true;
            tl_failureMsg = "Test timed out after " + std::to_string(timeoutMs_) + " ms";
            tl_file = test.file; tl_line = test.line;
            if (runner.joinable()) runner.detach();
        }
    } else {
        try { test.testFunc(); }
        catch (const AssertionFailure&) {}
        catch (const std::exception& e) {
            tl_failed = true; tl_failureMsg = std::string("Uncaught exception: ") + e.what();
            tl_file = test.file; tl_line = test.line;
        } catch (...) {
            tl_failed = true; tl_failureMsg = "Unknown exception thrown";
            tl_file = test.file; tl_line = test.line;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double timeMs = std::chrono::duration<double, std::milli>(end - start).count();

    TestResult result;
    result.testName = test.name;
    result.suiteName = test.suite;
    if (tl_skipped) {
        result.status = TestStatus::SKIPPED;
        result.failureMessage = tl_skipMsg;
        result.fileName = tl_file.empty() ? test.file : tl_file;
        result.lineNumber = tl_line == 0 ? test.line : tl_line;
    } else if (tl_failed) {
        result.status = TestStatus::FAILED;
        result.failures = tl_failures;
        result.failureMessage = !result.failures.empty() ? JoinFailureMessages(result.failures) : tl_failureMsg;
        if (!result.failures.empty()) {
            result.fileName = result.failures.front().fileName.empty() ? test.file : result.failures.front().fileName;
            result.lineNumber = result.failures.front().lineNumber == 0 ? test.line : result.failures.front().lineNumber;
        } else {
            result.fileName = tl_file.empty() ? test.file : tl_file;
            result.lineNumber = tl_line == 0 ? test.line : tl_line;
        }
    } else {
        result.status = TestStatus::PASSED;
        result.fileName = test.file;
        result.lineNumber = test.line;
    }
    result.executionTimeMs = timeMs;
    if (slowThresholdMs_ > 0.0 && timeMs > slowThresholdMs_) {
        result.slow = true;
        std::ostringstream oss;
        oss << "Slow test: " << timeMs << " ms > threshold " << slowThresholdMs_ << " ms";
        result.warningMessage = oss.str();
    }
    return result;
}

// ============================================================================
// RunAllTests
// ============================================================================

int TestRegistry::RunAllTests() {
    if (reporters_.empty()) {
        reporters_.push_back(std::make_shared<Reporters::ConsoleReporter>(useColors_));
    }
    results_.clear();

    // Build filtered list (multi-pattern + tag filter)
    std::vector<TestInfo> runnable;
    runnable.reserve(tests_.size());
    for (const auto& t : tests_) {
        // Pattern filter: empty = all; otherwise match any pattern
        if (!filterPatterns_.empty()) {
            std::string fullName = t.suite + "." + t.name;
            bool matched = false;
            for (const auto& pat : filterPatterns_) {
                if (WildcardMatch(pat, fullName)) { matched = true; break; }
            }
            if (!matched) continue;
        }

        // Tag filter
        auto key = std::make_pair(t.suite, t.name);
        const auto& tags = tagMap_.count(key) ? tagMap_.at(key) : std::vector<std::string>{};

        if (!includeTags_.empty()) {
            bool hasInclude = false;
            for (const auto& inc : includeTags_) {
                for (const auto& tag : tags) {
                    if (tag == inc) { hasInclude = true; break; }
                }
                if (hasInclude) break;
            }
            if (!hasInclude) continue;
        }
        if (!excludeTags_.empty()) {
            bool hasExclude = false;
            for (const auto& exc : excludeTags_) {
                for (const auto& tag : tags) {
                    if (tag == exc) { hasExclude = true; break; }
                }
            }
            if (hasExclude) continue;
        }

        runnable.push_back(t);
    }

    // Group by suite (stable order)
    std::vector<std::string> baseSuiteOrder;
    std::unordered_map<std::string, std::vector<TestInfo>> baseSuiteTests;
    for (const auto& t : runnable) {
        auto it = baseSuiteTests.find(t.suite);
        if (it == baseSuiteTests.end()) {
            baseSuiteOrder.push_back(t.suite);
            baseSuiteTests.emplace(t.suite, std::vector<TestInfo>{t});
        } else {
            it->second.push_back(t);
        }
    }

    for (auto& rep : reporters_) rep->OnTestRunStart(static_cast<int>(runnable.size() * repeatCount_));

    auto overallStart = std::chrono::high_resolution_clock::now();
    bool aborted = false;
    uint64_t effectiveSeed = (seed_ != 0) ? seed_
        : static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());

    for (int iteration = 0; iteration < repeatCount_ && !aborted; ++iteration) {
        // Global Environment SetUp
        for (auto* env : environments_) {
            try { env->SetUp(); }
            catch (const std::exception& e) {
                std::cerr << "Global Environment SetUp failed: " << e.what() << "\n";
                aborted = true; break;
            } catch (...) {
                std::cerr << "Global Environment SetUp failed with unknown exception.\n";
                aborted = true; break;
            }
        }
        if (aborted) break;

        std::vector<std::string> suiteOrder = baseSuiteOrder;
        std::unordered_map<std::string, std::vector<TestInfo>> suiteTests = baseSuiteTests;

        if (shuffle_) {
            std::mt19937_64 rng(effectiveSeed + static_cast<uint64_t>(iteration));
            std::shuffle(suiteOrder.begin(), suiteOrder.end(), rng);
            for (auto& kv : suiteTests) std::shuffle(kv.second.begin(), kv.second.end(), rng);
        }

        for (const auto& suiteName : suiteOrder) {
            auto suiteIt = suiteTests.find(suiteName);
            if (suiteIt == suiteTests.end()) continue;

            for (auto& rep : reporters_) rep->OnTestSuiteStart(suiteName);

            // Suite-level setup (only for fixture tests)
            bool suiteSetUpCalled = false;
            std::function<void()> suiteTearDown;
            for (const auto& test : suiteIt->second) {
                if (!suiteSetUpCalled && test.isFixture && test.setUpFunc) {
                    test.setUpFunc();
                    suiteSetUpCalled = true;
                    suiteTearDown = test.tearDownFunc;
                    break;
                }
            }

            bool parallel = (parallelWorkers_ > 1) && (suiteIt->second.size() > 1);

            if (parallel) {
                // ---- Parallel execution ----
                int workers = std::min(parallelWorkers_, static_cast<int>(suiteIt->second.size()));
                ThreadPool pool(workers);
                std::vector<std::future<TestResult>> futures;
                futures.reserve(suiteIt->second.size());

                for (const auto& test : suiteIt->second) {
                    auto pt = std::make_shared<std::packaged_task<TestResult()>>(
                        [this, test]() { return RunSingleTest(test); }
                    );
                    futures.push_back(pt->get_future());
                    pool.Submit([pt]() { (*pt)(); });
                }

                for (auto& fut : futures) {
                    TestResult result = fut.get();
                    {
                        std::lock_guard<std::mutex> lock(reporterMutex_);
                        results_.push_back(result);
                        for (auto& rep : reporters_) rep->OnTestEnd(result);
                    }
                    if (failFast_ && result.Failed()) { aborted = true; break; }
                }
            } else {
                // ---- Serial execution ----
                for (const auto& test : suiteIt->second) {
                    ResetTestState();
                    TestResult result = RunSingleTest(test);
                    results_.push_back(result);
                    for (auto& rep : reporters_) rep->OnTestEnd(result);
                    if (failFast_ && result.Failed()) { aborted = true; break; }
                }
            }

            if (suiteSetUpCalled && suiteTearDown) suiteTearDown();

            for (auto& rep : reporters_) rep->OnTestSuiteEnd(suiteName);
            if (aborted) break;
        }

        // Global Environment TearDown (reverse order)
        for (auto it = environments_.rbegin(); it != environments_.rend(); ++it) {
            try { (*it)->TearDown(); }
            catch (const std::exception& e) {
                std::cerr << "Global Environment TearDown failed: " << e.what() << "\n";
            } catch (...) {
                std::cerr << "Global Environment TearDown failed with unknown exception.\n";
            }
        }
    }

    auto overallEnd = std::chrono::high_resolution_clock::now();
    double totalTime = std::chrono::duration<double, std::milli>(overallEnd - overallStart).count();

    for (auto& rep : reporters_) rep->OnTestRunEnd(results_, totalTime);

    int failedCount = 0;
    for (const auto& r : results_) { if (r.Failed()) ++failedCount; }
    return failedCount;
}

// ============================================================================
// RunTestSuite
// ============================================================================

int TestRegistry::RunTestSuite(const std::string& suiteName) {
    std::vector<TestInfo> originalTests = tests_;
    tests_.clear();
    for (const auto& test : originalTests) {
        if (test.suite == suiteName) tests_.push_back(test);
    }
    int result = RunAllTests();
    tests_ = originalTests;
    return result;
}

// ============================================================================
// Reporter / filter / option setters
// ============================================================================

void TestRegistry::AddReporter(std::shared_ptr<IReporter> reporter) {
    reporters_.push_back(reporter);
}

void TestRegistry::ClearReporters() {
    reporters_.clear();
}

void TestRegistry::SetFilter(const std::string& pattern) {
    filterPatterns_.clear();
    if (pattern.empty()) return;
    // Split by ':' for multi-pattern support (e.g. "Math.*:Physics.*")
    std::string token;
    std::istringstream ss(pattern);
    while (std::getline(ss, token, ':')) {
        if (!token.empty()) filterPatterns_.push_back(token);
    }
}

void TestRegistry::ClearFilter() {
    filterPatterns_.clear();
}

void TestRegistry::AddTags(const std::string& suite,
                           const std::string& name,
                           std::vector<std::string> tags) {
    auto key = std::make_pair(suite, name);
    auto& existing = tagMap_[key];
    for (auto& tag : tags) existing.push_back(std::move(tag));
}

void TestRegistry::SetIncludeTags(std::vector<std::string> tags) {
    includeTags_ = std::move(tags);
}

void TestRegistry::SetExcludeTags(std::vector<std::string> tags) {
    excludeTags_ = std::move(tags);
}

void TestRegistry::EnableShuffle(bool enable) { shuffle_ = enable; }
void TestRegistry::SetSeed(uint64_t seed)     { seed_ = seed; }
void TestRegistry::SetRepeat(int count)        { repeatCount_ = std::max(1, count); }
void TestRegistry::EnableFailFast(bool enable) { failFast_ = enable; }
void TestRegistry::SetRunDisabled(bool enable) { runDisabled_ = enable; }
void TestRegistry::SetSlowThreshold(double ms) { slowThresholdMs_ = ms; }
void TestRegistry::SetTimeout(double ms)       { timeoutMs_ = ms; }
void TestRegistry::SetParallelWorkers(int n)   { parallelWorkers_ = std::max(1, n); }

void TestRegistry::SetUseColors(bool enable) {
    useColors_ = enable;
    for (auto& rep : reporters_) {
        if (auto console = std::dynamic_pointer_cast<Reporters::ConsoleReporter>(rep)) {
            console->SetUseColors(enable);
        }
    }
}

} // namespace Core
} // namespace AZTest
