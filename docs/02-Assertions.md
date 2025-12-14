# 2. Assertions im Detail

AZTest bietet eine Vielzahl von Assertions, um Ihre Testbedingungen zu überprüfen. Jede Assertion existiert in einer `EXPECT_*`-Variante (meldet Fehler und fährt fort) und einer `ASSERT_*`-Variante (meldet Fehler und bricht den Test ab).

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

Diese vergleichen zwei Werte miteinander. Sie funktionieren für alle Standarddatentypen und Objekte, die die entsprechenden Vergleichsoperatoren (`==`, `!=`, `<`, etc.) überladen haben.

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

Diese sind speziell für den Vergleich von C-Strings (`const char*`) optimiert.

| Makro | Prüfung |
|---|---|
| `EXPECT_STREQ(s1, s2)` / `ASSERT_STREQ(s1, s2)` | Die C-Strings `s1` und `s2` sind identisch. |
| `EXPECT_STRNE(s1, s2)` / `ASSERT_STRNE(s1, s2)` | Die C-Strings `s1` und `s2` sind nicht identisch. |
| `EXPECT_STRCASEEQ(s1, s2)` / `ASSERT_STRCASEEQ(s1, s2)` | Die C-Strings `s1` und `s2` sind identisch (ohne Berücksichtigung der Groß-/Kleinschreibung). |
| `EXPECT_STRCASENE(s1, s2)` / `ASSERT_STRCASENE(s1, s2)` | Die C-Strings `s1` und `s2` sind ungleich (ohne Berücksichtigung der Groß-/Kleinschreibung). |

**Beispiel:**
```cpp
const char* str1 = "Hallo";
const char* str2 = "hALLo";
EXPECT_STREQ(str1, "Hallo");
EXPECT_STRCASEEQ(str1, str2);
```

## Fließkomma-Assertions

Der direkte Vergleich von Fließkommazahlen mit `==` ist oft problematisch aufgrund von Rundungsfehlern. Verwenden Sie stattdessen `NEAR`, um eine Toleranz anzugeben.

| Makro | Prüfung |
|---|---|
| `EXPECT_NEAR(v1, v2, err)` / `ASSERT_NEAR(v1, v2, err)` | Der absolute Unterschied zwischen `v1` und `v2` ist kleiner oder gleich `err`. |

**Beispiel:**
```cpp
double result = 0.1 + 0.2;
EXPECT_NEAR(result, 0.3, 0.000001);
```

## Exception-Assertions

Diese prüfen, ob ein Code-Block eine Exception auslöst oder nicht.

| Makro | Prüfung |
|---|---|
| `EXPECT_THROW(stmt, ex)` / `ASSERT_THROW(stmt, ex)` | `statement` löst eine Exception vom Typ `exception_type` aus. |
| `EXPECT_ANY_THROW(stmt)` / `ASSERT_ANY_THROW(stmt)` | `statement` löst irgendeine Exception aus. |
| `EXPECT_NO_THROW(stmt)` / `ASSERT_NO_THROW(stmt)` | `statement` löst keine Exception aus. |

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

## Prädikat-Assertions

Für komplexere Überprüfungen können Sie eine beliebige Funktion (ein "Prädikat") verwenden, um eine Bedingung zu testen.

| Makro | Prüfung |
|---|---|
| `EXPECT_PRED1(pred, v1)` / `ASSERT_PRED1(pred, v1)` | Das Prädikat `pred(v1)` gibt `true` zurück. |
| `EXPECT_PRED2(pred, v1, v2)` / `ASSERT_PRED2(pred, v1, v2)` | Das Prädikat `pred(v1, v2)` gibt `true` zurück. |

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

In manchen Fällen möchten Sie vielleicht einen Fehlschlag manuell aufzeichnen, z.B. in einer komplexen if/else-Struktur. Hierfür können Sie `ADD_FAILURE()` verwenden. Dies zeichnet einen nicht-fatalen Fehler auf.

`ADD_FAILURE()` ist "streamable", was bedeutet, dass Sie mit `<<` eine benutzerdefinierte Nachricht anhängen können.

**Beispiel:**
```cpp
TEST(ManualFailure, Pruefung) {
    int status = GetStatus(); // Angenommen, diese Funktion gibt einen Statuscode zurück
    if (status != 0) {
        ADD_FAILURE() << "GetStatus() ist fehlgeschlagen mit Code: " << status;
    }
}
```
