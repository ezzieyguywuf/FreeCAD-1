#ifndef FILLET_ELEMENT
#define FILLET_ELEMENT

#include "PartFeature.h"
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
#endif // FILLET_ELEMENT
