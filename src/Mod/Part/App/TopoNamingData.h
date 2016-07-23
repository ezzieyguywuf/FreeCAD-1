#ifndef TopoNamingData_H
#define TopoNamingData_H

#include <vector>
#include <array>
#include <TopoDS_Shape.hxx>

//enum class BoxState{
    //length     = 1<<0,
    //height     = 1<<1,
    //width      = 1<<2,
    //translated = 1<<3,
//};

struct TopoData{
    TopoDS_Shape NewShape;
    std::vector<TopoDS_Face> GeneratedFaces;
    std::vector< std::pair<TopoDS_Face, TopoDS_Face> > ModifiedFaces;
    std::vector<TopoDS_Face> DeletedFaces;
};

struct FilletData : TopoData{
    std::vector< std::pair<TopoDS_Edge, TopoDS_Face> > GeneratedFacesFromEdge;
    std::vector< std::pair<TopoDS_Vertex, TopoDS_Face> > GeneratedFacesFromVertex;
};

struct BoxData{
    BoxData(double height, double length, double width){
        Height = height;
        Length = length;
        Width = width;
    }
    double Height=0;
    double Length=0;
    double Width=0;
};
#endif /* ifndef TopoNamingData_H */
