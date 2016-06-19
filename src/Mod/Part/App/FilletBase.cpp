#include "PreCompiled.h"

#ifndef _PreComp_
# include <gp_Trsf.hxx>
# include <gp_Ax1.hxx>
# include <BRepBuilderAPI_MakeShape.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <Standard_Failure.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <gp_Dir.hxx>
# include <gp_Pln.hxx> // for Precision::Confusion()
# include <Bnd_Box.hxx>
# include <BRepBndLib.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
#endif

#include <strstream>
#include <Base/Console.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>
#include <App/FeaturePythonPyImp.h>

#include "PartFeature.h"
#include "PartFeaturePy.h"
#include "FilletBase.h"

PROPERTY_SOURCE(Part::FilletBase, Part::Feature)

FilletBase::FilletBase()
{
    ADD_PROPERTY(Base,(0));
    ADD_PROPERTY(Edges,(0,0,0, ""));
    Edges.setSize(0);
}

short FilletBase::mustExecute() const
{
    if (Base.isTouched() || Edges.isTouched())
        return 1;
    return 0;
}

//void FilletBase::setValue(){
    //Base::Console().Message("setValue in FilletBase called");
//}

void FilletBase::setEdge(int id, double r1, double r2){
    Base::Console().Message("'setEdge' in PartFeature.cpp called\n");
    // Get a reference to the Part::Feature
    Part::Feature *base = static_cast<Part::Feature*>(Base.getValue());

    // Get a reference to the TopoDS_Shape and TopoShape
    TopoDS_Shape BaseShape = base->Shape.getValue();
    TopoShape myTopoShape  = base->Shape.getShape();

    // Get a list of all the edges
    TopTools_IndexedMapOfShape listOfEdges;
    TopExp::MapShapes(BaseShape, TopAbs_EDGE, listOfEdges);

    // Get the specific edge, I hope
    TopoDS_Edge anEdge = TopoDS::Edge(listOfEdges.FindKey(id));

    // 'Select' this edge, or retrieve the TDF_Label if it's already been selected
    std::string selectionLabel = myTopoShape.selectEdge(anEdge, BaseShape);
    // This following setValue is defined in PropertyTopoShape, and is the original
    // implementation
    this->Edges.setValue(id, r1, r2, selectionLabel);
}
void FilletBase::setEdges(const std::vector<FilletElement>& values){
}
