#include <AZTest/AZTest.h>
#include <memory>
#include <string>
#include <map>

using namespace AZTest;
using namespace AZTest::Core;

// ============================================================================
// SIMULATED ENGINE RESOURCE MANAGER
// ============================================================================

class Texture {
private:
    std::string path_;
    int width_;
    int height_;
    bool loaded_;

public:
    Texture(const std::string& path, int width, int height)
        : path_(path), width_(width), height_(height), loaded_(true) {}

    const std::string& GetPath() const { return path_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    bool IsLoaded() const { return loaded_; }
};

class ResourceManager {
private:
    std::map<std::string, std::shared_ptr<Texture>> textures_;

public:
    std::shared_ptr<Texture> LoadTexture(const std::string& path) {
        // Simulate loading
        auto texture = std::make_shared<Texture>(path, 512, 512);
        textures_[path] = texture;
        return texture;
    }

    std::shared_ptr<Texture> GetTexture(const std::string& path) {
        auto it = textures_.find(path);
        if (it != textures_.end()) {
            return it->second;
        }
        return nullptr;
    }

    void UnloadTexture(const std::string& path) {
        textures_.erase(path);
    }

    void UnloadAll() {
        textures_.clear();
    }

    size_t GetTextureCount() const {
        return textures_.size();
    }
};

// ============================================================================
// FIXTURE TESTS
// ============================================================================

/**
 * @brief Fixture for ResourceManager tests
 *
 * SetUp() is called before each test
 * TearDown() is called after each test
 */
class ResourceManagerFixture : public TestFixture {
protected:
    std::unique_ptr<ResourceManager> manager_;

    void SetUp() override {
        // Create a fresh ResourceManager for each test
        manager_ = std::make_unique<ResourceManager>();
    }

    void TearDown() override {
        // Clean up after each test
        if (manager_) {
            manager_->UnloadAll();
        }
        manager_.reset();
    }

    // Helper method available to all tests
    void LoadCommonTextures() {
        manager_->LoadTexture("player.png");
        manager_->LoadTexture("enemy.png");
        manager_->LoadTexture("background.png");
    }
};

TEST_F(ResourceManagerFixture, LoadSingleTexture) {
    auto texture = manager_->LoadTexture("test.png");

    ASSERT_NE(texture, nullptr);
    EXPECT_STREQ(texture->GetPath(), "test.png");
    EXPECT_EQ(texture->GetWidth(), 512);
    EXPECT_TRUE(texture->IsLoaded());
}

TEST_F(ResourceManagerFixture, LoadMultipleTextures) {
    LoadCommonTextures();

    EXPECT_EQ(manager_->GetTextureCount(), 3u);

    auto player = manager_->GetTexture("player.png");
    ASSERT_NE(player, nullptr);
    EXPECT_STREQ(player->GetPath(), "player.png");
}

TEST_F(ResourceManagerFixture, GetNonExistentTexture) {
    auto texture = manager_->GetTexture("nonexistent.png");

    EXPECT_EQ(texture, nullptr);
}

TEST_F(ResourceManagerFixture, UnloadTexture) {
    manager_->LoadTexture("temp.png");
    EXPECT_EQ(manager_->GetTextureCount(), 1u);

    manager_->UnloadTexture("temp.png");
    EXPECT_EQ(manager_->GetTextureCount(), 0u);
}

TEST_F(ResourceManagerFixture, UnloadAll) {
    LoadCommonTextures();
    EXPECT_EQ(manager_->GetTextureCount(), 3u);

    manager_->UnloadAll();
    EXPECT_EQ(manager_->GetTextureCount(), 0u);
}

TEST_F(ResourceManagerFixture, CachedTextureRetrieval) {
    auto texture1 = manager_->LoadTexture("cached.png");
    auto texture2 = manager_->GetTexture("cached.png");

    // Should be the same object
    EXPECT_EQ(texture1.get(), texture2.get());
}

// ============================================================================
// DATABASE FIXTURE EXAMPLE
// ============================================================================

class MockDatabase {
private:
    std::map<int, std::string> data_;
    bool connected_;

public:
    MockDatabase() : connected_(false) {}

    bool Connect() {
        connected_ = true;
        return true;
    }

    void Disconnect() {
        connected_ = false;
        data_.clear();
    }

    bool IsConnected() const { return connected_; }

    void Insert(int id, const std::string& value) {
        data_[id] = value;
    }

    std::string Get(int id) const {
        auto it = data_.find(id);
        return (it != data_.end()) ? it->second : "";
    }

    void Delete(int id) {
        data_.erase(id);
    }

    size_t Count() const {
        return data_.size();
    }
};

class DatabaseFixture : public TestFixture {
protected:
    std::unique_ptr<MockDatabase> db_;

    void SetUp() override {
        db_ = std::make_unique<MockDatabase>();
        db_->Connect();
    }

    void TearDown() override {
        if (db_ && db_->IsConnected()) {
            db_->Disconnect();
        }
        db_.reset();
    }

public:
    static void SetUpTestSuite() {
        // Called once before all tests in this suite
        std::cout << "[DB] Setting up database test suite..." << std::endl;
    }

    static void TearDownTestSuite() {
        // Called once after all tests in this suite
        std::cout << "[DB] Tearing down database test suite..." << std::endl;
    }
};

TEST_F(DatabaseFixture, InsertAndRetrieve) {
    db_->Insert(1, "Hello");
    db_->Insert(2, "World");

    EXPECT_STREQ(db_->Get(1), "Hello");
    EXPECT_STREQ(db_->Get(2), "World");
    EXPECT_EQ(db_->Count(), 2u);
}

TEST_F(DatabaseFixture, DeleteEntry) {
    db_->Insert(1, "Test");
    EXPECT_EQ(db_->Count(), 1u);

    db_->Delete(1);
    EXPECT_EQ(db_->Count(), 0u);
    EXPECT_STREQ(db_->Get(1), "");
}

TEST_F(DatabaseFixture, GetNonExistent) {
    std::string result = db_->Get(999);
    EXPECT_STREQ(result, "");
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    InitializeDefaultReporter();
    return RUN_ALL_TESTS();
}
