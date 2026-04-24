#include "memory_system.h"
#include <random>
#include <algorithm>

// Use std:: prefixes or add these using declarations
using std::string;
using std::vector;
using std::pair;
using std::map;

std::map<std::string, SemanticMemory> MemorySystem::semantic_store_;

// Fix external declarations with correct types
extern std::random_device rd;
extern std::mt19937 rng;

void MemorySystem::initialize() {
    // Renamed local type for clarity and added std::
    vector<pair<string, pair<string, vector<string>>>> base_concepts = {
        {"consciousness", {"the state of being aware", {"qualia", "subjective"}}},
        {"logic", {"systematic reasoning", {"deduction", "inference"}}}
    };
    
    // RENAMED structured binding: [cpt_key, def_and_examples]
    for (const auto& [cpt_key, def_and_examples] : base_concepts) {
        store_semantic_memory(cpt_key, def_and_examples.first, def_and_examples.second);
    }
}

void MemorySystem::store_semantic_memory(const string& concept_id, 
                                        const string& definition, 
                                        const vector<string>& examples) {
    SemanticMemory mem;
    mem.concept_id = concept_id;
    mem.definition = definition;
    mem.examples = examples;
    mem.confidence = 1.0;
    semantic_store_[concept_id] = mem;
}

SemanticMemory MemorySystem::retrieve_semantic(const string& concept_id) {
    if (semantic_store_.contains(concept_id)) {
        return semantic_store_[concept_id];
    }
    return SemanticMemory{concept_id, "unknown", {}, 0.0};
}