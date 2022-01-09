/*
    FCHUPatterns are partitioned by both the support and the max size
    of sequences that share the same support.

    When there's a new candidate pattern that its size is bigger than
    the max size of the list whose support is equal to the candidate,
    skip checking since its obviously a new close pattern (it does not
    have superpatterns with the same support). Otherwise, we need to
    scan the list to check for both conditions.
*/
#include <iostream>
#include <iomanip>
#include <limits>
#include <algorithm>
#include "omp.h"
#include "utils.h"

class FCloSublist {
    public:
        unsigned int pattern_max_size;
        std::list<Pattern> cloPatterns;
};

void dfs(
    Pattern pattern,
    std::unordered_map<unsigned int, Pattern> I,
    std::unordered_map<unsigned int, Pattern> S,
    float MIN_SUPP,
    float MIN_UTILITY,
    std::unordered_map<unsigned int, Sequence> sequences,
    std::unordered_map<unsigned int, FCloSublist> &FCHUPatterns
) {
    // std::cout <<
    // "DFS called for " << pattern.name <<
    // ", thread=" << omp_get_thread_num() << std::endl;
    bool do_s_ext = true;
    if (pattern.umin >= MIN_UTILITY) {
        bool isClosed = true;
        bool isPruned = false;
        #pragma omp critical
        {
            /*
                Since the pattern_max_size >= the size of the current candidate, there exists both
                closed patterns whose size is either (1) >= or (2) < the size of the candidate.

                Do this until all closed patterns are considered or we confirm that the new
                pattern is not closed.
            */
            if (!(FCHUPatterns[pattern.siduls.size()].pattern_max_size < pattern.size)) {
                // std::cout <<
                // "There are sequences whose size is >= that of the candidate " <<
                // pattern.name << std::endl;
                auto it = FCHUPatterns[pattern.siduls.size()].cloPatterns.begin();
                while (isClosed && (it != FCHUPatterns[pattern.siduls.size()].cloPatterns.end())) {
                    /*
                        (1)
                    */
                    if (
                        ((*it).size >= pattern.size) &&
                        ((*it).name.length() >= pattern.name.length())
                    ) {
                        // std::cout <<
                        // omp_get_thread_num() << " Thread, " <<
                        // "Checking (candidate size is >=)" << 
                        // (*it).name << " with " << pattern.name << std::endl;
                        if (isContainedBy((*it).name, pattern.name)) {
                            // std::cout <<
                            // "Candidate " << pattern.name << " is not closed" << std::endl;
                            isClosed = false;
                            if ((*it).SE == pattern.SE) {
                                do_s_ext = false;
                                if ((*it).SLIP == pattern.SLIP) isPruned = true;
                            }
                        } 
                        /*
                            When the size is bigger but the candidate is not a subpattern,
                            continue consider other closed patterns
                        */
                        else {
                            ++it;
                            // std::cout <<
                            // "Candidate " << pattern.name << " is not in " << (*it).name << std::endl;
                        }
                    } 
                    /*
                        (2)
                    */
                    else {
                        // std::cout << 
                        // omp_get_thread_num() << " Thread, " <<
                        // "Checking (candidate size is <)" << (*it).name << " with " << pattern.name << std::endl;
                        if (isContainedBy(pattern.name, (*it).name)) {
                            // std::cout <<
                            // "Deleting " << (*it).name << std::endl;
                            it = FCHUPatterns[pattern.siduls.size()].cloPatterns.erase(it);
                        } else {
                            ++it;
                            // std::cout <<
                            // "Candidate " << pattern.name << " is not in " << (*it).name << std::endl;
                        }
                    }
                }
                if (isClosed) {
                    // std::cout <<
                    // omp_get_thread_num() << " Thread, " <<
                    // "Adding " << pattern.name << std::endl;
                    FCHUPatterns[pattern.siduls.size()].cloPatterns.push_back(pattern);
                    // std::cout <<
                    // "Current list, supp=" << pattern.siduls.size() << ": " << std::endl;
                    // for (auto it : FCHUPatterns[pattern.siduls.size()].cloPatterns)
                    //     std::cout << it.name << std::endl;
                    FCHUPatterns[pattern.siduls.size()].pattern_max_size = std::max(
                        FCHUPatterns[pattern.siduls.size()].pattern_max_size,
                        pattern.size
                    );
                }
            }
            /*
                The size of all closed patterns in this bucket are less than that of the new candidate
                -> The candidate becomes a closed pattern

                We go back and eliminate existing closed patterns if any
            */
            else {
                // std::cout <<
                // "No sequence whose size is >= that of the candidate " <<
                // pattern.name << std::endl;
                auto it = FCHUPatterns[pattern.siduls.size()].cloPatterns.begin();
                while (it != FCHUPatterns[pattern.siduls.size()].cloPatterns.end()) {
                    // std::cout << 
                    // omp_get_thread_num() << " Thread, " <<
                    // "Checking (candidate size is >=)" 
                    // << (*it).name << " with " << pattern.name << std::endl;
                    if (isContainedBy(pattern.name, (*it).name)) 
                        it = FCHUPatterns[pattern.siduls.size()].cloPatterns.erase(it);
                }
                // std::cout <<
                // omp_get_thread_num() << " Thread, " <<
                // "Adding " << pattern.name << std::endl;
                FCHUPatterns[pattern.siduls.size()].cloPatterns.push_back(pattern);
                // std::cout <<
                // "Current list, supp=" << pattern.siduls.size() << ": " << std::endl;
                // for (auto it : FCHUPatterns[pattern.siduls.size()].cloPatterns)
                //     std::cout << it.name << std::endl;
                FCHUPatterns[pattern.siduls.size()].pattern_max_size = pattern.size;
            }
        }
        if (isPruned) return;
    }
    if (pattern.RBU < MIN_UTILITY) return;
    std::unordered_map<unsigned int, Pattern> newI, newS, newIList;
    for (auto item : I) {
        if (item.first > pattern.lastItem) {
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
            // if (extensionLRU >= MIN_UTILITY && extensionSupp >= MIN_SUPP) newI[item.first] = item.second;
            if (extensionLRU >= MIN_UTILITY && extensionSupp >= MIN_SUPP) {
                newI[item.first] = item.second;
                Pattern extendedPattern = construct_i_ext(pattern, item.second, sequences);
                newIList[item.first] = extendedPattern;
                if (extendedPattern.SE == pattern.SE) do_s_ext = false;                
            }
        }
    }
    // for (auto item : newI) {
    //     Pattern extendedPattern = construct_i_ext(pattern, item.second, sequences);
    //     newIList[item.first] = extendedPattern;
    //     if (extendedPattern.SE == pattern.SE) do_s_ext = false;
    // }
    if (do_s_ext) {
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
            if (extensionLRU >= MIN_UTILITY && extensionSupp >= MIN_SUPP) newS[item.first] = item.second;
        }
        for (auto item : newS) {
            Pattern extendedPattern = construct_s_ext(pattern, item.second, sequences);
            #pragma omp task untied shared(newS, sequences, FCHUPatterns) firstprivate(extendedPattern)
            {
                dfs(extendedPattern, newS, newS, MIN_SUPP, MIN_UTILITY, sequences, FCHUPatterns);
            }
        // #pragma omp taskwait
        }
    } else newS = S;
    for (auto extendedPattern : newIList) 
        #pragma omp task untied shared(newI, newS, sequences, FCHUPatterns) firstprivate(extendedPattern)
        {
            dfs(extendedPattern.second, newI, newS, MIN_SUPP, MIN_UTILITY, sequences, FCHUPatterns);
        }
    #pragma omp taskwait
}

int main(int argvc, char** argv) {
    const float MIN_SUPP = std::stof(argv[1]);
    const float MIN_UTILITY = std::stof(argv[2]);
    const std::string INPUT_DATA_PATH = argv[3];

    std::unordered_map<unsigned int, Sequence> sequences = readInputData(INPUT_DATA_PATH);
    /*
        Scan the database to compute all SIDULs for all items
    */
    std::unordered_map<unsigned int, Pattern> sidulItems = construct_siduls(sequences);
    /*
        Remove items whose supp < MIN_SUPP || LRU < MIN_UTILITY
    */
    std::unordered_map<unsigned int, Sequence> updatedSequences = WPS_by_LRU_and_Support(sequences, sidulItems, MIN_SUPP, MIN_UTILITY);
    /*
        Re-construct the siduls with the recently updated sequences
    */
    sidulItems = construct_siduls(updatedSequences);

    std::unordered_map<unsigned int, FCloSublist> FCHUPatterns;

    std::cout << "# items: " << sidulItems.size() << ", # sequences: " << updatedSequences.size() << std::endl;

    #pragma omp parallel default(none) shared(sidulItems, FCHUPatterns, updatedSequences)
    {
        #pragma omp single
        {
            for (auto pattern : sidulItems)
                #pragma omp task untied default(none) shared(FCHUPatterns, sidulItems, updatedSequences) firstprivate(pattern)
                {
                    dfs(pattern.second, sidulItems, sidulItems, MIN_SUPP, MIN_UTILITY, updatedSequences, FCHUPatterns);
                }
            #pragma omp taskwait
        }
    }

    int numOfPattern = 0;
    for (auto it : FCHUPatterns) {
        numOfPattern += it.second.cloPatterns.size();
        // for (auto ii : it.second.cloPatterns)
        //     std::cout << ii.name << 
        //     ", supp=" << ii.siduls.size() << 
        //     ", size=" << ii.size <<
        //     std::endl;
    }
    std::cout << "Total: " << numOfPattern << std::endl;
    
    return 0;
}
