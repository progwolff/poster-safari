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

#ifdef WORKER_LIBRARY
#undef WORKER_LIBRARY //make sure we don't export symbols twice
#endif
#include "naivesemanticanalysis.h"

/**
 * parts of this code are taken from
 * http://www.ping127001.com/pingpage/ping.text
 */

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>

#include <arpa/inet.h>

#include <unistd.h>

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	64
#endif

std::string hostName(const std::string& url)
{
    size_t pos,start,length;
    start = 0;
    length = url.length();
    pos = url.find("://");
    if(pos != url.npos)
        start = pos+3;
    pos = url.find('/', start);
    if(pos != url.npos)
        length = pos-1-start;
    return url.substr(start, length);
}

namespace Postr 
{
    bool _WORKER_CLASS_::hostAvailable(const std::string& url)
    {
        struct hostent *hp;/* Pointer to host info */

        struct sockaddr whereto;/* Who to ping */
        
        struct sockaddr_in *to = (struct sockaddr_in *) &whereto;
        
        std::string host = hostName(url);
        
        bzero((char *)&whereto, sizeof(struct sockaddr) );
        to->sin_family = AF_INET;
        to->sin_addr.s_addr = inet_addr(host.c_str());
        if(to->sin_addr.s_addr != (unsigned)-1) {
            return true;
        } else {
            hp = gethostbyname(host.c_str());
            if (hp) {
                return true;
            } else {
                return false;
            }
        }
        return true;
    }
}
