/*--------------------------------------------------------------------------*/
/*-------------------------- File C05Function.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the C05Function class, which implements a Function that is
 * able to provide *linearizations*, i.e., first-order information. However,
 * the linearizations need not be a continuous function, which means that
 * the Function may be non-smooth.
 *
 * \version 0.30
 *
 * \date 15 - 03 - 2019
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
 *     (g in the comments) AND A SINGLR REAL SCALAR (\alpha in the comments)
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
xs *     f( y ) >= \f( \bar{x} ) + g ( y - \bar{x} ) for all y  ,
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
 ///< type used to define linear combinations of linearizations

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
  * Function can only produce a single linearization at a time (for it is,
  * say, smooth). */

 intGPMaxSz ,  ///< maximum size of the "global pool"
               /**< The algorithmic parameter for setting the size of the 
		* "global pool", that is, the maximum number of
 * linearizations that should be stored in the local pool. The default is 0,
 * which corresponds to the fact that the Function cannot store any
 * linearization (for it is, say, smooth and therefore there is no need to).
 */

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
  * at x, i.e., \alpha = f(x). In general linearizations that are not
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
  * case one knows that f(x) >= \alpha, and therefore typically an upper
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
  * case one knows that f(x) >= \alpha, and therefore typically an upper
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

 virtual void set_par( const idx_type par , const int value ) override
 {
  switch( par ) {
   case( intLPMaxSz ):
    if( value != 1 )
     throw( std::invalid_argument( "intLPMaxSz cannot be changed" ) );
    break;
   case( intGPMaxSz ):
    if( value != 0 )
     throw( std::invalid_argument( "intGPMaxSz cannot be changed" ) );
    break;
   default: Function::set_par( par , value );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set a given float (double) numerical parameter
 /** Set a given float (double) numerical parameter. Actually, the base
  * class just ignores the setting of these parameters, since the default
  * value of 0 is good for smooth functions giving just the exact gradient;
  * yet, setting any (non-negative) number allows these functions to just
  * keep doing the same, so whatever number is set can just be ignored. */

 virtual void set_par( const idx_type par , const double value ) override
 {
  switch( par ) {
   case( dblRAccLin ):
   case( dblAAccLin ):
    break;
   default: Function::set_par( par , value );
   }
  }

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
  * the global pool by replacing many linearizations by just one
  * "representing them all" (think conjugate subgradients).
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
  * function example: for a point x^* to be \eps-optimal for the minimization
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
  * The method has a default empty implementation as some Functions may not
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
  * components of the linearization vector should be obtained. If indices
  * is empty all the components from start to end will be retrieved. The
  * components of a linearization are numbered from 0 to n - 1, where
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
  * where the inner vectors G[ i ] are dimensioned to the *maximum* number of
  * Variable that the C05Function may have, and that a FunctionModVars occurs
  * which adds k more Variable, say [ n , ... , n + k - 1 ]. Note that the
  * position of these new Variable in G[] depends on their pointer, so it
  * may well be that including them in G[] would require a complete reshuffle
  * of the vector. It may however, be that the new Variable all have a
  * pointer > that any of the existing ones; then, the new entries of all the
  * linearizations corresponding to these new Variable can be written in place
  * in the existing vectors by just calling
  *
  *   get_linearization_coefficients( G[ i ].data() , i , null , n , k )
  *
  * for all i. */

 virtual void get_linearization_coefficients( FunctionValue *g ,
      const LinearizationName name = Inf<LinearizationName>() ,
	  c_Vec_Index & indices = {} , c_Index start = 0 ,
	  Index end = Inf<Index>() ) = 0;

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
      const LinearizationName name = Inf<LinearizationName>() ,
	  c_Vec_Index & indices = {} , c_Index start = 0 ,
	  Index end = Inf<Index>() ) = 0;

/*--------------------------------------------------------------------------*/
 /// return the constant term of a linearization
 /** This method returns the constant term (alpha) of a linearization. If the
  * name of the linearization is the default value Inf<LinearizationName>(),
  * then it refers to the last computed linearization, which may "not yet
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
  * Inf<FunctionValue>() as the corresponding \alpha, which might prompt the
  * linearization to be removed from the global pool (but at least is should
  * warn whatever algorithm is using the Function not to use it any longer). 
  */

 virtual FunctionValue get_linearization_constant(
   const LinearizationName name = Inf<Index>()  ) = 0;

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
  if( par == intLPMaxSz )
   return( 1 );
  else
   if( par == intGPMaxSz )
    return( 0 );
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
 * changed in its own way, *but all the g remained unchanged*. The idea is
 * that it is possible to query the value of \alpha for all the
 * linearizations that were stored in the global pool [see
 * get_linearization_constant()], thereby re-using all the corresponding
 * information to warm-start whatever algorithm one is using.
 *
 * A case where this happens is the Lagrangian one, where each linearization
 * is attached to one solution u_i \in U, and:
 *
 * - either the objective function c of (P) changes;
 *
 * - or U itself changes to some U' \neq U.
 *
 * In the first case, having stored u_i in the global pool it is immediate to
 * compute the new value of \alpha = c u'. In the second case, each u_i is
 * either feasible (u_i \in U') or not (u_i \notin U'). In the first case
 * \alpha = c u_i does not change, but in the second the original
 * linearization (even the g part) can no longer be used, as it is no longer
 * valid. This can be handled by having get_linearization_constant()
 * returning Inf<FunctionValue>() for the corresponding \alpha.
 *
 * Note that one would expect that the change in the \alpha implies a change
 * in the values of the Function as well. If and how this actually happens
 * is encoded in the f_shift field with the same encoding as in the base
 * class FunctionMod. For the Lagrangian case, for instance, if the feasible
 * region U becomes smaller than the value of the problem can only reduce,
 * whence f_shift ==INFshift is the appropriate return value.
 *
 * - AllEntriesChanged
 *
 * This type of modification states that all the entries of the g part of
 * all previously computed linearizations may have changed, *but all the
 * \alpha have remained unchanged*.
 *
 * A case where this happens is the Lagrangian one, where each linearization
 * is attached to one solution u_i \in U, g_i = b - A u_i. If the constraint
 * A u = b change to completely unrelated (save for the size) A' u = b',
 * then all the corresponding g_i need to be recomputed, but the \alpha_i =
 * c u_i remains the same. It is then possible to query the new values of the
 * linearizations that were stored in the global pool [see
 * get_linearization_coefficients()], thereby re-using all the corresponding
 * information to warm-start whatever algorithm one is using.
 *
 * Note that one would expect that the change in the \g implies a change in
 * the values of the Function as well. If and how this actually happens
 * is encoded in the f_shift field with the same encoding as in the base
 * class FunctionMod. For the Lagrangian case, for instance, it is unlikely
 * (but not downright impossible) that some monotonicity relationship can
 * be derived between the previous and the new function values, whence
 * std::isnan( f_shift ) == true is the appropriate return value.
 *
 * - AllLinearizationChanged
 *
 * This type of modification is the logical union of both previous types:
 * for all of previously computed linearizations, both the g part and the
 * \alpha may have changed. This again typically implies that the function
 * values have changed as well, with f_shift specifying how. It is then
 * possible to query the new values of the linearizations that were stored
 * in the global pool using the available methods as in the two previous
 * cases. */

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
		 const FunctionValue shift = NaNshift , const bool cB = true )
  : FunctionMod( f , shift , cB ), f_type( mod ) { }

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~C05FunctionMod() { }  ///< destructor: does nothing

/*----------------------- PUBLIC FIELDS OF THE CLASS -----------------------*/

  int f_type;  ///< type of modification

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
  /// print the C05FunctionMod

  virtual inline void print( std::ostream &output ) const override
  {
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
   output << " have changed ==> f-values changed";
   if( std::isnan( f_shift ) )
    output << "(+-)";
   else
    if( f_shift >= INFshift )
     output << "(+)";
    else
     if( f_shift <= -INFshift )
      output << "(-)";
     else
      output << " by " << f_shift;
   output << std::endl;
   }

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionMod ) )

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS C05FunctionModSbst -------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe C05Function changes limited to a subset of Variable
/** Derived class from C05FunctionMod, extends it to the concept that the
 * changes to the "complicated" part of the linearization (the vector g) may
 * be localized to some subset of the entries as opposed to involving the
 * whole vector. For this reason it include the std::vector<Variable *> field
 * v_vars (akin to that of FunctionModVars).
 *
 * Formally, the class supports all the three types of change
 *
 * - AlphaChanged
 * - AllEntriesChanged
 * - AllLinearizationChanged
 *
 * as C05FunctionMod, but "AllEntries" and "AllLinearization" now have to be
 * taken to mean "all those specified by v_vars". Also, it does not make
 * sense to issue a C05FunctionModSbst with type AlphaChanged, since actually
 * this does not involve g and therefore v_vars is useless and should at best
 * be ignored; to all intents and purposes, a C05FunctionModSbst with type
 * AlphaChanged is the same as a C05FunctionMod with the same type and should
 * be dealt with in the same way (which should be possible at low cost).
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
 * about that linearization (\bar{u}) has been saved in the global pool;
 * indeed, this is why these methods have both the "name" and "indices"
 * parameters.
 *
 * The mapping between the pointers in v_vars and the involved entries of g
 * (i.e., g_I, i.e., the subset I) would seem to be obvious: in fact, that
 * each entry of g is uniquely associated to a Variable, and therefore the
 * "name = pointer" of the Variable can be used as an "index" in g.
 * Therefore, the v_vars field of C05FunctionModSbst can be transformed into
 * a set of indices (I) in g by means of ThinVarDepInterface::is_active() or
 * ThinVarDepInterface::map_active(). However,
 *
 *     THOSE METHODS PROVIDE INDICES FOR THE CURRENT MAPPING IN THE
 *     C05Function, WHICH MAY BE COMPLETELY DIFFERENT FOR THE MAPPING
 *     THAT THE Solver/Observer HAD CONSTRUCTED
 *
 * In fact, when this Modification is being processed, any number of
 * additions/removals of Variable [see C05FunctionModVar] may have occurred.
 * The mapping stored in the C05Function may therefore have changed in any
 * ways. As an example,
 *
 *     A Variable * STORED IN v_var MAY NOT EVEN BE THAT OF A Variable
 *     THAT IS CURRENTLY "ACTIVE" IN THE C05Function
 *
 * as the Variable may well have been subsequently removed before this
 * Modification is processed.
 *
 * This just means that whatever Solver/Observer has to process this
 * Modification has to be careful about how to store the entries of the
 * linearizations, possibly internally keeping its own map. The simplest way
 * to do that is by keeping a std::vector< Variable * > whose i-th position
 * contains the pointer to the Variable whose first-orded information is
 * currently stored in the i-th position of the corresponding vectors. Yet,
 * the Solver/Observer may have many other choices, such as storing the
 * information in a std::map or any other data structure readily accessible
 * by using a Variable * key. Also, the Solver/Observer may make assumptions
 * on how the Variable of the C05Function are handled which simplify this
 * task (provided, of course, that these assumptions are clearly stated in
 * the interface); the simplest one being that Variable are never
 * added/removed, so that the mapping is static. Alternatively, for instance,
 * the Solver/Observer may assume all the Variable to belong to a given --
 * say -- std::vector< Variable > of fixed size m, and store the g vectors
 * into parallel std::vector< FunctionValue >. Whatever the choice, the
 * responsibility of properly keeping appropiate data structures representing
 * the "internal" mapping lies on the Solver/Observer; the C05Function is not
 * supposed to help this in any way, save of course by issuing the
 * [C05]FunctionModVars that describe what happens to the Variable, and
 * therefore how the mapping has to be updated */

class C05FunctionModSbst : public C05FunctionMod {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: takes the set of indices (besides all else)
/** constructor: takes the type of the Modification, a pointer to the
  * affected Function, and the subset of affected Variable under the form
  * of a std::vector<Variable *>. As the the && tells, the vector "becomes
  * property" of the FunctionModVars object. The boolean parameter ordered
  * allows to tell whether or not the vars set passed to the constructor is
  * ordered: if not this is done right away, so that whomever receives the
  * Modification can assume v_vars always is. */

 C05FunctionModSbst( C05Function * const f , const int mod ,
		     Vec_p_Var && vars , const bool ordered = true , 
		     const FunctionValue shift = NaNshift ,
		     const bool cB = true )
  : C05FunctionMod( f , mod , shift , cB ) , v_vars( std::move( vars ) )
 {
  if( ! ordered )
   std::sort( v_vars.begin() , v_vars.end() );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~C05FunctionModSbst() { }  ///< destructor: does nothing

/*----------------------- PUBLIC FIELDS OF THE CLASS -----------------------*/

 Vec_p_Var v_vars;  ///< vector of pointers to affected Variable

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
  /// print the C05FunctionModSbst

  virtual inline void print( std::ostream &output ) const override
  {
   output << "C05FunctionModSbst[";
   if( concerns_Block() )
    output << "t";
   else
    output << "f";
   output << "] on Function [" << &f_function << " ]: ";
   if( f_type == AlphaChanged )
    output << "all the \alpha";
   else {
    if( f_type == AllEntriesChanged )
     output << "all the \alpha and";
    output << v_vars.size() << "entries of g";
    }
   output << " have changed ==> f-values ";
   if( std::isnan( f_shift ) )
    output << "changed unpredictably";
   else
    if( f_shift >= INFshift )
     output << "all increased";
    else
     if( f_shift <= -INFshift )
      output << "all decreased";
     else
      output << "all changed by exactly " << f_shift;
   output << std::endl;
   }

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionModSbst ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS C05FunctionModRngd --------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe changes to a C05Function involving a range of Variable
/** Derived class from C05FunctionMod, extends it to the concept that the
 * changes to the "complicated" part of the linearization (the vector g) may
 * be localized to some subset of the entries as opposed to involving the
 * whole vector. Unlike C05FunctionModSbst, however, the range is "dense",
 * i.e., it is specified as "all the Variable whose pointer lie between two
 * given pointers f_strt and f_stop" (the second excluded).
 *
 * For all the rest, this Modification works exactly as C05FunctionModSbst,
 * hence see the comments there. The significant point to notice is due to
 * the fact, again, that
 *
 *     THE SET OF "ACTIVE" Variable THAT WERE COMPRISED BETWEEN f_strt AND
 *     f_stop WHEN THE Modification WAS ISSUED MAY BE COMPLETELY DIFFERENT
 *     FROM WHAT IS NOW
 *
 * up to the point that
 *
 *     f_strt MAY NOT EVEN BE A POINTER TO A Variable THAT IS CURRENTLY
 *     "ACTIVE" IN THE C05Function
 *
 * (nor may f_stop, but this is not a problem since that Variable is never
 * accessed anyway, so the pointer may well be a "fake" one that points to
 * no Variable object at all).
 *
 * Indeed, after this Modification has been issued Variable can be
 * added/removed; this means that some of the Variable that were previously
 * in the range [ f_strt , f_stop ) may no longer be active (the range may
 * even be empty), or that new ones may have been inserted there.
 *
 * None of the two cases is really an issue. If a Variable is no longer in
 * the range, its component in g have to be removed anyway [see
 * C05FunctionModVars]; thus, the fact that it has changed can be safely
 * ignored, since a later Modification will reveal that the entry is no
 * longer significant. For a Variable that has been inserted, the entry was
 * not there in the first place and hence it clearly has to be recomputed;
 * the only issue is that that Variable is not present in the "internal
 * mapping" of the Solver/Observer that processes this Modification [see the
 * comments in C05FunctionModSbst], so that one does not have to expect that
 * it is. Yet, recognizing that the entry to be changed is not yet there one
 * can again plainly ignore it, as it will be computed at the moment in which
 * the Modification inserting it is processed. The worst case is that of a
 * Variable that was there, is removed and then added again: in this case
 * all the work is done thrice. Yet, should this ever become an issue, the
 * Solver/Observer can deploy logic to examine all the Modification in its
 * queue and avoid doing un-necessary work.
 *
 * Yet, all this again stresses the fact that whatever Solver/Observer has
 * to process this Modification possibly has to internally keeping its own
 * map between Variable * and entries of g, since it can not rely on the
 * one that is kept by the C05Function. Again there can be many ways for
 * doing this, or the Solver/Observer may make assumptions on how the
 * Variable of the C05Function are handled which simplify this task. For
 * "ranged" changes, a particularly convenient assumption is that all the
 * Variable to belong to a given -- say -- std::vector< Variable > of fixed
 * size m, to that the g vectors can be stored into parallel
 * std::vector< FunctionValue >, and the [ f_strt , f_stop ) range can be
 * easily transformed into ranges in those vectors, too. Whatever the choice,
 * the responsibility of properly keeping appropiate data structures
 * representing the "internal" mapping lies on the Solver/Observer; the
 * C05Function is not supposed to help this in any way, save of course by
 * issuing the [C05]FunctionModVars that describe what happens to the
 * Variable, and therefore how the mapping has to be updated */

class C05FunctionModRngd : public C05FunctionMod
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*-------------------- CONSTRUCTOR AND DESTRUCTOR --------------------------*/

 /// constructor: takes the type of the Modification and the range
 /** constructor: takes the type of the Modification, a pointer to the
  * affected Function, and the subset of affected Variable under the form
  * of a std::vector<Variable *>. As the the && tells, the vector "becomes
  * property" of the FunctionModVars object. The boolean parameter ordered
  * allows to tell whether or not the vars set passed to the constructor is
  * ordered: if not this is done right away, so that whomever receives the
  * Modification can assume v_vars always is. */

  C05FunctionModRngd( C05Function * const f , const int mod ,
		     Variable * const strt = nullptr ,
		     Variable * const stop = nullptr ,
		     const FunctionValue shift = 0 , const bool cB = true )
  : C05FunctionMod( f , mod , shift , cB ) , f_strt( strt ) , f_stop( stop )
  { }

 /// destructor: does nothing
 virtual ~C05FunctionModRngd() { }

/*---------------------- PUBLIC FIELDS OF THE CLASS ------------------------*/

 Variable * f_strt;   ///< the beginning of the range
 Variable * f_stop;   ///< the beginning of the range

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the C05FunctionModRngd

 virtual inline void print( std::ostream &output ) const
 {
  output << "C05FunctionModRngd[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function [" << f_function << " ]: ";
  if( f_type == AlphaChanged )
   output << "all the \alpha";
  else {
   if( f_type == AllEntriesChanged )
    output << "all the \alpha and";
   output << "the entries of g in [ " << f_strt << ", "
	  << f_stop << "]";
   }
  output << " have changed ==> f-values ";
  if( std::isnan( f_shift ) )
   output << "changed unpredictably";
  else
   if( f_shift >= INFshift )
    output << "all increased";
   else
    if( f_shift <= -INFshift )
     output << "all decreased";
    else
     output << "all changed by exactly " << f_shift;
  output << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionModRngd ) )

/*--------------------------------------------------------------------------*/
/*---------------------- Class C05FunctionModVars --------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe adding/removing Variable of a C05Function
/** Derived class from FunctionModVars to describe changes of a Function that
 * involve adding/removing the Variable "active" in it.
 *
 * This class adds some information to FunctionModVars that is related to how
 * changes in the set of "active" Variable impact the *linearization*, that
 * are specific to C05Function. The point is that, similarly to what is done
 * for function values in the base Function class, it is crucial to define
 * conditions under which previously computed linearizations can be
 * "salvaged" in case of addition/removal of Variable.
 *
 * Quasi-additivity provides a starting point, in the sense that if the
 * Function f_old( x , y ) is quasi-additive on y, and a linearization
 * ( g = [ g_x , g_y ] , \alpha ) is obtained at a point ( x , 0 ), then
 * ( g_x , \alpha ) should remain "valid" for the Function f( x ) with the y
 * Variable removed. Indeed, consider the case where the linearization is
 * the gradient: the component g_i corresponding to the variable x_i is the
 * partial derivative, i.e., the limit for t \to 0 of
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
 *     WHICH SURVIVE REMANIS "VALID" EVEN IF IT HAS BEEN COMPUTED AT A
 *     POINT WITH y \neq 0
 *
 * In other words, when passing from f_old( x , y ) to f( x ), the old
 * linearizations ( g = [ g_x , g_y ] , \alpha ) of f_old() should be able
 * to yield new linearizations ( g = g_x , \alpha' ) even if it has been
 * obtained at a point where y \neq 0. Also,
 *
 *     THE g PART OF THE LINEARIZATION CORRESPONDING TO THE PREVIOUS
 *     VARIABLES REMANIS "VALID" AND IT ONLY NEED TO BE EXTENDED
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
 * The new part g_y =  b' - A' \bar{u}  can be easily computed, provided
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
 * the first-order information obained by deleting the g_y component can be
 * of any use.
 *
 * All this is the reason why this Modification has the extra boolean field
 * f_strong; if true, it is intended to signal that the Modification to the
 * C05Function is *strongly* quasi-additive (which likely means that the
 * C05Function is either convex or concave, but this is not fixed in stone).
 * One would expect that strong quasi-additivity implies quasi-additivity,
 * and therefore that f_strong == true ==> f_shift finite and non-NaN, but
 * surely the vice-versa need not be true. If f_strong == false, all
 * linearizations computed at points ( x , y ) where y \neq 0 have to be
 * considered as completely invalid, even in their g_x part.
 *
 * For strongly quasi-additive additions, it is possible to update the
 * previously computed linearizations, computed by the Function before the
 * Modification, provided they are stored in the global pool. Indeed, any
 * g-part of any linerizarion of the Function after the Modification has
 * N = v_vars.size() extra entries; these can be retrieved by calls to
 * get_linearization_coefficients(), and in fact this use case is one of the
 * primary reasons why these methods have the "name" and "indices"
 * parameters. Similarly, for removals, v_vars uniquely identifies the N
 * "active" Variable that need be removed, and removing these Variable
 * "simply" correspond to deleting the corresponding entries from all
 * previous linearization vectors; unlike for additions, removals can in
 * principle be done without any further input from the C05Function.
 *
 * It still has to be remarked, however, that any Solver (or, in general,
 * Observer) processing this sort of Modification has to be careful. The
 * point is how information about previous linearizations, that has to be
 * updated, is stored. The most "natural" form would be something akin to a
 * std::vector< FunctionValue >; one could then think that, given v_vars,
 * the indices of the affected entries could be immediately computed by
 * calls to ThinVarDepInterface::is_active() or
 * ThinVarDepInterface::map_active(). However,
 *
 *     THOSE METHODS PROVIDE INDICES FOR THE CURRENT MAPPING IN THE
 *     C05Function, WHICH MAY BE COMPLETELY DIFFERENT FOR THE MAPPING
 *     THAT THE Solver/Observer HAD CONSTRUCTED
 *
 * In fact, when this Modification is being processed, any number of other
 * additions/removals of Variable may have occurred. The mapping stored in
 * the C05Function may therefore have changed in any ways. As an example,
 *
 *     A Variable * STORED IN v_var MAY NOT EVEN BE THAT OF A Variable
 *     THAT IS CURRENTLY "ACTIVE" IN THE C05Function
 *
 * as the Variable -- that is, say, added with this Modification -- may well
 * have been subsequently removed before this Modification is processed.
 *
 * It is of course possible to adapt the mapping inside the Solver/Observer to
 * match the current one in the C05Function. This can be done by processing
 * all Modification (in the right order), or by simply reading from scratch
 * the current mapping and adapting the internal data structures of the
 * Solver/Observer to it (incidentally, the first is probably preferable if
 * the Modifications are "few/small", while the second may well be more
 * efficient if they are "many/large"). However, in both cases the
 * Solver/Observer has to be able to map a Variable * with the index in its
 * vector(s) where the corresponding linearization information is without
 * expecting any support for it by the C05Function.
 *
 * This can of course be done, the simplest way is by keeping a
 * std::vector< Variable * > whose i-th position contains the pointer to the
 * Variable whose first-orded information is currently stored in the i-th
 * position of the corresponding vectors. Yet, the Solver/Observer may have
 * many other choices, such as storing the information in a std::map or any
 * other data structure readily accessible by using a Variable * key. Also,
 * the Solver/Observer may make assumptions on how the Variable of the
 * C05Function are handled which simplify this task (provided, of course,
 * that these assumptions are clearly stated in the interface); the simplest
 * one being that Variable are never added/removed, so that this Modification
 * never occurs and the mapping is static. Alternatively, for instance, the
 * Solver/Observer may assume all the Variable to belong to a given -- say --
 * std::vector< Variable > of fixed size m, and store the g vectors into
 * parallel std::vector< FunctionValue >. Whatever the choice, the
 * responsibility of properly keeping appropiate data structures representing
 * the "internal" mapping lies on the Solver/Observer; the C05Function is not
 * supposed to help this in any way, save of course by issuing the
 * Modification that describe what happens to the Variable, and therefore how
 * the mapping has to be updated. */

class C05FunctionModVars : public FunctionModVars
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*-------------------- CONSTRUCTOR AND DESTRUCTOR --------------------------*/

 /// constructor: identical to that of FunctionModVars, plus strong
 /** Constructor of C05FunctionModVars; besides the same arguments as that of
  * the base FunctionModVars, it takes a boolean indicating if the
  * addition/removal of Variable is *strongly* quasi-additive. This is saved
  * in the field f_strong. */

 C05FunctionModVars( Function * const f , const int mod ,
		     std::vector<Variable *> && vars ,
		     const bool ordered = true ,
		     const FunctionValue shift = 0 ,
		     const bool strong = false , const bool cB = true )
  : FunctionModVars( f , mod , std::move( vars ) , ordered , shift , cB ) ,
    f_strong( strong ) { }

 /// destructor: does nothing
 virtual ~C05FunctionModVars() { }

/*---------------------- PUBLIC FIELDS OF THE CLASS ------------------------*/

 bool f_strong;  ///< true if the Modification is strongly quasi-additive

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the C05FunctionModVars

 virtual inline void print( std::ostream &output ) const override
 {
  output << "C05FunctionModVars[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function [" << f_function << " ]: ";

  if( f_strong )
   output << "strongly ";

  if( std::isnan( f_shift ) )
   output << "non quasi-additively (any)";
  else
   if( f_shift >= std::numeric_limits<FunctionValue>::infinity() )
    output << "non quasi-additively (+)";
   else
    if( f_shift <= -std::numeric_limits<FunctionValue>::infinity() )
     output << "non quasi-additively (-)";
    else
     output << "quasi-additively (" << f_shift << ") ";

  if( f_type == AddVar )
   output << "adding ";
  else
   output << "deleting ";

  output  << v_vars.size() << " variables" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionModVars ) )

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
 *   (d_i = 0 for i \notin I);
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
 * A final observation is that this is a FunctionMod, and therefore it has a
 * f_shift value. Clearly, the value *can't* be finite, as the value of the
 * shift for two different points \bar{x} and \bar{x}' is d \bar{x} and
 * d \bar{x}', which cannot be always equal. Thus, the expected value of
 * f_shift should be NaN, except if the C05Function can infer something on
 * the sign; say, all Variable are non-negative and d >= 0, hence the shift
 * can only be positive and f_shift = 
 * std::numeric_limits<FunctionValue>::infinity() is appropriate. */

class C05FunctionModLin : public FunctionMod {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/

 /// constructor: takes the type of the Modification and the delta vector
 /** constructor: takes the type of the Modification, a pointer to the
  * affected Function, the vector of changes in the linear part of the
  * C05Function under the form of a std::vector< FunctionValue >, and the
  * subset of the Variable whose "linear part" changes under the form of a
  * std::vector< Variable * >. As the the && tells, both vectors "becomes
  * property" of the C05FunctionModLin object. The boolean parameter ordered
  * allows to tell whether or not the vars set passed to the constructor is
  * ordered: if not this is done right away, so that whomever receives the
  * Modification can assume v_vars always is. Note that, of course, then the
  * vector delta is re-ordered as well. */

 C05FunctionModLin( C05Function * const f ,
		    Function::Vec_FunctionValue && delta ,
		    Vec_p_Var && vars , const bool ordered = true ,
		    FunctionValue shift = NaNshift , const bool cB = true )
  : FunctionMod( f , shift , cB )
 {
  if( vars.size() != delta.size() )
   throw( std::invalid_argument( "vars and delta sizes do not match" ) );

  if( ordered ) {
   v_vars = std::move( vars );
   v_delta = std::move( delta );
   }
  else {
   std::vector<Function::Index> ord( vars.size() );
   std::iota( ord.begin() , ord.end() , 0 );
   std::sort( ord.begin() , ord.end() ,
	      [ & vars ]( Function::Index i , Function::Index j ) {
	       return( vars[ i ] < vars[ j ] ); }
	      );
   v_vars.resize( vars.size() );
   v_delta.resize( vars.size() );
   for( Function::Index i = 0 ; i < ord.size() ; ++i ) {
    v_delta[ i ] = delta[ ord[ i ] ];
    v_vars[ i ] = vars[ ord[ i ] ];
    }
   }
  }

/*------------------------------ DESTRUCTOR --------------------------------*/

 ///< destructor: does nothing

 virtual ~C05FunctionModLin() { }

/*---------------------- PUBLIC FIELDS OF THE CLASS ------------------------*/

 Function::Vec_FunctionValue v_delta;  ///< the vector d = b' - b

 Vec_p_Var v_vars;       ///< the vector of pointers to Variable

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the C05FunctionModLin

 virtual inline void print( std::ostream &output ) const override
 {
  output << "C05FunctionModLin[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function [" << &f_function
	 << " ]: change in the linear part of "<< v_delta.size()
	 << " variables" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( C05FunctionModLin ) )

/*@}  end( group( C05Function_CLASSES ) ) ----------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* C05Function.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File C05Function.h --------------------------*/
/*--------------------------------------------------------------------------*/
