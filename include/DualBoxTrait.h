/*--------------------------------------------------------------------------*/
/*-------------------------- File DualBoxTrait.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the DualBoxTrait class, a mixin equipping a Constraint
 * with a box that its optimal Lagrangian multiplier is known to belong to,
 * mixed into FRowConstraint.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __DualBoxTrait
 #define __DualBoxTrait
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "RowConstraint.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS DualBoxTrait ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a box the optimal Lagrangian multiplier of a Constraint belongs to
/** A Constraint that is relaxed in a Lagrangian fashion may know, out of the
 * structure of the problem it belongs to, a box that any optimal multiplier
 * of the corresponding Lagrangian term belongs to. This is worth declaring
 * whenever some of the Variable it involves are unbounded, for then the box
 * is precisely what keeps the Lagrangian subproblems bounded.
 *
 * Say the Constraint is \f$ y' = y'' \f$ between two sub-Block, with
 * \f$ y' \geq 0 \f$ of cost \f$ b' \geq 0 \f$ occurring in the first one
 * with a nonpositive coefficient, and symmetrically for \f$ y'' \f$. Once
 * the Constraint is relaxed with multiplier \f$ \pi \f$, the two Lagrangian
 * subproblems read
 * \f[
 *   \min \{ c'x' + ( b' + \pi ) y' \} \quad + \quad
 *   \min \{ c''x'' + ( b'' - \pi ) y'' \}
 * \f]
 * so that \f$ ( b' + \pi ) < 0 \f$ sends \f$ y' \f$ to \f$ +\infty \f$ and
 * the first one to \f$ -\infty \f$, and symmetrically \f$ ( b'' - \pi ) < 0
 * \f$ does the same to the second one. Neither is ever optimal for a
 * Lagrangian dual, which is a maximization, whence
 * \f[
 *   -b' \; \leq \; \pi \; \leq \; b''
 * \f]
 * a small and clean box that spares the algorithm solving the dual from
 * having to discover it out of the extreme rays of unbounded subproblems.
 *
 * The box is [ -INF , +INF ] by default, i.e., "nothing is known", which is
 * always a valid answer. Whoever declares a tighter one is responsible for
 * it containing an optimal multiplier: a box that does not is no different
 * from any other constraint wrongly added to the dual, and cuts its optimal
 * value. */

class DualBoxTrait
{

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/

 using DualValue = RowConstraint::RHSValue;  ///< type of a multiplier

/*------------- CONSTRUCTING AND DESTRUCTING DualBoxTrait ------------------*/

 /// constructor: the box is "nothing is known" unless said otherwise

 explicit DualBoxTrait( DualValue lb = -Inf< DualValue >() ,
                        DualValue ub =  Inf< DualValue >() )
  : f_dual_lb( lb ) , f_dual_ub( ub ) {}

/*-------------------------- READING THE BOX -------------------------------*/

 /// the lower bound on the optimal multiplier, -INF if none is known

 [[nodiscard]] DualValue get_dual_lb( void ) const { return( f_dual_lb ); }

/*--------------------------------------------------------------------------*/

 /// the upper bound on the optimal multiplier, +INF if none is known

 [[nodiscard]] DualValue get_dual_ub( void ) const { return( f_dual_ub ); }

/*--------------------------------------------------------------------------*/

 /// true if either bound is finite, i.e., the box says something

 [[nodiscard]] bool has_dual_box( void ) const {
  return( ( f_dual_lb > -Inf< DualValue >() ) ||
          ( f_dual_ub <  Inf< DualValue >() ) );
  }

/*-------------------------- CHANGING THE BOX ------------------------------*/

 /// sets both bounds at once, throwing if the box is empty

 void set_dual_box( DualValue lb , DualValue ub ) {
  if( lb > ub )
   throw( std::invalid_argument( "DualBoxTrait::set_dual_box: empty box" ) );
  f_dual_lb = lb;
  f_dual_ub = ub;
  }

/*--------------------------------------------------------------------------*/

 void set_dual_lb( DualValue lb ) { set_dual_box( lb , f_dual_ub ); }

/*--------------------------------------------------------------------------*/

 void set_dual_ub( DualValue ub ) { set_dual_box( f_dual_lb , ub ); }

/*--------------------------------------------------------------------------*/
/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

 DualValue f_dual_lb;  ///< lower bound on the optimal multiplier

 DualValue f_dual_ub;  ///< upper bound on the optimal multiplier

 };  // end( class( DualBoxTrait ) )

/*--------------------------------------------------------------------------*/
}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* DualBoxTrait.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File DualBoxTrait.h ---------------------------*/
/*--------------------------------------------------------------------------*/
