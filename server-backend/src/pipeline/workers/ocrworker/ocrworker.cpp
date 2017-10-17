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

#include "ocrworker.h"
#include "util.h"
#include <iostream>
#include <string>
#include <tesseract/publictypes.h>
#include <opencv2/highgui.hpp>
#include <opencv2/photo.hpp>

#undef LOG
#define LOG(LEVEL) (CLOG(LEVEL, ELPP_CURR_FILE_LOGGER_ID) << "[" << m_name << "] ")

namespace Postr 
{
    
    _WORKER_CLASS_::_WORKER_CLASS_()
        : AsyncWorker(_WORKER_NAME_)
    {
        
        /*std::vector<std::string> imagenames = File::locateAll(".*tmp", {"/home/wolff/repos/fallstudie/postr-processing/src/pipeline/workers/spellcorrect/glyphs/images/"});
        
        std::ofstream out("imagenamecosts.txt", std::ofstream::out);
        
        std::map<std::string,ImageData> images;
        for(const std::string& a : imagenames)
        {
            LOG(INFO) << "reading image " << a;
            images[a] = cv::imread(a, cv::IMREAD_GRAYSCALE);
            cv::resize(images[a], images[a], cv::Size(20,20), 0, 0, cv::INTER_AREA);
        }
        
        std::map<std::string,std::map<std::string,float>> diffpixels;
        float maxdiffpixels = 0;
        for(const std::string& a : imagenames)
        {
            std::string alower = File::fileName(a);
            alower = alower.substr(0, alower.length()-8);
            std::transform(alower.begin(), alower.end(), alower.begin(), ::tolower);
            
            for(const std::string& b : imagenames)
            {
                std::string blower = File::fileName(b);
                blower = blower.substr(0, blower.length()-8);
                std::transform(blower.begin(), blower.end(), blower.begin(), ::tolower);
                
                float nonzero = countNonZero(images[a] ^ images[b]);
                if(diffpixels.find(alower) == diffpixels.end() || diffpixels[alower].find(blower) == diffpixels[alower].end() || diffpixels[alower][blower] > nonzero)
                {
                    diffpixels[alower][blower] = nonzero;
                    LOG(INFO)  << alower << "|" << blower << "|" << nonzero / maxdiffpixels << " (" << nonzero << ")";
                }
                if(diffpixels[alower][blower] > maxdiffpixels)
                    maxdiffpixels = diffpixels[alower][blower];
            }
        }
        
        
        for(auto a : diffpixels)
        {
            for(auto b : a.second)
            {
                out << a.first << "|" << b.first << "|" << b.second / maxdiffpixels << std::endl;
            }
        }   
        
        out.flush();
        */
        
        #define HAVE_TESSERACT //TODO: find a way to determine if OpenCV was compiled against Tesseract
#ifndef HAVE_TESSERACT
        LOG(ERROR) << "Tesseract was found, but OpenCV is not compiled against Tesseract. Recompile OpenCV.";
#endif
        
        LOG(DEBUG) << "writing ocr_whitelist to config";
        std::string validChars = config("OCR_whitelist", "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZäöüßÄÖÜ!\"%&/()=?{[]}\\-_#+*,;.:€$@ ", "string including all characters that should be detected by OCR");
        std::string lang = config("OCR_language", "deu+eng", "languages used by OCR");
        
        std::vector<std::string> dirs = File::DataLocation;
        std::string customdatapath = File::directory(File::locate("deu.traineddata", dirs, -1));
        const char* datapath = configBool("OCR_useSystemTessData", false, "use system provided tessdata instead of custom ones for Poster Safari.")?NULL:customdatapath.c_str();
        
        api = cv::text::OCRTesseract::create(
            datapath, 
            lang.c_str(), 
            validChars.c_str(), 
            tesseract::OEM_DEFAULT,
            tesseract::PSM_SINGLE_LINE);

        fullpageapi = cv::text::OCRTesseract::create(
            datapath, 
            lang.c_str(), 
            validChars.c_str(), 
            tesseract::OEM_DEFAULT,
            tesseract::PSM_SPARSE_TEXT);
        
        try 
        {
            // Create ERFilter objects with the 1st and 2nd stage default classifiers
            std::vector<std::string> dirs = File::DataLocation;
            dirs.push_back("..");
            const std::string& classifierfile1 = File::locate("trained_classifierNM1.xml", dirs, -1);
            const std::string& classifierfile2 = File::locate("trained_classifierNM2.xml", dirs, -1);
            LOG(DEBUG) << "Using trained classifier " << classifierfile1;
            LOG(DEBUG) << "Using trained classifier " << classifierfile2;
            er_filter1 = cv::text::createERFilterNM1(cv::text::loadClassifierNM1(classifierfile1),6,0.0001f,0.6f,0.2f,true,0.1f);
            er_filter2 = cv::text::createERFilterNM2(cv::text::loadClassifierNM2(classifierfile2),0.3);
        } 
        catch(cv::Exception& e)
        {
            m_status = 1;
            LOG(ERROR) << "Files trained_classifierNM1.xml and trained_classifierNM2.xml are missing. Copy them to the current working directory.";
        }
    }
    
    _WORKER_CLASS_::~_WORKER_CLASS_()
    {
        m_cancel = true;
        while(progress());
        
        if(er_filter1)
            er_filter1.release();
        if(er_filter2)
            er_filter2.release();
    }
    
    void _WORKER_CLASS_::initAsync()
    {
    }
    
    void _WORKER_CLASS_::processAsync(Data& data)
    {
        ImageData src;
        ImageData textImage;
        
        if(data.bestImage() < 0)
        {
            m_status = 1;
            LOG(ERROR) << "there is no image to recognize characters from";
        }
        
        std::vector<cv::Mat> channels;
        
        if(0 == m_status && !m_cancel)
        {
            src = data.images[data.bestImage()].clone();
            
            bool forceScale = !configBool("ocr_onlyResizeIfSmaller", false, "Resize image only if it is smaller than DIN A4 at 300dpi.");
            if(configBool("ocr_resizeTo300dpi", true, "Resize each image to at least DIN A4 at 300dpi (keeping aspect ratio) for OCR. Might improve OCR quality."))
            {
                if(src.cols > src.rows && (src.cols < 3508 || forceScale))
                    cv::resize(src, src, cv::Size(), 3508./(double)src.cols, 3508./(double)src.cols, cv::INTER_AREA);
                else if(src.cols < 2480 || forceScale)
                    cv::resize(src, src, cv::Size(), 2480./(double)src.cols, 2480./(double)src.cols, cv::INTER_AREA);
            }
            
            if(debug)
                textImage = src.clone();
        
            int maxdim = std::max(src.cols,src.rows);
            
            if(configBool("ocr_denoise", false, "Denoise image. This is a very resource intensive step."))
                cv::fastNlMeansDenoising(src, src, 10, 7, 27);
            
            if(false && debug)
            {
                cv::Mat denoised = src.clone();
                cv::resize(denoised, denoised, cv::Size(), 800./maxdim, 800./maxdim, cv::INTER_AREA);
                imshow("denoised", denoised);
                cv::waitKey(-1);
            }
            
            // Extract channels to be processed individually
            cv::text::computeNMChannels(src, channels, cv::text::ERFILTER_NM_RGBLGrad); //cv::text::ERFILTER_NM_IHSGrad

            int cn = (int)channels.size();
            // Append negative channels to detect ER- (bright regions over dark background)
            for (int c = 0; c < cn-1; c++)
                channels.push_back(255-channels[c]);
            
            progress(progress()+1);
        }
        
        std::vector< std::vector<cv::text::ERStat> > regions(channels.size());
        
        if(0 == m_status && !m_cancel)
        {                
            // Apply the default cascade classifier to each independent channel (could be done in parallel)
            for (int c=0; c<(int)channels.size(); c++)
            {
                er_filter1->run(channels[c], regions[c]);
                er_filter2->run(channels[c], regions[c]);
                progress(progress()+1);
            }
        }
        
        if(0 == m_status && !m_cancel)
        {
            if(!er_filter1 || !er_filter2)
            {
                LOG(ERROR) << "ER filters are not initialized";
                m_status = 1;
            }
        }
        
        std::vector< std::vector<cv::Vec2i> > region_groups;
        std::vector<cv::Rect> groups_boxes;
            
        if(0 == m_status && !m_cancel)
        {
            // Detect character groups
            cv::text::erGrouping(src, channels, regions, region_groups, groups_boxes, cv::text::ERGROUPING_ORIENTATION_HORIZ);
        
            progress(progress()+5);
        }
        
        std::string textLineOutputDir = config("OCR_saveTextLinesTo", "", "If set to a directory, extracted text lines will be saved as images.");
        String::replaceAll(textLineOutputDir, "~", File::homeDirectory());
        if(!textLineOutputDir.empty())
        {
            if(File::isDirectory(textLineOutputDir))
                LOG(INFO) << "extracted text lines will be saved to " << textLineOutputDir;
            else
            {
                LOG(WARNING) << "extracted text lines will not be saved to " << textLineOutputDir << ", as this is not an existing directory";
                textLineOutputDir = "";
            }
        }
        bool analyzeOriginalImage = configBool("OCR_readOriginalImage", false, "Analyze not only ER filtered words, but also the original image");
        
        if(0 == m_status && !m_cancel)
        {
            float progincr = (100-progress())/groups_boxes.size();
            
            for (int i=0; i < groups_boxes.size() && !m_cancel; i++)
            {
                if(debug)
                    rectangle(textImage, groups_boxes[i], cv::Scalar(0, 255, 255));
                
                cv::Mat group_img = cv::Mat::zeros(src.rows+2, src.cols+2, CV_8UC1);
                cv::Mat group_img_orig = cv::Mat::zeros(src.rows+2, src.cols+2, src.type());
    
                er_draw(channels, regions, region_groups[i], group_img);
                cv::Mat group_segmentation;
                group_img.copyTo(group_segmentation);
                
                copyMakeBorder(src, group_img_orig, 1, 1, 1, 1, cv::BORDER_CONSTANT, 0);
                
                cv::Mat group_img_scaled;
                
                if(debug)
                {
                    group_img_scaled = group_img_orig(groups_boxes[i]);
                    int maxdim = std::max(src.cols,src.rows);
                    cv::resize(group_img_scaled, group_img_scaled, cv::Size(), 800./maxdim, 800./maxdim, cv::INTER_AREA);
                }
                
                group_img(groups_boxes[i]).copyTo(group_img);
                copyMakeBorder(group_img,group_img,15,15,15,15,cv::BORDER_CONSTANT,cv::Scalar(0));
                
                std::vector<cv::Rect>   boxes;
                std::vector<std::string> words;
                std::vector<float>  confidences;
                std::string output;
                api->run(group_img, output, &boxes, &words, &confidences, cv::text::OCR_LEVEL_TEXTLINE);
                output.erase(remove(output.begin(), output.end(), '\n'), output.end());
                
                std::vector<cv::Rect>   boxes_orig;
                std::vector<std::string> words_orig;
                std::vector<float>  confidences_orig;
                std::string output_orig;
                
                if(groups_boxes.size() < 5)
                    analyzeOriginalImage = true;
                
                if(analyzeOriginalImage)
                {
                    fullpageapi->run(group_img_orig, output_orig, &boxes_orig, &words_orig, &confidences_orig, cv::text::OCR_LEVEL_TEXTLINE);
                    output_orig.erase(remove(output_orig.begin(), output_orig.end(), '\n'), output_orig.end());
                }
                
                if(false && debug)
                {
                    imshow("group "+std::to_string(i)+": "+output,group_img_scaled);
                }
                
                if(!textLineOutputDir.empty())
                {
                    static long long textLineCount=0;
                    while(File::exists(textLineOutputDir+"/"+std::to_string(textLineCount)+".tiff"))
                        ++textLineCount;
                    std::vector<int> compression_params;
                    compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
                    compression_params.push_back(9);
                    cv::imwrite(textLineOutputDir+"/"+std::to_string(textLineCount)+".tiff", group_img, compression_params);
                }
                    
                if(progress()+progincr < 100)
                    progress(progress()+progincr);
            
                if (output.size() < 3)
                    continue;

                for (int j=0; j < boxes.size() && !m_cancel; j++)
                {
                    boxes[j].x += groups_boxes[i].x-15;
                    boxes[j].y += groups_boxes[i].y-15;
                    //cout << "  word = " << words[j] << "\t confidence = " << confidences[j] << endl;
                    if ((words[j].size() < 2) || (confidences[j] < 51) ||
                            ((words[j].size()==2) && (words[j][0] == words[j][1])) ||
                            ((words[j].size()< 4) && (confidences[j] < 60)) ||
                            isRepetitive(words[j]))
                        continue;
                    
                    int index = data.meta["text"].size();
                    words[j].erase(remove(words[j].begin(), words[j].end(), '\n'), words[j].end());
                    data.meta["text"][index]["text"] = words[j];
                    data.meta["text"][index]["confidence"] = confidences[j];
                    data.meta["text"][index]["x"] = boxes[j].x;
                    data.meta["text"][index]["y"] = boxes[j].y;
                    data.meta["text"][index]["width"] = boxes[j].width;
                    data.meta["text"][index]["height"] = boxes[j].height;
                    if(debug)
                        rectangle(textImage, boxes[j], cv::Scalar(255, 0, 0));
                }
                
                if(analyzeOriginalImage)
                {
                    for (int j=0; j < boxes_orig.size() && !m_cancel; j++)
                    {
                        boxes_orig[j].x += groups_boxes[i].x-15;
                        boxes_orig[j].y += groups_boxes[i].y-15;
                        //cout << "  word = " << words_orig[j] << "\t confidence = " << confidences_orig[j] << endl;
                        if ((words_orig[j].size() < 2) || (confidences_orig[j] < 51) ||
                                ((words_orig[j].size()==2) && (words_orig[j][0] == words_orig[j][1])) ||
                                ((words_orig[j].size()< 4) && (confidences_orig[j] < 60)) ||
                                isRepetitive(words_orig[j]))
                            continue;
                        
                        int index = data.meta["text"].size();
                        words_orig[j].erase(remove(words_orig[j].begin(), words_orig[j].end(), '\n'), words_orig[j].end());
                        data.meta["text"][index]["text"] = words_orig[j];
                        data.meta["text"][index]["confidence"] = confidences_orig[j];
                        data.meta["text"][index]["x"] = boxes_orig[j].x;
                        data.meta["text"][index]["y"] = boxes_orig[j].y;
                        data.meta["text"][index]["width"] = boxes_orig[j].width;
                        data.meta["text"][index]["height"] = boxes_orig[j].height;
                        if(debug)
                            rectangle(textImage, boxes_orig[j], cv::Scalar(255, 0, 0));
                    }
                }
            }
            
            if(!m_cancel && debug)
            {
                int maxdim = std::max(textImage.rows,textImage.cols);
                if(maxdim > 800)
                    cv::resize(textImage, textImage, cv::Size(), 800./maxdim, 800./maxdim, cv::INTER_AREA);
                data.images.push_back(textImage);
                data.meta["images"]["text"] = (int)data.images.size()-1;
            }
        }
        
        if(0 == m_status && !m_cancel)
        {
            progress(100);
        }
        
        //LOG(DEBUG) << "terminate";
    
        //cleanup
        regions.clear();
        if (!groups_boxes.empty())
        {
            groups_boxes.clear();
        }
        
    }
    
    void _WORKER_CLASS_::er_draw(std::vector<cv::Mat> &channels, std::vector< std::vector<cv::text::ERStat> > &regions, std::vector<cv::Vec2i> group, cv::Mat& segmentation)
    {
        for (int r=0; r<(int)group.size(); r++)
        {
            cv::text::ERStat er = regions[group[r][0]][group[r][1]];
            if (er.parent != NULL) // deprecate the root region
            {
                int newMaskVal = 255;
                int flags = 4 + (newMaskVal << 8) + cv::FLOODFILL_FIXED_RANGE + cv::FLOODFILL_MASK_ONLY;
                floodFill(channels[group[r][0]],segmentation,cv::Point(er.pixel%channels[group[r][0]].cols,er.pixel/channels[group[r][0]].cols),
                        cv::Scalar(255),0,cv::Scalar(er.level),cv::Scalar(0),flags);
            }
        }
    }
    
    bool _WORKER_CLASS_::isRepetitive(const std::string& s)
    {
        int count = 0;
        for (int i=0; i<(int)s.size(); i++)
        {
            if ((s[i] == 'i') ||
                    (s[i] == 'l') ||
                    (s[i] == 'I'))
                count++;
        }
        if (count > ((int)s.size()+1)/2)
        {
            return true;
        }
        return false;
    }

}
