#pragma once

#include "AZTest.h"
#include "Reporters/CallbackReporter.h"
#include <functional>
#include <string>
#include <vector>

namespace AZTest {
namespace Integrations {

// Build reporter set that logs via provided callback; optional JSON output.
inline std::vector<std::shared_ptr<Core::IReporter>> MakeUnrealReporters(
    const std::function<void(const std::string&)>& logFn,
    const std::string& jsonPath = "") {
    std::vector<std::shared_ptr<Core::IReporter>> reps;
    reps.push_back(std::make_shared<Reporters::CallbackReporter>(logFn));
    if (!jsonPath.empty()) {
        reps.push_back(std::make_shared<Reporters::JSONReporter>(jsonPath));
    }
    return reps;
}

inline int RunUnrealTests(const std::function<void(const std::string&)>& logFn,
                          int argc, char** argv,
                          const std::string& jsonPath = "") {
    Core::TestRegistry::Instance().ClearReporters();
    auto reps = MakeUnrealReporters(logFn, jsonPath);
    AZTest::InitializeWithReporters(reps);
    return AZTest::RunWithArgs(argc, argv);
}

#define AZTEST_UE_MAIN(log_lambda) \
    int main(int argc, char** argv) { \
        auto _et_log = (log_lambda); \
        return AZTest::Integrations::RunUnrealTests(_et_log, argc, argv); \
    }

#define UE_TEST(suite, name) TEST(suite, name)
#define UE_TEST_F(fixture, name) TEST_F(fixture, name)
#define UE_TEST_CASE(name) TEST_CASE(name)

} // namespace Integrations
} // namespace AZTest
