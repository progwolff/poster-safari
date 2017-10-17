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

#include "spellcorrect.h"

#include "util.h"

#include <iostream>

#undef LOG
#define LOG(LEVEL) (CLOG(LEVEL, ELPP_CURR_FILE_LOGGER_ID) << "[" << m_name << "] ")

namespace Postr 
{
    _WORKER_CLASS_::_WORKER_CLASS_()
        : AsyncWorker(_WORKER_NAME_)
    {
    }
    
    void _WORKER_CLASS_::initAsync()
    {
        std::string dictfile = File::locate("cities.txt", File::DataLocation, 1);
        std::string costfile = File::locate("substitutioncosts.txt", File::DataLocation, 1);
        
        SpellCorrect::DISTANCE_FUNCT alg;
        std::string distalg = config("SpellCorrect_distanceAlgorithm", "OCR_OPTIMIZED", "edit distance algorithm. One of \"LEVENSTEIN\", \"DAMERAU_LEVENSTEIN\", \"OCR_OPTIMIZED\".");
        if(distalg == "LEVENSTEIN" || distalg == "levenstein")
            alg = SpellCorrect::LEVENSTEIN;
        if(distalg == "DAMERAU_LEVENSTEIN" || distalg == "damerau_levenstein")
            alg = SpellCorrect::DAMERAU_LEVENSTEIN;
        if(distalg == "OCR_OPTIMIZED" || distalg == "ocr_optimized")
            alg = SpellCorrect::OCR_OPTIMIZED;
        
        //need to set the edit distance prior to creating a dictionary!
        m_spell["city"].editDistanceMax = config("SpellCorrect_editDistanceMax", 2, "maximum edit distance"); 
        m_spell["city"].verbose = 1; // 0->only one result, 1->all results with minimum distance or matching length, 2->all results with distance < editDistanceMax
        m_spell["city"].distalg = alg;
        m_spell["city"].LoadCostFile(costfile,'|');
        m_spell["city"].CreateDictionary(dictfile);
        
        dictfile = File::locate("dayofweek.txt", File::DataLocation, 1);
        
        m_spell["dayofweek"].editDistanceMax = m_spell["city"].editDistanceMax;
        m_spell["dayofweek"].verbose = 1; // 0->only one result, 1->all results with minimum distance or matching length, 2->all results with distance < editDistanceMax
        m_spell["dayofweek"].distalg = alg;
        m_spell["dayofweek"].LoadCostFile(costfile,'|');
        m_spell["dayofweek"].CreateDictionary(dictfile);
        
        std::vector<std::string> locationfiles = File::locateAll(".*txt", File::DataLocation, 2);
        for(const std::string& locationfile : locationfiles)
        {
            if(locationfile.find("/locations/") == locationfile.npos)
                continue;
            LOG(DEBUG) << locationfile;
            std::string location = locationfile.substr(locationfile.find_last_of('/')+1);
            location = location.substr(0, location.find('.'));
            m_spell["location/"+location].editDistanceMax = m_spell["city"].editDistanceMax;
            m_spell["location/"+location].verbose = 1; // 0->only one result, 1->all results with minimum distance or matching length, 2->all results with distance < editDistanceMax
            m_spell["location/"+location].distalg = alg;
            m_spell["location/"+location].LoadCostFile(costfile,'|');
            m_spell["location/"+location].CreateDictionary(locationfile);
        }
        std::vector<std::string> artistsfiles = File::locateAll(".*txt", File::DataLocation, 2);
        for(const std::string& artistsfile : artistsfiles)
        {
            if(artistsfile.find("/artists/") == artistsfile.npos)
                continue;
            LOG(DEBUG) << artistsfile;
            std::string artist = artistsfile.substr(artistsfile.find_last_of('/')+1);
            artist = artist.substr(0, artist.find('.'));
            m_spell["artists/"+artist].editDistanceMax = m_spell["city"].editDistanceMax;
            m_spell["artists/"+artist].verbose = 1; // 0->only one result, 1->all results with minimum distance or matching length, 2->all results with distance < editDistanceMax
            m_spell["artists/"+artist].distalg = alg;
            m_spell["artists/"+artist].LoadCostFile(costfile,'|');
            m_spell["artists/"+artist].CreateDictionary(artistsfile);
        }
    }
    
    _WORKER_CLASS_::~_WORKER_CLASS_()
    {
    }
    
    void _WORKER_CLASS_::processAsync(Data& data)
    {
        double progincr = 99./data.meta["text"].size()/m_spell.size();
        for(auto it=m_spell.begin(); it!=m_spell.end(); ++it)
        {
            std::string key = it->first;
            std::string secondkey = "";
            size_t pos = key.find('/');
            if(pos != key.npos)
            {
                secondkey = key.substr(pos+1);
                key = key.substr(0, pos);
            }
            
            for(int i=0; 0 == m_status && !m_cancel && data.meta["text"].isArray() && i < data.meta["text"].size(); ++i)
            {                
                std::string text = data.meta["text"][i]["text"].asString();
                std::vector<SpellCorrect::suggestItem> items = it->second.Correct(text);
                int index = i;
                if(data.meta["text"][i].isMember("baseid"))
                {
                    for(int j=0; j < data.meta["text"].size(); ++j)
                    {
                        if(data.meta["text"][j].isMember("id") && data.meta["text"][j]["id"].asString() == data.meta["text"][i]["baseid"].asString())
                        {
                            index = j;
                            break;
                        }
                    }
                }
                int offset = 0;
                if(secondkey.empty())
                {
                    if(data.meta["text"][index]["info"].isMember(key))
                        offset = data.meta["text"][index]["info"][key].size();
                    for(int j=0; j<items.size(); ++j)
                    {
                        if(items[j].distance < 0.2*(double)text.length())
                            data.meta["text"][index]["info"][key][offset+j] = items[j].term;
                    }
                }
                else
                {
                    if(data.meta["text"][index]["info"].isMember(key) && data.meta["text"][index]["info"][key].isMember(secondkey))
                        offset = data.meta["text"][index]["info"][key][secondkey].size();
                    for(int j=0; j<items.size(); ++j)
                    {
                        if(items[j].distance < 0.2*(double)text.length())
                            data.meta["text"][index]["info"][key][secondkey][offset+j] = items[j].term;
                    }
                }
                progress(progress()+progincr);
            }
        }
    }
    
    void _WORKER_CLASS_::writeDefaultCostFile(const std::string& fname)
    {
        std::ofstream out(fname, std::ofstream::out);
        
        std::vector<std::string> chars({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "ä", "ö", "ü", "ß", "€", "\"", 
            "!", "\\", "%", "&", "/", "(", ")", "=", "?", "{", "}", "[", "]", "-", "_", "#", "+", "*", ",", ";", ".", "'", ":", "$", "@", "", " "});
        for(const std::string& char1 : chars)
        {
            for(const std::string& char2 : chars)
            {
                if(char1 == char2)
                    out << char1 << "|" << char2 << "|" << std::to_string(0) << std::endl;
                else
                    out << char1 << "|" << char2 << "|" << std::to_string(1) << std::endl;
            }
        }
    }

#undef LOG
#define LOG(LEVEL) (CLOG(LEVEL, ELPP_CURR_FILE_LOGGER_ID) << "[SpellCorrect] ")


    _WORKER_CLASS_::SpellCorrect::SpellCorrect()
    {
    #ifdef USE_GOOGLE_DENSE_HASH_MAP
        dictionary.set_empty_key(0);
    #endif
    }

    void _WORKER_CLASS_::SpellCorrect::LoadCostFile(const std::string fname, const char separator)
    {
        std::ifstream f(fname);

        if (!f.good())
        {
            LOG(ERROR) << "File not found: " << fname;
            return;
        }
        
        LOG(INFO) << "using cost file "<< fname;

        const char* locale = setlocale(LC_ALL, "C");
        std::string a,b;
        double c;
        for (std::string line; std::getline(f, line); ) {
            std::vector<std::string> chunks = String::split(line, separator);
            if(chunks.size() < 3)
                continue;
            a = chunks[0];
            String::transformUmlauts(a);
            b = chunks[1];
            String::transformUmlauts(b);
            c = std::stod(chunks[2]);
            substitution_costs[{a,b}] = c;
            
            if(max_prefix_length < std::max(a.size(),b.size()))
                max_prefix_length = std::max(a.size(),b.size());
        }

        setlocale(LC_ALL, locale);
        f.close();
    }


    void _WORKER_CLASS_::SpellCorrect::CreateDictionary(std::string corpus)
    {
        std::ifstream sr(corpus);

        if (!sr.good())
        {
            LOG(ERROR) << "File not found: " << corpus;
            return;
        }

        LOG(INFO) << "Creating dictionary ...";

        long wordCount = 0;

        for (std::string line; std::getline(sr, line); ) {

            String::transformUmlauts(line);
            
            for (std::string & key : parseWords(line))
            {
                if (CreateDictionaryEntry(key)) ++wordCount;
            }
        }

        sr.close();
    }

    #ifdef IO_OPERATIONS

    void _WORKER_CLASS_::SpellCorrect::Save(string filePath)
    {
        std::ofstream fileStream(filePath, ios::binary);

        if (fileStream.good())
        {
    #ifdef USE_GOOGLE_DENSE_HASH_MAP
            std::unordered_map<size_t, dictionaryItemContainer> tmpDict(dictionary.begin(), dictionary.end()); // should be undered_map
    #endif

            msgpack::packer<std::ofstream> packer(&fileStream);

            packer.pack(this->verbose);
            packer.pack(this->editDistanceMax);
            packer.pack(this->maxlength);

    #ifdef USE_GOOGLE_DENSE_HASH_MAP
            packer.pack(tmpDict);
    #else
            packer.pack(dictionary);
    #endif
            packer.pack(this->wordlist);
        }

        fileStream.close();
    }

    void _WORKER_CLASS_::SpellCorrect::Load(string filePath)
    {
        std::ifstream fileStream(filePath, ios::binary);

        if (fileStream.good())
        {
            std::string str((std::istreambuf_iterator<char>(fileStream)), (std::istreambuf_iterator<char>()));

            msgpack::unpacker packer;

            packer.reserve_buffer(str.length());
            memcpy(packer.buffer(), str.data(), str.length());
            packer.buffer_consumed(str.length());

            msgpack::object_handle handler;
            packer.next(handler);

            handler.get().convert(this->verbose);
            packer.next(handler);

            handler.get().convert(this->editDistanceMax);
            packer.next(handler);

            handler.get().convert(this->maxlength);
            packer.next(handler);

    #ifdef USE_GOOGLE_DENSE_HASH_MAP
            std::unordered_map<size_t, dictionaryItemContainer> tmpDict;
            handler.get().convert(tmpDict);
            this->dictionary.insert(tmpDict.begin(), tmpDict.end());
    #else
            handler.get().convert(this->dictionary);
    #endif
            packer.next(handler);
            handler.get().convert(this->wordlist);

        }

        fileStream.close();
    }

    #endif

    bool _WORKER_CLASS_::SpellCorrect::CreateDictionaryEntry(std::string key)
    {
        bool result = false;
        dictionaryItemContainer value;

        auto valueo = dictionary.find(getHastCode(key));
        if (valueo != dictionary.end())
        {
            value = valueo->second;

            if (valueo->second.itemType == ItemType::INTEGER)
            {
                value.itemType = ItemType::DICT;
                value.dictValue = std::make_shared<dictionaryItem>();
                value.dictValue->suggestions.push_back(valueo->second.intValue);
            }
            else
                value = valueo->second;

            if (value.dictValue->count < INT_MAX)
                ++(value.dictValue->count);
        }
        else if (wordlist.size() < INT_MAX)
        {
            value.itemType = ItemType::DICT;
            value.dictValue = std::make_shared<dictionaryItem>();
            ++(value.dictValue->count);
            std::string mapKey = key;
            dictionary.insert(std::pair<size_t, dictionaryItemContainer>(getHastCode(mapKey), value));

            if (key.size() > maxlength)
                maxlength = key.size();
        }

        if (value.dictValue->count == 1)
        {
            wordlist.push_back(key);
            size_t keyint = wordlist.size() - 1;

            result = true;

            auto deleted = CUSTOM_SET<std::string>();
    #ifdef USE_GOOGLE_DENSE_HASH_MAP
            deleted.set_empty_key("");
    #endif

            Edits(key, deleted);

            for (std::string del : deleted)
            {
                auto value2 = dictionary.find(getHastCode(del));
                if (value2 != dictionary.end())
                {
                    if (value2->second.itemType == ItemType::INTEGER)
                    {
                        value2->second.itemType = ItemType::DICT;
                        value2->second.dictValue = std::make_shared<dictionaryItem>();
                        value2->second.dictValue->suggestions.push_back(value2->second.intValue);
                        dictionary[getHastCode(del)].dictValue = value2->second.dictValue;

                        if (std::find(value2->second.dictValue->suggestions.begin(), value2->second.dictValue->suggestions.end(), keyint) == value2->second.dictValue->suggestions.end())
                            AddLowestDistance(value2->second.dictValue, key, keyint, del);
                    }
                    else if (std::find(value2->second.dictValue->suggestions.begin(), value2->second.dictValue->suggestions.end(), keyint) == value2->second.dictValue->suggestions.end())
                        AddLowestDistance(value2->second.dictValue, key, keyint, del);
                }
                else
                {
                    dictionaryItemContainer tmp;
                    tmp.itemType = ItemType::INTEGER;
                    tmp.intValue = keyint;

                    dictionary.insert(std::pair<size_t, dictionaryItemContainer>(getHastCode(del), tmp));
                }
            }
        }
        return result;
    }

    std::vector<_WORKER_CLASS_::SpellCorrect::suggestItem> _WORKER_CLASS_::SpellCorrect::Correct(std::string input)
    {

        std::transform(input.begin(), input.end(), input.begin(), ::tolower);
        String::transformUmlauts(input);
        std::vector<SpellCorrect::suggestItem> suggestions;

    #ifdef ENABLE_TEST
        using namespace std::chrono;

        high_resolution_clock::time_point t1 = high_resolution_clock::now();

        for (size_t i = 0; i < 100000; ++i)
        {
            Lookup(input, editDistanceMax);
        }

        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

        LOG(DEBUG) << "It took me " << time_span.count() << " seconds.";
    #endif
        suggestions = Lookup(input, editDistanceMax);
        return suggestions;

    }

    std::vector<std::string> _WORKER_CLASS_::SpellCorrect::parseWords(std::string text) const
    {
        std::vector<std::string> returnData;

        std::transform(text.begin(), text.end(), text.begin(), ::tolower);
        
        returnData.push_back(text);
        /*std::regex word_regex("[\\w\\.' /]+");
        auto words_begin = std::sregex_iterator(text.begin(), text.end(), word_regex);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            returnData.push_back(match.str());
        }*/

        return returnData;
    }

    void _WORKER_CLASS_::SpellCorrect::AddLowestDistance(std::shared_ptr<dictionaryItem> const & item, std::string suggestion, size_t suggestionint, std::string del)
    {
        if ((verbose < 2) && (item->suggestions.size() > 0) && (wordlist[item->suggestions[0]].size() - del.size() > suggestion.size() - del.size()))
            item->suggestions.clear();

        if ((verbose == 2) || (item->suggestions.size() == 0) || (wordlist[item->suggestions[0]].size() - del.size() >= suggestion.size() - del.size()))
            item->suggestions.push_back(suggestionint);
    }

    void _WORKER_CLASS_::SpellCorrect::Edits(std::string word, CUSTOM_SET<std::string> & deletes) const
    {
        CUSTOM_MAP<size_t, std::string> queue;
    #ifdef USE_GOOGLE_DENSE_HASH_MAP
        queue.set_empty_key(0);
    #endif
        queue.insert(std::pair<size_t, const char*>(getHastCode(word), word.c_str()));

        for (size_t d = 0; d < editDistanceMax; ++d)
        {
            CUSTOM_MAP<size_t, std::string> tempQueue;
    #ifdef USE_GOOGLE_DENSE_HASH_MAP
            tempQueue.set_empty_key(0);
    #endif

            for (auto item : queue) {
                if (item.second.size()) {
                    // For Performance ->
                    for (size_t i = 0; i < item.second.size(); ++i)
                    {

                        std::string del = item.second.c_str();        
                        
                        size_t k = i;
                        int len = item.second.size();
                        for (; k < len - 1; k++)
                            del[k] = item.second[k + 1];
                        del[k] = '\0';
                        // <- For Performance

                        if (!deletes.count(del) && !del.empty())
                            deletes.insert(del);
                            
                        if (tempQueue.find(getHastCode(del)) == tempQueue.end())
                        {
                            tempQueue.insert(std::pair<size_t, std::string>(getHastCode(del), del));
                        }
                    }
                }
            }
            queue = tempQueue;
        }
    }

    std::vector<_WORKER_CLASS_::SpellCorrect::suggestItem> _WORKER_CLASS_::SpellCorrect::Lookup(std::string input, size_t editDistanceMax)
    {
        if (input.size() - editDistanceMax > maxlength)
            return std::vector<SpellCorrect::suggestItem>();

        std::vector<std::string> candidates;
        candidates.reserve(2048);
        CUSTOM_SET<size_t> hashset1;
    #ifdef USE_GOOGLE_DENSE_HASH_MAP
        hashset1.set_empty_key(0);
    #endif

        std::vector<SpellCorrect::suggestItem> suggestions;
        CUSTOM_SET<size_t> hashset2;
    #ifdef USE_GOOGLE_DENSE_HASH_MAP
        hashset2.set_empty_key(0);
    #endif

        //object valueo;

        candidates.push_back(input);

        size_t candidatesIndexer = 0; // for performance
        while ((candidates.size() - candidatesIndexer) > 0)
        {
            std::string candidate = candidates[candidatesIndexer];
            size_t candidateSize = candidate.size(); // for performance
            ++candidatesIndexer;

            if ((verbose < 2) && (suggestions.size() > 0) && (input.size() - candidateSize > suggestions[0].distance))
                goto sort;

            auto valueo = dictionary.find(getHastCode(candidate));

            //read candidate entry from dictionary
            if (valueo != dictionary.end())
            {
                if (valueo->second.itemType == ItemType::INTEGER)
                {
                    valueo->second.itemType = ItemType::DICT;
                    valueo->second.dictValue = std::make_shared<dictionaryItem>();
                    valueo->second.dictValue->suggestions.push_back(valueo->second.intValue);
                }


                if (valueo->second.itemType == ItemType::DICT &&
                    valueo->second.dictValue->count > 0 &&
                    hashset2.insert(getHastCode(candidate)).second)
                {
                    //add correct dictionary term term to suggestion list
                    suggestItem si;
                    std::string term = candidate;
                    String::retransformUmlauts(term);
                    si.term = term;
                    si.count = valueo->second.dictValue->count;
                    if(distalg != OCR_OPTIMIZED)
                        si.distance = input.size() - candidateSize;
                    else
                        si.distance = ocr_distance(input, candidate);
                    suggestions.push_back(si);
                    //early termination
                    if (distalg != OCR_OPTIMIZED && (verbose < 2) && (input.size() - candidateSize == 0))
                        goto sort;
                }

                for (size_t suggestionint : valueo->second.dictValue->suggestions)
                {
                    //save some time
                    //skipping double items early: different deletes of the input term can lead to the same suggestion
                    //index2word
                    std::string suggestion = wordlist[suggestionint];
                    if (hashset2.insert(getHastCode(suggestion)).second)
                    {
                        double distance = 0;
                        if (suggestion != input)
                        {
                            if (distalg != OCR_OPTIMIZED && suggestion.size() == candidateSize) distance = input.size() - candidateSize;
                            else if (distalg != OCR_OPTIMIZED && input.size() == candidateSize) distance = suggestion.size() - candidateSize;
                            else
                            {
                                auto func = levenshtein_distance;
                                switch(distalg)
                                {
                                    case LEVENSTEIN:
                                        func = levenshtein_distance;
                                        break;
                                    case DAMERAU_LEVENSTEIN:
                                        func = DamerauLevenshteinDistance;
                                        break;
                                    case OCR_OPTIMIZED:
                                        func = ocr_distance;
                                        break;
                                    default:
                                        func = levenshtein_distance;
                                }
                                
                                if(distalg != OCR_OPTIMIZED)
                                {
                                    size_t ii = 0;
                                    size_t jj = 0;
                                    while ((ii < suggestion.size()) && (ii < input.size()) && (suggestion[ii] == input[ii]))
                                        ++ii;

                                    while ((jj < suggestion.size() - ii) && (jj < input.size() - ii) && (suggestion[suggestion.size() - jj - 1] == input[input.size() - jj - 1]))
                                        ++jj;
                                    
                                    if ((ii > 0) || (jj > 0))
                                        distance = func(suggestion.substr(ii, suggestion.size() - ii - jj), input.substr(ii, input.size() - ii - jj));
                                    else
                                        distance = func(suggestion, input);
                                }
                                else
                                    distance = func(suggestion, input);
                            }
                        }

                        if ((verbose < 2) && (suggestions.size() > 0) && (suggestions[0].distance > distance))
                            suggestions.clear();

                        if ((verbose < 2) && (suggestions.size() > 0) && (distance > suggestions[0].distance))
                            continue;

                        if (distance <= editDistanceMax)
                        {
                            auto value2 = dictionary.find(getHastCode(suggestion));

                            if (value2 != dictionary.end())
                            {
                                suggestItem si;
                                std::string term = suggestion;
                                String::retransformUmlauts(term);
                                si.term = term;
                                if (value2->second.itemType == ItemType::DICT)
                                    si.count = value2->second.dictValue->count;
                                else
                                    si.count = 1;
                                si.distance = distance;
                                suggestions.push_back(si);
                            }
                        }
                    }
                }
            }


            if (input.size() - candidateSize < editDistanceMax)
            {
                if ((verbose < 2) && (suggestions.size() > 0) && (input.size() - candidateSize >= suggestions[0].distance))
                    continue;

                for (size_t i = 0; i < candidateSize; ++i)
                {
                    std::string wordClone = candidate;
                    std::string & del = wordClone.erase(i, 1);
                    if (hashset1.insert(getHastCode(del)).second)
                        candidates.push_back(del);
                }
            }
        }//end while

            //sort by ascending edit distance, then by descending word frequency
    sort:
        if (verbose < 2)
            sort(suggestions.begin(), suggestions.end(), Xgreater1());
        else
            sort(suggestions.begin(), suggestions.end(), Xgreater2());

        if ((verbose == 0) && (suggestions.size() > 1))
            return std::vector<suggestItem>(suggestions.begin(), suggestions.begin() + 1);
        else
            return suggestions;
    }

    //from https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C.2B.2B
    double _WORKER_CLASS_::SpellCorrect::levenshtein_distance(const std::string &s1, const std::string &s2)
    {
        // To change the type this function manipulates and returns, change
        // the return type and the types of the two variables below.
        size_t s1len = s1.size();
        size_t s2len = s2.size();
        
        auto column_start = (decltype(s1len))1;
        
        auto column = new decltype(s1len)[s1len + 1];
        std::iota(column + column_start, column + s1len + 1, column_start);
        
        for (auto x = column_start; x <= s2len; x++) {
            column[0] = x;
            auto last_diagonal = x - column_start;
            for (auto y = column_start; y <= s1len; y++) {
                auto old_diagonal = column[y];
                auto possibilities = {
                    column[y] + 1,
                    column[y - 1] + 1,
                    last_diagonal + (s1[y - 1] == s2[x - 1]? 0 : 1)
                };
                column[y] = std::min(possibilities);
                last_diagonal = old_diagonal;
            }
        }
        auto result = column[s1len];
        delete[] column;
        return result;
    }

    double _WORKER_CLASS_::SpellCorrect::ocr_distance(const std::string& s1, const std::string& s2)
    {
        // Declare and initialize probability matrix for substrings (dynamic programming)
        size_t m = s1.size();
        size_t n = s2.size();
        
        //total costs needed to change s1 to s2
        std::vector< std::vector<double> > total_costs(m+1,std::vector<double>(n+1,std::numeric_limits<double>::max()));
        total_costs[0][0] = 0;

        // IMPORTANT: Note that strings are 0-indexed while they are 1-indexed in the paper!
        
        for (int i=0; i <= m; ++i)
        {
            for(int j=0; j <= n; ++j)
            {
                for(int p1=0; p1<=max_prefix_length && i+p1 <= m; ++p1)
                {
                    for(int p2=0; p2<=max_prefix_length && j+p2 <= n; ++p2)
                    {
                        double prefix_costs = (s1.substr(i,p1) != s2.substr(j,p2))?std::max(p1,p2):0;
                        if(substitution_costs.find({s1.substr(i,p1), s2.substr(j,p2)}) != substitution_costs.end())
                            prefix_costs = substitution_costs[{s1.substr(i,p1), s2.substr(j,p2)}];
                        if(prefix_costs < std::numeric_limits<double>::max())
                            prefix_costs += total_costs[i][j];
                        total_costs[i+p1][j+p2] = std::min(total_costs[i+p1][j+p2], prefix_costs);
                    }
                }
            }
        }
        return total_costs[m][n];
        
    }

    double _WORKER_CLASS_::SpellCorrect::DamerauLevenshteinDistance(const std::string &s1, const std::string &s2)
    {
        const size_t m(s1.size());
        const size_t n(s2.size());

        if (m == 0) return n;
        if (n == 0) return m;

        double *costs = new double[n + 1];

        for (size_t k = 0; k <= n; ++k) costs[k] = k;

        double i = 0;
        auto s1End = s1.end();
        auto s2End = s2.end();
        for (std::string::const_iterator it1 = s1.begin(); it1 != s1End; ++it1, ++i)
        {
            costs[0] = i + 1;
            double corner = i;

            size_t j = 0;
            for (std::string::const_iterator it2 = s2.begin(); it2 != s2End; ++it2, ++j)
            {
                double upper = costs[j + 1];
                if (*it1 == *it2)
                {
                    costs[j + 1] = corner;
                }
                else
                {
                    double t(upper < corner ? upper : corner);
                    costs[j + 1] = (costs[j] < t ? costs[j] : t) + 1;
                }

                corner = upper;
            }
        }

        double result = costs[n];
        delete[] costs;

        return result;
    }
    
    std::map< std::pair< std::string, std::string>, double > _WORKER_CLASS_::SpellCorrect::substitution_costs;    
    size_t _WORKER_CLASS_::SpellCorrect::max_prefix_length = 1;
}
