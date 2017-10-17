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

#include "symspellworker.h"

#include <iostream>

int main(int argc, char** argv)
{
    if(argc < 2)
        std::cout << "usage: postr_pipeline text..." << std::endl;
    
    Postr::Data data;
    
    data.meta["text"][0]["text"] = "Lubeck";
    
    for(int i=1; i<argc; ++i)
    {
        data.meta["text"][i]["text"] = argv[i];
    }
    
    Postr::SymSpellWorker symspellworker;
    int status = symspellworker.processBlocking(data);
    
    std::cout << data.serialize(false);
    
    return status;
}
