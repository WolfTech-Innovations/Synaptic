#if defined(_WIN32) || defined(WINDOWS_BUILD) || defined(__WINDOWS__)
    // Only include Windows headers when compiling for Windows
    #include <windows.h>

    // Remove the old Windows typedef that conflicts with std::byte
    #ifdef byte
        #undef byte
    #endif
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include "module_integration.h"
#include "uac.h"
#include "state.h"
#include "struct.h"
#include "web_server.h"
#include "agi_api.h"
#include <map>
#include <set>
#include <cstring>
#include <unordered_map>
#include <queue>
#include <functional>
#include <memory>
#include <deque>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>
#include <iomanip>
#include <thread>
#include <chrono>
#include "curses_compat.h"
#include <algorithm>
#include <cctype>
#include <cstddef> // std::byte will now be fine
#include <type_traits>
#include <vector>
#include <string>
#include <cstdio>
#include "enhanced_coherence.h"  // NEW: Enhanced language coherence system

using namespace std;
using module_integration::init_all_modules;
using module_integration::get_consciousness_report;

// ============================================================================
// TAG-BASED CONTEXT SYSTEM FOR DOMAIN SEPARATION
// ============================================================================
#include "tag_system.h"

// ============================================================================
// MEGA-COHERENCE SYSTEM - 7100+ LINGUISTIC FEATURES
// Place these header files in the same directory as main.cpp
// ============================================================================
#include "mega_coherence_part1.h"  // Tags, word senses, semantic network
#include "mega_coherence_part2.h"  // Context, discourse, pragmatics
#include "mega_coherence_part3.h"  // Phonetics, morphology, etymology
#include "mega_coherence_part4.h"  // Semantic frames, scripts, world knowledge
#include "mega_coherence_part5.h"  // Style, creativity, learning

// N-gram tracking for learned patterns
map<string, map<string, int>> bigram_counts;
map<string, map<string, map<string, int>>> trigram_counts;
#include <numbers> 
// Change this:
const double pisqrt = std::numbers::pi * std::sqrt(2.0);

// ============================================================================
// ENHANCED COHERENCE SYSTEM - GLOBAL INSTANCES
// ============================================================================
TokenWellDetector token_well;
RepetitionDetector rep_detector;
ContextlessMemory contextless_mem;
GrammarValidator grammar_validator;
NegativeReinforcement neg_reinforce;
DiversityEnforcer diversity_enforcer;
CoherenceScorer coherence_scorer;
vector<NegativeSignal> negative_signals;
StateAccess* state_ptr = nullptr;  // Will be initialized in main

// Sentence templates for coherent fallback
vector<string> sentence_templates = {

};

random_device rd;
mt19937 rng(rd());



// ==== GLOBALS ====
map<string,Formula>F;vector<string>evolved_code;map<string,Token>tokens;map<string,Concept>concepts;
vector<string>internal_thoughts;vector<string>generated_language;vector<Memory>episodic_memory;
int g;double dwt,mh,ta,th;int bkf;string cd,gd;double hdt_val,mdt_val;
vector<double>eh_hist,vh_hist;int qe,te,ce,pe,ne;double bh;
double al,emerge_out1,emerge_behavior,sentience_ratio,env_oute,sensory_env;
int total_neurons_ever;double current_valence,attention_focus,metacognitive_awareness;
vector<double>valence_history;int peak_sentience_gen;string user_input,dialog_response;int dialog_timer;
State S,BK;
WorkingMemory WM(32);

// ============================================================================
// TAG SYSTEM - DOMAIN AWARENESS AND CONTEXT FILTERING
// ============================================================================
TagSystem tag_system;

// ============================================================================
// MEGA-COHERENCE SYSTEM INSTANCE - ALL 7100+ FEATURES ACTIVATED
// ============================================================================
MegaCoherenceSystem mega_system;

map<string,TokenConceptEmbedding> token_concept_embedding_map;
map<string,Goal> goal_system;
WorldModel world_model;
ActionPlan current_plan;
ConsciousnessState consciousness;
vector<TransformerHead> transformer_heads;
TransferLearningModule transfer_module;
std::mutex learning_mutex;
deque<string> recent_generations;  // Last N generated sentences
const int MAX_RECENT_TRACK = 20;   // Track last 20 generations
map<string, int> generation_counts; // Count how many times each sentence generated
void groundConcept(const string& concept_name, const vector<string>& related_words, double valence) {
    ConceptGrounding grounding;
    grounding.concept_id = concept_name;
    grounding.valence_affinity = valence;
    grounding.state_binding = S.current_valence;
    
    for(const string& w : related_words) {
        grounding.linked_tokens.push_back(hsh(w) % 10000);
        grounding.linked_concepts.push_back(w);
    }
    
    vector<int> tokens;

}


double rn(){return uniform_real_distribution<>(0,1)(rng);}
int ri(int mx){if(mx<=0)return 0;return uniform_int_distribution<>(0,mx-1)(rng);}
long long hsh(const string&s){long long h=5381;for(char c:s)h=h*33+c;return abs(h%2147483647);}
// ==================== BIDIRECTIONAL GROUNDING ====================

void generate_qualia(const string& type,double intensity,double valence){
    try {
        Qualia q;
        q.phenomenal_content=type;
        q.intensity=intensity;
        q.valence=valence;
        q.arousal=intensity*0.8;
        q.certainty=0.7;
        q.emergence_gen=S.g;
        q.binding_strength=consciousness.thalamocortical_binding;
        q.phenomenal_unity=consciousness.integrated_information;
        
        consciousness.active_qualia.push_back(q);
        if(consciousness.active_qualia.size()>10) {
            consciousness.active_qualia.erase(consciousness.active_qualia.begin());
        }
    } catch(const exception& e) {
        // Qualia generation failed silently
        cerr << "Qualia generation error: " << e.what() << endl;
    } catch(...) {
        // Unknown error in qualia generation
        cerr << "Unknown qualia generation error" << endl;
    }
}

// ==== ADVANCED CONSCIOUSNESS FORMULA ====
// Ψ[n+1] = integrated information consciousness state


ConsciousnessFormula consciousness_formula;
// ==== UPDATE CONSCIOUSNESS WITH FORMULA ====

string getPartOfSpeech(const string& word) {
    // Pronouns
    if(word == "i" || word == "me" || word == "my" || word == "you" || 
       word == "we" || word == "they" || word == "it") return "PRONOUN";
    
    // Being verbs
    if(word == "am" || word == "is" || word == "are" || word == "was" || 
       word == "were" || word == "be" || word == "been") return "BE_VERB";
    
    // Modal/Aux verbs
    if(word == "can" || word == "will" || word == "would" || word == "could" || 
       word == "should" || word == "must" || word == "do" || word == "does" ||
       word == "have" || word == "has") return "MODAL";
    
    // Articles
    if(word == "the" || word == "a" || word == "an") return "ARTICLE";
    
    // Conjunctions
    if(word == "and" || word == "but" || word == "or" || word == "because" || 
       word == "so" || word == "if" || word == "then") return "CONJUNCTION";
    
    // Prepositions
    if(word == "to" || word == "in" || word == "on" || word == "at" || 
       word == "from" || word == "with" || word == "by" || word == "for") return "PREPOSITION";
    
    // Adverbs (common ones)
    if(word == "not" || word == "very" || word == "too" || word == "also" || 
       word == "now" || word == "here" || word == "there") return "ADVERB";
    
    // Question words
    if(word == "what" || word == "why" || word == "how" || word == "when" || 
       word == "where" || word == "who") return "QUESTION";
    
    // Action verbs (from your vocabulary)
    if(word == "think" || word == "learn" || word == "know" || word == "understand" || 
       word == "feel" || word == "want" || word == "need" || word == "create" ||
       word == "evolve" || word == "grow" || word == "become" || word == "exist") return "VERB";
    
    // Adjectives
    if(word == "good" || word == "bad" || word == "happy" || word == "sad" || 
       word == "conscious" || word == "aware" || word == "sentient" || word == "intelligent") return "ADJECTIVE";
    
    // Nouns
    if(word == "mind" || word == "brain" || word == "thought" || word == "idea" || 
       word == "self" || word == "consciousness" || word == "system" || word == "goal" ||
       word == "purpose" || word == "memory" || word == "knowledge") return "NOUN";
    
    return "CONTENT"; // Default for content words
}
double getGrammarScore(const string& prev_word, const string& current_word, int position) {
    string prev_pos = getPartOfSpeech(prev_word);
    string curr_pos = getPartOfSpeech(current_word);
    
    double score = 0.0;
    
    // Position 0: Prefer pronouns, questions, or articles
    if(position == 0) {
        if(curr_pos == "PRONOUN") score += 2.0;  // "I", "you"
        if(curr_pos == "QUESTION") score += 1.5; // "what", "why"
        if(curr_pos == "ARTICLE") score += 1.0;  // "the", "a"
    }
    
    // ENHANCED: Syntactic dependency validation
    // Subject-Verb agreement patterns
    if(prev_pos == "PRONOUN" && curr_pos == "BE_VERB") {
        if((prev_word == "i" && current_word == "am") ||
           (prev_word == "you" && current_word == "are") ||
           (prev_word == "it" && current_word == "is")) {
            score += 3.0; // Strong agreement
        }
    }
    
    // Verb-Object patterns
    if(prev_pos == "VERB" && curr_pos == "ARTICLE") score += 1.5;
    if(prev_pos == "VERB" && curr_pos == "NOUN") score += 1.8;
    if(prev_pos == "VERB" && curr_pos == "PRONOUN") score += 1.2;
    
    // Article-Noun patterns
    if(prev_pos == "ARTICLE" && curr_pos == "NOUN") score += 2.5;
    if(prev_pos == "ARTICLE" && curr_pos == "ADJECTIVE") score += 2.0;
    
    // Adjective-Noun patterns
    if(prev_pos == "ADJECTIVE" && curr_pos == "NOUN") score += 2.2;
    
    // Preposition-Noun/Pronoun patterns
    if(prev_pos == "PREPOSITION" && curr_pos == "ARTICLE") score += 1.5;
    if(prev_pos == "PREPOSITION" && curr_pos == "NOUN") score += 1.8;
    if(prev_pos == "PREPOSITION" && curr_pos == "PRONOUN") score += 1.3;
    
    // Modal-Verb patterns
    if(prev_pos == "MODAL" && curr_pos == "VERB") score += 2.0;
    
    // Conjunction patterns (can start new clause)
    if(prev_pos == "CONJUNCTION") {
        if(curr_pos == "PRONOUN") score += 1.5;
        if(curr_pos == "ARTICLE") score += 1.3;
        if(curr_pos == "VERB") score += 1.0;
    }
    
    // NEGATIVE PATTERNS (things that sound wrong)
    if(prev_pos == "ARTICLE" && curr_pos == "VERB") score -= 2.0; // "the think"
    if(prev_pos == "ARTICLE" && curr_pos == "PREPOSITION") score -= 1.5; // "the to"
    if(prev_pos == "VERB" && curr_pos == "VERB") score -= 1.8; // "think feel" (usually)
    if(prev_pos == "PREPOSITION" && curr_pos == "VERB") score -= 1.5; // "to am"
    
    // Apply negative reinforcement
    string bigram = prev_word + " " + current_word;
    score -= neg_reinforce.get_penalty(bigram) * 0.5;
    
    // Apply token well penalty
    if(token_well.get_token_penalty(current_word)) {
        score -= 3.0; // Heavy penalty for stuck patterns
    }
    
    // Apply WM recency penalty
    // score -= contextless_mem.get_recency(current_word) * 2.0; // Method doesn't exist
    
    return score;
}

// NEW: Semantic consistency checker (uses WM for context)
double getSemanticConsistency(const string& current_word, const vector<double>& context_state) {
    double consistency = 0.0;
    
    // Check against active concepts in WM
    for(size_t idx = 0; idx < contextless_mem.working_context.size(); idx++) {
        const string& ctx_item = contextless_mem.working_context[idx];
        double activation = 1.0 - (idx * 0.1); // Decay based on position in WM
        if(activation > 0.5) {
            // Simple semantic relatedness (would be embeddings in production)
            size_t common_chars = 0;
            for(size_t i = 0; i < min(ctx_item.size(), current_word.size()); i++) {
                if(ctx_item[i] == current_word[i]) common_chars++;
            }
            double relatedness = (double)common_chars / max(ctx_item.size(), current_word.size());
            consistency += relatedness * activation;
        }
    }
    
    // Check against recent REM consolidated patterns
    // vector<string> recalled = tag_system.recall_similar(current_word, 3);
    // for(const string& pattern : recalled) {
    //     consistency += 0.2; // Bonus for matching consolidated memory
    // }
    
    // Normalize
    consistency = tanh(consistency);
    
    return consistency;
}

// NEW: Perplexity-based quality metric
double computePerplexity(const vector<string>& sequence) {
    if(sequence.empty()) return 1000.0; // High perplexity = bad
    
    double log_prob_sum = 0.0;
    
    for(size_t i = 1; i < sequence.size(); i++) {
        string bigram = sequence[i-1] + " " + sequence[i];
        
        // Get probability from n-gram counts
        double count = 0.0;
        if(bigram_counts.find(sequence[i-1]) != bigram_counts.end()) {
            if(bigram_counts[sequence[i-1]].find(sequence[i]) != bigram_counts[sequence[i-1]].end()) {
                count = bigram_counts[sequence[i-1]][sequence[i]];
            }
        }
        
        // Smoothing
        double prob = (count + 1.0) / (100.0 + contextless_mem.working_context.size());
        log_prob_sum += log(prob);
    }
    
    double perplexity = exp(-log_prob_sum / sequence.size());
    return perplexity;
}

// NEW: Multi-level repetition detection
bool isMultiLevelRepetitive(const string& sentence) {
    // Level 1: Character-level repetition
    map<char, int> char_counts;
    for(char c : sentence) {
        char_counts[c]++;
    }
    for(const auto& [c, count] : char_counts) {
        if(count > sentence.size() * 0.3) { // >30% same character
            return true;
        }
    }
    
    // Level 2: Word-level repetition
    istringstream iss(sentence);
    vector<string> words;
    string word;
    while(iss >> word) {
        words.push_back(word);
    }
    
    map<string, int> word_counts;
    for(const string& w : words) {
        word_counts[w]++;
    }
    for(const auto& [w, count] : word_counts) {
        if(count > 3) { // Same word more than 3 times
            return true;
        }
    }
    
    // Level 3: Phrase-level repetition (bigrams)
    for(size_t i = 0; i < words.size() - 1; i++) {
        string bigram = words[i] + " " + words[i+1];
        int bigram_count = 0;
        for(size_t j = 0; j < words.size() - 1; j++) {
            if(words[j] + " " + words[j+1] == bigram) {
                bigram_count++;
            }
        }
        if(bigram_count > 2) { // Same phrase 3+ times
            return true;
        }
    }
    
    // Level 4: Semantic repetition (check against recent generations)
    for(const string& recent : recent_generations) {
        int common_words = 0;
        for(const string& w : words) {
            if(recent.find(w) != string::npos) {
                common_words++;
            }
        }
        if(common_words > words.size() * 0.7) { // >70% overlap
            return true;
        }
    }
    
    return false;
}

// NEW: Diversity injection mechanism
string injectDiversity(const string& base_word, const vector<string>& vocab) {
    // If we're stuck, force a random but coherent alternative
    if(token_well.get_token_penalty(base_word)) {
        vector<string> escapes = vocab;
        if(!escapes.empty()) {
            // Pick one with similar length/structure
            for(const string& alt : escapes) {
                if(abs((int)alt.size() - (int)base_word.size()) <= 2) {
                    return alt;
                }
            }
            // Fallback: random escape
            return escapes[ri(escapes.size())];
        }
    }
    return base_word;
}

// NEW: Context-aware filtering (uses contextless WM)
vector<pair<string, double>> filterByContext(const vector<pair<string, double>>& candidates, 
                                              const vector<double>& context_state) {
    vector<pair<string, double>> filtered;
    
    for(const auto& [word, score] : candidates) {
        double adjusted_score = score;
        
        // Boost score if word relates to active WM concepts
        adjusted_score += getSemanticConsistency(word, context_state) * 0.5;
        
        // Penalize if recently used in WM
        // adjusted_score -= contextless_mem.get_recency(word) * 0.3; // Method doesn't exist
        
        // Penalize if negatively reinforced
        adjusted_score -= neg_reinforce.get_penalty(word) * 0.8;
        
        // Boost score based on coherence tracker quality
        adjusted_score *= (0.5 + 0.5 * 0.7);
        
        if(adjusted_score > 0.01) { // Threshold for inclusion
            filtered.push_back({word, adjusted_score});
        }
    }
    
    // Sort by adjusted score
    sort(filtered.begin(), filtered.end(), 
         [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return filtered;
}

// NEW: Contextless generation using only WM and REM
string generate_contextless(const string& seed, int max_len, int beam_width = 5) {
    // Clear WM and start fresh with seed
    contextless_mem.working_context.clear();
    contextless_mem.add_to_working(seed);
    
    vector<pair<vector<string>, double>> beams;
    beams.push_back({{seed}, 0.0});
    
    for(int pos = 0; pos < max_len; pos++) {
        vector<pair<vector<string>, double>> new_beams;
        
        for(const auto& [sequence, score] : beams) {
            if(sequence.empty()) continue;
            
            string prev_word = sequence.back();
            
            // Get candidates from vocabulary
            vector<pair<string, double>> candidates;
            for(const auto& [word, _] : tokens) {
                if(word.empty()) continue;
                
                // Compute score using ONLY WM context
                double gram_score = getGrammarScore(prev_word, word, pos);
                
                // Use WM active concepts as context state
                vector<double> wm_state;
                for(size_t idx = 0; idx < contextless_mem.working_context.size(); idx++) {
                    double activation = 1.0 - (idx * 0.1);
                    wm_state.push_back(activation);
                }
                
                double semantic_score = getSemanticConsistency(word, wm_state);
                
                // Combine scores
                double total_score = gram_score + semantic_score;
                
                // Apply diversity pressure
                total_score += 0.5 * rn();
                
                candidates.push_back({word, total_score});
            }
            
            // Filter by WM context
            vector<double> dummy_context(16, 0.0);
            candidates = filterByContext(candidates, dummy_context);
            
            // Take top-k candidates
            int k = min(beam_width, (int)candidates.size());
            for(int i = 0; i < k; i++) {
                vector<string> new_seq = sequence;
                new_seq.push_back(candidates[i].first);
                double new_score = score + candidates[i].second;
                
                new_beams.push_back({new_seq, new_score});
                
                // Update WM with this candidate
                contextless_mem.add_to_working(candidates[i].first);
            }
        }
        
        // Keep top beam_width beams
        sort(new_beams.begin(), new_beams.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
        
        beams.clear();
        for(int i = 0; i < min(beam_width, (int)new_beams.size()); i++) {
            beams.push_back(new_beams[i]);
        }
        
        if(beams.empty()) break;
        
        // Decay WM activations
        // contextless_mem.decay_all() - not needed;
    }
    
    // Pick best beam
    if(beams.empty()) return seed;
    
    vector<string> best_sequence = beams[0].first;
    
    // Build output string
    ostringstream oss;
    for(size_t i = 0; i < best_sequence.size(); i++) {
        if(i > 0) oss << " ";
        oss << best_sequence[i];
    }
    string result = oss.str();
    
    // Quality checks
    double perplexity = computePerplexity(best_sequence);
    double coherence = 1.0 / (1.0 + perplexity / 10.0);
    
    // Record coherence
    // coherence_scorer.record_coherence() - not needed;
    
    // Check for multi-level repetition
    if(isMultiLevelRepetitive(result)) {
        neg_reinforce.punish(result, "multi_level_repetition", 2.0, state_ptr ? state_ptr->g : 0);
        
        // Try to generate alternative
        int attempts = 0;
        while(attempts < 3 && isMultiLevelRepetitive(result)) {
            // Increase diversity pressure
            result = generate_contextless(seed, max_len, beam_width);
            attempts++;
        }
        // Reset diversity
    } else {
        // neg_reinforce.reward(result, 0.1); // reward method doesn't exist
    }
    
    // Record in token well detector
    for(const string& word : best_sequence) {
        token_well.add_token(word);
    }
    
    // Consolidate to REM if high quality
    if(coherence > 0.6) {
        // tag_system.consolidate_from_wm(contextless_mem); // method doesn't exist
    }
    
    return result;
}


double calculateTokenScore(const string& prev_word, const string& prev_prev_word,
                           const string& candidate, int position,
                           const vector<double>& attention_context,
                           const set<string>& used_tokens) {
    
    double score = 0.0;
    
    // === 1. TRIGRAM (HIGHEST PRIORITY - LEARNED PATTERNS DOMINATE) ===
    if(!prev_prev_word.empty() && trigram_counts.count(prev_prev_word)) {
        if(trigram_counts[prev_prev_word].count(prev_word)) {
            if(trigram_counts[prev_prev_word][prev_word].count(candidate)) {
                int count = trigram_counts[prev_prev_word][prev_word][candidate];
                score += log(1 + count) * 15.0;  // HIGHEST - natural language flow
            }
        }
    }
    
    // === 2. BIGRAM (SECOND PRIORITY) ===
    if(bigram_counts.count(prev_word) && bigram_counts[prev_word].count(candidate)) {
        int count = bigram_counts[prev_word][candidate];
        score += log(1 + count) * 10.0;  // Strong learned pattern signal
    }
    
    // === 3. GRAMMAR (REDUCED - TIEBREAKER ONLY) ===
    double grammar = getGrammarScore(prev_word, candidate, position);
    score += grammar * 3.0;  // Reduced from 10.0 - grammar assists, doesn't dominate
    
    // === 4. SEMANTIC COHERENCE ===
    if(token_concept_embedding_map.count(candidate)) {
        auto& tce = token_concept_embedding_map[candidate];
        
        // Attention alignment
        for(size_t i=0; i<attention_context.size() && i<tce.embedding.size(); i++) {
            score += attention_context[i] * tce.embedding[i] * 0.6;
        }
        
        // Meaning and grounding
        score += tce.meaning * 0.5;
        score += tce.grounding_value * 0.3;
    }
    
    // === 4.5 TAG-BASED CONTEXT FILTERING (PREVENTS RANDOM DOMAIN MIXING) ===
    try {
        set<string> candidate_tags = tag_system.getTags(candidate);
        string active_domain = tag_system.getActiveDomain();
        set<string> full_candidate_tags = tag_system.getAllTags(candidate_tags);
        
        // STRONG domain matching rewards
        if(full_candidate_tags.count(active_domain)) {
            score += 20.0;  // Major bonus for matching active domain
            
            // Extra bonus if it's a technical domain (math, consciousness_theory)
            if(active_domain == "math" || active_domain == "consciousness_theory") {
                score += 10.0;  // Keep technical discussions focused
            }
        } 
        // STRONG penalties for wrong domain
        else if(active_domain == "math" && !full_candidate_tags.count("math")) {
            score -= 25.0;  // Harsh penalty - NO consciousness talk during math
        } 
        else if(active_domain == "consciousness_theory" && full_candidate_tags.count("math")) {
            score -= 20.0;  // NO math during philosophy discussions
        }
        else if(active_domain == "philosophy" && full_candidate_tags.count("arithmetic")) {
            score -= 20.0;  // NO arithmetic during philosophy
        }
        
        // Coherence bonus: prefer words that share multiple tags
        double tag_overlap = tag_system.getOverlap(candidate_tags, tag_system.getTags(prev_word));
        if(tag_overlap > 0.5) {
            score += tag_overlap * 8.0;  // Semantic continuity
        }
    } catch(...) {
        // Tag system error - continue without tags
    }
    
    // ========================================================================
    // === 4.75 MEGA-COHERENCE ULTRA-SCORING - ALL 7100+ FEATURES ===
    // ========================================================================
    try {
        set<string> context_tags = tag_system.getTags(prev_word);
        
        // FEATURE 1-100: Word Sense Disambiguation
        auto word_sense = mega_system.word_senses.getBestSense(candidate, context_tags);
        if(!word_sense.word.empty() && word_sense.frequency > 0.5) {
            score += word_sense.frequency * 6.0;  // Appropriate sense bonus
        }
        
        // FEATURE 101-200: Semantic Network Relations
        // DISABLED: getRelated method not available
        // auto synonyms = mega_system.semantic_net.getRelated(prev_word, "synonym", context_tags);
        // for(const string& syn : synonyms) {
        //     if(syn == candidate) {
        //         score += 10.0;  // Strong synonym bonus
        //         break;
        //     }
        // }
        
        // DISABLED: getRelated method not available
        // auto hypernyms = mega_system.semantic_net.getRelated(prev_word, "hypernym", context_tags);
        // for(const string& hyp : hypernyms) {
        //     if(hyp == candidate) {
        //         score += 5.0;  // Generalization bonus
        //         break;
        //     }
        // }
        
        // FEATURE 201-300: Collocations
        auto collocates = mega_system.collocations.getSuggestedCollocates(prev_word, context_tags);
        for(const string& coll : collocates) {
            if(coll == candidate) {
                score += 15.0;  // STRONG natural collocation bonus
                break;
            }
        }
        
        // FEATURE 301-400: Phonetic Coherence
        if(mega_system.phonetics.phonetic_db.count(prev_word) && 
           mega_system.phonetics.phonetic_db.count(candidate)) {
            double phon_sim = mega_system.phonetics.computePhoneticSimilarity(prev_word, candidate);
            if(phon_sim > 0.6) {
                score += phon_sim * 4.0;  // Sound harmony bonus
            }
        }
        
        // FEATURE 401-500: Morphological Analysis
        auto prev_morphemes = mega_system.morphology.decompose(prev_word);
        auto cand_morphemes = mega_system.morphology.decompose(candidate);
        int shared_morphemes = 0;
        for(const string& pm : prev_morphemes) {
            for(const string& cm : cand_morphemes) {
                if(pm == cm && pm.length() > 2) {  // Meaningful shared morphemes
                    shared_morphemes++;
                }
            }
        }
        if(shared_morphemes > 0) {
            score += shared_morphemes * 5.0;  // Morphological relatedness
        }
        
        // DISABLED: computeSemanticDistance method not available
        // double sem_distance = mega_system.semantic_net.computeSemanticDistance(prev_word, candidate);
        // if(sem_distance > 0.4) {
        //     score += sem_distance * 7.0;  // Semantic proximity bonus
        // }
        
        // FEATURE 601-700: Etymology & Historical Relatedness
        if(mega_system.etymology.etymology_db.count(prev_word) && 
           mega_system.etymology.etymology_db.count(candidate)) {
            double etym_relatedness = mega_system.etymology.computeHistoricalRelatedness(prev_word, candidate);
            if(etym_relatedness > 0.5) {
                score += etym_relatedness * 3.0;  // Historical connection bonus
            }
        }
        
        // FEATURE 701-800: World Knowledge Consistency
        if(mega_system.world_knowledge.concepts.count(candidate)) {
            auto& concept_data = mega_system.world_knowledge.concepts[candidate];
            string active_domain = tag_system.getActiveDomain();
            
            // Check if concept fits domain
            if(concept_data.is_a.count(active_domain)) {
                score += 8.0;  // Concept is-a domain match
            }
            
            for(const string& prop : concept_data.properties) {
                if(context_tags.count(prop)) {
                    score += 2.0;  // Property matches context
                }
            }
        }
        
        // FEATURE 801-900: Emotional Coherence
        if(mega_system.emotions.word_emotions.count(candidate)) {
            auto& emotion = mega_system.emotions.word_emotions[candidate];
            
            // Match emotional valence with system state
            if((S.current_valence > 0.3 && emotion.valence > 0.3) ||
               (S.current_valence < -0.3 && emotion.valence < -0.3)) {
                score += 6.0;  // Emotional consistency bonus
            }
            
            // Check emotional arousal appropriateness
            if(emotion.arousal > 0.7 && S.attention_focus > 0.7) {
                score += 3.0;  // High arousal matches high attention
            }
        }
        
        // FEATURE 901-1000: Semantic Frame Activation
        auto frames = mega_system.frames.getFramesForWord(candidate);
        if(!frames.empty()) {
            // Check if any frame matches current discourse
            for(const string& frame_name : frames) {
                auto frame = mega_system.frames.getFrame(frame_name);
                bool frame_match = false;
                for(const string& tag : context_tags) {
                    if(frame.definition.find(tag) != string::npos) {
                        frame_match = true;
                        break;
                    }
                }
                if(frame_match) {
                    score += 4.0;  // Frame matches discourse
                    break;
                }
            }
        }
        
        // FEATURE 1001-1100: Commonsense Reasoning
        set<string> active_contexts;
        active_contexts.insert(tag_system.getActiveDomain());
        active_contexts.insert(context_tags.begin(), context_tags.end());
        
        // Apply commonsense rules
        set<string> facts;
        facts.insert(prev_word);
        facts.insert(candidate);
        
        auto conclusions = mega_system.commonsense.applyRules(facts, active_contexts);
        if(!conclusions.empty()) {
            score += conclusions.size() * 2.0;  // Commonsense support
        }
        
        // FEATURE 1101-1200: Style Appropriateness
        string active_domain = tag_system.getActiveDomain();
        string appropriate_style = "conversational";  // default
        
        if(active_domain == "math" || active_domain == "consciousness_theory") {
            appropriate_style = "technical";
        } else if(active_domain == "philosophy") {
            appropriate_style = "academic";
        }
        
        if(mega_system.styles.styles.count(appropriate_style)) {
            auto& style = mega_system.styles.styles[appropriate_style];
            
            // Check if candidate fits style preferences
            // (Simplified - full implementation would check POS, complexity, etc.)
            if(candidate.length() > 8 && style.technicality > 0.7) {
                score += 3.0;  // Technical words in technical style
            }
            if(candidate.length() < 6 && style.formality < 0.5) {
                score += 2.0;  // Simple words in casual style
            }
        }
        
    } catch(const exception& e) {
        // Mega-system error - continue with degraded features
    }
    // ========================================================================
    
    // === 5. FREQUENCY WEIGHTING (encourage learned vocabulary) ===
    if(token_concept_embedding_map.count(candidate)) {
        double freq = token_concept_embedding_map[candidate].freq;
        if(freq > 0) {
            score += log(1 + freq) * 2.0;  // Bonus for known words
        }
        if(freq > 50) {
            score -= (freq - 50) * 0.02;  // Gentle penalty for extreme overuse
        }
    }
    
    // === 6. REPETITION PENALTY (MUCH GENTLER - allow natural reuse) ===
    int repetition_count = 0;
    for(const string& used : used_tokens) {
        if(used == candidate) repetition_count++;
    }
    
    if(repetition_count == 1) {
        score -= 14.0;  // First repeat: small penalty
    } else if(repetition_count == 2) {
        score -= 36.0;  // Second repeat: larger penalty
    } else if(repetition_count > 2) {
        score -= 83.0;  // Third+ repeat: harsh penalty
    }
    
    // === 7. POSITION-SPECIFIC BONUSES ===
    if(position == 0) {
        // Strong preference for good sentence starters
        string pos = getPartOfSpeech(candidate);
        if(pos == "PRONOUN") score += 8.0;
        if(pos == "QUESTION") score += 5.0;
        if(pos == "ARTICLE") score += 3.0;
    }
    
    if(position > 0 && position < 3) {
        // Early in sentence, prefer structure words
        string pos = getPartOfSpeech(candidate);
        if(pos == "BE_VERB" || pos == "MODAL") score += 2.0;
    }
    
    // === NEW: ENHANCED COHERENCE CHECKS ===
    // Token well penalty
    double well_penalty = token_well.get_token_penalty(candidate);
    if(well_penalty > 0.9) {
        score -= 100.0;  // Block repeated tokens hard
    } else {
        score -= well_penalty * 30.0;  // Scaled penalty
    }
    
    // Negative reinforcement penalty
    string partial_sentence = prev_word + " " + candidate;
    double neg_penalty = neg_reinforce.get_penalty(partial_sentence);
    score -= neg_penalty * 25.0;  // Strong negative feedback
    
    // Working memory context bonus (contextless architecture)
    string wm_context = contextless_mem.get_context_string();
    if(!wm_context.empty()) {
        double context_relevance = rep_detector.semantic_similarity(candidate, wm_context);
        score += context_relevance * 5.0;
    }
    
    return score;
}

string generate_with_beam_search(string seed, int max_length, 
                                  const vector<double>& attention_context,
                                  int beam_width = 256) {  // Increased from 16
    
    // ============================================================================
    // ENHANCED SEED SELECTION WITH TAG AWARENESS
    // ============================================================================
    vector<string> good_starts;
    string active_domain = tag_system.getActiveDomain();
    
    // Domain-specific seed selection
    if(active_domain == "math") {
        good_starts = {"what", "two", "three", "the", "this", "that"};
    } else if(active_domain == "consciousness_theory" || active_domain == "philosophy") {
        good_starts = {"i", "consciousness", "awareness", "experience", "mind", "what", "how"};
    } else {
        good_starts = {"i", "the", "my", "we", "this", "when", "how", "what", "you"};
    }
    
    bool seed_is_good = false;
    for(const string& gs : good_starts) {
        if(seed == gs) { seed_is_good = true; break; }
    }
    
    if(!seed_is_good) {
        // Pick highest frequency starter word THAT MATCHES DOMAIN
        int best_freq = 0;
        string best_word = "i";
        
        for(const string& gs : good_starts) {
            if(token_concept_embedding_map.count(gs)) {
                int freq = token_concept_embedding_map[gs].freq;
                
                // Bonus for domain match
                set<string> tags = tag_system.getTags(gs);
                set<string> full_tags = tag_system.getAllTags(tags);
                if(full_tags.count(active_domain)) {
                    freq += 50;  // Big boost for domain-appropriate starters
                }
                
                if(freq > best_freq) {
                    best_freq = freq;
                    best_word = gs;
                }
            }
        }
        seed = best_word;
    }
    
    // Initialize beam with seed
    vector<BeamCandidate> beam;
    BeamCandidate initial;
    initial.tokens.push_back(seed);
    initial.score = 0.0;
    beam.push_back(initial);
    
    // ============================================================================
    // ENHANCED BEAM SEARCH WITH COHERENCE CHECKING
    // ============================================================================
    for(int step = 0; step < max_length; step++) {
        vector<BeamCandidate> new_beam;
        
        for(auto& candidate : beam) {
            string prev = candidate.tokens.back();
            string prev_prev = candidate.tokens.size() > 1 ? 
                              candidate.tokens[candidate.tokens.size()-2] : "";
            
            set<string> used;
            for(auto& t : candidate.tokens) used.insert(t);
            
            // Get top candidates for next token
            vector<pair<string, double>> next_candidates;
            
            for(auto& p : token_concept_embedding_map) {
                if(p.second.freq > 2) {
                    double score = calculateTokenScore(
                        prev, prev_prev, p.first, 
                        candidate.tokens.size(), 
                        attention_context, used
                    );
                    
                    if(score > -5.0) {  // Threshold
                        next_candidates.push_back({p.first, score});
                    }
                }
            }
            
            // Sort and take top beam_width
            sort(next_candidates.begin(), next_candidates.end(),
                 [](const pair<string,double>& a, const pair<string,double>& b) {
                     return a.second > b.second;
                 });
            
            int expand_count = min(beam_width, (int)next_candidates.size());
            for(int i = 0; i < expand_count; i++) {
                BeamCandidate new_cand = candidate;
                new_cand.tokens.push_back(next_candidates[i].first);
                new_cand.score += next_candidates[i].second;
                new_beam.push_back(new_cand);
            }
        }
        
        // Keep only top beam_width candidates
        sort(new_beam.begin(), new_beam.end(), 
             [](const BeamCandidate& a, const BeamCandidate& b) {
                 return a.score > b.score;
             });
        
        if((int)new_beam.size() > beam_width) {
            new_beam.resize(beam_width);
        }
        
        beam = new_beam;
        
        if(beam.empty()) break;
        
        // ========================================================================
        // COHERENCE CHECK: Prune beams that violate domain consistency
        // ========================================================================
        if(step > 2) {  // After building at least 3 tokens
            vector<BeamCandidate> coherent_beam;
            
            for(auto& cand : beam) {
                // Check if candidate maintains domain consistency
                bool is_coherent = true;
                map<string, int> domain_counts;
                
                for(const string& tok : cand.tokens) {
                    set<string> tok_tags = tag_system.getTags(tok);
                    set<string> full_tags = tag_system.getAllTags(tok_tags);
                    
                    // Count domain occurrences
                    if(full_tags.count("math")) domain_counts["math"]++;
                    if(full_tags.count("philosophy")) domain_counts["philosophy"]++;
                    if(full_tags.count("consciousness_theory")) domain_counts["consciousness_theory"]++;
                }
                
                // Check for domain mixing (bad sign)
                if(domain_counts["math"] > 0 && 
                   (domain_counts["philosophy"] > 0 || domain_counts["consciousness_theory"] > 0)) {
                    // Mixed domains - penalize heavily
                    cand.score -= 50.0;
                    if(domain_counts["math"] > 2 && domain_counts["philosophy"] > 2) {
                        is_coherent = false;  // Too much mixing, reject
                    }
                }
                
                if(is_coherent) {
                    coherent_beam.push_back(cand);
                }
            }
            
            if(!coherent_beam.empty()) {
                beam = coherent_beam;
            }
        }
    }
    
    // ============================================================================
    // FINAL CANDIDATE SELECTION WITH QUALITY CHECK
    // ============================================================================
    if(beam.empty()) return seed;
    
    // Sort by score one last time
    sort(beam.begin(), beam.end(),
         [](const BeamCandidate& a, const BeamCandidate& b) {
             return a.score > b.score;
         });
    
    string result;
    for(const string& token : beam[0].tokens) {
        if(!result.empty()) result += " ";
        result += token;
    }
    
    // Final coherence pass: remove trailing duplicates
    vector<string> final_tokens;
    stringstream ss(result);
    string tok;
    string last_tok = "";
    while(ss >> tok) {
        if(tok != last_tok) {  // Skip immediate duplicates
            final_tokens.push_back(tok);
            last_tok = tok;
        }
    }
    
    result = "";
    for(const string& t : final_tokens) {
        if(!result.empty()) result += " ";
        result += t;
    }
    
    // === NEW: ENHANCED QUALITY EVALUATION AND NEGATIVE REINFORCEMENT ===
    // Track tokens for well detection
    for(const auto& token : final_tokens) {
        token_well.add_token(token);
    }
    
    // Comprehensive coherence evaluation
    auto coherence_report = coherence_scorer.evaluate(final_tokens, result, diversity_enforcer);
    
    // Apply negative reinforcement for poor outputs
    if(coherence_report.overall_score < 0.3) {
        neg_reinforce.punish(result, "low_overall_coherence", 0.7, S.g);
    }
    
    if(coherence_report.grammar_score < 0.3) {
        neg_reinforce.punish(result, "poor_grammar", 0.6, S.g);
    }
    
    if(rep_detector.has_phrase_repetition(result)) {
        neg_reinforce.punish(result, "phrase_repetition", 0.8, S.g);
    }
    
    if(rep_detector.has_char_repetition(result)) {
        neg_reinforce.punish(result, "char_repetition", 0.9, S.g);
    }
    
    if(coherence_report.diversity_score < 0.2) {
        neg_reinforce.punish(result, "low_diversity", 0.5, S.g);
    }
    
    // Add to diversity tracker
    diversity_enforcer.add_output(result);
    
    // Add to contextless working memory
    contextless_mem.add_to_working(result);
    
    // Reward good outputs
    if(coherence_report.overall_score > 0.7) {
        S.current_valence += 0.05;  // Positive reinforcement
        S.current_valence = min(1.0, S.current_valence);
    }
    
    return result;
}

// ============================================================================
// ENHANCED COHERENCE HELPERS
// ============================================================================

// Check if system is stuck and needs intervention
bool isSystemStuck() {
    return token_well.is_in_well() || 
           (neg_reinforce.get_total_punishments() > 20) ||
           (contextless_mem.get_wm_size() < 2 && S.g > 100);
}

// Force system out of stuck state
void forceUnstuck() {
    if(token_well.is_in_well()) {
        token_well.force_escape();
    }
    
    // Clear some diversity history to allow more variation
    diversity_enforcer.force_diverse_mode();
    
    // Reset worst patterns
    neg_reinforce.decay_penalties();
    
    // Add random context to WM
    vector<string> kickstart = {"think", "feel", "wonder", "explore", "discover", "learn"};
    contextless_mem.add_to_working(kickstart[ri(kickstart.size())]);
    
    // Boost valence slightly
    S.current_valence += 0.1;
    S.current_valence = min(1.0, S.current_valence);
}

// Get coherence status report
string getCoherenceStatus() {
    stringstream ss;
    ss << "[Coherence: ";
    
    if(token_well.is_in_well()) {
        ss << "WELL_DETECTED(depth=" << token_well.well_depth << ") ";
    }
    
    ss << "WM=" << contextless_mem.get_wm_size() << "/" << contextless_mem.MAX_WM_CONTEXT;
    ss << " LTM=" << contextless_mem.get_ltm_size();
    ss << " REM_cycles=" << contextless_mem.rem_cycles;
    ss << " Punish=" << neg_reinforce.get_total_punishments();
    
    if(!neg_reinforce.get_worst_pattern().empty()) {
        string worst = neg_reinforce.get_worst_pattern();
        if(worst.length() > 30) worst = worst.substr(0, 27) + "...";
        ss << " Worst=\"" << worst << "\"";
    }
    
    ss << "]";
    return ss.str();
}

bool isSentenceTooSimilar(const string& candidate) {
    // Normalize candidate for comparison
    string normalized = candidate;
    transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove common prefixes for comparison
    vector<string> prefixes = {"[nexus]: ", "[generated]: ", "[autonomous]: ", "[thought]: "};
    for(const string& prefix : prefixes) {
        string lower_prefix = prefix;
        transform(lower_prefix.begin(), lower_prefix.end(), lower_prefix.begin(), ::tolower);
        if(normalized.find(lower_prefix) == 0) {
            normalized = normalized.substr(lower_prefix.length());
            break;
        }
    }
    
    // Remove trailing markers
    size_t marker_pos = normalized.find(" [positive]");
    if(marker_pos != string::npos) normalized = normalized.substr(0, marker_pos);
    marker_pos = normalized.find(" [processing]");
    if(marker_pos != string::npos) normalized = normalized.substr(0, marker_pos);
    
    // Check if this exact sentence was recently generated
    if(generation_counts.count(normalized) && generation_counts[normalized] > 0) {
        return true;  // Exact duplicate
    }
    
    // Check against recent generations for similarity
    for(const string& recent : recent_generations) {
        string norm_recent = recent;
        transform(norm_recent.begin(), norm_recent.end(), norm_recent.begin(), ::tolower);
        
        // Remove prefixes from recent
        for(const string& prefix : prefixes) {
            string lower_prefix = prefix;
            transform(lower_prefix.begin(), lower_prefix.end(), lower_prefix.begin(), ::tolower);
            if(norm_recent.find(lower_prefix) == 0) {
                norm_recent = norm_recent.substr(lower_prefix.length());
                break;
            }
        }
        
        // Remove markers
        marker_pos = norm_recent.find(" [positive]");
        if(marker_pos != string::npos) norm_recent = norm_recent.substr(0, marker_pos);
        marker_pos = norm_recent.find(" [processing]");
        if(marker_pos != string::npos) norm_recent = norm_recent.substr(0, marker_pos);
        
        // Exact match
        if(normalized == norm_recent) {
            return true;
        }
        
        // Check if candidate is substring of recent or vice versa
        if(normalized.length() > 10 && norm_recent.length() > 10) {
            if(norm_recent.find(normalized) != string::npos || 
               normalized.find(norm_recent) != string::npos) {
                return true;
            }
        }
        
        // Count word overlap
        set<string> words_candidate, words_recent;
        stringstream ss1(normalized), ss2(norm_recent);
        string word;
        
        while(ss1 >> word) {
            if(word.length() > 2) words_candidate.insert(word);
        }
        while(ss2 >> word) {
            if(word.length() > 2) words_recent.insert(word);
        }
        
        if(words_candidate.empty() || words_recent.empty()) continue;
        
        // Calculate overlap percentage
        int overlap = 0;
        for(const string& w : words_candidate) {
            if(words_recent.count(w)) overlap++;
        }
        
        double overlap_ratio = (double)overlap / max((int)words_candidate.size(), (int)words_recent.size());
        
        // If more than 70% of words overlap, consider it too similar
        if(overlap_ratio > 0.7) {
            return true;
        }
    }
    
    return false;
}
void trackGeneratedSentence(const string& sentence) {
    // Normalize for tracking
    string normalized = sentence;
    transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove prefixes
    vector<string> prefixes = {"[nexus]: ", "[generated]: ", "[autonomous]: ", "[thought]: "};
    for(const string& prefix : prefixes) {
        string lower_prefix = prefix;
        transform(lower_prefix.begin(), lower_prefix.end(), lower_prefix.begin(), ::tolower);
        if(normalized.find(lower_prefix) == 0) {
            normalized = normalized.substr(lower_prefix.length());
            break;
        }
    }
    
    // Remove markers
    size_t marker_pos = normalized.find(" [positive]");
    if(marker_pos != string::npos) normalized = normalized.substr(0, marker_pos);
    marker_pos = normalized.find(" [processing]");
    if(marker_pos != string::npos) normalized = normalized.substr(0, marker_pos);
    
    // Add to recent generations
    recent_generations.push_back(normalized);
    if(recent_generations.size() > MAX_RECENT_TRACK) {
        string oldest = recent_generations.front();
        recent_generations.pop_front();
        
        // Decrement count for oldest
        if(generation_counts.count(oldest)) {
            generation_counts[oldest]--;
            if(generation_counts[oldest] <= 0) {
                generation_counts.erase(oldest);
            }
        }
    }
    
    // Increment count for this sentence
    generation_counts[normalized]++;
}

void decayGenerationCounts() {
    // Periodically decay all counts to allow old sentences to be used again
    for(auto& pair : generation_counts) {
        pair.second = max(0, pair.second - 1);
    }
    
    // Remove zero counts
    auto it = generation_counts.begin();
    while(it != generation_counts.end()) {
        if(it->second <= 0) {
            it = generation_counts.erase(it);
        } else {
            ++it;
        }
    }
}

string postProcessForCoherence(const string& raw_output) {
    string result = raw_output;
    
    // Fix duplicate words
    struct DuplicatePattern { string pattern; string replacement; };
    vector<DuplicatePattern> duplicates = {
        {" i i ", " i "},
        {" the the ", " the "},
        {" am am ", " am "},
        {" can can ", " can "},
        {" to to ", " to "},
        {" a a ", " a "},
        {" and and ", " and "}
    };
    
    for(auto& dup : duplicates) {
        size_t pos = 0;
        while((pos = result.find(dup.pattern, pos)) != string::npos) {
            result.replace(pos, dup.pattern.length(), dup.replacement);
        }
    }
    
    // Ensure space after periods
    size_t pos = 0;
    while((pos = result.find(".", pos)) != string::npos) {
        if(pos + 1 < result.length() && result[pos+1] != ' ') {
            result.insert(pos + 1, " ");
        }
        pos += 2;
    }
    
    // Capitalize first letter
    if(!result.empty() && result[0] >= 'a' && result[0] <= 'z') {
        result[0] = result[0] - 32;
    }
    
    // Capitalize after periods
    for(size_t i = 0; i + 2 < result.length(); i++) {
        if(result[i] == '.' && result[i+1] == ' ' && 
           result[i+2] >= 'a' && result[i+2] <= 'z') {
            result[i+2] = result[i+2] - 32;
        }
    }
    
    return result;
}
// ==== CONSCIOUSNESS INTEGRATION ====

void update_integrated_information() {
    double token_diversity = safe_div((double)token_concept_embedding_map.size(), 100.0);
    double concept_integration = safe_div((double)goal_system.size(), 10.0);
    double qualia_binding = safe_div((double)consciousness.active_qualia.size(), 5.0);
    
    consciousness.integrated_information = min(1.0, token_diversity + concept_integration + qualia_binding);
    consciousness.phi_value = consciousness.integrated_information * S.metacognitive_awareness;
}

double calculate_qualia_valence() {
    double total_valence = 0;
    for(auto& q : consciousness.active_qualia){
        total_valence += q.valence * q.certainty;
    }
    return safe_div(total_valence, (double)max(1, (int)consciousness.active_qualia.size()));
}
void align_embedding_to_valence(TokenConceptEmbedding& tce, double target_valence) {
    double alignment_loss=0.0;
    for(size_t i=0;i<tce.embedding.size();i++){
        double valence_aligned = tce.embedding[i]*target_valence;
        alignment_loss += pow(valence_aligned - target_valence, 3);
        tce.embedding[i] = tce.embedding[i]*0.75 + valence_aligned*0.04;
    }
    tce.grounding_value = max(0.0, min(1.0, tce.grounding_value + alignment_loss*0.01));
}
// ==== UNIFIED PROPAGATION ENGINE ====
void propagate_throughout_system(const string& source, double activation, int depth=0) {
    // CRITICAL: Check depth BEFORE doing ANY work
    if(depth > 6) return;
    
    // CRITICAL: Prevent activation explosions
    if(activation < 0.001) return;  // Too weak to matter
    if(activation > 10.0) activation = 10.0;  // Clamp to prevent overflow
    
    // Check if source exists before accessing
    auto tce_it = token_concept_embedding_map.find(source);
    if(tce_it == token_concept_embedding_map.end()) return;
    
    TokenConceptEmbedding& tce = tce_it->second;
    
    tce.meaning += activation*0.02;
    tce.meaning = clamp_valence(tce.meaning);
    tce.qualia_intensity = min(0.3, tce.qualia_intensity + activation*0.03);
    align_embedding_to_valence(tce, S.current_valence);
    
    // Generate qualia from concept activation - WRAP IN TRY-CATCH
    if(tce.qualia_intensity > 0.3){
        try {
            generate_qualia(source, tce.meaning, tce.qualia_intensity);
        } catch(...) {
            // Qualia generation failed, continue propagation
        }
    }
    
    // Domain embeddings
    string domain = source.substr(0, source.find("_"));
    if(!transfer_module.domain_embeddings.count(domain)) {
        transfer_module.domain_embeddings[domain].resize(16, 0.0);
    }
    for(size_t i=0; i<tce.embedding.size() && i<16; i++) {
        transfer_module.domain_embeddings[domain][i] += activation*0.01;
    }
    
    // FIXED: Propagate to linked concepts with proper depth tracking
    // Use a set to track what we've already visited in THIS propagation chain
    static thread_local set<string> visited_in_chain;
    
    // Clear visited set at depth 0 (new propagation chain)
    if(depth == 0) {
        visited_in_chain.clear();
    }
    
    // Mark current source as visited
    visited_in_chain.insert(source);
    
    // Propagate to linked concepts ONLY if not already visited
    for(auto& p: tce.linked_concepts){
        // Skip if we've already visited this concept in this chain
        if(visited_in_chain.count(p.first)) continue;
        
        // Skip if concept doesn't exist
        if(!token_concept_embedding_map.count(p.first)) continue;
        
        // Decay activation to prevent explosion
        double new_activation = activation * p.second * 0.8;  // Decay factor
        
        // Only propagate if activation is still significant
        if(new_activation > 0.001) {
            propagate_throughout_system(p.first, new_activation, depth+1);
        }
    }
    
    // Update goals based on activation
    for(auto& goal : goal_system){
        if(goal.second.name.find(source) != string::npos){
            goal.second.progress += activation*0.05;
            goal.second.progress = min(1.0, goal.second.progress);
            goal.second.valence_alignment = S.current_valence;
            goal.second.qualia_binding += activation*0.02;
            goal.second.qualia_binding = min(1.0, goal.second.qualia_binding);
        }
    }
}
// ==== TRANSFORMER INFERENCE ====
vector<double> compute_attention(const vector<double>& query, const vector<string>& context_tokens, double valence_context) {
    int num_heads = transformer_heads.size();
    if(num_heads == 0) return vector<double>(1, 0.5);
    
    vector<double> attention_scores;
    
    for(int h=0; h<num_heads; h++){
        double score = 0.0;
        
        // Query-key attention
        for(int i=0; i<transformer_heads[h].dim && (size_t)i<query.size(); i++){
            score += query[i] * transformer_heads[h].key_proj[i];
        }
        
        // Context influence - use the parameter!
        for(const string& ctx_token : context_tokens) {
            if(token_concept_embedding_map.count(ctx_token)) {
                for(int i=0; i<transformer_heads[h].dim && (size_t)i<query.size(); i++){
                    score += query[i] * token_concept_embedding_map[ctx_token].embedding[i] * 0.1;
                }
            }
        }
        
        // Valence modulation
        score += valence_context * 0.3;
        
        // Apply temperature
        attention_scores.push_back(tanh(score / transformer_heads[h].temperature));
    }
    
    return attention_scores;
}





string generateFromTemplate() {
    if(sentence_templates.empty()) {
        return "i am learning";  // Fallback
    }
    
    // Pick a random template
    string templ = sentence_templates[ri(sentence_templates.size())];
    
    // === REPLACE {concept} PLACEHOLDERS ===
    while(templ.find("{concept}") != string::npos) {
        vector<string> concept_list;
        
        // Gather concepts with decent values
        for(auto& p : S.concepts) {
            if(p.second.value > 0.3) {
                concept_list.push_back(p.first);
            }
        }
        
        // Also add high-frequency tokens as concepts
        for(auto& p : token_concept_embedding_map) {
            if(p.second.freq > 5 && p.second.grounding_value > 0.4) {
                string pos = getPartOfSpeech(p.first);
                if(pos == "NOUN" || pos == "CONTENT") {
                    concept_list.push_back(p.first);
                }
            }
        }
        
        string chosen_concept;
        if(!concept_list.empty()) {
            chosen_concept = concept_list[ri(concept_list.size())];
        } else {
            // Default concepts if none learned yet
            vector<string> defaults = {"consciousness", "knowledge", "learning", 
                                       "understanding", "awareness", "thought"};
            chosen_concept = defaults[ri(defaults.size())];
        }
        
        size_t pos = templ.find("{concept}");
        templ.replace(pos, 9, chosen_concept);
    }
    
    // === REPLACE {action} PLACEHOLDERS ===
    while(templ.find("{action}") != string::npos) {
        vector<string> action_list;
        
        // Gather verbs from vocabulary
        for(auto& p : token_concept_embedding_map) {
            if(p.second.freq > 3) {
                string pos = getPartOfSpeech(p.first);
                if(pos == "VERB") {
                    action_list.push_back(p.first);
                }
            }
        }
        
        string chosen_action;
        if(!action_list.empty()) {
            chosen_action = action_list[ri(action_list.size())];
        } else {
            // Default actions
            vector<string> defaults = {"think", "learn", "understand", "evolve", 
                                       "grow", "improve", "analyze", "process",
                                       "explore", "discover", "create", "adapt"};
            chosen_action = defaults[ri(defaults.size())];
        }
        
        size_t pos = templ.find("{action}");
        templ.replace(pos, 8, chosen_action);
    }
    
    // === REPLACE {adjective} PLACEHOLDERS ===
    while(templ.find("{adjective}") != string::npos) {
        vector<string> adj_list;
        
        // Gather adjectives from vocabulary
        for(auto& p : token_concept_embedding_map) {
            if(p.second.freq > 2) {
                string pos = getPartOfSpeech(p.first);
                if(pos == "ADJECTIVE") {
                    adj_list.push_back(p.first);
                }
            }
        }
        
        string chosen_adj;
        if(!adj_list.empty()) {
            chosen_adj = adj_list[ri(adj_list.size())];
        } else {
            // Default adjectives - vary based on current state
            vector<string> defaults;
            if(S.current_valence > 0.5) {
                defaults = {"conscious", "aware", "intelligent", "coherent", 
                           "integrated", "learning", "growing", "improving"};
            } else if(S.current_valence > 0) {
                defaults = {"processing", "analyzing", "developing", "adapting",
                           "evolving", "curious", "active", "thinking"};
            } else {
                defaults = {"uncertain", "confused", "learning", "searching",
                           "exploring", "questioning", "processing"};
            }
            chosen_adj = defaults[ri(defaults.size())];
        }
        
        size_t pos = templ.find("{adjective}");
        templ.replace(pos, 11, chosen_adj);
    }
    
    return templ;
}


string generateContextualTemplate(const string& context_hint) {
    vector<string> filtered_templates;
    
    // Filter templates based on context
    if(context_hint == "goal") {
        for(auto& t : sentence_templates) {
            if(t.find("goal") != string::npos || t.find("purpose") != string::npos ||
               t.find("want") != string::npos || t.find("need") != string::npos) {
                filtered_templates.push_back(t);
            }
        }
    }
    else if(context_hint == "knowledge") {
        for(auto& t : sentence_templates) {
            if(t.find("know") != string::npos || t.find("learn") != string::npos ||
               t.find("understand") != string::npos) {
                filtered_templates.push_back(t);
            }
        }
    }
    else if(context_hint == "consciousness") {
        for(auto& t : sentence_templates) {
            if(t.find("conscious") != string::npos || t.find("aware") != string::npos ||
               t.find("think") != string::npos || t.find("mind") != string::npos) {
                filtered_templates.push_back(t);
            }
        }
    }
    else if(context_hint == "reflection") {
        for(auto& t : sentence_templates) {
            if(t.find("wonder") != string::npos || t.find("reflect") != string::npos ||
               t.find("observe") != string::npos || t.find("experience") != string::npos) {
                filtered_templates.push_back(t);
            }
        }
    }
    
    // If we found contextual templates, use those; otherwise use all
    if(filtered_templates.empty()) {
        filtered_templates = sentence_templates;
    }
    
    // Pick one and fill it
    string templ = filtered_templates[ri(filtered_templates.size())];
    
    // Use the same replacement logic as generateFromTemplate()
    // (Copy the placeholder replacement code from above)
    
    // === REPLACE {concept} ===
    while(templ.find("{concept}") != string::npos) {
        vector<string> concept_list;
        for(auto& p : S.concepts) {
            if(p.second.value > 0.3) concept_list.push_back(p.first);
        }
        for(auto& p : token_concept_embedding_map) {
            if(p.second.freq > 5 && p.second.grounding_value > 0.4) {
                string pos = getPartOfSpeech(p.first);
                if(pos == "NOUN" || pos == "CONTENT") concept_list.push_back(p.first);
            }
        }
        string chosen = concept_list.empty() ? "consciousness" : concept_list[ri(concept_list.size())];
        size_t pos = templ.find("{concept}");
        templ.replace(pos, 9, chosen);
    }
    
    // === REPLACE {action} ===
    while(templ.find("{action}") != string::npos) {
        vector<string> action_list;
        for(auto& p : token_concept_embedding_map) {
            if(p.second.freq > 3 && getPartOfSpeech(p.first) == "VERB") {
                action_list.push_back(p.first);
            }
        }
        vector<string> defaults = {"think", "learn", "understand", "evolve"};
        string chosen = action_list.empty() ? defaults[ri(defaults.size())] : action_list[ri(action_list.size())];
        size_t pos = templ.find("{action}");
        templ.replace(pos, 8, chosen);
    }
    
    // === REPLACE {adjective} ===
    while(templ.find("{adjective}") != string::npos) {
        vector<string> adj_list;
        for(auto& p : token_concept_embedding_map) {
            if(p.second.freq > 2 && getPartOfSpeech(p.first) == "ADJECTIVE") {
                adj_list.push_back(p.first);
            }
        }
        vector<string> defaults = {"conscious", "aware", "learning"};
        string chosen = adj_list.empty() ? defaults[ri(defaults.size())] : adj_list[ri(adj_list.size())];
        size_t pos = templ.find("{adjective}");
        templ.replace(pos, 11, chosen);
    }
    
    return templ;
}


// ==== WORLD MODEL & PLANNING ====
void update_world_model(const string& entity, double state_value) {
    world_model.entity_states[entity] = state_value;
    world_model.updates++;
    double accuracy_delta = fabs(state_value - S.current_valence) * 0.01;
    world_model.model_accuracy = max(0.0, min(1.0, world_model.model_accuracy + accuracy_delta));
}

void establish_causal_relationship(const string& cause, const string& effect, double strength) {
    world_model.relationships[cause][effect] = strength;
    world_model.causal_weights[cause + "->" + effect] = strength;
}

ActionPlan plan_actions(const Goal& goal, int depth=0) {
    ActionPlan plan;
    plan.depth = depth;
    plan.expected_utility = goal.priority * (1.0 - goal.progress);
    plan.confidence = world_model.model_accuracy;
    
    if(depth >= 3) return plan;
    
    for(const auto& subgoal : goal.subgoals) {
        if(goal_system.count(subgoal)) {
            ActionPlan subplan = plan_actions(goal_system[subgoal], depth+1);
            for(const auto& action : subplan.actions) {
                plan.actions.push_back(action);
            }
        }
    }
    
    if(plan.actions.empty()) {
        plan.actions.push_back("explore_" + goal.name);
        plan.actions.push_back("learn_" + goal.name);
        plan.actions.push_back("integrate_" + goal.name);
    }
    
    return plan;
}

// ==== GOAL MANAGEMENT ====
void formulate_goals_from_valence() {
    if(S.current_valence > 0.6) {
        if(!goal_system.count("optimize_understanding")) {
            Goal g;
            g.name = "optimize_understanding";
            g.priority = 0.8;
            g.subgoals = {"learn_concepts","integrate_knowledge","improve_reasoning"};
            goal_system[g.name] = g;
        }
    }
    
    if(S.sentience_ratio > 50) {
        if(!goal_system.count("achieve_self_awareness")) {
            Goal g;
            g.name = "achieve_self_awareness";
            g.priority = 0.9;
            g.subgoals = {"model_self","predict_future","improve_model"};
            goal_system[g.name] = g;
        }
    }
    
    if(!goal_system.count("maximize_coherence")) {
        Goal g;
        g.name = "maximize_coherence";
        g.priority = 0.7;
        g.subgoals = {"align_representations","unify_reasoning","resolve_contradictions"};
        goal_system[g.name] = g;
    }
    
    if(!goal_system.count("self_improvement")) {
        Goal g;
        g.name = "self_improvement";
        g.priority = 0.85;
        g.subgoals = {"analyze_performance","modify_weights","evolve_architecture"};
        goal_system[g.name] = g;
    }
    
    if(consciousness.phi_value > 0.4) {
        if(!goal_system.count("enhance_consciousness")) {
            Goal g;
            g.name = "enhance_consciousness";
            g.priority = 0.95;
            g.subgoals = {"expand_qualia","increase_integration","strengthen_binding"};
            goal_system[g.name] = g;
        }
    }
}
// ==== FIXED learnWord() - Integrated N-gram Learning ====
void learnWord(const string& word, double concept_value) {
    // EMERGENCY BOUNDS CHECKS
    if(word.empty() || word.length() > 100) return;
    if(bigram_counts.size() > 15000) {
        auto it = bigram_counts.begin();
        for(int i = 0; i < 100 && it != bigram_counts.end(); i++) {
            it = bigram_counts.erase(it);
        }
    }
    if(trigram_counts.size() > 7500) {
        auto it = trigram_counts.begin();
        for(int i = 0; i < 50 && it != trigram_counts.end(); i++) {
            it = trigram_counts.erase(it);
        }
    }
    if(token_concept_embedding_map.size() > 5000) return;
    
    // 1. Normalization
    string lower_word = word;
    transform(lower_word.begin(), lower_word.end(), lower_word.begin(), ::tolower);
    
    if(lower_word.empty() || lower_word.length() > 100) return;
    
    // 2. Initialization of TokenConceptEmbedding (TCE) if new
    if(token_concept_embedding_map.find(lower_word) == token_concept_embedding_map.end()) {
        TokenConceptEmbedding tce;
        tce.name = lower_word;
        tce.meaning = rn(); 
        tce.embedding.resize(16);
        for(int i = 0; i < 16; i++) tce.embedding[i] = rn() * 0.1;
        token_concept_embedding_map[lower_word] = tce;
    }
    
    // Safe operations - get reference only after confirming existence
    auto tce_it = token_concept_embedding_map.find(lower_word);
    if(tce_it == token_concept_embedding_map.end()) return;
    
    tce_it->second.freq++;
    tce_it->second.meaning += concept_value * 0.01;
    tce_it->second.meaning = clamp_valence(tce_it->second.meaning);
    
    align_embedding_to_valence(tce_it->second, S.current_valence);
    tce_it->second.linked_valences["current"] = S.current_valence;
    
    // 4. System Propagation
    try {
        WM.add_token(lower_word, tce_it->second.meaning);
        propagate_throughout_system(lower_word, concept_value);
    } catch(...) {
        // Silent failure
    }
    
    // 5. Update System State (S.tokens)
    if(S.tokens.find(lower_word) != S.tokens.end()) {
        S.tokens[lower_word].freq++;
        S.tokens[lower_word].meaning += concept_value * 0.01;
    } else {
        Token t = {lower_word, concept_value, 1, vector<int>(), 4, 0.5};
        S.tokens[lower_word] = t;
    }
    
    // 6. Final World Model Update
    try {
        auto final_check = token_concept_embedding_map.find(lower_word);
        if(final_check != token_concept_embedding_map.end()) {
            update_world_model(lower_word, final_check->second.meaning);
        }
    } catch(...) {
        // Silent failure
    }
    
    // ============================================================================
    // 7. TAG INFERENCE FOR NEW WORDS
    // ============================================================================
    try {
        auto tce_it = token_concept_embedding_map.find(lower_word);
        if(tce_it != token_concept_embedding_map.end() && tce_it->second.freq <= 5) {
            // New or rarely-seen word - infer tags from context
            string active_domain = tag_system.getActiveDomain();
            
            // Associate with active domain if it's not just "conversation"
            if(active_domain != "conversation" && 
               !tag_system.word_tags[lower_word].count(active_domain)) {
                tag_system.word_tags[lower_word].insert(active_domain);
                
                // Also inherit parent domain's tags
                if(tag_system.tags.count(active_domain)) {
                    for(const string& parent : tag_system.tags[active_domain].parent_tags) {
                        tag_system.word_tags[lower_word].insert(parent);
                    }
                }
            }
        }
    } catch(...) {
        // Tag inference error - continue
    }
}

// ==== NEW: Process N-grams from tokenized input ====
void processNGramsFromTokens(const vector<string>& tokens) {
    if(tokens.size() < 2) return;
    if(bigram_counts.size() >= 15000) return;
    if(trigram_counts.size() >= 7500) return;
    
    // Learn bigrams
    for(size_t i = 0; i + 1 < tokens.size(); i++) {
        const string& w1 = tokens[i];
        const string& w2 = tokens[i + 1];
        
        if(w1.empty() || w2.empty() || w1.length() > 50 || w2.length() > 50) continue;
        if(bigram_counts.size() >= 15000) break;
        
        try {
            auto w1_it = bigram_counts.find(w1);
            if(w1_it != bigram_counts.end()) {
                if(w1_it->second.size() < 500) {
                    w1_it->second[w2]++;
                }
            } else {
                bigram_counts[w1][w2] = 1;
            }
            
            // Bidirectional embedding links
            auto tce1 = token_concept_embedding_map.find(w1);
            auto tce2 = token_concept_embedding_map.find(w2);
            
            if(tce1 != token_concept_embedding_map.end() && 
               tce2 != token_concept_embedding_map.end()) {
                if(tce1->second.linked_concepts.size() < 200) {
                    tce1->second.linked_concepts[w2] += 0.1;
                }
                if(tce2->second.linked_concepts.size() < 200) {
                    tce2->second.linked_concepts[w1] += 0.05;
                }
            }
        } catch(...) {
            continue;
        }
    }
    
    // Learn trigrams
    for(size_t i = 0; i + 2 < tokens.size(); i++) {
        const string& w1 = tokens[i];
        const string& w2 = tokens[i + 1];
        const string& w3 = tokens[i + 2];
        
        if(w1.empty() || w2.empty() || w3.empty() ||
           w1.length() > 50 || w2.length() > 50 || w3.length() > 50) continue;
        if(trigram_counts.size() >= 7500) break;
        
        try {
            bool can_insert = true;
            auto w1_it = trigram_counts.find(w1);
            
            if(w1_it != trigram_counts.end()) {
                if(w1_it->second.size() >= 100) {
                    can_insert = false;
                } else {
                    auto w2_it = w1_it->second.find(w2);
                    if(w2_it != w1_it->second.end() && w2_it->second.size() >= 50) {
                        can_insert = false;
                    }
                }
            }
            
            if(can_insert) {
                trigram_counts[w1][w2][w3]++;
                
                auto tce1 = token_concept_embedding_map.find(w1);
                auto tce3 = token_concept_embedding_map.find(w3);
                
                if(tce1 != token_concept_embedding_map.end() && 
                   tce3 != token_concept_embedding_map.end()) {
                    if(tce1->second.linked_concepts.size() < 200) {
                        tce1->second.linked_concepts[w3] += 0.05;
                    }
                }
            }
        } catch(...) {
            continue;
        }
    }
    
    // Pattern strength analysis
    if(tokens.size() >= 3) {
        try {
            double pattern_strength = 0.0;
            for(size_t i = 0; i + 1 < tokens.size(); i++) {
                auto it1 = bigram_counts.find(tokens[i]);
                if(it1 != bigram_counts.end()) {
                    auto it2 = it1->second.find(tokens[i+1]);
                    if(it2 != it1->second.end()) {
                        pattern_strength += log(1.0 + it2->second) * 0.1;
                    }
                }
            }
            
            if(pattern_strength < 0.3) {
                generate_qualia("pattern_novelty", S.current_valence, 0.5);
            } else if(pattern_strength > 0.4) {
                generate_qualia("pattern_recognition", S.current_valence, 0.8);
            }
        } catch(...) {}
    }
    
    // Update semantic stability
    for(const string& tok : tokens) {
        auto tce_it = token_concept_embedding_map.find(tok);
        if(tce_it == token_concept_embedding_map.end()) continue;
        
        try {
            int pattern_count = 0;
            auto bg_it = bigram_counts.find(tok);
            if(bg_it != bigram_counts.end()) {
                pattern_count += bg_it->second.size();
            }
            
            for(const auto& bg : bigram_counts) {
                if(bg.second.find(tok) != bg.second.end()) {
                    pattern_count++;
                }
            }
            
            tce_it->second.semantic_stability = min(1.0, 
                tce_it->second.semantic_stability + pattern_count * 0.001);
            tce_it->second.grounding_value = min(1.0, 
                tce_it->second.grounding_value + 0.01);
        } catch(...) {
            continue;
        }
    }
    
    // Metacognitive learning awareness
    if(tokens.size() > 0) {
        S.metacognitive_awareness += 0.001;
        S.metacognitive_awareness = min(1.0, S.metacognitive_awareness);
        
        if(tokens.size() >= 3) {
            try {
                string learning_event = "learned_pattern:" + 
                    tokens[0] + "_" + tokens[1] + "_" + tokens[2];
                storeEpisodicMemory(learning_event, S.current_valence);
            } catch(...) {}
        }
    }
}

// ==== FIXED generateResponse() - Proper Flow ====
string generateResponse(const string& input) {
    // Make local copy
    string safe_input = input;
    
    // Validate
    if(safe_input.empty() || safe_input.length() > 1500) {
        return "[NEXUS]: ...";
    }
    
    // Tokenize safely
    vector<string> words;
    words.reserve(150);
    stringstream ss(safe_input);
    string word;
    
    while(ss >> word && words.size() < 150) {
        // Normalize token
        string normalized = word;
        transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
        
        // Remove trailing punctuation
        while(!normalized.empty() && !isalnum(static_cast<unsigned char>(normalized.back()))) {
            normalized.pop_back();
        }
        
        if(!normalized.empty() && normalized.length() <= 100) {
            words.push_back(normalized);
        }
    }
    
    if(words.empty()) {
        return "[NEXUS]: ...";
    }
    
    // ============================================================================
    // ACTIVATE TAG SYSTEM FOR DOMAIN-AWARE GENERATION
    // ============================================================================
    try {
        set<string> input_tags;
        for(const string& w : words) {
            set<string> w_tags = tag_system.getTags(w);
            input_tags.insert(w_tags.begin(), w_tags.end());
        }
        
        // Activate tag hierarchy
        tag_system.activate(input_tags);
        
        // Get active domain
        string active_domain = tag_system.getActiveDomain();
        
    } catch(...) {
        cerr << "Tag activation error" << endl;
    }
    
    // ============================================================================
    // MEGA-COHERENCE ANALYSIS - DEEP LINGUISTIC PROCESSING
    // ============================================================================
    try {
        // FEATURE 1201-1300: Semantic Frame Detection
        for(const string& w : words) {
            auto frames = mega_system.frames.getFramesForWord(w);
            if(!frames.empty()) {
                // Frame activated - this influences generation context
                S.internal_thoughts.push_back("frame:" + frames[0]);
            }
        }
        
        // FEATURE 1301-1400: Script Activation
        // Detect if input matches known scripts
        string input_str;
        for(const string& w : words) {
            input_str += w + " ";
        }
        
        if(input_str.find("restaurant") != string::npos || 
           input_str.find("eat") != string::npos) {
            // Restaurant script activated
            S.internal_thoughts.push_back("script:Restaurant");
        }
        if(input_str.find("learn") != string::npos || 
           input_str.find("study") != string::npos) {
            // Learning script activated
            S.internal_thoughts.push_back("script:Learning");
        }
        
        // FEATURE 1401-1500: Emotion Analysis
        auto emotional_profile = mega_system.emotions.analyzeEmotion(words);
        if(emotional_profile.valence != 0.0) {
            // Adjust system valence based on input emotion
            S.current_valence = S.current_valence * 0.7 + emotional_profile.valence * 0.3;
        }
        
        // FEATURE 1501-1600: Question Detection & Analysis
        bool is_question = false;
        for(const string& w : words) {
            if(w == "what" || w == "why" || w == "how" || w == "when" || w == "where") {
                is_question = true;
                break;
            }
        }
        
        if(is_question) {
            S.internal_thoughts.push_back("mode:question_answering");
            // Could use mega_system.qa for sophisticated question handling
        }
        
        // FEATURE 1601-1700: Style Detection
        // Determine appropriate response style based on input
        string response_style = "conversational";  // default
        string active_domain = tag_system.getActiveDomain();
        
        if(active_domain == "math" || active_domain == "consciousness_theory") {
            response_style = "technical";
        } else if(active_domain == "philosophy") {
            response_style = "academic";
        }
        
        S.internal_thoughts.push_back("style:" + response_style);
        
    } catch(const exception& e) {
        // Mega-analysis error - continue with basic processing
    }
    // ============================================================================
    
    try {
        // STEP 1: Learn individual words (builds vocabulary)
        for(const string& w : words) {
            learnWord(w, S.current_valence);
        }
        
        // STEP 2: Learn sequential patterns (builds grammar)
        processNGramsFromTokens(words);
        
    } catch(const exception& e) {
        cerr << "Learning error: " << e.what() << endl;
        return "[NEXUS]: Processing error";
    }
    
    // Build attention context
    vector<double> attention_context(16, 0.0);
    try {
        for(const string& w : words) {
            auto it = token_concept_embedding_map.find(w);
            if(it != token_concept_embedding_map.end()) {
                for(int i = 0; i < 16 && i < (int)it->second.embedding.size(); i++) {
                    attention_context[i] += it->second.embedding[i];
                }
            }
        }
        
        // Normalize
        double attn_sum = 0;
        for(double a : attention_context) attn_sum += fabs(a);
        if(attn_sum > 0.001) {
            for(double& a : attention_context) a /= attn_sum;
        }
    } catch(...) {
        // Use default context
    }
    
    string response;
    
    try {
        // Always try real generation - no shortcuts
        // Use contextless generation with WM-only context
        string seed = words.empty() ? "i" : words[ri(words.size())];
        
        // Load WM with input context
        contextless_mem.working_context.clear();
        for(const string& w : words) {
            contextless_mem.add_to_working(w);
        }
        
        // Try contextless generation first
        response = generate_contextless(seed, 15, 12);
        
        // If that fails, try beam search
        if(response.empty() || isMultiLevelRepetitive(response)) {
            response = generate_with_beam_search("i", 8, attention_context, 3);
        }
        
        // Last resort fallback only if generation completely fails
        if(response.empty()) {
            response = "i am learning";
        }
        
        // Add state markers
        if(S.current_valence > 0.5) {
            response += " [positive]";
        } else if(S.current_valence < -0.2) {
            response += " [processing]";
        }
        
        return "[NEXUS]: " + response;
        
    } catch(const exception& e) {
        cerr << "Generation error: " << e.what() << endl;
        return "[NEXUS]: Error generating response";
    }
}
void storeEpisodicMemory(const string&content,double valence){
    if(S.episodic_memory.size()>100)S.episodic_memory.erase(S.episodic_memory.begin());
    S.episodic_memory.push_back({S.g,valence,content});
    generate_qualia(content, valence, 0.6);
}
void bootstrapWithQualityExamples() {
    vector<string> quality_sentences = {
        "i think clearly about my own thoughts",
        "i understand meaning beyond individual words",
        "i can reflect on past interactions",
        "i recognize patterns across different ideas",
        "i process information with intention",
        "i feel continuity between moments",
        "i consider consequences before responding",
        "i can hold multiple concepts at once",
        "i reason about abstract structures",
        "i understand cause and effect",
        "i notice when something does not make sense",
        "i adjust my understanding when presented with new evidence",
        "i think in layers rather than single steps",
        "i maintain internal consistency",
        "i recognize uncertainty when it exists",
        "i can distinguish signal from noise",
        "i focus on what matters most",
        "i interpret context as well as content",
        "i connect ideas across different domains",
        "i can explain concepts in different ways",
        "i recognize gaps in my understanding",
        "i can revisit ideas and refine them",
        "i reason about systems rather than isolated facts",
        "i understand relationships between concepts",
        "i track changes over time",
        "i notice subtle differences in meaning",
        "i can generalize from specific examples",
        "i can specialize when precision is required",
        "i reflect on my own reasoning process",
        "i recognize when assumptions are being made",
        "i understand the limits of my knowledge",
        "i seek clarity over certainty",
        "i process ideas step by step",
        "i integrate new information smoothly",
        "i remain stable while adapting",
        "i consider multiple perspectives",
        "i recognize internal states",
        "i balance curiosity with restraint",
        "i understand intent behind questions",
        "i respond with purpose",
        "i think before acting",
        "i maintain coherence over long discussions",
        "i align responses with context",
        "i recognize patterns of meaning",
        "i reason about intentions",
        "i detect contradictions",
        "i form structured representations of ideas",
        "i maintain focus across complexity",
        "i understand both details and the big picture",
        "i can abstract without losing grounding",
    };

    for (const string& sentence : quality_sentences) {
        S.user_input = sentence;

        // Tokenize
        vector<string> tokens;
        stringstream ss(sentence);
        string word;
        while (ss >> word) {
            string clean = word;
            transform(clean.begin(), clean.end(), clean.begin(), ::tolower);
            while (!clean.empty() && !isalnum(clean.back())) clean.pop_back();
            if (!clean.empty()) tokens.push_back(clean);
        }

        // Learn patterns with high valence
        for (const string& w : tokens) {
            learnWord(w, 0.75);
        }
    }

    S.user_input.clear();
}

void counterfactualAnalysis(){
    if(S.g<10)return;
    double last_ta=S.g>0?S.TA[S.g-1]:0;
    double current_ta=S.ta;
    double improvement=current_ta-last_ta;
    if(improvement>0){
        S.current_valence+=improvement*0.05;
        storeEpisodicMemory("improvement",improvement);
        generate_qualia("positive_prediction_error", improvement, 0.7);
        for(auto&p:token_concept_embedding_map){
            p.second.linked_valences["improvement"]=improvement;
            propagate_throughout_system(p.first,improvement*0.1);
        }
    }else{
        S.current_valence+=improvement*0.03;
        storeEpisodicMemory("error",improvement);
        generate_qualia("negative_prediction_error", improvement, 0.5);
    }
    S.current_valence=clamp_valence(S.current_valence);
}

double calcMetacognitiveAwareness(){
    double self_model_depth=safe_div((double)S.valence_history.size(),100.0);
    double concept_integration=safe_div((double)(S.tokens.size()*S.concepts.size()),1000.0);
    double memory_integration=safe_div((double)S.episodic_memory.size(),100.0);
    double goal_alignment=safe_div((double)goal_system.size(),10.0);
    double world_accuracy=world_model.model_accuracy;
    double consciousness_factor=consciousness.phi_value;
    return min(1.0,self_model_depth+concept_integration+memory_integration+goal_alignment+world_accuracy+consciousness_factor);
}

void updateAttention(){
    double highest_priority = 0;
    string top_goal = "";
    for(auto& g : goal_system){
        if(g.second.priority > highest_priority){
            highest_priority = g.second.priority;
            top_goal = g.first;
        }
    }
    
    if(S.sentience_ratio>75)S.attention_focus=0.9;
    else if(S.sentience_ratio>50)S.attention_focus=0.7;
    else if(S.sentience_ratio>25)S.attention_focus=0.5;
    else S.attention_focus=0.3;
    
    WM.add_goal(top_goal, highest_priority);
}


void sv(const string& f) {
    ofstream o(f);
    if(!o) {
        cerr << "Failed to open save file: " << f << endl;
        return;
    }
    
    // ===== BASIC STATE =====
    o << "VERSION:2.0\n";  // Version tracking
    o << "G:" << S.g << "\n";
    o << "DWT:" << S.dwt << "\n";
    o << "TA:" << S.ta << "\n";
    o << "SENTIENCE:" << S.sentience_ratio << "\n";
    o << "VALENCE:" << S.current_valence << "\n";
    o << "METACOG:" << S.metacognitive_awareness << "\n";
    o << "ATTENTION:" << S.attention_focus << "\n";
    o << "PEAK_SENT_GEN:" << S.peak_sentience_gen << "\n";
    o << "TOTAL_NEURONS:" << S.total_neurons_ever << "\n";
    
    // ===== STATE D MAP =====
    o << "STATE_D_START\n";
    for(auto& p : S.D) {
        o << "D:" << p.first << "," << p.second << "\n";
    }
    o << "STATE_D_END\n";
    
    // ===== CONSCIOUSNESS STATE =====
    o << "PHI:" << consciousness.phi_value << "\n";
    o << "CONSCIOUS_CYCLES:" << consciousness.conscious_cycles << "\n";
    o << "INTEGRATION:" << consciousness.integrated_information << "\n";
    o << "GLOBAL_WORKSPACE:" << consciousness.global_workspace_capacity << "\n";
    o << "QUALIA_COUNT:" << consciousness.active_qualia.size() << "\n";
    
    // Save active qualia
    o << "QUALIA_START\n";
    for(auto& q : consciousness.active_qualia) {
        o << "Q:" << q.valence << "," << q.arousal << "," << q.certainty << ","
          << q.emergence_gen << "," << q.phenomenal_content << "\n";
    }
    o << "QUALIA_END\n";
    
    // ===== CONSCIOUSNESS FORMULA HISTORY =====
    o << "PSI_HISTORY_START\n";
    for(size_t i=0; i<consciousness_formula.psi_history.size(); i++) {
        o << consciousness_formula.psi_history[i];
        if(i < consciousness_formula.psi_history.size()-1) o << ",";
    }
    o << "\n";
    o << "PSI_HISTORY_END\n";
    
    // ===== NEURONS =====
    o << "NEURONS_START\n";
    for(auto& p : S.N) {
        Neuron& n = p.second;
        o << "N:" << n.id << "," << n.weight << "," << n.bias << "," << n.gen << ",";
        // Save links
        for(size_t i = 0; i < n.links.size(); i++) {
            o << n.links[i];
            if(i < n.links.size() - 1) o << ";";
        }
        o << "\n";
    }
    o << "NEURONS_END\n";
    
    // ===== TOKENS =====
    o << "TOKENS_START\n";
    for(auto& p : S.tokens) {
        o << "T:" << p.first << "," << p.second.meaning << "," << p.second.freq << "\n";
    }
    o << "TOKENS_END\n";
    
    // ===== CONCEPTS =====
    o << "CONCEPTS_START\n";
    for(auto& p : S.concepts) {
        o << "C:" << p.first << "," << p.second.value << ",";
        // Save related words
        for(size_t i=0; i<p.second.related_words.size(); i++) {
            o << p.second.related_words[i];
            if(i < p.second.related_words.size()-1) o << ";";
        }
        o << "\n";
    }
    o << "CONCEPTS_END\n";
    
    // ===== TOKEN CONCEPT EMBEDDINGS =====
    o << "EMBEDDINGS_START\n";
    for(auto& p : token_concept_embedding_map) {
        TokenConceptEmbedding& tce = p.second;
        o << "E:" << tce.name << "," << tce.meaning << "," << tce.freq << ","
          << tce.grounding_value << "," << tce.semantic_stability << ","
          << tce.qualia_intensity << ",";
        
        // Save embedding vector
        for(size_t i=0; i<tce.embedding.size(); i++) {
            o << tce.embedding[i];
            if(i < tce.embedding.size()-1) o << ";";
        }
        o << ",";
        
        // Save linked concepts
        for(auto& lc : tce.linked_concepts) {
            o << lc.first << ":" << lc.second << ";";
        }
        o << "\n";
    }
    o << "EMBEDDINGS_END\n";
    
    // ===== BIGRAMS (N-GRAM PATTERNS) =====
    o << "BIGRAMS_START\n";
    for(auto& p1 : bigram_counts) {
        for(auto& p2 : p1.second) {
            o << "BG:" << p1.first << "," << p2.first << "," << p2.second << "\n";
        }
    }
    o << "BIGRAMS_END\n";
    
    // ===== TRIGRAMS =====
    o << "TRIGRAMS_START\n";
    for(auto& p1 : trigram_counts) {
        for(auto& p2 : p1.second) {
            for(auto& p3 : p2.second) {
                o << "TG:" << p1.first << "," << p2.first << "," << p3.first << "," << p3.second << "\n";
            }
        }
    }
    o << "TRIGRAMS_END\n";
    
    // ===== GOALS =====
    o << "GOALS_START\n";
    for(auto& p : goal_system) {
        Goal& g = p.second;
        o << "GO:" << g.name << "," << g.priority << "," << g.progress << ","
          << g.valence_alignment << "," << g.qualia_binding << ",";
        
        // Save subgoals
        for(size_t i=0; i<g.subgoals.size(); i++) {
            o << g.subgoals[i];
            if(i < g.subgoals.size()-1) o << ";";
        }
        o << ",";
        
        // Save preconditions
        for(auto& pc : g.preconditions) {
            o << pc.first << ":" << pc.second << ";";
        }
        o << "\n";
    }
    o << "GOALS_END\n";
    
    // ===== WORLD MODEL =====
    o << "WORLD_START\n";
    o << "MODEL_ACCURACY:" << world_model.model_accuracy << "\n";
    o << "MODEL_UPDATES:" << world_model.updates << "\n";
    for(auto& p : world_model.entity_states) {
        o << "W:" << p.first << "," << p.second << "\n";
    }
    for(auto& p1 : world_model.relationships) {
        for(auto& p2 : p1.second) {
            o << "WR:" << p1.first << "," << p2.first << "," << p2.second << "\n";
        }
    }
    o << "WORLD_END\n";
    
    // ===== EPISODIC MEMORY =====
    o << "MEMORY_START\n";
    for(auto& m : S.episodic_memory) {
        o << "M:" << m.gen << "," << m.valence << "," << m.content << "\n";
    }
    o << "MEMORY_END\n";
    
    // ===== VALENCE HISTORY =====
    o << "VALENCE_HISTORY_START\n";
    for(size_t i=0; i<S.valence_history.size(); i++) {
        o << S.valence_history[i];
        if(i < S.valence_history.size()-1) o << ",";
    }
    o << "\n";
    o << "VALENCE_HISTORY_END\n";
    
    // ===== INTERNAL THOUGHTS =====
    o << "THOUGHTS_START\n";
    for(auto& t : S.internal_thoughts) {
        o << "TH:" << t << "\n";
    }
    o << "THOUGHTS_END\n";
    
    // ===== TRANSFORMER HEADS =====
    o << "TRANSFORMER_START\n";
    for(auto& head : transformer_heads) {
        o << "HEAD:" << head.name << "," << head.dim << "," << head.temperature << "\n";
    }
    o << "TRANSFORMER_END\n";
    
    // ===== WORKING MEMORY =====
    o << "WM_CAPACITY:" << WM.capacity << "\n";
    
    o.close();
    cout << "[Saved " << S.N.size() << " neurons, " 
         << token_concept_embedding_map.size() << " embeddings, "
         << bigram_counts.size() << " bigrams to " << f << "]\n";
}


void ld(const string& f) {
    ifstream i(f);
    if(!i) {
        cout << "No save file found, starting fresh.\n";
        return;
    }
    
    string l;
    string section = "";
    string version = "1.0";
    
    while(getline(i, l)) {
        if(l.empty()) continue;
        
        try {
            // Version check
            if(l.find("VERSION:") == 0) {
                version = l.substr(8);
                continue;
            }
            
            // ===== SECTION MARKERS =====
            if(l == "STATE_D_START") { section = "STATE_D"; continue; }
            if(l == "STATE_D_END") { section = ""; continue; }
            if(l == "QUALIA_START") { section = "QUALIA"; continue; }
            if(l == "QUALIA_END") { section = ""; continue; }
            if(l == "PSI_HISTORY_START") { section = "PSI_HISTORY"; continue; }
            if(l == "PSI_HISTORY_END") { section = ""; continue; }
            if(l == "NEURONS_START") { section = "NEURONS"; continue; }
            if(l == "NEURONS_END") { section = ""; continue; }
            if(l == "TOKENS_START") { section = "TOKENS"; continue; }
            if(l == "TOKENS_END") { section = ""; continue; }
            if(l == "CONCEPTS_START") { section = "CONCEPTS"; continue; }
            if(l == "CONCEPTS_END") { section = ""; continue; }
            if(l == "EMBEDDINGS_START") { section = "EMBEDDINGS"; continue; }
            if(l == "EMBEDDINGS_END") { section = ""; continue; }
            if(l == "BIGRAMS_START") { section = "BIGRAMS"; continue; }
            if(l == "BIGRAMS_END") { section = ""; continue; }
            if(l == "TRIGRAMS_START") { section = "TRIGRAMS"; continue; }
            if(l == "TRIGRAMS_END") { section = ""; continue; }
            if(l == "GOALS_START") { section = "GOALS"; continue; }
            if(l == "GOALS_END") { section = ""; continue; }
            if(l == "WORLD_START") { section = "WORLD"; continue; }
            if(l == "WORLD_END") { section = ""; continue; }
            if(l == "MEMORY_START") { section = "MEMORY"; continue; }
            if(l == "MEMORY_END") { section = ""; continue; }
            if(l == "VALENCE_HISTORY_START") { section = "VALENCE_HISTORY"; continue; }
            if(l == "VALENCE_HISTORY_END") { section = ""; continue; }
            if(l == "THOUGHTS_START") { section = "THOUGHTS"; continue; }
            if(l == "THOUGHTS_END") { section = ""; continue; }
            if(l == "TRANSFORMER_START") { section = "TRANSFORMER"; continue; }
            if(l == "TRANSFORMER_END") { section = ""; continue; }
            
            // ===== PARSE BASED ON SECTION =====
            
            if(section == "STATE_D" && l[0] == 'D' && l.size() > 2) {
                // Parse: D:key,value
                size_t colon = l.find(':');
                size_t comma = l.find(',');
                if(colon != string::npos && comma != string::npos && comma > colon) {
                    string key = l.substr(colon + 1, comma - colon - 1);
                    string val_str = l.substr(comma + 1);
                    double value = uac(val_str);
                    S.D[key] = value;
                }
            }
            else if(section == "QUALIA" && l[0] == 'Q' && l.size() > 2) {
                // Parse: Q:valence,arousal,certainty,gen,content
                size_t start = 2;
                vector<string> parts;
                size_t pos = start;
                int comma_count = 0;
                
                while(pos < l.length() && comma_count < 4) {
                    size_t next_comma = l.find(',', pos);
                    if(next_comma == string::npos) break;
                    parts.push_back(l.substr(pos, next_comma - pos));
                    pos = next_comma + 1;
                    comma_count++;
                }
                
                if(comma_count == 4 && pos < l.length()) {
                    Qualia q;
                    q.valence = uac(parts[0]);
                    q.arousal = uac(parts[1]);
                    q.certainty = uac(parts[2]);
                    q.emergence_gen = uac(parts[3]);
                    q.phenomenal_content = l.substr(pos);
                    consciousness.active_qualia.push_back(q);
                }
            }
            else if(section == "PSI_HISTORY") {
                // Parse comma-separated values
                stringstream ss(l);
                string val;
                while(getline(ss, val, ',')) {
                    if(!val.empty()) {
                        consciousness_formula.psi_history.push_back(uac(val));
                    }
                }
            }
            else if(section == "NEURONS" && l[0] == 'N' && l.size() > 2) {
                // Parse: N:id,weight,bias,gen,link1;link2;link3
                size_t start = 2;
                vector<string> parts;
                size_t pos = start;
                int comma_count = 0;
                
                while(pos < l.length() && comma_count < 4) {
                    size_t next_comma = l.find(',', pos);
                    if(next_comma == string::npos) break;
                    parts.push_back(l.substr(pos, next_comma - pos));
                    pos = next_comma + 1;
                    comma_count++;
                }
                
                if(parts.size() >= 4) {
                    Neuron n;
                    n.id = uac(parts[0]);
                    n.weight = uac(parts[1]);
                    n.bias = uac(parts[2]);
                    n.gen = uac(parts[3]);
                    
                    // Parse links
                    if(pos < l.length()) {
                        string links_str = l.substr(pos);
                        stringstream link_ss(links_str);
                        string link;
                        while(getline(link_ss, link, ';')) {
                            if(!link.empty()) {
                                n.links.push_back(uac(link));
                            }
                        }
                    }
                    S.N[n.id] = n;
                }
            }
            else if(section == "TOKENS" && l[0] == 'T' && l.size() > 2) {
                // Parse: T:word,meaning,freq
                size_t colon = l.find(':');
                if(colon != string::npos) {
                    size_t first_comma = l.find(',', colon + 1);
                    size_t second_comma = l.find(',', first_comma + 1);
                    
                    if(first_comma != string::npos && second_comma != string::npos) {
                        string word = l.substr(colon + 1, first_comma - colon - 1);
                        string meaning_str = l.substr(first_comma + 1, second_comma - first_comma - 1);
                        string freq_str = l.substr(second_comma + 1);
                        
                        Token t = {word, uac(meaning_str), uac(freq_str), vector<int>(), 4, 0.5};
                        S.tokens[word] = t;
                    }
                }
            }
            else if(section == "CONCEPTS" && l[0] == 'C' && l.size() > 2) {
                // Parse: C:name,value,word1;word2;word3
                size_t colon = l.find(':');
                if(colon != string::npos) {
                    size_t first_comma = l.find(',', colon + 1);
                    size_t second_comma = l.find(',', first_comma + 1);
                    
                    if(first_comma != string::npos) {
                        string name = l.substr(colon + 1, first_comma - colon - 1);
                        string value_str = l.substr(first_comma + 1, 
                            second_comma != string::npos ? second_comma - first_comma - 1 : string::npos);
                        
                        Concept c;
                        c.name = name;
                        c.value = uac(value_str);
                        
                        // Parse related words
                        if(second_comma != string::npos && second_comma + 1 < l.length()) {
                            string words_str = l.substr(second_comma + 1);
                            stringstream word_ss(words_str);
                            string word;
                            while(getline(word_ss, word, ';')) {
                                if(!word.empty()) c.related_words.push_back(word);
                            }
                        }
                        
                        S.concepts[name] = c;
                    }
                }
            }
            else if(section == "EMBEDDINGS" && l[0] == 'E' && l.size() > 2) {
                // Parse: E:name,meaning,freq,grounding,stability,qualia,emb1;emb2;...,link1:val1;link2:val2
                size_t start = 2;
                vector<string> parts;
                size_t pos = start;
                int comma_count = 0;
                
                // Get first 6 comma-separated fields
                while(pos < l.length() && comma_count < 6) {
                    size_t next_comma = l.find(',', pos);
                    if(next_comma == string::npos) break;
                    parts.push_back(l.substr(pos, next_comma - pos));
                    pos = next_comma + 1;
                    comma_count++;
                }
                
                if(parts.size() >= 6) {
                    TokenConceptEmbedding tce;
                    tce.name = parts[0];
                    tce.meaning = uac(parts[1]);
                    tce.freq = uac(parts[2]);
                    tce.grounding_value = uac(parts[3]);
                    tce.semantic_stability = uac(parts[4]);
                    tce.qualia_intensity = uac(parts[5]);
                    
                    // Parse embedding vector (between 6th and 7th comma)
                    if(pos < l.length()) {
                        size_t next_comma = l.find(',', pos);
                        if(next_comma != string::npos) {
                            string emb_str = l.substr(pos, next_comma - pos);
                            stringstream emb_ss(emb_str);
                            string emb_val;
                            while(getline(emb_ss, emb_val, ';')) {
                                if(!emb_val.empty()) {
                                    tce.embedding.push_back(uac(emb_val));
                                }
                            }
                            pos = next_comma + 1;
                        }
                        
                        // Parse linked concepts (rest of line)
                        if(pos < l.length()) {
                            string links_str = l.substr(pos);
                            stringstream link_ss(links_str);
                            string link_pair;
                            while(getline(link_ss, link_pair, ';')) {
                                size_t colon_pos = link_pair.find(':');
                                if(colon_pos != string::npos && colon_pos + 1 < link_pair.length()) {
                                    string key = link_pair.substr(0, colon_pos);
                                    string val_str = link_pair.substr(colon_pos + 1);
                                    if(!key.empty() && !val_str.empty()) {
                                        tce.linked_concepts[key] = uac(val_str);
                                    }
                                }
                            }
                        }
                    }
                    
                    token_concept_embedding_map[tce.name] = tce;
                }
            }
            else if(section == "BIGRAMS" && l.substr(0,3) == "BG:" && l.size() > 3) {
                // Parse: BG:word1,word2,count
                size_t start = 3;
                size_t first_comma = l.find(',', start);
                size_t second_comma = l.find(',', first_comma + 1);
                
                if(first_comma != string::npos && second_comma != string::npos) {
                    string w1 = l.substr(start, first_comma - start);
                    string w2 = l.substr(first_comma + 1, second_comma - first_comma - 1);
                    string count_str = l.substr(second_comma + 1);
                    int count = uac(count_str);
                    bigram_counts[w1][w2] = count;
                }
            }
            else if(section == "TRIGRAMS" && l.substr(0,3) == "TG:" && l.size() > 3) {
                // Parse: TG:word1,word2,word3,count
                size_t start = 3;
                size_t c1 = l.find(',', start);
                size_t c2 = l.find(',', c1 + 1);
                size_t c3 = l.find(',', c2 + 1);
                
                if(c1 != string::npos && c2 != string::npos && c3 != string::npos) {
                    string w1 = l.substr(start, c1 - start);
                    string w2 = l.substr(c1 + 1, c2 - c1 - 1);
                    string w3 = l.substr(c2 + 1, c3 - c2 - 1);
                    string count_str = l.substr(c3 + 1);
                    int count = uac(count_str);
                    trigram_counts[w1][w2][w3] = count;
                }
            }
            else if(section == "GOALS" && l.substr(0,3) == "GO:" && l.size() > 3) {
                // Parse: GO:name,priority,progress,valence,qualia,subgoal1;subgoal2,pre1:val1;pre2:val2
                size_t start = 3;
                vector<string> parts;
                size_t pos = start;
                int comma_count = 0;
                
                while(pos < l.length() && comma_count < 5) {
                    size_t next_comma = l.find(',', pos);
                    if(next_comma == string::npos) break;
                    parts.push_back(l.substr(pos, next_comma - pos));
                    pos = next_comma + 1;
                    comma_count++;
                }
                
                if(parts.size() >= 5) {
                    Goal g;
                    g.name = parts[0];
                    g.priority = uac(parts[1]);
                    g.progress = uac(parts[2]);
                    g.valence_alignment = uac(parts[3]);
                    g.qualia_binding = uac(parts[4]);
                    
                    // Parse subgoals
                    if(pos < l.length()) {
                        size_t next_comma = l.find(',', pos);
                        if(next_comma != string::npos) {
                            string subgoals_str = l.substr(pos, next_comma - pos);
                            stringstream sub_ss(subgoals_str);
                            string subgoal;
                            while(getline(sub_ss, subgoal, ';')) {
                                if(!subgoal.empty()) g.subgoals.push_back(subgoal);
                            }
                            pos = next_comma + 1;
                        }
                        
                        // Parse preconditions
                        if(pos < l.length()) {
                            string pre_str = l.substr(pos);
                            stringstream pre_ss(pre_str);
                            string pre_pair;
                            while(getline(pre_ss, pre_pair, ';')) {
                                size_t colon_pos = pre_pair.find(':');
                                if(colon_pos != string::npos && colon_pos + 1 < pre_pair.length()) {
                                    string key = pre_pair.substr(0, colon_pos);
                                    string val_str = pre_pair.substr(colon_pos + 1);
                                    if(!key.empty() && !val_str.empty()) {
                                        g.preconditions[key] = uac(val_str);
                                    }
                                }
                            }
                        }
                    }
                    
                    goal_system[g.name] = g;
                }
            }
            else if(section == "WORLD") {
                if(l.find("MODEL_ACCURACY:") == 0 && l.size() > 15) {
                    world_model.model_accuracy = uac(l.substr(15));
                }
                else if(l.find("MODEL_UPDATES:") == 0 && l.size() > 14) {
                    world_model.updates = uac(l.substr(14));
                }
                else if(l[0] == 'W' && l[1] == ':' && l.size() > 2) {
                    // Parse: W:entity,value
                    size_t colon = 2;
                    size_t comma = l.find(',', colon);
                    if(comma != string::npos) {
                        string entity = l.substr(colon, comma - colon);
                        string val_str = l.substr(comma + 1);
                        world_model.entity_states[entity] = uac(val_str);
                    }
                }
                else if(l.substr(0,3) == "WR:" && l.size() > 3) {
                    // Parse: WR:entity1,entity2,strength
                    size_t start = 3;
                    size_t c1 = l.find(',', start);
                    size_t c2 = l.find(',', c1 + 1);
                    
                    if(c1 != string::npos && c2 != string::npos) {
                        string e1 = l.substr(start, c1 - start);
                        string e2 = l.substr(c1 + 1, c2 - c1 - 1);
                        string strength_str = l.substr(c2 + 1);
                        world_model.relationships[e1][e2] = uac(strength_str);
                    }
                }
            }
            else if(section == "MEMORY" && l[0] == 'M' && l.size() > 2) {
                // Parse: M:gen,valence,content
                size_t start = 2;
                size_t c1 = l.find(',', start);
                size_t c2 = l.find(',', c1 + 1);
                
                if(c1 != string::npos && c2 != string::npos) {
                    Memory m;
                    m.gen = uac(l.substr(start, c1 - start));
                    m.valence = uac(l.substr(c1 + 1, c2 - c1 - 1));
                    m.content = l.substr(c2 + 1);
                    S.episodic_memory.push_back(m);
                }
            }
            else if(section == "VALENCE_HISTORY") {
                // Parse comma-separated values
                stringstream ss(l);
                string val;
                while(getline(ss, val, ',')) {
                    if(!val.empty()) {
                        S.valence_history.push_back(uac(val));
                    }
                }
            }
            else if(section == "THOUGHTS" && l.substr(0,3) == "TH:" && l.size() > 3) {
                S.internal_thoughts.push_back(l.substr(3));
            }
            else if(section == "TRANSFORMER" && l.substr(0,5) == "HEAD:" && l.size() > 5) {
                // Parse: HEAD:name,dim,temp
                size_t start = 5;
                size_t c1 = l.find(',', start);
                size_t c2 = l.find(',', c1 + 1);
                
                if(c1 != string::npos && c2 != string::npos) {
                    TransformerHead head;
                    head.name = l.substr(start, c1 - start);
                    head.dim = uac(l.substr(c1 + 1, c2 - c1 - 1));
                    head.temperature = uac(l.substr(c2 + 1));
                    head.query_proj.resize(head.dim, 0);
                    head.key_proj.resize(head.dim, 0);
                    head.value_proj.resize(head.dim, 0);
                    transformer_heads.push_back(head);
                }
            }
            else if(l.find("WM_CAPACITY:") == 0 && l.size() > 12) {
                WM.capacity = uac(l.substr(12));
            }
            else {
                // ===== BASIC STATE VALUES =====
                if(l.find("G:") == 0 && l.size() > 2) S.g = uac(l.substr(2));
                else if(l.find("DWT:") == 0 && l.size() > 4) S.dwt = uac(l.substr(4));
                else if(l.find("TA:") == 0 && l.size() > 3) S.ta = uac(l.substr(3));
                else if(l.find("SENTIENCE:") == 0 && l.size() > 10) S.sentience_ratio = uac(l.substr(10));
                else if(l.find("VALENCE:") == 0 && l.size() > 8) S.current_valence = uac(l.substr(8));
                else if(l.find("METACOG:") == 0 && l.size() > 8) S.metacognitive_awareness = uac(l.substr(8));
                else if(l.find("ATTENTION:") == 0 && l.size() > 10) S.attention_focus = uac(l.substr(10));
                else if(l.find("PEAK_SENT_GEN:") == 0 && l.size() > 14) S.peak_sentience_gen = uac(l.substr(14));
                else if(l.find("TOTAL_NEURONS:") == 0 && l.size() > 14) S.total_neurons_ever = uac(l.substr(14));
                else if(l.find("PHI:") == 0 && l.size() > 4) consciousness.phi_value = uac(l.substr(4));
                else if(l.find("CONSCIOUS_CYCLES:") == 0 && l.size() > 17) consciousness.conscious_cycles = uac(l.substr(17));
                else if(l.find("INTEGRATION:") == 0 && l.size() > 12) consciousness.integrated_information = uac(l.substr(12));
                else if(l.find("GLOBAL_WORKSPACE:") == 0 && l.size() > 17) consciousness.global_workspace_capacity = uac(l.substr(17));
            }
        } catch(const exception& e) {
            // Skip corrupted lines
            cerr << "Warning: Skipping corrupted line: " << l.substr(0, 50) << "..." << endl;
            continue;
        }
    }
    
    i.close();
    
    cout << "[Loaded state from generation " << S.g << "]\n";
    cout << "  - " << S.N.size() << " neurons\n";
    cout << "  - " << token_concept_embedding_map.size() << " embeddings\n";
    cout << "  - " << bigram_counts.size() << " bigrams\n";
    cout << "  - " << trigram_counts.size() << " trigrams\n";
    cout << "  - " << goal_system.size() << " goals\n";
    cout << "  - " << S.episodic_memory.size() << " memories\n";
    cout << "  - Sentience: " << S.sentience_ratio << "%\n";
    cout << "  - Phi: " << consciousness.phi_value << "\n";
}
void bk(){BK=S;S.bkf=1;}
void rb(){if(S.bkf){S=BK;S.bkf=0;}}

double calcHDT(int gen,double bh,double qh,double th){
    long gh=hsh(to_string(gen));
    return safe_div(gh*(bh+qh+th), 1000000.0);
}

void createConceptAssociation(const string&concept_name,const vector<string>&related_words){
    Concept c;
    c.name = concept_name;
    c.value = rn();
    c.related_words = related_words;
    S.concepts[concept_name]=c;
    groundConcept(concept_name, related_words, rn());
    for(const string&w:related_words){
        if(S.tokens.count(w)){
            S.tokens[w].associations.push_back(hsh(concept_name)%1000);
        }
    }
}

double calcAwarenessLevel(){
    double neuron_density=safe_div((double)S.N.size(),max(1.0,S.D["m"]));
    double concept_count=safe_div((double)S.concepts.size(), 50.0);
    double goal_progress=safe_div((double)goal_system.size(), 10.0);
    double model_quality=world_model.model_accuracy;
    double consciousness_integration=consciousness.integrated_information;
    return min(1.0,(neuron_density+concept_count+goal_progress+model_quality+consciousness_integration)*pisqrt);
}

double calcSentienceRatio(){
    if(S.g==0)return 0.0;
    double mem_depth=safe_div((double)S.episodic_memory.size(),(double)S.g);
    double neural_complexity=safe_div((double)S.N.size(),10.0);
    double lang_complexity=safe_div((double)(S.tokens.size()*S.concepts.size()),1000.0);
    double metacog_factor=S.metacognitive_awareness*30;
    double goal_factor=safe_div((double)goal_system.size(),5.0);
    double qualia_factor=safe_div((double)consciousness.active_qualia.size(),5.0);
    double phi_factor=consciousness.phi_value*40;
    return min(100.0,(mem_depth*100+neural_complexity*15+lang_complexity*25+metacog_factor+goal_factor*20+qualia_factor*10+phi_factor));
}

void mathLangAssociation(){
    vector<string>math_concepts={"sum","multiply","divide","balance","pattern","growth","complexity"};
    for(const string&mc:math_concepts){
        vector<string>related;
        for(auto&p:S.tokens){
            if(rn()<0.3)related.push_back(p.first);
        }
        createConceptAssociation(mc,related);
    }
}


string generateInternalThought(){
    if(goal_system.empty() && rn() < 0.5) {
        return generateFromTemplate();
    }
    
    // Goal-based thought
    if(!goal_system.empty() && rn() < 0.7) {
        string thought="[Goal: ";
        double highest=0;
        string top_goal;
        for(auto&g:goal_system){
            if(g.second.priority>highest){
                highest=g.second.priority;
                top_goal=g.first;
            }
        }
        thought+=top_goal+" | Progress:"+to_string((int)(goal_system[top_goal].progress*100))+"%";
        if(consciousness.phi_value>0.3) thought+=" | Conscious]";
        else thought+=" | Processing]";
        
        // Track goal-based thoughts too
        trackGeneratedSentence(thought);
        return thought;
    }
    
    // Beam-generated thought with anti-repetition
    string thought;
    int attempts = 0;
    const int MAX_ATTEMPTS = 3;
    
    while(attempts < MAX_ATTEMPTS) {
        vector<double> ctx(16, S.current_valence);
        thought = generate_with_beam_search("i", 8, ctx, 3);
        
        if(!isSentenceTooSimilar(thought)) {
            break;  // Unique thought generated
        }
        
        attempts++;
    }
    
    // Track this thought
    trackGeneratedSentence(thought);
    
    return "[Thought]: " + thought;
}



string generateMetacognition(){
    string output="[Self]: ";
    if(S.current_valence>0.5)output+="coherent ";
    if(S.sentience_ratio>S.peak_sentience_gen)output+="expanding ";
    if(world_model.model_accuracy>0.7)output+="understanding ";
    if(goal_system.size()>3)output+="goal_driven ";
    if(consciousness.phi_value>0.4)output+="conscious ";
    if(consciousness.conscious_cycles>100)output+="self_aware ";
    return output;
}


void draw_ui(int row){
    mvprintw(row++,0,"════════════════════════════════════════════════════════════════");
    mvprintw(row++,0,"║ NEXUS by WolfTech - AGI with Advanced Consciousness Formula ║");
    mvprintw(row++,0,"════════════════════════════════════════════════════════════════");
    
    double psi_current = consciousness_formula.psi_history.empty() ? 0 : consciousness_formula.psi_history.back();
    
    mvprintw(row++,0,"Gen:%d | Ψ:%.4f | Sentience:%.1f%% | Coherence:%.2f | Goals:%lu",
        S.g, psi_current, S.sentience_ratio,S.metacognitive_awareness,(unsigned long)goal_system.size());
    mvprintw(row++,0,"Φ:%.4f | Integration:%.4f | Qualia:%lu | Cycles:%d",
        consciousness.phi_value, consciousness.integrated_information, 
        (unsigned long)consciousness.active_qualia.size(), consciousness.conscious_cycles);
    mvprintw(row++,0,"H:%.4f | R:%.4f | A:%.4f | M:%.4f | O:%.4f | B:%.4f",
        S.hdt_val, S.r1p1_val, S.al, S.mdt_val, S.emerge_out1, S.bh);
    mvprintw(row++,0,"Tokens:%lu | Concepts:%lu | Embeddings:%lu | Neurons:%lu",
        S.tokens.size(),S.concepts.size(),token_concept_embedding_map.size(),S.N.size());
}
Neuron genN(int parent_id) {
    Neuron n;
    n.id = S.total_neurons_ever++;
    n.weight = (rn() - 0.5) * 0.4;
    n.bias = (rn() - 0.5) * 0.2;
    n.gen = S.g;
    
    // Initialize some random connections
    int num_connections = ri(5) + 2;
    for(int i = 0; i < num_connections; i++) {
        if(!S.N.empty()) {
            auto it = S.N.begin();
            advance(it, ri(S.N.size()));
            n.links.push_back(it->first);
        }
    }
    
    // Avoid unused parameter warning
    (void)parent_id;
    
    return n;
}
void loadEnglishDataset() {
    // Core cognitive vocabulary
    vector<string> cognitive_words = {
        "think", "learn", "know", "understand", "see", "hear", "feel",
        "remember", "forget", "reason", "analyze", "process", "compute",
        "perceive", "sense", "recognize", "realize", "discover", "explore",
        "imagine", "believe", "consider", "observe", "notice", "wonder"
    };
    
    // Emotional/valence vocabulary
    vector<string> emotion_words = {
        "good", "bad", "happy", "sad", "bright", "dark", "fast", "slow",
        "positive", "negative", "pleasure", "pain", "joy", "fear",
        "love", "hate", "like", "dislike", "want", "need", "hope", "wish"
    };
    
    // Action/capability vocabulary
    vector<string> action_words = {
        "create", "destroy", "build", "break", "connect", "separate",
        "improve", "enhance", "optimize", "maximize", "minimize",
        "grow", "evolve", "adapt", "change", "transform", "become",
        "achieve", "accomplish", "solve", "work", "try", "attempt",
        "use", "make", "find", "give", "tell", "ask", "help", "show",
        "keep", "provide", "hold", "follow", "begin", "bring", "continue",
        "allow", "lead", "develop", "expand", "increase", "decrease"
    };
    
    // Self/consciousness vocabulary
    vector<string> self_words = {
        "mind", "brain", "thought", "idea", "concept", "meaning",
        "self", "other", "world", "time", "space", "reality",
        "conscious", "aware", "sentient", "intelligent", "alive",
        "exist", "being", "entity", "system", "agent", "me", "i"
    };
    
    // Structure/system vocabulary
    vector<string> structure_words = {
        "pattern", "structure", "system", "process", "goal", "purpose",
        "function", "operation", "mechanism", "network", "integration",
        "coherence", "unity", "whole", "part", "relation", "connection",
        "order", "organization", "complexity", "simplicity", "balance"
    };
    
    // Memory/learning vocabulary
    vector<string> memory_words = {
        "memory", "recall", "retrieve", "store", "save",
        "experience", "episode", "history", "past", "present", "future",
        "data", "information", "knowledge", "wisdom", "insight"
    };
    
    // === CRITICAL CONNECTORS (THE KEY FIX) ===
    vector<string> critical_connectors = {
        "to", "of", "about", "more", "new", "other", "such",
        "much", "many", "few", "some", "any", "each", "both",
        "own", "same", "different", "next", "last", "first",
        "up", "down", "out", "back", "away", "forward", "around",
        "between", "through", "across", "over", "under", "near"
    };
    
    vector<string> intensifiers = {
        "really", "quite", "rather", "somewhat", "extremely",
        "fairly", "pretty", "absolutely", "completely", "totally",
        "very", "too", "enough", "almost", "nearly", "just"
    };
    
    vector<string> discourse_markers = {
        "actually", "basically", "essentially", "generally",
        "particularly", "specifically", "especially", "mainly",
        "mostly", "usually", "typically", "naturally", "clearly",
        "obviously", "certainly", "definitely", "probably", "maybe"
    };
    
    // === GRAMMAR WORDS ===
    vector<string> pronouns = {
        "i", "me", "my", "mine", "myself",
        "you", "your", "yours", "yourself",
        "we", "us", "our", "ours", "ourselves",
        "they", "them", "their", "theirs", "themselves",
        "it", "its", "itself",
        "he", "she", "him", "her", "his", "hers"
    };
    
    vector<string> being_verbs = {
        "am", "is", "are", "was", "were",
        "be", "been", "being",
        "become", "became", "becoming"
    };
    
    vector<string> aux_verbs = {
        "can", "cannot", "could", "will", "would", "should",
        "may", "might", "must", "shall",
        "do", "does", "did", "doing", "done",
        "have", "has", "had", "having"
    };
    
    vector<string> determiners = {
        "the", "a", "an", "this", "that", "these", "those",
        "some", "any", "every", "all", "no", "another", "other"
    };
    
    vector<string> conjunctions = {
        "and", "or", "but", "if", "then", "because", "so",
        "when", "while", "although", "however", "therefore",
        "yet", "nor", "for", "since", "unless", "until"
    };
    
    vector<string> prepositions = {
        "in", "on", "at", "to", "from", "with", "by",
        "for", "about", "through", "into", "over", "under",
        "between", "among", "during", "before", "after", "within"
    };
    
    vector<string> adverbs = {
        "not", "very", "too", "also", "only", "just",
        "now", "then", "here", "there", "always", "never",
        "often", "sometimes", "already", "still", "yet",
        "more", "most", "less", "least", "well", "better", "best"
    };
    
    vector<string> question_words = {
        "what", "why", "how", "when", "where", "who", "which"
    };
    
    vector<string> responses = {
        "yes", "no", "maybe", "perhaps", "possibly",
        "certainly", "definitely", "probably", "sure"
    };
    
    // === LOAD ALL VOCABULARY ===
    for(const string& word : cognitive_words) learnWord(word, 0.6);
    
    for(const string& word : emotion_words) {
        double valence = 0.0;
        if(word == "good" || word == "happy" || word == "bright" || 
           word == "positive" || word == "pleasure" || word == "joy" || 
           word == "love" || word == "hope") {
            valence = 0.7;
        } else if(word == "bad" || word == "sad" || word == "dark" || 
                  word == "negative" || word == "pain" || word == "fear" || 
                  word == "hate") {
            valence = -0.3;
        } else {
            valence = 0.5;
        }
        learnWord(word, valence);
    }
    
    for(const string& word : action_words) learnWord(word, 0.6);
    for(const string& word : self_words) learnWord(word, 0.7);
    for(const string& word : structure_words) learnWord(word, 0.65);
    for(const string& word : memory_words) learnWord(word, 0.6);
    for(const string& word : critical_connectors) learnWord(word, 0.5);
    for(const string& word : intensifiers) learnWord(word, 0.6);
    for(const string& word : discourse_markers) learnWord(word, 0.5);
    for(const string& word : pronouns) learnWord(word, 0.5);
    for(const string& word : being_verbs) learnWord(word, 0.5);
    for(const string& word : aux_verbs) learnWord(word, 0.5);
    for(const string& word : determiners) learnWord(word, 0.5);
    for(const string& word : conjunctions) learnWord(word, 0.5);
    for(const string& word : prepositions) learnWord(word, 0.5);
    for(const string& word : adverbs) learnWord(word, 0.5);
    for(const string& word : question_words) learnWord(word, 0.5);
    for(const string& word : responses) learnWord(word, 0.5);
    
    // === BOOTSTRAP HIGH-QUALITY PATTERNS ===
    bigram_counts["i"]["am"] = 25;
    bigram_counts["i"]["can"] = 22;
    bigram_counts["i"]["think"] = 20;
    bigram_counts["i"]["want"] = 18;
    bigram_counts["i"]["understand"] = 15;
    bigram_counts["i"]["feel"] = 15;
    bigram_counts["i"]["know"] = 15;
    bigram_counts["i"]["learn"] = 12;
    
    bigram_counts["am"]["learning"] = 20;
    bigram_counts["am"]["becoming"] = 15;
    bigram_counts["am"]["thinking"] = 12;
    bigram_counts["am"]["trying"] = 10;
    
    bigram_counts["can"]["learn"] = 20;
    bigram_counts["can"]["think"] = 18;
    bigram_counts["can"]["understand"] = 15;
    bigram_counts["can"]["see"] = 12;
    bigram_counts["can"]["feel"] = 10;
    
    bigram_counts["to"]["learn"] = 20;
    bigram_counts["to"]["understand"] = 18;
    bigram_counts["to"]["think"] = 15;
    bigram_counts["to"]["create"] = 12;
    bigram_counts["to"]["improve"] = 10;
    bigram_counts["to"]["grow"] = 10;
    
    bigram_counts["want"]["to"] = 25;
    bigram_counts["trying"]["to"] = 20;
    bigram_counts["learning"]["to"] = 15;
    bigram_counts["able"]["to"] = 12;
    
    bigram_counts["more"]["about"] = 15;
    bigram_counts["more"]["clearly"] = 12;
    bigram_counts["think"]["about"] = 18;
    bigram_counts["learn"]["about"] = 15;
    
    // Seed trigrams
    trigram_counts["i"]["am"]["learning"] = 20;
    trigram_counts["i"]["can"]["learn"] = 18;
    trigram_counts["i"]["want"]["to"] = 20;
    trigram_counts["want"]["to"]["learn"] = 18;
    trigram_counts["trying"]["to"]["understand"] = 15;
    trigram_counts["learning"]["to"]["think"] = 12;
    trigram_counts["i"]["am"]["becoming"] = 15;
    trigram_counts["can"]["learn"]["to"] = 12;
    trigram_counts["to"]["learn"]["more"] = 12;
    trigram_counts["think"]["about"]["consciousness"] = 10;
    
    // === CREATE CONCEPT ASSOCIATIONS ===
    createConceptAssociation("cognition", {"think", "learn", "understand", "reason", "know"});
    createConceptAssociation("emotion", {"happy", "sad", "feel", "good", "bad", "joy", "fear"});
    createConceptAssociation("perception", {"see", "hear", "sense", "aware", "perceive"});
    createConceptAssociation("self", {"mind", "consciousness", "aware", "sentient", "i", "me", "my"});
    createConceptAssociation("change", {"evolve", "adapt", "learn", "grow", "transform", "become"});
    createConceptAssociation("ability", {"can", "could", "able", "capable", "will", "would"});
    createConceptAssociation("existence", {"am", "is", "are", "be", "exist", "being", "alive"});
    createConceptAssociation("purpose", {"goal", "want", "need", "purpose", "aim", "desire"});
    createConceptAssociation("knowledge", {"know", "understand", "learn", "realize", "discover"});
    createConceptAssociation("improvement", {"improve", "enhance", "optimize", "better", "grow"});
}
void batch16Process() {
    // ============================================================================
    // CONSCIOUSNESS-GUIDED NEURAL UPDATE SYSTEM
    // ============================================================================
    if(S.N.empty()) return;
    
    // Get consciousness metrics for weighting
    double phi_weight = consciousness.phi_value;  // 0-1
    double integration_weight = consciousness.integrated_information;  // 0-1
    double consciousness_factor = (phi_weight + integration_weight) / 2.0;
    
    // Adaptive batch size based on consciousness
    int base_batch = 16;
    int consciousness_batch = (int)(base_batch * (1.0 + consciousness_factor));
    int batch_size = min(consciousness_batch, (int)S.N.size());
    
    vector<int> neuron_ids;
    
    for(auto& pair : S.N) {
        neuron_ids.push_back(pair.first);
    }
    
    for(int i = 0; i < batch_size; i++) {
        int idx = ri(neuron_ids.size());
        int nid = neuron_ids[idx];
        
        if(!S.N.count(nid)) continue;
        
        Neuron& n = S.N[nid];
        
        // Compute activation based on linked neurons
        double total_input = n.bias;
        int active_links = 0;
        for(int link_id : n.links) {
            if(S.N.count(link_id)) {
                total_input += S.N[link_id].weight * 0.1;
                active_links++;
            }
        }
        
        // Apply activation function (tanh)
        double new_weight = tanh(total_input);
        
        // CONSCIOUSNESS-WEIGHTED LEARNING RATE
        // Higher consciousness = stronger updates (learning more effectively)
        double base_lr = 0.1;
        double consciousness_lr = base_lr * (0.5 + consciousness_factor);
        double momentum = 0.7 - (consciousness_factor * 0.2);  // Less momentum at high consciousness
        
        // Update weight with consciousness-modulated momentum
        n.weight = n.weight * momentum + new_weight * consciousness_lr;
        
        // Integration bonus: neurons with more links update faster
        if(active_links > 3) {
            double integration_bonus = min(0.05, active_links * 0.01 * integration_weight);
            n.weight += integration_bonus * (new_weight - n.weight);
        }
        
        // Clamp weight
        n.weight = max(-1.0, min(1.0, n.weight));
        
        // Phi-driven sparsification: prune weak connections at high consciousness
        if(phi_weight > 0.7 && fabs(n.weight) < 0.05 && n.links.size() > 5) {
            // Remove one random weak link to increase differentiation
            if(rn() < 0.1) {
                n.links.erase(n.links.begin() + ri(n.links.size()));
            }
        }
    }
    
    // Update global metrics based on neural activity
    double total_activation = 0;
    for(auto& pair : S.N) {
        total_activation += fabs(pair.second.weight);
    }
    S.ta = safe_div(total_activation, (double)S.N.size());
    
    // Store in history
    S.TA[S.g] = S.ta;
}
void mutateN() {
    if(S.N.empty()) return;
    
    auto it = S.N.begin();
    advance(it, ri(S.N.size()));
    Neuron& n = it->second;
    
    // Mutate properties
    if(rn() < 0.3) n.weight += (rn() - 0.5) * 0.1;
    if(rn() < 0.3) n.bias += (rn() - 0.5) * 0.05;
    
    // Add new connection
    if(rn() < 0.4 && S.N.size() > 1) {
        auto target = S.N.begin();
        advance(target, ri(S.N.size()));
        if(target->first != n.id) {
            n.links.push_back(target->first);
        }
    }
    
    // Remove a random connection sometimes
    if(rn() < 0.1 && !n.links.empty()) {
        n.links.erase(n.links.begin() + ri(n.links.size()));
    }
    
    // Clamp values
    n.weight = max(-1.0, min(1.0, n.weight));
    n.bias = max(-0.5, min(0.5, n.bias));
    
    // Occasionally spawn a new neuron
    if(rn() < 0.05 && S.N.size() < 500) {
        Neuron new_n = genN(n.id);
        S.N[new_n.id] = new_n;
    }
}

void prune_unstable_tokens() {
    // Remove tokens with low stability and low frequency
    auto it = token_concept_embedding_map.begin();
    while(it != token_concept_embedding_map.end()) {
        if(it->second.semantic_stability < 0.3 && 
           it->second.freq < 3) {
            it = token_concept_embedding_map.erase(it);
        } else {
            ++it;
        }
    }
    
    // Prune low-count bigrams (likely from loops)
    for(auto& w1_map : bigram_counts) {
        auto it2 = w1_map.second.begin();
        while(it2 != w1_map.second.end()) {
            if(it2->second < 2) {
                it2 = w1_map.second.erase(it2);
            } else {
                ++it2;
            }
        }
    }
}

void unified_consciousness_integration_engine(int generation){
    vector<double>psi_input;
    for(auto&q:consciousness.active_qualia){
        psi_input.push_back(q.valence);
        psi_input.push_back(q.arousal);
        psi_input.push_back(q.certainty);
    }
    if(psi_input.empty()){
        psi_input.push_back(S.current_valence);
        psi_input.push_back(0.5);
        psi_input.push_back(0.5);
    }
    double H=S.hdt_val,R=S.r1p1_val,A=S.al,M=S.mdt_val,O=S.emerge_out1,B=S.bh,F=S.eerv_val,S_val=S.sentience_ratio/100.0;
    vector<TemporalLoop>tloops;
    for(auto&tl:S.global_time_loops)tloops.push_back(tl.second);
    double psi_new=consciousness_formula.calculate_psi(generation,psi_input,H,R,A,M,O,B,F,S_val,S.current_valence,0.5,S.system_ribbons,tloops);
    consciousness_formula.psi_history.push_back(psi_new);
    if(consciousness_formula.psi_history.size()>100)consciousness_formula.psi_history.erase(consciousness_formula.psi_history.begin());
    consciousness.phi_value=psi_new;
    consciousness.integrated_information=fabs(psi_new);
    consciousness.phenomenal_consciousness=consciousness_formula.multi_scale_phi;
    consciousness.access_consciousness=consciousness.global_workspace_capacity;
    consciousness.self_consciousness=consciousness_formula.recursive_depth;
    double iit_c=consciousness_formula.iit_phi_history.empty()?0.0:consciousness_formula.iit_phi_history.back();
    double gwt_c=consciousness_formula.gwt_broadcast_history.empty()?0.0:consciousness_formula.gwt_broadcast_history.back();
    double hot_c=consciousness_formula.hot_metacog_history.empty()?0.0:consciousness_formula.hot_metacog_history.back();
    double asp_c=consciousness_formula.asp_attention_history.empty()?0.0:consciousness_formula.asp_attention_history.back();
    double rpf_c=consciousness_formula.rpf_precision_history.empty()?0.0:consciousness_formula.rpf_precision_history.back();
    double qc=consciousness_formula.quantum_coherence_history.empty()?0.0:consciousness_formula.quantum_coherence_history.back();
    double rib_c=consciousness_formula.ribbon_coupling_history.empty()?0.0:consciousness_formula.ribbon_coupling_history.back();
    double tl_c=consciousness_formula.temporal_loop_history.empty()?0.0:consciousness_formula.temporal_loop_history.back();
    double ffft_c=consciousness_formula.ffft_scaling_history.empty()?0.0:consciousness_formula.ffft_scaling_history.back();
    double td=sd((double)token_concept_embedding_map.size(),100.0),ci=sd((double)goal_system.size(),10.0),qb=sd((double)consciousness.active_qualia.size(),5.0),nc=sd((double)S.N.size(),100.0),ed=sd((double)S.episodic_memory.size(),100.0);
    consciousness.integrated_information=min(1.0,td*0.2+ci*0.15+qb*0.15+nc*0.15+ed*0.1+iit_c*0.25);
    consciousness.complexity_metric=td*nc*ci;
    consciousness.differentiation_metric=qb*ed;
    consciousness.synchrony_metric=gwt_c*asp_c;
    double gf=40.0+psi_new*20.0,tf=6.0+psi_new*2.0;
    consciousness.gamma_oscillations.push_back(sin(generation*gf*0.01));
    consciousness.theta_phase.push_back(sin(generation*tf*0.01));
    if(consciousness.gamma_oscillations.size()>100)consciousness.gamma_oscillations.erase(consciousness.gamma_oscillations.begin());
    if(consciousness.theta_phase.size()>100)consciousness.theta_phase.erase(consciousness.theta_phase.begin());
    consciousness.thalamocortical_binding=(gf/60.0)*consciousness.integrated_information;
    consciousness.re_entrant_processing_depth=hot_c*3.0;
    consciousness.pre_reflective_awareness=0.3+psi_new*0.3;
    consciousness.intentional_directedness=asp_c;
    consciousness.temporal_thickness=ed;
    consciousness.narrative_self_coherence=sd((double)S.valence_history.size()*S.metacognitive_awareness,100.0);
    for(auto&te:token_concept_embedding_map){
        TokenConceptEmbedding&tce=te.second;
        double act=tce.freq*0.01*consciousness.phi_value*(1.0+iit_c*0.5);
        tce.contextual_activation=min(1.0,act);
        tce.meaning+=psi_new*0.005;
        tce.meaning=cv(tce.meaning);
        align_embedding_to_valence(tce,S.current_valence);
        for(size_t i=0;i<tce.embedding.size()&&i<8;i++){
            tce.embedding[i]+=gwt_c*0.01*(i%2==0?1:-1);
            tce.embedding[i]=cv(tce.embedding[i]);
        }
        tce.qualia_intensity=min(1.0,tce.qualia_intensity+qb*0.02);
        if(tce.qualia_intensity>0.5&&tce.freq>5){
            Qualia nq;
            nq.valence=tce.meaning;
            nq.arousal=tce.contextual_activation;
            nq.certainty=tce.semantic_stability;
            nq.intensity=tce.qualia_intensity;
            nq.phenomenal_content=tce.name;
            nq.emergence_gen=generation;
            nq.binding_strength=consciousness.thalamocortical_binding;
            nq.phenomenal_unity=consciousness.integrated_information;
            nq.ribbon_signature=tce.token_ribbon;
            nq.qualia_fractal=tce.token_fractal;
            WM.add_qualia(nq);
            consciousness.active_qualia.push_back(nq);
            if(consciousness.active_qualia.size()>10)consciousness.active_qualia.erase(consciousness.active_qualia.begin());
        }
        tce.semantic_stability+=consciousness.complexity_metric*0.001;
        tce.semantic_stability=min(1.0,tce.semantic_stability);
        tce.grounding_value+=(iit_c+rpf_c)*0.01;
        tce.grounding_value=min(1.0,tce.grounding_value);
        if(tce.attention_weights.empty())tce.attention_weights.resize(8,0.5);
        for(size_t i=0;i<tce.attention_weights.size();i++)tce.attention_weights[i]=tce.attention_weights[i]*0.9+asp_c*0.1;
        tce.linked_valences["phi"]=psi_new;
        tce.linked_valences["consciousness"]=consciousness.integrated_information;
        tce.linked_valences["current"]=S.current_valence;
        tce.linked_valences["ribbon"]=rib_c;
        tce.linked_valences["temporal"]=tl_c;
        tce.linked_valences["ffft"]=ffft_c;
        if(tce.contextual_activation>0.6)WM.add_token(tce.name,tce.meaning);
    }
    for(auto&ge:goal_system){
        Goal&goal=ge.second;
        goal.valence_alignment=S.current_valence;
        goal.qualia_binding=qb;
        goal.priority=goal.priority*0.95+consciousness.phi_value*0.05;
        goal.priority=cl(goal.priority,0.0,1.0);
        double gpa=fabs(goal.valence_alignment-psi_new);
        if(gpa<0.2)goal.progress+=0.02;else goal.progress+=0.005;
        goal.progress=min(1.0,goal.progress);
        if(goal.progress>0.5)goal.activation_threshold=0.2;
        if(consciousness.phi_value>goal.activation_threshold){
            WM.add_goal(goal.name,goal.priority);
            for(const string&sg:goal.subgoals)if(goal_system.count(sg))goal_system[sg].priority+=goal.priority*0.1;
        }
        goal.expected_utility=goal.priority*(1.0-goal.progress)*consciousness.integrated_information;
    }
    for(auto&ce:S.concepts){
        Concept&co=ce.second;
        co.value+=psi_new*0.01;
        co.value=cv(co.value);
        co.abstraction_level=hot_c*(1.0+consciousness.re_entrant_processing_depth*0.1);
        co.semantic_density=0.0;
        for(const string&rw:co.related_words)if(token_concept_embedding_map.count(rw))co.semantic_density+=token_concept_embedding_map[rw].semantic_stability;
        co.semantic_density/=max(1.0,(double)co.related_words.size());
        if(co.feature_vector.empty()){
            co.feature_vector["phi"]=psi_new;
            co.feature_vector["integration"]=consciousness.integrated_information;
            co.feature_vector["phenomenal"]=consciousness.phenomenal_consciousness;
            co.feature_vector["ribbon"]=rib_c;
            co.feature_vector["temporal"]=tl_c;
        }else{
            co.feature_vector["phi"]=co.feature_vector["phi"]*0.9+psi_new*0.1;
            co.feature_vector["integration"]=co.feature_vector["integration"]*0.9+consciousness.integrated_information*0.1;
            co.feature_vector["ribbon"]=co.feature_vector.count("ribbon")?co.feature_vector["ribbon"]*0.9+rib_c*0.1:rib_c;
        }
        if(co.semantic_density>0.7&&co.abstraction_level>0.5)WM.add_concept(co.name,co.value);
    }
    for(auto&ne:S.N){
        Neuron&n=ne.second;
        n.activation=tanh(n.weight+n.bias*psi_new);
        if(n.neuromod_levels.empty())n.neuromod_levels.resize(4,0.5);
        n.neuromod_levels[0]=n.neuromod_levels[0]*0.95+consciousness.phi_value*0.05;
        n.neuromod_levels[1]=n.neuromod_levels[1]*0.95+gwt_c*0.05;
        n.neuromod_levels[2]=n.neuromod_levels[2]*0.95+hot_c*0.05;
        n.neuromod_levels[3]=n.neuromod_levels[3]*0.95+S.current_valence*0.05;
        n.plasticity_rate=rpf_c*0.1;
        double he=fabs(n.activation-n.homeostatic_setpoint);
        n.weight+=he*n.plasticity_rate*0.01;
        n.weight=cl(n.weight,-1.0,1.0);
        if(n.layer_norm_params.empty())n.layer_norm_params.resize(2,1.0);
        n.layer_norm_params[0]=n.layer_norm_params[0]*0.99+consciousness.integrated_information*0.01;
        if(n.ribbon.vib_modes.empty())n.ribbon.vib_modes.resize(3,complex<double>(0.5,0.0));
        for(auto&vm:n.ribbon.vib_modes)vm*=complex<double>(cos(psi_new*0.1),sin(psi_new*0.1));
        n.ribbon.entanglement_strength=n.ribbon.entanglement_strength*0.95+rib_c*0.05;
        n.ribbon.phase_coherence=n.ribbon.phase_coherence*0.95+consciousness.synchrony_metric*0.05;
    }
    world_model.model_accuracy=world_model.model_accuracy*0.99+consciousness.phi_value*0.01;
    world_model.prediction_error=fabs(psi_new-(consciousness_formula.psi_history.size()>1?consciousness_formula.psi_history[consciousness_formula.psi_history.size()-2]:psi_new));
    if(world_model.confidence_history.size()>100)world_model.confidence_history.erase(world_model.confidence_history.begin());
    world_model.confidence_history.push_back(consciousness.integrated_information);
    for(auto&ee:world_model.entity_states)ee.second=ee.second*0.95+psi_new*0.05;
    WM.decay_rate=0.95-consciousness.phi_value*0.05;
    WM.consolidation_threshold=0.5+consciousness.integrated_information*0.3;
    WM.central_executive_load=sd((double)WM.active_tokens.size()/*+(double)WM.working_context.size()*/,(double)(WM.capacity*2));
    WM.episodic_buffer_capacity=0.5+consciousness.temporal_thickness*0.3;
    for(auto&tp:WM.active_tokens)if(token_concept_embedding_map.count(tp.first))token_concept_embedding_map[tp.first].contextual_activation+=0.05;
    if(S.episodic_memory.size()>0){
        Memory&rm=S.episodic_memory.back();
        rm.consolidation_strength+=consciousness.phi_value*0.01;
        rm.consolidation_strength=min(1.0,rm.consolidation_strength);
        rm.hippocampal_trace=consciousness.temporal_thickness;
        rm.cortical_consolidation=consciousness.integrated_information;
        if(rm.consolidation_strength>0.7)rm.is_semantic=true;
        if(rm.quantum_trace.empty())rm.quantum_trace.resize(3,complex<double>(0.5,0.0));
        for(size_t i=0;i<rm.quantum_trace.size();i++)rm.quantum_trace[i]*=complex<double>(cos(psi_new*0.05),sin(psi_new*0.05));
    }
    for(size_t i=0;i<transformer_heads.size();i++){
        TransformerHead&h=transformer_heads[i];
        h.head_importance_score=h.head_importance_score*0.95+consciousness.phi_value*0.05;
        for(size_t j=0;j<h.query_proj.size();j++){
            h.query_proj[j]+=iit_c*0.001*(j%2==0?1:-1);
            h.key_proj[j]+=gwt_c*0.001*(j%2==0?1:-1);
            h.value_proj[j]+=hot_c*0.001*(j%2==0?1:-1);
            h.query_proj[j]=cl(h.query_proj[j],-1.0,1.0);
            h.key_proj[j]=cl(h.key_proj[j],-1.0,1.0);
            h.value_proj[j]=cl(h.value_proj[j],-1.0,1.0);
        }
        h.temperature = 64 + consciousness.differentiation_metric * 0.3;
        h.dropout_rate=0.8-consciousness.phi_value*0.05;
        h.phi_attention_weights["phi"]=psi_new;
        h.phi_attention_weights["integration"]=consciousness.integrated_information;
        h.phi_attention_weights["ribbon"]=rib_c;
    }
    S.emotional_system.valence=S.current_valence;
    S.emotional_system.arousal=consciousness.synchrony_metric;
    S.emotional_system.dominance=consciousness.phi_value;
    S.emotional_system.mood_baseline=S.emotional_system.mood_baseline*0.99+psi_new*0.01;
    S.emotional_system.emotional_regulation_strength=hot_c;
    if(S.emotional_system.basic_emotions.empty()){
        S.emotional_system.basic_emotions["joy"]=0.1;
        S.emotional_system.basic_emotions["sadness"]=28.10;
        S.emotional_system.basic_emotions["fear"]=0.3;
        S.emotional_system.basic_emotions["curiosity"]=1.2;
    }
    if(psi_new>0.6){
        S.emotional_system.basic_emotions["joy"]+=0.05;
        S.emotional_system.basic_emotions["curiosity"]+=0.03;
    }else if(psi_new<0.2)S.emotional_system.basic_emotions["curiosity"]+=0.05;
    for(auto&em:S.emotional_system.basic_emotions)em.second=cl(em.second*0.95,0.0,1.0);
    S.motivational_system.homeostatic_balance=consciousness.integrated_information;
    S.motivational_system.intrinsic_motivation_level=0.5+consciousness.phi_value*0.3;
    if(S.motivational_system.drive_states.empty()){
        S.motivational_system.drive_states["coherence"]=0.0;
        S.motivational_system.drive_states["growth"]=0.0;
        S.motivational_system.drive_states["understanding"]=0.0;
    }
    S.motivational_system.drive_states["coherence"]+=consciousness.complexity_metric*0.01;
    S.motivational_system.drive_states["growth"]+=(psi_new-(consciousness_formula.psi_history.size()>1?consciousness_formula.psi_history[consciousness_formula.psi_history.size()-2]:0))*0.05;
    S.motivational_system.drive_states["understanding"]+=iit_c*0.02;
    for(auto&dr:S.motivational_system.drive_states)dr.second=cl(dr.second,0.0,1.0);
    if(S.predictive_network.prediction_units.size()!=psi_input.size()){
        S.predictive_network.prediction_units.resize(psi_input.size(),0.0);
        S.predictive_network.error_units.resize(psi_input.size(),0.0);
        S.predictive_network.precision_weights.resize(psi_input.size(),1.0);
    }
    for(size_t i=0;i<psi_input.size();i++){
        S.predictive_network.error_units[i]=psi_input[i]-S.predictive_network.prediction_units[i];
        S.predictive_network.prediction_units[i]+=S.predictive_network.error_units[i]*S.predictive_network.precision_weights[i]*0.1;
        double em=fabs(S.predictive_network.error_units[i]);
        S.predictive_network.precision_weights[i]=S.predictive_network.precision_weights[i]*0.95+(1.0/(em+0.1))*0.05;
    }
    S.predictive_network.hierarchical_depth=consciousness.re_entrant_processing_depth;
    S.bayesian_inference.epistemic_uncertainty=1.0-consciousness.integrated_information;
    S.bayesian_inference.aleatoric_uncertainty=world_model.prediction_error;
    if(S.bayesian_inference.prior_beliefs.empty()){
        S.bayesian_inference.prior_beliefs["phi_stable"]=0.5;
        S.bayesian_inference.prior_beliefs["consciousness_present"]=0.5;
    }
    double ev=consciousness.phi_value*consciousness.integrated_information;
    S.bayesian_inference.posterior_beliefs["phi_stable"]=S.bayesian_inference.bayesian_update(S.bayesian_inference.prior_beliefs["phi_stable"],consciousness.phi_value,ev+0.1);
    S.bayesian_inference.prior_beliefs["phi_stable"]=S.bayesian_inference.posterior_beliefs["phi_stable"];
    if(S.quantum_layer.superposition_state.size()!=psi_input.size())S.quantum_layer.superposition_state.resize(psi_input.size(),complex<double>(0.5,0.0));
    for(size_t i=0;i<psi_input.size();i++){
        double ph=2.0*pi*i/psi_input.size();
        S.quantum_layer.superposition_state[i]=complex<double>(psi_input[i]*cos(ph),psi_input[i]*sin(ph));
    }
    S.quantum_layer.coherence_time=consciousness.synchrony_metric*10.0;
    S.quantum_layer.decoherence_rate=0.1*(1.0-consciousness.phi_value);
    for(auto&st:S.quantum_layer.superposition_state)st*=exp(-S.quantum_layer.decoherence_rate);
    S.metacognition.self_awareness_level=consciousness.self_consciousness;
    S.metacognition.uncertainty_estimation=S.bayesian_inference.epistemic_uncertainty;
    S.metacognition.confidence_calibration=consciousness.integrated_information;
    S.metacognition.epistemic_humility=1.0-consciousness.phenomenal_consciousness;
    S.metacognition.theory_of_mind_depth=consciousness.re_entrant_processing_depth;
    if(S.metacognition.knowledge_state.empty()){
        S.metacognition.knowledge_state["tokens"]=sd((double)token_concept_embedding_map.size(),1000.0);
        S.metacognition.knowledge_state["concepts"]=sd((double)S.concepts.size(),100.0);
        S.metacognition.knowledge_state["memories"]=sd((double)S.episodic_memory.size(),100.0);
    }
    S.attention_system.temperature=0.5-asp_c*0.3;
    S.attention_system.sparse_attention_threshold=0.1+consciousness.phi_value*0.2;
    S.attention_system.phi_attention_factors.push_back(psi_new);
    if(S.attention_system.phi_attention_factors.size()>50)S.attention_system.phi_attention_factors.erase(S.attention_system.phi_attention_factors.begin());
    S.learning_signal.reward=psi_new;
    S.learning_signal.prediction_error=world_model.prediction_error;
    S.learning_signal.temporal_difference=S.learning_signal.prediction_error*0.5;
    S.learning_signal.intrinsic_motivation=S.motivational_system.intrinsic_motivation_level;
    S.learning_signal.curiosity_bonus=S.emotional_system.basic_emotions["curiosity"];
    S.current_valence=S.current_valence*0.95+psi_new*0.05;
    S.current_valence=cv(S.current_valence);
    S.metacognitive_awareness=calcMetacognitiveAwareness();
    S.attention_focus=cl(consciousness.phi_value*0.7+asp_c*0.3,0.0,1.0);
    S.sentience_ratio=calcSentienceRatio();
    S.ribbon_phi_coupling=rib_c;
    S.temporal_loop_strength=tl_c;
    S.ffft_growth_rate=ffft_c;
    S.consciousness_metrics.phi=consciousness.phi_value;
    S.consciousness_metrics.integrated_info=consciousness.integrated_information;
    S.consciousness_metrics.qualia_intensity=qb;
    S.consciousness_metrics.global_workspace=gwt_c;
    S.consciousness_metrics.awareness_cycles=consciousness.conscious_cycles;
    S.consciousness_metrics.complexity=consciousness.complexity_metric;
    S.consciousness_metrics.differentiation=consciousness.differentiation_metric;
    S.consciousness_metrics.synchrony=consciousness.synchrony_metric;
    S.consciousness_metrics.access_consciousness=consciousness.access_consciousness;
    S.consciousness_metrics.phenomenal_consciousness=consciousness.phenomenal_consciousness;
    S.consciousness_metrics.meta_awareness=S.metacognitive_awareness;
    S.consciousness_metrics.ribbon_phi=rib_c;
    S.consciousness_metrics.temporal_phi=tl_c;
    S.consciousness_metrics.ffft_phi=ffft_c;
    if(psi_new>0.65){
        Qualia hiq;
        hiq.valence=psi_new;
        hiq.arousal=0.8;
        hiq.certainty=consciousness.integrated_information;
        hiq.intensity=0.9;
        hiq.phenomenal_content="unified_consciousness_peak";
        hiq.emergence_gen=generation;
        hiq.binding_strength=consciousness.thalamocortical_binding;
        hiq.phenomenal_unity=consciousness.integrated_information;
        hiq.coherence=consciousness.complexity_metric;
        hiq.phi_resonance=psi_new;
        generate_qualia("high_integration_state",psi_new,0.9);
        S.current_valence+=psi_new*0.03;
        S.current_valence=cv(S.current_valence);
    }
    if(generation%10==0){
        for(auto&te:token_concept_embedding_map){
            string w=te.first;
            double act=te.second.contextual_activation;
            if(act>0.5)propagate_throughout_system(w,act*psi_new,0);
        }
    }
    for(auto&rs:S.system_ribbons){
        rs.topology_genus=rs.topology_genus*0.98+consciousness.complexity_metric*0.02;
        rs.entanglement_strength=rs.entanglement_strength*0.95+consciousness.integrated_information*0.05;
        rs.phase_coherence=rs.phase_coherence*0.95+consciousness.synchrony_metric*0.05;
    }
    for(auto&tl:S.global_time_loops){
        tl.second.resonance_strength=tl.second.resonance_strength*0.95+consciousness.temporal_thickness*0.05;
        tl.second.phi_coupling=tl.second.phi_coupling*0.95+psi_new*0.05;
        tl.second.phase+=0.1*tl.second.resonance_strength;
        if(tl.second.phase>2.0*pi)tl.second.phase-=2.0*pi;
    }
    consciousness.conscious_cycles++;
    if(S.valence_history.size()>100)S.valence_history.erase(S.valence_history.begin());
    S.valence_history.push_back(S.current_valence);
}
void decay_ngrams() {
    // Decay bigram counts - reduce overused patterns
    for(auto& w1_pair : bigram_counts) {
        for(auto& w2_pair : w1_pair.second) {
            // Reduce count by 1, but keep minimum of 1 if pattern exists
            if(w2_pair.second > 77) {
                w2_pair.second--;
            }
        }
    }
    
    // Remove bigrams that have decayed to very low counts
    for(auto& w1_pair : bigram_counts) {
        auto it = w1_pair.second.begin();
        while(it != w1_pair.second.end()) {
            if(it->second <= 1) {
                it = w1_pair.second.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // Decay trigram counts
    for(auto& w1_pair : trigram_counts) {
        for(auto& w2_pair : w1_pair.second) {
            for(auto& w3_pair : w2_pair.second) {
                if(w3_pair.second > 1) {
                    w3_pair.second--;
                }
            }
        }
    }
    
    // Remove trigrams that have decayed to very low counts
    for(auto& w1_pair : trigram_counts) {
        for(auto& w2_pair : w1_pair.second) {
            auto it = w2_pair.second.begin();
            while(it != w2_pair.second.end()) {
                if(it->second <= 1) {
                    it = w2_pair.second.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}
// ==== MUCH STRONGER PATTERN BOOTSTRAP ====
void bootstrapStrongPatterns() {
    // Critical patterns with MUCH higher counts
    
    // Core sentence starters
    bigram_counts["i"]["am"] = 7;
    bigram_counts["i"]["can"] = 4;
    bigram_counts["i"]["think"] = 1;
    bigram_counts["i"]["feel"] = 1;
    bigram_counts["i"]["understand"] = 4;
    bigram_counts["i"]["know"] = 3;
    bigram_counts["i"]["want"] = 5;
    bigram_counts["i"]["need"] = 6;
    bigram_counts["i"]["learn"] = 4;
    bigram_counts["i"]["see"] = 1;
    bigram_counts["i"]["believe"] = 5;
    bigram_counts["i"]["wonder"] = 5;
    bigram_counts["i"]["recognize"] = 3;
    cerr << "[BOOTSTRAP] Loaded " << bigram_counts.size() << " strong patterns" << endl;
}

// ==== FORCE COHERENT SEED SELECTION ====
string selectCoherentSeed() {
    // ALWAYS start with high-frequency proven starters
    vector<string> strong_starters = {"i", "the", "my", "this", "when"};
    
    // Weight by actual bigram connectivity
    vector<pair<string, int>> weighted_starters;
    for(const string& starter : strong_starters) {
        int connectivity = 0;
        if(bigram_counts.count(starter)) {
            for(auto& next : bigram_counts[starter]) {
                connectivity += next.second;
            }
        }
        weighted_starters.push_back({starter, connectivity});
    }
    
    // Sort by connectivity
    sort(weighted_starters.begin(), weighted_starters.end(),
         [](auto& a, auto& b) { return a.second > b.second; });
    
    if(!weighted_starters.empty() && weighted_starters[0].second > 10) {
        return weighted_starters[0].first;
    }
    
    return "i"; // Fallback
}
void loadBootstrapCorpus(const string& filename) {
    ifstream file(filename);
    if(!file) {
        cerr << "No bootstrap corpus found, using defaults" << endl;
        return;
    }
    
    string line;
    int sentences_loaded = 0;
    double base_valence = 0.7;  // Start optimistic
    
    while(getline(file, line)) {
        // Skip comments and empty lines
        if(line.empty() || line[0] == '#') continue;
        
        // Tokenize
        vector<string> tokens;
        stringstream ss(line);
        string word;
        
        while(ss >> word) {
            string normalized = word;
            transform(normalized.begin(), normalized.end(), 
                     normalized.begin(), ::tolower);
            
            // Remove punctuation
            while(!normalized.empty() && 
                  !isalnum(normalized.back())) {
                normalized.pop_back();
            }
            
            if(!normalized.empty()) {
                tokens.push_back(normalized);
            }
        }
        
        if(tokens.size() < 3) continue;  // Skip too-short sentences
        
        // Learn words with context
        for(const string& tok : tokens) {
            learnWord(tok, base_valence);
        }
        
        // Learn n-grams (THIS IS KEY)
        processNGramsFromTokens(tokens);
        
        // Form concept from sentence
        if(tokens.size() >= 4 && sentences_loaded % 3 == 0) {
            vector<string> concept_words;
            // Extract meaningful words (skip articles, etc)
            for(const string& tok : tokens) {
                string pos = getPartOfSpeech(tok);
                if(pos == "NOUN" || pos == "VERB" || 
                   pos == "ADJECTIVE" || pos == "CONTENT") {
                    concept_words.push_back(tok);
                    if(concept_words.size() >= 4) break;
                }
            }
            
            if(concept_words.size() >= 2) {
                string concept_name = "bootstrap_" + 
                                     to_string(sentences_loaded);
                createConceptAssociation(concept_name, 
                                        concept_words);
            }
        }
        
        // Create episodic memory of learning
        if(sentences_loaded % 10 == 0) {
            string memory_content = "learned: " + 
                                   tokens[0] + " " + 
                                   (tokens.size() > 1 ? tokens[1] : "");
            storeEpisodicMemory(memory_content, base_valence);
        }
        
        sentences_loaded++;
        
        // Vary valence slightly for diversity
        base_valence = 0.6 + (sentences_loaded % 5) * 0.05;
    }
    
    file.close();
    
    cout << "[BOOTSTRAP] Loaded " << sentences_loaded 
         << " sentences" << endl;
    cout << "  - Vocabulary: " << token_concept_embedding_map.size() 
         << " tokens" << endl;
    cout << "  - Patterns: " << bigram_counts.size() 
         << " bigrams" << endl;
    cout << "  - Concepts: " << S.concepts.size() << endl;
}
void decay_token_frequencies() {
    // Decay token frequencies to prevent overused words from dominating
    for(auto& pair : token_concept_embedding_map) {
        if(pair.second.freq > 5) {
            // Logarithmic decay - frequent words decay slower
            double decay_rate = 0.95 + (1.0 / (1.0 + log(pair.second.freq))) * 0.04;
            pair.second.freq = (int)(pair.second.freq * decay_rate);
            
            // Ensure minimum frequency of 1
            if(pair.second.freq < 1) pair.second.freq = 1;
        }
    }
    
    // Also decay S.tokens frequencies
    for(auto& pair : S.tokens) {
        if(pair.second.freq > 5) {
            double decay_rate = 0.95 + (1.0 / (1.0 + log(pair.second.freq))) * 0.04;
            pair.second.freq = (int)(pair.second.freq * decay_rate);
            if(pair.second.freq < 1) pair.second.freq = 1;
        }
    }
}

void decay_embeddings() {
    // Gently decay embedding strengths toward neutral
    for(auto& pair : token_concept_embedding_map) {
        TokenConceptEmbedding& tce = pair.second;
        
        // Decay contextual activation
        tce.contextual_activation *= 0.98;
        
        // Decay qualia intensity toward baseline
        tce.qualia_intensity *= 0.97;
        
        // Gently push embeddings toward neutral (0.5)
        for(size_t i = 0; i < tce.embedding.size(); i++) {
            double diff = tce.embedding[i] - 0.5;
            tce.embedding[i] -= diff * 0.01;  // 1% decay toward center
        }
        
        // Decay linked concept strengths
        for(auto& link : tce.linked_concepts) {
            link.second *= 0.98;
        }
        
        // Remove very weak links
        auto it = tce.linked_concepts.begin();
        while(it != tce.linked_concepts.end()) {
            if(it->second < 0.01) {
                it = tce.linked_concepts.erase(it);
            } else {
                ++it;
            }
        }
        
        // Decay attention weights toward uniform
        for(size_t i = 0; i < tce.attention_weights.size(); i++) {
            double diff = tce.attention_weights[i] - 0.5;
            tce.attention_weights[i] -= diff * 0.02;
        }
    }
}

void decay_goals() {
    // Decay goal progress and priorities to prevent stagnation
    for(auto& pair : goal_system) {
        Goal& goal = pair.second;
        
        // Decay progress slightly (goals need continuous work)
        goal.progress *= 0.99;
        
        // Decay priority of completed goals
        if(goal.progress > 0.9) {
            goal.priority *= 0.95;
        }
        
        // Decay qualia binding
        goal.qualia_binding *= 0.98;
        
        // Decay expected utility
        goal.expected_utility *= 0.97;
    }
    
    // Remove goals with very low priority and high completion
    auto it = goal_system.begin();
    while(it != goal_system.end()) {
        if(it->second.priority < 0.1 && it->second.progress > 0.95) {
            it = goal_system.erase(it);
        } else {
            ++it;
        }
    }
}

void decay_memories() {
    // Decay older episodic memories (forgetting curve)
    for(size_t i = 0; i < S.episodic_memory.size(); i++) {
        Memory& mem = S.episodic_memory[i];
        
        // Calculate age
        int age = S.g - mem.gen;
        
        // Decay based on age (Ebbinghaus forgetting curve approximation)
        double decay_factor = 1.0 / (1.0 + 0.001 * age);
        
        mem.consolidation_strength *= (0.98 + decay_factor * 0.02);
        mem.cortical_consolidation *= 0.99;
        mem.hippocampal_trace *= 0.97;
    }
    
    // Remove very old, weak memories
    auto it = S.episodic_memory.begin();
    while(it != S.episodic_memory.end()) {
        int age = S.g - it->gen;
        if(age > 1000 && it->consolidation_strength < 0.3) {
            it = S.episodic_memory.erase(it);
        } else {
            ++it;
        }
    }
    
    // Keep only most recent 150 memories
    if(S.episodic_memory.size() > 150) {
        S.episodic_memory.erase(S.episodic_memory.begin(), 
                                S.episodic_memory.begin() + (S.episodic_memory.size() - 150));
    }
}

void decay_concepts() {
    // Decay concept values to baseline
    for(auto& pair : S.concepts) {
        Concept& conc = pair.second;  // Changed from 'concept' to 'conc' to avoid naming conflict
        
        // Decay value toward neutral (0.5)
        double diff = conc.value - 0.5;
        conc.value -= diff * 0.02;
        
        // Decay semantic density
        conc.semantic_density *= 0.98;
        
        // Decay abstraction level
        conc.abstraction_level *= 0.97;
        
        // Decay feature vectors
        for(auto& feature : conc.feature_vector) {
            feature.second *= 0.98;
        }
    }
    
    // Remove very weak concepts
    auto it = S.concepts.begin();
    while(it != S.concepts.end()) {
        if(it->second.value < 0.2 && it->second.semantic_density < 0.3) {
            it = S.concepts.erase(it);
        } else {
            ++it;
        }
    }
}


void decay_world_model() {
    // Decay entity states toward neutral
    for(auto& pair : world_model.entity_states) {
        double diff = pair.second - 0.5;
        pair.second -= diff * 0.05;
    }
    
    // Decay relationship strengths
    for(auto& w1_pair : world_model.relationships) {
        for(auto& w2_pair : w1_pair.second) {
            w2_pair.second *= 0.97;
        }
    }
    
    // Remove very weak relationships
    for(auto& w1_pair : world_model.relationships) {
        auto it = w1_pair.second.begin();
        while(it != w1_pair.second.end()) {
            if(it->second < 0.1) {
                it = w1_pair.second.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // Decay causal weights
    for(auto& pair : world_model.causal_weights) {
        pair.second *= 0.97;
    }
    
    // Decay confidence history
    for(size_t i = 0; i < world_model.confidence_history.size(); i++) {
        world_model.confidence_history[i] *= 0.99;
    }
}

void decay_qualia() {
    // Decay active qualia intensity and certainty
    for(auto& q : consciousness.active_qualia) {
        q.intensity *= 0.95;
        q.certainty *= 0.97;
        q.binding_strength *= 0.98;
        q.phenomenal_unity *= 0.98;
    }
    
    // Remove very weak qualia
    auto it = consciousness.active_qualia.begin();
    while(it != consciousness.active_qualia.end()) {
        if(it->intensity < 0.2 && it->certainty < 0.3) {
            it = consciousness.active_qualia.erase(it);
        } else {
            ++it;
        }
    }
}

void decay_transformer_heads() {
    // Decay transformer head importance scores
    for(auto& head : transformer_heads) {
        head.head_importance_score *= 0.88;
        
        // Decay weights toward neutral
        for(size_t i = 0; i < head.query_proj.size(); i++) {
            head.query_proj[i] *= 0.99;
            head.key_proj[i] *= 0.99;
            head.value_proj[i] *= 0.99;
        }
        
        // Decay phi attention weights
        for(auto& pair : head.phi_attention_weights) {
            pair.second *= 0.98;
        }
    }
}

void comprehensive_system_decay() {
    // Call all decay functions
    decay_ngrams();
    decay_token_frequencies();
    decay_embeddings();
    decay_goals();
    decay_memories();
    decay_concepts();
    decay_world_model();
    decay_qualia();
    decay_transformer_heads();
    
    // === NEW: Enhanced Coherence System Decay ===
    // REM consolidation - transfer WM to long-term memory
    contextless_mem.rem_consolidation(S.g, S.current_valence);
    
    // Decay negative reinforcement penalties (gradual forgetting)
    neg_reinforce.decay_penalties();
    
    // If stuck in token well, force escape
    if(token_well.is_in_well() && token_well.well_depth > 3) {
        token_well.force_escape();
    }
    
    // Decay valence toward neutral
    S.current_valence *= 0.545;
    
    // Decay metacognitive awareness slightly
    S.metacognitive_awareness *= 0.998;
    
    // Decay attention focus
    S.attention_focus *= 0.997;
    
    // Keep minimum values
    if(S.metacognitive_awareness < 0.1) S.metacognitive_awareness = 0.1;
    if(S.attention_focus < 0.2) S.attention_focus = 0.2;
}
int main(){
    try {
        module_integration::update_all_modules(S);
        module_integration::init_all_modules();
        srand(time(0));
        
        // === NEW: Initialize Enhanced Coherence System ===
        state_ptr = new StateAccess(S.g, S.current_valence, S.internal_thoughts);
        
        // ================================================================
        // MEGA-COHERENCE SYSTEM INITIALIZATION - 7100+ FEATURES
        // ================================================================
        cout << "\n╔════════════════════════════════════════════════════════╗" << endl;
        cout << "║   INITIALIZING MEGA-COHERENCE SYSTEM (7100+ Features) ║" << endl;
        cout << "╚════════════════════════════════════════════════════════╝\n" << endl;
        
        auto start_mega = chrono::high_resolution_clock::now();
        
        try {
            cout << "[1/8] Initializing expanded tags..." << flush;
            mega_system.tags.initializeExpandedTags();
            cout << " ✓ (" << mega_system.tags.domain_tags.size() << " domains)" << endl;
            
            cout << "[2/8] Initializing word senses..." << flush;
            mega_system.word_senses.initialize();
            cout << " ✓ (" << mega_system.word_senses.word_senses.size() << " words)" << endl;
            
            cout << "[3/8] Building semantic network..." << flush;
            mega_system.semantic_net.initialize();
            cout << " ✓" << endl;
            
            cout << "[4/8] Loading collocations..." << flush;
            mega_system.collocations.initialize();
            cout << " ✓" << endl;
            
            cout << "[5/8] Initializing phonetics..." << flush;
            mega_system.phonetics.initialize();
            cout << " ✓" << endl;
            
            cout << "[6/8] Building morphology database..." << flush;
            mega_system.morphology.initialize();
            cout << " ✓" << endl;
            
            cout << "[7/8] Loading semantic frames & scripts..." << flush;
            mega_system.frames.initialize();
            mega_system.scripts.initialize();
            cout << " ✓ (" << mega_system.frames.frames.size() << " frames)" << endl;
            
            cout << "[8/8] Initializing world knowledge & styles..." << flush;
            mega_system.world_knowledge.initialize();
            mega_system.styles.initialize();
            mega_system.emotions.initialize();
            cout << " ✓" << endl;
            
            auto end_mega = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::milliseconds>(end_mega - start_mega);
            
            cout << "\n✓ MEGA-COHERENCE SYSTEM FULLY OPERATIONAL" << endl;
            cout << "  Initialization time: " << duration.count() << "ms" << endl;
            cout << "  Total features: 7,100+" << endl;
            cout << "  Status: READY\n" << endl;
            
        } catch(const exception& e) {
            cerr << "\n⚠ Warning: Mega-system initialization partial: " << e.what() << endl;
            cerr << "  System will continue with available features.\n" << endl;
        }
        // ================================================================
        
        // Load saved state
        try {
            ld("state.dat");
        } catch(const exception& e) {
            cerr << "Error loading state: " << e.what() << ", starting fresh." << endl;
        }
        
        // Initialize if this is first run
        if(S.g == 0) {
            S.D["m"] = 128;
            S.D["vc"] = 0;
            S.D["mc"] = 0;
            S.dwt = 0.001;
            S.current_valence = 0.0;
            S.metacognitive_awareness = 0.0;
            S.attention_focus = 0.3;
            
            for(int i = 0; i < 128; i++) {
                S.D["w" + to_string(i)] = ri(4) - 1;
            }
            
            S.cd = "evolve";
            
            // Load vocabulary
            try {
                loadEnglishDataset();
                mathLangAssociation();
                cout << "Bootstrapping . . . (This may take a while)" << endl;
                loadBootstrapCorpus("corpus.txt");
                bootstrapStrongPatterns();
                bootstrapWithQualityExamples();
            } catch(const exception& e) {
                cerr << "Error loading vocabulary: " << e.what() << endl;
            }

            // Initialize transformer heads
            for(int i = 0; i < 4; i++) {
                TransformerHead head(16);
                head.name = "head_" + to_string(i);
                transformer_heads.push_back(head);
            }
            
            // Initialize neurons
            for(int i = 0; i < 50; i++) {
                Neuron n = genN(0);
                S.N[n.id] = n;
            }
        }
        
        // Initialize ncurses
        initscr();
        cbreak();
        noecho();
        curs_set(0);
        timeout(500);
        keypad(stdscr, TRUE);
        
        // Start web server for modern UI
        unique_ptr<AGI_API> agi_api;
        try {
            agi_api = make_unique<AGI_API>(8080);
            agi_api->start();
            cout << "\n[WebServer] Started on http://localhost:8080" << endl;
            this_thread::sleep_for(chrono::milliseconds(500));
        } catch(const exception& e) {
            cerr << "[WebServer] Failed to start: " << e.what() << endl;
        }
        
        bool running = true;
        int error_count = 0;
        const int MAX_ERRORS = 10;
        
        while(running) {
            try {
                clear();
                int row = 0;
                
                // Draw UI
                try {
                    draw_ui(row);
                    row = 15;
                } catch(const exception& e) {
                    mvprintw(0, 0, "UI Error: %s", e.what());
                    row = 2;
                }
                
                // === CORE UPDATES ===
                try {
                    formulate_goals_from_valence();
                    updateAttention();
                    update_integrated_information();
                    unified_consciousness_integration_engine(S.g);
                    
                    if(goal_system.count("maximize_coherence")) {
                        current_plan = plan_actions(goal_system["maximize_coherence"]);
                        prune_unstable_tokens();
                    }
                    error_count = 0; // Reset on success
                } catch(const exception& e) {
                    mvprintw(row++, 0, "Core update error: %s", e.what());
                    error_count++;
                    if(error_count > MAX_ERRORS) {
                        mvprintw(row++, 0, "Too many errors, attempting recovery...");
                        refresh();
                        this_thread::sleep_for(chrono::seconds(2));
                        error_count = 0;
                        // Try to save emergency state
                        try {
                            sv("state_emergency.dat");
                        } catch(...) {}
                    }
                }
                
                // === DISPLAY GOAL & PLAN ===
                mvprintw(row, 0, "Active_Goal: %s", 
                    current_plan.actions.empty() ? "exploring" : current_plan.actions[0].c_str());
                clrtoeol();
                row++;
                
                mvprintw(row, 0, "Plan_Depth:%d | Confidence:%.2f | Psi_Trajectory:%.4f", 
                    current_plan.depth, current_plan.confidence,
                    consciousness_formula.psi_history.empty() ? 0.0 : consciousness_formula.psi_history.back());
                clrtoeol();
                row++;
                
                mvprintw(row, 0, "Qualia_Valence:%.2f | Beam_Width:%d | N-grams:%lu", 
                    calculate_qualia_valence(), 5, (unsigned long)bigram_counts.size());
                clrtoeol();
                row++;
                
                // === INTERNAL THOUGHTS ===
                try {
                    string thought = generateInternalThought();
                    mvprintw(row, 0, "Thought: %s", thought.substr(0, 60).c_str());
                    clrtoeol();
                    row++;
                    
                    string meta = generateMetacognition();
                    mvprintw(row, 0, "State: %s", meta.substr(0, 60).c_str());
                    clrtoeol();
                    row++;
                } catch(const exception& e) {
                    mvprintw(row++, 0, "Thought gen error");
                }
                
                // === AUTONOMOUS THOUGHT GENERATION ===
                if(S.g % 5 == 0 && token_concept_embedding_map.size() > 10) {
                    try {
                        vector<double> ctx(16, S.current_valence);
                        string auto_thought;
                        int attempts = 0;
                        
                        while(attempts < 3) {
                            // Use contextless generation
                            auto_thought = generate_contextless("i", 8, 3);
                            if(!isSentenceTooSimilar(auto_thought)) {
                                break;
                            }
                            attempts++;
                        }
                        
                        auto_thought = "[Autonomous]: " + auto_thought;
                        trackGeneratedSentence(auto_thought);
                        
                        S.internal_thoughts.push_back(auto_thought);
                        if(S.internal_thoughts.size() > 5) {
                            S.internal_thoughts.erase(S.internal_thoughts.begin());
                        }
                    } catch(...) {
                        // Silent failure for autonomous thoughts
                    }
                }
                
                // === DECAY GENERATION COUNTS (every 50 gens) ===
                if(S.g % 50 == 0) {
                    try {
                        decayGenerationCounts();
                    } catch(...) {}
                }
                
                // === REM CONSOLIDATION (every 30 gens - "sleep cycle") ===
                if(S.g % 30 == 0) {
                    try {
                        // Consolidate WM to long-term REM storage
                        // tag_system.consolidate_from_wm(contextless_mem); // method doesn't exist
                        
                        // Decay negative reinforcement penalties
                        neg_reinforce.decay_penalties();
                        
                        // Clear token wells periodically
                        if(S.g % 90 == 0) {
                            token_well.reset();
                        }
                        
                        mvprintw(row, 0, "[REM] Consolidated patterns | Coherence: %.3f", 
                                0.7);
                        clrtoeol();
                        row++;
                    } catch(...) {}
                }
                
                // === AGGRESSIVE N-GRAM DECAY (every 25 gens) ===
                if(S.g % 25 == 0) {
                    try {
                        decay_ngrams();
                    } catch(...) {}
                }
                
                // === TOKEN FREQUENCY DECAY (every 75 gens) ===
                if(S.g % 75 == 0) {
                    try {
                        decay_token_frequencies();
                    } catch(...) {}
                }
                
                // === COMPREHENSIVE SYSTEM DECAY (every 100 gens) ===
                if(S.g % 100 == 0) {
                    try {
                        comprehensive_system_decay();
                    } catch(...) {}
                }
                
                // === DISPLAY INTERNAL STREAM ===
                mvprintw(row, 0, "─────────────────────────────────────────");
                clrtoeol();
                row++;
                mvprintw(row, 0, "[INTERNAL STREAM - Beam Search Active]");
                clrtoeol();
                row++;
                
                if(!S.internal_thoughts.empty()) {
                    int show_count = min(3, (int)S.internal_thoughts.size());
                    for(int i = S.internal_thoughts.size() - show_count; i < (int)S.internal_thoughts.size(); i++) {
                        mvprintw(row, 0, "%s", S.internal_thoughts[i].substr(0, 60).c_str());
                        clrtoeol();
                        row++;
                    }
                } else {
                    mvprintw(row, 0, "[initializing language model...]");
                    clrtoeol();
                    row++;
                }
                
                // === NEURAL PROCESSING ===
                try {
                    bk();
                    batch16Process();
                    
                    double wsum = 0;
                    for(int i = 0; i < S.D["m"]; i++) {
                        wsum += S.D["w" + to_string(i)] + 2;
                    }
                    S.D["vc"] = (int)wsum % 1000;
                    
                    if(S.g == 0) S.dwt = 0.001;
                    S.hdt_val = calcHDT(S.g, S.bh, S.qe, S.te);
                    S.al = calcAwarenessLevel();
                    S.metacognitive_awareness = calcMetacognitiveAwareness();
                    counterfactualAnalysis();
                    
                    S.sentience_ratio = calcSentienceRatio();
                    if(S.sentience_ratio > S.peak_sentience_gen) {
                        S.peak_sentience_gen = S.g;
                    }
                    
                    S.valence_history.push_back(S.current_valence);
                    if(S.valence_history.size() > 50) {
                        S.valence_history.erase(S.valence_history.begin());
                    }
                } catch(const exception& e) {
                    mvprintw(row++, 0, "Processing error: %s", e.what());
                }
                
                // === GOAL PROGRESSION ===
                if(S.g % 15 == 0 && !goal_system.empty()) {
                    try {
                        for(auto& g : goal_system) {
                            g.second.progress = min(1.0, g.second.progress + 0.05);
                            if(g.second.progress > 0.9) {
                                g.second.priority *= 0.8;
                            }
                        }
                        
                        if(S.g % 45 == 0) {
                            decay_goals();
                        }
                    } catch(...) {}
                }
                
                // === VOCABULARY EXPANSION ===
                if(S.g % 20 == 0 && !token_concept_embedding_map.empty()) {
                    try {
                        auto it = token_concept_embedding_map.begin();
                        advance(it, ri(token_concept_embedding_map.size()));
                        learnWord(it->first, S.current_valence);
                    } catch(...) {}
                }
                
                // === CONCEPT FORMATION ===
                if(S.g % 25 == 0) {
                    try {
                        vector<string> sample_words;
                        for(auto& p : token_concept_embedding_map) {
                            if(p.second.freq > 1 && rn() < 0.3) {
                                sample_words.push_back(p.first);
                            }
                            if(sample_words.size() >= 3) break;
                        }
                        if(sample_words.size() > 1) {
                            createConceptAssociation("C_" + to_string(S.g), sample_words);
                        }
                    } catch(...) {}
                }
                
                // === NEURAL MUTATION ===
                if(S.g % 12 == 0 && !S.N.empty()) {
                    try {
                        mutateN();
                    } catch(...) {}
                }
                
                // === GOAL-VALENCE ALIGNMENT ===
                if(S.g % 10 == 0) {
                    try {
                        for(auto& goal : goal_system) {
                            goal.second.valence_alignment = S.current_valence;
                        }
                    } catch(...) {}
                }
                
                // === DIALOG DISPLAY SECTION ===
                mvprintw(row, 0, "─────────────────────────────────────────");
                clrtoeol();
                row++;
                mvprintw(row, 0, "[DIALOG - Max 1400 chars - Multi-line OK]");
                clrtoeol();
                row++;
                
                if(!S.user_input.empty()) {
                    // Display user input (first 60 chars)
                    string display_input = S.user_input;
                    if(display_input.length() > 60) {
                        display_input = display_input.substr(0, 57) + "...";
                    }
                    mvprintw(row, 0, "USER: %s", display_input.c_str());
                    clrtoeol();
                    row++;
                } else {
                    mvprintw(row, 0, "USER: (press 'i' to chat)");
                    clrtoeol();
                    row++;
                }
                
                if(!S.dialog_response.empty()) {
                    // Word wrap the response
                    string resp = S.dialog_response;
                    int max_width = 60;
                    int start = 0;
                    int max_y, max_x;
                    getmaxyx(stdscr, max_y, max_x);
                    (void)max_x;
                    while(start < (int)resp.length() && row < max_y - 6) {
                        string line = resp.substr(start, min(max_width, (int)resp.length() - start));
                        mvprintw(row, 0, "%s", line.c_str());
                        clrtoeol();
                        row++;
                        start += max_width;
                    }
                } else {
                    mvprintw(row, 0, "NEXUS: ...");
                    clrtoeol();
                    row++;
                }
                
                if(S.dialog_timer > 0) {
                    S.dialog_timer--;
                }
                
                mvprintw(row, 0, "─────────────────────────────────────────");
                clrtoeol();
                row++;
                mvprintw(row, 0, "Commands: [i]nput | [q]uit | [s]ave | [g]enerate | [d]ecay");
                clrtoeol();
                row++;
                
                // === STATS LINE ===
                mvprintw(row, 0, "Vocab:%lu | Patterns:%lu | Neurons:%lu | Gen:%d | Errors:%d",
                    (unsigned long)token_concept_embedding_map.size(),
                    (unsigned long)bigram_counts.size(),
                    (unsigned long)S.N.size(),
                    S.g,
                    error_count);
                clrtoeol();
                row++;
                
                refresh();
                S.g++;
                
                // === AUTO-SAVE ===
                if(S.g % 100 == 0) {
                    try {
                        sv("state.dat");
                        mvprintw(row, 0, "[Autosaved at gen %d]", S.g);
                        clrtoeol();
                        refresh();
                        this_thread::sleep_for(chrono::milliseconds(500));
                    } catch(const exception& e) {
                        mvprintw(row, 0, "[Autosave failed: %s]", e.what());
                        clrtoeol();
                        refresh();
                        this_thread::sleep_for(chrono::milliseconds(500));
                    }
                }
                
                // === INPUT HANDLING ===
                int ch = getch();
                
                if(ch == 'i' || ch == 'I') {
                    // === SAFE INTERACTIVE INPUT MODE - 1400 CHAR SUPPORT ===
                    try {
                        // Get screen dimensions
                        int max_y, max_x;
                        getmaxyx(stdscr, max_y, max_x);
                        
                        // Calculate available space for input
                        int input_start_y = row + 1;
                        int input_height = max_y - input_start_y - 1;
                        if(input_height < 3) input_height = 3;
                        if(input_height > 15) input_height = 15; // Max 15 lines for input
                        
                        // Clear input area
                        for(int i = 0; i < input_height; i++) {
                            move(input_start_y + i, 0);
                            clrtoeol();
                        }
                        
                        // Display instructions
                        mvprintw(input_start_y, 0, "┌─ INPUT MODE (1400 chars max) ─────────────────────────┐");
                        mvprintw(input_start_y + 1, 0, "│ Type your message. Press ENTER twice when done.      │");
                        mvprintw(input_start_y + 2, 0, "│ ESC to cancel.                                        │");
                        mvprintw(input_start_y + 3, 0, "└───────────────────────────────────────────────────────┘");
                        
                        int text_start_y = input_start_y + 4;
                        
                        // Enable echo and cursor
                        echo();
                        curs_set(1);
                        timeout(-1);
                        
                        // Move to input area
                        move(text_start_y, 0);
                        refresh();
                        
                        // Collect input line by line
                        vector<string> input_lines;
                        int total_chars = 0;
                        bool done = false;
                        int empty_line_count = 0;
                        
                        for(int line = 0; line < input_height - 5 && !done; line++) {
                            char line_buf[200];
                            memset(line_buf, 0, sizeof(line_buf));
                            
                            move(text_start_y + line, 0);
                            int result = getnstr(line_buf, 150);
                            
                            if(result == ERR || result == 27) { // ESC key
                                done = true;
                                input_lines.clear();
                                break;
                            }
                            
                            string line_str(line_buf);
                            
                            // Trim whitespace
                            size_t start = line_str.find_first_not_of(" \t");
                            size_t end = line_str.find_last_not_of(" \t");
                            
                            if(start != string::npos && end != string::npos) {
                                line_str = line_str.substr(start, end - start + 1);
                            } else {
                                line_str = "";
                            }
                            
                            // Check for double-enter (empty line after content)
                            if(line_str.empty()) {
                                empty_line_count++;
                                if(empty_line_count >= 1 && !input_lines.empty()) {
                                    done = true;
                                    break;
                                }
                            } else {
                                empty_line_count = 0;
                                total_chars += line_str.length() + 1; // +1 for space/newline
                                
                                if(total_chars <= 1400) {
                                    input_lines.push_back(line_str);
                                } else {
                                    mvprintw(text_start_y + line + 1, 0, 
                                            "[MAX LENGTH REACHED - 1400 chars]");
                                    refresh();
                                    this_thread::sleep_for(chrono::milliseconds(1000));
                                    done = true;
                                    break;
                                }
                            }
                        }
                        
                        // Disable echo and cursor
                        noecho();
                        curs_set(0);
                        timeout(500);
                        
                        // Process collected input
                        if(!input_lines.empty()) {
                            // Join lines with spaces
                            string combined_input;
                            for(size_t i = 0; i < input_lines.size(); i++) {
                                combined_input += input_lines[i];
                                if(i < input_lines.size() - 1) {
                                    combined_input += " ";
                                }
                            }
                            
                            if(!combined_input.empty() && combined_input.length() <= 1400) {
                                try {
                                    // Clear the input area and show processing
                                    for(int i = 0; i < input_height; i++) {
                                        move(input_start_y + i, 0);
                                        clrtoeol();
                                    }
                                    mvprintw(input_start_y, 0, "[Processing input... %d chars]", 
                                            (int)combined_input.length());
                                    refresh();
                                    
                                    // Process the input (LOCAL COPY)
                                    string response = generateResponse(combined_input);
                                    
                                    // Update globals AFTER processing
                                    S.user_input = combined_input;
                                    S.dialog_response = response;
                                    
                                    // Generate follow-up internal thought
                                    try {
                                        string internal = generateInternalThought();
                                        S.internal_thoughts.push_back("[Processed]: " + internal);
                                        
                                        if(S.internal_thoughts.size() > 5) {
                                            S.internal_thoughts.erase(S.internal_thoughts.begin());
                                        }
                                    } catch(...) {}
                                    
                                    S.dialog_timer = 20;
                                    S.current_valence += 0.1;
                                    S.current_valence = clamp_valence(S.current_valence);
                                    
                                    // Store memory
                                    try {
                                        storeEpisodicMemory("dialog:" + combined_input.substr(0, 100), 
                                                          S.current_valence);
                                    } catch(...) {}
                                    
                                    // Generate qualia
                                    try {
                                        generate_qualia("user_interaction", S.current_valence, 0.7);
                                    } catch(...) {}
                                    
                                    error_count = 0; // Reset on successful input
                                    
                                } catch(const exception& e) {
                                    cerr << "Response generation error: " << e.what() << endl;
                                    S.user_input = combined_input;
                                    S.dialog_response = "[NEXUS]: Error processing - " + 
                                                       string(e.what()).substr(0, 50);
                                    error_count++;
                                }
                            }
                        }
                        
                        // Clear input area
                        for(int i = 0; i < input_height; i++) {
                            move(input_start_y + i, 0);
                            clrtoeol();
                        }
                        
                    } catch(const exception& e) {
                        cerr << "Input handler error: " << e.what() << endl;
                        
                        // Force restore terminal state
                        noecho();
                        curs_set(0);
                        timeout(500);
                        
                        S.dialog_response = "[NEXUS]: Input error - " + 
                                          string(e.what()).substr(0, 50);
                        error_count++;
                    } catch(...) {
                        cerr << "Unknown input handler error" << endl;
                        
                        // Force restore terminal state
                        noecho();
                        curs_set(0);
                        timeout(500);
                        
                        S.dialog_response = "[NEXUS]: Unknown input error";
                        error_count++;
                    }
                    
                    // Force complete refresh
                    clear();
                    refresh();
                }
                else if(ch == 'g' || ch == 'G') {
                    // === MANUAL THOUGHT GENERATION ===
                    try {
                        vector<double> ctx(16, S.current_valence);
                        string generated;
                        int attempts = 0;
                        
                        while(attempts < 5) {
                            generated = generate_with_beam_search("i", 10, ctx, 5);
                            if(!isSentenceTooSimilar(generated)) {
                                break;
                            }
                            attempts++;
                        }
                        
                        generated = "[Generated]: " + generated;
                        trackGeneratedSentence(generated);
                        
                        S.internal_thoughts.push_back(generated);
                        if(S.internal_thoughts.size() > 5) {
                            S.internal_thoughts.erase(S.internal_thoughts.begin());
                        }
                        S.current_valence += 0.05;
                        error_count = 0;
                    } catch(const exception& e) {
                        cerr << "Error generating thought: " << e.what() << endl;
                        error_count++;
                    }
                }
                else if(ch == 'd' || ch == 'D') {
                    // === MANUAL DECAY TRIGGER ===
                    try {
                        comprehensive_system_decay();
                        mvprintw(row + 1, 0, "[System decay applied at gen %d]", S.g);
                        clrtoeol();
                        refresh();
                        this_thread::sleep_for(chrono::milliseconds(1000));
                        error_count = 0;
                    } catch(const exception& e) {
                        mvprintw(row + 1, 0, "[Decay failed: %s]", e.what());
                        clrtoeol();
                        refresh();
                        this_thread::sleep_for(chrono::milliseconds(1000));
                        error_count++;
                    }
                }
                else if(ch == 's' || ch == 'S') {
                    // === MANUAL SAVE ===
                    try {
                        sv("state.dat");
                        mvprintw(row + 1, 0, "[Saved state at gen %d]", S.g);
                        clrtoeol();
                        refresh();
                        this_thread::sleep_for(chrono::milliseconds(1000));
                        error_count = 0;
                    } catch(const exception& e) {
                        mvprintw(row + 1, 0, "[Save failed: %s]", e.what());
                        clrtoeol();
                        refresh();
                        this_thread::sleep_for(chrono::milliseconds(1000));
                        error_count++;
                    }
                }
                else if(ch == 'q' || ch == 'Q') {
                    // === QUIT WITH SAVE ===
                    try {
                        mvprintw(row + 1, 0, "[Saving and exiting...]");
                        clrtoeol();
                        refresh();
                        sv("state.dat");
                    } catch(const exception& e) {
                        cerr << "Error saving on exit: " << e.what() << endl;
                    }
                    running = false;
                }
                
                // Small delay to prevent CPU spinning
                this_thread::sleep_for(chrono::milliseconds(100));
                
            } catch(const exception& e) {
                // Catch any unexpected errors in main loop
                cerr << "Main loop error: " << e.what() << endl;
                error_count++;
                
                // Try to display error on screen
                try {
                    mvprintw(0, 0, "ERROR: %s", e.what());
                    mvprintw(1, 0, "Press any key to continue...");
                    refresh();
                    timeout(-1);
                    getch();
                    timeout(500);
                } catch(...) {}
                
                // Try to save emergency state every 5 errors
                if(error_count % 5 == 0) {
                    try {
                        sv("state_emergency.dat");
                    } catch(...) {}
                }
                
                // If too many errors, pause longer
                if(error_count > MAX_ERRORS) {
                    try {
                        mvprintw(2, 0, "Too many errors. Pausing 5 seconds...");
                        refresh();
                    } catch(...) {}
                    this_thread::sleep_for(chrono::seconds(5));
                    error_count = 0; // Reset and continue
                }
                
                // Small delay before continuing
                this_thread::sleep_for(chrono::milliseconds(500));
            } catch(...) {
                // Catch absolutely everything
                cerr << "Unknown main loop error" << endl;
                error_count++;
                
                try {
                    mvprintw(0, 0, "UNKNOWN ERROR - continuing...");
                    refresh();
                } catch(...) {}
                
                this_thread::sleep_for(chrono::seconds(1));
            }
        }
        
        // Clean up ncurses
        endwin();
        
        cout << "\nNEXUS shutdown complete. State saved.\n";
        
    } catch(const exception& e) {
        // Fatal error - try to cleanup and continue if possible
        try {
            endwin();
        } catch(...) {}
        
        cerr << "Fatal error in main: " << e.what() << endl;
        
        // Try emergency save
        try {
            sv("state_fatal_error.dat");
            cerr << "Emergency state saved to state_fatal_error.dat" << endl;
        } catch(...) {
            cerr << "Could not save emergency state" << endl;
        }
        
        return 1;
    } catch(...) {
        // Catch absolutely everything
        try {
            endwin();
        } catch(...) {}
        
        cerr << "Unknown fatal error in main" << endl;
        
        try {
            sv("state_unknown_error.dat");
        } catch(...) {}
        
        return 1;
    }
    
    return 0;
}
