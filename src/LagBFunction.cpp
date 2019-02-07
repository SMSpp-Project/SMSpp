/*--------------------------------------------------------------------------*/
/*------------------------ File LagBFunction.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the LagBFunction class.
 *
 * \version 0.01
 *
 * \date 18 - 01 - 2019
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Enrico Gorgone \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni, Enrico Gorgone
 *
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Observer.h"
#include "SMSTypedefs.h"
#include "LagBFunction.h"
#include <math.h>

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

LagBFunction::LagBFunction( v_dual_pair && static_lagrangian_pairs ,
		 const bool static_is_ordered )
 :  C05Function() , slag_p( std::move( static_lagrangian_pairs ) )
{

 if( ! static_is_ordered ) {
  std::sort( slag_p.begin() , slag_p.end() ,
		      []( const auto & p1, const auto & p2 ) {
		       return( p1.first < p2.first );
		       }
	        );
  }


 // define global pool of intGPMaxSz size - - - - - - - - - - - - - - - - - -

 g_pool.resize( intGPMaxSz );
 SlvHasSol = SlvHasDir = false;

 } // end LagBFunction::LagBFunction( ) - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

LagBFunction::~LagBFunction() {

 dlag_p.clear();
 g_pool.clear(); //
 }


/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/



/*--------------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE LagBFunction ----------*/
/*--------------------------------------------------------------------------*/

int LagBFunction::compute( bool changedvars )
{

 } // end LagBFunction::compute( )

/*--------------------------------------------------------------------------*/

void LagBFunction::get_linearization_coefficients( FunctionValue * g ,
				 const LinearizationName name ,
                                 const std::vector<Index> * const indices ,
                                 const Index start , const Index end ) const

{

 // get the Solver from inner Block - - - - - - - - - - - - - - - - - - - - -
 Solver* slv = v_Block[0]->get_registered_solvers().back();

 if( name == Inf<LinearizationName>() ) { // asking for the last computed - -
	                                      // linearization  - - - - - - - - -

  if( !SlvHasSol && !SlvHasDir )
   throw( std::logic_error( "no addition linearization is the local pool" ) );

  // get solution/direction from the solver - - - - - - - - - - - - - - - - -

  if( SlvHasSol )
   slv->get_var_solution();
  else
   slv->get_var_direction();

  }
 else {  // asking for a linearization of the global pool  - - - - - - - - - -

  // assign Solution to the inner Block from which the related linearization
  // <name> can be recovered

  g_pool[ name ].first->write( v_Block[0] );

  } // end else  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // for each Lagrangian multiplier, the value of the relaxed constraint
 // is the component of g  - - - - - - - - - - - - - - - - - - - - - - - - -

 c_Index end_p = std::min( Index(slag_p.size()) , end );

 if( indices != nullptr ) {
  for( const auto & i : *indices )
   if( ( i >= start ) && ( i < end_p ) )
    *(g++) = slag_p[ i ].second->get_value();
  }
 else
  for( Index i = start ; i < end_p ; ++i )
	*(g++) = slag_p[ i ].second->get_value();

 } // end( LagBFunction::get_linearization_coefficients( DenseVector ) )


/*--------------------------------------------------------------------------*/

void LagBFunction::get_linearization_coefficients( SparseVector & g ,
				         const LinearizationName name ,
                                         c_Vec_Index * const indices ,
                                         c_Index start , c_Index end ) const
{

}  // end( LagBFunction::get_linearization_coefficients( SparseVector ) )

/*--------------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE LagBFunction ---------*/
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE LagBFunction --------------------*/
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*---------------------- End File LagBFunction.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
