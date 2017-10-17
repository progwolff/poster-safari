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

#include <qtextstream.h>
#include "editor.h"
#include "ui_editor.h"

#include "documentmodel.h"
#include "base64.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QSystemTrayIcon>

#include <opencv2/imgcodecs.hpp>

editor::editor(QWidget *parent, bool debug)
    : QMainWindow(parent)
    , m_ui(new Ui::editor)
    , m_documentModel(new DocumentModel(this, debug))
    , m_scene(nullptr)
    , m_scene2(nullptr)
    , m_userDataValid(false)
    , m_postrDataValid(false)
    , m_scale(1.)
    , m_scale2(1.)
{
    qRegisterMetaType< QVector<int> >("QVector<int>");
    
    m_ui->setupUi(this);
    
    setWindowIcon(QIcon::fromTheme("postersafari"));
    
    m_tray = new QSystemTrayIcon(QIcon::fromTheme("postersafari"), this);
    if(m_tray->supportsMessages())
        m_tray->show();
    
    m_ui->documentList->setModel(m_documentModel);
    
    connect(m_ui->documentList, SIGNAL(activated(QModelIndex)), m_documentModel, SLOT(fetchData(QModelIndex)));
    connect(m_ui->documentList, SIGNAL(clicked(QModelIndex)), m_documentModel, SLOT(fetchData(QModelIndex)));
    
    connect(m_ui->documentList, &QListView::activated, this, [this](QModelIndex index){
        m_currentIndex = index;
        update();
    });
    
    connect(m_ui->documentList, &QListView::clicked, this, [this](QModelIndex index){
        m_currentIndex = index;
        update();
    });
    
    connect(m_documentModel, &DocumentModel::dataChanged, this, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles){
        Q_UNUSED(roles);
        if(topLeft.row() <= m_currentIndex.row() && bottomRight.row() >= m_currentIndex.row())
            update();
    });
    
    connect(m_ui->userDataEdit, &QTextEdit::textChanged, this, [this]{
        try 
        {
            Json::Value d;
            Json::Reader reader;
            if(!reader.parse(m_ui->userDataEdit->toPlainText().toStdString(), d))
                throw(Json::LogicError("invalid"));
            m_userDataValid = true;
            m_ui->userDataEdit->setStyleSheet("color: #000000;");
        }
        catch(Json::Exception e)
        {
            m_userDataValid = false;
            m_ui->userDataEdit->setStyleSheet("color: #ff0000;");
        }
    });
    
    connect(m_ui->postrDataEdit, &QTextEdit::textChanged, this, [this]{
        try 
        {
            Json::Value d;
            Json::Reader reader;
            if(!reader.parse(m_ui->postrDataEdit->toPlainText().toStdString(), d))
                throw(Json::LogicError("invalid"));
            m_postrDataValid = true;
            m_ui->postrDataEdit->setStyleSheet("color: #000000;");
        }
        catch(Json::Exception e)
        {
            m_postrDataValid = false;
            m_ui->postrDataEdit->setStyleSheet("color: #ff0000;");
        }
    });
    
    connect(m_ui->actionDelete, &QAction::triggered, this, [this]{
        if(!m_currentIndex.isValid())
            return;
        QMessageBox msgBox;
        msgBox.setText("Delete document");
        msgBox.setInformativeText("Do you really want to delete this document?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        int ret = msgBox.exec();
        if(ret == QMessageBox::Yes)
        {
            m_documentModel->deleteDocument(m_currentIndex);
            if(m_currentIndex.row() >= m_documentModel->rowCount(m_currentIndex))
                m_currentIndex = m_documentModel->index(m_currentIndex.row()-1, m_currentIndex.column());
            m_ui->documentList->setCurrentIndex(m_currentIndex);
            m_documentModel->fetchData(m_currentIndex, true);
        }
    });
    
    
    connect(m_ui->actionProcess, SIGNAL(triggered()), this, SLOT(process()));
    
    connect(m_ui->actionSaveDB, SIGNAL(triggered()), this, SLOT(saveUserData()));
    
    connect(m_ui->actionReload, &QAction::triggered, this, [this]{
        if(!m_currentIndex.isValid())
            return;
        m_documentModel->fetchData(m_currentIndex, true);
    });
    
    connect(m_ui->actionSaveJson, &QAction::triggered, this, [this]{
        if(!m_currentIndex.isValid())
            return;
        if(!m_userDataValid)
        {
            QMessageBox msgBox;
            msgBox.setText("Save document");
            msgBox.setInformativeText("This document has invalid json data. Not saving this document.");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            return;
        }
        Postr::Data d(m_ui->userDataEdit->toPlainText().toStdString());
        QString fileName = QFileDialog::getSaveFileName(this, "Save File", m_documentModel->data(m_currentIndex, DocumentModel::IdRole).toString(), "Json Documents (*.json)");
        QFile file(fileName);
        if(file.open(QIODevice::ReadWrite))
        {
            QTextStream stream(&file);
            stream << m_documentModel->data(m_currentIndex, DocumentModel::FullCopyRole).toString();
        }
        else
        {
            QMessageBox msgBox;
            msgBox.setText("Save document");
            msgBox.setInformativeText("Cannot write to file.");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            return;
        }
    });
    
    connect(m_ui->actionRotate90, &QAction::triggered, this, [this]{
        if(!m_currentIndex.isValid())
            return;
        QString data = m_documentModel->data(m_currentIndex, DocumentModel::FullCopyRole).toString();
        Postr::Data d(data.toStdString());
        if(d.meta["_attachments"].isMember("userimage"))
            rotateBy(d, "userimage", CW90);
        if(d.meta["_attachments"].isMember("userimage_thumb"))
            rotateBy(d, "userimage_thumb", CW90);
        m_ui->userDataEdit->setText(QString::fromStdString(d.serialize(true,true)));
        saveUserData();
        m_documentModel->fetchData(m_currentIndex, true);
    });
    
    connect(m_ui->actionRotate180, &QAction::triggered, this, [this]{
        if(!m_currentIndex.isValid())
            return;
        QString data = m_documentModel->data(m_currentIndex, DocumentModel::FullCopyRole).toString();
        Postr::Data d(data.toStdString());
        if(d.meta["_attachments"].isMember("userimage"))
            rotateBy(d, "userimage", CW180);
        if(d.meta["_attachments"].isMember("userimage_thumb"))
            rotateBy(d, "userimage_thumb", CW180);
        m_ui->userDataEdit->setText(QString::fromStdString(d.serialize(true,true)));
        saveUserData();
        m_documentModel->fetchData(m_currentIndex, true);
    });
    
    connect(m_ui->actionRotate270, &QAction::triggered, this, [this]{
        if(!m_currentIndex.isValid())
            return;
        QString data = m_documentModel->data(m_currentIndex, DocumentModel::FullCopyRole).toString();
        Postr::Data d(data.toStdString());
        if(d.meta["_attachments"].isMember("userimage"))
            rotateBy(d, "userimage", CCW90);
        if(d.meta["_attachments"].isMember("userimage_thumb"))
            rotateBy(d, "userimage_thumb", CCW90);
        m_ui->userDataEdit->setText(QString::fromStdString(d.serialize(true,true)));
        saveUserData();
        m_documentModel->fetchData(m_currentIndex, true);
    });
    
    connect(m_ui->documentImageViewZoom, &QSlider::valueChanged, this, [this](double val){
       m_scale = val/100.;
       scale(m_scale);
    });
    
    connect(m_ui->documentImageView2Zoom, &QSlider::valueChanged, this, [this](double val){
       m_scale2 = val/100.;
       scale2(m_scale2);
    });
    
    connect(m_documentModel, &DocumentModel::dataAvailable, this, [this]{
        notify("New posters available!");
    });
}

editor::~editor()
{
    delete m_ui;
    delete m_documentModel;
    delete m_tray;
}

void editor::update()
{
    if(!m_currentIndex.isValid())
        return;
    
    delete m_scene;
    m_scene = new QGraphicsScene(this);
    m_scene->addPixmap(m_documentModel->data(m_currentIndex, DocumentModel::UserImageRole).value<QPixmap>());
    m_ui->documentImageView->setScene(m_scene);
    m_scale = std::min(m_ui->documentImageView->width() / m_scene->width(), m_ui->documentImageView->height() / m_scene->height());
    m_ui->documentImageViewZoom->setValue(m_scale*100);
    scale(m_scale);
    
    delete m_scene2;
    m_scene2 = new QGraphicsScene(this);
    m_scene2->addPixmap(m_documentModel->data(m_currentIndex, DocumentModel::PostrImageRole).value<QPixmap>());
    m_ui->documentImageView2->setScene(m_scene2);
    m_scale2 = std::min(m_ui->documentImageView2->width() / m_scene2->width(), m_ui->documentImageView2->height() / m_scene2->height());
    m_ui->documentImageView2Zoom->setValue(m_scale2*100);
    scale2(m_scale2);
    
    if(m_scene2->width() == 0)
    {
        m_ui->documentImageView2->hide();
        m_ui->documentImageView2Zoom->hide();
    }
    else
    {
        m_ui->documentImageView2->show();
        m_ui->documentImageView2Zoom->show();
    }
    
    m_ui->userDataEdit->setText(m_documentModel->data(m_currentIndex, DocumentModel::UserDataRole).toString());
    
    m_ui->postrDataEdit->setText(m_documentModel->data(m_currentIndex, DocumentModel::PostrDataRole).toString());
}

void editor::scale(double f)
{
    m_ui->documentImageView->setTransform(QTransform::fromScale(f,f));
    m_ui->documentImageView->centerOn(m_ui->documentImageView->scene()->width()/2., m_ui->documentImageView->scene()->height()/2.);
}

void editor::scale2(double f)
{
    m_ui->documentImageView2->setTransform(QTransform::fromScale(f,f));
    m_ui->documentImageView2->centerOn(m_ui->documentImageView2->scene()->width()/2., m_ui->documentImageView2->scene()->height()/2.);
}

void editor::saveUserData()
{
    if(!m_currentIndex.isValid())
        return;
    if(!m_userDataValid)
    {
        QMessageBox msgBox;
        msgBox.setText("Update document");
        msgBox.setInformativeText("This document has invalid json data. Not updating this document.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return;
    }
    m_documentModel->setData(m_currentIndex, m_ui->userDataEdit->toPlainText(), DocumentModel::UserDataRole);
    if(m_postrDataValid)
        m_documentModel->setData(m_currentIndex, m_ui->postrDataEdit->toPlainText(), DocumentModel::PostrDataRole);
    m_documentModel->fetchData(m_currentIndex, true);
}


void editor::rotateBy(Postr::Data& data, const std::string& name, editor::ROTATEBY rotateBy)
{
    if(!data.meta["_attachments"].isMember(name))
        return;
    std::string image = data.meta["_attachments"][name]["data"].asString();
    image = image.substr(image.find_first_of(':')+1);
    image = image.substr(image.find_first_of(',')+1);
    std::string decoded = base64_decode(image);
    std::vector<uchar> imdata(decoded.begin(), decoded.end());

    cv::Mat img = cv::imdecode(cv::Mat(imdata),cv::IMREAD_ANYCOLOR);
    
    switch(rotateBy)
    {
        case CW90:
            cv::transpose(img, img);  
            cv::flip(img, img, 1);
            break;
        case CCW90:
            cv::transpose(img, img);  
            cv::flip(img, img, 0);
            break;
        case CW180:
            cv::flip(img, img, -1);
            break;
    }
    
    std::vector<uchar> encoded;
    cv::imencode(".jpg", img, encoded);
    image = base64_encode(encoded.data(), encoded.size());
    
    data.meta["_attachments"][name]["data"] = image;
}

void editor::process()
{
    if(!m_currentIndex.isValid())
        return;
    QProcess *exec = new QProcess(this);
    exec->setEnvironment(QProcess::systemEnvironment());

    QModelIndex index = m_currentIndex;
    connect(exec, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), [this,index]{
        m_documentModel->fetchData(index, true);
    });
    connect(exec, SIGNAL(finished(int, QProcess::ExitStatus)), exec, SLOT(deleteLater()));
    
    connect(exec, &QProcess::readyReadStandardOutput, this, [exec]{
        std::cout << QString::fromLatin1(exec->readAllStandardOutput()).toStdString();
    });
    connect(exec, &QProcess::readyReadStandardError, this, [exec]{
        std::cerr << QString::fromLatin1(exec->readAllStandardError()).toStdString();
    });
    
    exec->start("postersafari", QStringList() << m_documentModel->data(m_currentIndex, DocumentModel::IdRole).toString());
}

void editor::notify(const QString& message)
{
    if(m_tray->supportsMessages())
        m_tray->showMessage("Poster Safari", message, QSystemTrayIcon::Information);
}

void editor::closeEvent(QCloseEvent *event)
{
    if(m_tray)
        m_tray->hide();
}

