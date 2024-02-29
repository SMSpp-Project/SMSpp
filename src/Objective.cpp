/*--------------------------------------------------------------------------*/
/*-------------------------- File Objective.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the Objective class.
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
#include "Objective.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void Objective::set_sense( int new_sense, c_ModParam issueMod ) {
 if( new_sense == f_sense )  // actually doing nothing
  return;                    // cowardly (and silently) return

 f_sense = new_sense;        // set the new sense

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< ObjectiveMod >(
                             this , f_sense == eMin ? ObjectiveMod::eSetMin :
                                                      ObjectiveMod::eSetMax ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
}

/*--------------------------------------------------------------------------*/
/*------------------------ End File Objective.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
