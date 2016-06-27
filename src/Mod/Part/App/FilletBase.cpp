#include "PreCompiled.h"

#include <Base/Console.h>

#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
# include <Standard_Failure.hxx>

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

std::string FilletBase::getSelectedEdgeLabel(int id, double r1, double r2){
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

    const std::string selectionLabel = base->Shape.selectEdge(id);
    //Base::Console().Message("-----dumping 'base' in FilletBase.cpp\n");
    //Base::Console().Message(base->Shape.getShape().DumpTopoHistory().c_str());
    return selectionLabel;

}

void FilletBase::setEdge(int id, double r1, double r2){
    // This following setValue is defined in PropertyTopoShape, and is the original
    // implementation    
    //App::DocumentObject* link = Base.getValue();
    //if (!link)
        //Standard_Failure::Raise("No object linked");
    //if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        //Standard_Failure::Raise("Linked object is not a Part object");
    Part::FilletBase *base = static_cast<Part::FilletBase*>(Base.getValue());
    std::string selectionLabel = getSelectedEdgeLabel(id, r1, r2);
    this->Edges.setValue(id, r1, r2, selectionLabel);
}
void FilletBase::setEdges(std::vector<FilletElement>& values){
    //App::DocumentObject* link = Base.getValue();
    //if (!link)
        //Standard_Failure::Raise("No object linked");
    //if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        //Standard_Failure::Raise("Linked object is not a Part object");
    //Part::FilletBase *base = static_cast<Part::FilletBase*>(Base.getValue());
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
