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

#ifndef NAIVESEMANTICANALYSISWORKER_H
#define NAIVESEMANTICANALYSISWORKER_H

#include "asyncworker.h"


#include <thread>
#include <atomic>

#ifdef _WORKER_NAME_
#undef _WORKER_NAME_
#endif
#ifdef _WORKER_CLASS_
#undef _WORKER_CLASS_
#endif

#define _WORKER_NAME_ "Naive Semantic Analysis" 
#define _WORKER_CLASS_ NaiveSemanticAnalysisWorker

namespace Postr 
{
    /**
     * @brief This Worker tries to interpret extracted information in a semantic way
     */
    class _WORKER_CLASS_ final : public AsyncWorker
    {
    public:
        _WORKER_CLASS_();
        ~_WORKER_CLASS_();
    
    private:        
        void processAsync(Data& data) override;
    
        /**
         * @brief Returns the full name of a day given as a short form
         * @param day A day given as a string
         * @return The name of a day or an empty string if the given string could not be interpreted
         */
        std::string normalizeDay(const std::string& day);
        /**
         * @brief Returns a date in format DD.MM.YYYY 
         * @param date A date given as a string
         * @return A normalized date representation or an empty string if the given string could not be interpreted
         */
        std::string normalizeDate(const std::string& date);
        /**
         * @brief Returns a time in format MM:SS
         * @param time Time given as a string
         * @return A normalized time representation or an empty string if the given string could not be interpreted
         */
        std::string normalizeTime(const std::string& time);
        /**
         * @brief Returns a valid URL in format http://foo.bar/baz
         * @param url A url that may be incomplete given as a string
         * @return A valid URL
         */
        std::string normalizeURL(const std::string& url);
        /**
         * @brief Replaces month names with their corresponding numbers
         * @param date A date as a string, containing month names in lower case letters
         * @return The date with month names replaced by numbers
         */
        std::string replaceMonthNames(const std::string& date);
        /**
         * @brief Returns true if a given URL matches an existing host\n
         * Note that this method may return true even if the given url does not exist (404)
         * @param url A url
         * @return true if an existing host matches the given url
         */
        bool hostAvailable(const std::string& url);
        /**
         * @brief Returns a normalized address representation given an unformatted address
         * @param address An unformatted address
         * @return A MetaData object holding detailed information about this address
         */
        MetaData normalizeAddress(const std::string& address);
        /**
         * @brief Returns true if two rectangles given by the entries x,y,width and height overlap
         * @param a Object containing the entries x,y,width and height
         * @param b Object containing the entries x,y,width and height
         * @return true if the rectangles specified by a and b overlap
         */
        bool rectOverlap(const MetaData& a, const MetaData& b);
        /**
         * @brief Returns true if a given value is in inclusive range of min and max
         * @param value Value to check
         * @param min Lower bound of range
         * @param max Upper bound of range
         * @return true if min is less than or equal value and value is less than or equal max
         */
        bool inRange(int value, int min, int max);
        
        int currentYear;
        int currentMonth;
        int currentDay;
        
        static std::mutex m_timeMutex;
    };
};

#include "workerexport.h"

#endif //NAIVESEMANTICANALYSISWORKER_H
