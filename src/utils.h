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

std::unordered_map<unsigned int, std::shared_ptr<Sequence>> readInputData(std::string inputDataPath);
/*
    Utility functions for showing certain objects' information
*/
void print_sequences(std::unordered_map<int, Sequence> sequences);
void print_pattern(Pattern pattern);
void print_siduls(std::unordered_map<int, Pattern> sidulItems);
/*
    Utility function for initializing SIDULs of patterns
*/
std::unordered_map<unsigned int, std::shared_ptr<Pattern>> construct_siduls(std::unordered_map<unsigned int, std::shared_ptr<Sequence>> sequences);
/*
    Algorithm for pruning invalid patterns by LRU and Support
*/
std::unordered_map<unsigned int, std::shared_ptr<Sequence>> WPS_by_LRU_and_Support(
    std::unordered_map<unsigned int, std::shared_ptr<Sequence>> sequences,
    std::unordered_map<unsigned int, std::shared_ptr<Pattern>> sidulItems,
    float MIN_SUPP,
    float MIN_UTILITY
);
/*
    Utility functions for extending patterns
    For speeding up, metrics (RBU, umin, SE, SLIP) are computed as well
*/
std::shared_ptr<Pattern> construct_i_ext(
    std::shared_ptr<Pattern> pattern,
    std::shared_ptr<Pattern> item, 
    std::unordered_map<unsigned int, std::shared_ptr<Sequence>> sequences
);
std::shared_ptr<Pattern> construct_s_ext(
    std::shared_ptr<Pattern> pattern,
    std::shared_ptr<Pattern> item, 
    std::unordered_map<unsigned int, std::shared_ptr<Sequence>> sequences
);
/*
    Utility functions for computing certain pattern metrics and pattern comparison
*/
float computeRBU(Pattern pattern);
float computeUmin(Pattern pattern);
unsigned int computeSE(Pattern pattern, std::unordered_map<unsigned int, Sequence> sequences);
unsigned int computeSLIP(Pattern pattern);
bool isContainedBy(std::string superPatternName, std::string subPatternName);
