/*--------------------------------------------------------------------------*/
/*-------------------------- File Variable.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the ColVariable class.
 *
 * \version 0.20
 *
 * \date 19 - 02 - 2019
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

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

void ColVariable::set_type( const var_type type , c_ModParam issueMod )
{
 if( type == get_type() )  // actually doing nothing
  return;                  // cowardly (and silently) return

 f_state &= var_type( 1 );  // clear all bits except the LSB
 f_state |= type * 2;       // set the type, leaving the LSB unchanged

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared<VariableMod>( this ,
					Observer::par2concern( issueMod ) ) ,
			    Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File Variable.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
