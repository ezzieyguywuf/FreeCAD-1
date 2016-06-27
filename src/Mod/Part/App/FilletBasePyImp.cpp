/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

//#include "PartFeature.h"
#include <vector>
#include "FilletBase.h"
#include "PropertyTopoShape.h"

// inclusion of the generated files (generated out of FilletBasePy.xml)
#include "FilletBasePy.h"
#include "FilletBasePy.cpp"

using namespace Part;

// returns a string which represent the object e.g. when printed in python
std::string FilletBasePy::representation(void) const
{
    return std::string("<Part::FilletBase>");
}

PyObject *FilletBasePy::getCustomAttributes(const char* attr) const
{
    return 0;
}

int FilletBasePy::setCustomAttributes(const char* attr, PyObject *obj)
{
    return 0; 
}


PyObject* FilletBasePy::setEdge(PyObject *args){
    // initialize varibles to prevent compiler warnings
    int index=-1;
    double rad1=-1., rad2=-1.;

    if (!PyArg_ParseTuple(args, "idd", &index, &rad1, &rad2))
        return NULL;

    Part::FilletBase* Base = getFilletBasePtr();
    Base->setEdge(index, rad1, rad2);
    Py_Return; // return None
}

PyObject* FilletBasePy::setEdges(PyObject *args){
    // initialize varibles to prevent compiler warnings
    PyObject* tuple;
    int curIndex=-1;
    double rad1=-1., rad2=-1.;
    FilletElement curVals;
    std::vector<FilletElement> vals;

    if (!PyArg_ParseTuple(args, "O", &tuple))
        return NULL;

    PyObject *iter = PyObject_GetIter(tuple);
    PyObject *next;

    if (iter == NULL){
        //PyErr_SetString(PyExc_RuntimeError, "ERROR! Only accepts tuple of tuples = ((int, double, double), etc...");
        // Propogate the error
        return NULL;
    }

    while (next = (PyIter_Next(iter))){
        if (!PyArg_ParseTuple(next, "idd", &curIndex, &rad1, &rad2)){
            //PyErr_SetString(PyExc_RuntimeError, "ERROR! Must be a tuple as such 2: (int, double, double)");
            Py_DECREF(iter);
            Py_DECREF(next);
            return NULL;
        }
        else{
            curVals.edgeid  = curIndex;
            curVals.radius1 = rad1;
            curVals.radius2 = rad2;
            vals.push_back(curVals);
        }

        Py_DECREF(next);
    }
    Py_DECREF(iter);
    if (PyErr_Occurred()){
        // propogate error
        return NULL;
    }
    else{
        Part::FilletBase* Base = getFilletBasePtr();
        Base->setEdges(vals);
        Py_RETURN_NONE; // return None. Also, we shouldn't define macros that start with Py...
    }
}
