#include <AZTest/AZTest.h>
#include <stdexcept>
#include <string>
#include <vector>

// ============================================================================
// SCOPED_TRACE Example
// ============================================================================

TEST(FeatureDemos, ScopedTrace) {
    SCOPED_TRACE("This is the outer scope trace.");
    int x = 10;
    {
        SCOPED_TRACE("This is an inner scope trace, x is 10.");
        EXPECT_EQ(x, 10);
    }
    EXPECT_EQ(x, 10); // This should pass
}

// ============================================================================
// String Assertion Examples
// ============================================================================

TEST(FeatureDemos, StringAssertions) {
    const char* hello = "Hello";
    const char* world = "World";
    const char* hello_de = "hELLo";

    // Case-sensitive comparisons
    EXPECT_STREQ(hello, "Hello");
    EXPECT_STRNE(hello, world);

    ASSERT_STREQ(hello, "Hello");
    ASSERT_STRNE(hello, world);

    // Case-insensitive comparisons
    EXPECT_STRCASEEQ(hello, hello_de);
    ASSERT_STRCASEEQ(hello, hello_de);

    EXPECT_STRCASENE(hello, world);
    ASSERT_STRCASENE(hello, world);
}

// ============================================================================
// Floating Point and Exception Examples
// ============================================================================

void FunctionThatThrows() {
    throw std::runtime_error("This is an expected exception.");
}

void FunctionThatDoesNotThrow() {
    // This function is empty and does not throw.
}

TEST(FeatureDemos, FloatAndException) {
    // Floating point comparison
    double d1 = 1.0;
    double d2 = 1.0000001;
    EXPECT_NEAR(d1, d2, 0.0001);
    ASSERT_NEAR(d1, d2, 0.0001);

    // Exception checking
    EXPECT_THROW(FunctionThatThrows(), std::runtime_error);
    ASSERT_THROW(FunctionThatThrows(), std::runtime_error);

    EXPECT_ANY_THROW(FunctionThatThrows());
    ASSERT_ANY_THROW(FunctionThatThrows());

    EXPECT_NO_THROW(FunctionThatDoesNotThrow());
    ASSERT_NO_THROW(FunctionThatDoesNotThrow());
}

// ============================================================================
// Value-Parametrized Test Example
// ============================================================================

// You need a test suite class for parametrized tests
class ParametrizedMathTest {};

// Define a parametrized test case
TEST_P(ParametrizedMathTest, IsEven, int) {
    SCOPED_TRACE("Testing with param: " + std::to_string(param));
    EXPECT_EQ(param % 2, 0);
}

// Instantiate the test case with a set of values
INSTANTIATE_TEST_SUITE_P(EvenNumbers, ParametrizedMathTest, IsEven, 0, 2, 4, 8, 12);
// INSTANTIATE_TEST_SUITE_P(OddNumbers, ParametrizedMathTest, IsEven, 1, 3, 5, 9, 13); // This would fail

// ============================================================================
// Predicate Assertion Example
// ============================================================================

bool IsPositive(int n) {
    return n > 0;
}

bool IsSumCorrect(int a, int b) {
    return (a + b) > 0;
}

TEST(FeatureDemos, PredicateAssertions) {
    EXPECT_PRED1(IsPositive, 5);
    ASSERT_PRED1(IsPositive, 1);

    EXPECT_PRED2(IsSumCorrect, 5, 10);
    ASSERT_PRED2(IsSumCorrect, -2, 3);
}

// ============================================================================
// Other Features Example
// ============================================================================

TEST(MiscFeatures, Skip) {
    TEST_SKIP("This test is skipped intentionally for demonstration.");
    // This code will not be executed.
    EXPECT_EQ(1, 2);
}

TEST(MiscFeatures, ManualFailure) {
    bool some_condition = false;
    if (!some_condition) {
        ADD_FAILURE() << "Manual failure because some_condition was false.";
    }
}

TEST(DISABLED_MiscFeatures, DisabledTest) {
    // This test will not be run by the test runner because its suite
    // is prefixed with DISABLED_.
    EXPECT_EQ(1, 1);
}

AZTEST_MAIN();
