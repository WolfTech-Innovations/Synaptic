// metacognition_module.h - Self-modeling and metacognitive functions
#ifndef METACOGNITION_MODULE_H
#define METACOGNITION_MODULE_H

#include <string>
#include <vector>
#include <map>
#include <queue>
// metacognition_module.h
#include "state.h"  
// Dream state for memory consolidation
struct DreamFragment {
    std::vector<std::string> fragments;
    double vividness;
    int replay_count;
};

// Metacognitive state
struct MetacognitiveState {
    double emotional_resonance;
    double cognitive_dissonance;
    double flow_state;
    double existential_weight;
    double temporal_binding;
    double self_model_coherence;
    double entropy_production;
    std::queue<DreamFragment> dream_queue;
    bool in_dream_state;
    std::string dream_narrative;
    int idle_cycles;
};

// Global metacognitive state
extern MetacognitiveState META;

// Calculate emotional resonance
double calc_emotional_resonance(
    double valence,
    double surprise,
    const std::vector<std::string>& active_concepts,
    std::map<std::string, Concept>& concepts
);

// Calculate cognitive dissonance
double calc_cognitive_dissonance(
    double prediction_error,
    const std::vector<double>& valence_history,
    double current_valence,
    const std::vector<std::string>& active_concepts,
    std::map<std::string, Concept>& concepts
);

// Calculate flow state
double calc_flow_state(
    double working_memory,
    double attention,
    double curiosity,
    double introspection,
    double temporal_binding,
    double self_model_coherence
);

// Calculate existential weight (self-awareness depth)
double calc_existential_weight(
    double self_model_coherence,
    double introspection,
    const std::vector<Memory>& episodic_memory,
    std::map<std::string, Concept>& concepts
);

// Calculate temporal binding (continuity of experience)
double calc_temporal_binding(
    const std::map<int, double>& temporal_awareness,
    double working_memory
);

// Calculate self-model coherence
double calc_self_model_coherence(
    std::map<std::string, Concept>& concepts,
    double introspection,
    double reflection,
    const std::vector<Memory>& episodic_memory,
    const std::vector<double>& valence_history
);

// Calculate entropy production (system complexity)
double calc_entropy_production(
    const std::map<std::string, double>& state_dict,
    std::map<std::string, Concept>& concepts
);

// Consolidate memories during idle time
void consolidate_memory(
    std::vector<Memory>& episodic_memory,
    std::map<std::string, Token>& tokens,
    const std::vector<std::string>& active_buffer
);

// Dream state processing
void dream_state_process(
    std::vector<Memory>& episodic_memory,
    bool trigger
);

// Update metacognitive metrics
void update_metacognition(State& S);

// Hook into main system
void init_metacognition_module();

#endif
