/*--------------------------------------------------------------------------*/
/*-------------------------- File R3BSolver.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the R3BSolver class, which implements a Solver that solves
 * the Block it is attached to by means of one of its R3 Block: it makes the
 * R3 Block, attaches a Solver to it, forwards to it the Modification of the
 * original Block, and maps back the solution and the bounds that the Solver
 * of the R3 Block finds.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __R3BSolver
 #define __R3BSolver
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"

#include "BlockSolverConfig.h"

#include "Objective.h"

/*--------------------------------------------------------------------------*/
/*----------------------------- NAMESPACE ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup R3BSolver_CLASSES Classes in R3BSolver.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS R3BSolver -------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// Solver that solves a Block through one of its R3 Block
/** The concrete R3BSolver class derives from Solver and implements a Solver
 * that does not solve the Block it is attached to by itself, but through one
 * of its R3 Block [see Block::get_R3_Block()]: when it is attached to a
 * Block, it asks the Block for the R3 Block, BlockConfig-ures it,
 * attaches a Solver (the "inner Solver") to it, and from then on
 *
 * - every Modification of the original Block is mapped to the R3 Block
 *   as soon as it reaches R3BSolver [see
 *   Block::map_forward_Modification()], exactly as UpdateSolver does, and
 *   hence it reaches the inner Solver as a Modification of the R3 Block;
 *
 * - compute() is that of the inner Solver;
 *
 * - get_var_solution() has the inner Solver write its solution in the R3
 *   Block, and the original Block map it back in its own Variable [see
 *   Block::map_back_solution()];
 *
 * - get_lb() and get_ub() are those of the inner Solver, but only on the
 *   side where the R3 Block gives a valid bound for the original Block:
 *   both if the R3 Block is a Reformulation, only the dual bound (the lower
 *   one for a minimization problem, the upper one for a maximization one)
 *   if it is a Relaxation, only the primal bound if it is a Restriction
 *   [see intR3BKind]; the other is - / + Inf< OFValue >().
 *
 * R3BSolver is configured from file only, through the "extra" Configuration
 * of its ComputeConfig [see set_ComputeConfig()], which is a
 *
 *     SimpleConfiguration< std::vector< Configuration * > >
 *
 * with (up to) three elements:
 *
 * - the Configuration that makes the R3 Block, i.e., the r3bc argument of
 *   Block::get_R3_Block(), Block::map_forward_Modification() and
 *   Block::map_back_solution(); it may be nullptr, which asks for the
 *   default R3 Block of the Block (usually its copy);
 *
 * - the BlockConfig of the R3 Block, applied to it as soon as it is made;
 *   it may be nullptr (or missing), in which case the R3 Block is left as
 *   the original Block makes it;
 *
 * - the BlockSolverConfig that attaches the Solver to the R3 Block; it
 *   must be there, and the first Solver that it registers to the R3 Block is
 *   the inner Solver. Other Solver may be registered as well, e.g., to the
 *   sub-Block of the R3 Block, as the inner Solver may need; all of them
 *   receive the Modification of the R3 Block, but only the inner Solver is
 *   compute()-d by R3BSolver.
 *
 * The R3 Block is made when R3BSolver has both a Block and the extra
 * Configuration, i.e., at the latest of set_Block() and set_ComputeConfig(),
 * and it is made again (the old one being deleted with its Solver) if
 * either changes. It belongs to R3BSolver, which deletes it, together with
 * all the Solver that the BlockSolverConfig has registered to it, when it
 * is detached from the Block or deleted. The Modification issued by the
 * original Block before the R3 Block is made are not needed, as the R3
 * Block is made from the current state of the original Block.
 *
 * A Modification that the original Block does not map [see the return
 * value of Block::map_forward_Modification()] is ignored, as it is in
 * UpdateSolver: the R3 Block is then out of sync with the original one,
 * and it is up to the original Block to map all the Modification that
 * matter to the R3 Block it makes.
 *
 * The return value of compute(), has_var_solution(), new_var_solution()
 * and get_var_value() are those of the inner Solver, i.e., they refer to
 * the R3 Block; in particular, a solution of a Relaxation mapped back to
 * the original Block need not be feasible for it, hence is_var_feasible()
 * returns false for a Relaxation.
 *
 * R3BSolver is added to the Solver factory in R3BSolver.cpp. */

class R3BSolver : public Solver {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
 *  @{ */

 /// public enum "extending" int_par_type_S to R3BSolver

 enum int_par_type_R3BS {
  intR3BKind = intLastAlgPar ,  ///< what the R3 Block is
                                /**< What the R3 Block is with respect to
  * the original Block, which decides which bound of the inner Solver is
  * also a bound for the original Block [see kind_of_R3B]: 0
  * (kReformulation, the default) for a Reformulation, 1 (kRelaxation) for
  * a Relaxation, 2 (kRestriction) for a Restriction. */

  intLastParR3BS  ///< first allowed parameter value for derived classes
                  /**< convenience value for easily allow derived classes
                   * to further extend the set of types of parameters */
  };

/*--------------------------------------------------------------------------*/
 /// what the R3 Block is with respect to the original Block

 enum kind_of_R3B {
  kReformulation = 0 ,  ///< same optimal value: both bounds pass
  kRelaxation = 1 ,     ///< a dual bound: only that bound passes
  kRestriction = 2      ///< a primal bound: only that bound passes
  };

/** @} ---------------------------------------------------------------------*/
/*---------------- CONSTRUCTING AND DESTRUCTING R3BSolver ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing R3BSolver
 *  @{ */

 /// empty constructor: the R3 Block is made when there is all it needs

 R3BSolver( void ) : Solver() , f_kind( kReformulation ) , f_R3Cfg( nullptr ) ,
  f_R3B( nullptr ) , f_inner( nullptr ) , f_R3BSC( nullptr ) { }

/*--------------------------------------------------------------------------*/
 /// destructor: deletes the R3 Block, its Solver and the Configuration

 virtual ~R3BSolver();

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the Block: the R3 Block of the old one is deleted, that of the new
 /// one made if the extra Configuration is there

 void set_Block( Block * block ) override;

/*--------------------------------------------------------------------------*/
 /// set the parameters and the extra Configuration
 /** Sets the parameters as the base class does, and then takes care of
  * the extra Configuration of \p scfg, which has to be the
  * SimpleConfiguration< std::vector< Configuration * > > described in the
  * general notes of the class. A copy of it is kept, and if a Block is
  * already there its R3 Block is made again with the new one. If \p scfg
  * is nullptr, or it is not differential, the parameters are reset to
  * their default and the extra Configuration (and with it the R3 Block)
  * is dropped, unless \p scfg gives a new one; if \p scfg is differential
  * and gives no extra Configuration, the current one is kept. */

 void set_ComputeConfig( const ComputeConfig * scfg = nullptr ) override;

/*--------------------------------------------------------------------------*/
 using Solver::set_par;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set the (only) integer parameter of R3BSolver, intR3BKind

 void set_par( idx_type par , int value ) override {
  if( par == intR3BKind ) {
   if( ( value < kReformulation ) || ( value > kRestriction ) )
    throw( std::invalid_argument( "R3BSolver::set_par: intR3BKind = " +
				  std::to_string( value ) +
				  " is not 0, 1 or 2" ) );
   f_kind = value;
   return;
   }
  Solver::set_par( par , value );
  }

/** @} ---------------------------------------------------------------------*/
/*--------------------- METHODS FOR SOLVING THE MODEL ----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Solving the model encoded by the current Block
 *  @{ */

 /// solves the R3 Block with the inner Solver
 /** Returns what compute( \p changedvars ) of the inner Solver returns, or
  * kError if there is no inner Solver yet (no Block, or no extra
  * Configuration). The Modification of the original Block have all been
  * mapped to the R3 Block already, as they came. */

 int compute( bool changedvars = true ) override;

/** @} ---------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Accessing the found solutions (if any)
 *  @{ */

 /// the time of the last compute() of the inner Solver

 [[nodiscard]] double get_elapsed_time( void ) const override {
  return( f_inner ? f_inner->get_elapsed_time() : 0 );
  }

/*--------------------------------------------------------------------------*/
 /// a valid lower bound on the optimal value of the original Block
 /** The lower bound of the inner Solver if it is also a lower bound of the
  * original Block, i.e., if the R3 Block is a Reformulation, a Relaxation
  * of a minimization problem or a Restriction of a maximization one; -
  * Inf< OFValue >() otherwise. */

 [[nodiscard]] OFValue get_lb( void ) override;

/*--------------------------------------------------------------------------*/
 /// a valid upper bound on the optimal value of the original Block
 /** The upper bound of the inner Solver if it is also an upper bound of the
  * original Block, i.e., if the R3 Block is a Reformulation, a Relaxation
  * of a maximization problem or a Restriction of a minimization one;
  * Inf< OFValue >() otherwise. */

 [[nodiscard]] OFValue get_ub( void ) override;

/*--------------------------------------------------------------------------*/
 /// whether the inner Solver has a solution of the R3 Block

 [[nodiscard]] bool has_var_solution( void ) override {
  return( f_inner ? f_inner->has_var_solution() : false );
  }

/*--------------------------------------------------------------------------*/
 /// whether the solution mapped back is feasible for the original Block
 /** That of the inner Solver for a Reformulation and a Restriction, false
  * for a Relaxation, whose solutions need not be feasible for the original
  * Block. */

 [[nodiscard]] bool is_var_feasible( void ) override {
  return( f_inner && ( f_kind != kRelaxation ) ?
	  f_inner->is_var_feasible() : false );
  }

/*--------------------------------------------------------------------------*/
 /// the objective value of the solution of the inner Solver

 [[nodiscard]] OFValue get_var_value( void ) override;

/*--------------------------------------------------------------------------*/
 /// writes the solution of the R3 Block in the original Block
 /** The inner Solver writes the (whole) solution it has in the R3 Block,
  * and then the original Block maps it back in its own Variable, \p solc
  * telling which ones [see Block::map_back_solution()]. */

 void get_var_solution( Configuration * solc = nullptr ) override;

/*--------------------------------------------------------------------------*/
 /// whether the inner Solver has a new solution of the R3 Block

 [[nodiscard]] bool new_var_solution( void ) override {
  return( f_inner ? f_inner->new_var_solution() : false );
  }

/** @} ---------------------------------------------------------------------*/
/*-------------- METHODS FOR READING THE DATA OF THE Solver ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the R3BSolver
 *  @{ */

 /// the R3 Block, nullptr if it has not been made yet

 [[nodiscard]] Block * get_R3B( void ) const { return( f_R3B ); }

/*--------------------------------------------------------------------------*/
 /// the inner Solver, nullptr if the R3 Block has not been made yet

 [[nodiscard]] Solver * get_inner_Solver( void ) const { return( f_inner ); }

/*--------------------------------------------------------------------------*/
 /// the Configuration that makes the R3 Block, nullptr if none

 [[nodiscard]] Configuration * get_R3B_Configuration( void ) const {
  return( ( f_R3Cfg && ( ! f_R3Cfg->f_value.empty() ) ) ?
	  f_R3Cfg->f_value[ 0 ] : nullptr );
  }

/*--------------------------------------------------------------------------*/
 /// the ComputeConfig, with a copy of the extra Configuration (if any)

 [[nodiscard]] ComputeConfig * get_ComputeConfig( bool all = false ,
				  ComputeConfig * ocfg = nullptr ) const override;

/*--------------------------------------------------------------------------*/

 [[nodiscard]] idx_type get_num_int_par( void ) const override {
  return( idx_type( intLastParR3BS ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] int get_dflt_int_par( idx_type par ) const override {
  return( par == intR3BKind ? int( kReformulation )
	                    : Solver::get_dflt_int_par( par ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] int get_int_par( idx_type par ) const override {
  return( par == intR3BKind ? f_kind : Solver::get_int_par( par ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] idx_type int_par_str2idx( const std::string & name )
  const override {
  return( name == "intR3BKind" ? idx_type( intR3BKind )
	                       : Solver::int_par_str2idx( name ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] const std::string & int_par_idx2str( idx_type idx )
  const override {
  static const std::string _parnm( "intR3BKind" );
  return( idx == intR3BKind ? _parnm : Solver::int_par_idx2str( idx ) );
  }

/** @} ---------------------------------------------------------------------*/
/*------------- METHODS FOR ADDING / REMOVING / CHANGING DATA --------------*/
/*--------------------------------------------------------------------------*/
/** @name Changing the data of the model
 *  @{ */

 /// maps the Modification to the R3 Block at once
 /** Rather than storing the Modification, it immediately maps it to the R3
  * Block [see Block::map_forward_Modification()], so that the inner Solver
  * receives the corresponding Modification of the R3 Block; nothing is done
  * if there is no R3 Block yet, or if the Modification are inhibited [see
  * inhibit_Modification()]. */

 void add_Modification( sp_Mod & mod ) override;

/** @} ---------------------------------------------------------------------*/
/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/

 /// makes the R3 Block and attaches the Solver to it, if all is there

 void make_R3B( void );

/*--------------------------------------------------------------------------*/
 /// deletes the R3 Block and the Solver attached to it, if any

 void delete_R3B( void );

/*--------------------------------------------------------------------------*/
 /// whether the original Block is a minimization problem

 [[nodiscard]] bool is_min( void ) const {
  return( f_Block ? f_Block->get_objective_sense() == Objective::eMin
	          : true );
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS -----------------------------*/
/*--------------------------------------------------------------------------*/

 int f_kind;  ///< what the R3 Block is, see intR3BKind

 /// the extra Configuration: r3bc, BlockConfig and BlockSolverConfig
 SimpleConfiguration< std::vector< Configuration * > > * f_R3Cfg;

 Block * f_R3B;      ///< the R3 Block

 Solver * f_inner;   ///< the inner Solver, attached to the R3 Block

 /// the clear()-ed copy of the BlockSolverConfig applied to the R3 Block,
 /// whose apply() deletes the Solver it has registered
 BlockSolverConfig * f_R3BSC;

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };   // end( class R3BSolver )

/** @} end( group( R3BSolver_CLASSES ) ) -----------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* R3BSolver.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File R3BSolver.h ---------------------------*/
/*--------------------------------------------------------------------------*/
