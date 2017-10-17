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

#ifndef SpellCorrectWORKER_H
#define SpellCorrectWORKER_H

#include "asyncworker.h"

#define USE_GOOGLE_DENSE_HASH_MAP
//#define ENABLE_TEST
//#define IO_OPERATIONS

#include <thread>
#include <atomic>

#ifdef _WORKER_NAME_
#undef _WORKER_NAME_
#endif
#ifdef _WORKER_CLASS_
#undef _WORKER_CLASS_
#endif

#define _WORKER_NAME_ "SpellCorrect" 
#define _WORKER_CLASS_ SpellCorrectWorker

namespace Postr 
{
    /**
     * @brief This Worker tries to correct the spelling of words based on word lists and substitution cost maps
     */
    class _WORKER_CLASS_ final : public AsyncWorker
    {
    public:
        _WORKER_CLASS_();
        ~_WORKER_CLASS_();
    
    private:        
        class SpellCorrect;
        
        void processAsync(Data& data) override;
        void initAsync() override;
        /**
         * @brief Write a default cost map to a file 
         * @param fname the name of the file to write to
         */
        void writeDefaultCostFile(const std::string& fname);
        
        std::map<std::string,SpellCorrect> m_spell;
    };
};

#include "workerexport.h"

#endif //SpellCorrectWORKER_H
