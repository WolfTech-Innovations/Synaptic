#pragma once
#ifndef GRAMMAR_ENGINE_H
#define GRAMMAR_ENGINE_H

#include <string>
#include <vector>
#include <map>

struct GrammaticalToken {
    std::string word;
    int pos_type;
    double score;
};

class GrammarEngine {
public:
    static void initialize();
    static int get_pos(const std::string& word);
    static double score_transition(const std::string& prev, const std::string& current);
    static bool is_valid_transition(int prev_pos, int current_pos);
    static double calculate_sentence_coherence(const std::vector<std::string>& tokens);
    
private:
    static std::map<std::string, int> pos_map_;
    static std::map<std::pair<int,int>, double> transition_scores_;
};

#endif
