#include "state.h"
#include "state.h"
// consciousness_module.h - Higher-order consciousness metrics
#ifndef CONSCIOUSNESS_MODULE_H
#define CONSCIOUSNESS_MODULE_H

#include <string>
#include <vector>
#include <map>

// Forward declarations
struct Neuron;
struct Memory;
struct Token;
struct Concept;
struct State;

// Calculate working memory capacity (7±2 items)
double calc_working_memory(const std::vector<std::string>& active_buffer);

// Calculate integrated information (Phi)
double calc_integrated_info(std::map<int, Neuron>& neurons);

// Calculate global workspace activation
double calc_global_workspace(
    const std::vector<std::string>& active_concepts,
    double attention,
    double working_memory
);

// Calculate introspection depth
double calc_introspection(
    const std::vector<Memory>& episodic_mem,
    const std::map<int, double>& temporal_awareness,
    int current_gen
);

// Calculate reflection capability
double calc_reflection(
    const std::map<int, double>& temporal_states,
    const std::vector<double>& valence_history
);

// Calculate prediction error
double calc_prediction_error(
    const std::vector<std::string>& input_words,
    std::map<std::string, Token>& tokens,
    std::map<std::string, Concept>& concepts
);

// Update consciousness state (call every cycle)
void update_consciousness_state(State& S);

// Hook into main system
void init_consciousness_module();

#endif
