#pragma once

#include <string>
#include <chrono>

namespace AZTest {
namespace Core {

enum class TestStatus {
    PASSED,
    FAILED,
    SKIPPED,
    CRASHED
};

struct TestResult {
    std::string testName;
    std::string suiteName;
    TestStatus status;
    std::string failureMessage;
    std::string fileName;
    int lineNumber;
    double executionTimeMs;
    bool slow;
    std::string warningMessage;

    TestResult()
        : status(TestStatus::PASSED)
        , lineNumber(0)
        , executionTimeMs(0.0)
        , slow(false) {}

    bool Passed() const { return status == TestStatus::PASSED; }
    bool Failed() const { return status == TestStatus::FAILED; }
    bool Skipped() const { return status == TestStatus::SKIPPED; }
};

} // namespace Core
} // namespace AZTest
