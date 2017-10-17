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

#ifndef EDITOR_H
#define EDITOR_H

#include <QMainWindow>
#include <QModelIndex>

#include "postrdata.h"

namespace Ui {
class editor;
}

class DocumentModel;
class QGraphicsScene;
class QSystemTrayIcon;

class editor : public QMainWindow
{
    Q_OBJECT

public:
    explicit editor(QWidget *parent = 0, bool debug = false);
    ~editor();

private:
    enum ROTATEBY {CW90, CW180, CCW90};
    void closeEvent(QCloseEvent *event) override;
    
private slots:
    void saveUserData();
    void update();
    void rotateBy(Postr::Data& data, const std::string& name, ROTATEBY rotateBy);
    void scale(double f);
    void scale2(double f);
    void process();
    void notify(const QString& message);

private:    
    QModelIndex m_currentIndex;
    Ui::editor *m_ui;
    DocumentModel *m_documentModel;
    QGraphicsScene *m_scene,*m_scene2;
    QSystemTrayIcon *m_tray;
    
    bool m_userDataValid;
    bool m_postrDataValid;
    double m_scale,m_scale2;
};

#endif // EDITOR_H
