#ifndef FILLET_BASE_H
#define FILLET_BASE_H

#include "PartFeature.h"
#include "PropertyTopoShape.h"
#include <App/GeoFeature.h>
#include <App/FeaturePython.h>
#include <App/PropertyGeo.h>

namespace Part{
    class FilletBase : public Part::Feature
    {
        PROPERTY_HEADER(Part::FilletBase);

    public:
        FilletBase();

        App::PropertyLink   Base;
        PropertyFilletEdges Edges;

        void setEdge(int id, double r1, double r2);
        void setEdges(std::vector<Part::FilletElement>& values);
        
        virtual PyObject* getPyObject(void);

        short mustExecute() const;
    private:
        std::string getSelectedEdgeLabel(int id, double r1, double r2) const;
    };
} // namespace Part

#endif // FILLET_BASE_H
