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

#ifndef OCRWORKER_H
#define OCRWORKER_H

#include "asyncworker.h"

#include  <opencv2/text.hpp>

#include <thread>
#include <atomic>

#ifdef _WORKER_NAME_
#undef _WORKER_NAME_
#endif
#ifdef _WORKER_CLASS_
#undef _WORKER_CLASS_
#endif

#define _WORKER_NAME_ "OCR" 
#define _WORKER_CLASS_ OCRWorker

namespace Postr 
{
    /**
     * @brief This Worker tries to extract text from an image
     */
    class _WORKER_CLASS_ final : public AsyncWorker
    {
    public:
        _WORKER_CLASS_();
        ~_WORKER_CLASS_();
    
    private:        
        void processAsync(Data& data) override;
        void initAsync() override;
        /**
         * @brief Draw previously extracted ER regions to a matrix
         * @param channels channels that were used with an ER filter
         * @param regions regions found by an ER filter 
         * @param group groups found by an ER filter
         * @param segmentation the matrix to paint on
         */
        void er_draw(std::vector<cv::Mat> &channels, std::vector< std::vector<cv::text::ERStat> > &regions, std::vector<cv::Vec2i> group, cv::Mat& segmentation);
        /**
         * @brief Find out if a string is repetitive
         * @param s a string
         * @return true if s is repetitive, false else
         */
        bool isRepetitive(const std::string& s);
    
        cv::Ptr<cv::text::OCRTesseract> api,fullpageapi;
        cv::Ptr<cv::text::ERFilter> er_filter1,er_filter2;
    };
};

#include "workerexport.h"

#endif //OCRWORKER_H 
