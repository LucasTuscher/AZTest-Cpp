#include <AZTest/AZTest.h>
#include <string>
#include <vector>
#include <stdexcept>

using namespace AZTest;

// ============================================================================
// BASIC TESTS
// ============================================================================

TEST_CASE(SimpleAssertion) {
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
}

TEST_CASE(IntegerComparison) {
    int a = 5;
    int b = 10;

    EXPECT_EQ(a, 5);
    EXPECT_NE(a, b);
    EXPECT_LT(a, b);
    EXPECT_GT(b, a);
}

TEST_CASE(StringOperations) {
    std::string hello = "Hello";
    std::string world = "World";

    EXPECT_STREQ(hello, "Hello");
    EXPECT_NE(hello, world);
}

TEST_CASE(FloatingPointComparison) {
    float pi = 3.14159f;
    float approxPi = 3.14f;

    EXPECT_NEAR(pi, approxPi, 0.01f);
}

// ============================================================================
// MATH TESTS
// ============================================================================

TEST_CASE(Addition) {
    EXPECT_EQ(2 + 2, 4);
    EXPECT_EQ(10 + 5, 15);
}

TEST_CASE(Division) {
    EXPECT_EQ(10 / 2, 5);
    EXPECT_NEAR(10.0f / 3.0f, 3.333f, 0.001f);
}

// ============================================================================
// CONTAINER TESTS
// ============================================================================

TEST_CASE(VectorOperations) {
    std::vector<int> vec = {1, 2, 3, 4, 5};

    EXPECT_EQ(vec.size(), 5u);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[4], 5);

    vec.push_back(6);
    EXPECT_EQ(vec.size(), 6u);
}

TEST_CASE(VectorEmpty) {
    std::vector<int> vec;

    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0u);

    vec.push_back(1);
    EXPECT_FALSE(vec.empty());
}

TEST_CASE(SkippedPlaceholder) {
    TEST_SKIP("Feature not implemented yet");
}

TEST_CASE(ExceptionCheck) {
    auto willThrow = [] { throw std::runtime_error("boom"); };
    EXPECT_THROW(willThrow(), std::runtime_error);
    EXPECT_NO_THROW(int x = 42; (void)x;);
}

TEST_CASE(StringContains) {
    std::string text = "The quick brown fox";
    EXPECT_CONTAINS(text, "quick");
}

TEST_CASE(VectorRangeEquality) {
    std::vector<int> a{1, 2, 3};
    std::vector<int> b{1, 2, 3};
    EXPECT_RANGE_EQ(a, b);
}

// ============================================================================
// PARAMETERIZED TESTS
// ============================================================================

TEST_P(MathParams, DoubleIsSum, int) {
    EXPECT_EQ(param * 2, param + param);
}

INSTANTIATE_TEST_SUITE_P(DefaultParams, MathParams, DoubleIsSum, 0, 1, 5, 10);

// ============================================================================
// MAIN
// ============================================================================

AZTEST_MAIN();
