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

#include <cmath>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

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
/* Every one of the sixteen types, and the values at the edge of each. The
 * tests above try six of them with a value well inside or well outside the
 * set, which says nothing about what happens at the bounds themselves, and
 * the ten that are left are the "weird" combinations the class warns about,
 * which are the ones a caller gets wrong. Rather than writing down the two
 * bounds of each type, which would only repeat the code, what is checked
 * here is what has to hold of any of them. */

static void test_every_type( void )
{
 using VV = ColVariable::VarValue;
 const VV inf = Inf< VV >();

 /* Nine values that straddle every bound any of the types has: each of -1,
  * 0 and 1 is a bound of one of them, so each is tried on the bound, half a
  * unit inside it and half a unit outside, and one unit past either end. */

 static const VV probe[] = { -2 , -1.5 , -1 , -0.5 , 0 , 0.5 , 1 , 1.5 , 2 };

 for( int t = 0 ; t < ColVariable::ColVarLastType ; ++t ) {
  ColVariable var;
  var.set_type( ColVariable::col_var_type( t ) );

  const auto lb = var.get_lb();
  const auto ub = var.get_ub();

  // a set that is empty would be a type nobody can satisfy
  assert( lb <= ub );

  // an integer type has integer bounds, or the bound itself would be a
  // value the type forbids
  if( var.is_integer() ) {
   if( lb > - inf )
    assert( lb == std::floor( lb ) );
   if( ub < inf )
    assert( ub == std::floor( ub ) );
   }

  // is_positive() and is_negative() say where the set sits, and both hold
  // of the types that are the single point 0
  assert( var.is_positive() == ( lb >= 0 ) );
  assert( var.is_negative() == ( ub <= 0 ) );

  /* And the set is exactly what the two bounds and the integrality say:
   * this is the one thing every caller assumes of a type, it is what the
   * tests above check of six of the sixteen with one value each, and it is
   * what has to hold at the bounds themselves. */

  for( auto x : probe ) {
   var.set_value( x );
   const bool in = ( x >= lb ) && ( x <= ub ) &&
                   ( ( ! var.is_integer() ) || ( x == std::floor( x ) ) );
   assert( var.is_feasible() == in );
   }
  }
 }

/*--------------------------------------------------------------------------*/
/* The types whose set is a single point, and the two that are a point but
 * carry a range: these are the ones where an off-by-one in the bounds does
 * not show, both bounds being the same number. */

static void test_single_point_types( void )
{
 for( auto t : { ColVariable::kZeroReal , ColVariable::kZeroInteger ,
		 ColVariable::kZeroRealU , ColVariable::kZeroIntU } ) {
  ColVariable var;
  var.set_type( t );
  assert( var.get_lb() == 0 );
  assert( var.get_ub() == 0 );
  var.set_value( 0 );
  assert( var.is_feasible() );
  var.set_value( 1e-12 );
  assert( ! var.is_feasible() );
  }

 // the binary one, which is the type most models are made of
 ColVariable bin;
 bin.set_type( ColVariable::kBinary );
 assert( bin.get_lb() == 0 );
 assert( bin.get_ub() == 1 );
 assert( bin.is_integer() );
 for( auto v : { 0.0 , 1.0 } ) {
  bin.set_value( v );
  assert( bin.is_feasible() );
  }
 for( auto v : { -1.0 , 0.5 , 2.0 } ) {
  bin.set_value( v );
  assert( ! bin.is_feasible() );
  }
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 runAllTests();
 test_every_type();
 test_single_point_types();
 return( 0 );
}

/*--------------------------------------------------------------------------*/
/*--------------------- End File tests_ColVariable.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
