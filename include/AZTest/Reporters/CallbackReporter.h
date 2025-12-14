#pragma once

#include "IReporter.h"
#include <functional>
#include <string>

namespace AZTest {
namespace Reporters {

/**
 * @brief Reporter that forwards events to user-provided callbacks.
 *
 * Useful for embedding in engines (e.g., Unreal: wrap callbacks with UE_LOG).
 */
class CallbackReporter : public Core::IReporter {
public:
    using LogFn = std::function<void(const std::string&)>;
    using SuiteFn = std::function<void(const std::string&)>;
    using ResultFn = std::function<void(const Core::TestResult&)>;
    using RunEndFn = std::function<void(const std::vector<Core::TestResult>&, double)>;

private:
    LogFn log_;
    SuiteFn suiteStart_;
    SuiteFn suiteEnd_;
    ResultFn onResult_;
    RunEndFn onRunEnd_;

public:
    explicit CallbackReporter(LogFn logFn,
                              SuiteFn suiteStart = nullptr,
                              SuiteFn suiteEnd = nullptr,
                              ResultFn onResult = nullptr,
                              RunEndFn onRunEnd = nullptr)
        : log_(std::move(logFn))
        , suiteStart_(std::move(suiteStart))
        , suiteEnd_(std::move(suiteEnd))
        , onResult_(std::move(onResult))
        , onRunEnd_(std::move(onRunEnd)) {}

    void OnTestRunStart(int totalTests) override {
        if (log_) log_("Run start: " + std::to_string(totalTests) + " tests");
    }

    void OnTestSuiteStart(const std::string& suiteName) override {
        if (suiteStart_) suiteStart_(suiteName);
    }

    void OnTestStart(const std::string& testName) override {
        if (log_) log_("Running: " + testName);
    }

    void OnTestEnd(const Core::TestResult& result) override {
        if (onResult_) onResult_(result);
        else if (log_) {
            std::string line = "[" + result.suiteName + "] " + result.testName + " - ";
            if (result.Passed()) line += "PASS";
            else if (result.Skipped()) line += "SKIP";
            else line += "FAIL";
            if (result.slow && !result.warningMessage.empty()) {
                line += " (" + result.warningMessage + ")";
            }
            log_(line);
        }
    }

    void OnTestSuiteEnd(const std::string& suiteName) override {
        if (suiteEnd_) suiteEnd_(suiteName);
    }

    void OnTestRunEnd(const std::vector<Core::TestResult>& results, double totalTimeMs) override {
        if (onRunEnd_) onRunEnd_(results, totalTimeMs);
        else if (log_) {
            int passed = 0, failed = 0, skipped = 0;
            for (const auto& r : results) {
                if (r.Passed()) passed++;
                else if (r.Failed()) failed++;
                else skipped++;
            }
            log_("Run end: total=" + std::to_string(results.size()) +
                 " pass=" + std::to_string(passed) +
                 " fail=" + std::to_string(failed) +
                 " skip=" + std::to_string(skipped) +
                 " time_ms=" + std::to_string(static_cast<int>(totalTimeMs)));
        }
    }
};

} // namespace Reporters
} // namespace AZTest
