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

#include "workerloader.h"
#include "util.h"

#ifdef _MSC_VER
//TODO: include headers neccessary for shared object loading on MS Windows
#else
#include <dlfcn.h>
#endif

#undef LOG
#define LOG(LEVEL) (CLOG(LEVEL, ELPP_CURR_FILE_LOGGER_ID) << "[WorkerLoader] ")

namespace Postr 
{
    SharedObjectWorker::SharedObjectWorker(const std::string& name, const std::string& filename)
        : Worker("")
        , m_workername(name)
        , m_process(nullptr)
        , m_progress(nullptr)
        , m_free(nullptr)
        , m_serialized_callback(nullptr)
    {
        m_name = m_workername.c_str();
        
        if(filename.empty())
        {
            LOG(ERROR) << "cannot load Worker from empty file";
            return;
        }
        
#ifdef _MSC_VER
        //TODO: implement shared object loading for MS Windows
#else
        char *error;
        m_handle = dlopen(filename.c_str(), RTLD_LAZY);
        if (!m_handle)
        {
            LOG(ERROR) << dlerror();
            return;
        }
        dlerror();    /* Clear any existing error */
        
        m_process = (int(*)(const char *serialdata, void *callback))(dlsym(m_handle, "process"));
        if ((error = dlerror()) != NULL) 
        {
            LOG(ERROR) << error;
            m_process = nullptr;
            return;
        }
        
        m_progress = (float(*)())dlsym(m_handle, "progress");
        if ((error = dlerror()) != NULL) 
        {
            LOG(ERROR) << error;
            m_progress = nullptr;
            return;
        }
        
        m_free = (void(*)())dlsym(m_handle, "freeWorker");
        if ((error = dlerror()) != NULL) 
        {
            LOG(ERROR) << error;
            m_free = nullptr;
            return;
        }
#endif
        
        Dl_info info;
        dladdr((void*)m_free, &info);
        LOG(INFO) << "loaded " << name << " from " << info.dli_fname;
        
    }
    
    SharedObjectWorker::~SharedObjectWorker()
    {
        if(m_free)
            m_free();
        
        delete m_serialized_callback;
        
        dlclose(m_handle);
    }
    
    int SharedObjectWorker::process(Data data, WorkerCallback& callback)
    {
        if(m_process)
        {
            delete m_serialized_callback;
            m_serialized_callback = new std::function<void(const char*, int)>([callback](const char *serialdata, int status){
                Data fromserialized(serialdata);
                if(callback)
                    callback(fromserialized, status);
            });
            return m_process(data.serialize(true).data(), (void*)m_serialized_callback);
        }
        return 1;
    }
    
    float SharedObjectWorker::progress() const
    {
        if(m_progress)
            return m_progress();
        return 0;
    }
    
    const std::vector<std::string> WorkerLoader::availableWorkers()
    {
        std::map<std::string,std::string> workers = findWorkers();
        std::vector<std::string> names;
        for(auto worker : workers)
        {
            names.push_back(worker.first);
        }
        return names;
    }
    
    SharedWorkerPtr WorkerLoader::loadWorker(const std::string& name)
    {
        std::map<std::string,std::string> workers = findWorkers(name);
        if(workers.size())
        {
            auto it=workers.begin();
            return SharedWorkerPtr(new SharedObjectWorker(it->first, it->second));
        }
        
        return nullptr;
    }
       
    const std::map<std::string,std::string> WorkerLoader::findWorkers(const std::string& name)
    {
        std::map<std::string,std::string> workers;
        
        std::vector<std::string> dirs = File::LibraryLocation;
        dirs.push_back(".");
        const std::vector<std::string>& workerfiles = File::locateAll("(lib)?postr_.*\\.(so|dll)", dirs, 3);
        
        for(const std::string& filename : workerfiles)
        {
            const char *error;
            void *handle = dlopen(filename.c_str(), RTLD_LAZY);
            if (!handle)
            {
                //LOG(WARNING) << filename << " is not a valid Worker";
                continue;
            }
    
            dlerror();    /* Clear any existing error */
            
            std::string workername;
            const char*(*_name)() = (const char* (*)())(dlsym(handle, "name"));
            if ((error = dlerror()) != NULL)
            {
                LOG(WARNING) << filename << " is not a valid Worker";
                continue;
            }
            if(_name)
                workername = _name();
            else
                LOG(WARNING) << filename << " is not a valid Worker";
            if(!workername.empty() && (name.empty() || workername == name))
                workers[workername] = filename;
            
            dlclose(handle);
        }
        return workers;
    }
    
}
