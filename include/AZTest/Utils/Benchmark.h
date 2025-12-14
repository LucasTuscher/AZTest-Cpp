#pragma once

#include <chrono>
#include <string>
#include <iostream>

namespace AZTest {
namespace Utils {

class Benchmark {
private:
    std::string name_;
    std::chrono::high_resolution_clock::time_point start_;
    bool silent_;

public:
    explicit Benchmark(const std::string& name, bool silent = false)
        : name_(name)
        , start_(std::chrono::high_resolution_clock::now())
        , silent_(silent) {
        if (!silent_) {
            std::cout << "[BENCH] Starting: " << name_ << std::endl;
        }
    }

    ~Benchmark() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - start_).count();

        if (!silent_) {
            std::cout << "[BENCH] " << name_ << ": "
                     << duration << " ms" << std::endl;
        }
    }

    double ElapsedMs() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(now - start_).count();
    }
};

} // namespace Utils
} // namespace AZTest
