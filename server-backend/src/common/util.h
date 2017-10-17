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

#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <string>
#include <sstream>

namespace Postr
{
     
    namespace File
    {
        /**
         * @brief Get the home directory of the current user 
         * @return the home directory path of the current user
         */
        std::string homeDirectory();
        
        //TODO: add paths for MS Windows and Android
        const std::vector<std::string> DataLocation = {"/usr/local/share/postr", "/usr/share/postr"};
        const std::vector<std::string> LibraryLocation = {"/usr/local/lib64", "/usr/lib64", "/usr/local/lib", "/usr/lib"};
        const std::vector<std::string> BinaryLocation = {"/usr/local/bin", "/usr/bin"};
        const std::vector<std::string> ConfigLocation = {homeDirectory()+"/.config", "/usr/local/share/postr", "/usr/share/postr", "/etc"};
        
        /**
         * @brief Locates a file matching a given name scheme and returns its full path.
         * @param filename A regular expression matching the name of the file to locate.
         * @param directories A list of directories to search in.
         * @param depth The maximum depth of subdirectories to search in. Set to negative for unbounded recursion.
         * @return The full path of the located file or an empty string.
         */
        std::string locate(const std::string& filename, const std::vector<std::string>& directories, int depth=0);
        
        
        /**
         * @brief Locates all files matching a given name scheme and returns the files full path.
         * @param filename A regular expression matching the name of the files to locate.
         * @param directories A list of directories to search in.
         * @param depth The maximum depth of subdirectories to search in. Set to negative for unbounded recursion.
         * @return A list containing the full path of each located file.
         */
        std::vector<std::string> locateAll(const std::string& filename, const std::vector<std::string>& directories, int depth=0);
        
        /**
         * @brief Determine if a given string matches a file path
         * @param filepath path to the file
         * @return true if a file with the given path exists and the file is not a directory 
         */
        bool exists(const std::string& filepath);
        
        /**
         * @brief Determine if a given string matches a directory path
         * @param dirpath path to the directory
         * @return true if a directory with the given path exists 
         */
        bool isDirectory(const std::string& dirpath);
        
        /**
         * @brief Returns the path to the directory of a file
         * @param filepath the full path including the file name
         * @return the path of the directory of the provided file path
         */
        std::string fileName(const std::string& filepath);
        
        /**
         * @brief Returns the path to the directory of a file
         * @param filepath the full path including the file name
         * @return the path of the directory of the provided file path
         */
        std::string directory(const std::string& filepath); 
    }
    
    
    namespace Output
    {
        /**
         * @brief Print a progress bar to stdout
         * @param taskname name printed in front of the progress bar
         * @param progress progress in percent
         * @param cr print a  carriage return at the end
         * @param barWidth width of the bar in characters
         */
        void printProgress(std::string taskname, double progress, bool cr = true, int barWidth = -1);
    }
    
    
    namespace Util 
    {
        /**
         * @brief Generate a UUID
         */
        std::string uuid();
    }
    
    
    namespace String
    {
        /**
         * @brief Replace all occurences of a string with another string
         * @param str string in wich all occurences of from should be replaced with to (haystack)
         * @param from string which will be replaced (needle)
         * @param to string, all occurences of from will be replaced with
         */
        void replaceAll(std::string& str, const std::string& from, const std::string& to);
        
        /**
         * @brief Transform umlauts and other non ASCII characters with an ASCII representation
         * @param str string in which all umlauts should be replaced
         */
        void transformUmlauts(std::string& str);
        
        /**
         * @brief undo the transformation done with \link transformUmlauts \endlink
         * @param str string in wich all umlaut representations should be replaced with real umlauts
         */
        void retransformUmlauts(std::string& str);
        
        /**
         * @brief check if there are repeated letters in a string.
         * This indicates that this string is not a real word
         * @param str string to check
         * @return true if no letter repeats more than two times
         */
        bool hasRepeatingLetters(const std::string& str);
        
        /**
         * @brief split a string at a given delimiter into chunks
         * @param s string to split 
         * @param delim delimiter 
         * @param result resulting chunks
         */
        template<typename Out>
        static void split(const std::string &s, char delim, Out result) {
            std::stringstream ss;
            ss.str(s);
            std::string item;
            while (std::getline(ss, item, delim)) {
                *(result++) = item;
            }
        }
        
        /**
         * @brief split a string at a given delimiter into chunks
         * @param s string to split
         * @param delim delimiter
         * @return resulting chunks
         */
        static std::vector<std::string> split(const std::string &s, char delim)
        {
            std::vector<std::string> elems;
            split(s, delim, std::back_inserter(elems));
            return elems;
        }
        
        /**
         * @brief enocode a string to be passed as a parameter in a URL 
         * @param s the string to encode
         */
        void urlEncode(std::string& s);
    }
    
}

#endif
