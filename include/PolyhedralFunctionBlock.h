/*--------------------------------------------------------------------------*/
/*------------------- File PolyhedralFunctionBlock.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class PolyhedralFunctionBlock, which derives from
 * AbstractBlock to define the class of Block who have the specific structure
 * of having a PolyhedralFunction as objective, but otherwise can contain any
 * kind of Variable and Constraint (provided these are handled by the base
 * AbstractBlock class).
 *
 * \version 0.10
 *
 * \date 07 - 10 - 2019
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
 * PolyhedralFunction can also be represented in a very natural way by means
 * of some extra continuous ColVariable plus a finite set of FRowConstraint
 * with LinearFunction inside (a.k.a., linear constraints). Thus, the main
 * feature that PolyhedralFunctionBlock implements is the ability to
 * "present" itself (construct an "abstract representation") as either
 * having a FRealObjective with a PolyhedralFunction inside, or having a set
 * of (continuous) ColVariable and (linear) RowConstraint. We call the first
 * the "natural representation" of a PolyhedralFunctionBlock, and the latter
 * its "linearized representation".
 *
 * Indeed, having a PolyhedralFunction as objective is equivalent to the
 * linear program
 *
 *     min v : v >= a_i x + b_i     i = 1, ..., m
 *
 * in the convex case, and
 *
 *     max v : v <= a_i x + b_i     i = 1, ..., m
 *
 * in the concave one, where
 *
 *     x IS FIXED AND v IS THE ONLY Variable
 *
 * This underlines the fact that
 *
 *     PolyhedralFunctionBlock IS NOT NATURALLY USED AS A STAND-ALONE
 *     Block, BECAUSE THE INPUT ("ACTIVE") ColVariable OF ITS
 *     PolyhedralFunction ARE NOT Variable OF THE PolyhedralFunctionBlock
 *
 * Thus, the standard use of a PolyhedralFunctionBlock is as a sub-Block of
 * some other Block. This is not strictly necessary, because actually the
 * x ColVariable can be in the "arbitrary part" of the AbstractBlock (from
 * which PolyhedralFunctionBlock derives). However, the point is that
 *
 *     THE x ColVariable ARE NOT MANAGED By THE PolyhedralFunctionBlock,
 *     THIS BEING DEMANDED TO SOMETHING ELSE (see set_PolyhedralFunction())
 *
 * Thus, the "natural representation" and its "linearized representation"
 * differ regarding to which sets of Constraint, Variable and Objective are
 * "reserved" (see comments to AbstractBlock):
 *
 * - with the "natural representation", the Objective is reserved (since it
 *   must be a FRealObjective with a PolyhedralFunction inside), but nothing
 *   else is;
 *
 * - with the "linearized representation", the first group of static Variable
 *   contains a single ColVariable (v), the first group of dynamic Constraint
 *   contains the FRowConstraint (with LinearFunction inside, i.e.,
 *   v <op> a_i x + b_i for i = 1, ..., m where <op> depends on if the
 *   PolyhedralFunction is convex or concave), and the  Objective is also
 *   reserved (since it must be a FRealObjective with another LinearFunction
 *   inside, having nonzero coefficient only for v).
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
  : AbstractBlock( father ) , f_rep( false ) , f_polyf( nullptr ) { }

/*--------------------------------------------------------------------------*/
 /// de-serialize the current PolyhedralFunctionBlock out of netCDF::NcGroup
 /** The PolyhedralFunctionBlock de-serializes itself out of a
  * netCDF::NcGroup. Besides the mandatory "type" attribute of any :Block,
  * the group should contain the following:
  *
  * - all the data necessary to describe a PolyhedralFunction; see
  *   PolyhedralFunction::serialize() for details;
  *
  * - any other data necessary to represent the "arbitrary" part of the
  *   AbstractBlock, see AbstractBlock::deserialize() for details. */

 virtual void deserialize( netCDF::NcGroup & group ) override;

/*--------------------------------------------------------------------------*/
 /// the destructor actually destroys the abstract representation
 /** The destructor of PolyhedralFunctionBlock (unlike that of Block, but
  * like that of AbstractBlock) takes care of (clear()-ing first, and)
  * destroying (then) all the "abstract" Constraint/Variable, the Objective
  * and the inner Block. This is actually done by the destructor of
  * AbstractBlock for the "arbitrary" part , while that of
  * PolyhedralFunctionBlock takes care of the PolyhedralFunction and of
  * all Variable and Constraint of the "linearized representation".
  *
  * Note that PolyhedralFunctionBlock does not assume to be a "leaf" class:
  * further derived classes can be implemented for structures like "a
  * PolyhedralFunction, some other specific stuff and then an "arbitrary
  * part". In this case, the deletion of the "other specific stuff" is due to
  * the destructur of the further derived class, while that of the "arbitrary
  * part" is due to that of AbstractBlock. */

 virtual ~PolyhedralFunctionBlock() {
  guts_of_destructor();
  delete f_polyf;
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /** Completely resets the PolyhedralFunctionBlock with entirely new
  * PolyhedralFunction.
  *
  * If delf == true, the previous PolyhedralFunctionBlock (if any) is
  * deleted; hence, one expects the parameter to be true, unless someone else
  * is keeping tabs on the PolyhedralFunctionBlock and for some reason prefers
  * to destroy it later on. Thus, set_PolyhedralFunction( nullptr ) basically
  * completely resets the PolyhedralFunctionBlock (except the "arbitrary
  * part", of course) and leaves it "naked" ready to be filled again with a
  * different PolyhedralFunction.
  *
  * If there is any Solver attached to this PolyhedralFunctionBlock then a
  * NBModification (the "nuclear option") is issued. In this case, no
  * issueMod parameter is provided because this kind of change cannot
  * reasonably be ignored. */
 
 void set_PolyhedralFunction( PolyhedralFunction * polyf , bool delf = true )
 {
  if( f_polyf )
   guts_of_destructor();
  if( delf )
   delete f_polyf;
  f_polyf = polyf;
  if( anyone_there() )
   add_Modification( std::make_shared<NBModification>( this ) );
  }

/*--------------------------------------------------------------------------*/
 /// generate the Variable in the "linearized representation"
 /** This method serves is to ensure that the "abstract representation" of
  * the Variable in the PolyhedralFunctionBlock is initialized, so that it
  * can be read with get_static_variables() and get_dynamic_variables(). Of
  * course, the effect changes depending on whether the "natural
  * representation" or the "linearized representation" are used. In fact,
  *
  *    THE CHOICE BETWEEN THE TWO IS DONE PRECISELY IN THIS METHOD
  *
  * by means of the stvv parameter. The boolean field f_rep is set here to
  * true if the "linearized representation" is used, false otherwise, in
  * the following way:
  *
  * - if stvv is not nullptr and it is a SimpleConfiguration<int>, then it
  *   if bool( stvv->f_value );
  *
  * - otherwise, if f_BlockConfig is not nullptr,
  *   f_BlockConfig->f_static_variables_Configuration is not nullptr and it
  *   is a SimpleConfiguration<int>, then it is
  *   bool( f_BlockConfig->f_static_variables_Configuration->f_value );
  *
  * - otherwise, false ("natural representation") is assumed.
  *
  * The value is set upon call to this method, and never changed afterwards;
  * this means that the parameters of generate_abstract_constraints() and
  * generate_objective() are plainly ignored, and that this mathod has to
  * be called before these (which is only reasonable).
  *
  * If f_rep == false, the PolyhedralFunctionBlock has no extra Variable, be
  * them static or dynamic. If f_rep == true, the first group of static
  * Variable contains a single ColVariable (v), and there are no extra
  * dyanmic Variable. */

 void generate_abstract_variables( Configuration *stvv = nullptr ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// generate the Constraint in the "linearized representation"
 /** This method serves is to ensure that the "abstract representation" of
  * the Constraint, be they static or dynamic, of the PolyhedralFunctionBlock
  * is initialized, so that it can be read with get_static_constraints() and
  * get_dynamic_constraints(). Of course, the effect changes depending on
  * whether the "natural representation" or the "linearized representation"
  * are used. In fact,
  *
  *    THE CHOICE BETWEEN THE TWO IS DONE ELSEWHERE, PRECISELY IN
  *    generate_abstract_variables()
  *
  * and this method just assumes it has been done there and reads from the
  * f_rep field. This means that the stcc *Configuration is ignored, and so
  * is f_static_constraints_Configuration in the BlockConfig, if any.
  *
  * If f_rep == false, the PolyhedralFunctionBlock has no extra Constraint, be
  * them static or dynamic. If f_rep == true, the first group of dynamic
  * Constraint contains a single std::list< FRowConstraint > with
  * LinearFunction inside (a.k.a. "linear constraint") representing the m
  * inequalities v >= [<=] a_i x + b_i. Note that the "verse" of the
  * Constraint depend on PolyhedralFunction->is_convex(); if it is true than
  * the inequalities are ">=" (the LHS is -INF and the RHS is b_i), otherwise
  * they are "<=" (the LHS b_i and the RHS is INF). */

 void generate_abstract_constraints( Configuration *stcc = nullptr ) override;

/*--------------------------------------------------------------------------*/
 /// generate the Objective in the abstract representation, linearized or not
 /** This method serves is to ensure that the "abstract representation" of
  * the Objective of the PolyhedralFunctionBlock is initialized, so that it
  * can be read witt get_objective().Of course, the effect changes depending on
  * whether the "natural representation" or the "linearized representation"
  * are used. In fact,
  *
  *    THE CHOICE BETWEEN THE TWO IS DONE ELSEWHERE, PRECISELY IN
  *    generate_abstract_variables()
  *
  * and this method just assumes it has been done there and reads from the
  * f_rep field. This means that the *objc Configuration is ignored, and so is
  * is f_objective_Configuration in the BlockConfig, if any.
  *
  * If f_rep == false, the Objective of the PolyhedralFunctionBlock is a
  * FRealObjective having the PolyhedralFunction as Function. If f_rep ==
  * true the Objective of the PolyhedralFunctionBlock is still a
  * FRealObjective, but its Function is a LinearFunction having a single
  * nonzero coefficient (that of v, which is 1). Note that the "verse" of
  * the Objective depends on PolyhedralFunction->is_convex(); if it is true
  * then it is minimization, otherwise it is maximization. */

 void generate_objective( Configuration *objc = nullptr ) override;

/**@} ----------------------------------------------------------------------*/
/*------- Methods for reading the data of the PolyhedralFunctionBlock ------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the PolyhedralFunctionBlock
 *  @{ */

 PolyhedralFunction * get_PolyhedralFunction( void ) { return( f_polyf ); }

/*--------------------------------------------------------------------------*/

 // Index get_first_static_Constraint( void ) const override { return( 0 ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the index of the first group of "available" dynamic Constraint
 /** This method returns the index of the first group of dynamic Constraint in
  * the "arbitrary" part of the PolyhedralFunctionBlock. This depends on
  * whether the "natural representation" or the "linearized representation"
  * is used. Note that, instead, get_first_static_Constraint() need not be
  * re-defined, as the PolyhedralFunctionBlock never has any group of
  * static Constraint (in its "specific part", the "arbitrary part" clearly
  * being completely free to have any number of these). */

 Index get_first_dynamic_Constraint( void ) const override {
  return( f_rep ? 1 : 0 );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the index of the first group of "available" static Variable
 /** This method returns the index of the first group of static Variable in
  * the "arbitrary" part of the PolyhedralFunctionBlock. This depends on
  * whether the "natural representation" or the "linearized representation"
  * is used. Note that, instead, get_first_dynamic_Variable() need not be
  * re-defined, as the PolyhedralFunctionBlock never has any group of
  * dynamic Variable (in its "specific part", the "arbitrary part" clearly
  * being completely free to have any number of these). */

 Index get_first_static_Variable( void ) const override {
  return( f_rep ? 1 : 0 );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 // Index get_first_dynamic_Variable( void ) const override { return( 0 ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// tells if the Objective is reserved, which it *is*

 bool is_Objective_reserved( void ) const override { return( true ); }

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
/*--- METHODS FOR LOADING, PRINTING & SAVING THE PolyhedralFunctionBlock ---*/
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

 bool f_rep;                    ///< which of the two representations is used
 
 PolyhedralFunction * f_polyf;  ///< the PolyhedralFunction

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*--------------------------- PRIVATE METHODS ------------------------------*/
/*--------------------------------------------------------------------------*/
 // clears all the abstract representaton, but not f_polyf

 void guts_of_destructor( void );
 
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
