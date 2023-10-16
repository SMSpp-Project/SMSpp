/*--------------------------------------------------------------------------*/
/*------------------------- File C05Function.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the C05Function class.
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

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Observer.h"
#include "C05Function.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void C05Function::delete_linearizations( Subset && which , bool ordered ,
                                         c_ModParam issueMod )
{
 Index n = get_int_par( intGPMaxSz );

 if( which.empty() ) {  // delete them all
  for( Index i = 0 ; i < n ; ++i )
   if( is_linearization_there( i ) )
    delete_linearization( i, eNoMod );
   }
 else {                 // delete the given subset
  if( ! ordered )
   std::sort( which.begin() , which.end() );

  if( which.back() >= n )
   throw( std::invalid_argument(
       "C05Function::delete_linearizations: invalid linearization name" ) );

  for( Index i : which )
   if( is_linearization_there( i ) )
    delete_linearization( i , eNoMod );
  }

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                this , C05FunctionMod::GlobalPoolRemoved ,
                                std::move( which ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- End File C05Function.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
