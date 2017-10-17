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

#ifdef WORKER_LIBRARY
#undef WORKER_LIBRARY //make sure we don't export symbols twice
#endif
#include "naivesemanticanalysis.h"
#include "util.h"

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>

#undef LOG
#define LOG(LEVEL) (CLOG(LEVEL, ELPP_CURR_FILE_LOGGER_ID) << "[" << m_name << "/map] ")

namespace Postr 
{
    MetaData _WORKER_CLASS_::normalizeAddress(const std::string& searchstring)
    {
        curlpp::Cleanup cleaner;
        curlpp::Easy request;
        
        std::string searchparam = searchstring;
        String::urlEncode(searchparam);
        request.setOpt(new curlpp::options::Url(("http://nominatim.openstreetmap.org/search.php?countrycodes=de&format=json&addressdetails=1&q="+searchparam).c_str()));
        request.setOpt(new curlpp::options::Verbose(false));
        
        std::stringstream ss;
        request.setOpt(new curlpp::options::WriteStream(&ss));
        request.perform();
        
        std::string response = ss.str();
        Data address("{\"result\": "+response+"}");
        
        LOG(DEBUG) << response;
        
        if(address.meta["result"].isArray())
            address.meta = address.meta["result"][0];
        else
            address.meta = address.meta["result"];
        
        MetaData result;
        if(address.meta.isMember("lat"))
            result["latitude"] = address.meta["lat"];
        if(address.meta.isMember("lon"))
            result["longitude"] = address.meta["lon"];
        if(address.meta.isMember("address"))
        {
            if(address.meta["address"].isMember("city"))
                result["city"] = address.meta["address"]["city"];
            else if(address.meta["address"].isMember("town"))
                result["city"] = address.meta["address"]["town"];
            else if(address.meta["address"].isMember("village"))
                result["city"] = address.meta["address"]["village"];
            else if(address.meta["address"].isMember("residential"))
                result["city"] = address.meta["address"]["residential"];
            if(address.meta["address"].isMember("state"))
                result["region"] = address.meta["address"]["state"];
            if(address.meta["address"].isMember("country"))
                result["country"] = address.meta["address"]["country"];
            if(address.meta["address"].isMember("postcode"))
                result["postal_code"] = address.meta["address"]["postcode"];
            if(address.meta["address"].isMember("road") && address.meta["address"].isMember("house_number"))
                result["address"] = address.meta["address"]["road"].asString()+" "+address.meta["address"]["house_number"].asString();
            else if(address.meta["address"].isMember("road"))
                result["address"] = address.meta["address"]["road"];
        }
        
        return result;
    }
}
