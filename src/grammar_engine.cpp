#include "grammar_engine.h"
#include <map>
#include <string>
#include <vector>
#include <utility>   // For std::pair
#include <algorithm> // For std::min, std::max
#include <cmath>

// Use using declarations for cleaner syntax
using std::string;
using std::vector;
using std::map;
using std::pair;

// Properly scope and define static members
map<string, int> GrammarEngine::pos_map_;
map<pair<int, int>, double> GrammarEngine::transition_scores_;

void GrammarEngine::initialize() {
    // Basic POS mapping
    pos_map_["the"] = 1;   // Determiner
    pos_map_["logic"] = 2; // Noun
    pos_map_["is"] = 3;    // Verb
    pos_map_["sound"] = 4; // Adjective

    // Transition scoring: pair<prev_pos, current_pos>
    transition_scores_[{1, 2}] = 0.9; // Det -> Noun (High probability)
    transition_scores_[{2, 3}] = 0.8; // Noun -> Verb
    transition_scores_[{3, 4}] = 0.7; // Verb -> Adj
}

int GrammarEngine::get_pos(const string& word) {
    if (pos_map_.contains(word)) {
        return pos_map_[word];
    }
    return 0; // Unknown
}

double GrammarEngine::score_transition(const string& prev, const string& current) {
    int p1 = get_pos(prev);
    int p2 = get_pos(current);
    
    pair<int, int> transition = {p1, p2};
    if (transition_scores_.contains(transition)) {
        return transition_scores_[transition];
    }
    return 0.1; // Default low score for unknown transitions
}

double GrammarEngine::calculate_sentence_coherence(const vector<string>& tokens) {
    if (tokens.size() < 2) return 1.0;

    double total_score = 0.0;
    for (size_t i = 1; i < tokens.size(); ++i) {
        double trans_score = score_transition(tokens[i-1], tokens[i]);
        // Fixed: Use std::max
        total_score += std::max(0.0, trans_score);
    }

    double coherence = total_score / (tokens.size() - 1);
    // Fixed: Use std::min and std::max
    return std::min(1.0, std::max(0.0, coherence));
}