#include "utils.h"

/*
    Update the input data so that we only use one loop.
*/
std::unordered_map<unsigned int, std::shared_ptr<Sequence>> readInputData(std::string inputDataPath) {
    std::unordered_map<unsigned int, std::shared_ptr<Sequence>> sequences;

    std::fstream sequencesFile (inputDataPath + "/" + SEQUENCES_FILE);
    std::fstream utilitiesFile (inputDataPath + "/" + UTILITIES_FILE);

    unsigned int seqID = 0;
    std::string line;
    std::vector<std::string> tokens;

    while (std::getline(sequencesFile, line)) {
        std::shared_ptr<Sequence> sequence = std::make_shared<Sequence>();
        boost::split(tokens, line, boost::is_any_of("\t"));
        for (int i = 0; i < tokens.size(); i++) {
            int item = std::stoi(tokens[i]);
            if (item == END_ITEMSET || item == END_SEQUENCE) ++sequence->size;
            sequence->items.push_back(std::make_shared<Item>(item));
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
            sequences[seqID]->items[itemIdx]->utility = itemUtility;
            sequences[seqID]->utility += itemUtility;
            ++itemIdx;
        }

        ++seqID;
    }

    return sequences;
}

void print_sequences(std::unordered_map<int, Sequence> sequences) {
    for (auto seq : sequences) {
        std::cout << seq.first << "(" << seq.second.utility << "): ";
        for (auto item : seq.second.items) std::cout << item->id << "/" << item->utility << " "; 
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
        for (auto instance : seq.second) std::cout << instance->position << "/" << instance->utility << "/" << instance->rem << std::endl;
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
                std::cout << instance->position << "/" << instance->utility << "/" << instance->rem << "->";
            std::cout << std::endl;
        }
    }
}

std::unordered_map<unsigned int, std::shared_ptr<Pattern>> construct_siduls(
    std::unordered_map<unsigned int, std::shared_ptr<Sequence>> sequences
) {
    std::unordered_map<unsigned int, std::shared_ptr<Pattern>> sidulItems;
    for (auto seq : sequences) {
        /*

        */
        std::unordered_map<unsigned int, float> uminInSequence;

        unsigned int itemsetIdx = 0;
        float prefixUtility = 0;
        for (auto item : seq.second->items) {
            if (item->id != END_ITEMSET && item->id != END_SEQUENCE) {
                if (sidulItems.find(item->id) == sidulItems.end()) {
                    sidulItems[item->id] = std::make_shared<Pattern>();
                    sidulItems[item->id]->lastItem = item->id;
                    ++sidulItems[item->id]->size;
                    /*
                        If performance is all what we need, remove the name.
                    */
                    sidulItems[item->id]->name.append(std::to_string(item->id));
                }
                sidulItems[item->id]->siduls[seq.first].push_back(
                    std::make_shared<ItemInstance>(
                        item->utility,
                        seq.second->utility - (prefixUtility += item->utility),
                        itemsetIdx
                    )
                );
                /*
                    When an item first appears in a sequence, initialize its umin to max of float.
                    We seek for the smallest umin among all of this instances.
                */
                if (uminInSequence.find(item->id) == uminInSequence.end()) uminInSequence[item->id] = std::numeric_limits<float>::max();
                uminInSequence[item->id] = std::min(uminInSequence[item->id], item->utility);
            } else ++itemsetIdx;
        }
        for (auto item : uminInSequence) {
            sidulItems[item.first]->umin += item.second;
            sidulItems[item.first]->RBU += (sidulItems[item.first]->siduls[seq.first][0]->utility + sidulItems[item.first]->siduls[seq.first][0]->rem);
            sidulItems[item.first]->SE += (sequences[seq.first]->size - (sidulItems[item.first]->siduls[seq.first][0]->position+1) + 1);
            sidulItems[item.first]->SLIP += sidulItems[item.first]->siduls[seq.first].size();
        }
    }
    return sidulItems;
}

std::unordered_map<unsigned int, std::shared_ptr<Sequence>> WPS_by_LRU_and_Support(
    std::unordered_map<unsigned int, std::shared_ptr<Sequence>> sequences,
    std::unordered_map<unsigned int, std::shared_ptr<Pattern>> sidulItems,
    float MIN_SUPP,
    float MIN_UTILITY
) {
    std::unordered_map<int, float> LRUByItem;
    std::unordered_map<unsigned int, std::shared_ptr<Sequence>> updatedSequences;
    for (auto seq : sequences) {
        std::shared_ptr<Sequence> updatedSequence = std::make_shared<Sequence>();
        bool itemAdded = false;
        for (auto item : seq.second->items) {
            if (item->id != END_ITEMSET && item->id != END_SEQUENCE) {
                if (LRUByItem.find(item->id) == LRUByItem.end()) 
                    for (auto seq : sidulItems[item->id]->siduls) LRUByItem[item->id] += sequences[seq.first]->utility;
                if (sidulItems[item->id]->siduls.size() >= MIN_SUPP && LRUByItem[item->id] >= MIN_UTILITY) {
                    itemAdded = true;
                    updatedSequence->utility += item->utility;
                    updatedSequence->items.push_back(item);
                }
            } else {
                if (itemAdded) {
                    ++updatedSequence->size;
                    itemAdded = false;
                    updatedSequence->items.push_back(item);
                }
            }
        }
        if (updatedSequence->items.size()) updatedSequences[seq.first] = updatedSequence;
    }

    return updatedSequences;
}

std::shared_ptr<Pattern> construct_i_ext(
    std::shared_ptr<Pattern> pattern,
    std::shared_ptr<Pattern> item, 
    std::unordered_map<unsigned int, std::shared_ptr<Sequence>> sequences
) {
    std::shared_ptr<Pattern> extendedPattern = std::make_shared<Pattern>();
    extendedPattern->isSExt = false;
    extendedPattern->lastItem = item->lastItem;
    extendedPattern->parentLastItem = pattern->lastItem;
    extendedPattern->isParentSExt = pattern->isSExt;
    extendedPattern->name = pattern->name;
    extendedPattern->name.append(" ").append(item->name);
    extendedPattern->size = pattern->size;

    std::unordered_set<unsigned int> commonSeqs;
    for (auto seq : pattern->siduls) commonSeqs.insert(seq.first);
    for (auto seq : item->siduls) {
        float uminInSequence = std::numeric_limits<float>::max();
        if (commonSeqs.find(seq.first) != commonSeqs.end()) {
            std::unordered_map<unsigned int, unsigned int> commonPoss;
            for (int idx = 0; idx < item->siduls[seq.first].size(); ++idx) 
                commonPoss.insert({item->siduls[seq.first][idx]->position, idx});
            for (auto patternInstance : pattern->siduls[seq.first])
                if (commonPoss.find(patternInstance->position) != commonPoss.end()) {
                    const float instanceUmin = patternInstance->utility + item->siduls[seq.first][commonPoss[patternInstance->position]]->utility;
                    extendedPattern->siduls[seq.first].push_back(
                        std::make_shared<ItemInstance>(
                            instanceUmin,
                            item->siduls[seq.first][commonPoss[patternInstance->position]]->rem,
                            item->siduls[seq.first][commonPoss[patternInstance->position]]->position
                        )
                    );
                    uminInSequence = std::min(uminInSequence, instanceUmin);
                }
            /*
                When they both appear in the same sequence, but they do not share any common position(s).
            */    
            if (uminInSequence != std::numeric_limits<float>::max()) {
                extendedPattern->umin += uminInSequence;
                extendedPattern->RBU += (extendedPattern->siduls[seq.first][0]->utility + extendedPattern->siduls[seq.first][0]->rem);
                extendedPattern->SE += (sequences[seq.first]->size - (extendedPattern->siduls[seq.first][0]->position+1) + 1);
                extendedPattern->SLIP += extendedPattern->siduls[seq.first].size();
            }
        }
    }
    return extendedPattern;
}

std::shared_ptr<Pattern> construct_s_ext(
    std::shared_ptr<Pattern> pattern,
    std::shared_ptr<Pattern> item, 
    std::unordered_map<unsigned int, std::shared_ptr<Sequence>> sequences
) {
    std::shared_ptr<Pattern> extendedPattern = std::make_shared<Pattern>();
    extendedPattern->lastItem = item->lastItem;
    extendedPattern->isParentSExt = pattern->isSExt;
    extendedPattern->parentLastItem = pattern->lastItem;
    extendedPattern->name = pattern->name;
    extendedPattern->name.append(" -1 ").append(item->name);
    extendedPattern->size = pattern->size + 1;

    std::unordered_set<unsigned int> commonSeqs;
    for (auto seq : pattern->siduls) commonSeqs.insert(seq.first);
    for (auto seq : item->siduls) {
        float uminInSequence = std::numeric_limits<float>::max();
        if (commonSeqs.find(seq.first) != commonSeqs.end()) {
            for (auto itemInstance : item->siduls[seq.first]) {
                float newUmin = std::numeric_limits<float>::max();
                unsigned int patternInstanceIdx = 0;
                auto patternInstance = pattern->siduls[seq.first][patternInstanceIdx];
                while (patternInstance->position < itemInstance->position) {
                        float currentUmin = patternInstance->utility + itemInstance->utility;
                        newUmin = std::min(newUmin, currentUmin);
                        /*
                            Need to carefully handle the index, refs:
                            https://stackoverflow.com/questions/27754726/no-compilation-error-or-run-time-error-when-index-out-of-range-of-vector-class
                        */    
                        if (++patternInstanceIdx >= pattern->siduls[seq.first].size()) break;          
                        patternInstance = pattern->siduls[seq.first][patternInstanceIdx];
                }

                if (newUmin != std::numeric_limits<float>::max()) {
                    extendedPattern->siduls[seq.first].push_back(
                        std::make_shared<ItemInstance>(
                            newUmin,
                            itemInstance->rem,
                            itemInstance->position
                        )
                    );
                    uminInSequence = std::min(uminInSequence, newUmin);
                }
            }
            /*
                When they both appear in the same sequence, but they do not share any common position(s).
            */
            if (uminInSequence != std::numeric_limits<float>::max()) {
                extendedPattern->umin += uminInSequence;
                extendedPattern->RBU += (extendedPattern->siduls[seq.first][0]->utility + extendedPattern->siduls[seq.first][0]->rem);
                extendedPattern->SE += (sequences[seq.first]->size - (extendedPattern->siduls[seq.first][0]->position+1) + 1);
                extendedPattern->SLIP += extendedPattern->siduls[seq.first].size();
            }
        }
    }
    return extendedPattern;
}

float computeRBU(Pattern pattern) {
    float patternRBU = 0;
    for (auto sidul : pattern.siduls) patternRBU += (sidul.second[0]->utility + sidul.second[0]->rem);
    return patternRBU;
}

float computeUmin(Pattern pattern) {
    float patternUmin = 0;
    for (auto sidul : pattern.siduls) {
        float umin = std::numeric_limits<float>::max();
        for (auto instance : sidul.second) if (instance->utility < umin) umin = instance->utility;
        patternUmin += umin;
    }
    return patternUmin;
}

unsigned int computeSE(Pattern pattern, std::unordered_map<unsigned int, Sequence> sequences) {
    unsigned int patternSE = 0;
    /*
        seq.second[0].position+1 is because position starts with 0, not 1
    */
    for (auto seq : pattern.siduls) patternSE += (sequences[seq.first].size - (seq.second[0]->position+1) + 1);
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
