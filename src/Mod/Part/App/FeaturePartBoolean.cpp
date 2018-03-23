/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepAlgoAPI_BooleanOperation.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <Standard_Failure.hxx>
# include <memory>
#endif

#include "FeaturePartBoolean.h"
#include "modelRefine.h"
#include <App/Application.h>
#include <Base/Parameter.h>

#include <CompoundSolidManager.h>
#include <OccSolidMaker.h>
#include <OccBooleanSolid.h>
#include <OccModifiedSolid.h>
#include <TopoDS.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>

using namespace Part;

PROPERTY_SOURCE_ABSTRACT(Part::Boolean, Part::Feature)


Boolean::Boolean(void)
{
    ADD_PROPERTY(Base,(0));
    ADD_PROPERTY(Tool,(0));
    ADD_PROPERTY_TYPE(History,(ShapeHistory()), "Boolean", (App::PropertyType)
        (App::Prop_Output|App::Prop_Transient|App::Prop_Hidden), "Shape history");
    History.setSize(0);

    ADD_PROPERTY_TYPE(Refine,(0),"Boolean",(App::PropertyType)(App::Prop_None),"Refine shape (clean up redundant edges) after this boolean operation");

    //init Refine property
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part/Boolean");
    this->Refine.setValue(hGrp->GetBool("RefineModel", false));
}

short Boolean::mustExecute() const
{
    if (Base.getValue() && Tool.getValue()) {
        if (Base.isTouched())
            return 1;
        if (Tool.isTouched())
            return 1;
    }
    return 0;
}

App::DocumentObjectExecReturn *Boolean::execute(void)
{
    try {
#if defined(__GNUC__) && defined (FC_OS_LINUX)
        Base::SignalException se;
#endif
        Part::Feature *base = dynamic_cast<Part::Feature*>(Base.getValue());
        Part::Feature *tool = dynamic_cast<Part::Feature*>(Tool.getValue());

        if (!base || !tool)
            return new App::DocumentObjectExecReturn("Linked object is not a Part object");

        // Now, let's get the TopoDS_Shape
        TopoDS_Shape BaseShape = base->Shape.getValue();
        if (BaseShape.IsNull())
            throw Base::Exception("Base shape is null");
        TopoDS_Shape ToolShape = tool->Shape.getValue();
        if (ToolShape.IsNull())
            throw Base::Exception("Tool shape is null");

        std::unique_ptr<BRepAlgoAPI_BooleanOperation> mkBool(makeOperation(BaseShape, ToolShape));
        if (!mkBool->IsDone()) {
            return new App::DocumentObjectExecReturn("Boolean operation failed");
        }
        TopoDS_Shape resShape = mkBool->Shape();
        if (resShape.IsNull()) {
            return new App::DocumentObjectExecReturn("Resulting shape is null");
        }
        Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
            .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part/Boolean");

        if (hGrp->GetBool("CheckModel", false)) {
            BRepCheck_Analyzer aChecker(resShape);
            if (! aChecker.IsValid() ) {
                return new App::DocumentObjectExecReturn("Resulting shape is invalid");
            }
        }

        std::vector<ShapeHistory> history;
        history.push_back(buildHistory(*mkBool.get(), TopAbs_FACE, resShape, BaseShape));
        history.push_back(buildHistory(*mkBool.get(), TopAbs_FACE, resShape, ToolShape));

        if (this->Refine.getValue()) {
            try {
                TopoDS_Shape oldShape = resShape;
                BRepBuilderAPI_RefineModel mkRefine(oldShape);
                resShape = mkRefine.Shape();
                ShapeHistory hist = buildHistory(mkRefine, TopAbs_FACE, resShape, oldShape);
                history[0] = joinHistory(history[0], hist);
                history[1] = joinHistory(history[1], hist);
            }
            catch (Standard_Failure) {
                // do nothing
            }
        }
        std::clog << "----- PART BOOLEAN!!!!!!!" << std::endl;
        //----------------------------------------
        //--- Start TopoManager code--------------
        //----------------------------------------

        // First, get our ISolidManager, and those of our base and tool
        const ISolidManager& refIMgr(this->Shape.getManager());
        const ISolidManager& newBaseMgr = base->Shape.getManager();
        const ISolidManager& newToolMgr = tool->Shape.getManager();
        TopoDS_Shape myBase = newBaseMgr.getSolid().getShape();
        TopoDS_Shape myTool = newToolMgr.getSolid().getShape();
        std::unique_ptr<BRepAlgoAPI_BooleanOperation> myBool(makeOperation(myBase, 
                                                                           myTool));
        myBool->Build();
        CompoundSolidManager* aMgr;
        if (refIMgr.getSolid().isNull())
        {
            // If null, that means the solid manager hasn't been initialized. We'll
            // initialize it with a CompoundSolidManager
            Occ::BooleanSolid occBoolean = Occ::SolidMaker::makeBoolean(*myBool);
            aMgr = new CompoundSolidManager(occBoolean);
            myBaseMgr = PrimitiveSolidManager(myBool->Shape1());
            myToolMgr = PrimitiveSolidManager(myBool->Shape2());
        }
        else
        {
            // If not null, we know it contains a CompoundSolidManager (since we put the
            // manager there to begin with)
            const CompoundSolidManager& refMgr = 
                static_cast<const CompoundSolidManager&>(refIMgr);
            aMgr = new CompoundSolidManager(refMgr);

            // To update our CompoundSolidManager, we need the new solid as well as an
            // Occ:::ModifiedSolid for each of our Base and Tool.
            Occ::BooleanSolid newBoolean = Occ::SolidMaker::makeBoolean(*myBool);
            Occ::ModifiedSolid baseMod = ISolidManager::makeModifiedSolid(myBaseMgr, newBaseMgr);
            Occ::ModifiedSolid toolMod = ISolidManager::makeModifiedSolid(myToolMgr, newToolMgr);
            aMgr->updateSolid(newBoolean, {baseMod, toolMod});

            myBaseMgr = PrimitiveSolidManager(newBaseMgr.getSolid());
            myToolMgr = PrimitiveSolidManager(newToolMgr.getSolid());
        }

        this->Shape.setValue(aMgr->getSolid().getShape());
        this->Shape.setManager(unique_ptr<ISolidManager>(aMgr));
        //----------------------------------------
        //--- End TopoManager code----------------
        //----------------------------------------
        this->History.setValues(history);
        return App::DocumentObject::StdReturn;
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("A fatal error occurred when running boolean operation");
    }
}
