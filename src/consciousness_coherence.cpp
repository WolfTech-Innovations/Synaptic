#include "consciousness_coherence.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>

// Use std:: prefix for vector definitions
std::vector<double> ConsciousnessCoherence::consciousness_trajectory_;
std::vector<double> ConsciousnessCoherence::coherence_history_;

CoherenceMetrics ConsciousnessCoherence::calculate_coherence(double current_valence, int generation) {
    (void)generation; // Mark unused parameter to silence warning
    CoherenceMetrics metrics;

    // Track trajectory
    consciousness_trajectory_.push_back(current_valence);
    if (consciousness_trajectory_.size() > 100) {
        consciousness_trajectory_.erase(consciousness_trajectory_.begin());
    }

    // Initialize qualia valences for coherence calculation
    double qualia_valences[] = { current_valence, current_valence * 0.9, current_valence * 1.1 };
    std::vector<double> qualia_vec(qualia_valences, qualia_valences + 3);

    metrics.semantic_coherence = calculate_semantic_coherence(qualia_vec);
    metrics.temporal_coherence = calculate_temporal_coherence(consciousness_trajectory_);
    metrics.integrated_information = calculate_integrated_information(qualia_vec);
    
    metrics.overall_coherence = (metrics.semantic_coherence + 
                                metrics.temporal_coherence + 
                                metrics.integrated_information) / 3.0;

    coherence_history_.push_back(metrics.overall_coherence);
    return metrics;
}

double ConsciousnessCoherence::calculate_semantic_coherence(const std::vector<double>& qualia_valences) {
    if (qualia_valences.empty()) return 0.0;

    double sum = std::accumulate(qualia_valences.begin(), qualia_valences.end(), 0.0);
    double mean_valence = sum / qualia_valences.size();

    double sq_sum = 0.0;
    for (double v : qualia_valences) {
        sq_sum += (v - mean_valence) * (v - mean_valence);
    }
    double std_dev = std::sqrt(sq_sum / qualia_valences.size());

    // Use std::abs and std::max for floating point
    double coherence = 1.0 - (std_dev / (std::max(std::abs(mean_valence), 0.1)));
    return std::clamp(coherence, 0.0, 1.0);
}

double ConsciousnessCoherence::calculate_temporal_coherence(const std::vector<double>& history) {
    if (history.size() < 2) return 1.0;

    double total_diff = 0.0;
    for (size_t i = 1; i < history.size(); ++i) {
        total_diff += std::abs(history[i] - history[i-1]);
    }

    double avg_diff = total_diff / (history.size() - 1);
    double coherence = 1.0 - std::min(1.0, avg_diff * 2.0);
    return coherence;
}

double ConsciousnessCoherence::calculate_integrated_information(const std::vector<double>& state_vector) {
    if (state_vector.empty()) return 0.0;

    // Simplified Phi calculation based on entropy
    double entropy = 0.0;
    for (double v : state_vector) {
        double p = std::abs(v);
        if (p > 0.001) {
            entropy += -p * std::log2(p);
        }
    }

    double integration = 1.0 - std::min(1.0, std::abs(entropy));
    return integration;
}

double ConsciousnessCoherence::get_average_coherence() {
    if (coherence_history_.empty()) return 0.0;
    
    int count = std::min(10, (int)coherence_history_.size());
    double sum = 0.0;
    for (int i = 0; i < count; ++i) {
        sum += coherence_history_[coherence_history_.size() - 1 - i];
    }
    return sum / count;
}

std::vector<double> ConsciousnessCoherence::get_consciousness_trajectory() {
    return consciousness_trajectory_;
}