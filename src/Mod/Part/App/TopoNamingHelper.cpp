#include <vector>
#include <type_traits>

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

#include <TNaming.hxx>
#include <TNaming_Builder.hxx>
#include <TNaming_Selector.hxx>
#include <TNaming_NamedShape.hxx>
#include <TNaming_UsedShapes.hxx>
#include <TNaming_Tool.hxx>
#include <TNaming_Naming.hxx>

#include <BRepTools.hxx>
#include <BRep_Tool.hxx>

#ifndef NO_ZIPIOS
#include <zipios++/zipfile.h>
#include <zipios++/zipinputstream.h>
#include <zipios++/zipoutputstream.h>
#include <zipios++/meta-iostreams.h>
#endif


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
    std::clog << "----------TopoNamingHelper = called" << std::endl;
    this->myDataFramework = helper.myDataFramework;
    this->myRootNode      = helper.myRootNode;
    this->mySelectionNode = helper.mySelectionNode;
}

void TopoNamingHelper::TrackShape(const TopoData& TData, const std::string& TargetTag){
    TDF_Label TargetNode = LabelFromTag(TargetTag);
    if (!TargetNode.IsNull()){
        // create new node for modified shape and sub-nodes. Even if there are no
        // Modified/Generated/Deleted Faces, we'll still create the node so we know what's
        // where.
        TDF_Label NewNode = TDF_TagSource::NewChild(TargetNode);

        TNaming_Builder Builder(NewNode);
        AddTextToLabel(NewNode, TData.text);
        if (TData.OldShape.IsNull()){
            Builder.Generated(TData.NewShape);
        }
        else{
            Builder.Modify(TData.OldShape, TData.NewShape);
        }
        // Add the NewShape as a modification of the original shape
        
        // Add descriptive data for debugging purposes

        // Create subnodes for appropriate Topo Data and Builders, but only if necessary
        if (TData.GeneratedFaces.size() > 0){
            TDF_Label Generated = TDF_TagSource::NewChild(NewNode);
            AddTextToLabel(Generated, "Generated faces");
            this->MakeNodes<TopoDS_Face>(Generated, TData.GeneratedFaces, TNaming_GENERATED);
        }

        if (TData.ModifiedFaces.size() > 0){
            TDF_Label Modified  = TDF_TagSource::NewChild(NewNode);
            AddTextToLabel(Modified, "Modified faces");
            this->MakeNodes<TopoDS_Face>(Modified, TData.ModifiedFaces, TNaming_MODIFY);
        }

        if (TData.DeletedFaces.size() > 0){
            TDF_Label Deleted   = TDF_TagSource::NewChild(NewNode);
            AddTextToLabel(Deleted, "Deleted faces");
            this->MakeNodes<TopoDS_Face>(Deleted, TData.DeletedFaces, TNaming_DELETE);
        }

        if (TData.GeneratedFacesFromVertex.size() > 0){
            TDF_Label Generated = TDF_TagSource::NewChild(NewNode);
            AddTextToLabel(Generated, "Faces generated from Vertex");
            this->MakeNodes<TopoDS_Vertex>(Generated, TData.GeneratedFacesFromVertex, TNaming_GENERATED);
        }

        if (TData.GeneratedFacesFromEdge.size() > 0){
            TDF_Label Generated = TDF_TagSource::NewChild(NewNode);
            AddTextToLabel(Generated, "Faces generated from Edge");
            this->MakeNodes<TopoDS_Edge>(Generated, TData.GeneratedFacesFromEdge, TNaming_GENERATED);
        }

        if (TData.TranslatedFaces.size() > 0){
            TDF_Label Translated = TDF_TagSource::NewChild(NewNode);
            AddTextToLabel(Translated, "Translated faces");
            this->MakeTranslatedNodes(Translated, TData.TranslatedFaces);
        }

    }
    else{
        throw std::runtime_error("----------ERROR!!!!! TARGETNODE WAS NULL!!!!");
    }
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
bool TopoNamingHelper::AppendTopoHistory(const std::string& TargetRoot, const TopoNamingHelper& SourceData){
    TDF_Label TargetNode  = this->LabelFromTag(TargetRoot);
    TDF_Label BaseInput = SourceData.LabelFromTag(SourceData.GetTipNode());
    //std::clog << "----------Dumping SourceData in AppendTopoHistory" << std::endl;
    //std::clog << SourceData.DeepDump() << std::endl;
    if (BaseInput.NbChildren() - TargetNode.NbChildren() <= 0){
        // Note, should NOT be less than 0
        return false;
    }
    else{
        // Loop over every node in the SourceData that is not in the TargetRoot
        for (int i = (TargetNode.NbChildren() + 1); i <= BaseInput.NbChildren(); i++){
            // This is the new node to add
            TDF_Label SourceNode = BaseInput.FindChild(i, false);
            // This method should recursiviely append all children from SourceNode into
            // TargetNode...
            this->AppendNode(TargetNode, SourceNode);
        }
    }
    //std::clog << "----------Dumping TargetData in AppendTopoHistory at end" << std::endl;
    //std::clog << this->DeepDump() << std::endl;
    return true;
}

TopoDS_Edge TopoNamingHelper::GetSelectedEdge(const std::string NodeTag) const{
    std::clog << "----------Retrieving edge for tag: " << NodeTag <<  std::endl;
    TDF_Label EdgeNode = this->LabelFromTag(NodeTag);
    TDF_LabelMap MyMap;

    //TDF_ChildIterator it(myRootNode, true);
    //for (;it.More(); it.Next()){
        //std::clog << "----------Adding a label to the map" << std::endl;
        //TDF_Label label = it.Value();
        //MyMap.Add(label);
    //}

    if (!EdgeNode.IsNull()){
        //MyMap.Add(EdgeNode);
        TNaming_Selector MySelector(EdgeNode);
        std::clog << "EdgeNode.hasTNaming = " << EdgeNode.IsAttribute(TNaming_Naming::GetID()) << std::endl;
        bool solved = MySelector.Solve(MyMap);
        if (!solved){
            std::clog << "----------selection solve was NOT succesful......" << std::endl;
        }
        else{
            std::clog << "----------selection solve WAS succesful......" << std::endl;
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
    TDF_Label TargetNode = this->LabelFromTag(NodeTag);
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
    const std::string& tipTag = this->GetTipNode();
    TDF_Label tipLabel = this->LabelFromTag(tipTag);
    TopoDS_Shape tipShape = this->GetChildShape(tipLabel, 0);
    return tipShape;
}

TopoDS_Shape TopoNamingHelper::GetTipShape(const std::string& ParentTag) const {
    const std::string& tipTag = this->GetTipNode(ParentTag);
    TDF_Label tipLabel = this->LabelFromTag(tipTag);
    TopoDS_Shape tipShape = this->GetChildShape(tipLabel, 0);
    return tipShape;
}

std::string TopoNamingHelper::GetTipNode() const{
    return this->GetNode(this->myRootNode.NbChildren());
}

std::string TopoNamingHelper::GetTipNode(const std::string& ParentTag) const{
    TDF_Label Label = this->LabelFromTag(ParentTag);
    return this->GetNode(ParentTag, Label.NbChildren());
}

std::string TopoNamingHelper::GetNode(const int& n) const{
    return this->GetNode("0", n);
}

std::string TopoNamingHelper::GetNode(const std::string& tag, const int& n) const{
    TDF_Label parent = this->LabelFromTag(tag);
    TDF_Label outLabel = parent.FindChild(n, Standard_False);
    return this->GetTag(outLabel);
}

std::string TopoNamingHelper::GetTag(const TDF_Label& Label) const{
    TCollection_AsciiString outtag;
    TDF_Tool::Entry(Label, outtag);
    return outtag.ToCString();
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
    std::clog << "Outshape.IsNull() = " << OutShape.IsNull() <<"\n";
    return OutShape;
}

bool TopoNamingHelper::HasNodes() const{
    bool out = false;
    int numb = this->myRootNode.NbChildren();
    if (numb > 1){
        out = true;
    }
    return out;
}

std::string TopoNamingHelper::AddNode(const std::string& Name){
    TDF_Label label = TDF_TagSource::NewChild(this->myRootNode);
    this->AddTextToLabel(label, Name);
    return GetTag(label);
}

void TopoNamingHelper::AddTextToLabel(const TDF_Label& Label, const std::string& name, const std::string& extra){
    if (!Label.IsAttribute(TDataStd_AsciiString::GetID())){
        // Join name and str
        std::ostringstream stream;
        stream << "Name: " << name ;
        if (!extra.empty()){
            stream << ", " << extra;
        }
        Handle(TDataStd_AsciiString) nameAttribute;
        TCollection_AsciiString myName;
        myName = stream.str().c_str();
        nameAttribute = new TDataStd_AsciiString();
        nameAttribute->Set(myName);
        Label.AddAttribute(nameAttribute);
    }
}

std::string TopoNamingHelper::GetTextFromLabel(const TDF_Label& Label) const{
    std::ostringstream out;
    if (Label.IsAttribute(TDataStd_AsciiString::GetID())){
        Handle(TDataStd_AsciiString) data;
        Label.FindAttribute(TDataStd_AsciiString::GetID(), data);
        data->Get().Print(out);
    }
    else{
        out << "";
    }
    return out.str();
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
                //std::clog << "----------Found match!" << std::endl;
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
    //std::clog << "----------All edges match!" << std::endl;

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
        if (Surface2->IsKind(STANDARD_TYPE(Geom_Plane))) {
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
            return false;
        }
    }
    else{
        std::clog << "----------Don't know how to handle that geometry..." << std::endl;
    }
    // else if (IsKind(cylindrical something)
    return false;
}

bool TopoNamingHelper::CheckIfFacIsDisplaced(const TopoDS_Face& face1, const TopoDS_Face& face2, gp_Trsf& deltaLoc){
    Handle(Geom_Plane) Plane1, Plane2;
    //if (face1.Location().IsEqual(face2.Location())){
        //return false;
    //}
    //else{
        //return true;
    //}
    if (!TopoNamingHelper::GetPlaneFromFace(face1, Plane1) || !TopoNamingHelper::GetPlaneFromFace(face2, Plane2)){
        return false;
    }
    else{
        if (Plane1->Location().IsEqual(Plane2->Location(), Precision::Confusion())){
            return false;
        }
        else{
            // Alright, they're both planes and they don't have the same location. If they
            // ave the same normal then  they are translated
            gp_Pnt loc1 = Plane1->Location();
            gp_Pnt loc2 = Plane2->Location();
            deltaLoc.SetTranslation(loc1, loc2);
            TopoDS_Face check(face1);
            check.Move(deltaLoc);
            return TopoNamingHelper::CompareTwoFaceTopologies(check, face2);
        }
    }
}

void TopoNamingHelper::WriteShape(const TopoDS_Shape& aShape, const std::string& NameBase, const int& numb){
    std::ostringstream outname;
    outname << NameBase;
    if (numb >= 0){
        outname << "_" << numb;
    }
    outname << ".brep";
    std::clog << "----------writing " << outname.str() << std::endl;
    BRepTools::Write(aShape, outname.str().c_str());
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
        for (int i=2; i <= curLabel.Depth(); i++){
            stream << "--";
        }
        // add the Tag info
        curLabel.EntryDump(stream);
        // Add data from AsciiString attribute
        stream << " " << this->GetTextFromLabel(curLabel);
        if (curLabel.IsAttribute(TNaming_NamedShape::GetID())){
            stream << ", Evolution = ";
            Handle(TNaming_NamedShape) curNS;
            curLabel.FindAttribute(TNaming_NamedShape::GetID(), curNS);
            switch (curNS->Evolution()) {
                case TNaming_PRIMITIVE:{
                    stream << "PRIMITIVE";
                    break;}
                case TNaming_GENERATED:{
                    stream << "GENERATED";
                    break;}
                case TNaming_MODIFY   :{
                    stream << "MODIFY";
                    break;}
                case TNaming_DELETE   :{
                    stream << "DELETE";
                    break;}
                default:{
                    stream << "???";
                    break;}
            }
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
    this->WriteShape(ShapeToWrite, NameBase, numb);
}

void TopoNamingHelper::WriteNode(const std::string NodeTag, const std::string NameBase, const bool Deep) const{
    TDF_Label WriteNode = this->LabelFromTag(NodeTag);
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

//TopoDS_Shape TopoNamingHelper::GetGeneratedShape(const TDF_Label& parent, const int& node){
    //// The Generated node should always be the first one...
    //std::clog << "Trying to get node = "<< node << std::endl;
    //return this->GetChildShape(this->GetNode(parent, 1), node);
//}

TopoDS_Shape TopoNamingHelper::GetLatestShape(const std::string& tag){
    TDF_Label Node = this->LabelFromTag(tag);
    Handle(TNaming_NamedShape) ShapeNS;
    Node.FindAttribute(TNaming_NamedShape::GetID(), ShapeNS);
    return TNaming_Tool::CurrentShape(ShapeNS);
}

//-------------------- Private Methods --------------------
TDF_Label TopoNamingHelper::LabelFromTag(const std::string& tag) const{
    TDF_Label outLabel;
    TDF_Tool::Label(myDataFramework, tag.c_str(), outLabel);
    return outLabel;
}

void TopoNamingHelper::AppendNode(const TDF_Label& TargetParent, const TDF_Label& SourceParent){
    TDF_Label NewNode = TDF_TagSource::NewChild(TargetParent);
    if (SourceParent.IsAttribute(TNaming_NamedShape::GetID())){
        TNaming_Builder Builder(NewNode);
        Handle(TNaming_NamedShape) SourceNS;
        SourceParent.FindAttribute(TNaming_NamedShape::GetID(), SourceNS);
        switch (SourceNS->Evolution()){
            case TNaming_PRIMITIVE:{
                TopoDS_Shape NewShape = TNaming_Tool::GetShape(SourceNS);
                Builder.Generated(NewShape);
                break;}
            case TNaming_GENERATED:{
                TopoDS_Shape OldShape = TNaming_Tool::OriginalShape(SourceNS);
                TopoDS_Shape NewShape = TNaming_Tool::GetShape(SourceNS);
                Builder.Generated(OldShape, NewShape);
                break;}
            case TNaming_MODIFY   :{
                TopoDS_Shape OldShape = TNaming_Tool::OriginalShape(SourceNS);
                TopoDS_Shape NewShape = TNaming_Tool::GetShape(SourceNS);
                Builder.Modify(OldShape, NewShape);
                break;}
            case TNaming_DELETE   :{
                TopoDS_Shape OldShape = TNaming_Tool::OriginalShape(SourceNS);
                Builder.Delete(OldShape);
                break;}
            default:{
                std::runtime_error("Do not recognize this TNaming Evolution..."); }
        }
    }
    else{
        std::clog << "Tag " << this->GetTag(SourceParent) << " in SourceParent DOES NOT have a TNaming_NamedShape..." << std::endl;
    }
    if (SourceParent.IsAttribute(TDataStd_AsciiString::GetID())){
        this->AddTextToLabel(NewNode, this->GetTextFromLabel(SourceParent));
    }
    for (TDF_ChildIterator it(SourceParent, false); it.More(); it.Next()){
        this->AppendNode(NewNode, it.Value());
    }
}

bool TopoNamingHelper::GetPlaneFromFace(const TopoDS_Face& face, Handle(Geom_Plane)& plane){
    Handle(Geom_Surface) surface = BRep_Tool::Surface(TopoDS::Face(face));
    if(surface->IsKind(STANDARD_TYPE(Geom_Plane))){
        plane = Handle(Geom_Plane)::DownCast(surface);
        return true;
    }
    else{
        return false;
    }
}

//void TopoNamingHelper::MakeGeneratedNode(const TDF_Label& Parent, const TopoDS_Face& aFace){
    //TDF_Label childLabel = TDF_TagSource::NewChild(Parent);
    //TNaming_Builder Builder(childLabel);
    //Builder.Generated(aFace);
//}

//void TopoNamingHelper::MakeGeneratedNodes(const TDF_Label& Parent, const std::vector<TopoDS_Face>& Faces){
    //TDF_Label childLabel = TDF_TagSource::NewChild(Parent);
    //this->AddTextToLabel(childLabel, "Generated Faces");
    //for (auto&& aFace : Faces){
        //this->MakeGeneratedNode(childLabel, aFace);
    //}
//}

//void TopoNamingHelper::MakeGeneratedFromEdgeNode(const TDF_Label& Parent, const std::pair<TopoDS_Edge, TopoDS_Face>& aPair){
    //TDF_Label childLabel = TDF_TagSource::NewChild(Parent);
    //TNaming_Builder Builder(childLabel);
    //Builder.Generated(std::get<0>(aPair), std::get<1>(aPair));
//}

//void TopoNamingHelper::MakeGeneratedFromEdgeNodes(const TDF_Label& Parent, const std::vector< std::pair<TopoDS_Edge, TopoDS_Face> >& Pairs){
    //TDF_Label childLabel = TDF_TagSource::NewChild(Parent);
    //this->AddTextToLabel(childLabel, "Faces Generated from Edges");
    //for (auto&& aPair : Pairs){
        //this->MakeGeneratedFromEdgeNode(childLabel, aPair);
    //}
//}

//void TopoNamingHelper::MakeGeneratedFromVertexNode(const TDF_Label& Parent, const std::pair<TopoDS_Vertex, TopoDS_Face>& aPair){
    //TDF_Label childLabel = TDF_TagSource::NewChild(Parent);
    //TNaming_Builder Builder(childLabel);
    //Builder.Generated(std::get<0>(aPair), std::get<1>(aPair));
//}

//void TopoNamingHelper::MakeGeneratedFromVertexNodes(const TDF_Label& Parent, const std::vector< std::pair<TopoDS_Vertex, TopoDS_Face> >& Pairs){
    //TDF_Label childLabel = TDF_TagSource::NewChild(Parent);
    //this->AddTextToLabel(childLabel, "Faces generated from Vertexes");
    //for (auto&& aPair : Pairs){
        //this->MakeGeneratedFromVertexNode(childLabel, aPair);
    //}
//}

//void TopoNamingHelper::MakeModifiedNode(const TDF_Label& Parent, const std::pair<TopoDS_Face, TopoDS_Face>& aPair){
    //TDF_Label childLabel = TDF_TagSource::NewChild(Parent);
    //TNaming_Builder Builder(childLabel);
    //std::string tag = this->GetTag(childLabel);
    //std::ostringstream fname1, fname2;
    //fname1 << "16_oldShape_" << tag.back();
    //fname2 << "16_newShape_" << tag.back();
    //this->WriteShape(std::get<0>(aPair), fname1.str());
    //this->WriteShape(std::get<1>(aPair), fname2.str());
    //Builder.Modify(std::get<0>(aPair), std::get<1>(aPair));
//}
//void TopoNamingHelper::MakeModifiedNodes(const TDF_Label& Parent, const std::vector< std::pair<TopoDS_Face, TopoDS_Face> >& aPairs){
    //for (auto&& aPair: aPairs){
        //this->MakeModifiedNode(Parent, aPair);
    //}
//}
//void TopoNamingHelper::MakeDeletedNode(const TDF_Label& Parent, const TopoDS_Face& aFace){
    //TDF_Label childLabel = TDF_TagSource::NewChild(Parent);
    //TNaming_Builder Builder(childLabel);
    //Builder.Delete(aFace);
//}
//void TopoNamingHelper::MakeDeletedNodes(const TDF_Label& Parent, const std::vector<TopoDS_Face>& Faces){
    //for (auto&& aFace : Faces){
        //this->MakeDeletedNode(Parent, aFace);
    //}
//}

template <typename T>
void TopoNamingHelper::MakeNodes(const TDF_Label& Parent, const std::vector<TopoData::TrackedData<T>>& Datas, const TNaming_Evolution& evol){
    for (auto&& aData : Datas){
        TDF_Label label = TDF_TagSource::NewChild(Parent);
        this->AddTextToLabel(label, aData.text);

        auto oldShape = aData.origShape;
        switch (evol){
            case TNaming_GENERATED:{
                TNaming_Builder builder(label);
                builder.Generated(oldShape, aData.newFace);
                break;}
            case TNaming_MODIFY   :{
                TNaming_Builder builder(label);
                builder.Modify(oldShape, aData.newFace);
                break;}
            case TNaming_DELETE   :{
                TNaming_Builder builder(label);
                builder.Delete(oldShape);
                break;}
            default:{
                std::runtime_error("Do not recognize this TNaming Evolution..."); }
        }
    }
}

void TopoNamingHelper::MakeTranslatedNode(const TDF_Label& Parent, const std::pair<std::string, gp_Trsf>& aPair){
    //TDF_Label childLabel = TDF_TagSource::NewChild(Parent);
    //TNaming_Builder
    TopoDS_Shape oldFace = this->GetNodeShape(std::get<0>(aPair));
    TNaming::Displace(this->LabelFromTag(std::get<0>(aPair)), std::get<1>(aPair), true);
    TopoDS_Shape newFace = this->GetNodeShape(std::get<0>(aPair));
    this->WriteShape(oldFace, "16_oldShape_translated");
    this->WriteShape(newFace, "16_newShape_translated");
}
void TopoNamingHelper::MakeTranslatedNodes(const TDF_Label& Parent, const std::vector< std::pair<std::string, gp_Trsf> >& Pairs){
    for (auto&& aPair : Pairs){
        this->MakeTranslatedNode(Parent, aPair);
    }
}
