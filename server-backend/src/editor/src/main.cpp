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

#include "editor.h"
#include <QApplication>

#include "easylogging++.h"

#include "argagg.hpp"

INITIALIZE_EASYLOGGINGPP

int main(int argc, char *argv[])
{
    argagg::parser argparser {{
        { "help", {"-h", "--help"},
        "shows this help message", 0},
        { "debugdb", {"-b", "--debugdb"},
        "use debug tables in database", 0},
    }};
    
    argagg::parser_results args;
    try {
        args = argparser.parse(argc, argv);
    } catch (const std::exception& e) {
        LOG(ERROR) << e.what();
        return 1;
    }
        
    if (args["help"]) 
    {
        std::cout << "Usage: postr-processing [options] [images or IDs...]" << std::endl << argparser;
        return 0;
    }
    
    bool debugDB = false;
    if(args["debugdb"])
        debugDB = true;
    
    QApplication app(argc, argv);
    editor w(nullptr, debugDB);
    w.show();

    return app.exec();
}

