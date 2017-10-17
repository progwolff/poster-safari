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

#ifndef IMAGEDATA_H
#define IMAGEDATA_H

#include <opencv2/imgproc.hpp>
#include <string>
#include "metadata.h"

namespace Postr 
{
    /**
    * @brief Image data container
    */
    class ImageData : public cv::Mat
    {
    public:
        ImageData();
        ImageData(const cv::Mat& mat);
        
        /**
         * @brief Construct a new instance of ImageData from a serialized string
         * @param serialimage a serialized string representing an instance of ImageData
         * @return an instance of ImageData
         */
        static ImageData fromSerialized(const MetaData& serialimage);
        
        /**
         * @brief Serialize this instance of ImageData
         * @return a serialized string representing this instance of ImageData
         */
        const MetaData serialize() const;
    };
}

#endif
