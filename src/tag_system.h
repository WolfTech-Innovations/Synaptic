#ifndef TAG_SYSTEM_H
#define TAG_SYSTEM_H

#include <string>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
using namespace std;

// ============================================================================
// MINIMAL TAG SYSTEM - HEADER ONLY - DROP INTO EXISTING CODE
// ============================================================================

struct Tag {
    string name;
    set<string> parent_tags;
    set<string> child_tags;
    double activation;
    int usage_count;
    
    Tag() : activation(0.0), usage_count(0) {}
};

class TagSystem {
public:
    map<string, Tag> tags;
    map<string, set<string>> word_tags;  // word -> tags
    
    TagSystem() {
        initTags();
    }
    
    void initTags() {
        // Domains
        createTag("domain");
        createTag("math", {"domain"});
        createTag("philosophy", {"domain"});
        createTag("conversation", {"domain"});
        createTag("consciousness_theory", {"domain"});
        
        // Math
        createTag("arithmetic", {"math"});
        createTag("number", {"math"});
        createTag("operation", {"arithmetic"});
        createTag("addition", {"operation"});
        createTag("subtraction", {"operation"});
        createTag("multiplication", {"operation"});
        createTag("division", {"operation"});
        
        // Philosophy/Consciousness
        createTag("phenomenology", {"philosophy"});
        createTag("epistemology", {"philosophy"});
        createTag("mind", {"philosophy", "consciousness_theory"});
        createTag("qualia", {"consciousness_theory", "phenomenology"});
        createTag("awareness", {"consciousness_theory"});
        createTag("integration", {"consciousness_theory"});
        
        // Conversation
        createTag("greeting", {"conversation"});
        createTag("question", {"conversation"});
        createTag("statement", {"conversation"});
        
        // Word tagging
        tagWords();
    }
    
    void createTag(const string& name, const set<string>& parents = {}) {
        if(!tags.count(name)) {
            tags[name] = Tag();
            tags[name].name = name;
        }
        
        for(const string& parent : parents) {
            if(!tags.count(parent)) {
                tags[parent] = Tag();
                tags[parent].name = parent;
            }
            tags[name].parent_tags.insert(parent);
            tags[parent].child_tags.insert(name);
        }
    }
    
    void tagWords() {
        // Numbers
        for(auto w : {"zero","one","two","three","four","five","six","seven","eight","nine","ten"}) {
            word_tags[w] = {"number", "arithmetic"};
        }
        
        // Operators
        word_tags["plus"] = {"addition", "operation"};
        word_tags["add"] = {"addition", "operation"};
        word_tags["minus"] = {"subtraction", "operation"};
        word_tags["subtract"] = {"subtraction", "operation"};
        word_tags["times"] = {"multiplication", "operation"};
        word_tags["multiply"] = {"multiplication", "operation"};
        word_tags["divided"] = {"division", "operation"};
        word_tags["divide"] = {"division", "operation"};
        word_tags["equals"] = {"arithmetic"};
        
        // Consciousness
        word_tags["consciousness"] = {"consciousness_theory", "phenomenology", "mind"};
        word_tags["aware"] = {"awareness", "consciousness_theory"};
        word_tags["awareness"] = {"awareness", "consciousness_theory"};
        word_tags["qualia"] = {"qualia", "consciousness_theory"};
        word_tags["experience"] = {"phenomenology", "consciousness_theory"};
        word_tags["phi"] = {"consciousness_theory"};
        word_tags["integration"] = {"integration", "consciousness_theory"};
        word_tags["integrated"] = {"integration", "consciousness_theory"};
        
        // Philosophy
        word_tags["mind"] = {"mind", "philosophy"};
        word_tags["think"] = {"mind", "epistemology"};
        word_tags["thought"] = {"mind", "epistemology"};
        word_tags["know"] = {"epistemology"};
        word_tags["knowledge"] = {"epistemology"};
        word_tags["understand"] = {"epistemology"};
        word_tags["understanding"] = {"epistemology"};
        
        // Conversation
        word_tags["hello"] = {"greeting"};
        word_tags["hi"] = {"greeting"};
        word_tags["hey"] = {"greeting"};
        word_tags["what"] = {"question"};
        word_tags["why"] = {"question"};
        word_tags["how"] = {"question"};
        word_tags["who"] = {"question"};
        
        // Self
        word_tags["i"] = {"statement"};
        word_tags["me"] = {"statement"};
        word_tags["my"] = {"statement"};
        
        // Learning
        word_tags["learn"] = {"epistemology"};
        word_tags["learning"] = {"epistemology"};
    }
    
    set<string> getTags(const string& word) {
        if(word_tags.count(word)) {
            return word_tags[word];
        }
        return {"conversation"};  // default
    }
    
    set<string> getAllTags(const set<string>& direct_tags) {
        set<string> all;
        set<string> to_process = direct_tags;
        set<string> processed;
        
        while(!to_process.empty()) {
            string current = *to_process.begin();
            to_process.erase(to_process.begin());
            
            if(processed.count(current)) continue;
            processed.insert(current);
            all.insert(current);
            
            if(tags.count(current)) {
                for(const string& parent : tags[current].parent_tags) {
                    to_process.insert(parent);
                }
            }
        }
        
        return all;
    }
    
    void activate(const set<string>& active_tags) {
        // Decay all
        for(auto& [name, tag] : tags) {
            tag.activation *= 0.5;
        }
        
        // Activate direct
        for(const string& tag_name : active_tags) {
            if(tags.count(tag_name)) {
                tags[tag_name].activation += 1.0;
            }
        }
        
        // Spread to parents
        for(const string& tag_name : active_tags) {
            if(!tags.count(tag_name)) continue;
            
            for(const string& parent : tags[tag_name].parent_tags) {
                tags[parent].activation += 0.7;
            }
        }
    }
    
    string getActiveDomain() {
        string best = "conversation";
        double best_act = 0.0;
        
        if(!tags.count("domain")) return best;
        
        for(const string& domain : tags["domain"].child_tags) {
            if(tags.count(domain) && tags[domain].activation > best_act) {
                best_act = tags[domain].activation;
                best = domain;
            }
        }
        
        return best;
    }
    
    double getOverlap(const set<string>& tags1, const set<string>& tags2) {
        if(tags1.empty() || tags2.empty()) return 0.0;
        
        set<string> full1 = getAllTags(tags1);
        set<string> full2 = getAllTags(tags2);
        
        double overlap = 0.0;
        for(const string& t : full1) {
            if(full2.count(t)) {
                overlap += 1.0;
                if(tags.count(t) && tags[t].parent_tags.count("domain")) {
                    overlap += 0.5;  // Domain match bonus
                }
            }
        }
        
        double max_size = max(full1.size(), full2.size());
        return max_size > 0 ? overlap / max_size : 0.0;
    }
};

#endif // TAG_SYSTEM_H
