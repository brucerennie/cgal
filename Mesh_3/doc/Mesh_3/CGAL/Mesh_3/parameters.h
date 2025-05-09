namespace CGAL {

namespace parameters {

/*!
 * \ingroup PkgMesh3Parameters
 *
 * The function `parameters::manifold()` is used to drive the
 * meshing algorithm for surfaces.
 * It ensures that the surface of the output mesh is a manifold surface
 *   without boundaries.
 * The manifold property of the output mesh can be achieved only if the input surface
 * is a manifold.
 * Note that the meshing algorithm provably terminates only if the input
 * sharp edges have been protected, using the
 * feature protection (see \ref Mesh_3Protectionof0and1dimensionalExposed).
 *
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::refine_mesh_3()`
 * \sa `CGAL::parameters::manifold_with_boundary()`
 * \sa `CGAL::parameters::non_manifold()`
 */
unspecified_type manifold();

/*!
 * \ingroup PkgMesh3Parameters
 *
 * The function `parameters::non_manifold()` is used to drive the
 * meshing algorithm for surfaces.
 * It does not ensure that the surface of the output mesh is a manifold surface.
 * The manifold property of the output mesh might nevertheless result from an appropriate
 * choice of meshing criteria.
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::refine_mesh_3()`
 * \sa `CGAL::parameters::manifold_with_boundary()`
 * \sa `CGAL::parameters::manifold()`
 */
unspecified_type non_manifold();

/*!
 * \ingroup PkgMesh3Parameters
 *
 * The function `parameters::manifold_with_boundary()` is used to drive the
 * meshing algorithm for surfaces.
 * It ensures that the surface of the output mesh is a manifold surface which
 * may have boundaries.
 * The manifold property of the output mesh can be achieved only if the input surface
 * is a manifold.
 * Note that the meshing algorithm provably terminates only if the input
 * sharp edges have been protected, using the
 * feature protection (see \ref Mesh_3Protectionof0and1dimensionalExposed).
 *
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::refine_mesh_3()`
 * \sa `CGAL::parameters::non_manifold()`
 * \sa `CGAL::parameters::manifold()`
 */
unspecified_type manifold_with_boundary();

/*!
 * \ingroup PkgMesh3Parameters
 *
 * The function `parameters::exude()` enables the user to trigger a call to `exude_mesh_3()` in the
 * `make_mesh_3()` and `refine_mesh_3()` mesh generation functions.
 * It also enables the user to pass parameters
 * to the optimization function `exude_mesh_3()` through these mesh generation functions.
 *
 * @tparam NamedParameters a sequence of \ref bgl_namedparameters "Named Parameters"
 * @param np an optional sequence of \ref bgl_namedparameters "Named Parameters" among the ones listed below:
 *
 * \cgalNamedParamsBegin
 *   \cgalParamNBegin{time_limit}
 *     \cgalParamDescription{is used to set up, in seconds, a CPU time limit after which the optimization process
 *                           is stopped. This time is measured using the `Real_timer` class. The default value is
 *                           0 and means that there is no time limit.}
 *     \cgalParamType{`double`}
 *     \cgalParamPrecondition{`time_limit >= 0`}
 *     \cgalParamDefault{0}
 *   \cgalParamNEnd
 *   \cgalParamNBegin{sliver_bound}
 *     \cgalParamDescription{is designed to give, in degrees, a targeted lower bound on dihedral angles of mesh cells.
 *                           The exudation process considers in turn all the mesh cells that have a smallest dihedral
 *                           angle less than sliver_bound and tries to make them disappear by weighting their vertices.
 *                           The optimization process stops when every cell in the mesh achieves this quality. The
 *                           default value is 0 and means that there is no targeted bound: the exuder then runs as long
 *                           as it can improve the smallest dihedral angles of the set of cells incident to some vertices.}
 *     \cgalParamType{`double`}
 *     \cgalParamPrecondition{`0 <= sliver_bound <= 180`}
 *     \cgalParamDefault{0}
 *   \cgalParamNEnd
 * \cgalNamedParamsEnd
 *
 * \cgalHeading{Example}
 *
 * \code{.cpp}
 * // Mesh generation with an exudation step
 * C3t3 c3t3 = make_mesh_3<c3t3>(domain,
 *                               criteria,
 *                               parameters::exude());
 *
 * refine_mesh_3(c3t3,
 *               domain,
 *               criteria,
 *               parameters::exude(parameters::time_limit(10)));
 * \endcode
 *
 * \sa `CGAL::parameters::no_exude()`
 * \sa `CGAL::exude_mesh_3()`
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::refine_mesh_3()`
 *
 */
template <class NamedParameters = parameters::Default_named_parameters>
unspecified_type exude(const Named_function_parameters& np = parameters::default_values());

/*!
 * \ingroup PkgMesh3Parameters
 *
 * Provides an option indicating that 0 and 1-dimensional features
 * have to be taken into account (the domain must be a model of `MeshDomainWithFeatures_3`).
 * This is the default behavior when the domain is a model
 * of `MeshDomainWithFeatures_3`.
 *
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::refine_mesh_3()`
 * \sa `CGAL::parameters::no_features()`
 *
 */
unspecified_type features();

/*!
 * \ingroup PkgMesh3Parameters
 *
 * The function `parameters::lloyd()` enables the user to trigger a call of
 * `lloyd_optimize_mesh_3()` in the mesh generation functions
 * `make_mesh_3()` and `refine_mesh_3()`. It also enables the user to pass
 * parameters to the optimization function
 * `lloyd_optimize_mesh_3()` through these mesh generation functions.
 *
 * \tparam NamedParameters a sequence of \ref bgl_namedparameters "Named Parameters"
 *
 * @param np an optional sequence of \ref bgl_namedparameters "Named Parameters" among the ones listed below:
 *
 * \cgalNamedParamsBegin
 *   \cgalParamNBegin{time_limit}
 *     \cgalParamDescription{to set up, in seconds, a CPU time limit after which the optimization process is stopped.
 *                           This time is measured using `CGAL::Real_timer`. 0 means that there is no time limit.}
 *     \cgalParamType{`double`}
 *     \cgalParamPrecondition{`time_limit >= 0`}
 *     \cgalParamDefault{0}
 *   \cgalParamNEnd
 *   \cgalParamNBegin{max_iteration_number}
 *     \cgalParamDescription{limit on the number of performed iterations. 0 means that there is
 *                           no limit on the number of performed iterations.}
 *     \cgalParamPrecondition{`max_iteration_number >=0`}
 *     \cgalParamType{`int`}
 *     \cgalParamDefault{0}
 *   \cgalParamNEnd
 *   \cgalParamNBegin{freeze_bound}
 *     \cgalParamDescription{designed to reduce running time of each optimization iteration.
 *                           Any vertex that has a displacement less than a given fraction of the length
 *                           of its shortest incident edge, is frozen (i.e.\ is not relocated).
 *                           The parameter `freeze_bound` gives the threshold ratio.
 *                           If it is set to 0, freezing of vertices is disabled.}
 *     \cgalParamPrecondition{`0<= freeze_bound <=1`}
 *     \cgalParamType{`double`}
 *     \cgalParamDefault{0.01}
 *   \cgalParamNEnd
 *   \cgalParamNBegin{convergence}
 *     \cgalParamDescription{threshold ratio of stopping criterion based on convergence: the optimization process is stopped
 *                           when at the last iteration the displacement of any vertex is less than
 *                           a given fraction of the length of the shortest edge incident to that vertex.}
 *     \cgalParamPrecondition{`0 <=convergence <= 1`}
 *     \cgalParamType{`double`}
 *     \cgalParamDefault{0.02}
 *   \cgalParamNEnd
 *   \cgalParamNBegin{do_freeze}
 *     \cgalParamDescription{completes the `freeze_bound` parameter. If it is set to `true` (default value),
 *                           frozen vertices will not move anymore in next iterations. Otherwise, at each iteration, any vertex that
 *                           moves, unfreezes all its incident vertices.}
 *     \cgalParamType{`bool`}
 *     \cgalParamDefault{true}
 *   \cgalParamNEnd
 * \cgalNamedParamsEnd
 *
 * \cgalHeading{Example}
 *
 * \code{.cpp}
 * // Mesh generation with lloyd optimization step
 * C3t3 c3t3 = make_mesh_3<c3t3>(domain,
 *                               criteria,
 *                               parameters::lloyd());
 *
 * refine_mesh_3(c3t3,
 *               domain,
 *               criteria,
 *               parameters::lloyd(parameters::time_limit(10)));
 *
 * \endcode
 *
 * \sa `CGAL::parameters::no_lloyd()`
 * \sa `CGAL::lloyd_optimize_mesh_3()`
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::refine_mesh_3()`
 *
 */
template <class NamedParameters = parameters::Default_named_parameters>
unspecified_type lloyd(const Named_function_parameters& np = parameters::default_values());

/*!
 * \ingroup PkgMesh3Parameters
 *
 * The function `parameters::no_exude()` enables the user to tell the mesh generation functions
 * `make_mesh_3()` and `refine_mesh_3()` that no exudation must be done.
 *
 * \cgalHeading{Example}
 *
 * \code{.cpp}
 * // Mesh generation without exudation
 * C3t3 c3t3 = make_mesh_3<c3t3>(domain,
 *                               criteria,
 *                               parameters::no_exude());
 * \endcode
 *
 * \sa `CGAL::parameters::exude()`
 * \sa `CGAL::exude_mesh_3()`
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::refine_mesh_3()`
 *
 */
unspecified_type no_exude();

/*!
 * \ingroup PkgMesh3Parameters
 *
 * Provides an option indicating no special treatment should be done
 * with 0 and 1-dimensional features.
 *
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::refine_mesh_3()`
 * \sa `CGAL::parameters::features()`
 *
 */
unspecified_type no_features();

/*!
 * \ingroup PkgMesh3Parameters
 *
 * The function `parameters::surface_only()`
 * enables the meshing algorithm
 * to mesh the input surface only and not take the volume into account.
 *
 * When this option is enabled, the output mesh has no cells in the 3D complex,
 * only facets, edges and vertices.
 * Full-3D optimization steps such as mesh perturbation and mesh exudation are automatically disabled.
 *
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::refine_mesh_3()`
 */
unspecified_type surface_only();

/*!
 * \ingroup PkgMesh3Parameters
 *
 * The function `parameters::no_lloyd()` enables the user to tell the mesh generation functions
 * `make_mesh_3()` and `refine_mesh_3()` that no lloyd optimization must be done.
 *
 * \cgalHeading{Example}
 *
 * \code{.cpp}
 * // Mesh generation without lloyd optimization
 * C3t3 c3t3 = make_mesh_3<c3t3>(domain,
 *                               criteria,
 *                               parameters::no_lloyd());
 * \endcode
 *
 * \sa `CGAL::parameters::lloyd()`
 * \sa `CGAL::lloyd_optimize_mesh_3()`
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::refine_mesh_3()`
 *
 */
unspecified_type no_lloyd();

/*!
 * \ingroup PkgMesh3Parameters
 *
 * The function `parameters::no_odt()` enables the user to tell the mesh generation functions
 * `make_mesh_3()` and `refine_mesh_3()` that no ODT optimization must be done.
 *
 * \cgalHeading{Example}
 *
 * \code{.cpp}
 * // Mesh generation without odt optimization
 * C3t3 c3t3 = make_mesh_3<c3t3>(domain,
 *                               criteria,
 *                               parameters::no_odt());
 * \endcode
 *
 * \sa `CGAL::parameters::odt()`
 * \sa `CGAL::odt_optimize_mesh_3()`
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::refine_mesh_3()`
 *
 */
unspecified_type no_odt();

/*!
 * \ingroup PkgMesh3Parameters
 *
 * The function `parameters::no_perturb()` enables the user to tell mesh generation global functions
 * `make_mesh_3()` and `refine_mesh_3()` that no perturbation must be done.
 *
 * \cgalHeading{Example}
 *
 * \code{.cpp}
 * // Mesh generation without perturbation
 * C3t3 c3t3 = make_mesh_3<c3t3>(domain,
 *                               criteria,
 *                               parameters::no_perturb());
 * \endcode
 *
 * \sa `CGAL::parameters::perturb()`
 * \sa `CGAL::perturb_mesh_3()`
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::refine_mesh_3()`
 *
 */
unspecified_type no_perturb();

/*!
 * \ingroup PkgMesh3Parameters
 *
 * The function `parameters::odt()` enables the user to trigger a call to
 * `CGAL::odt_optimize_mesh_3()` in
 * `CGAL::make_mesh_3()` and `CGAL::refine_mesh_3()` mesh optimization functions. It also
 * enables the user to pass parameters to the optimization function
 * `odt_optimize_mesh_3()` through these mesh generation functions.
 *
 * @tparam NamedParameters a sequence of \ref bgl_namedparameters "Named Parameters"
 *
 * @param np an optional sequence of \ref bgl_namedparameters "Named Parameters" among the ones listed below:
 *
 * \cgalNamedParamsBegin
 *   \cgalParamNBegin{time_limit}
 *     \cgalParamDescription{is used to set up, in seconds,
 *                          a CPU time limit after which the optimization process is stopped. This time is
 *                          measured using `Real_timer`.
 *                          The default value is 0 and means that there is no time limit.}
 *     \cgalParamType{`double`}
 *     \cgalParamPrecondition{`time_limit >= 0`}
 *     \cgalParamDefault{0}
 *   \cgalParamNEnd
 *   \cgalParamNBegin{max_iteration_number}
 *     \cgalParamDescription{sets a limit on the number of performed iterations.
 *                          The default value of 0 means that there is
 *                          no limit on the number of performed iterations.}
 *     \cgalParamType{`std::size_t`}
 *     \cgalParamDefault{0}
 *   \cgalParamNEnd
 *   \cgalParamNBegin{convergence}
 *     \cgalParamDescription{is a stopping criterion based on convergence:
 *                          the optimization process is stopped, when at the last iteration,
 *                          the displacement of any vertex is less than a given percentage of the length
 *                          the shortest edge incident to that vertex.
 *                          The parameter `convergence` gives the threshold ratio.}
 *     \cgalParamType{`double`}
 *     \cgalParamPrecondition{`0 <= convergence <= 1`}
 *     \cgalParamDefault{0.02}
 *   \cgalParamNEnd
 *   \cgalParamNBegin{freeze_bound}
 *     \cgalParamDescription{is designed to reduce running time of each optimization iteration. Any vertex
 *                          that has a displacement less than a given percentage of the length of its shortest incident edge, is frozen (i.e.\ is
 *                          not relocated). The parameter `freeze_bound` gives the threshold ratio.}
 *     \cgalParamType{`double`}
 *     \cgalParamPrecondition{`0 <= freeze_bound <= 1`}
 *     \cgalParamDefault{0.01}
 *   \cgalParamNEnd
 *   \cgalParamNBegin{do_freeze}
 *     \cgalParamDescription{completes the `freeze_bound` parameter. If it is set to `true`,
 *                          frozen vertices will not move anymore in next iterations. Otherwise, at each iteration, any vertex that
 *                          moves, unfreezes all its incident vertices.}
 *     \cgalParamType{`bool`}
 *     \cgalParamDefault{true}
 *   \cgalParamNEnd
 * \cgalNamedParamsEnd
 *
 * \cgalHeading{Example}
 *
 * \code{.cpp}
 * // Mesh generation with odt optimization step
 * C3t3 c3t3 = make_mesh_3<c3t3>(domain,
 *                               criteria,
 *                               parameters::odt());
 *
 * refine_mesh_3(c3t3,
 *               domain,
 *               criteria,
 *               parameters::odt(parameters::time_limit(10)));
 * \endcode
 *
 * \sa `CGAL::parameters::no_odt()`
 * \sa `CGAL::odt_optimize_mesh_3()`
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::refine_mesh_3()`
 *
 */
template <class NamedParameters = parameters::Default_named_parameters>
unspecified_type odt(const Named_function_parameters& np = parameters::default_values());

/*!
 * \ingroup PkgMesh3Parameters
 *
 * The function `parameters::perturb()` enables the user to trigger a call to
 * `perturb_mesh_3()` in
 * `make_mesh_3()` and `refine_mesh_3()` mesh generation functions. It also
 * enables the user to pass parameters
 * to the optimization function `perturb_mesh_3()` through these mesh generation functions.
 *
 * \tparam NamedParameters a sequence of \ref bgl_namedparameters "Named Parameters"
 *
 *  @param np an optional sequence of \ref bgl_namedparameters "Named Parameters" among the ones listed below:
 *
 * \cgalNamedParamsBegin
 *   \cgalParamNBegin{time_limit}
 *     \cgalParamDescription{is used to set up, in seconds, a CPU time limit after which the optimization process
 *                           is stopped. This time is measured using the `Real_timer` class. The default value is
 *                           0 and means that there is no time limit.}
 *     \cgalParamType{`double`}
 *     \cgalParamPrecondition{`time_limit >= 0`}
 *   \cgalParamDefault{0}
 *   \cgalParamNEnd
 *   \cgalParamNBegin{sliver_bound}
 *     \cgalParamDescription{is designed to give, in degrees, a targeted lower bound on dihedral angles of mesh cells.
 *                          The exudation process considers in turn all the mesh cells that have a smallest dihedral
 *                          angle less than sliver_bound and tries to make them disappear by weighting their vertices.
 *                          The optimization process stops when every cell in the mesh achieves this quality. The
 *                          default value is 0 and means that there is no targeted bound: the exuder then runs as long
 *                          as it can improve the smallest dihedral angles of the set of cells incident to some vertices.}
 *     \cgalParamType{`double`}
 *     \cgalParamPrecondition{`0 <= sliver_bound <= 180`}
 *     \cgalParamDefault{0}
 *   \cgalParamNEnd
 * \cgalNamedParamsEnd
 *
 * \cgalHeading{Example}
 *
 * \code{.cpp}
 * // Mesh generation with a perturbation step
 * C3t3 c3t3 = make_mesh_3<c3t3>(domain,
 *                               criteria,
 *                               parameters::perturb());
 *
 * refine_mesh_3(c3t3,
 *               domain,
 *               criteria,
 *               parameters::perturb(parameters::time_limit(10)));
 *
 * \endcode
 *
 * \sa `CGAL::parameters::no_perturb()`
 * \sa `CGAL::perturb_mesh_3()`
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::refine_mesh_3()`
 *
 */
template <class NamedParameters = parameters::Default_named_parameters>
unspecified_type perturb(const Named_function_parameters& np = parameters::default_values());

/*!
 * \ingroup PkgMesh3Parameters
 *
 * The function `parameters::initial_points_generator()` enables the user to specify a functor that follows
 * the `InitialPointsGenerator_3` concept to the mesh generation function `make_mesh_3()`. This functor is called
 * for the initialization of the meshing process, by inserting well-distributed surface vertices.
 * If this parameter is not specified, the default behavior
 * is executed, which calls the domain's `construct_initial_points_object()` for the initialization of the meshing process.
 *
 * Initialization is considered to be complete if the triangulation is a 3D triangulation
 * with at least one facet in the restricted Delaunay triangulation (i.e., its dual intersects the
 * input surface).
 *
 * If one dimensional features are requested, their initialization is performed first.
 * If provided, the points of `parameters::initial_points()` are inserted next.
 * Then, `parameters::initial_points_generator()` is used to generate more initial points
 * and complete the initialization.
 * If the generator does not generate enough points for the initialization to be complete,
 * the domain's `construct_initial_points_object()` will be called to generate enough input points.
 *
 * \tparam InitialPointsGenerator a model of the `InitialPointsGenerator_3` concept
 *
 * @param generator an instance of `InitialPointsGenerator`
 *
 * \cgalHeading{Example}
 *
 * \snippet mesh_3D_image_with_image_initialization.cpp Meshing
 *
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::parameters::initial_points()`
 * \sa `MeshDomain_3::Construct_initial_points`
 *
 */
template <typename InitialPointsGenerator>
unspecified_type initial_points_generator(const InitialPointsGenerator& generator);

/*!
 * \ingroup PkgMesh3Parameters
 *
 * The function `parameters::initial_points()` enables the user to specify a container, model of `Range`, that contains
 * initial points constructed on surfaces,
 * to be used in the `make_mesh_3()` function for mesh generation. Items in the container are
 * tuple-like objects containing a `Weighted_point_3`, an `int`, and a `MeshDomain::Index`,
 * where `Weighted_point_3` represents the position and the weight of the point,
 * `int` the dimension of the minimal subcomplex on which the point lies,
 * and `MeshDomain::Index` the corresponding subcomplex index.
 * These initial points are inserted after one dimensional features initialization.
 *
 * Initialization is considered to be complete if the triangulation is a 3D triangulation
 * with at least one facet in the restricted Delaunay triangulation (i.e., its dual intersects the
 * input surface).
 *
 * If the parameter `parameters::initial_points_generator()` is set,
 * the points from this parameter will be inserted before calling the initial points generator.
 *
 * If after the insertion of initial points (possibly together with the input generator),
 * the initialization is not complete,
 * the domain's `construct_initial_points_object()` will be called.
 *
 * \tparam MeshDomain a model of `MeshDomain_3`
 * \tparam C3t3 a model of `MeshComplex_3InTriangulation_3`
 * \tparam InitialPointsRange a model of `Range` containing tuple-like objects of
 *    `C3t3::Triangulation::Geom_traits::Weighted_point_3, int, MeshDomain::Index`.
 *
 * @param initial_points an instance of `InitialPointsRange`
 *
 * \cgalHeading{Example}
 *
 * \code{.cpp}
 * // Create the initial_points vector
 * std::vector<std::tuple<K::Weighted_point_3, int, Mesh_domain::Index>> initial_points;
 * // Perform mesh generation from a labeled image with initial points
 * C3t3 c3t3 = make_mesh_3<c3t3>(domain,
 *                               criteria,
 *                               parameters::initial_points(std::cref(initial_points))); // Use std::cref to avoid a copy
 * \endcode
 *
 * \sa `CGAL::make_mesh_3()`
 * \sa `CGAL::parameters::initial_points_generator()`
 * \sa `MeshDomain_3::Construct_initial_points`
 *
 */
template <typename MeshDomain, typename C3t3, typename InitialPointsRange>
unspecified_type initial_points(const InitialPointsRange& initial_points);
} /* namespace parameters */

} /* namespace CGAL */
