# AZTest

A lightweight and fast C++17 testing framework in a single header. It's designed for simplicity, requires zero dependencies, and uses a familiar GoogleTest-like syntax.

## Features

- **Single Header:** Just drop `AZTest.h` in your project.
- **Zero Dependencies:** Only the C++17 standard library is needed.
- **GoogleTest Syntax:** Easy to adopt with macros like `TEST()`, `EXPECT_EQ()`, and `ASSERT_TRUE()`.
- **CMake Integration:** Easily integrates into any CMake project via `FetchContent`.
- **Test Fixtures:** Use `TEST_F` for tests that share setup and teardown logic (`SetUp()`/`TearDown()`).
- **Parametrized Tests:** Create data-driven tests with `TEST_P` and `INSTANTIATE_TEST_SUITE_P`.
- **Rich Assertions:** Includes checks for equality, strings, floats, exceptions, and more.
- **Scoped Tracing:** Use `SCOPED_TRACE` to add context to your failure messages.
- **Automatic Main:** The `AZTEST_MAIN()` macro generates the test runner entry point for you.
- **Command-line Control:** Filter, shuffle, repeat, and set timeouts for your tests.

## Quick Start: CMake Integration

This is the recommended way to use AZTest in your project.

### Step 1: Write Your Tests

Create a test file (e.g., `tests/MyTests.cpp`). Include `AZTest/AZTest.h` and add your tests. Use the `AZTEST_MAIN()` macro once in any test file to create the main function.

**`tests/MyTests.cpp`**
```cpp
#include <AZTest/AZTest.h>
#include <string>
#include <vector>

// A simple test case
TEST(Math, Addition) {
    EXPECT_EQ(2 + 2, 4);
    EXPECT_NE(2 + 2, 5);
}

// A different test suite
TEST(Containers, Vector) {
    std::vector<int> v;
    v.push_back(1);
    EXPECT_FALSE(v.empty());
    EXPECT_EQ(v.size(), 1);
}

// This macro creates the main() function for you
AZTEST_MAIN();
```

### Step 2: Configure CMake

In your `CMakeLists.txt`, use `FetchContent` to download AZTest and link it to your test executable.

**`CMakeLists.txt`**
```cmake
cmake_minimum_required(VERSION 3.15)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)

# --- AZTest Integration ---
include(FetchContent)
FetchContent_Declare(
  AZTest
  GIT_REPOSITORY https://github.com/your-github-username/AZTest-Cpp.git # <-- TODO: Change this URL
  GIT_TAG        main # Or a specific release tag
)
FetchContent_MakeAvailable(AZTest)
# --- End AZTest Integration ---

enable_testing()
add_executable(MyProjectTests tests/MyTests.cpp)
target_link_libraries(MyProjectTests PRIVATE AZTest::AZTest)
add_test(NAME MyProjectTests COMMAND MyProjectTests)
```

### Step 3: Build and Run

```bash
cmake -B build
cmake --build build
cd build && ctest
```

## Assertions

Use `EXPECT_*` macros to report non-fatal failures. Use `ASSERT_*` to report fatal failures that abort the current test.

### Basic Assertions
| Macro | Checks |
|---|---|
| `EXPECT_TRUE(cond)` / `ASSERT_TRUE(cond)` | `cond` is true |
| `EXPECT_FALSE(cond)` / `ASSERT_FALSE(cond)`| `cond` is false|

### Comparison Assertions
| Macro | Checks |
|---|---|
| `EXPECT_EQ(a, b)` / `ASSERT_EQ(a, b)` | `a == b` |
| `EXPECT_NE(a, b)` / `ASSERT_NE(a, b)` | `a != b` |
| `EXPECT_LT(a, b)` / `ASSERT_LT(a, b)` | `a < b` |
| `EXPECT_LE(a, b)` / `ASSERT_LE(a, b)` | `a <= b` |
| `EXPECT_GT(a, b)` / `ASSERT_GT(a, b)` | `a > b` |
| `EXPECT_GE(a, b)` / `ASSERT_GE(a, b)` | `a >= b` |

### String Assertions
| Macro | Checks |
|---|---|
| `EXPECT_STREQ(s1, s2)` / `ASSERT_STREQ(s1, s2)` | C-strings `s1` and `s2` are equal |
| `EXPECT_STRNE(s1, s2)` / `ASSERT_STRNE(s1, s2)` | C-strings `s1` and `s2` are not equal |
| `EXPECT_STRCASEEQ(s1, s2)` / `ASSERT_STRCASEEQ(s1, s2)` | C-strings `s1` and `s2` are equal, ignoring case |
| `EXPECT_STRCASENE(s1, s2)` / `ASSERT_STRCASENE(s1, s2)` | C-strings `s1` and `s2` are not equal, ignoring case |

### Floating-Point Assertions
| Macro | Checks |
|---|---|
| `EXPECT_NEAR(v1, v2, err)` / `ASSERT_NEAR(v1, v2, err)` | `abs(v1 - v2) <= err` |

### Exception Assertions
| Macro | Checks |
|---|---|
| `EXPECT_THROW(stmt, ex)` / `ASSERT_THROW(stmt, ex)` | `statement` throws an exception of type `exception_type` |
| `EXPECT_ANY_THROW(stmt)` / `ASSERT_ANY_THROW(stmt)` | `statement` throws any exception |
| `EXPECT_NO_THROW(stmt)` / `ASSERT_NO_THROW(stmt)` | `statement` does not throw any exception |

## Advanced Features

### Scoped Tracing

For more detailed failure logs, especially in loops, use `SCOPED_TRACE`. Any failures within its scope will include the trace message.

```cpp
TEST(MyTests, LoopTest) {
    int values[] = {1, 2, 3, 5, 8};
    for (int val : values) {
        SCOPED_TRACE("Testing with value: " + std::to_string(val));
        EXPECT_TRUE(val < 6); // Fails when val is 8
    }
}
```

### Test Fixtures

For tests that share common setup and teardown code, use a test fixture.

```cpp
class MyDatabaseFixture : public AZTest::TestFixture {
protected:
    void SetUp() override {
        // Connect to a test database
    }
    void TearDown() override {
        // Disconnect and clean up
    }
};

TEST_F(MyDatabaseFixture, CheckUser) {
    // Test logic using the fixture
    ASSERT_TRUE(true);
}
```

### Value-Parametrized Tests

To run the same test logic with different input values, use `TEST_P`.

```cpp
// 1. Define a test suite class (can be empty)
class IsPrimeTest {};

// 2. Define the test logic using `TEST_P`
// The special `param` variable holds the current value
TEST_P(IsPrimeTest, HandlesPrimeNumbers, int) {
    bool is_prime = /* your prime checking logic */;
    EXPECT_TRUE(is_prime) << "Number " << param << " is not prime.";
}

// 3. Instantiate the test suite with a list of values
INSTANTIATE_TEST_SUITE_P(
    PrimeNumbers,      // An arbitrary prefix for the instance
    IsPrimeTest,       // The test suite class
    HandlesPrimeNumbers, // The name of the test
    2, 3, 5, 7, 11, 13 // The values to test with
);
```

## License

AZTest is released under the MIT License.
