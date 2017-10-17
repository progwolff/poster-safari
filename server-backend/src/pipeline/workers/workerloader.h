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

#ifndef WORKERLOADER_H
#define WORKERLOADER_H

#include "worker.h"
#include <map>

namespace Postr
{
    
    class SharedObjectWorker;
    
    typedef std::shared_ptr<SharedObjectWorker> SharedWorkerPtr;
    
    /**
    * @brief Finds Workers in shared objects (.so or .dll-files) and loads them
    */
    class WorkerLoader 
    {
    public:
        /**
        * @brief Returns the names of all Workers found on this system
        */
        static const std::vector<std::string> availableWorkers();
        
        /**
        * @brief Loads the Worker with the given name
        * Make sure to delete the returned object!
        * @param name Name of the worker as given by availableWorkers
        * @return A worker with the given name or a nullptr if no appropriate shared object file was found
        */
        static SharedWorkerPtr loadWorker(const std::string& name);
        
    private:
        static const std::map<std::string,std::string> findWorkers(const std::string& name = "");
        static const std::vector<std::string> findWorkerFiles(const std::string& name = "");
    };
    
    /**
     * @brief A base class for Workers that are loaded from shared object files
     */
    class SharedObjectWorker final : public Worker
    {
    public:
        int process(Data data, WorkerCallback& callback = nullptr) override;
        float progress() const override;
        ~SharedObjectWorker();
    protected:
        friend SharedWorkerPtr WorkerLoader::loadWorker(const std::string&);
        SharedObjectWorker(const std::string& name = "", const std::string& filename = "");
    private:
        std::string m_workername;
        int(* m_process)(const char *serialdata, void *callback);
        float(* m_progress)();
        void(* m_free)();
        void *m_handle;
        WorkerCallbackSerialized m_serialized_callback;
        
        SharedObjectWorker(const SharedObjectWorker& other) = delete;
    };
}

#endif
