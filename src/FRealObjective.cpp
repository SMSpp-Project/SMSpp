/*--------------------------------------------------------------------------*/
/*----------------------- File FRealObjective.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the FRealObjective class.
 *
 * \version 0.10
 *
 * \date 05 - 04 - 2018
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
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"
#include "Variable.h"
#include "FRealObjective.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void FRealObjective::set_function( Function * const function ,
				   c_ModParam issueMod , bool deleteold )
{
 if( function == f_function )  // changing nothing
  return;                      // all done

 // this Objective is no longer interested in the Modification
 // of the old Function: unregister itself from that Function
 if( f_function )
  f_function->register_Observer();

 // if so instructed, delete the old Function
 if( deleteold )
  delete f_function;

 // update the Function associated with this Objective
 f_function = function;

  // register this Objective as an Observer of the given Function
 if( f_function )
  f_function->register_Observer( this );

 // if so instructed, issue the FRealObjectiveMod
 if( f_Block && f_Block->issue_mod( issueMod ) )
  f_Block->add_Modification( std::make_shared<FRealObjectiveMod>( this ,
				        FRealObjectiveMod::eFunctionChanged ,
					Observer::par2concern( issueMod ) ) ,
			     Observer::par2chnl( issueMod ) );

 }  // end( FRealObjective::set_function )

/*--------------------------------------------------------------------------*/
/*----------------------- End File FRealObjective.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
