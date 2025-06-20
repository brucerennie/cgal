// Copyright (c) 1998-2021
// Utrecht University (The Netherlands),
// ETH Zurich (Switzerland),
// INRIA Sophia-Antipolis (France),
// Max-Planck-Institute Saarbruecken (Germany),
// and Tel-Aviv University (Israel).  All rights reserved.
//
// This file is part of CGAL (www.cgal.org)
//
// $URL$
// $Id$
// SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Geert-Jan Giezeman, Andreas Fabri

#ifndef CGAL_DISTANCE_3_POINT_3_POINT_3_H
#define CGAL_DISTANCE_3_POINT_3_POINT_3_H

#include <CGAL/number_utils.h>
#include <CGAL/Point_3.h>

namespace CGAL {
namespace internal {

template <class K>
inline
typename K::FT
squared_distance(const typename K::Point_3& pt1,
                 const typename K::Point_3& pt2,
                 const K& k)
{
  return k.compute_squared_distance_3_object()(pt1, pt2);
}

template <class K>
inline
typename K::Comparison_result
compare_squared_distance(const typename K::Point_3& pt1,
                         const typename K::Point_3& pt2,
                         const K& k,
                         const typename K::FT& d2)
{
  return ::CGAL::compare(k.compute_squared_distance_3_object()(pt1, pt2), d2);
}

} // namespace internal

} // namespace CGAL

#endif // CGAL_DISTANCE_3_POINT_3_POINT_3_H
