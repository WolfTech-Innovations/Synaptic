#ifndef CONSCIOUSNESS_COHERENCE_H
#define CONSCIOUSNESS_COHERENCE_H

#include <vector>

struct CoherenceMetrics {
    double semantic_coherence;
    double temporal_coherence;
    double integrated_information;
    double overall_coherence; // Add this line
};

class ConsciousnessCoherence {
public:
    static CoherenceMetrics calculate_coherence(double current_valence, int generation);
    static std::vector<double> get_consciousness_trajectory();
    static double get_average_coherence(); // Add this line

private:
    static double calculate_semantic_coherence(const std::vector<double>& qualia_valences);
    static double calculate_temporal_coherence(const std::vector<double>& history);
    static double calculate_integrated_information(const std::vector<double>& state_vector);

    static std::vector<double> consciousness_trajectory_;
    static std::vector<double> coherence_history_;
};

#endif