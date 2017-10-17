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

#include "regexworker.h"
#include "util.h"

#undef LOG
#define LOG(LEVEL) (CLOG(LEVEL, ELPP_CURR_FILE_LOGGER_ID) << "[" << m_name << "] ")

namespace Postr 
{
    _WORKER_CLASS_::_WORKER_CLASS_()
        : AsyncWorker(_WORKER_NAME_)
    {
        m_regex["dayofweek"] = "Montag|Dienstag|Mittwoch|Donnerstag|Freitag|Samstag|Sonntag|MONTAG|DIENSTAG|MITTWOCH|DONNERSTAG|FREITAG|SAMSTAG|SONNTAG|Mo|Di|Mi|Do|Fr|Sa|So|MO|DI|MI|DO|FR|SA|SO";
        
        m_regex["date"] = "((am |AM |vom |VOM |zum |ZUM |bis |BIS )?((31|30|[012]\\d|\\d)([\\.\\/]|\\s)+((0\\d|1[012]|[1-9])|((Jan|Feb|Mar|Apr|Mai|Jun|Jul|Aug|Sep|Okt|Nov|Dez|jan|feb|mar|apr|mai|jun|jul|aug|sep|okt|nov|dez|JAN|FEB|MAR|APR|MAI|JUN|JUL|AUG|OKT|NOV|DEZ|Mär|mär|MÄR|Oct|oct|OCT|Dec|dec|DEC)([a-z]|[A-Z])*))(([\\.\\/]|\\s)+((20)?\\d{2}))?)($|(?=\\D)))";
        
        m_regex["datetime"] = "((um |ab |gegen |Einlass |EINLASS |UM |AB |GEGEN |Beginn |BEGINN |Start |START |von |VON |bis |BIS )*(1[0-9]|2[0-3]|[1-9])((:|\\.|\\s)([0-5](0|5)))?(([,-]|\\s)*(Uhr|H|UHR|h|uhr|AM|PM|am|pm))?($|(?=\\D)))";
        
        m_regex["url"] = "((https?:\\/\\/)?([a-zA-Z][\\da-zA-Z-]+\\.*)(\\.| )([a-zA-Z\\.]{2,6})([\\/\\w \\.-]*)*\\/?)";
        
        m_regex["email"] = "(([a-zA-Z][a-z0-9_\\.-]+)@([\\da-z\\.-]+)(\\.| )([a-z\\.]{2,6}))";
        
        m_regex["streetaddress"] = "(([a-z]|[A-Z]|ä|ö|ü|ß|Ä|Ö|Ü|-| ){4,}\\d{1,4}($|(?=\\D)))";
        
        m_regex["city"] = "(\\b\\d{5}( )+([a-z]|[A-Z]|ä|ö|ü|ß|Ä|Ö|Ü|\\-|\\. ){4,})";
        
        m_regex["price"] = "(\\b([0-9]+[,\\. ]*)+(€|\\$|EUR|,\\-))";
    }
    
    _WORKER_CLASS_::~_WORKER_CLASS_()
    {
        
    }
    
    void _WORKER_CLASS_::processAsync(Data& data)
    {
        if(m_cancel)
            return;
        
        if(!data.meta.isMember("text") || !data.meta["text"].isArray())
        {
            LOG(ERROR) << "no text input given";
            m_status = 1;
        }
        
        if(0 == m_status && !m_cancel)
        {
            std::smatch m;
            
            for(auto it : m_regex)
            {
                for(int i=0; !m_cancel && i < data.meta["text"].size(); ++i)
                {
                    std::string str = data.meta["text"][i]["text"].asString();
                    while(std::regex_search(str, m, it.second, std::regex_constants::match_any))
                    {
                        LOG(DEBUG) << str << " matches " << it.first;
                        std::string lastx = "";
                        if(!m.empty())
                        {
                            std::string x = m[0];
                            if((x.length() > 3 || it.first == "price" || it.first == "dayofweek") && x != lastx)
                            {
                                data.meta["text"][i]["info"][it.first].append(x);
                                lastx = x;
                            }
                        }
                        str = m.suffix().str();
                    }
                }
            }
        }
    }
}
