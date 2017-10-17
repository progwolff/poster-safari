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

#ifndef REGEXWORKER_H
#define REGEXWORKER_H

#include "asyncworker.h"

#include <map>
#include <regex>

#include <thread>
#include <atomic>

#ifdef _WORKER_NAME_
#undef _WORKER_NAME_
#endif
#ifdef _WORKER_CLASS_
#undef _WORKER_CLASS_
#endif

#define _WORKER_NAME_ "Regex Worker" 
#define _WORKER_CLASS_ RegexWorker

namespace Postr 
{
    /**
     * @brief This Worker tries to find information in texts by using regular expressions
     */
    class _WORKER_CLASS_ final : public AsyncWorker
    {
    public:
        _WORKER_CLASS_();
        ~_WORKER_CLASS_();
    
    private:        
        void processAsync(Data& data) override;
        
        std::map<std::string,std::regex> m_regex;
    };
};

#include "workerexport.h"

#endif //REGEXWORKER_H 
