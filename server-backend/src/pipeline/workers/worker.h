#ifndef WORKER_H
#define WORKER_H

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

#include "postrdata.h"
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>

#if (defined _MSC_VER || defined WIN32 || defined _WIN32 || defined WINCE || defined __CYGWIN__) && defined CVAPI_EXPORTS
#  define WORKER_EXPORTS __declspec(dllexport)
#elif defined __GNUC__ && __GNUC__ >= 4
#  define WORKER_EXPORTS __attribute__ ((visibility ("default")))
#else
#  define WORKER_EXPORTS
#endif

namespace Postr 
{
    /**
     * @brief This is the base class for Workers.
     * Workers are processing units that can be used in a pipelined chain.
     */
    class Worker
    {
    public:
        typedef std::function<void(Data data, int status)> const WorkerCallback;
        typedef std::function<void(const char*, int)>* WorkerCallbackSerialized;
        typedef std::shared_ptr<std::atomic_int> ChainCounter;
        typedef ChainCounter ChainValue;
        
        struct WorkerChain
        {
        private:
            
            std::function<void(DataPtr data, ChainCounter pending, std::promise<void> p)> m_function;
            int m_count;
            std::set<const Worker*> m_workers;
            
        public:
            WorkerChain(std::function<void(DataPtr data, ChainCounter pending, std::promise<void> p)> function = {}, int count = 0, std::set<const Worker*> workers = {})
                : m_function(function)
                , m_count(count)
                , m_workers(workers)
            {
            }
            
            void operator()(DataPtr data, ChainCounter pending, std::promise<void> p) const
            {
                return m_function(data, pending, std::move(p));
            }
            
            int count() const
            {
                return m_count;
            }
            
            const std::function<void(DataPtr data, ChainCounter pending, std::promise<void> p)>& function() const
            {
                return m_function;
            }
            
            const std::set<const Worker*>& workers() const
            {
                return m_workers;
            }
            
            std::set<const Worker*>& workers()
            {
                return m_workers;
            }
        };
        
        explicit Worker(const char *name);
        ~Worker();
        
        /**
         * @brief Begin processing Data.
         * @param data data to process
         * @param callback function to call when the processing ends
         * @return status. 0 on successful initiation. Note that a Worker may process data asynchronously and this function may return as soon as the processing has begun. Use Worker::progress to check the progress or set a callback function.
         */
        virtual int process(Data data, WorkerCallback& callback) = 0;
        
        /**
         * @brief Process Data.
         * This function is blocking. Use Worker::process to process Data asynchronously.
         * @param data data to process
         * @return status. 0 on success.
         */
        int processBlocking(Data& data);
        
        /**
         * @brief Progress of this Worker.
         * @return 0 if not busy. Progress in range 1 to 100 else.
         */
        virtual float progress() const = 0;
        
        /**
         * @brief Print the progress of this worker to stdout
         * @param cr add a carriage return
         */
        void printProgress(bool cr = true) const;
        
        /**
         * @brief Status of this Worker.
         * @return 0 or error code of last failure.
         */
        int status() const;
        
        /**
         * @brief Return the name of this Worker.
         * @return The name of this Worker
         */
        std::string name() const;
        
        static void initializeLog(int loglevel);
    
        /**
         * If interactive is true, Workers might print their progress to stdout and request input from the user
         */
        static std::atomic_bool interactive;
        
        /**
         * If debug is true, Workers might show intermediate results. 
         * Set this to false when using mutliple concurrent pipelines.
         */
        static std::atomic_bool debug;
      
        /**
         * @brief Implicit cast to a chain
         */
        operator WorkerChain ();
        
        /**
        * @brief Read a config entry.
        * @param name name of the entry
        * @param defval default value for this entry
        * @param descr description that is added to the config file
        * @return the value of the entry with the given name or the default value if it does not exist
        */
        std::string config(const std::string& name, const std::string& defval, const std::string& descr="") const;
        
        /**
        * @brief Read a config entry.
        * @param name name of the entry
        * @return the value of the entry with the given name or an empty string if it does not exist
        */
        std::string config(const std::string& name) const;
        
        /**
        * @brief Read a config entry.
        * @param name name of the entry
        * @param defval value for this entry
        * @param descr description that is added to the config file
        * @return the value of the entry with the given name or the default value if it does not exist
        */
        int config(const std::string& name, const int defval, const std::string& descr = "") const;
        
        /**
        * @brief Read a config entry.
        * @param name name of the entry
        * @param defval default value for this entry
        * @param descr description that is added to the config file
        * @return the value of the entry with the given name or the default value if it does not exist
        */
        double config(const std::string& name, const double defval, const std::string& descr = "") const;
        
        /**
        * @brief Read a config entry.
        * @param name name of the entry
        * @param defval default value for this entry
        * @param descr description that is added to the config file
        * @return the value of the entry with the given name or the default value if it does not exist
        */
        bool configBool(const std::string& name, const bool defval, const std::string& descr = "") const;
        
        /**
         * @brief Block until a chain has finished
         * @param c the ChainValue of the chain
         * @param workers a vector of pointers to workers to print a progress bar for
         * @return true if the chain finished normally. false if the chain was aborted (e.g. by a SIGINT or due to unexpected behaviour)
         */
        static bool waitForFinished(ChainValue c, std::set<const Worker*> workers = {});
        
        /**
         * @brief Check if a chain which includes this Worker has been aborted
         * @return if the chain has been aborted
         */
        static bool aborted();
        
        /**
        * @brief Join two Worker chains
        * @param one one worker chain
        * @param other another worker chain
        * @param fn a function that joins the results of both workers
        */
        static WorkerChain join(WorkerChain one, WorkerChain other, std::function<void(Data&,const Data&)> fn);
        
    protected:
        int m_status;
        const char *m_name;
        static el::base::type::StoragePointer m_storage;
        static std::condition_variable m_progresscondition;
        static std::mutex m_progressmutex;
        static std::atomic_bool m_abort;
        
        void initializeConfig();
        void releaseConfig();
        
    private:
        class LogDispatcher : public el::LogDispatchCallback
        {
            void handle(const el::LogDispatchData* handlePtr) override;
        };
        
        static Data m_config;
        static std::atomic_int m_configaccesscounter;
        static std::mutex m_configaccessmutex;
        
        static std::atomic_int m_progressbars;
        
        static void handleCrash(int sig);
        
        friend ChainValue operator<<(WorkerChain chain, DataPtr data);
        friend WorkerChain operator<<(WorkerChain left, Worker& right);
        friend WorkerChain operator<<(WorkerChain left, WorkerChain right);
    };

    /**
     * @brief Process Data in a chain
     * @param chain Worker chain to process data in
     * @param data data to process
     * @return A pointer to the number of Workers that are pending and a vector of pointers to involved workers and a promise that is set once the chain finished
     */
    Worker::ChainValue operator<<(Worker::WorkerChain chain, DataPtr data);
    
    /**
     * @brief Add a Worker to a chain
     */
    Worker::WorkerChain operator<<(Worker::WorkerChain left, Worker& right);
    
    /**
     * @brief Combine two chains
     */
    Worker::WorkerChain operator<<(Worker::WorkerChain left, Worker::WorkerChain right);
    

}
#endif
