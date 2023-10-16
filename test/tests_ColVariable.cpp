/** @file
 * Unit tests for Variable and ColVariable.
 *
 * \author Niccolo' Iardella \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Niccolo' Iardella
 */

#include <gtest/gtest.h>

#include "ColVariable.h"
#include "SMSTypedefs.h"

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------- TEST CASES -------------------------------*/
/*--------------------------------------------------------------------------*/

TEST( VariableTest, FixUnfix ) {
 ColVariable var;
 ASSERT_FALSE( var.is_fixed() );
 var.is_fixed( true );
 ASSERT_TRUE( var.is_fixed() );
 var.is_fixed( false );
 ASSERT_FALSE( var.is_fixed() );
}

TEST( ColVariableTest, SetsValue ) {
 ColVariable var;

 ColVariable::VarValue val = 42;
 var.set_value( val );
 ASSERT_EQ( val, var.get_value() );
}

TEST( ColVariableTest, IsContinuous ) {
 ColVariable var;
 ASSERT_EQ( var.get_value(), 0 );
 ASSERT_EQ( var.get_type(), ColVariable::kContinuous );
 ASSERT_FALSE( var.is_integer() );
 ASSERT_FALSE( var.is_positive() );
 ASSERT_FALSE( var.is_negative() );
 ASSERT_FALSE( var.is_unitary() );
 ASSERT_EQ( var.get_lb(), -Inf< ColVariable::VarValue >() );
 ASSERT_EQ( var.get_ub(), Inf< ColVariable::VarValue >() );

}

TEST( ColVariableTest, IsInteger ) {
 ColVariable var;

 var.set_type( ColVariable::kInteger );
 ASSERT_EQ( var.get_type(), ColVariable::kInteger );
 ASSERT_TRUE( var.is_integer() );
 ASSERT_FALSE( var.is_positive() );
 ASSERT_FALSE( var.is_negative() );
 ASSERT_FALSE( var.is_unitary() );
 ASSERT_EQ( var.get_lb(), -Inf< ColVariable::VarValue >() );
 ASSERT_EQ( var.get_ub(), Inf< ColVariable::VarValue >() );
}

TEST( ColVariableTest, IsPositive ) {
 ColVariable var;

 var.set_type( ColVariable::kNonNegative );
 ASSERT_EQ( var.get_type(), ColVariable::kNonNegative );
 ASSERT_FALSE( var.is_integer() );
 ASSERT_TRUE( var.is_positive() );
 ASSERT_FALSE( var.is_negative() );
 ASSERT_FALSE( var.is_unitary() );
 ASSERT_EQ( var.get_lb(), 0 );
 ASSERT_EQ( var.get_ub(), Inf< ColVariable::VarValue >() );
}

TEST( ColVariableTest, IsNegative ) {
 ColVariable var;

 var.set_type( ColVariable::kNonPositive );
 ASSERT_EQ( var.get_type(), ColVariable::kNonPositive );
 ASSERT_FALSE( var.is_integer() );
 ASSERT_FALSE( var.is_positive() );
 ASSERT_TRUE( var.is_negative() );
 ASSERT_FALSE( var.is_unitary() );
 ASSERT_EQ( var.get_lb(), -Inf< ColVariable::VarValue >() );
 ASSERT_EQ( var.get_ub(), 0 );
}

TEST( ColVariableTest, IsUnitary) {
 ColVariable var;

 var.set_type( ColVariable::kUnitary );
 ASSERT_EQ( var.get_type(), ColVariable::kUnitary );
 ASSERT_FALSE( var.is_integer() );
 ASSERT_FALSE( var.is_positive() );
 ASSERT_FALSE( var.is_negative() );
 ASSERT_TRUE( var.is_unitary() );
 ASSERT_EQ( var.get_lb(), -1 );
 ASSERT_EQ( var.get_ub(), 1 );
}

TEST( ColVariableTest, IsFeasible) {
 ColVariable var;

 var.set_value(42.42);
 var.set_type( ColVariable::kInteger );
 ASSERT_FALSE(var.is_feasible());

 var.set_value(42.42);
 var.set_type( ColVariable::kNonNegative );
 ASSERT_TRUE(var.is_feasible());

 var.set_value(-42.42);
 var.set_type( ColVariable::kUnitary );
 ASSERT_FALSE(var.is_feasible());

 var.set_value(0.42);
 var.set_type( ColVariable::kUnitary );
 ASSERT_TRUE(var.is_feasible());

 var.set_value(42);
 var.set_type( ColVariable::kNatural );
 ASSERT_TRUE(var.is_feasible());
}

/*--------------------------------------------------------------------------*/

int main( int argc, char ** argv ) {
 ::testing::InitGoogleTest( &argc, argv );
 return( RUN_ALL_TESTS() );
}

/*--------------------------------------------------------------------------*/
/*--------------------- End File tests_ColVariable.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
