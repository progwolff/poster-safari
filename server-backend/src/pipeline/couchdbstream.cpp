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

#include "couchdbstream.h"

#include "base64.h"
#include "util.h"

#include <opencv2/highgui.hpp>

#undef LOG
#define LOG(LEVEL) (CLOG(LEVEL, ELPP_CURR_FILE_LOGGER_ID) << "[Couch DB Stream] ")

namespace Postr 
{
    
    CouchDBStream::CouchDBStream(const std::string& url, const long port, const std::string& user, const std::string& pass, bool debug, bool dryrun)
        : m_engine(url, port, user, pass, debug)
        , m_good(true)
        , m_open(true)
        , m_dryrun(dryrun)
    {
        LOG(INFO) << "------------------------------------------------------------";
        LOG(INFO) << "Engine ID: " << m_engine.id();
        LOG(INFO) << "start fetching data from DB.";
    }
    
    CouchDBStream::~CouchDBStream()
    {
        LOG(INFO) << "closed engine " << m_engine.id();
        LOG(INFO) << "------------------------------------------------------------";
    }
    
    int CouchDBStream::count() const
    {
        if(!good() || !is_open())
            return 0;
        return m_engine.count();
    }

    CouchDBStream& CouchDBStream::get(Data& data)
    {
        while(good() && is_open())
        {
            //get id of the first entry available
            std::string id = "";
            std::string rev = "";
            
            while(good() && is_open() && !m_engine.getNextDocument(id, rev));
            
            if(good() && is_open())
            {

                Data resp;
                m_engine.fetchDocument(id, rev, resp, false, false);
                
                if(!good() || !is_open())
                    return *this;
                
                resp.meta["processedBy"] = m_engine.id();
                
                if(!m_dryrun && !m_engine.updateDocument(id, resp))
                {
                    //another engine is processing this document
                    continue;
                }
                
                data = resp;
                
                std::string image = m_engine.fetchAttachment(resp.meta["_id"].asString(), resp.meta["_rev"].asString(), "userimage", Worker::interactive);
                
                image = image.substr(image.find_first_of(':')+1);
                //std::string type = image.substr(0,image.find_first_of(';'));
                image = image.substr(image.find_first_of(',')+1);
            
                std::string decoded = base64_decode(image);
                
                std::vector<uchar> imdata(decoded.begin(), decoded.end());

                data.images.push_back(cv::imdecode(cv::Mat(imdata),cv::IMREAD_ANYCOLOR));
                
                data.meta["images"]["original"] = (int)(data.images.size()-1);
            
            }
            
            return *this;
        }
        LOG(ERROR) << "connection lost while waiting for next document";
        return *this;
    }
            
    bool CouchDBStream::good() const
    {
        //TODO: check for connection errors
        return m_good && !Worker::aborted();
    }
           
    bool CouchDBStream::is_open() const
    {
        //TODO: determine if we have an open connection to a DB
        return m_open;
    }
    
    bool CouchDBStream::attachImage(const std::string& name, const Data& from, Data& to)
    {
        if(!from.meta["images"].isMember(name))
            return false;
        
        std::vector<uchar> encoded;
        cv::imencode(".jpg", from.image(name), encoded);
        
        std::string imdata = base64_encode(encoded.data(), encoded.size());
        
        to.meta["_attachments"][name]["content_type"] = "image/jpeg";
        to.meta["_attachments"][name]["data"] = imdata;
        
        return true;
    }
            
    void CouchDBStream::handleResult(const Data& data)
    {
        if(m_dryrun)
            return;
        
        Data result = data;
        std::string id = result.meta["_id"].asString();
        std::string rev = result.meta["_rev"].asString();
        result.meta = result.meta["result"];
        if(result.meta["title"].asString().empty())
            result.meta["title"] = "Unbekannt";
        attachImage("best", data, result);
        if(data.meta.isMember("event") && !data.meta["event"].asString().empty())
            result.meta["_id"] = data.meta["event"];
        std::string eventId = m_engine.updateEvent(result);
        m_engine.fetchDocument(id, rev, result, false);
        result.meta["event"] = eventId;
        if(m_engine.updateDocument(id, result))
            LOG(INFO) << "finished processing " << id;
        else
            LOG(ERROR) << "failed uploading results for " << id;
    }
    
    void CouchDBStream::handleError(const Data& data)
    {
        if(m_dryrun)
            return;
        
        Data result = data;
        std::string id = result.meta["_id"].asString();
        if(!id.empty())
        {
            LOG(INFO) << "trying to signal other engines that an error occured with " << id;
            result.meta.removeMember("images");
            result.meta.removeMember("processedBy");
            if(m_engine.updateDocument(id, result))
                LOG(INFO) << "signalled another engine to process " << id;
            else
                LOG(ERROR) << "failed to signal other engines that an error occured with " << id;
        }
        m_good = false;
    }
    
    void CouchDBStream::close()
    {
        LOG(INFO) << "closing engine " << m_engine.id();
        m_open = false;
    }
    
    Postr::Data Postr::CouchDBStream::get(std::string id)
    {
        Data resp;
        m_engine.fetchDocument(id, "", resp, false);
        
        if(!good() || !is_open())
            return resp;
        
        std::string image = m_engine.fetchAttachment(resp.meta["_id"].asString(), resp.meta["_rev"].asString(), "userimage", Worker::interactive);
        resp.meta.removeMember("_attachments");
        
        image = image.substr(image.find_first_of(':')+1);
        //std::string type = image.substr(0,image.find_first_of(';'));
        image = image.substr(image.find_first_of(',')+1);
    
        std::string decoded = base64_decode(image);
        
        std::vector<uchar> imdata(decoded.begin(), decoded.end());

        resp.images.push_back(cv::imdecode(cv::Mat(imdata),cv::IMREAD_ANYCOLOR));
        
        resp.meta["images"]["original"] = (int)(resp.images.size()-1);
        
        return resp;
    }

}
