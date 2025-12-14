#include "../../include/AZTest/Reporters/XMLReporter.h"
#include <algorithm>
#include <iomanip>

namespace AZTest {
namespace Reporters {

XMLReporter::XMLReporter(const std::string& outputPath)
    : outputPath_(outputPath) {
}

XMLReporter::~XMLReporter() {
    if (file_.is_open()) {
        file_.close();
    }
}

void XMLReporter::OnTestRunStart(int /* totalTests */) {
    file_.open(outputPath_);
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open XML output file: " + outputPath_);
    }

    file_ << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file_ << "<testsuites>\n";
}

void XMLReporter::OnTestSuiteStart(const std::string& suiteName) {
    currentSuite_ = suiteName;
    suiteResults_.clear();
}

void XMLReporter::OnTestStart(const std::string& /* testName */) {
    // Nothing to do
}

void XMLReporter::OnTestEnd(const Core::TestResult& result) {
    suiteResults_.push_back(result);
}

void XMLReporter::OnTestSuiteEnd(const std::string& suiteName) {
    if (suiteResults_.empty()) {
        return;
    }

    int tests = static_cast<int>(suiteResults_.size());
    int failures = 0;
    int skipped = 0;
    double totalTime = 0.0;

    for (const auto& result : suiteResults_) {
        if (result.Failed()) failures++;
        if (result.Skipped()) skipped++;
        totalTime += result.executionTimeMs;
    }

    file_ << "  <testsuite name=\"" << EscapeXML(suiteName) << "\" "
          << "tests=\"" << tests << "\" "
          << "failures=\"" << failures << "\" "
          << "skipped=\"" << skipped << "\" "
          << "time=\"" << std::fixed << std::setprecision(3) << (totalTime / 1000.0) << "\">\n";

    for (const auto& result : suiteResults_) {
        file_ << "    <testcase name=\"" << EscapeXML(result.testName) << "\" "
              << "classname=\"" << EscapeXML(result.suiteName) << "\" "
              << "time=\"" << std::fixed << std::setprecision(3) << (result.executionTimeMs / 1000.0) << "\"";

        if (result.Failed()) {
            file_ << ">\n";
            file_ << "      <failure message=\"" << EscapeXML(result.failureMessage) << "\">\n";
            file_ << EscapeXML(result.fileName) << ":" << result.lineNumber << "\n";
            file_ << EscapeXML(result.failureMessage) << "\n";
            file_ << "      </failure>\n";
            file_ << "    </testcase>\n";
        } else if (result.Skipped()) {
            file_ << ">\n";
            file_ << "      <skipped/>\n";
            file_ << "    </testcase>\n";
        } else {
            file_ << "/>\n";
        }
    }

    file_ << "  </testsuite>\n";
}

void XMLReporter::OnTestRunEnd(const std::vector<Core::TestResult>& /* results */, double /* totalTimeMs */) {
    file_ << "</testsuites>\n";
    file_.close();
}

std::string XMLReporter::EscapeXML(const std::string& str) const {
    std::string result;
    result.reserve(str.size());

    for (char c : str) {
        switch (c) {
            case '&':  result += "&amp;";  break;
            case '<':  result += "&lt;";   break;
            case '>':  result += "&gt;";   break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            default:   result += c;        break;
        }
    }

    return result;
}

} // namespace Reporters
} // namespace AZTest
