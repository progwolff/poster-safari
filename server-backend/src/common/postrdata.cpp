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

#include <iostream>
#include <opencv2/highgui.hpp>

namespace Postr 
{
    Data::Data(std::string serialstring)
    {
        assign(serialstring);
    }
    
    void Data::assign(std::string serialstring)
    {
        const char* locale = setlocale(LC_ALL, "C");
        meta = MetaData();
        if(!serialstring.empty())
        {
            Json::Reader reader;
            if(!reader.parse(serialstring,meta))
            {
                LOG(ERROR) << "failed parsing json string " << serialstring;
                return;
            }
            
            
            if(meta.isMember("serialized_images") && meta["serialized_images"].isArray())
            {
                images.resize(meta["serialized_images"].size());
                for(int i=0; i<meta["serialized_images"].size(); ++i)
                {
                    int index = meta["serialized_images"][i]["index"].asInt();
                    MetaData serialimage = meta["serialized_images"][i]["data"];
                    images[index] = ImageData::fromSerialized(serialimage);
                }
                meta.removeMember("serialized_images");
            }
        }
        setlocale(LC_ALL, locale);
    }
    
    std::string Data::serialize(bool includeImages, bool beautify) const
    {
        const char* locale = setlocale(LC_ALL, "C");
        MetaData _meta = meta;
        if(includeImages)
        {
            for(int i=0; i<images.size(); ++i)
            {
                _meta["serialized_images"][i]["data"] = images[i].serialize();
                _meta["serialized_images"][i]["index"] = i;
            }
        }
        std::string result;
        if(!beautify)
        {
            Json::FastWriter writer;
            result = writer.write(_meta);
        }
        else
        {
            Json::StyledWriter writer;
            result = writer.write(meta);
        }
        setlocale(LC_ALL, locale);
        return result;
    }
    
    const int Data::originalImage() const
    {
        Json::Value indexVal = meta["images"]["original"];
        
        if(!indexVal.isInt())
            return -1;
        
        int index = indexVal.asInt();
        
        if(index >= 0 && index < images.size())
            return index;
        
        return -1;
    }
    
    const int Data::bestImage() const
    {
        Json::Value indexVal = meta["images"]["best"];
        
        if(!indexVal.isInt())
            return originalImage();
        
        int index = indexVal.asInt();
        
        if(index >= 0 && index < images.size())
            return index;
        
        return originalImage();
    }
    
    const ImageData& Data::image(const std::string name) const
    {
        return images[meta["images"][name].asInt()];
    }

    void Data::log(el::base::type::ostream_t& os) const
    {
        const char* locale = setlocale(LC_ALL, "C");
        Json::StyledWriter writer;
        os << writer.write(meta);
        setlocale(LC_ALL, locale);
    }
    
    void Data::swap(Data& other)
    {
        meta.swap(other.meta);
        images.swap(other.images);
    }
    
    Data& Data::operator =(const Data& other)
    {
        
        meta = MetaData(other.meta);
        images = other.images;
        return *this;
    }
}
