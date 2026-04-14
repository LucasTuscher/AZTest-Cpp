#include "../../include/AZTest/Core/TestRegistry.h"
#include "../../include/AZTest/Reporters/IReporter.h"
#include "../../include/AZTest/AZTest.h"
#include "../../include/AZTest/Environment.h"
#include <chrono>
#include <algorithm>
#include <random>
#include <future>
#include <thread>
 #include <sstream>
 #include <unordered_map>

namespace AZTest {
namespace Core {

TestRegistry::TestRegistry()
    : shuffle_(false)
    , seed_(0)
    , repeatCount_(1)
    , failFast_(false)
    , runDisabled_(false)
    , slowThresholdMs_(0.0)
    , useColors_(true)
    , timeoutMs_(0.0)
    , currentTestFailed_(false)
    , currentTestSkipped_(false)
    , currentLine_(0) {
}

TestRegistry::~TestRegistry() {
    for (auto* env : environments_) {
        delete env;
    }
    environments_.clear();
}

void TestRegistry::AddEnvironment(AZTest::Environment* env) {
    if (env) {
        environments_.push_back(env);
    }
}

TestRegistry& TestRegistry::Instance() {
    static TestRegistry instance;
    return instance;
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

void TestRegistry::RecordFailure(const std::string& msg, const char* file, int line) {
    currentTestFailed_ = true;
    TestFailure failure;
    failure.message = GetTraceStack() + msg;
    failure.fileName = file ? file : "";
    failure.lineNumber = line;
    currentFailures_.push_back(failure);

    currentFailureMsg_ = failure.message;
    currentFile_ = failure.fileName;
    currentLine_ = failure.lineNumber;
}

void TestRegistry::RecordSkip(const std::string& msg, const char* file, int line) {
    currentTestSkipped_ = true;
    currentSkipMsg_ = msg;
    currentFile_ = file;
    currentLine_ = line;
}

void TestRegistry::PushTrace(const std::string& trace) {
    traceStack_.push_back(trace);
}

void TestRegistry::PopTrace() {
    if (!traceStack_.empty()) {
        traceStack_.pop_back();
    }
}

std::string TestRegistry::GetTraceStack() const {
    if (traceStack_.empty()) {
        return "";
    }
    std::stringstream ss;
    ss << "Trace:\n";
    for (const auto& trace : traceStack_) {
        ss << "  " << trace << "\n";
    }
    return ss.str();
}

void TestRegistry::ResetTestState() {
    currentTestFailed_ = false;
    currentTestSkipped_ = false;
    currentFailureMsg_.clear();
    currentFailures_.clear();
    currentSkipMsg_.clear();
    currentFile_.clear();
    currentLine_ = 0;
    traceStack_.clear();
}

namespace {
std::string JoinFailureMessages(const std::vector<TestFailure>& failures) {
    std::ostringstream oss;
    for (size_t i = 0; i < failures.size(); ++i) {
        if (i > 0) {
            oss << "\n";
        }
        oss << failures[i].message;
    }
    return oss.str();
}
}


namespace {
bool WildcardMatch(const std::string& pattern, const std::string& value) {
    // Simple glob matcher supporting '*' and '?'
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
    while (p < pattern.size() && pattern[p] == '*') {
        ++p;
    }
    return p == pattern.size();
}
} // namespace

int TestRegistry::RunAllTests() {
    if (reporters_.empty()) {
        // Add default console reporter if none specified
        auto consoleReporter = std::make_shared<Reporters::ConsoleReporter>(useColors_);
        reporters_.push_back(consoleReporter);
    }

    results_.clear();

    // Build filtered list of tests
    std::vector<TestInfo> runnable;
    runnable.reserve(tests_.size());
    for (const auto& t : tests_) {
        if (filterPattern_.empty()) {
            runnable.push_back(t);
        } else {
            std::string fullName = t.suite + "." + t.name;
            if (WildcardMatch(filterPattern_, fullName)) {
                runnable.push_back(t);
            }
        }
    }

    // Group by suite to guarantee stable per-suite lifecycle even when shuffling.
    std::vector<std::string> baseSuiteOrder;
    baseSuiteOrder.reserve(runnable.size());
    std::unordered_map<std::string, std::vector<TestInfo>> baseSuiteTests;
    baseSuiteTests.reserve(runnable.size());
    for (const auto& t : runnable) {
        auto it = baseSuiteTests.find(t.suite);
        if (it == baseSuiteTests.end()) {
            baseSuiteOrder.push_back(t.suite);
            baseSuiteTests.emplace(t.suite, std::vector<TestInfo>{t});
        } else {
            it->second.push_back(t);
        }
    }

    // Notify reporters
    for (auto& reporter : reporters_) {
        reporter->OnTestRunStart(static_cast<int>(runnable.size() * repeatCount_));
    }

    auto overallStart = std::chrono::high_resolution_clock::now();

    bool aborted = false;
    uint64_t effectiveSeed = (seed_ != 0) ? seed_ : static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());

    for (int iteration = 0; iteration < repeatCount_ && !aborted; ++iteration) {
        // Global Environment SetUp
        for (auto* env : environments_) {
            try {
                env->SetUp();
            } catch (const std::exception& e) {
                std::cerr << "Global Environment SetUp failed: " << e.what() << "\n";
                aborted = true;
                break;
            } catch (...) {
                std::cerr << "Global Environment SetUp failed with unknown exception.\n";
                aborted = true;
                break;
            }
        }
        if (aborted) break;
        std::vector<std::string> suiteOrder = baseSuiteOrder;
        std::unordered_map<std::string, std::vector<TestInfo>> suiteTests = baseSuiteTests;

        if (shuffle_) {
            std::mt19937_64 rng(effectiveSeed + iteration);
            std::shuffle(suiteOrder.begin(), suiteOrder.end(), rng);
            for (auto& kv : suiteTests) {
                std::shuffle(kv.second.begin(), kv.second.end(), rng);
            }
        }

        for (const auto& suiteName : suiteOrder) {
            auto suiteIt = suiteTests.find(suiteName);
            if (suiteIt == suiteTests.end()) {
                continue;
            }

            for (auto& reporter : reporters_) {
                reporter->OnTestSuiteStart(suiteName);
            }

            bool suiteSetUpCalled = false;
            std::function<void()> suiteTearDown;

            for (const auto& test : suiteIt->second) {
                // If this suite contains fixture tests and they appear after non-fixture tests,
                // we still need to call suite setup exactly once before the first fixture test.
                if (!suiteSetUpCalled && test.isFixture && test.setUpFunc) {
                    test.setUpFunc();
                    suiteSetUpCalled = true;
                    suiteTearDown = test.tearDownFunc;
                }

                ResetTestState();

                for (auto& reporter : reporters_) {
                    reporter->OnTestStart(test.name);
                }

                // Run test with timing and optional timeout
                auto start = std::chrono::high_resolution_clock::now();

                if (test.disabled) {
                    if (!runDisabled_) {
                        RecordSkip("Disabled test (name or suite starts with DISABLED_)", test.file.c_str(), test.line);
                    } else {
                        try {
                            test.testFunc();
                        } catch (const AssertionFailure& e) {
                            (void)e;
                        } catch (const std::exception& e) {
                            currentTestFailed_ = true;
                            currentFailureMsg_ = std::string("Uncaught exception: ") + e.what();
                            currentFile_ = test.file;
                            currentLine_ = test.line;
                        } catch (...) {
                            currentTestFailed_ = true;
                            currentFailureMsg_ = "Unknown exception thrown";
                            currentFile_ = test.file;
                            currentLine_ = test.line;
                        }
                    }
                } else if (timeoutMs_ > 0.0) {
                    std::packaged_task<void()> task([&test]() { test.testFunc(); });
                    auto fut = task.get_future();
                    std::thread runner(std::move(task));

                    auto status = fut.wait_for(std::chrono::milliseconds(static_cast<int>(timeoutMs_)));
                    if (status == std::future_status::ready) {
                        try {
                            fut.get();
                        } catch (const AssertionFailure& e) {
                            (void)e; // Already recorded
                        } catch (const std::exception& e) {
                            currentTestFailed_ = true;
                            currentFailureMsg_ = std::string("Uncaught exception: ") + e.what();
                            currentFile_ = test.file;
                            currentLine_ = test.line;
                        } catch (...) {
                            currentTestFailed_ = true;
                            currentFailureMsg_ = "Unknown exception thrown";
                            currentFile_ = test.file;
                            currentLine_ = test.line;
                        }
                        if (runner.joinable()) runner.join();
                    } else {
                        // Timeout
                        currentTestFailed_ = true;
                        currentFailureMsg_ = "Test timed out after " + std::to_string(timeoutMs_) + " ms";
                        currentFile_ = test.file;
                        currentLine_ = test.line;
                        if (runner.joinable()) {
                            runner.detach(); // let it run out; we mark as failed
                        }
                    }
                } else {
                    try {
                        test.testFunc();
                    } catch (const AssertionFailure& e) {
                        (void)e; // Already recorded in RecordFailure
                    } catch (const std::exception& e) {
                        currentTestFailed_ = true;
                        currentFailureMsg_ = std::string("Uncaught exception: ") + e.what();
                        currentFile_ = test.file;
                        currentLine_ = test.line;
                    } catch (...) {
                        currentTestFailed_ = true;
                        currentFailureMsg_ = "Unknown exception thrown";
                        currentFile_ = test.file;
                        currentLine_ = test.line;
                    }
                }

                auto end = std::chrono::high_resolution_clock::now();
                double timeMs = std::chrono::duration<double, std::milli>(end - start).count();

            // Create result
            TestResult result;
            result.testName = test.name;
            result.suiteName = test.suite;
            if (currentTestSkipped_) {
                result.status = TestStatus::SKIPPED;
                result.failureMessage = currentSkipMsg_;
                result.fileName = currentFile_.empty() ? test.file : currentFile_;
                result.lineNumber = currentLine_ == 0 ? test.line : currentLine_;
            } else if (currentTestFailed_) {
                result.status = TestStatus::FAILED;
                result.failures = currentFailures_;
                result.failureMessage = !result.failures.empty() ? JoinFailureMessages(result.failures) : currentFailureMsg_;
                if (!result.failures.empty()) {
                    result.fileName = result.failures.front().fileName.empty() ? test.file : result.failures.front().fileName;
                    result.lineNumber = result.failures.front().lineNumber == 0 ? test.line : result.failures.front().lineNumber;
                } else {
                    result.fileName = currentFile_.empty() ? test.file : currentFile_;
                    result.lineNumber = currentLine_ == 0 ? test.line : currentLine_;
                }
            } else {
                result.status = TestStatus::PASSED;
                result.failureMessage.clear();
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
                results_.push_back(result);

                for (auto& reporter : reporters_) {
                    reporter->OnTestEnd(result);
                }

                if (failFast_ && result.Failed()) {
                    aborted = true;
                    break;
                }
            }

            if (suiteSetUpCalled && suiteTearDown) {
                suiteTearDown();
            }

            for (auto& reporter : reporters_) {
                reporter->OnTestSuiteEnd(suiteName);
            }

            if (aborted) {
                break;
            }
        }
        
        // Global Environment TearDown
        for (auto it = environments_.rbegin(); it != environments_.rend(); ++it) {
            try {
                (*it)->TearDown();
           } catch (const std::exception& e) {
                std::cerr << "Global Environment TearDown failed: " << e.what() << "\n";
            } catch (...) {
                std::cerr << "Global Environment TearDown failed with unknown exception.\n";
            }
        }
    }

    auto overallEnd = std::chrono::high_resolution_clock::now();
    double totalTime = std::chrono::duration<double, std::milli>(overallEnd - overallStart).count();

    for (auto& reporter : reporters_) {
        reporter->OnTestRunEnd(results_, totalTime);
    }

    // Count failures
    int failedCount = 0;
    for (const auto& result : results_) {
        if (result.Failed()) {
            failedCount++;
        }
    }

    return failedCount;
}

int TestRegistry::RunTestSuite(const std::string& suiteName) {
    // Filter tests by suite name
    std::vector<TestInfo> originalTests = tests_;
    tests_.clear();

    for (const auto& test : originalTests) {
        if (test.suite == suiteName) {
            tests_.push_back(test);
        }
    }

    int result = RunAllTests();

    // Restore original tests
    tests_ = originalTests;

    return result;
}

void TestRegistry::AddReporter(std::shared_ptr<IReporter> reporter) {
    reporters_.push_back(reporter);
}

void TestRegistry::ClearReporters() {
    reporters_.clear();
}

void TestRegistry::SetFilter(const std::string& pattern) {
    filterPattern_ = pattern;
}

void TestRegistry::ClearFilter() {
    filterPattern_.clear();
}

void TestRegistry::EnableShuffle(bool enable) {
    shuffle_ = enable;
}

void TestRegistry::SetSeed(uint64_t seed) {
    seed_ = seed;
}

void TestRegistry::SetRepeat(int count) {
    repeatCount_ = std::max(1, count);
}

void TestRegistry::EnableFailFast(bool enable) {
    failFast_ = enable;
}

void TestRegistry::SetRunDisabled(bool enable) {
    runDisabled_ = enable;
}

void TestRegistry::SetSlowThreshold(double ms) {
    slowThresholdMs_ = ms;
}

void TestRegistry::SetUseColors(bool enable) {
    useColors_ = enable;
    for (auto& rep : reporters_) {
        if (auto console = std::dynamic_pointer_cast<Reporters::ConsoleReporter>(rep)) {
            console->SetUseColors(enable);
        }
    }
}

void TestRegistry::SetTimeout(double ms) {
    timeoutMs_ = ms;
}

} // namespace Core
} // namespace AZTest
