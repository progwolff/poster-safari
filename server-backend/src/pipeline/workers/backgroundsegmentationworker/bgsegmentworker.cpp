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

#include <iostream>
#include <opencv2/highgui.hpp>

#undef LOG
#define LOG(LEVEL) (CLOG(LEVEL, ELPP_CURR_FILE_LOGGER_ID) << "[" << m_name << "] ")

//partly based on http://blog.ayoungprogrammer.com/2013/04/tutorial-detecting-multiple-rectangles.html/

namespace Postr 
{
    _WORKER_CLASS_::_WORKER_CLASS_()
        : AsyncWorker(_WORKER_NAME_)
    {
    }
    
    _WORKER_CLASS_::~_WORKER_CLASS_()
    {
    }
    
    void _WORKER_CLASS_::normdir(cv::Vec2f& dir)
    {
        dir /= cv::norm(dir);
        if(dir[0] < 0 || (dir[0] == 0 && dir[1] < 0))
            dir *= -1;
    }
    
    bool _WORKER_CLASS_::direquals(const cv::Vec2f& dir1, const cv::Vec2f& dir2, double dirDiff)
    {
        return (std::abs(dir1[0] - dir2[0]) < dirDiff && std::abs(dir1[1] - dir2[1]) < dirDiff);
    }
    
    double _WORKER_CLASS_::squaredist(const cv::Point2f& a, const cv::Point2f& b)
    {
        return (a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y);
    }
    
    cv::Point2f _WORKER_CLASS_::mirror(const cv::Point2f& a, const cv::Point2f& b, const cv::Point2f& c)
    {
        cv::Point2f center = a+(b-a)/2;
        cv::Point2f dist = center - c;
        return center + dist;
    }
    
    cv::Point2f _WORKER_CLASS_::findIntersectionPointNear(const cv::Point2f& p, const std::vector<cv::Vec4i>& lines)
    {
        cv::Point2f bestpoint;
        double bestdist = std::numeric_limits<double>::max();
        for(const cv::Vec4i& a : lines)
        {
            for(const cv::Vec4i& b : lines)
            {
                int x1 = a[0], y1 = a[1], x2 = a[2], y2 = a[3];  
                int x3 = b[0], y3 = b[1], x4 = b[2], y4 = b[3];  
                cv::Vec2f dir1 = {(float)x2-x1,(float)y2-y1};
                cv::Vec2f dir2 = {(float)x4-x3,(float)y4-y3};
                normdir(dir1);
                normdir(dir2);
                if(direquals(dir1,dir2,.5))
                {
                    //don't compute an intersection if the lines have the same direction
                    //(we want to detect right angles)
                    continue;
                }
                if (float d = ((float)(x1-x2) * (y3-y4)) - ((y1-y2) * (x3-x4)))
                {  
                    cv::Point2f pt;  
                    pt.x = ((x1*y2 - y1*x2) * (x3-x4) - (x1-x2) * (x3*y4 - y3*x4)) / d;  
                    pt.y = ((x1*y2 - y1*x2) * (y3-y4) - (y1-y2) * (x3*y4 - y3*x4)) / d;
                    
                    double dist;
                    if((dist = squaredist(pt, p)) < bestdist)
                    {
                        bestpoint = pt;
                        bestdist = dist;
                    }
                }
            }
        }
        
        return bestpoint;
    }
    
    void _WORKER_CLASS_::processAsync(Data& data)
    {
        
        ImageData src;
        
        if(data.bestImage() < 0)
        {
            m_status = 1;
            LOG(ERROR) << "there is no image to segment";                    
        }
        
        config("filename");
        
        cv::Mat dst, src_gray, detected_edges;
        int lowThreshold = config("backgroundsegmentation_lowThresh", 120, "low threshold value used by the Canny filter");
        int highThreshold = config("backgroundsegmentation_highThresh", 400, "high threshold value used by the Canny filter");
        
        int maxintersections = config("backgroundsegmentation_maxIntersections", 300, "maximum number of intersection points");
        
        double dirDiff = config("backgroundsegmentation_dirDiff", .08, "allowed difference in direction of a detected line to a existing line");
        
        double maxDist = config("backgroundsegmentation_maxDist", 0.01, "allowed distance that intersection points outside the image may have from the borders of the image, relative to image size");
        
        double minSize = config("backgroundsegmentation_minSize", .2, "minimum size of the detected area, relative to image size");
        double maxSize = config("backgroundsegmentation_maxSize", 1., "maximum size of the detected area, relative to image size");
        
        // Define the destination image  
        cv::Mat quad;
        
        if(0 == m_status && !m_cancel)
        {
            m_status = 1;
        
            int kernel_size = config("backgroundsegmentation_kernelsize", 3, "kernel size used by the Canny filter");
            int maxdim;
            
            int maxruns = config("backgroundsegmentation_maxRuns", 50, "maximum number of runs");
            int runs = 0;
            
            int maxblur = 410;
            int minblur = 10;
            int blurdecr = 20;
            for(int blurratio = maxblur; blurratio >= minblur && runs < maxruns && !m_cancel && m_status; blurratio -= blurdecr)
            {
                ++runs;
                m_status = 0;
                
                if(0 == m_status && !m_cancel)
                {
                    int bestindex = data.bestImage();
                    src = data.images[bestindex].clone();
                    maxdim = std::max(src.cols,src.rows);
                    if(maxdim > 800)
                        cv::resize(src, src, cv::Size(), 800./maxdim, 800./maxdim, cv::INTER_AREA);
                    quad = src;
                    
                    maxdim = std::max(src.cols,src.rows);
                    
                    // Create a matrix of the same type and size as src (for dst)
                    dst.create(src.size(), src.type());
                    
                    // Convert the image to grayscale
                    cvtColor(src, src_gray, CV_BGR2GRAY); 

                    cv::equalizeHist(src_gray, src_gray);
                    
                    if(blurratio < 400)
                    {
                        cv::blur(src_gray, src_gray, cv::Size(maxdim/blurratio,maxdim/blurratio));
                    }
                    
                }
                
                //filter edges
                if(0 == m_status && !m_cancel)
                {
                    detected_edges = cv::Mat::zeros(src.rows, src.cols, CV_8UC1);
                    
                    int low = lowThreshold;
                    int high = highThreshold;
                    for(; high > low && countNonZero(detected_edges) < .02*(detected_edges.cols*detected_edges.rows); high -= (highThreshold-lowThreshold)/6)
                    {
                        Canny(src_gray, detected_edges, low, high, kernel_size, false); 
                        LOG(DEBUG) << "non zero pixels: " << countNonZero(detected_edges) << " / " << (detected_edges.cols*detected_edges.rows);
                    } 
            
                    cvtColor(src_gray, src_gray, CV_GRAY2BGR);
                }
                
                std::vector<cv::Vec4i> lines;
                std::vector<cv::Vec2f> dirs;
                    
                //compute hough lines
                if(0 == m_status && !m_cancel)
                {
                    HoughLinesP(detected_edges, lines, 1, CV_PI/180, 80, maxdim/12, maxdim/8);
                    
                    /*std::vector<cv::Vec2f> lines2;
                    HoughLines(detected_edges, lines2, 1, CV_PI/180, 130, 0, 0);

                    for( size_t i = 0; i < lines2.size(); i++ )
                    {
                        float rho = lines2[i][0], theta = lines2[i][1];
                        cv::Point pt1, pt2;
                        double a = cos(theta), b = sin(theta);
                        double x0 = a*rho, y0 = b*rho;
                        pt1.x = cvRound(x0 + 1000*(-b));
                        pt1.y = cvRound(y0 + 1000*(a));
                        pt2.x = cvRound(x0 - 1000*(-b));
                        pt2.y = cvRound(y0 - 1000*(a));
                        lines.push_back({pt1.x,pt1.y,pt2.x,pt2.y});
                    }*/

                    for(size_t i=0; i < lines.size(); i++)
                    {
                        cv::Vec4i l = lines[i];
                        cv::Vec2f dir = {(float)l[2]-l[0],(float)l[3]-l[1]};
                        dir /= cv::norm(dir);
                        if(dir[0] < 0 || (dir[0] == 0 && dir[1] < 0))
                            dir *= -1;
                        
                        float diff = std::abs(dir[0]) - std::abs(dir[1]);
                        //delete lines that have an angle of approx. +/- 45 degrees
                        if(diff < 0.01 && diff > -0.01)
                        {
                            lines.erase(lines.begin()+i);
                            --i;
                            continue;
                        }
                        else
                        {
                            dirs.push_back(dir);
                            line(src_gray, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0,0,255), 3, CV_AA);
                        }
                    }
                }
                
                std::vector< std::vector<cv::Point2f> > corners;  
                std::vector<cv::Point2f> intersectionpoints;
                
                //find line intersections
                if(0 == m_status && !m_cancel)
                {                        
                    for (int i=0; intersectionpoints.size() < maxintersections && i < lines.size(); i++)  
                    {  
                        for (int j=i+1; intersectionpoints.size() < maxintersections && j < lines.size(); j++)  
                        {  
                            cv::Point2f pt = computeIntersect(lines[i], lines[j], maxdim/3, maxdim/12);
                            if(
                                    pt.x != -1 && pt.y != -1
                                &&  pt.x < src.cols*(1+maxDist)
                                &&  pt.x > -src.cols*(maxDist)
                                &&  pt.y < src.rows*(1+maxDist)
                                &&  pt.y > -src.rows*(maxDist)
                            )
                            {
                                intersectionpoints.push_back(pt);
                                cv::circle(src_gray, pt, 5, cv::Scalar( 255, 0, 0 ), -1, 8);
                            }
                        }  
                    }
                    
                    if(intersectionpoints.size() < maxintersections)
                    for (int i=0; intersectionpoints.size() < maxintersections && i < lines.size(); i++)  
                    {  
                        for (int j=i+1; intersectionpoints.size() < maxintersections && j < lines.size(); j++)  
                        {  
                            cv::Point2f pt = computeIntersect(lines[i], lines[j], maxdim/2, maxdim/4, true);
                            if(
                                    pt.x != -1 && pt.y != -1
                                &&  pt.x < src.cols*(1+maxDist)
                                &&  pt.x > -src.cols*(maxDist)
                                &&  pt.y < src.rows*(1+maxDist)
                                &&  pt.y > -src.rows*(maxDist)
                            )
                            {
                                intersectionpoints.push_back(pt);
                                cv::circle(src_gray, pt, 5, cv::Scalar( 255, 0, 0 ), -1, 8);
                            }
                        }  
                    }
                    
                    LOG(DEBUG) << intersectionpoints.size() << " intersection points found"; 
                    
                    if(intersectionpoints.size() < 4)
                    {
                        blurratio += 65;
                        m_status = 1;
                        LOG(DEBUG) << "too few intersection points";
                    }
                    if(intersectionpoints.size() >= maxintersections-1)
                    {
                        m_status = 1;
                        LOG(DEBUG) << "too many intersection points";
                    }
                }
                
                std::vector<cv::Point2f> hull;
                
                //find convex hull of points
                if(0 == m_status && !m_cancel)
                {
                    bool t_reasonable = false;
                    bool b_reasonable = false;
                    bool l_reasonable = false;
                    bool r_reasonable = false;
                    
                    while(intersectionpoints.size() >= 4)
                    {
                        
                        cv::convexHull(intersectionpoints, hull, true);
                        cv::Point2f tl=hull[0],tr=hull[0],bl=hull[0],br=hull[0];
                        int tli=0,tri=0,bli=0,bri=0;
                        for(int i=0; i<hull.size(); ++i)
                        {
                            float dhtl = /*sqrt*/(hull[i].x*hull[i].x + hull[i].y*hull[i].y);
                            float dhtr = /*sqrt*/((detected_edges.cols-hull[i].x)*(detected_edges.cols-hull[i].x) + hull[i].y*hull[i].y);
                            float dhbr = /*sqrt*/((detected_edges.cols-hull[i].x)*(detected_edges.cols-hull[i].x) + (detected_edges.rows-hull[i].y)*(detected_edges.rows-hull[i].y));
                            float dhbl = /*sqrt*/(hull[i].x*hull[i].x + (detected_edges.rows-hull[i].y)*(detected_edges.rows-hull[i].y));
                            
                            float dtl = (tl.x*tl.x + tl.y*tl.y);
                            float dtr = ((detected_edges.cols-tr.x)*(detected_edges.cols-tr.x) + tr.y*tr.y);
                            float dbr = ((detected_edges.cols-br.x)*(detected_edges.cols-br.x) + (detected_edges.rows-br.y)*(detected_edges.rows-br.y));
                            float dbl = (bl.x*bl.x + (detected_edges.rows-bl.y)*(detected_edges.rows-bl.y));
                            
                            if(dhtl < dtl)
                            {
                                tl = hull[i];
                                tli = i;
                            }
                            if(dhtr < dtr)
                            {
                                tr = hull[i];
                                tri = i;
                            }
                            if(dhbl < dbl)
                            {
                                bl = hull[i];
                                bli = i;
                            }
                            if(dhbr < dbr)
                            {
                                br = hull[i];
                                bri = i;
                            }
                        }
                        hull = {tl,tr,br,bl};
                        corners.push_back(hull);
                        
                        //check if hull is reasonable
                        t_reasonable = false;
                        b_reasonable = false;
                        l_reasonable = false;
                        r_reasonable = false;
                        
                        
                        auto isreasonable = [dirDiff,&dirs,&lines,&src_gray,maxdim](const cv::Vec2f& dir, cv::Point2f& a, cv::Point2f& b)
                        {
                            if(std::abs(a.x - b.x) < maxdim/100. && std::abs(a.y - b.y) < maxdim/100.)
                                return false;
                            
                            for(int j=0; j < dirs.size(); j++)
                            {
                                if(direquals(dirs[j],dir,dirDiff))
                                {
                                    cv::Vec2f linedir1 = {(float)a.x-lines[j][0],(float)a.y-lines[j][1]};
                                    cv::Vec2f linedir2 = {(float)a.x-lines[j][2],(float)a.y-lines[j][3]};
                                    double dista = sqrt(std::min(squaredist(a,{(float)lines[j][0],(float)lines[j][1]}), squaredist(a,{(float)lines[j][2],(float)lines[j][3]})));
                                    normdir(linedir1);
                                    normdir(linedir2);
                                    
                                    cv::Vec2f linedir3 = {(float)b.x-lines[j][0],(float)b.y-lines[j][1]};
                                    cv::Vec2f linedir4 = {(float)b.x-lines[j][2],(float)b.y-lines[j][3]};
                                    double distb = sqrt(std::min(squaredist(b,{(float)lines[j][0],(float)lines[j][1]}), squaredist(b,{(float)lines[j][2],(float)lines[j][3]})));
                                    normdir(linedir3);
                                    normdir(linedir4);

                                    if((direquals(linedir1, linedir2, 2*dirDiff) || dista < maxdim/12) && (direquals(linedir3, linedir4, 2*dirDiff) || distb < maxdim/12))
                                    {
                                        line(src_gray, cv::Point(lines[j][0], lines[j][1]), cv::Point(lines[j][2], lines[j][3]), cv::Scalar(255,0,0), 3, CV_AA);
                                        return true;
                                    }
                                }
                            }
                            return false;
                        };
                        
                        cv::Vec2f dir = {(float)tr.x-tl.x,(float)tr.y-tl.y};
                        normdir(dir);
                        t_reasonable = isreasonable(dir, tl, tr);
                        
                        dir = {(float)br.x-bl.x,(float)br.y-bl.y};
                        normdir(dir);
                        b_reasonable = isreasonable(dir, bl, br);
                        
                        dir = {(float)bl.x-tl.x,(float)bl.y-tl.y};
                        normdir(dir);
                        l_reasonable = isreasonable(dir, tl, bl);
                        
                        dir = {(float)br.x-tr.x,(float)br.y-tr.y};
                        normdir(dir);
                        r_reasonable = isreasonable(dir, tr, br);
                        
                        
                        LOG(DEBUG) << t_reasonable << l_reasonable << b_reasonable << r_reasonable;
                        
                        if(b_reasonable && l_reasonable && (!t_reasonable || !r_reasonable))
                        {
                            if(runs > maxruns/2.)
                            {
                                cv::Point pt = mirror(tl, br, bl);
                                cv::Point pt2 = findIntersectionPointNear(pt, lines);
                                if(squaredist(pt, pt2) < (maxdim/5)*(maxdim/5))
                                {
                                    intersectionpoints[tri] = pt2;
                                    tr = pt2;
                                    hull[1] = tr;
                                    cv::circle(src_gray, pt2, 5, cv::Scalar( 0, 255, 0 ), -1, 8);
                                }
                            }
                            else
                                intersectionpoints.erase(intersectionpoints.begin()+tri);
                        }
                        else if(t_reasonable && r_reasonable && (!l_reasonable || !b_reasonable))
                        {
                            if(runs > maxruns/2.)
                            {
                                cv::Point pt = mirror(tl, br, tr);
                                cv::Point pt2 = findIntersectionPointNear(pt, lines);
                                if(squaredist(pt, pt2) < (maxdim/5)*(maxdim/5))
                                {
                                    intersectionpoints[bli] = pt2;
                                    bl = pt2;
                                    hull[3] = bl;
                                    cv::circle(src_gray, pt2, 5, cv::Scalar( 0, 255, 0 ), -1, 8);
                                }
                            }
                            else
                                intersectionpoints.erase(intersectionpoints.begin()+bli);
                        }
                        else if(t_reasonable && l_reasonable && (!b_reasonable || !r_reasonable))
                        {
                            if(runs > maxruns/2.)
                            {
                                cv::Point pt = mirror(tr, bl, tl);
                                cv::Point pt2 = findIntersectionPointNear(pt, lines);
                                if(squaredist(pt, pt2) < (maxdim/5)*(maxdim/5))
                                {
                                    intersectionpoints[bri] = pt2;
                                    br = pt2;
                                    hull[2] = br;
                                    cv::circle(src_gray, pt2, 5, cv::Scalar( 0, 255, 0 ), -1, 8);
                                }
                            }
                            else
                                intersectionpoints.erase(intersectionpoints.begin()+bri);
                        }
                        else if(b_reasonable && r_reasonable && (!l_reasonable || !t_reasonable))
                        {
                            if(runs > maxruns/2.)
                            {
                                cv::Point pt = mirror(bl, tr, br);
                                cv::Point pt2 = findIntersectionPointNear(pt, lines);
                                if(squaredist(pt, pt2) < (maxdim/5)*(maxdim/5))
                                {
                                    intersectionpoints[tri] = pt2;
                                    tl = pt2;
                                    hull[0] = tl;
                                    cv::circle(src_gray, pt2, 5, cv::Scalar( 0, 255, 0 ), -1, 8);
                                }
                            }
                            else
                                intersectionpoints.erase(intersectionpoints.begin()+tli);
                        }
                        
                        dir = {(float)tr.x-tl.x,(float)tr.y-tl.y};
                        normdir(dir);
                        t_reasonable = isreasonable(dir, tl, tr);
                        
                        dir = {(float)br.x-bl.x,(float)br.y-bl.y};
                        normdir(dir);
                        b_reasonable = isreasonable(dir, bl, br);
                        
                        dir = {(float)bl.x-tl.x,(float)bl.y-tl.y};
                        normdir(dir);
                        l_reasonable = isreasonable(dir, tl, bl);
                        
                        dir = {(float)br.x-tr.x,(float)br.y-tr.y};
                        normdir(dir);
                        r_reasonable = isreasonable(dir, tr, br);
                        
                        if(b_reasonable && l_reasonable && (!t_reasonable || !r_reasonable))
                            intersectionpoints.erase(intersectionpoints.begin() + tri);
                        else if(t_reasonable && r_reasonable && (!l_reasonable || !b_reasonable))
                            intersectionpoints.erase(intersectionpoints.begin() + bli);
                        else if(b_reasonable && (!r_reasonable || !t_reasonable || !l_reasonable))
                            intersectionpoints.erase(intersectionpoints.begin() + tli);
                        else if(!b_reasonable)
                            intersectionpoints.erase(intersectionpoints.begin() + bri);
                        else
                            break;
                    }
                    
                    for(int i=0; i<hull.size(); ++i)
                        cv::line(src_gray, hull[i], hull[(i+1)%hull.size()], cv::Scalar(0, 255, 0));
                    
                    if(!b_reasonable || !t_reasonable || !l_reasonable || !r_reasonable)
                    {
                        m_status = 1;
                        LOG(DEBUG) << "the edges of the computed quad have no corresponding hough lines";
                    }
                
                }
                
                //imshow("newpoint", src_gray);
                //cv::waitKey(-1);
                
                //find polygon corners
                if(0 == m_status && !m_cancel)
                {   
                    cv::Point2f center(0,0);  
                    
                    if(hull.size()<4)
                    {
                        m_status = 1;  
                        LOG(DEBUG) << "too few quad corners";
                    }
                    
                    if(    (std::abs(hull[0].x - hull[1].x) < maxdim/100. && std::abs(hull[0].y - hull[1].y) < maxdim/100.)
                        || (std::abs(hull[0].x - hull[2].x) < maxdim/100. && std::abs(hull[0].y - hull[2].y) < maxdim/100.)
                        || (std::abs(hull[0].x - hull[3].x) < maxdim/100. && std::abs(hull[0].y - hull[3].y) < maxdim/100.)
                        || (std::abs(hull[1].x - hull[2].x) < maxdim/100. && std::abs(hull[1].y - hull[2].y) < maxdim/100.)
                        || (std::abs(hull[1].x - hull[3].x) < maxdim/100. && std::abs(hull[1].y - hull[3].y) < maxdim/100.)
                        || (std::abs(hull[2].x - hull[3].x) < maxdim/100. && std::abs(hull[2].y - hull[3].y) < maxdim/100.)
                    )
                    {
                        m_status = 1;
                        LOG(DEBUG) << "quad corners overlap";
                    }
                    
                    for(int j=0;j<hull.size();j++)
                    {  
                        center += hull[j];  
                    }  
                    center *= (1. / hull.size());  
                    sortCorners(hull, center);  
                }
                
                progress(progress()+100./maxruns);
                
                //extract the rectangle
                if(0 == m_status && !m_cancel)
                {
                    cv::Rect r = boundingRect(hull);  
                    double area = cv::contourArea(hull);
                    if(area < minSize*src.size().area())
                    {
                        LOG(DEBUG) << "bounding rect is too small (" << area/(double)src.size().area() << ")";
                        m_status = 1;
                    }
                    else if(area > maxSize*src.size().area())
                    {
                        LOG(DEBUG) << "bounding rect is too large (" << area/(double)src.size().area() << ")";
                        m_status = 1;
                        runs = maxruns;
                    }
                    else
                    {
                        cv::Mat original = data.images[data.bestImage()];
                        maxdim = std::max(original.cols,original.rows);
                        
                        //TODO: compute the proportions of the original rectangle: http://stackoverflow.com/questions/1194352/proportions-of-a-perspective-deformed-rectangle
                        quad = cv::Mat::zeros((maxdim/800.)*r.height, (maxdim/800.)*r.width, CV_8UC3);
                        
                        // Corners of the destination image  
                        std::vector<cv::Point2f> quad_pts;  
                        quad_pts.push_back(cv::Point2f(0, 0));  
                        quad_pts.push_back(cv::Point2f(quad.cols, 0));  
                        quad_pts.push_back(cv::Point2f(quad.cols, quad.rows));  
                        quad_pts.push_back(cv::Point2f(0, quad.rows));  
                        // Get transformation matrix  
                        if(maxdim > 800)
                        {
                            hull[0] *= (maxdim/800.);
                            hull[1] *= (maxdim/800.);
                            hull[2] *= (maxdim/800.);
                            hull[3] *= (maxdim/800.);
                        }
                        cv::Mat transmtx = cv::getPerspectiveTransform(hull, quad_pts);  
                        // Apply perspective transformation  
                        cv::warpPerspective(original, quad, transmtx, quad.size()); 
                    }
                }
            }
            //LOG(DEBUG) << "terminate after " << runs << " runs";
        }
        
        if(!m_cancel)
        {
            data.images.push_back(detected_edges);
            data.meta["images"]["edges"] = (int)data.images.size()-1;
            
            data.images.push_back(src_gray);
            data.meta["images"]["contours"] = (int)data.images.size()-1;
        }
        
        if(0 == m_status && !m_cancel)
        {                
            data.images.push_back(quad);
            data.meta["images"]["best"] = (int)data.images.size()-1;
            
            progress(100);
        }
        
    }
    
    cv::Point2f _WORKER_CLASS_::computeIntersect(cv::Vec4i a, cv::Vec4i b, int thresh, int thresh2, bool findLongDistance)  
    {  
        int x1 = a[0], y1 = a[1], x2 = a[2], y2 = a[3];  
        int x3 = b[0], y3 = b[1], x4 = b[2], y4 = b[3];  
        cv::Vec2f dir1 = {(float)x2-x1,(float)y2-y1};
        cv::Vec2f dir2 = {(float)x4-x3,(float)y4-y3};
        normdir(dir1);
        normdir(dir2);
        if(direquals(dir1,dir2,.5))
        {
            //don't compute an intersection if the lines have the same direction
            //(we want to detect right angles)
            return cv::Point2f(-1,-1);
        }
        if (float d = ((float)(x1-x2) * (y3-y4)) - ((y1-y2) * (x3-x4)))  
        {  
            cv::Point2f pt;  
            pt.x = ((x1*y2 - y1*x2) * (x3-x4) - (x1-x2) * (x3*y4 - y3*x4)) / d;  
            pt.y = ((x1*y2 - y1*x2) * (y3-y4) - (y1-y2) * (x3*y4 - y3*x4)) / d;  
            
            //-10 is a threshold, the POI can be off by at most 10 pixels
            if(pt.x < std::min(x1,x2)-thresh || pt.x > std::max(x1,x2)+thresh || pt.y < std::min(y1,y2)-thresh || pt.y > std::max(y1,y2)+thresh)
            {
                return cv::Point2f(-1,-1);  
            }  
            if(pt.x < std::min(x3,x4)-thresh || pt.x > std::max(x3,x4)+thresh || pt.y < std::min(y3,y4)-thresh || pt.y > std::max(y3,y4)+thresh)
            {
                return cv::Point2f(-1,-1);  
            }  
            
            //compute distances of the intesection point to the line end points.
            //we only want intersections at line ends
            int d1 = sqrt((x1-pt.x)*(x1-pt.x)+(y1-pt.y)*(y1-pt.y));
            int d2 = sqrt((x2-pt.x)*(x2-pt.x)+(y2-pt.y)*(y2-pt.y));
            int d3 = sqrt((x3-pt.x)*(x3-pt.x)+(y3-pt.y)*(y3-pt.y));
            int d4 = sqrt((x4-pt.x)*(x4-pt.x)+(y4-pt.y)*(y4-pt.y));
            
            if(((d1 > thresh2 && d2 > thresh2) || (d3 > thresh2 && d4 > thresh2)) && !findLongDistance)
                return cv::Point2f(-1,-1);
            
            if(((d1 > thresh2 && d2 > thresh2) || (d3 > thresh2 && d4 > thresh2)) && !((d1 > thresh2 && d2 > thresh2) && (d3 > thresh2 && d4 > thresh2)) && findLongDistance)
                return cv::Point2f(-1,-1);
            
            return pt;  
        }
        else  
            return cv::Point2f(-1, -1);  
    }
    
    bool _WORKER_CLASS_::compare(cv::Point2f a, cv::Point2f b)
    {
        return a.x < b.x;  
    }  
 
    void _WORKER_CLASS_::sortCorners(std::vector<cv::Point2f>& corners, cv::Point2f center)  
    {  
        std::vector<cv::Point2f> top, bot;  
        for (int i = 0; i < corners.size(); i++)  
        {  
            if (corners[i].y < center.y)  
            top.push_back(corners[i]);  
            else  
            bot.push_back(corners[i]);  
        }  
        
        std::sort(top.begin(),top.end(),compare);  
        std::sort(bot.begin(),bot.end(),compare);  
        cv::Point2f tl = top[0];
        cv::Point2f tr = top[top.size()-1];
        cv::Point2f bl = bot[0];
        cv::Point2f br = bot[bot.size()-1];  
        corners.clear();  
        corners.push_back(tl);  
        corners.push_back(tr);  
        corners.push_back(br);  
        corners.push_back(bl);  
    }  
        

}
