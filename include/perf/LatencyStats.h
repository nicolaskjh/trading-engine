#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace engine {

/**
 * LatencyStats: Collects and calculates latency statistics
 * Tracks samples and computes percentiles, mean, stddev, etc.
 */
class LatencyStats {
public:
    LatencyStats() = default;

    /**
     * Add a latency sample (in microseconds)
     */
    void addSample(uint64_t latencyUs) {
        samples_.push_back(latencyUs);
    }
    
    /**
     * Reserve capacity to avoid reallocations
     */
    void reserve(size_t capacity) {
        samples_.reserve(capacity);
    }

    /**
     * Calculate statistics (call this after all samples collected)
     */
    void calculate() {
        if (samples_.empty()) return;

        // Sort for percentile calculations
        std::sort(samples_.begin(), samples_.end());

        count_ = samples_.size();
        min_ = samples_.front();
        max_ = samples_.back();

        // Calculate mean
        uint64_t sum = 0;
        for (auto sample : samples_) {
            sum += sample;
        }
        mean_ = static_cast<double>(sum) / count_;

        // Calculate standard deviation
        double variance = 0.0;
        for (auto sample : samples_) {
            double diff = sample - mean_;
            variance += diff * diff;
        }
        stddev_ = std::sqrt(variance / count_);

        // Calculate percentiles
        median_ = percentile(50);
        p95_ = percentile(95);
        p99_ = percentile(99);
        p999_ = percentile(99.9);
    }

    /**
     * Get statistics
     */
    size_t getCount() const { return count_; }
    double getMean() const { return mean_; }
    double getMedian() const { return median_; }
    double getStdDev() const { return stddev_; }
    uint64_t getMin() const { return min_; }
    uint64_t getMax() const { return max_; }
    uint64_t getP95() const { return p95_; }
    uint64_t getP99() const { return p99_; }
    uint64_t getP999() const { return p999_; }

    /**
     * Clear all samples
     */
    void clear() {
        samples_.clear();
        count_ = 0;
        mean_ = 0.0;
        median_ = 0.0;
        stddev_ = 0.0;
        min_ = 0;
        max_ = 0;
        p95_ = 0;
        p99_ = 0;
        p999_ = 0;
    }

    /**
     * Get a formatted report string
     */
    std::string report(const std::string& label = "") const;

private:
    uint64_t percentile(double p) const {
        if (samples_.empty()) return 0;
        
        double index = (p / 100.0) * (samples_.size() - 1);
        size_t lower = static_cast<size_t>(std::floor(index));
        size_t upper = static_cast<size_t>(std::ceil(index));
        
        if (lower == upper) {
            return samples_[lower];
        }
        
        // Linear interpolation
        double weight = index - lower;
        return static_cast<uint64_t>(
            samples_[lower] * (1.0 - weight) + samples_[upper] * weight
        );
    }

    std::vector<uint64_t> samples_;
    
    // Calculated statistics
    size_t count_ = 0;
    double mean_ = 0.0;
    double median_ = 0.0;
    double stddev_ = 0.0;
    uint64_t min_ = 0;
    uint64_t max_ = 0;
    uint64_t p95_ = 0;
    uint64_t p99_ = 0;
    uint64_t p999_ = 0;
};

} // namespace engine
