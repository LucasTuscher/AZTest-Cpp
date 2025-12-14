#include <AZTest/AZTest.h>
#include <vector>
#include <memory>
#include <cmath>

using namespace AZTest;

// ============================================================================
// SIMULATED ENGINE COMPONENTS
// ============================================================================

// Simulated Vector3 class for a game engine
struct Vector3 {
    float x, y, z;

    Vector3(float x_ = 0, float y_ = 0, float z_ = 0)
        : x(x_), y(y_), z(z_) {}

    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    float Length() const {
        return std::sqrt(x*x + y*y + z*z);
    }

    Vector3 Normalized() const {
        float len = Length();
        if (len > 0.0f) {
            return Vector3(x/len, y/len, z/len);
        }
        return Vector3();
    }

    float Dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }
};

// Simulated GameObject
class GameObject {
private:
    Vector3 position_;
    Vector3 velocity_;
    int health_;
    bool active_;

public:
    GameObject()
        : position_(0, 0, 0)
        , velocity_(0, 0, 0)
        , health_(100)
        , active_(true) {}

    void SetPosition(const Vector3& pos) { position_ = pos; }
    Vector3 GetPosition() const { return position_; }

    void SetVelocity(const Vector3& vel) { velocity_ = vel; }
    Vector3 GetVelocity() const { return velocity_; }

    void Update(float deltaTime) {
        if (active_) {
            position_.x += velocity_.x * deltaTime;
            position_.y += velocity_.y * deltaTime;
            position_.z += velocity_.z * deltaTime;
        }
    }

    void TakeDamage(int damage) {
        health_ -= damage;
        if (health_ <= 0) {
            health_ = 0;
            active_ = false;
        }
    }

    int GetHealth() const { return health_; }
    bool IsActive() const { return active_; }
};

// ============================================================================
// VECTOR3 TESTS
// ============================================================================

TEST(Vector3Tests, Construction) {
    Vector3 v1;
    EXPECT_EQ(v1.x, 0.0f);
    EXPECT_EQ(v1.y, 0.0f);
    EXPECT_EQ(v1.z, 0.0f);

    Vector3 v2(1.0f, 2.0f, 3.0f);
    EXPECT_EQ(v2.x, 1.0f);
    EXPECT_EQ(v2.y, 2.0f);
    EXPECT_EQ(v2.z, 3.0f);
}

TEST(Vector3Tests, Addition) {
    Vector3 v1(1, 2, 3);
    Vector3 v2(4, 5, 6);
    Vector3 result = v1 + v2;

    EXPECT_EQ(result.x, 5.0f);
    EXPECT_EQ(result.y, 7.0f);
    EXPECT_EQ(result.z, 9.0f);
}

TEST(Vector3Tests, Length) {
    Vector3 v(3, 4, 0);
    float length = v.Length();

    EXPECT_NEAR(length, 5.0f, 0.001f);
}

TEST(Vector3Tests, Normalization) {
    Vector3 v(3, 4, 0);
    Vector3 normalized = v.Normalized();

    EXPECT_NEAR(normalized.Length(), 1.0f, 0.001f);
}

TEST(Vector3Tests, DotProduct) {
    Vector3 v1(1, 0, 0);
    Vector3 v2(0, 1, 0);

    float dot = v1.Dot(v2);
    EXPECT_NEAR(dot, 0.0f, 0.001f); // Orthogonal vectors
}

// ============================================================================
// GAMEOBJECT TESTS
// ============================================================================

TEST(GameObjectTests, InitialState) {
    GameObject obj;

    EXPECT_EQ(obj.GetHealth(), 100);
    EXPECT_TRUE(obj.IsActive());
    EXPECT_EQ(obj.GetPosition().x, 0.0f);
}

TEST(GameObjectTests, Movement) {
    GameObject obj;
    obj.SetVelocity(Vector3(10, 0, 0));

    obj.Update(1.0f); // 1 second

    Vector3 pos = obj.GetPosition();
    EXPECT_NEAR(pos.x, 10.0f, 0.001f);
    EXPECT_NEAR(pos.y, 0.0f, 0.001f);
}

TEST(GameObjectTests, Damage) {
    GameObject obj;

    obj.TakeDamage(30);
    EXPECT_EQ(obj.GetHealth(), 70);
    EXPECT_TRUE(obj.IsActive());

    obj.TakeDamage(70);
    EXPECT_EQ(obj.GetHealth(), 0);
    EXPECT_FALSE(obj.IsActive());
}

TEST(GameObjectTests, MovementStopsWhenDead) {
    GameObject obj;
    obj.SetVelocity(Vector3(10, 0, 0));

    obj.TakeDamage(100); // Kill object
    obj.Update(1.0f);

    Vector3 pos = obj.GetPosition();
    EXPECT_NEAR(pos.x, 0.0f, 0.001f); // Should not have moved
}

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================

PERFORMANCE_TEST(PerformanceTests, Vector3Creation, 100000) {
    Vector3 v(i, i*2, i*3);
    (void)v; // Suppress unused warning
}
END_PERFORMANCE_TEST

PERFORMANCE_TEST(PerformanceTests, GameObjectUpdate, 10000) {
    GameObject obj;
    obj.SetVelocity(Vector3(1, 1, 1));
    obj.Update(0.016f); // ~60 FPS
}
END_PERFORMANCE_TEST

// ============================================================================
// STRESS TESTS
// ============================================================================

TEST(StressTests, ManyGameObjects) {
    ENGINE_BENCHMARK("Create 10000 GameObjects");

    std::vector<GameObject> objects;
    objects.reserve(10000);

    for (int i = 0; i < 10000; ++i) {
        objects.emplace_back();
    }

    EXPECT_EQ(objects.size(), 10000u);
}

TEST(StressTests, LargeVectorOperations) {
    ENGINE_BENCHMARK("1000000 Vector additions");

    Vector3 result(0, 0, 0);
    for (int i = 0; i < 1000000; ++i) {
        result = result + Vector3(1, 1, 1);
    }

    EXPECT_GT(result.x, 0.0f);
}

// ============================================================================
// MAIN WITH MULTIPLE REPORTERS
// ============================================================================

int main() {
    // Initialize with both console and XML reporters
    auto consoleReporter = std::make_shared<Reporters::ConsoleReporter>();
    auto xmlReporter = std::make_shared<Reporters::XMLReporter>("advanced_test_results.xml");

    std::vector<std::shared_ptr<Core::IReporter>> reporters = {
        consoleReporter,
        xmlReporter
    };

    InitializeWithReporters(reporters);

    return RUN_ALL_TESTS();
}
