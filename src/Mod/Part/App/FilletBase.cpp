#include "PreCompiled.h"

#include <Base/Console.h>

#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include "PropertyTopoShape.h"
#include "FilletBase.h"
#include "FilletBasePy.h"

using namespace Part;

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

std::string FilletBase::getSelectedEdgeLabel(int id, double r1, double r2) const{
    App::DocumentObject* link = Base.getValue();
    if (!link){
        Standard_Failure::Raise("ERROR: No object found\n");
        return ""; // No object Linked
    }
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())){
        Standard_Failure::Raise("ERROR: linked object is not a part\n");
        return ""; // Linked object is not a Part
    }
    Part::Feature *base = static_cast<Part::Feature*>(Base.getValue());

    Base::Console().Message("'setEdge' in FilletBase.cpp called\n");
    // Get a reference to the Part::Feature

    // Get a reference to the TopoDS_Shape and TopoShape
    Base::Console().Message("Getting TopoDS_Shape\n");
    // Get a reference to the Part::Feature
    TopoDS_Shape BaseShape = base->Shape.getValue();
    Base::Console().Message("Getting TopoShape\n");
    TopoShape myTopoShape  = base->Shape.getShape();

    // Get a list of all the edges
    TopTools_IndexedMapOfShape listOfEdges;
    TopExp::MapShapes(BaseShape, TopAbs_EDGE, listOfEdges);

    // Get the specific edge, I hope
    TopoDS_Edge anEdge = TopoDS::Edge(listOfEdges.FindKey(id));

    // 'Select' this edge, or retrieve the TDF_Label if it's already been selected
    Base::Console().Message("Calling selectEdge\n");
    std::string selectionLabel = myTopoShape.selectEdge(anEdge, BaseShape);
    return selectionLabel;
}

void FilletBase::setEdge(int id, double r1, double r2){
    // This following setValue is defined in PropertyTopoShape, and is the original
    // implementation
    std::string selectionLabel = getSelectedEdgeLabel(id, r1, r2);
    this->Edges.setValue(id, r1, r2, selectionLabel);
}
void FilletBase::setEdges(std::vector<FilletElement>& values){
    FilletElement curElement;
    int curID;
    double rad1, rad2;
    std::string curSelectionLabel;
    for (std::vector<FilletElement>::iterator it = (values.begin()); it != values.end(); ++it){
        curID = it->edgeid;
        rad1  = it->radius1;
        rad2  = it->radius2;
        curSelectionLabel = getSelectedEdgeLabel(curID, rad1, rad2);
        it->edgetag = curSelectionLabel;
    }
    this->Edges.setValues(values);
}

PyObject *FilletBase::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new FilletBasePy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}
