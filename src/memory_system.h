#ifndef MEMORY_SYSTEM_H
#define MEMORY_SYSTEM_H

#include <string>
#include <vector>
#include <map>

struct SemanticMemory {
    std::string concept_id; // Renamed from concept
    std::string definition;
    std::vector<std::string> examples;
    double confidence;
};

class MemorySystem {
public:
    static void initialize();
    
    // Renamed 'concept' parameter to 'concept_id'
    static void store_semantic_memory(const std::string& concept_id, 
                                     const std::string& definition, 
                                     const std::vector<std::string>& examples);
    
    static SemanticMemory retrieve_semantic(const std::string& concept_id);

private:
    static std::map<std::string, SemanticMemory> semantic_store_;
};

#endif