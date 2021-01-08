/*--------------------------------------------------------------------------*/
/*-------------------------- File BoxSolver.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the BoxSolver class, which implements a CDASolver for
 * problems (or relaxations of problems) with an extremely simple structure:
 * only bound (box) Constraint on the ColVariable and a separable Objective
 * (a FRealObjective with either a LinearFunction or a DQuadFunction inside).
 * This only rarely is a significant problem in itself (although it may
 * indeed appear in the context of relaxation methods where everything else
 * has been relaxed), but it can be used to quickly obtain (hopefully,
 * finite) bounds on the optimal value of more complex problems that may be
 * useful during algorithmic approaches. For this reason, BoxSolver has an
 * uncommon "lax" attitude w.r.t. all the Constraint in the Block that are
 * not box ones: rather than protesting for their existance and refusing to
 * load, it plainly ignores them. This means that the computed optimal value
 * possibly is a(n hopefully finite) valid bound (lower or upper, according
 * to the verse of the original Objective) on the true optimal value. Yet,
 * this bound is obtained quickly.
 *
 * \version 0.10
 *
 * \date 04 - 01 - 2021
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __BoxSolver
 #define __BoxSolver
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Solver.h"

/*--------------------------------------------------------------------------*/
/*----------------------------- NAMESPACE ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{
/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BoxSolver -------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// Solver for "extremely simple" Block, or relaxations thereof
/** The BoxSolver class implements the CDASolver interface for problems (or
 * relaxations of problems) with an extremely simple structure:
 *
 * - only ColVariable
 *
 * - only bound (box) Constraint on the ColVariable, i.e., either those that
 *   are "inherent" in the ColVariable or :OneVarConstraint;
 *
 * - a separable Objective, i.e., a FRealObjective with either a
 *   LinearFunction or a DQuadFunction inside.
 *
 * This only rarely is a significant problem in itself (although it may
 * indeed appear in the context of relaxation methods where everything else
 * has been relaxed), but it can be used to quickly obtain (hopefully,
 * finite) bounds on the optimal value of more complex problems that may be
 * useful during algorithmic approaches. For this reason, BoxSolver has an
 * uncommon "lax" attitude w.r.t. all the Constraint in the Block that are
 * not box ones: rather than protesting for their existance and refusing to
 * load, it plainly ignores them. This means that the computed optimal value
 * possibly is a(n hopefully finite) valid bound (lower or upper, according
 * to the verse of the original Objective) on the true optimal value. Yet,
 * this bound is obtained quickly.
 *
 * In fact, for the problems solved by BoxSolver have the very uncommon
 * property that it is basically as costly to compute either the min or the
 * max of the Objective, and to compute *both* the min and the max. So, this
 * is what BoxSolver does: each time compute() is called, it computes and
 * makes it available both the minimum and the maximum of the objective,
 * (almost) regardless to what the original verse was. */

class BoxSolver : public CDASolver
{
/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */

 /// public enum "extending" int_par_type_CDAS to BoxSolvers

 enum int_par_type_BoxS {
  intPDSol = intLastParCDAS ,

  intLastParBoxS    ///< first allowed parameter value for derived classes
                    /**< convenience value for easily allow derived classes
                     * to further extend the set of types of return codes */
  };

/*--------------------------------------------------------------------------*/
/*----------------- CONSTRUCTING AND DESTRUCTING BoxSolver -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing BoxSolver
 *  @{ */

 /// empty constructor
 BoxSolver( void ) : CDASolver() , f_sol( 0 ) , f_status( kUnEval ) ,
  f_max_val( - Inf< OFValue >() ) , f_min_val( Inf< OFValue >() ) ,
  f_verse( -1 ) {}

/*--------------------------------------------------------------------------*/
 /// destructor: it really does nothing since v_mod is empty

 virtual ~BoxSolver() { }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 void set_Block( Block * block ) override {
  CDASolver::set_Block( block );
  f_verse = -1;
  reset();
  }

/*--------------------------------------------------------------------------*/
 /// set the (unique) integer parameter of BoxSolver, see set_sol()

 void set_par( idx_type par , int value ) override {
  if( par == intPDSol ) {
   sol = char( value );
   return;
   }
  CDASolver::set_par( par , value );
  }

/*--------------------------------------------------------------------------*/
 /// decide if the primal and/or dual solution is computed
 /** Decide if the primal and/or dual solution is computed. Since this has
  * the same cost as producing the bound(s), it is EITHER DONE DURING THE
  * SOLUTION PROCESS AND THE OPTIMAL PRIMAL AND / OR DUAL SOLUTION IS
  * IMMEDIATELY WRITTEN IN THE ColVariable / DUAL VALUES OF THE
  * :OneVarConstraint, OR NOT AT ALL.
  *
  * @param sol is a char, coded bit-wise, that decides if BoxSolver produces
  *        a primal and/or dual optimal solution 
  *
  *        - bit 0: the primal optimal solution is produced
  *
  *        - bit 1: the primal dual solution is produced
  *
  * Note that there is the usual issue with dual solutions: if one of the
  * "inherent" bounds of a ColVariable has a nonzero dual value but there is
  * no :OneVarConstraint with the same bound, then there is no place where to
  * store the dual value and it is "lost". If this is a problem, the Block
  * must always have the bounds specified via the :OneVarConstraint. */

 void set_sol( char sol = 0 ) { f_sol = sol; }

/**@} ----------------------------------------------------------------------*/
/*--------------------- METHODS FOR SOLVING THE MODEL ----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Solving the model encoded by the current Block
 *  @{ */

 /// solve the model encoded in the Block
 /** BoxSolver not only solves the model encoded in the Block (or a
  * relaxation thereof if there are other Constraint save the "box" ones),
  * but at the same time computes the maximum and the minimum of the
  * Objective over the feasible region. Of course the two can be infinite.
  * Also, compute() immediately writes the primal/dual optimal solution in
  * the ColVariable / dual values of the :OneVarConstraint, if so
  * instructed. */

 int compute( bool changedvars = true ) override;

/**@} ----------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/

 bool has_var_solution( void ) override { return( f_sol & 1 ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 bool has_dual_solution( void ) override { return( f_sol & 2 ); }

/*--------------------------------------------------------------------------*/

 OFValue get_var_value( void ) override {
  return( f_verse == 1 ? : f_max_val : f_min_val );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/




/*--------------------------------------------------------------------------*/

 void get_var_solution( Configuration *solc = nullptr ) override {}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void get_dual_solution( Configuration *solc = nullptr ) override {}

/*--------------------------------------------------------------------------*/

 OFValue get_lb( void ) override {
  return( f_verse == 1 ? : f_max_val : f_min_val );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 OFValue get_ub( void ) override {
  return( f_verse == 1 ? : f_max_val : f_min_val );
  }

/*--------------------------------------------------------------------------*/
 /// return the "opposite" bound w.r.t. get_[lb/ub]()
 /** While get_[lb/ub]() return the same value, corresponding to the
  * optimization with the verse specified by the objective,
  * get_opposite_bound() returns the value corresponding to the optimization
  * with the opposite verse. */

 OFValue get_opposite_bound( void ) override {
  return( f_verse == 0 ? : f_max_val : f_min_val );
  }

/**@} ----------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the Solver
 *  @{ */

 [[nodiscard]] idx_type get_num_int_par( void ) const override {
  return( CDASolver::get_num_int_par() + 1 );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] int get_dflt_int_par( idx_type par ) const override {
  return( par == intPDSol ? 0 : CDASolver::get_dflt_int_par( par ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] int get_int_par( idx_type par ) const override {
  return( par == intPDSol ? f_sol : CDASolver::get_int_par( par ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] idx_type int_par_str2idx( const std::string & name )
  const override {
  return( name == "intPDSol" ? intPDSol
	                     : CDASolver::int_par_str2idx( name ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] const std::string & int_par_idx2str( idx_type idx )
  const override {
  return( idx == intPDSol ?  "intPDSol"
	                  : CDASolver:::int_par_idx2str( idx ) );
  }


/**@} ----------------------------------------------------------------------*/
/*------------- METHODS FOR ADDING / REMOVING / CHANGING DATA --------------*/
/*--------------------------------------------------------------------------*/
/** @name Changing the data of the model
 *  @{ */

 void add_Modification( sp_Mod & mod ) override;

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

 void check_verse( Block * blck );
 
/*--------------------------------------------------------------------------*/

 void reset( void ) {
  f_max_val = - Inf< OFValue >();
  f_min_val = Inf< OFValue >();
  }
 
/*--------------------------------------------------------------------------*/
/*--------------------------- PRIVATE FIELDS -------------------------------*/
/*--------------------------------------------------------------------------*/

 char f_sol;        ///< whether the primal and/or dual solution is computed

 OFValue f_max_val;    ///< maximum of the Objective over the box

 OFValue f_min_val;    ///< minimum of the Objective over the box

 int f_verse;
 ///< 1 if the Objective is max, 0 if it is min, -1 if it has to be computed

/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };   // end( class BoxSolver )

/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* BoxSolver.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File BoxSolver.h ---------------------------*/
/*--------------------------------------------------------------------------*/





