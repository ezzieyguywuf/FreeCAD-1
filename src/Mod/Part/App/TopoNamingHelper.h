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

        void TrackGeneratedShape(const TopoDS_Shape& GeneratedShape);
        void TrackFuseOperation(BRepAlgoAPI_Fuse& Fuser);
        void TrackFilletOperation(const TopTools_ListOfShape& Edges, const TopoDS_Shape& BaseShape, BRepFilletAPI_MakeFillet& mkFillet);
        void AddTextToLabel(const TDF_Label& Label, char const *str);
        void Dump() const;
        void Dump(std::ostream& stream) const;
        void DeepDump(std::ostream& stream) const;
        void DeepDump() const;

    private:
        bool CheckIfSelectionExists(const TDF_Label aNode, const TopoDS_Shape aShape) const;
        Handle(TDF_Data) myDataFramework = new TDF_Data();
        TDF_Label myRootNode = myDataFramework->Root();
};
