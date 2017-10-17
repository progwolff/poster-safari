#ifndef WORDSPLITWORKER_H
#define WORDSPLITWORKER_H

#include "asyncworker.h"


#include <thread>
#include <atomic>

#ifdef _WORKER_NAME_
#undef _WORKER_NAME_
#endif
#ifdef _WORKER_CLASS_
#undef _WORKER_CLASS_
#endif

#define _WORKER_NAME_ "Word Splitting" 
#define _WORKER_CLASS_ WordSplitWorker

namespace Postr 
{
    /**
     * @brief This Worker splits a text into all possible combination of words
     */
    class _WORKER_CLASS_ final : public AsyncWorker
    {
    public:
        _WORKER_CLASS_();
        ~_WORKER_CLASS_();
    
    private:        
        void processAsync(Data& data) override;
        /**
         * @brief Insert a new text
         * @param target target data object 
         * @param basetext text object the new text is derived from
         * @param text the new text
         * @param baseid the id of the base text
         */
        void newText(Data& target, const MetaData& basetext, const std::string& text, const std::string& baseid);
    
        
    };
};

#include "workerexport.h"

#endif //OCRWORKER_H 
