/*
 * Triangle.h
 * 
 * This file is part of the "GeometronLib" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef __GM_TRIANGLE_H__
#define __GM_TRIANGLE_H__


#include <Gauss/Vector2.h>
#include <Gauss/Vector3.h>
#include <Gauss/Tags.h>


namespace Gm
{


//! Triangle base class.
template <typename T>
class Triangle
{
    
    public:
        
        Triangle() :
            a( T(0) ),
            b( T(0) ),
            c( T(0) )
        {
        }

        Triangle(const Triangle<T>&) = default;

        Triangle(const T& a, const T& b, const T& c) :
            a( a ),
            b( b ),
            c( c )
        {
        }

        Triangle(Gs::UninitializeTag)
        {
            // do nothing
        }

        T& operator [] (std::size_t vertex)
        {
            GS_ASSERT(vertex < 3);
            return *((&a) + vertex);
        }

        const T& operator [] (std::size_t vertex) const
        {
            GS_ASSERT(vertex < 3);
            return *((&a) + vertex);
        }

        T a, b, c;

};

//! Template specializationn for 2D triangles.
template <typename T>
class Triangle< Gs::Vector2T<T> >
{
    
    public:
        
        Triangle() = default;
        Triangle(const Triangle< Gs::Vector2T<T> >&) = default;

        Triangle(const Gs::Vector2T<T>& a, const Gs::Vector2T<T>& b, const Gs::Vector2T<T>& c) :
            a( a ),
            b( b ),
            c( c )
        {
        }

        Triangle(Gs::UninitializeTag) :
            a( Gs::UninitializeTag{} ),
            b( Gs::UninitializeTag{} ),
            c( Gs::UninitializeTag{} )
        {
            // do nothing
        }

        Gs::Vector2T<T>& operator [] (std::size_t vertex)
        {
            GS_ASSERT(vertex < 3);
            return *((&a) + vertex);
        }

        const Gs::Vector2T<T>& operator [] (std::size_t vertex) const
        {
            GS_ASSERT(vertex < 3);
            return *((&a) + vertex);
        }

        T Area() const
        {
            return Gs::Cross(b - a, c - a)/T(2);
        }

        /**
        \brief Computes the cartesian coordinate by the specified barycentric coordinate with respect to this triangle.
        \param[in] barycentricCoords Specifies the barycentric coordinates with respect to this triangle.
        The sum of all components must be one, i.e. x+y+z = 1.
        */
        Gs::Vector2T<T> Barycentric(const Gs::Vector3T<T>& barycentricCoords) const
        {
            return Gs::Vector2T<T>(
                a.x*barycentricCoords.x + b.x*barycentricCoords.y + c.x*barycentricCoords.z,
                a.y*barycentricCoords.x + b.y*barycentricCoords.y + c.y*barycentricCoords.z
            );
        }

        /**
        \brief Computes the triangle with cartesian coordinates by the specified triangle with barycentric coordinates with respecti to this triangle.
        \param[in] barycentricCoords Specifies the triangle with barycentric coordinates with respect to this triangle.
        If this input parameter is { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } }, the result is equal to this triangle.
        The sum of all components must be one for each triangle vertex, i.e. x+y+z = 1.
        */
        Triangle< Gs::Vector2T<T> > Barycentric(const Triangle< Gs::Vector3T<T> >& barycentricCoords) const
        {
            return Triangle< Gs::Vector2T<T> >(
                Barycentric(barycentricCoords.a),
                Barycentric(barycentricCoords.b),
                Barycentric(barycentricCoords.c)
            );
        }

        //! Returns the angle (in radians) of the specified triangle vertex (0, 1, or 2).
        T Angle(std::size_t vertex) const
        {
            return Gs::Angle(
                (*this)[(vertex + 1) % 3] - (*this)[vertex],
                (*this)[(vertex + 2) % 3] - (*this)[vertex]
            );
        }

        Gs::Vector2T<T> a, b, c;

};

//! Template specializationn for 3D triangles.
template <typename T>
class Triangle< Gs::Vector3T<T> >
{
    
    public:
        
        Triangle() = default;
        Triangle(const Triangle< Gs::Vector3T<T> >&) = default;

        Triangle(const Gs::Vector3T<T>& a, const Gs::Vector3T<T>& b, const Gs::Vector3T<T>& c) :
            a( a ),
            b( b ),
            c( c )
        {
        }

        Triangle(Gs::UninitializeTag) :
            a( Gs::UninitializeTag{} ),
            b( Gs::UninitializeTag{} ),
            c( Gs::UninitializeTag{} )
        {
            // do nothing
        }

        Gs::Vector3T<T>& operator [] (std::size_t vertex)
        {
            GS_ASSERT(vertex < 3);
            return *((&a) + vertex);
        }

        const Gs::Vector3T<T>& operator [] (std::size_t vertex) const
        {
            GS_ASSERT(vertex < 3);
            return *((&a) + vertex);
        }

        //! Returns the area of this triangle.
        T Area() const
        {
            return Gs::Cross(b - a, c - a)/T(2);
        }

        /**
        \brief Returns the normal vector of this triangle.
        \remarks This normal vector is not guaranteed to have a unit length of 1.0! To get a normal vector of unit length use "UnitNormal".
        \see UnitNormal
        */
        Gs::Vector3T<T> Normal() const
        {
            return Gs::Cross(b - a, c - a);
        }

        //! Returns the normal vector of this triangle in unit length (length = 1.0).
        Gs::Vector3T<T> UnitNormal() const
        {
            return Normal().Normalized();
        }

        /**
        \brief Computes the cartesian coordinate by the specified barycentric coordinate with respect to this triangle.
        \param[in] barycentricCoords Specifies the barycentric coordinates with respect to this triangle.
        The sum of all components must be one, i.e. x+y+z = 1.
        */
        Gs::Vector3T<T> Barycentric(const Gs::Vector3T<T>& barycentricCoords) const
        {
            return Gs::Vector3T<T>(
                a.x*barycentricCoords.x + b.x*barycentricCoords.y + c.x*barycentricCoords.z,
                a.y*barycentricCoords.x + b.y*barycentricCoords.y + c.y*barycentricCoords.z,
                a.z*barycentricCoords.x + b.z*barycentricCoords.y + c.z*barycentricCoords.z
            );
        }

        /**
        \brief Computes the triangle with cartesian coordinates by the specified triangle with barycentric coordinates with respecti to this triangle.
        \param[in] barycentricCoords Specifies the triangle with barycentric coordinates with respect to this triangle.
        If this input parameter is { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } }, the result is equal to this triangle.
        The sum of all components must be one for each triangle vertex, i.e. x+y+z = 1.
        */
        Triangle< Gs::Vector3T<T> > Barycentric(const Triangle< Gs::Vector3T<T> >& barycentricCoords) const
        {
            return Triangle< Gs::Vector3T<T> >(
                Barycentric(barycentricCoords.a),
                Barycentric(barycentricCoords.b),
                Barycentric(barycentricCoords.c)
            );
        }

        //! Returns the angle (in radians) of the specified triangle vertex (0, 1, or 2).
        T Angle(std::size_t vertex) const
        {
            return Gs::Angle(
                (*this)[(vertex + 1) % 3] - (*this)[vertex],
                (*this)[(vertex + 2) % 3] - (*this)[vertex]
            );
        }

        Gs::Vector3T<T> a, b, c;

};


/* --- Type Alias --- */

template <typename T> using Triangle2T = Triangle< Gs::Vector2T<T> >;
template <typename T> using Triangle3T = Triangle< Gs::Vector3T<T> >;

using Triangle2  = Triangle2T<Gs::Real>;
using Triangle2f = Triangle2T<float>;
using Triangle2d = Triangle2T<double>;

using Triangle3  = Triangle3T<Gs::Real>;
using Triangle3f = Triangle3T<float>;
using Triangle3d = Triangle3T<double>;


} // /namespace Gm


#endif



// ================================================================================
