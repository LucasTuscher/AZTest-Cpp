#pragma once

#include "IReporter.h"
#include <fstream>
#include <string>

namespace AZTest {
namespace Reporters {

/**
 * @brief XML reporter for CI/CD integration (JUnit format)
 */
class XMLReporter : public Core::IReporter {
private:
    std::string outputPath_;
    std::ofstream file_;
    std::string currentSuite_;
    std::vector<Core::TestResult> suiteResults_;

public:
    explicit XMLReporter(const std::string& outputPath = "test_results.xml");
    ~XMLReporter() override;

    void OnTestRunStart(int totalTests) override;
    void OnTestSuiteStart(const std::string& suiteName) override;
    void OnTestStart(const std::string& testName) override;
    void OnTestEnd(const Core::TestResult& result) override;
    void OnTestSuiteEnd(const std::string& suiteName) override;
    void OnTestRunEnd(const std::vector<Core::TestResult>& results, double totalTimeMs) override;

private:
    std::string EscapeXML(const std::string& str) const;
};

} // namespace Reporters
} // namespace AZTest
