# 4. Fortgeschrittene Themen

Dieses Dokument behandelt fortgeschrittene Funktionen von AZTest, die Ihnen helfen, komplexere Testszenarien zu bewältigen und das Debugging zu vereinfachen.

## Wert-parametrisierte Tests

Manchmal möchten Sie dieselbe Testlogik mit einer Reihe von unterschiedlichen Werten ausführen. Anstatt für jeden Wert einen separaten Test zu schreiben, können Sie einen parametrisierten Test erstellen.

Der Prozess hat drei Schritte:

**1. Definieren Sie eine Test-Suite-Klasse:** Dies ist eine leere Klasse, die als Gruppierung für die parametrisierten Tests dient.

```cpp
class IstGeradeZahlTest {};
```

**2. Definieren Sie die Testlogik mit `TEST_P`:**
Das `TEST_P`-Makro ähnelt `TEST`, hat aber einen zusätzlichen dritten Parameter für den Datentyp des Parameters. Innerhalb des Testkörpers können Sie auf die spezielle Variable `param` zugreifen, die den aktuellen Wert aus der Werteliste enthält.

```cpp
// Der dritte Parameter 'int' gibt an, dass die Testwerte vom Typ 'int' sind.
TEST_P(IstGeradeZahlTest, PrueftGeradeZahlen, int) {
    // 'param' enthält den aktuellen Wert für diese Testausführung.
    EXPECT_EQ(param % 2, 0);
}
```

**3. Instanziieren Sie die Test-Suite:**
Verwenden Sie das Makro `INSTANTIATE_TEST_SUITE_P`, um den Test mit einer Liste von Werten zu versehen. AZTest wird dann automatisch für jeden Wert eine eigene Testausführung generieren.

```cpp
INSTANTIATE_TEST_SUITE_P(
    GeradeZahlen,           // Ein eindeutiger Name (Präfix) für diese Instanziierung.
    IstGeradeZahlTest,      // Die Klasse der Test-Suite.
    PrueftGeradeZahlen,     // Der Name des Tests, den Sie instanziieren.
    0, 2, 4, 6, 8, 100      // Die Liste der Werte, die als `param` übergeben werden.
);
```

Wenn Sie diesen Test ausführen, meldet AZTest 6 einzelne Tests, einen für jede Zahl. Wenn einer fehlschlägt (z.B. wenn Sie `7` zur Liste hinzufügen), wird genau angezeigt, welcher Testfall (mit welchem Wert) fehlgeschlagen ist.

## Scoped Tracing

Wenn ein Test fehlschlägt, ist es manchmal schwierig zu wissen, in welchem Kontext der Fehler aufgetreten ist, besonders in Schleifen oder komplexen Funktionen. `SCOPED_TRACE` hilft dabei, indem es zusätzliche Informationen zu den Fehlermeldungen hinzufügt, die nur für den aktuellen Gültigkeitsbereich (Scope) gelten.

### Beispiel

```cpp
TEST(TraceTest, Schleife) {
    int werte[] = {1, 2, 5, 9, 13};
    for (int i = 0; i < 5; ++i) {
        int wert = werte[i];
        
        // Diese Nachricht wird zu jeder Fehlermeldung hinzugefügt,
        // die innerhalb dieser Schleifeniteration auftritt.
        SCOPED_TRACE("Iteration " + std::to_string(i) + ", Wert: " + std::to_string(wert));

        EXPECT_TRUE(wert < 10); // Dieser Test wird für 13 fehlschlagen.
    }
}
```

Wenn dieser Test läuft, wird die Ausgabe für den fehlschlagenden Test etwa so aussehen:

```
/pfad/zu/ihrem/test.cpp:12: Failure
Trace:
  /pfad/zu/ihrem/test.cpp:11: Iteration 4, Wert: 13
Expected TRUE but got FALSE: wert < 10
```

Ohne `SCOPED_TRACE` wüssten Sie nur, *dass* die Prüfung `wert < 10` fehlgeschlagen ist, aber nicht, bei welcher Iteration oder mit welchem Wert.

## Reporter (Ausgabe-Formate)

AZTest unterstützt mehrere Ausgabeformate für verschiedene Anwendungsfälle:

### Console Reporter (Standard)
Farbige Konsolenausgabe mit pass/fail/skip Status:
```cpp
// Wird automatisch mit AZTEST_MAIN() verwendet
// Farben werden automatisch basierend auf TTY/CI aktiviert
```

### JSON Reporter
Für Programmierung oder externe Tools:
```cpp
AZTest::Core::TestRegistry::Instance().AddReporter(
    std::make_shared<AZTest::Reporters::JSONReporter>("results.json")
);
```

Oder über CLI:
```bash
./my_tests --json=results.json
```

### JUnit XML Reporter
Für CI/CD-Integration (Jenkins, GitLab, GitHub Actions):
```cpp
AZTest::Core::TestRegistry::Instance().AddReporter(
    std::make_shared<AZTest::Reporters::XMLReporter>("results.xml")
);
```

Oder über CLI:
```bash
./my_tests --xml=results.xml
```

Die XML-Ausgabe folgt dem JUnit-Standard und enthält:
- Test-Suites mit Zeitmessung
- Einzelne Testfälle mit Status (passed/failed/skipped)
- Fehlermeldungen mit Datei/Zeile
- Gesamtlaufzeit

## Mehrere Reporter gleichzeitig

Sie können mehrere Reporter gleichzeitig aktivieren:

```cpp
TEST_MAIN() {
    AZTest::InitializeDefaultReporter();  // Console
    AZTest::Core::TestRegistry::Instance().AddReporter(
        std::make_shared<AZTest::Reporters::JSONReporter>("results.json")
    );
    AZTest::Core::TestRegistry::Instance().AddReporter(
        std::make_shared<AZTest::Reporters::XMLReporter>("results.xml")
    );
    return AZTest::RunWithArgs(argc, argv);
}
```

## CLI-Filter

Mit `--filter` können Sie Tests mit Wildcard-Patterns auswählen:

```bash
./my_tests --filter=Mathe.*           # Alle Tests in Mathe-Suite
./my_tests --filter=*.Addition        # Alle "Addition" Tests
./my_tests --filter=*Vector*          # Alle Tests mit "Vector" im Namen
./my_tests --filter=Math.*:Physics.*  # Mehrere Patterns (nicht unterstützt, aber mehrere Filter-Aufrufe)
```

**Hinweis:** Das Filter-Pattern ist ein einfacher Glob mit `*` (beliebig viele Zeichen) und `?` (ein Zeichen).

## Shuffle und deterministische Tests

Mit `--shuffle` werden Tests in zufälliger Reihenfolge ausgeführt:

```bash
./my_tests --shuffle              # Zufällige Reihenfolge
./my_tests --shuffle --seed=12345 # Reproduzierbare zufällige Reihenfolge
```

**Wichtig:**
- Shuffle erfolgt pro Suite (Tests innerhalb einer Suite werden gemischt)
- Die Reihenfolge der Suites wird auch gemischt
- Suite-Lifecycle (`SetUpTestSuite`/`TearDownTestSuite`) funktioniert trotz Shuffle korrekt

## Test-Timeout

Für Tests, die möglicherweise hängen bleiben:

```bash
./my_tests --timeout=5000   # 5 Sekunden Timeout pro Test
```

Ein Test, der das Timeout überschreitet, wird als FAILED markiert mit der Meldung "Test timed out".

## Langsame Tests markieren

Mit `--slow` werden Tests, die länger als die angegebene Zeit brauchen, als "slow" markiert:

```bash
./my_tests --slow=100       # Warnung bei Tests > 100ms
```

Im Output werden diese Tests mit `[SLOW]` markiert.

## CI-Integration

### GitHub Actions
```yaml
- name: Run Tests
  run: ./my_tests --xml=test-results.xml --color=always
  
- name: Publish Test Results
  uses: mikepenz/action-junit-report@v3
  if: success() || failure()
  with:
    report_paths: '**/test-results.xml'
```

### GitLab CI
```yaml
test:
  script:
    - ./my_tests --xml=junit-report.xml
  artifacts:
    reports:
      junit: junit-report.xml
```

### Jenkins
Jenkins erkennt JUnit-XML-Dateien automatisch im Post-Build-Schritt.

## Benutzerdefinierte Reporter

Sie können eigene Reporter implementieren:

```cpp
#include <AZTest/Reporters/IReporter.h>

class MyReporter : public AZTest::Core::IReporter {
public:
    void OnTestRunStart(int totalTests) override { }
    void OnTestSuiteStart(const std::string& suiteName) override { }
    void OnTestStart(const std::string& testName) override { }
    void OnTestEnd(const AZTest::Core::TestResult& result) override { 
        // Eigenes Format
    }
    void OnTestSuiteEnd(const std::string& suiteName) override { }
    void OnTestRunEnd(const std::vector<AZTest::Core::TestResult>& results, 
                      double totalTimeMs) override { }
};
```
