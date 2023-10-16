/*--------------------------------------------------------------------------*/
/*------------------------- File Constraint.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the Constraint class.
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
#include "Constraint.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void Constraint::relax( bool relax_it, c_ModParam issueMod ) {
 if( relax_it == f_is_relaxed )  // actually doing nothing
  return;                        // cowardly (and silently) return

 f_is_relaxed = relax_it;        // relaxed/enforce it

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification(
  std::make_shared< ConstraintMod >(
   this,
   f_is_relaxed ? ConstraintMod::eRelaxConst : ConstraintMod::eEnforceConst,
   Observer::par2concern( issueMod ) ),
  Observer::par2chnl( issueMod ) );
}

/*--------------------------------------------------------------------------*/
/*--------------------- End File Constraint.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
