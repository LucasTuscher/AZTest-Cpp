#pragma once

namespace AZTest {
namespace Core {

/**
 * @brief Base class for test fixtures
 *
 * Inherit from this class to create test fixtures with setup and teardown.
 *
 * Example:
 * class MyFixture : public TestFixture {
 * protected:
 *     void SetUp() override {
 *         // Setup code
 *     }
 *     void TearDown() override {
 *         // Cleanup code
 *     }
 * };
 */
class TestFixture {
public:
    virtual ~TestFixture() = default;

    // Called before each test
    virtual void SetUp() {}

    // Called after each test
    virtual void TearDown() {}

    // Called once before all tests in the suite
    static void SetUpTestSuite() {}

    // Called once after all tests in the suite
    static void TearDownTestSuite() {}
};

} // namespace Core
} // namespace AZTest
