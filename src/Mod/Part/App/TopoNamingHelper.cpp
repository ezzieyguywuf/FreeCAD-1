#include "PreCompiled.h"

// NOTE:: ifdef rather than ifndef. This is for integration with my other local
// freecadTopoTesting thing
#ifdef _PreComp_
#include <zipios++/zipfile.h>
#include <zipios++/zipinputstream.h>
#include <zipios++/zipoutputstream.h>
#include <zipios++/meta-iostreams.h>
#endif

#include <vector>

#include <Geom_Plane.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Geom_Line.hxx>
#include <Precision.hxx>

#include <TDataStd_AsciiString.hxx>
#include <TDF_IDFilter.hxx>
#include <TDF_TagSource.hxx>
#include <TDF_Tool.hxx>
#include <TDF_ChildIterator.hxx>
#include <TDF_LabelMap.hxx>

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
#include <TNaming_UsedShapes.hxx>
#include <TNaming_Tool.hxx>

#include <BRepTools.hxx>
#include <BRep_Tool.hxx>

TopoNamingHelper::TopoNamingHelper(){
    mySelectionNode = TDF_TagSource::NewChild(myRootNode);
    AddTextToLabel(mySelectionNode, "Selection Root Node");
}

TopoNamingHelper::TopoNamingHelper(const TopoNamingHelper& existing){
    this->myDataFramework = existing.myDataFramework;
    this->myRootNode      = existing.myRootNode;
    this->mySelectionNode = existing.mySelectionNode;
}

TopoNamingHelper::~TopoNamingHelper(){
}

void TopoNamingHelper::operator = (const TopoNamingHelper& helper){
    //std::clog << "----------Setting operator = TopoNaming stuff\n";
    this->myDataFramework = helper.myDataFramework;
    this->myRootNode      = helper.myRootNode;
    this->mySelectionNode = helper.mySelectionNode;
}

void TopoNamingHelper::TrackGeneratedShape(const TopoDS_Shape& GeneratedShape, const std::string& name){
    TopoData FaceData;

    TopTools_IndexedMapOfShape mapOfFaces;
    TopExp::MapShapes(GeneratedShape, TopAbs_FACE, mapOfFaces);
    for (int i=1; i <= mapOfFaces.Extent(); i++){
        TopoDS_Face curFace = TopoDS::Face(mapOfFaces.FindKey(i));
        FaceData.GeneratedFaces.Append(curFace);
    }
    this->TrackGeneratedShape(GeneratedShape, FaceData, name);
}

void TopoNamingHelper::TrackGeneratedShape(const TopoDS_Shape& GeneratedShape, const TopoData& TData, const std::string& name){
    //std::clog << "----------Tracking Generated Shape\n";
    //std::ostringstream outputStream;
    //DeepDump(outputStream);
    //Base::Console().Message(outputStream.str().c_str());
    // Declare variables
    TNaming_Builder* MyBuilderPtr;
    TDF_Label curLabel;

    // create a new node under Root
    TDF_Label LabelRoot = TDF_TagSource::NewChild(myRootNode);

    AddTextToLabel(LabelRoot, "Generated Shape node", name);

    // add the generated shape to the LabelRoot
    MyBuilderPtr = new TNaming_Builder(LabelRoot);
    MyBuilderPtr->Generated(GeneratedShape);

    // Now iterate over each face and add as a child to the new node we created
        // Create a new sub-node for the Face
    TopTools_ListIteratorOfListOfShape genIterator(TData.GeneratedFaces);
    for (; genIterator.More(); genIterator.Next()){
        TopoDS_Face curFace = TopoDS::Face(genIterator.Value());
        curLabel = TDF_TagSource::NewChild(LabelRoot);
        // add Face to the sub-node
        delete MyBuilderPtr;
        MyBuilderPtr = new TNaming_Builder(curLabel);
        MyBuilderPtr->Generated(curFace);
    }
    //std::ostringstream outputStream2;
    //DeepDump(outputStream2);
    ////std::clog << "----------Data Framework Dump Below\n";
    //Base::Console().Message(outputStream2.str().c_str());
}

void TopoNamingHelper::TrackFuseOperation(BRepAlgoAPI_Fuse& Fuser){
    //std::clog << "----------Tracking Fuse Operation\n";
    // TODO: Need to update to account for an abritrary number of shapes being fused.
    // In a Fuse operation, each face is either Modified from one of the -two- (scratch
    // that) 'many' Shapes being fused, or it is Deleted. There is not an instance where a
    // Fuse operation results in a Generated Face.
    TopoDS_Shape BaseShape   = Fuser.Shape1();
    TopoDS_Shape FuseShape   = Fuser.Shape2();
    TopoDS_Shape ResultShape = Fuser.Shape();

    //TopTools_ListOfShape 

    // create two new nodes under Root. The first is to track the 'Shape1' as a
    // 'Generated' shape, and the second is to track the ResultShape, with  sub-nodes for
    // the modified faces on the BaseShape and the 'Shape1' as well. TODO: Need to figure
    // out a way to incorporate the topo tree from Shape1 into our topo tree, if Shape1
    // has one.
    //std::clog << "----------Note: next 'tracking generated shape' msg from TrackFuseOperation\n";
    this->TrackGeneratedShape(FuseShape);
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
    //std::ostringstream outputStream;
    //DeepDump(outputStream);
    ////std::clog << "----------Data Framework Dump Below\n";
    //Base::Console().Message(outputStream.str().c_str());
}

void TopoNamingHelper::TrackFilletOperation(const TopoDS_Shape& BaseShape, BRepFilletAPI_MakeFillet& mkFillet){
    BRepFilletAPI_MakeFillet Filleter = mkFillet;
    //std::clog << "----------Tracking Fillet Operation\n";
    //std::ostringstream output;
    //DeepDump(output);
    //Base::Console().Message(output.str().c_str());

    TopoDS_Shape ResultShape = Filleter.Shape();

    // Create a new node under the Root node for the result filleted Shape and it's
    // modified/deleted/generated Faces.
    TDF_Label FilletRoot        = TDF_TagSource::NewChild(myRootNode);
    TDF_Label Modified          = TDF_TagSource::NewChild(FilletRoot);
    TDF_Label Deleted           = TDF_TagSource::NewChild(FilletRoot);
    TDF_Label FacesFromEdges    = TDF_TagSource::NewChild(FilletRoot);
    TDF_Label FacesFromVertices = TDF_TagSource::NewChild(FilletRoot);

    // Add some descriptive text for debugging
    AddTextToLabel(FilletRoot, "Fillet Node");
    AddTextToLabel(Modified, "Modified faces");
    AddTextToLabel(Deleted, "Deleted faces");
    AddTextToLabel(FacesFromEdges, "Faces from edges");
    AddTextToLabel(FacesFromVertices, "Faces from vertices");

    // Start by adding the result shape. This will also create the TNaming_UsedShapes
    // under the Root node if it doesn't exist
    TNaming_Builder FilletBuilder(FilletRoot);
    FilletBuilder.Modify(BaseShape, ResultShape);

    // Next, the Faces generated from Edges
    TNaming_Builder FacesFromEdgeBuilder(FacesFromEdges);
    TopTools_IndexedMapOfShape mapOfEdges;
    TopExp::MapShapes(BaseShape, TopAbs_EDGE, mapOfEdges);
    for (int i=1; i<=mapOfEdges.Extent(); i++){
        const TopoDS_Edge& curEdge = TopoDS::Edge(mapOfEdges.FindKey(i));
        const TopTools_ListOfShape& generatedFaces = Filleter.Generated(curEdge);
        TopTools_ListIteratorOfListOfShape it(generatedFaces);
        for (;it.More(); it.Next()){
            const TopoDS_Shape& checkShape = it.Value();
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
        const TopoDS_Face& curFace = TopoDS::Face(mapOfFaces.FindKey(i));

        // First check Modified
        const TopTools_ListOfShape& modifiedFaces = Filleter.Modified(curFace);
        TopTools_ListIteratorOfListOfShape it(modifiedFaces);
        for (; it.More(); it.Next()){
            const TopoDS_Shape& checkShape = it.Value();
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
        const TopoDS_Shape& curVertex = mapOfVertices.FindKey(i);
        const TopTools_ListOfShape& generatedFaces = Filleter.Generated(curVertex);
        TopTools_ListIteratorOfListOfShape it(generatedFaces);
        for (;it.More(); it.Next()){
            const TopoDS_Shape& checkShape = it.Value();
            if (!curVertex.IsSame(checkShape)){
                FacesFromEdgeBuilder.Generated(curVertex, checkShape);
            }
        }
    }
    //std::ostringstream outputStream;
    //DeepDump(outputStream);
    //std::clog << "----------Data Framework Dump Below\n";
    //Base::Console().Message(outputStream.str().c_str());
}

void TopoNamingHelper::TrackModifiedShape(const TopoDS_Shape& NewShape, const TopoData& TData){
    std::ostringstream tipTagStream;
    tipTagStream << "0:" << this->myRootNode.NbChildren();
    this->TrackModifiedShape(tipTagStream.str(), NewShape, TData);
}

void TopoNamingHelper::TrackModifiedShape(const std::string& OrigShapeNodeTag, const TopoDS_Shape& NewShape, const TopoData& TData){
    // NOTE: This method assumes that the NewShape has NOT been translated. If it has, the
    // behaviour of the topological naming algorithm is not defined, it will probably fail
    TDF_Label OrigNode;
    TDF_Tool::Label(myDataFramework, OrigShapeNodeTag.c_str(), OrigNode);

    if (!OrigNode.IsNull()){
        // create new node for modified shape
        TDF_Label NewNode = TDF_TagSource::NewChild(myRootNode);
        AddTextToLabel(NewNode, "Modified Node");

        // Create subnodes for appropriate Topo Data and Builders, but only if necessary
        if (TData.GeneratedFaces.Size() > 0){
            TDF_Label Generated = TDF_TagSource::NewChild(NewNode);
            AddTextToLabel(Generated, "Generated faces");
            TNaming_Builder GeneratedBuilder(Generated);

            TopTools_ListIteratorOfListOfShape genIterator(TData.GeneratedFaces);
            for (; genIterator.More(); genIterator.Next()){
                TopoDS_Face gennedFace = TopoDS::Face(genIterator.Value());
                GeneratedBuilder.Generated(gennedFace);
            }
        }

        if (TData.ModifiedFaces.size() > 0){
            TDF_Label Modified  = TDF_TagSource::NewChild(NewNode);
            AddTextToLabel(Modified, "Modified faces");
            TNaming_Builder ModifiedBuilder(Modified);

            for (std::vector< std::vector<TopoDS_Face> >::const_iterator it = TData.ModifiedFaces.begin(); it != TData.ModifiedFaces.end(); ++it){
                std::vector<TopoDS_Face> row = *it;
                TopoDS_Face origFace = TopoDS::Face(row[0]);
                TopoDS_Face newFace = TopoDS::Face(row[1]);
                ModifiedBuilder.Modify(origFace, newFace);
            }
        }

        if (TData.DeletedFaces.Size() > 0){
            TDF_Label Deleted   = TDF_TagSource::NewChild(NewNode);
            AddTextToLabel(Deleted, "Deleted faces");
            TNaming_Builder DeletedBuilder(Deleted);

            TopTools_ListIteratorOfListOfShape delIterator(TData.DeletedFaces);
            for (; delIterator.More(); delIterator.Next()){
                TopoDS_Face deletedFace = TopoDS::Face(delIterator.Value());
                DeletedBuilder.Delete(deletedFace);
            }
        }

    }
    else{
        throw std::runtime_error("----------ERROR!!!!! ORIGNODE WAS NULL!!!!");
    }
}

void TopoNamingHelper::TrackModifiedFilletBaseShape(const TopoDS_Shape& NewBaseShape){
    // TODO: How can we make sure that node "0:2" is _always_ the first instance of the
    // Base Shape in a Filleted Shape Data Framework? Is that already taken care of based
    // on FeatureFillet is using the TopoShape access methods?
}

std::string TopoNamingHelper::SelectEdge(const TopoDS_Edge& anEdge, const TopoDS_Shape& aShape){
    Handle(TNaming_NamedShape) EdgeNS;
    bool identified = TNaming_Selector::IsIdentified(mySelectionNode, anEdge, EdgeNS);

    std::ostringstream dumpedEntry;
    if (!identified){
        std::clog << "----------Creating selection (did not exist)...\n";
        const TDF_Label SelectedLabel = TDF_TagSource::NewChild(mySelectionNode);
        TNaming_Selector SelectionBuilder(SelectedLabel);
        bool check = SelectionBuilder.Select(anEdge, aShape);
        if (check){
            std::clog << "----------Selection WAS succesfull" << std::endl;
        }
        else{
            std::clog << "----------Selection WAS NOT suffesfull" << std::endl;
        }
        this->AddTextToLabel(SelectedLabel, "A selected edge. Sub-node is the context Shape");
        SelectedLabel.EntryDump(dumpedEntry);
    }
    else{
        std::clog << "----------Node existing, returning existing selection...\n";
        const TDF_Label SelectedLabel = EdgeNS->Label();
        SelectedLabel.EntryDump(dumpedEntry);
    }

    return dumpedEntry.str();
}

std::vector<std::string> TopoNamingHelper::SelectEdges(const std::vector<TopoDS_Edge> Edges,
                                                     const TopoDS_Shape& aShape){
    std::vector<std::string> outputLabels;
    for (std::vector<TopoDS_Edge>::const_iterator it = Edges.begin(); it != Edges.end(); ++it){
        TopoDS_Edge curEdge = *it;
        std::string curLabelEntry = this->SelectEdge(curEdge, aShape);
        outputLabels.push_back(curLabelEntry);
    }
    return outputLabels;
}

TopoDS_Edge TopoNamingHelper::GetSelectedEdge(const std::string NodeTag) const{
    std::clog << "----------Retrieving edge for tag: " << NodeTag <<  std::endl;
    TDF_Label EdgeNode;
    TDF_Tool::Label(myDataFramework, NodeTag.c_str(), EdgeNode);
    TDF_LabelMap MyMap;

    if (!EdgeNode.IsNull()){
        //MyMap.Add(EdgeNode);
        TNaming_Selector MySelector(EdgeNode);
        bool solved = MySelector.Solve(MyMap);
        if (solved){
            std::clog << "----------Selection solve was succesfull!" << std::endl;
        }
        else{
            std::clog << "----------selection solve was not succesful......" << std::endl;
        }
        //Handle(TNaming_NamedShape) EdgeNS = MySelector.NamedShape();
        //EdgeNode.FindAttribute(TNaming_NamedShape::GetID(), EdgeNS);
        //const TopoDS_Edge& SelectedEdge = TopoDS::Edge(MySelector.NamedShape()->Get());
        TopoDS_Edge SelectedEdge = TopoDS::Edge(MySelector.NamedShape()->Get());
        return SelectedEdge;
    }
    else{
        throw std::runtime_error("That Node does not appear to exist on the Data Framework");
    }
}

TopoDS_Shape TopoNamingHelper::GetSelectedBaseShape(const std::string NodeTag) const{
    std::ostringstream baseNodeStream;
    baseNodeStream << NodeTag << ":1";
    std::string baseNodeTag = baseNodeStream.str();
    std::clog << "----------Retrieving Base for tag: " << baseNodeTag <<  std::endl;
    return this->GetNodeShape(baseNodeTag);
}

TopoDS_Shape TopoNamingHelper::GetNodeShape(const std::string NodeTag) const{
    std::ostringstream out;
    out << "----------GetNodeShape for NodeTag = " << NodeTag << std::endl;
    std::clog << out.str();
    TDF_Label TargetNode;
    TDF_Tool::Label(myDataFramework, NodeTag.c_str(), TargetNode);
    //TDF_LabelMap MyMap;

    //TDF_ChildIterator TreeIterator(myRootNode, Standard_True);
    //for(;TreeIterator.More(); TreeIterator.Next()){
        //TDF_Label curLabel = TreeIterator.Value();
        //MyMap.Add(curLabel);
    //}

    if (!TargetNode.IsNull()){
        //TNaming_Selector MySelector(TargetNode);
        //MySelector.Solve(MyMap);
        Handle(TNaming_NamedShape) TargetNS;
        if (TargetNode.IsAttribute(TNaming_NamedShape::GetID())){
            TargetNode.FindAttribute(TNaming_NamedShape::GetID(), TargetNS);
            //TopoDS_Shape OutShape = TNaming_Tool::CurrentShape(TargetNS);
            TopoDS_Shape OutShape = TargetNS->Get();
            //if (OutShape.IsNull()){
                //std::clog << "----------OutShape is NULL..." << std::endl;
            //}
            //else{
                //std::clog << "----------OutShape is NOT NULL!!!!" << std::endl;
            //}
            return OutShape;
        }
        else{
            std::clog << "----------Throwing an error! Node does not contain a NamedShape...\n";
            throw std::runtime_error("That Node does not appear to contain a NamedShape\n");
        }
    }
    else{
        std::clog << "----------Throwing an error! Node doesn't exist...\n";
        throw std::runtime_error("That Node does not appear to exist on the Data Framework\n");
    }
}

TopoDS_Shape TopoNamingHelper::GetTipShape() const {
    const TDF_Label& tipLabel = this->GetTipNode();
    TopoDS_Shape tipShape = this->GetChildShape(tipLabel, 0);
    return tipShape;
}

TDF_Label TopoNamingHelper::GetTipNode() const{
    TDF_Label tipLabel = this->myRootNode.FindChild(this->myRootNode.NbChildren(), Standard_False);
    return tipLabel;
}

TopoDS_Shape TopoNamingHelper::GetChildShape(const TDF_Label& ParentLabel, const int& n) const{
    Handle(TNaming_NamedShape) OutNS;
    if (n > 0){
        TDF_Label ChildLabel = ParentLabel.FindChild(n, false);
        ChildLabel.FindAttribute(TNaming_NamedShape::GetID(), OutNS);
    }
    else if(n == 0){
        ParentLabel.FindAttribute(TNaming_NamedShape::GetID(), OutNS);
    }
    TopoDS_Shape OutShape = TNaming_Tool::GetShape(OutNS);
    return OutShape;
}

std::string TopoNamingHelper::GetLatestFilletBase() const{
    return "";
}

bool TopoNamingHelper::HasNodes() const{
    bool out = false;
    int numb = this->myRootNode.NbChildren();
    if (numb > 1){
        out = true;
    }
    return out;
}

void TopoNamingHelper::AddTextToLabel(const TDF_Label& Label, const char *str, const std::string& name){
    if (!Label.IsAttribute(TDataStd_AsciiString::GetID())){
        // Join name and str
        std::ostringstream stream;
        stream << "Name: " << name << ", " << str;
        Handle(TDataStd_AsciiString) nameAttribute;
        TCollection_AsciiString myName;
        myName = stream.str().c_str();
        nameAttribute = new TDataStd_AsciiString();
        nameAttribute->Set(myName);
        Label.AddAttribute(nameAttribute);
    }
}

bool TopoNamingHelper::CompareTwoEdgeTopologies(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2, int numCheckPoints){
    double c1Start, c1End, c2Start, c2End;
    Handle(Geom_Curve) curve1 = BRep_Tool::Curve(edge1, c1Start, c1End);
    Handle(Geom_Curve) curve2 = BRep_Tool::Curve(edge2, c2Start, c2End);

    // First, check if one is closed and the other isn't
    if ((curve1->IsClosed() && !curve2->IsClosed()) ||
        (!curve1->IsClosed() && curve2->IsClosed())){
        std::clog << "----------One edge is closed" << std::endl;
        // if one is closed and the other isn't they can't be equal.
        return false;
    }

    // Next compare endpoints
    gp_Pnt c1StartPnt = curve1->Value(c1Start);
    gp_Pnt c1EndPnt   = curve1->Value(c1End);
    gp_Pnt c2StartPnt = curve2->Value(c2Start);
    gp_Pnt c2EndPnt   = curve2->Value(c2End);

    // curve2's end point COULD be curve1's start point
    if (!(c1StartPnt.IsEqual(c2StartPnt, Precision::Confusion()) && c1EndPnt.IsEqual(c2EndPnt  , Precision::Confusion())) &&
        !(c1StartPnt.IsEqual(c2EndPnt  , Precision::Confusion()) && c1EndPnt.IsEqual(c2StartPnt, Precision::Confusion()))){
        // if endpoints don't match, can't be the same curve
        //std::clog << "----------End points don't match" << std::endl;
        return false;
    }

    // finally, compare points on the lines
    GeomAPI_ProjectPointOnCurve projector;
    float interval = (c1End - c1Start) / numCheckPoints;

    // NOTE: it should be OK if interval is negative: that just means that c1End is
    // smaller than c1Start, but since we're starting at c1Start, the negative interval
    // takes us in the correct direction.
    for (float point = (c1Start + interval); (point <= c1End && point >=c1Start) || (point >=c1End && point <=c1Start); point += interval){
        gp_Pnt point1 = curve1->Value(point);
        projector.Init(point1, curve2);
        if(projector.LowerDistance() > Precision::Confusion()){
            std::clog << "----------Projection not close enough" << std::endl;
            return false;
        }
    }
    return true;
}

bool TopoNamingHelper::CompareTwoFaceTopologies(const TopoDS_Shape& face1, const TopoDS_Shape& face2){
    TopTools_IndexedMapOfShape Edges1;
    TopTools_IndexedMapOfShape Edges2;
    TopExp::MapShapes(face1, TopAbs_EDGE, Edges1);
    TopExp::MapShapes(face2, TopAbs_EDGE, Edges2);
    if (Edges1.Extent() != Edges2.Extent()) {
        // Different number of edges mean different Faces
        return false;
    }
    for (int i=1; i<= Edges1.Extent(); i++){
        TopoDS_Edge Edge1 = TopoDS::Edge(Edges1.FindKey(i));
        bool match = false;
        for (int j=1; j<= Edges2.Extent(); j++){
            TopoDS_Edge Edge2 = TopoDS::Edge(Edges2.FindKey(j));
            if (BRepTools::Compare(Edge1, Edge2)){
                std::clog << "----------Found match!" << std::endl;
                match = true;
                break;
            }
            if (CompareTwoEdgeTopologies(Edge1, Edge2)){
                //std::clog << "----------Found match (my way)!" << std::endl;
                match = true;
                break;
            }
        }
        if (!match){
            // if any single edge does not match, then the faces cannot be equal.
            //std::clog << "----------No match found..." << std::endl;
            return false;
        }
    }
    std::clog << "----------All edges match!" << std::endl;

    //if (face2.IsEqual(face1)){
        //std::clog << "----------for i = " << i << " and j = "<<j << ", Faces ARE equal" << std::endl;
    //}
    //else{
        //std::clog << "----------for i = " << i << " and j = "<<j << ", Faces ARE NOT equal" << std::endl;
        
    //}
    //if (face2.IsSame(face1)){
        //std::clog << "----------for i = " << i << " and j = "<<j << ", Faces ARE same" << std::endl;
    //}
    //else{
        //std::clog << "----------for i = " << i << " and j = "<<j << ", Faces ARE NOT same" << std::endl;
        
    //}
    //if (face2.IsPartner(face1)){
        //std::clog << "----------for i = " << i << " and j = "<<j << ", Faces ARE partners" << std::endl;
    //}
    //else{
        //std::clog << "----------for i = " << i << " and j = "<<j << ", Faces ARE NOT partners" << std::endl;
        
    //}
    //if (face2.Location() == face1.Location()){
        //std::clog << "----------for i = " << i << " and j = "<<j << ", Faces ARE equal location" << std::endl;
    //}
    //else{
        //std::clog << "----------for i = " << i << " and j = "<<j << ", Faces ARE NOT equal location" << std::endl;
        
    //}
    //if (face2.Orientation() == face1.Orientation()){
        //std::clog << "----------for i = " << i << " and j = "<<j << ", Faces ARE equal orientation" << std::endl;
    //}
    //else{
        //std::clog << "----------for i = " << i << " and j = "<<j << ", Faces ARE NOT equal orientation" << std::endl;
        
    //}

    Handle(Geom_Surface) Surface1 = BRep_Tool::Surface(TopoDS::Face(face1));
    Handle(Geom_Surface) Surface2 = BRep_Tool::Surface(TopoDS::Face(face2));
    if(Surface1->IsKind(STANDARD_TYPE(Geom_Plane))){
        Handle(Geom_Plane) Plane1 = Handle(Geom_Plane)::DownCast(Surface1);
        Handle(Geom_Plane) Plane2 = Handle(Geom_Plane)::DownCast(Surface2);
        //gp_Dir p1Dir = Plane1->Axis().Direction();
        //gp_Dir p2Dir = Plane2->Axis().Direction();
        Standard_Real p1A, p1B, p1C, p1D, p2A, p2B, p2C, p2D;
        Plane1->Coefficients(p1A, p1B, p1C, p1D);
        Plane2->Coefficients(p2A, p2B, p2C, p2D);
        //std::clog << "----------Plane1: " << p1A << "x + " <<p1B << "y + "<<p1C << "z + " << p1D << " = 0.0" << std::endl;
        //std::clog << "----------Plane2: " << p2A << "x + " <<p2B << "y + "<<p2C << "z + " << p2D << " = 0.0" << std::endl;
        //std::clog << "----------Plane1 Dir: (" << p1Dir.X() << ", " << p1Dir.Y() << ", " << p1Dir.Z() << ")" << std::endl;
        //std::clog << "----------Plane2 Dir: (" << p2Dir.X() << ", " << p2Dir.Y() << ", " << p2Dir.Z() << ")" << std::endl;
        if (p1A == p2A && p1B == p2B && p1C == p2C && p1D ==p2D) {
            return true;
        }
    }
    else{
        std::clog << "----------Don't know how to handle that geometry..." << std::endl;
    }
    // else if (IsKind(cylindrical something)
    return false;
}

void TopoNamingHelper::Dump() const{
    TDF_Tool::DeepDump(std::clog, myDataFramework);
    std::clog << "\n";
}

void TopoNamingHelper::Dump(std::ostream& stream) const{
    TDF_Tool::DeepDump(stream, myDataFramework);
    stream << "\n";
}

void TopoNamingHelper::DeepDump(std::stringstream& stream) const{
    //std::clog << "-----TopoNamingHelper::DeepDump(std::ostream...)\n";
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

std::string TopoNamingHelper::DeepDump() const{
    //std::clog << "----------TopoNamingHelper::DeepDump()\n";
    std::stringstream output;
    DeepDump(output);
    //std::clog << output.str();
    return output.str();
}

std::string TopoNamingHelper::DFDump() const{
    std::ostringstream outStream;
    TDF_IDFilter myFilter;
    myFilter.Keep(TDataStd_AsciiString::GetID());
    myFilter.Keep(TNaming_NamedShape::GetID());
    myFilter.Keep(TNaming_UsedShapes::GetID());
    TDF_Tool::ExtendedDeepDump(outStream, myDataFramework, myFilter);
    return outStream.str();
}

void TopoNamingHelper::WriteShape(const TDF_Label aLabel, const std::string NameBase, const int numb) const{
    Handle(TNaming_NamedShape) WriteNS;
    aLabel.FindAttribute(TNaming_NamedShape::GetID(), WriteNS);
    TopoDS_Shape ShapeToWrite = WriteNS->Get();
    std::ostringstream outname;
    outname << NameBase << "_" << numb << ".brep";
    std::clog << "----------writing " << outname.str() << std::endl;
    BRepTools::Write(ShapeToWrite, outname.str().c_str());
}

void TopoNamingHelper::WriteNode(const std::string NodeTag, const std::string NameBase, const bool Deep) const{
    TDF_Label WriteNode;
    TDF_Tool::Label(myDataFramework, NodeTag.c_str(), WriteNode);
    if (!WriteNode.IsNull()){
        this->WriteShape(WriteNode, NameBase, 0);
        if (Deep){
            TDF_ChildIterator NodeIterator(WriteNode, Standard_True);
            int i=1;
            for(;NodeIterator.More(); NodeIterator.Next(), i++){
                TDF_Label curLabel = NodeIterator.Value();
                this->WriteShape(curLabel, NameBase, i);
            }
        }
    }
    else{
        throw std::runtime_error("That Node does not appear to exist on the Data Framework");
    }
}
