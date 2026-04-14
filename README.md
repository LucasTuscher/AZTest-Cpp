# AZTest

Ein leichtgewichtiges und schnelles C++17 Test-Framework in einer einzigen Header-Datei. Es ist auf Einfachheit ausgelegt, hat keine externen Abhängigkeiten und verwendet eine vertraute, GoogleTest-ähnliche Syntax.

## Features

- **Einzelne Header-Datei:** Fügen Sie einfach `AZTest.h` zu Ihrem Projekt hinzu.
- **Keine Abhängigkeiten:** Benötigt nur die C++17-Standardbibliothek.
- **GoogleTest-Syntax:** Einfache Übernahme durch Makros wie `TEST()`, `EXPECT_EQ()` und `ASSERT_TRUE()`.
- **CMake-Integration:** Lässt sich leicht via `FetchContent` in jedes CMake-Projekt integrieren.
- **Test-Fixtures:** Verwenden Sie `TEST_F` für Tests, die gemeinsame Setup- und Teardown-Logik teilen (`SetUp()`/`TearDown()`).
- **Parametrisierte Tests:** Erstellen Sie datengesteuerte Tests mit `TEST_P` und `INSTANTIATE_TEST_SUITE_P`.
- **Typ-Parametrisierte Tests:** Führen Sie den gleichen Test für verschiedene Datentypen aus mit `TYPED_TEST`.
- **Matcher System:** Komponierbare Assertions ähnlich wie gMock (`EXPECT_THAT()`).
- **Mocking Framework:** Eingebautes Mocking mit `MOCK_METHOD()` und `EXPECT_CALL()`.
- **Property-Based Testing:** Automatisches Fuzzing / randomisierte Eingaben mit `PROPERTY_TEST()`.
- **Umfangreiche Assertions:** Beinhaltet Prüfungen für Gleichheit, Strings, Fließkommazahlen, Exceptions, Prädikate und mehr.
- **Scoped Tracing:** Nutzen Sie `SCOPED_TRACE`, um Ihren Fehlermeldungen Kontext hinzuzufügen.
- **Automatisches `main`:** Das `AZTEST_MAIN()`-Makro generiert automatisch den Einstiegspunkt für den Test-Runner.
- **Kommandozeilen-Steuerung:** Filtern, Mischen, Wiederholen und Setzen von Timeouts für Ihre Tests.

## Detaillierte Dokumentation

Die folgenden Anleitungen bieten eine detaillierte Erklärung aller Kernfunktionen:

- **[1. Grundlagen](./docs/01-Grundlagen.md):** Wie man Tests schreibt und ausführt.
- **[2. Assertions](./docs/02-Assertions.md):** Eine vollständige Liste aller Prüf-Makros.
- **[3. Fixtures](./docs/03-Fixtures.md):** Wie man Setup- und Teardown-Code teilt.
- **[4. Fortgeschrittene Themen](./docs/04-Fortgeschrittene-Themen.md):** Parametrisierte Tests und Scoped Tracing.

## Schnellstart: CMake-Integration

Dies ist die empfohlene Methode, um AZTest in Ihrem Projekt zu verwenden.

### Schritt 1: Tests schreiben

Erstellen Sie eine Testdatei (z. B. `tests/MeineTests.cpp`). Inkludieren Sie `AZTest/AZTest.h` und fügen Sie Ihre Tests hinzu. Verwenden Sie das `AZTEST_MAIN()`-Makro einmal in einer Ihrer Testdateien, um die `main`-Funktion zu erstellen.

**`tests/MeineTests.cpp`**
```cpp
#include <AZTest/AZTest.h>
#include <string>
#include <vector>

// Ein einfacher Testfall
TEST(Math, Addition) {
    EXPECT_EQ(2 + 2, 4);
    EXPECT_NE(2 + 2, 5);
}

// Eine andere Test-Suite
TEST(Container, Vektor) {
    std::vector<int> v;
    v.push_back(1);
    EXPECT_FALSE(v.empty());
    EXPECT_EQ(v.size(), 1);
}

// Dieses Makro erstellt die main()-Funktion für Sie
AZTEST_MAIN();
```

### Schritt 2: CMake konfigurieren

Verwenden Sie in Ihrer `CMakeLists.txt` `FetchContent`, um AZTest herunterzuladen und mit Ihrer Test-Anwendung zu verlinken.

**`CMakeLists.txt`**
```cmake
cmake_minimum_required(VERSION 3.15)
project(MeinProjekt)

set(CMAKE_CXX_STANDARD 17)

# --- AZTest-Integration ---
include(FetchContent)
FetchContent_Declare(
  AZTest
  GIT_REPOSITORY https://github.com/ihr-github-benutzer/AZTest-Cpp.git # <-- TODO: Diese URL ändern
  GIT_TAG        main # Oder ein spezifischer Release-Tag
)
FetchContent_MakeAvailable(AZTest)
# --- Ende der AZTest-Integration ---

enable_testing()
add_executable(MeinProjektTests tests/MeineTests.cpp)
target_link_libraries(MeinProjektTests PRIVATE AZTest::AZTest)
add_test(NAME MeinProjektTests COMMAND MeinProjektTests)
```

### Schritt 3: Bauen und Ausführen

```bash
cmake -B build
cmake --build build
cd build && ctest
```

## Assertions (Zusicherungen)

Verwenden Sie `EXPECT_*`-Makros, um nicht-fatale Fehler zu melden. Verwenden Sie `ASSERT_*`, um fatale Fehler zu melden, die den aktuellen Test abbrechen.

### Basis-Assertions
| Makro | Prüfung |
|---|---|
| `EXPECT_TRUE(cond)` / `ASSERT_TRUE(cond)` | `cond` ist wahr |
| `EXPECT_FALSE(cond)` / `ASSERT_FALSE(cond)`| `cond` ist falsch |

### Vergleichs-Assertions
| Makro | Prüfung |
|---|---|
| `EXPECT_EQ(a, b)` / `ASSERT_EQ(a, b)` | `a == b` |
| `EXPECT_NE(a, b)` / `ASSERT_NE(a, b)` | `a != b` |
| `EXPECT_LT(a, b)` / `ASSERT_LT(a, b)` | `a < b` |
| `EXPECT_LE(a, b)` / `ASSERT_LE(a, b)` | `a <= b` |
| `EXPECT_GT(a, b)` / `ASSERT_GT(a, b)` | `a > b` |
| `EXPECT_GE(a, b)` / `ASSERT_GE(a, b)` | `a >= b` |

### String-Assertions
| Makro | Prüfung |
|---|---|
| `EXPECT_STREQ(s1, s2)` / `ASSERT_STREQ(s1, s2)` | Die C-Strings `s1` und `s2` sind gleich |
| `EXPECT_STRNE(s1, s2)` / `ASSERT_STRNE(s1, s2)` | Die C-Strings `s1` und `s2` sind nicht gleich |
| `EXPECT_STRCASEEQ(s1, s2)` / `ASSERT_STRCASEEQ(s1, s2)` | Die C-Strings `s1` und `s2` sind ohne Berücksichtigung der Groß-/Kleinschreibung gleich |
| `EXPECT_STRCASENE(s1, s2)` / `ASSERT_STRCASENE(s1, s2)` | Die C-Strings `s1` und `s2` sind ohne Berücksichtigung der Groß-/Kleinschreibung ungleich |

### Fließkomma-Assertions
| Makro | Prüfung |
|---|---|
| `EXPECT_NEAR(v1, v2, err)` / `ASSERT_NEAR(v1, v2, err)` | `abs(v1 - v2) <= err` |

### Exception-Assertions
| Makro | Prüfung |
|---|---|
| `EXPECT_THROW(stmt, ex)` / `ASSERT_THROW(stmt, ex)` | `statement` wirft eine Exception vom Typ `exception_type` |
| `EXPECT_ANY_THROW(stmt)` / `ASSERT_ANY_THROW(stmt)` | `statement` wirft irgendeine Exception |
| `EXPECT_NO_THROW(stmt)` / `ASSERT_NO_THROW(stmt)` | `statement` wirft keine Exception |

### Prädikat-Assertions und Manuelle Fehler
| Makro | Prüfung |
|---|---|
| `EXPECT_PRED1(pred, v1)` / `ASSERT_PRED1(pred, v1)` | `pred(v1)` gibt `true` zurück |
| `EXPECT_PRED2(pred, v1, v2)` / `ASSERT_PRED2(pred, v1, v2)` | `pred(v1, v2)` gibt `true` zurück |
| `ADD_FAILURE()` | Markiert den Test als fehlgeschlagen, ohne ihn abzubrechen. |

## Fortgeschrittene Features

### Scoped Tracing

Für detailliertere Fehlerprotokolle, besonders in Schleifen, verwenden Sie `SCOPED_TRACE`. Alle Fehler innerhalb seines Gültigkeitsbereichs enthalten die Trace-Nachricht.

```cpp
TEST(MeineTests, SchleifenTest) {
    int werte[] = {1, 2, 3, 5, 8};
    for (int wert : werte) {
        SCOPED_TRACE("Teste mit Wert: " + std::to_string(wert));
        EXPECT_TRUE(wert < 6); // Schlägt fehl, wenn wert 8 ist
    }
}
```

### Test-Fixtures

Für Tests, die gemeinsamen Code für Setup und Teardown teilen, verwenden Sie eine Test-Fixture.

```cpp
class MeineDatenbankFixture : public AZTest::TestFixture {
protected:
    void SetUp() override {
        // Mit einer Test-Datenbank verbinden
    }
    void TearDown() override {
        // Verbindung trennen und aufräumen
    }
};

TEST_F(MeineDatenbankFixture, BenutzerPruefen) {
    // Testlogik, die die Fixture verwendet
    ASSERT_TRUE(true);
}
```

### Wert-parametrisierte Tests

Um dieselbe Testlogik mit unterschiedlichen Eingangswerten auszuführen, verwenden Sie `TEST_P`.

```cpp
// 1. Definieren Sie eine Test-Suite-Klasse (kann leer sein)
class IstPrimzahlTest {};

// 2. Definieren Sie die Testlogik mit `TEST_P`
// Die spezielle Variable `param` enthält den aktuellen Wert
TEST_P(IstPrimzahlTest, BehandeltPrimzahlen, int) {
    bool ist_prim = /* Ihre Logik zur Primzahlprüfung */;
    EXPECT_TRUE(ist_prim) << "Zahl " << param << " ist keine Primzahl.";
}

// 3. Instanziieren Sie die Test-Suite mit einer Liste von Werten
INSTANTIATE_TEST_SUITE_P(
    Primzahlen,        // Ein beliebiger Präfix für die Instanz
    IstPrimzahlTest,     // Die Test-Suite-Klasse
    BehandeltPrimzahlen, // Der Name des Tests
    2, 3, 5, 7, 11, 13   // Die zu testenden Werte
);
```

### Typ-parametrisierte Tests (TYPED_TEST)

Führen Sie die gleiche Testlogik für verschiedene Datentypen aus.

```cpp
TYPED_TEST_SUITE(MatheTest, int, float, double);

TYPED_TEST(MatheTest, Addition) {
    TypeParam a = 1;
    TypeParam b = 2;
    EXPECT_EQ(a + b, 3);
}
```

### Matcher System 

Verwenden Sie `EXPECT_THAT(wert, Matcher)` für lesbarere und komponierbare Assertions.

```cpp
using namespace AZTest::Matchers;

TEST(StringTest, MatcherBeispiel) {
    std::string text = "Hallo Welt";
    EXPECT_THAT(text, StartsWith("Hallo"));
    EXPECT_THAT(text, EndsWith("Welt"));
    // Logische Operatoren
    EXPECT_THAT(text, AllOf(HasSubstr("lo"), Not(StartsWith("Tschüss"))));
}
```

### Mocking System

Erstellen Sie einfach Mocks von Schnittstellen, um Abhängigkeiten in Tests zu isolieren. AZTest hat ein eingebautes Mocking-System, das kein externes gMock erfordert!

```cpp
class IDataStore {
public:
    virtual ~IDataStore() = default;
    virtual bool Save(int id) = 0;
};

class MockDataStore : public IDataStore {
public:
    MOCK_METHOD1(bool, Save, int);
};

TEST(ServiceTest, NutztDataStore) {
    MockDataStore mock;
    
    // Verhalten definieren
    EXPECT_CALL(mock, Save).WillOnce([](int id) { return id > 0; });
    
    EXPECT_TRUE(mock.Save(1));
}
```

### Property-Based Testing (Fuzzing)

Nutzen Sie `PROPERTY_TEST` für automatisches Fuzzing / randomisierte Eingaben. Das Framework probiert automatisch X verschiedene zufällige Eingaben, um Ihren Code an den Randbedingungen zu testen.

```cpp
// Führt den Test 100 Mal mit zufälligen Werten (a und b) aus
PROPERTY_TEST(Mathe, AdditionIstKommutativ, 100, int a, int b) {
    EXPECT_EQ(a + b, b + a);
}
```

## Lizenz

AZTest wird unter der MIT-Lizenz veröffentlicht.
# AZTest-Cpp
