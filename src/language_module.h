// language_module.h - Advanced linguistic processing module
#ifndef LANGUAGE_MODULE_H
#define LANGUAGE_MODULE_H

#include <string>
#include <vector>
#include <map>
#include <queue>

// Forward declarations
struct Token;
struct Concept;

// Bigram structure for statistical language modeling
struct Bigram {
    std::string sequence;
    int frequency;
    double coherence;
};

// Trigram structure
struct Trigram {
    std::string sequence;
    int frequency;
    double coherence;
};

// Markov chain node for text generation
struct MarkovNode {
    std::map<std::string, double> transitions;
    int total_count;
};

// Advanced language state
struct LanguageState {
    std::map<std::string, Bigram> bigrams;
    std::map<std::string, Trigram> trigrams;
    std::map<std::string, MarkovNode> markov_chain;
    double markov_coherence;
    double vocab_scaling_factor;
    double semantic_density;
    double lexical_diversity;
};

// Global language state
extern LanguageState LANG;

// Learn n-grams from word sequence
void learn_ngram(const std::vector<std::string>& words);

// Build Markov chain from words
void build_markov_chain(const std::vector<std::string>& words);

// Calculate Markov coherence score
double calc_markov_coherence();

// Generate text from Markov chain
std::string generate_from_markov(const std::string& seed, int max_length);

// Calculate vocabulary scaling factor
double calc_vocab_scaling();

// Calculate semantic density
double calc_semantic_density();

// Calculate lexical diversity (Type-Token Ratio)
double calc_lexical_diversity();

// Hook into existing token system
void integrate_with_tokens(std::map<std::string, Token>& tokens);

// Hook into existing concept system
void integrate_with_concepts(std::map<std::string, Concept>& concepts);

#endif
