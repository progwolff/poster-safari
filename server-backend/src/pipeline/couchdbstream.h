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

#ifndef COUCHDB_STREAM_H
#define COUCHDB_STREAM_H

#include "stream.h"
#include "couchdb.h"

namespace Postr 
{
    /**
     * @brief A Stream fetching data from a CouchDB instance
     */
    class CouchDBStream final : public Stream
    {
    public:
        CouchDBStream(const std::string& url, const long port, const std::string& user, const std::string& pass, bool debug = false, bool dryrun = false);
        ~CouchDBStream();
        
        CouchDBStream& get(Data& data) override;
        /**
         * @brief Fetch a single document with given ID
         * Does not set the processedBy entry
         * @param id ID of the document
         * @return document with the given ID
         */
        Data get(std::string id);

        int count() const override;
        
        bool good() const override;
        
        bool is_open() const override;
        
        void close() override;
        
        void handleResult(const Data& data) override;
        
    private:
        
        void handleError(const Data& data) override;
        
        /**
         * @brief Converts an image in 'from' to an attachment in 'to' and appends it to to.meta
         * @return true on success, false else
         */
        bool attachImage(const std::string& name, const Data& from, Data& to);
        
        CouchDB m_engine;
        
        bool m_good;
        bool m_open;
        bool m_dryrun;
    };
}

#endif //COUCHDB_STREAM_H
