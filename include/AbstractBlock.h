/*--------------------------------------------------------------------------*/
/*------------------------ File AbstractBlock.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class AbstractBlock, which implements the Block
 * concept in two relevant cases:
 *
 * 1) THE Block ONLY HAVE THE ABSTRACT REPRESENTATION, WHICH IS EXTERNALLY
 *    PROVIDED, I.E., IT IS PROGRAMMATICALLY CONSTRUCTED BY SOME PIECE OF
 *    CODE OUTSIDE THE Block ITSELF.
 *
 *    This means that a lot of Block mechanisms do not apply, but on the
 *    other hand a few mechanisms that are usually thought to be implemented
 *    by derived classes, and therefore are protected, need to be made
 *    public.
 *
 * 2) THE Block HAS "PARTLY" A SPECIFIC STRUCTURE, BUT THEN A (MORE OR LESS)
 *    ARBITRARY SET OF Variable AND Contraint CAN BE ADDED WITH BASICALLY
 *    ANY STRUCTURE.
 *
 *    The idea is that any such Block can then be implemented into a derived
 *    class PartiallyStructuredBlock : AbstractBlock, where the derived class
 *    manages the "specific structure" part and AbstractBlock the "arbitrary"
 *    one. For this purpose, AbstractBlock has a few methods that allow
 *    derived classes to "reserve for themselves a part of the abstract
 *    representation", ensuring that AbstractBlock will not mess up with it
 *    (but, on the other hand, completely making the derived class' job to
 *    handle all its aspects).
 *
 * \version 0.20
 *
 * \date 01 - 09 - 2019
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
/** The class AbstractBlock implements the Block concept for the case for 
 * two relevant cases:
 *
 * 1) THE Block ONLY HAVE THE ABSTRACT REPRESENTATION, WHICH IS EXTERNALLY
 *    PROVIDED, I.E., IT IS PROGRAMMATICALLY CONSTRUCTED BY SOME PIECE OF
 *    CODE OUTSIDE THE Block ITSELF.
 *
 *    This means that a lot of Block mechanisms do not apply, but on the
 *    other hand a few mechanisms that are usually thought to be implemented
 *    by derived classes, and therefore are protected, need to be made
 *    public.
 *
 * 2) THE Block HAS "PARTLY" A SPECIFIC STRUCTURE, BUT THEN A (MORE OR LESS)
 *    ARBITRARY SET OF Variable AND Contraint CAN BE ADDED WITH BASICALLY
 *    ANY STRUCTURE.
 *
 *    The idea is that any such Block can then be implemented into a derived
 *    class PartiallyStructuredBlock : AbstractBlock, where the derived class
 *    manages the "specific structure" part and AbstractBlock the "arbitrary"
 *    one. For this purpose, AbstractBlock has a few methods that allow
 *    derived classes to "reserve for themselves a part of the abstract
 *    representation", ensuring that AbstractBlock will not mess up with it
 *    (but, on the other hand, completely making the derived class' job to
 *    handle all its aspects).
 *
 * The current implementation of AbstractBlock requires (for the "arbitrary"
 * part, as any specific part need necessarily be handled by a derived class):
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
 * the size of these vectors are *not* supposed to happen "in flight", i.e.,
 * when there is any Solver attached to the AbstractBlock, but only during
 * the initial construction phase.
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
 * construction of some R3Block for some "concrete" Block.
 *
 * However, AbstractBlock has methods that allow derived classes to specify
 * that:
 *
 * - the first sv groups of static Variable;
 *
 * - the first dv groups of dynamic Variable;
 *
 * - the first sc groups of static Constraint;
 *
 * - the first dc groups of dynamic Constraint;
 *
 * - the (only) objective;
 *
 * - the first sb inner Block;
 *
 * are "reserved for the derived class use". The base AbstractBlock will not
 * ever touch these elements. However, this means that all the parts that
 * are directly handled by the derived class must be completely handled by
 * it. In particular, it is expected that they are:
 *
 * - completely done after the AbstractBlock is out of deserialize(), so
 *   that it is then possible to add other groups of Variable / Constraint
 *   or inner Block AFTER THE ONES ALREADY THERE;
 *
 * - completely destroyed in the destructor of the derived class, which by
 *   definition is executed *before* that of AbstractBlock; this means that
 *   deleting the "specific structure" part must not leave any loose ends
 *   in the "arbitrary" one. */

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
  * Block constructor, and does little else. */

 AbstractBlock( Block * father = nullptr ) : Block( father ) ,
  f_ub( Inf<double>() ) , f_lb( - Inf<double>() ) ,
  f_ub_cond( false ) , f_lb_cond( false ) {}

/*--------------------------------------------------------------------------*/
 /// de-serialize the current :AbstractBlock out of netCDF::NcGroup
 /** The AbstractBlock de-serializes itself out of a netCDF::NcGroup. The
  * method of the base AbstractBlock class only handles
  *
  *      OR, BETTER, WILL HANDLE WHEN THIS METHOD WILL BE IMPLEMENTED
  *
  * the "arbitrary" part, i.e., that after the "reserved" one as dictated
  * by the get_first_*_*() methods. This in particular means that derived
  * classes will have to read their data first, at least enough that these
  * methods give the right answer, before calling the AbstractBlock version
  * of the method.
  *
  * In the current, partial implementation of the method, besides the
  * mandatory "type" attribute of any :Block, the group should contain the
  * following:
  *
  * - the dimension "NumberInnerBlock", containing the number of the
  *   inner Block. The dimension is optional, it is is not provided 0 is
  *   assumed.
  *
  * - if NumberInnerBlock > 0, and in particular it is
  *   > get_first_inner_Block(), then the groups "Block_<i>", for i = 0,. ...
  *   get_first_inner_Block() - 1; each one must contain the deserialization
  *   of the i-th inner Block.
  */

 virtual void deserialize( netCDF::NcGroup & group ) override;

/*--------------------------------------------------------------------------*/
 /// destructor of AbstractBlock, destroys the abstract representation
 /** The destructor of AbstractBlock (unlike that of Block) takes care of
  * (clear()-ing first, and) destroying (then) all the "abstract"
  * Constraint/Variable (and the Objective). Similarly, the pointers to the
  * inner Block in the v_Block vector are likely the only live references to
  * these Block, and therefore they are destroyed in the AbstractBlock
  * destructor.
  *
  * Derived classes which speficic structures will have to (but, anyway, they
  * necessarily have to) define their destructor to take care of them, which
  * is executed before that of AbstractBlock; this therefore assumes that all
  * specific structures have been dealt with and happily proceeds with
  * deleting the "arbitrary part". */

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
/*------------- Methods for reading the data of the AbstractBlock ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the AbstractBlock
 *  @{ */

 /// returns the index of the first group of "available" static Constraint
 /** This method returns the index of the first group of static Constraint in
  * the "arbitrary" part of the AbstractBlock. The implementation in the base
  * AbstractBlock class returns 0, i.e., there are no "reserved" static
  * Constraint. */

 virtual Index get_first_static_Constraint( void ) const { return( 0 ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the index of the first group of "available" dynamic Constraint
 /** This method returns the index of the first group of dynamic Constraint in
  * the "arbitrary" part of the AbstractBlock. The implementation in the base
  * AbstractBlock class returns 0, i.e., there are no "reserved" dynamic
  * Constraint. */

 virtual Index get_first_dynamic_Constraint( void ) const { return( 0 ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the index of the first group of "available" static Variable
 /** This method returns the index of the first group of static Variable in
  * the "arbitrary" part of the AbstractBlock. The implementation in the base
  * AbstractBlock class returns 0, i.e., there are no "reserved" static
  * Variable. */

 virtual Index get_first_static_Variable( void ) const { return( 0 ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the index of the first group of "available" dynamic Variable
 /** This method returns the index of the first group of dynamic Variable in
  * the "arbitrary" part of the AbstractBlock. The implementation in the base
  * AbstractBlock class returns 0, i.e., there are no "reserved" dynamic
  * Variable. */

 virtual Index get_first_dynamic_Variable( void ) const { return( 0 ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the index of the first "available" inner Block
 /** This method returns the index of the first inner Block in the
  * "arbitrary" part of the AbstractBlock. The implementation in the base
  * AbstractBlock class returns 0, i.e., there are no "reserved" inner Block.
  */

 virtual Index get_first_inner_Block( void ) const { return( 0 ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// tells if the Objective is reserved
 /** This method returns true if the Objective is handled by the derived
  * class, i.e., it is not in the "arbitrary" part of the AbstractBlock. The
  * implementation in the base AbstractBlock class returns false, i.e., the
  * Objective is in the "arbitrary" part of the AbstractBlock. */

 virtual bool is_Objective_reserved( void ) const { return( false ); }

/*--------------------------------------------------------------------------*/
 /// allow unfettered access to nested Block
 /** Returns a non-const reference to the vector of pointers of inner Block,
  * so that it can be freely modified. Of course, this has to be done with
  * great care, and *not* as soon as there is any solver attached to the
  * AbstractBlock. */

 Vec_Block & access_nested_Blocks( void ) { return( v_Block ); }

/*--------------------------------------------------------------------------*/

 virtual double get_valid_upper_bound( const bool conditional = false )
  override {
  return( f_ub_cond == conditional ? f_ub : + Inf<double>() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 virtual double get_valid_lower_bound( const bool conditional = false )
  override {
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
			   Configuration *fsbc = nullptr ) override;

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
				  bool emptys = true ) override;

/**@} ----------------------------------------------------------------------*/
/*-------- METHODS FOR LOADING, PRINTING & SAVING THE AbstractBlock --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the AbstractBlock
 * @{ */

 /// serialize the AbstractBlock (recursively) to a netCDF NcGroup
  /** The AbstractBlock serializes itself out of a netCDF::NcGroup. The
  * method of the base AbstractBlock class only handles
  *
  *      OR, BETTER, WILL HANDLE WHEN THIS METHOD WILL BE IMPLEMENTED
  *
  * the "arbitrary" part, i.e., that after the "reserved" one as dictated
  * by the get_first_*_*() methods; all the rest must be handled by the
  * derived classes (if any).
  *
  * For the format of the produced netCDF::NcGroup, see
  * AbstractBlock::deserialize(). */

 virtual void serialize( netCDF::NcGroup & group ) const override;

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

 virtual void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load the AbstractBlock out of an istream
 /** Method to deserialize the AbstractBlock out of an istream.
  *
  *     IT IS CURRENTLY NOT IMPLEMENTED
  *
  * but it still have to be defined (throwing exception) to make the class
  * concrete. */

 virtual void load( std::istream &input ) override {
  throw( std::logic_error( "AbstractBlock::load not implemented yet" ) );
  }

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
