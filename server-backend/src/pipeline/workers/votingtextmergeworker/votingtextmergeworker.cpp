#include "votingtextmergeworker.h"

#include "util.h"

#include <iostream>

#undef LOG
#define LOG(LEVEL) (CLOG(LEVEL, ELPP_CURR_FILE_LOGGER_ID) << "[" << m_name << "] ")

namespace Postr 
{
    _WORKER_CLASS_::_WORKER_CLASS_()
        : AsyncWorker(_WORKER_NAME_)
        , m_max(0)
    {
    }
    
    _WORKER_CLASS_::~_WORKER_CLASS_()
    {
        writeCostFile("substitutioncosts.txt");
    }
    
    void _WORKER_CLASS_::writeCostFile(const std::string& fname)
    {
        if(!m_max)
        {
            LOG(WARNING) << "not writing cost file because no strings were merged yet.";
        }
        
        std::ofstream out(fname, std::ofstream::out);
        
        for(const auto& m : m_costMap)
        {
            out << m.first.first << "|" << m.first.second << "|" << 1-(m_costMap[m.first]/m_max) << std::endl;
        }
    }
    
    void _WORKER_CLASS_::processAsync(Data& data)
    {
        if (!data.meta.isMember("text"))
        {
            LOG(ERROR) << "given data object does not contain a 'text' object";
            m_status = 1;
        }        
        
        if (0 == m_status && !m_cancel)
        {
            // .. and whether it is iterable
            if(!data.meta["text"].isArray())
            {
                LOG(ERROR) << "'text' object in the given data object is not an array";
                m_status = 1;
            }
        }
        
        if (0 == m_status && !m_cancel)
        {
            int size = data.meta["text"].size();
            double progincr = 100./size;
            
            for (int i=0; i < size && !m_cancel; ++i)
            {
                if((!data.meta["text"][i].isMember("text")) || (!data.meta["text"][i]["text"].isString()))
                    continue;
                
                if((!data.meta["text"][i].isMember("replaces")) || (!data.meta["text"][i]["replaces"].isArray()))
                    continue;
                
                std::vector< std::pair< std::vector<std::string>, double > > diff;
                
                std::string a = data.meta["text"][i]["text"].asString();
                String::transformUmlauts(a);
                //get the edits of each replacement text in respect to the replacing text
                for(int j=0; j < data.meta["text"][i]["replaces"].size(); ++j)
                {
                    std::string b = data.meta["text"][i]["replaces"][j]["text"].asString();
                    String::transformUmlauts(b);
                    std::vector<std::string> edit = edits(a, b);
                    diff.push_back({
                        edit,
                        data.meta["text"][i]["replaces"][j]["confidence"].asDouble()
                    });
//                     std::string editstring;
//                     for(int k=0; k<edit.size(); ++k)
//                     {
//                         editstring += "\t";
//                         editstring += a[k];
//                         editstring += " -> ";
//                         editstring += edit[k];
//                         editstring += "\n";
//                     }
//                     LOG(DEBUG) << a << std::endl << b << std::endl << editstring;
                }
                //split the replacing text to single character strings
                std::string text = data.meta["text"][i]["text"].asString();
                String::transformUmlauts(text);
                std::vector<std::string> textvector(text.size());
                for(int j=0; j<text.size(); ++j)
                    textvector[j] = text[j];
                diff.push_back({
                    textvector, 
                    data.meta["text"][i]["confidence"].asDouble()
                });
                
                std::string result;
                
                //for each edit position
                for(int j=0; j<text.size(); ++j)
                {
                    //map possible edits to probabilities
                    std::map<std::string, double> candidates;
                
                    //for each text
                    double max = 0;
                    std::string best = "";
                    for(int k=0; k<diff.size(); ++k)
                    {
                        if(candidates.find(diff[k].first[j]) != candidates.end())
                            candidates[diff[k].first[j]] += diff[k].second;
                        else
                            candidates[diff[k].first[j]] = diff[k].second;
                        if(candidates[diff[k].first[j]] > max)
                        {
                            max = candidates[diff[k].first[j]];
                            best = diff[k].first[j];
                        }
                    }
                    result += best;
                }
                
                String::retransformUmlauts(result);
                
                LOG(DEBUG) << "voted " << result << " to be better than "<< data.meta["text"][i]["text"].asString();
                
                data.meta["text"][i]["text"] = result;
                
                data.meta["text"][i].removeMember("replaces");
                
                progress(progress()+progincr);
                
                //update cost map
                for(int i=0; i<diff.size()-1; ++i)
                {
                    for(int j=0; j<diff[diff.size()-1].first.size(); ++j)
                    {
                        std::string a = diff[diff.size()-1].first[j];
                        std::string b = diff[i].first[j];
                        std::transform(a.begin(), a.end(), a.begin(), ::tolower);
                        std::transform(b.begin(), b.end(), b.begin(), ::tolower);
                        String::retransformUmlauts(a);
                        String::retransformUmlauts(b);
                        
                        if(a == b)
                            continue;
                        
                        if(m_costMap.find({a,b}) != m_costMap.end())
                            m_costMap[{a,b}] += 1;
                        else
                            m_costMap[{a,b}] = 1;
                        if(m_costMap.find({b,a}) != m_costMap.end())
                            m_costMap[{b,a}] += 1;
                        else
                            m_costMap[{b,a}] = 1;
                        m_max = std::max(m_max, m_costMap[{a,b}]);
                        m_max = std::max(m_max, m_costMap[{b,a}]);
                    }
                }
                
            }
        }
        writeCostFile("substitutioncosts.txt");
    }
    
    std::vector<std::string> _WORKER_CLASS_::edits(const std::string& s1, const std::string& s2)
    {
        size_t m = s1.size();
        size_t n = s2.size();
        
        std::vector<std::string> diff(m);
        
        //total costs needed to change s1 to s2
        std::vector< std::vector<int> > total_costs(m+1,std::vector<int>(n+1,std::numeric_limits<int>::max()));
        for(int j=0; j<=n; ++j)
            total_costs[0][j] = 0;
        
        for(int i=0; i <= m; ++i)
        {
            for(int j=0; j <= n; ++j)
            {
                for(int p1=1; p1 <= 1 && p1+i <= m; ++p1)
                {
                    for(int p2=0; j+p2 <= n; ++p2)
                    {
                        int prefix_costs = 1;
                        
                        prefix_costs += std::abs(p2-p1);
                        if(p2 && s2[j+1] == s1[i+1])
                            prefix_costs -= 5;
                        
                        prefix_costs += total_costs[i][j];
                        if(prefix_costs < total_costs[i+p1][j+p2])
                        {
                            total_costs[i+p1][j+p2] = prefix_costs;
                        }
                    }
                }
//                 std::cout << "\t";
//                 for(int j=0; j<=n; ++j)
//                     std::cout << "\t" << s2[j];
//                 std::cout << std::endl;
//                 for(int i=0; i <= m; ++i)
//                 {
//                     if(i>0)
//                         std::cout << s1[i-1];
//                     for(int j=0; j <= n; ++j)
//                     {
//                         std::cout << "\t" << total_costs[i][j];
//                     }
//                     std::cout << std::endl;
//                 }
//                 std::cout << std::endl;
            }
        }
        
        int lastminj = n;
        for(int i=m; i>0; --i)
        {
            int min = std::numeric_limits<int>::max();
            int minj = 0;
            for(int j=0; j<=lastminj; ++j)
            {
                if(total_costs[i][j] < min)
                {
                    min = total_costs[i][j];
                    minj = j;
                }
            }
            if(i < m)
                diff[i] = s2.substr(minj, lastminj-minj);
            lastminj = minj;
        }
        diff[0] = s2.substr(0,lastminj);
        
        //LOG(DEBUG) << "total costs " << total_costs[m][n];
        
        return diff;
    }
}
