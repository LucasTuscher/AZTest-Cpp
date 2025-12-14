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

Wenn Sie diesen Test ausführen, meldet CTest 6 einzelne Tests, einen für jede Zahl. Wenn einer fehlschlägt (z.B. wenn Sie `7` zur Liste hinzufügen), wird genau angezeigt, welcher Testfall (mit welchem Wert) fehlgeschlagen ist.

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
