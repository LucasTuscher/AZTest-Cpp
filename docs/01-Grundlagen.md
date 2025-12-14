# 1. Grundlagen von AZTest

Dieses Dokument erklärt die grundlegenden Konzepte zum Schreiben von Tests mit dem AZTest-Framework.

## Einen einfachen Test definieren

Der einfachste Weg, einen Test zu erstellen, ist die Verwendung des `TEST`-Makros. Dieses Makro hat zwei Argumente:

1.  **Test-Suite-Name:** Eine Gruppierung für logisch zusammengehörige Tests.
2.  **Test-Name:** Der spezifische Name für diesen Testfall.

```cpp
#include <AZTest/AZTest.h>

TEST(Mathe, Addition) {
    int a = 2;
    int b = 3;
    EXPECT_EQ(a + b, 5);
}

TEST(Mathe, Subtraktion) {
    int a = 5;
    int b = 3;
    EXPECT_EQ(a - b, 2);
}
```
In diesem Beispiel ist `Mathe` der Name der Test-Suite und `Addition` bzw. `Subtraktion` sind die Namen der Tests.

## Test-Runner erstellen

Damit Ihre Tests ausgeführt werden können, benötigt die Anwendung eine `main`-Funktion. AZTest bietet hierfür das praktische `AZTEST_MAIN()`-Makro. Fügen Sie dieses Makro einfach in eine Ihrer `.cpp`-Dateien ein, um den Test-Runner automatisch zu generieren.

```cpp
#include <AZTest/AZTest.h>

TEST(MeinTest, TutEtwas) {
    EXPECT_TRUE(true);
}

// Erstellt die `main`-Funktion und startet alle registrierten Tests
AZTEST_MAIN();
```

## Basis-Assertions

Assertions sind die Bausteine jedes Tests. Sie prüfen, ob eine bestimmte Bedingung wahr ist. AZTest unterscheidet zwischen zwei Arten von Assertions:

- **`EXPECT_*` (Erwarten):** Meldet einen Fehler, aber der Test wird **weiter ausgeführt**. Dies ist nützlich, wenn Sie mehrere unabhängige Bedingungen in einem Test prüfen möchten.
- **`ASSERT_*` (Zusichern):** Meldet einen Fehler und **bricht den aktuellen Test sofort ab**. Dies ist nützlich, wenn eine nachfolgende Prüfung von einer vorherigen abhängt.

### Beispiel

```cpp
TEST(Assertions, Beispiel) {
    int* ptr = nullptr;

    // Diese Prüfung schlägt fehl, aber der Test geht weiter.
    EXPECT_EQ(ptr, nullptr);

    // Diese Prüfung würde abstürzen, wenn ptr null ist.
    // Ein ASSERT in der Zeile davor hätte den Test sicher abgebrochen.
    EXPECT_EQ(*ptr, 42); // Wird nicht sicher erreicht, wenn EXPECT fehlschlägt
}

TEST(Assertions, SicheresBeispiel) {
    int* ptr = nullptr;

    // Wenn dieser ASSERT fehlschlägt, wird der Test hier beendet.
    ASSERT_NE(ptr, nullptr);

    // Dieser Code wird nur ausgeführt, wenn der ASSERT oben erfolgreich war.
    EXPECT_EQ(*ptr, 42);
}
```

Eine vollständige Liste aller Assertions finden Sie im Dokument `02-Assertions.md`.

## Tests Deaktivieren

Manchmal ist ein Test vorübergehend defekt oder nicht relevant. Anstatt ihn auszukommentieren, können Sie ihn deaktivieren, indem Sie dem Namen der Test-Suite oder dem Test-Namen das Präfix `DISABLED_` voranstellen.

Deaktivierte Tests werden vom Test-Runner ignoriert und nicht ausgeführt, bleiben aber im Code erhalten.

```cpp
// Diese gesamte Suite wird nicht ausgeführt.
TEST(DISABLED_VeralteteSuite, AlterTest) {
    EXPECT_EQ(1, 1);
}

// Nur dieser eine Test wird nicht ausgeführt.
TEST(MeineSuite, DISABLED_DefekterTest) {
    EXPECT_EQ(1, 2);
}
```

## Tests Überspringen

Im Gegensatz zum Deaktivieren, das zur Kompilierzeit geschieht, können Sie einen Test zur Laufzeit überspringen. Das ist nützlich, wenn eine Testbedingung zur Laufzeit nicht erfüllt ist (z.B. eine fehlende Konfigurationsdatei oder eine nicht unterstützte Plattform).

Verwenden Sie dazu das `TEST_SKIP()`-Makro. Der Test wird sofort beendet und als "übersprungen" (skipped) markiert.

```cpp
#include <fstream>

TEST(Dateisystem, LeseDatei) {
    std::ifstream f("meine_datei.txt");
    if (!f.good()) {
        TEST_SKIP("Die Datei 'meine_datei.txt' konnte nicht gefunden werden.");
    }
    // ... restlicher Test-Code ...
}
```
Der Test-Runner wird diesen Test als `SKIPPED` melden, nicht als `PASSED` oder `FAILED`.
