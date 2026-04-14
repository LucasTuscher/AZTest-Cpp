# 3. Test-Fixtures

Test-Fixtures sind eine Möglichkeit, gemeinsamen Code für die Vor- und Nachbereitung von Tests zu teilen. Wenn Sie mehrere Tests haben, die auf denselben oder ähnlichen Objekten arbeiten, können Sie eine Fixture verwenden, um Codeduplizierung zu vermeiden.

## Eine Fixture erstellen

Eine Fixture ist eine Klasse, die von `AZTest::Core::TestFixture` erbt. In dieser Klasse können Sie die folgenden virtuellen Methoden überschreiben:

- **`SetUp()`:** Diese Methode wird **vor jedem Test** aufgerufen, der die Fixture verwendet. Hier initialisieren Sie typischerweise Ihre Objekte.
- **`TearDown()`:** Diese Methode wird **nach jedem Test** aufgerufen, der die Fixture verwendet. Hier räumen Sie Ressourcen auf.
- **`SetUpTestSuite()` (statisch):** Wird einmal **vor allen Tests** in der Suite aufgerufen. Nützlich für aufwändige Setups, die nur einmal pro Suite benötigt werden.
- **`TearDownTestSuite()` (statisch):** Wird einmal **nach allen Tests** in der Suite aufgerufen.

**Wichtig:** Die Suite-Lifecycle-Methoden (`SetUpTestSuite`/`TearDownTestSuite`) werden auch bei `--shuffle` korrekt aufgerufen - pro Suite genau einmal, unabhängig von der Test-Reihenfolge.

Objekte, die Sie in Ihren Tests verwenden möchten, können als Member-Variablen der Fixture-Klasse deklariert werden.

### Beispiel

```cpp
#include <AZTest/AZTest.h>

class Datenbank {
public:
    void verbinden() { /* ... */ }
    void trennen() { /* ... */ }
    bool istVerbunden() const { return true; }
    void fuegeNutzerHinzu(const std::string& name) { /* ... */ }
    int anzahlNutzer() const { return 1; }
};

class DatenbankFixture : public AZTest::Core::TestFixture {
protected:
    // Diese Methode wird vor jedem TEST_F aufgerufen
    void SetUp() override {
        db.verbinden();
    }

    // Diese Methode wird nach jedem TEST_F aufgerufen
    void TearDown() override {
        db.trennen();
    }

    // Statische Suite-Setup (wird einmal pro Suite aufgerufen)
    static void SetUpTestSuite() {
        // z.B. Datenbank-Verbindungspool initialisieren
    }

    static void TearDownTestSuite() {
        // z.B. Pool aufräumen
    }

    // `db` ist für alle Tests, die diese Fixture verwenden, verfügbar
    Datenbank db;
};
```

## Eine Fixture verwenden

Um eine Fixture in einem Test zu verwenden, nutzen Sie das `TEST_F`-Makro anstelle des normalen `TEST`-Makros.

- **`TEST_F(FixtureName, TestName)`**

Innerhalb eines `TEST_F`-Blocks können Sie direkt auf die Member-Variablen der Fixture-Klasse zugreifen.

### Beispiel

```cpp
// Dieser Test verwendet die DatenbankFixture.
// AZTest stellt sicher, dass SetUp() vor diesem Test
// und TearDown() nach diesem Test aufgerufen wird.
TEST_F(DatenbankFixture, Verbindung) {
    // Wir können direkt auf 'db' aus der Fixture zugreifen.
    EXPECT_TRUE(db.istVerbunden());
}

TEST_F(DatenbankFixture, FuegeNutzerHinzu) {
    ASSERT_TRUE(db.istVerbunden());
    db.fuegeNutzerHinzu("Testnutzer");
    EXPECT_EQ(db.anzahlNutzer(), 1);
}
```

Durch die Verwendung der Fixture wird sichergestellt, dass jeder Test mit einer sauberen, frisch verbundenen Datenbankinstanz beginnt, ohne dass der Verbindungs- und Trennungscode in jedem Test wiederholt werden muss.

## Gemischte Tests in einer Suite

Eine Suite kann sowohl normale `TEST`-Tests als auch Fixture-Tests (`TEST_F`) enthalten. Die Suite-Lifecycle-Methoden werden nur für Fixture-Tests aufgerufen, aber korrekt vor dem ersten Fixture-Test und nach dem letzten Fixture-Test der Suite.

```cpp
// Normaler Test in der Suite
TEST(DatenbankSuite, EinfacherTest) {
    EXPECT_EQ(1, 1);
}

// Fixture-Tests mit Setup/Teardown
TEST_F(DatenbankFixture, MitDatenbank) {
    EXPECT_TRUE(db.istVerbunden());
}
```
