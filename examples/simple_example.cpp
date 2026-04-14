#include "AZTest.h"

// Simple tests
TEST(Math, Addition) {
    EXPECT_EQ(2 + 2, 4);
}

TEST(Math, Subtraction) {
    EXPECT_EQ(10 - 5, 5);
}

TEST(String, Comparison) {
    std::string hello = "Hello";
    EXPECT_EQ(hello, "Hello");
}

AZTEST_MAIN();
