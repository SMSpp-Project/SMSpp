/*--------------------------------------------------------------------------*/
/*----------------------- File FRowConstraint.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the FRowConstraint class.
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
#include "FRowConstraint.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void FRowConstraint::set_function( Function * const function ,
				   c_ModParam issueMod , bool deleteold )
{
 if( function == f_function )  // changing nothing
  return;                      // all done

 // this Constraint is no longer interested in the Modification
 // of the old Function: unregister itself from that Function
 if( f_function )
  f_function->register_Observer();

 // if so instructed, delete the old Function
 if( deleteold )
  delete f_function;

 // update the Function associated with this Constraint
 f_function = function;

 // register this Constraint as an Observer of the given Function (if any)
 if( f_function )
  f_function->register_Observer( this );

 // if so instructed, issue the FRowConstraintMod
 if( f_Block && f_Block->issue_mod( issueMod ) )
  f_Block->add_Modification( std::make_shared<FRowConstraintMod>( this ,
				        FRowConstraintMod::eFunctionChanged ,
					Observer::par2concern( issueMod ) ) ,
			     Observer::par2chnl( issueMod ) );

 }  // end( FRowConstraint::set_function )

/*--------------------------------------------------------------------------*/

void FRowConstraint::set_rhs( c_RHSValue rhs_value , c_ModParam issueMod )
{
 if( f_rhs == rhs_value )  // actually doing nothing
  return;                  // cowardly (and silently) return

 f_rhs = rhs_value;        // change the value

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared<RowConstraintMod>( this ,
	    RowConstraintMod::eChgRHS , Observer::par2concern( issueMod ) ) ,
			    Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/
 
void FRowConstraint::set_lhs( c_RHSValue lhs_value , c_ModParam issueMod )
{
 if( f_lhs == lhs_value )  // actually doing nothing
  return;                  // cowardly (and silently) return

 f_lhs = lhs_value;        // change the value

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared<RowConstraintMod>( this ,
	    RowConstraintMod::eChgLHS , Observer::par2concern( issueMod ) ) ,
			    Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void FRowConstraint::set_both( c_RHSValue both_value , c_ModParam issueMod )
{
 if( ( f_rhs == both_value ) && ( f_lhs == both_value ) )  // doing nothing
  return;                                 // cowardly (and silently) return

 f_lhs = both_value;
 f_rhs = both_value;

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared<RowConstraintMod>( this ,
	    RowConstraintMod::eChgBTS , Observer::par2concern( issueMod ) ) ,
			    Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File FRowConstraint.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
