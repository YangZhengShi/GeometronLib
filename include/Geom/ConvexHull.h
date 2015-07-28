/*
 * ConvexHull.h
 * 
 * This file is part of the "GeometronLib" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef __GM_CONVEX_HULL_H__
#define __GM_CONVEX_HULL_H__


#include "Plane.h"

#include <vector>


namespace Gm
{


//! Base convex hull class.
template <typename T> class ConvexHullT
{
    
    public:
        
        ConvexHullT()
        {
        }

        std::vector< PlaneT<T> > planes;

};


/* --- Type Alias --- */

using ConvexHull    = ConvexHullT<Gs::Real>;
using ConvexHullf   = ConvexHullT<float>;
using ConvexHulld   = ConvexHullT<double>;


} // /namespace Gm


#endif



// ================================================================================
