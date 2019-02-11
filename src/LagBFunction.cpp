/*--------------------------------------------------------------------------*/
/*------------------------ File LagBFunction.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the LagBFunction class.
 *
 * \version 0.02
 *
 * \date 08 - 02 - 2019
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
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

static const char VarIsDir = 0;  // Variable contain a direction
static const char VarIsSol = 1;  // Variable contain a solution
static const char NoVar = 2;  // No Variable is in the Solver

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

 // so far, the current solution is unknown - - - - - - - - - - - - - - - - -
 VarType = NoVar;
 LastSolution = Inf<LinearizationName>();

 } // end LagBFunction::LagBFunction( )  - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

LagBFunction::~LagBFunction() {

 dlag_p.clear();
 g_pool.clear(); //
 } // end LagBFunction::~LagBFunction( ) - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/



/*--------------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE LagBFunction ------------*/
/*--------------------------------------------------------------------------*/

bool LagBFunction::has_linearization( const bool diagonal )
{
 // get the Solver from inner Block - - - - - - - - - - - - - - - - - - - - -
 Solver* slv = v_Block[0]->get_registered_solvers().back();

 // true if a linearization of the related type exists  - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 bool SlvHasNewLin;

 if( diagonal ) {
  SlvHasNewLin = slv->has_var_solution();
  if( SlvHasNewLin )
   VarType = VarIsSol;
  }
 else {
  SlvHasNewLin = slv->has_var_direction();
  if( SlvHasNewLin )
   VarType = VarIsDir;
  }

 if( !SlvHasNewLin )
  VarType = NoVar;

 return( SlvHasNewLin );

 }  // end LagBFunction::has_linearization( )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

bool LagBFunction::compute_new_linearization( const bool diagonal )
{
 // get the Solver from inner Block - - - - - - - - - - - - - - - - - - - - -
 Solver* slv = v_Block[0]->get_registered_solvers().back();

 // true if a new linearization of the related type exists in the local pool
 // which is kept in the Solver   - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 bool SlvHasNewLin;

 if( diagonal ) {
  SlvHasNewLin = slv->new_var_solution();
  if( SlvHasNewLin )
   VarType = VarIsSol;
  }
 else {
  SlvHasNewLin = slv->new_var_direction();
  if( SlvHasNewLin )
   VarType = VarIsDir;
  }

 if( !SlvHasNewLin )
  VarType = NoVar;

 return( SlvHasNewLin );

 } // end LagBFunction::compute_new_linearization( ) - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::store_linearization( const LinearizationName name )
{

 if( VarType == NoVar )
  throw( std::logic_error( "there is no solution in the solver" ) );

 // get the current solution   - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( g_pool[ name ].first == nullptr )
  g_pool[ name ].first = v_Block[0]->get_Solution();

 g_pool[ name ].first->read( v_Block[0] );

 if( VarType == VarIsSol )
  g_pool[ name ].second = VarIsSol;
 else
  g_pool[ name ].second = VarIsDir;

 } // end LagBFunction::store_linearization( ) - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

int LagBFunction::compute( bool changedvars )
{
 LastSolution = Inf<Index>();	// set LastSolution as the current solution, i.e.
                                // that solution which has computed in compute();

 // get the Solver from inner Block  - - - - - - - - - - - - - - - - - - - - -
 Solver* slv = v_Block[0]->get_registered_solvers().back();

 // no solution/direction is available so far  - - - - - - - - - - - - - - - -

 VarType = NoVar;

 return( slv->compute( false ) );

 } // end LagBFunction::compute( ) - - - - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::get_linearization_coefficients( FunctionValue * g ,
				 const LinearizationName name ,
                                 const std::vector<Index> * const indices ,
                                 const Index start , const Index end )

{

 c_Index end_p = std::min( Index(slag_p.size()) , end );
 if( end_p <= start )
  return;

 // get the Solver from inner Block - - - - - - - - - - - - - - - - - - - - -
 Solver* slv = v_Block[0]->get_registered_solvers().back();

 if( name == Inf<LinearizationName>() ) { // asking for the last computed - -
	                                      // linearization  - - - - - - - - -

  if( VarType == NoVar )
   throw( std::logic_error( "no addition linearization is the local pool" ) );

  // get solution/direction from the solver - - - - - - - - - - - - - - - - -

  if( VarType == VarIsSol )
   slv->get_var_solution();
  else
   slv->get_var_direction();

  }
 else {  // asking for a linearization of the global pool  - - - - - - - - - -

  // assign Solution to the inner Block from which the related linearization
  // <name> can be recovered

  if( name != LastSolution )
   g_pool[ name ].first->write( v_Block[0] );

  LastSolution = name ;    // update last solution

  VarType = NoVar; // the current linearization is not available anymore

  } // end else  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // for each Lagrangian multiplier, the value of the relaxed constraint
 // is the component of g  - - - - - - - - - - - - - - - - - - - - - - - - - -

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
                                         c_Index start , c_Index end )
{

 c_Index end_p = std::min( Index(slag_p.size()) , end );
 if( end_p <= start )
  return;

 // get the Solver from inner Block - - - - - - - - - - - - - - - - - - - - -
 Solver* slv = v_Block[0]->get_registered_solvers().back();

 if( name == Inf<LinearizationName>() ) { // asking for the last computed - -
	                                      // linearization  - - - - - - - - -

  if( VarType == NoVar )
   throw( std::logic_error( "no addition linearization is the local pool" ) );

  // get solution/direction from the solver - - - - - - - - - - - - - - - - -

  if( VarType == VarIsSol )
   slv->get_var_solution();
  else
   slv->get_var_direction();

  }
 else {  // asking for a linearization of the global pool  - - - - - - - - - -

  // assign Solution to the inner Block from which the related linearization
  // <name> can be recovered

  if( name != LastSolution )
   g_pool[ name ].first->write( v_Block[0] );

  LastSolution = name ;    // update last solution

  VarType = NoVar; // the current linearization is not available anymore

  } // end else  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // for each Lagrangian multiplier, the value of the relaxed constraint
 // is the component of g  - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( g.nonZeros() == 0 ) {  // the given vector contains no non-zero element

  if( g.size() < Index(slag_p.size()) )
   g.resize( Index(slag_p.size()) );

  g.reserve( end_p - start );

  if( indices != nullptr ) {
   for( const auto & i : *indices )
    if( ( i >= start ) && ( i < end_p ) )
     g.insert( i ) = slag_p[ i ].second->get_value();
   }
  else
   for( Index i = start ; i < end_p ; ++i )
    g.insert( i ) = slag_p[ i ].second->get_value();

  }
 else {  // the given vector contains some non-zero elements
  if( g.size() != Index(slag_p.size()) )
   throw( std::invalid_argument(
      "LagBFunction::get_linearization_coefficients: "
      "the size of the sparse vector must be equal to the number "
      "of Lagrangian multipliers" ) );

  if( indices != nullptr ) {
   for( const auto & i : *indices )
    if( ( i >= start ) && ( i < end_p ) )
     g.coeffRef( i ) = slag_p[ i ].second->get_value();
     }
    else
     for( Index i = start ; i < end_p ; ++i )
      g.coeffRef( i ) = slag_p[ i ].second->get_value();
  }

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
