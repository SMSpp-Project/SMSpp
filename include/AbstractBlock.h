/*--------------------------------------------------------------------------*/
/*------------------------ File AbstractBlock.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class AbstractBlock, which represent implements the
 * Block concept for the case where
 * 
 *      only the abstract representation exist
 *
 * and
 *
 *      it is externally provided
 *
 * This means that a lot of Block mechanisms do not apply, but on the other
 * hand a few mechanisms that are usually thought to be implemented by
 * derived classes, and therefore are protected, need to be made public.
 *
 * \version 0.10
 *
 * \date 24 - 07 - 2019
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __AbstractBlock
 #define __AbstractBlock
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <fstream>
#include <sstream>
#include <iomanip>

#include "Block.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS AbstractBlock ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// implementation of the Block concept for the "purely abstract" case
/** The class AbstractBlock implements the Block concept for the case where
 * only the abstract representation exist *and it is externally provided*.
 * This means that it has to be programmatically constructed by some piece
 * of code *outside* the AbstractBlock.
 *
 * The current implementation of AbstractBlock requires:
 *
 * - all Variable to be ColVariable;
 *
 * - all Constraint to be either FRowConstraint or (any concrete class
 *   derived from the abstract) OneVarConstraint;
 *
 * - the Objective to be a FRealObjective.
 *
 * These are to be constructed outside and passed to the object via the
 * corresponding add_[static/dynamic]_[constraint/variable]() methods, that
 * for this reason *are made public* (while they are protected in the base
 * Block class), together with the other ones for handling the corresponding
 * data structures. Similarly, AbstractBlock allows unfettered access to the
 * list of inner Block by returning a non-const reference to the
 * corresponding vector of pointers (unlike Block). Of course, changes in
 * the size of these vectors *not* supposed to happen "in flight", i.e., when
 * there is any Solver attached to the AbstractBlock, but only during the
 * initial construction phase.
 *
 * Note that, conversely, add_dynamic_constraint*s*() and
 * add_dynamic_variable*s*() (the methods for adding/removing stuff from an
 * existing list, rather than for creating a new group of constraint, which
 * in the dynamic case entails one or more lists), are already public in the
 * base class. While "concrete" Block may want to redefine them, the
 * implamentations in the base class do all that is needed here.
 *
 * In AbstractBlock, *the abstract representation is the physical
 * representation*: there is no other reference to the Variable/Constraint
 * save that in those data structures. Hence, the destructor of AbstractBlock
 * (unlike that of Block) takes care of (clear()-ing first, and) destroying
 * (then) all the "abstract" Constraint/Variable (and the Objective).
 * Similarly, the pointers to the inner Block in the v_Block vector are
 * likely the only live references to these Block, and therefore they are
 * destroyed in the AbstractBlock destructor.
 *
 * Among other things, AbstractBlock can be a useful target for the
 * construction of some R3Block for some "concrete" Block. */

class AbstractBlock : public Block {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*-------------- CONSTRUCTING AND DESTRUCTING AbstractBlock ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing AbstractBlock
 *  @{ */

 /// constructor of AbstractBlock, taking a pointer to the father Block
 /** Constructor of AbstractBlock. It accepts a pointer to the father Block
  * (defaulting to nullptr, both because the root Block has no father and so
  * that this can also be used as the void constructor), passes it to the
  * Block constructor, and does little else.. */

 AbstractBlock( Block * father = nullptr ) : Block( father ) ,
  f_ub( Inf<double>() ) , f_lb( - Inf<double>() ) ,
  f_ub_cond( false ) , f_lb_cond( false ) {}

/*--------------------------------------------------------------------------*/
 /// destructor of AbstractBlock, destroys the abstract representation
 /** The destructor of AbstractBlock (unlike that of Block) takes care of
  * (clear()-ing first, and) destroying (then) all the "abstract"
  * Constraint/Variable (and the Objective). Similarly, the pointers to the
  * inner Block in the v_Block vector are likely the only live references to
  * these Block, and therefore they are destroyed in the AbstractBlock
  * destructor. */

 virtual ~AbstractBlock( );

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set a (either globally valid or conditionally valid) upper bound
 /** Upper bounds (either globally valid or conditionally valid ones) cannot
  * reasonably be computed by the AbstractBlock itself; so, if any is known
  * "from the outside", it has to be explicitly passed to the AbstractBlock
  * so that the latter can rely it to any attached Solver. Only one of the
  * two kinds of bounds makese sense at any given time (if a globally valid
  * upper bound is known then the problem is not unbounded above and there
  * is no point in checking for a conditionally valid one), so the method
  * sets the globally valid bound if the conditional parameter is false, and
  * the conditionally valid one otherwise, with the other being automatically
  * set to + infinity. */

 void set_valid_upper_bound( const double newub = + Inf<double>() ,
			     const bool conditional = false )
 {
  f_ub = newub; f_ub_cond = conditional;
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set a (either globally valid or conditionally valid) lower bound
 /** Lower bounds (either globally valid or conditionally valid ones) cannot
  * reasonably be computed by the AbstractBlock itself; so, if any is known
  * "from the outside", it has to be explicitly passed to the AbstractBlock
  * so that the latter can rely it to any attached Solver. Only one of the
  * two kinds of bounds makese sense at any given time (if a globally valid
  * lower bound is known then the problem is not unbounded below and there
  * is no point in checking for a conditionally valid one), so the method
  * sets the globally valid bound if the conditional parameter is false, and
  * the conditionally valid one otherwise, with the other being automatically
  * set to - infinity. */

 void set_valid_lower_bound( const double newlb = - Inf<double>() ,
			     const bool conditional = false )
 {
  f_lb = newlb; f_lb_cond = conditional;
  }

/**@} ----------------------------------------------------------------------*/
/*----------------- Methods for reading the data of the Block --------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the Block
 *  @{ */

 /// allow unfettered access to nested Block
 /** Returns a non-const reference to the vector of pointers of inner Block,
  * so that it can be freely modified. Of course, this has to be done with
  * great care, and *not* as soon as there is any solver attached to the
  * AbstractBlock. */

 Vec_Block & access_nested_Blocks( void ) { return( v_Block ); }

/*--------------------------------------------------------------------------*/

 virtual double get_valid_upper_bound( const bool conditional = false )
  override final {
  return( f_ub_cond == conditional ? f_ub : + Inf<double>() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 virtual double get_valid_lower_bound( const bool conditional = false )
  override final {
  return( f_lb_cond == conditional ? f_lb : - Inf<double>() );
  }

/**@} ----------------------------------------------------------------------*/
/*----- Methods for adding/removing (dynamic) Variables and Constraints ----*/
/*--------------------------------------------------------------------------*/
/** @name Methods for changing Variables, Constraints and Objective
 *
 * The protected methods of the base Block class
 *
 * - reset_static_constraints
 *
 * - reset_static_variables
 *
 * - reset_dynamic_constraints
 *
 * - reset_dynamic_variables
 *
 * - reset_objective
 *
 * - add_static_constraint (all versions)
 *
 * - add_static_variable (all versions)
 *
 * - add_dynamic_constraint (all versions)
 *
 * - add_dynamic_variable (all versions)
 *
 * are made public in AbstractBlock, so that they can be used "from outside"
 * the AbstractBlock to manage its abstract representation;
 *  @{ */
 
 using Block::reset_static_constraints;

 using Block::reset_static_variables;

 using Block::reset_dynamic_constraints;

 using Block::reset_dynamic_variables;

 using Block::reset_objective;

 using Block::add_static_constraint;

 using Block::add_static_variable;

 using Block::add_dynamic_constraint;

 using Block::add_dynamic_variable;

 /**@} ----------------------------------------------------------------------*/
/*----------------- Methods for checking the AbstractBlock -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for checking solution information in the AbstractBlock
 **/

 /// returns true if the current solution is (approximately) feasible
 /** Returns true if the solution encoded in the current value of the
  * Variable of the Block is approximately feasible within the given
  * tolerances. The method works by basically calling is_feasible() on all
  * the [Col]Variable and [FRow/OneVar]Constraint of the abstract
  * representation, for there clearly is no other possible way to do this.
  *
  * The parameter for deciding what "approximately feasible" exactly means is
  * a single double value, representing the *relative* tolerance for
  * satisfaction of all [FRow/OneVar]Constraint, and domain restrictions for
  * [Col]Variable. This value is to be found as:
  *
  * - if fsbc is not nullptr and it is a SimpleConfiguration<double>, then it
  *   if fsbc->f_value;
  *
  * - otherwise, if f_BlockConfig is not nullptr,
  *   f_BlockConfig->f_is_feasible_Configuration is not nullptr and it
  *   is a SimpleConfiguration<double>, then it is
  *   f_BlockConfig->f_is_feasible_Configuration->f_value;
  *
  * - otherwise, it is 0. */

 virtual bool is_feasible( bool useabstract = false ,
			   Configuration *fsbc = nullptr ) override final;

/**@} ----------------------------------------------------------------------*/
/*----------------------- Methods for handling Solution --------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling Solution
 *  @{ */

 /// returns a Solution representing the current solution of this Block
 /** This method must construct and return a (pointer to a) Solution object
  * representing the current "solution state" of this Block. For an
  * AbstractBlock, the "only reasonable" Solution is a ColVariableSolution.
  */

 virtual Solution * get_Solution( Configuration *solc = nullptr ,
				  bool emptys = true ) override final;

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/** @name Protected methods for inserting and extracting
 */

 /// print information about the AbstractBlock on an ostream 
 /** Protected method intended to print information about the AbstractBlock;
  * it basically goes over all the abstract representation (static and dynamic
  * Variable and Constraint, Objective, and inner Block) and asks everyone to
  * print itself. */

 virtual void print( std::ostream &output ) const override final;

/*--------------------------------------------------------------------------*/
 /// load the Block out of an istream

 // provide an empty implementation of load() to make the class concrete
 virtual void load( std::istream &input ) override final {}

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 double f_ub;          ///< the global upper bound

 double f_lb;          ///< the global lower bound

 bool f_ub_cond;       ///< wether f_ub is only conditionally valid

 bool f_lb_cond;       ///< wether f_lb is only conditionally valid

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h;

 /*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

 };  // end( class( AbstractBlock ) )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* AbstractBlock.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File AbstractBlock.h ------------------------*/
/*--------------------------------------------------------------------------*/
