/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <qapplication.h>
# include <qevent.h>
# include <qpainter.h>
# include <qpixmap.h>
# include <QGLFramebufferObject>
# include <QMenu>
# include <Inventor/SbBox.h>
# include <Inventor/events/SoEvent.h>
# include <Inventor/events/SoKeyboardEvent.h>
# include <Inventor/events/SoLocation2Event.h>
# include <Inventor/events/SoMouseButtonEvent.h>
#endif

#include <Base/Console.h>

#include "MouseSelection.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"

using namespace Gui;

AbstractMouseSelection::AbstractMouseSelection() : _pcView3D(0)
{
    m_bInner = true;
    mustRedraw = false;
}

void AbstractMouseSelection::grabMouseModel(Gui::View3DInventorViewer* viewer)
{
    _pcView3D = viewer;
    m_cPrevCursor = _pcView3D->getWidget()->cursor();

    // do initialization of your mousemodel
    initialize();
}

void AbstractMouseSelection::releaseMouseModel()
{
    if (_pcView3D) {
        // do termination of your mousemodel
        terminate();

        _pcView3D->getWidget()->setCursor(m_cPrevCursor);
        _pcView3D = 0;
    }
}

void AbstractMouseSelection::redraw()
{
    // Note: For any reason it does not work to do a redraw in the actualRedraw() method of the
    // viewer class. So, we do the redraw when the user continues moving the cursor. E.g. have
    // a look to PolyPickerSelection::draw()
    mustRedraw = true;
}

int AbstractMouseSelection::handleEvent(const SoEvent* const ev, const SbViewportRegion& vp)
{
    int ret=Continue;

    const SbVec2s& sz = vp.getWindowSize();
    short w,h;
    sz.getValue(w,h);

    SbVec2s loc = ev->getPosition();
    short x,y;
    loc.getValue(x,y);
    y = h-y; // the origin is at the left bottom corner (instead of left top corner)

    if (ev->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const SoMouseButtonEvent* const event = (const SoMouseButtonEvent*) ev;
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? TRUE : FALSE;

        if (press) {
            _clPoly.push_back(ev->getPosition());
            ret = mouseButtonEvent(static_cast<const SoMouseButtonEvent*>(ev), QPoint(x,y));
        }
        else {
            ret = mouseButtonEvent(static_cast<const SoMouseButtonEvent*>(ev), QPoint(x,y));
        }
    }
    else if (ev->getTypeId().isDerivedFrom(SoLocation2Event::getClassTypeId())) {
        ret = locationEvent(static_cast<const SoLocation2Event*>(ev), QPoint(x,y));
    }
    else if (ev->getTypeId().isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        ret = keyboardEvent(static_cast<const SoKeyboardEvent*>(ev));
    }

    if (ret == Restart)
        _clPoly.clear();

    return ret;
}

// -----------------------------------------------------------------------------------

BaseMouseSelection::BaseMouseSelection()
    : AbstractMouseSelection()
{
}

// -----------------------------------------------------------------------------------
#if 0
/* XPM */
static const char* cursor_polypick[]= {
    "32 32 2 1",
    "# c #646464",
    ". c None",
    "................................",
    "................................",
    ".......#........................",
    ".......#........................",
    ".......#........................",
    "................................",
    ".......#........................",
    "..###.###.###...................",
    ".......#...............#........",
    "......................##........",
    ".......#..............#.#.......",
    ".......#.............#..#.......",
    ".......#............#...#.......",
    "....................#....#......",
    "...................#.....#......",
    "..................#......#......",
    "............#.....#.......#.....",
    "...........#.##..#........#.....",
    "..........#....##.........#.....",
    ".........#...............#......",
    "........#................#......",
    ".......#................#.......",
    "......#.................#.......",
    ".....#.................#........",
    "....#####..............#........",
    ".........#########....#.........",
    "..................#####.........",
    "................................",
    "................................",
    "................................",
    "................................",
    "................................"
};

/* XPM */
static const char* cursor_scissors[]= {
    "32 32 3 1",
    "# c #000000",
    "+ c #ffffff",
    ". c None",
    "....+...........................",
    "....+...........................",
    "....+...........................",
    "................................",
    "+++.+.+++.......................",
    "................................",
    "....+...........................",
    "....+...................#####...",
    "....+.................########..",
    ".....................#########..",
    ".....###............##########..",
    "....##++##.........#####...###..",
    "...#++++++##.......####...####..",
    "...##+++++++#......####.######..",
    ".....#+++++++##....##########...",
    "......##+++++++##.##########....",
    "........##+++++++#########......",
    "..........#+++++++#####.........",
    "...........##+++++####..........",
    "...........##+++++###...........",
    ".........##+++++++########......",
    "........##+++++++###########....",
    "......##+++++++##.###########...",
    "....##+++++++##....##########...",
    "...#+++++++##......####..#####..",
    "...#++++++#........#####..####..",
    "....##++##..........#####..###..",
    "......#.............##########..",
    ".....................#########..",
    ".......................######...",
    "................................",
    "................................"
};
#endif
static const char* cursor_cut_scissors[]= {
    "32 32 6 1",
    "a c #800000",
    "c c #808080",
    "+ c #c0c0c0",
    "b c #ff0000",
    "# c #ffffff",
    ". c None",
    "....#...........................",
    "....#...........................",
    "....#...........................",
    "................................",
    "###.#.###.......................",
    "................................",
    "....#...........................",
    "....#...................aaaaa...",
    "....#.................aabbbbba..",
    ".....................abbbbbbba..",
    ".....ccc............abbaaaaabb..",
    "....cc++cc.........babaa...aba..",
    "...c+#++++cc.......abba...abba..",
    "...cc+#+++++c......abba.aabbaa..",
    ".....c+++++#+cc....abbaaabbaa...",
    "......cc+#+++#+cc.aabbbbbbaa....",
    "........cc+#+++#+cabbbaaaa......",
    "..........c+++++++abbaa.........",
    "...........cc+++#+aaaa..........",
    "...........cc+#+++caa...........",
    ".........cc+++++#+cbbaaaaa......",
    "........cc+#+++#+cabbabbbaaa....",
    "......cc+#+++#+cc.aaabbbbbbaa...",
    "....cc+#+++#+cc....abbaaaabba...",
    "...c++#++#+cc......abba..aabba..",
    "...c+###++c........aabaa..aaba..",
    "....cc++cc..........abbaa..aba..",
    "......c.............aabbaaaaba..",
    ".....................baabbbbba..",
    ".......................aaaaaa...",
    "................................",
    "................................"
};

PolyPickerSelection::PolyPickerSelection()
{
}

void PolyPickerSelection::initialize()
{
    QPixmap p(cursor_cut_scissors);
    QCursor cursor(p, 4, 4);
    _pcView3D->getWidget()->setCursor(cursor);

    polyline.setViewer(_pcView3D);
    polyline.setColor(0.0,0.0,1.0,1.0);
    
    _pcView3D->addGraphicsItem(&polyline);
    _pcView3D->setRenderType(View3DInventorViewer::Image);
    _pcView3D->redraw();
}

void PolyPickerSelection::terminate()
{
    _pcView3D->removeGraphicsItem(&polyline);
    _pcView3D->setRenderType(View3DInventorViewer::Native);
    _pcView3D->redraw();
}

void PolyPickerSelection::draw()
{
    _pcView3D->redraw();
}

PolyPickerSelection::~PolyPickerSelection()
{
}

int PolyPickerSelection::popupMenu()
{
    QMenu menu;
    QAction* fi = menu.addAction(QObject::tr("Finish"));
    menu.addAction(QObject::tr("Clear"));
    QAction* ca = menu.addAction(QObject::tr("Cancel"));

    if(getPositions().size() < 3)
        fi->setEnabled(false);

    QAction* id = menu.exec(QCursor::pos());

    if (id == fi)
        return Finish;
    else if (id == ca)
        return Cancel;
    else
        return Restart;
}

int PolyPickerSelection::mouseButtonEvent(const SoMouseButtonEvent* const e, const QPoint& pos)
{
    const int button = e->getButton();
    const SbBool press = e->getState() == SoButtonEvent::DOWN ? TRUE : FALSE;

    if(press) {
        switch(button)
        {
        case SoMouseButtonEvent::BUTTON1:
        {
            if (!polyline.isWorking()) {
                polyline.setWorking(true);
                polyline.clear();
            };
            polyline.addNode(pos);
            polyline.setCoords(pos.x(), pos.y());
            m_iXnew = pos.x();  m_iYnew = pos.y();
            m_iXold = pos.x();  m_iYold = pos.y();
        }
        break;

        case SoMouseButtonEvent::BUTTON2:
        {
             polyline.addNode(pos);
             m_iXnew = pos.x();  m_iYnew = pos.y();
             m_iXold = pos.x();  m_iYold = pos.y();
        }
        break;

        default:
        {
        }   break;
        }
    }
    // release
    else {
        switch(button)
        {
        case SoMouseButtonEvent::BUTTON2:
        {
            QCursor cur = _pcView3D->getWidget()->cursor();
            _pcView3D->getWidget()->setCursor(m_cPrevCursor);

            // The pop-up menu should be shown when releasing mouse button because
            // otherwise the navigation style doesn't get the UP event and gets into
            // an inconsistent state.
            int id = popupMenu();

            if (id == Finish || id == Cancel) {
                releaseMouseModel();
            }
            else if (id == Restart) {
                _pcView3D->getWidget()->setCursor(cur);
            }

            polyline.setWorking(false);
            return id;
        }
        break;

        default:
        {
        }   break;
        }
    }

    return Continue;
}

int PolyPickerSelection::locationEvent(const SoLocation2Event* const e, const QPoint& pos)
{
    // do all the drawing stuff for us
    QPoint clPoint = pos;

    if (polyline.isWorking()) {
        // check the position
        QRect r = _pcView3D->getGLWidget()->rect();

        if (!r.contains(clPoint)) {
            if (clPoint.x() < r.left())
                clPoint.setX(r.left());

            if (clPoint.x() > r.right())
                clPoint.setX(r.right());

            if (clPoint.y() < r.top())
                clPoint.setY(r.top());

            if (clPoint.y() > r.bottom())
                clPoint.setY(r.bottom());

#ifdef FC_OS_WINDOWS
            QPoint newPos = _pcView3D->getGLWidget()->mapToGlobal(clPoint);
            QCursor::setPos(newPos);
#endif
        }
        polyline.setCoords(clPoint.x(), clPoint.y());
    }
    
    draw();
    m_iXnew = clPoint.x();
    m_iYnew = clPoint.y();

    return Continue;
}

int PolyPickerSelection::keyboardEvent(const SoKeyboardEvent* const e)
{
    return Continue;
}

// -----------------------------------------------------------------------------------

PolyClipSelection::PolyClipSelection()
{
}

PolyClipSelection::~PolyClipSelection()
{
}

int PolyClipSelection::popupMenu()
{
    QMenu menu;
    QAction* ci = menu.addAction(QObject::tr("Inner"));
    QAction* co = menu.addAction(QObject::tr("Outer"));
    QAction* ca = menu.addAction(QObject::tr("Cancel"));

    if (getPositions().size() < 3) {
        ci->setEnabled(false);
        co->setEnabled(false);
    }

    QAction* id = menu.exec(QCursor::pos());

    if (id == ci) {
        m_bInner = true;
        return Finish;
    }
    else if (id == co) {
        m_bInner = false;
        return Finish;
    }
    else if (id == ca)
        return Cancel;
    else
        return Restart;
}

// -----------------------------------------------------------------------------------

BrushSelection::BrushSelection()
{
}


BrushSelection::~BrushSelection()
{

}

void BrushSelection::setColor(float r, float g, float b, float a)
{
    polyline.setColor(r,g,b,a);
}

void BrushSelection::setLineWidth(float l)
{
    polyline.setLineWidth(l);
}

void BrushSelection::setClosed(bool on)
{
   //TODO: closed = false is not supported yet
}

int BrushSelection::popupMenu()
{
    QMenu menu;
    QAction* fi = menu.addAction(QObject::tr("Finish"));
    menu.addAction(QObject::tr("Clear"));
    QAction* ca = menu.addAction(QObject::tr("Cancel"));

    if(getPositions().size() < 3)
        fi->setEnabled(false);

    QAction* id = menu.exec(QCursor::pos());

    if (id == fi)
        return Finish;
    else if (id == ca)
        return Cancel;
    else
        return Restart;
}

int BrushSelection::locationEvent(const SoLocation2Event* const e, const QPoint& pos)
{
    // do all the drawing stuff for us
    QPoint clPoint = pos;

    if (polyline.isWorking()) {
        // check the position
        QRect r = _pcView3D->getGLWidget()->rect();

        if (!r.contains(clPoint)) {
            if (clPoint.x() < r.left())
                clPoint.setX(r.left());

            if (clPoint.x() > r.right())
                clPoint.setX(r.right());

            if (clPoint.y() < r.top())
                clPoint.setY(r.top());

            if (clPoint.y() > r.bottom())
                clPoint.setY(r.bottom());
        }

        SbVec2s last = _clPoly.back();
        SbVec2s curr = e->getPosition();

        if (abs(last[0]-curr[0]) > 20 || abs(last[1]-curr[1]) > 20)
            _clPoly.push_back(curr);

        polyline.addNode(clPoint);
        polyline.setCoords(clPoint.x(), clPoint.y());
    }

    m_iXnew = clPoint.x();
    m_iYnew = clPoint.y();
    draw();
    m_iXold = clPoint.x();
    m_iYold = clPoint.y();

    return Continue;
}

// -----------------------------------------------------------------------------------

RectangleSelection::RectangleSelection() : RubberbandSelection()
{
    rubberband.setColor(0.0,0.0,1.0,1.0);
}

RectangleSelection::~RectangleSelection()
{
}

// -----------------------------------------------------------------------------------

RubberbandSelection::RubberbandSelection()
{
}

RubberbandSelection::~RubberbandSelection()
{
}

void RubberbandSelection::initialize()
{
    rubberband.setViewer(_pcView3D);
    rubberband.setWorking(false);
    rubberband.setColor(1.0, 1.0, 0.0, 0.5);
    _pcView3D->addGraphicsItem(&rubberband);
    if (QGLFramebufferObject::hasOpenGLFramebufferObjects()) {
        _pcView3D->setRenderType(View3DInventorViewer::Image);
    }
    _pcView3D->redraw();
}

void RubberbandSelection::terminate()
{
    _pcView3D->removeGraphicsItem(&rubberband);
    if (QGLFramebufferObject::hasOpenGLFramebufferObjects()) {
        _pcView3D->setRenderType(View3DInventorViewer::Native);
    }
    _pcView3D->redraw();
}

void RubberbandSelection::draw()
{
    _pcView3D->redraw();
}

int RubberbandSelection::mouseButtonEvent(const SoMouseButtonEvent* const e, const QPoint& pos)
{
    const int button = e->getButton();
    const SbBool press = e->getState() == SoButtonEvent::DOWN ? TRUE : FALSE;

    int ret = Continue;

    if (press) {
        switch(button)
        {
        case SoMouseButtonEvent::BUTTON1:
        {
            rubberband.setWorking(true);
            m_iXold = m_iXnew = pos.x();
            m_iYold = m_iYnew = pos.y();
        }
        break;

        default:
        {
        }   break;
        }
    }
    else {
        switch(button) {
        case SoMouseButtonEvent::BUTTON1:
        {
            rubberband.setWorking(false);
            releaseMouseModel();
            _clPoly.push_back(e->getPosition());
            ret = Finish;
        }
        break;

        default:
        {
        }   break;
        }
    }

    return ret;
}

int RubberbandSelection::locationEvent(const SoLocation2Event* const e, const QPoint& pos)
{
    m_iXnew = pos.x();
    m_iYnew = pos.y();
    rubberband.setCoords(m_iXold, m_iYold, m_iXnew, m_iYnew);
    draw();
    return Continue;
}

int RubberbandSelection::keyboardEvent(const SoKeyboardEvent* const e)
{
    return Continue;
}

// -----------------------------------------------------------------------------------

BoxZoomSelection::BoxZoomSelection()
{
}

BoxZoomSelection::~BoxZoomSelection()
{
}

void BoxZoomSelection::terminate()
{
    RubberbandSelection::terminate();

    int xmin = std::min<int>(m_iXold, m_iXnew);
    int xmax = std::max<int>(m_iXold, m_iXnew);
    int ymin = std::min<int>(m_iYold, m_iYnew);
    int ymax = std::max<int>(m_iYold, m_iYnew);
    SbBox2s box(xmin, ymin, xmax, ymax);
    _pcView3D->boxZoom(box);
}

