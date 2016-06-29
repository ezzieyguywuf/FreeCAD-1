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
#include "TopoShape.h"
#include <Base/Exception.h>
#include <Base/Console.h>


using namespace Part;


PROPERTY_SOURCE(Part::Fillet, Part::FilletBase)

Fillet::Fillet()
{
}

App::DocumentObjectExecReturn *Fillet::execute(void)
{
    Base::Console().Message("-------------------------------------------------------------------------\n");
    Base::Console().Message("-----Entered FILLET EXECUTE -----\n");
    Base::Console().Message("-------------------------------------------------------------------------\n");
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
        TopoShape FilletShape = this->Shape.getShape();
        if (FilletShape.hasTopoNamingNodes()){
            Base::Console().Message("----- Found topo naming nodes...\n");
            // If there are nodes, this means an edge (or more) has already been selected.
            // Let's maintain this topological history
            
            // Since we're being recomputed, likely the Base shape has changed. Let's add
            // the updated base shape to the topological history.
            // TODO: make a 'check if updated' call in TopoNamingHelper
            FilletShape.addShape(base->Shape.getValue());
        }
        else{
            Base::Console().Message("----- Did not find topo naming nodes...\n");
            // If there are no nodes, then no fillets have been made yet. Let's use the
            // Base shape as the topo basis, i.e. no topological history except for
            // 'Generated'
            FilletShape = TopoShape(base->Shape.getValue());
        }
        //Base::Console().Message(FilletShape.DumpTopoHistory().c_str());

        std::vector<FilletElement> values = Edges.getValues();
        for (std::vector<FilletElement>::iterator it = values.begin(); it != values.end(); ++it) {
            int id = it->edgeid;
            std::string edgetag = it->edgetag;
            std::ostringstream outstream;
            outstream << "edgetag = " << edgetag << "\n";
            Base::Console().Message(outstream.str().c_str());
            if (edgetag.empty()){
                Base::Console().Message("Retrieving edgetag?\n");
                edgetag = FilletShape.selectEdge(id);
                Edges.setValue(it->edgeid, it->radius1, it->radius2, edgetag);
            }
        }

        // This is where the fillet operation is done now, in TopoShape_Fillet
        BRepFilletAPI_MakeFillet mkFillet = FilletShape.makeTopoShapeFillet(Edges.getValues());

        if (!mkFillet.IsDone())
            return new App::DocumentObjectExecReturn("Resulting shape is null");

        TopoDS_Shape shape = mkFillet.Shape();
        ShapeHistory history = buildHistory(mkFillet, TopAbs_FACE, shape, base->Shape.getValue());
        //this->Shape.setValue(shape);
        this->Shape.setValue(FilletShape);

        // make sure the 'PropertyShapeHistory' is not safed in undo/redo (#0001889)
        PropertyShapeHistory prop;
        prop.setValue(history);
        prop.setContainer(this);
        prop.touch();

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("A fatal error occurred when making fillets");
    }
}
