/*--------------------------------------------------------------------------*/
/*------------------------ File FRowConstraint.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the FRowConstraint class, derived from RowConstraint,
 * which is a class that defines a row constraint in terms of an
 * externally-provided Function object.
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

class FRowConstraint : public RowConstraint, public Observer {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
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

 explicit FRowConstraint( Block * block = nullptr ,
                          RHSValue lhs = 0 , RHSValue rhs = 0 ,
                          Function * const function = nullptr )
  : RowConstraint( block ) , f_lhs( lhs ),  f_rhs( rhs ) ,
    f_function( nullptr ) {
  set_function( function , eNoMod );
  }

/*--------------------------------------------------------------------------*/
 /// destructor: deletes the Function and un-registers with the Variable
 /** By calling set_function( nullptr ), the destructor un-registers with the
  * Variable of the Function (if clear() has not been called first) and then
  * deletes it. */

 ~FRowConstraint() override { set_function( nullptr, eNoMod ); }

/*--------------------------------------------------------------------------*/
 /// "rough destructor": calls the version of the Function object
 /** The clear() method just calls clear() in the inner Function (if any).
  * This results in the list of Variable of the Function to be emptied,
  * so that in the destructor they re not un-registered. */

 void clear() override {
  if( f_function )
   f_function->clear();
 }

/** @} ---------------------------------------------------------------------*/
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

 void set_function( Function * const function = nullptr,
                    ModParam issueMod = eModBlck, bool deleteold = true );

/*--------------------------------------------------------------------------*/

 void set_rhs( RHSValue rhs_value, ModParam issueMod = eModBlck ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void set_lhs( RHSValue lhs_value, ModParam issueMod = eModBlck ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void set_both( RHSValue both_value, ModParam issueMod = eModBlck ) override;

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
/*----------- METHODS FOR READING THE DATA OF THE FRowConstraint -----------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the FRowConstraint
 *  @{ */

 /// returns the Block to which this Observer belongs/
 /** FRowConstraint is an Observer, and it belongs to the Block to which it
  * belongs as a Constraint. However, note that FRowConstraint::get_Block()
  * is virtual while Constraint::get_Block() is not, hence the former has to
  * be explicitly implemented in terms of the latter. */

 [[nodiscard]] Block * get_Block() const override {
  return( Constraint::get_Block() );
  }

/*--------------------------------------------------------------------------*/
 ///< method to get a pointer to the Function of the FRowConstraint

 [[nodiscard]] Function * get_function( void ) const { return( f_function ); }

/*--------------------------------------------------------------------------*/
 /// method to get the RHS of the RowConstraint

 [[nodiscard]] RHSValue get_rhs( void ) const override { return( f_rhs ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get the LHS of the RowConstraint

 [[nodiscard]] RHSValue get_lhs( void ) const override { return( f_lhs ); }

/** @} ---------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF A FRowConstraint ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a FRowConstraint
    @{ */

 /// compute the value of variable part of the FRowConstraint
 /** Compute the value of variable part of the FRowConstraint by
  * compute()-ing the Function that defines this FRowConstraint. */

 int compute( bool changedvars = true ) override {
  return( f_function ? f_function->compute( changedvars ) : kUnEval );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get a lower bound on the value of the variable part
 /** Method to get a lower bound on the value of variable part of the
  * OneVarConstraint, which is the lower estimate on the value of the
  * Function that defines this FRowConstraint. */

 [[nodiscard]] RHSValue lb( void ) const override {
  return( f_function ? f_function->get_lower_estimate() : -RHSINF );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get an upper bound on the value of the variable part
 /** Method to get an upper bound on the value of variable part of the
  * OneVarConstraint, which is the upper estimate on the value of the
  * Function that defines this FRowConstraint. */

 [[nodiscard]] RHSValue ub( void ) const override {
  return( f_function ? f_function->get_upper_estimate() : RHSINF );
  }

/** @} ---------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the FRowConstraint; they all dispatch
 * the method of the underlying Function, so it is an error to call them if
 * the Function has not been set yet.
 * @{ */

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
                                    ComputeConfig * ocfg = nullptr )
 const override {
  return( f_function->get_ComputeConfig( all, ocfg ) );
  }

/** @} ---------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE FRowConstraint -------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" Variable in the
 * FRowConstraint; they all dispatch the method of the underlying Function
 *  @{ */

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

 void remove_variables( Subset && nms , bool ordered = false,
                        ModParam issueMod = eModBlck ) final;

/** @} ---------------------------------------------------------------------*/
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

 [[nodiscard]] bool anyone_there( void ) const override { return( true ); }

/*--------------------------------------------------------------------------*/
 /// mostly just dispatch to add_Modification() of the Block (if any)
 /** add_Modification() mostly just dispatch to add_Modification() of the
  * Block (if any). However, it also checks if mod is a FunctionModVars
  * (which is why the FRowConstraint "is always listening", see
  * anyone_there(), and in case register/unregister itself with the
  * added/removed Variable. */

 void add_Modification( sp_Mod mod , c_ChnlName chnl = 0 ) override;

/*--------------------------------------------------------------------------*/
 /// just dispatch to open_channel() of the Block (if any)

 ChnlName open_channel( ChnlName chnl = 0 ,
			GroupModification * gmpmod = nullptr ) override {
  return( f_Block ? f_Block->open_channel( chnl , gmpmod ) : 0 );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 /// just dispatch to close_channel() of the Block (if any)

 void close_channel( ChnlName chnl , bool force = false ) override {
  if( f_Block )
   f_Block->close_channel( chnl , force );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 /// just dispatch to set_default_channel() of the Block (if any)

 void set_default_channel( ChnlName chnl = 0 ) override {
  if( f_Block )
   f_Block->set_default_channel( chnl );
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

 /// print information about the FRowConstraint on an ostream
 void print( std::ostream & output ) const override {
  output << "FRowConstraint [" << this << "] of Block [" << f_Block
         << "] with Function [" << f_function << "] with "
         << ( f_function ? f_function->get_num_active_var() : 0 )
         << " active variables" << std::endl;
  }

/** @} ---------------------------------------------------------------------*/
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

class FRowConstraintMod : public RowConstraintMod {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/
 /// public enum for the types of FRowConstraintMod thrown
 /** Public enum "extending" RowC_mod_type with the new types of Modification
  * thrown by FRowConstraint. */

 enum FRC_cons_mod_type {
  eFunctionChanged = eRowConstModLastParam,
  ///< the Function underlying this FRowConstraint changed whole

  eFRCConstModLastParam
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of types of Modification. */
  };

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: just calls that of RowConstraintMod

 explicit FRowConstraintMod( FRowConstraint * cnst ,
			     int mod = eFunctionChanged ,
			     bool cB = true )
  : RowConstraintMod( cnst , mod , cB ) {}

 /*------------------------------ DESTRUCTOR --------------------------------*/

 ~FRowConstraintMod() override = default;  ///< destructor: does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the FRowConstraintMod

 void print( std::ostream & output ) const override {
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

/** @} end( group( FRowConstraint_CLASSES ) ) ------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* FRowConstraint.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File FRowConstraint.h -------------------------*/
/*--------------------------------------------------------------------------*/
