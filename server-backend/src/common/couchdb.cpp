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

#include "couchdb.h"

#include <sstream>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>

#include "easylogging++.h"

#include "base64.h"
#include "util.h"

namespace Postr
{
    CouchDB::CouchDB(const std::string& url, const long port, const std::string& user, const std::string& pass, bool debug)
        : m_id(Postr::Util::uuid())
        , m_url(url)
        , m_port(port)
        , m_user(user)
        , m_pass(pass)
        , m_debugDB(debug)
    {
    }

    CouchDB::~CouchDB()
    {
    }

    std::string CouchDB::id() const
    {
        return m_id;
    }

    double CouchDB::progressCallback(double dltotal, double dlnow, double ultotal, double ulnow) const
    {
        if(dlnow < dltotal && dlnow >= 0)
        {
            Output::printProgress("http download", 100*dlnow/dltotal, false);
            std::cout << "\t";
        }
        if(ulnow < ultotal && ulnow >= 0)
            Output::printProgress("http upload", 100*ulnow/ultotal, false);
        std::cout << "\r";

        return 0;
    }

    int CouchDB::count(bool interactive) const
    {
        int ret = 0;
        try {
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            if(interactive)
            {
                curlpp::options::ProgressFunction progressBar([this](double dltotal, double dlnow, double ultotal, double ulnow){return progressCallback(dltotal, dlnow, ultotal, ulnow);});
                request.setOpt(new curlpp::options::NoProgress(0));
                request.setOpt(progressBar);
            }

            request.setOpt(new curlpp::options::Url((m_url+"/"+posterDB()+"/_all_docs").c_str()));
            request.setOpt(new curlpp::options::Port(m_port));
            request.setOpt(new curlpp::options::Verbose(false));

            request.setOpt(new curlpp::options::UserPwd((m_user+":"+m_pass).c_str()));

            std::stringstream ss;
            request.setOpt(new curlpp::options::WriteStream(&ss));
            request.perform();

            std::string response = ss.str();

            Data resp(response);

            ret = resp.meta["total_rows"].asInt();
        }
        catch ( curlpp::LogicError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return 0;
        }
        catch ( curlpp::RuntimeError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return 0;
        }

        return ret;
    }
    
    MetaData CouchDB::getSelectedDocumentIds(const std::string& sel, int limit, bool interactive)
    {
        try {
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            if(interactive)
            {
                curlpp::options::ProgressFunction progressBar([this](double dltotal, double dlnow, double ultotal, double ulnow){return progressCallback(dltotal, dlnow, ultotal, ulnow);});
                request.setOpt(new curlpp::options::NoProgress(0));
                request.setOpt(progressBar);
            }

            request.setOpt(new curlpp::options::Url(("http://"+m_url+"/"+posterDB()+"/_find").c_str()));
            request.setOpt(new curlpp::options::Port(m_port));
            request.setOpt(new curlpp::options::Verbose(false));

            request.setOpt(new curlpp::options::UserPwd((m_user+":"+m_pass).c_str()));

            std::list<std::string> header;
            header.push_back("Content-Type: application/json");
            header.push_back("Accept: application/json");
            header.push_back("Referer: http://localhost/"+posterDB()+"");
            header.push_back("Host: localhost");
            request.setOpt(new curlpp::options::HttpHeader(header));

            std::string selector = "{ \"selector\": " + sel + ", \"limit\": " + std::to_string(limit) + ", \"fields\": [ \"_id\", \"_rev\" ] }";
            
            request.setOpt(new curlpp::options::PostFields(selector));
            request.setOpt(new curlpp::options::PostFieldSize(selector.length()));

            std::stringstream ss;
            request.setOpt(new curlpp::options::WriteStream(&ss));
            request.perform();

            std::string response = ss.str();

            Data resp(response);

            return resp.meta["docs"];
        }
        catch ( curlpp::LogicError & e ) {
            LOG(ERROR) << e.what() << std::endl;
        }
        catch ( curlpp::RuntimeError & e ) {
            LOG(ERROR) << e.what() << std::endl;
        }
        return MetaData();
    }
    
    MetaData CouchDB::getSelectedDocumentIdsByEvents(const std::string& sel, int limit, bool interactive)
    {
        try {
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            if(interactive)
            {
                curlpp::options::ProgressFunction progressBar([this](double dltotal, double dlnow, double ultotal, double ulnow){return progressCallback(dltotal, dlnow, ultotal, ulnow);});
                request.setOpt(new curlpp::options::NoProgress(0));
                request.setOpt(progressBar);
            }

            request.setOpt(new curlpp::options::Url(("http://"+m_url+"/"+eventDB()+"/_find").c_str()));
            request.setOpt(new curlpp::options::Port(m_port));
            request.setOpt(new curlpp::options::Verbose(false));

            request.setOpt(new curlpp::options::UserPwd((m_user+":"+m_pass).c_str()));

            std::list<std::string> header;
            header.push_back("Content-Type: application/json");
            header.push_back("Accept: application/json");
            header.push_back("Referer: http://localhost/"+eventDB()+"");
            header.push_back("Host: localhost");
            request.setOpt(new curlpp::options::HttpHeader(header));

            std::string selector = "{ \"selector\": " + sel + ", \"limit\": " + std::to_string(limit) + ", \"fields\": [ \"_id\", \"_rev\" ] }";
            request.setOpt(new curlpp::options::PostFields(selector));
            request.setOpt(new curlpp::options::PostFieldSize(selector.length()));

            std::stringstream ss;
            request.setOpt(new curlpp::options::WriteStream(&ss));
            request.perform();

            std::string response = ss.str();

            Data resp(response);
            
            selector = "{                                        "
                        "         \"event\" : {                  "
                        "                  \"$in\": [            ";
            for(const Postr::MetaData& doc : resp.meta["docs"])
            {
                selector += "\"" + doc["_id"].asString() + "\",";
            }
            selector = selector.substr(0,selector.length()-1);
            selector += "              ]                         "
                        "         }                              "
                        "}                                       ";
            
            return getSelectedDocumentIds(selector, limit, interactive);
        }
        catch ( curlpp::LogicError & e ) {
            LOG(ERROR) << e.what() << std::endl;
        }
        catch ( curlpp::RuntimeError & e ) {
            LOG(ERROR) << e.what() << std::endl;
        }
        return MetaData();
    }

    bool CouchDB::getNextDocument(std::string& id, std::string& rev, bool interactive)
    {
        
        //select the first document which is not currently being processed by any engine
        MetaData docs = getSelectedDocumentIds("{"
                            "        \"$and\": ["
                            "        {   \"$not\": {"
                            "                \"processedBy\": { \"$and\": ["
                            "                    {\"$exists\": true},"
                            "                    {\"$ne\": null}"
                            "                ]}"
                            "            }"
                            "        },"
                            "        {"
                            "            \"_attachments.userimage\": {\"$exists\": true}"
                            "        },"
                            "        {   \"$not\": {"
                            "                \"event\": { \"$and\": ["
                            "                    {\"$exists\": true},"
                            "                    {\"$ne\": null}"
                            "                ]}"
                            "            }"
                            "        }"
                            "        ]"
                            "}", 1, interactive);
    
        if(!docs.isArray() || !docs.size())
            return false;
        
        id = docs[0]["_id"].asString();
        rev = docs[0]["_rev"].asString();

        LOG(INFO) << "fetching document with id " << id;
        return true;
    }

    bool CouchDB::getDocumentId(int index, std::string& id, std::string& rev, bool interactive)
    {
        try {
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            if(interactive)
            {
                curlpp::options::ProgressFunction progressBar([this](double dltotal, double dlnow, double ultotal, double ulnow){return progressCallback(dltotal, dlnow, ultotal, ulnow);});
                request.setOpt(new curlpp::options::NoProgress(0));
                request.setOpt(progressBar);
            }

            request.setOpt(new curlpp::options::Url(("http://"+m_url+"/"+posterDB()+"/_all_docs").c_str()));
            request.setOpt(new curlpp::options::Port(m_port));
            request.setOpt(new curlpp::options::Verbose(false));

            request.setOpt(new curlpp::options::UserPwd((m_user+":"+m_pass).c_str()));

            std::list<std::string> header;
            header.push_back("Content-Type: application/json");
            header.push_back("Accept: application/json");
            header.push_back("Referer: http://localhost/"+posterDB()+"");
            header.push_back("Host: localhost");
            request.setOpt(new curlpp::options::HttpHeader(header));

            std::stringstream ss;
            request.setOpt(new curlpp::options::WriteStream(&ss));
            request.perform();

            std::string response = ss.str();

            Data resp(response);

            int count = resp.meta["rows"].size();

            if(count <= index)
            {
                //no data available
                return false;
            }

            id = resp.meta["rows"][index]["id"].asString();
            rev = resp.meta["rows"][index]["value"]["rev"].asString();

            LOG(INFO) << "fetching document with id " << id;
        }
        catch ( curlpp::LogicError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return false;
        }
        catch ( curlpp::RuntimeError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return false;
        }

        return true;
    }

    bool CouchDB::getAllDocumentIds(std::vector<std::string>& id, std::vector<std::string>& rev, bool interactive)
    {
        try {
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            if(interactive)
            {
                curlpp::options::ProgressFunction progressBar([this](double dltotal, double dlnow, double ultotal, double ulnow){return progressCallback(dltotal, dlnow, ultotal, ulnow);});
                request.setOpt(new curlpp::options::NoProgress(0));
                request.setOpt(progressBar);
            }

            request.setOpt(new curlpp::options::Url(("http://"+m_url+"/"+posterDB()+"/_all_docs").c_str()));
            request.setOpt(new curlpp::options::Port(m_port));
            request.setOpt(new curlpp::options::Verbose(false));

            request.setOpt(new curlpp::options::UserPwd((m_user+":"+m_pass).c_str()));

            std::list<std::string> header;
            header.push_back("Content-Type: application/json");
            header.push_back("Accept: application/json");
            header.push_back("Referer: http://localhost/"+posterDB()+"");
            header.push_back("Host: localhost");
            request.setOpt(new curlpp::options::HttpHeader(header));

            std::stringstream ss;
            request.setOpt(new curlpp::options::WriteStream(&ss));
            request.perform();

            std::string response = ss.str();

            Data resp(response);

            int count = resp.meta["rows"].size();

            id.resize(count);
            rev.resize(count);

            for(int i=0; i < count; ++i)
            {
                id[i] = resp.meta["rows"][i]["id"].asString();
                rev[i] = resp.meta["rows"][i]["value"]["rev"].asString();
            }
        }
        catch ( curlpp::LogicError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return false;
        }
        catch ( curlpp::RuntimeError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return false;
        }

        return true;
    }

    std::string CouchDB::fetchDocument(const std::string& id, const std::string& rev, Data& data, bool withAttachments, bool interactive)
    {
        if(id.empty())
            return "";
        
        try
        {
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            if(interactive)
            {
                curlpp::options::ProgressFunction progressBar([this](double dltotal, double dlnow, double ultotal, double ulnow){return progressCallback(dltotal, dlnow, ultotal, ulnow);});
                request.setOpt(new curlpp::options::NoProgress(0));
                request.setOpt(progressBar);
            }

            std::list<std::string> header;
            header.push_back("Accept: application/json");
            header.push_back("Referer: http://localhost/"+posterDB()+"");
            header.push_back("Host: localhost");
            request.setOpt(new curlpp::options::HttpHeader(header));

            std::string url = m_url+"/"+posterDB()+"/"+id;
            if(!rev.empty())
            {
                url += "?rev="+rev;
                if(withAttachments)
                    url += "&attachments=true";
            }
            else if(withAttachments)
                url += "?attachments=true";

            request.setOpt(new curlpp::options::Url((url).c_str()));
            request.setOpt(new curlpp::options::Port(m_port));
            request.setOpt(new curlpp::options::Verbose(false));

            request.setOpt(new curlpp::options::UserPwd((m_user+":"+m_pass).c_str()));

            std::stringstream ss;
            request.setOpt(new curlpp::options::WriteStream(&ss));
            request.perform();

            std::string response = ss.str();

            data.assign(response);

            if(data.meta["_id"].asString() == id)
                return data.meta["_rev"].asString();
        }
        catch ( curlpp::LogicError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return "";
        }
        catch ( curlpp::RuntimeError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return "";
        }

        return "";
    }

    std::string CouchDB::fetchAttachment(const std::string& id, const std::string& rev, const std::string& name, bool interactive, bool fromEvent)
    {
        try
        {
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            if(interactive)
            {
                curlpp::options::ProgressFunction progressBar([this](double dltotal, double dlnow, double ultotal, double ulnow){return progressCallback(dltotal, dlnow, ultotal, ulnow);});
                request.setOpt(new curlpp::options::NoProgress(0));
                request.setOpt(progressBar);
            }

            std::list<std::string> header;
            header.push_back("Accept: application/json");
            header.push_back("Referer: http://localhost/"+posterDB()+"");
            header.push_back("Host: localhost");
            request.setOpt(new curlpp::options::HttpHeader(header));

            std::string url = m_url+"/"+((!fromEvent)?posterDB():eventDB())+"/"+id+"/"+name;
            if(!rev.empty())
            {
                url += "?rev="+rev;
            }

            request.setOpt(new curlpp::options::Url((url).c_str()));
            request.setOpt(new curlpp::options::Port(m_port));
            request.setOpt(new curlpp::options::Verbose(false));

            request.setOpt(new curlpp::options::UserPwd((m_user+":"+m_pass).c_str()));

            std::stringstream ss;
            request.setOpt(new curlpp::options::WriteStream(&ss));
            request.perform();

            if(curlpp::infos::ResponseCode::get(request) != 200)
                return "";

            std::string response = ss.str();

            return base64_encode(reinterpret_cast<const unsigned char*>(response.data()), response.length());
        }
        catch ( curlpp::LogicError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return "";
        }
        catch ( curlpp::RuntimeError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return "";
        }

        return "";
    }

    bool CouchDB::updateDocument(const std::string& id, Data& data, bool interactive)
    {
        try
        {
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            if(interactive)
            {
                curlpp::options::ProgressFunction progressBar([this](double dltotal, double dlnow, double ultotal, double ulnow){return progressCallback(dltotal, dlnow, ultotal, ulnow);});
                request.setOpt(new curlpp::options::NoProgress(0));
                request.setOpt(progressBar);
            }

            std::list<std::string> header;
            header.push_back("Accept: application/json");
            header.push_back("Referer: http://localhost/"+posterDB()+"");
            header.push_back("Host: localhost");
            request.setOpt(new curlpp::options::HttpHeader(header));

            std::string url = m_url+"/"+posterDB()+"/"+id;

            request.setOpt(new curlpp::options::Url(url.c_str()));
            request.setOpt(new curlpp::options::Port(m_port));
            request.setOpt(new curlpp::options::Verbose(false));

            request.setOpt(new curlpp::options::UserPwd((m_user+":"+m_pass).c_str()));

            if(data.meta["_id"].asString() != id)
                LOG(WARNING) << "updating document " << id << " with content id " << data.meta["_id"].asString();
            data.meta["_id"] = id;

            std::string putdata = data.serialize(false);
            request.setOpt(new curlpp::options::Put(true));

            std::stringstream rss(putdata);
            request.setOpt(new curlpp::options::ReadStream(&rss));

            std::stringstream wss;
            request.setOpt(new curlpp::options::WriteStream(&wss));
            request.perform();

            std::string response = wss.str();
            Data resp(response);

            if(!resp.meta["error"].asString().empty())
                LOG(ERROR) << resp.meta["error"].asString() << ". " << resp.meta["reason"].asString();

            if(resp.meta["id"].asString() == id)
            {
                data.meta["_rev"] = resp.meta["rev"];
                return true;
            }
        }
        catch ( curlpp::LogicError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return false;
        }
        catch ( curlpp::RuntimeError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return false;
        }

        LOG(ERROR) << "mismatching id's after updating document" << id;
        return false;
    }
    
    std::string CouchDB::updateEvent(Data& data, bool interactive)
    {
        try
        {
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            if(interactive)
            {
                curlpp::options::ProgressFunction progressBar([this](double dltotal, double dlnow, double ultotal, double ulnow){return progressCallback(dltotal, dlnow, ultotal, ulnow);});
                request.setOpt(new curlpp::options::NoProgress(0));
                request.setOpt(progressBar);
            }
            
            
            std::string id;
            if(data.meta.isMember("_id") && !data.meta["_id"].asString().empty())
                id = data.meta["_id"].asString();
            else 
                return createEvent(data, interactive);
            
            std::string url = m_url+"/"+eventDB()+"/"+id;
            
            Postr::Data d;
            fetchEvent(id, "", d);
            
            if(!d.meta.isMember("_rev"))
                return createEvent(data, interactive);
            
            data.meta["_rev"] = d.meta["_rev"];
            
            std::list<std::string> header;
            header.push_back("Accept: application/json");
            header.push_back("Referer: http://localhost/"+eventDB()+"");
            header.push_back("Host: localhost");
            request.setOpt(new curlpp::options::HttpHeader(header));

            request.setOpt(new curlpp::options::Url(url.c_str()));
            request.setOpt(new curlpp::options::Port(m_port));
            request.setOpt(new curlpp::options::Verbose(false));

            request.setOpt(new curlpp::options::UserPwd((m_user+":"+m_pass).c_str()));

            std::string putdata = data.serialize(false);
            request.setOpt(new curlpp::options::Put(true));

            std::stringstream rss(putdata);
            request.setOpt(new curlpp::options::ReadStream(&rss));

            std::stringstream wss;
            request.setOpt(new curlpp::options::WriteStream(&wss));
            request.perform();

            std::string response = wss.str();
            Data resp(response);

            if(!resp.meta["error"].asString().empty())
                LOG(ERROR) << resp.meta["error"].asString() << ". " << resp.meta["reason"].asString();

            if(resp.meta.isMember("id"))
                return resp.meta["id"].asString();
        }
        catch ( curlpp::LogicError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return "";
        }
        catch ( curlpp::RuntimeError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return "";
        }

        LOG(ERROR) << "mismatching id's after updating event" << data.meta["_id"];
        return "";
    }

    std::string CouchDB::createEvent(Data& data, bool interactive)
    {
        try
        {
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            if(interactive)
            {
                curlpp::options::ProgressFunction progressBar([this](double dltotal, double dlnow, double ultotal, double ulnow){return progressCallback(dltotal, dlnow, ultotal, ulnow);});
                request.setOpt(new curlpp::options::NoProgress(0));
                request.setOpt(progressBar);
            }

            std::list<std::string> header;
            header.push_back("Content-Type: application/json");
            header.push_back("Accept: application/json");
            header.push_back("Referer: http://localhost/"+posterDB()+""); 
            header.push_back("Host: localhost");
            request.setOpt(new curlpp::options::HttpHeader(header));

            std::string url = m_url+"/"+eventDB()+"";

            request.setOpt(new curlpp::options::Url(url.c_str()));
            request.setOpt(new curlpp::options::Port(m_port));
            request.setOpt(new curlpp::options::Verbose(false));

            request.setOpt(new curlpp::options::UserPwd((m_user+":"+m_pass).c_str()));

            std::string postdata = data.serialize(false);

            request.setOpt(new curlpp::options::PostFields(postdata));
            request.setOpt(new curlpp::options::PostFieldSize(postdata.length()));

            std::stringstream ss;
            request.setOpt(new curlpp::options::WriteStream(&ss));
            request.perform();

            std::string response = ss.str();
            Data resp(response);

            if(!resp.meta["error"].asString().empty())
                LOG(ERROR) << resp.meta["error"].asString() << ". " << resp.meta["reason"].asString();

            if(resp.meta.isMember("id"))
                return resp.meta["id"].asString();
        }
        catch ( curlpp::LogicError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return "";
        }
        catch ( curlpp::RuntimeError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return "";
        }

        return "";
    }

    std::string CouchDB::fetchEvent(const std::string& id, const std::string& rev, Data& data, bool interactive)
    {
        if(id.empty())
            return "";
        
        try
        {
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            if(interactive)
            {
                curlpp::options::ProgressFunction progressBar([this](double dltotal, double dlnow, double ultotal, double ulnow){return progressCallback(dltotal, dlnow, ultotal, ulnow);});
                request.setOpt(new curlpp::options::NoProgress(0));
                request.setOpt(progressBar);
            }

            std::list<std::string> header;
            header.push_back("Accept: application/json");
            header.push_back("Referer: http://localhost/"+eventDB()+"");
            header.push_back("Host: localhost");
            request.setOpt(new curlpp::options::HttpHeader(header));

            std::string url = m_url+"/"+eventDB()+"/"+id;
            if(!rev.empty())
            {
                url += "?rev="+rev;
            }

            request.setOpt(new curlpp::options::Url((url).c_str()));
            request.setOpt(new curlpp::options::Port(m_port));
            request.setOpt(new curlpp::options::Verbose(false));

            request.setOpt(new curlpp::options::UserPwd((m_user+":"+m_pass).c_str()));

            std::stringstream ss;
            request.setOpt(new curlpp::options::WriteStream(&ss));
            request.perform();

            std::string response = ss.str();

            data.assign(response);

            if(data.meta["_id"].asString() == id)
                return data.meta["_rev"].asString();
        }
        catch ( curlpp::LogicError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return "";
        }
        catch ( curlpp::RuntimeError & e ) {
            LOG(ERROR) << e.what() << std::endl;
            return "";
        }

        return "";
    }

}
