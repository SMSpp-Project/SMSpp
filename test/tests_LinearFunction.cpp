/** @file
 * Unit tests for LinearFunction.
 * They test stuff not already tested with Function.
 *
 * \author Niccolò Iardella \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni, Niccolò Iardella
 */

#include <random>
#include <gtest/gtest.h>

#include "LinearFunction.h"

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/

static LinearFunction::Coefficient get_random_coeff() {
 std::random_device rd;
 std::default_random_engine re( rd() );
 std::uniform_real_distribution< double > unif( -100, 100 );

 return unif( re );
}

/*--------------------------------------------------------------------------*/
/*------------------------------- TEST CASES -------------------------------*/
/*--------------------------------------------------------------------------*/
// TODO: Linearization stuff
//  hessian approximation
//  Map active
//  iterator
//  ComputeConfig
//  modify_coefficients, constant term, remove subset

TEST( LinearFunctionTest, AddsVariable ) {

 LinearFunction::Coefficient c = get_random_coeff();
 ColVariable v;
 LinearFunction fun;

 fun.add_variable( &v, c );
 ASSERT_EQ( fun.get_num_active_var(), 1 );
 ASSERT_EQ( fun.is_active( &v ), 0 );
 ASSERT_EQ( fun.get_active_var( 0 ), &v );
 ASSERT_EQ( fun.get_coefficient( 0 ), c );
 ASSERT_EQ( fun.compute( true ), LinearFunction::kOK );
 ASSERT_EQ( fun.get_value(), v.get_value() * c );
}

TEST( LinearFunctionTest, AddsVariables ) {
 LinearFunction fun;
 LinearFunction::v_coeff_pair vars( 10 );

 for( auto & p: vars ) {
  p.first = new ColVariable();
  p.second = get_random_coeff();
 }

 LinearFunction::v_coeff_pair check = vars;
 fun.add_variables( std::move( vars ) );
 ASSERT_EQ( fun.get_num_active_var(), check.size() );
 for( int i = 0; i < check.size(); ++i ) {
  auto v = check[ i ].first;
  auto c = check[ i ].second;
  ASSERT_EQ( fun.is_active( v ), i );
  ASSERT_EQ( fun.get_active_var( i ), v );
  ASSERT_EQ( fun.get_coefficient( i ), c );
 }
 ASSERT_EQ( fun.compute( true ), LinearFunction::kOK );

 LinearFunction::FunctionValue sum = 0;
 for( auto & i : check ) {
  auto var = i.first;
  auto c = i.second;
  sum += var->get_value() * c;
 }
 ASSERT_EQ( fun.get_value(), sum );
}

TEST( LinearFunctionTest, RemovesVariable ) {
 LinearFunction fun;

 ColVariable v1, v2;
 LinearFunction::Coefficient c1 = get_random_coeff();
 LinearFunction::Coefficient c2 = get_random_coeff();

 fun.add_variable( &v1, c1 );
 fun.add_variable( &v2, c2 );

 ASSERT_EQ( fun.get_num_active_var(), 2 );
 ASSERT_EQ( fun.is_active( &v1 ), 0 );
 ASSERT_EQ( fun.is_active( &v2 ), 1 );
 ASSERT_EQ( fun.get_active_var( 0 ), &v1 );
 ASSERT_EQ( fun.get_coefficient( 0 ), c1 );
 ASSERT_EQ( fun.get_active_var( 1 ), &v2 );
 ASSERT_EQ( fun.get_coefficient( 1 ), c2 );

 ASSERT_NO_THROW( fun.remove_variable( 0 ) );
 ASSERT_EQ( fun.is_active( &v1 ), Inf< LinearFunction::Index >() );

 ASSERT_EQ( fun.is_active( &v2 ), 0 );
 ASSERT_EQ( fun.get_active_var( 0 ), &v2 );
 ASSERT_EQ( fun.get_coefficient( 0 ), c2 );
 // ASSERT_EQ( fun.get_active_var( 1 ), nullptr );
}

TEST( LinearFunctionTest, RemovesVariables ) {
 LinearFunction fun;
 LinearFunction::v_coeff_pair vars( 10 );

 for( auto & p: vars ) {
  p.first = new ColVariable();
  p.second = get_random_coeff();
 }

 LinearFunction::v_coeff_pair check = vars;
 fun.add_variables( std::move( vars ) );
 ASSERT_EQ( fun.get_num_active_var(), 10 );

 LinearFunction::Range range{ 1, 9 };
 fun.remove_variables( range );
 ASSERT_EQ( fun.get_num_active_var(), 2 );

 auto v1 = check[ 0 ].first;
 auto c1 = check[ 0 ].second;

 ASSERT_EQ( fun.is_active( v1 ), 0 );
 ASSERT_EQ( fun.get_active_var( 0 ), v1 );
 ASSERT_EQ( fun.get_coefficient( 0 ), c1 );

 auto v2 = check[ 9 ].first;
 auto c2 = check[ 9 ].second;

 ASSERT_EQ( fun.is_active( v2 ), 1 );
 ASSERT_EQ( fun.get_active_var( 1 ), v2 );
 ASSERT_EQ( fun.get_coefficient( 1 ), c2 );
}

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/
int main( int argc, char ** argv ) {
 ::testing::InitGoogleTest( &argc, argv );
 return RUN_ALL_TESTS();
}
