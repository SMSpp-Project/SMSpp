/*--------------------------------------------------------------------------*/
/*------------------------- File FRealObjective.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the FRealObjective class, which is a RealObjective whose
 * value if computed by an externally-provided Function object.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __FRealObjective
 #define __FRealObjective
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Function.h"

#include "Objective.h"

#include "Observer.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup FRealObjective_CLASSES Classes in FRealObjective.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS FRealObjective ---------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a RealObjective whose function is given by a Function
/** The class FRealObjective, derived from RealObjective, implements the
 * concept of "Objective with a real return value" by having the return
 * value computed by a Function object, whose pointer is provided from the
 * outside.
 *
 * The FRealObjective is set as the Observer of the Function (which is why
 * it also derives from Observer, besides from ThinVarDepInterface since it
 * derives from Constraint), so that any Modification issued by the Function
 * is received by the FRealObjective. The FRealObjective may either repackage
 * that Modification and send a new Modification to the Block, or directly
 * send the received Modification. This means that the FRealObjective may
 * issue FunctionMod*.
 *
 * The FRealObjective registers itself as "active" in the Variable of the
 * Function. This has to be maintained if the Variable of the Function change
 * dynamically. In order to achieve this, the FRealObjective checks the
 * Modification issued by the Function for FunctionModVars ones. As a
 * consequence, the FRealObjective "is always listening" to the Function
 * even if its Block has no registered Solver. This may lead to Modification
 * of the Function to be issued even if there is in fact no-one "listening".
 * Hopefully this potential inefficiency will be fixed later on by some
 * mechanism allowing a finer control on which Modification are "listened to".
 */

class FRealObjective : public RealObjective , public Observer {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

 /// constructor of FRealObjective, taking a Block and a Function
 /** Constructor of FRealObjective. It accepts a pointer to the Block to
  * which the FRealObjective belongs and a pointer to the Function that
  * defines it. Both parameters default to nullptr so that this can be used
  * as the void constructor. */

 explicit FRealObjective( Block * my_block = nullptr ,
                          Function * function = nullptr )
  : RealObjective( my_block ) , f_function( nullptr ) {
  set_function( function, eNoMod );
  }

/*--------------------------------------------------------------------------*/
 /// destructor: deletes the Function and un-registers with the Variable
 /** By calling set_function( nullptr ), the destructor un-registers with the
  * Variable of the Function (if clear() has not been called first) and then
  * deletes it. */

 ~FRealObjective() override { set_function( nullptr , eNoMod ); }

/*--------------------------------------------------------------------------*/
 /// "rough destructor": calls the version of the Function object
 /** The clear() method just calls clear() in the inner Function (if any).
  * This results in the list of Variable of the Function to be emptied,
  * so that in the destructor they re not un-registered. */

 void clear( void ) override {
  if( f_function )
   f_function->clear();
  }

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the pointer to the Function in this FRealObjective
 /**< Method to set the pointer to the Function that defines this
  * FRealObjective. Note that the pointed Function object becomes property of
  * the FRealObjective, which therefore deletes it in the destructor. One may
  * wonder why an rvalue reference is not used, but this is because the "name"
  * of a Function is its memory address, so moving a Function creates a
  * different Function.
  *
  * For some reason, the caller may want instead to manage the Function object
  * herself. This is why the deleteold is provided; if true then the previous
  * Function object (if any) is deleted, otherwise it is not. In the latter
  * case, it is assumed that the called has another pointer to the Function
  * and will dispose of it in due time. Thus, a call to set_function()
  * removes any Function from the FRealObjective, leaving it "empty".
  *
  * Note that
  *
  *    FRealObjective REGISTERS ITSELF IN THE Variable OF THE Function
  *
  * This process, which is completely transparent to the Function itself,
  * primarily happen between this method. However, in addition
  *
  *     EACH TIME A Variable IS ADDED/REMOVED FROM THE Function, THE
  *     FRealObjective WILL HAVE TO REGISTER/UNREGISTER ITSELF FROM
  *     THAT Variable
  *
  * This is possible, because
  *
  *     THE FRealObjective IS SET AS THE Observer OF THE Function
  *
  * (which also happens inside this method). Hence, the addition/deletion of
  * the Variable issues an appropriate :FunctionModVars, which therefore can
  * be "seen" by the FRealObjective (within add_Modification()), allowing it
  * to react accordingly.
  *
  * However, for the latter to happen, the :FunctionModVars must be issued
  * by the Function even if there is no Solver "listening" to the Block of
  * this FRealObjective. To force this to happen, the FRealObjective "is
  * always listening". This may lead to Modification of the Function to be
  * issued even if there is in fact no-one "listening" to them, Hopefully
  * this potential inefficiency will be fixed later on by some mechanism
  * allowing a finer control on which Modification are "listened to".
  *
  * The parameter issueMod decides if and how the FRealObjectiveMod is
  * issued, as described in Observer::make_par(). */

 void set_function( Function * function = nullptr ,
                    ModParam issueMod = eModBlck , bool deleteold = true );

/** @} ---------------------------------------------------------------------*/
/*----------- METHODS FOR READING THE DATA OF THE FRealObjective -----------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the FRealObjective
 *  @{ */

 /// returns the Block to which this Observer belongs/
 /** FRealObjective is an Observer, and it belongs to the Block to which it
  * belongs as a Constraint. However, note that FRealObjective::get_Block()
  * is virtual while Constraint::get_Block() is not, hence the former has to
  * be explicitly implemented in terms of the latter. */

 [[nodiscard]] Block * get_Block( void ) const override {
  return( Objective::get_Block() );
  }

/*--------------------------------------------------------------------------*/

 /// returns the pointer to the Function in this FRealObjective
 [[nodiscard]] Function * get_function( void ) const { return( f_function ); }

/*--------------------------------------------------------------------------*/

 void set_par( const idx_type par , int value ) override {
  if( f_function )
   f_function->set_par( par , value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void set_par( const idx_type par , double value ) override {
  if( f_function )
   f_function->set_par( par , value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void set_par( const idx_type par , std::string && value ) override {
  if( f_function )
   f_function->set_par( par , std::move( value ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void set_par( idx_type par , const std::string & value ) override {
  if( f_function )
   f_function->set_par( par , value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void set_par( idx_type par , std::vector< int > && value ) override {
  if( f_function )
   f_function->set_par( par , std::move( value ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void set_par( idx_type par , const std::vector< int > & value ) override {
  if( f_function )
   f_function->set_par( par , value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void set_par( idx_type par , std::vector< double > && value ) override {
  if( f_function )
   f_function->set_par( par , std::move( value ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void set_par( idx_type par , const std::vector< double > & value ) override {
  if( f_function )
   f_function->set_par( par , value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void set_par( idx_type par , std::vector< std::string > && value ) override {
  if( f_function )
   f_function->set_par( par , std::move( value ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void set_par( idx_type par , const std::vector< std::string > & value )
  override {
  if( f_function )
   f_function->set_par( par , value );
  }

/*--------------------------------------------------------------------------*/

 void set_ComputeConfig( ComputeConfig * scfg = nullptr ) override {
  if( f_function )
   f_function->set_ComputeConfig( scfg );
  }

/** @} ---------------------------------------------------------------------*/
/*------------ METHODS DESCRIBING THE BEHAVIOR OF A FRealObjective ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a FRealObjective
 *  @{ */

 /// evaluate the FRealObjective
 /** This FRealObjective is evaluated by evaluating the Function that
  * defines it. */

 int compute( bool changedvars = true ) override {
  return( f_function ? f_function->compute( changedvars ) : kUnEval );
  }

/*--------------------------------------------------------------------------*/
 /// returns the (real) value of the FRealObjective
 /** Method that returns the (real) value of this FRealObjective. It can
  * only be called after that compute() has been called. */

 [[nodiscard]] OFValue value( void ) const override {
  return( f_function ? f_function->get_value() : Inf< OFValue >() );
  }

/*--------------------------------------------------------------------------*/
 /// returns the constant of the FRealObjective
 /** Method that returns the constant term of this FRealObjective, which is
  * the constant term of the underlying Function. */

 [[nodiscard]] OFValue get_constant_term( void ) const override {
  return( f_function ? f_function->get_constant_term() : 0 );
  }

/** @} ---------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the FRealObjective; they all dispatch
 * the method of the underlying Function, so it is an error to call them if
 * the Function has not been set yet.
 *  @{ */

 [[nodiscard]] idx_type get_num_int_par( void ) const override {
  return( f_function->get_num_int_par() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type get_num_dbl_par( void ) const override {
  return( f_function->get_num_dbl_par() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type get_num_str_par( void ) const override {
  return( f_function->get_num_str_par() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type get_num_vint_par( void ) const override {
  return( f_function->get_num_vint_par() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type get_num_vdbl_par( void ) const override {
  return( f_function->get_num_vdbl_par() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type get_num_vstr_par( void ) const override {
  return( f_function->get_num_vstr_par() );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] int get_dflt_int_par( idx_type par ) const override {
  return( f_function->get_dflt_int_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] double get_dflt_dbl_par( idx_type par ) const override {
  return( f_function->get_dflt_dbl_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & get_dflt_str_par( idx_type par )
  const override {
  return( f_function->get_dflt_str_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::vector< int > & get_dflt_vint_par( idx_type par )
  const override {
  return( f_function->get_dflt_vint_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::vector< double > & get_dflt_vdbl_par( idx_type par )
  const override {
  return( f_function->get_dflt_vdbl_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::vector< std::string > & get_dflt_vstr_par(
					      idx_type par ) const override {
  return( f_function->get_dflt_vstr_par( par ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] int get_int_par( idx_type par ) const override {
  return( f_function->get_int_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] double get_dbl_par( idx_type par ) const override {
  return( f_function->get_dbl_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & get_str_par( idx_type par )
  const override {
  return( f_function->get_str_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::vector< int > & get_vint_par( idx_type par )
  const override {
  return( f_function->get_vint_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::vector< double > & get_vdbl_par( idx_type par )
  const override {
  return( f_function->get_vdbl_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::vector< std::string > & get_vstr_par( idx_type par )
  const override {
  return( f_function->get_vstr_par( par ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] idx_type int_par_str2idx( const std::string & name )
  const override {
  return( f_function->int_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type dbl_par_str2idx( const std::string & name )
  const override {
  return( f_function->dbl_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type str_par_str2idx( const std::string & name )
  const override {
  return( f_function->str_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type vint_par_str2idx( const std::string & name )
  const override {
  return( f_function->vint_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type vdbl_par_str2idx( const std::string & name )
  const override {
  return( f_function->vdbl_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type vstr_par_str2idx( const std::string & name )
  const override {
  return( f_function->vstr_par_str2idx( name ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] const std::string & int_par_idx2str( idx_type idx )
  const override {
  return( f_function->int_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & dbl_par_idx2str( idx_type idx )
  const override {
  return( f_function->dbl_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & str_par_idx2str( idx_type idx )
  const override {
  return( f_function->str_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & vint_par_idx2str( idx_type idx )
  const override {
  return( f_function->vint_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & vdbl_par_idx2str( idx_type idx )
  const override {
  return( f_function->vdbl_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & vstr_par_idx2str( idx_type idx )
  const override {
  return( f_function->vstr_par_idx2str( idx ) );
  }

/*--------------------------------------------------------------------------*/

 ComputeConfig * get_ComputeConfig( bool all = false ,
                    ComputeConfig * ocfg = nullptr ) const override {
  return( f_function->get_ComputeConfig( all, ocfg ) );
  }

/** @} ---------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE FRealObjective -------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" Variable in the
 * FRealObjective; they all dispatch the method of the underlying Function
 * @{ */

 [[nodiscard]] Index get_num_active_var( void ) const override {
  return( f_function ? f_function->get_num_active_var() : 0 );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 Index is_active( const Variable * f_variable ) const override {
  return( f_function ? f_function->is_active( f_variable ) : Inf< Index >() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] Variable * get_active_var( Index i ) const override {
  return( f_function ? f_function->get_active_var( i ) : nullptr );
  }

/*--------------------------------------------------------------------------*/

 v_iterator * v_begin( void ) override {
  return( f_function ? f_function->v_begin() : nullptr );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] v_const_iterator * v_begin( void ) const override {
  return( f_function ?
	  static_cast< const Function * >( f_function )->v_begin() : nullptr );
  }

/*--------------------------------------------------------------------------*/

 v_iterator * v_end( void ) override {
  return( f_function ? f_function->v_end() : nullptr );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] v_const_iterator * v_end( void ) const override {
  return( f_function ?
           static_cast< const Function * >( f_function )->v_end() : nullptr );
  }

/*--------------------------------------------------------------------------*/

 void remove_variable( Index i , ModParam issueMod = eModBlck ) final;

/*--------------------------------------------------------------------------*/

 void remove_variables( Range range , ModParam issueMod = eModBlck ) final;

/*--------------------------------------------------------------------------*/

 void remove_variables( Subset && nms , bool ordered = false ,
                        ModParam issueMod = eModBlck ) final;

/** @} ---------------------------------------------------------------------*/
/*------------- METHODS DESCRIBING THE BEHAVIOR OF AN Observer -------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of an Observer
 *  @{ */

 /// the FRealObjective "is always listening"
 /** In principle, the FRealObjective should "be listening" only if the Block
  * (if any) is. However, FRealObjective relies on FunctionModVars to know if
  * something has happened to the Variable of the Function and
  * register/unregister itself from them. For this to happen, the
  * FunctionModVars must be issued by the Function even if there is no Solver
  * "listening" to the Block of this c. To force this to happen,
  * the FRealObjective "is always listening". This may lead to Modification
  * of the Function to be issued even if there is in fact no-one "listening"
  * to them. Hopefully this potential inefficiency will be fixed later on by
  * some mechanism allowing a finer control on which Modification are
  * "listened to". */

 [[nodiscard]] bool anyone_there( void ) const override {
  // return( f_Block ? f_Block->anyone_there() : false );
  return( true );
  }

/*--------------------------------------------------------------------------*/
 /// mostly just dispatch to add_Modification() of the Block (if any)
 /** add_Modification() mostly just dispatch to add_Modification() of the
  * Block (if any). However, it also checks if mod is a FunctionModVars
  * (which is why the FRealObjective "is always listening", see
  * anyone_there(), and in case register/unregister itself with the
  * added/removed Variable. */

 void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override;

/*--------------------------------------------------------------------------*/
 /// just dispatch to open_channel() of the Block (if any)

 ChnlName open_channel( ChnlName chnl = 0 ,
			GroupModification * gmpmod = nullptr ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 /// just dispatch to close_channel() of the Block (if any)

 void close_channel( ChnlName chnl , bool force = false  ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 /// just dispatch to set_default_channel() of the Block (if any)

 void set_default_channel( ChnlName chnl = 0 ) override;

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
 /// print information about the FRealObjective on an ostream
 /** Protected method intended to print information about the
  * FRealObjective; it is virtual so that derived classes can
  * print their specific information in the format they choose. The
  * level of the verbosity of the printed information can be
  * controlled looking at the appropriate information in the Block to
  * which this FRealObjective belongs [see Block.h]. */

 void print( std::ostream & output ) const override {
  output << "FRealObjectiveFunction [" << this << "] of Block [" << f_Block
         << "] with Function [" << f_function << "] with "
         << ( f_function ? f_function->get_num_active_var() : 0 )
         << " active variables, to ";
  if( f_sense == eMin )
   output << "minimize" << std::endl;
  else
   output << "maximize" << std::endl;
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 Function * f_function = nullptr;
 ///< pointer to the Function that defines this FRealObjective

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

 };  // end( class( FRealObjective ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS FRealObjectiveMod ---------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modifications specific to a FRealObjective
/** Derived class from ObjectiveMod to describe Modification specific to a
 * FRealObjective, i.e., changing the Function.
 *
 * Defining a class is a bit weird because the only thing the class does is
 * to define am enum for the new value of the type of Modification in the
 * ObjectiveMod. However, throwing a Modification of a different class (but
 * derived from ObjectiveMod) may make it easier for the Solver to handle it
 * (at the very least, it directly knows it comes from a FRealObjective
 * without having to check it). */

class FRealObjectiveMod : public ObjectiveMod {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/
 /// public enum for the types of ObjectiveMod thrown
 /** Public enum "extending" of_mod_type with the new types of Modification
  * thrown by FRealObjective. */

 enum FRO_cons_mod_type {
  eFunctionChanged = eOFModLastParam,
  ///< the Function underlying this FRealObjective changed whole

  eFROConstModLastParam
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of types of Modification. */
  };

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: just calls that of ObjectiveMod

 explicit FRealObjectiveMod( FRealObjective * obj ,
                             int mod = eFunctionChanged , bool cB = true )
  : ObjectiveMod( obj , mod , cB ) {}

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~FRealObjectiveMod() override = default;  ///< destructor: does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the FRealObjectiveMod

 void print( std::ostream & output ) const override {
  output << "FRealObjectiveMod[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on FRealObjective["
         << f_of << "]: changing the Function" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( FRealObjectiveMod ) )

/** @} end( group( FRealObjective_CLASSES ) ) ------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* FRealObjective.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File FRealObjective.h ----------------------*/
/*--------------------------------------------------------------------------*/
