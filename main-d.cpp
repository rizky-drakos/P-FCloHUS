#include <iostream>
#include <vector>
#include <utility>
#include <unordered_map>
#include "src/utils.h"

class PatternD {
    public:
        std::vector<std::string> name;
        std::vector<std::pair<int, int>> sidsets;
        std::vector<std::pair<float, float>> utilities;
        float RBU;
        float UMIN;
        unsigned int SE;
        unsigned int SLIP;
};

void print_patternd(const PatternD pattern) {
    std::cout << pattern.name[0] << ": " << std::endl;
    for (auto sidset : pattern.sidsets) std::cout << sidset.first << "/" << sidset.second << " ";
    std::cout << std::endl;
    for (auto utility : pattern.utilities) std::cout << utility.first << "/" << utility.second << " ";
    std::cout << std::endl;
    std::cout <<  pattern.UMIN << "/" <<  pattern.RBU << "/" <<  pattern.SE << "/" << pattern.SLIP << std::endl;
}

std::unordered_map<unsigned int, PatternD> construct_initialPatterns(std::unordered_map<unsigned int, Sequence> sequences) {
    std::unordered_map<unsigned int, PatternD> initialPatterns;
    for (auto seq : sequences) {
        unsigned int itemsetIdx = 0;
        float prefixUtility = 0;
        std::unordered_map<unsigned int, float> uminInSequence;
        std::unordered_map<unsigned int, int> SLIPInSequence;
        std::unordered_map<unsigned int, int> firstInstanceInSequence;
        for (auto item : seq.second.items) {
            if (item.id != END_ITEMSET && item.id != END_SEQUENCE) {
                if (initialPatterns.find(item.id) == initialPatterns.end()) initialPatterns[item.id].name.push_back(std::to_string(item.id));
                /*
                    When an item first appears in a sequence:
                        - initialize its umin to max of float.
                        - reserve its first instance position in this sequence.
                */
                if (uminInSequence.find(item.id) == uminInSequence.end()) {
                    uminInSequence[item.id] = std::numeric_limits<float>::max();
                    firstInstanceInSequence[item.id] = initialPatterns[item.id].sidsets.size();
                }
                initialPatterns[item.id].sidsets.push_back(std::make_pair(seq.first, itemsetIdx));
                initialPatterns[item.id].utilities.push_back(std::make_pair(
                    item.utility,
                    seq.second.utility - (prefixUtility += item.utility))
                );
                /*
                    We seek for the smallest umin among all of this instances.
                */
                uminInSequence[item.id] = std::min(uminInSequence[item.id], item.utility);
            } else ++itemsetIdx;
        }
        for (auto item : uminInSequence) {
            initialPatterns[item.first].UMIN += item.second;
            initialPatterns[item.first].RBU += initialPatterns[item.first].utilities[firstInstanceInSequence[item.first]].first + initialPatterns[item.first].utilities[firstInstanceInSequence[item.first]].second;
            initialPatterns[item.first].SE += (sequences[seq.first].size - (initialPatterns[item.first].sidsets[firstInstanceInSequence[item.first]].second+1) + 1);
            initialPatterns[item.first].SLIP = initialPatterns[item.first].sidsets.size();
        }
    }

    return initialPatterns;
}

PatternD construct_i(PatternD pattern, PatternD item, std::unordered_map<unsigned int, Sequence> sequences) {
    PatternD extendedPattern;
    extendedPattern.name = pattern.name;
    extendedPattern.name.push_back(item.name[0]);

    std::unordered_map<int, int> commonPosWithIndex;
    std::unordered_map<int, float> uminInSequence;
    
    for (int patternPosIndex = 0; patternPosIndex < pattern.sidsets.size(); ++patternPosIndex) 
        commonPosWithIndex.insert({pattern.sidsets[patternPosIndex].first*100 + pattern.sidsets[patternPosIndex].second, patternPosIndex});
    
    for (int itemPosIndex = 0; itemPosIndex < item.sidsets.size(); ++itemPosIndex) {
        const int itemPos = item.sidsets[itemPosIndex].first*100 + item.sidsets[itemPosIndex].second;
        if (commonPosWithIndex.find(itemPos) != commonPosWithIndex.end()) {
            const int patternPosIdx = commonPosWithIndex[itemPos];
            const float instanceUmin = pattern.utilities[patternPosIdx].first + item.utilities[itemPosIndex].first;
            extendedPattern.sidsets.push_back(item.sidsets[itemPosIndex]);
            extendedPattern.utilities.push_back(std::make_pair(instanceUmin, item.utilities[itemPosIndex].second));

            if (uminInSequence.find(item.sidsets[itemPosIndex].first) == uminInSequence.end()) {
                uminInSequence[item.sidsets[itemPosIndex].first] = std::numeric_limits<float>::max();
                extendedPattern.RBU += instanceUmin + item.utilities[itemPosIndex].second;
                extendedPattern.SE += (sequences[item.sidsets[itemPosIndex].first].size - (item.sidsets[itemPosIndex].second+1) + 1);
            }

            uminInSequence[item.sidsets[itemPosIndex].first] = std::min(uminInSequence[item.sidsets[itemPosIndex].first], instanceUmin);
        }
    }
    extendedPattern.SLIP += extendedPattern.sidsets.size();
    for (auto umin : uminInSequence) extendedPattern.UMIN += umin.second;

    return extendedPattern;
}

PatternD construct_s_ext(PatternD pattern, PatternD item, std::unordered_map<unsigned int, Sequence> sequences) {
    PatternD extendedPattern;
    extendedPattern.name = pattern.name;
    extendedPattern.name.push_back("-1");
    extendedPattern.name.push_back(item.name[0]);

    // for (auto seq : item.siduls) {
    //     float uminInSequence = std::numeric_limits<float>::max();
    //     if (commonSeqs.find(seq.first) != commonSeqs.end()) {
    //         for (auto itemInstance : item.siduls[seq.first]) {
    //             float newUmin = std::numeric_limits<float>::max();
    //             unsigned int patternInstanceIdx = 0;
    //             auto patternInstance = pattern.siduls[seq.first][patternInstanceIdx];
    //             while (patternInstance.position < itemInstance.position) {
    //                     float currentUmin = patternInstance.utility + itemInstance.utility;
    //                     newUmin = std::min(newUmin, currentUmin);
                        /*
                            Need to carefully handle the index, refs:
                            https://stackoverflow.com/questions/27754726/no-compilation-error-or-run-time-error-when-index-out-of-range-of-vector-class
                        */    
    //                     if (++patternInstanceIdx >= pattern.siduls[seq.first].size()) break;          
    //                     patternInstance = pattern.siduls[seq.first][patternInstanceIdx];
    //             }

    //             if (newUmin != std::numeric_limits<float>::max()) {
    //                 extendedPattern.siduls[seq.first].push_back(ItemInstance(
    //                     newUmin,
    //                     itemInstance.rem,
    //                     itemInstance.position
    //                 ));
    //                 uminInSequence = std::min(uminInSequence, newUmin);
    //             }
    //         }
    //         if (uminInSequence != std::numeric_limits<float>::max()) {
    //             extendedPattern.umin += uminInSequence;
    //             extendedPattern.RBU += (extendedPattern.siduls[seq.first][0].utility + extendedPattern.siduls[seq.first][0].rem);
    //             extendedPattern.SE += (sequences[seq.first].size - (extendedPattern.siduls[seq.first][0].position+1) + 1);
    //             extendedPattern.SLIP += extendedPattern.siduls[seq.first].size();
    //         }
    //     }
    // }

    std::unordered_map<int, int> commonPosWithIndex;
    for (int patternPosIndex = 0; patternPosIndex < pattern.sidsets.size(); ++patternPosIndex) 
        commonPosWithIndex.insert({pattern.sidsets[patternPosIndex].first, patternPosIndex});

    for (int itemPosIndex = 0; itemPosIndex < item.sidsets.size(); ++itemPosIndex) { {
        
    }

    return extendedPattern;
}

int main(int argvc, char** argv) {
    const std::string INPUT_DATA_PATH = argv[1];
    std::unordered_map<unsigned int, Sequence> sequences = readInputData(INPUT_DATA_PATH);
    std::unordered_map<unsigned int, PatternD> initialPatterns = construct_initialPatterns(sequences);

    return 0;
}