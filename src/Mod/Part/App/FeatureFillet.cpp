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
    std::clog << "----- copying BaseShape" << std::endl;
    TopoShape BaseShape = base->Shape.getShape();

    try {
#if defined(__GNUC__) && defined (FC_OS_LINUX)
        Base::SignalException se;
#endif

        std::clog << "----- copying FilletShape" << std::endl;
        TopoShape FilletShape = this->Shape.getShape();
        bool creating = false;
        std::clog << "-----Dumpnig topo history in fillet base near top" << std::endl;
        std::clog << FilletShape.getTopoHelper().DeepDump() << std::endl;

        if (FilletShape.hasTopoNamingNodes()){
            Base::Console().Message("----- Found topo naming nodes...\n");
            // If there are nodes, this means an edge (or more) has already been selected.
            // Let's maintain this topological history
            
            // Since we're being recomputed, likely the Base shape has changed. Let's add
            // the updated base shape to the topological history.
            // TODO: make a 'check if updated' call in TopoNamingHelper
            //FilletShape.addShape(base->Shape.getValue());
            // TODO: change to modifyFillet
            //FilletShape.modifyShape("0:2", base->Shape.getValue());
        }
        else{
            creating = true;
            Base::Console().Message("----- Did not find topo naming nodes...\n");
            // If there are no nodes, then no fillets have been made yet. Let's use the
            // Base shape as the topo basis, i.e. no topological history except for
            // 'Generated'
            FilletShape.createFilletBaseShape(BaseShape);
            std::clog << "-----Dumpnig topo history in fillet base after createFilletBaseShape" << std::endl;
            std::clog << FilletShape.getTopoHelper().DeepDump() << std::endl;
        }
        //Base::Console().Message(FilletShape.DumpTopoHistory().c_str());

        std::vector<FilletElement> fdatas = Edges.getValues();
        for (auto&& fdata : fdatas) {
            std::string edgetag = fdata.edgetag;
            if (edgetag.empty()){
                int id = fdata.edgeid;
                fdata.edgetag = FilletShape.selectEdge(id);
            }
        }
        Edges.setValues(fdatas);

        ShapeHistory history;
        if (creating){
            std::clog << "---------------" << std::endl;
            std::clog << "-----Creating fillet" << std::endl;
            std::clog << "---------------" << std::endl;
            BRepFilletAPI_MakeFillet mkFillet = FilletShape.createFillet(BaseShape, Edges.getValues());
            if (!mkFillet.IsDone())
                return new App::DocumentObjectExecReturn("Resulting shape is null");

            TopoDS_Shape shape = mkFillet.Shape();
            history = buildHistory(mkFillet, TopAbs_FACE, shape, BaseShape.getShape());
        }
        else{
            std::clog << "---------------" << std::endl;
            std::clog << "-----Updating fillet" << std::endl;
            std::clog << "---------------" << std::endl;
            BRepFilletAPI_MakeFillet mkFillet = FilletShape.updateFillet(BaseShape, Edges.getValues());
            // TODO: somehow eliminate this code duplication.
            if (!mkFillet.IsDone())
                return new App::DocumentObjectExecReturn("Resulting shape is null");

            TopoDS_Shape shape = mkFillet.Shape();
            history = buildHistory(mkFillet, TopAbs_FACE, shape, BaseShape.getShape());
        }

        //this->Shape.setValue(shape);
        this->Shape.setShape(FilletShape);

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
