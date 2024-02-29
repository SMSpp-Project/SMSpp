/*--------------------------------------------------------------------------*/
/*------------------------ File RowConstraint.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *abstract* RowConstraint class, derived from
 * Constraint, which is intended as the base class for all the Constraints
 * that are have a "row form", that is
 *
 *   LHS <= ( some function from Variables to reals ) <= RHS
 *
 * where LHS and RHS are two extended reals (hopefully at least one of which
 * is finite and LHS <= RHS, but this is not enforced in the class).
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __RowConstraint
 #define __RowConstraint
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Constraint.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup RowConstraint_CLASSES Classes in RowConstraint.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS RowConstraint ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a Constraint that is a "single row"
/** The class RowConstraint, derived from Constraint, is intended as the base
 * class for all the Constraints that are have a "row form", that is,
 *
 *   LHS <= ( some function from Variable to reals ) <= RHS
 *
 * where LHS and RHS are two extended reals (hopefully at least one of which
 * is finite and LHS <= RHS, but this is not enforced in the class). A type
 * "RHSValue" is defined, which is bound by default to doubles, to hold the
 * type of the LHS and RHS. Changing this type here is possible, but this
 * changes it to the whole SMS++ hierarchy, so this does not look too
 * reasonable; if one really needs a different return value than double she
 * can rather re-define a similar class to this.
 *
 * The above form encodes all possible kinds of equality, inequality and
 * ranged constraints. In this base class, *no assumption is done upon the
 * form of the function*: typical examples are linear functions, quadratic
 * functions etc., but this is dealt with in derived classes.
 *
 * It is expected that, in general, compute()-ing a RowConstraint is "hard";
 * this clearly means that what is hard is compute()-ing its variable part,
 * i.e., the "some function from Variable to reals". The exact form of it is
 * not specified by RowConstraint as it is expected to be implemented in
 * derived classes. However, due to the expectation that the computation is
 * hard, RowConstraint supports the notion that the actual value of the
 * variable part may not be known exactly; rather, a lower bound and an upper
 * bound on it will be computed (which may of course coincide in "easy"
 * cases). In fact, with lb() <= ub() being these two values, of course the
 * RowConstraint is guaranteed to be satisfied as long as
 *
 *   LHS <= lb()    and    ub() <= RHS
 *
 * which clearly does not require lb() == ub() to hold, thereby potentially
 * allowing to save computation time by avoiding to compute the real value
 * with high precision. Of course, equality constraints with LHS == RHS do
 * force lb() == ub() to hold if they are to be declared satisfied, but since
 * numerical tolerances are necessarily involved anyway (see abs_viol() and
 * rel_viol()) even equality constraints can be deemed to be satisfied with
 * lb() < ub() if the difference is "small" / the tolerances are "loose".
 *
 * For many classes of problems this kind of Constraint naturally have a
 * "dual" information attached [see CDASolver.h], which typically has the
 * form of a *Lagrangian multiplier*: a single real value (in this case,
 * again taken to be of type RHSValue). Support to this case is therefore
 * offered by this class. This may be redundant in some cases, but if this
 * really is a problem then a similar derived class from Constraint lacking
 * the dual value can be easily defined.
 *
 * Notice that an object of this class actually represents *two* constraints
 * (in particular, when the two bounds are finite and different from each
 * other, i.e., -inf < LHS < RHS < inf); yet, it has only *one* Lagrangian
 * multiplier. This is because we can assume without loss of generality (at
 * least regarding optimality) that
 *
 *     AT LEAST ONE BETWEEN THE Lagrangian MULTIPLIER ASSOCIATED WITH THE
 *     LOWER BOUND CONSTRAINT AND THE Lagrangian MULTIPLIER ASSOCIATED
 *     WITH THE UPPER BOUND CONSTRAINT IS ALWAYS 0
 *
 * This is of course true if one of the two bounds is infinite, as the
 * corresponding Lagrangian multiplier is simply not defined. Also, if
 * LHS == RHS then this is actually *one* (equality) constraint, and
 * therefore obviously has one Lagrangian multiplier. To see that we can
 * actually assume this "without loss of generality", consider the
 * following very general optimization problem
 * \f[
 *   (P) \quad \min \{ f( x ) : l \le g( x ) \le u , x \in X \}
 * \f]
 * where \f$ X \f$ is any set, \f$ g : X \to R \f$ is any function, and
 * \f$ -\infty < l < u < \infty \f$. The most general form of duality is the
 * Lagrangian dual, which starts by defining the Lagrangian function
 * associated with (P)
 * \f[
 *   L( w , z ) = \min \{ f( x ) + w ( l - g( x ) ) + z ( g( x ) - u ) :
 *                        x \in X \}
 * \f]
 * where \f$ z \ge 0 \f$ is the Lagrangian multiplier of the constraint
 * \f$ g( x ) \le u \f$, while \f$ w \ge 0 \f$ is the Lagrangian multiplier
 * of the constraint \f$ l \le g( x ) \f$. We can rewrite \f$ L() \f$ as
 * \f[
 *   L( w , z ) = w l - z u +
 *                \min \{ f( x ) + ( z - w ) g( x ) : x \in X \}  ,
 * \f]
 * so that the Lagrangian dual of (P) is
 * \f[
 *   (D) \quad \max \{ w l - z u +
 *                     \min \{ f( x ) + ( z - w ) g( x ) : x \in X \} :
 *                     w \ge 0  ,  z \ge 0 \}
 * \f]
 * The above formula already makes it immediately apparent that the
 * minimization problem that underlies the Lagrangian function actually
 * depends on the difference \f$ z - w \f$; that is, any choice of \f$ z \f$
 * and \f$ w \f$ that have the same difference provides exactly the same value
 * of the minimization problem (the Lagrangian relaxation) in the computation
 * of \f$ L() \f$. Besides, the set of optimal solutions is the same, and so
 * is the set of supergradients of (the concave function) \f$ L() \f$. Of
 * course, due to the term \f$  w l - z  u \f$ in the objective, and the fact
 * that \f$  l <  u \f$, different choices of \f$ z \f$ and \f$ w\f$ with the
 * same difference do have an impact on the value of \f$ L() \f$, but a
 * very predictable one. Indeed, assume that one has a feasible solution
 * \f$ ( \bar{w} , \bar{z} ) \f$ for (D) such that \f$ \bar{w} > 0 \f$ and
 * \f$ \bar{z} > 0 \f$: it is easy to prove that it cannot be an optimal
 * solution for (D). In fact, define
 * \f[
 *   ( w' = \max \{ 0 , \bar{w} - \bar{z} \} ,
 *     z' = \max \{ 0 , \bar{z} - \bar{w} \} )
 * \f].
 * It is immediate to check that \f$ ( w' , z' ) \ge 0 \f$ (is feasible),
 * and that
 * \f[
 *   z' - w' = \bar{z} - \bar{w} \qquad\qquad (*)
 * \f]
 * It is also obvious that
 * \f[
 *   w' l - z' u > \bar{w} l - \bar{z} u
 * \f]
 * Indeed,
 * \f[
 *   w' l - z' u = w' l - w' u + w' u - z' u = w' ( l - u ) + ( w' - z' ) u
 * \f]
 * and a similar relationship holds for \f$ \bar{w} l - \bar{z} u \f$. But
 * from (*) one has that \f$ ( w' - z' ) u = ( \bar{w} - \bar{z} ) u \f$, and
 * \f[
 *  w' ( l - u ) > \bar{w} ( l - u )
 * \f]
 * because obviously \f$ w' < \bar{w} \f$ and \f$ l - u < 0 \f$. Hence,
 * \f$ ( w' , z' ) \f$ is the announced feasible solution of (D) whose
 * objective value is better than that of  \f$ ( \bar{w} , \bar{z} ) \f$,
 * and such that \f$ w' z' = 0 \f$. We will therefore assume that
 *
 *     ALL DUAL SOLUTIONS STORED IN A RowConstraint ARE "NON DOMINATED"
 *     ONES WITH THE ABOVE PROPERTY
 *
 * This is not really "without loss of generality", but arguably worse dual
 * solutions should never be preferred to better ones. Besides, the
 * transformation between \f$ ( \bar{w} , \bar{z} ) \f$ and
 * \f$ ( w' , z' ) \f$ is so trivial that, even if a Solver naturally
 * produces the former, it is perfectly reasonable to ask it to produce the
 * latter instead.
 *
 * The same arguments can be immediately extended to the case where (P) has
 * any number of ranged constraints.
 *
 * As a consequence
 *
 *     THE SIGN OF THE DUAL MULTIPLIER OF A RANGED RowConstraint (WITH
 *     -inf < l < u < inf) CAN BE USED TO SPECIFY WHICH OF THE TWO
 *     (NON-NEGATIVE) MULTIPLIERS w AND z IS NONZERO.
 *
 * This, however, requires setting some standard. Indeed, for a
 * *minimization* problem such as (P) above, we will assume that the value
 * stored in \c d_value (which can be retrieved by get_dual() and set by
 * set_dual()) is
 * \f[
 *   ( z - w )
 * \f]
 * i.e., the coefficient of \f$ g( x ) \f$ in \f$ L() \f$. This means that
 *
 * - if d_value > 0, \f$ z > 0 \f$ and \f$ w = 0 \f$
 *
 * - if d_value < 0, \f$ z = 0 \f$ and \f$ w > 0 \f$
 *
 * Note that the assumption \f$ l \le u \f$ may be violated: this makes the
 * problem unfeasible, and indeed (D) then is unbounded above. In fact, in
 * \f[
 *   (D) \quad \max \{ w l - z u +
 *                     \min \{ f( x ) + ( z - w ) g( x ) : x \in X \} :
 *                     w \ge 0  ,  z \ge 0 \}
 * \f]
 * it is possible to fix \f$ z - w \f$ to any value, thereby fixing the
 * value of the inner minimization. Choosing  \f$ z - w = 0 \f$, i.e.,
 * \f$ z = w \f$, then yields
 * \f[
 *   w l - z u = w l - w u = w ( l - u ) \ge 0
 * \f]
 * which means that increasing \f$ w \f$ (and \f$ z \f$ with it) the value
 * can be brought to infinity. This means that the dual direction
 * \f$ [ 1 , 1 ] \f$ (increase both) is an infinite ascent direction the
 * Lagrangian dual. Under the sign rule postulated above this is
 * represented as d_value = -1 (< 0 and unitary) to imply that is the
 * multiplier of the lower bound constraint that has to go to infinity
 * (although this implies that also the one of the upper bound constraint
 * has to keep feasibility: this contradicts the rule that only one of the
 * two is nonzero, but the rule holds for solutions, not for directions).
 *
 * However, if (P) is rather a *maximization* problem
 * \f[
 *   (P) \quad \max \{ f( x ) : l \le g( x ) \le u , x \in X \}
 * \f]
 * then its Lagrangian function rather need to be defined as
 * \f[
 *   L( w , z ) = \max \{ f( x ) + w ( g( x ) - l ) + z ( u - g( x ) ) :
 *                        x \in X \}   ,
 * \f]
 * i.e.,
 * \f[
 *   L( w , z ) = z u - w l +
 *                \max \{ f( x ) + ( w - z ) g( x ) : x \in X \}  ,
 * \f]
 * This is of course if one insists that \f$ z \ge 0 \f$ and \f$ w \ge 0 \f$;
 * it would be perfectly possible to rather have  \f$ z \le 0 \f$ and
 * \f$ w \ge 0 \f$ (either in the maximization case, or always), but the
 * non-negative version is usually preferred, so this is what we use. The
 * transformation from \f$ ( \bar{w} , \bar{z} ) \f$ to \f$ ( w' , z' ) \f$
 * remains the same, but now the coefficient of \f$ g( x ) \f$ in
 * \f$ L() \f$ is rather \f$ w - z \f$; hence, it is more natural in this
 * case to assume that that is the value stored in \c d_value, which leads
 * to the opposite standard:
 *
 * - if d_value > 0, \f$ w > 0 \f$ and \f$ z = 0 \f$
 *
 * - if d_value < 0, \f$ w = 0 \f$ and \f$ z > 0 \f$
 *
 * Note that in the infeasible case \f$ l > u \f$, the Lagrangian function
 * \f[
 *   L( w , z ) = z u - w l + ...
 * \f]
 * now has to be minimised, which leads to sending \f$ z \f$ to infinity
 * (although of course \f$ w \f$ has to). This means that the dual direction
 * \f$ [ 1 , 1 ] \f$ (increase both), that is an infinite ascent direction
 * for the Lagrangian dual, under the sign rule postulated above should
 * again be represented as d_value = -1 (< 0 and unitary).
 *
 * It is the Solver's responsibility to obey these rules in order for the
 * value stored in \c d_value to have the correct meaning.
 *
 *     THE SOLVER WRITING THE DUAL SOLUTION IN THIS RowConstraint
 *     MUST RESPECT THE CONVENTION STATED ABOVE.
 *
 * This means, in particular, that the dual value stored here may differ in
 * sign from the value of the dual variable "naturally produced" by a
 * particular Solver, but this is not a real issue implementation-wise.
 *
 * On top of the basic ConstraintMod, other modifications are possible for
 * this kind of Constraint, namely
 *
 * - changing the LHS/RHS.
 *
 * Yet, note that the base class does not explicitly store LHS and RHS values
 * in order to allow more flexibility for derived classes to do that as they
 * better see fit (for instance, not storing them at all if they are fixed).
 * Thus, the methods for setting and changing these values (which are the
 * ones to throw these Modification) are pure virtual. */

class RowConstraint : public Constraint
{
/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
 /** @name Public Types
  *  @{ */

 typedef double RHSValue;  ///< type of the LHS/RHS of the RowConstraint
                           /**< type of the LHS/RHS of the RowConstraint,
                            * and therefore also of the attached dual
                            * information (Lagrangian multiplier). */

 typedef const RHSValue c_RHSValue;  ///< a const VarValue

/*----------------------------- CONSTANTS ----------------------------------*/

 static constexpr RHSValue RHSINF = Inf< RHSValue >();
 ///< convenience constexpr for "Infty"

/** @} ---------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
    @{ */

 /// constructor of RowConstraint, taking the Block
 /** Constructor of RowConstraint. Takes the pointer to the Block to which
  * the RowConstraint belongs, to be passed to the constructor of
  * Constrain (default nullptr, so that this can be used as the void
  * constructor). Since the base class does not directly handles LHS and
  * RHS, there is no point for them to be passed here. */

 explicit RowConstraint( Block * my_block = nullptr )
  : Constraint( my_block ) , d_value( 0 ) {}

/*--------------------------------------------------------------------------*/
 /// destructor: it is virtual, and empty

 ~RowConstraint() override = default;

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the RHS of this to RowConstraint
 /** Set the RHS of this to RowConstraint to rhs_value. Since the base class
  * does not directly handles these values, this method is pure virtual.
  *
  * The parameter issueMod decides if and how any Modification is issued, as
  * described in Observer::make_par(). */

 virtual void set_rhs( RHSValue rhs_value ,
                       ModParam issueMod = eModBlck ) = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set the LHS of this to RowConstraint
 /** Set the LHS of this to RowConstraint to lhs_value. Since the base class
  * does not directly handles these values, this method is pure virtual.
  *
  * The parameter issueMod decides if and how any Modification is issued, as
  * described in Observer::make_par(). */

 virtual void set_lhs( RHSValue lhs_value ,
                       ModParam issueMod = eModBlck ) = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set both the LHS and the RHS of this to RowConstraint
 /** Set the both the LHS and the RHS of this to RowConstraint to the same
  * value, both_value. This is useful for equality constraints. Since the base
  * class does not directly handles these values, this method is pure virtual.
  *
  * The parameter issueMod decides if and how any Modification is issued, as
  * described in Observer::make_par(). */

 virtual void set_both( RHSValue both_value ,
                        ModParam issueMod = eModBlck ) = 0;

/*--------------------------------------------------------------------------*/
 /// method to set the dual value of the RowConstraint
 /** method to set the dual value (i.e., the Lagrangian multiplier) of the
  * RowConstraint; typically, a CDASolver [see CDASolver.h] attached to the
  * Block to which this RowConstraint belongs will do it. For more ease of
  * mind, this method is virtual. */

 virtual void set_dual( c_RHSValue new_value = 0 ) { d_value = new_value; }

/** @} ---------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE DATA OF THE RowConstraint -----------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the RowConstraint
    @{ */

 /// pure virtual method to get the RHS of the RowConstraint
 [[nodiscard]] virtual RHSValue get_rhs( void ) const = 0;

 /// pure virtual method to get the LHS of the RowConstraint
 [[nodiscard]] virtual RHSValue get_lhs( void ) const = 0;

/** @} --------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF A RowConstraint -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a RowConstraint
    @{ */

 /// compute the value of variable part of the RowConstraint

 int compute( bool changedvars = true ) override = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get a lower bound on the value of the variable part
 /** Method to get a lower bound on the value of variable part of the
  * RowConstraint. This is likely the value having been computed in compute(),
  * and stored somewhere to avoid having to recompute it, which is potentially
  * a costly process (as any compute() can be). However, whether or not this
  * is true depends on the specific :RowConstraint, which is why the method is
  * virtual: to allow derived classes to store the value as they best see fit.
  * However, in general it must be understood that lb() does not necessarily
  * need to be a valid lower bound corresponding to the current value of the
  * Variable in the RowConstraint, but rather to what their value was at the
  * last time in which compute() has been called; in other words, compute()
  * must be called prior to lb() to ensure that the value is the up-to-date
  * one. Yet, because derived classes are free to implement this as they
  * choose, one *must not* assume that the value *does not* change when the
  * Variable in the RowConstraint do, as derived classes where the value of
  * the variable part is "easy" to compute may decide to do just that.
  *
  * It is however expected that lb() <= ub(), because this is easy for
  * derived classes to ensure and there is no real reason not to. */

 [[nodiscard]] virtual RHSValue lb( void ) const = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get an upper bound on the value of the variable part
 /** Method to get an upper bound on the value of variable part of the
  * RowConstraint; see lb() for comments about the relationships between the
  * value returned by this method and compute().
  *
  * Note that it is expected that lb() <= ub(), because this is easy for
  * derived classes to ensure and there is no real reason not to. */

 [[nodiscard]] virtual RHSValue ub( void ) const = 0;

/*--------------------------------------------------------------------------*/

 [[nodiscard]] bool feasible( void ) const override {
  bool feas = true;
  auto lhs = get_lhs();
  auto rhs = get_rhs();

  if( lhs > -RHSINF )
   feas &= ( lb() >= lhs );

  if( rhs < RHSINF )
   feas &= ( ub() <= rhs );

  return( feas );
  }

/*--------------------------------------------------------------------------*/
 /// returns the absolute violation of the RowConstraint
 /** The method computes and returns the absolute violation of the
  * RowConstraint corresponding to the currently available upper and lower
  * bounds on the value of the variable part [see lb() and ub()]. The value
  * is positive if at least one of the two bounds is violated; note that
  * both can be since lb() < lhs <= rhs < ub() can happen, and the violation
  * is the maximum among the two. An infinite bound (lhs == - RHSINF or
  * rhs == RHSINF) corresponds to a 0 violation whatever the lb()/ub()
  * (even if it is the same infinity).
  *
  * This method is provided because checking feasibility of a RowConstraint
  * should reasonably require numerical tolerances, which are not there in
  * the feasible() method (because in general constraints could be checked
  * "exactly", say if only integer arithmetic is involved). This is somehow
  * against the grain of ThinComputeInterface: there could be parameters for
  * doing this. However, this would complicate the interface, and could mean
  * that each RowConstraint need to carry its own values of the parameters
  * (while they are generally constant across many constraints of "the same
  * type" in a model). Via this method checking can be done outside of the
  * RowConstraint, which for the time being we consider the better choice.
  *
  * Note that since in general lb()/ub() may not return a valid lower/upper
  * bound on the value that the variable part has corresponding to the
  * *current* value of the Variable in the RowConstraint, but rather on the
  * value that is had the last time that compute() was called, compute()
  * must be called "right before" this method to ensure that the lb()/ub()
  * are "up to date"; see the comments to lb(). */

 [[nodiscard]] virtual RHSValue abs_viol( void ) const {
  RHSValue viol = 0;

  if( auto lhs = get_lhs() ; lhs > -RHSINF )
   if( auto lbv = lb() ; lbv < RHSINF ) {
    if( lbv <= -RHSINF )
     return( RHSINF );
    viol = std::max( viol , lhs - lbv );
    }

  if( auto rhs = get_rhs() ; rhs < RHSINF )
   if( auto ubv = ub() ; ubv > -RHSINF ) {
    if( ubv >= RHSINF )
     return( RHSINF );
    viol = std::max( viol , ubv - rhs );
    }

  return( viol );
  }

/*--------------------------------------------------------------------------*/
 /// returns the relative violation of the RowConstraint
 /** The method computes and returns the relative violation of the
  * RowConstraint corresponding to the currently available upper and lower
  * bounds on the value of the variable part [see lb() and ub()]. This is the
  * same value as abs_viol() returns, except each violation is divided by the
  * maximum between 1 and the absolute value of the corresponding bound. In
  * particular, this means that if one bound is between -1 and 1, its absolute
  * violation is used (the scaling factor is 1). Only finite bounds are
  * considered.
  *
  * See abs_viol() for the rationale of providing such a method and for the
  * need to properly call compute() before calling it to ensure the
  * correctness of the result. */

 [[nodiscard]] virtual RHSValue rel_viol( void ) const {
  RHSValue viol = 0;

  if( auto lhs = get_lhs() ; lhs > -RHSINF )
   if( auto lbv = lb() ; lbv < RHSINF ) {
    if( lbv <= -RHSINF )
     return( RHSINF );
    if( auto tv = lhs - lbv ; tv > 0 ) {
     tv /= std::max( RHSValue( 1 ) , std::abs( lhs ) );
     viol = tv;
     }
    }

  if( auto rhs = get_rhs() ; rhs < RHSINF )
   if( auto ubv = ub() ; ubv > -RHSINF ) {
    if( ubv >= RHSINF )
     return( RHSINF );
    if( auto tv = ubv - rhs ; tv > 0 ) {
     tv /= std::max( RHSValue( 1 ) , std::abs( rhs ) );
     if( tv > viol )
      viol = tv;
     }
    }

  return( viol );
  }

/*--------------------------------------------------------------------------*/
 /// get the dual value (the Lagrangian multiplier) of the RowConstraint

 [[nodiscard]] RHSValue get_dual( void ) const { return( d_value ); }

/*--------------------------------------------------------------------------*/
 /// checks if the violation of each RowConstraint does not exceed the tolerance
 /** This function checks whether the violation of each RowConstraint (that is
  * not relaxed; see is_relaxed()) in the given collection of RowConstraint is
  * not greater than the provided tolerance. The parameter \p rel_viol
  * indicates whether the relative (see rel_viol()) or the absolute (see
  * abs_viol()) violation must be considered.
  *
  * If a RowConstraint in the collection is not relaxed, then it will be
  * compute()-ed before its violation is checked. Moreover, it is not
  * guaranteed that every RowConstraint in the collection will be
  * compute()-ed, since this function may return false early (in the case a
  * RowConstraint is not correctly compute()-ed or its violation exceeds the
  * given tolerance).
  *
  * @param constraints A collection of RowConstraint.
  *
  * @param tolerance The value that determines whether each RowConstraint in
  *        the given collection is satisfied.
  *
  * @param rel_viol If true, the the relative violation is considered (see
  *        rel_viol()). Otherwise, the absolute violation is considered (see
  *        abs_viol()).
  *
  * @return This function returns true if and only if the violation of each
  *         RowConstraint of the given collection is not greater than the
  *         given tolerance. */

 template< template< class ... > class C , class T >
 static std::enable_if_t< std::is_base_of_v< RowConstraint , T > , bool >
 is_feasible( C< T > & constraints , double tolerance = 1e-10 ,
              bool rel_viol = true ) {
  for( auto & constraint : constraints ) {
   if( constraint.is_relaxed() )
    continue;
   if( auto ret = constraint.compute();
    ( ret <= Constraint::kUnEval ) || ( ret > Constraint::kOK ) )
    return( false );
   if( ( rel_viol ? constraint.rel_viol() :
         constraint.abs_viol() ) > tolerance )
    return( false );
  }
  return( true );
 }

/*--------------------------------------------------------------------------*/
 /// checks if the violation of each RowConstraint does not exceed the tolerance
 /** This function checks whether the violation of each RowConstraint (that is
  * not relaxed; see is_relaxed()) in the given collection of RowConstraint is
  * not greater than the provided tolerance. The parameter \p rel_viol
  * indicates whether the relative (see rel_viol()) or the absolute (see
  * abs_viol()) violation must be considered.
  *
  * If a RowConstraint in the collection is not relaxed, then it will be
  * compute()-ed before its violation is checked. Moreover, it is not
  * guaranteed that every RowConstraint in the collection will be
  * compute()-ed, since this function may return false early (in the case a
  * RowConstraint is not correctly compute()-ed or its violation exceeds the
  * given tolerance).
  *
  * @param constraints A collection of RowConstraint.
  *
  * @param tolerance The value that determines whether each RowConstraint in
  *        the given collection is satisfied.
  *
  * @param rel_viol If true, the the relative violation is considered (see
  *        rel_viol()). Otherwise, the absolute violation is considered (see
  *        abs_viol()).
  *
  * @return This function returns true if and only if the violation of each
  *         RowConstraint of the given collection is not greater than the
  *         given tolerance. */

 template< typename T , std::size_t K >
 static std::enable_if_t< std::is_base_of_v< RowConstraint , T > , bool >
 is_feasible( boost::multi_array< T , K > & constraints ,
              double tolerance = 1e-10 , bool rel_viol = true ) {
  auto n = constraints.num_elements();
  auto constraint = constraints.data();
  for( decltype( n ) i = 0 ; i < n ; ++i , ++constraint ) {
   if( constraint->is_relaxed() )
    continue;
   if( auto ret = constraint->compute();
    ( ret <= Constraint::kUnEval ) || ( ret > Constraint::kOK ) )
    return( false );
   if( ( rel_viol ? constraint->rel_viol() :
         constraint->abs_viol() ) > tolerance )
    return( false );
  }
  return( true );
 }

/*--------------------------------------------------------------------------*/
 /// checks if the violation of each RowConstraint does not exceed the tolerance
 /** This function checks whether the violation of each RowConstraint (that is
  * not relaxed; see is_relaxed()) in the given collection of RowConstraint is
  * not greater than the provided tolerance. The parameter \p rel_viol
  * indicates whether the relative (see rel_viol()) or the absolute (see
  * abs_viol()) violation must be considered.
  *
  * If a RowConstraint in the collection is not relaxed, then it will be
  * compute()-ed before its violation is checked. Moreover, it is not
  * guaranteed that every RowConstraint in the collection will be
  * compute()-ed, since this function may return false early (in the case a
  * RowConstraint is not correctly compute()-ed or its violation exceeds the
  * given tolerance).
  *
  * @param constraints A collection of RowConstraint.
  *
  * @param tolerance The value that determines whether each RowConstraint in
  *        the given collection is satisfied.
  *
  * @param rel_viol If true, the the relative violation is considered (see
  *        rel_viol()). Otherwise, the absolute violation is considered (see
  *        abs_viol()).
  *
  * @return This function returns true if and only if the violation of each
  *         RowConstraint of the given collection is not greater than the
  *         given tolerance. */

 template< template< class ... > class C ,
           template< class ... > class D , class T >
 static std::enable_if_t< std::is_base_of_v< RowConstraint , T > , bool >
 is_feasible( const C< D< T > > & constraints , double tolerance = 1e-10 ,
              bool rel_viol = true ) {
  // if empty, std::all_of returns true, i.e., the solution is feasible
  return std::all_of( constraints.begin() , constraints.end() ,
                      [ tolerance , rel_viol ]( const auto & l_constraints ) {
                       return RowConstraint::is_feasible
                        ( l_constraints , tolerance , rel_viol );
                      } );
 }

/*--------------------------------------------------------------------------*/
 /// checks if the violation of each RowConstraint does not exceed the tolerance
 /** This function checks whether the violation of each RowConstraint (that is
  * not relaxed; see is_relaxed()) in the given collection of RowConstraint is
  * not greater than the provided tolerance. The parameter \p rel_viol
  * indicates whether the relative (see rel_viol()) or the absolute (see
  * abs_viol()) violation must be considered.
  *
  * If a RowConstraint in the collection is not relaxed, then it will be
  * compute()-ed before its violation is checked. Moreover, it is not
  * guaranteed that every RowConstraint in the collection will be
  * compute()-ed, since this function may return false early (in the case a
  * RowConstraint is not correctly compute()-ed or its violation exceeds the
  * given tolerance).
  *
  * @param constraints A collection of RowConstraint.
  *
  * @param tolerance The value that determines whether each RowConstraint in
  *        the given collection is satisfied.
  *
  * @param rel_viol If true, the the relative violation is considered (see
  *        rel_viol()). Otherwise, the absolute violation is considered (see
  *        abs_viol()).
  *
  * @return This function returns true if and only if the violation of each
  *         RowConstraint of the given collection is not greater than the
  *         given tolerance. */

 template< template< class ... > class C , class T , std::size_t K >
 static std::enable_if_t< std::is_base_of_v< RowConstraint , T > , bool >
 is_feasible( const boost::multi_array< C< T > , K > & constraints ,
              double tolerance = 1e-10 , bool rel_viol = true ) {
  auto n = constraints.num_elements();
  auto l_constraints = constraints.data();
  for( decltype( n ) i = 0 ; i < n ; ++i , ++l_constraints ) {
   if( ! RowConstraint::is_feasible( *l_constraints , tolerance , rel_viol ) )
    return( false );
  }
  return( true );
 }

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
 *  @{ */

 /// print information about the RowConstraint on an ostream
 void print( std::ostream & output ) const override {
  output << "RowConstraint [" << this << "] of Block [" << f_Block
         << "] with " << get_num_active_var() << " active variables"
	 << std::endl;
  }

/** @} ---------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 RHSValue d_value;  ///< dual value (Lagrangian multiplier)

/*--------------------------------------------------------------------------*/

};  // end( class( RowConstraint ) )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS RowConstraintMod --------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modifications specific to a RowConstraint
/** Derived class from ConstraintMod to describe Modification specific to a
 * RowConstraint, i.e., change its LHS / RHS.
 *
 * Defining a class is a bit weird because the only thing the class does is
 * to define an enum for the new value of the type of Modification in the
 * ConstraintMod. However, throwing a Modification of a different class (but
 * derived from ConstraintMod) may make it easier for the solver to handle
 * it (at the very least, it directly knows it comes from a RowConstraint
 * without having to check it). */

class RowConstraintMod : public ConstraintMod {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/
 /// public enum for the types of ConstraintMod thrown
 /** Public enum "extending" cons_mod_type with the new types of Modification
  * thrown by a RowConstraint. */

 enum RowC_mod_type {
  eChgLHS = eConstModLastParam ,   ///< change the LHS
  eChgRHS ,                        ///< change the RHS
  eChgBTS ,                        ///< change both the RHS and the LHS
  eRowConstModLastParam  ///< first allowed value for derived classes
                         /**< Convenience value for easily allow derived
                          * classes to further extend the set of types of
                          * Modification. */
  };

/*---------------------- CONSTRUCTOR & DESTRUCTOR --------------------------*/

 /// constructor: just calls that of ConstraintMod

 explicit RowConstraintMod( RowConstraint * cnst , int mod = eChgLHS ,
			    bool cB = true )
  : ConstraintMod( cnst , mod , cB ) {}

 ~RowConstraintMod() override = default;  ///< destructor: does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the RowConstraintMod

 inline void print( std::ostream & output ) const override {
  output << "RowConstraintMod[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "]: changing ";
  switch( f_type ) {
   case ( eChgLHS ):
    output << "LHS";
    break;
   case ( eChgRHS ):
    output << "RHS";
    break;
   default:
    output << "both";
  }

  output << " of RowConstraint [" << f_constraint << "]" << std::endl;
  }

/*--------------------------------------------------------------------------*/

};  // end( class( RowConstraintMod ) )

/*--------------------------------------------------------------------------*/

/** @} end( group( RowConstraint_CLASSES ) ) -------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* RowConstraint.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File RowConstraint.h --------------------------*/
/*--------------------------------------------------------------------------*/
