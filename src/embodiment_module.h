// embodiment_module.h - Grounding and embodiment functions
#ifndef EMBODIMENT_MODULE_H
#define EMBODIMENT_MODULE_H

#include <string>
#include <vector>
#include <map>

// Forward declarations
struct Concept;
struct Token;

// Qualia binding structure (subjective experience)
struct Qualia {
    std::string concept_ptr;
    double intensity;
    std::string sensory_state;
    double binding_strength;
};

// Embodiment state
struct EmbodimentState {
    std::map<std::string, std::map<std::string, double>> sensory_associations;
    std::map<std::string, std::map<std::string, double>> internal_associations;
    std::vector<Qualia> qualia_bindings;
    std::map<std::string, std::vector<double>> concept_state_mappings;
    double embodiment_index;
    double situatedness;
    double grounding_depth;
};

// Global embodiment state
extern EmbodimentState EMBOD;

// Ground a concept to sensory and internal states
void ground_concept_to_state(
    const std::string& concept_name,
    double sensory_input,
    double internal_state,
    std::map<std::string, Concept>& concepts
);

// Bind qualia (subjective experience) to concepts
void bind_qualia(
    const std::string& concept_ptr,
    double intensity,
    double sensory_env,
    double emotional_valence,
    double integrated_info,
    double global_workspace
);

// Calculate embodiment index (how grounded the system is)
double calc_embodiment_index();

// Calculate situatedness (context awareness)
double calc_situatedness(
    double env_output,
    const std::map<std::string, Token>& tokens,
    double temporal_binding,
    double attention
);

// Calculate grounding depth (multimodal integration)
double calc_grounding_depth();

// Deep concept integration (connects abstract to grounded)
void deep_concept_integration(
    std::vector<std::string>& active_concepts,
    std::map<std::string, Concept>& concepts,
    double sensory_env,
    double hdt_val
);

// Hook into main system
void init_embodiment_module();

#endif
