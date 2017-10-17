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

#include "worker.h"
#include "util.h"

#include <chrono>
#include <thread>
#include <iostream>
#include <functional>


#ifdef WORKER_LIBRARY
//INITIALIZE_NULL_EASYLOGGINGPP
#else
INITIALIZE_EASYLOGGINGPP
#endif

namespace Postr
{
#ifndef WORKER_LIBRARY
    std::atomic_bool Worker::interactive(true); 
    std::atomic_bool Worker::debug(false);
    el::base::type::StoragePointer Worker::m_storage = nullptr;
    std::atomic_int Worker::m_configaccesscounter(0);
    std::mutex Worker::m_configaccessmutex;
    Data Worker::m_config;
    std::condition_variable Worker::m_progresscondition;
    std::mutex Worker::m_progressmutex;
    std::atomic_bool Worker::m_abort(false);
    std::atomic_int Worker::m_progressbars(0);
#endif
    
#ifdef ELPP_DISABLE_DEFAULT_CRASH_HANDLING
    void Worker::handleCrash(int sig) {    
        m_abort = true;
        
        if(sig != SIGINT)
        {
            static bool crashed = false;
            if(!crashed)
                el::Helpers::logCrashReason(sig, true, el::Level::Error, el::base::consts::kDefaultLoggerId);
            crashed = true;
            //el::Helpers::crashAbort(sig);
        }
        else
            LOG(INFO) << "Received SIGINT. Waiting for all workers to cancel...";

    }
#endif
    
    void Worker::LogDispatcher::handle(const el::LogDispatchData* /*handlePtr*/)
    {
        m_progressbars = 0;
        std::cout << "\033[J";
        m_progresscondition.notify_all();
    }
    
    Worker::Worker(const char *name)
        : m_status(0)
        , m_name(name)
    {        
        cv::redirectError(cvNulDevReport);
        
        initializeLog(-1);
        
        initializeConfig();
    }
    
    Worker::~Worker()
    {
        releaseConfig();
    }

    void Worker::initializeLog(int loglevel)
    {
        if(!m_storage)
            m_storage = el::Helpers::storage();
        
        el::Helpers::setStorage(m_storage);
#ifdef ELPP_DISABLE_DEFAULT_CRASH_HANDLING
        el::Helpers::setCrashHandler(Worker::handleCrash);
#endif
        el::Helpers::installLogDispatchCallback<Worker::LogDispatcher>(Util::uuid());
        el::Configurations defaultConf;
        defaultConf = *el::Loggers::getLogger("default")->configurations();
        defaultConf.setGlobally(el::ConfigurationType::Format, "%datetime %level %msg");
        defaultConf.setGlobally(el::ConfigurationType::Filename, "pipeline.log");
        defaultConf.setGlobally(el::ConfigurationType::MaxLogFileSize, "2097152"); //2MB
        if(loglevel >= 0)
        {
            el::Level level[6] = {el::Level::Fatal, el::Level::Error, el::Level::Warning, el::Level::Info, el::Level::Verbose, el::Level::Debug};
            for(int i=0; i<6; ++i)
            {
                defaultConf.set(level[i], el::ConfigurationType::ToStandardOutput, loglevel > i ? "true" : "false");
            }
        }
        el::Loggers::reconfigureLogger("default", defaultConf);
        el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
        el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);
    }
    
    void Worker::initializeConfig()
    {
        if(config("filename").empty())
        {
            LOG(INFO) << "initializing config";
            std::string fname = File::locate("postr.conf", File::ConfigLocation);
            if(fname.empty())
            {
                LOG(WARNING) << "Could not find a config file. Creating a new one.";
                config("filename", File::homeDirectory()+"/.config/postr.conf");
            }
            else
            {
                std::ifstream ifs(fname, std::ifstream::in);
                
                std::string str;
                std::string file_contents;
                while (std::getline(ifs, str))
                {
                    file_contents += str;
                    file_contents.push_back('\n');
                } 
                
                { //new scope as the next call to "config" tries to lock the mutex itself
                    std::lock_guard<std::mutex> guard(m_configaccessmutex);
                    Data oldconfig(file_contents);
                    for(auto it = oldconfig.meta.begin(); it != oldconfig.meta.end(); ++it)
                    {
                        m_config.meta[it.key().asString()] = oldconfig.meta[it.key().asString()];
                    }
                }
                config("filename", fname);
            }
            
            LOG(INFO) << "using config file " << config("filename");
        }
        
        ++m_configaccesscounter;
    }
    
    void Worker::releaseConfig()
    {
        --m_configaccesscounter;
        if(m_configaccesscounter <= 0)
        {
            std::string fname = config("filename", File::homeDirectory()+"/.config/postr.conf");
                
            std::ofstream ofs(fname, std::ofstream::out);
            
            std::lock_guard<std::mutex> guard(m_configaccessmutex);
            m_config.meta.removeMember("filename");
            Json::StyledWriter writer;
            ofs << writer.write(m_config.meta);
            LOG(DEBUG) << "writing config to " << fname;
        }
    }
    
    int Worker::processBlocking(Data& data)
    {
        std::set<const Worker*> progress;
        if(interactive)
            progress = {this};
        
        DataPtr d(new Data);
        *d = data;
        waitForFinished(*this << d, progress);
        
        return status();
    }
    
    void Worker::printProgress(bool cr) const
    {
        if(!progress() && 0 == status())
            Output::printProgress(m_name, 100, cr);
        else
            Output::printProgress(m_name, progress(), cr);
    }
    
    int Worker::status() const
    {
        return m_status;
    }
    
    std::string Worker::name() const
    {
        return m_name;
    }
    
    std::string Worker::config(const std::string& name, const std::string& def, const std::string& descr) const
    {
        std::lock_guard<std::mutex> guard(m_configaccessmutex);
        //LOG(DEBUG) << name << ", string: " << m_config.serialize();
        
        m_config.meta[name]["default"] = def;
        if(!descr.empty())
            m_config.meta[name]["description"] = descr;
        
        if(m_config.meta.isMember(name) && m_config.meta[name].isMember("value"))
            return m_config.meta[name]["value"].asString();
        
        m_config.meta[name]["value"] = def;
        
        return def;
    }
    
    std::string Worker::config(const std::string& name) const
    {
        std::lock_guard<std::mutex> guard(m_configaccessmutex);
        //LOG(DEBUG) << name << ", string, no default: " << m_config.serialize();
        
        if(m_config.meta.isMember(name) && m_config.meta[name].isMember("value"))
            return m_config.meta[name]["value"].asString();
        
        return "";
    }
    
    int Worker::config(const std::string& name, const int def, const std::string& descr) const
    {
        std::lock_guard<std::mutex> guard(m_configaccessmutex);
        //LOG(DEBUG) << name << ", int: " << m_config.serialize();
        
        m_config.meta[name]["default"] = def;
        if(!descr.empty())
            m_config.meta[name]["description"] = descr;
    
        if(m_config.meta.isMember(name) && m_config.meta[name].isMember("value") && m_config.meta[name]["value"].isInt())
            return m_config.meta[name]["value"].asInt();
        
        m_config.meta[name]["value"] = def;
        
        return def;
    }
    
    double Worker::config(const std::string& name, const double def, const std::string& descr) const
    {
        std::lock_guard<std::mutex> guard(m_configaccessmutex);
        //LOG(DEBUG) << name << ", double: " << m_config.serialize();
        
        m_config.meta[name]["default"] = def;
        if(!descr.empty())
            m_config.meta[name]["description"] = descr;
    
        if(m_config.meta.isMember(name) && m_config.meta[name].isMember("value") && m_config.meta[name]["value"].isDouble())
            return m_config.meta[name]["value"].asDouble();
        
        m_config.meta[name]["value"] = def;
        
        return def;
    }
    
    bool Worker::configBool(const std::string& name, const bool def, const std::string& descr) const
    {
        std::lock_guard<std::mutex> guard(m_configaccessmutex);
        //LOG(DEBUG) << name << ", bool: " << m_config.serialize();
        
        m_config.meta[name]["default"] = def;
        if(!descr.empty())
            m_config.meta[name]["description"] = descr;
    
        if(m_config.meta.isMember(name) && m_config.meta[name].isMember("value") && m_config.meta[name]["value"].isBool())
            return m_config.meta[name]["value"].asBool();
        
        m_config.meta[name]["value"] = def;
        
        return def;
    }
    
    bool Worker::waitForFinished(ChainValue c, std::set<const Worker*> workers)
    {
        std::unique_lock<std::mutex> lk(m_progressmutex);
        
        if((*c)>0)
        Worker::m_progresscondition.wait(lk, [c,workers](){
            //LOG(DEBUG) << "A notified";
            
            if(interactive && !workers.empty())
            {
                bool printed = false;
                std::cout << "\033[2K";
                static int lastcount = 0;
                int count = 0;
                for(const Worker* w : workers)
                {
                    if(w->progress())
                        ++count;
                }
                if(!m_progressbars)
                    --lastcount;
                if(count < lastcount)
                    std::cout << "\033["+std::to_string(lastcount-count)+"B\r";
                for(const Worker* w : workers)
                {
                    if(w->progress())
                    {
                        printed = true;
                        w->printProgress(false);
                        std::cout << "\n";
                    }
                }
                std::cout << "\033[J";
                m_progressbars = count;
                if(m_progressbars)
                    std::cout << "\033["+std::to_string(count)+"A\r";
                std::cout.flush();
                lastcount = count;
            }
            
            if(m_abort)
                return true;
            
            //LOG(DEBUG) << "A done";
            return ((*c) <= 0);
        });
        
        LOG(DEBUG) << "stop waiting";
        
        return !m_abort;
    }
    
    Worker::ChainValue operator<<(Worker::WorkerChain chain, DataPtr data)
    {
        Worker::ChainCounter pending(new std::atomic_int);
        (*pending) = chain.count();
        //Begin processing this chain
        
        std::shared_ptr< std::promise<void> > p(new std::promise<void>);
        
        if(!Worker::m_abort)
            chain(data, pending, std::move(*p));
        return pending;
    }
    
    Worker::operator WorkerChain ()
    {
        //return a function which...
        return {[this](DataPtr data, ChainCounter pending, std::promise<void> promise) {
            
            //...first calls the right worker
            if(!m_abort)
            {
                std::thread t([this,data,pending](std::promise<void> promise){
                    std::shared_ptr< std::promise<void> > p(new std::promise<void>);
                    *p = std::move(promise);
                    process(*data, [this,data,pending,p](Data d, int status) {
                        if(0 != status)
                            LOG(ERROR) << name() << " failed";
                        
                        *data = d;
                    
                        --(*pending);
                        
                        if(0 == *pending)
                            (*p).set_value();
                        
                    });
                },std::move(promise));
                t.detach();
            }
        },1, {this}};
    }
    
    Worker::WorkerChain operator<<(Worker::WorkerChain left, Worker& right)
    {
        left.workers().insert(&right);
        //return a function which...
        return {[left, &right](DataPtr data, Worker::ChainCounter pending, std::promise<void> promise) {
            
            //...first calls the right worker
            if(!Worker::m_abort)
            {
                std::thread t([&right,left,data,pending](std::promise<void> promise){
                    std::shared_ptr< std::promise<void> > p(new std::promise<void>);
                    *p = std::move(promise);
                    right.process(*data, [&right,left,data,pending,p](Data d, int status) {
                        *data = d;
                        if(0 != status)
                        {
                            LOG(ERROR) << right.name() << " failed";
                            *pending = 0;
                        }
                        else
                        {
                            --(*pending);
                            //...and calls the rest of the chain when the right finished.
                            left(data, pending, std::move(*p));
                        }
                        if(0 == *pending)
                            (*p).set_value();
                    });
                },std::move(promise));
                t.detach();
            }
        }, left.count()+1, left.workers()};
    }
    
    Worker::WorkerChain operator<<(Worker::WorkerChain left, Worker::WorkerChain right)
    {        
        left.workers().insert(right.workers().begin(), right.workers().end());
        return {[left, right](DataPtr data, Worker::ChainCounter pending, std::promise<void> promise) {
    
            std::thread t([left, right, data, pending](std::promise<void> promise){
                int p;
                
                Worker::ChainCounter p1(new std::atomic_int);   
                
                (*p1) = right.count();
                p = *pending;
 
                std::promise<void> promiseRight;
                std::future<void> future = promiseRight.get_future();
                if(!Worker::m_abort)
                {
                    right(data, p1, std::move(promiseRight));
                    future.wait();
                }
                
                (*pending) = p-right.count();
                LOG(DEBUG) << "right chain done. start processing left chain. pending: " << *pending;
                
                if(!Worker::m_abort)
                    left(data,pending,std::move(promise));
                
            },std::move(promise));
            t.detach();
            
        }, right.count()+left.count(), left.workers()};
    }
    
    Worker::WorkerChain Worker::join(Worker::WorkerChain one, Worker::WorkerChain other, std::function<void(Data&,const Data&)> fn)
    {
        one.workers().insert(other.workers().begin(), other.workers().end());
        return {[one, other, fn](DataPtr data, ChainCounter pending, std::promise<void> promise) {
            
            std::thread t([fn,one,other,data,pending](std::promise<void> promise){
                LOG(DEBUG) << "started joiner thread for " << data->meta["filename"].asString();
                
                int p = *pending;
                DataPtr d1 = DataPtr(new Data);
                (*d1) = (*data);
                DataPtr d2 = DataPtr(new Data);
                (*d2) = (*data);
                ChainCounter p1(new std::atomic_int);
                ChainCounter p2(new std::atomic_int);
                
                (*p1) = one.count();
                (*p2) = other.count();
                
                std::promise<void> promise1;
                std::promise<void> promise2;
                std::future<void> future1 = promise1.get_future();
                std::future<void> future2 = promise2.get_future();
                
                one(d1,p1,std::move(promise1));
                other(d2,p2,std::move(promise2));
            
                future1.wait();
                future2.wait();
                
                LOG(DEBUG) << "joining!";
                
                fn(*d1,*d2);
                *data = *d1;
                
                (*pending) = p-one.count()-other.count();
                LOG(DEBUG) << "pending after join: " << *pending;
                
                promise.set_value();
            },std::move(promise));
            t.detach();
            
        }, one.count()+other.count(), one.workers()};
    }
    
    bool Worker::aborted()
    {
        return m_abort;
    }
}
