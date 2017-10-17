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

#ifndef SpellCorrect_H
#define SpellCorrect_H

#include "spellcorrectworker.h"

#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <math.h>
#include <chrono>
#include <fstream>
#include <list>
#include <numeric>
#include <unordered_map>

#ifdef IO_OPERATIONS
#include <msgpack.hpp>
#endif

#ifdef USE_GOOGLE_DENSE_HASH_MAP
#include <dense_hash_map>
#include <dense_hash_set>

#define CUSTOM_SET google::dense_hash_set
#define CUSTOM_MAP google::dense_hash_map
#else
#include <map>
#include <set>

#define CUSTOM_SET std::set
#define CUSTOM_MAP std::map
#endif

#define getHastCode(term) std::hash<std::string>()(term)

namespace Postr
{

    /**
     * @brief Spelling correction
     */
    class SpellCorrectWorker::SpellCorrect 
    {
        
    public:
        enum DISTANCE_FUNCT{ LEVENSTEIN, DAMERAU_LEVENSTEIN, OCR_OPTIMIZED };
        
        /**
         * @brief A dictionary item used by SpellCorrect
         */
        class dictionaryItem 
        {
        public:
            std::vector<size_t> suggestions;
            size_t count = 0;

            dictionaryItem(size_t c)
            {
                count = c;
            }

            dictionaryItem()
            {
                count = 0;
            }
            ~dictionaryItem()
            {
                suggestions.clear();
            }

        #ifdef IO_OPERATIONS
            MSGPACK_DEFINE(suggestions, count);
        #endif
        };

        enum ItemType { NONE, DICT, INTEGER };

        #ifdef IO_OPERATIONS
        MSGPACK_ADD_ENUM(ItemType);
        #endif

        /**
         * @brief A container holding dictionary items
         */
        class dictionaryItemContainer
        {
        public:
            dictionaryItemContainer() : itemType(NONE), intValue(0)
            {
            }

            ItemType itemType;
            size_t intValue;
            std::shared_ptr<dictionaryItem> dictValue;

        #ifdef IO_OPERATIONS
            MSGPACK_DEFINE(itemType, intValue, dictValue);
        #endif
        };

        /**
         * @brief An item that has been suggested as a possible correction
         */
        class suggestItem
        {
        public:
            std::string term;
            double distance = 0;
            unsigned short count;

            bool operator== (const suggestItem & item) const
            {
                return term.compare(item.term) == 0;
            }

            size_t HastCode() const
            {
                return std::hash<std::string>()(term);
            }

        #ifdef IO_OPERATIONS
            MSGPACK_DEFINE(term, distance, count);
        #endif
        };
        
        DISTANCE_FUNCT distalg = LEVENSTEIN;
        size_t verbose = 0;
        size_t editDistanceMax = 2;
        
        static std::map< std::pair< std::string, std::string>, double > substitution_costs;

        SpellCorrect();

        void LoadCostFile(const std::string fname, const char seperator = ',');
            
        void CreateDictionary(std::string corpus);
    #ifdef IO_OPERATIONS

        void Save(string filePath);

        void Load(string filePath);

    #endif

        bool CreateDictionaryEntry(std::string key);

        std::vector<suggestItem> Correct(std::string input);
        
        static double levenshtein_distance(const std::string &s1, const std::string &s2);
        
        static double ocr_distance(const std::string& s1, const std::string& s2);

        static double DamerauLevenshteinDistance(const std::string &s1, const std::string &s2);

    private:
        size_t maxlength = 0;
        CUSTOM_MAP<size_t, dictionaryItemContainer> dictionary;
        std::vector<std::string> wordlist;
        static size_t max_prefix_length;

        std::vector<std::string> parseWords(std::string text) const;

        void AddLowestDistance(std::shared_ptr<dictionaryItem> const & item, std::string suggestion, size_t suggestionint, std::string del);
        
        void Edits(std::string word, CUSTOM_SET<std::string> & deletes) const;

        std::vector<suggestItem> Lookup(std::string input, size_t editDistanceMax);

        struct Xgreater1
        {
            bool operator()(const suggestItem& lx, const suggestItem& rx) const {
                return lx.count > rx.count;
            }
        };

        struct Xgreater2
        {
            bool operator()(const suggestItem& lx, const suggestItem& rx) const {
                auto cmpForLx = 2 * (lx.distance - rx.distance) - (lx.count - rx.count);
                auto cmpForRx = 2 * (rx.distance - lx.distance) - (rx.count - lx.count);
                return cmpForLx > cmpForRx;
            }
        };
    };
}

#endif
