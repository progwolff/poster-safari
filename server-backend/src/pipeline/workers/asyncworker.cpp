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

#include "asyncworker.h"

#undef LOG
#define LOG(LEVEL) (CLOG(LEVEL, ELPP_CURR_FILE_LOGGER_ID) << "[" << m_name << "] ")

namespace Postr 
{
    AsyncWorker::AsyncWorker(const char *name)
        : Worker(name)
        , m_cancel(false)
        , m_initialized(false)
        , m_progress(0)
    {
        m_initialized = false;
        LOG(DEBUG) << "calling _init()";
        _init();
    }
    
    AsyncWorker::~AsyncWorker()
    {
        releaseConfig();
        m_cancel = true; //signal any working thread to cancel
        auto start = std::chrono::steady_clock::now();
        while(m_progress)
        {
            //wait until any working thread finished cleanup
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start);
            if(duration.count() > 10)
            {
                LOG(ERROR) << "killed detached thread of " << m_name << " after waiting for " << duration.count() << " seconds for it to finish.";
                break;
            }
            else
                std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    int AsyncWorker::process(Data data, WorkerCallback& callback)
    {
        std::unique_lock<std::mutex> lk(m_progressmutex);
        //wait for previously started tasks to finish
        m_startprocesscondition.wait(lk, [this]{
            return (0 == m_progress);
        });
        
        m_status = 0;
        
        if(m_abort)
            return 1;
        
        m_cancel = false;
    
        m_progress = 100; //means 1%
        
        lk.unlock();
        
        m_progresscondition.notify_all();
        
        m_thread = std::thread(&AsyncWorker::_process, this, data, callback);
        
        m_thread.detach();
        
        return 0;
    }
    
    void AsyncWorker::_process(Data data, WorkerCallback callback)
    {
        while(!m_initialized && !m_abort && !m_cancel);
        
        LOG(DEBUG) << m_name << " start processing";
        
        if(!m_abort && !m_cancel)
            processAsync(data);
        
        LOG(DEBUG) << m_name << " done";
        
        m_progress = 0;
        m_startprocesscondition.notify_all();
        
        if(callback && !m_abort)
            callback(data, status());
        else
            LOG(DEBUG) << m_name << " has no callback";
        
        std::lock_guard<std::mutex> lk(m_progressmutex);
        m_progresscondition.notify_all();
    }
    
    void AsyncWorker::_init()
    {
        initializeConfig();
        LOG(DEBUG) << "starting init thread";
        m_initthread = std::thread([this]{
            initAsync();
            m_initialized = true;
        });
        
        m_initthread.detach();
    }
    
    void AsyncWorker::progress(float progress)
    {
        std::lock_guard<std::mutex> lk(m_progressmutex);
        m_progress = 100*progress;
        //notify others that progress has changed
        m_progresscondition.notify_all();
    }
    
    float AsyncWorker::progress() const
    {
        return m_progress/100.;
    }
    
}

