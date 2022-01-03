#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <iostream>
#include <numeric>
#include <boost/algorithm/string.hpp>
#include "models.h"

const std::string SEQUENCES_FILE = "sequences.csv";
const std::string UTILITIES_FILE = "utilities.csv";
const int END_ITEMSET = -1;
const int END_SEQUENCE = -2;

std::unordered_map<unsigned int, Sequence> readInputData(std::string inputDataPath);
/*
    Utility functions for showing certain objects' information
*/
void print_sequences(std::unordered_map<int, Sequence> sequences);
void print_pattern(Pattern pattern);
void print_siduls(std::unordered_map<int, Pattern> sidulItems);
/*
    Utility function for initializing SIDULs of patterns
*/
std::unordered_map<unsigned int, Pattern> construct_siduls(std::unordered_map<unsigned int, Sequence> sequences);
/*
    Algorithm for pruning invalid patterns by LRU and Support
*/
std::unordered_map<unsigned int, Sequence> WPS_by_LRU_and_Support(
    std::unordered_map<unsigned int, Sequence> sequences,
    std::unordered_map<unsigned int, Pattern> sidulItems,
    float MIN_SUPP,
    float MIN_UTILITY
);
/*
    Utility functions for extending patterns
    For speeding up, metrics (RBU, umin, SE, SLIP) are computed as well
*/
Pattern construct_i_ext(Pattern pattern, Pattern item, std::unordered_map<unsigned int, Sequence> sequences);
Pattern construct_s_ext(Pattern pattern, Pattern item, std::unordered_map<unsigned int, Sequence> sequences);
/*
    Utility functions for computing certain pattern metrics and pattern comparison
*/
float computeRBU(Pattern pattern);
float computeUmin(Pattern pattern);
unsigned int computeSE(Pattern pattern, std::unordered_map<unsigned int, Sequence> sequences);
unsigned int computeSLIP(Pattern pattern);
bool isContainedBy(std::string superPatternName, std::string subPatternName);
/*
    Core algorithms
*/
void LocalPruningCHU(Pattern& superPattern, Pattern& subPattern);
bool UpdateFMaxCloHUS(
    Pattern pattern,
    std::unordered_map<unsigned int,
    Sequence> sequences,
    const float MIN_UTILITY,
    std::list<Pattern> &FCHUS
);
void DFS_FMaxCloHUS(
    Pattern pattern,
    std::unordered_map<unsigned int, Pattern> I,
    std::unordered_map<unsigned int, Pattern> S,
    std::unordered_map<unsigned int, Pattern> IList,
    std::unordered_map<unsigned int, Pattern> SList,
    std::unordered_map<unsigned int, Sequence> sequences,
    const float MIN_SUPP,
    const float MIN_UTILITY,
    std::list<Pattern> &FCHUS
);
