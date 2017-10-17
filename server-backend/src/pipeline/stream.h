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

#ifndef POSTR_STREAM_H
#define POSTR_STREAM_H

#include "worker.h"

namespace Postr 
{
    /**
     * @brief Base class for Streams that can be used to supply Worker chains with data
     */
    class Stream
    {
    public:
        explicit Stream();
        ~Stream();
        
        /**
         * @brief Extract Data from a stream.
         * This function blocks until a data object is available.
         * @param data Data read from the stream
         * @return a reference to the stream (*this)
         */
        virtual Stream& get(Data& data) = 0;

        /**
         *@brief  Get the number of Data objects available in the stream
         * @return the number of Data objects available in the stream
         */
        virtual int count() const = 0;
        
        /**
         * @brief Get the state of the stream
         * @return true if there are no errors, false otherwise.
         */
        virtual bool good() const = 0;
        
        /**
         * @brief Returns if the stream is currently associated to a data source
         * @return true if the stream is associated to a data source, false otherwise.
         */
        virtual bool is_open() const = 0;
        
        /**
         * @brief Extract Data from a stream
         * This function blocks until a data object is available.
         * @param data Data read from the stream
         * @return a reference to the stream (*this)
         */
        Stream& operator>>(Data& data);
        
        /**
         * @brief close this stream
         * i.e. stop fetching Data
         */
        virtual void close() = 0;
        
        /**
         * @brief Handle a Data object that has been processed in a Worker chain.
         * @param data a Data object that has been processed in a chain.
         */
        virtual void handleResult(const Data& data) = 0;
    
    protected:
        
        /**
         * @brief Handle an error that occured during processing a Data object in a Worker chain.
         * @param data a Data object that has been processed in a chain while an error occured.
         */
        virtual void handleError(const Data& data) = 0;
        
        friend void operator<<(Worker::WorkerChain chain, Stream& stream);
        
    private:
        /**
         * @brief Process data in a chain.
         * Calls handleResult once the chain finished.
         * @param chain a reference to the chain that will process the Data object.
         * @param data a pointer to the Data object that will be processed in the chain.
         */
        void process(Worker::WorkerChain chain, const Data& data);
        
        std::atomic_int m_tasks;
    };
    
    /**
    * @brief Process a stream of Data in a chain of Workers
    */
    void operator<<(Worker::WorkerChain chain, Stream& stream);
    
}

#endif //POSTR_STREAM_H
