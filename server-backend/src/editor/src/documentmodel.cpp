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

#include "documentmodel.h"

#include <QPixmap>
#include <QImage>
#include <QIcon>
#include <QtConcurrent>
#include <QTimer>
#include <QPalette>
#include <QApplication>

#include <opencv2/imgcodecs.hpp>

#include "base64.h"

DocumentModel::DocumentModel(QObject* parent, bool debug)
    : QAbstractListModel(parent)
    , m_engine("cloud42.homenet.org", 5984, "chef", "fallstudienChef!", debug)
{
    m_userData = std::vector<Postr::Data>(m_engine.count());
    m_userImage = std::vector<Postr::ImageData>(m_engine.count());
    
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [this]{
        QtConcurrent::run(QThreadPool::globalInstance(), [this]{
            fetchIds();
        });
    });
    m_timer->start(10000);
    QtConcurrent::run(QThreadPool::globalInstance(), [this]{
        fetchIds();
        fetchQuality();
    });
    
}

DocumentModel::~DocumentModel()
{
    m_timer->deleteLater();
}

void DocumentModel::fetchIds()
{
    int count = m_userData.size();
    
    int newcount = m_engine.count();
    
    beginInsertRows(index(0,0), 0, newcount);
    
    m_userData.resize(newcount);
    m_userImage.resize(newcount);
    
    std::vector<std::string> id,rev;
    if(m_engine.getAllDocumentIds(id, rev))
    {
        for(int i=0; i < id.size(); ++i)
        {
            if(m_userData[i].meta.isNull())
            {
                m_userData[i].meta["_id"] = id[i];
                m_userData[i].meta["_rev"] = rev[i];
            }
        }
    }
    
    std::string selector = "{                                        "
                           "     \"$and\" : [{                       "
                           "         \"event\" : {                   "
                           "              \"$and\":[{                "
                           "                  \"$exists\" : true     "
                           "              },{                        "
                           "                  \"$ne\" : null         "
                           "              }]                         "
                           "         }                               "
                           "     }]                                  "
                           "}                                        ";
    m_processedDocs.clear();
    for(const Postr::MetaData& doc : m_engine.getSelectedDocumentIds(selector, m_userData.size()))
    {
        m_processedDocs.insert(doc["_id"].asString());
    }
        
    selector =             "{                                        "
                           "     \"$and\" : [{                       "
                           "         \"userEvent\" : {               "
                           "              \"$and\":[{                "
                           "                  \"$exists\" : true     "
                           "              },{                        "
                           "                  \"$ne\": {}            "
                           "              },{                        "
                           "                  \"$ne\" : null         "
                           "              }]                         "
                           "         }                               "
                           "     }]                                  "
                           "}                                        ";
    m_userEditedDocs.clear();
    for(const Postr::MetaData& doc : m_engine.getSelectedDocumentIds(selector, m_userData.size()))
    {
        m_userEditedDocs.insert(doc["_id"].asString());
    }
    
    endInsertRows();
    emit dataChanged(index(0,0), index(newcount,0));
    
    if(newcount > count)
    {
        fetchQuality();
        emit dataAvailable();
    }
}

void DocumentModel::fetchQuality()
{
    std::string selector = "{                                        "
                           "     \"$and\" : [{                       "
                           "         \"title\" : {                   "
                           "              \"$and\":[{                "
                           "                  \"$exists\" : true     "
                           "              },{                        "
                           "                  \"$ne\": {}            "
                           "              },{                        "
                           "                  \"$ne\" : null         "
                           "              }]                         "
                           "         }                               "
                           "     }]                                  "
                           "}                                        ";
    
    m_title.clear();
    for(const Postr::MetaData& doc : m_engine.getSelectedDocumentIdsByEvents(selector, m_userData.size()))
    {
        m_title.insert(doc["_id"].asString());
    }
    LOG(INFO) << "documents with a title: " << m_title.size();
    
    selector =             "{                                        "
                           "     \"$and\" : [{                       "
                           "         \"longitude\" : {               "
                           "              \"$and\":[{                "
                           "                  \"$exists\" : true     "
                           "              },{                        "
                           "                  \"$ne\": {}            "
                           "              },{                        "
                           "                  \"$ne\" : null         "
                           "              }]                         "
                           "         }                               "
                           "     }]                                  "
                           "}                                        ";
    m_address.clear();
    for(const Postr::MetaData& doc : m_engine.getSelectedDocumentIdsByEvents(selector, m_userData.size()))
    {
        m_address.insert(doc["_id"].asString());
    }
    LOG(INFO) << "documents with a address: " << m_address.size();
    
    selector =             "{                                        "
                           "     \"$and\" : [{                       "
                           "         \"times\" : {                   "
                           "              \"$and\":[{                "
                           "                  \"$exists\" : true     "
                           "              },{                        "
                           "                  \"$ne\": {}            "
                           "              },{                        "
                           "                  \"$ne\" : null         "
                           "              }]                         "
                           "         }                               "
                           "     }]                                  "
                           "}                                        ";
    m_time.clear();
    for(const Postr::MetaData& doc : m_engine.getSelectedDocumentIdsByEvents(selector, m_userData.size()))
    {
        m_time.insert(doc["_id"].asString());
    }
    LOG(INFO) << "documents with a date: " << m_time.size();
}

void DocumentModel::fetchData(const QModelIndex& index, bool force)
{
    if(!index.isValid())
        return;
    
    if(index.row() >= m_userData.size())
        return;
    
    int i = index.row();
    
    if(m_userData[i].meta.isMember("_attachments") && !force)
    {
        //nothing to do here
        return;
    }
    
    m_userData[i].meta.removeMember("_rev");
    m_engine.fetchDocument(m_userData[i].meta["_id"].asString(), "", m_userData[i], false);
    
    if(m_userData[i].meta.isMember("event") && m_userData[i].meta["event"].isString() && ( m_postrData.find(m_userData[i].meta["event"].asString()) == m_postrData.end() || force))
    {
        std::string key = m_userData[i].meta["event"].asString();
        Postr::Data d;
        m_engine.fetchEvent(key, "", d);
        m_postrData[key] = d;
    }
    
    emit dataChanged(index, index);
    
    std::string image;
    image = m_engine.fetchAttachment(m_userData[i].meta["_id"].asString(), m_userData[i].meta["_rev"].asString(), "userimage_thumb", true);
    if(image.empty())
        image = m_engine.fetchAttachment(m_userData[i].meta["_id"].asString(), m_userData[i].meta["_rev"].asString(), "userimage", true);
    
    image = image.substr(image.find_first_of(':')+1);
    //std::string type = image.substr(0,image.find_first_of(';'));
    image = image.substr(image.find_first_of(',')+1);

    std::string decoded = base64_decode(image);
    
    std::vector<uchar> imdata(decoded.begin(), decoded.end());

    if(!imdata.empty())
        m_userImage[i] = cv::imdecode(cv::Mat(imdata),cv::IMREAD_ANYCOLOR);
    
    emit dataChanged(index, index);
    
    if(m_userData[i].meta.isMember("event") && m_userData[i].meta["event"].isString() && ( m_postrImage.find(m_userData[i].meta["event"].asString()) == m_postrImage.end() || force))
    {
        std::string key = m_userData[i].meta["event"].asString();
        
        std::string image;
        image = m_engine.fetchAttachment(m_postrData[key].meta["_id"].asString(),m_postrData[key].meta["_rev"].asString(), "best", true, true);
        
        image = image.substr(image.find_first_of(':')+1);
        //std::string type = image.substr(0,image.find_first_of(';'));
        image = image.substr(image.find_first_of(',')+1);

        std::string decoded = base64_decode(image);
        
        std::vector<uchar> imdata(decoded.begin(), decoded.end());

        if(!imdata.empty())
            m_postrImage[key] = cv::imdecode(cv::Mat(imdata),cv::IMREAD_ANYCOLOR);
        
        emit dataChanged(index, index);
    }
}

QVariant DocumentModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
        return QVariant();
    
    if(index.row() >= m_userData.size())
        return QVariant();
    
    Postr::Data d = m_userData[index.row()];
    
    QIcon ico;
    QBrush brush;
    QColor color;

    bool userDataAvailable, serverDataAvailable, hasTitle, hasAddress, hasTimes;
    if(role != IdRole)
    {
        userDataAvailable = m_userEditedDocs.find(data(index, IdRole).toString().toStdString()) != m_userEditedDocs.end();
        serverDataAvailable = m_processedDocs.find(data(index, IdRole).toString().toStdString()) != m_processedDocs.end();
        hasTitle =  m_title.find(data(index, IdRole).toString().toStdString()) != m_title.end();
        hasAddress =  m_address.find(data(index, IdRole).toString().toStdString()) != m_address.end();
        hasTimes =  m_time.find(data(index, IdRole).toString().toStdString()) != m_time.end();
    }
    
    switch(role) {
    case Qt::DisplayRole:
    case IdRole:
        if(d.meta.isMember("_id"))
            return QString::fromStdString(d.meta["_id"].asString());
        break;
    case Qt::DecorationRole:
        if(userDataAvailable && serverDataAvailable)
            ico = QIcon::fromTheme("user-available");
        else if(userDataAvailable)
            ico = QIcon::fromTheme("user-idle");
        else if(serverDataAvailable)
            ico = QIcon::fromTheme("mail-replied");
        return ico;
    case Qt::BackgroundRole:
        color = QApplication::palette().base().color();
        if(serverDataAvailable)
        {
            double quality = (hasTitle?1./3.:0) + (hasAddress?1./3.:0) + (hasTimes?1./3.:0);
            color = QColor::fromRgbF(
                    color.redF() * (quality) + (1.-quality),
                    color.greenF() * (quality) + 0,
                    color.blueF() * (quality) + 0);
            brush = QBrush(color);
        }
        return brush;
    case UserImageRole:
        return Image2QPixmap(m_userImage[index.row()]);
    case PostrImageRole:
        if(d.meta.isMember("event") && d.meta["event"].isString() && m_postrImage.find(d.meta["event"].asString()) != m_postrImage.end())
            return Image2QPixmap(m_postrImage[d.meta["event"].asString()]);
    case UserDataRole:
        return QString::fromStdString(d.serialize(false, true));
    case PostrDataRole:
        if(d.meta.isMember("event") && d.meta["event"].isString() && m_postrData.find(d.meta["event"].asString()) != m_postrData.end())
            return QString::fromStdString(m_postrData[d.meta["event"].asString()].serialize(false, true));
        break;
    case FullCopyRole:
        if(d.meta.isMember("_id") && d.meta.isMember("_rev"))
        {
            m_engine.fetchDocument(d.meta["_id"].asString(), d.meta["_rev"].asString(), d, true);
            return QString::fromStdString(d.serialize(true, true));
        }
        break;
    }

    return QVariant();
}

Qt::ItemFlags DocumentModel::flags(const QModelIndex& index) const
{
    if(m_userData[index.row()].meta.isMember("_id"))
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable |  Qt::ItemIsEditable;
    return Qt::NoItemFlags;
}

int DocumentModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_userData.size();
}

int DocumentModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool DocumentModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    switch(role)
    {
        case UserDataRole:
            m_userData[index.row()].assign(value.toString().toStdString());
            m_engine.updateDocument(m_userData[index.row()].meta["_id"].asString(), m_userData[index.row()]);
            break;
        case PostrDataRole:
            Postr::Data d = m_userData[index.row()];
            if(d.meta.isMember("event") && d.meta["event"].isString())
            {
                if(value.toString() == data(index, PostrDataRole).toString())
                    break;
                m_postrData[d.meta["event"].asString()] = Postr::Data(value.toString().toStdString());
                d.meta["event"] = m_engine.updateEvent(m_postrData[d.meta["event"].asString()]);
            }
            else
            {
                m_postrData[d.meta["event"].asString()] = Postr::Data(value.toString().toStdString());
                d.meta["event"] = m_engine.updateEvent(m_postrData[d.meta["event"].asString()]);
            }
            m_engine.updateDocument(m_userData[index.row()].meta["_id"].asString(), m_userData[index.row()]);
            break;
    }
    QtConcurrent::run(QThreadPool::globalInstance(), [this]{
        fetchIds();
        fetchQuality();
    });
}

void DocumentModel::deleteDocument(const QModelIndex& index)
{
    fetchData(index);
    m_userData[index.row()].meta["_deleted"] = true;
    m_engine.updateDocument(m_userData[index.row()].meta["_id"].asString(), m_userData[index.row()]);
    m_userData.erase(m_userData.begin()+index.row());
    m_userImage.erase(m_userImage.begin()+index.row());
    emit dataChanged(index, createIndex(rowCount(QModelIndex())-1,0));
}

inline QImage DocumentModel::Image2QImage(const Postr::ImageData &inMat) const
{
    switch(inMat.type())
    {
        // 8-bit, 4 channel
        case CV_8UC4:
        {
            QImage image(inMat.data, inMat.cols, inMat.rows, static_cast<int>(inMat.step), QImage::Format_ARGB32);
            return image;
        }

        // 8-bit, 3 channel
        case CV_8UC3:
        {
            QImage image(inMat.data, inMat.cols, inMat.rows, static_cast<int>(inMat.step), QImage::Format_RGB888); 
            return image.rgbSwapped();
        }

        // 8-bit, 1 channel
        case CV_8UC1:
        {
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
            QImage image(inMat.data, inMat.cols, inMat.rows, static_cast<int>(inMat.step), QImage::Format_Grayscale8);
#else
            static QVector<QRgb> sColorTable;

            // only create our color table the first time
            if(sColorTable.isEmpty())
            {
                sColorTable.resize( 256 );
                for(int i = 0; i < 256; ++i)
                {
                    sColorTable[i] = qRgb(i, i, i);
                }
            }

            QImage image(inMat.data, inMat.cols, inMat.rows, static_cast<int>(inMat.step), QImage::Format_Indexed8);
            image.setColorTable( sColorTable );
#endif
            return image;
        }

        default:
            LOG(WARNING) << "ASM::cvMatToQImage() - cv::Mat image type not handled in switch:" << inMat.type();
        break;
    }

    return QImage();
}

inline QPixmap DocumentModel::Image2QPixmap(const Postr::ImageData &inMat) const
{
    return QPixmap::fromImage(Image2QImage(inMat));
}
