#include "utils.h"

/*
    Update the input data so that we only use one loop.
*/
std::unordered_map<unsigned int, Sequence> readInputData(std::string inputDataPath) {
    std::unordered_map<unsigned int, Sequence> sequences;

    std::fstream sequencesFile (inputDataPath + "/" + SEQUENCES_FILE);
    std::fstream utilitiesFile (inputDataPath + "/" + UTILITIES_FILE);

    unsigned int seqID = 0;
    std::string line;
    std::vector<std::string> tokens;

    while (std::getline(sequencesFile, line)) {
        Sequence sequence;
        boost::split(tokens, line, boost::is_any_of("\t"));
        for (int i = 0; i < tokens.size(); i++) {
            int item = std::stoi(tokens[i]);
            if (item == END_ITEMSET || item == END_SEQUENCE) ++sequence.size;
            sequence.items.push_back(Item(item));
        }
        sequences[seqID] = sequence;

        ++seqID;
    }

    seqID = 0;
    while (std::getline(utilitiesFile, line)) {

        unsigned int itemIdx = 0;
        boost::split(tokens, line, boost::is_any_of("\t"));
        for (auto const& token : tokens) {
            float itemUtility = std::stof(token);
            sequences[seqID].items[itemIdx].utility = itemUtility;
            sequences[seqID].utility += itemUtility;
            ++itemIdx;
        }

        ++seqID;
    }

    return sequences;
}

void print_sequences(std::unordered_map<int, Sequence> sequences) {
    for (auto seq : sequences) {
        std::cout << seq.first << "(" << seq.second.utility << "): ";
        for (auto item : seq.second.items) std::cout << item.id << "/" << item.utility << " "; 
        std::cout << std::endl;
    }
}

void print_pattern(Pattern pattern) {
    std::cout << pattern.name << 
        ", lastItem: " << pattern.lastItem <<
        ", RBU: " << pattern.RBU <<
        ", umin: " << pattern.umin <<
        ", SE: " << pattern.SE <<
        ", SLIP: " << pattern.SLIP
        << std::endl;
    for (auto seq : pattern.siduls) {
        std::cout << seq.first << std::endl;
        for (auto instance : seq.second) std::cout << instance.position << "/" << instance.utility << "/" << instance.rem << std::endl;
        std::cout << std::endl;
    }
}

void print_siduls(std::unordered_map<int, Pattern> sidulItems) {
    std::cout << "# of items: " << sidulItems.size() << std::endl;
    for (auto item : sidulItems) {
        std::cout << "Item: " << item.first << std::endl;
        for (auto seq : item.second.siduls) {
            std::cout << "Seq: " << seq.first << ": ";
            for (auto instance : seq.second)
                std::cout << instance.position << "/" << instance.utility << "/" << instance.rem << "->";
            std::cout << std::endl;
        }
    }
}

std::unordered_map<unsigned int, Pattern> construct_siduls(std::unordered_map<unsigned int, Sequence> sequences) {
    std::unordered_map<unsigned int, Pattern> sidulItems;
    for (auto seq : sequences) {
        /*

        */
        std::unordered_map<unsigned int, float> uminInSequence;

        unsigned int itemsetIdx = 0;
        float prefixUtility = 0;
        for (auto item : seq.second.items) {
            if (item.id != END_ITEMSET && item.id != END_SEQUENCE) {
                if (sidulItems.find(item.id) == sidulItems.end()) {
                    sidulItems[item.id].lastItem = item.id;
                    ++sidulItems[item.id].size;
                    /*
                        If performance is all what we need, remove the name.
                    */
                    sidulItems[item.id].name.append(std::to_string(item.id));
                }
                sidulItems[item.id].siduls[seq.first].push_back(ItemInstance(
                    item.utility,
                    seq.second.utility - (prefixUtility += item.utility),
                    itemsetIdx
                ));
                /*
                    When an item first appears in a sequence, initialize its umin to max of float.
                    We seek for the smallest umin among all of this instances.
                */
                if (uminInSequence.find(item.id) == uminInSequence.end()) uminInSequence[item.id] = std::numeric_limits<float>::max();
                uminInSequence[item.id] = std::min(uminInSequence[item.id], item.utility);
            } else ++itemsetIdx;
        }
        for (auto item : uminInSequence) {
            sidulItems[item.first].umin += item.second;
            sidulItems[item.first].RBU += (sidulItems[item.first].siduls[seq.first][0].utility + sidulItems[item.first].siduls[seq.first][0].rem);
            sidulItems[item.first].SE += (sequences[seq.first].size - (sidulItems[item.first].siduls[seq.first][0].position+1) + 1);
            sidulItems[item.first].SLIP += sidulItems[item.first].siduls[seq.first].size();
        }
    }
    return sidulItems;
}

std::unordered_map<unsigned int, Sequence> WPS_by_LRU_and_Support(
    std::unordered_map<unsigned int, Sequence> sequences,
    std::unordered_map<unsigned int, Pattern> sidulItems,
    float MIN_SUPP,
    float MIN_UTILITY
) {
    std::unordered_map<int, float> LRUByItem;
    std::unordered_map<unsigned int, Sequence> updatedSequences;
    for (auto seq : sequences) {
        Sequence updatedSequence;
        bool itemAdded = false;
        for (auto item : seq.second.items) {
            if (item.id != END_ITEMSET && item.id != END_SEQUENCE) {
                if (LRUByItem.find(item.id) == LRUByItem.end()) 
                    for (auto seq : sidulItems[item.id].siduls) LRUByItem[item.id] += sequences[seq.first].utility;
                if (sidulItems[item.id].siduls.size() >= MIN_SUPP && LRUByItem[item.id] >= MIN_UTILITY) {
                    itemAdded = true;
                    updatedSequence.utility += item.utility;
                    updatedSequence.items.push_back(item);
                }
            } else {
                if (itemAdded) {
                    ++updatedSequence.size;
                    itemAdded = false;
                    updatedSequence.items.push_back(item);
                }
            }
        }
        if (updatedSequence.items.size()) updatedSequences[seq.first] = updatedSequence;
    }

    return updatedSequences;
}

Pattern construct_i_ext(Pattern pattern, Pattern item, std::unordered_map<unsigned int, Sequence> sequences) {
    Pattern extendedPattern = Pattern();
    extendedPattern.isSExt = false;
    extendedPattern.lastItem = item.lastItem;
    extendedPattern.parentLastItem = pattern.lastItem;
    extendedPattern.isParentSExt = pattern.isSExt;
    extendedPattern.name = pattern.name.append(" ").append(item.name);
    extendedPattern.size = pattern.size;

    std::unordered_set<unsigned int> commonSeqs;
    for (auto seq : pattern.siduls) commonSeqs.insert(seq.first);
    for (auto seq : item.siduls) {
        float uminInSequence = std::numeric_limits<float>::max();
        if (commonSeqs.find(seq.first) != commonSeqs.end()) {
            std::unordered_map<unsigned int, unsigned int> commonPoss;
            for (int idx = 0; idx < item.siduls[seq.first].size(); ++idx) 
                commonPoss.insert({item.siduls[seq.first][idx].position, idx});
            for (auto patternInstance : pattern.siduls[seq.first])
                if (commonPoss.find(patternInstance.position) != commonPoss.end()) {
                    const float instanceUmin = patternInstance.utility + item.siduls[seq.first][commonPoss[patternInstance.position]].utility;
                    extendedPattern.siduls[seq.first].push_back(ItemInstance(
                        instanceUmin,
                        item.siduls[seq.first][commonPoss[patternInstance.position]].rem,
                        item.siduls[seq.first][commonPoss[patternInstance.position]].position
                    ));
                    uminInSequence = std::min(uminInSequence, instanceUmin);
                }
            /*
                When they both appear in the same sequence, but they do not share any common position(s).
            */    
            if (uminInSequence != std::numeric_limits<float>::max()) {
                extendedPattern.umin += uminInSequence;
                extendedPattern.RBU += (extendedPattern.siduls[seq.first][0].utility + extendedPattern.siduls[seq.first][0].rem);
                extendedPattern.SE += (sequences[seq.first].size - (extendedPattern.siduls[seq.first][0].position+1) + 1);
                extendedPattern.SLIP += extendedPattern.siduls[seq.first].size();
            }
        }
    }
    return extendedPattern;
}

Pattern construct_s_ext(Pattern pattern, Pattern item, std::unordered_map<unsigned int, Sequence> sequences) {
    Pattern extendedPattern = Pattern();
    extendedPattern.lastItem = item.lastItem;
    extendedPattern.isParentSExt = pattern.isSExt;
    extendedPattern.parentLastItem = pattern.lastItem;
    extendedPattern.name = pattern.name.append(" -1 ").append(item.name);
    extendedPattern.size = pattern.size + 1;

    std::unordered_set<unsigned int> commonSeqs;
    for (auto seq : pattern.siduls) commonSeqs.insert(seq.first);
    for (auto seq : item.siduls) {
        float uminInSequence = std::numeric_limits<float>::max();
        if (commonSeqs.find(seq.first) != commonSeqs.end()) {
            for (auto itemInstance : item.siduls[seq.first]) {
                float newUmin = std::numeric_limits<float>::max();
                unsigned int patternInstanceIdx = 0;
                auto patternInstance = pattern.siduls[seq.first][patternInstanceIdx];
                while (patternInstance.position < itemInstance.position) {
                        float currentUmin = patternInstance.utility + itemInstance.utility;
                        newUmin = std::min(newUmin, currentUmin);
                        /*
                            Need to carefully handle the index, refs:
                            https://stackoverflow.com/questions/27754726/no-compilation-error-or-run-time-error-when-index-out-of-range-of-vector-class
                        */    
                        if (++patternInstanceIdx >= pattern.siduls[seq.first].size()) break;          
                        patternInstance = pattern.siduls[seq.first][patternInstanceIdx];
                }

                if (newUmin != std::numeric_limits<float>::max()) {
                    extendedPattern.siduls[seq.first].push_back(ItemInstance(
                        newUmin,
                        itemInstance.rem,
                        itemInstance.position
                    ));
                    uminInSequence = std::min(uminInSequence, newUmin);
                }
            }
            /*
                When they both appear in the same sequence, but they do not share any common position(s).
            */
            if (uminInSequence != std::numeric_limits<float>::max()) {
                extendedPattern.umin += uminInSequence;
                extendedPattern.RBU += (extendedPattern.siduls[seq.first][0].utility + extendedPattern.siduls[seq.first][0].rem);
                extendedPattern.SE += (sequences[seq.first].size - (extendedPattern.siduls[seq.first][0].position+1) + 1);
                extendedPattern.SLIP += extendedPattern.siduls[seq.first].size();
            }
        }
    }
    return extendedPattern;
}

float computeRBU(Pattern pattern) {
    float patternRBU = 0;
    for (auto sidul : pattern.siduls) patternRBU += (sidul.second[0].utility + sidul.second[0].rem);
    return patternRBU;
}

float computeUmin(Pattern pattern) {
    float patternUmin = 0;
    for (auto sidul : pattern.siduls) {
        float umin = std::numeric_limits<float>::max();
        for (auto instance : sidul.second) if (instance.utility < umin) umin = instance.utility;
        patternUmin += umin;
    }
    return patternUmin;
}

unsigned int computeSE(Pattern pattern, std::unordered_map<unsigned int, Sequence> sequences) {
    unsigned int patternSE = 0;
    /*
        seq.second[0].position+1 is because position starts with 0, not 1
    */
    for (auto seq : pattern.siduls) patternSE += (sequences[seq.first].size - (seq.second[0].position+1) + 1);
    return patternSE;
}

unsigned int computeSLIP(Pattern pattern) {
    unsigned int patternSLIP = 0;
    for (auto seq : pattern.siduls) patternSLIP += seq.second.size();
    return patternSLIP;
}

bool isContainedBy(std::string superPatternName, std::string subPatternName) {
    std::vector<std::string> superPattern, subPattern;
    boost::split(superPattern, superPatternName, boost::is_any_of(" "));
    boost::split(subPattern, subPatternName, boost::is_any_of(" "));
    unsigned int pos = 0;
    for (auto c : superPattern) if (subPattern.size() > pos && subPattern[pos] == c) ++pos;
    return pos == subPattern.size();
}

void LocalPruningCHU(Pattern& superPattern, Pattern& subPattern) {
    if (superPattern.SE == subPattern.SE) {
        subPattern.do_s_ext = false;
        if (superPattern.SLIP == subPattern.SLIP)
            subPattern.do_ext = false;
    }
}

bool UpdateFMaxCloHUS(
    Pattern pattern,
    std::unordered_map<unsigned int,
    Sequence> sequences,
    const float MIN_UTILITY,
    std::list<Pattern> &FCHUS
) {
    if (pattern.RBU < MIN_UTILITY) return true;
    if (!pattern.do_ext) return true;
    if (!pattern.do_s_ext) return false;
    if (pattern.umin < MIN_UTILITY) return false;
    bool isMaximal = true;
    for (auto FCHPattern : FCHUS) {
        if (isContainedBy(FCHPattern.name, pattern.name)) {
            isMaximal = false;
            if (FCHPattern.siduls.size() == pattern.siduls.size()) {
                LocalPruningCHU(FCHPattern, pattern);
                if (!pattern.do_ext) return true;
                return false;
            }
        }
    }
    std::list<Pattern> updatedFCHUS;
    for (auto FCHPattern : FCHUS)
        if (isContainedBy(pattern.name, FCHPattern.name)) {
            if (FCHPattern.isMaximal) FCHPattern.isMaximal = false;
            if (FCHPattern.siduls.size() != pattern.siduls.size()) updatedFCHUS.push_back(FCHPattern);
        }
    FCHUS = updatedFCHUS;
    if (isMaximal) pattern.isMaximal = true;
    FCHUS.push_back(pattern);
    return false;
}

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
) {
    if (UpdateFMaxCloHUS(pattern, sequences, MIN_UTILITY, FCHUS)) return;
    std::unordered_map<unsigned int, Pattern> newI, newS, newIList, newSList;
    for (auto item : I) {
        if (item.first > pattern.lastItem) {
            /*
                Compuate LRU and Support of the extension
            */
            float extensionLRU = 0;
            unsigned int extensionSupp = 0;
            std::unordered_set<int> commonSeqs;
            for (auto seq : item.second.siduls) commonSeqs.insert(seq.first);
            for (auto seq : pattern.siduls) {
                if (commonSeqs.find(seq.first) != commonSeqs.end()) {
                    std::unordered_set<int> commonPoss;
                    for (auto instance : item.second.siduls[seq.first]) commonPoss.insert(instance.position);
                    for (auto instance : pattern.siduls[seq.first]) {
                        if (commonPoss.find(instance.position) != commonPoss.end()) {
                            extensionLRU += (pattern.siduls[seq.first][0].utility + pattern.siduls[seq.first][0].rem);
                            ++extensionSupp;
                            break;
                        }
                    }
                }
            }
            /*
                Prune candidates by LRU and MIN_SUPP
            */
            if (extensionLRU >= MIN_UTILITY && extensionSupp >= MIN_SUPP) newI[item.first] = item.second;
        }
    }
    for (auto item : newI) {
        /*
            Construct the extended pattern
        */
        Pattern extendedPattern = construct_i_ext(pattern, item.second, sequences);

        if (extendedPattern.RBU >= MIN_UTILITY) {
            newIList[item.first] = extendedPattern;
            LocalPruningCHU(extendedPattern, pattern);
            if (pattern.isSExt) LocalPruningCHU(extendedPattern, SList[item.first]);
            else LocalPruningCHU(extendedPattern, IList[item.first]);
            if (
                pattern.isSExt &&
                pattern.isParentSExt &&
                (pattern.lastItem == pattern.parentLastItem) &&
                (IList.find(item.first) != IList.end()) &&
                IList[item.first].lastItem == item.first
            ) LocalPruningCHU(extendedPattern, IList[item.first]);
        }
    }
    if (pattern.do_ext or pattern.do_s_ext) {
        /*
            Compuate LRU and Support of the extension
        */
        for (auto item : S) {
            float extensionLRU = 0;
            unsigned int extensionSupp = 0;
            std::unordered_set<int> commonSeqs;
            for (auto seq : item.second.siduls) commonSeqs.insert(seq.first);
            for (auto seq : pattern.siduls) {
                if (commonSeqs.find(seq.first) != commonSeqs.end()) {
                    if (item.second.siduls[seq.first].back().position > pattern.siduls[seq.first].front().position) {
                        extensionLRU += (pattern.siduls[seq.first][0].utility + pattern.siduls[seq.first][0].rem);
                        extensionSupp += 1;
                    }
                }
            }
            /*
                Prune candidates by LRU and MIN_SUPP
            */
            if (extensionLRU >= MIN_UTILITY && extensionSupp >= MIN_SUPP) newS[item.first] = item.second;
        }
        for (auto item : newS) {
            /*
                Construct new SIDULs for the extended pattern
            */
            Pattern extendedPattern = construct_s_ext(pattern, item.second, sequences);
            
            if (extendedPattern.RBU >= MIN_UTILITY) {
                /*
                    d->a
                    check a because d->a contains a
                    
                    bcd->f->a
                    check bcd->a because bcd->f->a contains bcd->a
                */
                newSList[item.first] = extendedPattern;
                LocalPruningCHU(extendedPattern, SList[item.first]);
            }
        }
        for (auto pattern : newSList) {
            DFS_FMaxCloHUS(
                pattern.second,
                newS,
                newS,
                newIList,
                newSList,
                sequences,
                MIN_SUPP,
                MIN_UTILITY,
                FCHUS 
            );
        }
    } else newS = S;
    for (auto pattern : newIList) {
        DFS_FMaxCloHUS(
            pattern.second,
            newI,
            newS,
            newIList,
            newSList,
            sequences,
            MIN_SUPP,
            MIN_UTILITY,
            FCHUS
        );
    }
}
