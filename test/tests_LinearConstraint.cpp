/*--------------------------------------------------------------------------*/
/*--------------------- File tests_LinearConstraint.cpp --------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Unit tests for LinearConstraint, the FRowConstraint whose Function is a
 * LinearFunction built by the constraint itself.
 *
 * LinearConstraint.h is header-only and included by no file of the library,
 * so this test is also what compiles it.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractBlock.h"
#include "FakeSolver.h"
#include "LinearConstraint.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using Subset = Block::Subset;
using Range = Block::Range;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// how many times the ColVariable lists stuff among its active stuff

static long listed( const ColVariable & var , const ThinVarDepInterface * stuff )
{
 const auto & act = var.active_stuff();
 return( std::count( act.begin() , act.end() , stuff ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- TESTS ----------------------------------*/
/*--------------------------------------------------------------------------*/
/* The LinearFunction the constructor builds: its coefficients and constant
 * term, the row as its Observer and in its Variable, the value it gives,
 * and a print-out that says whether the row is feasible. */

static void test_construction( void )
{
 ColVariable x , y;
 x.set_value( 1 );
 y.set_value( 2 );
 {
  LinearConstraint lc( nullptr , 0 , 6 , { { & x , 2 } , { & y , 3 } } , -1 );
  auto f = lc.get_linear_function();
  assert( f && ( f == lc.get_function() ) && ( f->get_Observer() == & lc ) );
  assert( ( lc.get_coefficient( 0 ) == 2 ) && ( lc.get_coefficient( 1 ) == 3 ) );
  assert( ( lc.get_num_active_var() == 2 ) && ( listed( x , & lc ) == 1 ) &&
	  ( listed( y , & lc ) == 1 ) );
  assert( lc.compute() == Constraint::kOK );
  assert( ( lc.lb() == 7 ) && ( ! lc.feasible() ) && ( lc.abs_viol() == 1 ) );

  std::ostringstream out;
  out << lc;
  assert( out.str().find( "unfeasible" ) != std::string::npos );

  LinearConstraint empty;
  assert( empty.get_linear_function() && ( empty.get_num_active_var() == 0 ) );
  assert( ( empty.compute() == Constraint::kOK ) && ( empty.lb() == 0 ) &&
	  empty.feasible() );
  }
 assert( ( x.get_num_active() == 0 ) && ( y.get_num_active() == 0 ) );

 std::cout << "construction: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* set_function() is refused with std::logic_error, the Function of the row
 * stays, and the Function passed stays the caller's. */

static void test_set_function( void )
{
 ColVariable x;
 LinearConstraint lc( nullptr , 0 , 1 , { { & x , 1 } } );
 auto f = lc.get_function();
 auto other = new LinearFunction();
 bool threw = false;
 try {
  lc.set_function( other );
  }
 catch( std::logic_error & ) {
  threw = true;
  }
 assert( threw );
 assert( ( lc.get_function() == f ) && ( listed( x , & lc ) == 1 ) );
 assert( other->get_Observer() == nullptr );
 delete other;

 std::cout << "set_function: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The redirected methods of LinearFunction change the Function, reach the
 * Solver of the Block and keep the registration in the Variable, the
 * eNoMod ones included, as the row takes the removals with eNoMod on
 * itself [see FRowConstraint::remove_variable()]. */

static void test_redirected( void )
{
 auto block = new AbstractBlock();
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & list = solver->get_Modification_list();
 ColVariable x , y , z;
 {
  LinearConstraint lc( block , -1 , 1 , { { & x , 1 } , { & y , 1 } } );
  assert( list.empty() );

  lc.modify_coefficient( 0 , 5 );
  assert( ( lc.get_coefficient( 0 ) == 5 ) && ( list.size() == 1 ) );
  list.clear();
  lc.modify_coefficients( { 7 , 8 } , Range( 0 , 2 ) );
  assert( ( lc.get_coefficient( 0 ) == 7 ) && ( lc.get_coefficient( 1 ) == 8 ) );
  assert( list.size() == 1 );
  list.clear();
  lc.modify_coefficients( { 9 } , Subset( { 1 } ) , true );
  assert( ( lc.get_coefficient( 0 ) == 7 ) && ( lc.get_coefficient( 1 ) == 9 ) );
  assert( list.size() == 1 );
  list.clear();

  lc.add_variable( & z , 2 );
  assert( ( list.size() == 1 ) && ( listed( z , & lc ) == 1 ) );
  assert( lc.get_coefficient( 2 ) == 2 );
  lc.remove_variable( 2 );
  assert( listed( z , & lc ) == 0 );
  list.clear();

  lc.add_variables( { { & z , 3 } } );
  assert( ( list.size() == 1 ) && ( listed( z , & lc ) == 1 ) );
  lc.remove_variable( 2 , eNoMod );
  assert( listed( z , & lc ) == 0 );
  list.clear();

  // added with eNoMod: the row has to join the Variable all the same
  lc.add_variable( & z , 1 , eNoMod );
  assert( list.empty() && ( lc.get_num_active_var() == 3 ) );
  assert( listed( z , & lc ) == 1 );
  lc.remove_variable( 2 , eNoMod );
  assert( listed( z , & lc ) == 0 );

  lc.add_variables( { { & z , 1 } } , eNoMod );
  assert( listed( z , & lc ) == 1 );
  }
 assert( ( x.get_num_active() == 0 ) && ( y.get_num_active() == 0 ) &&
	 ( z.get_num_active() == 0 ) );

 block->unregister_Solvers( true );
 delete block;
 std::cout << "redirected methods: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

int main( void )
{
 test_construction();
 test_set_function();
 test_redirected();

 std::cout << "All tests passed!!" << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_LinearConstraint.cpp -----------------*/
/*--------------------------------------------------------------------------*/
