#include "../../include/AZTest/Reporters/JSONReporter.h"
#include <sstream>
#include <iomanip>

namespace AZTest {
namespace Reporters {

JSONReporter::JSONReporter(const std::string& outputPath)
    : outputPath_(outputPath) {}

void JSONReporter::OnTestRunStart(int /* totalTests */) {
    results_.clear();
}

void JSONReporter::OnTestSuiteStart(const std::string& /* suiteName */) {
    // No-op
}

void JSONReporter::OnTestStart(const std::string& /* testName */) {
    // No-op
}

void JSONReporter::OnTestEnd(const Core::TestResult& result) {
    results_.push_back(result);
}

void JSONReporter::OnTestSuiteEnd(const std::string& /* suiteName */) {
    // No-op
}

void JSONReporter::OnTestRunEnd(const std::vector<Core::TestResult>& /* results */, double totalTimeMs) {
    WriteResults(results_, totalTimeMs);
}

std::string JSONReporter::Escape(const std::string& s) {
    std::ostringstream oss;
    for (char c : s) {
        switch (c) {
            case '\\': oss << "\\\\"; break;
            case '"': oss << "\\\""; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << c; break;
        }
    }
    return oss.str();
}

std::string JSONReporter::StatusToString(Core::TestStatus status) {
    switch (status) {
        case Core::TestStatus::PASSED: return "PASSED";
        case Core::TestStatus::FAILED: return "FAILED";
        case Core::TestStatus::SKIPPED: return "SKIPPED";
        case Core::TestStatus::CRASHED: return "CRASHED";
    }
    return "UNKNOWN";
}

void JSONReporter::WriteResults(const std::vector<Core::TestResult>& results, double totalTimeMs) {
    int passed = 0;
    int failed = 0;
    int skipped = 0;

    for (const auto& r : results) {
        if (r.Passed()) passed++;
        else if (r.Failed()) failed++;
        else skipped++;
    }

    std::ofstream file(outputPath_);
    if (!file.is_open()) {
        return;
    }

    file << "{\n";
    file << "  \"total\": " << results.size() << ",\n";
    file << "  \"passed\": " << passed << ",\n";
    file << "  \"failed\": " << failed << ",\n";
    file << "  \"skipped\": " << skipped << ",\n";
    file << "  \"time_ms\": " << std::fixed << std::setprecision(3) << totalTimeMs << ",\n";
    file << "  \"tests\": [\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        file << "    {\n";
        file << "      \"suite\": \"" << Escape(r.suiteName) << "\",\n";
        file << "      \"name\": \"" << Escape(r.testName) << "\",\n";
        file << "      \"status\": \"" << StatusToString(r.status) << "\",\n";
        file << "      \"time_ms\": " << std::fixed << std::setprecision(3) << r.executionTimeMs << ",\n";
        file << "      \"file\": \"" << Escape(r.fileName) << "\",\n";
        file << "      \"line\": " << r.lineNumber << ",\n";
        file << "      \"failure_message\": \"" << Escape(r.failureMessage) << "\",\n";
        file << "      \"failures\": [\n";
        for (size_t fi = 0; fi < r.failures.size(); ++fi) {
            const auto& f = r.failures[fi];
            file << "        {\n";
            file << "          \"file\": \"" << Escape(f.fileName) << "\",\n";
            file << "          \"line\": " << f.lineNumber << ",\n";
            file << "          \"message\": \"" << Escape(f.message) << "\"\n";
            file << "        }";
            if (fi + 1 < r.failures.size()) file << ",";
            file << "\n";
        }
        file << "      ],\n";
        file << "      \"slow\": " << (r.slow ? "true" : "false") << ",\n";
        file << "      \"warning_message\": \"" << Escape(r.warningMessage) << "\"\n";
        file << "    }";
        if (i + 1 < results.size()) file << ",";
        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";
    file.close();
}

} // namespace Reporters
} // namespace AZTest
