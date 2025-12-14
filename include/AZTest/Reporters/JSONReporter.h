#pragma once

#include "IReporter.h"
#include <fstream>
#include <string>
#include <vector>

namespace AZTest {
namespace Reporters {

class JSONReporter : public Core::IReporter {
private:
    std::string outputPath_;
    std::vector<Core::TestResult> results_;

public:
    explicit JSONReporter(const std::string& outputPath = "test_results.json");
    ~JSONReporter() override = default;

    void OnTestRunStart(int totalTests) override;
    void OnTestSuiteStart(const std::string& suiteName) override;
    void OnTestStart(const std::string& testName) override;
    void OnTestEnd(const Core::TestResult& result) override;
    void OnTestSuiteEnd(const std::string& suiteName) override;
    void OnTestRunEnd(const std::vector<Core::TestResult>& results, double totalTimeMs) override;

private:
    void WriteResults(const std::vector<Core::TestResult>& results, double totalTimeMs);
    static std::string Escape(const std::string& s);
    static std::string StatusToString(Core::TestStatus status);
};

} // namespace Reporters
} // namespace AZTest
