/** @file
 * Unit tests for Variable and ColVariable.
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

#include "ColVariable.h"
#include "SMSTypedefs.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

void runAllTests()
{
 ColVariable var;

 // test FixUnfix
 assert( ! var.is_fixed() );
 var.is_fixed( true );
 assert( var.is_fixed() );
 var.is_fixed( false );
 assert( ! var.is_fixed() );

 // test IsContinuous
 assert( var.get_value() == 0 );
 assert( var.get_type() == ColVariable::kContinuous );
 assert( ! var.is_integer() );
 assert( ! var.is_positive() );
 assert( ! var.is_negative() );
 assert( ! var.is_unitary() );
 assert( var.get_lb() == -Inf< ColVariable::VarValue >() );
 assert( var.get_ub() == Inf< ColVariable::VarValue >() );

 // test SetsValue
 ColVariable::VarValue val = 42.0;
 var.set_value( val );
 assert( val == var.get_value() );

 // test IsInteger
 var.set_type( ColVariable::kInteger );
 assert( var.get_type() == ColVariable::kInteger );
 assert( var.is_integer() );
 assert( ! var.is_positive() );
 assert( ! var.is_negative() );
 assert( ! var.is_unitary() );
 assert( var.get_lb() == -Inf< ColVariable::VarValue >() );
 assert( var.get_ub() == Inf< ColVariable::VarValue >() );

 // test IsPositive
 var.set_type( ColVariable::kNonNegative );
 assert( var.get_type() == ColVariable::kNonNegative );
 assert( ! var.is_integer() );
 assert( var.is_positive() );
 assert( ! var.is_negative() );
 assert( ! var.is_unitary() );
 assert( var.get_lb() == 0 );
 assert( var.get_ub() == Inf< ColVariable::VarValue >() );

 // test IsNegative
 var.set_type( ColVariable::kNonPositive );
 assert( var.get_type() == ColVariable::kNonPositive );
 assert( ! var.is_integer() );
 assert( ! var.is_positive() );
 assert( var.is_negative() );
 assert( ! var.is_unitary() );
 assert( var.get_lb() == -Inf< ColVariable::VarValue >() );
 assert( var.get_ub() == 0 );

 // test IsUnitary
 var.set_type( ColVariable::kUnitary );
 assert( var.get_type() == ColVariable::kUnitary );
 assert( ! var.is_integer() );
 assert( ! var.is_positive() );
 assert( ! var.is_negative() );
 assert( var.is_unitary() );
 assert( var.get_lb() == -1 );
 assert( var.get_ub() == 1 );

 // test IsFeasible
 var.set_value( 42.42 );
 var.set_type( ColVariable::kInteger );
 assert( ! var.is_feasible() );

 var.set_value( 42.42 );
 var.set_type( ColVariable::kNonNegative );
 assert( var.is_feasible() );

 var.set_value( -42.42 );
 var.set_type( ColVariable::kUnitary );
 assert( ! var.is_feasible() );

 var.set_value( 0.42 );
 var.set_type( ColVariable::kUnitary );
 assert( var.is_feasible() );

 var.set_value( 42 );
 var.set_type( ColVariable::kNatural );
 assert( var.is_feasible() );
}

/*--------------------------------------------------------------------------*/

int main() {
 runAllTests();
 return( 0 );
}

/*--------------------------------------------------------------------------*/
/*--------------------- End File tests_ColVariable.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
