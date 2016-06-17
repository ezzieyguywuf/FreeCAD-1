#include <iostream>
#include <TDF_Data.hxx>
#include <TDF_Label.hxx>

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

        void TrackGeneratedShape(TopoDS_Shape GeneratedShape) const;
        void TrackFuseOperation(BRepAlgoAPI_Fuse Fuser) const;
        void TrackFilletOperation(TopTools_ListOfShape Edges, TopoDS_Shape BaseShape, BRepFilletAPI_MakeFillet Filleter) const;
        void AddTextToLabel(TDF_Label& Label, char const *str) const;
        void Dump() const;
        void Dump(std::ostream& stream) const;
        void DeepDump(std::ostream& stream) const;
        void DeepDump() const;

    private:
        Handle(TDF_Data) myDataFramework = new TDF_Data();
        TDF_Label myRootNode = myDataFramework->Root();
};
