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

#ifndef COUCHDB_H
#define COUCHDB_H

#include "postrdata.h"

namespace Postr 
{
    class CouchDB 
    {
    public:
        /**
         * @brief Constructor
         * @param url URL of the database server
         * @param port Port of the database server
         * @param user Username used with the database server
         * @param pass Password used with the database server
         * @param debug If true, debug versions of tables will be used
         */
        explicit CouchDB(const std::string& url, const long port, const std::string& user, const std::string& pass, bool debug = false);
        ~CouchDB();
        
        int count(bool interactive = false) const;
        
        /**
         * @brief Return the id and revision number of the next document available in DB
         * @param id ID of the retreived document
         * @param rev Revision of the retreived document
         * @param interactive show progress bar for http transaction
         * @return true on success, false else
         */
        bool getNextDocument(std::string& id, std::string& rev, bool interactive = false);
        
        /**
         * @brief Return the id and revision number of the document at given index available in DB
         * @param index index of the document to retreive
         * @param id ID of the retreived document
         * @param rev Revision of the retreived document
         * @param interactive show progress bar for http transaction
         * @return true on success, false else
         */
        bool getDocumentId(int index, std::string& id, std::string& rev, bool interactive = false);
        
        /**
         * @brief Return the id and revision number of all documents available in DB
         * @param ids IDs of the retreived documents
         * @param revs Revisions of the retreived documents
         * @param interactive show progress bar for http transaction
         * @return true on success, false else
         */
        bool getAllDocumentIds(std::vector<std::string>& ids, std::vector<std::string>& revs, bool interactive = false);
        
        /**
         * @brief Fetch ids and revision numbers for all documents that match a given selector
         * @param selector The selector all documents must match
         * @param limit Maximum number of documents in result
         * @param interactive show progress bar for http transaction
         * @return An array of objects each containing an id and a revision number
         */
        MetaData getSelectedDocumentIds(const std::string& selector, int limit = 25, bool interactive = false);
        
        /**
         * @brief Fetch ids and revision numbers for all documents that have an event that matches a given selector
         * @param selector The selector all events must match
         * @param limit Maximum number of documents in result
         * @param interactive show progress bar for http transaction
         * @return An array of objects each containing an id and a revision number
         */
        MetaData getSelectedDocumentIdsByEvents(const std::string& selector, int limit = 25, bool interactive = false);
        
        /**
         * @brief Fetch a document available in DB
         * @param id ID of the document
         * @param rev Revision of the document, may be left empty
         * @param data Data fetched from DB
         * @param withAttachments Include attachments
         * @param interactive show progress bar for http transaction
         * @return A new revision string for the document or an empty string on error
         */
        std::string fetchDocument(const std::string& id, const std::string& rev, Data& data, bool withAttachments = false, bool interactive = true);
        
        /**
         * @brief Fetch an attachment 
         * @param id id of the document to fetch an attachment from
         * @param rev revision of the document to fetch an attachment from
         * @param name name of the attachment
         * @param interactive show progress bar for http transaction
         * @param fromEvent fetch Attachment from eventDB instead of posterDB
         * @return base64 encoded attachment or an empty string
         */
        std::string fetchAttachment(const std::string& id, const std::string& rev, const std::string& name, bool interactive = true, bool fromEvent = false);
        
        /**
         * @brief Update a document in DB
         * @param id ID of the document
         * @param data Data to write to DB. The _rev field will be updated.
         * @param interactive show progress bar for http transaction
         * @return true on success, false else
         */
        bool updateDocument(const std::string& id, Data& data, bool interactive = false);
        
        /**
         * Creates a new event in DB
         * @param data event data
         * @param interactive show progress bar for http transaction
         * @return id of the new event
         */
        std::string createEvent(Data& data, bool interactive = false);
        
        /**
         * Updates or creates a new event in DB
         * @param data event data
         * @param interactive show progress bar for http transaction
         * @return id of the new event
         */
        std::string updateEvent(Data& data, bool interactive = false);
        
        /**
         * @brief Fetch an event available in DB
         * @param id ID of the event
         * @param rev Revision of the event, may be left empty
         * @param data Data fetched from DB
         * @param interactive show progress bar for http transaction
         * @return A new revision string for the event or an empty string on error
         */
        std::string fetchEvent(const std::string& id, const std::string& rev, Data& data, bool interactive = false);
        
        /**
         * Get the ID of this engine
         * @return the ID of this engine
         */
        std::string id() const;
        
    private:
        /**
         * @brief Progress callback for http transactions
         */
        double progressCallback(double dltotal, double dlnow, double ultotal, double ulnow) const;
        
        /**
         * @brief Returns the name of the events table in DB
         */
        inline std::string eventDB() const
        {
            return (m_debugDB?"debug_event":"event");
        };
        
        /**
         * @brief Returns the name of the poster table in DB
         */
        inline std::string posterDB() const
        {
            return (m_debugDB?"debug_poster":"poster");
        }
    
        std::string m_id;
        std::string m_url;
        long m_port;
        std::string m_user;
        std::string m_pass;
        bool m_debugDB;
    };
}

#endif
