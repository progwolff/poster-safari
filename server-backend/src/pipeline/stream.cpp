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

#include "stream.h"
#include <thread>

namespace Postr 
{
    Stream::Stream()
        : m_tasks(0)
    {
    }
    
    Stream::~Stream()
    {
        while(m_tasks);
    }
    
    void Stream::process(Worker::WorkerChain chain, const Data& data)
    {
        std::shared_ptr<Data> d(new Data);
        *d = data;
        
        std::thread t([this,chain](std::shared_ptr<Data> data) {
            Worker::ChainValue pending = chain << data;
            std::set<const Worker*> workers;
            if(Worker::interactive)
                workers = chain.workers();
            
            if(Worker::waitForFinished(pending,workers))
                handleResult(*data);
            else
                handleError(*data);
            --m_tasks;
        }, d);
        t.detach();
    }
    
    
    Stream& Stream::operator>>(Data& data)
    {
        return get(data);
    }
    
    
    void operator<<(Worker::WorkerChain chain, Stream& stream)
    {
        while(stream.is_open() && stream.good())
        {
            int maxtasks = (*chain.workers().begin())->config("stream_maxTasks", 5, "maximum number of documents being processed in parallel");
            while(stream.m_tasks > maxtasks)
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            Data data;
            
            //extract data from stream
            stream >> data;
            
            ++stream.m_tasks;
            
            //process data in a Worker chain
            stream.process(chain, data);
        }
    }
    
}
