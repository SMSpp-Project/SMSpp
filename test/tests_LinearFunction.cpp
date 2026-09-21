/** @file
 * Unit tests for LinearFunction.
 * They test stuff not already tested with Function.
 *
 * \author Niccolo' Iardella \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Niccolo' Iardella, Donato Meoli
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <random>

#include "LinearFunction.h"

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

static LinearFunction::Coefficient get_random_coeff()
{
 std::random_device rd;
 std::default_random_engine re( rd() );
 std::uniform_real_distribution< double > unif( -100 , 100 );

 return( unif( re ) );
}

/*--------------------------------------------------------------------------*/

// TODO: Linearization stuff
//  hessian approximation
//  Map active
//  iterator
//  ComputeConfig
//  modify_coefficients, constant term, remove subset

void runAllTests()
{
 // test AddsVariable
 LinearFunction add_fun;
 ColVariable v;
 LinearFunction::Coefficient c = get_random_coeff();

 add_fun.add_variable( &v , c );
 assert( add_fun.get_num_active_var() == 1 );
 assert( add_fun.is_active( &v ) == 0 );
 assert( add_fun.get_active_var( 0 ) == &v );
 assert( add_fun.get_coefficient( 0 ) == c );
 assert( add_fun.compute( true ) == LinearFunction::kOK );
 assert( add_fun.get_value() == v.get_value() * c );

 // test AddsVariables
 LinearFunction::v_coeff_pair add_vars( 10 );
 LinearFunction adds_fun;

 for( auto & p : add_vars ) {
  p.first = new ColVariable();
  p.second = get_random_coeff();
 }

 LinearFunction::v_coeff_pair add_check = add_vars;
 adds_fun.add_variables( std::move( add_vars ) );
 assert( adds_fun.get_num_active_var() == add_check.size() );
 for( int i = 0 ; i < add_check.size() ; ++i ) {
  assert( adds_fun.is_active( add_check[ i ].first ) == i );
  assert( adds_fun.get_active_var( i ) == add_check[ i ].first );
  assert( adds_fun.get_coefficient( i ) == add_check[ i ].second );
 }
 assert( adds_fun.compute( true ) == LinearFunction::kOK );

 LinearFunction::FunctionValue sum = 0;
 for( auto & i : add_check )
  sum += i.first->get_value() * i.second;
 assert( adds_fun.get_value() == sum );

 // test RemovesVariable
 LinearFunction del_fun;
 ColVariable v1 , v2;
 LinearFunction::Coefficient c1 = get_random_coeff();
 LinearFunction::Coefficient c2 = get_random_coeff();

 del_fun.add_variable( &v1 , c1 );
 del_fun.add_variable( &v2 , c2 );

 assert( del_fun.get_num_active_var() == 2 );
 assert( del_fun.is_active( &v1 ) == 0 );
 assert( del_fun.is_active( &v2 ) == 1 );
 assert( del_fun.get_active_var( 0 ) == &v1 );
 assert( del_fun.get_coefficient( 0 ) == c1 );
 assert( del_fun.get_active_var( 1 ) == &v2 );
 assert( del_fun.get_coefficient( 1 ) == c2 );

 del_fun.remove_variable( 0 );
 assert( del_fun.is_active( &v1 ) == Inf< LinearFunction::Index >() );

 assert( del_fun.is_active( &v2 ) == 0 );
 assert( del_fun.get_active_var( 0 ) == &v2 );
 assert( del_fun.get_coefficient( 0 ) == c2 );
 // assert( del_fun.get_active_var( 1 ) == nullptr );

 // test RemovesVariables
 LinearFunction::v_coeff_pair del_vars( 10 );
 LinearFunction dels_fun;

 for( auto & p : del_vars ) {
  p.first = new ColVariable();
  p.second = get_random_coeff();
 }

 LinearFunction::v_coeff_pair del_check = del_vars;
 dels_fun.add_variables( std::move( del_vars ) );
 assert( dels_fun.get_num_active_var() == 10 );

 LinearFunction::Range range{ 1 , 9 };
 dels_fun.remove_variables( range );
 assert( dels_fun.get_num_active_var() == 2 );

 assert( dels_fun.is_active( del_check[ 0 ].first ) == 0 );
 assert( dels_fun.get_active_var( 0 ) == del_check[ 0 ].first );
 assert( dels_fun.get_coefficient( 0 ) == del_check[ 0 ].second );

 assert( dels_fun.is_active( del_check[ 9 ].first ) == 1 );
 assert( dels_fun.get_active_var( 1 ) == del_check[ 9 ].first );
 assert( dels_fun.get_coefficient( 1 ) == del_check[ 9 ].second );
}

/*--------------------------------------------------------------------------*/
/* What the tests above never touch: a LinearFunction with nothing in it, the
 * constant term, a coefficient that is zero, and the three ways of removing
 * Variable at their own edges, one of which means the opposite of what it
 * looks like. */

static void test_edge_cases( void )
{
 // ---- a function with no Variable at all ----------------------------

 LinearFunction empty;
 assert( empty.get_num_active_var() == 0 );
 assert( empty.get_constant_term() == 0 );
 assert( empty.compute( true ) == LinearFunction::kOK );
 assert( empty.get_value() == 0 );

 // a Variable that was never added is not active, and answering with Inf
 // is what tells it apart from the one that sits at index 0
 ColVariable stranger;
 assert( empty.is_active( & stranger ) ==
	 Inf< LinearFunction::Index >() );

 // ---- the constant term ---------------------------------------------
 // it is the value of a function of no Variable, and it is added to the
 // value of one that has some: an affine function, not a linear one

 empty.set_constant_term( 3.5 );
 assert( empty.get_constant_term() == 3.5 );
 assert( empty.compute( true ) == LinearFunction::kOK );
 assert( empty.get_value() == 3.5 );

 ColVariable v;
 v.set_value( 2 );
 empty.add_variable( & v , 4 );
 assert( empty.compute( true ) == LinearFunction::kOK );
 assert( empty.get_value() == 3.5 + 8 );

 // ---- a coefficient of zero -----------------------------------------
 // the Variable is active all the same: it is in the function, it just
 // does not move its value, and whoever walks the pairs sees it

 LinearFunction zero;
 ColVariable z;
 z.set_value( 7 );
 zero.add_variable( & z , 0 );
 assert( zero.get_num_active_var() == 1 );
 assert( zero.is_active( & z ) == 0 );
 assert( zero.get_coefficient( 0 ) == 0 );
 assert( zero.compute( true ) == LinearFunction::kOK );
 assert( zero.get_value() == 0 );

 // and a coefficient can be changed to something that does move it
 zero.modify_coefficient( 0 , 2 );
 assert( zero.get_coefficient( 0 ) == 2 );
 assert( zero.compute( true ) == LinearFunction::kOK );
 assert( zero.get_value() == 14 );

 // ---- removing by range ---------------------------------------------

 auto ten = []( LinearFunction & f , std::vector< ColVariable > & vars ) {
  LinearFunction::v_coeff_pair p( vars.size() );
  for( LinearFunction::Index i = 0 ; i < vars.size() ; ++i )
   p[ i ] = { & vars[ i ] , double( i + 1 ) };
  f.add_variables( std::move( p ) );
  };

 std::vector< ColVariable > vars( 10 );

 {                              // an empty range removes nothing
  LinearFunction f;
  ten( f , vars );
  f.remove_variables( LinearFunction::Range{ 4 , 4 } );
  assert( f.get_num_active_var() == 10 );
 }
 {                              // a range past the end stops at the end
  LinearFunction f;
  ten( f , vars );
  f.remove_variables( LinearFunction::Range{ 8 , 1000 } );
  assert( f.get_num_active_var() == 8 );
  assert( f.get_active_var( 7 ) == & vars[ 7 ] );
 }
 {                              // and one that covers it all empties it
  LinearFunction f;
  ten( f , vars );
  f.remove_variables( LinearFunction::Range{ 0 , 10 } );
  assert( f.get_num_active_var() == 0 );
  assert( f.compute( true ) == LinearFunction::kOK );
  assert( f.get_value() == 0 );
 }

 // ---- removing by subset --------------------------------------------

 {                              // the last one, which leaves it empty
  LinearFunction f;
  ColVariable only;
  f.add_variable( & only , 1 );
  f.remove_variable( 0 );
  assert( f.get_num_active_var() == 0 );
  assert( f.is_active( & only ) == Inf< LinearFunction::Index >() );
 }
 {                              // an unordered subset, said to be unordered
  LinearFunction f;
  ten( f , vars );
  f.remove_variables( LinearFunction::Subset{ 7 , 1 , 4 } , false );
  assert( f.get_num_active_var() == 7 );
  assert( f.is_active( & vars[ 1 ] ) == Inf< LinearFunction::Index >() );
  assert( f.is_active( & vars[ 4 ] ) == Inf< LinearFunction::Index >() );
  assert( f.is_active( & vars[ 7 ] ) == Inf< LinearFunction::Index >() );
  assert( f.is_active( & vars[ 0 ] ) == 0 );
 }
 {
  /* ⚠️ An EMPTY subset removes EVERY Variable, which is the opposite of
   * what an empty range does and of what the word suggests: it is written
   * in the comments of remove_variables( Subset ), and a caller that
   * builds the subset and finds it empty has to know it. */

  LinearFunction f;
  ten( f , vars );
  f.remove_variables( LinearFunction::Subset{} );
  assert( f.get_num_active_var() == 0 );
 }
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 runAllTests();
 test_edge_cases();
 return( 0 );
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_LinearFunction.cpp -------------------*/
/*--------------------------------------------------------------------------*/
