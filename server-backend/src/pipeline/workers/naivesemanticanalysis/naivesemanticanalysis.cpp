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

#include "naivesemanticanalysis.h"
#include "util.h"
#include <iostream>
#include <chrono>
#include <time.h>

#undef LOG
#define LOG(LEVEL) (CLOG(LEVEL, ELPP_CURR_FILE_LOGGER_ID) << "[" << m_name << "] ")

namespace Postr 
{
    std::mutex _WORKER_CLASS_::m_timeMutex;
    
    _WORKER_CLASS_::_WORKER_CLASS_()
        : AsyncWorker(_WORKER_NAME_)
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        struct tm *parts = std::localtime(&now_c);

        currentYear = 1900 + parts->tm_year;
        currentMonth = parts->tm_mon+1;
        currentDay = parts->tm_mday;
        
        LOG(DEBUG) << "current year:" << currentYear;
        LOG(DEBUG) << "current month:" << currentMonth;
        LOG(DEBUG) << "current day:" << currentDay;
        
        CHECK(hostAvailable("http://google.com/test")) << "something wrong with urlIsAvailable";
        CHECK(hostAvailable("www.google.com/test")) << "something wrong with urlIsAvailable";
        CHECK(hostAvailable("http://www.google.com/test/weaf/ew")) << "something wrong with urlIsAvailable";
        CHECK(hostAvailable("google.com/asfr/rag")) << "something wrong with urlIsAvailable";
        CHECK(!hostAvailable("urlthatdoesnotexist.com/asfr/rag")) << "something wrong with urlIsAvailable";
        CHECK(hostAvailable("google.com")) << "something wrong with urlIsAvailable";
    }
    
    _WORKER_CLASS_::~_WORKER_CLASS_()
    {
    }
    
    void _WORKER_CLASS_::processAsync(Data& data)
    {
        
        std::set<std::string> date;
        std::set<std::string> day;
        std::set<std::string> time;
        std::vector<std::string> resultDate;
        
        std::set<std::string> price;
        
        std::set<std::string> street;
        std::set<std::string> city;
        std::set<std::string> address;
        std::vector<MetaData> resultAddress;
    
        std::map< std::string, std::set<std::string> > location;
        std::set<std::string> artist;
        
        std::set<std::string> url;
        std::set<std::string> mail;
        
        std::map<double, MetaData> textsize;
        
        if(!data.meta.isMember("text") || !data.meta["text"].isArray())
        {
            LOG(ERROR) << "no text input given";
            m_status = 1;
        }
        
        if(0 == m_status && !m_cancel)
        {
            for(const MetaData& text: data.meta["text"])
            {
                if(m_cancel)
                    break;
                
                if(text.isMember("text")
                && text.isMember("width") 
                && text.isMember("height") 
                && text.isMember("confidence")
                && (!text.isMember("info") || (
                    !text["info"].isMember("date")
                    && !text["info"].isMember("datetime")
                    && !text["info"].isMember("dayofweek")
                    && !text["info"].isMember("streetaddress")
                    && !text["info"].isMember("city")
                    && !text["info"].isMember("price")
                )))
                {
                    std::string s = text["text"].asString();
                    bool garbage = false;
                    for(size_t i = 0; i < s.length()-std::min(s.length(),(size_t)4); ++i)
                    { // if letters repeat more than 3 times, this is probably garbage
                        size_t pos = s.find_first_not_of(s[i], i+1);
                        if(pos != s.npos && pos > i+3)
                        {
                            garbage = true;
                            break;
                        }
                    }
                    if(!garbage && text["confidence"].asDouble() > 70)
                        textsize[text["width"].asDouble()*text["height"].asDouble()] = text;
                }
                
                if(!text.isMember("info"))
                    continue;
                
                if(text["info"].isMember("dayofweek"))
                {
                    for(const MetaData& d : text["info"]["dayofweek"])
                    {
                        std::string s = normalizeDay(d.asString());
                        if(!s.empty())
                            day.insert(s);
                    }
                }
                
                if(text["info"].isMember("date"))
                {
                    for(const MetaData& d : text["info"]["date"])
                    {
                        std::string s = normalizeDate(d.asString());
                        if(!s.empty())
                            date.insert(s);
                    }
                }
                
                if(text["info"].isMember("datetime") && text.isMember("confidence"))
                {
                    for(const MetaData& d : text["info"]["datetime"])
                    {
                        if(text["confidence"].asDouble() < 80)
                            break;
                        std::string s = normalizeTime(d.asString());
                        if(!s.empty())
                            time.insert(s);
                    }
                }
                
                if(text["info"].isMember("streetaddress") && text["info"].isMember("city"))
                {
                    for(const MetaData& d : text["info"]["streetaddress"])
                    {
                        for(const MetaData& e : text["info"]["city"])
                        {
                            address.insert(d.asString()+", "+e.asString());
                        }
                    }
                }
                
                if(text["info"].isMember("streetaddress"))
                {
                    for(const MetaData& d : text["info"]["streetaddress"])
                    {
                        street.insert(d.asString());
                    }
                }
                
                if(text["info"].isMember("city"))
                {
                    for(const MetaData& d : text["info"]["city"])
                    {
                        city.insert(d.asString());
                    }
                }
                
                if(text["info"].isMember("url"))
                {
                    for(const MetaData& d : text["info"]["url"])
                    {
                        std::string s = d.asString();
                        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
                        String::replaceAll(s, " ", "-");
                        String::replaceAll(s, "www-", "www.");
                        size_t dotpos = s.find_last_of('.');
                        size_t dashpos = s.find_last_of('-');
                        if(dashpos != s.npos && (dashpos > dotpos || dotpos == s.npos))
                            s[dashpos] = '.';
                        LOG(DEBUG) << s;
                        if(hostAvailable(s))
                            url.insert(normalizeURL(s));
                    }
                }
                
                if(text["info"].isMember("price"))
                {
                    for(const MetaData& d : text["info"]["price"])
                    {
                        price.insert(d.asString());
                    }
                }
                
                if(text["info"].isMember("location"))
                {
                    for(auto it = text["info"]["location"].begin(); it != text["info"]["location"].end(); ++it)
                    {
                        std::string c = it.name();
                        LOG(DEBUG) << c;
                        for(const MetaData& d : text["info"]["location"][c])
                            location[c].insert(d.asString());
                    }
                }
                
                if(text["info"].isMember("artists"))
                {
                    for(auto it = text["info"]["artists"].begin(); it != text["info"]["artists"].end(); ++it)
                    {
                        std::string c = it.name();
                        LOG(DEBUG) << c;
                        for(const MetaData& d : text["info"]["artists"][c])
                            artist.insert(d.asString());
                    }
                }
            }
            progress(10);
        }
        
        if(0 == m_status && !m_cancel)
        {
            // make sure we did not misinterpret a date info as a time info
            for(auto t_it = time.begin(); t_it != time.end(); )
            {
                std::string t_a = t_it->substr(0,2);
                std::string t_b = t_it->substr(3,2);
                auto d_it = date.begin();
                for(; d_it != date.end(); ++d_it)
                {
                    std::string d_a = d_it->substr(0,2);
                    std::string d_b = d_it->substr(3,2);
                    if(d_a == t_a && d_b == t_b) // we probably misinterpreted a date info as a time info
                    {
                        t_it = time.erase(t_it);
                        break;
                    }
                }
                if(d_it == date.end())
                    ++t_it;
            }
            
            progress(20);
            
            // make sure we have a valid date for each day of week
            for(auto w_it = day.begin(); w_it != day.end(); )
            {
                auto d_it = date.begin();
                for(; d_it != date.end(); ++d_it)
                {
                    std::tm time_in = { 0, 0, 0, // second, minute, hour
                        std::stoi(d_it->substr(0,2)), // 1-based day,
                        std::stoi(d_it->substr(3,2))-1, // 0-based month, 
                        std::stoi(d_it->substr(6)) - 1900 }; // year since 1900

                    std::time_t time_temp = std::mktime( & time_in );

                    std::lock_guard<std::mutex> guard(m_timeMutex);
                    // the return value from localtime is a static global - do not call
                    // this function from more than one thread!
                    std::tm const *time_out = std::localtime( & time_temp );

                    if(time_out->tm_wday == 1 && (*w_it) == "Montag")
                        break;
                    else if(time_out->tm_wday == 2 && (*w_it) == "Dienstag")
                        break;
                    else if(time_out->tm_wday == 3 && (*w_it) == "Mittwoch")
                        break;
                    else if(time_out->tm_wday == 4 && (*w_it) == "Donnerstag")
                        break;
                    else if(time_out->tm_wday == 5 && (*w_it) == "Freitag")
                        break;
                    else if(time_out->tm_wday == 6 && (*w_it) == "Samstag")
                        break;
                    else if(time_out->tm_wday == 0 && (*w_it) == "Sonntag")
                        break;
                }
                if(d_it == date.end())
                    w_it = day.erase(w_it);
                else
                {
                    //resultDate.insert((*w_it)+", "+(*d_it));
                    resultDate.push_back(*d_it);
                    d_it = date.erase(d_it);
                    ++w_it;
                }
            }
            
            progress(30);
            
            if(resultDate.empty())
                //add dates with no day of week to the result array
                std::copy(date.begin(), date.end(), std::inserter(resultDate, resultDate.end()));
            
            if(address.empty())
            {
                for(const std::string& c : city)
                {
                    for(const std::string& s : street)
                    {
                        address.insert(s+", "+c);
                    }
                    address.insert(c);
                }
            }
            for(const std::string& d : address)
            {
                MetaData s = normalizeAddress(d);
                if(!s.empty())
                    resultAddress.push_back(s);
            }
            
            progress(50);
        }
        
        if(0 == m_status && !m_cancel)
        {
            if(resultDate.size())
            {
                for(const std::string d : time)
                {
                    int i = data.meta["result"]["times"].size();
                    data.meta["result"]["times"][i]["start"] = resultDate[0]+"T"+d+":00.000";
                }
                if(time.empty())
                {
                    int i = data.meta["result"]["times"].size();
                    data.meta["result"]["times"][i]["start"] = resultDate[0];
                }
            }
            for(const std::string d : url)
            {
                int i = data.meta["result"]["urls"].size();
                data.meta["result"]["urls"][i] = d;
            }
            
            //get the 5 largests texts
            auto begin = textsize.begin();
            auto end = textsize.end();
            if(textsize.size() > 5)
                std::advance(begin,textsize.size()-5);
            std::vector<MetaData> largesttexts;
            for(auto it=begin; it!=end; ++it)
                largesttexts.push_back(it->second);
            //sort texts by position
            std::sort(largesttexts.begin(), largesttexts.end(), [](const MetaData& a, const MetaData& b){   
                return a["x"].asDouble()*a["x"].asDouble()+a["y"].asDouble()*a["y"].asDouble()
                    < b["x"].asDouble()*b["x"].asDouble()+b["y"].asDouble()*b["y"].asDouble();
            });
            
            progress(70);
            
            //remove overlapping texts
            for(auto a_it = largesttexts.begin(); a_it != largesttexts.end();)
            {
                if(String::hasRepeatingLetters((*a_it)["text"].asString()))
                {
                    a_it = largesttexts.erase(a_it);
                    continue;
                }
                
                bool erased = false;
                for(auto b_it = largesttexts.begin(); b_it != largesttexts.end(); ++b_it)
                {
                    if(b_it == a_it)
                        continue;
                    
                    if(rectOverlap(*a_it,*b_it))
                    {
                        if((*a_it)["confidence"].asDouble() > (*a_it)["confidence"].asDouble())
                        {
                            largesttexts.erase(b_it);
                            a_it = largesttexts.begin();
                        }
                        else
                        {
                            a_it = largesttexts.erase(a_it);
                        }
                        erased = true;
                        break;
                    }
                }
                if(!erased)
                    ++a_it;
            }
            
            progress(90);
            
            if(largesttexts.size())
                data.meta["result"]["title"] = largesttexts[0]["text"];
            for(int i=1; i < largesttexts.size(); ++i)
            {
                data.meta["result"]["description"] = data.meta["description"].asString() + largesttexts[i]["text"].asString();
            }
            
            for(const std::string& d : price)
            {
                int i = data.meta["result"]["tickets"].size();
                std::string price = d;
                String::transformUmlauts(price);
                std::string currency = price.substr(price.size()-2);
                price = price.substr(0,price.size()-2);
                String::retransformUmlauts(price);
                String::retransformUmlauts(currency);
                data.meta["result"]["tickets"][i]["price"] = price;
                data.meta["result"]["tickets"][i]["currency"] = currency;
            }
            
            bool exitloop = false;
            for(const std::string& c : city)
            {
                for(const std::string& l : location[c])
                {
                    data.meta["result"]["venue_name"] = l;
                    for(const MetaData& address : resultAddress)
                    {
                        if(!address.isMember("city"))
                            continue;
                        std::string s = address["city"].asString();
                        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
                        if(s.find(c) != s.npos)
                        {
                            resultAddress = {normalizeAddress(l+", "+address["city"].asString()+", "+address["address"].asString()+", "+address["country"].asString())};
                            exitloop = true;
                            break;
                        }
                    }
                    if(exitloop)
                        break;
                }
                if(exitloop)
                    break;
            }
            
            if(resultAddress.size())
            {
                for(auto it = resultAddress[0].begin(); it != resultAddress[0].end(); ++it)
                {
                    data.meta["result"][it.key().asString()] = resultAddress[0][it.key().asString()];
                }
            }
            
            for(auto it = artist.begin(); it != artist.end(); ++it)
            {
                int i = data.meta["result"]["artists"].size();
                data.meta["result"]["artists"][i] = *it;
            }
            
            data.meta["result"]["version"] = "0.2";
        }
    }
    
    std::string _WORKER_CLASS_::normalizeDay(const std::string& day)
    {
        std::string daylower = day;
        std::transform(daylower.begin(), daylower.end(), daylower.begin(), ::tolower);
        daylower = daylower.substr(0,2);
        
        if(daylower == "mo") return "Montag";
        if(daylower == "di") return "Dienstag";
        if(daylower == "mi") return "Mittwoch";
        if(daylower == "do") return "Donnerstag";
        if(daylower == "fr") return "Freitag";
        if(daylower == "sa") return "Samstag";
        if(daylower == "so") return "Sonntag";
        
        return "";
    }
    
    std::string _WORKER_CLASS_::normalizeDate(const std::string& date)
    {
        std::string datelower = date;
        std::transform(datelower.begin(), datelower.end(), datelower.begin(), ::tolower);
        
        datelower = replaceMonthNames(datelower);

        std::string normaldate(datelower.length(), '\0');
        std::string::iterator i = std::remove_copy_if(datelower.begin(), datelower.end(), normaldate.begin(), [](char c) { 
            return !isdigit(c); 
        });
        normaldate.erase(i, normaldate.end());

        std::string intermediatedate = normaldate;
        
        try 
        {
            switch(normaldate.length())
            {
                case 8:
                    normaldate = normaldate.substr(4)+"-"+normaldate.substr(2,2)+"-"+normaldate.substr(0,2);
                    break;
                case 6:
                    if(std::stoul(normaldate.substr(4)) > 12 || std::stoul(normaldate.substr(4)) == 0) // years end with 16,17,18,19,00 for the next four years
                        normaldate = "20"+normaldate.substr(4)+"-"+normaldate.substr(2,2)+"-"+normaldate.substr(0,2);
                    else
                        normaldate = ""; // this date is invalid
                    break;
                case 4:
                    if(std::stoul(normaldate.substr(0,2)) == 20) // years start with 20 for the next 93 years
                        normaldate = ""; // skip date with year only
                    else
                    {
                        LOG(DEBUG) << "date with 4 digits: " << normaldate;
                        if(
                            std::stoul(normaldate.substr(2,2)) > currentMonth 
                            || (std::stoul(normaldate.substr(2,2)) == currentMonth && std::stoul(normaldate.substr(0,2)) >= currentDay)
                        )
                        {
                            normaldate = std::to_string(currentYear)+"-"+normaldate.substr(2,2)+"-"+normaldate.substr(0,2);
                            LOG(DEBUG) << "this date seems to be in the current year: " << normaldate;
                        }
                        else
                        {
                            normaldate = std::to_string(currentYear+1)+"-"+normaldate.substr(2,2)+"-"+normaldate.substr(0,2);
                            LOG(DEBUG) << "this date seems to be in the next year: " << normaldate;
                        }
                    }
                    break;
                default:
                    normaldate = ""; 
                    break;
            }
            
            if(!normaldate.empty())
            {
                if(std::stoul(normaldate.substr(8,2)) > 31 || std::stoul(normaldate.substr(5,2)) > 12 
                || std::stoul(normaldate.substr(8,2)) == 0 || std::stoul(normaldate.substr(5,2)) == 0)
                {
                    LOG(DEBUG) << "this date is invalid: " << normaldate;
                    normaldate = "";
                }
            }
        } catch(std::invalid_argument e)
        {
            normaldate = "";
        }
        
        return normaldate;
    }
    
    std::string _WORKER_CLASS_::replaceMonthNames(const std::string& date)
    {
        std::string replaced = date;
        String::replaceAll(replaced, "jan", "01");
        String::replaceAll(replaced, "feb", "02");
        String::replaceAll(replaced, "mar", "03");
        String::replaceAll(replaced, "mär", "03");
        String::replaceAll(replaced, "apr", "04");
        String::replaceAll(replaced, "mai", "05");
        String::replaceAll(replaced, "may", "05");
        String::replaceAll(replaced, "jun", "06");
        String::replaceAll(replaced, "jul", "07");
        String::replaceAll(replaced, "aug", "08");
        String::replaceAll(replaced, "sep", "09");
        String::replaceAll(replaced, "okt", "10");
        String::replaceAll(replaced, "oct", "10");
        String::replaceAll(replaced, "nov", "11");
        String::replaceAll(replaced, "dez", "12");
        String::replaceAll(replaced, "dec", "12");
        
        return replaced;
    }
    
    std::string _WORKER_CLASS_::normalizeTime(const std::string& time)
    {
        std::string normaltime(time.length(), '\0');
        std::string::iterator i = std::remove_copy_if(time.begin(), time.end(), normaltime.begin(), [](char c) { 
            return !isdigit(c); 
        });
        normaltime.erase(i, normaltime.end());
        
        char* end;
        int lastdigit;
        switch(normaltime.length())
        {
            case 4:
                lastdigit = std::strtol(normaltime.substr(3,1).c_str(), &end, 10);
                if(lastdigit != 5 && lastdigit != 0)
                    normaltime = "";
                else
                    normaltime = normaltime.substr(0,2)+":"+normaltime.substr(2);
                break;
            case 3:
                normaltime = "0"+normaltime.substr(0,1)+":"+normaltime.substr(1);
                break;
            case 2:
                normaltime += ":00";
                break;
            case 1:
                normaltime = "0"+normaltime+":00";
            default:
                normaltime = ""; 
                break;
        }
        
        //this should have been checked by regex worker already
        /*if(!normaltime.empty())
        {
            if(std::stoul(normaltime.substr(0,2)) > 23) // hours from 0 to 23
                normaltime = "";
            else if(std::stoul(normaltime.substr(2,2)) > 59) // minutes from 0 to 59
                normaltime = "";
        }*/
        
        return normaltime;
    }
    
    std::string _WORKER_CLASS_::normalizeURL(const std::string& url)
    {
        std::string normalurl = url;
        std::transform(normalurl.begin(), normalurl.end(), normalurl.begin(), ::tolower);
        size_t pos = normalurl.find("://");
        if(pos == normalurl.npos)
            normalurl = "http://"+normalurl;
        else
            normalurl = "http://"+normalurl.substr(pos+3);
        return normalurl;
    }
    
    bool _WORKER_CLASS_::inRange(int value, int min, int max)
    { 
        return (value >= min) && (value <= max); 
    }

    bool _WORKER_CLASS_::rectOverlap(const MetaData& a, const MetaData& b)
    {
        bool xOverlap = inRange(a["x"].asDouble(), b["x"].asDouble(), b["x"].asDouble() + b["width"].asDouble())
            || inRange(b["x"].asDouble(), a["x"].asDouble(), a["x"].asDouble() + a["width"].asDouble());

        bool yOverlap = inRange(a["y"].asDouble(), b["y"].asDouble(), b["y"].asDouble() + b["height"].asDouble())
            || inRange(b["y"].asDouble(), a["y"].asDouble(), a["y"].asDouble() + a["height"].asDouble());

        return xOverlap && yOverlap;
    }
    
}
