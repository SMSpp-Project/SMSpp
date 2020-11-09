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
/*------------------------------- TEST CASES -------------------------------*/
/*--------------------------------------------------------------------------*/

TEST( LinearFunctionTest, AddVariable ) {

 std::random_device rd;
 std::default_random_engine re( rd() );
 std::uniform_real_distribution< double > unif( -100, 100 );

 LinearFunction::Coefficient c1 = unif( re );
 ColVariable v1;
 LinearFunction fun;

 fun.add_variable( &v1, c1 );
 EXPECT_EQ( fun.get_active_var( 0 ), &v1 );
 EXPECT_EQ( fun.compute( true ), LinearFunction::kOK );
 EXPECT_EQ( fun.get_value(), v1.get_value() * c1 );
}


/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/
int main( int argc, char ** argv ) {
 ::testing::InitGoogleTest( &argc, argv );
 return RUN_ALL_TESTS();
}
