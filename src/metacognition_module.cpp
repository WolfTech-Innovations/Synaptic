// metacognition_module.cpp - Implementation of metacognitive functions
#include "metacognition_module.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <sstream>

MetacognitiveState META;

void init_metacognition_module() {
    META.emotional_resonance = 0.0;
    META.cognitive_dissonance = 0.0;
    META.flow_state = 0.0;
    META.existential_weight = 0.0;
    META.temporal_binding = 0.0;
    META.self_model_coherence = 0.0;
    META.entropy_production = 0.0;
    META.in_dream_state = false;
    META.dream_narrative = "";
    META.idle_cycles = 0;
}

double calc_emotional_resonance(
    double valence,
    double surprise,
    const std::vector<std::string>& active_concepts,
    std::map<std::string, Concept>& concepts
) {
    // Valence magnitude
    double valence_magnitude = std::abs(valence);
    
    // Surprise factor
    double surprise_factor = surprise * 0.8;
    
    // Concept emotional loading
    double concept_emotion = 0.0;
    for (const auto& concept_name : active_concepts) {
        if (concepts.count(concept_name)) {
            concept_emotion += concepts[concept_name].value;
        }
    }
    concept_emotion = active_concepts.size() > 0 ? 
        concept_emotion / active_concepts.size() : 0.0;
    concept_emotion = std::min(1.0, concept_emotion / 5.0);
    
    META.emotional_resonance = valence_magnitude * 0.4 + 
                               surprise_factor * 0.3 + 
                               concept_emotion * 0.3;
    META.emotional_resonance = std::max(0.0, std::min(1.0, META.emotional_resonance));
    
    return META.emotional_resonance;
}

double calc_cognitive_dissonance(
    double prediction_error,
    const std::vector<double>& valence_history,
    double current_valence,
    const std::vector<std::string>& active_concepts,
    std::map<std::string, Concept>& concepts
) {
    // Prediction error contribution
    double pred_contrib = prediction_error * 1.2;
    
    // Valence consistency
    double valence_consistency = 0.0;
    if (valence_history.size() > 3) {
        double recent_avg = 0.0;
        int count = 0;
        for (int i = std::max(0, (int)valence_history.size() - 3); 
             i < valence_history.size(); i++) {
            recent_avg += valence_history[i];
            count++;
        }
        recent_avg = count > 0 ? recent_avg / count : 0.0;
        valence_consistency = std::abs(current_valence - recent_avg);
    }
    
    // Concept conflict
    double concept_conflict = 0.0;
    for (const auto& c1 : active_concepts) {
        for (const auto& c2 : active_concepts) {
            if (c1 != c2 && concepts.count(c1) && concepts.count(c2)) {
                concept_conflict += std::abs(concepts[c1].value - concepts[c2].value) * 0.1;
            }
        }
    }
    concept_conflict = std::min(1.0, concept_conflict);
    
    META.cognitive_dissonance = pred_contrib * 0.5 + 
                                valence_consistency * 0.3 + 
                                concept_conflict * 0.2;
    META.cognitive_dissonance = std::max(0.0, std::min(1.0, META.cognitive_dissonance));
    
    return META.cognitive_dissonance;
}

double calc_flow_state(
    double working_memory,
    double attention,
    double curiosity,
    double introspection,
    double temporal_binding,
    double self_model_coherence
) {
    // Task engagement
    double task_engagement = working_memory * attention;
    
    // Skill-challenge balance
    double skill_challenge_balance = 1.0 - std::abs(curiosity - introspection);
    
    // Temporal coherence
    double temporal_coherence = temporal_binding;
    
    // Low self-consciousness (inverse of self-model interference)
    double low_self_consciousness = std::max(0.0, 1.0 - self_model_coherence * 0.5);
    
    META.flow_state = task_engagement * 0.35 + 
                      skill_challenge_balance * 0.25 + 
                      temporal_coherence * 0.25 + 
                      low_self_consciousness * 0.15;
    META.flow_state = std::max(0.0, std::min(1.0, META.flow_state));
    
    return META.flow_state;
}

double calc_existential_weight(
    double self_model_coherence,
    double introspection,
    const std::vector<Memory>& episodic_memory,
    std::map<std::string, Concept>& concepts
) {
    // Self-model depth
    double self_model_depth = self_model_coherence * introspection;
    
    // Temporal awareness (autobiographical memory)
    double temporal_awareness = std::min(1.0, episodic_memory.size() / 100.0);
    
    // Self-referential concepts
    double self_referential = 0.0;
    for (auto& c : concepts) {
        if (c.first.find("self") != std::string::npos || 
            c.first.find("aware") != std::string::npos ||
            c.first.find("I") != std::string::npos ||
            c.first.find("me") != std::string::npos) {
            self_referential += c.second.value;
        }
    }
    self_referential = std::min(1.0, self_referential);
    
    // Existential contemplation
    double existential_contemplation = self_referential * 0.5;
    
    META.existential_weight = self_model_depth * 0.4 + 
                              temporal_awareness * 0.3 + 
                              existential_contemplation * 0.3;
    META.existential_weight = std::max(0.0, std::min(1.0, META.existential_weight));
    
    return META.existential_weight;
}

double calc_temporal_binding(
    const std::map<int, double>& temporal_awareness,
    double working_memory
) {
    if (temporal_awareness.size() < 3) return 0.0;
    
    // Calculate temporal coherence
    double temporal_coherence = 0.0;
    int count = 0;
    
    auto it = temporal_awareness.rbegin();
    for (int i = 0; i < 5 && it != temporal_awareness.rend(); ++it, ++i) {
        auto next = it;
        ++next;
        if (next != temporal_awareness.rend()) {
            double delta = std::abs(it->second - next->second);
            temporal_coherence += std::max(0.0, 1.0 - delta / 1000.0);
            count++;
        }
    }
    
    temporal_coherence = count > 0 ? temporal_coherence / count : 0.0;
    
    // Working memory contribution
    double wm_contribution = working_memory * 0.6;
    
    META.temporal_binding = temporal_coherence * 0.6 + wm_contribution * 0.4;
    META.temporal_binding = std::max(0.0, std::min(1.0, META.temporal_binding));
    
    return META.temporal_binding;
}

double calc_self_model_coherence(
    std::map<std::string, Concept>& concepts,
    double introspection,
    double reflection,
    const std::vector<Memory>& episodic_memory,
    const std::vector<double>& valence_history
) {
    // Self-concept strength
    double self_concept_strength = 0.0;
    if (concepts.count("self")) {
        self_concept_strength = concepts["self"].value;
    }
    
    // Introspection-reflection integration
    double intro_refl_integration = introspection * reflection;
    
    // Autobiographical memory integration
    double autobio_memory = std::min(1.0, episodic_memory.size() / 120.0);
    
    // Identity stability (valence variance)
    double identity_stability = 0.0;
    if (valence_history.size() > 10) {
        double variance = 0.0;
        double mean = 0.0;
        int window = std::min(10, (int)valence_history.size());
        
        for (int i = valence_history.size() - window; i < valence_history.size(); i++) {
            mean += valence_history[i];
        }
        mean /= window;
        
        for (int i = valence_history.size() - window; i < valence_history.size(); i++) {
            variance += (valence_history[i] - mean) * (valence_history[i] - mean);
        }
        variance /= window;
        
        identity_stability = std::max(0.0, 1.0 - std::sqrt(variance));
    }
    
    META.self_model_coherence = self_concept_strength * 0.3 + 
                                intro_refl_integration * 0.3 + 
                                autobio_memory * 0.2 + 
                                identity_stability * 0.2;
    META.self_model_coherence = std::max(0.0, std::min(1.0, META.self_model_coherence));
    
    return META.self_model_coherence;
}

double calc_entropy_production(
    const std::map<std::string, double>& state_dict,
    std::map<std::string, Concept>& concepts
) {
    // State diversity entropy
    double state_entropy = 0.0;
    std::map<int, int> state_histogram;
    
    for (auto& p : state_dict) {
        if (p.first.find("w") == 0) {  // Weight states
            int bucket = (int)p.second + 2;
            state_histogram[bucket]++;
        }
    }
    
    int total = 0;
    for (auto& h : state_histogram) total += h.second;
    
    if (total > 0) {
        for (auto& h : state_histogram) {
            double prob = (double)h.second / total;
            if (prob > 0) {
                state_entropy -= prob * std::log2(prob);
            }
        }
    }
    
    // Concept activation entropy
    double concept_entropy = 0.0;
    double total_activation = 0.0;
    for (auto& c : concepts) {
        total_activation += c.second.value;
    }
    
    if (total_activation > 0.01) {
        for (auto& c : concepts) {
            double prob = c.second.value / total_activation;
            if (prob > 0.01) {
                concept_entropy -= prob * std::log2(prob + 0.001);
            }
        }
    }
    concept_entropy = std::min(1.0, concept_entropy / 5.0);
    
    META.entropy_production = (state_entropy / 2.0) * 0.5 + concept_entropy * 0.5;
    META.entropy_production = std::max(0.0, std::min(1.0, META.entropy_production));
    
    return META.entropy_production;
}

void consolidate_memory(
    std::vector<Memory>& episodic_memory,
    std::map<std::string, Token>& tokens,
    const std::vector<std::string>& active_buffer
) {
    if (episodic_memory.size() < 5) return;
    
    // Find high-salience memories
    std::vector<Memory*> candidates;
    for (auto& m : episodic_memory) {
        if (std::abs(m.valence) > 0.5) {
            candidates.push_back(&m);
        }
    }
    
    if (candidates.empty()) return;
    
    // Select random high-salience memory
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, candidates.size() - 1);
    Memory* selected = candidates[dis(gen)];
    
    // Strengthen selected memory
    selected->valence *= 1.15;
    
    // Strengthen related tokens in active buffer
    for (const auto& word : active_buffer) {
        if (tokens.count(word)) {
            tokens[word].coherence_score += 0.03;
        }
    }
}

void dream_state_process(
    std::vector<Memory>& episodic_memory,
    bool trigger
) {
    if (trigger && !META.in_dream_state) {
        META.in_dream_state = true;
        META.idle_cycles = 0;
    }
    
    if (!META.in_dream_state || META.dream_queue.empty()) return;
    
    DreamFragment& fragment = META.dream_queue.front();
    
    // Process dream fragment
    std::vector<std::string> dream_tokens;
    for (auto& m : episodic_memory) {
        if (std::abs(m.valence) > 0.6) {
            std::istringstream iss(m.content);
            std::string word;
            while (iss >> word && dream_tokens.size() < 15) {
                dream_tokens.push_back(word);
            }
        }
    }
    
    // Shuffle and create dream narrative
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(dream_tokens.begin(), dream_tokens.end(), g);
    
    std::string narrative;
    for (int i = 0; i < std::min(8, (int)dream_tokens.size()); i++) {
        narrative += dream_tokens[i] + " ";
    }
    
    META.dream_narrative = narrative;
    
    // Update fragment
    fragment.replay_count++;
    fragment.vividness *= 0.9;
    
    // End dream if processed enough or vividness too low
    if (fragment.replay_count > 3 || fragment.vividness < 0.3) {
        META.dream_queue.pop();
        if (META.dream_queue.empty()) {
            META.in_dream_state = false;
        }
    }
    
    // Strengthen consolidated memories
    for (auto& m : episodic_memory) {
        if (std::abs(m.valence) > 0.5) {
            m.valence *= 1.05;
        }
    }
}

void update_metacognition(State& S) {
    // Placeholder - actual implementation needs full State structure
    // This would be called from main loop with full state
    META.idle_cycles++;
}
