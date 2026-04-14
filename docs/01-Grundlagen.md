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

- **`EXPECT_*` (Erwarten):** Meldet einen Fehler, aber der Test wird **weiter ausgeführt**. Mehrere `EXPECT_*` Fehler pro Test werden **alle gesammelt** und am Ende ausgegeben (nicht nur der letzte!).
- **`ASSERT_*` (Zusichern):** Meldet einen Fehler und **bricht den aktuellen Test sofort ab**. Dies ist nützlich, wenn eine nachfolgende Prüfung von einer vorherigen abhängt.

### Beispiel

```cpp
TEST(Assertions, MehrereFehler) {
    // Alle EXPECT_* Fehler werden gesammelt und angezeigt
    EXPECT_EQ(2 + 2, 5);  // Fehlschlag #1
    EXPECT_EQ(3 * 3, 10); // Fehlschlag #2
    EXPECT_TRUE(false);   // Fehlschlag #3
    // Am Ende sieht man alle 3 Fehler, nicht nur den letzten!
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

Deaktivierte Tests werden vom Test-Runner übersprungen (als "SKIPPED" markiert), bleiben aber im Code erhalten.

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

Mit `--run_disabled` können deaktivierte Tests trotzdem ausgeführt werden:
```bash
./my_tests --run_disabled
```

## Tests Überspringen

Im Gegensatz zum Deaktivieren, das statisch ist, können Sie einen Test zur Laufzeit überspringen. Das ist nützlich, wenn eine Testbedingung zur Laufzeit nicht erfüllt ist.

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

## Kommandozeilen-Optionen (CLI)

AZTest bietet ein professionelles CLI-System:

```bash
./my_tests --help                    # Hilfe anzeigen
./my_tests --version                 # Version anzeigen
./my_tests --list                    # Alle Tests auflisten (gruppiert nach Suite)
./my_tests --list_suites             # Nur Suite-Namen auflisten
./my_tests --filter=Mathe.*          # Nur Tests aus der Mathe-Suite
./my_tests --shuffle                 # Tests zufällig mischen
./my_tests --seed=12345              # Seed für Shuffle setzen
./my_tests --repeat=10               # Alle Tests 10-mal wiederholen
./my_tests --fail_fast               # Bei erstem Fehler stoppen
./my_tests --run_disabled            # DISABLED_ Tests ausführen
./my_tests --slow=100                # Warnung bei Tests > 100ms
./my_tests --timeout=5000            # Timeout nach 5000ms
./my_tests --json=results.json       # JSON-Report
./my_tests --xml=results.xml         # JUnit XML-Report
./my_tests --color=auto|always|never # Farbsteuerung
```

Unbekannte Argumente führen zu Exit-Code 2 mit einer Fehlermeldung.

## Exit-Codes

- `0` - Alle Tests erfolgreich
- `1` - Mindestens ein Test fehlgeschlagen
- `2` - CLI-Fehler (unbekanntes Argument, fehlender Wert, etc.)

## Umgebungsvariablen

| Variable | Effekt |
|----------|--------|
| `NO_COLOR` | Keine Farben (wie `--color=never`) |
| `CLICOLOR_FORCE` | Immer Farben (wie `--color=always`) |
| `CLICOLOR=0` | Keine Farben |
| `CI`, `GITHUB_ACTIONS`, `GITLAB_CI`, etc. | Automatisch Farben in CI aktivieren |

## Mehrfach-Failures

Ein wichtiges Feature von AZTest ist, dass **alle** `EXPECT_*` Fehler pro Test gesammelt werden. Im Gegensatz zu manchen anderen Frameworks, die nur den letzten Fehler anzeigen, sehen Sie bei AZTest alle fehlgeschlagenen Erwartungen mit Datei, Zeile und Nachricht.

Beispiel-Output bei mehreren Fehlern:
```
[FAIL] MeinTest.MehrereFehler
      test.cpp:10: Expected: 2 + 2 == 5
            Actual: 4 != 5
      test.cpp:11: Expected: 3 * 3 == 10
            Actual: 9 != 10
      test.cpp:12: Expected TRUE but got FALSE: false
```
