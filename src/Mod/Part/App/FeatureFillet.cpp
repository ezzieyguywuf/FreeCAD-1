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
#include <Base/Exception.h>

#include <PrimitiveSolidManager.h>
#include <OccEdge.h>
#include <TopoDS.hxx>

using namespace Part;


PROPERTY_SOURCE(Part::Fillet, Part::FilletBase)

Fillet::Fillet()
    : converted(false)
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

    try {
#if defined(__GNUC__) && defined (FC_OS_LINUX)
        Base::SignalException se;
#endif
        // Retrieve SolidManager
        const PrimitiveSolidManager& mgr = base->Shape.getManager();

        BRepFilletAPI_MakeFillet mkFillet(mgr.getSolid().getShape());
        TopTools_IndexedMapOfShape mapOfShape;
        TopExp::MapShapes(mgr.getSolid().getShape(), TopAbs_EDGE, mapOfShape);

        std::vector<FilletElement> values = Edges.getValues();
        std::vector<FilletElement> newValues;
        for (std::vector<FilletElement>::iterator it = values.begin(); it != values.end(); ++it) {
            int id = it->edgeid;
            std::cout << "retreived ID = " << id << std::endl;
            double radius1 = it->radius1;
            double radius2 = it->radius2;

            // if mgr.getSolid is Null, that means mgr hasn't been initialized.
            if (not converted)
            {
                // first, retrieve the "old school" Edge.
                const TopoDS_Edge& edge = TopoDS::Edge(mapOfShape.FindKey(id));

                // get the "Robust" ID of the desired edge
                id = mgr.getEdgeIndex(Occ::Edge(edge));
                std::cout << "setting id = " << id << std::endl;

                // set the converted flag
                converted = true;
            }
            else{
                std::cout << "----- mgr was already initialized!!!" << std::endl;
            }

            // retrieve the desired Edge
            Occ::Edge retreivedEdge = mgr.getEdgeByIndex(id);
            // add to the mkFillet
            mkFillet.Add(radius1, radius2, TopoDS::Edge(retreivedEdge.getShape()));

            // store the values, including the "Robust" ID.
            FilletElement newVal;
            newVal.edgeid = id;
            newVal.radius1 = radius1;
            newVal.radius2 = radius2;
            newValues.push_back(newVal);
        }
        Edges.setValues(newValues);

        mkFillet.Build();
        TopoDS_Shape shape = mkFillet.Shape();
        if (shape.IsNull())
            return new App::DocumentObjectExecReturn("Resulting shape is null");
        ShapeHistory history = buildHistory(mkFillet, TopAbs_FACE, shape, base->Shape.getValue());
        this->Shape.setValue(shape);

        // make sure the 'PropertyShapeHistory' is not safed in undo/redo (#0001889)
        PropertyShapeHistory prop;
        prop.setValue(history);
        prop.setContainer(this);
        prop.touch();

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("A fatal error occurred when making fillets");
    }
}
