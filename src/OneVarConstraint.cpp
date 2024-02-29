/*--------------------------------------------------------------------------*/
/*---------------------- File OneVarConstraint.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of OneVarConstraint and related classes.
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
#include "OneVarConstraint.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------------------- FUNCTIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void OneVarConstraint::set_variable( ColVariable * const variable,
                                     ModParam issueMod )
{
 if( variable == f_variable )  // changing nothing
  return;                      // all done

 if( f_variable )   // remove this Constraint from its only active Variable
  f_variable->remove_active( this );

 // update the ColVariable associated with this Constraint
 f_variable = variable;

 if( f_variable )   // add this Constraint to its only active Variable
  f_variable->add_active( this );

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< OneVarConstraintMod >(
                             this , OneVarConstraintMod::eVariableChanged ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );

 }  // end( OneVarConstraint::set_variable )

/*--------------------------------------------------------------------------*/

void BoxConstraint::set_rhs( c_RHSValue rhs_value , ModParam issueMod )
{
 if( f_rhs == rhs_value )  // actually doing nothing
  return;                  // cowardly (and silently) return

 f_rhs = rhs_value;        // change the value

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< OneVarConstraintMod >(
                             this , RowConstraintMod::eChgRHS ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void BoxConstraint::set_lhs( c_RHSValue lhs_value , ModParam issueMod )
{
 if( f_lhs == lhs_value )  // actually doing nothing
  return;                  // cowardly (and silently) return

 f_lhs = lhs_value;        // change the value

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< OneVarConstraintMod >(
                             this , RowConstraintMod::eChgLHS ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void BoxConstraint::set_both( c_RHSValue both_value , ModParam issueMod )
{
 if( ( f_rhs == both_value ) && ( f_lhs == both_value ) )  // doing nothing
  return;                                 // cowardly (and silently) return

 f_lhs = both_value;
 f_rhs = both_value;

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< OneVarConstraintMod >(
                             this , RowConstraintMod::eChgBTS ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void LB0Constraint::set_rhs( c_RHSValue rhs_value, ModParam issueMod )
{
 if( f_rhs == rhs_value )  // actually doing nothing
  return;                  // cowardly (and silently) return

 f_rhs = rhs_value;        // change the value

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< OneVarConstraintMod >(
                             this , RowConstraintMod::eChgRHS ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void UB0Constraint::set_lhs( c_RHSValue lhs_value , ModParam issueMod )
{
 if( f_lhs == lhs_value )  // actually doing nothing
  return;                  // cowardly (and silently) return

 f_lhs = lhs_value;        // change the value

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< OneVarConstraintMod >(
                             this , RowConstraintMod::eChgLHS ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void LBConstraint::set_lhs( c_RHSValue lhs_value , ModParam issueMod )
{
 if( f_lhs == lhs_value )  // actually doing nothing
  return;                  // cowardly (and silently) return

 f_lhs = lhs_value;        // change the value

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< OneVarConstraintMod >(
                             this , RowConstraintMod::eChgLHS ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void UBConstraint::set_rhs( c_RHSValue rhs_value, ModParam issueMod )
{
 if( f_rhs == rhs_value )  // actually doing nothing
  return;                  // cowardly (and silently) return

 f_rhs = rhs_value;        // change the value

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< OneVarConstraintMod >(
                             this , RowConstraintMod::eChgRHS ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- End File OneVarConstraint.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
