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

#ifndef BGSEGMENTWORKER_H
#define BGSEGMENTWORKER_H

#include "asyncworker.h"

#include <thread>
#include <atomic>

#ifdef _WORKER_NAME_
#undef _WORKER_NAME_
#endif
#ifdef _WORKER_CLASS_
#undef _WORKER_CLASS_
#endif

#define _WORKER_NAME_ "Background Segmentation" 
#define _WORKER_CLASS_ BgSegmentWorker

namespace Postr 
{
    /**
     * @brief This Worker tries to segment a poster from the image background
     */
    class _WORKER_CLASS_ final : public AsyncWorker
    {
    public:
        _WORKER_CLASS_();
        ~_WORKER_CLASS_();
    
    private:        
        void processAsync(Data& data) override;
        /**
         * @brief compute the intersection point of two lines.
         * @param a one line 
         * @param b another line 
         * @param thresh the intersection point may be thresh pixels away from a line
         * @param thresh2 ignore intersection points that are more than thresh2 pixels away from an end point of one of the lines
         * @param longDistance find intersection points on extended lines
         * @return the intersection point or (-1,-1) if none was found.
         */
        static cv::Point2f computeIntersect(cv::Vec4i a, cv::Vec4i b, int thresh = 10, int thresh2 = 5, bool longDistance = false);
        /**
         * @brief sort corners of a shape
         * @param corners the corners to sort
         * @param center the center of the shape
         */ 
        static void sortCorners(std::vector<cv::Point2f>& corners, cv::Point2f center);
        /**
         * @brief compare two points by position
         * @param a one point 
         * @param b another point
         * @return true if a.x < b.x, false else
         */
        static bool compare(cv::Point2f a,cv::Point2f b);
        /**
         * @brief normalize a direction vector
         * @param dir a direction vector
         */
        static void normdir(cv::Vec2f& dir);
        /**
         * @brief compare two direction vectors 
         * @param dir1 one direction vector
         * @param dir2 another direction vector
         * @param dirDiff maximum difference between the directions
         * @return true if the directions match, false else
         */
        static bool direquals(const cv::Vec2f& dir1, const cv::Vec2f& dir2, double dirDiff);
        /**
         * @brief compute the squared distance of two points
         * @param a one point 
         * @param b another point 
         * @return the squared distance between a and b 
         */
        static double squaredist(const cv::Point2f& a, const cv::Point2f& b);
        /**
         * compute the mirror point of c along the line defined by a and b
         * @param a one point on the line
         * @param b another point on the line 
         * @param c point to mirror
         * @return mirrored point
         */
        static cv::Point2f mirror(const cv::Point2f& a, const cv::Point2f& b, const cv::Point2f& c);
        /**
         * find an intersection point of given lines near a point
         * @param p point to find the nearest intersection point for
         * @param lines lines that intersect 
         * @return the intersection point that is nearest to p
         */
        static cv::Point2f findIntersectionPointNear(const cv::Point2f& p, const std::vector<cv::Vec4i>& lines);
    };
};

#include "workerexport.h"

#endif //BGSEGMENTWORKER_H
