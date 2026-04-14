# AZTest

Ein modernes, leichtgewichtiges C++17 Test-Framework mit vertrauter GoogleTest-ähnlicher Syntax. Entwickelt für Spiele-Engines und Performance-kritische Anwendungen.

[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/std/the-standard)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## Features

- **Modulare Architektur:** Klare Trennung von Core, Reportern, Matchern und Mocks
- **Keine externen Abhängigkeiten:** Benötigt nur die C++17-Standardbibliothek
- **GoogleTest-kompatible Syntax:** Einfache Migration durch Makros wie `TEST()`, `EXPECT_EQ()`, `ASSERT_TRUE()`
- **Flexible Reporter:** Console, XML, JSON oder eigene Callback-Reporter
- **Performance-Tests:** Eingebaute Benchmark-Funktionalität mit `PERFORMANCE_TEST`
- **Test-Fixtures:** `TEST_F` mit `SetUp()`/`TearDown()` und Suite-Level Hooks
- **Parametrisierte Tests:** Datengesteuerte Tests mit `TEST_P` und `INSTANTIATE_TEST_SUITE_P`
- **Typ-Parametrisierte Tests:** `TYPED_TEST` für generische Datentyp-Tests
- **Matcher-System:** Komponierbare Assertions (`EXPECT_THAT`, `HasSubstr`, `StartsWith`, etc.)
- **Mocking-Framework:** Header-only Mocking ohne gMock-Abhängigkeit
- **Property-Based Testing:** Automatisches Fuzzing mit `PROPERTY_TEST`
- **Umfangreiche Assertions:** Gleichheit, Strings, Fließkommazahlen, Exceptions, Container, Prädikate
- **Automatische Test-Suites:** `TEST_CASE` erzeugt Suites aus dem Dateinamen
- **Test-Skipping:** `TEST_SKIP` für vorübergehend deaktivierte Tests

## Inhaltsverzeichnis

- [Schnellstart](#schnellstart)
- [Installation](#installation)
  - [CMake FetchContent](#cmake-fetchcontent)
  - [Git Submodule](#git-submodule)
  - [System-Installation](#system-installation)
- [Grundlagen](#grundlagen)
  - [Einfache Tests](#einfache-tests)
  - [Test-Suites](#test-suites)
  - [Automatische main()](#automatische-main)
- [Assertions](#assertions)
- [Fortgeschrittene Features](#fortgeschrittene-features)
  - [Fixtures](#fixtures)
  - [Parametrisierte Tests](#parametrisierte-tests)
  - [Performance-Tests](#performance-tests)
  - [Matcher](#matcher)
  - [Mocking](#mocking)
  - [Reporter](#reporter)
- [Dokumentation](#dokumentation)
- [Lizenz](#lizenz)

## Schnellstart

**Minimaler Test in 30 Sekunden:**

```cpp
#include <AZTest/AZTest.h>

TEST(Math, Addition) {
    EXPECT_EQ(2 + 2, 4);
}

AZTEST_MAIN();
```

```bash
cmake -B build && cmake --build build && ./build/MyTests
```

## Installation

### CMake FetchContent (Empfohlen)

```cmake
include(FetchContent)
FetchContent_Declare(
  AZTest
  GIT_REPOSITORY https://github.com/Airzade/AZTest-Cpp.git
  GIT_TAG        v2.0.0
)
FetchContent_MakeAvailable(AZTest)

enable_testing()
add_executable(MyTests tests.cpp)
target_link_libraries(MyTests PRIVATE AZTest::AZTest)
add_test(NAME MyTests COMMAND MyTests)
```

**Alternative: AZTestMain für automatische main()**

```cmake
target_link_libraries(MyTests PRIVATE AZTest::AZTestMain)  # Enthält bereits main()
```

### Git Submodule

```bash
git submodule add https://github.com/Airzade/AZTest-Cpp.git third_party/AZTest
```

```cmake
add_subdirectory(third_party/AZTest)
target_link_libraries(MyTests PRIVATE AZTest::AZTest)
```

### System-Installation

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

```cmake
find_package(AZTest REQUIRED)
target_link_libraries(MyTests PRIVATE AZTest::AZTest)
```

## Grundlagen

### Einfache Tests

```cpp
#include <AZTest/AZTest.h>
#include <vector>

TEST(Math, BasicOperations) {
    EXPECT_EQ(2 + 2, 4);
    EXPECT_NE(2 + 2, 5);
    EXPECT_LT(2, 5);
    EXPECT_GT(5, 2);
}

TEST(Strings, Comparison) {
    std::string hello = "Hello";
    EXPECT_STREQ(hello, "Hello");
    EXPECT_NE(hello, "World");
}

TEST(Containers, Vector) {
    std::vector<int> v = {1, 2, 3};
    EXPECT_EQ(v.size(), 3u);
    EXPECT_FALSE(v.empty());
    EXPECT_CONTAINS(v, 2);  // Prüft auf Element
}
```

### Test-Suites

**Automatische Suite aus Dateiname:**

```cpp
// In Datei: MathTests.cpp
TEST_CASE(Addition) {      // Erzeugt Suite "MathTests"
    EXPECT_EQ(2 + 2, 4);   // Test: MathTests.Addition
}
```

**Explizite Suite:**

```cpp
TEST(MathTests, Addition) {
    EXPECT_EQ(2 + 2, 4);   // Test: MathTests.Addition
}

TEST(MathTests, Subtraction) {
    EXPECT_EQ(5 - 3, 2);   // Test: MathTests.Subtraction
}
```

### Automatische main()

**Variante 1: AZTEST_MAIN() Makro**

```cpp
// Am Ende EINER Testdatei:
AZTEST_MAIN();
```

**Variante 2: Manuelle main() mit Custom Reportern**

```cpp
int main() {
    // Optional: Custom Reporter konfigurieren
    auto xmlReporter = std::make_shared<AZTest::Reporters::XMLReporter>("results.xml");
    AZTest::InitializeWithReporters({xmlReporter});
    
    return AZTest::RUN_ALL_TESTS();
}
```

**Variante 3: AZTestMain Bibliothek**

```cmake
target_link_libraries(MyTests PRIVATE AZTest::AZTestMain)  # main() ist bereits enthalten
```

## Assertions (Zusicherungen)

Verwenden Sie `EXPECT_*`-Makros, um nicht-fatale Fehler zu melden. Verwenden Sie `ASSERT_*`, um fatale Fehler zu melden, die den aktuellen Test abbrechen.

### Basis-Assertions
| Makro | Beschreibung |
|-------|-------------|
| `EXPECT_TRUE(cond)` / `ASSERT_TRUE(cond)` | Prüft, dass Bedingung wahr ist |
| `EXPECT_FALSE(cond)` / `ASSERT_FALSE(cond)` | Prüft, dass Bedingung falsch ist |

### Vergleichs-Assertions
| Makro | Beschreibung |
|-------|-------------|
| `EXPECT_EQ(a, b)` / `ASSERT_EQ(a, b)` | Gleich (`a == b`) |
| `EXPECT_NE(a, b)` / `ASSERT_NE(a, b)` | Ungleich (`a != b`) |
| `EXPECT_LT(a, b)` / `ASSERT_LT(a, b)` | Kleiner als (`a < b`) |
| `EXPECT_LE(a, b)` / `ASSERT_LE(a, b)` | Kleiner oder gleich (`a <= b`) |
| `EXPECT_GT(a, b)` / `ASSERT_GT(a, b)` | Größer als (`a > b`) |
| `EXPECT_GE(a, b)` / `ASSERT_GE(a, b)` | Größer oder gleich (`a >= b`) |

### String-Assertions
| Makro | Beschreibung |
|-------|-------------|
| `EXPECT_STREQ(a, b)` / `ASSERT_STREQ(a, b)` | C-Strings gleich |
| `EXPECT_STRNE(a, b)` / `ASSERT_STRNE(a, b)` | C-Strings ungleich |
| `EXPECT_STRCASEEQ(a, b)` / `ASSERT_STRCASEEQ(a, b)` | C-Strings gleich (case-insensitive) |
| `EXPECT_STRCASENE(a, b)` / `ASSERT_STRCASENE(a, b)` | C-Strings ungleich (case-insensitive) |
| `EXPECT_CONTAINS(str, substr)` | String enthält Teilstring |
| `EXPECT_STARTSWITH(str, prefix)` | String beginnt mit Prefix |
| `EXPECT_ENDSWITH(str, suffix)` | String endet mit Suffix |

### Container-Assertions
| Makro | Beschreibung |
|-------|-------------|
| `EXPECT_CONTAINER_EQ(a, b)` | Container-Inhalt gleich |
| `EXPECT_RANGE_EQ(a, b)` | Bereiche gleich (mit `std::begin/end`) |
| `EXPECT_CONTAINS(container, element)` | Element im Container |
| `EXPECT_EMPTY(container)` | Container ist leer |

### Fließkomma-Assertions
| Makro | Beschreibung |
|-------|-------------|
| `EXPECT_NEAR(a, b, epsilon)` / `ASSERT_NEAR(a, b, epsilon)` | Absoluter Fehler <= epsilon |
| `EXPECT_FLOAT_EQ(a, b)` / `ASSERT_FLOAT_EQ(a, b)` | Float-Gleichheit mit Toleranz |
| `EXPECT_DOUBLE_EQ(a, b)` / `ASSERT_DOUBLE_EQ(a, b)` | Double-Gleichheit mit Toleranz |

### Exception-Assertions
| Makro | Beschreibung |
|-------|-------------|
| `EXPECT_THROW(stmt, exc)` / `ASSERT_THROW(stmt, exc)` | Wirft Exception vom Typ `exc` |
| `EXPECT_ANY_THROW(stmt)` / `ASSERT_ANY_THROW(stmt)` | Wirft beliebige Exception |
| `EXPECT_NO_THROW(stmt)` / `ASSERT_NO_THROW(stmt)` | Wirft keine Exception |

### Weitere Assertions
| Makro | Beschreibung |
|-------|-------------|
| `EXPECT_PRED1(pred, v)` / `ASSERT_PRED1(pred, v)` | Prädikat mit 1 Argument |
| `EXPECT_PRED2(pred, a, b)` / `ASSERT_PRED2(pred, a, b)` | Prädikat mit 2 Argumenten |
| `TEST_SKIP(msg)` | Test überspringen (wie `GTEST_SKIP`) |
| `ADD_FAILURE()` / `ADD_FAILURE_AT(file, line)` | Manuelles Fehlschlagen |

## Fortgeschrittene Features

### Fixtures

Fixtures kapseln gemeinsames Setup und Teardown für mehrere Tests.

```cpp
#include <AZTest/AZTest.h>
#include <memory>

class DatabaseFixture : public AZTest::TestFixture {
protected:
    std::unique_ptr<MockDatabase> db_;

    void SetUp() override {
        db_ = std::make_unique<MockDatabase>();
        db_->Connect();
    }

    void TearDown() override {
        if (db_) db_->Disconnect();
        db_.reset();
    }

    // Optional: Suite-Level Hooks
    static void SetUpTestSuite() {
        std::cout << "Suite startet" << std::endl;
    }
    static void TearDownTestSuite() {
        std::cout << "Suite endet" << std::endl;
    }
};

TEST_F(DatabaseFixture, InsertAndRetrieve) {
    db_->Insert(1, "Hello");
    EXPECT_STREQ(db_->Get(1), "Hello");
}

TEST_F(DatabaseFixture, DeleteEntry) {
    db_->Insert(1, "Test");
    db_->Delete(1);
    EXPECT_EQ(db_->Count(), 0u);
}
```

### Parametrisierte Tests

**Wert-parametrisierte Tests:**

```cpp
// 1. Suite-Klasse definieren
class MathParams {};

// 2. Test mit Parameter definieren
TEST_P(MathParams, DoubleIsSum, int) {
    EXPECT_EQ(param * 2, param + param);
}

// 3. Parameter instanziieren
INSTANTIATE_TEST_SUITE_P(DefaultParams, MathParams, DoubleIsSum, 0, 1, 5, 10);
```

**Typ-parametrisierte Tests:**

```cpp
TYPED_TEST_SUITE(MathTest, int, float, double);

TYPED_TEST(MathTest, Addition) {
    TypeParam a = 1;
    TypeParam b = 2;
    EXPECT_EQ(a + b, 3);
}
```

### Performance-Tests

Benchmarks mit automatischer Zeitmessung:

```cpp
PERFORMANCE_TEST(PerformanceTests, VectorAllocation, 100000) {
    std::vector<int> v;
    v.push_back(i);  // i ist der Iterationsindex
}
END_PERFORMANCE_TEST

PERFORMANCE_TEST(PerformanceTests, StringOps, 10000) {
    std::string s = "Test_" + std::to_string(i);
    (void)s;
}
END_PERFORMANCE_TEST
```

**Ad-hoc Benchmarking in regulären Tests:**

```cpp
TEST(StressTests, HeavyComputation) {
    AZTEST_BENCHMARK("Berechnung durchführen");
    
    // Ihr Code hier
    int sum = 0;
    for (int i = 0; i < 1000000; ++i) sum += i;
    
    EXPECT_GT(sum, 0);
}
```

### Matcher

Lesbare, komponierbare Assertions mit `EXPECT_THAT`:

```cpp
#include <AZTest/AZTest.h>
using namespace AZTest::Matchers;

TEST(MatcherDemo, StringMatchers) {
    std::string text = "Hello World";
    
    EXPECT_THAT(text, StartsWith("Hello"));
    EXPECT_THAT(text, EndsWith("World"));
    EXPECT_THAT(text, HasSubstr("lo Wo"));
    EXPECT_THAT(text, Not(StartsWith("Goodbye")));
    
    // Kombination mit AllOf/AnyOf
    EXPECT_THAT(text, AllOf(
        HasSubstr("Hello"),
        HasSubstr("World"),
        Not(EndsWith("!"))
    ));
}

TEST(MatcherDemo, ComparisonMatchers) {
    EXPECT_THAT(42, Eq(42));
    EXPECT_THAT(42, Ne(0));
    EXPECT_THAT(42, Gt(40));
    EXPECT_THAT(42, Lt(50));
    EXPECT_THAT(42, Ge(42));
    EXPECT_THAT(42, Le(42));
}

TEST(MatcherDemo, FloatingPointMatchers) {
    EXPECT_THAT(3.14159, DoubleNear(3.14, 0.01));
}

TEST(MatcherDemo, ContainerMatchers) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    EXPECT_THAT(vec, Contains(3));
    EXPECT_THAT(vec, Each(Gt(0)));      // Alle Elemente > 0
    EXPECT_THAT(vec, SizeIs(5));         // Größe ist 5
    EXPECT_THAT(vec, ElementsAre(1, 2, 3, 4, 5));
}
```

### Mocking

Eingebautes Mocking ohne externe Abhängigkeiten:

```cpp
#include <AZTest/AZTest.h>
#include <AZTest/Mock.h>

class IDataStore {
public:
    virtual ~IDataStore() = default;
    virtual bool Save(int id) = 0;
    virtual int Load(int id) = 0;
};

class MockDataStore : public IDataStore {
public:
    MOCK_METHOD1(bool, Save, int);      // (ReturnType, MethodName, Arg1Type)
    MOCK_METHOD1(int, Load, int);
};

TEST(MockDemo, BasicMock) {
    MockDataStore mock;
    
    // Verhalten definieren
    mock.mock_Save.WillOnce([](int id) { return id > 0; });
    mock.mock_Load.Returns(42);
    
    EXPECT_TRUE(mock.Save(1));
    EXPECT_EQ(mock.Load(999), 42);
    
    // Aufrufanzahl prüfen
    EXPECT_EQ(AZTest::Mock::MockRegistry::Instance().GetCallCount("Save"), 1);
}

TEST(MockDemo, WithEXPECT_CALL) {
    MockDataStore mock;
    
    EXPECT_CALL(mock, Save).WillRepeatedly([](int id) { return true; });
    
    EXPECT_TRUE(mock.Save(1));
    EXPECT_TRUE(mock.Save(2));
}
```

### Reporter

AZTest unterstützt verschiedene Ausgabeformate:

```cpp
#include <AZTest/AZTest.h>

int main() {
    using namespace AZTest;
    using namespace AZTest::Reporters;
    
    // Mehrere Reporter gleichzeitig
    auto consoleReporter = std::make_shared<ConsoleReporter>();
    auto xmlReporter = std::make_shared<XMLReporter>("test_results.xml");
    auto jsonReporter = std::make_shared<JSONReporter>("test_results.json");
    
    std::vector<std::shared_ptr<Core::IReporter>> reporters = {
        consoleReporter,
        xmlReporter,
        jsonReporter
    };
    
    InitializeWithReporters(reporters);
    
    return RUN_ALL_TESTS();
}
```

**Verfügbare Reporter:**
- `ConsoleReporter` - Farbige Konsolenausgabe
- `XMLReporter` - JUnit-kompatibles XML für CI/CD
- `JSONReporter` - JSON-Format für externe Tools
- `CallbackReporter` - Eigene Callback-Funktionen

**Einfache Initialisierung:**

```cpp
int main() {
    AZTest::InitializeDefaultReporter();  // Nur ConsoleReporter
    return AZTest::RUN_ALL_TESTS();
}
```

## Dokumentation

Detaillierte Anleitungen zu allen Features:

- **[1. Grundlagen](./docs/01-Grundlagen.md)** - Tests schreiben und ausführen
- **[2. Assertions](./docs/02-Assertions.md)** - Alle Prüf-Makros im Detail
- **[3. Fixtures](./docs/03-Fixtures.md)** - Setup/Teardown und Test-Suites
- **[4. Fortgeschrittene Themen](./docs/04-Fortgeschrittene-Themen.md)** - Parametrisierte Tests, Tracing

## Projektstruktur

```
AZTest-Cpp/
├── include/AZTest/        # Header-Dateien
│   ├── AZTest.h          # Hauptheader
│   ├── Core/             # Test-Kern (Fixture, Registry, Result)
│   ├── Matchers.h        # Matcher-System
│   ├── Mock.h            # Mocking-Framework
│   ├── PropertyTest.h    # Property-Based Testing
│   ├── Reporters/        # Ausgabe-Formate
│   └── Utils/            # Hilfsfunktionen (Benchmark)
├── src/                   # Implementierungen
├── examples/              # Beispiel-Code
│   ├── BasicExample.cpp
│   ├── AdvancedExample.cpp
│   └── FixtureExample.cpp
└── docs/                  # Detaillierte Dokumentation
```

## Lizenz

AZTest wird unter der MIT-Lizenz veröffentlicht. Siehe [LICENSE](LICENSE) für Details.
