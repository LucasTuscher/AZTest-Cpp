#include "../../include/AZTest/Reporters/ConsoleReporter.h"
#include <iomanip>

namespace AZTest {
namespace Reporters {

void ConsoleReporter::OnTestRunStart(int totalTests) {
    std::cout << GetColor(Color::Bold()) << GetColor(Color::Cyan())
              << "\n========================================\n"
              << "  AZ TEST FRAMEWORK v2.0\n"
              << "========================================\n"
              << GetColor(Color::Reset()) << std::endl;

    std::cout << "Running " << totalTests << " test(s)...\n" << std::endl;
}

void ConsoleReporter::OnTestSuiteStart(const std::string& suiteName) {
    currentSuite_ = suiteName;
    std::cout << GetColor(Color::Bold()) << "\n[" << suiteName << "]\n"
              << GetColor(Color::Reset());
}

void ConsoleReporter::OnTestStart(const std::string& /* testName */) {
    // Optional: Could show "Running: testName..." here
}

void ConsoleReporter::OnTestEnd(const Core::TestResult& result) {
    // Print test status
    if (result.Passed()) {
        std::cout << GetColor(Color::Green()) << "  [PASS] " << GetColor(Color::Reset());
    } else if (result.Failed()) {
        std::cout << GetColor(Color::Red()) << "  [FAIL] " << GetColor(Color::Reset());
    } else {
        std::cout << GetColor(Color::Yellow()) << "  [SKIP] " << GetColor(Color::Reset());
    }

    std::cout << result.testName;
    if (result.slow) {
        std::cout << GetColor(Color::Yellow()) << " [SLOW]" << GetColor(Color::Reset());
    }

    // Show timing for slow tests
    if (result.executionTimeMs > 10.0) {
        std::cout << GetColor(Color::Yellow()) << " ("
                  << std::fixed << std::setprecision(2)
                  << result.executionTimeMs << " ms)"
                  << GetColor(Color::Reset());
    }
    std::cout << std::endl;

    // Show failure details
    if (result.Failed() || result.Skipped() || result.slow) {
        const char* detailColor = result.Failed() ? Color::Red() : (result.Skipped() ? Color::Yellow() : Color::Yellow());
        if (result.Failed() && !result.failures.empty()) {
            for (const auto& f : result.failures) {
                std::cout << GetColor(detailColor)
                          << "      " << f.fileName << ":" << f.lineNumber << "\n";
                std::cout << "      " << f.message << "\n";
            }
        } else {
            std::cout << GetColor(detailColor)
                      << "      " << result.fileName << ":" << result.lineNumber << "\n";
            if (result.Failed() || result.Skipped()) {
                std::cout << "      " << result.failureMessage << "\n";
            }
        }
        if (result.slow) {
            std::cout << "      " << (result.warningMessage.empty() ? "Slow test" : result.warningMessage) << "\n";
        }
        std::cout << GetColor(Color::Reset());
    }
}

void ConsoleReporter::OnTestSuiteEnd(const std::string& /* suiteName */) {
    // Optional: Could show suite summary
}

void ConsoleReporter::OnTestRunEnd(const std::vector<Core::TestResult>& results, double totalTimeMs) {
    int passed = 0;
    int failed = 0;
    int skipped = 0;

    for (const auto& result : results) {
        if (result.Passed()) passed++;
        else if (result.Failed()) failed++;
        else skipped++;
    }

    std::cout << GetColor(Color::Bold()) << GetColor(Color::Cyan())
              << "\n========================================\n"
              << GetColor(Color::Reset());

    std::cout << "Total: " << results.size() << " tests\n";
    std::cout << GetColor(Color::Green()) << "Passed: " << passed << GetColor(Color::Reset()) << "\n";

    if (failed > 0) {
        std::cout << GetColor(Color::Red()) << "Failed: " << failed << GetColor(Color::Reset()) << "\n";
    }

    if (skipped > 0) {
        std::cout << GetColor(Color::Yellow()) << "Skipped: " << skipped << GetColor(Color::Reset()) << "\n";
    }

    std::cout << "Time: " << std::fixed << std::setprecision(2)
              << totalTimeMs << " ms\n";

    std::cout << GetColor(Color::Bold()) << GetColor(Color::Cyan())
              << "========================================\n"
              << GetColor(Color::Reset()) << std::endl;
}

} // namespace Reporters
} // namespace AZTest
