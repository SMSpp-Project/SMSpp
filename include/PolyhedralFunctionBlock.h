/*--------------------------------------------------------------------------*/
/*------------------- File PolyhedralFunctionBlock.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class PolyhedralFunctionBlock, which derives from
 * AbstractBlock to define the class of Block who have the specific structure
 * of having a PolyhedralFunction as objective, but otherwise can contain any
 * kind of Variable and Constraint (provided these are handled by the base
 * AbstractBlock class.
 *
 * \version 0.10
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

#ifndef __PolyhedralFunctionBlock
 #define __PolyhedralFunctionBlock
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractBlock.h"

#include "PolyhedralFunction.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS PolyhedralFunctionBlock -----------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// an AbstractBlock whose distinguishing feature is a PolyhedralFunction
/** The class PolyhedralFunctionBlock derives from AbstractBlock to define
 * the following concept: a Block whose sole distinguishing feature is a
 * PolyhedralFunction as objective, but which can otherwise contain any kind
 * of "abstract" Variable and Constraint (provided these are handled by the
 * base AbstractBlock class.
 *
 * The rationale for the PolyhedralFunctionBlock class is that a
 * PolyhedralFunction is a perfectly fine object in itself, but one that
 * several solver cannot easily deal with in its "natural" form. However, a
 * PolyhedralFunction can also be represented in a very natural way as an
 * extra continuous ColVariable plus a finite set of FRowConstraint with
 * LinearFunction inside (a.k.a., linear constraints). Thus, the main
 * feature that PolyhedralFunctionBlock implements is the ability to
 * "present" itself (construct an "abstract representation") as either
 * having a FRealObjective with a PolyhedralFunction inside, or having a set
 * of linear constraints. We call the first the "natural representation" of
 * a PolyhedralFunctionBlock, and the latter its "linearized representation".
 * The two cases clearly differ regarding to which sets of Constraint,
 * Variable and Objective are "reserved" (see comments to AbstractBlock).
 * In particular:
 *
 * - with the "natural representation", the Objective is reserved (since it
 *   must be a FRealObjective with a PolyhedralFunction inside), but nothing
 *   else is;
 *
 * - with the "linearized representation", the first group of static Variable
 *   contains a single ColVariable, the first group of dynamic Constraint
 *   contains the FRowConstraint (with LinearFunction inside), and the
 *   Objective is also reserved (since it must be a FRealObjective with
 *   another LinearFunction inside).
 *
 * An important note is that
 *
 *     THE ACTIVE Variable OF THE PolyhedralFunction NEED NOT NECESSARILY
 *     BE Variable OF THE PolyhedralFunctionBlock.
 *
 * This may happen, in which case they must be put within the "other"
 * (either static or dynamic) groups of Variable in the
 * PolyhedralFunctionBlock, or not. PolyhedralFunctionBlock does not make
 * any assumption about this.
 *
 * One nontrivial issue in this setup is that, when the "linearized
 * representation" is used, it is necessary to:
 *
 * - "capture" the *FunctionMod* issued by the PolyhedralFunction and use
 *   them for properly changing the "linearized representation";
 *
 * - "capture" the Modification issued by elements in the "linearized
 *   representation" and use them for properly changing the
 *   PolyhedralFunction.
 *
 * Other than that, PolyhedralFunctionBlock entirely relies on the machinery
 * proivided by AbstractBlock to handle all the rest of the Block, and
 * therefore is subject to the limitations of that class regarding what
 * kind of Constraint, Variable and Objective are supported. */

class PolyhedralFunctionBlock : public AbstractBlock {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------- CONSTRUCTING AND DESTRUCTING PolyhedralFunctionBlock -----------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing PolyhedralFunctionBlock
 *  @{ */

 /// constructor of PolyhedralFunctionBlock, taking a pointer to the father
 /** Constructor of PolyhedralFunctionBlock. It accepts a pointer to the
  * father Block (defaulting to nullptr, both because the root Block has no
  * father and so that this can also be used as the void constructor),
  * passes it to the Block constructor, and does little else. */

 PolyhedralFunctionBlock( Block * father = nullptr )
  : AbstractBlock( father ) { }

/*--------------------------------------------------------------------------*/
 /// de-serialize the current PolyhedralFunctionBlock out of netCDF::NcGroup
 /** The PolyhedralFunctionBlock de-serializes itself out of a
  * netCDF::NcGroup. Besides the mandatory "type" attribute of any :Block,
  * the group should contain the following:
  *
  * - all the data necessary to describe a PolyhedralFunction; see
  *   PolyhedralFunction::serialize() for details;
  *
  * - HOW ABOUT THE VARIABLE ????
  *
  * - any other data necessary to represent the "arbitrary" part of the
  *   AbstractBlock, see AbstractBlock::deserialize() for details. */

 virtual void deserialize( netCDF::NcGroup & group ) override;

/*--------------------------------------------------------------------------*/
 /// destructor of PolyhedralFunctionBlock, destroys the abstract representation
 /** The destructor of PolyhedralFunctionBlock (unlike that of Block) takes care of
  * (clear()-ing first, and) destroying (then) all the "abstract"
  * Constraint/Variable (and the Objective). Similarly, the pointers to the
  * inner Block in the v_Block vector are likely the only live references to
  * these Block, and therefore they are destroyed in the AbstractBlock
  * destructor. */

 virtual ~PolyhedralFunctionBlock( );

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 void set_PolyhedralFunction( PolyhedralFunction * polyf )
 {
  f_polyf = polyf;
  }

/**@} ----------------------------------------------------------------------*/
/*------------- Methods for reading the data of the PolyhedralFunctionBlock ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the PolyhedralFunctionBlock
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
 /// tells is the Objective is reserved
 /** This method returns true if the Objective is handled by the derived
  * class, i.e., it is not in the "arbitrary" part of the AbstractBlock. The
  * implementation in the base AbstractBlock class returns false, i.e., the
  * Objective is in the "arbitrary" part of the AbstractBlock. */

 virtual Index is_Objective_reserved( void ) const { return( false ); }

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
/*----------------- Methods for checking the PolyhedralFunctionBlock -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for checking solution information in the PolyhedralFunctionBlock
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
  * PolyhedralFunctionBlock, the "only reasonable" Solution is a ColVariableSolution.
  */

 virtual Solution * get_Solution( Configuration *solc = nullptr ,
				  bool emptys = true ) override;

/**@} ----------------------------------------------------------------------*/
/*-------- METHODS FOR LOADING, PRINTING & SAVING THE PolyhedralFunctionBlock --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the PolyhedralFunctionBlock
 * @{ */

 /// serialize the PolyhedralFunctionBlock (recursively) to a netCDF NcGroup
  /** The PolyhedralFunctionBlock serializes itself out of a netCDF::NcGroup. The
  * method of the base AbstractBlock class only handles
  *
  *      OR, BETTER, WILL HANDLE WHEN THIS METHOD WILL BE IMPLEMENTED
  *
  * the "arbitrary" part, i.e., that after the "reserved" one as dictated
  * by the get_first_*_*() methods; all the rest must be handled by the
  * derived classes (if any).
  *
  * For the format of the produced netCDF::NcGroup, see
  * PolyhedralFunctionBlock::deserialize(). */

 virtual void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/** @name Protected methods for inserting and extracting
 */

 /// print information about the PolyhedralFunctionBlock on an ostream 
 /** Protected method intended to print information about the PolyhedralFunctionBlock;
  * it basically goes over all the abstract representation (static and dynamic
  * Variable and Constraint, Objective, and inner Block) and asks everyone to
  * print itself. */

 virtual void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load the PolyhedralFunctionBlock out of an istream
 /** Method to deserialize the PolyhedralFunctionBlock out of an istream.
  *
  *     IT IS CURRENTLY NOT IMPLEMENTED
  *
  * but it still have to be defined (throwing exception) to make the class
  * concrete. */

 virtual void load( std::istream &input ) override {
  throw( std::logic_error(
		     "PolyhedralFunctionBlock::load not implemented yet" ) );
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 PolyhedralFunction * f_polyf;  ///< the PolyhedralFunctions

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

 };  // end( class( PolyhedralFunctionBlock ) )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* PolyhedralFunctionBlock.h included */

/*--------------------------------------------------------------------------*/
/*------------------- End File PolyhedralFunctionBlock.h -------------------*/
/*--------------------------------------------------------------------------*/
