/*--------------------------------------------------------------------------*/
/*------------------------ File tests_Constraint.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Unit tests for the row Constraint of the core: FRowConstraint, with a
 * LinearFunction, and the OneVarConstraint family (BoxConstraint,
 * LB0Constraint, UB0Constraint, LBConstraint, UBConstraint, NNConstraint,
 * NPConstraint and ZOConstraint). LinearConstraint is in
 * tests_LinearConstraint.cpp, which is built by a test of its own.
 *
 * The tests go over what RowConstraint documents for the value of a row,
 * i.e., feasible(), abs_viol() and rel_viol() on rows of every sense, with
 * infinite sides and infinite values, over the Modification each setter
 * issues to a Solver of the Block and the ones it must not issue, and over
 * the registration of the Constraint in its Variable, which has to follow
 * every change of the Variable of the row (a new Function, a Variable added
 * to or removed from it, a new ColVariable of a OneVarConstraint).
 *
 * A few checks are made with expect() rather than assert(), so that one
 * run prints all the values of a table that are wrong rather than the
 * first one only.
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
#include "FRowConstraint.h"
#include "LinearFunction.h"
#include "OneVarConstraint.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <vector>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using Index = Block::Index;
using Subset = Block::Subset;
using Range = Block::Range;

static constexpr double INF = RowConstraint::RHSINF;
static constexpr Index NOTACTIVE = Inf< Index >();

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

static int n_failed = 0;  ///< number of failed expect()

/// records and prints a failed check without stopping the test

static void expect( bool ok , const std::string & what )
{
 if( ok )
  return;
 ++n_failed;
 std::cout << "FAILED: " << what << std::endl;
 }

/*--------------------------------------------------------------------------*/
/// true if calling f() throws E

template< class E , class F >
static bool throws( F f )
{
 try {
  f();
  }
 catch( E & ) {
  return( true );
  }
 return( false );
 }

/*--------------------------------------------------------------------------*/
/// how many times the ColVariable lists stuff among its active stuff
/** Scans the list rather than asking ColVariable::is_active(), which is
 * itself under test in test_variable_is_active(). */

static long listed( const ColVariable & var , const ThinVarDepInterface * stuff )
{
 const auto & act = var.active_stuff();
 return( std::count( act.begin() , act.end() , stuff ) );
 }

/*--------------------------------------------------------------------------*/
/// a and b are the same number, the infinite ones included

static bool same( double a , double b )
{
 if( std::isinf( a ) || std::isinf( b ) )
  return( a == b );
 return( std::abs( a - b ) <= 1e-12 * std::max( 1.0 , std::abs( b ) ) );
 }

/*--------------------------------------------------------------------------*/
/// the number of Variable the v_iterator of the Constraint go through

static Index iterated( Constraint & c )
{
 auto it = c.v_begin();
 auto end = c.v_end();
 if( ! it )
  return( end ? NOTACTIVE : 0 );
 Index n = 0;
 for( ; *it != *end ; ++( *it ) )
  ++n;
 delete it;
 delete end;
 return( n );
 }

/*--------------------------------------------------------------------------*/
/// the RowConstraintMod the Solver got, which are then cleared

static std::vector< std::shared_ptr< RowConstraintMod > > row_mods(
							 FakeSolver * solver )
{
 std::vector< std::shared_ptr< RowConstraintMod > > rv;
 for( auto & mod : solver->get_Modification_list() ) {
  auto rmod = std::dynamic_pointer_cast< RowConstraintMod >( mod );
  assert( rmod );
  rv.push_back( rmod );
  }
 solver->get_Modification_list().clear();
 return( rv );
 }

/*--------------------------------------------------------------------------*/
/// a LinearFunction that counts how many of its kind are alive

class CountedFunction : public LinearFunction {
 public:
 static int alive;
 explicit CountedFunction( v_coeff_pair && vars = {} )
  : LinearFunction( std::move( vars ) ) { ++alive; }
 ~CountedFunction() override { --alive; }
 };

int CountedFunction::alive = 0;

/*--------------------------------------------------------------------------*/
/*------------------------- TESTS OF FRowConstraint ------------------------*/
/*--------------------------------------------------------------------------*/
/* The value of a row 2 x + 3 y + 1 at x = 1, y = 2, i.e., 9, against rows
 * of every sense: feasible(), abs_viol() and rel_viol() as RowConstraint
 * documents them, the relative violation being divided by
 * max( 1 , | bound | ), an infinite side giving no violation, and the
 * violation of a row with lhs > rhs being the larger of the two. */

static void test_row_values( void )
{
 ColVariable x , y;
 x.set_value( 1 );
 y.set_value( 2 );

 auto check = [ & ]( double lhs , double rhs , bool feas , double abs ,
		     double rel ) {
  FRowConstraint r( nullptr , lhs , rhs ,
		    new LinearFunction( { { & x , 2 } , { & y , 3 } } , 1 ) );
  assert( r.compute() == Constraint::kOK );
  assert( ( r.lb() == 9 ) && ( r.ub() == 9 ) );
  assert( r.feasible() == feas );
  assert( same( r.abs_viol() , abs ) );
  assert( same( r.rel_viol() , rel ) );
  };

 check( -INF , 10 , true , 0 , 0 );        // <= satisfied
 check( -INF , 4 , false , 5 , 5.0 / 4 );  // <= violated
 check( 5 , INF , true , 0 , 0 );          // >= satisfied
 check( 20 , INF , false , 11 , 11.0 / 20 );  // >= violated
 check( 9 , 9 , true , 0 , 0 );            // = satisfied exactly
 check( 8 , 8 , false , 1 , 1.0 / 8 );     // = violated
 check( 10 , 12 , false , 1 , 1.0 / 10 );  // ranged, below
 check( 0 , 0.5 , false , 8.5 , 8.5 );     // | rhs | < 1 scales by 1
 check( -INF , INF , true , 0 , 0 );       // free row
 check( 12 , 4 , false , 5 , 5.0 / 4 );    // lhs > rhs: both violated

 // a changed Variable is seen after compute()
 FRowConstraint r( nullptr , -INF , 10 ,
		   new LinearFunction( { { & x , 2 } , { & y , 3 } } , 1 ) );
 r.compute();
 assert( r.feasible() );
 x.set_value( 3 );
 assert( r.compute() == Constraint::kOK );
 assert( ( r.lb() == 13 ) && ( ! r.feasible() ) && ( r.abs_viol() == 3 ) );

 std::cout << "row values: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* Infinite values of the Function: +INF violates a finite rhs by INF and
 * does not violate a finite lhs, -INF the other way round, and a free row
 * is never violated, whatever the value. */

static void test_row_infinite_values( void )
{
 ColVariable x;
 FRowConstraint le( nullptr , -INF , 10 , new LinearFunction( { { & x , 1 } } ) );
 FRowConstraint ge( nullptr , 5 , INF , new LinearFunction( { { & x , 1 } } ) );
 FRowConstraint fr( nullptr , -INF , INF , new LinearFunction( { { & x , 1 } } ) );

 x.set_value( INF );
 le.compute(); ge.compute(); fr.compute();
 assert( ( ! le.feasible() ) && ( le.abs_viol() == INF ) &&
	 ( le.rel_viol() == INF ) );
 assert( ge.feasible() && ( ge.abs_viol() == 0 ) && ( ge.rel_viol() == 0 ) );
 assert( fr.feasible() && ( fr.abs_viol() == 0 ) && ( fr.rel_viol() == 0 ) );

 x.set_value( -INF );
 le.compute(); ge.compute(); fr.compute();
 assert( le.feasible() && ( le.abs_viol() == 0 ) && ( le.rel_viol() == 0 ) );
 assert( ( ! ge.feasible() ) && ( ge.abs_viol() == INF ) &&
	 ( ge.rel_viol() == INF ) );
 assert( fr.feasible() && ( fr.abs_viol() == 0 ) && ( fr.rel_viol() == 0 ) );

 std::cout << "row infinite values: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A row with no Function: compute() is kUnEval, lb() = -INF and ub() = INF,
 * so any finite side is violated by INF while a free row is feasible; it
 * has no active Variable and its v_iterator are nullptr. */

static void test_row_no_function( void )
{
 FRowConstraint r( nullptr , 0 , 1 );
 assert( r.get_function() == nullptr );
 assert( r.compute() == Constraint::kUnEval );
 assert( ( r.lb() == -INF ) && ( r.ub() == INF ) );
 assert( ! r.feasible() );
 assert( ( r.abs_viol() == INF ) && ( r.rel_viol() == INF ) );
 assert( ! RowConstraint::is_feasible( r , 1e300 ) );

 FRowConstraint free_row( nullptr , -INF , INF );
 assert( free_row.feasible() && ( free_row.abs_viol() == 0 ) &&
	 ( free_row.rel_viol() == 0 ) );

 ColVariable x;
 assert( r.get_num_active_var() == 0 );
 assert( r.is_active( & x ) == NOTACTIVE );
 assert( r.get_active_var( 0 ) == nullptr );
 assert( ( r.v_begin() == nullptr ) && ( r.v_end() == nullptr ) );

 // removing from a row with no Function does nothing
 r.remove_variable( 0 );
 r.remove_variables( Range( 0 , 1 ) );
 r.remove_variables( Subset() );
 assert( r.get_num_active_var() == 0 );

 std::cout << "row with no Function: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* RowConstraint::is_feasible(): the tolerance applies to the relative or to
 * the absolute violation as asked, a relaxed row is always feasible, and a
 * collection is feasible if all its rows are. */

static void test_row_is_feasible( void )
{
 ColVariable x;
 x.set_value( 9 );
 FRowConstraint r( nullptr , 8 , 8 , new LinearFunction( { { & x , 1 } } ) );

 // abs_viol = 1, rel_viol = 1 / 8
 assert( RowConstraint::is_feasible( r , 0.2 , true ) );
 assert( ! RowConstraint::is_feasible( r , 0.2 , false ) );
 assert( RowConstraint::is_feasible( r , 1 , false ) );
 assert( ! RowConstraint::is_feasible( r ) );

 r.relax( true , eNoMod );
 assert( r.is_relaxed() && RowConstraint::is_feasible( r ) );
 r.relax( false , eNoMod );
 assert( ! RowConstraint::is_feasible( r ) );

 std::list< FRowConstraint > rows( 2 );
 assert( ! RowConstraint::is_feasible( rows ) );  // no Function: kUnEval
 for( auto & row : rows ) {
  row.set_function( new LinearFunction( { { & x , 1 } } ) , eNoMod );
  row.set_both( 9 , eNoMod );
  }
 assert( RowConstraint::is_feasible( rows ) );
 rows.back().set_rhs( 8.5 , eNoMod );
 assert( ! RowConstraint::is_feasible( rows ) );

 std::cout << "is_feasible: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* set_lhs(), set_rhs() and set_both() issue a RowConstraintMod of type
 * eChgLHS, eChgRHS and eChgBTS to the Solver of the Block, with
 * concerns_Block() as the ModParam says; a value equal to the current one
 * issues nothing, eNoMod changes the value silently, and a row with no
 * Block, or whose Block has no Solver, changes the value all the same. */

static void test_row_bound_mods( void )
{
 auto block = new AbstractBlock();
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 ColVariable x;
 {
  FRowConstraint r( block , 0 , 10 , new LinearFunction( { { & x , 1 } } ) );
  assert( solver->get_Modification_list().empty() );  // the constructor

  r.set_lhs( 1 );
  r.set_rhs( 5 , eNoBlck );
  r.set_both( 3 );
  auto mods = row_mods( solver );
  assert( mods.size() == 3 );
  int expected[] = { RowConstraintMod::eChgLHS , RowConstraintMod::eChgRHS ,
		     RowConstraintMod::eChgBTS };
  for( int k = 0 ; k < 3 ; ++k )
   assert( ( mods[ k ]->constraint() == & r ) &&
	   ( mods[ k ]->type() == expected[ k ] ) &&
	   ( mods[ k ]->concerns_Block() == ( k != 1 ) ) );
  assert( ( r.get_lhs() == 3 ) && ( r.get_rhs() == 3 ) );

  // the same values issue nothing
  r.set_both( 3 );
  r.set_lhs( 3 );
  r.set_rhs( 3 );
  assert( row_mods( solver ).empty() );

  // eNoMod changes silently
  r.set_lhs( -INF , eNoMod );
  r.set_rhs( INF , eNoMod );
  r.set_both( 7 , eNoMod );
  assert( row_mods( solver ).empty() );
  assert( ( r.get_lhs() == 7 ) && ( r.get_rhs() == 7 ) );

  // an infinite side is a value as any other
  r.set_rhs( INF );
  mods = row_mods( solver );
  assert( ( mods.size() == 1 ) && ( mods[ 0 ]->type() ==
				    RowConstraintMod::eChgRHS ) );
  assert( r.get_rhs() == INF );

  // the Modification of a channel arrive together when it is closed
  auto chnl = block->open_channel();
  auto par = Observer::make_par( eModBlck , chnl );
  r.set_lhs( 2 , par );
  r.set_rhs( 4 , par );
  assert( solver->get_Modification_list().empty() );
  block->close_channel( chnl );
  auto & list = solver->get_Modification_list();
  assert( list.size() == 1 );
  auto gmod = std::dynamic_pointer_cast< GroupModification >( list.front() );
  assert( gmod && ( gmod->sub_Modifications().size() == 2 ) );
  list.clear();
  }

 // no Block, and a Block with no Solver
 {
  FRowConstraint r( nullptr , 0 , 1 );
  r.set_lhs( -1 );
  r.set_rhs( 2 );
  assert( ( r.get_lhs() == -1 ) && ( r.get_rhs() == 2 ) );
  r.set_both( 5 );
  assert( ( r.get_lhs() == 5 ) && ( r.get_rhs() == 5 ) );

  auto quiet = new AbstractBlock();
  FRowConstraint q( quiet , 0 , 1 );
  q.set_lhs( -1 );
  q.set_both( 2 , eNoBlck );
  assert( ( q.get_lhs() == 2 ) && ( q.get_rhs() == 2 ) );
  q.set_Block( nullptr );
  delete quiet;
  }

 block->unregister_Solvers( true );
 delete block;
 std::cout << "row bound Modification: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* set_function(): the row stops observing the old Function and leaves its
 * Variable, observes the new one and joins its Variable (once each, also
 * the ones the two Function share), issues a FRowConstraintMod of type
 * eFunctionChanged, and deletes the old Function unless told not to; the
 * same Function changes nothing, nullptr leaves the row empty, and the
 * destructor deletes the Function and leaves its Variable. */

static void test_row_set_function( void )
{
 auto block = new AbstractBlock();
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 ColVariable x , y , z;
 {
  auto f1 = new CountedFunction( { { & x , 1 } , { & y , 1 } } );
  FRowConstraint r( block , 0 , 10 , f1 );
  assert( solver->get_Modification_list().empty() );  // the constructor
  assert( f1->get_Observer() == & r );
  assert( ( listed( x , & r ) == 1 ) && ( listed( y , & r ) == 1 ) );
  assert( r.get_num_active_var() == 2 );

  // replaced, the old one kept
  auto f2 = new CountedFunction( { { & y , 2 } , { & z , 1 } } );
  r.set_function( f2 , eModBlck , false );
  assert( CountedFunction::alive == 2 );
  assert( r.get_function() == f2 );
  assert( ( f1->get_Observer() == nullptr ) && ( f2->get_Observer() == & r ) );
  assert( ( listed( x , & r ) == 0 ) && ( listed( y , & r ) == 1 ) &&
	  ( listed( z , & r ) == 1 ) );
  auto & list = solver->get_Modification_list();
  assert( list.size() == 1 );
  auto fmod = std::dynamic_pointer_cast< FRowConstraintMod >( list.front() );
  assert( fmod && ( fmod->constraint() == & r ) &&
	  ( fmod->type() == FRowConstraintMod::eFunctionChanged ) );
  list.clear();

  // the old Function no longer reaches the Solver
  f1->modify_coefficient( 0 , 7 );
  f1->add_variable( & z , 1 );
  assert( list.empty() );
  assert( listed( z , & r ) == 1 );
  delete f1;
  assert( CountedFunction::alive == 1 );

  // the same Function changes nothing
  r.set_function( f2 );
  assert( list.empty() && ( listed( y , & r ) == 1 ) );
  assert( CountedFunction::alive == 1 );

  // replaced, the old one deleted
  r.set_function( new CountedFunction( { { & x , 1 } } ) , eNoBlck );
  assert( CountedFunction::alive == 1 );
  assert( ( listed( x , & r ) == 1 ) && ( listed( y , & r ) == 0 ) &&
	  ( listed( z , & r ) == 0 ) );
  assert( ( list.size() == 1 ) && ( ! list.front()->concerns_Block() ) );
  list.clear();

  // emptied, silently
  r.set_function( nullptr , eNoMod );
  assert( ( CountedFunction::alive == 0 ) && list.empty() );
  assert( ( listed( x , & r ) == 0 ) && ( r.get_num_active_var() == 0 ) );
  assert( r.compute() == Constraint::kUnEval );

  r.set_function( new CountedFunction( { { & x , 1 } , { & y , 1 } } ) ,
		  eNoMod );
  assert( CountedFunction::alive == 1 );
  }

 // the destructor deletes the Function and leaves its Variable
 assert( CountedFunction::alive == 0 );
 assert( ( x.get_num_active() == 0 ) && ( y.get_num_active() == 0 ) &&
	 ( z.get_num_active() == 0 ) );

 block->unregister_Solvers( true );
 delete block;
 std::cout << "set_function: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A Modification of the Function reaches the Solver of the Block through
 * the row, and a FunctionModVars adds the row to, or removes it from, the
 * Variable it names; with no Solver the row keeps listening, so that the
 * registration follows the Function all the same. */

static void test_row_function_mods( void )
{
 auto block = new AbstractBlock();
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & list = solver->get_Modification_list();
 ColVariable x , y , z;
 {
  FRowConstraint lc( block , -INF , 10 ,
		     new LinearFunction( { { & x , 1 } , { & y , 2 } } ) );
  auto f = static_cast< LinearFunction * >( lc.get_function() );
  assert( list.empty() );

  // a coefficient
  f->modify_coefficient( 1 , 3 );
  assert( list.size() == 1 );
  auto fmod = std::dynamic_pointer_cast< FunctionMod >( list.front() );
  assert( fmod && ( fmod->function() == f ) );
  assert( f->get_coefficient( 1 ) == 3 );
  list.clear();

  x.set_value( 1 );
  y.set_value( 2 );
  lc.compute();
  assert( lc.lb() == 7 );

  // a Variable added
  f->add_variable( & z , 4 );
  assert( list.size() == 1 );
  auto vmod = std::dynamic_pointer_cast< FunctionModVars >( list.front() );
  assert( vmod && vmod->added() && ( vmod->vars().size() == 1 ) &&
	  ( vmod->vars()[ 0 ] == & z ) );
  assert( ( listed( z , & lc ) == 1 ) && ( lc.is_active( & z ) == 2 ) );
  assert( ( lc.get_num_active_var() == 3 ) && ( iterated( lc ) == 3 ) );
  list.clear();

  // a Variable removed
  lc.remove_variable( 0 );
  assert( list.size() == 1 );
  vmod = std::dynamic_pointer_cast< FunctionModVars >( list.front() );
  assert( vmod && ( ! vmod->added() ) && ( vmod->vars()[ 0 ] == & x ) );
  assert( ( listed( x , & lc ) == 0 ) && ( lc.is_active( & x ) == NOTACTIVE ) );
  list.clear();

  // a range, then all of them
  lc.remove_variables( Range( 0 , 1 ) );
  assert( ( listed( y , & lc ) == 0 ) && ( lc.get_num_active_var() == 1 ) );
  f->add_variables( { { & x , 1 } , { & y , 1 } } );
  assert( ( listed( x , & lc ) == 1 ) && ( listed( y , & lc ) == 1 ) );
  lc.remove_variables( Subset() );
  assert( lc.get_num_active_var() == 0 );
  assert( ( listed( x , & lc ) == 0 ) && ( listed( y , & lc ) == 0 ) &&
	  ( listed( z , & lc ) == 0 ) );
  list.clear();

  // a subset, given unordered
  f->add_variables( { { & x , 1 } , { & y , 1 } , { & z , 1 } } );
  lc.remove_variables( Subset( { 2 , 0 } ) );
  assert( ( listed( x , & lc ) == 0 ) && ( listed( y , & lc ) == 1 ) &&
	  ( listed( z , & lc ) == 0 ) );
  assert( ( lc.get_num_active_var() == 1 ) && ( lc.get_active_var( 0 ) == & y ) );
  lc.remove_variable( 0 );
  list.clear();

  // an added Variable in a channel reaches the Solver when it is closed
  auto chnl = block->open_channel();
  f->add_variable( & x , 1 , Observer::make_par( eModBlck , chnl ) );
  assert( list.empty() && ( listed( x , & lc ) == 1 ) );
  block->close_channel( chnl );
  assert( list.size() == 1 );
  list.clear();
  lc.remove_variable( 0 );
  list.clear();

  // no Solver: the registration still follows the Function
  block->unregister_Solvers( true );
  solver = nullptr;
  f->add_variable( & x , 1 );
  assert( listed( x , & lc ) == 1 );
  lc.remove_variable( 0 );
  assert( listed( x , & lc ) == 0 );
  f->add_variables( { { & x , 1 } , { & y , 1 } } );
  lc.remove_variables( Range( 0 , 2 ) );
  assert( ( listed( x , & lc ) == 0 ) && ( listed( y , & lc ) == 0 ) );

  /* All the Variable removed with no one listening: FRowConstraint takes
   * the removal on itself, and leaves every Variable. */
  f->add_variables( { { & x , 1 } , { & y , 1 } } );
  lc.remove_variables( Subset() );
  assert( lc.get_num_active_var() == 0 );
  assert( ( listed( x , & lc ) == 0 ) && ( listed( y , & lc ) == 0 ) );

  // eNoMod: the row removes, and leaves the Variable, by itself
  f->add_variables( { { & x , 1 } , { & y , 1 } } );
  lc.remove_variable( 1 , eNoMod );
  assert( ( listed( x , & lc ) == 1 ) && ( listed( y , & lc ) == 0 ) );
  lc.remove_variables( Subset( { 0 } ) , false , eNoMod );
  assert( ( listed( x , & lc ) == 0 ) && ( lc.get_num_active_var() == 0 ) );
  }
 assert( ( x.get_num_active() == 0 ) && ( y.get_num_active() == 0 ) &&
	 ( z.get_num_active() == 0 ) );

 delete block;
 std::cout << "row Function Modification: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A row with no Block: a Modification of its Function goes nowhere, and a
 * Variable removed with the default ModParam leaves the row all the same. */

static void test_row_without_block( void )
{
 ColVariable x , y;
 FRowConstraint lc( nullptr , 0 , 1 , new LinearFunction( { { & x , 1 } } ) );
 auto f = static_cast< LinearFunction * >( lc.get_function() );
 f->modify_coefficient( 0 , 2 );
 f->add_variable( & y , 1 );
 assert( listed( y , & lc ) == 1 );
 lc.remove_variable( 1 , eNoMod );
 assert( listed( y , & lc ) == 0 );

 f->add_variable( & y , 1 );
 lc.remove_variable( 1 );
 assert( ( listed( y , & lc ) == 0 ) && ( lc.get_num_active_var() == 1 ) );
 f->add_variable( & y , 1 );
 lc.remove_variables( Range( 1 , 2 ) );
 assert( ( listed( y , & lc ) == 0 ) && ( lc.get_num_active_var() == 1 ) );
 f->add_variable( & y , 1 );
 lc.remove_variables( Subset( { 1 } ) );
 assert( ( listed( y , & lc ) == 0 ) && ( lc.get_num_active_var() == 1 ) );
 f->add_variable( & y , 1 );
 lc.remove_variables( Subset() );
 assert( ( listed( x , & lc ) == 0 ) && ( listed( y , & lc ) == 0 ) &&
	 ( lc.get_num_active_var() == 0 ) );
 std::cout << "row with no Block: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The dual value: 0 at first, whatever set_dual() writes, 0 by default,
 * and untouched by a change of the sides. */

static void test_dual( void )
{
 ColVariable x;
 FRowConstraint r( nullptr , 0 , 1 , new LinearFunction( { { & x , 1 } } ) );
 assert( r.get_dual() == 0 );
 r.set_dual( -2.5 );
 assert( r.get_dual() == -2.5 );
 r.set_both( 3 );
 assert( r.get_dual() == -2.5 );
 r.set_dual();
 assert( r.get_dual() == 0 );
 r.set_dual( INF );
 assert( r.get_dual() == INF );

 BoxConstraint b( nullptr , & x , 0 , 1 );
 assert( b.get_dual() == 0 );
 b.set_dual( 4 );
 b.set_rhs( 2 );
 assert( b.get_dual() == 4 );

 std::cout << "dual value: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*----------------------- TESTS OF OneVarConstraint ------------------------*/
/*--------------------------------------------------------------------------*/
/* The sides each :OneVarConstraint has by construction. */

static void test_ovc_default_sides( void )
{
 auto sides = []( const RowConstraint & c , double l , double r ) {
  return( ( c.get_lhs() == l ) && ( c.get_rhs() == r ) );
  };

 assert( sides( BoxConstraint() , 0 , INF ) );
 assert( sides( BoxConstraint( nullptr , nullptr , -3 , 4 ) , -3 , 4 ) );
 assert( sides( LB0Constraint() , 0 , INF ) );
 assert( sides( LB0Constraint( nullptr , nullptr , 5 ) , 0 , 5 ) );
 assert( sides( UB0Constraint() , -INF , 0 ) );
 assert( sides( UB0Constraint( nullptr , nullptr , -5 ) , -5 , 0 ) );
 assert( sides( LBConstraint() , 0 , INF ) );
 assert( sides( LBConstraint( nullptr , nullptr , 2 ) , 2 , INF ) );
 assert( sides( UBConstraint() , -INF , 0 ) );
 assert( sides( UBConstraint( nullptr , nullptr , 2 ) , -INF , 2 ) );
 assert( sides( NNConstraint() , 0 , INF ) );
 assert( sides( NPConstraint() , -INF , 0 ) );
 assert( sides( ZOConstraint() , 0 , 1 ) );

 std::cout << "OneVarConstraint sides: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The setters each :OneVarConstraint refuses throw std::invalid_argument,
 * change nothing and issue nothing, while the value a fixed side already
 * has is accepted silently; the ones it takes issue a OneVarConstraintMod
 * of the type of the side changed. */

static void test_ovc_setters( void )
{
 auto block = new AbstractBlock();
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 ColVariable x;

 auto refused = [ & ]( RowConstraint & c , const std::function< void() > & f ) {
  auto l = c.get_lhs();
  auto r = c.get_rhs();
  bool rv = throws< std::invalid_argument >( f );
  rv &= ( c.get_lhs() == l ) && ( c.get_rhs() == r );
  rv &= solver->get_Modification_list().empty();
  return( rv );
  };

 auto one_mod = [ & ]( const Constraint & c , int type ) {
  auto mods = row_mods( solver );
  if( mods.size() != 1 )
   return( false );
  auto omod = std::dynamic_pointer_cast< OneVarConstraintMod >( mods[ 0 ] );
  return( omod && ( omod->constraint() == & c ) && ( omod->type() == type ) );
  };

 using RCM = RowConstraintMod;

 {  // BoxConstraint: both sides free
  BoxConstraint c( block , & x , 0 , 1 );
  c.set_lhs( -1 );
  assert( one_mod( c , RCM::eChgLHS ) );
  c.set_rhs( 2 );
  assert( one_mod( c , RCM::eChgRHS ) );
  c.set_both( 5 );
  assert( one_mod( c , RCM::eChgBTS ) );
  assert( ( c.get_lhs() == 5 ) && ( c.get_rhs() == 5 ) );
  c.set_both( 5 );
  c.set_lhs( 5 );
  c.set_rhs( 5 );
  c.set_lhs( -INF , eNoMod );
  assert( row_mods( solver ).empty() && ( c.get_lhs() == -INF ) );
  }

 {  // LB0Constraint: lhs fixed to 0
  LB0Constraint c( block , & x , 3 );
  assert( refused( c , [ & ] { c.set_lhs( 1 ); } ) );
  assert( refused( c , [ & ] { c.set_both( 2 ); } ) );
  c.set_lhs( 0 );
  assert( row_mods( solver ).empty() );
  c.set_rhs( 4 );
  assert( one_mod( c , RCM::eChgRHS ) && ( c.get_rhs() == 4 ) );
  c.set_both( 0 );
  auto mods = row_mods( solver );
  assert( ( mods.size() == 1 ) &&
	  ( ( mods[ 0 ]->type() == RCM::eChgRHS ) ||
	    ( mods[ 0 ]->type() == RCM::eChgBTS ) ) );
  assert( ( c.get_lhs() == 0 ) && ( c.get_rhs() == 0 ) );
  }

 {  // UB0Constraint: rhs fixed to 0
  UB0Constraint c( block , & x , -3 );
  assert( refused( c , [ & ] { c.set_rhs( 1 ); } ) );
  assert( refused( c , [ & ] { c.set_both( -2 ); } ) );
  c.set_rhs( 0 );
  assert( row_mods( solver ).empty() );
  c.set_lhs( -4 );
  assert( one_mod( c , RCM::eChgLHS ) && ( c.get_lhs() == -4 ) );
  c.set_both( 0 );
  auto mods = row_mods( solver );
  assert( ( mods.size() == 1 ) &&
	  ( ( mods[ 0 ]->type() == RCM::eChgLHS ) ||
	    ( mods[ 0 ]->type() == RCM::eChgBTS ) ) );
  assert( ( c.get_lhs() == 0 ) && ( c.get_rhs() == 0 ) );
  }

 {  // LBConstraint: rhs fixed to +INF
  LBConstraint c( block , & x , 1 );
  assert( refused( c , [ & ] { c.set_rhs( 10 ); } ) );
  assert( refused( c , [ & ] { c.set_both( 1 ); } ) );
  c.set_rhs( INF );
  assert( row_mods( solver ).empty() );
  c.set_lhs( -2 );
  assert( one_mod( c , RCM::eChgLHS ) && ( c.get_lhs() == -2 ) );
  c.set_lhs( -INF );
  assert( one_mod( c , RCM::eChgLHS ) && ( c.get_lhs() == -INF ) );
  }

 {  // UBConstraint: lhs fixed to -INF
  UBConstraint c( block , & x , 1 );
  assert( refused( c , [ & ] { c.set_lhs( -10 ); } ) );
  assert( refused( c , [ & ] { c.set_both( 1 ); } ) );
  c.set_lhs( -INF );
  assert( row_mods( solver ).empty() );
  c.set_rhs( 2 );
  assert( one_mod( c , RCM::eChgRHS ) && ( c.get_rhs() == 2 ) );
  c.set_rhs( INF );
  assert( one_mod( c , RCM::eChgRHS ) && ( c.get_rhs() == INF ) );
  }

 {  // NNConstraint, NPConstraint, ZOConstraint: nothing to change
  NNConstraint nn( block , & x );
  assert( refused( nn , [ & ] { nn.set_lhs( 1 ); } ) );
  assert( refused( nn , [ & ] { nn.set_rhs( 1 ); } ) );
  assert( refused( nn , [ & ] { nn.set_both( 0 ); } ) );
  nn.set_lhs( 0 );
  nn.set_rhs( INF );

  NPConstraint np( block , & x );
  assert( refused( np , [ & ] { np.set_lhs( -1 ); } ) );
  assert( refused( np , [ & ] { np.set_rhs( -1 ); } ) );
  assert( refused( np , [ & ] { np.set_both( 0 ); } ) );
  np.set_lhs( -INF );
  np.set_rhs( 0 );

  ZOConstraint zo( block , & x );
  assert( refused( zo , [ & ] { zo.set_lhs( 1 ); } ) );
  assert( refused( zo , [ & ] { zo.set_rhs( 0 ); } ) );
  assert( refused( zo , [ & ] { zo.set_both( 1 ); } ) );
  zo.set_lhs( 0 );
  zo.set_rhs( 1 );
  assert( row_mods( solver ).empty() );
  }

 block->unregister_Solvers( true );
 delete block;
 std::cout << "OneVarConstraint setters: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The ColVariable of a OneVarConstraint: the constructor registers it
 * silently, set_variable() moves the registration and issues a
 * OneVarConstraintMod of type eVariableChanged (nothing for the same one),
 * nullptr leaves the constraint empty, remove_variable*() accept only the
 * index 0 (and the empty Subset, meaning all), clear() forgets the
 * ColVariable without leaving it, and the destructor leaves it. */

static void test_ovc_variable( void )
{
 auto block = new AbstractBlock();
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 ColVariable x , y;
 {
  BoxConstraint c( block , & x , 0 , 1 );
  assert( solver->get_Modification_list().empty() );
  assert( ( listed( x , & c ) == 1 ) && ( c.get_num_active_var() == 1 ) );
  assert( ( c.is_active( & x ) == 0 ) && ( c.get_active_var( 0 ) == & x ) );
  assert( iterated( c ) == 1 );
  {
   auto it = c.v_begin();
   assert( & **it == & x );
   delete it;
   }

  auto changed = [ & ]( void ) {
   auto mods = row_mods( solver );
   return( ( mods.size() == 1 ) && ( mods[ 0 ]->constraint() == & c ) &&
	   std::dynamic_pointer_cast< OneVarConstraintMod >( mods[ 0 ] ) &&
	   ( mods[ 0 ]->type() == OneVarConstraintMod::eVariableChanged ) );
   };

  // switched
  c.set_variable( & y );
  assert( changed() );
  assert( ( listed( x , & c ) == 0 ) && ( listed( y , & c ) == 1 ) );
  assert( ( c.is_active( & y ) == 0 ) && ( c.is_active( & x ) == NOTACTIVE ) );
  assert( ( c.get_active_var( 0 ) == & y ) &&
	  ( c.get_active_var( 1 ) == nullptr ) );

  // the same one
  c.set_variable( & y );
  assert( row_mods( solver ).empty() && ( listed( y , & c ) == 1 ) );

  // emptied
  c.set_variable( nullptr );
  assert( changed() );
  assert( ( listed( y , & c ) == 0 ) && ( c.get_num_active_var() == 0 ) );
  assert( ( c.get_active_var( 0 ) == nullptr ) && ( iterated( c ) == 0 ) );
  assert( c.is_active( & x ) == NOTACTIVE );
  assert( ( c.lb() == 0 ) && ( c.ub() == 0 ) );
  assert( c.feasible() );  // the value of an empty one is 0, in [ 0 , 1 ]

  // silently
  c.set_variable( & x , eNoMod );
  assert( row_mods( solver ).empty() && ( listed( x , & c ) == 1 ) );

  // indices other than 0 are refused
  assert( throws< std::invalid_argument >( [ & ] { c.remove_variable( 1 ); } ) );
  assert( throws< std::invalid_argument >( [ & ] {
    c.remove_variables( Range( 0 , 2 ) ); } ) );
  assert( throws< std::invalid_argument >( [ & ] {
    c.remove_variables( Range( 1 , 2 ) ); } ) );
  assert( throws< std::invalid_argument >( [ & ] {
    c.remove_variables( Subset( { 1 } ) ); } ) );
  assert( throws< std::invalid_argument >( [ & ] {
    c.remove_variables( Subset( { 0 , 0 } ) ); } ) );
  assert( row_mods( solver ).empty() && ( listed( x , & c ) == 1 ) );

  // and 0 empties it, in every form
  c.remove_variable( 0 );
  assert( changed() && ( listed( x , & c ) == 0 ) &&
	  ( c.get_num_active_var() == 0 ) );
  c.set_variable( & x , eNoMod );
  c.remove_variables( Range( 0 , 1 ) , eNoBlck );
  assert( changed() && ( listed( x , & c ) == 0 ) );
  c.set_variable( & x , eNoMod );
  c.remove_variables( Subset() );
  assert( changed() && ( listed( x , & c ) == 0 ) );
  c.set_variable( & x , eNoMod );
  c.remove_variables( Subset( { 0 } ) , true , eNoMod );
  assert( row_mods( solver ).empty() && ( listed( x , & c ) == 0 ) );

  // clear() forgets the ColVariable without leaving it
  c.set_variable( & x , eNoMod );
  c.clear();
  assert( ( c.get_num_active_var() == 0 ) && ( listed( x , & c ) == 1 ) );
  x.remove_active( & c );

  // the destructor leaves it
  c.set_variable( & y , eNoMod );
  }
 assert( ( x.get_num_active() == 0 ) && ( y.get_num_active() == 0 ) );

 // no Block
 {
  NNConstraint c( nullptr , & x );
  c.set_variable( & y );
  assert( ( listed( x , & c ) == 0 ) && ( listed( y , & c ) == 1 ) );
  c.remove_variable( 0 );
  assert( listed( y , & c ) == 0 );
  }

 block->unregister_Solvers( true );
 delete block;
 std::cout << "OneVarConstraint ColVariable: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* Seen from the ColVariable: a Constraint it is not active in gets an index
 * >= get_num_active() out of is_active() (Variable.h), and remove_active()
 * of it throws, also when a Constraint it is active in comes after it in
 * memory. */

static void test_variable_is_active( void )
{
 ColVariable x , y;
 BoxConstraint two[ 2 ];
 two[ 1 ].set_variable( & x , eNoMod );
 two[ 0 ].set_variable( & y , eNoMod );
 assert( ( x.get_num_active() == 1 ) && ( x.is_active( & two[ 1 ] ) == 0 ) );
 assert( x.is_active( & two[ 0 ] ) >= x.get_num_active() );

 // removing it is refused, and removes nothing else
 assert( throws< std::invalid_argument >( [ & ] {
   x.remove_active( & two[ 0 ] ); } ) );
 assert( ( x.get_num_active() == 1 ) && ( listed( x , & two[ 1 ] ) == 1 ) );

 // the switch of test_ovc_variable(), seen this way
 two[ 0 ].set_variable( & x , eNoMod );
 two[ 0 ].set_variable( & y , eNoMod );
 assert( x.is_active( & two[ 0 ] ) >= x.get_num_active() );
 assert( listed( x , & two[ 0 ] ) == 0 );

 std::cout << "ColVariable::is_active: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The value of each :OneVarConstraint against the documented value of a
 * row with the same sides, i.e., the one of RowConstraint, that
 * BoxConstraint does not override: the same feasible(), abs_viol() and
 * rel_viol(), 0 for a satisfied row and for an infinite side included. */

static void test_ovc_values( void )
{
 ColVariable x;
 BoxConstraint ref( nullptr , & x );

 LBConstraint lb4( nullptr , & x , 4 );
 LBConstraint lbh( nullptr , & x , 0.5 );
 LBConstraint lbm( nullptr , & x , -INF );
 UBConstraint ub4( nullptr , & x , -4 );
 UBConstraint ubh( nullptr , & x , 0.5 );
 UBConstraint ubp( nullptr , & x , INF );
 LB0Constraint lb0( nullptr , & x , 3 );
 UB0Constraint ub0( nullptr , & x , -3 );
 NNConstraint nn( nullptr , & x );
 NPConstraint np( nullptr , & x );
 ZOConstraint zo( nullptr , & x );
 BoxConstraint box( nullptr , & x , -0.5 , 2 );

 std::vector< std::pair< std::string , RowConstraint * > > all = {
  { "LBConstraint( 4 )" , & lb4 } , { "LBConstraint( 0.5 )" , & lbh } ,
  { "LBConstraint( -INF )" , & lbm } , { "UBConstraint( -4 )" , & ub4 } ,
  { "UBConstraint( 0.5 )" , & ubh } , { "UBConstraint( INF )" , & ubp } ,
  { "LB0Constraint( 3 )" , & lb0 } , { "UB0Constraint( -3 )" , & ub0 } ,
  { "NNConstraint" , & nn } , { "NPConstraint" , & np } ,
  { "ZOConstraint" , & zo } , { "BoxConstraint( -0.5 , 2 )" , & box } };

 for( double v : { -INF , -5.0 , -1.0 , -0.25 , 0.0 , 0.25 , 0.5 , 1.0 ,
		   3.0 , 6.0 , INF } ) {
  x.set_value( v );
  for( auto & [ name , c ] : all ) {
   ref.set_lhs( c->get_lhs() , eNoMod );
   ref.set_rhs( c->get_rhs() , eNoMod );
   assert( c->compute() == Constraint::kOK );
   assert( ( c->lb() == v ) && ( c->ub() == v ) );
   auto what = name + " at " + std::to_string( v );
   expect( c->feasible() == ref.feasible() , what + ": feasible()" );
   auto av = c->abs_viol();
   expect( same( av , ref.abs_viol() ) , what + ": abs_viol() = " +
	   std::to_string( av ) + " instead of " +
	   std::to_string( ref.abs_viol() ) );
   auto rv = c->rel_viol();
   expect( same( rv , ref.rel_viol() ) , what + ": rel_viol() = " +
	   std::to_string( rv ) + " instead of " +
	   std::to_string( ref.rel_viol() ) );
   }
  }

 // a few by hand, on the documented formula
 x.set_value( 5 );
 BoxConstraint b( nullptr , & x , 1 , 3 );
 assert( ( ! b.feasible() ) && ( b.abs_viol() == 2 ) &&
	 same( b.rel_viol() , 2.0 / 3 ) );
 x.set_value( 0.5 );
 assert( ( ! b.feasible() ) && ( b.abs_viol() == 0.5 ) &&
	 ( b.rel_viol() == 0.5 ) );
 x.set_value( 3 );
 assert( b.feasible() && ( b.abs_viol() == 0 ) && ( b.rel_viol() == 0 ) );

 std::cout << "OneVarConstraint values: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* An empty :OneVarConstraint has the value 0 (lb() and ub() say so), so its
 * feasible(), abs_viol() and rel_viol() are those of 0, as they are for a
 * BoxConstraint. */

static void test_ovc_empty_values( void )
{
 BoxConstraint box( nullptr , nullptr , -1 , 1 );
 assert( box.feasible() && ( box.abs_viol() == 0 ) && ( box.rel_viol() == 0 ) );

 LBConstraint lb( nullptr , nullptr , 2 );
 UBConstraint ub( nullptr , nullptr , -2 );
 NNConstraint nn;
 NPConstraint np;
 ZOConstraint zo;
 for( RowConstraint * c : std::vector< RowConstraint * >{ & lb , & ub , & nn ,
							   & np , & zo } ) {
  assert( ( c->lb() == 0 ) && ( c->ub() == 0 ) );
  BoxConstraint ref( nullptr , nullptr , c->get_lhs() , c->get_rhs() );
  assert( c->feasible() == ref.feasible() );
  assert( same( c->abs_viol() , ref.abs_viol() ) &&
	  same( c->rel_viol() , ref.rel_viol() ) );
  }

 // 0 is below the lhs 2 of lb and above the rhs -2 of ub
 assert( ( ! lb.feasible() ) && ( lb.abs_viol() == 2 ) && ( lb.rel_viol() == 1 ) );
 assert( ( ! ub.feasible() ) && ( ub.abs_viol() == 2 ) && ( ub.rel_viol() == 1 ) );

 std::cout << "empty OneVarConstraint values: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

int main( void )
{
 test_row_values();
 test_row_infinite_values();
 test_row_no_function();
 test_row_is_feasible();
 test_row_bound_mods();
 test_row_set_function();
 test_row_function_mods();
 test_row_without_block();
 test_dual();

 test_ovc_default_sides();
 test_ovc_setters();
 test_ovc_variable();
 test_variable_is_active();
 test_ovc_values();
 test_ovc_empty_values();

 if( n_failed ) {
  std::cout << n_failed << " checks FAILED" << std::endl;
  return( 1 );
  }

 std::cout << "All tests passed!!" << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File tests_Constraint.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
