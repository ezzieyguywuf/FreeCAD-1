#include <vector>
#include <string>

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

        // Make changes to the Data Framework to track Topological Changes
        void TrackGeneratedShape(const TopoDS_Shape& GeneratedShape, const std::string& name="n/a");
        void TrackFuseOperation(BRepAlgoAPI_Fuse& Fuser);
        void TrackFilletOperation(const TopoDS_Shape& BaseShape, BRepFilletAPI_MakeFillet& mkFillet);
        void TrackModifiedShape(const std::string& OrigShapeNodeTag, const TopoDS_Shape& NewShape);
        void TrackModifiedFilletBaseShape(const TopoDS_Shape& NewBaseShape);
        std::string SelectEdge(const TopoDS_Edge& anEdge, const TopoDS_Shape& aShape);
        std::vector<std::string> SelectEdges(const std::vector<TopoDS_Edge> Edges, const TopoDS_Shape& aShape);

        // Various helper functions

        // This helps make the DeepDump output more legible
        void AddTextToLabel(const TDF_Label& Label, const char *str, const std::string& name="n/a");
        // Returns the edge at the NodeTag, i.e. "0:2"
        TopoDS_Edge GetSelectedEdge(const std::string NodeTag) const;
        // Returns the Context Shape for selected edge located at NodeTag: TODO does this
        // work 100% of the time? It seems sometimes the sub-node is NOT the context
        // shape...
        TopoDS_Shape GetSelectedBaseShape(const std::string NodeTag) const;
        // Return the TopoDS_Shape stored in the NodeTag, i.e. "0:4"
        TopoDS_Shape GetNodeShape(const std::string NodeTag) const;
        // Intended to be used for a Data Framework that is keeping track of the evolution
        // of a Filleted shape. If the Data Framework does not contain a Fillet node, this
        // method may not work as expected
        std::string GetLatestFilletBase() const;
        // Return the TopoDS_Shape at the very tip of the Data Framework. TODO do we need
        // to check to make sure the latest operation actually stored a TopoDS_Shape?
        TopoDS_Shape GetTipShape() const;
        // Does the Topo tree have additional nodes aside from the Selection one created
        // at initialization?
        bool HasNodes() const;

        // Non-Member Class functions
        static bool CompareTwoFaceTopologies(const TopoDS_Shape& face1, const TopoDS_Shape& face2);
        static bool CompareTwoEdgeTopologies(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2, int numCheckPoints=10);


        // debugging stuff

        // Dump actually uses OCC DeepDump. Sort of confusing, TODO should I rename these?
        void Dump() const;
        void Dump(std::ostream& stream) const;
        // A custom Dump function that is more concise and informative than the OCC one
        std::string DeepDump() const;
        void DeepDump(std::stringstream& stream) const;
        // Dump the whole Data Framework and attributes too
        std::string DFDump() const;
        // Write out a BREP file of the TopoDS_Shape at aLabel. The file will be named
        // "<NameBase>_<numb>.brep"
        void WriteShape(const TDF_Label aLabel, const std::string NameBase, const int numb) const;
        // Write out a BREP file of the TopoDS_Shape at the label with the address
        // <NodeTag>. if <Deep> is true, also write out for all children, with
        // <NameBase>_1.brep, <NameBase>_2.brep etc... as the filename.
        void WriteNode(const std::string NodeTag, const std::string NameBase, const bool Deep) const;

    private:
        bool CheckIfSelectionExists(const TDF_Label aNode, const TopoDS_Shape aShape) const;
        Handle(TDF_Data) myDataFramework = new TDF_Data();
        TDF_Label myRootNode = myDataFramework->Root();
        TDF_Label mySelectionNode;
};
