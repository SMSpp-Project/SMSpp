/*--------------------------------------------------------------------------*/
/*-------------------------- File C05Function.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the C05Function class, which implements a Function that is
 * able to provide *linearizations*, i.e., first-order information. However,
 * the linearizations need not be a continuous function, which means that
 * the Function may be non-smooth.
 *
 * \version 0.20
 *
 * \date 10 - 08 - 2018
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Department of Applied Mathematics \n
 *         State University of Campinas, Brazil \n
 *
 * Copyright &copy by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __C05Function
 #define __C05Function
                      /* self-identification: #endif at the end of the file */

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
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*--------------------- C05Function-RELATED TYPES --------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup C05Function_TYPES C05Function-related types.
 *  @{ */

/*@}  end( group( C05Function_TYPES ) ) */
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
 * A linearization is defined as a pair formed by a real vector g and a real
 * number \alpha. The dimension of the vector g is equal to the size of the
 * input space of the Function, i.e., the number of its active Variable (for
 * short, "n" in all comments of the class unless otherwise specified).
 * Linearization are therefore objects in the graphical space \R^{n + 1}
 * of pairs ( x , v ), where x belongs to the input space \R^n and v \in R is
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
 *   (P(x))  f(x) = max \{ c u + x ( b - A u ) : u \in U \}
 *
 * Given x, any optimal solution u^* of P(x) provides a linearization
 *
 *    ( g , \alpha ) = L( u ) = ( b - A u^* , c u^* + x ( b - A u^* ) )
 *
 * which is clearly the "best possible" linearization:
 *
 * - it supports Gr(f) from below: f(y) >= \alpha + g ( y - x ) for all y;
 *
 * - it is "active" at x: \alpha = f(x);
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
 * In particular, there is usually "one important convex combination" that
 * is very relevant for algorithmic purposes. This can be clearly seen in
 * the Lagrangian function example: for a point x^* to be \eps-optimal for
 * the mininization of f (the Lagrangian Dual), one must collect a set of
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
 * properties. It may be very useful to be able to store away this object
 * in case the Function (or other parts of the Block) changes, in order to
 * provide effective reoptimization. For instances, the changes may be such
 * that x^* may nonetheless remain an optimal solution, and the availability
 * of the "important linearization" may allow to prove this with very little
 * computational effort. This is why the interface has specific provisions
 * for producing and storing in the global pool this kind of objects.
 *
 * A further important detail is that there can be two types of
 * linearizations: "diagonal" and "vertical". Diagonal linearizations are
 * the previously illustrated ones; the names comes from picturing the
 * ( x , v ) space \R^{n+1} as having the x component on the horizontal
 * axis and the v component as the vertical one (the typical arrangement
 * for the graphical space of a function). Then, a linearization
 * ( g , \alpha ) typically corresponds to a line approximating f in the
 * neighbourhood of some x. If, say, f is convex, this corresponds to a
 * linear constraint
 *
 *       ( 1 , - g ) ( v , x ) >= \alpha       (*)
 *
 * which is globally correct for the graph of f (actually, the epigraph).
 * The corresponding line in the ( x , v ) space either intersects the x axis
 * "diagonally" or is parallel to it (if g = 0), but it can never be
 * hortogonal to it. Thus, there is an entirely different form of lines in
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
 * with the same property, i.e., that the constraint  c v + x ( A v ) <= 0
 * is valid for the (epi)graph of f. Since v is typically constructed by
 * whatever algorithm is used to solve P(x) in order to prove that it is
 * unbounded above, the corresponding vertical linearization of the form (**)
 * can be returned by the C05Function. Note that, customarily, a solution
 * algorithm would also compute a feasible solution u \in U together with v,
 * and therefore can return both a diagonal and a vertical linearization,
 * which the interface allows. In the parlance of Benders' decomposition,
 * diagonal linearizations are "optimality cuts" and vertical linearizations
 * are "feasibility cuts". However, the concept is possibly general. */

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

 typedef Eigen::SparseVector<FunctionValue> SparseVector;
 ///< type used to store a sparse vector

 typedef unsigned int LinearizationName;
 ///< type used to define names of linearizations

 typedef std::vector< std::pair < LinearizationName , FunctionValue > >
 LinearCombination;
 ///< type used to define linear combinations of linarizations

/*--------------------------------------------------------------------------*/
 /// public enum for the int algorithmic parameters of C05Function
 /** Public enum describing the different parameters of "int" type that a
  * C05Function must have (although specific Function may choose to ignore
  * some of them). The value intLastParC0F is provided so that the list can
  * be easily further extended by derived classes. */

 enum int_par_type_C0F {
 intLPMaxSz = Function::intLastParFun ,
 ///< maximum size of the "local pool"
 /**< The algorithmic parameter for setting the size of the "local pool",
  * that is, the maximum number of linearizations that should be stored in
  * the local pool. The default is 1, which corresponds to the fact that the
  * Function can only produce a single Linearization at a time (for it is,
  * say, smooth). */

 intGPMaxSz ,  ///< maximum size of the "global pool"
               /**< The algorithmic parameter for setting the size of the 
		* "global pool", that is, the maximum number of
 * linearizations that should be stored in the local pool. The default is
 * 1, which corresponds to the fact that the Function need not store any
 * more that a single Linearization at a time (for it is, say, smooth). */

 intLastParC0F   ///< first allowed new int parameter for derived classes
                 /**< Convenience value for easily allow derived classes
		  * to extend the set of int algorithmic parameters. */
 };  // end( int_par_type_C0F )

/*--------------------------------------------------------------------------*/
 /// public enum for the double algorithmic parameters of C05Function
 /** Public enum describing the different parameters of "double" type that a
  * C05Function must have (although specific C05Function may choose to ignore
  * some of them). The value dblLastParC0F is provided so that the list can
  * be easily further extended by derived classes. */

 enum dbl_par_type_C0F {
 dblRAccLin = Function::dblLastParFun ,
 ///< maximum relative error in any linearization
 /**< The parameter for setting the relative accuracy of the linearizations.
  * A linearization ( g , \alpha ) computed at the point x is "accurate" if
  * the value of the linearization coincides with the value of the function
  * at x, i.e., alpha = f(x). In general linearizations that are not
  * "completely accurate" can still be useful: for instance, in the Lagrangian
  * case an \eps-optimal solution to the Lagrangian problem gives rise to a
  * valid linearization ( g , \alpha ) with \eps >= f(x) - \alpha. This can be
  * deemed interesting if \eps is "small", but not if \eps is "large". This
  * parameter instructs the C05Function not to bother reporting (and therefore
  * storing in the "local pool") any linearization having a relative error
  * with f(x) larger than dblRAccLin. This would generally mean
  *
  *  | f(x) - \alpha | <= dblRAccLin * max( | f(x) | , | \alpha | , 1 )
  *
  * except that the value f(x) may not be known exactly, with only lower
  * and/or upper bounds on it available. The actual formula therefore depends
  * on what information is actually available: for instance, in the Lagrangian
  * case one knows that f(x) >= \alpha, and therefore typically un upper
  * estimate ub >= f(x) is used in the formula instead of f(x). The default is
  * 0, i.e., "only perfect linearizations are allowed". */

 dblAAccLin ,   ///< maximum absolute error in any reported solution
		/**< Similar to dblRAccLin but for an *absolute* accuracy;
                  * that is, a linearization is deemed acceptable if
  *
  *      | f(x) - \alpha | <= dblAAccLin
  *
  * except that the value f(x) may not be known exactly, with only lower
  * and/or upper bounds on it available. The actual formula therefore depends
  * on what information is actually available: for instance, in the Lagrangian
  * case one knows that f(x) >= \alpha, and therefore typically un upper
  * estimate ub >= f(x) is used in the formula instead of f(x). The default is
  * 0, i.e., "only perfect linearizations are allowed". */

 dblLastParC0F   ///< first allowed new double parameter for derived classes
                 /**< Convenience value for easily allow derived classes
		  * to extend the set of double algorithmic parameters. */
 };  // end( dbl_par_type_F )


/*@}------------------------------------------------------------------------*/
/*--------------- CONSTRUCTING AND DESTRUCTING C05Function -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing C05Function
 *  @{ */

 /// constructor of C05Function: does nothing
 C05Function() : Function() { }

/*--------------------------------------------------------------------------*/
 /// destructor: it is virtual, and empty
 virtual ~C05Function() { }

/*@} -----------------------------------------------------------------------*/
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
  * Once "the first" linearization (if ever) has been read, new ones may be
  * produced, if the Function allows it, by means of
  * compute_new_linearization().
  *
  * The default implementation in the base class returns the same value as
  * diagonal, i.e., true if diagonal == true and false otherwise. This is
  * OK, for instance, for a, say, algebraic Function that is finite and
  * continuously differentiable everywhere, so that the linearization
  * (gradient) is well-defined and always easily computable as soon as the
  * value of the active Variable is set. */

 virtual bool has_linearization( const bool diagonal = true )
 {
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

 virtual bool compute_new_linearization( const bool diagonal = true )
 {
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
  * times over. */

 virtual void store_linearization( const LinearizationName name ) { }

/*--------------------------------------------------------------------------*/
 /// stores a convex combination of the given linearizations
 /** This method creates a convex combination of a given set of
  * linearizations, with given coefficients, and stores it (or information
  * allowing to compute it) into the global pool of linearizations with the
  * given name.
  *
  * The linearizations whose convex combination the new one must be created
  * from, together with the corresponding coefficients, are indicated by 
  * vector of pairs. Each pair has the name of a linearization (that must be
  * currently stored in the global pool of linearizations) and the
  * coefficient this linearization must have in the convex combination. Once
  * this convex combination is created, it is stored into the global pool
  * with the name passed as argument. If the new linearization is stored
  * under the name of a previously stored linearization, the latter is
  * replaced with the former.
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
  * as they are. This may allow, for instance, to reduce the maximum size of
  * the global pool by replacing many linearizarizations by just one
  * "representing them all" (think conjugate subgradients).
  *
  * Note that the C05Function may freely decide to either store the
  * convexified linearization itself, or information allowing to compute it,
  * or both. For instance, in the Lagrangian case, any linearization g_i
  * corresponds to a point u_i in U. Hence, the convexified linearization
  * corresponds to a point in conv( U ); if U is convex then that point is
  * still in U, otherwise it is a point in its convex hull which can still
  * have numerous algoroithmic uses.
  *
  * The method has a default empty implementation as some Function may
  * ignore this information because it can perform the other required
  * actions [see get_linearization_coefficients()] by other means. In
  * particular, a linear function always have the same linearization
  * everywhere, and therefore any convex combination of these always returns
  * the same vector. */

 virtual void store_convex_combination_of_linearizations(
	  LinearCombination coefficients , const LinearizationName name ) { }

/*--------------------------------------------------------------------------*/
 /// specify which linearization is "the important one"
 /** This method sets the linearization with the given name as "the important
  * one". A linearization with the given name should be stored in the global
  * pool of linearizations, otherwise an exception may be thrown (unless, for
  * instance, the concept is completely ignored, see below).
  *
  * There is usually "one important convex combination" that is very relevant
  * for algorithmic purposes. This can be clearly seen in the Lagrangian
  * function example: for a point x^* to be \eps-optimal for the mininization
  * of f (the Lagrangian Dual), one must collect a set of \eps-subgradients
  * g_i such that 0 \in conv( { g_i } ). It is immediate to realize that this
  * corresponds to a set of \eps-optimal solutions u_i to P(x^*) such that
  * u^* = \sum_i u_i \theta_i, for appropriate convex multipliers \theta_i,
  * is such that A u^* = b. If U in (P) is a convex set then such an u^* is
  * an optimal solution to (P), otherwise u^* is the optimal solution of the
  * relaxation of (P) substituting U with conv( U ). Any reasonable algorithm
  * for solving the Lagrangian Dual should be able to conceptually produce
  * such an object in order to stop; when this is done, u^* can be
  * *explicitly* produced by calling
  * store_convex_combination_of_linearizations() with the appropriate
  * multipliers, and then be available for algorithmic purposes. For
  * instance, u^* can be used to to separation of constraints (say, in case
  * the A u = b ones are very many, so that an active-set strategy is
  * necessary), or to guide heuristics or branching operations (if the set U
  * included integrality constraints, so that the Lagrangian Dual of (P) is
  * only a relaxation).
  *
  * More in general, proving optimality/stationariety of some x^* involves
  * constructing one convex combination of linearizations with appropriate
  * properties. It may be very useful to be able to store away this object
  * in case the Function (or other parts of the Block) changes, in order to
  * provide effective reoptimization. For instances, the changes may be such
  * that x^* may nonetheless remain an optimal solution, and the availability
  * of the "important linearization" may allow to prove this with very little
  * computational effort. This is why saving the "important linearization"
  * when the algorithm is finished (but, possibly, even while it is running)
  * may be useful.
  *
  * This method has a default empty implementation as some Functions may not
  * need to store linearizations, in which case the name is irrelevant. In
  * particular, a linear function always have the same linearization
  * everywhere, and therefore only one well-known linearization can be the
  * "important one". */

 virtual void set_important_linearization( LinearizationName name ) { }

/*--------------------------------------------------------------------------*/
 /// return the name of "the important linearization"
 /** This method returns the name of "the important linearization". It has a
  * default empty implementation, returning 0, as some Functions may not
  * need to store linearizations, in which case the name is irrelevant. */

 virtual LinearizationName get_important_linearization_name( void ) {
  return( 0 );
  }

/*--------------------------------------------------------------------------*/
 /// rename a linearization that is stored in the global pool
 /** This method renames a linearization that is stored in the global pool of
  * linearizations. current_name is the current name of the linearization,
  * and new_name is its new name; names must be integers between 0 and
  * intGPMaxSz - 1. If there is no linearization stored with the name
  * current_name in the global pool, an exception should be thrown (unless,
  * for instance, the concept is completely ignored). If there is already a
  * linearization stored with the name new_name, it will be replaced with the
  * linearization currently named current_name.
  *
  * The method can be useful, e.g., to move linearizations in the initial
  * part of the global pool, that with "small" names", before shrinking it.
  *
  * The method has a default emptyimplementation as some Functions may not
  * store anything. */

 virtual void rename_linearization( const LinearizationName current_name ,
				    const LinearizationName new_name ) { }

/*--------------------------------------------------------------------------*/
 /// delete the given linearization from the global pool of linearizations
 /** This method deletes the linearization associated with the given name
  * from the global pool of linearizations. If there is no linearization
  * associated with the given name, an exception should be thrown (unless,
  * for instance, the concept is completely ignored). Indeed, the method has
  * a default empty implementation as some Functions may not need to store
  * anything. */

 virtual void delete_linearization( const LinearizationName name ) { }

/*--------------------------------------------------------------------------*/
 /// retrieve the coefficients (g vector) of a linearization in a vector
 /** This method retrieves the vector of coefficients g that is the (largest)
  * part of the linearization with the given name.
  *
  * If the name of the linearization is the default value
  * Inf<LinearizationName>(), then it refers to the last computed
  * linearization, which may "not yet have a name" because
  * store_linearization() may not have been called yet (and it may never be,
  * if this linearization is not deemed "important enough" to be kept in the
  * global pool). Otherwise, it (obviously) refers the linearization
  * associated with the given name from the global pool of linearizations.
  * If a linearization with the given name is not stored in the global pool,
  * an exception may be thrown (unless, for instance, the concept is
  * completely ignored because, say, the Function is a linear one and
  * therefore all linearizations are the same). 
  *
  * It is possible to retrieve the whole vector of coefficients or only part
  * of it. The parameters indices, start, and end are used to indicate which
  * components of the linearization vector should be obtained. The components
  * of a linearization are numbered from 0 to n - 1, where
  * n = get_num_active_var() is the number of active Variables of this
  * Function. Moreover, the i-th component of a linearization is associated
  * with the i-th active Variable of this Function.
  *
  * The returned components are those whose indices are contained in the
  * parameter indices *and* in the half-closed interval [start, end). Each
  * element stored in the vector pointed by indices (if any) must be an
  * integer between 0 and n - 1, and the parameters start and end must be
  * such that 0 <= start < end.
  *
  * This is the "rough version" of the method where the output is directly in
  * an array. If indices is null, then all components between start and
  * min( n , end ) - 1 will be stored in the array g in the positions from 0
  * to min( n , end ) - 1 - start. In other words, component i of the
  * linearization vector will be stored in position i - start of the array g.
  * Otherwise, if indices is not null, then only the components indicated by
  * indices *and* which are between start and end - 1 will be stored in g.
  * For example, if start = 2, end = 8, and the vector pointed by indices
  * stores the numbers 0, 3, 7, and 9, then only the components 3 and 7 will
  * be stored in g (respectively in g[ 0 ] and g[ 1 ]), leaving the other
  * ones in g unchanged.
  *
  * The rationale for having such a "rough" version is that it allows to
  * "extend linearizations already in place". For instance, assume that
  * whatever is using this C05Function has stored the linearizations in the
  * global pool as a 
  * 
  *   std::vector < std::vector< FunctionValue > > G;
  *
  * wherethe inner vectors G[ i ] are dimensioned to the *maximum* number of
  * Variable that the C05Functio may have, and that a FunctionModVars occurs
  * which adds k more variables, say [ n , ... , n + k - 1 ). Then, the new
  * entries of all the linearizations corresponding to these new Variable
  * can be written in place in the existing vectors by just calling
  *
  *   get_linearization_coefficients( G[ i ].data() , i , null , n , k )
  *
  * for all i. */

 virtual void get_linearization_coefficients( FunctionValue *g ,
      const LinearizationName name =
			     std::numeric_limits<LinearizationName>::max() ,
      c_Vec_Index * const indices = nullptr , c_Index start = 0 ,
      c_Index end = std::numeric_limits<Index>::max() ) = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// retrieve the coefficients (g) of a linearization in a sparse vector
 /** This method retrieves the sparse vector of coefficients g that is part
  * of a linearization. It works like
  *
  *   get_linearization_coefficients( FunctionValue *g , ... )
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
  * component i of the linearization vector that is desired (components
  * determined by the indices vector, and start and end parameters), the
  * value of this component will be stored in g and any previously stored
  * value in g for component i will be lost. If vector g stores a value for 
  * a component j that is not among the ones desired, this will be left
  * unchanged in the vector. It is important to notice that the operation of
  * updating the given sparse vector may be computationally costly; in order
  * to avoid this, consider passing to this method an empty g.
  *
  * TODO: default implementation of this using the other one???
  */

 virtual void get_linearization_coefficients( SparseVector &g ,
      const LinearizationName name =
                               std::numeric_limits<LinearizationName>::max() ,
      c_Vec_Index * const indices = nullptr , c_Index start = 0 ,
      c_Index end = std::numeric_limits<Index>::max() ) const = 0;

/*--------------------------------------------------------------------------*/
 /// return the constant term of a linearization
 /** This method returns the constant term (alpha) of a linearization. If the
  * name of the linearization is the default value Inf<LinearizationName>(),
  * then it refers to the last computed linearization, which many "not yet
  * have a name" because store_linearization() may not have been called yet
  * (and it may never be, if this linearization is not deemed "important
  * enough" to be kept in the global pool). Otherwise, it (obviously) refers
  * the linearization associated with the given name from the global pool of
  * linearizations. If a linearization with the given name is not stored in
  * the global pool, an exception may be thrown (unless, for instance, the
  * concept is completely ignored because, say, the Function is a linear one
  * and therefore all linearizations are the same).
  *
  * Note that the method can be queried, after that an appropriate 
  * Modification has been issued [see C05FunctionMod], to get the *new* value
  * of \alpha for each of the linearizations stored in the global pool, which
  * may allow reoptimization to be performed. In this case, a linearization
  * may have become invalid: this is signaled by returning
  * Inf<FunctionValue>() as the correspondong \alpha, which might prompt the
  * linearization to be removed from the global pool (but at least is should
  * warn whatever algorithm is using the Function not to use it any longer). 
  */

 virtual double get_linearization_constant( const LinearizationName name =
	    std::numeric_limits<unsigned int>::max() ) const = 0;

/*--------------------------------------------------------------------------*/
 /// returns true if and only if this Function is continuously differentiable
 /** Method that returns true if and only if this Function is continuously
  * differentiable. The default is false. Note that a continuously
  * differentiable function, when called with the default value of the
  * accuracy parameters dblRAccLin and dblAAccLin, should only return one
  * single linearization per compute(), as the only exact linearization is
  * the gradient, which is unique. */

 virtual bool is_continuously_differentiable( void ) const {
  return( false );
  }

/*@} -----------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the Function
 *  @{ */

 virtual idx_type get_num_int_par( void ) const override
 {
  return( intLastParC0F );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual idx_type get_num_dbl_par( void ) const override
 {
  return( dblLastParC0F );
  }

/*--------------------------------------------------------------------------*/
 
 virtual int get_dflt_int_par( const idx_type par ) const override
 {
  if( ( par == intLPMaxSz ) || ( par == intGPMaxSz ) )
   return( 1 );
  else
   return( Function::get_dflt_int_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 
 virtual double get_dflt_dbl_par( const idx_type par ) const override
 {
  if( ( par == dblRAccLin ) || ( par == dblAAccLin ) )
   return( 0 );
  else
   return( Function::get_dflt_dbl_par( par ) );
  }

/*--------------------------------------------------------------------------*/

 virtual idx_type int_par_str2idx( const std::string & name ) const override
 {
  if( name == "intLPMaxSz" )
   return( intLPMaxSz );
  if( name == "intGPMaxSz" )
   return( intGPMaxSz );

  return( Function::int_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual idx_type dbl_par_str2idx( const std::string & name ) const override
 {
  if( name == "dblRAccLin" )
   return( dblRAccLin );
  if( name == "dblAAccLin" )
   return( dblAAccLin );

  return( Function::dbl_par_str2idx( name ) );
  }

/*--------------------------------------------------------------------------*/

 virtual const std::string & int_par_idx2str( const idx_type idx )
  const override
 {
  static const std::vector< std::string > pars =
   { "intLPMaxSz" , "intGPMaxSz" };

  if( ( idx >= intLPMaxSz ) && ( idx <= intGPMaxSz ) )
   return( pars[ idx - intGPMaxSz ] );
  else
   return( Function::int_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual const std::string & dbl_par_idx2str( const idx_type idx )
  const override
 {
  static const std::vector< std::string > pars =
   { "dblRAccLin" , "dblAAccLin" };

  if( ( idx >= dblRAccLin ) && ( idx <= dblAAccLin ) )
   return( pars[ idx - dblRAccLin ] );
  else
   return( Function::dbl_par_idx2str( idx ) );
  }

/*@}------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

  protected:

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
    @{ */

 /// print information about the C05Function on an ostream
 /** Protected method intended to print information about the C05Function; it
  * is virtual so that derived classes can print their specific information
  * in the format they choose. */

 virtual void print( std::ostream &output ) const override {
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
 * - AlphaChanged
 *
 * This type of modification states that the constant (\alpha) of the
 * linearizations has changed. Note, however, that this does *not* mean that
 * *all the \alpha have changed in the same way*; this happens if the whole
 * function is shifted by a constant, which is what the original FunctionMod
 * is intended to convey. Rather, the idea is that each individual \alpha has
 * changed in its own way, *but all the rest has remained unchanged* (unless 
 * other Modification say otherwise). The idea is that it is possible to query
 * the value of \alpha for all the linearizations that were stored in the
 * global pool [see get_linearization_constant()], thereby re-using all the
 * corresponding information to warm-start whatever algorithm one is using.
 *
 * A case where this happens is the Lagrangian one, where each linearization
 * is attached to one solution u_i \in U, and:
 *
 * - either the objective function c of (P) changes;
 *
 * - or U itself changes to some U' \neq U.
 *
 * In the first case, having stored u_i in the global pool it is immediate to
 * compute the new value of \alpha. In the second case, each u_i is either
 * feasuble (u_i \in U') or not (u_i \notin U'). In the first case \alpha
 * does not change, but in the second the original linearization (even the g
 * part) can no longer be used, as it is no longer valid. This can be handled
 * by having get_linearization_constant() returning Inf<FunctionValue>() for
 * the corresponding \alpha.
 *
 * - AllEntriesChanged
 *
 * This type of modification states that all entries of previously computed
 * linearizations (the g part) may have changed, *but all the rest has
 * remained unchanged* (unless other Modification say otherwise). It is
 * then possible to query the new values of the linearizations that were
 * stored in the global pool [see get_linearization_coefficients()], thereby
 * re-using all the corresponding information to warm-start whatever
 * algorithm one is using.
 *
 * - AllLinearizationChanged
 *
 * This type of modification is the logical union of both previous types:
 * for all of previously computed linearizations, both the g part and the
 * \alpha may have changed, *but all the rest has remained unchanged* (unless
 * other Modification say otherwise). It is then possible to query the new
 * values of the linearizations that were stored in the global pool using the
 * available methods as in the two previous cases.
 *
 * A case where this happens is the Lagrangian one, where each linearization
 * is attached to one solution u_i \in U, and the whole set of complicating
 * constraints A u = b abruptly changes, whereas all the rest of (P) remains
 * the same. Having stored the u_i in the global pool it is immediate to
 * compute the new values of g and \alpha. For more "localized" changes of
 * the complicating constraints see C05FunctionModVars. */

class C05FunctionMod : public FunctionMod {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/

  /// Definition of the possibles type of C05FunctionMod
  enum c05function_mod_type {
   AlphaChanged ,             ///< all the \alpha have changed
   AllEntriesChanged ,        ///< all the g have changed
   AllLinearizationChanged ,  ///< both \alpha and g have changed
   C05FunctionModLastParam
   ///< First allowed parameter value for derived classes
   /**< Convenience value for easily allow derived classes to extend
    * the set of types of modifications. */
   };

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: takes the type of Modification and a C05Function pointer
 /** Constructor: takes the type of the Modification and a pointer to
  * the affected C05Function. Note that while the enum
  * c05function_mod_type is provided to encode the possible values of
  * modification, the field f_type is of type "int", and therefore so
  * is the parameter of the constructor, in order to allow derived
  * classes to "extend" the set of possible types of modifications. */

 C05FunctionMod( C05Function * const f , const int mod ,
		 const FunctionValue shift = 0 , const bool cB = true )
  : FunctionMod( f , shift , cB ), f_type( mod ) { }

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~C05FunctionMod() { }  ///< destructor: does nothing

/*----------------------- PUBLIC FIELDS OF THE CLASS -----------------------*/

  int f_type; ///< type of modification

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
  /// print the C05FunctionMod

  virtual inline void print( std::ostream &output ) const {
   output << "C05FunctionMod[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function [" << &f_function << " ]: ";
   switch( f_type ) {
    case( AlphaChanged ): output << "all the \alpha"; break;
    case( AllEntriesChanged ): output << "all the g"; break;
    default: output << "both \alpha and g";
    }
   output << " have changed" << std::endl;
   }

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionMod ) )

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS C05FunctionModVarsRngd ------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe changes to a C05Function involving a range of Variable
/** Derived class from FunctionModVarsRngd to describe changes specific to a 
 * C05Function. This class defines the following type of modification:
 *
 * - SomeEntriesChange
 *
 * "extending" function_mod_variables_type, and, basically, does nothing else.
 * 
 * It is, however, important to extensively comment how the information in
 * this Modification relates to changes in the *linearization*, that are
 * specific to C05Function. The point is simply that each entry of any
 * linearization vector is uniquely associated to a Variable, and therefore
 * the "name = pointer" of the Variable can be used as an "index" in the
 * linearization vector. Therefore, the the f_strt and f_stop fields of the
 * FunctionModVarsRngd can be transformed into indices in the linearization
 * vector by means of ThinVarDepInterface::is_active().
 *
 * In particular, SomeEntriesChange indicates that some entries of *all*
 * previously computed linearizations may have changed and must be updated in
 * order for them to reflect linearizations of the current function. The
 * (Variable names corresponding to the) indices of the components of the
 * linearization vector that must be recomputed are all those in the range
 * specified by the FunctionModVarsRngd. It is then possible to query the new
 * values of the corresponding entries of the linearizations that were stored
 * in the global pool [see get_linearization_coefficients()], thereby
 * re-using all the corresponding information to warm-start whatever
 * algorithm one is using.
 *
 * A case where this happens is the Lagrangian one, where each linearization
 * is attached to one solution u_i \in U, and *some* of the complicating
 * constraints A u = b abruptly changes, whereas all the rest of (P) remains
 * the same. Having stored the u_i in the global pool it is immediate to
 * compute the new values of g for the appropriate entries.
 *
 * Note that if all the entries change, this can be somehow more clearly and
 * succinctly signaled by issuing a C05FunctionMod with type
 * AllEntriesChanged (or AllLinearizationChanged); however, this is the same
 * as a FunctionModVarsRngd with strt == stop == nullptr.
 *
 * Since this Modification derives from FunctionModVars, it also in principle
 * supports the
 *
 * - AddVar
 * - RemoveVar
 *
  * types. Indeed, this makes full sense for the removal: the provided range
  * uniquely identifies the N "active" Variable that need be removed, which
  * obviously correspond to a contiguous range of N entries in the
  * linearization vectors, and removing these Variable "simply" correspond to
  * cutting away that range from all previous linearization vectors; this can
  * be done without any further input from the C05Function. Note that, of
  * course,
  *
  *    THIS ONLY HOLDS IF THE C05Function IS QUASI-ADDITIVE
  *
  * (see comments to FunctionModVars), which is signalled by the value of
  * shift. If shift == Inf<FunctionValue>(), one has to assume that all the
  * entries of the previous linearizations have changed.
  *
  * A case where this happens is the Lagrangian one, where each linearization
  * is attached to one solution u_i \in U, in the sense that the vector is
  * g_i = b - A u_i. Removing one complicating constraints A_h u = b_h
  * simply corresponds to removing the h-th row of g_i.
  *
  * However, this form of Modification does not lend itself well to
  * handling the *addition* of N Variable. For this to be done, the
  * pointers to all new Variable need be retrieved: the only way to do this
  * with range information is to assume that both strt and stop belong to an
  * array of Variable, so that all the ones "between" are added. However,
  * this would require knowing *exactly which type of Variable* these are.
  * Although the possibility is left open for some specific Function to use,
  * it is not possible to use C05FunctionModVarsRngd in full generality to
  * handle addition of Variable, even if the Function is quasi-additive. */

class C05FunctionModVarsRngd : public FunctionModVarsRngd
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/

 /// definition of the possible type of C05FunctionModVarsRngd
 enum c05function_mod_variables_type {
  SomeEntriesChange = FunctionModVarsLastParam ,
  ///< some entries of previously computed linearizations may have changed
  C05FunctionModVarsLastParam
  ///< first allowed parameter value for derived classes
  /** Convenience value for easily allow derived classes to extend the set of
   * types of modifications. */
  };

/*-------------------- CONSTRUCTOR AND DESTRUCTOR --------------------------*/

 /// constructor: identical to that of FunctionModVarsRngd

 C05FunctionModVarsRngd( Function * const f , const int mod ,
			 Variable * const strt = nullptr ,
			 Variable * const stop = nullptr ,
			 const FunctionValue shift = 0 , const bool cB = true )
  : FunctionModVarsRngd( f , mod , strt , stop , shift , cB ) { }

 /// destructor: does nothing
 virtual ~C05FunctionModVarsRngd() { }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the C05FunctionModVarsRngd

 virtual inline void print( std::ostream &output ) const {
  output << "C05FunctionModVarsRngd[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function [" << f_function << " ]: ";
  switch( f_type ) {
   case( AddVar ): output << "add variables"; break;
   case( RemoveVar ): output << "delete variables"; break;
   case( SomeEntriesChange ):  output << "some entries change";
   }
  output << "[ " << f_strt << ", " << f_stop << "]" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionModVarsRngd ) )

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS C05FunctionModVarsSbst ------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe changes to a C05Function involving a subset of Variable
/** Derived class from FunctionModVarsSbst to describe changes specific to a 
 * C05Function. This class is (almost) completely analogous to
 * C05FunctionModVarsRngd, up to defining the identical type of modification
 *
 * - SomeEntriesChange
 *
 * "extending" function_mod_variables_type, and doing nothing else. However,
 * the set of indices of the linearization vectors affected by the change
 * are now provided by the v_vars std::vector of Variable *. These can be,
 * if needed, (hopefully, efficiently) translated into indices of the
 * linearization vector by means of ThinVarDepInterface::map_active().
 *
 * There is one relevant difference with C05FunctionModVarsRngd, though;
 * because it provides a full set of Vector *, this method can reasonably
 * be used with type AddVar. In this case, any linearization vector of the
 * Function after the modification has N extra components. It is possible
 * to update a previously computed linearization (of the old pre-modification
 * Function) stored in the global pool by computing the N new entries of the
 * linearization associated with the new variables; this is allowed by
 * get_linearization_coefficients(), and in fact this use case is *exactly*
 * why these methods have the "name" parameter. Of course,
  *
  *    THIS ONLY HOLDS IF THE C05Function IS QUASI-ADDITIVE
  *
  * (see comments to FunctionModVars), which is true for instance for
  * Lagrangian functions and is signalled by the value of shift. If shift
  * == Inf<FunctionValue>(), one has to assume that all the entries of the
  * previous linearizations have changed when new Variable are added. */

class C05FunctionModVarsSbst : public FunctionModVarsSbst
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/

 /// definition of the possible type of C05FunctionModVarsSbst
 enum c05function_mod_variables_type {
  SomeEntriesChange = FunctionModVarsLastParam ,
  ///< some entries of previously computed linearizations may have changed
  C05FunctionModVarsLastParam
  /**< first allowed parameter value for derived classes Convenience value
   * for easily allow derived classes to extend the set of types of
   * modifications. */
  };

/*-------------------- CONSTRUCTOR AND DESTRUCTOR --------------------------*/

 /// constructor: identical to that of FunctionModVarsSbst

 C05FunctionModVarsSbst( Function * const f , const int mod ,
			 std::vector<Variable *> && vars ,
			 const FunctionValue shift = 0 ,
			 const bool cB = true )
  : FunctionModVarsSbst( f , mod , std::move( vars ) , shift , cB ) { }

 /// destructor: does nothing
 virtual ~C05FunctionModVarsSbst() { }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the C05FunctionModVarsSbst

 virtual inline void print( std::ostream &output ) const {
  output << "C05FunctionModVarsSbst[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function [" << f_function << " ]: ";
  switch( f_type ) {
   case( AddVar ): output << "add variables"; break;
   case( RemoveVar ): output << "delete variables"; break;
   case( SomeEntriesChange ):  output << "some entries change";
   }
  output << "(# " <<  v_vars.size() << ")" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionModVarsSbst ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS C05FunctionModShift -------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modifications specific to a C05Function
/** Derived class from FunctionModVars to describe a change to a C05Function
 * where some entries of the linearization are modified by a simple shift.
 *
 * A case where this happens is the Lagrangian one, where each linearization
 * is attached to one solution u_i \in U, and *some* of the RHS of the
 * complicating constraints A u = b change, whereas all the rest of (P)
 * remains the same. If b' is the new RHS vector, then all the previous
 * linearizations g_i = b - A u_i now have to become g'_i = b' - A u_i;
 * in other words, g'_i = g_i + ( b' - b ). This allows to update all the
 * linearizations that were stored in the global pool, without even having to
 * ask the Function object about them (since the vector b' - b is all the
 * information that is needed), thereby re-using all the corresponding
 * information to warm-start whatever algorithm one is using. This
 * Modification basically provides (the equivalent of) b' - b, in a sparse
 * format. */

class C05FunctionModShift : public FunctionMod {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/

 typedef std::pair< Variable * , Function::FunctionValue > SingleShift;
 ///< type for describing a shift: a pair < Variiable * , real >

 typedef std::vector<SingleShift> ShiftSet;  ///< a set of SingleShift

/*---------------------------- CONSTRUCTOR ---------------------------------*/

 /// constructor: takes a pointer to the affected C0%Function
 C05FunctionModShift( C05Function * const f ,
		      Function::FunctionValue shift = 0 ,
		      const bool cB = true )
  : FunctionMod( f , shift , cB ) { }

/*------------------------------ DESTRUCTOR --------------------------------*/

 ///< destructor: does nothing
 virtual ~C05FunctionModShift() { }

/*------------------------- OTHER INITIALIZATIONS --------------------------*/

 /// add one single entry to the set of shifts
 void add_linearization_shift( SingleShift shift ) {
  v_shifts.push_back( shift );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// add all the shifts in one blow
 void add_all_shifts( ShiftSet & shfts ) {
  v_shifts = shfts;
  }

/*------- METHODS FOR READING THE DATA OF THE C05FunctionModShift ----------*/

 /// get the vector of shifts
 /** Returns the vector with the components of the linearization vector (g)
  * that were shifted and the respective amount. For each pair < V* , s > in
  * this shift vector, the pointer to the Variable indicates the affected
  * component of the linearization and the second element of the pair is the
  * amount the value of that component has been shifted. So, if the value of
  * the component associated with the variable V is g in some linearization,
  * then the value of that component should become g + s. */

 const ShiftSet & get_linearization_shifts( void ) const {
  return( v_shifts );
  }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*--------------------------- PROTECTED FIELDS  ----------------------------*/

 ShiftSet v_shifts;  ///< the vector of shifts

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the C05FunctionModShift
 virtual inline void print( std::ostream &output ) const {
  output << "C05FunctionModShift[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function [" << &f_function << " ], (# shifts = "
	 << v_shifts.size() << ")" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionModShift ) )

/*@}  end( group( C05Function_CLASSES ) ) ----------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* C05Function.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File C05Function.h --------------------------*/
/*--------------------------------------------------------------------------*/
