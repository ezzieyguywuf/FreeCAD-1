#include "PreCompiled.h"

#ifndef _PreComp_
#include <iostream>
#include <Base/Console.h>
#endif

#include <TDF_TagSource.hxx>
#include <TDF_Tool.hxx>

#include "TopoNamingHelper.h"

#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>

#include <TopExp.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>

#include <TNaming_Builder.hxx>
#include <TNaming_Selector.hxx>

TopoNamingHelper::TopoNamingHelper(){
    Base::Console().Message("-----Instantiated TopoNamingHelper\n");
}

TopoNamingHelper::~TopoNamingHelper(){
    Base::Console().Message("-----UnInstantiated TopoNamingHelper\n");
}


void TopoNamingHelper::TrackGeneratedShape(TopoDS_Shape GeneratedShape) const{
    Base::Console().Message("-----Tracking Generated Shape\n");
    // Declare variables
    TNaming_Builder* MyBuilderPtr;
    TDF_Label curLabel;
    TopTools_IndexedMapOfShape mapOfFaces;
    TopoDS_Face curFace;
    int i;

    // create a new node under Root
    TDF_Label LabelRoot = TDF_TagSource::NewChild(myRootNode);

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
    std::ostringstream outputStream;
    TDF_Tool::DeepDump(outputStream, myDataFramework);
    Base::Console().Message("Data Framework Dump Below\n");
    Base::Console().Message(outputStream.str().c_str());
}

void TopoNamingHelper::TrackFuseOperation(BRepAlgoAPI_Fuse Fuser) const{
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
    TDF_Tool::DeepDump(outputStream, myDataFramework);
    Base::Console().Message("Data Framework Dump Below\n");
    Base::Console().Message(outputStream.str().c_str());
}

void TopoNamingHelper::TrackFilletOperation(TopTools_ListOfShape Edges, TopoDS_Shape BaseShape, BRepFilletAPI_MakeFillet Filleter) const{
    Base::Console().Message("-----Tracking Fillet Operation\n");
    TopoDS_Shape ResultShape = Filleter.Shape();

    // create two new nodes under Root. The first is for the Selection of the Edges. The
    // second is for the result filleted Shape and it's modified/deleted/generated Faces.
    TDF_Label SelectionRoot     = TDF_TagSource::NewChild(myRootNode);
    TDF_Label FilletRoot        = TDF_TagSource::NewChild(myRootNode);
    TDF_Label Modified          = TDF_TagSource::NewChild(FilletRoot);
    TDF_Label Deleted           = TDF_TagSource::NewChild(FilletRoot);
    TDF_Label FacesFromEdges    = TDF_TagSource::NewChild(FilletRoot);
    TDF_Label FacesFromVertices = TDF_TagSource::NewChild(FilletRoot);

    // Add all the edges that we're filleting to the Selection
    TopTools_ListIteratorOfListOfShape edgeIterator(Edges);
    for (;edgeIterator.More(); edgeIterator.Next()){
        TopoDS_Edge curEdge = TopoDS::Edge(edgeIterator.Value());
        TDF_Label curSubLabel = TDF_TagSource::NewChild(SelectionRoot);
        TNaming_Selector EdgeSelector(curSubLabel);
        EdgeSelector.Select(curEdge, BaseShape);
    }

    // Now add the appropriate information from the filleted shape
    // First, the result shape on our base node
    TNaming_Builder FilletBuilder(FilletRoot);
    FilletBuilder.Modify(BaseShape, ResultShape);

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
    TDF_Tool::DeepDump(outputStream, myDataFramework);
    Base::Console().Message("Data Framework Dump Below\n");
    Base::Console().Message(outputStream.str().c_str());
}
