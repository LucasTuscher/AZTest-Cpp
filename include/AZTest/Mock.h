#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <stdexcept>
#include <typeindex>
#include <mutex>

namespace AZTest {
namespace Mock {

struct MockCallRecord {
    std::string methodName;
    int callCount = 0;
};

class MockRegistry {
    std::mutex mtx_;
public:
    static MockRegistry& Instance() {
        static MockRegistry instance;
        return instance;
    }
    std::map<std::string, MockCallRecord> records;
    void RecordCall(const std::string& name) {
        std::lock_guard<std::mutex> lock(mtx_);
        records[name].callCount++;
    }
    int GetCallCount(const std::string& name) {
        std::lock_guard<std::mutex> lock(mtx_);
        return records[name].callCount;
    }
    void Clear() {
        std::lock_guard<std::mutex> lock(mtx_);
        records.clear();
    }
};

template<typename ReturnType, typename... Args>
class MockFunction {
public:
    MockFunction(const std::string& name) : name_(name), defaultReturnSet_(false) {}

    ReturnType operator()(Args... args) {
        MockRegistry::Instance().RecordCall(name_);
        if (overrideFunc_) {
            return overrideFunc_(std::forward<Args>(args)...);
        }
        if (defaultReturnSet_) {
            return defaultReturn_;
        }
        if constexpr (!std::is_void_v<ReturnType>) {
            return ReturnType{};
        }
    }

    void WillOnce(std::function<ReturnType(Args...)> func) { overrideFunc_ = func; }
    void WillRepeatedly(std::function<ReturnType(Args...)> func) { overrideFunc_ = func; }
    void Returns(ReturnType val) { defaultReturn_ = val; defaultReturnSet_ = true; }

private:
    std::string name_;
    std::function<ReturnType(Args...)> overrideFunc_;
    ReturnType defaultReturn_;
    bool defaultReturnSet_;
};

#define MOCK_METHOD0(ReturnType, MethodName) \
    virtual ReturnType MethodName() override { return mock_##MethodName(); } \
    AZTest::Mock::MockFunction<ReturnType> mock_##MethodName{#MethodName};

#define MOCK_METHOD1(ReturnType, MethodName, Arg1Type) \
    virtual ReturnType MethodName(Arg1Type a) override { return mock_##MethodName(a); } \
    AZTest::Mock::MockFunction<ReturnType, Arg1Type> mock_##MethodName{#MethodName};

#define MOCK_METHOD2(ReturnType, MethodName, Arg1Type, Arg2Type) \
    virtual ReturnType MethodName(Arg1Type a, Arg2Type b) override { return mock_##MethodName(a, b); } \
    AZTest::Mock::MockFunction<ReturnType, Arg1Type, Arg2Type> mock_##MethodName{#MethodName};

#define MOCK_METHOD3(ReturnType, MethodName, Arg1Type, Arg2Type, Arg3Type) \
    virtual ReturnType MethodName(Arg1Type a, Arg2Type b, Arg3Type c) override { return mock_##MethodName(a, b, c); } \
    AZTest::Mock::MockFunction<ReturnType, Arg1Type, Arg2Type, Arg3Type> mock_##MethodName{#MethodName};

#define MOCK_METHOD4(ReturnType, MethodName, Arg1Type, Arg2Type, Arg3Type, Arg4Type) \
    virtual ReturnType MethodName(Arg1Type a, Arg2Type b, Arg3Type c, Arg4Type d) override { return mock_##MethodName(a, b, c, d); } \
    AZTest::Mock::MockFunction<ReturnType, Arg1Type, Arg2Type, Arg3Type, Arg4Type> mock_##MethodName{#MethodName};

#define MOCK_METHOD5(ReturnType, MethodName, Arg1Type, Arg2Type, Arg3Type, Arg4Type, Arg5Type) \
    virtual ReturnType MethodName(Arg1Type a, Arg2Type b, Arg3Type c, Arg4Type d, Arg5Type e) override { return mock_##MethodName(a, b, c, d, e); } \
    AZTest::Mock::MockFunction<ReturnType, Arg1Type, Arg2Type, Arg3Type, Arg4Type, Arg5Type> mock_##MethodName{#MethodName};

#define MOCK_METHOD6(ReturnType, MethodName, Arg1Type, Arg2Type, Arg3Type, Arg4Type, Arg5Type, Arg6Type) \
    virtual ReturnType MethodName(Arg1Type a, Arg2Type b, Arg3Type c, Arg4Type d, Arg5Type e, Arg6Type f) override { return mock_##MethodName(a, b, c, d, e, f); } \
    AZTest::Mock::MockFunction<ReturnType, Arg1Type, Arg2Type, Arg3Type, Arg4Type, Arg5Type, Arg6Type> mock_##MethodName{#MethodName};

#define EXPECT_CALL(mock_obj, method_name) mock_obj.mock_##method_name

} // namespace Mock
} // namespace AZTest
