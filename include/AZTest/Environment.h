#pragma once

namespace AZTest {

/**
 * @brief Base class for global test environments.
 * 
 * Environments allow for setting up and tearing down state 
 * globally before any tests are run, and after all tests have completed.
 * This is useful for initializing global resources like database connections,
 * singletons, or external systems required for tests.
 */
class Environment {
public:
    virtual ~Environment() = default;

    /**
     * @brief Called before the first test in the framework runs.
     */
    virtual void SetUp() {}

    /**
     * @brief Called after the last test in the framework has finished running.
     */
    virtual void TearDown() {}
};

} // namespace AZTest
