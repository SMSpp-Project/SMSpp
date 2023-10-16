/*--------------------------------------------------------------------------*/
/*-------------------------- File C05Function.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the C05Function class, which implements a Function that is
 * able to provide *linearizations*, i.e., first-order information. However,
 * the linearizations need not be a continuous function, which means that
 * the Function may be non-smooth.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __C05Function
#define __C05Function /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Function.h"

#include <Eigen/Dense>
#include <Eigen/Sparse>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/
// namespace for the Structured Modeling System++ (SMS++)

namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup C05Function_CLASSES Classes in C05Function.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS C05Function ------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// base class of Function that can provide linearizations
/** The C05Function class is a specialization of a general Function class
 * which implements a Function that is able to provide *linearizations*,
 * i.e., first-order information. However, the linearizations need not be a
 * continuous function, which means that the Function may be non-smooth.
 *
 * A "linearization" of the Function is a linear function in the same
 * Variable as the Function that can be computed at each point and provides
 * information about the behaviour of the Function in the neighbourhood of
 * that point. It is therefore necessary that the interface specifies
 *
 *     THE WAY IN WHICH THE LINEAR FUNCTION IS DESCRIBED
 *
 * However, this in general
 *
 *     DEPENDS ON EXACTLY WHAT THE Variable OF THE FUNCTION ARE
 *
 * For instance, let "n" be the number of active Variable of the Function
 * (in all comments of the class unless otherwise specified). If each
 * Variable were a k x k real matrix, the linear function would have to
 * specify how to (linearly) turn it into a *single* real number, which
 * would typically require another k x k real matrix. This might be
 * dealt with by having some general concept of "coefficient of the
 * linearization", but here we take the route that is by far the more common:
 *
 *     THE Variable ARE ASSUMED TO BE SINGLE REAL VALUES (say, ColVariable)
 *
 * This immediately implies that
 *
 *     A LINEARIZATION CAN BE DEFINED BY A PAIR FORMED BY A REAL n-VECTOR
 *     (g in the comments) AND A SINGLE REAL SCALAR (\alpha in the comments),
 *     AND THEREFORE IS A LINEAR (AFFINE) FUNCTION L(x) = g x + \alpha
 *
 * Linearization are therefore objects in the graphical space \R^{n + 1}
 * of pairs ( x , v ), where x belongs to the input space (which is assumed
 * to be \R^n, i.e., each Variable holds a single real value) and v \in R is
 * the function value; indeed, Gr(f) = \{ ( x , f(x) ) : x \in \R^n \} (the
 * graph of f) is an object in \R^{n + 1}. However, like function values
 * linearizations are usually computed at specific points x \in \R^n. Often,
 * but not always, the computation of linearizations is a (more or less
 * cheap) by-product of the evaluation of the Function. A convenient example
 * of this behaviour is that of a structured optimization problem
 *
 *   (P)  max \{ c u : A u = b , u \in U \}
 *
 * and of its (convex) Lagrangian function
 *
 *   (P( x ))  f( x ) = max \{ c u + x ( b - A u ) : u \in U \}
 *
 * Given some \bar{x}, any optimal solution u^* of P( \bar{x} ) provides
 * the pair
 *
 *    ( g , \alpha ) = ( b - A u^* , c u^* )
 *
 * which defines the "best possible" linearization
 *
 *    L( x ) = \alpha + g x = c u^* + x ( b - A u^* )
 *
 * Indeed:
 *
 * - L() is "active" at \bar{x}: \L( \bar{x} ) = f( \bar{x} );
 *
 * - it supports Gr(f) from below, i.e.,
 *
 *     f( y ) >= \f( \bar{x} ) + g ( y - \bar{x} ) for all y  ,
 *
 *   as it can be seen by just expanding:
 *
 *     f( y ) \geq c u^* + \bar{x} ( b - A u^* )
 *                       + ( y - \bar{x} ) ( b - A u^* ) =
 *               = c u^* + y ( b - A u^* )
 *
 *   (the inequality being obviously true since u^* is a feasible, but
 *   not necessarily optimal, solution of P( y ));
 *
 * - if u^* is *unique*, then f is differentiable at x and g = \Nabla f(x).
 *
 * So, obtaining u^*, which is crucial for computing f(x), gives for free as
 * a by-product the best possible linearization. However:
 *
 * - a linearization may be a "global" object, not necessarily tied to a
 *   specific point x: in the Lagrangian case, the linearization supports
 *   Gr(f) everywhere, i.e., at points y arbitrarily far from x;
 *
 * - conversely, multiple linearizations may be produced at some x. For
 *   instance, even if u^* is unique, any \eps-optimal solution u' of P(x)
 *   produces a linearization ( g', \alpha' ) = L( u' ) which is an
 *   \eps-subgradient of f, i.e., \alpha' \geq f(x) - \eps (by the very
 *   definition of \eps-optimal solution) and f(y) >= \alpha' + g' ( y - x )
 *   for all y.
 *
 * Thus, one linearization may in fact "refer to multiple points", and,
 * conversely, "a single point may provide multiple linearizations". This
 * class has to cater for all these mechanics, which is done via two distinct
 * "pools" of linearizations, the "local" and the "global" one.
 *
 * The local pool is directly tied to the last point x where compute() has
 * been called, and it is automatically cleared as soon as compute() is
 * called again (on a different x). The idea is therefore that the local
 * pool is to be populated with linearizations "significant at x". The
 * specific concept is left intentionally vague, but examples are:
 *
 * - for a convex function, the \eps-subdifferential, i.e., the set of
 *   \eps-subgradients at x (see above);
 *
 * - for a concave function, the set of \eps-supergradients [the same thing
 *   with properly inverted signs];
 *
 * - for a reasonably regular function (say, continuous and differentiable
 *   almost everywhere), either the Goldstein or the Clarke
 *   \eps-subdifferential, which are something like the closed convex hull
 *   of the set of vectors obtained as limits of \Nabla f(x_i) as x_i goes
 *   to x, or to any point in a ball of radius \eps around x.
 *
 * Note that whenever useful, as in this case, with a little abuse of
 * notation we refer to only the g part of the linearization as "the
 * linearization". This, for instance, allows to say that, if the Function
 * is differentiable at x and \eps = 0, all the three examples collapse to
 * \Nabla f(x) as the only possible linearization.
 *
 * We purposely refrain from giving a more formal definition as the above
 * examples are only meant to convey the general idea that there is a set of
 * "linearizations relevant up to a number \eps", which is what the interface
 * supports. Specific algorithms will have specific requirements on the
 * Function (say, convex) which will typically imply requirements on the set
 * in question (say, is the \eps-subdifferential), but since there are
 * several possible variants this is left for further specifications: the
 * C05Function aims at capturing all these cases.
 *
 * Note that all the sets mentioned in the examples are *convex*. Accordingly,
 * the C05Function interface explicitly supports the notion that convex
 * combinations of linearizations are important. It should be apparent why
 * this is necessary: proving (approximate) optimality/stationariety of a
 * point x^* (even in the simple case where there are no constraints or other
 * components in the objective function) amounts at proving that 0 \in \R^n
 * belongs to the corresponding set of linearizations (\eps-subdifferential).
 * In general, even if called at such a x^* the C05Function cannot be expected
 * to produce the 0 linearization; this is why proving (approximate)
 * optimality/stationariety of x^* typically boils down to producing a set
 * of linearizations g_i at points x_i "close" to x^* and then proving that
 * 0 \in conv( { g_i } ). Thus, the interface has to cater for:
 *
 * - collecting and storing in a "long-term memory" (the global pool, as
 *   opposed to the short-term memory of the local pool) linearizations
 *   that have been produced at different points x_i;
 *
 * - making convex combinations of linearizations of the global pool.
 *
 * Note that
 *
 *     THE CONTENTS OF THE GLOBAL POOL ARE, IN A SENSE, THE (PARTIAL)
 *     "ABSTRACT REPRESENTATION" OF THE C05Function
 *
 * As such, they may change if the C05Function does, which the Modification
 * have to properly signal. On the other hand
 *
 *     CHANGING THE GLOBAL POOL AMOUNTS AT CHANGING THE C05Function
 *
 * in the same sense as adding a dynamic Variable/Constraint amount at
 * changing a Block: even if the mathematical entity reprsented by the
 * C05Function / Block has remained the same, its available (partial)
 * representation is different. Hence, Modification have to properly signal
 * changes in the global pool.
 *
 * In particular, there is usually "one important convex combination" that
 * is very relevant for algorithmic purposes. This can be clearly seen in
 * the Lagrangian function example: for a point x^* to be \eps-optimal for
 * the minimization of f (the Lagrangian Dual), one must collect a set of
 * \eps-subgradients g_i such that 0 \in conv( { g_i } ). It is immediate to
 * realize that this corresponds to a set of \eps-optimal solutions u_i to
 * P(x^*) such that u^* = \sum_i u_i \theta_i, for appropriate convex
 * multipliers \theta_i, is such that A u^* = b. If U in (P) is a convex
 * set then such an u^* is an optimal solution to (P), otherwise u^* is the
 * optimal solution of the relaxation of (P) substituting U with conv( U ).
 * In all cases, u^* can be a relevant object to construct. For instance, u^*
 * can be used to to separation of constraints (in case the A u = b ones are
 * very many, so that an active-set strategy is necessary), or to guide
 * heuristics or branching operations (if the set U included integrality
 * constraints, so that the Lagrangian Dual of (P) is only a relaxation).
 *
 * More in general, proving optimality/stationariety of some x^* involves
 * constructing one convex combination of linearizations with appropriate
 * properties.  This is why C05Function has the concept of "important
 * linearization": once an optimization involving the function has
 * terminated, the Solver can store (information about how to compute) the
 * "important" linearization in the global pool for future use. For instance,
 * in case the Function (or other parts of the Block) changes, the changes
 * may be such that x^* may nonetheless remain an optimal solution, and the
 * availability of the "important linearization" may allow to prove this with
 * very little computational effort. In other words,
 *
 *    THE "IMPORTANT LINEARIZATION" TYPICALLY CORRESPONDS TO THE
 *    OPTIMAL DUAL SOLUTION
 *
 * of a optimization problem involving the C05Function.
 *
 * Actually, depending on how the dual problem is written, one may be
 * interested in a "different representation" of the "important
 * linearization": not the linearization per se, but rather the set of
 * (convex) multipliers used to construct it out of other linearizations
 * from the global pool. In the Lagrangian case, for instance, the latter
 * correspond to (say) unfeasible integer solutions. These can be used e.g.
 * in Lagrangian heuristics to construct a (say) feasible integer solution,
 * where the weights can be used as "probability that some feature of the
 * solution is conserved in the optimal one". This is why the
 * "important linearization" is not by default constructed in global pool;
 * rather, the multipliers producing it are stored. Those multipliers can be
 * considered (a part of) the optimal dual solution of the optimization
 * problem involving the C05Function. Of course, one possible use for this
 * information is to call store_combination_of_linearizations() to
 * "physically" construct the "important linearization" and have it stored in
 * the global pool. In this case, of course, \p coefficients should just be
 * < name of the important linearization , 1 >. Indeed, there may be no need 
 * to "physically" construct the "important linearization" since it could
 * well be one of those directly produced by the C05Function; think to the
 * case where f is smooth at x^*. Alternatively, the algorithm doing the
 * optimization may find it algorithmically expedient to produce it anyway.
 * However, in principle there is no need for the "important linearization"
 * to be physically constructed: once the information of \p coefficients is
 * stored, the "important linearization" is "virtually" present in the global
 * pool (as it is only a store_combination_of_linearizations() call away from
 * being there).
 *
 * It is important to remark that, differently from the global pool
 *
 *     "CONSUMING" LINEARIZATIONS FROM THE LOCAL POOL DOES NOT AMOUNT AT
 *     CHANGING THE C05Function, AND THEREFORE NO Modification IS ISSUED
 *
 * The rationale here is that linearizations in the local pool are dynmically
 * generated when the C05Function is computed, and they are meant to "be lost
 * once seen" unless they are explicitly saved in the global pool. This is the
 * same usage pattern as solutions of a Block generated by a Solver: if the
 * Solver is required to store a new solution in the Block, then whatever is
 * there already is lost for good. That is, users cannot rely on solutions to
 * remain there (unless they properly lock the Block), and have to save them
 * themselves if they need to be kept. The same happens with linearizations in
 * the local pool, and in fact these may well be associated to solutions
 * computed by a Solver, as in the Lagrangian example. That is, the local
 * pool of linearizations may well directly correspond to the pool of
 * solutions in a Solver.
 *
 * A final important detail is that there can be two types of linearizations:
 * "diagonal" and "vertical". Diagonal linearizations are the previously
 * illustrated ones; the name comes from picturing the ( x , v ) space
 * \R^{n+1} as having the x component on the horizontal axis and the v
 * component as the vertical one (the typical arrangement for the graphical
 * space of a function). Then, a linearization ( g , \alpha ) typically
 * corresponds to a line approximating f in the neighbourhood of some x. If,
 * say, f is convex, this corresponds to a linear constraint
 *
 *       ( 1 , - g ) ( v , x ) >= \alpha       (*)
 *
 * which is globally valid for the graph of f (actually, the epigraph).
 * The corresponding line in the ( x , v ) space either intersects the x axis
 * "diagonally" or is parallel to it (if g = 0), but it can never be
 * orthogonal to it. Thus, there is an entirely different form of lines in
 * the graphical space, those corresponding of linear constraints of the form
 *
 *       ( 0 , - g ) ( v , x ) >= \alpha       (**)
 *
 * These may still be valid for the (epi)graph of f. This is in particular
 * the case for convex functions evaluated outside of their domain, i.e., at
 * points y s.t. f(y) = +\infty. In some cases, it is possible for the
 * function to compute a "vertical" linearization (**) which is valid for
 * all ( x , v ) in the (epi)graph of f but that "cuts off" y, i.e.,
 * such that 0 < g y + alpha. This is true for instance in the Lagrangian
 * case P(x) where U is an unbounded set, say a convex one: then, P(x) is
 * unbounded below if there is some v in the recession cone of U such that
 * ( c + x A ) v > 0. This immediately implies that f(y) = +\inf for all y
 * with the same property, i.e., that the constraint c v + x ( A v ) <= 0
 * is valid for the (epi)graph of f. Since v is typically constructed by
 * whatever algorithm is used to solve P(x) in order to prove that it is
 * unbounded above, the corresponding vertical linearization of the form (**)
 * can be returned by the C05Function. Note that, customarily, a solution
 * algorithm would also compute a feasible solution u \in U together with v,
 * and therefore can return both a diagonal and a vertical linearization,
 * which the interface allows. In the parlance of Benders' decomposition,
 * diagonal linearizations are "optimality cuts" and vertical linearizations
 * are "feasibility cuts". However, the concept is possibly general.
 *
 * Note that the signs of the inequalities are tied to the convexity of the
 * function; that is, for a concave function (*) becomes
 *
 *       ( 1 , - g ) ( v , x ) <= \alpha
 *
 * Hence, although not strictly necessary, it makes sense to assume that
 * (**) analogously becomes
 *
 *       ( 0 , - g ) ( v , x ) <= \alpha
 *
 * (in other words, both g and \alpha are multipled by -1, which is precisely
 * what happens if the concave function f() is changed in sign to become the
 * convex -f()). For neither convex nor concave functions neither version of
 * (*) is a valid inequality for the epi/ipograph, hence the discussion does
 * not make sense.
 *
 * Note that, while typically combinations of "diagonal" linearizations need
 * be convex ones, combination of "vertical" linearizations can usually be
 * performed with arbitrary non-negative weights. In the Lagrangian case,
 * "diagonal" linearizations corresponds to points of the feasible region,
 * while "vertical" ones correspond to directions. Thus, all previous
 * comments about *convex* combination of linearizations, and in particular
 * about the "important" linearization, need in general to be understood as
 * convex combinations of "diagonal" linearizations plus conical combinations
 * of "vertical" linearizations. */

class C05Function : public Function {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */

 /// type used to store a sparse vector
 using SparseVector = Eigen::SparseVector< FunctionValue >;

 /// type used to define linear combinations of linearizations
 using LinearCombination = std::vector< std::pair< Index , FunctionValue > >;

 /// a const LinearCombination
 using c_LinearCombination = const LinearCombination;

/*--------------------------------------------------------------------------*/
 /// public enum for the int algorithmic parameters of C05Function
 /** Public enum describing the different parameters of "int" type that a
  * C05Function must have (although specific Function may choose to ignore
  * some of them). The value intLastParC05F is provided so that the list can
  * be easily further extended by derived classes. */

 enum int_par_type_C05F {
  intLPMaxSz = Function::intLastParFun ,  ///< max size of the "local pool"
  /**< The algorithmic parameter for setting the size of the "local pool",
   * that is, the maximum number of linearizations that should be stored in
   * the local pool. The default is 1, which corresponds to the fact that the
   * Function can only produce a single linearization at a time (for it is,
   * say, smooth). */

  intGPMaxSz,  ///< maximum size of the "global pool"
               /**< The algorithmic parameter for setting the size of the 
                * "global pool", that is, the maximum number of
   * linearizations that should be stored in the local pool. The default is 0,
   * which corresponds to the fact that the Function cannot store any
   * linearization (for it is, say, smooth and therefore there is no need to).
   */

  intLastParC05F  ///< first allowed new int parameter for derived classes
                  /**< Convenience value for easily allow derived classes
                   * to extend the set of int algorithmic parameters. */
  };  // end( int_par_type_C05F )

/*--------------------------------------------------------------------------*/
 /// public enum for the double algorithmic parameters of C05Function
 /** Public enum describing the different parameters of "double" type that a
  * C05Function must have (although specific C05Function may choose to ignore
  * some of them). The value dblLastParC05F is provided so that the list can
  * be easily further extended by derived classes. */

 enum dbl_par_type_C05F {
  dblRAccLin = Function::dblLastParFun ,
  ///< maximum relative error in any linearization
  /**< The parameter for setting the relative accuracy of the linearizations.
   * A linearization ( g , \alpha ) computed at the point x is "accurate" if
   * the value of the linearization coincides with the value of the function
   * at x, i.e., g x + \alpha = f(x). In general linearizations that are not
   * "completely accurate" can still be useful: for instance, in the
   * Lagrangian case an \eps-optimal solution to the Lagrangian problem gives
   * rise to a valid linearization ( g , \alpha ) with \eps >=
   * f( x ) - ( g x + \alpha ). Indeed, by convexity f( x ) >= g x + \alpha,
   * and the term
   *
   *    f( x ) - ( g x + \alpha ) >= 0
   *
   * is called the "linearization error" of ( g , \alpha ) at x. Whenever the
   * linearization error is <= \eps the linearization is an \eps-subgradient
   * of f at x, and \eps-optimal solutions of the Lagrangian problem are those
   * which characterise \eps-subgradient. Such a linearization can be deemed
   * "interesting" if \eps is "small", but not if \eps is "large". This
   * parameter instructs the C05Function not to bother reporting (and
   * therefore storing in the "local pool") any linearization having a
   * relative error with f(x) larger than dblRAccLin. This would generally
   * mean
   *
   *  | f( x ) - ( g x + \alpha )  | <= dblRAccLin * max( | f( x ) | , 1 )
   *
   * except that the value f( x ) may not be known exactly, with only lower
   * and/or upper bounds on it available. The actual formula therefore depends
   * on what information is actually available: for instance, in the
   * Lagrangian case one knows that f( x ) >= g x + \alpha, and therefore
   * typically an upper estimate ub >= f( x ) is used in the formula instead
   * of f( x ). The default is 0, i.e., "only perfect linearizations are
   * allowed". */

  dblAAccLin ,   ///< maximum absolute error in any reported solution
  /**< Similar to dblRAccLin but for an *absolute* accuracy; that is, a
   * linearization is deemed acceptable if
   *
   *      | f( x ) - ( g x + \alpha ) | <= dblAAccLin
   *
   * except that the value f( x ) may not be known exactly, with only lower
   * and/or upper bounds on it available. The actual formula therefore depends
   * on what information is actually available: for instance, in the
   * Lagrangian case one knows that f( x ) >= ( g x + \alpha ), and therefore
   * typically an upper estimate ub >= f( x ) is used in the formula instead
   * of f( x ). The default is 0, i.e., "only perfect linearizations are
   * allowed". */

  dblAAccMlt ,   ///< maximum absolute error in the multipliers
  /**< The multipliers used in store_combination_of_linearizations() and in
   * set_important_linearization() are typically (a part of) the dual optimal
   * solution of the optimization problem involving the C05Function. As such
   * they typically have some constraint. Almost always they are bounded to be
   * non-negative, and typically their sum has to be 1 (convex multipliers) or
   * <= 1.
   *
   * This parameter controls the tolerance in these constraints. Since the
   * unitary simplex is "well scaled" by default, the tolerance is absolute.
   * While the specific form of the constraints is :C05Function-dependent, for
   * the standard case of the unitary simplex the meaning should be:
   *
   * - each multiplier has to be >= - dblAAccMlt
   *
   * - abs( 1 - sum of multipliers ) <= dblAAccMlt * ( number of multipliers )
   *
   * The default is "small but not zero": 1e-10. */

  dblLastParC05F  ///< first allowed new double parameter for derived classes
                  /**< Convenience value for easily allow derived classes
                   * to extend the set of double algorithmic parameters. */
 };  // end( dbl_par_type_C05F )

/*--------------------------------------------------------------------------*/
 /// public enum "extending" str_par_type_F to the case of C05Function

 enum str_par_type_C05F {
  strLastParC05F = strLastParFun
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of string parameters. */
  };  // end( str_par_type_C05F )

/*--------------------------------------------------------------------------*/
 /// public enum "extending" vint_par_type_F to the case of C05Function

 enum vint_par_type_C05F {
  vintLastParC05F = vintLastParFun
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-int parameters. */
  };  // end( vint_par_type_C05F )

/*--------------------------------------------------------------------------*/
 /// public enum "extending" vdbl_par_type_F to the case of C05Function

 enum vdbl_par_type_C05F {
  vdblLastParC05F = vdblLastParFun
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-double parameters. */
  };  // end( vdbl_par_type_C05F )

/*--------------------------------------------------------------------------*/
 /// public enum "extending" vstr_par_type_F to the case of C05Function

 enum vstr_par_type_C05F {
  vstrLastParC05F = vstrLastParFun
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-string parameters. */
  };  // end( vstr_par_type_C05F )

/** @} ---------------------------------------------------------------------*/
/*--------------- CONSTRUCTING AND DESTRUCTING C05Function -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing C05Function
 *  @{ */

 /// constructor of C05Function: does nothing except calling that of Function
 /** Constructor of C05Function. Takes as input an optional pointer to an
  * Observer and passes it to the constructor of Function. */

 explicit C05Function( Observer * observer = nullptr )
  : Function( observer ) {}

/*--------------------------------------------------------------------------*/
 /// destructor: it is virtual, and empty

 ~C05Function() override = default;

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set a given integer (int) numerical parameter
 /** Set a given integer (int) numerical parameter. Actually, the base class
  * does not allow to change the default value to its parameters, which is
  * good for smooth functions, so that while implementing a smooth function
  * nothing needs to be done, not even checking that the parameters (which
  * the base class does not even store) are changed. */

 void set_par( idx_type par , int value ) override {
  switch( par ) {
   case( intLPMaxSz ):
    if( value != 1 )
     throw( std::invalid_argument( "intLPMaxSz cannot be changed" ) );
    break;
   case( intGPMaxSz ):
    if( value != 0 )
     throw( std::invalid_argument( "intGPMaxSz cannot be changed" ) );
    break;
   default: Function::set_par( par, value );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set a given float (double) numerical parameter
 /** Set a given float (double) numerical parameter. Actually, the base
  * class just ignores the setting of these parameters, since the default
  * value of 0 is good for smooth functions giving just the exact gradient;
  * yet, setting any (non-negative) number allows these functions to just
  * keep doing the same, so whatever number is set can just be ignored. */

 void set_par( idx_type par , double value ) override {
  switch( par ) {
   case( dblRAccLin ):
   case( dblAAccLin ):
   case( dblAAccMlt ):
    break;
   default: Function::set_par( par, value );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 using Function::set_par;

/** @} ---------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF A C05Function -------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a C05Function
 * The methods in this section allow to retrieve first-order information
 * about the point where the C05Function have been last evaluated by calling
 * compute(). Calls to to any of these methods are therefore associated with
 * that point: it is expected that the point (i.e., the values of the active
 * Variable) at the moment in which these methods are called is the same as
 * the one in which compute() was called. In other words, these methods are 
 * "extensions" of compute(), used to extract further information (likely)
 * computed in there, and therefore in principle an extension to the
 * fundamental rule regarding compute() is to be enforced:
 *
 *   between a call to compute() and all the calls to these methods intended
 *   to retrieve information about linearizations computed in that point, no
 *   changes must occur to the Function which change the answer that
 *   compute() was supposed to compute
 *
 * The rule leaves scope for some changes occurring, although these must
 * ensure that the answer is not affected; see comments to compute(). As
 * always,
 *
 *               IT IS UNIQUELY THE CALLER'S RESPONSIBILITY
 *                 TO ENSURE THAT THE RULE IS RESPECTED
 * @{ */

 /// tells whether a linearization is available
 /** Called after compute() this method has to return true if a linearization
  * of the given type (a diagonal one if diagonal == true, a vertical one if
  * diagonal == false) is available to be read with
  * get_linearization_coefficients() and get_linearization_constant(). It is
  * often the case that a linearization is "automatically" available as a
  * by-product of compute(); hence, this method is typically (but not
  * necessarily) true after the end of compute().
  *
  * In particular, one quite specific (but still relevant) case in which a
  * perfectly well-behaved C05Function, say a finite-valued differentiable
  * one producing exactly one gradient per compute() call, may refuse to
  * produce a diagonal linearization is that of a convex [concave] function
  * that "knows" its global minimum [maximum] value and exposes it via
  * get_global_lower_bound() [get_global_upper_bound()]. Such a C05Function
  * may reasonably have has_linearization() return false if compute() is
  * evaluated at a point (approximately, considering the setting of
  * dblRelAcc and dblRelAcc) yielding the minimum [maximum] value. Indeed,
  * at such a point the "reasonable" linearization would be the all-0,
  * "horizontal" [sub]gradient; this actually does not provide any more
  * information than that provided by get_global_lower_bound() [...], i.e.,
  * that the point is a global minimum [...]. Note that the C05Function may
  * well rather decide to explicitly return the "horizontal" gradient, as
  * well as produce non-horizontal approximate sub[super]gradients in the
  * point, considering the setting of dblRAccLin and dblAAccLin.
  *
  * In fact, once "the first" linearization (if ever) has been read, new
  * ones may be produced, if the C05Function does it, by means of
  * compute_new_linearization().
  *
  * The default implementation in the base class returns the same value as
  * diagonal, i.e., true if diagonal == true and false otherwise. This is
  * OK, for instance, for a, say, algebraic Function that is finite and
  * continuously differentiable everywhere, so that the linearization
  * (gradient) is well-defined and always easily computable as soon as the
  * value of the active Variable is set. */

 virtual bool has_linearization( bool diagonal = true ) {
  return( diagonal );
  }

/*--------------------------------------------------------------------------*/
 /// compute a new linearization for this Function
 /** This method has to compute a *new* linearization for this Function. The
  * type of the linearization that must be computed is given as a parameter:
  * if diagonal is true (default), then a diagonal linearization is required,
  * otherwise a vertical one is.
  *
  * This method can only be called after the compute() method of this Function
  * has been invoked at least once, and if has_linearization() (for the same
  * type) has already returned true. The method has to return true if a
  * linearization that is *different* from the previously available one (of
  * the same type) is available. For a, say, algebraic Function that is
  * finite and continuously differentiable everywhere, this method should
  * always return false since only one (diagonal) linearization (the
  * gradient) is typically available (and no vertical ones are). Indeed,
  * the default implementation in the base class always returns false.
  *
  * If this method returns true, then the computed linearization can be
  * retrieved by the get_linearization_coefficients() and
  * get_linearization_constant() methods. Otherwise, it does not make sense
  * to call it again (with the same value of diagonal) unless the value of
  * the Variable has changed and compute() has been called again.
  *
  * Note the one of the possible behavior of this method is that the
  * linearizations (or information allowing to compute them, cf. the
  * Lagrangian case) is actually computed *during* the call to compute(),
  * and stored in order to be possibly later retrieved. In such a case, this
  * method basically only "goes through the list of computed linearizations".
  * Yet, linearizations (or information allowing to compute them) can be
  * "large" objects, and hence require considerable memory to be stored.
  * This is what intLPMaxSz, dblRAccLin and dblAAccLin are for: allowing
  * the C05Function to limit the number of linearizations (...) stored for
  * later use during compute(), i.e., both the maximum size of the local pool
  * and the number (quality) of the linearizations (...) stored in there.
  * For instance, many algorithms are interested in only one linearization,
  * which is why by default the maximum size of the local pool is set to 1,
  * which tells the Function that it needs to store only one linearization
  * (which saves space by not allocating more memory than it is needed) and
  * compute only one linearization (which can save computational time by not
  * computing more than one linearization). Note, however, that a Function
  * may only be able to produce a single linearization (say, because it is
  * continuously differentiable), and therefore can ignore all this.
  *
  * The local pool is strictly local to the point where compute() has been
  * called: whenever the Function is evaluated again, the local pool is
  * entirely reset. Keeping a long-term memory of linearizations (or
  * information allowing to compute them) is the task of the "global" pool
  * of linearizations. */

 virtual bool compute_new_linearization( bool diagonal = true ) {
  return( false );
  }

/*--------------------------------------------------------------------------*/
 /// store a linearization in the global pool 
 /** The last linearization that was computed by calling
  * compute_new_linearization() (which returned true) can be permanently
  * stored in the global pool of linearizations by calling this method.
  * Linearizations in the global pool are identified by their names, which
  * must be integers between 0 and intGPMaxSz - 1 (this allows to implement
  * the global pool as a simple vector-like structure containing either the
  * linearizations or the information allowing to compute them). The name
  * must be provided when storing a linearization, so that this specific
  * linearization can be later retrieved by using its name. If a
  * linearization is stored under the name of a previously stored
  * linearization, the latter is replaced with the former in the global pool.
  * A linearization is kept in the global pool until it is explicitly deleted
  * [see delete_linearization()] or replaced with another linearization by a
  * call of this method. A linearization with "large" name is also deleted if
  * the global pool of linearizations is shrank by changing intGPMaxSz.
  *
  * The method has a default empty implementation as some Function may
  * ignore this information because it can perform the other required
  * actions [see get_linearization_coefficients()] by other means. In
  * particular, a linear function always have the same linearization
  * everywhere, and therefore there is arguably no need to store it many
  * times over.
  *
  * The parameter issueMod decides if and how the C05FunctionMod with
  * type() == GlobalPoolAdded is issued, as described in Observer::make_par().
  * Note that typically shift() of the modification will be == 0, in that
  * adding a linearization to the global pool does not really change the
  * "physical representation" of the C05Function, but only its (partial)
  * "abstract representation" that the global pool provides. */

 virtual void store_linearization( Index name ,
                                   ModParam issueMod = eModBlck ) {}

/*--------------------------------------------------------------------------*/
 /// tells if there is a linearization in the global pool with that name
 /** The method has to return true if \p name is the index (name) of a
  * linearization currently in the global pool. The default implementation
  * of the method is to always return false, which is OK for C05Function that
  * does not store linearization at all. */

 [[nodiscard]] virtual bool is_linearization_there( Index name ) const {
  return( false );
  }

/*--------------------------------------------------------------------------*/
 /// tells if the linearization in the global pool with that name is vertical
 /** The method has to return true if \p name is the index (name) of a
  * vertical linearization currently in the global pool. Clearly,
  * is_linearization_vertical( name ) == true implies
  * is_linearization_there( name ) == true (if there is no linearization, it
  * cannot be vertical). The default implementation of the method is to
  * always return false, which is OK for C05Function that never produces
  * vertical linearizations (that is, the function is real-valued, i.e., it
  * never evaluates to +/- INF), or does not store linearization at all. */

 [[nodiscard]] virtual bool is_linearization_vertical( Index name ) const {
  return( false );
  }

/*--------------------------------------------------------------------------*/
 /// stores a combination of the given linearizations
 /** This method creates a linear combination of a given set of
  * linearizations, with given coefficients, and stores it (or information
  * allowing to compute it) into the global pool of linearizations with the
  * given name.
  *
  * The linearizations whose combination the new one must be created from,
  * together with the corresponding coefficients, are indicated by vector of
  * pairs. Each pair has the name of a linearization (that must be currently
  * stored in the global pool of linearizations) and the coefficient this
  * linearization must have in the convex combination. Once the combination
  * is created, it is stored into the global pool with the name passed as
  * argument. If the new linearization is stored under the name of a
  * previously stored linearization, the latter is replaced with the former.
  *
  * The rationale for this method is that most approximate sub-differentials
  * are convex sets. Thus, proving (approximate) optimality/stationariety of
  * a point x^* (even in the simple case where there are no constraints or
  * other components in the objective function) amounts at proving that
  * 0 \in \R^n belongs to the corresponding set of linearizations
  * (\eps-subdifferential). In general, even if called at such a x^* the
  * C05Function cannot be expected to produce the 0 linearization; this is
  * why proving (approximate) optimality/stationariety of x^* typically boils
  * down to producing a set of linearizations g_i at points x_i "close" to
  * x^* and then proving that 0 \in conv( { g_i } ). Such a "convexified
  * linearization" is often a valid linearization as any one directly
  * produced by the function, and therefore can be saved in the global pool
  * as they are. Indeed, typically the "important linearization" (see
  * set_important_linearization()), which is basically the dual optimal
  * solution of any optimization problem involving the C05Function, is one of
  * these. Yet, combinations of linearizations also have other algorithmic
  * uses besides being "dual solutions": the may be used, for instance, to
  * reduce the maximum size of the global pool by replacing many
  * linearizations by just one "representing them all" (think conjugate
  * subgradients).
  *
  * Note that the C05Function may freely decide to either store the
  * convexified linearization itself, or information allowing to compute it,
  * or both. For instance, in the Lagrangian case, any linearization g_i
  * corresponds to a point u_i in U. Hence, the convexified linearization
  * corresponds to a point in conv( U ); if U is convex then that point is
  * still in U, otherwise it is a point in its convex hull which can still
  * have numerous algorithmic uses.
  *
  * The method has a default empty implementation as some Function may
  * ignore this information because it can perform the other required
  * actions [see get_linearization_coefficients()] by other means. In
  * particular, a linear function always have the same linearization
  * everywhere, and therefore any convex combination of these always returns
  * the same vector.
  *
  * The parameter issueMod decides if and how the C05FunctionMod with
  * type() == GlobalPoolAdded is issued, as described in Observer::make_par().
  * Note that typically shift() of the modification will be == 0, in that
  * adding a linearization to the global pool does not really change the
  * "physical representation" of the C05Function, but only its (partial)
  * "abstract representation" that the global pool provides. */

 virtual void store_combination_of_linearizations(
  c_LinearCombination & coefficients , Index name ,
  ModParam issueMod = eModBlck ) {}

/*--------------------------------------------------------------------------*/
 /// specify how to construct "the important linearization"
 /** This method allows to record in the C05Function, so that it can be
  * retrieved at a later time, information that allows to construct "the
  * important linearization".
  *
  * Usually, proving optimality/stationariety of some x^* in problems
  * containing a C05Function involves using previously accrued linearizations
  * to construct "one important combination" of them. Often the combination is
  * convex and/or it should yield the all-0 vector. This is basically the
  * *dual optimal solution* of the optimization problem concerning the
  * C05Function. It may be very useful to be able to store this combination,
  * or rather the information needed to re-construct it, in case the
  * C05Function (or other parts of the Block) changes, in order to provide
  * effective reoptimization. For instance, the changes may be such that x^*
  * may nonetheless remain an optimal solution, and the availability of
  * (the information needed to re-construct) "the important linearization" may
  * allow to prove this with very little computational effort. Hence, saving
  * (information concerning) the "important linearization" when the algorithm
  * is finished (but, possibly, even while it is running) may be useful.
  *
  * This can be clearly seen in the Lagrangian function example: for a point
  * x^* to be \eps-optimal for the minimization of f (the Lagrangian Dual),
  * one must collect a set of \eps-subgradients g_i such that
  * 0 \in conv( { g_i } ). It is immediate to realize that this corresponds to
  * a set of \eps-optimal solutions u_i to P(x^*) such that
  * u^* = \sum_i u_i \theta_i, for appropriate convex multipliers \theta_i,
  * is such that A u^* = b. If U in (P) is a convex set then such an u^* is
  * an optimal solution to (P), otherwise u^* is the optimal solution of the
  * relaxation of (P) substituting U with conv( U ). Any reasonable algorithm
  * for solving the Lagrangian Dual should be able to conceptually produce
  * such an object, i.e., the (convex) multipliers \theta_i, in order to stop.
  * Once the multipliers are known, u^* can be *explicitly* stored by calling
  * store_combination_of_linearizations(), which can be useful for algorithmic
  * purposes. For instance, u^* can be used to do separation of constraints
  * (say, in case the A u = b ones are very many, so that an active-set
  * strategy is necessary), or to guide heuristics or branching operations (if
  * the set U includes integrality constraints, so that the Lagrangian Dual of
  * (P) is only a relaxation). For all this to be possible, u^* and/or the
  * (convex) multipliers \theta_i must be available.
  *
  * The \p coefficients parameter just contains the encoding of the \theta_i
  * multipliers, which the C05Function is supposed to store. Note that the
  * C05Function itself has little use for the information, but it seems to be
  * the most appropriate repository (since the information is tied to
  * elements of the global pool, which is managed by the C05Function). Note
  * that
  *
  *     THE C05Function IS NOT ASSUMED TO CHECK IF LINEARIZATIONS
  *     INVOLVED IN coefficients ARE DELETED OR CHANGED (because there is
  *     little sensible recourse in this case)
  *
  * If there is a risk that this happens, the entity having use for this
  * information should check and react appropriately, whatever this means.
  *
  * Of course, one possible use for this information is to call
  * store_combination_of_linearizations() to "physically" construct the
  * "important linearization" and have it stored in the global pool. In this
  * case, of course, \p coefficients should just be < name of the important
  * linearization , 1 >. Indeed, there may be no need to "physically" construct
  * the "important linearization" since it could well be one of those directly
  * produced by the C05Function; think to the case where f is smooth at x^*.
  * Alternatively, the algorithm doing the optimization may find it
  * algorithmically expedient to produce it anyway. However, in principle
  * there is no need for the "important linearization" to be physically
  * constructed: once the information of \p coefficients is stored, the
  * "important linearization" is "virtually" present in the global pool (as
  * it is only a store_combination_of_linearizations() call away from being
  * there). Thus, we consider \p coefficients to be equivalent to the
  * "important linearization" itself; they clearly are when \p is a singleton
  * which just specifies the name, but also in the general case. Whence the
  * name of this method.
  *
  * As the "&&" tells, \p coefficient becomes "property" of the C05Function,
  * and it is assumed to be retrieved by 
  * get_important_linearization_coefficients().
  *
  * This method has a default empty implementation as some Functions may not
  * need to store linearizations, in which case the name is irrelevant. In
  * particular, a linear function always have the same linearization
  * everywhere, and therefore only one well-known linearization can be the
  * "important one". In general smooth functions may only require one
  * nonzero coefficient to characterize the important linearization.
  *
  * Note that
  *
  *     CHANGING THE IMPORTANT LINEARIZATION DOES NOT AMOUNT, IN ITSELF, AT
  *     CHANGING THE GLOBAL POOL, AND THEREFORE NO Modification IS ISSUED
  *
  * In fact, what this method sets is only the information characterizing the
  * "virtual important linearization"; if the "physical" one is constructed
  * out of this information this changes the global pool and a Modification is
  * indeed issued, but this is a separate operation from just storing the
  * combination. Besides, the important linearization is basically (a part
  * of) the dual optimal solution of the optimization problem involving the
  * C05Function. As such, it is "impermanent" like all solutions in a Block:
  * the users cannot assume them to be kept (unless they properly lock the
  * Block), and must use them immediately and/or explicitly save them for
  * later use if they need to. Indeed, note that since there may be multiple
  * optimal dual solutions, this method may be called more than once at the
  * end of an optimization to retrieve them all, one by one. */

 virtual void set_important_linearization(
				       LinearCombination && coefficients ) {}

/*--------------------------------------------------------------------------*/
 /// return(the combination used to form) "the important linearization"
 /** This method has to return the combination that can be used to form "the
  * important linearization", as set by set_important_linearization(). It has
  * a default "empty" implementation returning the pair < 0 , 1 > because for
  * some C05Function there is actually no need to store this information (in
  * particular, a linear function always has the same linearization, hence
  * all linearizations in the global pool taken individually are the important
  * one, comprised the one with name 0). */

 [[nodiscard]] virtual c_LinearCombination &
 get_important_linearization_coefficients( void ) const {
  static c_LinearCombination _tmp = { std::make_pair( 0 , 1 ) };
  return( _tmp );
  }

/*--------------------------------------------------------------------------*/
 /// delete the given linearization from the global pool of linearizations
 /** This method deletes the linearization associated with the given name
  * from the global pool of linearizations. If there is no linearization
  * associated with the given name, an exception should be thrown (unless,
  * for instance, the concept is completely ignored). Indeed, the method has
  * a default empty implementation as some Functions may not need to store
  * anything.
  *
  * The parameter issueMod decides if and how the C05FunctionMod with
  * type() == GlobalPoolRemoved is issued, as described in
  * Observer::make_par(). Note that typically shift() of the modification
  * will be == 0, in that deleting a linearization from the global pool does
  * not really change the "physical representation" of the C05Function, but
  * only its (partial) "abstract representation" that the global pool
  * provides. */

 virtual void delete_linearization( Index name ,
                                    ModParam issueMod = eModBlck ) {}

/*--------------------------------------------------------------------------*/
 /// delete the given subset of linearizations from the global pool
 /** Delete the linearizations whose names are contained in \p which from
  * the global pool. If which().empty(), all the linearizations are removed
  * (i.e., the global pool is completely flushed). If \p ordered == true
  * then \p which which must be ordered in increasing sense. As the && tells,
  * which becomes property of the method, typically to be dispatched to the
  * C05FunctionMod with type() == GlobalPoolRemoved that is issued, according
  * to the value of the parameter \p issueMod, as described in
  * Observer::make_par(). Note that typically shift() of the Modification
  * will be == 0, in that deleting a linearization from the global pool does
  * not really change the "physical representation" of the C05Function, but
  * only its (partial) "abstract representation" that the global pool
  * provides.
  *
  * This method is given a rough default implementation that just calls
  * delete_linearization( i ) for all linearizations in the global pool.
  * This is unlikely to be the most effective way, but at least the
  * individual calls to not generate individual C05FunctionMod; rather, an
  * unique one is issued at the end. */

 virtual void delete_linearizations( Subset && which , bool ordered = true ,
                                     ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// get a range of coefficients (g vector) of a linearization in an array
 /** This method retrieves a range of the vector of coefficients g that is
  * the (largest part of the) linearization with the given name.
  *
  * If the name of the linearization is the default value Inf< Index >(), then
  * it refers to the last computed linearization, which may "not yet have a
  * name" because store_linearization() may not have been called yet (and it
  * may never be, if this linearization is not deemed "important enough" to
  * be kept in the global pool). Otherwise, it (obviously) refers to the
  * linearization associated with the given name from the global pool of
  * linearizations. If a linearization with the given name is not stored in
  * the global pool, an exception may be thrown (unless, for instance, the
  * concept is completely ignored because, say, the C05Function is a linear
  * one and therefore all linearizations are the same). 
  *
  * The method allows to only retrieve a range of the components of g (by
  * default, all of it). The components of a linearization are numbered from
  * 0 to n - 1, where n == get_num_active_var() is the number of active
  * Variables of this Function. Moreover, the i-th component of a
  * linearization is associated with the i-th active Variable of this
  * C05Function, i.e., (the one pointed by) get_active_var( i ). For
  * exposition purposes, we can therefore view the (largest part of the)
  * linearization as the n-vector l[]. The parameter range dictates which
  * components of l[] must be returned, i.e., all the ones in the half-closed
  * interval
  *
  *     [ range.first , min( n , range.second ) )
  *
  * This is the "rough version" of the method where the output is directly
  * into an array: all components of l[] between range.first (included) and
  * min( n , range.second ) (excluded) will be stored in the array g in the
  * positions from 0 to min( n , range.second ) - 1 - range.first, i.e.,
  * g[ i - range.first ] = l[ i ] for all range.first <= i <
  * min( n , range.second ).
  *
  * The rationale for having such a "rough" version is that it allows to
  * "extend or change linearizations already in place". For instance, assume
  * that whatever is using this C05Function has stored the linearizations in
  * the global pool as a 
  * 
  *   std::vector < Vec_FunctionValue > > G;
  *
  * where the inner vectors G[ i ] are dimensioned to the *maximum* number of
  * Variable that the C05Function may have, and that a FunctionModVarsAddd
  * occurs which adds k more Variable. These by definition take indices
  * [ n , n + 1 , ... , n + k - 1 ): hence, the new entries of the i-th
  * linearization in the global pool corresponding to these new Variable can
  * be written in place in the existing vectors by just calling
  *
  *   get_linearization_coefficients( G[ i ].data() , Range( n , k ) , i )
  *
  * Similarly, if a C05FunctionModRngd mod occurs, then the corresponding
  * mod->range() of changed entries of f the i-th linearization in the global
  * pool can be written in place in the existing vectors by just calling
  *
  *   get_linearization_coefficients( G[ i ].data() , mod->range() , i )
  */

 virtual void get_linearization_coefficients( FunctionValue * g ,
					      Range range = INFRange ,
					   Index name = Inf< Index >() ) = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get a range of coefficients of a linearization in a SparseVector
 /** This method retrieves the sparse vector of coefficients g that is part
  * of a linearization. It works like
  *
  *   get_linearization_coefficients( FunctionValue * g , range , name  )
  *
  * except that the extracted coefficients are stored in the SparseVector g
  * instead of in a "rough" array.
  *
  * If the SparseVector g passed as argument does not have any non-zero
  * element, it will be resized to the appropriate size, which is the number
  * n of active Variables of the Function, and the desired components of the
  * linearization will be stored in it. The computational cost of this
  * operation is proportional to the number of desired components.
  *
  * If the SparseVector g already stores some non-zero elements, then the
  * size of g must be equal to the number n of active Variables of this
  * Function. Moreover, this vector will be updated. This means that for each
  * component j of the linearization vector that is desired (the components
  * determined by range, see the "rough" version for details), the value
  * l[ j ] will be stored in g[ j ], and any previously stored value in
  * g[ j ] will be lost. If there is any (nonzero) g[ h ] for any index h
  * that is not among the ones desired, this will be left unchanged in the
  * SparseVector. It is important to notice that the operation of updating
  * the given SparseVector may be computationally costly; in order to avoid
  * this, consider passing to this method an empty g.
  *
  * This method is given by the base class a default implementation using the
  * "rough" version. Derived classes are welcome to provide specialized
  * implementations avoiding the copy for efficiency. */

 virtual void get_linearization_coefficients( SparseVector & g ,
					      Range range = INFRange ,
					      Index name = Inf< Index >() ) {
  range.second = std::min( range.second, get_num_active_var() );
  if( range.second <= range.first )  // range is empty
   return;                           // cowardly (and silently) return

  Vec_FunctionValue gg( range.second - range.first );
  FunctionValue * ggp = gg.data();
  get_linearization_coefficients( ggp, range, name );

  if( g.nonZeros() == 0 ) {  // g has no nonzeroes

   if( g.size() < get_num_active_var() )
    g.resize( get_num_active_var() );

   g.reserve( range.second - range.first );

   for( Index i = range.first ; i < range.second ; ++i , ++ggp )
    if( *ggp )
     g.insert( i ) = *ggp;
   }
  else {                  // g has some nonzeroes
   if( g.size() != get_num_active_var() )
    throw( std::invalid_argument( "wrong size of nonempty SparseVector g" ) );

   for( Index i = range.first; i < range.second; ++i )
    g.coeffRef( i ) = *(ggp++);

   g.prune( 0 , 0 );
   }
  }

/*--------------------------------------------------------------------------*/
 /// get a subset of coefficients (g vector) of a linearization in an array
 /** This method retrieves a subset of the vector of coefficients g that is
  * the (largest part of the) linearization with the given name.
  *
  * If the name of the linearization is the default value Inf< Index >(), then
  * it refers to the last computed linearization, which may "not yet have a
  * name" because store_linearization() may not have been called yet (and it
  * may never be, if this linearization is not deemed "important enough" to
  * be kept in the global pool). Otherwise, it (obviously) refers to the
  * linearization associated with the given name from the global pool of
  * linearizations. If a linearization with the given name is not stored in
  * the global pool, an exception may be thrown (unless, for instance, the
  * concept is completely ignored because, say, the Function is a linear one
  * and therefore all linearizations are the same). 
  *
  * The method allows to retrieve a specific arbitrary subset of the 
  * coefficients, the ones whose indices are to be found in the Subset subset.
  * The components of a linearization are numbered from 0 to n - 1, where
  * n == get_num_active_var() is the number of active Variables of this
  * Function. Moreover, the i-th component of a linearization is associated
  * with the i-th active Variable of this Function, i.e., (the one pointed by)
  * get_active_var( i ). For exposition purposes, we can therefore view the
  * linearization as the n-vector l[]. The parameter subset is used to
  * dictate which components of l[] must be returned. Each element
  * subset[ i ] must clearly be an integer between 0 and n - 1.
  *
  * This is the "rough version" of the method where the output is directly
  * into an array. Only the components found in subset will be stored in g;
  * that is, g[ i ] = l[ subset[ i ] ] for all 0 <= i < subset.size(). All
  * components of g[] that need not be written are left unchanged.
  *
  * The parameter ordered tells if the subset vector is ordered for
  * increasing Index of the "active" Variable. This may be useful for some
  * implementation; say, if the data is kept in something like a list, i.e.,
  * without random access, so that ordered reading of a subset is linear
  * rather than quadratic. In this case, is subset is not ordered the
  * derived class may have to (copy and) order it, so it may be good to know
  * that this is not required.
  *
  * The rationale for having such a "rough" version is that it allows to
  * "change linearizations already in place". For instance, assume that
  * whatever is using this C05Function has stored the linearizations in the
  * global pool as a 
  * 
  *   std::vector < Vec_FunctionValue > > G;
  *
  * where the inner vectors G[ i ] are dimensioned to the *maximum* number of
  * Variable that the C05Function may have, and that a C05FunctionModSbst mod
  * occurs; then the corresponding mod->subset() of changed entries of the
  * i-th linearization in the global pool can be written in place in the
  * existing vectors by just calling
  *
  *   get_linearization_coefficients( G[ i ].data() , mod->subset() , i  )
  */

 virtual void get_linearization_coefficients( FunctionValue * g ,
                                  c_Subset & subset , bool ordered = false ,
                                  Index name = Inf< Index >() ) = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get a subset of coefficients of a linearization in a SparseVector
 /** This method retrieves the sparse vector of coefficients g that is part
  * of a linearization. It works like
  *
  *   get_linearization_coefficients( FunctionValue * g , subset , name )
  *
  * except that the extracted coefficients are stored in the SparseVector g
  * instead of in a "rough" array.
  *
  * If the SparseVector g passed as argument does not have any non-zero
  * element, it will be resized to the appropriate size, which is the number
  * n of active Variables of the Function, and the desired components of the
  * linearization will be stored in it. The computational cost of this
  * operation is proportional to the number of desired components.
  *
  * If the SparseVector g already stores some non-zero elements, then the
  * size of g must be equal to the number n of active Variables of this
  * Function. Moreover, this vector will be updated. This means that for each
  * component j of the linearization vector that is desired (the components
  * determined by range, see the "rough" version for details), the value
  * l[ j ] will be stored in g[ j ], and any previously stored value in
  * g[ j ] will be lost. If there is any (nonzero) g[ h ] for any index h
  * that is not among the ones desired, this will be left unchanged in the
  * SparseVector. It is important to notice that the operation of updating
  * the given SparseVector may be computationally costly; in order to avoid
  * this, consider passing to this method an empty g.
  *
  * This method is given by the base class a default implementation using the
  * "rough" version. Derived classes are welcome to provide specialized
  * implementations avoiding the copy for efficiency. */

 virtual void get_linearization_coefficients( SparseVector & g ,
                                    c_Subset & subset , bool ordered = false ,
                                    Index name = Inf< Index >() ) {
  if( subset.empty() )  // subset is empty
   return;              // cowardly (and silently) return

  Vec_FunctionValue gg( subset.size() );
  FunctionValue * ggp = gg.data();
  get_linearization_coefficients( ggp, subset, ordered, name );

  if( g.nonZeros() == 0 ) {  // g has no nonzeroes

   if( g.size() < get_num_active_var() )
    g.resize( get_num_active_var() );

   g.reserve( subset.size() );

   for( auto i : subset ) {
    if( i >= get_num_active_var() )
     throw( std::invalid_argument( "wrong index in subset" ) );
    auto gi = *(ggp++);
    if( gi )
     g.insert( i ) = gi;
    }
   }
  else {                  // g has some nonzeroes
   if( g.size() != get_num_active_var() )
    throw( std::invalid_argument( "wrong size of nonempty SparseVector g" ) );

   for( auto i : subset ) {
    if( i >= get_num_active_var() )
     throw( std::invalid_argument( "wrong index in subset" ) );
    g.coeffRef( i ) = *(ggp++);
    }

   g.prune( 0 , 0 );
   }
  }

/*--------------------------------------------------------------------------*/
 /// return the constant term of a linearization
 /** This method returns the constant term (alpha) of a linearization. If the
  * name of the linearization is the default value Inf< Index >(), then it
  * refers to the last computed linearization, which may "not yet have a
  * name" because store_linearization() may not have been called yet (and it
  * may never be, if this linearization is not deemed "important enough" to
  * be kept in the global pool). Otherwise, it (obviously) refers to the
  * linearization associated with the given name from the global pool of
  * linearizations. If a linearization with the given name is not stored in
  * the global pool, an exception may be thrown (unless, for instance, the
  * concept is completely ignored because, say, the Function is a linear one
  * and therefore all linearizations are the same).
  *
  * The method can be used, after that an appropriate Modification has been
  * issued [see C05FunctionMod*], to get the value of \alpha for each of the
  * linearizations stored in the global pool (for which it has been changed,
  * as the C05FunctionMod* specifies). If a linearization has become
  * invalid, the linearization should not be asked; if it is, the method
  * should retur NaN (e.g. as what is reported by
  * std::numeric_limits::quiet_NaN< FunctionValue >() or by
  * std::numeric_limits::signaling_NaN< FunctionValue >()). */

 virtual FunctionValue get_linearization_constant(
                                           Index name = Inf< Index >() ) = 0;

/*--------------------------------------------------------------------------*/
 /// returns true only if this Function is continuously differentiable
 /** Method that returns true only if this Function is continuously
  * differentiable. The default is false. Note that a continuously
  * differentiable function, when called with the default value of the
  * accuracy parameters dblRAccLin and dblAAccLin, should only return one
  * single linearization per compute(), as the only exact linearization is
  * the gradient, which is unique. */

 [[nodiscard]] virtual bool is_continuously_differentiable( void ) const {
  return( false );
  }

/** @} ---------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the Function
 *  @{ */

 [[nodiscard]] idx_type get_num_int_par( void ) const override {
  return( intLastParC05F );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type get_num_dbl_par( void ) const override {
  return( dblLastParC05F );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] int get_dflt_int_par( idx_type par ) const override {
  if( par == intLPMaxSz )
   return( 1 );
  if( par == intGPMaxSz )
   return( 0 );

  return( Function::get_dflt_int_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] double get_dflt_dbl_par( idx_type par ) const override {
  if( ( par == dblRAccLin ) || ( par == dblAAccLin ) )
   return( 0 );
  if( par == dblAAccMlt )
   return( 1e-10 );

  return( Function::get_dflt_dbl_par( par ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] idx_type int_par_str2idx( const std::string & name )
  const override {
  if( name == "intLPMaxSz" )
   return( intLPMaxSz );
  if( name == "intGPMaxSz" )
   return( intGPMaxSz );

  return( Function::int_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type dbl_par_str2idx( const std::string & name )
  const override {
  if( name == "dblRAccLin" )
   return( dblRAccLin );
  if( name == "dblAAccLin" )
   return( dblAAccLin );
  if( name == "dblAAccMlt" )
   return( dblAAccMlt );

  return( Function::dbl_par_str2idx( name ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] const std::string & int_par_idx2str( idx_type idx )
  const override {
  static const std::vector< std::string > pars =
                                              { "intLPMaxSz", "intGPMaxSz" };
  if( idx == intLPMaxSz )
   return( pars[ 0 ] );
  if( idx == intGPMaxSz )
   return( pars[ 1 ] );

  return( Function::int_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & dbl_par_idx2str( idx_type idx )
  const override {
  static const std::vector< std::string > pars =
                             { "dblRAccLin" , "dblAAccLin" , "dblAAccMlt" };
  if( idx == dblRAccLin )
   return( pars[ 0 ] );
  if( idx == dblAAccLin )
   return( pars[ 1 ] );
  if( idx == dblAAccMlt )
   return( pars[ 2 ] );

  return( Function::dbl_par_idx2str( idx ) );
  }

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
 *  @{ */

 /// print information about the C05Function on an ostream
 /** Protected method intended to print information about the C05Function; it
  * is virtual so that derived classes can print their specific information
  * in the format they choose. */

 void print( std::ostream & output ) const override {
  output << "C05Function [" << this << "]"
         << " with " << get_num_active_var() << " active variables";
  }

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

 };  // end( class( C05Function ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS C05FunctionMod ---------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modifications specific to a C05Function
/** Derived class from FunctionMod to describe modifications to a
 * C05Function. This class defines the following types of modifications:
 *
 * - NothingChanged
 *
 * This type of Modification states that the all the previously produced
 * linearizations, both g and \alpha, are still perfectly valid for the
 * changed C05Function. This may look weird at first, but there can be cases
 * where it happens. For instance, consider a (convex, finite) max-function
 *
 *     f( x ) = max { g_i x + \alpha_i : i = 1 , ... , m }
 *
 * and assume that f() changes because one more linear form
 *
 *               g_{m + 1} x + \alpha_{m + 1}
 *
 * is added in the max. Clearly the new function is different from before, and
 * in general >= (thus, shift() == INFshift can be reported); however, all the
 * previous linearizations (that are lower linearizations of the epigraph of
 * the convex function) still remain perfectly valid \epsilon-subgradients
 * for the new f(), albeit in general with a larger \epsilon (but since the
 * linearization is given in terms of \alpha, which does not depend on the
 * actual function value, this is irrelevant). Another case is the one in
 * which the max-function is abruptly changed to be a min-function on exactly
 * the same data: albeit the function changes from convex to concave, and
 * clearly the function values change (in this case they become <=, which
 * means that shift() == - INFshift can be reported); still all the previously
 * computed linearizations remain valid without any change. The difference is
 * that when the function was convex they were (approximate) *sub*gradients,
 * i.e., *lower* linearizations of the *epi*graph; as the function is turned
 * into concave they become (approximate) *super*gradients, i.e., *upper*
 * linearizations of the *ipo*graph. Of course the Solver will have to be able
 * to deal with this, which is not necessarily obvious (in fact the Solver may
 * well refuse to and throw an exception instead), but still this can in
 * principle happen. Note that a similar occurrence happens in the Lagrangian
 * case (a non-necessarily-finite max function) if the verse of the objective
 * is reversed. In this case, which() must be empty().
 *
 * - AlphaChanged
 *
 * This type of Modification states that the constant (\alpha) of the
 * linearizations has changed. Note, however, that this does *not* mean that
 * *all the \alpha have changed in the same way*; this happens if the whole
 * function is shifted by a constant, which is what the original FunctionMod
 * is intended to convey. Rather, the idea is that each individual \alpha has
 * changed in its own way, *but all the g remained unchanged*. The idea is
 * that it is possible to query the value of \alpha for all the
 * linearizations that were stored in the global pool [see
 * get_linearization_constant()], thereby re-using all the corresponding
 * information to warm-start whatever algorithm one is using. Actually, the
 * C05Function may make life easier to the user and store in which() the
 * names of the linearizations that actually have changed. Since these may
 * be all (or too many to bother, or the C05Function may be "lazy"),
 * which.empty() has to be taken to mean "possibly all, so check them all".
 *
 * A case where this happens is the Lagrangian one, where each linearization
 * is attached to one solution u_i \in U, and the objective function c of (P)
 * changes to some c' different from c. Having stored u_i in the global pool,
 * it is possible to compute the new value of \alpha = c' u_i. Since the
 * objective is real-valued (as opposed to extended-real-valued, i.e., it
 * cannot evaluate to +INF, in fact encoding a violated constraint), every
 * u_i that was previously feasible still is, and therefore no linearization
 * is lost.
 *
 * Note that one would expect that the change in the \alpha implies a change
 * in the values of the Function as well. If and how this actually happens
 * is encoded in shift() with the same encoding as in the base class
 * FunctionMod. For the Lagrangian case, for instance, if u >= 0 and c' >= c
 * then the objective value can only increase, and therefore shift() ==
 * INFshift is the appropriate return value. If, instead, no sign pattern
 * can be established then std::isnan( shift() ) == true should be returned.
 *
 * - AllEntriesChanged
 *
 * This type of Modification states that all the entries of the g part of
 * all previously computed linearizations may have changed, *but all the
 * \alpha have remained unchanged*.
 *
 * A case where this happens is the Lagrangian one, where each linearization
 * is attached to one solution u_i \in U, g_i = b - A u_i. If the constraint
 * A u = b change to completely unrelated (save for the size) A' u = b', then
 * all the corresponding g_i need to be recomputed, but the \alpha_i =
 * c u_i remains the same. It is then possible to query the new values of the
 * linearizations that were stored in the global pool [see
 * get_linearization_coefficients()], thereby re-using all the corresponding
 * information to warm-start whatever algorithm one is using.
 *
 * Note that one would expect that the change in the \g implies a change in
 * the values of the Function as well. If and how this actually happens
 * is encoded in shift() with the same encoding as in the base class
 * FunctionMod. For the Lagrangian case, for instance, it is unlikely (but
 * not downright impossible) that some monotonicity relationship can be
 * derived between the previous and the new function values, whence
 * std::isnan( shift() ) == true is the appropriate return value.
 *
 * Again, the C05Function may make life easier to the user and store in
 * which() the names of the linearizations that actually have changed, with
 * which.empty() to be taken to mean "possibly all, so check them all".
 *
 * - AllLinearizationChanged
 *
 * This type of Modification is the logical union of both previous types:
 * for all of previously computed linearizations, both the g part and the
 * \alpha may have changed. This again typically implies that the function
 * values have changed as well, with shift() specifying how. It is then
 * possible to query the new values of the linearizations that were stored
 * in the global pool using the available methods as in the two previous
 * cases.
 *
 * Again, the C05Function may make life easier to the user and store in
 * which() the names of the linearizations that actually have changed, with
 * which.empty() to be taken to mean "possibly all, so check them all".
 *
 * - GlobalPoolAdded
 *
 * This type of Modification indicates that, while the C05Function has not
 * "physically changed" (which means shift() == 0 is expected), some new
 * linearizations have been added to "its abstract representation", i.e.,
 * the global pool. In this case, which().empty() is *not* allowed, and
 * which() has to contain the names of the linearizations that have been
 * added.
 *
 * - GlobalPoolRemoved
 *
 * This type of Modification indicates that some existing linearizations
 * have been removed from the global pool. This may either mean that the
 * C05Function has not "physically changed" (which is encoded by shift()
 * == 0), but some existing linearizations have been removed from "its
 * abstract representation", or that the C05Function has indeed "physically
 * changed" (in which case a nonzero shift() is possible) and as a consequence
 * some linearizations had become invalid.
 *
 * A case where the latter happens is the Lagrangian one, where each
 * linearization is attached to one solution u_i \in U, and U itself changes
 * to some U' \neq U. Hence, each u_i is either feasible (u_i \in U') or not
 * (u_i \notin U'). In the first case nothing has to be done, as the
 * linearization remains valid (and its  \alpha = c u_i does not change), but
 * in the second case the linearization it is no longer valid and has to be
 * removed from the global pool. Note that this can easily have an effect
 * on the function value as well: for instance, if the feasible region U
 * becomes smaller (which is what typically happens after either branching
 * or cutting in a Branch-and-X approach in which the Lagrangian is providing
 * the bound), than the optimal value of the problem can only reduce, whence
 * shift() == INFshift is the appropriate return value.
 *
 * In this case, which().empty() is allowed, but it means *all linearization
 * in the global pool are deleted*, and it is therefore not something to be
 * done just because one is "lazy".
 *
 * Also, note that
 *
 *     A COMPLETE RESET OF THE GLOBAL POOL CAN BE IMPLICITLY REQUIRED BY
 *     ANY Modification THAT SIGNAL CHANGES IN THE FUNCTION THAT ARE NOT
 *     STRONGLY QUASI-ADDITIVE (CF. THE DISCUSSION IN C05FunctionModVars*),
 *     AS THIS IMPLIES THAT ALL THE LINEARIZATION HAVE NECESSARILY BECOME
 *     INVALID. THAT IS, RECEIVING A FunctionMod THAT IS *NOT* A
 *     C05FunctionMod OR A FunctionModVars* THAT IS *NOT* A
 *     C05FunctionModVars* ALSO IMPLIES THAT THE GLOBAL POOL NEED BE ENTIRELY
 *     FLUSHED. IN THIS CASE, IT IS *NOT* EXPECTED THAT A SEPARATE
 *     C05FunctionMod WITH type() == GlobalPoolRemoved AND which().empty() IS
 *     ISSUED.
 */

class C05FunctionMod : public FunctionMod
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/

 /// Definition of the possibles type of C05FunctionMod
 /** This enum specifies what kind of assumption can be made about any
  * previously produced linearization. */
 enum c05function_mod_type {
  NothingChanged,           ///< both \alpha and g are still valid
  AlphaChanged,             ///< all the \alpha have changed, but g has not
  AllEntriesChanged,        ///< all the g have changed, but \alpha has not
  AllLinearizationChanged,  ///< both \alpha and g have changed
  GlobalPoolAdded,          ///< linearizations added to the global pool
  GlobalPoolRemoved,        ///< linearizations removed from the global pool
  C05FunctionModLastParam
  ///< First allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend
   * the set of types of modifications. */
  };

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: takes the type of Modification and a C05Function pointer
 /** Constructor: takes a pointer to the affected C05Function, the type of the
  * Modification, the Subset of affected linearizations, ordered in increasing
  * sense (if empty, it means "all of them"), the value of the shift, and the
  * "concerns Block" value. Note that while the enum c05function_mod_type is
  * provided to encode the possible values of modification, the field f_type
  * is of type "int", and therefore so is the parameter of the constructor, in
  * order to allow derived classes to "extend" the set of possible types of
  * modifications. */

 C05FunctionMod( C05Function * f, int type, Subset && which = {} ,
                 FunctionValue shift = NaNshift , bool cB = true )
  : FunctionMod( f , shift , cB ) , f_type( type ) ,
    v_which( std::move( which ) ) {
  #ifndef NDEBUG
   for( Index i = 1; i < v_which.size(); ++i )
    if( v_which[ i - 1 ] >= v_which[ i ] )
     throw( std::invalid_argument( "unordered or repeated which" ) );
  #endif
  }

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~C05FunctionMod() override = default;  ///< destructor: does nothing

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the type of modification

 [[nodiscard]] int type( void ) const { return( f_type ); }

 /// accessor to the names of the linearizations that have been affected
 /** Returns the names of the linearizations that have been affected (added,
  * removed, changed). */

 [[nodiscard]] c_Subset which( void ) const { return( v_which ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the C05FunctionMod

 void print( std::ostream & output ) const override {
  output << "C05FunctionMod[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function [" << &f_function << " ]: ";
  guts_of_print( output );
  }

/*--------------------------------------------------------------------------*/

 void guts_of_print( std::ostream & output ) const {
  if( f_type == NothingChanged )
   output << "nothing changed";
  else {
   if( f_type <= AllLinearizationChanged ) {
    if( v_which.empty() )
     output << "in all";
    else
     output << "in " << v_which.size();
    output << " linearizations ";
    }
   else
    output << v_which.size();
   switch( f_type ) {
    case( AlphaChanged ): output << "the \alpha have changed"; break;
    case( AllEntriesChanged ): output << "the g have changed"; break;
    case( AllLinearizationChanged ):
     output << "both \alpha and g have changed"; break;
    case( GlobalPoolAdded ): output << " linearizations added"; break;
    case( GlobalPoolRemoved ): output << " linearizations removed"; break;
    default: output << " unknown operation (?)";
    }
   }
  if( f_shift ) {
   output << ", f-values changed";
   if( std::isnan( f_shift ) )
    output << "(+-)";
   else
    if( f_shift >= INFshift )
     output << "(+)";
    else
     if( f_shift <= - INFshift )
      output << "(-)";
     else
      output << " by " << f_shift;
   }
  output << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 int f_type;      ///< type of modification

 Subset v_which;  ///< which linearizations have changed

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionMod ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS C05FunctionModRngd --------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe changes to a C05Function involving a range of Variable
/** Derived class from C05FunctionMod, extends it to the concept that the
 * changes to the "complicated" part of the linearization (the vector g) may
 * be localized to some subset of the entries as opposed to involving the
 * whole vector. For this reason it includes a std::vector< Variable * >
 * field (akin to that of FunctionModVars), returned by vars(). However,
 * this Modification (unlike C05FunctionModSbst) assumes that the Variable
 * pointed vars() are not "arbitrarily dispersed", but correspond to a given
 * range of indices. Hence, besides vars() the Modification also contains a
 * Range (returned by range()) describing it.
 *
 * The class supports only two of the types of change
 *
 * - AllEntriesChanged
 * - AllLinearizationChanged
 *
 * of C05FunctionMod, but "All" here have to be taken to mean "all those
 * specified by vars()". It does not make sense to issue a
 * C05FunctionModRngd with any other type(), since they do not involve g.
 * No check is performed that type() is of the right kind.
 *
 * A convenient use case for this Modification is that of a structured
 * optimization problem
 *
 *   (P)  max \{ c u : A u = b , u \in U \}
 *
 * and of its (convex) Lagrangian function
 *
 *   (P( x ))  f( x ) = max \{ c u + x ( b - A u ) : u \in U \}
 *
 * Linearizations are associated with feasible solutions \bar{u} \in U, as
 *
 *   ( g , \alpha ) = ( b - A \bar{u} , c \bar{u} )
 *
 * A subset I \subset { 1 , ... , n } of the (indices of the) relaxed
 * constraints (hence, of the Lagrangian variables x_i) may be abruptly
 * changed: the corresponding constraints A_I u = b_I become completely
 * unrelated ones A'_I u = b'_I. Yet, the entries corresponding to I in
 * (the g part of) a previous linearization corresponding to some \bar{u}
 * can be re-computed as g_I = b'_I - A'_I \bar{u} while keeping all the
 * rest unchanged. These entries can be recovered by calling
 * get_linearization_coefficients() provided that appropriate information
 * about that linearization (\bar{u}) has been saved in the global pool.
 * Note that in this case the subset I is a range [ start , stop ) [but see
 * the warning below about the fact that the indices may have not remained
 * valid], which justifies why get_linearization_coefficients() have the
 * corresponding parameters. The case where I is "any" subset of the indices
 * is dealt with by C05FunctionModSbst, but get_linearization_coefficients()
 * also have an appropriate parameter to handle that case.
 *
 * The important remark that the range refers to the index that the Variable
 *
 *     HAD AT THE MOMENT IN WHICH THE C05FunctionModRngd WAS ISSUED,
 *     SINCE WHEN THE C05FunctionModRngd IS PROCESSED, THESE Variable MAY
 *     HAVE CHANGED INDEX (IF Variable WITH SMALLER INDEX HAVE BEEN
 *     REMOVED), OR COULD HAVE EVEN BEEN REMOVED (IN WHICH CASE AN
 *     APPROPRIATE Modification MUST BE SITTING IN THE QUEUE AFTER THIS ONE).
 *
 * Yet, this information may still be useful to a Solver which keeps some
 * internal data structures depending on the order of the Variable in the
 * Function, as it might make it easier to quickly locate the current index
 * of the affected Variable. Indeed, note that
 *
 *     THE INDEX THAT THE AFFECTED Variable HAVE AT THE MOMENT IN WHICH THE
 *     C05FunctionModRngd IS PROCESSED CAN ONLY BE SMALLER THAN OR EQUAL
 *     TO THAT THAT THE INFORMATION REPORTED HERE IMPLIES, EXCEPT IF A
 *     Variable HAS BEEN DELETED AND RE-ADDED (IN WHICH CASE TWO
 *     APPROPRIATE Modification MUST BE SITTING IN THE QUEUE AFTER THIS ONE).
 *
 * Arguably, then,
 *
 *     IN MANY CASES, IF A Variable IN var() DOES NOT HAVE THE SAME INDEX AS
 *     THE Range IMPLIES OR A SMALLER ONE, THE C05FunctionModRngd CAN BE
 *     IGNORED FOR THAT Variable SINCE IT IS ANYWAY SUBSUMED BY THE
 *     Modification (SITTING IN THE QUEUE AFTER THIS ONE) CORRESPONDING TO
 *     ITS REMOVAL AND SUBSEQUENT RE-INSERTION.
 *
 * This may allow to simplify somewhat the work for some Solver. */

class C05FunctionModRngd : public C05FunctionMod
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: takes all the necessary information
 /** Constructor: besides all the information required by the base
  * C05FunctionMod, it takes the subset of affected Variable under the form
  * of a std::vector< Variable * >, and the corresponding range identifying
  * the indices of these Variable at the moment in which the
  * C05FunctionModRngd was issued. The range is a pair of indices ( start ,
  * stop ) representing the typical left-closed, right-open range
  * { i : start <= i < stop }, and the correspondence between that and vars
  * is positional: vars[ 0 ] had index range, vars[ 1 ] had index range + 1,
  * ..., which implies that vars.size() == stop - start. As the &&
  * tells, the vars[] vector "becomes property" of the C05FunctionModRngd
  * object. */

 C05FunctionModRngd( C05Function * f , int type, Vec_p_Var && vars ,
                     c_Range & range , Subset && which = {} ,
                     FunctionValue shift = 0 , bool cB = true )
  : C05FunctionMod( f , type , std::move( which ) , shift , cB ) ,
    v_vars( std::move( vars ) ) , f_range( range ) {
  if( v_vars.size() != f_range.second - f_range.first )
   throw( std::invalid_argument( "vars and range sizes do not match" ) );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~C05FunctionModRngd() override = default;  ///< destructor: does nothing

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the vector of pointers to affected Variable

 [[nodiscard]] c_Vec_p_Var & vars( void ) const { return( v_vars ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to the range of the deleted Variable

 [[nodiscard]] c_Range & range( void ) const { return( f_range ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the C05FunctionModRngd

 void print( std::ostream & output ) const override {
  output << "C05FunctionModRngd[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function [" << f_function << " ]: in ";
  if( ( f_type == AllEntriesChanged ) ||
      ( f_type == AllLinearizationChanged ) ) {
   if( v_which.empty() )
    output << "all";
   else
    output << v_which.size();
   output << "linearizations";
   if( f_type == AllEntriesChanged )
    output << " the g have changed";
   else
    output << "both \alpha and g have changed";
   }
  else
   output << "[ incorrect type() ]";
  if( f_shift ) {
   output << ", f-values changed";
   if( std::isnan( f_shift ) )
    output << "(+-)";
   else
    if( f_shift >= INFshift )
     output << "(+)";
    else
     if( f_shift <= - INFshift )
      output << "(-)";
     else
      output << " by " << f_shift;
   }
  output << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Vec_p_Var v_vars;       ///< vector of pointers to affected Variable

 Range f_range;          ///< the range of the affected Variable

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionModRngd ) )

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS C05FunctionModSbst -------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe C05Function changes limited to a subset of Variable
/** Derived class from C05FunctionMod, extends it to the concept that the
 * changes to the "complicated" part of the linearization (the vector g) may
 * be localized to some subset of the entries as opposed to involving the
 * whole vector. For this reason it includes a std::vector< Variable * >
 * field (akin to that of FunctionModVars), returned by vars(). Unlike for
 * C05FunctionModRngd, the Variable (whose pointer are) returned by vars()
 * can be "arbitrarily dispersed" among the active ones of the C05Function;
 * thus, the accompanying index information is now a Subset.
 *
 * Apart from this, this Modification behaves exactly as a C05FunctionModRngd,
 * hence see the comments to that class for further details. */

class C05FunctionModSbst : public C05FunctionMod
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: takes all the necessary information
 /** Constructor: besides all the information required by the base
  * C05FunctionMod, it takes the subset of affected Variable under the form
  * of a std::vector< Variable * >, and their (original) indices under the
  * form of a std::vector< Index >. The indices of have the obvious positional
  * correspondence: subset[ i ] is the index that the Variable vars[ i ] had
  * *at the moment in which the C05FunctionModSbst was issued*. As the &&
  * tells, both vectors vars[] and subset[] "becomes property" of the
  * C05FunctionModSbst object. The ordered parameter tells if subset is
  * ordered by increasing Index, which may be helpful for some Block/Solver
  * having to deal with this FunctionModVarsSbst. Indeed, if subset is not
  * "naturally" ordered, then it is ordered in the constructor. Of course,
  * this means that vars gets re-ordered at the same time. */

 C05FunctionModSbst( C05Function * f , int type , Vec_p_Var && vars ,
                     Subset && subset , bool ordered = false ,
                     Subset && which = {} , FunctionValue shift = NaNshift ,
                     bool cB = true )
  : C05FunctionMod( f , type , std::move( which ) , shift, cB ) ,
    v_vars( std::move( vars ) ) , v_subset( std::move( subset ) ) {
  if( ( ! v_subset.empty() ) && ( v_vars.size() != v_subset.size() ) )
   throw( std::invalid_argument( "vars and subset sizes do not match" ) );

  if( ( ! ordered ) && ( ! v_subset.empty() ) && ( v_vars.size() > 1 ) ) {
   using IdxVar = std::pair< Index , Variable * >;
   std::vector< IdxVar > tmp( v_vars.size() );
   for( Index i = 0 ; i < v_vars.size() ; ++i )
    tmp[ i ] = IdxVar( v_subset[ i ] , v_vars[ i ] );
   std::sort( tmp.begin() , tmp.end() ,
              []( auto & a , auto & b ) { return( a.first < b.first ); } );
   for( Index i = 0 ; i < v_vars.size() ; ++i ) {
    v_subset[ i ] = tmp[ i ].first;
    v_vars[ i ] = tmp[ i ].second;
    }
   }
  #ifndef NDEBUG
   for( Index i = 1; i < v_subset.size(); ++i )
    if( v_subset[ i - 1 ] >= v_subset[ i ] )
     throw( std::invalid_argument( "unordered or repeated subset" ) );
  #endif
  }

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~C05FunctionModSbst() override = default;  ///< destructor: does nothing

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the vector of pointers to affected Variable

 [[nodiscard]] c_Vec_p_Var & vars( void ) const { return( v_vars ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to vector of indices of affected Variable

 [[nodiscard]] c_Subset & subset( void ) const { return( v_subset ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the C05FunctionModSbst

 inline void print( std::ostream & output ) const override {
  output << "C05FunctionModSbst[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function [" << &f_function << " ]: in ";
  if( ( f_type == AllEntriesChanged ) ||
      ( f_type == AllLinearizationChanged ) ) {
   if( v_which.empty() )
    output << "all";
   else
    output << v_which.size();
   output << "linearizations";
   if( f_type == AllEntriesChanged )
    output << " the g have changed";
   else
    output << "both \alpha and g have changed";
   }
  else
   output << "[ incorrect type() ]";
  if( f_shift ) {
   output << ", f-values changed";
   if( std::isnan( f_shift ) )
    output << "(+-)";
   else
    if( f_shift >= INFshift )
     output << "(+)";
    else
     if( f_shift <= - INFshift )
      output << "(-)";
     else
      output << " by " << f_shift;
   }
  output << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Vec_p_Var v_vars;  ///< vector of pointers to affected Variable

 Subset v_subset;   ///< vector of indices of the affected Variable

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionModSbst ) )

/*--------------------------------------------------------------------------*/
/*-------------------- Class C05FunctionModVarsAddd ------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe "nicely" adding Variable of a C05Function
/** Derived class from FunctionModVarsAddd to describe adding "active"
 * Variable to a C05Function in such a way that
 *
 *     PREVIOUSLY COMPUTED LINEARIZATION INFORMATION CAN BE SALVAGED
 *
 * This class is actually identical to its base FunctionModVarsAddd; it does
 * not add any information to it. However, by being a different class it
 * allows to encode information regarding the impact that changes in the set
 * of "active" Variable have on the *linearization*, that are specific to
 * C05Function.
 *
 * The point is that, similarly to what is done for function values in the
 * base Function class, it is crucial to define conditions under which
 * previously computed linearizations can be "salvaged" in case of
 * addition/removal of Variable. Quasi-additivity provides a starting point,
 * in the sense that if the Function f_old( x , y ) is quasi-additive on y,
 * and a linearization ( g = [ g_x , g_y ] , \alpha ) is obtained at a point
 * ( x , 0 ), then ( g_x , \alpha ) should remain "valid" for the Function
 * f( x ) with the y Variable removed. Indeed, consider the case where the
 * linearization is the gradient: the component g_i corresponding to the
 * variable x_i is the partial derivative, i.e., the limit for t \to 0 of
 *
 *  [ f_old( x_1 , ... , x_i + t , ... , x_n , y ) - f_old( x , y ) ] / t
 *
 * Clearly, when computed at y = 0 this is equal to (the limit ...)
 *
 *      [ f( x_1 , ... , x_i + t , ... , x_n ) - f( x ) ] / t
 *
 * Note that quasi-additivity allows f_old( x , 0 ) = f( x ) + shift, but
 * clearly the shift cancels out while doing derivatives (although it may
 * influence the \alpha). The same holds when new Variable are
 * quasi-additively added, with the difference that the g_y part was not
 * previously available and has to be computed anew. The "should" in the
 * statement is only related to the fact that we purposely avoid to define
 * what a "valid linearization" exactly is in order to leave flexibility in
 * the usage of C05Function, but it is clear that whatever concept is used
 * to define the linearization should collapse to that of the gradient when
 * f() is smooth and only "exact" linearizations are allowed; therefore,
 * the statement should hold in all cases.
 *
 * However quasi-additivity is useless to re-use information computed at
 * points ( x , y ) with y \neq 0. This makes sense in the case of a
 * Function, where the property only involves the function value: f( x , y )
 * can be arbitrarily different from f( x , 0 ), unless one imposes very
 * strong constraints on f(). However, for linearizations one would like a
 * stronger property to hold:
 *
 *     THE g PART OF THE LINEARIZATION CORRESPONDING TO THE VARIABLES
 *     WHICH SURVIVE REMAINS "VALID" EVEN IF IT HAS BEEN COMPUTED AT A
 *     POINT WITH y \neq 0
 *
 * In other words, when passing from f_old( x , y ) to f( x ), the old
 * linearizations ( g = [ g_x , g_y ] , \alpha ) of f_old() should be able
 * to yield new linearizations ( g = g_x , \alpha' ) even if it has been
 * obtained at a point where y \neq 0. Also,
 *
 *     THE g PART OF THE LINEARIZATION CORRESPONDING TO THE PREVIOUS
 *     VARIABLES REMAINS "VALID" AND IT ONLY NEED TO BE EXTENDED
 *
 * In other words, when passing from f_old( x ) to f( x , y ), the old
 * linearizations ( g = g_x , \alpha ) of f_old() should be able to yield
 * new linearizations ( g = [ g_x , g_y ] , \alpha' ), i.e., to be directly
 * extended by just providing the "missing" part for the new Variable. We
 * call this property STRONG QUASI-ADDITIVITY.
 *
 * A convenient example of this behaviour is, as usual, that of a structured
 * optimization problem
 *
 *   (P)  max \{ c u : A u = b , u \in U \}
 *
 * and of its (convex) Lagrangian function
 *
 *   (P( x ))  f( x ) = max \{ c u + x ( b - A u ) : u \in U \}
 *
 * Linearizations are associated with feasible solutions \bar{u} \in U, as
 *
 *   ( g , \alpha ) = ( b - A \bar{u} , c \bar{u} )
 *
 * Assuming a new block A' u = b' of constraints is added to (P), and relaxed
 * with multipliers y, the function becomes
 *
 *   f( x , y ) = max \{ c u + x ( b - A u ) + y ( b' - A' u ) : u \in U \}
 *
 * with linearizations
 *
 *   ( g = [ g_x , g_y ] , \alpha ) =
 *         ( [ b - A \bar{u} , b' - A' \bar{u} ] , c \bar{u} )
 *
 * The new part g_y = b' - A' \bar{u}  can be easily computed, provided
 * that the information \bar{u} is stored in the global pool, irrespectively
 * to the fact that \bar{u} was obtained or not at a point where y = 0.
 * Similarly, if constraints are removed (which is equivalent to setting the
 * corresponding x_i to 0) the g_y component can be erased without the g_x
 * one ceasing to be "valid", irrespectively to the fact that y be = 0 when
 * the new constraints are added.
 *
 * However, it has to be remarked that the concept of "valid" linearization
 * here is strongly tied to the fact that f() is convex, and therefore that
 * linearizations are globally valid lower approximations. In particular,
 * the above linearizations are epsilon-subgradients: adding/removing some
 * Variable may not change the linearization (besides adding/removing the
 * corresponding entries to g), but it does change the function value and
 * therefore the epsilon. Another, simpler example of this behaviour is the
 * (convex) quasi-additive deletion
 *
 *     f_old( x , y ) = e^( x + y )     and      f( x ) = e^x
 *
 * for which one has 
 *
 *     \nabla f_old( x , y ) = [ e^( x + y ) , e^( x + y ) ]
 *
 *     \nabla f( x ) = [ e^x ]
 *
 * Having computed a linearization at some point ( \bar{x} , \bar{y} ),
 * after the removal of y said linearization is reduced to
 * [ e^(  \bar{x} + \bar{y} ) ]. This is no longer a gradient in \bar{x}
 * (unless, of course \bar{y} = 0), but it is still an epsilon-subgradient
 * there for a proper \epsilon that could be easily computed if one had kept
 * proper information in the global pool.
 *
 * It is unclear if similar tricks can be played without convexity. Indeed,
 * for the quasi-additive but nonconvex deletion
 *
 *     f_old( x , y ) = x e^y    and      f( x ) = x
 *
 * one rather has
 *
 *     \nabla f_old( x , y ) = [ e^y , x e^y ]
 *
 *     \nabla f( x ) = [ 1 ]
 *
 * Save for linearizations computed precisely when y = 0, it is unclear how
 * the first-order information obtained by deleting the g_y component can be
 * of any use.
 *
 * All this is the reason why C05FunctionModVarsAddd exists. Issuing a
 * C05FunctionModVarsAddd is intended to signal that the modification to the
 * C05Function is *strongly* quasi-additive (which likely means that the
 * C05Function is either convex or concave, but this is not fixed in stone).
 * One would expect that strong quasi-additivity implies quasi-additivity,
 * and therefore that shift() is finite and non-NaN, but surely the
 * vice-versa need not be true. If the modification is *not strongly*
 * quasi-additive, the C05Function only has to rather issue a base
 * FunctionModVarsAddd. That is, upon receiving a FunctionModVarsAddd that
 * is *not* a C05FunctionModVarsAddd, a Solver has to assume that all
 * linearizations computed at points ( x , y ) where y \neq 0 have to be
 * considered as completely invalid, even in their g_x part. Of course
 * this means that C05FunctionModVarsAddd have to be "catched" before
 * FunctionModVarsAddd are (since a C05FunctionModVarsAddd will obviously
 * also "register" as a FunctionModVarsAddd). However, if this is true then
 * all the elements in the global pool have clearly became invalid: this
 * need be explicitly made apparent. That is:
 *
 *     IF A C05Function ISSUES A FunctionModVarsAddd THAT IS *NOT* A
 *     C05FunctionModVarsAddd, IMPLYING THAT ALL THE GLOBAL POOL HAS BECOME
 *     INVALID, THEN IT MUST ALSO ISSUE A C05FunctionMod WITH type() ==
 *     GlobalPoolRemoved AND which().empty() TO EXPLICITLY INDICATE THIS.
 *     IT CAN BE EXPECTED THAT THE C05FunctionMod BE ISSUED BEFORE THE
 *     C05FunctionModVarsAddd, AS WORKING ON AN "EMPTY" GLOBAL POOL SHOULD
 *     BE CHEAPER.
 *
 * Issuing a C05FunctionModVarsAddd, which indicates a strongly
 * quasi-additive additions, rather signals that it is possible to update the
 * previously computed linearizations, returned by the C05Function before the
 * modification, provided they are stored in the global pool. Indeed, any
 * g-part of any linearization of the C05Function after the modification has
 * N = v_vars.size() extra entries; these can be retrieved by calls to
 * get_linearization_coefficients(), and in fact this use case is the primary
 * reasons why these methods have the "name" and "indices" parameters.
 * Similarly, for removals, vars() uniquely identifies the N "active"
 * Variable that need be removed, and removing these Variable "simply"
 * correspond to deleting the corresponding entries from all previous
 * linearization vectors. Therefore, unlike for additions, removals can in
 * principle be done without any further input from the C05Function.
 *
 * It still has to be remarked, however, that any Solver (or, in general,
 * Observer) processing this sort of Modification has to be careful. The
 * point is how information about previous linearizations, that has to be
 * updated, is stored. The most "natural" form would be something akin to a
 * std::vector< FunctionValue >; one could then think that the indices of
 * the affected entries could be immediately computed by referring to the
 * information provided by the Modification object (for
 * C05FunctionModVarsAddd, the single first() value). However, one must
 * always keep in mind that these refer to the indices that the Variable
 *
 *     HAD AT THE MOMENT IN WHICH THE C05FunctionModVars* WAS ISSUED, SINCE
 *     WHEN THE C05FunctionModVars* IS PROCESSED, ANY ADDED Variable MAY HAVE
 *     CHANGED INDEX (IF A Variable WITH SMALLER INDEX HAVE BEEN REMOVED) OR
 *     EVEN BEEN REMOVED, AND ANY REMOVED Variable MAY HAVE BEEN RE-ADDED.
 *     THIS MAY HAVE HAPPENED MULTIPLE TIMES (ADDED - REMOVED - RE-ADDED,
 *     RE-REMOVED, ...), ALTHOUGH IN THIS CASE THE APPROPRIATE Modification
 *     MUST BE SITTING IN THE QUEUE AFTER THIS ONE.
 *
 * Yet, this information may still be useful to a Solver which keeps some
 * internal data structures depending on the order of the Variable in the
 * Function, as it then would immediately know where the affected Variable
 * currently are in these. Note that
 *
 *     THE INDEX THAT ANY ADDED Variable HAVE AT THE MOMENT IN WHICH THE
 *     FunctionModVarsAddd IS PROCESSED CAN ONLY BE SMALLER THAN OR EQUAL
 *     TO THAT THAT THE INFORMATION REPORTED HERE IMPLIES, EXCEPT IF A
 *     Variable HAS BEEN DELETED AND RE-ADDED, IN WHICH CASE TWO
 *     APPROPRIATE Modification MUST BE SITTING IN THE QUEUE AFTER THIS ONE.
 *     SIMILARLY, ANY Variable THAT IS DECLARED REMOVED BY SOME Modification
 *     BUT IS CURRENTLY DECLARED "ACTIVE" BY THE C05Function MUST HAVE BEEN
 *     RE-ADDED LATER, IN WHICH CASE AN APPROPRIATE Modification MUST BE
 *     SITTING IN THE QUEUE AFTER THIS ONE. */

class C05FunctionModVarsAddd : public FunctionModVarsAddd
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: identical to that of FunctionModVarsAddd

 C05FunctionModVarsAddd( C05Function * f , Vec_p_Var && vars , Index first ,
                         FunctionValue shift = NaNshift , bool cB = true )
  : FunctionModVarsAddd( f , std::move( vars ) , first , shift , cB ) {}

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~C05FunctionModVarsAddd() override = default;  ///< destructor: does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the C05FunctionModVarsAddd

 void print( std::ostream & output ) const override {
  output << "C05FunctionModVarsAddd[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function [" << f_function
         << " ]: strongly quasi-additively (";

  if( std::isnan( f_shift ) )
   output << "+-(?)";
  else
   if( f_shift >= Inf< FunctionValue >() )
    output << "+(?)";
   else
    if( f_shift <= - Inf< FunctionValue >() )
     output << "-(?)";
    else
     output << f_shift;

  output << ") adding variables [ " << f_first << " , "
         << f_first + v_vars.size() << " ]" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionModVarsAddd ) )

/*--------------------------------------------------------------------------*/
/*-------------------- Class C05FunctionModVarsRngd ------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe "nicely" removing a range of Variable of a C05Function
/** Derived class from FunctionModVarsRngd to describe removing a range of
 * "active" Variable from a C05Function in such a way that
 *
 *     PREVIOUSLY COMPUTED LINEARIZATION INFORMATION CAN BE SALVAGED
 *
 * This class is actually identical to its base FunctionModVarsRngd; it does
 * not add any information to it. However, by being a different class it
 * allows to encode information regarding the impact that changes in the set
 * of "active" Variable have on the *linearization*, that are specific to
 * C05Function. In particular, issuing a C05FunctionModVarsRngd is intended
 * to signal that the modification to the C05Function is *strongly*
 * quasi-additive; see the comments to C05FunctionModVarsAddd. If the
 * modification is *not strongly* quasi-additive, the C05Function only has
 * to rather issue a base FunctionModVarsRngd. That is, upon receiving a
 * FunctionModVarsRngd that is *not* a C05FunctionModVarsRngd, a Solver has
 * to assume that all linearizations computed at points ( x , y ) where
 * y \neq 0 have to be considered as completely invalid, even in their g_x
 * part. Of course this means that C05FunctionModVarsRngd have to be
 * "catched" before FunctionModVarsRngd are (since a C05FunctionModVarsRngd
 * will obviously also "register" as a FunctionModVarsRngd). However, as
 * for adding variables
 *
 *     IF A C05Function ISSUES A FunctionModVarsRngd THAT IS *NOT* A
 *     C05FunctionModVarsRngd, IMPLYING THAT ALL THE GLOBAL POOL HAS BECOME
 *     INVALID, THEN IT MUST ALSO ISSUE A C05FunctionMod WITH type() ==
 *     GlobalPoolRemoved AND which().empty() TO EXPLICITLY INDICATE THIS.
 *     IT CAN BE EXPECTED THAT THE C05FunctionMod BE ISSUED BEFORE THE
 *     FunctionModVarsRngd, AS WORKING ON AN "EMPTY" GLOBAL POOL SHOULD
 *     BE CHEAPER.
 */

class C05FunctionModVarsRngd : public FunctionModVarsRngd
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: identical to that of FunctionModVarsRngd

 C05FunctionModVarsRngd( C05Function * f , Vec_p_Var && vars ,
                         c_Range & range , FunctionValue shift = NaNshift ,
                         bool cB = true )
  : FunctionModVarsRngd( f , std::move( vars ) , range , shift , cB ) {}

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~C05FunctionModVarsRngd() override = default;  ///< destructor: does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the C05FunctionModVarsRngd

 void print( std::ostream & output ) const override {
  output << "C05FunctionModVarsRngd[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function[" << &f_function
         << " ]: strongly quasi-additively (";

  if( std::isnan( f_shift ) )
   output << "+-(?)";
  else
   if( f_shift >= Inf< FunctionValue >() )
    output << "+(?)";
   else
    if( f_shift <= - Inf< FunctionValue >() )
     output << "-(?)";
    else
     output << f_shift;

  output << ") deleting variables [ " << f_range.first << " , "
         << f_range.second << " ]" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionModVarsRngd ) )

/*--------------------------------------------------------------------------*/
/*-------------------- Class C05FunctionModVarsSbst ------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe "nicely" removing a subset of Variable of a C05Function
/** Derived class from FunctionModVarsSbst to describe removing a subset of
 * "active" Variable from a C05Function in such a way that
 *
 *     PREVIOUSLY COMPUTED LINEARIZATION INFORMATION CAN BE SALVAGED
 *
 * This class is actually identical to its base FunctionModVarsSbst; it does
 * not add any information to it. However, by being a different class it
 * allows to encode information regarding the impact that changes in the set
 * of "active" Variable have on the *linearization*, that are specific to
 * C05Function. In particular, issuing a FunctionModVarsSbst is intended to
 * signal that the modification to the C05Function is *strongly*
 * quasi-additive; see the comments to C05FunctionModVarsAddd. If the
 * modification is *not strongly* quasi-additive, the C05Function only has
 * to rather issue a base FunctionModVarsSbst. That is, upon receiving a
 * FunctionModVarsSbst that is *not* a C05FunctionModVarsSbst, a Solver has
 * to assume that all linearizations computed at points ( x , y ) where
 * y \neq 0 have to be considered as completely invalid, even in their g_x
 * part. Of course this means that C05FunctionModVarsSbst have to be
 * "catched" before FunctionModVarsSbst are (since a C05FunctionModVarsSbst
 * will obviously also "register" as a FunctionModVarsSbst). However, as
 * for adding variables
 *
 *     IF A C05Function ISSUES A FunctionModVarsSbst THAT IS *NOT* A
 *     C05FunctionModVarsSbst, IMPLYING THAT ALL THE GLOBAL POOL HAS BECOME
 *     INVALID, THEN IT MUST ALSO ISSUE A C05FunctionMod WITH type() ==
 *     GlobalPoolRemoved AND which().empty() TO EXPLICITLY INDICATE THIS.
 *     IT CAN BE EXPECTED THAT THE C05FunctionMod BE ISSUED BEFORE THE
 *     FunctionModVarsSbst, AS WORKING ON AN "EMPTY" GLOBAL POOL SHOULD
 *     BE CHEAPER.
 */

class C05FunctionModVarsSbst : public FunctionModVarsSbst
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: identical to that of FunctionModVarsSbst

 C05FunctionModVarsSbst( C05Function * f , Vec_p_Var && vars ,
                         Subset && subset , bool ordered = false ,
                         FunctionValue shift = NaNshift , bool cB = true )
  : FunctionModVarsSbst( f , std::move( vars ) , std::move( subset ) ,
                         ordered , shift , cB ) {}

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~C05FunctionModVarsSbst() override = default;  ///< destructor: does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the C05FunctionModVarsSbst

 void print( std::ostream & output ) const override {
  output << "C05FunctionModVarsSbst[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function[" << &f_function
         << " ]: strongly quasi-additively (";

  if( std::isnan( f_shift ) )
   output << "+-(?)";
  else
   if( f_shift >= Inf< FunctionValue >() )
    output << "+(?)";
   else
    if( f_shift <= - Inf< FunctionValue >() )
     output << "-(?)";
    else
     output << f_shift;

  output << ") deleting " << v_subset.size() << " variables" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionModVarsSbst ) )

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS C05FunctionModLin --------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe "linear" modifications specific to a C05Function
/** Linear functions are the simplest class of non-trivial functions: any
 * function can be thought to have a linear component (possibly with all-0
 * coefficients). Besides, they clearly play a fundamental role for a
 * C05Function. It therefore makes sense to offer specific support in 
 * C05Function for "changes in the linear part of the function". This is
 * what this class of Modification is about: it indicates that the value of
 * the Function has changed "in a linear way on a subset of the Variable".
 * This immediately implies a "simple" change in the linearizations.
 *
 * A convenient example of this behaviour is, as usual, that of a structured
 * optimization problem
 *
 *   (P)  max \{ c u : A u = b , u \in U \}
 *
 * and of its (convex) Lagrangian function
 *
 *   (P( x ))  f( x ) = max \{ c u + x ( b - A u ) : u \in U \}
 *
 * Linearizations are associated with feasible solutions \bar{u} \in U, as
 *
 *   ( g , \alpha ) = ( b - A \bar{u} , c \bar{u} )
 *
 * Let I \subset { 1 , ... , n } be a subset of the (indices of the)
 * relaxed constraints (hence, of the Lagrangian variables x_i), and assume
 * that the corresponding constraints A_I u = b_I become A_I u = b'_I; as
 * opposed to the case catered for by C05FunctionModSbst and
 * C05FunctionModRngd, *only the right-hand side changes*. For simplicity let
 * us denote by b' the whole new right-hand side vector, i.e., b'_i = b_i for
 * i \notin I. The change has two effects:
 *
 * - each linearization b - A \bar{u} now becomes b' - A \bar{u}, i.e.,
 *   the *fixed vector* d = b' - b can be used to transform every previously
 *   valid linearization into a new valid linearization (and, of course,
 *   d_i = 0 for i \notin I);
 *
 * - given the value f( \bar{x} ) = c u^* + \bar{x}  ( b - A u^* ) computed
 *   at any point \bar{x}, one can compute the *exact* new value
 *   f( \bar{x} ) = c u^* + \bar{x}  ( b' - A u^* ) by just adding
 *   \bar{x}  ( b' - b ) = \bar{x} d to the old one (and, of course, the
 *   scalar product only need to be computed for i \in I, since d_i = 0 for
 *   i \notin I); this is guaranteed to be the right function value, because
 *   the right-hand side b clearly has no role in the solution of
 *   P( \bar{x} ), and therefore u^* remains an optimal solution to that
 *   problem.
 *
 * In other words, the Lagrangian function can be rewritten as
 *
 *    f( x ) = x b + max \{ ( c - x A ) : u \in U \}
 *
 * i.e., as the sum of the simple linear function x b plus a "complicated
 * one". Again, any function can be thought to have a linear component
 * (at worst, b = 0): this Modification caters for the case where the linear
 * component of f() changes in a given way.
 *
 * The particular form of this Modification takes all the flexibility
 * implied in rule 1. of the Modification [see Modification.h]:
 *
 * 1. A Modification typically says *what* has changed but not (necessarily)
 *    *how*. [...]
 *
 * While in general a Modification would not say "how", this one does: it
 * provides the index set I of the Variable whose "linear part" changes,
 * together with the coefficients d_i = b'_i - b_i (allegedly nonzero). This
 * means that both all function values and all previous linearizations can be
 * immediately updated without making any query to the C05Function, unlike
 * C05FunctionModSbst and C05FunctionModRngd which require calls to
 * get_linearization_coefficients(), thereby re-using all the corresponding
 * information to warm-start whatever algorithm one is using.
 *
 * It should be remarked that this is a FunctionMod, and therefore it has a
 * shift() value. Clearly, the value *can't* be finite, as the value of the
 * shift for two different points \bar{x} and \bar{x}' is d \bar{x} and
 * d \bar{x}', which cannot be always equal. Thus, the expected value of
 * shift() should be NaN, except if the C05Function can infer something on
 * the sign; say, all Variable are non-negative and d >= 0, hence the shift
 * can only be positive and shift() = Inf< FunctionValue >() is appropriate.
 *
 * Finally, note that this is intended as the base class of this kind of
 * Modification, since it provides the pointers but *not* indices of the
 * affected Variable. It is therefore expected that C05Function will not
 * issue Modification of the base class, but rather of the derived ones that
 * also provide index information. Yet, a Solver/Observer not having any use
 * for index information may rather decide to "catch" the base class. */

class C05FunctionModLin : public FunctionMod
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/

 /// constructor: takes all the necessary data
 /** constructor: besides the data for the FunctionMod (a pointer to the
  * affected Function, the shift and the "concerns" value), takes the vector
  * of changes in the linear part of the C05Function under the form of a
  * std::vector< FunctionValue >, and the subset of the Variable whose
  * "linear part" changes under the form of a std::vector< Variable * >. As
  * the && tells, both vectors "become property" of the C05FunctionModLin
  * object. */

 C05FunctionModLin( C05Function * f , Vec_FunctionValue && delta ,
                    Vec_p_Var && vars , FunctionValue shift = NaNshift ,
                    bool cB = true )
  : FunctionMod( f , shift , cB ) , v_vars( std::move( vars ) ) ,
    v_delta( std::move( delta ) ) {
  if( vars.size() != delta.size() )
   throw( std::invalid_argument( "vars and delta sizes do not match" ) );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~C05FunctionModLin() override = default;  ///< destructor: does nothing

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the vector of pointers to affected Variable

 [[nodiscard]] c_Vec_p_Var & vars( void ) const { return( v_vars ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 /// accessor to the "delta" vector

 [[nodiscard]] c_Vec_FunctionValue & delta( void ) const {
  return( v_delta );
  }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the C05FunctionModLin

 void print( std::ostream & output ) const override {
  output << "C05FunctionModLin[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function [" << &f_function
         << " ]: change in the linear part of " << v_delta.size()
         << " variables" << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Vec_p_Var v_vars;           ///< the vector of pointers to affected Variable

 Vec_FunctionValue v_delta;  ///< the vector d = b' - b

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionModLin ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS C05FunctionModLinRngd -----------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe "linear" modifications to a range of Variable
/** Derived class from C05FunctionModLin to describe the case where the
 * Variable (whose pointer is) in vars() are not "arbitrarily dispersed", but
 * correspond to a given range of indices. Hence, the Modification contains a
 * Range (returned by range()) describing it.
 *
 * It is important to remark (again and again) that the range refers to the
 * indices that the Variable
 *
 *     HAD AT THE MOMENT IN WHICH THE C05FunctionModLinRngd WAS ISSUED, SINCE
 *     WHEN THE C05FunctionModLinRngd IS PROCESSED, ANY Variable MAY HAVE
 *     CHANGED INDEX (IF A Variable WITH SMALLER INDEX HAVE BEEN REMOVED) OR
 *     EVEN BEEN REMOVED, ALTHOUGH IN THIS CASE THE APPROPRIATE Modification
 *     MUST BE SITTING IN THE QUEUE AFTER THIS ONE.
 *
 * Yet, this information may still be useful to a Solver which keeps some
 * internal data structures depending on the order of the Variable in the
 * Function, as it then would immediately know where the affected Variable
 * currently are in these. Note that
 *
 *     THE INDEX THAT ANY Variable HAVE AT THE MOMENT IN WHICH THE
 *     C05FunctionModLinRngd IS PROCESSED CAN ONLY BE SMALLER THAN OR EQUAL
 *     TO THAT THAT THE INFORMATION REPORTED HERE IMPLIES, EXCEPT IF A
 *     Variable HAS BEEN DELETED AND RE-ADDED, IN WHICH CASE TWO
 *     APPROPRIATE Modification MUST BE SITTING IN THE QUEUE AFTER THIS ONE.
 *
 * This may simplify the job of the Solver/Observer somewhat. */

class C05FunctionModLinRngd : public C05FunctionModLin
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: takes all the necessary information
 /** Constructor: besides all the information required by the base
  * FunctionModLinRngd, it takes the range identifying the indices of the
  * affected Variable at the moment in which the C05FunctionModLinRngd was
  * issued. The range is a pair of indices ( start , stop ) representing the
  * typical left-closed, right-open range { i : start <= i < stop }, and the
  * correspondence between that and vars is positional: vars[ 0 ] had index
  * range, vars[ 1 ] had index range + 1 ..., which implies that
  * vars.size() == stop - start. */

 C05FunctionModLinRngd( C05Function * f, Vec_FunctionValue && delta,
                        Vec_p_Var && vars, c_Range & range ,
                        FunctionValue shift = NaNshift, bool cB = true )
  : C05FunctionModLin( f , std::move( delta ) , std::move( vars ) , shift ,
                       cB ) , f_range( range ) {
  if( v_vars.size() != f_range.second - f_range.first )
   throw( std::invalid_argument( "vars and range sizes do not match" ) );
  if( v_vars.size() != v_delta.size() )
   throw( std::invalid_argument( "vars and delta sizes do not match" ) );
 }

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~C05FunctionModLinRngd() override = default;  ///< destructor: does nothing

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the range of the deleted Variable

 [[nodiscard]] c_Range & range( void ) const { return( f_range ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the C05FunctionModLinRngd

 void print( std::ostream & output ) const override {
  output << "C05FunctionModLinRngd[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function[" << &f_function
         << " ]: change in the linear part of variables [ "
         << f_range.first << " , " << f_range.second << " ]" << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Range f_range;   ///< the range of the removed Variable

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionModLinRngd ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS C05FunctionModLinSbst -----------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe "linear" modifications to a subset of Variable
/** Derived class from C05FunctionModLin to describe the case where the
 * Variable (whose pointer is) in vars() can be "arbitrarily dispersed"
 * among the active ones of the C05Function; thus, the accompanying index
 * information is now a std::vector< Index >.
 *
 * Apart from this, this Modification behaves exactly as a
 * C05FunctionModLinRngd, hence see the comments to that class for further
 * details. */

class C05FunctionModLinSbst : public C05FunctionModLin
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: takes all the necessary information
 /** Constructor: besides all the information required by the base
  * C05FunctionModLin, it takes the subset identifying the indices of the
  * affected Variable. The indices of have the obvious positional
  * correspondence: subset[ i ] is the index that the Variable vars[ i ] had
  * *at the moment in which the C05FunctionModLinSbst was issued*. As the the
  * && tells, the vector "becomes property" of the C05FunctionModLinSbst
  * object. The ordered parameter tells if subset is ordered by increasing
  * Index, which may be helpful for some Block/Solver having to deal with
  * this FunctionModVarsSbst; indeed, if subset is not "naturally" ordered,
  * it is ordered in the constructor. Of course, this means that both vars
  * and delta get re-ordered at the same time. */

 C05FunctionModLinSbst( C05Function * f, Vec_FunctionValue && delta,
                        Vec_p_Var && vars, Subset && subset,
                        bool ordered = false,
                        FunctionValue shift = NaNshift, bool cB = true )
  : C05FunctionModLin( f, std::move( delta ), std::move( vars ), shift,
                       cB ), v_subset( std::move( subset ) ) {
  if( v_vars.size() != v_subset.size() )
   throw( std::invalid_argument( "vars and subset sizes do not match" ) );
  if( v_vars.size() != v_delta.size() )
   throw( std::invalid_argument( "vars and delta sizes do not match" ) );
  if( ( ! ordered ) && ( v_vars.size() > 1 ) ) {
   using IdxVar = std::tuple< Index , Variable * , FunctionValue >;
   std::vector< IdxVar > tmp( v_vars.size() );
   for( Index i = 0 ; i < v_vars.size() ; ++i )
    tmp[ i ] = IdxVar( v_subset[ i ] , v_vars[ i ] , v_delta[ i ] );
   std::sort( tmp.begin(), tmp.end(),
              []( auto & a, auto & b ) {
               return( std::get< 0 >( a ) < std::get< 0 >( b ) );
               } );
   for( Index i = 0 ; i < v_vars.size() ; ++i ) {
    v_subset[ i ] = std::get< 0 >( tmp[ i ] );
    v_vars[ i ] = std::get< 1 >( tmp[ i ] );
    v_delta[ i ] = std::get< 2 >( tmp[ i ] );
    }
   }
  #ifndef NDEBUG
   else
    for( Index i = 1 ; i < v_subset.size() ; ++i )
     if( v_subset[ i - 1 ] >= v_subset[ i ] )
      throw( std::invalid_argument( "unordered or repeated subset" ) );
  #endif
  }

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~C05FunctionModLinSbst() override = default;  ///< destructor: does nothing

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the subset of the affected Variable

 [[nodiscard]] c_Subset & subset( void ) const { return( v_subset ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the C05FunctionModLinSbst

 void print( std::ostream & output ) const override {
  output << "C05FunctionModLinSbst[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function[" << &f_function
         << " ]: change in the linear part of " << v_subset.size()
         << " variables" << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Subset v_subset;   ///< the subset of the removed Variable

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionModLinSbst ) )

/** @} end( group( C05Function_CLASSES ) ) ---------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* C05Function.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File C05Function.h --------------------------*/
/*--------------------------------------------------------------------------*/
