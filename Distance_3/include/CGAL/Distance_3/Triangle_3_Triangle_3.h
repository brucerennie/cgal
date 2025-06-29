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
// Author(s)     : Mael Rouxel-Labbé

#ifndef CGAL_DISTANCE_3_TRIANGLE_3_TRIANGLE_3_H
#define CGAL_DISTANCE_3_TRIANGLE_3_TRIANGLE_3_H

#include <CGAL/Distance_3/Point_3_Point_3.h>
#include <CGAL/Distance_3/Segment_3_Segment_3.h>

#include <CGAL/Triangle_3.h>

namespace CGAL {
namespace Distance_3 {
namespace internal {

template <typename K>
std::pair<Segment_3_Segment_3_Result<K>, bool>
test_edge_pair(const typename K::Point_3& p1,
               const typename K::Point_3& q1,
               const typename K::Point_3& r1,
               const typename K::Point_3& p2,
               const typename K::Point_3& q2,
               const typename K::Point_3& r2,
               const K& k,
               typename K::FT& global_min_sqd,
               bool& are_triangles_known_to_be_disjoint)
{
  typedef typename K::FT                                                  FT;
  typedef typename K::Point_3                                             Point_3;
  typedef typename K::Vector_3                                            Vector_3;

  typename K::Compute_scalar_product_3 scalar_product = k.compute_scalar_product_3_object();
  typename K::Construct_segment_3 segment = k.construct_segment_3_object();
  typename K::Construct_scaled_vector_3 scale_vector = k.construct_scaled_vector_3_object();
  typename K::Construct_vector_3 vector = k.construct_vector_3_object();
  typename K::Construct_translated_point_3 translate = k.construct_translated_point_3_object();

  Distance_3::internal::Segment_3_Segment_3_Result<K> res =
      internal::squared_distance(segment(p1, q1), segment(p2, q2), k);

  if(res.squared_distance <= global_min_sqd)
    global_min_sqd = res.squared_distance;
  else
    return std::make_pair(res, false);

  const Vector_3 v1 = vector(p1, q1), v2 = vector(p2, q2);
  const Point_3 m1 = translate(p1, scale_vector(v1, res.x));
  const Point_3 m2 = translate(p2, scale_vector(v2, res.y));
  const Vector_3 vr1 = vector(m1, r1), vr2 = vector(m2, r2);
  const Vector_3 n = vector(m1, m2);

  const FT sp_r1 = scalar_product(vr1, n);
  const FT sp_r2 = scalar_product(vr2, n);
  const bool is_r1_closer = (sp_r1 > 0); // Plane_3{m1, n}.has_on_positive_side(r1);
  const bool is_r2_closer = (sp_r2 < 0); // Plane_3{m2, -n}.has_on_positive_side(r2);
  const bool is_best_pair = !is_r1_closer && !is_r2_closer;

  // Even if it is not the best pair, one may be able to deduce if the triangles do not intersect
  // by checking if there is a void space between the planes orthogonal to the vector realizing
  // the min distance between the edges and passing through the third points.
  if(!is_best_pair)
  {
    FT separating_distance = res.squared_distance;
    if(is_r1_closer)
      separating_distance -= sp_r1;
    if(is_r2_closer)
      separating_distance += sp_r2;

    if(separating_distance > 0)
      are_triangles_known_to_be_disjoint = true;
  }

  return std::make_pair(res, is_best_pair);
}

template <typename K>
std::pair<typename K::FT, bool>
test_vertex_triangle(const typename K::Triangle_3& tr1,
                     const typename K::Triangle_3& tr2,
                     const K& k,
                     bool& are_triangles_known_to_be_disjoint)
{
  typedef typename K::FT                                                  FT;
  typedef typename K::Point_3                                             Point_3;
  typedef typename K::Vector_3                                            Vector_3;

  typename K::Compute_scalar_product_3 scalar_product = k.compute_scalar_product_3_object();
  typename K::Construct_cross_product_vector_3 cross_product = k.construct_cross_product_vector_3_object();
  typename K::Construct_vector_3 vector = k.construct_vector_3_object();
  typename K::Construct_vertex_3 vertex = k.construct_vertex_3_object();

  const Point_3& p1 = vertex(tr1, 0);
  const Point_3& q1 = vertex(tr1, 1);
  const Point_3& r1 = vertex(tr1, 2);
  const Point_3& p2 = vertex(tr2, 0);
  const Point_3& q2 = vertex(tr2, 1);
  const Point_3& r2 = vertex(tr2, 2);

  const Vector_3 p2q2 = vector(p2, q2);
  const Vector_3 p2r2 = vector(p2, r2);
  const Vector_3 n2 = cross_product(p2q2, p2r2);

  if(scalar_product(n2, n2) == FT(0))
    return std::make_pair(0, false);

  std::array<FT, 3> sps = { scalar_product(vector(p2, p1), n2),
                            scalar_product(vector(p2, q1), n2),
                            scalar_product(vector(p2, r1), n2) };

  // All the vertices of tr1 must be on the same side of tr2
  // Coplanarity is tolerated, so '1' and '0' should be allowed, but not '1' and '-1'
  if(CGAL::sign(sps[0]) == - CGAL::sign(sps[1]) || CGAL::sign(sps[1]) == - CGAL::sign(sps[2]))
    return std::make_pair(0, false);

  std::for_each(sps.begin(), sps.end(), [](FT& v) { v = abs(v); });
  const auto min_pos = std::min_element(sps.begin(), sps.end());
  const std::size_t min_id = static_cast<std::size_t>(std::distance(sps.begin(), min_pos));

  if(sps[min_id] > 0)
    are_triangles_known_to_be_disjoint = true;

  const Point_3& x1 = vertex(tr1, static_cast<int>(min_id));

  if(CGAL::internal::on_left_of_triangle_edge(x1, n2, p2, q2, k) &&
     CGAL::internal::on_left_of_triangle_edge(x1, n2, q2, r2, k) &&
     CGAL::internal::on_left_of_triangle_edge(x1, n2, r2, p2, k))
  {
    // the projection of `x1` is inside the triangle
    return std::make_pair(CGAL::internal::squared_distance_to_plane(n2, vector(p2, x1), k), true);
  }

  return std::make_pair(0, false);
}

} // namespace internal
} // namespace Distance_3

namespace internal {

template <typename K>
typename K::FT
squared_distance(const typename K::Triangle_3& tr1,
                 const typename K::Triangle_3& tr2,
                 const K& k)
{
  typedef typename K::FT                                                  FT;

  typename K::Construct_vertex_3 vertex = k.construct_vertex_3_object();

  // ideally just limits<FT>::infinity|max(), but it is not available for exact NTs...
  FT global_min_sqd = squared_distance(vertex(tr1, 0), vertex(tr2, 0));

  bool are_triangles_known_to_be_disjoint = false;
  std::pair<Distance_3::internal::Segment_3_Segment_3_Result<K>, bool> ss_res;
  for(int i=0; i<3; ++i)
  {
    for(int j=0; j<3; ++j)
    {
      ss_res = Distance_3::internal::test_edge_pair(
                 vertex(tr1, i%3), vertex(tr1, (i+1)%3), vertex(tr1, (i+2)%3),
                 vertex(tr2, j%3), vertex(tr2, (j+1)%3), vertex(tr2, (j+2)%3), k,
                 global_min_sqd, are_triangles_known_to_be_disjoint);

      if(ss_res.second)
        return ss_res.first.squared_distance;
    }
  }

  // Failed to find a minimum between segment pairs, explore vertex-triangle distances

#if 1
  std::pair<FT, bool> pt_res =
      Distance_3::internal::test_vertex_triangle(tr1, tr2, k, are_triangles_known_to_be_disjoint);
  if(pt_res.second)
    return pt_res.first;

  pt_res = Distance_3::internal::test_vertex_triangle(tr2, tr1, k, are_triangles_known_to_be_disjoint);
  if(pt_res.second)
    return pt_res.first;

  if(are_triangles_known_to_be_disjoint)
    return global_min_sqd;
  else
    return 0;
#else // A tiny bit less efficient, but a lot clearer!
  // @todo does not handle degenerate inputs
  if(!are_triangles_known_to_be_disjoint && CGAL::do_intersect(tr1, tr2))
    return 0;

  FT sqd_p1 = CGAL::squared_distance(vertex(tr1, 0), tr2);
  FT sqd_q1 = CGAL::squared_distance(vertex(tr1, 1), tr2);
  FT sqd_r1 = CGAL::squared_distance(vertex(tr1, 2), tr2);
  FT sqd_p2 = CGAL::squared_distance(vertex(tr2, 0), tr1);
  FT sqd_q2 = CGAL::squared_distance(vertex(tr2, 1), tr1);
  FT sqd_r2 = CGAL::squared_distance(vertex(tr2, 2), tr1);

  const FT m = (std::min)({sqd_p1, sqd_q1, sqd_r1, sqd_p2, sqd_q2, sqd_r2});

  return m;
#endif
}

template <typename K>
typename K::Comparison_result
compare_squared_distance_disjoint(const typename K::Triangle_3& tr1,
                                  const typename K::Triangle_3& tr2,
                                  const K& k,
                                  const typename K::FT& d2)
{
  typedef typename K::Segment_3                                           Segment_3;

  typename K::Construct_vertex_3 vertex = k.construct_vertex_3_object();
  typename K::Compare_squared_distance_3 csq_dist = k.compare_squared_distance_3_object();

  typename K::Comparison_result res(LARGER);

  // The tiangle are supposed to be disjoint
  assert(!do_intersect(tr1, tr2));

  for(int i=0; i<3; ++i)
  {
    //Compare the distance between edges
    for(int j=0; j<3; ++j)
    {
      typename K::Comparison_result temp_res_ss=csq_dist(Segment_3(vertex(tr1, i%3), vertex(tr1, (i+1)%3)),Segment_3(vertex(tr2, j%3), vertex(tr2, (j+1)%3)),d2);
      if(certainly(temp_res_ss==SMALLER))
        return SMALLER;
      res=smaller_of(res, temp_res_ss);
    }

    //Compare the distance between vertices and triangles
    typename K::Comparison_result temp_res_v_pl= csq_dist(vertex(tr1, i), tr2,d2);
    if(certainly(temp_res_v_pl==SMALLER))
      return SMALLER;
    res=smaller_of(res, temp_res_v_pl);

    temp_res_v_pl= csq_dist(vertex(tr2, i), tr1,d2);
    if(certainly(temp_res_v_pl==SMALLER))
      return SMALLER;
    res=smaller_of(res, temp_res_v_pl);
  }
  return res;

}

template <typename K>
typename K::Comparison_result
compare_squared_distance(const typename K::Triangle_3& tr1,
                         const typename K::Triangle_3& tr2,
                         const K& k,
                         const typename K::FT& d2){
    // sq_dist and csq_dist does not exist for Segment-Triangle, sq supports degenerate triangle
    if(tr1.is_degenerate() || tr2.is_degenerate())
      return ::CGAL::compare(squared_distance(tr1,tr2, k), d2);
    if(do_intersect(tr1, tr2))
      return ::CGAL::compare(typename K::FT(0), d2);
    return compare_squared_distance_disjoint(tr1, tr2, k, d2);
}

} // namespace internal

} // namespace CGAL

#endif // CGAL_DISTANCE_3_TRIANGLE_3_TRIANGLE_3_H
