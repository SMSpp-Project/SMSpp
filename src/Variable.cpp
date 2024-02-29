/*--------------------------------------------------------------------------*/
/*-------------------------- File Variable.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the Variable class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
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
#include "Variable.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void Variable::is_fixed( bool fixed , c_ModParam issueMod )
{
 if( fixed == is_fixed() )  // actually doing nothing
  return;                   // cowardly (and silently) return

 auto old_state = f_state;
 if( fixed )  // fix/unfix it
  f_state = f_state | var_type( 1 );
 else
  f_state = f_state & ~var_type( 1 );

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< VariableMod >(
                             this , old_state , f_state ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File Variable.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
