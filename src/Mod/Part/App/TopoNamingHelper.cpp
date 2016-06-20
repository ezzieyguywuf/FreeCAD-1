#include "PreCompiled.h"

#ifndef _PreComp_
#include <iostream>
#include <Base/Console.h>
#endif

#include <TDataStd_AsciiString.hxx>
#include <TDF_IDFilter.hxx>
#include <TDF_TagSource.hxx>
#include <TDF_Tool.hxx>
#include <TDF_ChildIterator.hxx>

#include "TopoNamingHelper.h"

#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>

#include <TopExp.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>

#include <TNaming_Builder.hxx>
#include <TNaming_Selector.hxx>
#include <TNaming_NamedShape.hxx>

TopoNamingHelper::TopoNamingHelper(){
    Base::Console().Message("-----Instantiated TopoNamingHelper\n");
}

TopoNamingHelper::TopoNamingHelper(const TopoNamingHelper& existing){
    this->myDataFramework = existing.myDataFramework;
    this->myRootNode      = existing.myRootNode;
}

TopoNamingHelper::~TopoNamingHelper(){
    Base::Console().Message("-----UnInstantiated TopoNamingHelper\n");
}


void TopoNamingHelper::TrackGeneratedShape(const TopoDS_Shape& GeneratedShape){
    Base::Console().Message("-----Tracking Generated Shape\n");
    std::ostringstream outputStream;
    DeepDump(outputStream);
    Base::Console().Message(outputStream.str().c_str());
    // Declare variables
    TNaming_Builder* MyBuilderPtr;
    TDF_Label curLabel;
    TopTools_IndexedMapOfShape mapOfFaces;
    TopoDS_Face curFace;
    int i;

    // create a new node under Root
    TDF_Label LabelRoot = TDF_TagSource::NewChild(myRootNode);

    AddTextToLabel(LabelRoot, "Generated Shape node");

    // add the generated shape to the LabelRoot
    MyBuilderPtr = new TNaming_Builder(LabelRoot);
    MyBuilderPtr->Generated(GeneratedShape);

    // Now iterate over each face and add as a child to the new node we created
    TopExp::MapShapes(GeneratedShape, TopAbs_FACE, mapOfFaces);
    for (i=1; i <= mapOfFaces.Extent(); i++){
        curFace = TopoDS::Face(mapOfFaces.FindKey(i));
        // Create a new sub-node for the Face
        curLabel = TDF_TagSource::NewChild(LabelRoot);
        // add Face to the sub-node
        delete MyBuilderPtr;
        MyBuilderPtr = new TNaming_Builder(curLabel);
        MyBuilderPtr->Generated(curFace);
    }
    std::ostringstream outputStream2;
    DeepDump(outputStream2);
    Base::Console().Message("Data Framework Dump Below\n");
    Base::Console().Message(outputStream2.str().c_str());
}

void TopoNamingHelper::TrackFuseOperation(BRepAlgoAPI_Fuse& Fuser){
    Base::Console().Message("-----Tracking Fuse Operation\n");
    // In a Fuse operation, each face is either Modified from one of the two Shapes being
    // fused, or it is Deleted. There is not an instance where a Fuse operation results in
    // a Generated Face.
    TopoDS_Shape BaseShape   = Fuser.Shape1();
    TopoDS_Shape FuseShape   = Fuser.Shape2();
    TopoDS_Shape ResultShape = Fuser.Shape();

    //TopTools_ListOfShape 

    // create a new node under Root, and sub-nodes for the modified faces
    TDF_Label LabelRoot    = TDF_TagSource::NewChild(myRootNode);
    TDF_Label BaseModified = TDF_TagSource::NewChild(LabelRoot);
    TDF_Label FuseModified = TDF_TagSource::NewChild(LabelRoot);

    // Add some descriptive text for debugging
    AddTextToLabel(LabelRoot, "Fusion Node");
    AddTextToLabel(BaseModified, "Modified Faces on Base Shape");
    AddTextToLabel(FuseModified, "Modified Faces on Add Shape");

    // Add the fused shape as a modification of BaseShape
    TNaming_Builder ResultBuilder(LabelRoot);
    ResultBuilder.Modify(BaseShape, ResultShape);

    // Add the BaseShape modified faces and the FuseShape modified faces
    TNaming_Builder BaseModifiedBuilder(BaseModified);
    TNaming_Builder FuseModifiedBuilder(FuseModified);

    TopTools_IndexedMapOfShape origFaces, fuseFaces, allBaseModified;

    TopExp::MapShapes(BaseShape, TopAbs_FACE, origFaces);
    for (int i=1; i<=origFaces.Extent(); i++){
        TopoDS_Face curFace = TopoDS::Face(origFaces.FindKey(i));
        TopTools_ListOfShape modified = Fuser.Modified(curFace);
        TopTools_ListIteratorOfListOfShape modIterator(modified);
        for (; modIterator.More(); modIterator.Next()){
            TopoDS_Face moddedFace = TopoDS::Face(modIterator.Value());
            if (!curFace.IsEqual(moddedFace)){
                BaseModifiedBuilder.Modify(curFace, moddedFace);
                allBaseModified.Add(moddedFace);
            }
        }
    }

    TopExp::MapShapes(FuseShape, TopAbs_FACE, fuseFaces);
    for (int i=1; i<=fuseFaces.Extent(); i++){
        TopoDS_Face curFace = TopoDS::Face(fuseFaces.FindKey(i));
        TopTools_ListOfShape modified = Fuser.Modified(curFace);
        TopTools_ListIteratorOfListOfShape modIterator(modified);
        for (; modIterator.More(); modIterator.Next()){
            TopoDS_Face moddedFace = TopoDS::Face(modIterator.Value());
            if (!curFace.IsEqual(moddedFace) && !allBaseModified.Contains(moddedFace)){
                FuseModifiedBuilder.Modify(curFace, moddedFace);
            }
        }
    }
    std::ostringstream outputStream;
    DeepDump(outputStream);
    Base::Console().Message("Data Framework Dump Below\n");
    Base::Console().Message(outputStream.str().c_str());
}

void TopoNamingHelper::TrackFilletOperation(const TopTools_ListOfShape& Edges, const TopoDS_Shape& BaseShape, BRepFilletAPI_MakeFillet& mkFillet){
    BRepFilletAPI_MakeFillet Filleter = mkFillet;
    Base::Console().Message("-----Tracking Fillet Operation\n");
    std::ostringstream output;
    DeepDump(output);
    Base::Console().Message(output.str().c_str());

    TopoDS_Shape ResultShape = Filleter.Shape();

    // create two new nodes under Root. The first is for the Selection of the Edges. The
    // second is for the result filleted Shape and it's modified/deleted/generated Faces.
    TDF_Label SelectionRoot     = TDF_TagSource::NewChild(myRootNode);
    TDF_Label FilletRoot        = TDF_TagSource::NewChild(myRootNode);
    TDF_Label Modified          = TDF_TagSource::NewChild(FilletRoot);
    TDF_Label Deleted           = TDF_TagSource::NewChild(FilletRoot);
    TDF_Label FacesFromEdges    = TDF_TagSource::NewChild(FilletRoot);
    TDF_Label FacesFromVertices = TDF_TagSource::NewChild(FilletRoot);

    // Add some descriptive text for debugging
    AddTextToLabel(SelectionRoot, "Selection Node (fillet follows)");
    AddTextToLabel(FilletRoot, "Fillet Node");
    AddTextToLabel(Modified, "Modified faces");
    AddTextToLabel(Deleted, "Deleted faces");
    AddTextToLabel(FacesFromEdges, "Faces from edges");
    AddTextToLabel(FacesFromVertices, "Faces from vertices");

    // Start by adding the result shape. This will also create the TNaming_UsedShapes
    // under the Root node if it doesn't exist
    TNaming_Builder FilletBuilder(FilletRoot);
    FilletBuilder.Modify(BaseShape, ResultShape);

    // Add all the edges that we're filleting to the Selection
    TopTools_ListIteratorOfListOfShape edgeIterator(Edges);
    for (;edgeIterator.More(); edgeIterator.Next()){
        TopoDS_Edge curEdge = TopoDS::Edge(edgeIterator.Value());
        TDF_Label curSubLabel = TDF_TagSource::NewChild(SelectionRoot);
        TNaming_Selector EdgeSelector(curSubLabel);
        EdgeSelector.Select(curEdge, BaseShape);
        AddTextToLabel(curSubLabel, "Selection sub-label, I made this");
    }

    // Next, the Faces generated from Edges
    TNaming_Builder FacesFromEdgeBuilder(FacesFromEdges);
    TopTools_IndexedMapOfShape mapOfEdges;
    TopExp::MapShapes(BaseShape, TopAbs_EDGE, mapOfEdges);
    for (int i=1; i<=mapOfEdges.Extent(); i++){
        TopoDS_Edge curEdge = TopoDS::Edge(mapOfEdges.FindKey(i));
        TopTools_ListOfShape generatedFaces = Filleter.Generated(curEdge);
        TopTools_ListIteratorOfListOfShape it(generatedFaces);
        for (;it.More(); it.Next()){
            TopoDS_Shape checkShape = it.Value();
            if (!curEdge.IsSame(checkShape)){
                FacesFromEdgeBuilder.Generated(curEdge, checkShape);
            }
        }
    }

    // Faces from BaseShape Modified or Deleted by the Fillet operation
    TNaming_Builder ModifiedBuilder(Modified);
    TNaming_Builder DeletedBuilder(Deleted);
    TopTools_IndexedMapOfShape mapOfFaces;
    TopExp::MapShapes(BaseShape, TopAbs_FACE, mapOfFaces);
    for (int i=1; i<=mapOfFaces.Extent(); i++){
        TopoDS_Face curFace = TopoDS::Face(mapOfFaces.FindKey(i));

        // First check Modified
        TopTools_ListOfShape modifiedFaces = Filleter.Modified(curFace);
        TopTools_ListIteratorOfListOfShape it(modifiedFaces);
        for (; it.More(); it.Next()){
            TopoDS_Shape checkShape = it.Value();
            if (!curFace.IsSame(checkShape)){
                ModifiedBuilder.Modify(curFace, checkShape);
            }
        }

        // Then check Deleted
        if (Filleter.IsDeleted(curFace)){
            DeletedBuilder.Delete(curFace);
        }
    }

    // Finally, the Faces generated from Vertices
    TNaming_Builder FacesFromVerticesBuilder(FacesFromVertices);
    TopTools_IndexedMapOfShape mapOfVertices;
    TopExp::MapShapes(BaseShape, TopAbs_VERTEX, mapOfVertices);
    for (int i=1; i<=mapOfVertices.Extent(); i++){
        TopoDS_Shape curVertex = mapOfVertices.FindKey(i);
        TopTools_ListOfShape generatedFaces = Filleter.Generated(curVertex);
        TopTools_ListIteratorOfListOfShape it(generatedFaces);
        for (;it.More(); it.Next()){
            TopoDS_Shape checkShape = it.Value();
            if (!curVertex.IsSame(checkShape)){
                FacesFromEdgeBuilder.Generated(curVertex, checkShape);
            }
        }
    }
    std::ostringstream outputStream;
    DeepDump(outputStream);
    Base::Console().Message("Data Framework Dump Below\n");
    Base::Console().Message(outputStream.str().c_str());
}

void TopoNamingHelper::AddTextToLabel(const TDF_Label& Label, char const *str){
    Handle(TDataStd_AsciiString) nameAttribute;
    TCollection_AsciiString myName;
    myName = str;
    nameAttribute = new TDataStd_AsciiString();
    nameAttribute->Set(myName);
    Label.AddAttribute(nameAttribute);
}

void TopoNamingHelper::Dump() const{
    TDF_Tool::DeepDump(std::cout, myDataFramework);
    std::cout << "\n";
}

void TopoNamingHelper::Dump(std::ostream& stream) const{
    TDF_Tool::DeepDump(stream, myDataFramework);
    stream << "\n";
}

void TopoNamingHelper::DeepDump(std::ostream& stream) const{
    TDF_IDFilter myFilter;
    myFilter.Keep(TDataStd_AsciiString::GetID());
    myFilter.Keep(TNaming_NamedShape::GetID());
    //TDF_Tool::ExtendedDeepDump(stream, myDataFramework, myFilter);
    //stream << "\n";
    TDF_ChildIterator TreeIterator(myRootNode, Standard_True);
    for(;TreeIterator.More(); TreeIterator.Next()){
        TDF_Label curLabel = TreeIterator.Value();
        // add the Tag info
        curLabel.EntryDump(stream);
        // If a AsciiString is present, add the data
        if (curLabel.IsAttribute(TDataStd_AsciiString::GetID())){
            Handle(TDataStd_AsciiString) date;
            curLabel.FindAttribute(TDataStd_AsciiString::GetID(), date);
            stream << " ";
            date->Get().Print(stream);
        }
        stream << "\n";
    }
}

void TopoNamingHelper::DeepDump() const{
    std::ostringstream output;
    DeepDump(output);
    Base::Console().Message(output.str().c_str());
}

void TopoNamingHelper::operator = (const TopoNamingHelper& helper){
    this->myDataFramework = helper.myDataFramework;
    this->myRootNode      = helper.myRootNode;
}
