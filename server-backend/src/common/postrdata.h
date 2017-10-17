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

#ifndef POSTRDATA_H
#define POSTRDATA_H


#include <vector>
#include <string>

#include "metadata.h"
#include "imagedata.h"
#include "easylogging++.h"

namespace Postr 
{
    /**
    * @brief Struct holding all the information that may be neccessary by any stage in the image processing / semantic analysis pipeline
    */
    struct Data : public el::Loggable
    {
        Data(std::string serialstring = "");
        
        void assign(std::string serialstring);
        
        /**
        * @brief Metadata holding information about the images in this struct as well as results of semantic analysis steps.
        */
        MetaData meta;

        /**
        * @brief Images produces by any stages of the pipeline.
        * May include the source image, processed images, cutouts, ... which are further described by MetaData meta
        */
        std::vector<ImageData> images;
        
        /**
         * @brief Index of the original image that is processed in a pipeline.
         * @return The index of the original image or -1 if no such image exists.
         */
        const int originalImage() const; 
        
        /**
         * @brief Index of the best image that has been generated by the pipeline.
         * This is an image that contains the full poster but may be "improved" in a sense that features can be extracted better than from the original image. 
         * @return The index of the best image or the original image.
         */
        const int bestImage() const; 
        
        /**
         * @brief Get the image with given name
         * Shorthand for this.images[this.meta["images"][name].asInt()]
         * @param name name of the image
         * @return a reference to the image
         */
        const ImageData& image(const std::string name) const;
        
        /**
         * @brief Return a c-string representing the current data.
         * @param includeImages if true, images are includes as base64 encoded strings
         * @param beautify Print with linebreaks and indentations
         */
        std::string serialize(bool includeImages = true, bool beautify = false) const;
        
        virtual void log(el::base::type::ostream_t& os) const override;
        
        /**
         * @brief swap everything
         * @param other data object to swap internals with
         */
        void swap(Data& other);
        
        Data& operator =(const Data& other);
    };
    
    
    typedef std::shared_ptr<Data> DataPtr;
}
#endif