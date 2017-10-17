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

#include "imagedata.h"
#include "metadata.h"
#include "base64.h"

namespace Postr 
{
    ImageData::ImageData()
        : cv::Mat()
    {
    }
    
    ImageData::ImageData(const cv::Mat& m)
    {
        if( this != &m )
        {
            if( m.u )
                CV_XADD(&m.u->refcount, 1);
            release();
            flags = m.flags;
            if( dims <= 2 && m.dims <= 2 )
            {
                dims = m.dims;
                rows = m.rows;
                cols = m.cols;
                step[0] = m.step[0];
                step[1] = m.step[1];
            }
            else
                copySize(m);
            data = m.data;
            datastart = m.datastart;
            dataend = m.dataend;
            datalimit = m.datalimit;
            allocator = m.allocator;
            u = m.u;
        }
    }
    
    ImageData ImageData::fromSerialized(const MetaData& serialimage)
    {
        int type = serialimage["type"].asInt();
        int rows = serialimage["rows"].asInt();
        int cols = serialimage["cols"].asInt();
        std::string datastring = base64_decode(serialimage["data"].asString());
        cv::Mat tmp(rows, cols, type, (void*)datastring.data());
        return tmp.clone();
    }
    
    const MetaData ImageData::serialize() const
    {
        MetaData serialimage;
        serialimage["type"] = type();
        serialimage["rows"] = rows;
        serialimage["cols"] = cols;
        serialimage["elemSize"] = (int)elemSize();
        serialimage["total"] = (int)total();
        serialimage["data"] = base64_encode(data, elemSize()*total());;
        return serialimage;
    }
 
}
