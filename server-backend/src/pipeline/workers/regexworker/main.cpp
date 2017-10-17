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

#include <iostream>

int main(int /*argc*/, char** argv)
{
    std::ifstream ifs(argv[1], std::ifstream::in);
    
    std::string str;
    std::string file_contents;
    while (std::getline(ifs, str))
    {
        file_contents += str;
        file_contents.push_back('\n');
    } 
    
    Postr::Data data(file_contents);
    
    Postr::RegexWorker worker;
    
    Postr::Worker::interactive = false;
    int status = worker.processBlocking(data);
    
    std::cout << data;
    
    return status;
}
