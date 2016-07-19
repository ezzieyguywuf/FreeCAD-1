#ifndef TopoNamingData_H
#define TopoNamingData_H

#include <vector>
#include <array>

//enum class BoxState{
    //length     = 1<<0,
    //height     = 1<<1,
    //width      = 1<<2,
    //translated = 1<<3,
//};

struct TopoData{
    TopTools_ListOfShape GeneratedFaces;
    std::vector< std::vector<TopoDS_Face> > ModifiedFaces;
    TopTools_ListOfShape DeletedFaces;
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
