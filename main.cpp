#include <iostream>
#include <chrono>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <cmath>

class MemoryManager {
public:
    class Float8 {
        uint8_t value;
        static const int EXP_BITS = 4;
        static const int MANT_BITS = 3;
        static const int BIAS = 7;

    public:
        Float8(float f = 0.0f) {
            if (f == 0) {
                value = 0;
                return;
            }

            int sign = (f < 0) ? 1 : 0;
            f = std::abs(f);
            int exponent;
            float mantissa = std::frexp(f, &exponent);

            // Exponent with bias applied
            exponent += BIAS;

            if (exponent <= 0) {
                value = 0;
            } else if (exponent >= (1 << EXP_BITS)) {
                value = (sign << 7) | (((1 << EXP_BITS) - 1) << MANT_BITS);  // Overflow
            } else {
                int mant = static_cast<int>(mantissa * (1 << MANT_BITS));
                value = (sign << 7) | (exponent << MANT_BITS) | mant;
            }
        }

        operator float() const {
            if (value == 0) return 0.0f;

            int sign = (value >> 7) & 1;
            int exponent = ((value >> MANT_BITS) & ((1 << EXP_BITS) - 1)) - BIAS;
            float mantissa = (value & ((1 << MANT_BITS) - 1)) / static_cast<float>(1 << MANT_BITS);
            mantissa += 1.0f;

            float result = std::ldexp(mantissa, exponent);
            return sign ? -result : result;
        }

        Float8 operator+(const Float8& other) const {
            return Float8(static_cast<float>(*this) + static_cast<float>(other));
        }

        Float8 operator*(const Float8& other) const {
            return Float8(static_cast<float>(*this) * static_cast<float>(other));
        }

        void* operator new(size_t size) {
            return MemoryManager::allocate(size);
        }

        void operator delete(void* ptr, size_t size) noexcept {
            MemoryManager::deallocate(ptr, size);
        }

        void* operator new[](size_t size) {
            return MemoryManager::allocate(size);
        }

        void operator delete[](void* ptr, size_t size) noexcept {
            MemoryManager::deallocate(ptr, size);
        }
    };

private:
    static size_t totalAllocated;
    static size_t totalFreed;
    static std::mutex memMutex;

    static void* allocate(size_t size) {
        std::lock_guard<std::mutex> lock(memMutex);
        totalAllocated += size;
        return std::malloc(size);
    }

    static void deallocate(void* ptr, size_t size) noexcept {
        std::lock_guard<std::mutex> lock(memMutex);
        totalFreed += size;
        std::free(ptr);
    }

public:
    void nonOptimizedMemoryUsage() {
        for (int i = 0; i < 1000000; ++i) {
            Float8* data = new Float8[1000];
            for (int j = 0; j < 1000; ++j) {
                data[j] = static_cast<float>(j);
            }
            delete[] data;
        }
        std::cout << "Non-optimized memory: Allocated: "
                  << totalAllocated / (1024.0 * 1024.0) << " MB, Freed: "
                  << totalFreed / (1024.0 * 1024.0) << " MB\n";
    }

    void optimizedMemoryUsage() {
        Float8* data = new Float8[1000];

        for (int i = 0; i < 1000000; ++i) {
            for (int j = 0; j < 1000; ++j) {
                data[j] = static_cast<float>(j);
            }
        }

        delete[] data;
        std::cout << "Optimized memory: Allocated: "
                  << totalAllocated / (1024.0 * 1024.0) << " MB, Freed: "
                  << totalFreed / (1024.0 * 1024.0) << " MB\n";
    }

    double estimateEnergyConsumption(double executionTime, double cpuPower) {
        return executionTime * cpuPower;
    }

    void resetMemoryTracking() {
        totalAllocated = 0;
        totalFreed = 0;
    }

    void runAllTests() {
        std::cout << "Memory usage and energy consumption comparison.\n";

        // Estimated CPU power consumption in Watts for M2 Pro
        double cpuPower = 20.0;

        // Non-optimized version
        auto start = std::chrono::high_resolution_clock::now();
        nonOptimizedMemoryUsage();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> nonOptimizedDuration = end - start;
        double nonOptimizedEnergy = estimateEnergyConsumption(nonOptimizedDuration.count(), cpuPower);

        std::cout << "Non-optimized duration: " << nonOptimizedDuration.count() << " seconds\n";
        std::cout << "Non-optimized energy consumption: " << nonOptimizedEnergy << " Joules\n";

        // Reset allocation tracking for the optimized version
        resetMemoryTracking();

        // Optimized version
        start = std::chrono::high_resolution_clock::now();
        optimizedMemoryUsage();
        end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> optimizedDuration = end - start;
        double optimizedEnergy = estimateEnergyConsumption(optimizedDuration.count(), cpuPower);

        std::cout << "Optimized duration: " << optimizedDuration.count() << " seconds\n";
        std::cout << "Optimized energy consumption: " << optimizedEnergy << " Joules\n";
    }
};

size_t MemoryManager::totalAllocated = 0;
size_t MemoryManager::totalFreed = 0;
std::mutex MemoryManager::memMutex;

int main() {
    MemoryManager manager;
    manager.runAllTests();
    return 0;
}