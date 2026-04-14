#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <type_traits>
#include <algorithm>

namespace AZTest {
namespace Matchers {

// Forward decl for internal stringifier
template <typename T> std::string PrintValue(const T& val);

template <typename T>
class MatcherBase {
public:
    virtual ~MatcherBase() = default;
    virtual bool Match(const T& actual) const = 0;
    virtual std::string Describe() const = 0;
};

template <typename T>
class Matcher {
public:
    Matcher(std::shared_ptr<MatcherBase<T>> impl) : impl_(std::move(impl)) {}
    bool Match(const T& actual) const { return impl_->Match(actual); }
    std::string Describe() const { return impl_->Describe(); }
private:
    std::shared_ptr<MatcherBase<T>> impl_;
};

// --- Implementations ---

template <typename T>
class EqMatcher : public MatcherBase<T> {
    T expected_;
public:
    explicit EqMatcher(T expected) : expected_(std::move(expected)) {}
    bool Match(const T& actual) const override { return actual == expected_; }
    std::string Describe() const override { return "is equal to " + PrintValue(expected_); }
};

template <typename T>
class NeMatcher : public MatcherBase<T> {
    T expected_;
public:
    explicit NeMatcher(T expected) : expected_(std::move(expected)) {}
    bool Match(const T& actual) const override { return actual != expected_; }
    std::string Describe() const override { return "is not equal to " + PrintValue(expected_); }
};

template <typename T>
class GtMatcher : public MatcherBase<T> {
    T expected_;
public:
    explicit GtMatcher(T expected) : expected_(std::move(expected)) {}
    bool Match(const T& actual) const override { return actual > expected_; }
    std::string Describe() const override { return "is greater than " + PrintValue(expected_); }
};

template <typename T>
class LtMatcher : public MatcherBase<T> {
    T expected_;
public:
    explicit LtMatcher(T expected) : expected_(std::move(expected)) {}
    bool Match(const T& actual) const override { return actual < expected_; }
    std::string Describe() const override { return "is less than " + PrintValue(expected_); }
};

class StartsWithMatcher : public MatcherBase<std::string> {
    std::string prefix_;
public:
    explicit StartsWithMatcher(std::string prefix) : prefix_(std::move(prefix)) {}
    bool Match(const std::string& actual) const override { return actual.find(prefix_) == 0; }
    std::string Describe() const override { return "starts with \"" + prefix_ + "\""; }
};

class EndsWithMatcher : public MatcherBase<std::string> {
    std::string suffix_;
public:
    explicit EndsWithMatcher(std::string suffix) : suffix_(std::move(suffix)) {}
    bool Match(const std::string& actual) const override {
        if (suffix_.size() > actual.size()) return false;
        return std::equal(suffix_.rbegin(), suffix_.rend(), actual.rbegin());
    }
    std::string Describe() const override { return "ends with \"" + suffix_ + "\""; }
};

class HasSubstrMatcher : public MatcherBase<std::string> {
    std::string substr_;
public:
    explicit HasSubstrMatcher(std::string substr) : substr_(std::move(substr)) {}
    bool Match(const std::string& actual) const override { return actual.find(substr_) != std::string::npos; }
    std::string Describe() const override { return "contains substring \"" + substr_ + "\""; }
};

// Container elements
template <typename Container, typename Element>
class ContainsMatcher : public MatcherBase<Container> {
    Element elem_;
public:
    explicit ContainsMatcher(Element elem) : elem_(std::move(elem)) {}
    bool Match(const Container& actual) const override {
        for (const auto& item : actual) {
            if (item == elem_) return true;
        }
        return false;
    }
    std::string Describe() const override { return "contains element " + PrintValue(elem_); }
};

// Logical Combinators
template <typename T>
class AllOfMatcher : public MatcherBase<T> {
    std::vector<Matcher<T>> matchers_;
public:
    explicit AllOfMatcher(std::vector<Matcher<T>> matchers) : matchers_(std::move(matchers)) {}
    bool Match(const T& actual) const override {
        for (const auto& m : matchers_) {
            if (!m.Match(actual)) return false;
        }
        return true;
    }
    std::string Describe() const override {
        std::string res = "(";
        for (size_t i = 0; i < matchers_.size(); ++i) {
            res += matchers_[i].Describe();
            if (i < matchers_.size() - 1) res += " AND ";
        }
        res += ")";
        return res;
    }
};

template <typename T>
class AnyOfMatcher : public MatcherBase<T> {
    std::vector<Matcher<T>> matchers_;
public:
    explicit AnyOfMatcher(std::vector<Matcher<T>> matchers) : matchers_(std::move(matchers)) {}
    bool Match(const T& actual) const override {
        for (const auto& m : matchers_) {
            if (m.Match(actual)) return true;
        }
        return false;
    }
    std::string Describe() const override {
        std::string res = "(";
        for (size_t i = 0; i < matchers_.size(); ++i) {
            res += matchers_[i].Describe();
            if (i < matchers_.size() - 1) res += " OR ";
        }
        res += ")";
        return res;
    }
};

template <typename T>
class NotMatcher : public MatcherBase<T> {
    Matcher<T> m_;
public:
    explicit NotMatcher(Matcher<T> m) : m_(std::move(m)) {}
    bool Match(const T& actual) const override { return !m_.Match(actual); }
    std::string Describe() const override { return "not (" + m_.Describe() + ")"; }
};

template <typename T>
class AnyMatcher : public MatcherBase<T> {
public:
    bool Match(const T&) const override { return true; }
    std::string Describe() const override { return "anything"; }
};

template <typename T>
class IsNullMatcher : public MatcherBase<T> {
public:
    bool Match(const T& actual) const override { return actual == nullptr; }
    std::string Describe() const override { return "is null"; }
};

template <typename T>
class FloatNearMatcher : public MatcherBase<T> {
    T expected_;
    T tolerance_;
public:
    FloatNearMatcher(T expected, T tolerance) : expected_(expected), tolerance_(tolerance) {}
    bool Match(const T& actual) const override {
        T diff = (actual > expected_) ? (actual - expected_) : (expected_ - actual);
        return diff <= tolerance_;
    }
    std::string Describe() const override {
        std::stringstream ss;
        ss << "is near " << PrintValue(expected_) << " (within " << PrintValue(tolerance_) << ")";
        return ss.str();
    }
};

// --- Factory Functions ---

template <typename T>
Matcher<T> Eq(T expected) { return Matcher<T>(std::make_shared<EqMatcher<T>>(std::move(expected))); }

template <typename T>
Matcher<T> Ne(T expected) { return Matcher<T>(std::make_shared<NeMatcher<T>>(std::move(expected))); }

template <typename T>
Matcher<T> Gt(T expected) { return Matcher<T>(std::make_shared<GtMatcher<T>>(std::move(expected))); }

template <typename T>
Matcher<T> Lt(T expected) { return Matcher<T>(std::make_shared<LtMatcher<T>>(std::move(expected))); }

inline Matcher<std::string> StartsWith(std::string prefix) { return Matcher<std::string>(std::make_shared<StartsWithMatcher>(std::move(prefix))); }
inline Matcher<std::string> EndsWith(std::string suffix) { return Matcher<std::string>(std::make_shared<EndsWithMatcher>(std::move(suffix))); }
inline Matcher<std::string> HasSubstr(std::string substr) { return Matcher<std::string>(std::make_shared<HasSubstrMatcher>(std::move(substr))); }

template <typename Container, typename Element>
Matcher<Container> Contains(Element elem) {
    return Matcher<Container>(std::make_shared<ContainsMatcher<Container, Element>>(std::move(elem)));
}

template <typename T, typename... Args>
Matcher<T> AllOf(Matcher<T> first, Args... args) {
    std::vector<Matcher<T>> matchers = { first, args... };
    return Matcher<T>(std::make_shared<AllOfMatcher<T>>(std::move(matchers)));
}

template <typename T, typename... Args>
Matcher<T> AnyOf(Matcher<T> first, Args... args) {
    std::vector<Matcher<T>> matchers = { first, args... };
    return Matcher<T>(std::make_shared<AnyOfMatcher<T>>(std::move(matchers)));
}

template <typename T>
Matcher<T> Not(Matcher<T> m) {
    return Matcher<T>(std::make_shared<NotMatcher<T>>(std::move(m)));
}

template <typename T>
Matcher<T> Anything() { return Matcher<T>(std::make_shared<AnyMatcher<T>>()); }

template <typename T>
Matcher<T> IsNull() { return Matcher<T>(std::make_shared<IsNullMatcher<T>>()); }

inline Matcher<bool> IsTrue() { return Eq(true); }
inline Matcher<bool> IsFalse() { return Eq(false); }

template <typename T>
Matcher<T> Near(T expected, T tolerance) {
    return Matcher<T>(std::make_shared<FloatNearMatcher<T>>(expected, tolerance));
}

inline Matcher<float> FloatEq(float expected) {
    return Near(expected, 4.0f * std::numeric_limits<float>::epsilon() * std::max(1.0f, std::abs(expected)));
}

inline Matcher<double> DoubleEq(double expected) {
    return Near(expected, 4.0 * std::numeric_limits<double>::epsilon() * std::max(1.0, std::abs(expected)));
}

// Internal stringifier
template <typename T> std::string PrintValue(const T& val) {
    return AZTest::Detail::ToString(val);
}

} // namespace Matchers
} // namespace AZTest

// EXPECT_THAT Macro
#define EXPECT_THAT(value, matcher) \
    do { \
        auto AZTEST_UNIQUE_NAME(val) = (value); \
        auto AZTEST_UNIQUE_NAME(m) = (matcher); \
        if (!AZTEST_UNIQUE_NAME(m).Match(AZTEST_UNIQUE_NAME(val))) { \
            std::stringstream ss; \
            ss << "Value of: " #value "\n" \
               << "Expected: " << AZTEST_UNIQUE_NAME(m).Describe() << "\n" \
               << "  Actual: " << AZTest::Matchers::PrintValue(AZTEST_UNIQUE_NAME(val)); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_THAT(value, matcher) \
    do { \
        auto AZTEST_UNIQUE_NAME(val) = (value); \
        auto AZTEST_UNIQUE_NAME(m) = (matcher); \
        if (!AZTEST_UNIQUE_NAME(m).Match(AZTEST_UNIQUE_NAME(val))) { \
            std::stringstream ss; \
            ss << "Value of: " #value "\n" \
               << "Expected: " << AZTEST_UNIQUE_NAME(m).Describe() << "\n" \
               << "  Actual: " << AZTest::Matchers::PrintValue(AZTEST_UNIQUE_NAME(val)); \
            AZTest::Core::TestRegistry::Instance().RecordFailure(ss.str(), __FILE__, __LINE__); \
            throw AZTest::AssertionFailure(ss.str()); \
        } \
    } while(0)
