/*--------------------------------------------------------------------------*/
/*----------------------- File FRealObjective.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the FRealObjective class.
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
                                   ModParam issueMod , bool deleteold )
{
 if( function == f_function )  // changing nothing
  return;                      // all done

 if( f_function ) {
  // this Objective is no longer interested in the Modification
  // of the old Function: unregister itself from that Function
  f_function->register_Observer();

  // also, this Objective is no longer active in the Variable of the Function
  const auto vend = f_function->end();
  for( auto vbeg = f_function->begin(); vbeg != vend; ++vbeg )
   vbeg->remove_active( this );
  }

 // if so instructed, delete the old Function
 if( deleteold )
  delete f_function;

 // update the Function associated with this Objective
 f_function = function;

 if( f_function ) {
  // register this Objective as an Observer of the given Function (if any)
  f_function->register_Observer( this );

  // register this Objective as active in the Variable of the Function
  const auto vend = f_function->end();
  for( auto vbeg = f_function->begin() ; vbeg != vend ; ++vbeg )
   vbeg->add_active( this );
  }

 // if so instructed, issue the FRealObjectiveMod
 if( f_Block && f_Block->issue_mod( issueMod ) )
  f_Block->add_Modification( std::make_shared< FRealObjectiveMod >(
                              this , FRealObjectiveMod::eFunctionChanged ,
                              Observer::par2concern( issueMod ) ) ,
                             Observer::par2chnl( issueMod ) );

 }  // end( FRealObjective::set_function )

/*--------------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE FRealObjective -------*/
/*--------------------------------------------------------------------------*/

void FRealObjective::remove_variable( Index i, ModParam issueMod ) {
 /* FRealObjective typically relies on FunctionModVars to know if something
  * has happened to the Variable of the Function and register/unregister
  * itself from them. However, in this case it knows beforehand what is
  * happening. If there is no real reason to have the Modification issued,
  * it will instruct the Function not to and do the unregistering herein. */

 if( ! f_function )
  return;

 if( ( par2mod( issueMod ) > eNoMod ) && f_Block->anyone_there() )
  f_function->remove_variable( i, issueMod );
 else {
  // unregistration can preceed removal, since the Function completely
  // ignores this information
  f_function->get_active_var( i )->remove_active( this );
  f_function->remove_variable( i, eNoMod );
 }
}  // end( FRealObjective::remove_variable )

/*--------------------------------------------------------------------------*/

void FRealObjective::remove_variables( Range range, ModParam issueMod ) {
 if( ! f_function )
  return;

 if( ( par2mod( issueMod ) > eNoMod ) && f_Block->anyone_there() )
  f_function->remove_variables( range, issueMod );
 else {
  // unregistration can preceed removal, since the Function completely
  // ignores this information
  for( Index i = range.first; i < range.second; )
   f_function->get_active_var( i++ )->remove_active( this );
  f_function->remove_variables( range, eNoMod );
 }
}  // end( FRealObjective::remove_variables( range ) )

/*--------------------------------------------------------------------------*/

void FRealObjective::remove_variables( Subset && nms, bool ordered,
                                       ModParam issueMod ) {
 if( ! f_function )
  return;

 if( ( par2mod( issueMod ) > eNoMod ) && f_Block->anyone_there() )
  f_function->remove_variables( std::move( nms ), ordered, issueMod );
 else {
  // unregistration can preceed removal, since the Function completely
  // ignores this information
  for( auto i : nms )
   f_function->get_active_var( i++ )->remove_active( this );
  f_function->remove_variables( std::move( nms ), ordered, eNoMod );
 }
}  // end( FRealObjective::remove_variables( subset ) )

/*--------------------------------------------------------------------------*/

void FRealObjective::add_Modification( sp_Mod mod , c_ChnlName chnl ) {
 // first check if mod is some :FunctionModVars, and if it is- - - - - - - - -
 // register/unregister this FRowConstraint with the added/removed Variable
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

 }  // end( FRealObjective::add_Modification )

/*--------------------------------------------------------------------------*/
/// just dispatch to open_channel() of the Block (if any)

Observer::ChnlName FRealObjective::open_channel( ChnlName chnl ,
						 GroupModification * gmpmod )
{
 return( f_Block ? f_Block->open_channel( chnl , gmpmod ) : 0 );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/// just dispatch to close_channel() of the Block (if any)

void FRealObjective::close_channel( ChnlName chnl , bool force )
{
 if( f_Block )
  f_Block->close_channel( chnl , force );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/// just dispatch to set_default_channel() of the Block (if any)

void FRealObjective::set_default_channel( ChnlName chnl )
{
 if( f_Block )
  f_Block->set_default_channel( chnl );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- End File FRealObjective.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
