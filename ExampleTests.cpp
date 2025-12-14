#include "AZTest.h"
#include <vector>
#include <string>
#include <cmath>

using namespace AZTest;

// ============================================================================
// BASIC TESTS - Demonstriert grundlegende Funktionalität
// ============================================================================

TEST(BasicTests, SimpleAddition) {
    int result = 2 + 2;
    EXPECT_EQ(result, 4);
    EXPECT_TRUE(result == 4);
}

TEST(BasicTests, StringComparison) {
    std::string hello = "Hello";
    std::string world = "World";

    EXPECT_NE(hello, world);
    EXPECT_EQ(hello, "Hello");
}

TEST(BasicTests, BooleanChecks) {
    bool isEngineRunning = true;
    bool isShuttingDown = false;

    EXPECT_TRUE(isEngineRunning);
    EXPECT_FALSE(isShuttingDown);
}

TEST(BasicTests, ComparisonOperators) {
    int health = 100;
    int maxHealth = 150;

    EXPECT_LT(health, maxHealth);
    EXPECT_LE(health, 100);
    EXPECT_GT(maxHealth, health);
    EXPECT_GE(maxHealth, 150);
}

// ============================================================================
// MATH TESTS - Simuliert Engine-Mathematik
// ============================================================================

TEST(MathTests, VectorMagnitude) {
    float x = 3.0f;
    float y = 4.0f;
    float magnitude = std::sqrt(x*x + y*y);

    EXPECT_EQ(magnitude, 5.0f);
}

TEST(MathTests, DotProduct) {
    // Simuliere 2D Vektor Dot Product
    float v1x = 1.0f, v1y = 0.0f;
    float v2x = 0.0f, v2y = 1.0f;

    float dotProduct = v1x * v2x + v1y * v2y;
    EXPECT_EQ(dotProduct, 0.0f); // Orthogonale Vektoren
}

TEST(MathTests, Lerp) {
    // Linear interpolation
    auto lerp = [](float a, float b, float t) {
        return a + (b - a) * t;
    };

    float result = lerp(0.0f, 100.0f, 0.5f);
    EXPECT_EQ(result, 50.0f);
}

// ============================================================================
// ASSERT TESTS - Zeigt ASSERT vs EXPECT
// ============================================================================

TEST(AssertTests, AssertStopsExecution) {
    int value = 42;

    ASSERT_EQ(value, 42); // Wenn das fehlschlägt, stoppt der Test hier

    // Dieser Code wird nur ausgeführt wenn ASSERT erfolgreich war
    EXPECT_TRUE(value > 0);
}

TEST(AssertTests, ExpectContinues) {
    int x = 5;

    EXPECT_EQ(x, 10); // Schlägt fehl, aber Test läuft weiter
    EXPECT_TRUE(x > 0); // Wird trotzdem ausgeführt
    EXPECT_LT(x, 20); // Wird auch ausgeführt
}

// ============================================================================
// PERFORMANCE TESTS - Benchmark-Funktionalität
// ============================================================================

PERFORMANCE_TEST(PerformanceTests, VectorAllocation, 100000) {
    std::vector<int> v;
    v.push_back(i);
}
END_PERFORMANCE_TEST

PERFORMANCE_TEST(PerformanceTests, StringConcatenation, 10000) {
    std::string result = "Test_" + std::to_string(i);
    (void)result; // Unterdrücke unused warning
}
END_PERFORMANCE_TEST

// ============================================================================
// ENGINE SIMULATION TESTS
// ============================================================================

// Simuliere eine einfache Engine-Komponente
class GameObject {
public:
    GameObject() : x(0), y(0), health(100) {}

    void Move(float dx, float dy) {
        x += dx;
        y += dy;
    }

    void TakeDamage(int damage) {
        health -= damage;
        if (health < 0) health = 0;
    }

    bool IsAlive() const { return health > 0; }

    float GetX() const { return x; }
    float GetY() const { return y; }
    int GetHealth() const { return health; }

private:
    float x, y;
    int health;
};

TEST(GameObjectTests, InitialState) {
    GameObject obj;

    EXPECT_EQ(obj.GetX(), 0.0f);
    EXPECT_EQ(obj.GetY(), 0.0f);
    EXPECT_EQ(obj.GetHealth(), 100);
    EXPECT_TRUE(obj.IsAlive());
}

TEST(GameObjectTests, Movement) {
    GameObject obj;

    obj.Move(10.0f, 5.0f);

    EXPECT_EQ(obj.GetX(), 10.0f);
    EXPECT_EQ(obj.GetY(), 5.0f);
}

TEST(GameObjectTests, TakingDamage) {
    GameObject obj;

    obj.TakeDamage(30);
    EXPECT_EQ(obj.GetHealth(), 70);
    EXPECT_TRUE(obj.IsAlive());

    obj.TakeDamage(70);
    EXPECT_EQ(obj.GetHealth(), 0);
    EXPECT_FALSE(obj.IsAlive());
}

TEST(GameObjectTests, OverDamage) {
    GameObject obj;

    obj.TakeDamage(150); // Mehr als maximal Health
    EXPECT_EQ(obj.GetHealth(), 0);
    EXPECT_GE(obj.GetHealth(), 0); // Health sollte nie negativ sein
}

// ============================================================================
// STRESS TESTS
// ============================================================================

TEST(StressTests, LargeVectorOperations) {
    AZTEST_BENCHMARK("Large Vector Creation");

    std::vector<int> largeVector;
    largeVector.reserve(1000000);

    for (int i = 0; i < 1000000; ++i) {
        largeVector.push_back(i);
    }

    EXPECT_EQ(largeVector.size(), 1000000);
    EXPECT_EQ(largeVector[500000], 500000);
}

TEST(StressTests, ManyObjectCreations) {
    AZTEST_BENCHMARK("Create 10000 GameObjects");

    std::vector<GameObject> objects;
    objects.reserve(10000);

    for (int i = 0; i < 10000; ++i) {
        objects.emplace_back();
    }

    EXPECT_EQ(objects.size(), 10000);
}

// ============================================================================
// FAILURE DEMONSTRATION (auskommentiert, damit Tests nicht fehlschlagen)
// ============================================================================

// Unkommentiere diese Tests um Fehler zu sehen:

/*
TEST(FailureExamples, IntentionalFailure) {
    EXPECT_EQ(1, 2); // Wird fehlschlagen
    EXPECT_TRUE(false); // Wird auch fehlschlagen
}

TEST(FailureExamples, AssertFailure) {
    ASSERT_EQ(5, 10); // Stoppt hier
    EXPECT_TRUE(true); // Wird nie erreicht
}
*/

// ============================================================================
// MAIN
// ============================================================================

int main() {
    return RUN_ALL_TESTS();
}
