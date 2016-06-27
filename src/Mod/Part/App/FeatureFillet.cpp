/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepFilletAPI_MakeFillet.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif


#include "FeatureFillet.h"
#include "FilletBase.h"
#include <Base/Exception.h>
#include <Base/Console.h>


using namespace Part;


PROPERTY_SOURCE(Part::Fillet, Part::FilletBase)

Fillet::Fillet()
{
}

App::DocumentObjectExecReturn *Fillet::execute(void)
{
    App::DocumentObject* link = Base.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    Part::Feature *base = static_cast<Part::Feature*>(Base.getValue());
    Base::Console().Message("-----Dumping base tree in FeatureFillet near top\n");
    Base::Console().Message(base->Shape.getShape().DumpTopoHistory().c_str());

    try {
#if defined(__GNUC__) && defined (FC_OS_LINUX)
        Base::SignalException se;
#endif
        std::vector<FilletElement> targetEdges = Edges.getValues();
        TopoShape NewTopoShape = base->Shape.getShape();
        this->Shape.addGeneratedShape(NewTopoShape);

        BRepFilletAPI_MakeFillet mkFillet = this->Shape.addFilletedShape(targetEdges);

        if (!mkFillet.IsDone())
            return new App::DocumentObjectExecReturn("Fillet operation appears to have failed");

        // then all the history junk
        // make sure the 'PropertyShapeHistory' is not safed in undo/redo (#0001889)
        TopoDS_Shape shape = mkFillet.Shape();
        ShapeHistory history = buildHistory(mkFillet, TopAbs_FACE, shape, base->Shape.getValue());
        //this->Shape.setValue(NewTopoShape);
        Base::Console().Message("-----Dumping tree in FeatureFillet, from NewTopoShape\n");
        Base::Console().Message(NewTopoShape.DumpTopoHistory().c_str());

        PropertyShapeHistory prop;
        prop.setValue(history);
        prop.setContainer(this);
        prop.touch();

        Base::Console().Message("-----Dumping tree in FeatureFillet before return\n");
        Base::Console().Message(base->Shape.getShape().DumpTopoHistory().c_str());
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("A fatal error occurred when making fillets");
    }
    Base::Console().Message("-----Dumping tree in FeatureFillet before end\n");
    Base::Console().Message(base->Shape.getShape().DumpTopoHistory().c_str());
}
