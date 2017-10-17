#include "textgroupcollateworker.h"
#include "util.h"

#undef LOG
#define LOG(LEVEL) (CLOG(LEVEL, ELPP_CURR_FILE_LOGGER_ID) << "[" << m_name << "] ")

namespace Postr 
{
    _WORKER_CLASS_::_WORKER_CLASS_()
        : AsyncWorker(_WORKER_NAME_)
    {

    }
    
    _WORKER_CLASS_::~_WORKER_CLASS_()
    {
    }
    
    void _WORKER_CLASS_::processAsync(Data& data)
    {
        if(!data.meta.isMember("text") || !data.meta["text"].isArray())
        {
            LOG(ERROR) << "no text input given";
            m_status = 1;
        }
        
        if(0 == m_status && !m_cancel)
        {
            cv::Rect a,b;
            float confa,confb;
            MetaData m;
            
            float compare;
            
            std::string stringa,stringb;
            
            float progincr = (100-progress())/data.meta["text"].size();
            
            for(int i=0; i < data.meta["text"].size() && !m_cancel; ++i)
            {
                for(int j=i+1; j < data.meta["text"].size() && !m_cancel; ++j)
                {
                    
                    if(!data.meta["text"][i].isMember("x")
                        || !data.meta["text"][i].isMember("y")
                        || !data.meta["text"][i].isMember("width")
                        || !data.meta["text"][i].isMember("height")
                        || !data.meta["text"][i].isMember("confidence")
                        || !data.meta["text"][i].isMember("text")
                    )
                        continue;
                    
                    a = cv::Rect(data.meta["text"][i]["x"].asInt(), data.meta["text"][i]["y"].asInt(), data.meta["text"][i]["width"].asInt(), data.meta["text"][i]["height"].asInt());
                    b = cv::Rect(data.meta["text"][j]["x"].asInt(), data.meta["text"][j]["y"].asInt(), data.meta["text"][j]["width"].asInt(), data.meta["text"][j]["height"].asInt());
                    
                    float areapercentage = (a & b).area() / (float)std::max(a.area(),b.area());
                    
                    if(areapercentage > 0.8)
                    {
                        size_t lena,lenb,spacesa,spacesb;
                        float lenpercentage,confpercentage;  
                        
                        confa = data.meta["text"][i]["confidence"].asFloat();
                        confb = data.meta["text"][j]["confidence"].asFloat();
                        stringa = data.meta["text"][i]["text"].asString().length();
                        stringb = data.meta["text"][j]["text"].asString().length();
                        compare = stringa.compare(stringb);
                        lena = stringa.length();
                        lenb = stringb.length();
                        compare /= (float)std::min(lena,lenb);
                        stringa.erase(remove(stringa.begin(), stringa.end(), ' '), stringa.end());                    
                        stringb.erase(remove(stringb.begin(), stringb.end(), ' '), stringb.end());
                        spacesa = lena-stringa.length();
                        spacesb = lenb-stringb.length();
                        
                        lenpercentage = (float)lena / (float)lenb;
                        confpercentage = confa / confb;
                        
                        LOG(DEBUG) << "compare: " << compare << ", confpercentage: " << confpercentage; 
                    
                        if((compare == 0 && confpercentage >= 1) || compare > 1 || confpercentage > 1)
                        {
                            LOG(DEBUG) << "removed " << data.meta["text"][j]["text"] << " in favour of " << data.meta["text"][i]["text"];
                            for(const MetaData& repl : data.meta["text"][j]["replaces"])
                                data.meta["text"][i]["replaces"].append(repl);
                            data.meta["text"][j].removeMember("replaces");
                            data.meta["text"][i]["replaces"].append(data.meta["text"][j]);
                            data.meta["text"].removeIndex(j, &m);
                            --j;
                        }
                        else if(compare < -1 || 1/confpercentage > 1)
                        {
                            LOG(DEBUG) << "removed " << data.meta["text"][i]["text"] << " in favour of " << data.meta["text"][j]["text"];
                            for(const MetaData& repl : data.meta["text"][i]["replaces"])
                                data.meta["text"][j]["replaces"].append(repl);
                            data.meta["text"][i].removeMember("replaces");
                            data.meta["text"][j]["replaces"].append(data.meta["text"][i]);
                            data.meta["text"].removeIndex(i, &m);
                            --i;
                            break;
                        }
                    }
                }
                progress(progress()+progincr);
            }
        }
    }
}
