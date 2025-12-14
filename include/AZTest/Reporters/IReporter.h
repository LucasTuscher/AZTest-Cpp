#pragma once

#include "../Core/TestResult.h"
#include <string>
#include <vector>

namespace AZTest {
namespace Core {

/**
 * @brief Interface for test reporters
 *
 * Implement this interface to create custom test output formats.
 */
class IReporter {
public:
    virtual ~IReporter() = default;

    // Called at the start of test execution
    virtual void OnTestRunStart(int totalTests) = 0;

    // Called when a test suite starts
    virtual void OnTestSuiteStart(const std::string& suiteName) = 0;

    // Called when a test starts
    virtual void OnTestStart(const std::string& testName) = 0;

    // Called when a test ends
    virtual void OnTestEnd(const TestResult& result) = 0;

    // Called when a test suite ends
    virtual void OnTestSuiteEnd(const std::string& suiteName) = 0;

    // Called at the end of test execution
    virtual void OnTestRunEnd(const std::vector<TestResult>& results, double totalTimeMs) = 0;
};

} // namespace Core
} // namespace AZTest
