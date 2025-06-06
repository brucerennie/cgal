// Copyright (c) 2016 GeometryFactory (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Sebastien Loriot

#ifndef CGAL_POLYGON_MESH_PROCESSING_INTERNAL_FACE_GRAPH_OUTPUT_BUILDER_H
#define CGAL_POLYGON_MESH_PROCESSING_INTERNAL_FACE_GRAPH_OUTPUT_BUILDER_H

#include <CGAL/license/Polygon_mesh_processing/corefinement.h>


#include <CGAL/Polygon_mesh_processing/internal/Corefinement/face_graph_utils.h>

#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/connected_components.h>
#include <CGAL/Side_of_triangle_mesh.h>

#include <CGAL/property_map.h>
#include <CGAL/Default.h>

#include <boost/dynamic_bitset.hpp>
#include <boost/mpl/has_xxx.hpp>

// required to handle the multiple types of edge constrained maps
// for the different output types. CGAL_COREF_FUNCTION_CALL_DEF
// must be defined prior to using this macro.
#define CGAL_COREF_FUNCTION_CALL(BO_type) \
  switch(BO_type) \
  { \
    case UNION: \
      CGAL_COREF_FUNCTION_CALL_DEF(UNION); \
      break; \
    case INTERSECTION: \
      CGAL_COREF_FUNCTION_CALL_DEF(INTERSECTION); \
      break; \
    case TM1_MINUS_TM2: \
      CGAL_COREF_FUNCTION_CALL_DEF(TM1_MINUS_TM2); \
      break; \
    default: \
      CGAL_assertion( BO_type == TM2_MINUS_TM1 ); \
      CGAL_COREF_FUNCTION_CALL_DEF(TM2_MINUS_TM1); \
  }

namespace CGAL {
namespace Polygon_mesh_processing {
namespace Corefinement {


template <class TriangleMesh,
          class VPM1 = typename boost::property_map<TriangleMesh, vertex_point_t>::type,
          class VPM2 = typename boost::property_map<TriangleMesh, vertex_point_t>::type>
struct Non_manifold_output_visitor
  : public Default_visitor<TriangleMesh>
{
  struct Handle_non_manifold_output{}; // flag for extra function in Face_graph_output_builder

  typedef boost::graph_traits<TriangleMesh> GT;
  typedef typename GT::face_descriptor face_descriptor;
  typedef typename GT::vertex_descriptor vertex_descriptor;
  typedef typename GT::halfedge_descriptor halfedge_descriptor;
  typedef typename boost::property_map<TriangleMesh, CGAL::dynamic_face_property_t<bool> >::type Classification_map;
  typedef std::unordered_map<vertex_descriptor, std::size_t> Vertex_id_map; // We cannot really use a dynamic property map as the number of vertices increases
  typedef boost::container::flat_map<const TriangleMesh*, Vertex_id_map> Vertex_id_maps;
  std::shared_ptr<Vertex_id_maps> vertex_id_maps;
  TriangleMesh& m_tm1;
  TriangleMesh& m_tm2;
  VPM1 m_vpm1;
  VPM1 m_vpm2;
  std::shared_ptr< std::array< Classification_map, 4> > m_face_classifications_tm1_ptr;
  std::shared_ptr< std::array< Classification_map, 4> > m_face_classifications_tm2_ptr;

  Non_manifold_output_visitor(TriangleMesh& tm1,
                              TriangleMesh& tm2,
                              VPM1 vpm1,
                              VPM2 vpm2)
    : vertex_id_maps( new Vertex_id_maps() )
    , m_tm1(tm1)
    , m_tm2(tm2)
    , m_vpm1(vpm1)
    , m_vpm2(vpm2)
    // m_face_classifications_tm1 and m_face_classifications_tm2 are not init here on purpose
    , m_face_classifications_tm1_ptr(new std::array< Classification_map, 4>())
    , m_face_classifications_tm2_ptr(new std::array< Classification_map, 4>())
  {
    vertex_id_maps->reserve(2);
    vertex_id_maps->emplace(&tm1, Vertex_id_map());
    vertex_id_maps->emplace(&tm2, Vertex_id_map());
  }

  Non_manifold_output_visitor(TriangleMesh& tm1,
                                  TriangleMesh& tm2,
                                  VPM1 vpm1)
    : Non_manifold_output_visitor(tm1, tm2, vpm1, get(CGAL::vertex_point, tm2))
  {}

  Non_manifold_output_visitor(TriangleMesh& tm1,
                                  TriangleMesh& tm2)
    : Non_manifold_output_visitor(tm1, tm2,
                                      get(CGAL::vertex_point, tm1),
                                      get(CGAL::vertex_point, tm2))
  {}

  // only used for intersection on already existing vertices
  void intersection_point_detected(std::size_t node_id,
                                   int sdim,
                                   halfedge_descriptor h_e,
                                   halfedge_descriptor h_f,
                                   const TriangleMesh& tm_e,
                                   const TriangleMesh& tm_f,
                                   bool is_target_coplanar,
                                   bool is_source_coplanar)
  {
    if (sdim==0)
    {
      (*vertex_id_maps)[&tm_f].emplace(target(h_f,tm_f), node_id);
    }
    if (is_target_coplanar)
    {
      CGAL_assertion(!is_source_coplanar);
      (*vertex_id_maps)[&tm_e].emplace(target(h_e,tm_e), node_id);
    }
    if (is_source_coplanar)
    {
      (*vertex_id_maps)[&tm_e].emplace(source(h_e,tm_e), node_id);
    }
  }

  // existing vertices are not listed here
  void new_vertex_added(std::size_t node_id,
                        vertex_descriptor vh,
                        TriangleMesh& tm)
  {
    (*vertex_id_maps)[&tm].emplace(vh, node_id);
  }

  template <class FaceIndexMap>
  void export_flags(FaceIndexMap fim, const std::vector<std::size_t>& tm_patch_ids,
                    const boost::dynamic_bitset<>&  is_patch_inside_other_tm,
                    const boost::dynamic_bitset<>& coplanar_patches,
                    const boost::dynamic_bitset<>& coplanar_patches_of_tm_for_union_and_intersection,
                    const boost::dynamic_bitset<>& /* patch_status_not_set */,
                    TriangleMesh& tm)
  {
  // BO bitsets
    std::array<boost::dynamic_bitset<>, 4> patches_used;

    if (&tm == &m_tm1)
    {
      // UNION
      patches_used[UNION] = ~is_patch_inside_other_tm;
      if (coplanar_patches.any())
      {
        patches_used[UNION] -= coplanar_patches;
        patches_used[UNION] |= coplanar_patches_of_tm_for_union_and_intersection;
      }
      // INTERSECTION
      patches_used[INTERSECTION] = is_patch_inside_other_tm;
      if (coplanar_patches.any())
        patches_used[INTERSECTION] |= coplanar_patches_of_tm_for_union_and_intersection;
      // TM1_MINUS_TM2
      patches_used[TM1_MINUS_TM2] = ~is_patch_inside_other_tm;
      if (coplanar_patches.any())
      {
        patches_used[TM1_MINUS_TM2] -= coplanar_patches;
        patches_used[TM1_MINUS_TM2] |= ~coplanar_patches_of_tm_for_union_and_intersection & coplanar_patches;
      }
      //TM2_MINUS_TM1
      patches_used[TM2_MINUS_TM1] = is_patch_inside_other_tm;
    }
    else
    {
      // UNION
      patches_used[UNION] = ~is_patch_inside_other_tm;
      if (coplanar_patches.any())
      {
        patches_used[UNION] -= coplanar_patches;
      }
      // INTERSECTION
      patches_used[INTERSECTION] = is_patch_inside_other_tm;
      // TM1_MINUS_TM2
      patches_used[TM1_MINUS_TM2] = is_patch_inside_other_tm;
      //TM2_MINUS_TM1
      patches_used[TM2_MINUS_TM1] = ~is_patch_inside_other_tm;
      if (coplanar_patches.any())
      {
        patches_used[TM2_MINUS_TM1] -= coplanar_patches;
        patches_used[TM2_MINUS_TM1] |= ~coplanar_patches_of_tm_for_union_and_intersection & coplanar_patches;
      }
    }

    // now store the face classification
    std::array< Classification_map, 4>& face_classifications = &tm==&m_tm1
                                                             ? *m_face_classifications_tm1_ptr
                                                             : *m_face_classifications_tm2_ptr;
    if (&tm==&m_tm1)
    {
      face_classifications = make_array(
        get(CGAL::dynamic_face_property_t<bool>(), m_tm1, false),
        get(CGAL::dynamic_face_property_t<bool>(), m_tm1, false),
        get(CGAL::dynamic_face_property_t<bool>(), m_tm1, false),
        get(CGAL::dynamic_face_property_t<bool>(), m_tm1, false));
    }
    else
    {
      face_classifications = make_array(
        get(CGAL::dynamic_face_property_t<bool>(), m_tm2, false),
        get(CGAL::dynamic_face_property_t<bool>(), m_tm2, false),
        get(CGAL::dynamic_face_property_t<bool>(), m_tm2, false),
        get(CGAL::dynamic_face_property_t<bool>(), m_tm2, false));
      }


    for(typename boost::graph_traits<TriangleMesh>::face_descriptor f : faces(tm))
    {
      std::size_t pid = tm_patch_ids[get(fim, f)];
      for (int i=0; i<4; ++i)
      {
        if (patches_used[i].test(pid))
          put(face_classifications[i], f, true);
      }
    }
  }



  template <bool inverse_tm1_faces, bool inverse_tm2_faces,
            class PointRange, class PolygonRange,
            class FCM>
  void extract_soup(PointRange& points,
                    PolygonRange& triangles,
                    FCM tm1_face_classification,
                    FCM tm2_face_classification)
  {
    using Polygon = typename std::iterator_traits<typename PolygonRange::iterator>::value_type;
    using ID = typename std::iterator_traits<typename Polygon::iterator>::value_type;

    ID vertex_offset = static_cast<ID>(vertex_id_maps->begin()->second.size());
    Vertex_id_map vid1 = vertex_id_maps->find(&m_tm1)->second; // copy on purpose
    Vertex_id_map vid2 = vertex_id_maps->find(&m_tm2)->second; // copy on purpose

    // first check if some intersection vertices are not used
    // (TODO: only if there are some coplanar-->could be optimized)
    boost::dynamic_bitset<> vid_not_used(vertex_offset);
    vid_not_used.set();
    for (auto p : vid1)
    {
      if (!vid_not_used.test(p.second)) continue;
      for (auto f : faces_around_target(halfedge(p.first, m_tm1), m_tm1))
      {
        if (get(tm1_face_classification,f))
        {
          vid_not_used.reset(p.second);
          break;
        }
      }
    }
    for (auto p : vid2)
    {
      if (!vid_not_used.test(p.second)) continue;
      for (auto f : faces_around_target(halfedge(p.first, m_tm2), m_tm2))
      {
        if (get(tm2_face_classification,f))
        {
          vid_not_used.reset(p.second);
          break;
        }
      }
    }

    if (vid_not_used.any()){
      std::vector<ID> free_vids;
      for (auto i = vid_not_used.find_first();
                       i < vid_not_used.npos;
                       i = vid_not_used.find_next(i))
      {
        free_vids.push_back(static_cast<ID>(i));
      }
      // now reindex the vertices
      for (auto& p : vid1)
      {
        auto it = std::upper_bound(free_vids.begin(), free_vids.end(), p.second);
        p.second -= static_cast<ID>(std::distance(free_vids.begin(), it));
      }
      for (auto& p : vid2)
      {
        auto it = std::upper_bound(free_vids.begin(), free_vids.end(), p.second);
        p.second -= static_cast<ID>(std::distance(free_vids.begin(), it));
      }

      vertex_offset-=static_cast<ID>(free_vids.size());

    }
    points.resize(vertex_offset);

    auto get_vertex_id1 = [&vid1, &vertex_offset, this, &points](vertex_descriptor v)
    {
      auto it_and_b = vid1.emplace(v, vertex_offset);
      if (!it_and_b.second)
      {
        points[it_and_b.first->second]=get(m_vpm1, v);
      }
      else
      {
        points.push_back(get(m_vpm1, v));
        ++vertex_offset;
      }
      return static_cast<ID>(it_and_b.first->second);
    };

    auto get_vertex_id2 = [&vid2, &vertex_offset, this, &points](vertex_descriptor v)
    {
      auto it_and_b = vid2.emplace(v, vertex_offset);
      if (!it_and_b.second)
      {
        points[it_and_b.first->second]=get(m_vpm2,v);
      }
      else
      {
        points.push_back(get(m_vpm2,v));
        ++vertex_offset;
      }
      return static_cast<ID>(it_and_b.first->second);
    };

    // import faces from tm1
    for (face_descriptor f : faces(m_tm1))
    {
      if (get(tm1_face_classification,f))
      {
        halfedge_descriptor h=halfedge(f, m_tm1);
        triangles.push_back( Polygon({get_vertex_id1(source(h, m_tm1)),
                                      get_vertex_id1(target(h, m_tm1)),
                                      get_vertex_id1(target(next(h, m_tm1), m_tm1))}) );
        if (inverse_tm1_faces)
          std::swap(triangles.back()[0],triangles.back()[1]);
      }
    }

    // import faces from tm2
    for (face_descriptor f : faces(m_tm2))
    {
      if (get(tm2_face_classification,f))
      {
        halfedge_descriptor h=halfedge(f, m_tm2);
        triangles.push_back( Polygon({get_vertex_id2(source(h, m_tm2)),
                                      get_vertex_id2(target(h, m_tm2)),
                                      get_vertex_id2(target(next(h, m_tm2), m_tm2))}) );
        if (inverse_tm2_faces)
          std::swap(triangles.back()[0],triangles.back()[1]);
      }
    }
  }

  template <typename PointRange, typename PolygonRange>
  void extract_intersection(PointRange& points,
                            PolygonRange& triangles)
  {
    extract_soup<false, false>(points, triangles,
                               (*m_face_classifications_tm1_ptr)[INTERSECTION],
                               (*m_face_classifications_tm2_ptr)[INTERSECTION]);
  }

  template <typename PointRange, typename PolygonRange>
  void extract_union(PointRange& points,
                     PolygonRange& triangles)
  {
    extract_soup<false, false>(points, triangles,
                               (*m_face_classifications_tm1_ptr)[UNION],
                               (*m_face_classifications_tm2_ptr)[UNION]);
  }

  template <typename PointRange, typename PolygonRange>
  void extract_tm1_minus_tm2(PointRange& points,
                             PolygonRange& triangles)
  {
    extract_soup<false, true>(points, triangles,
                              (*m_face_classifications_tm1_ptr)[TM1_MINUS_TM2],
                              (*m_face_classifications_tm2_ptr)[TM1_MINUS_TM2]);
  }

  template <typename PointRange, typename PolygonRange>
  void extract_tm2_minus_tm1(PointRange& points,
                             PolygonRange& triangles)
  {
    extract_soup<true, false>(points, triangles,
                              (*m_face_classifications_tm1_ptr)[TM2_MINUS_TM1],
                              (*m_face_classifications_tm2_ptr)[TM2_MINUS_TM1]);
  }
};

// extra functions for handling non-documented functions for user visitors
// with no extra functions
BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(Has_extra_functions,
                                  Has_extra_functions,
                                  false)
BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(Has_handle_non_manifold_output,
                                  Handle_non_manifold_output,
                                  false)

template <class UserVisitor, class TriangleMesh, class FaceIndexMap>
void export_flags(UserVisitor&, boost::mpl::bool_<false>,
                  FaceIndexMap, const std::vector<std::size_t>&,
                  const boost::dynamic_bitset<>&,
                  const boost::dynamic_bitset<>&,
                  const boost::dynamic_bitset<>&,
                  const boost::dynamic_bitset<>&,
                  TriangleMesh&)
{}

template <class UserVisitor, class halfedge_descriptor>
void register_halfedge_pair(UserVisitor&, boost::mpl::bool_<false>,
                            halfedge_descriptor, halfedge_descriptor,
                            bool, bool,
                            bool, bool,
                            bool=false, bool=false,
                            bool=false, bool=false)
{}

// with extra functions (forward the call to the visitor)
template <class UserVisitor, class TriangleMesh, class FaceIndexMap>
void export_flags(UserVisitor& visitor, boost::mpl::bool_<true>,
                  FaceIndexMap fim, const std::vector<std::size_t>& tm_patch_ids,
                  const boost::dynamic_bitset<>& is_patch_inside_other_tm,
                  const boost::dynamic_bitset<>& coplanar_patches,
                  const boost::dynamic_bitset<>& coplanar_patches_of_tm_for_union_and_intersection,
                  const boost::dynamic_bitset<>& patch_status_not_set,
                  TriangleMesh& tm)
{
  visitor.export_flags(fim, tm_patch_ids,
                       is_patch_inside_other_tm,
                       coplanar_patches,
                       coplanar_patches_of_tm_for_union_and_intersection,
                       patch_status_not_set,
                       tm);
}

template <class UserVisitor, class halfedge_descriptor>
void register_halfedge_pair(UserVisitor& visitor, boost::mpl::bool_<true>,
                            halfedge_descriptor h1, halfedge_descriptor h2,
                            bool q1_is_between_p1p2, bool q2_is_between_p1p2,
                            bool p1_is_between_q1q2, bool p2_is_between_q1q2,
                            bool p1_is_coplanar=false, bool p2_is_coplanar=false,
                            bool q1_is_coplanar=false, bool q2_is_coplanar=false)
{
  visitor.register_halfedge_pair(h1, h2,
                                 q1_is_between_p1p2, q2_is_between_p1p2,
                                 p1_is_between_q1q2, p2_is_between_q1q2,
                                 p1_is_coplanar, p2_is_coplanar,
                                 q1_is_coplanar, q2_is_coplanar);
}


template <class TriangleMesh,
          class VertexPointMap1,
          class VertexPointMap2,
          class VpmOutTuple,
          class FaceIdMap1,
          class FaceIdMap2,
          class Kernel_ = Default,
          class EdgeMarkMapBind_  = Default,
          class EdgeMarkMapTuple_ = Default,
          class UserVisitor_      = Default>
class Face_graph_output_builder
{
//Default typedefs
  typedef typename Default::Get<
    Kernel_,
    typename Kernel_traits<
      typename boost::property_traits<VertexPointMap1>::value_type
    >::Kernel >::type                                                   Kernel;

  typedef typename Default::Get<EdgeMarkMapBind_,
    Ecm_bind<TriangleMesh, No_mark<TriangleMesh> >
      >::type                                          EdgeMarkMapBind;
  typedef typename Default::Get<EdgeMarkMapTuple_,
    std::tuple< No_mark<TriangleMesh>,
                  No_mark<TriangleMesh>,
                  No_mark<TriangleMesh>,
                  No_mark<TriangleMesh> > >::type     EdgeMarkMapTuple;
  typedef typename Default::Get<
    UserVisitor_, Default_visitor<TriangleMesh> >::type  UserVisitor;
  typedef typename Has_extra_functions<UserVisitor>::type VUNDF; //shortcut
  typedef typename Has_handle_non_manifold_output<UserVisitor>::type HSV; //shortcut

  static constexpr bool has_soup_visitor = HSV::value;

// graph_traits typedefs
  typedef TriangleMesh                                              TM;
  typedef boost::graph_traits<TM>                                   GT;
  typedef typename GT::edge_descriptor                 edge_descriptor;
  typedef typename GT::face_descriptor                 face_descriptor;
  typedef typename GT::halfedge_descriptor         halfedge_descriptor;
  typedef typename GT::vertex_descriptor             vertex_descriptor;
// Internal typedefs
  typedef std::size_t                                          Node_id;
  typedef std::pair<Node_id,Node_id>                      Node_id_pair;
  typedef std::unordered_set<edge_descriptor>  Intersection_edge_map;
  // to maintain a halfedge on each polyline per TriangleMesh + pair<bool,size_t>
  // with first = "is the key (pair<Node_id,Node_id>) was reversed?" and
  // second is the number of edges -1 in the polyline
  typedef std::map< Node_id_pair,
                    std::pair< std::map<TriangleMesh*,
                                        halfedge_descriptor>,
                               std::pair<bool,std::size_t> > >
                                              An_edge_per_polyline_map;

  typedef std::unordered_map<vertex_descriptor, Node_id>   Node_id_map;
  typedef std::unordered_map<edge_descriptor,
                             edge_descriptor>                 Edge_map;
//Data members
  TriangleMesh &tm1, &tm2;
  // property maps of input meshes
  const VertexPointMap1& vpm1;
  const VertexPointMap2& vpm2;
  FaceIdMap1 fids1;
  FaceIdMap2 fids2;
  EdgeMarkMapBind& marks_on_input_edges;
  // property maps of output meshes
  const VpmOutTuple& output_vpms;
  EdgeMarkMapTuple& out_edge_mark_maps;
  UserVisitor& user_visitor;
  // mapping vertex to node id
  Node_id_map vertex_to_node_id1, vertex_to_node_id2;

  // output meshes
  const std::array<std::optional<TriangleMesh*>, 4>& requested_output;
  // input meshes closed ?
  bool is_tm1_closed;
  bool is_tm2_closed;
  // orientation of input surface meshes
  bool is_tm1_inside_out;
  bool is_tm2_inside_out;
  // constants
  const Node_id NID;
  // bitset containing information about operations that cannot be
  // performed because of non-manifoldness or that is ambiguous
  // 0 = tm1 + tm2
  // 1 = tm1 n tm2
  // 2 = tm1 - tm2
  // 3 = tm2 - tm1
  std::bitset<4> impossible_operation;
  // for mapping an edge per polyline per triangle mesh
  An_edge_per_polyline_map an_edge_per_polyline;
  // To collect all intersection edges
  class Mesh_to_intersection_edges{
    TriangleMesh& m_tm;
    Intersection_edge_map tm_map;
    Intersection_edge_map other_map;
  public:
    Mesh_to_intersection_edges(TriangleMesh& tm1, TriangleMesh) : m_tm(tm1) {}
    Intersection_edge_map& operator[](TriangleMesh* tm_ptr) {
      return &m_tm==tm_ptr?tm_map:other_map;
    }
  };
  Mesh_to_intersection_edges mesh_to_intersection_edges;
  bool used_to_clip_a_surface;
  bool use_compact_clipper;

  typename An_edge_per_polyline_map::iterator last_polyline;

  Node_id get_node_id(vertex_descriptor v,
                      const Node_id_map& node_ids)
  {
    typename Node_id_map::const_iterator it = node_ids.find(v);
    if (it == node_ids.end())
      return NID;
    return it->second;
  }

  bool is_dangling_edge(Node_id src_id, Node_id tgt_id,
                        halfedge_descriptor hedge,
                        TriangleMesh& tm,
                        const boost::dynamic_bitset<>& is_node_of_degree_one) const
  {
    if ( is_node_of_degree_one.test(src_id) )
    {
      bool res=true;
      for(halfedge_descriptor h : halfedges_around_source(hedge, tm))
        if (is_border(h, tm))
        {
          res = false;
          break;
        }
      if (res) return true;
    }
    if ( is_node_of_degree_one.test(tgt_id) )
    {
      for(halfedge_descriptor h : halfedges_around_target(hedge, tm))
        if (is_border(h, tm))
          return false;
      return true;
    }
    return false;
  }

  struct Intersection_polylines{
    const std::vector<halfedge_descriptor>& tm1;
    const std::vector<halfedge_descriptor>& tm2;
    const std::vector<std::size_t>& lengths;
    boost::dynamic_bitset<> to_skip;
    boost::dynamic_bitset<> to_skip_in_tm1;
    boost::dynamic_bitset<> to_skip_in_tm2;
    Intersection_polylines(
      const std::vector<halfedge_descriptor>& tm1_polylines,
      const std::vector<halfedge_descriptor>& tm2_polylines,
      const std::vector<std::size_t>& lengths_
    ) : tm1( tm1_polylines )
      , tm2( tm2_polylines )
      , lengths( lengths_ )
      , to_skip(tm1.size(),false)
      , to_skip_in_tm1(tm1.size(),false)
      , to_skip_in_tm2(tm1.size(),false)
    {}
  };

  // detect if a polyline is incident to two patches that won't be imported
  // for the current operation (polylines skipped are always incident to a
  // coplanar patch)
  template <class TM, class FIM1, class FIM2>
  static
  void fill_polylines_to_skip(
    Intersection_polylines& polylines,
    const std::vector<std::size_t>& tm1_patch_ids,
    const std::vector<std::size_t>& tm2_patch_ids,
    const boost::dynamic_bitset<>& patches_of_tm1_used,
    const boost::dynamic_bitset<>& patches_of_tm2_used,
    const FIM1 fids1,
    const FIM2 fids2,
    const TM& tm1,
    const TM& tm2)
  {
    for (std::size_t i=0;i<polylines.tm1.size();++i)
    {
      halfedge_descriptor h1 = polylines.tm1[i];
      halfedge_descriptor h2 = polylines.tm2[i];
      bool skip_polyline_in_tm1=true;
      if (!is_border(h1,tm1)){
        std::size_t patch_id = tm1_patch_ids[get(fids1, face(h1,tm1))];
        if (patches_of_tm1_used.test(patch_id))
          skip_polyline_in_tm1=false;
      }
      if (skip_polyline_in_tm1 && !is_border(opposite(h1,tm1),tm1)){
        std::size_t patch_id = tm1_patch_ids[get(fids1, face(opposite(h1,tm1),tm1))];
        if (patches_of_tm1_used.test(patch_id))
          skip_polyline_in_tm1=false;
      }
      bool skip_polyline_in_tm2=true;
      if (!is_border(h2,tm2)){
        std::size_t patch_id = tm2_patch_ids[get(fids2, face(h2,tm2))];
        if (patches_of_tm2_used.test(patch_id))
          skip_polyline_in_tm2=false;
      }
      if (skip_polyline_in_tm2 && !is_border(opposite(h2,tm2),tm2)){
        std::size_t patch_id = tm2_patch_ids[get(fids2, face(opposite(h2,tm2),tm2))];
        if (patches_of_tm2_used.test(patch_id))
          skip_polyline_in_tm2=false;
      }

      if (skip_polyline_in_tm1) polylines.to_skip_in_tm1.set(i);
      if (skip_polyline_in_tm2) polylines.to_skip_in_tm2.set(i);
      if (skip_polyline_in_tm1 && skip_polyline_in_tm2)
        polylines.to_skip.set(i);
    }
  }

  template<class EdgeMarkMap>
  void mark_edges(const EdgeMarkMap& edge_mark_map,
                  const std::vector<edge_descriptor>& edges)
  {
    for(edge_descriptor ed : edges)
      put(edge_mark_map, ed, true);
  }

  void mark_edges(const No_mark<TriangleMesh>&,
                  const std::vector<edge_descriptor>&)
  {} //nothing to do

  template<class EdgeMarkMapTuple>
  void mark_edges(const EdgeMarkMapTuple& edge_mark_maps,
                  const std::vector<edge_descriptor>& edges,
                  int tuple_id)
  {
    CGAL_assertion(tuple_id < 4 && tuple_id >= 0);
    switch (tuple_id)
    {
    case 0:
      mark_edges(std::get<0>(edge_mark_maps),edges);
    break;
    case 1:
      mark_edges(std::get<1>(edge_mark_maps),edges);
    break;
    case 2:
      mark_edges(std::get<2>(edge_mark_maps),edges);
    break;
    default:
      mark_edges(std::get<3>(edge_mark_maps),edges);
    }
  }

  template<class EdgeMarkMapTuple>
  void mark_edges(const EdgeMarkMapTuple& edge_mark_maps,
                  const Intersection_edge_map& edge_map,
                  int tuple_id)
  {
    std::vector<edge_descriptor> edges;
    edges.reserve(edge_map.size());
    for(edge_descriptor ed : edge_map)
      edges.push_back(ed);

    CGAL_assertion(tuple_id < 4 && tuple_id >= 0);
    switch (tuple_id)
    {
    case 0:
      mark_edges(std::get<0>(edge_mark_maps),edges);
    break;
    case 1:
      mark_edges(std::get<1>(edge_mark_maps),edges);
    break;
    case 2:
      mark_edges(std::get<2>(edge_mark_maps),edges);
    break;
    default:
      mark_edges(std::get<3>(edge_mark_maps),edges);
    }
  }

  void mark_edges(const std::tuple<No_mark<TriangleMesh>,
                                     No_mark<TriangleMesh>,
                                     No_mark<TriangleMesh>,
                                     No_mark<TriangleMesh> >&,
                 const std::vector<edge_descriptor>&,
                 int)
  {} // nothing to do

  void mark_edges(const std::tuple<No_mark<TriangleMesh>,
                                     No_mark<TriangleMesh>,
                                     No_mark<TriangleMesh>,
                                     No_mark<TriangleMesh> >&,
                 const Intersection_edge_map&,
                 int)
  {} // nothing to do

public:

  Face_graph_output_builder(TriangleMesh& tm1,
                            TriangleMesh& tm2,
                            const VertexPointMap1& vpm1,
                            const VertexPointMap2& vpm2,
                            FaceIdMap1 fids1,
                            FaceIdMap2 fids2,
                            EdgeMarkMapBind& marks_on_input_edges,
                            const VpmOutTuple& output_vpms,
                            EdgeMarkMapTuple& out_edge_mark_maps,
                            UserVisitor& user_visitor,
                            const std::array<std::optional<TriangleMesh*>, 4 >& requested_output)
    : tm1(tm1), tm2(tm2)
    , vpm1(vpm1), vpm2(vpm2)
    , fids1(fids1), fids2(fids2)
    , marks_on_input_edges(marks_on_input_edges)
    , output_vpms(output_vpms)
    , out_edge_mark_maps(out_edge_mark_maps)
    , user_visitor(user_visitor)
    , requested_output(requested_output)
    , is_tm1_closed( is_closed(tm1))
    , is_tm2_closed( is_closed(tm2))
    , is_tm1_inside_out( is_tm1_closed && !is_outward_oriented(tm1, parameters::vertex_point_map(vpm1)) )
    , is_tm2_inside_out( is_tm2_closed && !is_outward_oriented(tm2, parameters::vertex_point_map(vpm2)) )
    , NID((std::numeric_limits<Node_id>::max)())
    , mesh_to_intersection_edges(tm1, tm2)
    , used_to_clip_a_surface(false)
    , use_compact_clipper(false)
  {}

  void setup_for_clipping_a_surface(bool ucc)
  {
    used_to_clip_a_surface = true;
    use_compact_clipper = ucc;
  }

  bool union_is_valid() const
  {
    return !impossible_operation[UNION];
  }
  bool intersection_is_valid() const
  {
    return !impossible_operation[INTERSECTION];
  }
  bool tm1_minus_tm2_is_valid() const
  {
    return !impossible_operation[TM1_MINUS_TM2];
  }
  bool tm2_minus_tm1_is_valid() const
  {
    return !impossible_operation[TM2_MINUS_TM1];
  }
// functions called by the intersection visitor
  void start_new_polyline(Node_id i, Node_id j)
  {
    std::pair<typename An_edge_per_polyline_map::iterator,bool> res=
      an_edge_per_polyline.insert(
        std::make_pair( make_sorted_pair(i,j),
          std::make_pair( std::map<TriangleMesh*,halfedge_descriptor>(),std::make_pair(false,0))  )
      );
    CGAL_assertion(res.second);
    last_polyline=res.first;
    if ( i !=last_polyline->first.first )
      last_polyline->second.second.first=true;
  }

  void add_node_to_polyline(Node_id)
  {
    ++(last_polyline->second.second.second);
  }

  void set_edge_per_polyline(TriangleMesh& tm,
                             Node_id_pair indices,
                             halfedge_descriptor hedge)
  {
    //register an intersection halfedge
    // It is important here not to use operator[] since a two edges might be
    // equals while the indices are reversed
    mesh_to_intersection_edges[&tm].insert(edge(hedge, tm));

    if (indices.first>indices.second)
    {
      std::swap(indices.first,indices.second);
      hedge=opposite(hedge,tm);
    }
    typename An_edge_per_polyline_map::iterator it =
      an_edge_per_polyline.find(indices);

    if (it!=an_edge_per_polyline.end()){
      CGAL_assertion(it->second.first.count(&tm) == 0 ||
                     it->second.first[&tm]==hedge);
      it->second.first.insert( std::make_pair( &tm,hedge) );
    }
  }

  void set_vertex_id(vertex_descriptor v, Node_id node_id, const TriangleMesh& tm)
  {
    if (&tm == &tm1)
      vertex_to_node_id1.insert( std::make_pair(v, node_id) );
    else
    {
      CGAL_assertion(&tm == &tm2);
      vertex_to_node_id2.insert( std::make_pair(v, node_id) );
    }
  }

  template <class Nodes_vector, class Mesh_to_map_node>
  void operator()(
    const Nodes_vector& nodes,
    bool input_have_coplanar_faces,
    const boost::dynamic_bitset<>& is_node_of_degree_one,
    const Mesh_to_map_node&)
  {
    const bool used_to_classify_patches =  requested_output[UNION]==std::nullopt &&
                                           requested_output[TM1_MINUS_TM2]==std::nullopt &&
                                           requested_output[TM2_MINUS_TM1]==std::nullopt &&
                                           requested_output[INTERSECTION]==std::nullopt;

    CGAL_assertion( vertex_to_node_id1.size() <= nodes.size() );
    CGAL_assertion( vertex_to_node_id2.size() <= nodes.size() );
    CGAL_assertion(used_to_classify_patches || vertex_to_node_id1.size() == vertex_to_node_id2.size());
    CGAL_assertion(used_to_classify_patches || vertex_to_node_id1.size() == nodes.size());

    Intersection_edge_map& intersection_edges1 = mesh_to_intersection_edges[&tm1];
    Intersection_edge_map& intersection_edges2 = mesh_to_intersection_edges[&tm2];

    // The property map must be either writable or well-initialized
    if( CGAL::internal::Is_writable_property_map<FaceIdMap1>::value &&
        !BGL::internal::is_index_map_valid(fids1, num_faces(tm1), faces(tm1)) )
    {
      BGL::internal::initialize_face_index_map(fids1, tm1);
    }
    CGAL_assertion(BGL::internal::is_index_map_valid(fids1, num_faces(tm1), faces(tm1)));

    if( CGAL::internal::Is_writable_property_map<FaceIdMap2>::value &&
        !BGL::internal::is_index_map_valid(fids2, num_faces(tm2), faces(tm2)) )
    {
      BGL::internal::initialize_face_index_map(fids2, tm2);
    }
    CGAL_assertion(BGL::internal::is_index_map_valid(fids2, num_faces(tm2), faces(tm2)));

    // In the following loop we filter intersection edges that are strictly inside a patch
    // of coplanar facets so that we keep only the edges on the border of the patch.
    // This is not optimal and in an ideal world being able to find the outside edges
    // directly would avoid to compute the intersection of edge/facets inside the patch
    // This loop is done only if the input have some coplanar faces.
    typename An_edge_per_polyline_map::iterator
     epp_it_end=an_edge_per_polyline.end();
    typename An_edge_per_polyline_map::iterator
      epp_it=input_have_coplanar_faces ? an_edge_per_polyline.begin()
                                       : epp_it_end;
    std::unordered_set<edge_descriptor> inter_edges_to_remove1,
                                        inter_edges_to_remove2;

    // Each vector contains a subset of coplanar faces. More particularly only
    // the coplanar faces incident to an intersection edge. Note
    // that for coplanar faces, intersection edges are on the input
    // edges and some coplanar faces might not be seen as they are
    // the result of the retriangulation.
    std::vector<face_descriptor> tm1_coplanar_faces, tm2_coplanar_faces;


    user_visitor.filter_coplanar_edges();

    for (;epp_it!=epp_it_end;)
    {
      halfedge_descriptor h1  = epp_it->second.first[&tm1];
      CGAL_assertion( h1 != GT::null_halfedge());
      halfedge_descriptor h1_opp = opposite(h1, tm1);
      halfedge_descriptor h2 = epp_it->second.first[&tm2];
      CGAL_assertion( h2 != GT::null_halfedge());
      halfedge_descriptor h2_opp = opposite(h2, tm2);

      //vertices from tm1
      vertex_descriptor p1 = target(next(h1_opp, tm1), tm1);
      vertex_descriptor p2 = target(next(h1, tm1), tm1);
      Node_id index_p1 = get_node_id(p1, vertex_to_node_id1);
      Node_id index_p2 = get_node_id(p2, vertex_to_node_id1);
      //vertices from tm2
      vertex_descriptor q1 = target(next(h2_opp, tm2), tm2);
      vertex_descriptor q2 = target(next(h2, tm2), tm2);
      Node_id index_q1 = get_node_id(q1, vertex_to_node_id2);
      Node_id index_q2 = get_node_id(q2, vertex_to_node_id2);

      // set boolean for the position of p1 wrt to q1 and q2
      bool p1_eq_q1 = false, p1_eq_q2 = false;
      if (!is_border(h1_opp, tm1) && index_p1!=NID)
      {
        if (!is_border(h2_opp, tm2))
        {
          p1_eq_q1 = index_p1 == index_q1;
          if (p1_eq_q1)
          {
            //mark coplanar facets if any
            tm1_coplanar_faces.push_back(face(h1_opp, tm1));
            tm2_coplanar_faces.push_back(face(h2_opp, tm2));
          }
        }
        if (!is_border(h2, tm2))
        {
          p1_eq_q2 = index_p1 == index_q2;
          if (p1_eq_q2)
          {
            //mark coplanar facets if any
            tm1_coplanar_faces.push_back(face(h1_opp, tm1));
            tm2_coplanar_faces.push_back(face(h2, tm2));
          }
        }
      }

      // set boolean for the position of p2 wrt to q1 and q2
      bool p2_eq_q1 = false, p2_eq_q2 = false;
      if (!is_border(h1, tm1) && index_p2!=NID)
      {
        if (!is_border(h2_opp, tm2))
        {
          p2_eq_q1 = index_p2 == index_q1;
          if (p2_eq_q1){
            //mark coplanar facets if any
            tm1_coplanar_faces.push_back(face(h1, tm1));
            tm2_coplanar_faces.push_back(face(h2_opp, tm2));
          }
        }
        if (!is_border(h2, tm2))
        {
          p2_eq_q2 = index_p2 == index_q2;
          if (p2_eq_q2){
            //mark coplanar facets if any
            tm1_coplanar_faces.push_back(face(h1, tm1));
            tm2_coplanar_faces.push_back(face(h2, tm2));
          }
        }
      }

      if ( (p1_eq_q1 || p1_eq_q2) && (p2_eq_q1 || p2_eq_q2) )
      {
        typename An_edge_per_polyline_map::iterator it_to_rm=epp_it;
        ++epp_it;
        an_edge_per_polyline.erase(it_to_rm);
        inter_edges_to_remove1.insert(edge(h1,tm1));
        inter_edges_to_remove2.insert(edge(h2,tm2));

        // on the border, we can have a degree 2 node so prev/next
        // halfedge should be also considered for removal
        // (as the coplanar edge will not be reported in an_edge_per_polyline
        //  and thus not removed from intersection_edges[12])
        if ( !is_border(h1, tm1) )
        {
          h1 = opposite(h1, tm1);
          h2 = opposite(h2, tm2);
        }
        if ( is_border(h1, tm1) )
        {
          if ( opposite(next(h1, tm1), tm1) == prev(opposite(h1, tm1), tm1) )
          {
            inter_edges_to_remove1.insert(edge(next(h1, tm1),tm1));
            inter_edges_to_remove2.insert(edge(next(h2, tm2),tm2));
          }
          if ( opposite(prev(h1, tm1), tm1) == next(opposite(h1, tm1), tm1) )
          {
            inter_edges_to_remove1.insert(edge(prev(h1, tm1), tm1));
            inter_edges_to_remove2.insert(edge(prev(h2, tm2), tm2));
          }
        }
        // same but for h2
        if ( !is_border(h2, tm2) )
        {
          h1 = opposite(h1, tm1);
          h2 = opposite(h2, tm2);
        }
        if ( is_border(h2, tm2) )
        {
          if ( opposite(next(h2, tm2), tm2) == prev(opposite(h2, tm2), tm2) )
          {
            inter_edges_to_remove1.insert(edge(next(h1, tm1),tm1));
            inter_edges_to_remove2.insert(edge(next(h2, tm2),tm2));
          }
          if ( opposite(prev(h2, tm2), tm2) == next(opposite(h2, tm2), tm2) )
          {
            inter_edges_to_remove1.insert(edge(prev(h1, tm1), tm1));
            inter_edges_to_remove2.insert(edge(prev(h2, tm2), tm2));
          }
        }
      }
      else
        ++epp_it;
    }
    for(edge_descriptor ed : inter_edges_to_remove1)
    {
      put(marks_on_input_edges.ecm1, ed, false);
      intersection_edges1.erase(ed);
    }
    for(edge_descriptor ed : inter_edges_to_remove2)
    {
      put(marks_on_input_edges.ecm2, ed, false);
      intersection_edges2.erase(ed);
    }

    user_visitor.detect_patches();

    // (1) Assign a patch id to each facet indicating in which connected
    // component limited by intersection edges of the surface they are.
    // ... for tm1
    std::vector<std::size_t> tm1_patch_ids( num_faces(tm1),NID );
    Border_edge_map<TriangleMesh> is_marked_1(intersection_edges1, tm1);
    std::size_t nb_patches_tm1 =
      connected_components(tm1,
                           make_compose_property_map(fids1,make_property_map(&tm1_patch_ids[0])),
                           parameters::edge_is_constrained_map(is_marked_1)
                                      .face_index_map(fids1));

    std::vector <std::size_t> tm1_patch_sizes(nb_patches_tm1, 0);
    for(std::size_t i : tm1_patch_ids)
      if(i!=NID)
        ++tm1_patch_sizes[i];
    // ... for tm2
    std::vector<std::size_t> tm2_patch_ids( num_faces(tm2),NID );
    Border_edge_map<TriangleMesh> is_marked_2(intersection_edges2, tm2);
    std::size_t nb_patches_tm2 =
      connected_components(tm2,
                           make_compose_property_map(fids2,make_property_map(&tm2_patch_ids[0])),
                           parameters::edge_is_constrained_map(is_marked_2)
                                      .face_index_map(fids2));

    std::vector <std::size_t> tm2_patch_sizes(nb_patches_tm2, 0);
    for(Node_id i : tm2_patch_ids)
      if(i!=NID)
        ++tm2_patch_sizes[i];


    user_visitor.classify_patches();

    // (2-a) Use the orientation around an edge to classify a patch
    boost::dynamic_bitset<> is_patch_inside_tm2(nb_patches_tm1, false);
    boost::dynamic_bitset<> is_patch_inside_tm1(nb_patches_tm2, false);
    boost::dynamic_bitset<> patch_status_not_set_tm1(nb_patches_tm1);
    boost::dynamic_bitset<> patch_status_not_set_tm2(nb_patches_tm2);
    boost::dynamic_bitset<> coplanar_patches_of_tm1(nb_patches_tm1,false);
    boost::dynamic_bitset<> coplanar_patches_of_tm2(nb_patches_tm2,false);
    boost::dynamic_bitset<> coplanar_patches_of_tm1_for_union_and_intersection(nb_patches_tm1,false);
    boost::dynamic_bitset<> coplanar_patches_of_tm2_for_union_and_intersection(nb_patches_tm2,false);
    patch_status_not_set_tm1.set();
    patch_status_not_set_tm2.set();
    // extra containers used when entire connected components are identical (filled only if needed)
    std::vector<std::size_t> coplanar_tm1_to_coplanar_tm2;
    std::vector<vertex_descriptor> extreme_vertex_per_cc_1;
    std::vector<vertex_descriptor> extreme_vertex_per_cc_2;

    // first set coplanar status of patches using the coplanar faces collected during the
    // extra intersection edges collected. This is important in the case of full connected components
    // being coplanar. They have no intersection edges (closed cc) or only intersection edges on the
    // boundary (non-closed cc)
    for (face_descriptor f1 : tm1_coplanar_faces)
    {
      std::size_t fid1 = get(fids1, f1);
      coplanar_patches_of_tm1.set(tm1_patch_ids[ fid1 ]);
    }
    for (face_descriptor f2 : tm2_coplanar_faces)
    {
      std::size_t fid2 = get(fids2, f2);
      coplanar_patches_of_tm2.set(tm2_patch_ids[ fid2 ]);
    }

    for (typename An_edge_per_polyline_map::iterator
            it=an_edge_per_polyline.begin(),
            it_end=an_edge_per_polyline.end(); it!=it_end;++it)
    {
      CGAL_assertion(it->second.first.size()==2);
      //orientation of faces around the edge (to be sure we can do it)
      const std::pair<Node_id,Node_id>& ids = it->first;
      //const std::pair<bool,int>& polyline_info=it->second.second;

      //get the two halfedges incident to the edge [ids.first,ids.second]
      halfedge_descriptor h1 = it->second.first[&tm1];
      halfedge_descriptor h2 = it->second.first[&tm2];

#ifdef CGAL_COREFINEMENT_DEBUG
      std::cout << "Looking at triangles around edge " << tm1.point(source(h1, tm1)) << " " << tm1.point(target(h1, tm1)) << "\n";
#endif

      CGAL_assertion(ids.first==vertex_to_node_id1[source(h1,tm1)]);
      CGAL_assertion(ids.second==vertex_to_node_id1[target(h1,tm1)]);
      CGAL_assertion(ids.first==vertex_to_node_id2[source(h2,tm2)]);
      CGAL_assertion(ids.second==vertex_to_node_id2[target(h2,tm2)]);

      // different handling depending on the number of incident
      // triangles to the edge. After sewing there are two, three or
      // four volumes if there are two, three or four incident
      // triangles respectively
      if ( is_border_edge(h1, tm1) ){
        if ( is_border_edge(h2,tm2) )
        {
          if ( is_border(h1,tm1) != is_border(h2,tm2) )
          {
            //No restriction at this level
            std::size_t fid1 =
              get(fids1, is_border(h1,tm1) ? face(opposite(h1,tm1),tm1)
                                           : face(h1,tm1));
            std::size_t fid2 =
               get(fids2, is_border(h2,tm2) ? face(opposite(h2,tm2),tm2)
                                            : face(h2,tm2));
            std::size_t patch_id_p=tm1_patch_ids[ fid1 ];
            std::size_t patch_id_q=tm2_patch_ids[ fid2 ];

            patch_status_not_set_tm1.reset(patch_id_p);
            patch_status_not_set_tm2.reset(patch_id_q);
          }
          else
          {
            if (!used_to_clip_a_surface)
            {
              if (used_to_classify_patches)
              {
                Node_id index_o_prime = ids.first, index_o = ids.second;
                if( is_border(h1, tm1) )
                {
                  h1 = opposite(h1, tm1);
                  h2 = opposite(h2, tm2);
                  std::swap(index_o_prime, index_o);
                }

                std::size_t fid1 = get(fids1, face(h1,tm1));
                std::size_t fid2 = get(fids2, face(h2,tm2));
                std::size_t patch_id_p=tm1_patch_ids[ fid1 ];
                std::size_t patch_id_q=tm2_patch_ids[ fid2 ];

                //indicates that patch status will be updated
                patch_status_not_set_tm1.reset(patch_id_p);
                patch_status_not_set_tm2.reset(patch_id_q);

                if (coplanar_patches_of_tm1.test(patch_id_p) && coplanar_patches_of_tm2.test(patch_id_q))
                {
                  coplanar_patches_of_tm1_for_union_and_intersection.set(patch_id_p);
                  coplanar_patches_of_tm2_for_union_and_intersection.set(patch_id_q);
                }
                else
                {
                  vertex_descriptor p = target(next(h1,tm1),tm1);
                  vertex_descriptor q = target(next(h2,tm2),tm2);
                  Node_id index_p = get_node_id(p, vertex_to_node_id1);
                  Node_id index_q = get_node_id(q, vertex_to_node_id2);

                  if ( p_is_below_q(index_o_prime, index_o,
                                    index_p, index_q, p, q,
                                    vpm1, vpm2,
                                    nodes) )
                    is_patch_inside_tm2.set(patch_id_p);
                  else
                    is_patch_inside_tm1.set(patch_id_q);
                }
              }
              else
              {
                //Nothing allowed
#ifdef CGAL_COREFINEMENT_DEBUG
                std::cout << "  Non-manifold edge case 1\n";
#endif
                impossible_operation.set();
                return;
              }
            }
          }
        }
        else
        {
          if (!used_to_clip_a_surface && !used_to_classify_patches)
          {
#ifdef CGAL_COREFINEMENT_DEBUG
              std::cout << "  Non-manifold edge case 2\n";
#endif
            //Ambiguous, we can do nothing
            impossible_operation.set();
            return;
          }
          else
          {
            //Sort the three triangle faces around their common edge
            //  we assume that the exterior of the volume is indicated by
            //  counterclockwise oriented faces
            //  (corrected by is_tmi_inside_tmi).
            halfedge_descriptor h = is_border(h1, tm1) ? opposite(h1, tm1) : h1;
            vertex_descriptor p = target(next(h,tm1),tm1);
            //    when looking from the side of indices.second,
            //    the interior of the first triangle mesh is described
            //    by turning counterclockwise from p1 to p2
            vertex_descriptor q1=target(next(opposite(h2,tm2),tm2),tm2);
            vertex_descriptor q2=target(next(h2,tm2),tm2);
            //    when looking from the side of indices.second,
            //    the interior of the second volume is described
            //    by turning from q1 to q2

            //check if the third point of each triangular face is an original point (stay NID)
            //or a intersection point (in that case we need the index of the corresponding node to
            //have the exact value of the point)
            Node_id index_p = get_node_id(p, vertex_to_node_id1);
            Node_id index_q1 = get_node_id(q1, vertex_to_node_id2);
            Node_id index_q2 = get_node_id(q2, vertex_to_node_id2);

            std::size_t patch_id_p=tm1_patch_ids[ get(fids1, face(h,tm1)) ];
            std::size_t patch_id_q1=tm2_patch_ids[ get(fids2, face(opposite(h2,tm2),tm2)) ];
            std::size_t patch_id_q2=tm2_patch_ids[ get(fids2, face(h2,tm2)) ];

            if (index_p!=index_q1 && index_p!=index_q2)
            {
              //indicates that patch status will be updated
              patch_status_not_set_tm1.reset(patch_id_p);
              patch_status_not_set_tm2.reset(patch_id_q1);
              patch_status_not_set_tm2.reset(patch_id_q2);

              bool p_is_between_q1q2 = sorted_around_edge(
                  ids.first, ids.second,
                  index_q1, index_q2, index_p,
                  q1, q2, p,
                  vpm2, vpm1,
                  nodes);

              if (p_is_between_q1q2)
              {
                is_patch_inside_tm2.set(patch_id_p);
                // locally does not really make sense globally
                if (h == h1) // i.e. p2
                  is_patch_inside_tm1.set(patch_id_q2);
                else
                  is_patch_inside_tm1.set(patch_id_q1);
              }
              else
              {
                // locally does not really make sense globally
                if (h == h1) // i.e. p2
                  is_patch_inside_tm1.set(patch_id_q1);
                else
                  is_patch_inside_tm1.set(patch_id_q2);
              }
            }
          }
        }
      }
      else
        if ( is_border_edge(h2,tm2) )
        {
          CGAL_assertion(!used_to_clip_a_surface);
          if (!used_to_classify_patches)
          {
#ifdef CGAL_COREFINEMENT_DEBUG
            std::cout << "  Non-manifold edge case 3\n";
#endif
            impossible_operation.set();
            return;
          }
          else
          {
            //Sort the three triangle faces around their common edge
            //  we assume that the exterior of the volume is indicated by
            //  counterclockwise oriented faces
            //  (corrected by is_tmi_inside_tmi).
            halfedge_descriptor h = is_border(h2, tm2) ? opposite(h2, tm2) : h2;
            vertex_descriptor q = target(next(h,tm2),tm2);
            //    when looking from the side of indices.second,
            //    the interior of the first triangle mesh is described
            //    by turning counterclockwise from p1 to p2
            vertex_descriptor p1=target(next(opposite(h1,tm1),tm1),tm1);
            vertex_descriptor p2=target(next(h1,tm1),tm1);
            //    when looking from the side of indices.second,
            //    the interior of the second volume is described
            //    by turning from p1 to p2

            //check if the third point of each triangular face is an original point (stay NID)
            //or a intersection point (in that case we need the index of the corresponding node to
            //have the exact value of the point)
            Node_id index_q = get_node_id(q, vertex_to_node_id2);
            Node_id index_p1 = get_node_id(p1, vertex_to_node_id1);
            Node_id index_p2 = get_node_id(p2, vertex_to_node_id1);

            std::size_t patch_id_q=tm2_patch_ids[ get(fids2, face(h,tm2)) ];
            std::size_t patch_id_p1=tm1_patch_ids[ get(fids1, face(opposite(h1,tm1),tm1)) ];
            std::size_t patch_id_p2=tm1_patch_ids[ get(fids1, face(h1,tm1)) ];

            if (index_q!=index_p1 && index_q!=index_p2)
            {
              //indicates that patch status will be updated
              patch_status_not_set_tm2.reset(patch_id_q);
              patch_status_not_set_tm1.reset(patch_id_p1);
              patch_status_not_set_tm1.reset(patch_id_p2);

              bool q_is_between_p1p2 = sorted_around_edge(
                  ids.first, ids.second,
                  index_p1, index_p2, index_q,
                  p1, p2, q,
                  vpm1, vpm2,
                  nodes);

              if (q_is_between_p1p2)
              {
                is_patch_inside_tm1.set(patch_id_q);
                // locally does not really make sense globally
                if (h == h2) // i.e. q2
                  is_patch_inside_tm2.set(patch_id_p2);
                else
                  is_patch_inside_tm2.set(patch_id_p1);
              }
              else
              {
                // locally does not really make sense globally
                if (h == h2) // i.e. q2
                  is_patch_inside_tm2.set(patch_id_p1);
                else
                  is_patch_inside_tm2.set(patch_id_p2);
              }
            }
          }
        }
        else
        {
          //Sort the four triangle faces around their common edge
          //  we assume that the exterior of the volume is indicated by
          //  counterclockwise oriented faces
          //  (corrected by is_tmi_inside_tmi).
          vertex_descriptor p1=target(next(opposite(h1,tm1),tm1),tm1);
          vertex_descriptor p2=target(next(h1,tm1),tm1);
          //    when looking from the side of indices.second,
          //    the interior of the first triangle mesh is described
          //    by turning counterclockwise from p1 to p2
          vertex_descriptor q1=target(next(opposite(h2,tm2),tm2),tm2);
          vertex_descriptor q2=target(next(h2,tm2),tm2);
          //    when looking from the side of indices.second,
          //    the interior of the second volume is described
          //    by turning from q1 to q2

          //check if the third point of each triangular face is an original point (stay NID)
          //or a intersection point (in that case we need the index of the corresponding node to
          //have the exact value of the point)
          Node_id index_p1 = get_node_id(p1, vertex_to_node_id1);
          Node_id index_p2 = get_node_id(p2, vertex_to_node_id1);
          Node_id index_q1 = get_node_id(q1, vertex_to_node_id2);
          Node_id index_q2 = get_node_id(q2, vertex_to_node_id2);

          std::size_t patch_id_p1=tm1_patch_ids[ get(fids1, face(opposite(h1,tm1),tm1)) ];
          std::size_t patch_id_p2=tm1_patch_ids[ get(fids1, face(h1,tm1)) ];
          std::size_t patch_id_q1=tm2_patch_ids[ get(fids2, face(opposite(h2,tm2),tm2)) ];
          std::size_t patch_id_q2=tm2_patch_ids[ get(fids2, face(h2,tm2)) ];

          // info on whether the patches were already classified
          std::bitset<4> patch_status_was_not_already_set;
          std::bitset<4> previous_bitvalue;
          // info for tm1
          patch_status_was_not_already_set[0] = patch_status_not_set_tm1.test(patch_id_p1);
          patch_status_was_not_already_set[1] = patch_status_not_set_tm1.test(patch_id_p2);
          previous_bitvalue[0] = is_patch_inside_tm2.test(patch_id_p1);
          previous_bitvalue[1] = is_patch_inside_tm2.test(patch_id_p2);
          // info for tm2
          patch_status_was_not_already_set[2] = patch_status_not_set_tm2.test(patch_id_q1);
          patch_status_was_not_already_set[3] = patch_status_not_set_tm2.test(patch_id_q2);
          previous_bitvalue[2] = is_patch_inside_tm1.test(patch_id_q1);
          previous_bitvalue[3] = is_patch_inside_tm1.test(patch_id_q2);

/*
          // Note that this code is commented as impossible_operation flag could be set thanks to
          // another polyline and such a `continue` would make us miss it.
          if (is_tm1_closed && is_tm2_closed)
          {
             if (!patch_status_was_not_already_set[0] &&
                 !patch_status_was_not_already_set[1] &&
                 !patch_status_was_not_already_set[2] &&
                 !patch_status_was_not_already_set[3])
               continue; // all patches were already classified, no need to redo it
          }
*/

          // check incompatibility of patch classifications
          auto inconsistent_classification = [&]()
          {
            if (!used_to_clip_a_surface && !used_to_classify_patches && (!is_tm1_closed || !is_tm2_closed))
            {
              //make sure there is no ambiguity in tm1
              if( (!patch_status_was_not_already_set[0] && previous_bitvalue[0]!=is_patch_inside_tm2.test(patch_id_p1) ) ||
                  (!patch_status_was_not_already_set[1] && previous_bitvalue[1]!=is_patch_inside_tm2.test(patch_id_p2) ) )
              {
                impossible_operation.set();
                return true;
              }
              //make sure there is no ambiguity in tm2
              if( (!patch_status_was_not_already_set[2] && previous_bitvalue[2]!=is_patch_inside_tm1.test(patch_id_q1) ) ||
                  (!patch_status_was_not_already_set[3] && previous_bitvalue[3]!=is_patch_inside_tm1.test(patch_id_q2) ) )
              {
                impossible_operation.set();
                return true;
              }
            }
            return false;
          };
#ifndef CGAL_NDEBUG
          auto debug_check_consistency = [&]()
          {
            if (!used_to_clip_a_surface && !used_to_classify_patches)
            {
              CGAL_assertion( patch_status_was_not_already_set[0] || (previous_bitvalue[0]==is_patch_inside_tm2[patch_id_p1]) );
              CGAL_assertion( patch_status_was_not_already_set[1] || (previous_bitvalue[1]==is_patch_inside_tm2[patch_id_p2]) );
              CGAL_assertion( patch_status_was_not_already_set[2] || (previous_bitvalue[2]==is_patch_inside_tm1[patch_id_q1]) );
              CGAL_assertion( patch_status_was_not_already_set[3] || (previous_bitvalue[3]==is_patch_inside_tm1[patch_id_q2]) );
            }
          };
          is_patch_inside_tm2.reset(patch_id_p1);
          is_patch_inside_tm2.reset(patch_id_p2);
          is_patch_inside_tm1.reset(patch_id_q1);
          is_patch_inside_tm1.reset(patch_id_q2);
#else
          auto debug_check_consistency = [&](){};

#endif

          //indicates that patch status will be updated
          patch_status_not_set_tm1.reset(patch_id_p1);
          patch_status_not_set_tm1.reset(patch_id_p2);
          patch_status_not_set_tm2.reset(patch_id_q1);
          patch_status_not_set_tm2.reset(patch_id_q2);

          // restore initial state, needed when checking in `inconsistent_classification()`
          if (!is_tm1_closed  || !is_tm2_closed)
          {
            is_patch_inside_tm2.reset(patch_id_p1);
            is_patch_inside_tm2.reset(patch_id_p2);
            is_patch_inside_tm1.reset(patch_id_q1);
            is_patch_inside_tm1.reset(patch_id_q2);
          }

#ifdef CGAL_COREFINEMENT_POLYHEDRA_DEBUG
          #warning: Factorize the orientation predicates.
#endif //CGAL_COREFINEMENT_POLYHEDRA_DEBUG
          // handle case of coplanar facets
          // We choose that a coplanar patch is classified like the other incident patch since they bound the same volume.
          if ( are_triangles_coplanar_same_side(
                ids.first, ids.second,
                index_p1, index_q1,
                p1, q1,
                vpm1, vpm2,
                nodes) ) //p1==q1
          {
            coplanar_patches_of_tm1.set(patch_id_p1);
            coplanar_patches_of_tm2.set(patch_id_q1);
            coplanar_patches_of_tm1_for_union_and_intersection.set(patch_id_p1);
            coplanar_patches_of_tm2_for_union_and_intersection.set(patch_id_q1);

            CGAL_assertion(
              !are_triangles_coplanar_same_side(
                ids.first, ids.second,
                index_p2, index_q2,
                p2, q2,
                vpm1, vpm2,
                nodes) );

            bool q2_is_between_p1p2 = sorted_around_edge(
              ids.first, ids.second,
              index_p1, index_p2, index_q2,
              p1, p2, q2,
              vpm1, vpm2,
              nodes);

            register_halfedge_pair(user_visitor, VUNDF(),
                                   h1, h2,
                                   false, q2_is_between_p1p2, false, !q2_is_between_p1p2,
                                   true, false, true, false);

            if ( q2_is_between_p1p2 ) is_patch_inside_tm1.set(patch_id_q2); //case 1
            else is_patch_inside_tm2.set(patch_id_p2); //case 2
            if (inconsistent_classification()) return;
            debug_check_consistency();
            continue;
          }
          else{
            if ( are_triangles_coplanar_same_side(
                   ids.first, ids.second,
                   index_p1, index_q2,
                   p1, q2,
                   vpm1, vpm2,
                   nodes) ) //p1==q2
            {
              CGAL_assertion( index_p1!=index_p2 || index_p1==NID );
              coplanar_patches_of_tm1.set(patch_id_p1);
              coplanar_patches_of_tm2.set(patch_id_q2);
              bool q1_is_between_p1p2 = sorted_around_edge(
                ids.first, ids.second,
                index_p1, index_p2, index_q1,
                p1, p2, q1,
                vpm1, vpm2,
                nodes);

              register_halfedge_pair(user_visitor, VUNDF(),
                                     h1, h2,
                                     q1_is_between_p1p2, false, false, !q1_is_between_p1p2,
                                     true, false, false, true);

              if ( q1_is_between_p1p2 )
              { // case 3
                is_patch_inside_tm1.set(patch_id_q1);
                is_patch_inside_tm2.set(patch_id_p2);
              } //else case 4
              if (inconsistent_classification()) return;
              debug_check_consistency();
              continue;
            }
            else
            {
              if ( are_triangles_coplanar_same_side(
                     ids.first, ids.second,
                     index_p2, index_q1,
                     p2, q1,
                     vpm1, vpm2,
                     nodes) ) //p2==q1
              {
                coplanar_patches_of_tm1.set(patch_id_p2);
                coplanar_patches_of_tm2.set(patch_id_q1);
                bool q2_is_between_p1p2 = sorted_around_edge(
                  ids.first, ids.second,
                  index_p1, index_p2, index_q2,
                  p1, p2, q2,
                  vpm1, vpm2,
                  nodes);

                register_halfedge_pair(user_visitor, VUNDF(),
                                       h1, h2,
                                       false, q2_is_between_p1p2, !q2_is_between_p1p2, false,
                                       false, true, true, false);

                if ( q2_is_between_p1p2 )
                {  //case 5
                  is_patch_inside_tm1.set(patch_id_q2);
                  is_patch_inside_tm2.set(patch_id_p1);
                } // else case 6
                if (inconsistent_classification()) return;
                debug_check_consistency();
                continue;
              }
              else{
                if ( are_triangles_coplanar_same_side(
                       ids.first, ids.second,
                       index_p2, index_q2,
                       p2, q2,
                       vpm1, vpm2,
                       nodes) ) //p2==q2
                {
                  coplanar_patches_of_tm1.set(patch_id_p2);
                  coplanar_patches_of_tm2.set(patch_id_q2);
                  coplanar_patches_of_tm1_for_union_and_intersection.set(patch_id_p2);
                  coplanar_patches_of_tm2_for_union_and_intersection.set(patch_id_q2);
                  bool q1_is_between_p1p2 = sorted_around_edge(
                    ids.first, ids.second,
                    index_p1, index_p2, index_q1,
                    p1, p2, q1,
                    vpm1, vpm2,
                    nodes);

                  register_halfedge_pair(user_visitor, VUNDF(),
                                         h1, h2,
                                         q1_is_between_p1p2, false, !q1_is_between_p1p2, false,
                                         false, true, false, true);

                  if ( q1_is_between_p1p2 ) is_patch_inside_tm1.set(patch_id_q1);  //case 7
                  else is_patch_inside_tm2.set(patch_id_p1); //case 8
                  if (inconsistent_classification()) return;
                  debug_check_consistency();
                  continue;
                }
              }
            }
          }
#ifdef CGAL_COREFINEMENT_POLYHEDRA_DEBUG
          #warning At some point we should have a check if a patch status is already set, what we do is consistent otherwise --> ambiguous
#endif //CGAL_COREFINEMENT_POLYHEDRA_DEBUG

          CGAL_assertion(
              ( index_p1 == NID ? nodes.to_exact(get(vpm1,p1)): nodes.exact_node(index_p1) ) !=
              ( index_q1 == NID ? nodes.to_exact(get(vpm2,q1)): nodes.exact_node(index_q1) )
          &&
              ( index_p2 == NID ? nodes.to_exact(get(vpm1,p2)): nodes.exact_node(index_p2) ) !=
              ( index_q1 == NID ? nodes.to_exact(get(vpm2,q1)): nodes.exact_node(index_q1) )
          &&
              ( index_p1 == NID ? nodes.to_exact(get(vpm1,p1)): nodes.exact_node(index_p1) ) !=
              ( index_q2 == NID ? nodes.to_exact(get(vpm2,q2)): nodes.exact_node(index_q2) )
          &&
              ( index_p2 == NID ? nodes.to_exact(get(vpm1,p2)): nodes.exact_node(index_p2) ) !=
              ( index_q2 == NID ? nodes.to_exact(get(vpm2,q2)): nodes.exact_node(index_q2) )
          );

          bool q1_is_between_p1p2 = sorted_around_edge(
            ids.first, ids.second,
            index_p1, index_p2, index_q1,
            p1, p2, q1,
            vpm1, vpm2,
            nodes);
          bool q2_is_between_p1p2 = sorted_around_edge(
            ids.first, ids.second,
            index_p1, index_p2, index_q2,
            p1, p2, q2,
            vpm1, vpm2,
            nodes);

          if ( q1_is_between_p1p2 ){
            is_patch_inside_tm1.set(patch_id_q1);
            if( q2_is_between_p1p2 )
            {
              is_patch_inside_tm1.set(patch_id_q2);
              bool p1_is_between_q1q2 = sorted_around_edge(
                ids.first, ids.second,
                index_q1, index_q2, index_p1,
                q1, q2, p1,
                vpm2, vpm1,
                nodes);
              if (!p1_is_between_q1q2){
                register_halfedge_pair(user_visitor, VUNDF(), h1, h2, true, true, false, false);
                // case (a4)
                // poly_first  - poly_second            = p1q1 U q2p2
                // poly_second - poly_first             = {0}
                // poly_first \cap poly_second          = q1q2
                // opposite( poly_first U poly_second ) = p2p1
#ifdef CGAL_COREFINEMENT_DEBUG
              std::cout << "  Non-manifold edge case 4\n";
#endif
                impossible_operation.set(TM1_MINUS_TM2); // tm1-tm2 is non-manifold
              }
              else{
                register_halfedge_pair(user_visitor, VUNDF(), h1, h2, true, true, true, true);
                // case (b4)
                // poly_first  - poly_second            = q2q1
                // poly_second - poly_first             = p2p1
                // poly_first \cap poly_second          = p1q2 U q1p2
                // opposite( poly_first U poly_second ) = {O}
                is_patch_inside_tm2.set(patch_id_p1);
                is_patch_inside_tm2.set(patch_id_p2);
                if (!used_to_clip_a_surface)
                {
#ifdef CGAL_COREFINEMENT_DEBUG
              std::cout << "  Non-manifold edge case 5\n";
#endif
                  impossible_operation.set(INTERSECTION); // tm1 n tm2 is non-manifold
                }
              }
            }
            else
            {
              register_halfedge_pair(user_visitor, VUNDF(), h1, h2, true, false, false, true);
              //case (c4)
              // poly_first  - poly_second            = p1q1
              // poly_second - poly_first             = p2q2
              // poly_first \cap poly_second          = q1p2
              // opposite( poly_first U poly_second ) = q2p1
              if ( is_dangling_edge(ids.first, ids.second, h1, tm1, is_node_of_degree_one) ||
                   is_dangling_edge(ids.first, ids.second, h2, tm2, is_node_of_degree_one) )
              {
                if (!used_to_clip_a_surface)
                {
#ifdef CGAL_COREFINEMENT_DEBUG
              std::cout << "  Non-manifold edge case 6\n";
#endif
                  impossible_operation.set();
                  return;
                }
              }
              is_patch_inside_tm2.set(patch_id_p2);
            }
          }
          else
          {
            if( q2_is_between_p1p2 )
            {
              register_halfedge_pair(user_visitor, VUNDF(), h1, h2, false, true, true, false);
              //case (d4)
              // poly_first  - poly_second            = q2p2
              // poly_second - poly_first             = q1p1
              // poly_first \cap poly_second          = p1q2
              // opposite( poly_first U poly_second ) = p2q1
              if ( is_dangling_edge(ids.first, ids.second, h1, tm1, is_node_of_degree_one) ||
                   is_dangling_edge(ids.first, ids.second, h2, tm2, is_node_of_degree_one) )
              {
                if (!used_to_clip_a_surface)
                {
#ifdef CGAL_COREFINEMENT_DEBUG
              std::cout << "  Non-manifold edge case 7\n";
#endif
                  impossible_operation.set();
                  return;
                }
              }
              is_patch_inside_tm1.set(patch_id_q2);
              is_patch_inside_tm2.set(patch_id_p1);
            }
            else
            {
              bool p1_is_between_q1q2 = sorted_around_edge(
                ids.first, ids.second,
                index_q1, index_q2, index_p1,
                q1, q2, p1,
                vpm2, vpm1,
                nodes);
              if (!p1_is_between_q1q2){
                register_halfedge_pair(user_visitor, VUNDF(), h1, h2, false, false, false, false);
                //case (e4)
                // poly_first  - poly_second            = p1p2
                // poly_second - poly_first             = q1q2
                // poly_first \cap poly_second          = {0}
                // opposite( poly_first U poly_second ) = p2q1 U q2p1
#ifdef CGAL_COREFINEMENT_DEBUG
              std::cout << "  Non-manifold edge case 8\n";
#endif
                impossible_operation.set(UNION); // tm1 U tm2 is non-manifold
              }
              else{
                register_halfedge_pair(user_visitor, VUNDF(), h1, h2, false, false, true, true);
                //case (f4)
                is_patch_inside_tm2.set(patch_id_p1);
                is_patch_inside_tm2.set(patch_id_p2);
                // poly_first  - poly_second            = {0}
                // poly_second - poly_first             = q1p1 U p2q2
                // poly_first \cap poly_second          = p1p2
                // opposite( poly_first U poly_second ) = q2q1
#ifdef CGAL_COREFINEMENT_DEBUG
              std::cout << "  Non-manifold edge case 9\n";
#endif
                impossible_operation.set(TM2_MINUS_TM1); // tm2 - tm1 is non-manifold
              }
            }
          }
          if (inconsistent_classification()) return;
          debug_check_consistency();
        }
    }

    if (used_to_classify_patches)
    {
      export_flags( user_visitor, VUNDF(),fids1, tm1_patch_ids,
                    is_patch_inside_tm2,
                    coplanar_patches_of_tm1,
                    coplanar_patches_of_tm1_for_union_and_intersection,
                    patch_status_not_set_tm1,
                    tm1);
      export_flags( user_visitor, VUNDF(),
                    fids2, tm2_patch_ids,
                    is_patch_inside_tm1,
                    coplanar_patches_of_tm2,
                    coplanar_patches_of_tm2_for_union_and_intersection,
                    patch_status_not_set_tm2,
                    tm2);
      return;
    }

    // (2-b) Classify isolated surface patches wrt the other mesh
    // in case a mesh is not closed, any cc of the second mesh that is
    // free from intersection is considered as outside/inside
    // (depending on is_tmi_inside_out)
    if (!is_tm1_closed)
      patch_status_not_set_tm2.reset();
    if (!is_tm2_closed)
      patch_status_not_set_tm1.reset();

#ifdef CGAL_COREFINEMENT_POLYHEDRA_DEBUG
    #warning stop using next_marked_halfedge_around_target_vertex and create lists of halfedges instead?
#endif

    if ( patch_status_not_set_tm1.any() )
    {
      user_visitor.classify_intersection_free_patches(tm1);

      CGAL::Bounded_side in_tm2 = is_tm2_inside_out
                                ? ON_UNBOUNDED_SIDE : ON_BOUNDED_SIDE;

      typedef typename Nodes_vector::Exact_kernel Exact_kernel;
      typedef Side_of_helper<TriangleMesh,
                             Node_id_map,
                             VertexPointMap2,
                             Nodes_vector, Kernel> VPM_helper;
      typedef typename VPM_helper::VPM SOTM_vpm2;
      typedef typename VPM_helper::Tree_type Tree_type;

      Tree_type tree;
      VPM_helper::build_tree(tm2, tree, vertex_to_node_id2, fids2, vpm2, nodes);
      Side_of_triangle_mesh<TriangleMesh, Exact_kernel, SOTM_vpm2, Tree_type> inside_tm2(tree);

      for(face_descriptor f : faces(tm1))
      {
        const std::size_t f_id = get(fids1, f);
        const std::size_t patch_id = tm1_patch_ids[ f_id ];
        if ( patch_status_not_set_tm1.test( patch_id ) )
        {
          patch_status_not_set_tm1.reset( patch_id );
          halfedge_descriptor h = halfedge(f, tm1);
          Node_id index_p1 = get_node_id(target(h, tm1), vertex_to_node_id1);
          std::array<Node_id, 3> fnids = { index_p1, index_p1, index_p1 };
          if (index_p1 != NID)
          {
            h=next(h, tm1);
            index_p1 = get_node_id(target(h, tm1), vertex_to_node_id1);
            fnids[1]=index_p1;
            if (index_p1 != NID)
            {
              h=next(h, tm1);
              index_p1 = get_node_id(target(h, tm1), vertex_to_node_id1);
              fnids[2]=index_p1;
            }
          }

          if (index_p1 != NID)
          {
            if (coplanar_patches_of_tm1.test(patch_id))
            {
              // Two "identical" coplanar patches that are entire connected components
              // we have the correspondence between cpln patches thanks to faces in tm1_coplanar_faces and tm2_coplanar_faces
              CGAL_assertion(tm1_coplanar_faces.size()==tm2_coplanar_faces.size());
              if (coplanar_tm1_to_coplanar_tm2.empty()) // fill container only once
              {
                coplanar_tm1_to_coplanar_tm2.resize(nb_patches_tm1, NID);
                for (std::size_t i=0; i<tm1_coplanar_faces.size(); ++i)
                {
                  std::size_t pid1 = tm1_patch_ids[get(fids1, tm1_coplanar_faces[i])];
                  std::size_t pid2 = tm2_patch_ids[get(fids2, tm2_coplanar_faces[i])];
                  coplanar_tm1_to_coplanar_tm2[pid1]=pid2;
                }

                const vertex_descriptor null_v = boost::graph_traits<TriangleMesh>::null_vertex();
                extreme_vertex_per_cc_1.assign(nb_patches_tm1, null_v);
                for (face_descriptor fd : faces(tm1))
                {
                  std::size_t patch_id = tm1_patch_ids[get(fids1, fd)];
                  if (!coplanar_patches_of_tm1.test(patch_id)) continue;
                  halfedge_descriptor hd=halfedge(fd, tm1);
                  for (halfedge_descriptor h : CGAL::halfedges_around_face(hd, tm1))
                  {
                    vertex_descriptor vd = target(h, tm1);
                    if (extreme_vertex_per_cc_1[patch_id]==null_v || get(vpm1,extreme_vertex_per_cc_1[patch_id])<get(vpm1,vd))
                      extreme_vertex_per_cc_1[patch_id]=vd;
                  }
                }

                extreme_vertex_per_cc_2.assign(nb_patches_tm2, null_v);
                for (face_descriptor fd : faces(tm2))
                {
                  std::size_t patch_id = tm2_patch_ids[get(fids2, fd)];
                  if (!coplanar_patches_of_tm2.test(patch_id)) continue;
                  halfedge_descriptor hd=halfedge(fd, tm2);
                  for (halfedge_descriptor h : CGAL::halfedges_around_face(hd, tm2))
                  {
                    vertex_descriptor vd = target(h, tm2);
                    if (extreme_vertex_per_cc_2[patch_id]==null_v || get(vpm2,extreme_vertex_per_cc_2[patch_id])<get(vpm2,vd))
                      extreme_vertex_per_cc_2[patch_id]=vd;
                  }
                }
              }

              const std::size_t patch_id2=coplanar_tm1_to_coplanar_tm2[patch_id];
              CGAL_assertion(patch_id2!=NID);

              bool is_oo_tm1 = ::CGAL::Polygon_mesh_processing::internal::is_outward_oriented(extreme_vertex_per_cc_1[patch_id], tm1, parameters::vertex_point_map(vpm1)),
                   is_oo_tm2 = ::CGAL::Polygon_mesh_processing::internal::is_outward_oriented(extreme_vertex_per_cc_2[patch_id2], tm2, parameters::vertex_point_map(vpm2));

              if (is_oo_tm1==is_oo_tm2)
              {
                coplanar_patches_of_tm1_for_union_and_intersection.set(patch_id);
                coplanar_patches_of_tm2_for_union_and_intersection.set(patch_id2);
                patch_status_not_set_tm2.reset( patch_id2 );
              }
            }
            else
            {
              typename Exact_kernel::Point_3 e_centroid =
                centroid(nodes.exact_node(fnids[0]),
                         nodes.exact_node(fnids[1]),
                         nodes.exact_node(fnids[2]));

              Bounded_side position = inside_tm2(e_centroid);

              CGAL_assertion( position != ON_BOUNDARY);
              if ( position == in_tm2 )
                is_patch_inside_tm2.set(patch_id);
            }
          }
          else
          {
            Bounded_side position = inside_tm2( nodes.to_exact(get(vpm1, target(h, tm1))));
            CGAL_assertion( position != ON_BOUNDARY);
            if ( position == in_tm2 )
              is_patch_inside_tm2.set(patch_id);
          }
          if ( patch_status_not_set_tm1.none() ) break;
        }
      }
    }

    if (used_to_clip_a_surface) patch_status_not_set_tm2.reset();

    if ( patch_status_not_set_tm2.any() )
    {
      user_visitor.classify_intersection_free_patches(tm2);

      CGAL::Bounded_side in_tm1 = is_tm1_inside_out
                                ? ON_UNBOUNDED_SIDE : ON_BOUNDED_SIDE;

      typedef typename Nodes_vector::Exact_kernel Exact_kernel;
      typedef Side_of_helper<TriangleMesh,
                             Node_id_map,
                             VertexPointMap1,
                             Nodes_vector, Kernel> VPM_helper;
      typedef typename VPM_helper::VPM SOTM_vpm1;
      typedef typename VPM_helper::Tree_type Tree_type;

      Tree_type tree;
      VPM_helper::build_tree(tm1, tree, vertex_to_node_id1, fids1, vpm1, nodes);
      Side_of_triangle_mesh<TriangleMesh, Exact_kernel, SOTM_vpm1, Tree_type> inside_tm1(tree);

      for(face_descriptor f : faces(tm2))
      {
        const std::size_t f_id = get(fids2, f);
        std::size_t patch_id=tm2_patch_ids[ f_id ];
        if ( patch_status_not_set_tm2.test( patch_id ) )
        {
          patch_status_not_set_tm2.reset( patch_id );
          halfedge_descriptor h = halfedge(f, tm2);
          Node_id index_p2 = get_node_id(target(h, tm2), vertex_to_node_id2);
          std::array<Node_id, 3> fnids = { index_p2, index_p2, index_p2 };
          if (index_p2 != NID)
          {
            h=next(h, tm2);
            index_p2 = get_node_id(target(h, tm2), vertex_to_node_id2);
            fnids[1]=index_p2;
            if (index_p2 != NID)
            {
              h=next(h, tm2);
              index_p2 = get_node_id(target(h, tm2), vertex_to_node_id2);
              fnids[2]=index_p2;
            }
          }
          if (index_p2 != NID)
          {
            if (coplanar_patches_of_tm2.test(patch_id))
            {
              if (is_tm1_inside_out == is_tm2_inside_out)
                coplanar_patches_of_tm2_for_union_and_intersection.set(patch_id);
            }
            else
            {
              typename Exact_kernel::Point_3 e_centroid =
                centroid(nodes.exact_node(fnids[0]),
                         nodes.exact_node(fnids[1]),
                         nodes.exact_node(fnids[2]));
              Bounded_side position = inside_tm1(e_centroid);
              CGAL_assertion( position != ON_BOUNDARY);
              if ( position == in_tm1 )
                is_patch_inside_tm1.set(patch_id);
            }
          }
          else
          {
            Bounded_side position = inside_tm1( nodes.to_exact(get(vpm2, target(h, tm2))));
            CGAL_assertion( position != ON_BOUNDARY);
            if ( position == in_tm1 )
              is_patch_inside_tm1.set(patch_id);
          }
          if ( patch_status_not_set_tm2.none() ) break;
        }
      }
    }

    CGAL_assertion(patch_status_not_set_tm1.none());
    CGAL_assertion(patch_status_not_set_tm2.none());

    if (has_soup_visitor)
    {
      export_flags( user_visitor, HSV(),
                    fids1, tm1_patch_ids,
                    is_patch_inside_tm2,
                    coplanar_patches_of_tm1,
                    coplanar_patches_of_tm1_for_union_and_intersection,
                    patch_status_not_set_tm1,
                    tm1);
      export_flags( user_visitor, HSV(),
                    fids2, tm2_patch_ids,
                    is_patch_inside_tm1,
                    coplanar_patches_of_tm2,
                    coplanar_patches_of_tm2_for_union_and_intersection,
                    patch_status_not_set_tm2,
                    tm2);

      bool nothing_possible = true;
      for (int i=0;i<4; ++i)
        if ( !impossible_operation.test(i) && requested_output[i] )
          nothing_possible = false;
      if (nothing_possible)
        return;
    }

    //to maintain a halfedge on each polyline + pair<bool,int>
    //with first = "is the key (pair<Node_id,Node_id>) was reversed?"
    // and second is the number of edges -1 in the polyline
    //typedef std::map< std::pair<Node_id,Node_id>,
    //                  std::pair< std::map<TriangleMesh*,
    //                                      halfedge_descriptor>,
    //                             std::pair<bool,int> > >
    //                                        An_edge_per_polyline_map;

#ifdef CGAL_COREFINEMENT_POLYHEDRA_DEBUG
    #warning add a mechanism to handle the patches independently \
             (for example calculating the volume without \
               building the polyhedron) \
             This can be done by using a functor to which we give \
             the bitset, the polyhedra and one facet per patch?
#endif // CGAL_COREFINEMENT_POLYHEDRA_DEBUG

#ifdef CGAL_COREFINEMENT_DEBUG
    std::cout << "is_patch_inside_tm2 " <<  is_patch_inside_tm2 << "\n";
    std::cout << "is_patch_inside_tm1 " << is_patch_inside_tm1 << "\n";
    std::cout << "coplanar_patches_of_tm1 " << coplanar_patches_of_tm1 << "\n";
    std::cout << "coplanar_patches_of_tm2 " << coplanar_patches_of_tm2 << "\n";
    std::cout << "coplanar_patches_of_tm1_for_union_and_intersection "
              << coplanar_patches_of_tm1_for_union_and_intersection << "\n";
    std::cout << "coplanar_patches_of_tm2_for_union_and_intersection "
              << coplanar_patches_of_tm2_for_union_and_intersection << "\n";
    std::cout << "Size of patches of tm1: ";
    std::copy(tm1_patch_sizes.rbegin(), tm1_patch_sizes.rend(),
              std::ostream_iterator<std::size_t>(std::cout," ") );
    std::cout << "\n";
    std::cout << "Size of patches of tm2: ";
    std::copy(tm2_patch_sizes.rbegin(), tm2_patch_sizes.rend(),
              std::ostream_iterator<std::size_t>(std::cout," ") );
    std::cout << "\n";
#endif

    //backup an halfedge per polyline
    std::vector <halfedge_descriptor> tm1_polylines, tm2_polylines;
    std::vector<std::size_t> polyline_lengths;

    for (typename An_edge_per_polyline_map::const_iterator
          it=an_edge_per_polyline.begin(),
          it_end=an_edge_per_polyline.end();
          it!=it_end;++it)
    {
      const std::pair<bool, std::size_t>& polyline_info=it->second.second;

      halfedge_descriptor h1 = it->second.first.find(&tm1)->second;
      halfedge_descriptor h2 = it->second.first.find(&tm2)->second;

      if( polyline_info.first ){
        h1=opposite(h1,tm1);
        h2=opposite(h2,tm2);
      }

      tm1_polylines.push_back(h1);
      tm2_polylines.push_back(h2);
      polyline_lengths.push_back(polyline_info.second+1);
    }

    typedef Patch_container<TriangleMesh, FaceIdMap1, Intersection_edge_map> Patches1;
    typedef Patch_container<TriangleMesh, FaceIdMap2, Intersection_edge_map> Patches2;

    std::unordered_set<vertex_descriptor> border_nm_vertices; // only used if used_to_clip_a_surface == true
    if (used_to_clip_a_surface)
    {
      if (!is_tm1_closed)
      {
        // \todo Note a loop over vertex_to_node_id1 would be sufficient
        // if we merge the patch-id of patches to be removed (that way
        // non-manifold vertices would not be duplicated in interior
        // vertices of patche)
        // special code to handle non-manifold vertices on the boundary
        for (vertex_descriptor vd : vertices(tm1))
        {
          std::optional<halfedge_descriptor> op_h = is_border(vd, tm1);
          if (op_h == std::nullopt) continue;
          halfedge_descriptor h = *op_h;
          CGAL_assertion( target(h, tm1) == vd);
          // check if the target of h is a non-manifold vertex
          halfedge_descriptor nh = prev( opposite(h, tm1), tm1 );
          while (!is_border( opposite(nh, tm1), tm1 ) )
          {
            nh = prev( opposite(nh, tm1), tm1 );
          }
          nh = opposite(nh, tm1);
          if (next(h, tm1) != nh)
            border_nm_vertices.insert(target(h, tm1));
        }
      }
    }

    //store the patch description in a container to avoid recomputing it several times
    Patches1 patches_of_tm1(tm1, tm1_patch_ids, fids1, intersection_edges1, nb_patches_tm1);
    Patches2 patches_of_tm2(tm2, tm2_patch_ids, fids2, intersection_edges2, nb_patches_tm2);

    // report input coplanar faces
    if (coplanar_patches_of_tm1.any())
    {
      for (std::size_t i = coplanar_patches_of_tm1.find_first();
                 i < coplanar_patches_of_tm1.npos;
                 i = coplanar_patches_of_tm1.find_next(i))
      {

        for (face_descriptor f : patches_of_tm1[i].faces)
        {
          user_visitor.subface_of_coplanar_faces_intersection(f, tm1);
        }
      }
    }
    if (coplanar_patches_of_tm2.any())
    {
      for (std::size_t i = coplanar_patches_of_tm2.find_first();
                 i < coplanar_patches_of_tm2.npos;
                 i = coplanar_patches_of_tm2.find_next(i))
      {

        for (face_descriptor f : patches_of_tm2[i].faces)
        {
          user_visitor.subface_of_coplanar_faces_intersection(f, tm2);
        }
      }
    }

    // for each boolean operation, define two bitsets of patches contributing
    // to the result
    std::vector< boost::dynamic_bitset<> > patches_of_tm1_used(4);
    std::vector< boost::dynamic_bitset<> > patches_of_tm2_used(4);

    /// handle the bitset for the union
    if ( !impossible_operation.test(UNION) && requested_output[UNION] )
    {
      //define patches to import from P
      patches_of_tm1_used[UNION] = ~is_patch_inside_tm2 - coplanar_patches_of_tm1;
      //define patches to import from Q
      patches_of_tm2_used[UNION] = ~is_patch_inside_tm1 - coplanar_patches_of_tm2;
      //handle coplanar patches
      if (coplanar_patches_of_tm1.any())
      {
        if (requested_output[UNION]==&tm2)
          patches_of_tm2_used[UNION] |= coplanar_patches_of_tm2_for_union_and_intersection;
        else
          patches_of_tm1_used[UNION] |= coplanar_patches_of_tm1_for_union_and_intersection;
      }
    }

    /// handle the bitset for the intersection
    if ( !impossible_operation.test(INTERSECTION) && requested_output[INTERSECTION] )
    {
      //define patches to import from P
      patches_of_tm1_used[INTERSECTION] = is_patch_inside_tm2;
      //define patches to import from Q
      patches_of_tm2_used[INTERSECTION] = is_patch_inside_tm1;
      //handle coplanar patches
      if (coplanar_patches_of_tm1.any())
      {
        if (requested_output[INTERSECTION]==&tm2)
          patches_of_tm2_used[INTERSECTION] |= coplanar_patches_of_tm2_for_union_and_intersection;
        else
          patches_of_tm1_used[INTERSECTION] |= coplanar_patches_of_tm1_for_union_and_intersection;
      }
    }
    // handle special cases for clipping case
    if (used_to_clip_a_surface)
    {
      if (coplanar_patches_of_tm1.any())
      {
        if (use_compact_clipper)
          patches_of_tm1_used[INTERSECTION] |= coplanar_patches_of_tm1;
        else
          patches_of_tm1_used[INTERSECTION] -= coplanar_patches_of_tm1;
      }
      patches_of_tm2_used[INTERSECTION].reset();
    }

    /// handle the bitset for P-Q
    if ( !impossible_operation.test(TM1_MINUS_TM2) && requested_output[TM1_MINUS_TM2] )
    {
      //define patches to import from P
      patches_of_tm1_used[TM1_MINUS_TM2] = (~is_patch_inside_tm2 - coplanar_patches_of_tm1);
      //define patches to import from Q
      patches_of_tm2_used[TM1_MINUS_TM2] = is_patch_inside_tm1;
      //handle coplanar patches
      if (coplanar_patches_of_tm1.any())
      {
        if (requested_output[TM1_MINUS_TM2]==&tm2)
          patches_of_tm2_used[TM1_MINUS_TM2] |= ~coplanar_patches_of_tm2_for_union_and_intersection & coplanar_patches_of_tm2;
        else
          patches_of_tm1_used[TM1_MINUS_TM2] |= ~coplanar_patches_of_tm1_for_union_and_intersection & coplanar_patches_of_tm1;
      }
    }

    /// handle the bitset for Q-P
    if ( !impossible_operation.test(TM2_MINUS_TM1) && requested_output[TM2_MINUS_TM1] )
    {
      //define patches to import from P
      patches_of_tm1_used[TM2_MINUS_TM1] = is_patch_inside_tm2;
      //define patches to import from Q
      patches_of_tm2_used[TM2_MINUS_TM1] = ~is_patch_inside_tm1 - coplanar_patches_of_tm2;
      //handle coplanar patches
      if (coplanar_patches_of_tm1.any())
      {
        if (requested_output[TM2_MINUS_TM1]==&tm2)
          patches_of_tm2_used[TM2_MINUS_TM1] |= ~coplanar_patches_of_tm2_for_union_and_intersection & coplanar_patches_of_tm2;
        else
          patches_of_tm1_used[TM2_MINUS_TM1] |= ~coplanar_patches_of_tm1_for_union_and_intersection & coplanar_patches_of_tm1;
      }
    }

    #ifdef CGAL_COREFINEMENT_DEBUG
    std::cout << "patches_of_tm1_used[UNION] " << patches_of_tm1_used[UNION] << "\n";
    std::cout << "patches_of_tm2_used[UNION] " << patches_of_tm2_used[UNION] << "\n";
    std::cout << "patches_of_tm1_used[INTERSECTION] " << patches_of_tm1_used[INTERSECTION] << "\n";
    std::cout << "patches_of_tm2_used[INTERSECTION] " << patches_of_tm2_used[INTERSECTION] << "\n";
    std::cout << "patches_of_tm1_used[TM1_MINUS_TM2] " << patches_of_tm1_used[TM1_MINUS_TM2] << "\n";
    std::cout << "patches_of_tm2_used[TM1_MINUS_TM2] " << patches_of_tm2_used[TM1_MINUS_TM2] << "\n";
    std::cout << "patches_of_tm1_used[TM2_MINUS_TM1] " << patches_of_tm1_used[TM2_MINUS_TM1] << "\n";
    std::cout << "patches_of_tm2_used[TM2_MINUS_TM1] " << patches_of_tm2_used[TM2_MINUS_TM1] << "\n";
    #endif // CGAL_COREFINEMENT_DEBUG
    // Schedule the order in which the different boolean operations
    // should be done. First operations are those filling meshes
    // different from tm1 and tm2, then the one modifying tm1 and
    // finally the one modifying tm2.
    std::vector<Boolean_operation_type> out_of_place_operations;
    Boolean_operation_type inplace_operation_tm1=NONE,
                           inplace_operation_tm2=NONE;
    for (int i=0;i<4;++i)
    {
      Boolean_operation_type operation=enum_cast<Boolean_operation_type>(i);

      if (!requested_output[operation] || impossible_operation.test(operation))
        continue;

      if (requested_output[operation]==&tm1)
        inplace_operation_tm1=operation;
      else
        if (requested_output[operation]==&tm2)
          inplace_operation_tm2=operation;
        else
          out_of_place_operations.push_back(operation);
    }

    /// first handle operations in a mesh that is neither tm1 nor tm2
    for(Boolean_operation_type operation : out_of_place_operations)
    {
      user_visitor.out_of_place_operation(operation);

      TriangleMesh& output = *(*requested_output[operation]);
      CGAL_assertion(&tm1!=&output && &tm2!=&output);

      Intersection_polylines polylines(tm1_polylines,
                                       tm2_polylines,
                                       polyline_lengths);
      // skip the import of polylines only incident to patch(es)
      // not used by the current operation
      fill_polylines_to_skip(
        polylines, tm1_patch_ids, tm2_patch_ids,
        patches_of_tm1_used[operation], patches_of_tm2_used[operation],
        fids1, fids2, tm1, tm2
      );

      std::vector<edge_descriptor> shared_edges;

      #define CGAL_COREF_FUNCTION_CALL_DEF(BO_type) \
        fill_new_triangle_mesh( \
          output, \
          patches_of_tm1_used[BO_type], patches_of_tm2_used[BO_type], \
          patches_of_tm1, patches_of_tm2, \
          BO_type == TM2_MINUS_TM1, BO_type == TM1_MINUS_TM2, \
          polylines, \
          intersection_edges1, intersection_edges2, \
          vpm1, vpm2, *std::get<BO_type>(output_vpms), \
          marks_on_input_edges.ecm1, \
          marks_on_input_edges.ecm2, \
          std::get<BO_type>(out_edge_mark_maps), \
          shared_edges, \
          user_visitor \
        )
      CGAL_COREF_FUNCTION_CALL(operation)
      #undef CGAL_COREF_FUNCTION_CALL_DEF
      mark_edges(out_edge_mark_maps, shared_edges, operation);
    }

    Edge_map disconnected_patches_edge_to_tm2_edge;

    /// handle the operations updating tm1 and/or tm2
    if ( inplace_operation_tm1!=NONE )
    {
      // mark intersection edges in tm1 (using output constrained edge map)
      mark_edges(out_edge_mark_maps,
                 mesh_to_intersection_edges[&tm1],
                 inplace_operation_tm1);

      CGAL_assertion( *requested_output[inplace_operation_tm1] == &tm1 );

      if ( inplace_operation_tm2!=NONE)
      {
        user_visitor.in_place_operations(inplace_operation_tm1, inplace_operation_tm2);

        // mark intersection edges in tm2 (using output constrained edge map)
        mark_edges(out_edge_mark_maps,
                   mesh_to_intersection_edges[&tm2],
                   inplace_operation_tm2);

        // operation in tm1 with removal (and optionally inside-out) delayed
        // First backup the border edges of patches to be used
        Patches1 tmp_patches_of_tm1(tm1,
                                    patches_of_tm1.patch_ids,
                                    patches_of_tm1.fids,
                                    patches_of_tm1.is_intersection_edge,
                                    patches_of_tm1.patches.size());

        boost::dynamic_bitset<> patches_of_tm1_removed =
            ~patches_of_tm1_used[inplace_operation_tm1];
        for (std::size_t i = patches_of_tm1_removed.find_first();
                         i < patches_of_tm1_removed.npos;
                         i = patches_of_tm1_removed.find_next(i))
        {
          // we are only interested by patch border halfedges so
          // squeeze the auto-filling mechanism
          tmp_patches_of_tm1.patches[i].is_initialized=true;
          tmp_patches_of_tm1.patches[i].shared_edges=
            patches_of_tm1[i].shared_edges;
        }

        Intersection_polylines polylines_in_tm1(
          tm1_polylines, tm2_polylines, polyline_lengths);
        Intersection_polylines polylines_in_tm2=polylines_in_tm1;
        fill_polylines_to_skip(
          polylines_in_tm1, tm1_patch_ids, tm2_patch_ids,
          patches_of_tm1_used[inplace_operation_tm1],
          patches_of_tm2_used[inplace_operation_tm1],
          fids1, fids2, tm1, tm2);
        fill_polylines_to_skip(
          polylines_in_tm2, tm1_patch_ids, tm2_patch_ids,
          patches_of_tm1_used[inplace_operation_tm2],
          patches_of_tm2_used[inplace_operation_tm2],
          fids1, fids2, tm1, tm2);
        // force the initialization of the patches of tm1 used
        // for the operation in tm2 before tm1 is modified
        for (std::size_t i=patches_of_tm1_used[inplace_operation_tm2].find_first();
                         i < patches_of_tm1_used[inplace_operation_tm2].npos;
                         i = patches_of_tm1_used[inplace_operation_tm2].find_next(i))
        {
          patches_of_tm1[i];
        }
        // Operation in tm1: disconnect patches not use and append the one from tm2
        #define CGAL_COREF_FUNCTION_CALL_DEF(BO_type)\
        compute_inplace_operation_delay_removal_and_insideout( \
          tm1, \
          tm2, \
          patches_of_tm1_used[BO_type], \
          patches_of_tm2_used[BO_type], \
          patches_of_tm1, patches_of_tm2, \
          BO_type == TM1_MINUS_TM2 || \
            BO_type == TM2_MINUS_TM1, \
          polylines_in_tm1, \
          vpm1, vpm2, \
          marks_on_input_edges.ecm1, \
          marks_on_input_edges.ecm2, \
          std::get<BO_type>(out_edge_mark_maps), \
          disconnected_patches_edge_to_tm2_edge, \
          user_visitor)
        CGAL_COREF_FUNCTION_CALL(inplace_operation_tm1)
        #undef CGAL_COREF_FUNCTION_CALL_DEF
        // Operation in tm2: discard patches and append the one from tm2
        CGAL_assertion( *requested_output[inplace_operation_tm2] == &tm2 );

        #define CGAL_COREF_FUNCTION_CALL_DEF(BO_type)\
          compute_inplace_operation( tm2, tm1, \
                                     patches_of_tm2_used[BO_type], \
                                     patches_of_tm1_used[BO_type], \
                                     patches_of_tm2, patches_of_tm1, \
                                     BO_type==TM1_MINUS_TM2, \
                                     BO_type==TM2_MINUS_TM1, \
                                     vpm2, \
                                     vpm1, \
                                     marks_on_input_edges.ecm2, \
                                     marks_on_input_edges.ecm1, \
                                     std::get<BO_type>(out_edge_mark_maps), \
                                     disconnected_patches_edge_to_tm2_edge, \
                                     user_visitor)
        CGAL_COREF_FUNCTION_CALL(inplace_operation_tm2)
        #undef CGAL_COREF_FUNCTION_CALL_DEF

        // remove polylines only on the border of patches not kept in tm2
        if (polylines_in_tm2.to_skip.any())
          remove_unused_polylines(tm2,
                                  ~patches_of_tm2_used[inplace_operation_tm2],
                                  patches_of_tm2);
        // now remove patches temporarily kept in tm1
        remove_disconnected_patches(tm1,
                                    patches_of_tm1,
                                    patches_of_tm1_removed,
                                    marks_on_input_edges.ecm1);

        // transfer marks of edges of patches kept to the output edge mark property
        #define CGAL_COREF_FUNCTION_CALL_DEF(BO_type) \
          copy_edge_mark<TriangleMesh>( \
          tm1, marks_on_input_edges.ecm1, \
          std::get<BO_type>(out_edge_mark_maps))
        CGAL_COREF_FUNCTION_CALL(inplace_operation_tm1)
        #undef CGAL_COREF_FUNCTION_CALL_DEF

        // remove polylines only on the border of patches not kept in tm1
        if (polylines_in_tm1.to_skip.any())
          remove_unused_polylines(tm1,
                                  ~patches_of_tm1_used[inplace_operation_tm1],
                                  tmp_patches_of_tm1);
         // finally reverse orientation of tm1 if needed
         if (inplace_operation_tm1 == TM2_MINUS_TM1)
           CGAL::Polygon_mesh_processing::reverse_face_orientations(*&tm1);
      }
      else{
        user_visitor.in_place_operation(inplace_operation_tm1);

        /// handle the operation updating only tm1
        CGAL_assertion( *requested_output[inplace_operation_tm1] == &tm1 );
        Intersection_polylines polylines(
          tm1_polylines, tm2_polylines, polyline_lengths);
        fill_polylines_to_skip(
          polylines, tm1_patch_ids, tm2_patch_ids,
          patches_of_tm1_used[inplace_operation_tm1],
          patches_of_tm2_used[inplace_operation_tm1],
          fids1, fids2, tm1, tm2);

        if (used_to_clip_a_surface)
        {
          // The following code is here to handle the case when an intersection polyline
          // contains some border edges of tm1 that should be considered as an independent polyline.
          // This polyline removal should be handled by remove_unused_polylines.
          // However, since all nodes are of degree 2 the polyline is not split at
          // the correct point and trouble happen. Here the workaround consists in
          // removing border edges of patches to be removed that are not in a
          // polyline schedule for removal.
          boost::dynamic_bitset<> patches_to_remove = ~patches_of_tm1_used[inplace_operation_tm1];
          for (std::size_t i = patches_to_remove.find_first();
                           i < patches_to_remove.npos;
                           i = patches_to_remove.find_next(i))
          {
            typedef typename std::vector<halfedge_descriptor>::iterator Hedge_iterator;
            std::vector< Hedge_iterator > to_rm;
            for (Hedge_iterator it = patches_of_tm1[i].shared_edges.begin();
                                it!= patches_of_tm1[i].shared_edges.end();
                                ++it)
            {
              if ( is_border(opposite(*it, tm1), tm1) )
                to_rm.push_back(it);
            }
            if (!to_rm.empty())
            {
              std::reverse(to_rm.begin(), to_rm.end());
              for(Hedge_iterator it : to_rm)
              {
                patches_of_tm1[i].interior_edges.push_back(*it);
                if (it!=std::prev(patches_of_tm1[i].shared_edges.end()))
                  std::swap(patches_of_tm1[i].shared_edges.back(), *it);
                patches_of_tm1[i].shared_edges.pop_back();
              }
              //now update interior vertices
              std::set<vertex_descriptor> border_vertices;
              for(halfedge_descriptor h : patches_of_tm1[i].shared_edges)
              {
                border_vertices.insert( target(h,tm1) );
                border_vertices.insert( source(h,tm1) );
              }

              for(halfedge_descriptor h : patches_of_tm1[i].interior_edges)
              {
                if ( !border_vertices.count( target(h,tm1) ) )
                  patches_of_tm1[i].interior_vertices.insert( target(h,tm1) );
                if ( !border_vertices.count( source(h,tm1) ) )
                  patches_of_tm1[i].interior_vertices.insert( source(h,tm1) );
              }
            }
          }

          // Code dedicated to the handling of non-manifold vertices
          for(vertex_descriptor vd : border_nm_vertices)
          {
            // first check if at least one incident patch will be kept
            std::unordered_set<std::size_t> id_p_rm;
            bool all_removed=true;
            for(halfedge_descriptor h : halfedges_around_target(vd, tm1))
            {
              face_descriptor f = face(h, tm1);
              if ( f != GT::null_face() )
              {
                const std::size_t p_id = tm1_patch_ids[ get(fids1, f) ];
                if ( patches_to_remove.test(p_id) )
                  id_p_rm.insert(p_id);
                else
                  all_removed=false;
              }
            }
            if (all_removed)
              id_p_rm.erase(id_p_rm.begin());
            // remove the vertex from the interior vertices of patches to be removed
            for(std::size_t pid : id_p_rm)
              patches_of_tm1[pid].interior_vertices.erase(vd);

            // we now need to update the next/prev relationship induced by the future removal of patches
            // that will not be updated after patch removal
            if (!all_removed && !id_p_rm.empty())
            {
              typedef std::pair<halfedge_descriptor, halfedge_descriptor> Hedge_pair;
              std::vector< Hedge_pair> hedges_to_link;
              typename CGAL::Halfedge_around_target_iterator<TriangleMesh> hit, end;
              std::tie(hit,end) = halfedges_around_target(vd, tm1);
              for(; hit!=end; ++hit)
              {
                // look for a border halfedge incident to the non-manifold vertex that will not be
                // removed.
                if ( !is_border(*hit, tm1) ||
                     patches_to_remove.test( tm1_patch_ids[ get(fids1, face(opposite(*hit, tm1), tm1)) ] ) )
                {
                  continue;
                }
                // we have to fix only cases when the next halfedge is to be removed
                halfedge_descriptor nh = next(*hit, tm1);
                if ( !patches_to_remove.test( tm1_patch_ids[ get(fids1, face(opposite(nh, tm1), tm1)) ] ) )
                  continue;

                halfedge_descriptor h = *hit;
                // we are now looking for a potential next candidate halfedge
                do{
                  ++hit;
                  if (hit == end) break;
                  if ( is_border(*hit, tm1) )
                  {
                    if ( patches_to_remove.test( tm1_patch_ids[ get(fids1, face(opposite(*hit, tm1), tm1)) ] ) )
                    {
                      // we check if the next halfedge is a good next
                      nh = next(*hit, tm1);
                      if ( !patches_to_remove.test( tm1_patch_ids[ get(fids1, face(opposite(nh, tm1), tm1)) ] ) )
                      {
                        hedges_to_link.push_back( Hedge_pair(h, nh) );
                        break;
                      }
                    }
                    else
                    {
                      // we push-back the halfedge for the next round only if it was not the first
                      if (h != *std::prev(hit))
                        --hit;
                      break;
                    }
                  }
                }
                while(true);
                if (hit == end) break;
              }
              for(const Hedge_pair& p : hedges_to_link)
                set_next(p.first, p.second, tm1);
            }
          }
        }

        #define CGAL_COREF_FUNCTION_CALL_DEF(BO_type) \
          compute_inplace_operation( \
            tm1, tm2, \
            patches_of_tm1_used[BO_type], \
            patches_of_tm2_used[BO_type], \
            patches_of_tm1, patches_of_tm2, \
            BO_type == TM2_MINUS_TM1, \
            BO_type == TM1_MINUS_TM2, \
            vpm1, \
            vpm2, \
            marks_on_input_edges.ecm1, \
            marks_on_input_edges.ecm2, \
            std::get<BO_type>(out_edge_mark_maps), \
            polylines, \
            user_visitor \
          )
        CGAL_COREF_FUNCTION_CALL(inplace_operation_tm1)
        #undef CGAL_COREF_FUNCTION_CALL_DEF
        // remove polylines only on the border of patches not kept
        if (polylines.to_skip.any())
          remove_unused_polylines(tm1,
                                  ~patches_of_tm1_used[inplace_operation_tm1],
                                  patches_of_tm1);
      }
    }
    else
      if ( inplace_operation_tm2!=NONE )
      {
        user_visitor.in_place_operation(inplace_operation_tm2);

        // mark intersection edges in tm2 (using output constrained edge map)
        mark_edges(out_edge_mark_maps,
                   mesh_to_intersection_edges[&tm2],
                   inplace_operation_tm2);

        /// handle the operation updating only tm2
        CGAL_assertion( *requested_output[inplace_operation_tm2] == &tm2 );
        Intersection_polylines polylines(
          tm2_polylines, tm1_polylines, polyline_lengths);
        fill_polylines_to_skip(
          polylines, tm2_patch_ids, tm1_patch_ids,
          patches_of_tm2_used[inplace_operation_tm2],
          patches_of_tm1_used[inplace_operation_tm2],
          fids2, fids1, tm2, tm1
        );
        #define CGAL_COREF_FUNCTION_CALL_DEF(BO_type) \
          compute_inplace_operation( tm2, tm1, \
                                     patches_of_tm2_used[BO_type], \
                                     patches_of_tm1_used[BO_type], \
                                     patches_of_tm2, patches_of_tm1, \
                                     BO_type==TM1_MINUS_TM2, \
                                     BO_type==TM2_MINUS_TM1, \
                                     vpm2, \
                                     vpm1, \
                                     marks_on_input_edges.ecm2, \
                                     marks_on_input_edges.ecm1, \
                                     std::get<BO_type>(out_edge_mark_maps), \
                                     polylines, \
                                     user_visitor);
        CGAL_COREF_FUNCTION_CALL(inplace_operation_tm2)
        #undef CGAL_COREF_FUNCTION_CALL_DEF
        // remove polylines only on the border of patches not kept
        if (polylines.to_skip.any())
          remove_unused_polylines(tm2,
                                  ~patches_of_tm2_used[inplace_operation_tm2],
                                  patches_of_tm2);
      }
  }
};


} } } // CGAL::Polygon_mesh_processing::Corefinement

#undef CGAL_COREF_FUNCTION_CALL
#endif // CGAL_POLYGON_MESH_PROCESSING_INTERNAL_FACE_GRAPH_OUTPUT_BUILDER_H
