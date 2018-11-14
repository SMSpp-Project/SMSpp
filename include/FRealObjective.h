/*--------------------------------------------------------------------------*/
/*------------------------- File FRealObjective.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the FRealObjective class, which is a RealObjective whose
 * function is given by a Function. Any Modification thrown by the Function
 * associated with this Objective is received by this Objective, which may
 * either repackage that Modification and send a new Modification to the
 * Block or directly send the Modification received. This means that this
 * Objective may throw a FunctionModification.
 *
 * \version 0.20
 *
 * \date 15 - 08 - 2018
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

#ifndef __FRealObjective
 #define __FRealObjective
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Function.h"
#include "Objective.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*@} -----------------------------------------------------------------------*/
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
/** The class FRealObjective is a RealObjective whose function is given by a
 * Function. This class has a field which is pointer to a Function.
 */

class FRealObjective : public RealObjective , Observer {

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
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

 /// constructor of FRealObjective, taking a Block and a Function
 /** Constructor of FRealObjective. It accepts a pointer to the Block to
  * which the FRealObjective belongs and a pointer to the Function that
  * defines it. Both parameters default to nullptr so that this can be used
  * as the void constructor. */

 FRealObjective( Block *my_block = nullptr , Function *function = nullptr )
  : RealObjective( my_block ) , f_function( nullptr ) {
  this->set_function( function , eNoMod );
  }

/*--------------------------------------------------------------------------*/
 /// destructor: deletes the Function object

 virtual ~FRealObjective() {
  set_function( nullptr , eNoMod );
  }

/*--------------------------------------------------------------------------*/

 virtual void clear( void ) override {
  if( f_function )
   f_function->clear();
  }

/*@} -----------------------------------------------------------------------*/
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
  * The parameter issueMod decides if and how the BlockModAD is issued, as
  * described in Observer::make_par(). */

 virtual void set_function( Function * const function = nullptr ,
			    c_ModParam issueMod = eModBlck ,
			    bool deleteold = true );

/*@} -----------------------------------------------------------------------*/
/*----------- METHODS FOR READING THE DATA OF THE FRealObjective -----------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the FRealObjective
 *  @{ */

  /// returns the pointer to the Function in this FRealObjective
  Function *get_function( void ) const { return( f_function ); }

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
/*------------ METHODS DESCRIBING THE BEHAVIOR OF A FRealObjective ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a FRealObjective
 *  @{ */

  /// evaluate the FRealObjective
  /** This FRealObjective is evaluated by evaluating the Function that
   * defines it. */

 virtual int compute( bool changedvars = true ) override
 {
  return( f_function ? f_function->compute( changedvars ) : kUnEval );
  }

/*--------------------------------------------------------------------------*/
 /// returns the (real) value of the FRealObjective
 /** Method that returns the (real) value of this FRealObjective. It can
  * only be called after that compute() has been called. */

 virtual OFValue value( void ) const override
 {
  return( f_function ? f_function->get_value() : Inf<OFValue>() );
  }

/*@} -----------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the FRealObjective; they all dispatch
 * the method of the underlying Function, so it is an error to call them if
 * the Function has not been set yet.
 *  @{ */

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

 virtual ComputeConfig * get_ComputeConfig( bool all = false ,
			     ComputeConfig * ocfg = nullptr ) const override
 {
  return( f_function->get_ComputeConfig( all , ocfg ) );
  }

/*@} -----------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE FRealObjective -------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" Variable in the
 * FRealObjective; they all dispatch the method of the underlying Function
 * @{ */

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

 Variable * get_active_var( const Index i ) const override
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
  if( f_function ) {
   variable->remove_active( this );
   f_function->remove_variable( variable , issueMod );
   }
  }

/*--------------------------------------------------------------------------*/

 virtual void remove_variables( std::vector<Variable *> && vars ,
                                const bool ordered = false ,
                                c_ModParam issueMod = eModBlck ) override
 {
  if( f_function ) {
   for( auto var : vars )
    var->remove_active( this );
   f_function->remove_variables( std::move( vars ) , ordered , issueMod );
   }
  }

/*@} -----------------------------------------------------------------------*/
/*------------- METHODS DESCRIBING THE BEHAVIOR OF AN Observer -------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of an Observer
 *  @{ */

 /// the FRealObjective "is listening" if the Block (if any) is

 virtual bool anyone_there( void ) const override {
  return( f_Block ? f_Block->anyone_there() : false );
  }

/*--------------------------------------------------------------------------*/
 /// just dispatch to add_Modification() of the Block (if any)

 virtual void add_Modification( sp_Mod mod , ChnlName chnl = 0  ) override
 {
  if( f_Block )
   f_Block->add_Modification( mod , chnl );
  }

/*--------------------------------------------------------------------------*/
 /// just dispatch to open_channel() of the Block (if any)

 virtual ChnlName open_channel( GroupModification * gmpmod = nullptr ,
				c_ModParam issueMod = eModBlck )
  override {
  return( f_Block ? f_Block->open_channel( gmpmod , issueMod ) : 0 );
  }

/*--------------------------------------------------------------------------*/
 /// just dispatch to nest_channel() of the Block (if any)

 virtual void nest_channel( c_ChnlName chnl ,
			    GroupModification * gmpmod = nullptr ,
			    c_ModParam issueMod = eModBlck )  override {
  if( f_Block )
   f_Block->nest_channel( chnl , gmpmod , issueMod );
  }

/*--------------------------------------------------------------------------*/
 /// just dispatch to un_nest_channel() of the Block (if any)

 virtual void un_nest_channel( c_ChnlName chnl ) override {
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
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
    @{ */

 /// print information about the FRealObjective on an ostream
  /** Protected method intended to print information about the
   * FRealObjective; it is virtual so that derived classes can
   * print their specific information in the format they choose. The
   * level of the verbosity of the printed information can be
   * controlled looking at the appropriate information in the Block to
   * which this FRealObjective belongs [see Block.h]. */

 virtual void print( std::ostream &output ) const override {
  output << "FRealObjectiveFunction [" << this << "] of Block [" << f_Block
	 << "] with Function [" << f_function << "] with "
         << ( f_function ? f_function->get_num_active_var() : 0 )
         << " active variables, to "
	 << std::endl;
  if( f_sense == eMin )
    output << "minimize" << std::endl;
  else
    output << "maximize" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

  Function *f_function = nullptr;
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

class FRealObjectiveMod : public ObjectiveMod
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/
 /// public enum for the types of ObjectiveMod thrown
 /** Public enum "extending" of_mod_type with the new types of Modification
  * thrown by FRealObjective. */

 enum FRO_cons_mod_type {
  eFunctionChanged = eOFModLastParam ,
  ///< the Function underlying this FRealObjective changed whole

  eFROConstModLastParam
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of types of Modification. */
  };

/*---------------------- CONSTRUCTOR & DESTRUCTOR --------------------------*/

 /// constructor: just calls that of ObjectiveMod

 FRealObjectiveMod( FRealObjective *obj , int mod = eFunctionChanged ,
		    const bool cB = true )
  : ObjectiveMod( obj , mod , cB ) { }

 virtual ~FRealObjectiveMod() { }  ///< destructor: does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the FRealObjectiveMod

 virtual inline void print( std::ostream &output ) const {
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

/*@}  end( group( FRealObjective_CLASSES ) ) -------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* FRealObjective.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File FRealObjective.h ----------------------*/
/*--------------------------------------------------------------------------*/
