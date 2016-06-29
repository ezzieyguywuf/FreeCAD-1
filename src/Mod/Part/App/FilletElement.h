#ifndef FILLET_ELEMENT_H
#define FILLET_ELEMENT_H
#include "PreCompiled.h"
#include <string>
namespace Part
{
    /** A property class to store hash codes and two radii for the fillet algorithm.
     * @author Werner Mayer
     */
    struct PartExport FilletElement {
        int edgeid;
        double radius1, radius2;
        std::string edgetag;
    };
}
#endif /* ifndef FILLET_ELEMENT_H */
