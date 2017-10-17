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

#ifndef DOCUMENTMODEL_H
#define DOCUMENTMODEL_H

#include <QAbstractListModel>

#include "couchdb.h"
#include "postrdata.h"

class QTimer;

class DocumentModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole,
        UserImageRole,
        PostrImageRole,
        UserDataRole,
        PostrDataRole,
        FullCopyRole
    };

    explicit DocumentModel(QObject *parent=nullptr, bool debug = false);
    virtual ~DocumentModel();
    
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    
signals:
    void dataAvailable();
    
public slots:
    void fetchData(const QModelIndex& index, bool force = false);
    void deleteDocument(const QModelIndex& index);
    
private:
    void fetchIds();
    void fetchQuality();
    inline QPixmap Image2QPixmap(const Postr::ImageData &inMat) const;
    inline QImage Image2QImage(const Postr::ImageData &inMat) const;
    
    mutable Postr::CouchDB m_engine;
    mutable std::vector<Postr::Data> m_userData;
    mutable std::map<std::string,Postr::Data> m_postrData;
    mutable std::vector<Postr::ImageData> m_userImage;
    mutable std::map<std::string,Postr::ImageData> m_postrImage;
    
    mutable std::set<std::string> m_processedDocs;
    mutable std::set<std::string> m_userEditedDocs;
    mutable std::set<std::string> m_title;
    mutable std::set<std::string> m_address;
    mutable std::set<std::string> m_time;
    
    QTimer *m_timer;
};


#endif
