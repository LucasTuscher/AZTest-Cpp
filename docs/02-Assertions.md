# 2. Assertions im Detail

AZTest bietet eine Vielzahl von Assertions, um Ihre Testbedingungen zu überprüfen. Jede Assertion existiert in einer `EXPECT_*`-Variante (meldet Fehler und fährt fort) und einer `ASSERT_*`-Variante (meldet Fehler und bricht den Test ab).

**Wichtig:** Mehrere `EXPECT_*` Fehler in einem Test werden **alle gesammelt** und angezeigt, nicht nur der letzte!

## Wahrheits-Assertions

Diese prüfen einfache boolesche Bedingungen.

| Makro | Prüfung |
|---|---|
| `EXPECT_TRUE(cond)` / `ASSERT_TRUE(cond)` | `cond` ist `true` |
| `EXPECT_FALSE(cond)` / `ASSERT_FALSE(cond)`| `cond` ist `false` |

**Beispiel:**
```cpp
bool is_ready = true;
EXPECT_TRUE(is_ready);
```

## Vergleichs-Assertions

Diese vergleichen zwei Werte miteinander. Sie funktionieren für alle Standarddatentypen und Objekte, die die entsprechenden Vergleichsoperatoren überladen haben.

| Makro | Prüfung |
|---|---|
| `EXPECT_EQ(a, b)` / `ASSERT_EQ(a, b)` | `a == b` |
| `EXPECT_NE(a, b)` / `ASSERT_NE(a, b)` | `a != b` |
| `EXPECT_LT(a, b)` / `ASSERT_LT(a, b)` | `a < b` |
| `EXPECT_LE(a, b)` / `ASSERT_LE(a, b)` | `a <= b` |
| `EXPECT_GT(a, b)` / `ASSERT_GT(a, b)` | `a > b` |
| `EXPECT_GE(a, b)` / `ASSERT_GE(a, b)` | `a >= b` |

**Beispiel:**
```cpp
int a = 5;
EXPECT_EQ(a, 5);
EXPECT_GT(a, 3);
```

## String-Assertions

Diese sind speziell für String-Vergleiche optimiert.

| Makro | Prüfung |
|---|---|
| `EXPECT_STREQ(s1, s2)` / `ASSERT_STREQ(s1, s2)` | C-Strings sind identisch |
| `EXPECT_STRNE(s1, s2)` / `ASSERT_STRNE(s1, s2)` | C-Strings sind unterschiedlich |
| `EXPECT_STRCASEEQ(s1, s2)` / `ASSERT_STRCASEEQ(s1, s2)` | Case-insensitive Gleichheit |
| `EXPECT_STRCASENE(s1, s2)` / `ASSERT_STRCASENE(s1, s2)` | Case-insensitive Ungleichheit |
| `EXPECT_STARTS_WITH(str, prefix)` / `ASSERT_STARTS_WITH` | String beginnt mit Prefix |
| `EXPECT_ENDS_WITH(str, suffix)` / `ASSERT_ENDS_WITH` | String endet mit Suffix |
| `EXPECT_CONTAINS(haystack, needle)` / `ASSERT_CONTAINS` | Enthält Substring |

**Beispiel:**
```cpp
const char* str1 = "Hallo";
const char* str2 = "hALLo";
EXPECT_STREQ(str1, "Hallo");
EXPECT_STRCASEEQ(str1, str2);

std::string text = "Hello World";
EXPECT_STARTS_WITH(text, "Hello");
EXPECT_ENDS_WITH(text, "World");
EXPECT_CONTAINS(text, "lo Wo");
```

## Fließkomma-Assertions

Der direkte Vergleich von Fließkommazahlen mit `==` ist oft problematisch. AZTest bietet mehrere spezialisierte Assertions.

| Makro | Prüfung |
|---|---|
| `EXPECT_NEAR(v1, v2, err)` / `ASSERT_NEAR(v1, v2, err)` | Absoluter Unterschied ≤ `err` |
| `EXPECT_FLOAT_EQ(a, b)` / `ASSERT_FLOAT_EQ(a, b)` | ~4 ULPs Toleranz (float) |
| `EXPECT_DOUBLE_EQ(a, b)` / `ASSERT_DOUBLE_EQ(a, b)` | ~4 ULPs Toleranz (double) |
| `EXPECT_NAN(val)` / `ASSERT_NAN(val)` | Wert ist NaN |
| `EXPECT_INF(val)` / `ASSERT_INF(val)` | Wert ist +∞ |
| `EXPECT_NINF(val)` / `ASSERT_NINF(val)` | Wert ist -∞ |
| `EXPECT_FINITE(val)` / `ASSERT_FINITE(val)` | Wert ist endlich (nicht NaN/Inf) |

**Beispiel:**
```cpp
double result = 0.1 + 0.2;
EXPECT_NEAR(result, 0.3, 0.000001);
EXPECT_DOUBLE_EQ(result, 0.3);  // 4 ULPs Toleranz

float inf = 1.0f / 0.0f;
EXPECT_INF(inf);
EXPECT_FINITE(3.14159f);

float nan = std::sqrt(-1.0f);
EXPECT_NAN(nan);
```

## Container-Assertions

| Makro | Prüfung |
|---|---|
| `EXPECT_RANGE_EQ(a, b)` / `ASSERT_RANGE_EQ(a, b)` | Zwei Container sind elementweise gleich |
| `EXPECT_CONTAINER_EQ(a, b)` / `ASSERT_CONTAINER_EQ(a, b)` | Alias für RANGE_EQ |
| `EXPECT_CONTAINS(container, element)` / `ASSERT_CONTAINS` | Container enthält Element |

**Beispiel:**
```cpp
std::vector<int> v1 = {1, 2, 3};
std::vector<int> v2 = {1, 2, 3};
EXPECT_RANGE_EQ(v1, v2);
EXPECT_CONTAINS(v1, 2);
```

## Exception-Assertions

Diese prüfen, ob ein Code-Block eine Exception auslöst oder nicht.

| Makro | Prüfung |
|---|---|
| `EXPECT_THROW(stmt, ex)` / `ASSERT_THROW(stmt, ex)` | Statement löst Exception vom Typ `ex` aus |
| `EXPECT_ANY_THROW(stmt)` / `ASSERT_ANY_THROW(stmt)` | Statement löst irgendeine Exception aus |
| `EXPECT_NO_THROW(stmt)` / `ASSERT_NO_THROW(stmt)` | Statement löst keine Exception aus |

**Beispiel:**
```cpp
void WirftImmer() {
    throw std::runtime_error("Fehler!");
}

TEST(Exceptions, Pruefung) {
    EXPECT_THROW(WirftImmer(), std::runtime_error);
    EXPECT_NO_THROW(int a = 1;);
}
```

## Death Tests (Absturz-Tests)

Death Tests prüfen, ob Code abstürzt (SEH auf Windows, Signals auf POSIX).

| Makro | Prüfung |
|---|---|
| `EXPECT_DEATH(stmt, msg_pattern)` / `ASSERT_DEATH(stmt, msg_pattern)` | Statement stürzt ab mit passender Fehlermeldung |

**Beispiel:**
```cpp
// Prüfe, dass ein nullptr-Dereferenz abstürzt
TEST(DeathTests, NullptrDeref) {
    int* p = nullptr;
    EXPECT_DEATH(*p = 42, "*");  // "*" = beliebige Fehlermeldung
}

// Prüfe Division durch Null
TEST(DeathTests, DivByZero) {
    int a = 1, b = 0;
    EXPECT_DEATH(a / b, "*");
}
```

**Hinweis:** Auf Windows werden SEH-Exceptions (Access Violation, Divide by Zero, etc.) gefangen. Auf POSIX-Systemen werden Signale (SIGSEGV, SIGFPE, etc.) abgefangen.

## Prädikat-Assertions

Für komplexere Überprüfungen können Sie eine beliebige Funktion verwenden.

| Makro | Prüfung |
|---|---|
| `EXPECT_PRED1(pred, v1)` / `ASSERT_PRED1(pred, v1)` | `pred(v1)` gibt `true` zurück |
| `EXPECT_PRED2(pred, v1, v2)` / `ASSERT_PRED2(pred, v1, v2)` | `pred(v1, v2)` gibt `true` zurück |

**Beispiel:**
```cpp
bool IstUngerade(int n) {
    return n % 2 != 0;
}

TEST(Predicates, Pruefung) {
    EXPECT_PRED1(IstUngerade, 5);
    ASSERT_PRED1(IstUngerade, 7);
}
```

## Manueller Fehlschlag

In manchen Fällen möchten Sie einen Fehlschlag manuell aufzeichnen.

| Makro | Beschreibung |
|---|---|
| `ADD_FAILURE()` | Zeichnet einen nicht-fatalen Fehler auf (streamable mit `<<`) |

**Beispiel:**
```cpp
TEST(ManualFailure, Pruefung) {
    int status = GetStatus();
    if (status != 0) {
        ADD_FAILURE() << "GetStatus() fehlgeschlagen mit Code: " << status;
    }
}
```
