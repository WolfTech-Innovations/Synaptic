#ifndef ENHANCED_REASONING_H
#define ENHANCED_REASONING_H

#include <string>
#include <vector>
#include <map>
#include "struct.h"

struct ConceptNode {
    std::string id;
    std::string label;
    double valence;
    double grounding_strength;
    double activation_potential;
    std::vector<std::string> related_concepts;
    std::map<std::string, double> valence_associations;
};

struct ReasoningContext {
    std::vector<ConceptNode> active_concepts;
    std::map<std::string, double> valence_field;
    double ambient_valence;
    int reasoning_depth;
};

class EnhancedReasoning {
public:
    static void initialize_concept_graph();
    static std::string reason_about_topic(const std::string& topic, double current_valence);
    static std::string generate_coherent_thought();
    static std::string infer_from_context(const std::vector<std::string>& context_tokens);
    
    // RENAMED: 'concept' parameter changed to 'concept_id' to avoid C++20 keyword conflict
    static std::vector<std::string> generate_causal_chain(const std::string& concept_id, int depth);
    static double calculate_concept_relevance(const std::string& concept_id, double current_valence);
    static void update_concept_valence(const std::string& concept_id, double valence_delta);
    static void activate_concept_network(const std::string& root_concept);

private:
    static void build_default_concepts();
    static void propagate_valence_field();
    
    static std::map<std::string, ConceptNode> concept_graph_;
    static ReasoningContext current_context_;
};

#endif // ENHANCED_REASONING_H