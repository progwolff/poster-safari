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

#include "bgsegmentworker.h"

#include <opencv2/highgui.hpp>
#include <iostream>

int main(int argc, char** argv)
{
    if(argc < 2)
        std::cout << "usage: postr_pipeline <imagefile>" << std::endl;
    
    Postr::ImageData image(cv::imread(argv[1]));
    
    Postr::Data data;
    data.images.push_back(image);
    data.meta["images"]["original"] = (int)data.images.size()-1;
    
    Postr::BgSegmentWorker bgsegmentworker;
    int status = bgsegmentworker.processBlocking(data);
    
    Postr::ImageData edges(data.images[data.meta["images"]["edges"].asInt()]);
    imshow("detected edges", edges);
    cv::waitKey(0);
    
    Postr::ImageData contours(data.images[data.meta["images"]["contours"].asInt()]);
    imshow("contours", contours);
    cv::waitKey(0);
    
    Postr::ImageData best(data.images[data.meta["images"]["best"].asInt()]);
    imshow("result", best);
    cv::waitKey(0);
    
    std::cout << data.serialize(false);
    
    return status;
}
