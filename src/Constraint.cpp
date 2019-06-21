/*--------------------------------------------------------------------------*/
/*------------------------- File Constraint.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the Constraint class.
 *
 * \version 0.10
 *
 * \date 04 - 04 - 2018
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni, Kostas Tavlaridis-Gyparakis
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"
#include "Constraint.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void Constraint::relax( bool relax_it , c_ModParam issueMod )
{
 if( relax_it == f_is_relaxed )  // actually doing nothing
  return;                        // cowardly (and silently) return

 f_is_relaxed = relax_it;        // relaxed/enforce it

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared<ConstraintMod>( this ,
			      f_is_relaxed ? ConstraintMod::eRelaxConst :
					     ConstraintMod::eEnforceConst ,
			      Observer::par2concern( issueMod ) ) ,
			    Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- End File Constraint.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
