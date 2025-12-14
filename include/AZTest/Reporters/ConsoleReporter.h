#pragma once

#include "IReporter.h"
#include <iostream>

namespace AZTest {
namespace Reporters {

namespace Color {
    inline const char* Reset()  { return "\033[0m"; }
    inline const char* Red()    { return "\033[31m"; }
    inline const char* Green()  { return "\033[32m"; }
    inline const char* Yellow() { return "\033[33m"; }
    inline const char* Blue()   { return "\033[34m"; }
    inline const char* Cyan()   { return "\033[36m"; }
    inline const char* Bold()   { return "\033[1m"; }
    inline const char* Gray()   { return "\033[90m"; }
}

class ConsoleReporter : public Core::IReporter {
private:
    std::string currentSuite_;
    bool useColors_;

public:
    explicit ConsoleReporter(bool useColors = true)
        : useColors_(useColors) {}

    void OnTestRunStart(int totalTests) override;
    void OnTestSuiteStart(const std::string& suiteName) override;
    void OnTestStart(const std::string& testName) override;
    void OnTestEnd(const Core::TestResult& result) override;
    void OnTestSuiteEnd(const std::string& suiteName) override;
    void OnTestRunEnd(const std::vector<Core::TestResult>& results, double totalTimeMs) override;

private:
    const char* GetColor(const char* color) const {
        return useColors_ ? color : "";
    }
public:
    void SetUseColors(bool use) { useColors_ = use; }
};

} // namespace Reporters
} // namespace AZTest
