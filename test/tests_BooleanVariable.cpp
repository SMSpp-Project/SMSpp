/*--------------------------------------------------------------------------*/
/*----------------------- File tests_BooleanVariable.cpp -------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Unit tests for BooleanVariable, ClauseConstraint and
 * BooleanVariableSolution.
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

#include <cstdio>
#include <stdexcept>

#include "AbstractBlock.h"
#include "BooleanVariable.h"
#include "BooleanVariableSolution.h"
#include "ClauseConstraint.h"
#include "ColVariable.h"
#include "FakeSolver.h"

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using Lits = ClauseConstraint::v_Literal;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// true if calling f() throws std::invalid_argument

template< class F >
static bool throws( F f )
{
 try {
  f();
  }
 catch( std::invalid_argument & ) {
  return( true );
  }
 return( false );
 }

/*--------------------------------------------------------------------------*/

static void test_BooleanVariable( void )
{
 BooleanVariable x;
 assert( ! x.get_value() );
 x.set_value( true );
 assert( x.get_value() );
 x.set_to_default_value();
 assert( ! x.get_value() );

 assert( ! x.is_fixed() );
 x.is_fixed( true );
 assert( x.is_fixed() );
 x.is_fixed( false );
 assert( ! x.is_fixed() );

 // the copy keeps the value, not the active stuff
 x.set_value( true );
 ClauseConstraint c( nullptr , Lits{ { &x , false } } );
 assert( x.get_num_active() == 1 );
 BooleanVariable y( x );
 assert( y.get_value() && ( y.get_num_active() == 0 ) );

 // is_active() says Inf for a stuff that is not there, even one that would
 // sort before or after the ones that are
 assert( x.is_active( &c ) == 0 );
 ClauseConstraint d;
 assert( x.is_active( &d ) == Inf< BooleanVariable::Index >() );
 assert( throws( [ & ]() { x.remove_active( &d ); } ) );
 }

/*--------------------------------------------------------------------------*/

static void test_ClauseConstraint( void )
{
 BooleanVariable x , y , z;

 // x or not y or z
 auto c = new ClauseConstraint( nullptr ,
				Lits{ { &x , false } , { &y , true } ,
				      { &z , false } } );
 assert( c->get_num_active_var() == 3 );
 assert( c->get_active_var( 1 ) == &y );
 assert( c->is_active( &z ) == 2 );
 for( auto v : { &x , &y , &z } )
  assert( ( v->get_num_active() == 1 ) && ( v->is_active( c ) == 0 ) );

 // the iterators run over the BooleanVariable, in the order of the literals
 {
  BooleanVariable * expected[] = { &x , &y , &z };
  int k = 0;
  for( auto it = c->begin() ; it != c->end() ; ++it )
   assert( &( *it ) == expected[ k++ ] );
  assert( k == 3 );
  }

 // satisfied unless x = false, y = true, z = false
 for( int m = 0 ; m < 8 ; ++m ) {
  x.set_value( m & 1 );
  y.set_value( m & 2 );
  z.set_value( m & 4 );
  assert( c->compute() == ClauseConstraint::kOK );
  const bool sat = x.get_value() || ( ! y.get_value() ) || z.get_value();
  assert( c->feasible() == sat );
  assert( c->get_num_true_literals() ==
	  unsigned( x.get_value() ) + unsigned( ! y.get_value() ) +
	  unsigned( z.get_value() ) );
  }

 // a relaxed clause is always feasible
 x.set_value( false ); y.set_value( true ); z.set_value( false );
 c->compute();
 assert( ! c->feasible() );
 c->relax( true , eNoMod );
 assert( c->feasible() );
 c->relax( false , eNoMod );
 assert( ! c->feasible() );

 // a BooleanVariable twice, or null, throws and changes nothing
 assert( throws( [ & ]() { c->add_literals( Lits{ { &x , true } } ); } ) );
 assert( throws( [ & ]() {
	  c->set_literals( Lits{ { &x , true } , { &x , false } } ); } ) );
 assert( throws( [ & ]() { c->add_literals( Lits{ { nullptr , true } } ); } ) );
 assert( c->get_num_active_var() == 3 );
 assert( x.get_num_active() == 1 );

 // removing a literal detaches its BooleanVariable
 c->remove_variable( 1 , eNoMod );
 assert( c->get_num_active_var() == 2 );
 assert( y.get_num_active() == 0 );
 assert( c->is_active( &y ) == Inf< ClauseConstraint::Index >() );
 assert( throws( [ & ]() { c->remove_variable( 2 , eNoMod ); } ) );

 // replacing the literals detaches the old BooleanVariable only
 c->set_literals( Lits{ { &y , false } , { &z , true } } , eNoMod );
 assert( ( x.get_num_active() == 0 ) && ( y.get_num_active() == 1 ) &&
	 ( z.get_num_active() == 1 ) );

 // the destructor detaches all of them
 delete c;
 for( auto v : { &x , &y , &z } )
  assert( v->get_num_active() == 0 );

 // the empty clause is never satisfied
 ClauseConstraint e;
 e.compute();
 assert( ! e.feasible() );
 }

/*--------------------------------------------------------------------------*/

static void test_Modification( void )
{
 auto block = new AbstractBlock();
 auto vars = new std::vector< BooleanVariable >( 3 );
 block->add_static_variable( *vars , "x" );
 auto cls = new std::vector< ClauseConstraint >( 1 );
 block->add_static_constraint( *cls , "c" );
 auto & c = ( *cls )[ 0 ];
 c.set_literals( Lits{ { &( *vars )[ 0 ] , false } } , eNoMod );

 auto solver = new FakeSolver();
 block->register_Solver( solver );
 solver->get_Modification_list().clear();

 c.add_literals( Lits{ { &( *vars )[ 1 ] , true } } );
 c.remove_variable( 0 );
 c.set_literals( Lits{ { &( *vars )[ 2 ] , false } } );

 auto & mods = solver->get_Modification_list();
 assert( mods.size() == 3 );
 int expected[] = { ClauseConstraintMod::eLiteralsAdded ,
		    ClauseConstraintMod::eLiteralsRemoved ,
		    ClauseConstraintMod::eLiteralsChanged };
 int k = 0;
 for( auto & mod : mods ) {
  auto cmod = std::dynamic_pointer_cast< ClauseConstraintMod >( mod );
  assert( cmod && ( cmod->constraint() == &c ) &&
	  ( cmod->type() == expected[ k++ ] ) );
  }

 // eNoMod issues nothing
 mods.clear();
 c.add_literals( Lits{ { &( *vars )[ 0 ] , false } } , eNoMod );
 assert( mods.empty() );

 block->unregister_Solvers( true );
 delete block;
 }

/*--------------------------------------------------------------------------*/

static void test_BooleanVariableSolution( void )
{
 auto block = new AbstractBlock();
 auto vars = new std::vector< BooleanVariable >( 4 );
 block->add_static_variable( *vars , "x" );
 auto more = new std::vector< BooleanVariable >( 2 );
 block->add_static_variable( *more , "y" );

 // read
 ( *vars )[ 1 ].set_value( true );
 ( *more )[ 0 ].set_value( true );
 BooleanVariableSolution s1;
 s1.read( block );
 const auto & v1 = s1.get_static_variable_values();
 assert( ( v1.size() == 2 ) && ( v1[ 0 ].size() == 4 ) &&
	 ( v1[ 1 ].size() == 2 ) );
 assert( ( v1[ 0 ][ 0 ] == 0 ) && ( v1[ 0 ][ 1 ] == 1 ) &&
	 ( v1[ 1 ][ 0 ] == 1 ) && ( v1[ 1 ][ 1 ] == 0 ) );

 // write
 for( auto & x : *vars ) x.set_value( false );
 for( auto & x : *more ) x.set_value( false );
 s1.write( block );
 assert( ( *vars )[ 1 ].get_value() && ( *more )[ 0 ].get_value() );
 assert( ! ( ( *vars )[ 0 ].get_value() || ( *vars )[ 2 ].get_value() ||
	     ( *vars )[ 3 ].get_value() || ( *more )[ 1 ].get_value() ) );

 // a convex combination gives frequencies, and write() rounds at 1/2
 ( *vars )[ 1 ].set_value( false );
 ( *vars )[ 2 ].set_value( true );
 BooleanVariableSolution s2;
 s2.read( block );
 auto mix = s1.scale( 0.25 );
 mix->sum( &s2 , 0.75 );
 const auto & vm = mix->get_static_variable_values();
 assert( ( vm[ 0 ][ 1 ] == 0.25 ) && ( vm[ 0 ][ 2 ] == 0.75 ) &&
	 ( vm[ 1 ][ 0 ] == 1 ) );
 mix->write( block );
 assert( ( ! ( *vars )[ 1 ].get_value() ) && ( *vars )[ 2 ].get_value() &&
	 ( *more )[ 0 ].get_value() );
 delete mix;

 // serialize and deserialize
 const char * file = "BooleanVariableSolution_test.nc4";
 {
  netCDF::NcFile f( file , netCDF::NcFile::replace );
  auto g = f.addGroup( "Solution" );
  s1.serialize( g );
  }
 {
  netCDF::NcFile f( file , netCDF::NcFile::read );
  BooleanVariableSolution s3;
  s3.deserialize( f.getGroup( "Solution" ) );
  assert( s3.get_static_variable_values() == s1.get_static_variable_values() );
  }
 std::remove( file );

 // a group of other Variable is not a solution of this kind
 auto cols = new std::vector< ColVariable >( 1 );
 block->add_static_variable( *cols , "z" );
 BooleanVariableSolution s4;
 bool threw = false;
 try {
  s4.read( block );
  }
 catch( std::logic_error & ) {
  threw = true;
  }
 assert( threw );

 delete block;
 }

/*--------------------------------------------------------------------------*/

void runAllTests()
{
 test_BooleanVariable();
 test_ClauseConstraint();
 test_Modification();
 test_BooleanVariableSolution();
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 runAllTests();
 std::cout << "BooleanVariable_test: all tests passed" << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_BooleanVariable.cpp ------------------*/
/*--------------------------------------------------------------------------*/
