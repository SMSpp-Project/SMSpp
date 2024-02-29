/*--------------------------------------------------------------------------*/
/*-------------------------- File Variable.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the ColVariable class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"
#include "ColVariable.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void ColVariable::set_type( var_type type , c_ModParam issueMod )
{
 if( type == get_type() )  // actually doing nothing
  return;                  // cowardly (and silently) return

 auto old_state = f_state;
 f_state &= var_type( 1 );  // clear all bits except the LSB
 f_state |= type * 2;       // set the type, leaving the LSB unchanged

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< VariableMod >(
                             this , old_state , f_state ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void ColVariable::is_integer( bool yn , c_ModParam issueMod )
{
 if( yn == is_integer() )  // actually doing nothing
  return;                  // cowardly (and silently) return

 auto old_state = f_state;
 if( yn )
  f_state |= var_type( 2 );
 else
  f_state &= ~var_type( 2 );

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< VariableMod >(
                             this , old_state , f_state ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void ColVariable::is_positive( bool yn, c_ModParam issueMod )
{
 if( yn == is_positive() )  // actually doing nothing
  return;                   // cowardly (and silently) return

 auto old_state = f_state;
 if( yn )
  f_state |= var_type( 4 );
 else
  f_state &= ~var_type( 4 );

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< VariableMod >(
                             this , old_state , f_state ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void ColVariable::is_negative( bool yn , c_ModParam issueMod )
{
 if( yn == is_negative() )  // actually doing nothing
  return;                   // cowardly (and silently) return

 auto old_state = f_state;
 if( yn )
  f_state |= var_type( 8 );
 else
  f_state &= ~var_type( 8 );

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< VariableMod >(
                             this , old_state , f_state ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void ColVariable::is_unitary( bool yn , c_ModParam issueMod )
{
 if( yn == is_unitary() )  // actually doing nothing
  return;                  // cowardly (and silently) return

 auto old_state = f_state;
 if( yn )
  f_state |= var_type( 16 );
 else
  f_state &= ~var_type( 16 );

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< VariableMod >(
                             this , old_state , f_state ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

ColVariable::VarValue ColVariable::get_lb( void ) const {
 return( is_positive() ? VarValue( 0 ) :
         ( is_unitary() ? VarValue( -1 ) : -Inf< VarValue >() ) );
 }

/*--------------------------------------------------------------------------*/

ColVariable::VarValue ColVariable::get_ub( void ) const {
 return( is_negative() ? VarValue( 0 ) :
         ( is_unitary() ? VarValue( 1 ) : Inf< VarValue >() ) );
 }

/*--------------------------------------------------------------------------*/

ColVariable::Index ColVariable::is_active(
                                 const ThinVarDepInterface * stuff ) const {
 auto idx = std::lower_bound( v_active.begin(), v_active.end(), stuff );

 if( idx != v_active.end() )
  return( std::distance( v_active.begin() , idx ) );
 else
  return( Inf< Index >() );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File Variable.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
