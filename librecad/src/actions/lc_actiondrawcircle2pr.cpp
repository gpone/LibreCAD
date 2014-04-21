/****************************************************************************
*
This file is part of the LibreCAD project, a 2D CAD program

Copyright (C) 2014 Dongxu Li (dongxuli2011 at gmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include "lc_actiondrawcircle2pr.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"



LC_ActionDrawCircle2PR::LC_ActionDrawCircle2PR(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
    :RS_ActionDrawCircleCR(container, graphicView)
{
    reset();
}



LC_ActionDrawCircle2PR::~LC_ActionDrawCircle2PR() {}


QAction* LC_ActionDrawCircle2PR::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
        // "Circle: 2 Points, Radius"
    QAction* action = new QAction(tr("2 Points, Radius"), NULL);
        action->setIcon(QIcon(":/extui/circle2pr.png"));
    return action;
}



void LC_ActionDrawCircle2PR::reset() {
    data.reset();
    data.radius=0.;
    point1 = RS_Vector(false);
    point2 = RS_Vector(false);
}



void LC_ActionDrawCircle2PR::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();
}



void LC_ActionDrawCircle2PR::trigger() {
    RS_PreviewActionInterface::trigger();

        RS_Circle* circle = new RS_Circle(container,
                                          data);
        circle->setLayerToActive();
        circle->setPenToActive();
        container->addEntity(circle);

        // upd. undo list:
        if (document!=NULL) {
            document->startUndoCycle();
            document->addUndoable(circle);
            document->endUndoCycle();
        }
        RS_Vector rz = graphicView->getRelativeZero();
                graphicView->redraw(RS2::RedrawDrawing);
        graphicView->moveRelativeZero(rz);
        drawSnapper();

        setStatus(SetPoint1);
        reset();
}


bool LC_ActionDrawCircle2PR::preparePreview(const RS_Vector& mouse) {
    const RS_Vector vp=(point1 + point2)*0.5;
    const RS_Vector dvp=RS_Vector(point1.angleTo(point2) + 0.5*M_PI)*sqrt(data.radius*data.radius-0.25*point1.squaredTo(point2));
    const RS_Vector center1= vp + dvp;
    const RS_Vector center2= vp - dvp;

    if( center1.squaredTo(center2) < RS_TOLERANCE ) {
        //no need to select center, as only one solution possible
        data.center=vp;
        return false;
    }

    const double ds=mouse.squaredTo(center1) - mouse.squaredTo(center2);
    if (ds < 0. ){
        data.center=center1;
        return true;
    }
    if (ds > 0. ){
        data.center=center2;
        return true;
    }
    data.center.valid=false;
    return false;

}



void LC_ActionDrawCircle2PR::mouseMoveEvent(QMouseEvent* e) {
    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
    case SetPoint1:
        point1 = mouse;
        break;

    case SetPoint2:
        if(mouse.distanceTo(point1) <= 2.*data.radius) point2 = mouse;
        break;

    case SelectCenter: {
        if(preparePreview(mouse)){
            RS_Circle* circle = new RS_Circle(preview, data);

            deletePreview();
            preview->addEntity(circle);
            drawPreview();
        }else{
            if(data.isValid()) trigger();
        }
    }
}
}



void LC_ActionDrawCircle2PR::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void LC_ActionDrawCircle2PR::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }
    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetPoint1:
        point1 = mouse;
        graphicView->moveRelativeZero(mouse);
        setStatus(SetPoint2);
        break;

    case SetPoint2:
        if(mouse.distanceTo(point1) <= 2.*data.radius){
            point2 = mouse;
            graphicView->moveRelativeZero(mouse);
            setStatus(SelectCenter);
        }else{
            RS_DIALOGFACTORY->commandMessage(tr("radius=%1 is too small for points selected\ndistance between points=%2 is larger than diameter=%3").
                                             arg(data.radius).arg(point1.distanceTo(point2)).arg(2.*data.radius));
        }
        break;

    case SelectCenter: {
        bool showPreview=preparePreview(mouse);
        if(showPreview || data.isValid())
            trigger();
        else
            RS_DIALOGFACTORY->commandMessage(tr("Select from two possible circle centers"));
    }
        break;

    default:
        break;
    }
}



void LC_ActionDrawCircle2PR::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }
}



QStringList LC_ActionDrawCircle2PR::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}



void LC_ActionDrawCircle2PR::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetPoint1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first point"),
                                            tr("Cancel"));
        break;
    case SetPoint2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second point"),
                                            tr("Back"));
        break;
    case SelectCenter:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select circle center"),
                                            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void LC_ActionDrawCircle2PR::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF

