/*--------------------------------------------------------------------------*/
/*-------------------------- File Objective.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the Objective class.
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
#include "Objective.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void Objective::set_sense( int new_sense , c_ModParam issueMod )
{
 if( new_sense == f_sense )  // actually doing nothing
  return;                    // cowardly (and silently) return

 f_sense = new_sense;        // set the new sense

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared<ObjectiveMod>( this ,
		                    f_sense == eMin ? ObjectiveMod::eSetMin :
				  	              ObjectiveMod::eSetMax ,
				    Observer::par2concern( issueMod ) ) ,
			    Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File Objective.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
