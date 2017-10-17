#ifndef VOTINGTEXTMERGEWORKER_H
#define VOTINGTEXTMERGEWORKER_H

#include "asyncworker.h"

#ifdef _WORKER_NAME_
#undef _WORKER_NAME_
#endif
#ifdef _WORKER_CLASS_
#undef _WORKER_CLASS_
#endif

#define _WORKER_NAME_ "Voting Text Merger" 
#define _WORKER_CLASS_ VotingTextMergeWorker

namespace Postr 
{
    /**
     * @brief This Worker tries to merge text blocks with errors to a corrected text
     */
    class _WORKER_CLASS_ final : public AsyncWorker
    {
    public:
        _WORKER_CLASS_();
        ~_WORKER_CLASS_();
    
    private:        
        void processAsync(Data& data) override;
        /**
         * @brief Get the edits needed to transform s1 to s2
         * @param s1 one string
         * @param s2 another string
         * @return a vector of edits needed to transform s1 to s2. each entry corresponds to a character in s1.
         */
        std::vector<std::string> edits(const std::string& s1, const std::string& s2);
        /**
         * @brief Write a cost map to a file.
         * The cost map represents the probability of edits faced by this worker
         * @param fname name of the file to write to
         */
        void writeCostFile(const std::string& fname);
        
        std::map<std::pair<std::string,std::string>,double> m_costMap;
        double m_max; 
    };
};

#include "workerexport.h"

#endif //VOTINGTEXTMERGEWORKER_H
