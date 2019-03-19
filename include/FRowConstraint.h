/*--------------------------------------------------------------------------*/
/*------------------------ File FRowConstraint.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the FRowConstraint class, derived from RowConstraint,
 * which is a class that defines a row constraint in terms of an
 * externally-provided Function object.
 *
 * \version 0.30
 *
 * \date 14 - 03 - 2019
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

#ifndef __FRowConstraint
 #define __FRowConstraint
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"
#include "Function.h"
#include "RowConstraint.h"
#include "Variable.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup FRowConstraint_CLASSES Classes in FRowConstraint.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS FRowConstraint ---------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a Constraint that is a "single row" and defined by a Function
/** The class FRowConstraint, derived from RowConstraint, implements the
 * concept of "Constraint that are have a row form", that is,
 *
 *   LHS <= f(x) <= RHS
 *
 * where LHS <= RHS are two extended reals, at least one of which is
 * finite, and f is a real-valued function. In FRowConstraint, f is simply
 * a Function object, whose pointer is provided from the outside.
 *
 * The FRowConstraint is set as the Observer of the Function (which is why
 * it also derives from Observer, besides from ThinVarDepInterface since it
 * derives from Constraint), so that any Modification issued by the Function
 * is received by the FRowConstraint. The FRowConstraint may either repackage
 * that Modification and send a new Modification to the Block, or directly
 * send the received Modification. This means that the FRowConstraint may
 * issue FunctionMod*.
 *
 * The FRowConstraint registers itself as "active" in the Variable of the
 * Function. This has to be mantained if the Variable of the Function change
 * dynamically. In order to achieve this, the FRowConstraint checks the
 * Modification issued by the Function for FunctionModVars ones. As a
 * consequence, the FRowConstraint "is always listening" to the Function
 * even if its Block has no registered Solver. This may lead to Modification
 * of the Function to be issued even if there is in fact no-one "listening".
 * Hopefully this potential inefficiency will be fixed later on by some
 * mechanism allowing a finer control on which Modification are "listened to".
 */

class FRowConstraint : public RowConstraint , Observer {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */


/*@}------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
    @{ */

 /// constructor of FRowConstraint
 /** Constructor of FRowConstraint. It receives a pointer to the Block to
  * which the FRowConstraint belongs, the values for LHS and RHS, and a
  * pointer to the Function that defines the constraint. Everything has a
  * default (nullptr, 0 and 0, nullptr, respectively) so that this can be
  * used as the void constructor. */

 FRowConstraint( Block *my_block = nullptr ,
		 c_RHSValue lhs_value = 0 , c_RHSValue rhs_value = 0 ,
		 Function * const function = nullptr )
  : RowConstraint( my_block ) , f_lhs( lhs_value ) , f_rhs( rhs_value ) ,
    f_function( nullptr )
 {
  set_function( function , eNoMod );
  }

/*--------------------------------------------------------------------------*/
 /// destructor: deletes the Function and un-registers with the Variable
 /** By calling set_function( nullptr ), the destructor un-registers with the
  * Variable of the Function (if clear() has not been called first) and then
  * deletes it. */

 virtual ~FRowConstraint() {
  set_function( nullptr , eNoMod );
  }

/*--------------------------------------------------------------------------*/
 /// "rough destructor": calls the version of the Function object
 /** The clear() method just calls clear() in the inner Function (if any).
  * This results in the list of Variable of the Function to be emptied,
  * so that in the destructor they re not un-registered. */

 virtual void clear( void ) override {
  if( f_function )
   f_function->clear();
  }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the pointer to the Function in this FRowConstraint
 /**< Method to set the pointer to the Function that defines this
  * FRowConstraint. Note that the pointed Function object becomes property of
  * the FRowConstraint, which therefore deletes it in the destructor. One may
  * wonder why an rvalue reference is not used, but this is because the "name"
  * of a Function is its memory address, so moving a Function creates a
  * different Function.
  *
  * For some reason, the caller may want instead to manage the Function object
  * herself. This is why the deleteold is provided; if true then the previous
  * Function object (if any) is deleted, otherwise it is not. In the latter
  * case, it is assumed that the called has another pointer to the Function
  * and will dispose of it in due time. Thus, a call to set_function()
  * removes any Function from the FRowConstraint, leaving it "empty".
  *
  * Note that
  *
  *    FRowConstraint REGISTERS ITSELF IN THE Variable OF THE Function
  *
  * This process, which is completely transparent to the Function itself,
  * primarily happen between this method. However, in addition
  *
  *     EACH TIME A Variable IS ADDED/REMOVED FROM THE Function, THE
  *     FRowConstraint WILL HAVE TO REGISTER/UNREGISTER ITSELF FROM
  *     THAT Variable
  *
  * This is possible, because
  *
  *     THE FRowConstraint IS SET AS THE Observer OF THE Function
  *
  * (which also happens inside this method). Hence, the addition/deletion of
  * the Variable issues an appropriate :FunctionModVars, which therefore can
  * be "seen" by the FRowConstraint (within add_Modification()), allowing it
  * to react accordingly.
  *
  * However, for the latter to happen, the :FunctionModVars must be issued
  * by the Function even if there is no Solver "listening" to the Block of
  * this FRowConstraint. To force this to happen, the FRowConstraint "is
  * always listening". This may lead to Modification of the Function to be
  * issued even if there is in fact no-one "listening" to them, Hopefully
  * this potential inefficiency will be fixed later on by some mechanism
  * allowing a finer control on which Modification are "listened to".
  *
  * The parameter issueMod decides if and how the Modification is issued, as
  * described in Observer::make_par(). */

 virtual void set_function( Function * const function = nullptr ,
			    c_ModParam issueMod = eModBlck ,
			    bool deleteold = true );

/*--------------------------------------------------------------------------*/

 virtual void set_rhs( c_RHSValue rhs_value ,
		       c_ModParam issueMod = eModBlck ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual void set_lhs( c_RHSValue lhs_value ,
		       c_ModParam issueMod = eModBlck ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual void set_both( c_RHSValue both_value ,
			c_ModParam issueMod = eModBlck ) override;

/*--------------------------------------------------------------------------*/

 /// dispatches the method of the underlying Function
 virtual void set_par( const idx_type par , const int value ) override {
  if( f_function )
   f_function->set_par( par , value );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 /// dispatches the method of the underlying Function
 virtual void set_par( const idx_type par , const double value ) override {
  if( f_function )
   f_function->set_par( par , value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 /// dispatches the method of the underlying Function
 virtual void set_par( const idx_type par , const std::string & value )
  override {
  if( f_function )
   f_function->set_par( par , value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 /// dispatches the method of the underlying Function
 virtual void set_ComputeConfig( ComputeConfig *scfg = nullptr ) override {
  if( f_function )
   f_function->set_ComputeConfig( scfg );
  }

/*@} -----------------------------------------------------------------------*/
/*----------- METHODS FOR READING THE DATA OF THE FRowConstraint -----------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the FRowConstraint
    @{ */

 ///< method to get a pointer to the Function of the FRowConstraint
 const Function* get_function( void ) const { return( f_function ); }

/*--------------------------------------------------------------------------*/
 /// method to get the RHS of the RowConstraint
 virtual RHSValue get_rhs( void ) const override { return( f_rhs ); }

 /// method to get the LHS of the RowConstraint
 virtual RHSValue get_lhs( void ) const override { return( f_lhs ); }

/*@} -----------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF A FRowConstraint ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a FRowConstraint
    @{ */

 /// compute the value of variable part of the FRowConstraint
 /** It evaluates the Function that defines this FRowConstraint, and
  * the value of the Function is is then stored into the protected
  * field f_value and returned by value(). */

 virtual int compute( bool changedvars = true ) override {
  return( f_function ? f_function->compute( changedvars ) : kUnEval );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get the value of variable part of the FRowConstraint
 /** Method to get the value of variable part of the FRowConstraint, which is
  * just the value of the value of the underlying Function. */

 virtual RHSValue value( void ) const override {
  return( f_function ? f_function->get_value() : 0 );
  }

/*--------------------------------------------------------------------------*/
 /// returns true if the FRowConstraint is feasible
 /** As opposed to the implementation of the base RowConstraint class, which
  * only considers a single value, takes into account both the upper and
  * the lower estimate prodiced by the Function; the FRowConstraint is only
  * deemed feasible if upper_estimate <= f_rhs and lower_estimete >= f_lhs. */

 virtual bool feasible( void ) const override {
  bool feas = true;
  if( f_lhs > - Inf<double>() )
   feas &= f_function->get_lower_estimate() >= f_lhs;

  if( f_rhs < Inf<double>() )
   feas &= f_function->get_upper_estimate() <= f_rhs;

  return( feas );
  }

/*--------------------------------------------------------------------------*/
 /// returns the absolute violation of the FRowConstraint
 /** As opposed to the implementation of the base RowConstraint class, which
  * only considers a single value, takes into account both the upper and
  * the lower estimate prodiced by the Function to compute the absolute
  * violation of the FRowConstraint. */

 virtual RHSValue abs_viol( void ) const override {
  RHSValue viol = -Inf<double>();

  if( f_lhs > - Inf<double>() ) {
   RHSValue lval = f_function->get_lower_estimate();
   if( lval < Inf<double>() )
    viol = ( lval <= -Inf<double>() ? Inf<double>() : f_lhs - lval );
   }

  if( f_rhs < Inf<double>() ) {
   RHSValue uval = f_function->get_upper_estimate();
   if( uval > -Inf<double>() )
    viol = std::max( viol , ( uval >= Inf<double>() ? Inf<double>() :
			                              uval - f_rhs ) );
   }

  return( viol );
  }

/*--------------------------------------------------------------------------*/
 /// returns the relative violation of the FRowConstraint
 /** As opposed to the implementation of the base RowConstraint class, which
  * only considers a single value, takes into account both the upper and
  * the lower estimate prodiced by the Function to compute the relative
  * violation of the FRowConstraint. */

 virtual RHSValue rel_viol( void ) const override {
  RHSValue uval = f_function->get_upper_estimate();

  // if the upper estimate is +INF, then if the RHS is < +INF then the
  // constraint is infinitely violated, otherwise is infinitely slackened
  if( uval >= Inf<double>() )
   return( f_rhs < Inf<double>() ? Inf<double>() : - Inf<double>() );

  RHSValue lval = f_function->get_lower_estimate();
  // if the lower estimate is -INF, then if the LHS is > -INF then the
  // constraint is infinitely violated, otherwise is infinitely slackened
  if( lval <= -Inf<double>() )
   return( f_lhs > -Inf<double>() ? Inf<double>() : - Inf<double>() );

  // both upper and lower estimate are finite
  if( f_lhs <= - Inf<double>() )
   if( f_rhs >= Inf<double>() )
    return( -Inf<double>() );
   else
    return( f_rhs == 0 ? uval - f_rhs :
	                 ( uval - f_rhs ) / std::abs( f_rhs ) );
   else
    if( f_rhs >= Inf<double>() )
     return( f_lhs == 0 ? f_lhs - lval :
	                  ( f_lhs - lval ) / std::abs( f_lhs ) );
   
  // both LHS and RHS are finite
  if( f_lhs == 0 )
   if( f_rhs == 0 )
    return( std::max( f_lhs - lval , uval - f_rhs ) );
   else
    return( std::max( f_lhs - lval , uval - f_rhs ) / std::abs( f_rhs ) );
  else
   if( f_rhs == 0 )
    return( std::max( f_lhs - lval , uval - f_rhs ) / std::abs( f_lhs ) );
   else
    return( std::max( ( f_lhs - lval ) / std::abs( f_lhs ) ,
		      ( uval - f_rhs ) / std::abs( f_rhs ) ) );
  }

/*@} -----------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the FRowConstraint; they all dispatch
 * the method of the underlying Function, so it is an error to call them if
 * the Function has not been set yet.
 * @{ */

 virtual idx_type get_num_int_par( void ) const override {
  return( f_function->get_num_int_par() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual idx_type get_num_dbl_par( void ) const override {
  return( f_function->get_num_dbl_par() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual idx_type get_num_str_par( void ) const override {
  return( f_function->get_num_str_par() );
  }

/*--------------------------------------------------------------------------*/
 
 virtual int get_dflt_int_par( const idx_type par ) const override {
  return( f_function->get_dflt_int_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 
 virtual double get_dflt_dbl_par( const idx_type par ) const override {
  return( f_function->get_dflt_dbl_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 
 virtual const std::string & get_dflt_str_par( const idx_type par )
  const override {
  return( f_function->get_dflt_str_par( par ) );
  }

/*--------------------------------------------------------------------------*/

 virtual int get_int_par( const idx_type par ) const override {
  return( f_function->get_int_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual double get_dbl_par( const idx_type par ) const override {
  return( f_function->get_dbl_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual const std::string & get_str_par( const idx_type par ) const override
 {
  return( f_function->get_str_par( par ) );
  }

/*--------------------------------------------------------------------------*/

 virtual idx_type int_par_str2idx( const std::string & name ) const override
 {
  return( f_function->int_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual idx_type dbl_par_str2idx( const std::string & name ) const override
 {
  return( f_function->dbl_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual idx_type str_par_str2idx( const std::string & name ) const override
 {
  return( f_function->str_par_str2idx( name ) );
  }

/*--------------------------------------------------------------------------*/

 virtual const std::string & int_par_idx2str( const idx_type idx ) const
  override {
  return( f_function->int_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual const std::string & dbl_par_idx2str( const idx_type idx ) const
  override {
  return( f_function->dbl_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual const std::string & str_par_idx2str( const idx_type idx ) const
  override {
  return( f_function->str_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual ComputeConfig * get_ComputeConfig( bool all = false,
			     ComputeConfig * ocfg = nullptr ) const override
 {
  return( f_function->get_ComputeConfig( all , ocfg ) );
  }

/*@} -----------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE FRowConstraint -------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" Variable in the
 * FRowConstraint; they all dispatch the method of the underlying Function
 *  @{ */

 Index get_num_active_var( void ) const override
 {
  return( f_function ? f_function->get_num_active_var() : 0 );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual Index is_active( const Variable * const f_variable ) const override
 {
  return( f_function ? f_function->is_active( f_variable ) : Inf<Index>() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual Variable * get_active_var( const Index i ) const override
 {
  return( f_function ? f_function->get_active_var( i ) : nullptr );
  }

/*--------------------------------------------------------------------------*/

 virtual v_iterator * v_begin( void ) override
 {
  return( f_function ? f_function->v_begin() : nullptr );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual v_const_iterator * v_begin( void ) const override
 {
  return( f_function ?
	  static_cast<const Function *>( f_function )->v_begin() : nullptr );
  }

/*--------------------------------------------------------------------------*/

 virtual v_iterator * v_end( void ) override
 {
  return( f_function ? f_function->v_end() : nullptr );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual v_const_iterator * v_end( void ) const override
 {
  return( f_function ?
	  static_cast<const Function *>( f_function )->v_end() : nullptr );
  }

/*--------------------------------------------------------------------------*/

 virtual void remove_variable( Variable * variable ,
			       c_ModParam issueMod = eModBlck ) override
 {
  /* FRowConstraint typically relies on FunctionModVars to know if something
   * has happened to the Variable of the Function and register/unregister
   * itself from them. However, in this case it knows beforehand what is
   * happening. If there is no real reason to have the Modification issued,
   * it will instruct the Function not to and do the unregistering herein.
   */
  
  if( ! f_function )
   return;

  if( ( par2mod( issueMod ) > eNoMod ) && f_Block->anyone_there() )
   f_function->remove_variable( variable , issueMod );
  else {
   // unregistration can preceed removal, since the Function completely
   // ignores this information
   variable->remove_active( this );
   f_function->remove_variable( variable , eNoMod );
   }
  }

/*--------------------------------------------------------------------------*/

 virtual void remove_variables( std::vector<Variable *> && vars ,
                                const bool ordered = false ,
                                c_ModParam issueMod = eModBlck ) override
 {
  /* FRowConstraint typically relies on FunctionModVars to know if something
   * has happened to the Variable of the Function and register/unregister
   * itself from them. However, in this case it knows beforehand what is
   * happening. If there is no real reason to have the Modification issued,
   * it will instruct the Function not to and do the unregistering herein.
   */
  
  if( ( ! f_function ) || vars.empty() )
   return;

  if( ( par2mod( issueMod ) > eNoMod ) && f_Block->anyone_there() )
   f_function->remove_variables( std::move( vars ) , ordered , issueMod );
  else {
   // unregistration can preceed removal, since the Function completely
   // ignores this information
   for( auto var : vars )
    var->remove_active( this );

   f_function->remove_variables( std::move( vars ) , ordered , eNoMod );
   }
  }

/*@} -----------------------------------------------------------------------*/
/*------------- METHODS DESCRIBING THE BEHAVIOR OF AN Observer -------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of an Observer
 *  @{ */

 /// the FRowConstraint "is always listening"
 /** In principle, the FRowConstraint should "be listening" only if the Block
  * (if any) is. However, FRowConstraint relies on FunctionModVars to know if
  * something has happened to the Variable of the Function and
  * register/unregister itself from them. For this to happen, the
  * FunctionModVars must be issued by the Function even if there is no Solver
  * "listening" to the Block of this FRowConstraint. To force this to happen,
  * the FRowConstraint "is always listening". This may lead to Modification
  * of the Function to be issued even if there is in fact no-one "listening"
  * to them. Hopefully this potential inefficiency will be fixed later on by
  * some mechanism allowing a finer control on which Modification are
  * "listened to". */

 virtual bool anyone_there( void ) const override {
  // return( f_Block ? f_Block->anyone_there() : false );
  return( true );
  }

/*--------------------------------------------------------------------------*/
 /// mostly just dispatch to add_Modification() of the Block (if any)
 /** add_Modification() mostly just dispatch to add_Modification() of the
  * Block (if any). However, it also checks if mod is a FunctionModVars
  * (which is why the FRowConstraint "is always listening", see
  * anyone_there(), and in case register/unregister itself with the
  * added/removed Variable. */

 virtual void add_Modification( sp_Mod mod , c_ChnlName chnl = 0 ) override;

/*--------------------------------------------------------------------------*/
 /// just dispatch to open_channel() of the Block (if any)

 virtual ChnlName open_channel( GroupModification * gmpmod = nullptr ,
				c_ModParam issueMod = eModBlck ) override
 {
  return( f_Block ? f_Block->open_channel( gmpmod , issueMod ) : 0 );
  }

/*--------------------------------------------------------------------------*/
 /// just dispatch to nest_channel() of the Block (if any)

 virtual void nest_channel( c_ChnlName chnl ,
			    GroupModification * gmpmod = nullptr ,
			    c_ModParam issueMod = eModBlck )  override
 {
  if( f_Block )
   f_Block->nest_channel( chnl , gmpmod , issueMod );
  }

/*--------------------------------------------------------------------------*/
 /// just dispatch to un_nest_channel() of the Block (if any)

 virtual void un_nest_channel( c_ChnlName chnl )  override {
  if( f_Block )
   f_Block->un_nest_channel( chnl );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 /// just dispatch to close_channel() of the Block (if any)

 virtual void close_channel( c_ChnlName chnl ) override {
  if( f_Block )
   f_Block->close_channel( chnl );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 /// just dispatch to set_default_channel() of the Block (if any)

 virtual void set_default_channel( c_ChnlName chnl = 0 ) override {
  if( f_Block )
   f_Block->set_default_channel( chnl );
  }

/*@}------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
 *  @{ */

 /// print information about the FRowConstraint on an ostream
 virtual void print( std::ostream &output ) const override {
  output << "FRowConstraint [" << this << "] of Block [" << f_Block
	 << "] with Function [" << f_function << "] with "
	 << ( f_function ? f_function->get_num_active_var() : 0 )
	 << " active variables, ";
  if( feasible() )
   output << "feasible";
  else
   output << "unfeasible";

  output << " (value = " << value() << ")" << std::endl;
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 RHSValue f_lhs;    ///< the LHS of the RowConstraint
 RHSValue f_rhs;    ///< the RHS of the RowConstraint

 Function * f_function;
 ///< pointer to the Function that defines this Constraint

/*--------------------------------------------------------------------------*/

 };  // end( class( FRowConstraint ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS FRowConstraintMod ---------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modifications specific to a FRowConstraint
/** Derived class from RowConstraintMod to describe Modification specific to
 * a FRowConstraint, i.e., changing the Function.
 *
 * Defining a class is a bit weird because the only thing the class does is
 * to define am enum for the new value of the type of Modification in the
 * RowConstraintMod. However, throwing a Modification of a different class
 * (but derived from RowConstraintMod) may make it easier for the solver to
 * handle it (at the very least, it directly knows it comes from a
 * FRowConstraint without having to check it). */

class FRowConstraintMod : public RowConstraintMod
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/
 /// public enum for the types of FRowConstraintMod thrown
 /** Public enum "extending" RowC_mod_type with the new types of Modification
  * thrown by FRowConstraint. */

 enum FRC_cons_mod_type {
  eFunctionChanged = eRowConstModLastParam ,
  ///< the Function underlying this FRowConstraint changed whole

  eFRCConstModLastParam
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of types of Modification. */
  };

/*---------------------- CONSTRUCTOR & DESTRUCTOR --------------------------*/

 /// constructor: just calls that of RowConstraintMod

 FRowConstraintMod( FRowConstraint *cnst , int mod = eFunctionChanged ,
		    const bool cB = true )
  : RowConstraintMod( cnst , mod , cB ) { }

 virtual ~FRowConstraintMod() { }  ///< destructor: does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the FRowConstraintMod

 virtual inline void print( std::ostream &output ) const {
  output << "FRowConstraintMod[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";  
  output << "] on FRowConstraint[" << f_constraint
	 << "]: changing the Function" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( FRowConstraintMod ) )

/*--------------------------------------------------------------------------*/

/*@}  end( group( FRowConstraint_CLASSES ) ) -------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* FRowConstraint.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File FRowConstraint.h -------------------------*/
/*--------------------------------------------------------------------------*/
