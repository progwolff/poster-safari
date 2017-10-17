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

#include "util.h"
#include <dirent.h>
#include <regex>
#include <iostream>
#include <algorithm>

#ifdef _MSC_VER
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#endif

namespace Postr
{
    namespace File 
    {
        std::string homeDirectory()
        {
#ifdef _MSC_VER
            return "not implemented!";
#else
            const char *homedir;
            if ((homedir = getenv("HOME")) == NULL) {
                homedir = getpwuid(getuid())->pw_dir;
            }
            return homedir;
#endif
        }
        
        std::vector<std::string> _locate(const std::regex& filename, const std::vector<std::string>& directories, int depth, bool returnFirstMatch)
        {
            std::vector<std::string> result;
            
            for(const std::string& directoryname : directories)
            {
                std::string dirname = directoryname;
                String::replaceAll(dirname, "~", homeDirectory());
                
                DIR *dir;
                struct dirent *ent;
                if((dir = opendir(dirname.c_str())) != NULL) 
                {
                    std::vector<std::string> subdirs;
                    while((ent = readdir(dir)) != NULL) 
                    {
                        if(ent->d_type == DT_DIR && ent->d_name != std::string(".") && ent->d_name != std::string(".."))
                            subdirs.push_back(ent->d_name);
                        else if(regex_match(ent->d_name,filename))
                        {
                            result.push_back(dirname+"/"+ent->d_name);
                            if(returnFirstMatch)
                                break;
                        }
                    }
                    closedir(dir);
                    
                    if(depth != 0)
                    {
                        for(const std::string& subdirname : subdirs)
                        {
                            const std::vector<std::string>& subdirresult = _locate(filename, {dirname+"/"+subdirname}, depth-1, returnFirstMatch);
                            result.insert(result.end(),subdirresult.begin(),subdirresult.end());
                            if(returnFirstMatch && !result.empty())
                                break;
                        }
                    }
                } 
                else 
                {
                    /* could not open directory */
                    continue;
                }
                
                if(returnFirstMatch && !result.empty())
                    break;
            }

            return result;
        }
        
        std::string locate(const std::string& filename, const std::vector<std::string>& directories, int depth)
        {
            const std::vector<std::string>& result = _locate(std::regex(filename), directories, depth, true);
            if(result.empty())
                return "";
            else
                return result[0];
        }
        
        std::vector<std::string> locateAll(const std::string& filename, const std::vector<std::string>& directories, int depth)
        {
            return _locate(std::regex(filename), directories, depth, false);
        }
        
        bool isDirectory(const std::string& dirpath)
        {
            std::string path = dirpath;
            String::replaceAll(path, "~", homeDirectory());
            DIR *dir;
            if((dir = opendir(path.c_str())) != NULL) 
            {
                closedir(dir);
                return true;
            }
            return false;
        }
        
        bool exists(const std::string& path)
        {
            std::string dirpath = directory(path);
            std::string filename = fileName(path);
            
            if(dirpath.empty())
                dirpath = ".";
            if(filename.empty())
            {
                filename = path;
                dirpath = ".";
            }
            
            String::replaceAll(dirpath, "~", homeDirectory());
            
            DIR *dir;
            struct dirent *ent;
            if((dir = opendir(dirpath.c_str())) != NULL) 
            {
                while((ent = readdir(dir)) != NULL) 
                {
                    if(ent->d_name == filename)
                    {
                        closedir(dir);
                        return true;
                    }
                }
                closedir(dir);
            }
            return false;
        }
        
        std::string fileName(const std::string& filepath)
        {
            size_t splitpos = filepath.find_last_of('/');
            return filepath.substr(splitpos+1);
        }

        std::string directory(const std::string& filepath)
        {
            size_t splitpos = filepath.find_last_of('/');
            return filepath.substr(0, splitpos);
        }


    }
    
    
    namespace Output 
    {
        int consoleWidth()
        {
            int cols = 80;
            int lines = 24;

#ifdef TIOCGSIZE
            struct ttysize ts;
            ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
            cols = ts.ts_cols;
            lines = ts.ts_lines;
#elif defined(TIOCGWINSZ)
            struct winsize ts;
            ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
            cols = ts.ws_col;
            lines = ts.ws_row;
#endif /* TIOCGSIZE */

            return cols;
        }
        
        void printProgress(std::string taskname, double progress, bool cr, int barWidth)
        {
            std::string line = "";
            
            static int width = consoleWidth();
            
            if(barWidth < 0)
                barWidth = width;
            
            static int nameWidth = 0;
            if(taskname.size() > nameWidth)
                nameWidth = taskname.size();
    
            line += taskname;
            for(int i=taskname.size(); i<nameWidth; ++i)
                line += " ";
            line += ": [";
            
            std::string number = ""; 
            if(int(progress) < 10)
                number += " ";
            if(int(progress) < 100)
                number += " ";
            number += std::to_string(int(progress));
            number += "%";
            
            int linewidth = line.length();
            
            barWidth -= number.length() + linewidth + 3;
            int pos = barWidth * progress/100.;
            
            for (int i = 0; i < barWidth; ++i) {
                if (i < pos) 
                    line += "=";
                else if (i == pos) 
                    line += ">";
                else 
                    line += " ";
            }
            line += "] ";
            line += number+" ";
            
            std::cout << line;
            if(cr)
                std::cout << "\r";
            std::cout.flush();
        }
    }
    
    
    namespace Util 
    {
        std::string uuid()
        {
            #ifdef _MSC_VER
                UUID uuid;
                UuidCreate ( &uuid );

                unsigned char * str;
                UuidToStringA ( &uuid, &str );

                std::string s( ( char* ) str );

                RpcStringFreeA ( &str );
            #else
                uuid_t uuid;
                uuid_generate_random ( uuid );
                char s[37];
                uuid_unparse ( uuid, s );
            #endif
                return s;
        }
    }
    
    
    namespace String
    {
        void replaceAll(std::string& str, const std::string& from, const std::string& to)
        {
            std::string::size_type start_pos = 0;
            while((start_pos = str.find(from, start_pos)) != std::string::npos) 
            {
                str.replace(start_pos, from.length(), to);
                start_pos += to.length(); 
            }
        }
        
        void transformUmlauts(std::string& str)
        {
            replaceAll(str, "ä", "'a");
            replaceAll(str, "ö", "'o");
            replaceAll(str, "ü", "'u");
            replaceAll(str, "Ä", "'A");
            replaceAll(str, "Ö", "'O");
            replaceAll(str, "Ü", "'U");
            replaceAll(str, "ß", "'s");
            replaceAll(str, "€", "'e");
            replaceAll(str, "$", "'t");
        }
        
        void retransformUmlauts(std::string& str)
        {
            replaceAll(str, "'a", "ä");
            replaceAll(str, "'o", "ö");
            replaceAll(str, "'u", "ü");
            replaceAll(str, "'A", "Ä");
            replaceAll(str, "'O", "Ö");
            replaceAll(str, "'U", "Ü");
            replaceAll(str, "'s", "ß");
            replaceAll(str, "'e", "€");
            replaceAll(str, "'t", "$");
        }
        
        void urlEncode(std::string& str)
        {
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);
            
            replaceAll(str, "%", "%25");
            replaceAll(str, " ", "%20");
            replaceAll(str, "!", "%21");
            replaceAll(str, "\"", "%22");
            replaceAll(str, "#", "%23");
            replaceAll(str, "$", "%24");
            replaceAll(str, "&", "%26");
            replaceAll(str, "'", "%27");
            replaceAll(str, "(", "%28");
            replaceAll(str, ")", "%29");
            replaceAll(str, "*", "%2A");
            replaceAll(str, "+", "%2B");
            replaceAll(str, ",", "%2C");
            replaceAll(str, "/", "%2F");
            replaceAll(str, ":", "%3A");
            replaceAll(str, ";", "%3B");
            replaceAll(str, "=", "%3D");
            replaceAll(str, "?", "%3F");
            replaceAll(str, "@", "%40");
            replaceAll(str, "[", "%5B");
            replaceAll(str, "\\", "%5C");
            replaceAll(str, "]", "%5D");
            replaceAll(str, "{", "%7B");
            replaceAll(str, "|", "%7C");
            replaceAll(str, "}", "%7D");
            
            replaceAll(str, "ä", "%C3%A4");
            replaceAll(str, "ö", "%C3%B6");
            replaceAll(str, "ü", "%C3%BC");
            replaceAll(str, "ß", "%C3%9F");
        }
        
        bool hasRepeatingLetters(const std::string& str)
        {
            for(size_t i = 0; i < str.size()-2; ++i)
            {
                if(str[i] == str[i+1] && str[i] == str[i+2])
                    return true;
            }
            return false;
        }

    }
}
