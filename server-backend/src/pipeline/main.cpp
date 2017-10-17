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

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include "ocrworker.h"
#include "bgsegmentworker.h"
#include "workerloader.h"
#include "textgroupcollateworker.h"
#include "regexworker.h"
#include "wordsplitworker.h"
#include "spellcorrectworker.h"
#include "votingtextmergeworker.h"
#include "naivesemanticanalysis.h"

#include "couchdbstream.h"

#include <iostream>
#include <thread>
#include <chrono>

#include "argagg.hpp"

#include "util.h"

//TODO: define database credentials
#define DATABASE_URL todo.todo.org
#define DATABASE_PORT 1234
#define DATABASE_USER username 
#define DATABASE_PASSWORD 0000


int main(int argc, char **argv)
{
    argagg::parser argparser {{
        { "help", {"-h", "--help"},
        "shows this help message", 0},
        { "debug", {"-d", "--debug"},
        "produce debug output (like intermediate result images)", 0},
        { "debug-db", {"-b", "--debug-db"},
        "use debug tables in database", 0},
        { "dry-run", {"-n", "--dry-run"},
        "process and print result, but don't send the result back to database", 0},
        { "quiet", {"-q", "--quiet"},
        "hide progress", 0},
        { "verbose", {"-v", "--verbose"},
        "log to stdout (set to a value in range 1 (FATAL) to 6 (DEBUG) to specify the log level)", 1},
    }};
    
    argagg::parser_results args;
    try {
        args = argparser.parse(argc, argv);
    } catch (const std::exception& e) {
        LOG(ERROR) << e.what();
        return 1;
    }
        
    if (args["help"]) 
    {
        std::cout << "Usage: postr-processing [options] [images or IDs...]" << std::endl << argparser;
        return 0;
    }

    bool debug = false;
    if(args["debug"])
        debug = true;
    
    bool debugDB = false;
    if(args["debug-db"])
        debugDB = true;
    
    bool dryrun = false;
    if(args["dry-run"])
        dryrun = true;
    
    bool interactive = true;
    if(args["quiet"])
        interactive = false;
    
    int loglevel = 3;
    if(args["verbose"])
        loglevel = args["verbose"];
    
    Postr::Worker::initializeLog(loglevel);
    
    Postr::CouchDBStream datastream(DATABASE_URL, DATABASE_PORT, DATABASE_USER, DATABASE_PASSWORD, debugDB, dryrun);
    
    Postr::BgSegmentWorker bgsegmentworker;
    Postr::OCRWorker ocrworker,ocrworker2;
    Postr::TextGroupCollateWorker textgroupworker,textgroupworker2;
    Postr::RegexWorker regexworker;
    Postr::WordSplitWorker wordsplitworker;
    Postr::SpellCorrectWorker spellcorrectworker;
    Postr::VotingTextMergeWorker votingworker;
    Postr::NaiveSemanticAnalysisWorker semanticanalysis;
    
    auto joiner = [](Postr::Data& data,const Postr::Data& data2){
                        for(int i=0; i < data2.meta["text"].size(); ++i)
                            data.meta["text"].append(data2.meta["text"][i]);
                        if(!data.meta["images"].isMember("text"))
                        {
                            data.images.push_back(data2.image("text"));
                            data.meta["images"]["text"] = data.meta["images"].size();
                        }
                    };
    

    bool success = true;
    std::vector<Postr::Worker::ChainValue> remaining(args.pos.size());
    std::vector<Postr::DataPtr> data(args.pos.size());
    std::vector<Postr::Worker::WorkerChain> chain(args.pos.size());
        
    for(int argindex = 0; success && argindex < args.pos.size() && !Postr::Worker::aborted(); ++argindex)
    {
        std::string imagefile = args.as<std::string>(argindex);
    
        data[argindex] = Postr::DataPtr(new Postr::Data);
        if(Postr::File::exists(imagefile))
        {
            Postr::ImageData image = cv::imread(imagefile);
            //int maxdim = std::max(image.cols,image.rows);
            //float scalef = 1024./maxdim;
            
            LOG(INFO) << "reading " << imagefile;
            
            data[argindex]->meta["filename"] = imagefile;
            data[argindex]->images.push_back(image);
            data[argindex]->meta["images"]["original"] = (int)data[argindex]->images.size()-1;
        }
        else //file not found, this might be a document ID?
        {
            *(data[argindex]) = datastream.get(imagefile);
        }
            
        Postr::Worker::interactive = interactive;
        Postr::Worker::debug = debug; //debug shall be disabled when using concurrent pipelines
        
        std::vector<std::string> available = Postr::WorkerLoader::availableWorkers();
        LOG(INFO) << "available Workers: " << available;
                    
        //store the chain in a variable to make sure we don't run into undefined behaviour because of invalid references
        chain[argindex] = semanticanalysis << spellcorrectworker << wordsplitworker << regexworker << votingworker << Postr::Worker::join(
            textgroupworker << ocrworker << bgsegmentworker, 
            textgroupworker2 << ocrworker2, 
            joiner
        );
        
        //start processing data in the chain and store the chain counter in a variable to be able to track the chain's progress
        remaining[argindex] = chain[argindex] << data[argindex];
    
    }
    
    if(args.pos.size() == 0)
    {    
        semanticanalysis << spellcorrectworker << wordsplitworker << regexworker << votingworker << Postr::Worker::join(
            textgroupworker << ocrworker << bgsegmentworker, 
            textgroupworker2 << ocrworker2, 
            joiner
        ) << datastream;
    }
    
    for(int i=0; i < remaining.size(); ++i)
    {
        if(interactive)
            success = Postr::Worker::waitForFinished(remaining[i], chain[i].workers());
        else
            success = Postr::Worker::waitForFinished(remaining[i]);
        
        if(success)
        {
            if(loglevel > 3)
                LOG(INFO) << (*data[i]);
            else
                std::cout << data[i]->meta["result"];
            
            if(!data[i]->meta.isMember("filename") || data[i]->meta["filename"].isNull() && !dryrun) // this is a document from db
            {
                datastream.handleResult(*data[i]);
            }
        }
        else
            LOG(INFO) << "aborted.";
        if(debug && success)
        {
            cv::imshow("edges", data[i]->image("edges"));
            cv::imshow("contours", data[i]->image("contours"));
            cv::imshow("text", data[i]->image("text"));
            cv::waitKey(-1);
        }
        
        LOG(DEBUG) << "removing " << data[i]->meta["filename"].asString() << " from list";
    }
    
    LOG(DEBUG) << "exit";
    
    return 0;
}
