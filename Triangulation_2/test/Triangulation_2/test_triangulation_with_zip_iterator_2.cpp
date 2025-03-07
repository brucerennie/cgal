#include <CGAL/Triangulation_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

#include <boost/iterator/zip_iterator.hpp>
#include <vector>
#include <cassert>

typedef CGAL::Exact_predicates_inexact_constructions_kernel         K;
typedef CGAL::Triangulation_vertex_base_with_info_2<unsigned, K>    Vb;
typedef CGAL::Triangulation_data_structure_2<Vb>                    Tds;
typedef CGAL::Triangulation_2<K, Tds>                               Triangulation;
typedef Triangulation::Point                                        Point;

int main()
{

  std::vector<unsigned> indices;
  indices.push_back(0);
  indices.push_back(1);
  indices.push_back(2);
  indices.push_back(3);
  indices.push_back(4);
  indices.push_back(5);

  std::vector<Point> points;
  points.push_back(Point(0,0));
  points.push_back(Point(1,0));
  points.push_back(Point(0,1));
  points.push_back(Point(1,47));
  points.push_back(Point(2,2));
  points.push_back(Point(-1,0));



  Triangulation T( boost::make_zip_iterator(boost::make_tuple( points.begin(),indices.begin() )),
                   boost::make_zip_iterator(boost::make_tuple( points.end(),indices.end() ) )  );

  assert( T.number_of_vertices() == 6 );


  // check that the info was correctly set.
  Triangulation::Finite_vertices_iterator vit;
  for (vit = T.finite_vertices_begin(); vit != T.finite_vertices_end(); ++vit)
    if( points[ vit->info() ] != vit->point() ){
      std::cerr << "Error different info" << std::endl;
      exit(EXIT_FAILURE);
    }

  return 0;
}
