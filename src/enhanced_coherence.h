#ifndef ENHANCED_COHERENCE_H
#define ENHANCED_COHERENCE_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <random>

using namespace std;

// Forward declarations
extern map<string, map<string, int>> bigram_counts;
extern map<string, map<string, map<string, int>>> trigram_counts;
extern mt19937 rng;

// ============================================================================
// CONTEXTLESS ARCHITECTURE - ALL CONTEXT FLOWS THROUGH WM → REM
// ============================================================================

// External state access
struct StateAccess {
    int& g;
    double& current_valence;
    vector<string>& internal_thoughts;
    
    StateAccess(int& gen, double& val, vector<string>& thoughts)
        : g(gen), current_valence(val), internal_thoughts(thoughts) {}
};

extern StateAccess* state_ptr;

// Negative reinforcement tracker
struct NegativeSignal {
    string pattern;
    double penalty;
    int generation;
    string reason;
};

// Token well detection - tracks if we're stuck repeating
class TokenWellDetector {
private:
    deque<string> recent_tokens;  // Last 100 tokens
    map<string, int> token_frequency;
    
public:
    double repetition_score = 0.0;
    int well_depth = 0;  // How deep in repetition we are
    
    void add_token(const string& token) {
        recent_tokens.push_back(token);
        token_frequency[token]++;
        
        if(recent_tokens.size() > 100) {
            string old = recent_tokens.front();
            recent_tokens.pop_front();
            token_frequency[old]--;
            if(token_frequency[old] == 0) {
                token_frequency.erase(old);
            }
        }
        
        // Calculate repetition score
        repetition_score = 0.0;
        for(auto& [tok, count] : token_frequency) {
            if(count > 3) {
                repetition_score += (count - 3) * 0.1;
            }
        }
        
        well_depth = (repetition_score > 1.0) ? (int)(repetition_score) : 0;
    }
    
    bool is_in_well() const {
        return repetition_score > 1.5;
    }
    
    double get_token_penalty(const string& token) const {
        auto it = token_frequency.find(token);
        if(it == token_frequency.end()) return 0.0;
        int count = it->second;
        if(count > 5) return 1.0;  // Hard block
        if(count > 3) return 0.7;
        if(count > 2) return 0.3;
        return 0.0;
    }
    
    void reset() {
        recent_tokens.clear();
        token_frequency.clear();
        repetition_score = 0.0;
        well_depth = 0;
    }
    
    // Escape mechanism when stuck
    void force_escape() {
        // Clear most frequent tokens
        int max_freq = 0;
        for(auto& [tok, count] : token_frequency) {
            max_freq = max(max_freq, count);
        }
        
        // Remove tokens that appear > 50% of max
        vector<string> to_remove;
        for(auto& [tok, count] : token_frequency) {
            if(count > max_freq * 0.5) {
                to_remove.push_back(tok);
            }
        }
        
        for(const auto& tok : to_remove) {
            token_frequency[tok] = 1;  // Reset to minimum
        }
        
        repetition_score *= 0.3;  // Drastically reduce
    }
};

// Multi-level repetition detector
class RepetitionDetector {
public:
    // Character-level (detects "aaaa" or "ababab")
    bool has_char_repetition(const string& text) {
        if(text.length() < 4) return false;
        for(size_t i = 0; i < text.length() - 3; i++) {
            if(text[i] == text[i+1] && text[i] == text[i+2] && text[i] == text[i+3]) {
                return true;
            }
        }
        return false;
    }
    
    // Word-level repetition
    double word_repetition_score(const vector<string>& words) {
        if(words.size() < 3) return 0.0;
        map<string, int> counts;
        for(const auto& w : words) counts[w]++;
        
        double score = 0.0;
        for(auto& [word, count] : counts) {
            if(count > 2 && word.length() > 2) {
                score += (count - 2) * 0.2;
            }
        }
        return score;
    }
    
    // Phrase-level (detects "the the" or "I am I am")
    bool has_phrase_repetition(const string& text) {
        vector<string> words;
        stringstream ss(text);
        string word;
        while(ss >> word) words.push_back(word);
        
        if(words.size() < 4) return false;
        
        // Check for 2-gram repetition
        for(size_t i = 0; i < words.size() - 3; i++) {
            if(words[i] == words[i+2] && words[i+1] == words[i+3]) {
                return true;
            }
        }
        
        // Check for immediate repetition
        for(size_t i = 0; i < words.size() - 1; i++) {
            if(words[i] == words[i+1] && words[i].length() > 3) {
                return true;
            }
        }
        
        return false;
    }
    
    // Structural repetition (same pattern)
    bool has_structural_repetition(const vector<string>& sentence1, 
                                   const vector<string>& sentence2) {
        if(sentence1.size() != sentence2.size()) return false;
        if(sentence1.size() < 3) return false;
        
        int matches = 0;
        for(size_t i = 0; i < sentence1.size(); i++) {
            if(sentence1[i] == sentence2[i]) matches++;
        }
        
        return (double)matches / sentence1.size() > 0.7;
    }
    
    // Semantic repetition (same concept different words)
    double semantic_similarity(const string& s1, const string& s2) {
        // Simple Jaccard similarity
        set<string> words1, words2;
        stringstream ss1(s1), ss2(s2);
        string w;
        while(ss1 >> w) words1.insert(w);
        while(ss2 >> w) words2.insert(w);
        
        if(words1.empty() || words2.empty()) return 0.0;
        
        int intersection = 0;
        for(const auto& word : words1) {
            if(words2.count(word)) intersection++;
        }
        
        int union_size = words1.size() + words2.size() - intersection;
        return union_size > 0 ? (double)intersection / union_size : 0.0;
    }
    
    // N-gram overlap
    double ngram_overlap(const string& s1, const string& s2, int n = 3) {
        if(s1.length() < n || s2.length() < n) return 0.0;
        
        set<string> ngrams1, ngrams2;
        for(size_t i = 0; i <= s1.length() - n; i++) {
            ngrams1.insert(s1.substr(i, n));
        }
        for(size_t i = 0; i <= s2.length() - n; i++) {
            ngrams2.insert(s2.substr(i, n));
        }
        
        int intersection = 0;
        for(const auto& ng : ngrams1) {
            if(ngrams2.count(ng)) intersection++;
        }
        
        int union_size = ngrams1.size() + ngrams2.size() - intersection;
        return union_size > 0 ? (double)intersection / union_size : 0.0;
    }
};

// Contextless memory - everything must be explicitly stored/retrieved
class ContextlessMemory {
public:
    // Current working context (limited capacity)
    vector<string> working_context;  // Max 8 items
    const int MAX_WM_CONTEXT = 8;
    
    // Consolidated memories (from REM)
    struct ConsolidatedMemory {
        string content;
        double importance;
        int access_count;
        int generation_created;
        vector<string> associated_concepts;
        double emotional_valence;
        int sleep_cycles_survived;
    };
    
    vector<ConsolidatedMemory> long_term;
    int rem_cycles = 0;
    
    void add_to_working(const string& item) {
        working_context.push_back(item);
        if(working_context.size() > MAX_WM_CONTEXT) {
            // Oldest item drops out unless consolidated
            working_context.erase(working_context.begin());
        }
    }
    
    void rem_consolidation(int current_gen, double current_valence) {
        // Consolidate working memory to long-term during REM
        rem_cycles++;
        
        for(const auto& item : working_context) {
            ConsolidatedMemory mem;
            mem.content = item;
            mem.importance = 0.5 + abs(current_valence) * 0.5;  // Emotional memories stronger
            mem.access_count = 1;
            mem.generation_created = current_gen;
            mem.emotional_valence = current_valence;
            mem.sleep_cycles_survived = 0;
            
            long_term.push_back(mem);
        }
        
        // Decay old memories
        for(auto& mem : long_term) {
            mem.importance *= 0.95;  // Slow decay
            mem.sleep_cycles_survived++;
            
            // Boost frequently accessed memories
            if(mem.access_count > 5) {
                mem.importance += 0.05;
            }
        }
        
        // Keep only most important memories
        if(long_term.size() > 100) {
            sort(long_term.begin(), long_term.end(), 
                [](const ConsolidatedMemory& a, const ConsolidatedMemory& b) {
                    double score_a = a.importance * (1.0 + log(a.access_count + 1));
                    double score_b = b.importance * (1.0 + log(b.access_count + 1));
                    return score_a > score_b;
                });
            long_term.resize(100);
        }
        
        // Clear working memory after consolidation
        working_context.clear();
    }
    
    vector<string> retrieve_relevant(const string& query, int max_items = 3) {
        RepetitionDetector detector;
        vector<pair<double, string>> scored_memories;
        
        for(auto& mem : long_term) {
            double relevance = detector.semantic_similarity(query, mem.content);
            double recency = 1.0 / (1.0 + mem.sleep_cycles_survived * 0.1);
            double score = relevance * mem.importance * (1.0 + log(mem.access_count + 1)) * recency;
            
            scored_memories.push_back({score, mem.content});
            mem.access_count++;  // Strengthen with use
            mem.importance += 0.01;  // Small boost from retrieval
        }
        
        sort(scored_memories.begin(), scored_memories.end(), greater<>());
        
        vector<string> results;
        for(int i = 0; i < min(max_items, (int)scored_memories.size()); i++) {
            if(scored_memories[i].first > 0.1) {  // Threshold
                results.push_back(scored_memories[i].second);
            }
        }
        return results;
    }
    
    string get_context_string() {
        // Returns current accessible context (WM only - truly contextless!)
        string ctx;
        for(const auto& item : working_context) {
            ctx += item + " ";
        }
        return ctx;
    }
    
    void clear_working() {
        working_context.clear();
    }
    
    int get_wm_size() const {
        return working_context.size();
    }
    
    int get_ltm_size() const {
        return long_term.size();
    }
};

// Enhanced grammar validation with syntax rules
class GrammarValidator {
private:
    map<string, set<string>> valid_transitions;  // POS -> valid next POS
    map<string, double> transition_weights;  // Cached weights
    
public:
    GrammarValidator() {
        // Build transition rules
        valid_transitions["PRONOUN"] = {"BE_VERB", "MODAL", "VERB", "ADVERB"};
        valid_transitions["BE_VERB"] = {"ADJECTIVE", "NOUN", "ADVERB", "VERB", "PRONOUN"};
        valid_transitions["MODAL"] = {"VERB", "BE_VERB", "ADVERB", "PRONOUN"};
        valid_transitions["ARTICLE"] = {"NOUN", "ADJECTIVE"};
        valid_transitions["ADJECTIVE"] = {"NOUN", "CONJUNCTION", "ADJECTIVE"};
        valid_transitions["NOUN"] = {"BE_VERB", "VERB", "CONJUNCTION", "PREPOSITION", "ADJECTIVE"};
        valid_transitions["VERB"] = {"NOUN", "PRONOUN", "ARTICLE", "ADVERB", "PREPOSITION", "ADJECTIVE"};
        valid_transitions["ADVERB"] = {"VERB", "ADJECTIVE", "ADVERB"};
        valid_transitions["PREPOSITION"] = {"ARTICLE", "NOUN", "PRONOUN", "ADJECTIVE"};
        valid_transitions["CONJUNCTION"] = {"PRONOUN", "ARTICLE", "NOUN", "ADJECTIVE"};
        valid_transitions["QUESTION"] = {"BE_VERB", "MODAL", "VERB"};
        valid_transitions["CONTENT"] = {"NOUN", "VERB", "ADJECTIVE", "ADVERB"};
    }
    
    double validate_transition(const string& prev_pos, const string& next_pos) {
        auto it = valid_transitions.find(prev_pos);
        if(it == valid_transitions.end()) return 0.5;  // Unknown, neutral
        
        if(it->second.count(next_pos)) return 1.0;  // Valid transition
        
        // Partial credit for reasonable transitions
        if(prev_pos == "NOUN" && next_pos == "NOUN") return 0.4;  // Compound nouns OK
        if(prev_pos == "ADJECTIVE" && next_pos == "ADJECTIVE") return 0.3;  // Multiple adjectives possible
        
        return 0.0;  // Invalid transition
    }
    
    double sentence_grammar_score(const vector<string>& words);  // Declared below
    
    bool has_subject_verb(const vector<string>& words);
    bool has_complete_thought(const vector<string>& words);
};

// Negative reinforcement system
class NegativeReinforcement {
public:
    struct Punishment {
        string pattern;
        double intensity;
        string reason;
        int generation;
    };
    
    deque<Punishment> recent_punishments;
    map<string, double> pattern_penalties;
    map<string, int> pattern_counts;
    
    void punish(const string& output, const string& reason, double intensity, int gen) {
        Punishment p;
        p.pattern = output;
        p.intensity = intensity;
        p.reason = reason;
        p.generation = gen;
        
        recent_punishments.push_back(p);
        if(recent_punishments.size() > 50) {
            recent_punishments.pop_front();
        }
        
        // Store penalty for pattern
        pattern_penalties[output] += intensity;
        pattern_counts[output]++;
        
        // Generalize to substrings if pattern is long
        if(output.length() > 20) {
            vector<string> words;
            stringstream ss(output);
            string word;
            while(ss >> word) words.push_back(word);
            
            // Penalize 3-word phrases from this output
            for(size_t i = 0; i + 2 < words.size(); i++) {
                string phrase = words[i] + " " + words[i+1] + " " + words[i+2];
                pattern_penalties[phrase] += intensity * 0.3;
            }
        }
    }
    
    double get_penalty(const string& candidate) {
        double penalty = 0.0;
        
        // Direct match
        if(pattern_penalties.count(candidate)) {
            penalty += pattern_penalties[candidate];
        }
        
        // Partial match
        for(const auto& [pattern, pen] : pattern_penalties) {
            if(candidate.find(pattern) != string::npos) {
                penalty += pen * 0.5;
            }
        }
        
        return min(1.0, penalty);
    }
    
    void decay_penalties() {
        for(auto& [pattern, penalty] : pattern_penalties) {
            penalty *= 0.95;  // Gradual forgetting
        }
        
        // Remove very old/weak penalties
        vector<string> to_remove;
        for(auto& [pattern, penalty] : pattern_penalties) {
            if(penalty < 0.01) {
                to_remove.push_back(pattern);
            }
        }
        for(const auto& pattern : to_remove) {
            pattern_penalties.erase(pattern);
            pattern_counts.erase(pattern);
        }
    }
    
    int get_total_punishments() const {
        return recent_punishments.size();
    }
    
    string get_worst_pattern() const {
        if(pattern_penalties.empty()) return "";
        
        double max_penalty = 0.0;
        string worst = "";
        for(const auto& [pattern, penalty] : pattern_penalties) {
            if(penalty > max_penalty) {
                max_penalty = penalty;
                worst = pattern;
            }
        }
        return worst;
    }
};

// Diversity enforcement
class DiversityEnforcer {
private:
    deque<string> recent_outputs;
    const int HISTORY_SIZE = 30;  // Increased for better tracking
    RepetitionDetector detector;
    
public:
    void add_output(const string& output) {
        recent_outputs.push_back(output);
        if(recent_outputs.size() > HISTORY_SIZE) {
            recent_outputs.pop_front();
        }
    }
    
    double diversity_score(const string& candidate) {
        if(recent_outputs.empty()) return 1.0;
        
        double min_similarity = 1.0;
        for(const auto& prev : recent_outputs) {
            double sim = detector.semantic_similarity(candidate, prev);
            double ngram_sim = detector.ngram_overlap(candidate, prev, 3);
            double combined_sim = (sim + ngram_sim) / 2.0;
            min_similarity = min(min_similarity, 1.0 - combined_sim);
        }
        
        return max(0.0, min_similarity);
    }
    
    bool is_too_similar(const string& candidate, double threshold = 0.6) {
        for(const auto& prev : recent_outputs) {
            double sim = detector.semantic_similarity(candidate, prev);
            if(sim > threshold) return true;
            
            // Also check n-gram overlap
            double ngram_sim = detector.ngram_overlap(candidate, prev, 4);
            if(ngram_sim > threshold + 0.1) return true;
        }
        return false;
    }
    
    void force_diverse_mode() {
        // Clear half the history to allow more repetition temporarily
        while(recent_outputs.size() > HISTORY_SIZE / 2) {
            recent_outputs.pop_front();
        }
    }
};

// Perplexity calculator for quality measurement
class PerplexityCalculator {
public:
    double calculate_perplexity(const vector<string>& sentence) {
        if(sentence.size() < 2) return 1000.0;  // Poor quality indicator
        
        double log_prob_sum = 0.0;
        int count = 0;
        
        for(size_t i = 1; i < sentence.size(); i++) {
            string prev = sentence[i-1];
            string curr = sentence[i];
            
            // Get bigram probability
            if(bigram_counts.count(prev)) {
                auto& next_words = bigram_counts[prev];
                int total = 0;
                for(auto& [w, c] : next_words) total += c;
                
                if(next_words.count(curr) && total > 0) {
                    double prob = (double)next_words[curr] / total;
                    log_prob_sum += log(prob + 1e-10);  // Avoid log(0)
                    count++;
                }
            }
        }
        
        if(count == 0) return 1000.0;
        
        double avg_log_prob = log_prob_sum / count;
        return exp(-avg_log_prob);
    }
    
    bool is_coherent(const vector<string>& sentence, double threshold = 50.0) {
        double perplexity = calculate_perplexity(sentence);
        return perplexity < threshold;
    }
};

// Coherence scorer - combines multiple metrics
class CoherenceScorer {
private:
    GrammarValidator grammar;
    RepetitionDetector repetition;
    PerplexityCalculator perplexity;
    
public:
    struct CoherenceReport {
        double grammar_score;
        double repetition_score;
        double perplexity;
        double diversity_score;
        double overall_score;
        vector<string> issues;
    };
    
    CoherenceReport evaluate(const vector<string>& sentence, const string& sentence_str,
                            DiversityEnforcer& diversity) {
        CoherenceReport report;
        
        // Grammar
        report.grammar_score = grammar.sentence_grammar_score(sentence);
        if(report.grammar_score < 0.4) {
            report.issues.push_back("poor_grammar");
        }
        
        // Repetition
        double rep_score = repetition.word_repetition_score(sentence);
        report.repetition_score = max(0.0, 1.0 - rep_score);
        if(repetition.has_phrase_repetition(sentence_str)) {
            report.issues.push_back("phrase_repetition");
            report.repetition_score *= 0.5;
        }
        if(repetition.has_char_repetition(sentence_str)) {
            report.issues.push_back("char_repetition");
            report.repetition_score *= 0.3;
        }
        
        // Perplexity
        report.perplexity = perplexity.calculate_perplexity(sentence);
        double perp_score = 1.0 / (1.0 + report.perplexity / 50.0);
        if(report.perplexity > 100.0) {
            report.issues.push_back("high_perplexity");
        }
        
        // Diversity
        report.diversity_score = diversity.diversity_score(sentence_str);
        if(report.diversity_score < 0.3) {
            report.issues.push_back("low_diversity");
        }
        
        // Overall score (weighted combination)
        report.overall_score = 
            report.grammar_score * 0.3 +
            report.repetition_score * 0.3 +
            perp_score * 0.2 +
            report.diversity_score * 0.2;
        
        return report;
    }
};

// Global instances
extern TokenWellDetector token_well;
extern RepetitionDetector rep_detector;
extern ContextlessMemory contextless_mem;
extern GrammarValidator grammar_validator;
extern NegativeReinforcement neg_reinforce;
extern DiversityEnforcer diversity_enforcer;
extern CoherenceScorer coherence_scorer;
extern vector<NegativeSignal> negative_signals;

// Helper function declarations
string getPartOfSpeech(const string& word);

// Implementation of functions that need getPartOfSpeech
inline double GrammarValidator::sentence_grammar_score(const vector<string>& words) {
    if(words.size() < 2) return 0.5;
    
    double score = 0.0;
    for(size_t i = 0; i < words.size() - 1; i++) {
        string pos1 = getPartOfSpeech(words[i]);
        string pos2 = getPartOfSpeech(words[i+1]);
        score += validate_transition(pos1, pos2);
    }
    
    return score / (words.size() - 1);
}

inline bool GrammarValidator::has_subject_verb(const vector<string>& words) {
    bool has_subject = false;
    bool has_verb = false;
    
    for(const auto& word : words) {
        string pos = getPartOfSpeech(word);
        if(pos == "PRONOUN" || pos == "NOUN") has_subject = true;
        if(pos == "VERB" || pos == "BE_VERB") has_verb = true;
    }
    
    return has_subject && has_verb;
}

inline bool GrammarValidator::has_complete_thought(const vector<string>& words) {
    if(words.size() < 3) return false;
    return has_subject_verb(words);
}

#endif // ENHANCED_COHERENCE_H
