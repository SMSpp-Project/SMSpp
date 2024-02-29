/*--------------------------------------------------------------------------*/
/*----------------------- File FRowConstraint.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the FRowConstraint class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
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
                                   ModParam issueMod , bool deleteold )
{
 if( function == f_function )  // changing nothing
  return;                      // all done

 if( f_function ) {
  // this Constraint is no longer interested in the Modification
  // of the old Function: unregister itself from that Function
  f_function->register_Observer();

  // also, this Constraint is no longer active in the Variable of the Function
  const auto vend = f_function->end();
  for( auto vbeg = f_function->begin() ; vbeg != vend ; ++vbeg )
   vbeg->remove_active( this );
  }

 // if so instructed, delete the old Function
 if( deleteold )
  delete f_function;

 // update the Function associated with this Constraint
 f_function = function;

 if( f_function ) {
  // register this Constraint as an Observer of the given Function (if any)
  f_function->register_Observer( this );

  // register this Constraint as active in the Variable of the Function
  const auto vend = f_function->end();
  for( auto vbeg = f_function->begin() ; vbeg != vend ; ++vbeg )
   vbeg->add_active( this );
  }

 // if so instructed, issue the FRowConstraintMod
 if( f_Block && f_Block->issue_mod( issueMod ) )
  f_Block->add_Modification( std::make_shared< FRowConstraintMod >(
                              this , FRowConstraintMod::eFunctionChanged ,
                              Observer::par2concern( issueMod ) ) ,
                             Observer::par2chnl( issueMod ) );

 }  // end( FRowConstraint::set_function )

/*--------------------------------------------------------------------------*/

void FRowConstraint::set_rhs( c_RHSValue rhs_value , ModParam issueMod )
{
 if( f_rhs == rhs_value )  // actually doing nothing
  return;                  // cowardly (and silently) return

 f_rhs = rhs_value;        // change the value

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< RowConstraintMod >(
                             this , RowConstraintMod::eChgRHS ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void FRowConstraint::set_lhs( c_RHSValue lhs_value , ModParam issueMod )
{
 if( f_lhs == lhs_value )  // actually doing nothing
  return;                  // cowardly (and silently) return

 f_lhs = lhs_value;        // change the value

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< RowConstraintMod >(
                             this , RowConstraintMod::eChgLHS ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void FRowConstraint::set_both( c_RHSValue both_value , ModParam issueMod )
{
 if( ( f_rhs == both_value ) && ( f_lhs == both_value ) )  // doing nothing
  return;                                 // cowardly (and silently) return

 f_lhs = both_value;
 f_rhs = both_value;

 if( ( ! f_Block ) || ( ! f_Block->issue_mod( issueMod ) ) )
  return;

 f_Block->add_Modification( std::make_shared< RowConstraintMod >(
                             this , RowConstraintMod::eChgBTS ,
                             Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE FRowConstraint -------*/
/*--------------------------------------------------------------------------*/

void FRowConstraint::remove_variable( Index i , ModParam issueMod )
{
 /* FRowConstraint typically relies on FunctionModVars to know if something
  * has happened to the Variable of the Function and register/unregister
  * itself from them. However, in this case it knows beforehand what is
  * happening. If there is no real reason to have the Modification issued,
  * it will instruct the Function not to and do the unregistering herein. */

 if( ! f_function )
  return;

 if( ( par2mod( issueMod ) > eNoMod ) && f_Block->anyone_there() )
  f_function->remove_variable( i , issueMod );
 else {
  // unregistration can preceed removal, since the Function completely
  // ignores this information
  f_function->get_active_var( i )->remove_active( this );
  f_function->remove_variable( i, eNoMod );
  }
 }  // end( FRowConstraint::remove_variable )

/*--------------------------------------------------------------------------*/

void FRowConstraint::remove_variables( Range range , ModParam issueMod )
{
 if( ! f_function )
  return;

 if( ( par2mod( issueMod ) > eNoMod ) && f_Block->anyone_there() )
  f_function->remove_variables( range, issueMod );
 else {
  // unregistration can preceed removal, since the Function completely
  // ignores this information
  for( Index i = range.first ; i < range.second ; )
   f_function->get_active_var( i++ )->remove_active( this );
  f_function->remove_variables( range, eNoMod );
  }
 }  // end( FRowConstraint::remove_variables( range ) )

/*--------------------------------------------------------------------------*/

void FRowConstraint::remove_variables( Subset && nms , bool ordered ,
                                       ModParam issueMod )
{
 if( ! f_function )
  return;

 if( ( par2mod( issueMod ) > eNoMod ) && f_Block->anyone_there() )
  f_function->remove_variables( std::move( nms ) , ordered , issueMod );
 else {
  // unregistration can preceed removal, since the Function completely
  // ignores this information
  for( auto i : nms )
   f_function->get_active_var( i++ )->remove_active( this );
  f_function->remove_variables( std::move( nms ) , ordered , eNoMod );
  }
 }  // end( FRowConstraint::remove_variables( subset ) )

/*--------------------------------------------------------------------------*/
/*------------- METHODS DESCRIBING THE BEHAVIOR OF AN Observer -------------*/
/*--------------------------------------------------------------------------*/

void FRowConstraint::add_Modification( sp_Mod mod , c_ChnlName chnl )
{
 // first check if mod is some :FunctionModVars, and if it is- - - - - - - - -
 // register/unregister this FRowConstraintwith the added/removed Variable
 /* Use a Lambda to define a "guts" of the method that can be called
    recursively without having to pass "local globals". Note the trick of
    defining the std::function object and "passing" it to the lambda,
    which allows recursive calls. Note the need to explicitly capture
    "this" to use fields/methods of the class. */

 std::function< void( sp_Mod ) > guts_of_aM;
 guts_of_aM = [ this , & guts_of_aM ]( const sp_Mod & mod ) {
  // process Modification- - - - - - - - - - - - - - - - - - - - - - - - - - -
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  /* This requires to patiently sift through the possible Modification types
     to find what this Modification exactly is; for the :FunctionModVars
     ones, Variable registration/unregistration has to ensue. */

  // GroupModification - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  {
   const auto tmod = std::dynamic_pointer_cast< GroupModification >( mod );
   if( tmod ) {
    for( const auto & submod : tmod->sub_Modifications() )
     guts_of_aM( submod );
    }
   }

  // FunctionModVars - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  {
   const auto tmod = std::dynamic_pointer_cast< FunctionModVars >( mod );
   if( ! tmod )
    return;

   if( tmod->added() )
    for( auto el : tmod->vars() )
     el->add_active( this );
   else
    for( auto el : tmod->vars() )
     el->remove_active( this );
   }

  // whatever has happened, or not, so far, nothing else has to be done

  };  // end( guts_of_aM ) - - - - - - - - - - - - - - - - - - - - - - - - - -
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 guts_of_aM( mod );  // now the actual call to the "guts of"

 // finally, dispatch to add_Modification() of the Block - - - - - - - - - - -

 if( f_Block && f_Block->anyone_there() )  // ... if any, and listening
  f_Block->add_Modification( mod , chnl );

 }  // end( FRowConstraint::add_Modification )

/*--------------------------------------------------------------------------*/
/*---------------------- End File FRowConstraint.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
