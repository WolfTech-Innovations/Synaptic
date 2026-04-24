#pragma once
#ifndef DIGITZ_STATE_H
#define DIGITZ_STATE_H

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include "uac.h"
#include <map>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>
#include <functional>
#include <iomanip>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
using namespace std;

// Include the struct definitions from the shared header
#include "struct.h"

// External variable declarations
extern random_device rd;
extern mt19937 rng;
extern State S;
extern State BK;
extern WorkingMemory WM;
extern map<string, TokenConceptEmbedding> token_concept_embedding_map;
extern map<string, Goal> goal_system;
extern WorldModel world_model;
extern ConsciousnessState consciousness;
extern ConsciousnessFormula consciousness_formula;
extern vector<string> sentence_templates;

// Function declarations
double rn();
int ri(int mx);
long long hsh(const string &s);
long long hsh(double d);
string tokenize(const string &text);

string generateResponse(const string &input);
double calcSentienceRatio();
string get_embodiment_report();
void update_all_modules(State &S);
void sv(const string &f);
void ld(const string &f);

void storeEpisodicMemory(const string &content, double valence);
void counterfactualAnalysis();
void mathLangAssociation();
void learnWord(const string &word, double concept_value);
void createConceptAssociation(const string &concept_name, const vector<string> &related_words);
void loadEnglishDataset();
void downloadVocabulary();
void mutateN();
void runCode(const string &c);
void batch16Process();
void updateAttention();
void bk();
void rb();

#endif // DIGITZ_STATE_H