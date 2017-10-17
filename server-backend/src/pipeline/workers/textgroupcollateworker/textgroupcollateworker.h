#ifndef TEXTGROUPCOLLATEWORKER_H
#define TEXTGROUPCOLLATEWORKER_H

#include "asyncworker.h"

#include  <opencv2/text.hpp>

#include <thread>
#include <atomic>

#ifdef _WORKER_NAME_
#undef _WORKER_NAME_
#endif
#ifdef _WORKER_CLASS_
#undef _WORKER_CLASS_
#endif

#define _WORKER_NAME_ "Text Group Collate Worker" 
#define _WORKER_CLASS_ TextGroupCollateWorker

namespace Postr 
{
    /**
     * @brief This Worker tries to merge text blocks that overlap
     */
    class _WORKER_CLASS_ final : public AsyncWorker
    {
    public:
        _WORKER_CLASS_();
        ~_WORKER_CLASS_();
    
    private:        
        void processAsync(Data& data) override;
    };
};

#include "workerexport.h"

#endif //TEXTGROUPCOLLATEWORKER_H 
