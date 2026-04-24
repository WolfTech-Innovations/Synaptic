// language_module.cpp - Implementation of advanced linguistic processing
#include "language_module.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include <random>

// External references from main code
struct Token {
    std::string word;
    double meaning;
    double freq;
    std::vector<int> associations;
    int pos_hint;
    double coherence_score;
};

struct Concept {
    std::string name;
    double value;
    std::vector<std::string> related_words;
};

LanguageState LANG;

void learn_ngram(const std::vector<std::string>& words) {
    // Learn bigrams
    for (size_t i = 0; i < words.size() - 1; i++) {
        std::string bg = words[i] + " " + words[i + 1];
        if (LANG.bigrams.count(bg)) {
            LANG.bigrams[bg].frequency++;
            LANG.bigrams[bg].coherence += 0.01;
        } else {
            LANG.bigrams[bg] = {bg, 1, 0.5};
        }
    }
    
    // Learn trigrams
    for (size_t i = 0; i < words.size() - 2; i++) {
        std::string tg = words[i] + " " + words[i + 1] + " " + words[i + 2];
        if (LANG.trigrams.count(tg)) {
            LANG.trigrams[tg].frequency++;
            LANG.trigrams[tg].coherence += 0.02;
        } else {
            LANG.trigrams[tg] = {tg, 1, 0.6};
        }
    }
}

void build_markov_chain(const std::vector<std::string>& words) {
    for (size_t i = 0; i < words.size() - 1; i++) {
        const std::string& current = words[i];
        const std::string& next = words[i + 1];
        
        if (!LANG.markov_chain.count(current)) {
            LANG.markov_chain[current] = {{{next, 1.0}}, 1};
        } else {
            auto& node = LANG.markov_chain[current];
            if (node.transitions.count(next)) {
                node.transitions[next] += 1.0;
            } else {
                node.transitions[next] = 1.0;
            }
            node.total_count++;
        }
    }
    
    // Normalize probabilities
    for (auto& mc : LANG.markov_chain) {
        for (auto& trans : mc.second.transitions) {
            trans.second = trans.second / mc.second.total_count;
        }
    }
}

double calc_markov_coherence() {
    if (LANG.markov_chain.empty()) return 0.0;
    
    double total_entropy = 0;
    int count = 0;
    
    for (auto& mc : LANG.markov_chain) {
        double entropy = 0;
        for (auto& trans : mc.second.transitions) {
            double p = trans.second;
            if (p > 0.001) {
                entropy -= p * std::log2(p);
            }
        }
        total_entropy += entropy;
        count++;
    }
    
    double avg_entropy = count > 0 ? total_entropy / count : 0;
    LANG.markov_coherence = std::max(0.0, std::min(1.0, 1.0 - avg_entropy / 4.0));
    return LANG.markov_coherence;
}

std::string generate_from_markov(const std::string& seed, int max_length) {
    if (!LANG.markov_chain.count(seed)) return seed;
    
    std::string result = seed;
    std::string current = seed;
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (int i = 0; i < max_length; i++) {
        if (!LANG.markov_chain.count(current)) break;
        
        auto& node = LANG.markov_chain[current];
        if (node.transitions.empty()) break;
        
        // Random selection based on probabilities
        std::uniform_real_distribution<> dis(0.0, 1.0);
        double r = dis(gen);
        double cumulative = 0;
        std::string next_word = "";
        
        for (auto& trans : node.transitions) {
            cumulative += trans.second;
            if (r <= cumulative) {
                next_word = trans.first;
                break;
            }
        }
        
        if (next_word.empty()) break;
        result += " " + next_word;
        current = next_word;
    }
    
    return result;
}

double calc_vocab_scaling() {
    // Count unique bigrams and trigrams
    double vocab_size = LANG.bigrams.size() + LANG.trigrams.size();
    double base_scale = std::log(vocab_size + 1) / std::log(1000);
    
    // Count used items
    int used_count = 0;
    for (auto& bg : LANG.bigrams) {
        if (bg.second.frequency > 0) used_count++;
    }
    
    double usage_ratio = vocab_size > 0 ? used_count / vocab_size : 0;
    
    LANG.vocab_scaling_factor = std::max(0.1, std::min(2.0, base_scale * 0.5 + usage_ratio * 0.5));
    return LANG.vocab_scaling_factor;
}

double calc_semantic_density() {
    // This will be calculated based on concept/token ratio when integrated
    LANG.semantic_density = 0.5; // Placeholder
    return LANG.semantic_density;
}

double calc_lexical_diversity() {
    if (LANG.bigrams.empty()) return 0.0;
    
    int unique_words = 0;
    int total_freq = 0;
    
    for (auto& bg : LANG.bigrams) {
        unique_words++;
        total_freq += bg.second.frequency;
    }
    
    // Type-Token Ratio
    double ttr = total_freq > 0 ? (double)unique_words / total_freq : 0;
    
    // Hapax ratio (words appearing once)
    int hapax_count = 0;
    for (auto& bg : LANG.bigrams) {
        if (bg.second.frequency == 1) hapax_count++;
    }
    double hapax_ratio = unique_words > 0 ? (double)hapax_count / unique_words : 0;
    
    LANG.lexical_diversity = std::max(0.0, std::min(1.0, ttr * 0.6 + hapax_ratio * 0.4));
    return LANG.lexical_diversity;
}

void integrate_with_tokens(std::map<std::string, Token>& tokens) {
    // Calculate semantic density based on tokens
    double token_count = tokens.size();
    double bigram_count = LANG.bigrams.size();
    
    if (token_count > 0) {
        LANG.semantic_density = std::min(1.0, bigram_count / (token_count * 2.0));
    }
}

void integrate_with_concepts(std::map<std::string, Concept>& concepts) {
    // Update semantic density with concept information
    double concept_count = concepts.size();
    if (concept_count > 0) {
        LANG.semantic_density = std::min(1.0, LANG.semantic_density + concept_count / 100.0);
    }
}
