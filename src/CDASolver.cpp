/*--------------------------------------------------------------------------*/
/*--------------------------- File CDASolver.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the CDASolver class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"

#include "CDASolver.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------- METHODS OF CDASolver -----------------------------*/
/*--------------------------------------------------------------------------*/

Solution * CDASolver::get_Solution( Configuration * solc )
{
 // the primal information, exactly as the base implementation does
 if( has_var_solution() ) {
  f_Block->lock( f_id );
  get_var_solution( solc );
  }
 else
  if( has_var_direction() ) {
   f_Block->lock( f_id );
   get_var_direction( solc );
   }
  else
   return( nullptr );

 /* The dual information, which the base implementation leaves to whatever
  * happens to be in the Constraint, i.e., typically zero: this is what a
  * :Solution that stores the dual of the Block would be filled with. */

 if( has_dual_solution() )
  get_dual_solution( solc );
 else
  if( has_dual_direction() )
   get_dual_direction( solc );

 auto sol = f_Block->get_Solution( solc , false );
 f_Block->unlock( f_id );

 return( sol );

 }  // end( CDASolver::get_Solution )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File CDASolver.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
