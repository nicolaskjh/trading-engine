#include "perf/LatencyStats.h"

#include <iomanip>
#include <sstream>

namespace engine {

std::string LatencyStats::report(const std::string& label) const {
    std::ostringstream oss;
    
    if (!label.empty()) {
        oss << label << ":\n";
    }
    
    oss << "  Samples: " << count_ << "\n";
    oss << "  Mean:    " << std::fixed << std::setprecision(2) << mean_ << " μs\n";
    oss << "  Median:  " << median_ << " μs\n";
    oss << "  StdDev:  " << std::fixed << std::setprecision(2) << stddev_ << " μs\n";
    oss << "  Min:     " << min_ << " μs\n";
    oss << "  Max:     " << max_ << " μs\n";
    oss << "  P95:     " << p95_ << " μs\n";
    oss << "  P99:     " << p99_ << " μs\n";
    oss << "  P99.9:   " << p999_ << " μs";
    
    return oss.str();
}

} // namespace engine
