/**
 * This file is part of Poster Safari.
 *
 *  Poster Safari is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Poster Safari is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Poster Safari.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * 
 *  Copyright 2017 Moritz Dannehl, Rebecca Lühmann, Nicolas Marin, Michael Nieß, Julian Wolff
 * 
 */

#ifndef ASYNCWORKER_H
#define ASYNCWORKER_H

#include "worker.h"

#include <thread>
#include <atomic>

namespace Postr 
{
    /**
     * @brief Base class for Workers which process data asynchronously
     */ 
    class AsyncWorker : public Worker
    {
    public:
        explicit AsyncWorker(const char *name);
        ~AsyncWorker();
        
        int process(Data data, WorkerCallback& callback) final override;
        float progress() const final override;
        
    protected:        
        /**
         * This is the function to override in your worker.
         * It is called by process. Use m_cancel to check if you should skip your computations
         * @param data in/out input data to process and output result
         */
        virtual void processAsync(Data& data) = 0;
        
        /**
         * This is function is called right after the constructor.
         * Override this to do any heavy operations that are needed before processing data.
         * processAsync will wait for this function to finish.
         */
        virtual void initAsync() {};
        
        /**
         * Set the progress of the current task
         * @param progress The new progress value
         */
        void progress(float progress);
        
        std::atomic_bool m_cancel;
        
    private:
        std::atomic_bool m_initialized;
        std::atomic_int m_progress;
        std::thread m_thread,m_initthread;
        void _process(Data data, WorkerCallback callback);
        void _init();
        std::condition_variable m_startprocesscondition;
    };
};

#endif //ASYNCWORKER_H 
