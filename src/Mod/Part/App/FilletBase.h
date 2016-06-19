#ifndef FILLET_BASE_H
#define FILLET_BASE_H

#include "PartFeature.h"
#include "TopoShape.h"
#include "PropertyTopoShape.h"
#include <App/GeoFeature.h>
#include <App/FeaturePython.h>
#include <App/PropertyGeo.h>
// includes for findAllFacesCutBy()
#include <TopoDS_Face.hxx>
#include "FilletBase.h"

using namespace Part;

class FilletBase : public Part::Feature
{
    PROPERTY_HEADER(Part::FilletBase);

public:
    FilletBase();

    App::PropertyLink   Base;
    Part::PropertyFilletEdges Edges;

    void setEdge(int id, double r1, double r2);
    void setEdges(const std::vector<Part::FilletElement>& values);

    short mustExecute() const;
};

#endif // FILLET_BASE_H
