/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef TECHDRAWGUI_QGITILE_H
#define TECHDRAWGUI_QGITILE_H

#include <QFont>
#include <QPointF>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QPainterPath>
#include <QColor>
#include <QGraphicsColorizeEffect>

#include <Base/Vector3D.h>

#include "QGIArrow.h"
#include "QGCustomText.h"
#include "QGCustomRect.h"
#include "QGCustomSvg.h"
#include "QGIDecoration.h"

namespace TechDraw {
class DrawTile;
class DrawTileWeld;
}

namespace TechDrawGui
{

class TechDrawGuiExport QGITile : public QGIDecoration
{
public:
    explicit QGITile(TechDraw::DrawTile* tileFeat);
    ~QGITile(void) {}

    enum {Type = QGraphicsItem::UserType + 325};
    int type(void) const { return Type;}

    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
    virtual QRectF boundingRect() const;

    void setTileTextLeft(std::string s);
    void setTileTextRight(std::string s);
    void setTileTextCenter(std::string s);
    void setFont(QFont f, double fsize);
    void setSymbolFile(std::string s);
    void setTilePosition(QPointF org, int row, int col);
    void setTileScale(double s);
//    double getSymbolScale(void) const;
    virtual void draw(void);

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    QColor getTileColor(void) const;
    void setPrettyNormal();
    void setPrettyPre();
    void setPrettySel();

    double getSymbolWidth(void) const;
    double getSymbolHeight(void) const;
    double getSymbolFactor(void) const;
    QString getTextFont(void) const;
    double getFontSize(void) const;
    double scaleToFont(void) const;
    void makeSymbol(void);
    void makeText(void);

private:
    TechDraw::DrawTile*  m_tileFeat;
    QGCustomText*      m_qgTextL;
    QGCustomText*      m_qgTextR;
    QGCustomText*      m_qgTextC;
    QGCustomSvg*       m_qgSvg;
    QGraphicsColorizeEffect* m_effect;
    QString            m_svgPath;
    QString            m_textL;
    QString            m_textR;
    QString            m_textC;
    QString            m_fontName;
    QFont              m_font;
    double             m_textSize;
    int                m_row;
    int                m_col;
    QPointF            m_origin;
    double             m_wide;
    double             m_high;
    double             m_scale;
};

}

#endif // TECHDRAWGUI_QGITILE_H
