#include "wordsplitworker.h"
#include "util.h"
#include <iostream>
#include <iterator>
#include <numeric>

#undef LOG
#define LOG(LEVEL) (CLOG(LEVEL, ELPP_CURR_FILE_LOGGER_ID) << "[" << m_name << "] ")

#define TEXT_KEY "text"

namespace Postr 
{
    _WORKER_CLASS_::_WORKER_CLASS_()
        : AsyncWorker(_WORKER_NAME_)
    {
        //Constructor
    }
    
    _WORKER_CLASS_::~_WORKER_CLASS_()
    {
        //Destructor
    }
    
    void _WORKER_CLASS_::processAsync(Data& data)
    {
        // Check if data contains a "text" object...
        if (!data.meta.isMember(TEXT_KEY))
        {
            LOG(ERROR) << "given data object does not contain a 'text' object";
            m_status = 1;
        }        
        
        if (0 == m_status && !m_cancel)
        {
            // .. and whether it is iterable
            if(!data.meta[TEXT_KEY].isArray())
            {
                LOG(ERROR) << "'text' object in the given data object is not an array";
                m_status = 1;
            }
        }
        
        if (0 == m_status && !m_cancel)
        {
            int size = data.meta[TEXT_KEY].size();
            double progincr = 100./size;
            
            // Iterate over every text frame
            for (int idx = 0; idx < size; ++idx)
            {
                MetaData& textframe = data.meta[TEXT_KEY][idx];
                if ( (!textframe.isMember("text")) || (!textframe["text"].isString()) )
                {
                    LOG(ERROR) << "textframe does not contain a 'text' string";
                    m_status = 1;
                }
                
                // Write string into vector of space separated strings
                std::istringstream iss(textframe["text"].asString());
                std::vector<std::string> words{std::istream_iterator<std::string>(iss), {}};
                
                std::string baseid = textframe["id"].asString();
                if(baseid.empty())
                {
                    baseid = Util::uuid();
                    textframe["id"] = baseid;
                }


                std::string s;
                for (int i = 0; i < words.size()-1; ++i) {s += words[i]; s += " ";}
                s += words.back();
               
                for (int i = 0; i < words.size(); ++i) // i is starting index
                {

                    for (int j = 0; j+i < words.size(); ++j) // number of parts to be added
                    {
                        s = "";
                        for (int k = 0; k < j; ++k)
                        {
                            s += words[i+k]; s+= " ";
                        }
                        s += words[i+j];
                        
                        newText(data, textframe, s, baseid);
                    }
                }
                progress(progress()+progincr);
            }
        }
    }
    
    void _WORKER_CLASS_::newText(Data& targetData, const MetaData& textframe, const std::string& text, const std::string& baseid)
    {
        int index = targetData.meta["text"].size();
        targetData.meta["text"][index]["text"] = text;
        targetData.meta["text"][index]["confidence"] = textframe["confidence"];
        targetData.meta["text"][index]["baseid"] = baseid;

    }
}
