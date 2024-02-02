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

int main() {
 runAllTests();
 return( 0 );
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_LinearFunction.cpp -------------------*/
/*--------------------------------------------------------------------------*/
