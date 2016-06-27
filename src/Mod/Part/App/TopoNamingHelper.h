#include <iostream>
#include <TDF_Data.hxx>
#include <TDF_Label.hxx>
#include <TDF_TagSource.hxx>

#include <TopoDS_Shape.hxx>

#include <TopTools_Array1OfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>

class TopoNamingHelper{
    public:
        TopoNamingHelper();
        TopoNamingHelper(const TopoNamingHelper& existing);
        ~TopoNamingHelper();

        void operator = (const TopoNamingHelper&);

        void TrackGeneratedShape(const TopoDS_Shape& GeneratedShape, const std::string& name="n/a");
        void TrackFuseOperation(BRepAlgoAPI_Fuse& Fuser);
        void TrackFilletOperation(const TopoDS_Shape& BaseShape, BRepFilletAPI_MakeFillet& mkFillet);
        void AddTextToLabel(const TDF_Label& Label, const char *str, const std::string& name="n/a");
        std::string SelectEdge(const TopoDS_Edge& anEdge, const TopoDS_Shape& aShape);
        std::vector<std::string> SelectEdges(const std::vector<TopoDS_Edge> Edges, const TopoDS_Shape& aShape);
        TopoDS_Edge GetSelectedEdge(const std::string NodeTag) const;
        TopoDS_Shape GetNodeShape(const std::string NodeTag) const;

        // Does the Topo tree have additional nodes aside from the Selection one created
        // at initialization?
        bool HasNodes() const;

        // debugging stuff
        void Dump() const;
        void Dump(std::ostream& stream) const;
        std::string DeepDump() const;
        void DeepDump(std::stringstream& stream) const;

        void WriteShape(const TDF_Label aLabel, const std::string NameBase, const int numb) const;
        void WriteNode(const std::string NodeTag, const std::string NameBase, const bool Deep) const;

    //private:
        bool CheckIfSelectionExists(const TDF_Label aNode, const TopoDS_Shape aShape) const;
        Handle(TDF_Data) myDataFramework = new TDF_Data();
        TDF_Label myRootNode = myDataFramework->Root();
        TDF_Label mySelectionNode;
};
