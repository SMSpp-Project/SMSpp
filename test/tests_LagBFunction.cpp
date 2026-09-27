/*--------------------------------------------------------------------------*/
/*------------------------ File tests_LagBFunction.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Unit tests for LagBFunction.
 *
 * The inner Block is a box: three ColVariable, each with a BoxConstraint,
 * and a linear Objective, solved by BoxSolver. The Lagrangian function then
 * has a closed form: with c^y = c + y A the Lagrangian costs,
 *
 *     l( y ) = \sum_j min { c^y_j x_j : l_j <= x_j <= u_j } + y b
 *
 * and every value and linearization the LagBFunction gives is checked
 * against it. The tests go over what happens to the Lagrangian term when it
 * is empty, when it is removed all at once and then given again, and when
 * it is set twice, over the Modification that these changes issue, over the
 * points where the minimizer is not unique, and over the copy of the global
 * pool that a State holds.
 *
 * A second set of tests checks the by-column representation of the
 * Lagrangian term that the Lagrangian costs c_j + y A^j are computed from,
 * as get_A_by_col() gives it, after the Lagrangian pairs are set (once and
 * twice), added and removed (all of them, a Range, an ordered and an
 * unordered Subset, a single one), and on the edge cases of those methods
 * (empty Range and Subset, a Range past the end, a wrong index, no pair at
 * all).
 *
 * A check that fails because of a defect of the library prints what it
 * found and what it expected, and the test goes on with the next one; the
 * return value of main() says whether any failed.
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
#include "BoxSolver.h"
#include "FakeSolver.h"
#include "FRealObjective.h"
#include "LagBFunction.h"
#include "LinearFunction.h"
#include "OneVarConstraint.h"

#include <cmath>
#include <iostream>
#include <vector>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using Index = Block::Index;
using Range = Block::Range;
using Subset = Block::Subset;
using v_dual_pair = LagBFunction::v_dual_pair;
using v_mon_pair = LagBFunction::v_mon_pair;

/*--------------------------------------------------------------------------*/
/*------------------------------- CONSTANTS --------------------------------*/
/*--------------------------------------------------------------------------*/

// the inner Block: min c x, l <= x <= u
static const std::vector< double > c_cost = { 1 , -2 , 0.5 };
static const std::vector< double > c_lb = { 0 , -1 , 1 };
static const std::vector< double > c_ub = { 2 , 1 , 3 };

// the Lagrangian term g( x ) = A x + b
static const std::vector< std::vector< double > > c_A = { { 1 , 1 , 0 } ,
							  { 0 , 1 , -2 } };
static const std::vector< double > c_b = { -1 , 3 };

static constexpr double c_eps = 1e-10;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

static int n_failures = 0;  ///< number of failed checks of the library

/// records a failed check of the library, saying what was found

static void expect( bool ok , const std::string & what )
{
 if( ok )
  return;
 std::cout << "FAILED: " << what << std::endl;
 ++n_failures;
 }

/*--------------------------------------------------------------------------*/

static bool equal( double a , double b )
{
 return( std::abs( a - b ) <= c_eps * std::max( 1.0 , std::abs( b ) ) );
 }

/*--------------------------------------------------------------------------*/
/// the closed form of l( y ) for the rows of A in rows

static double closed_form( const std::vector< double > & y ,
			   const std::vector< Index > & rows )
{
 double val = 0;
 for( Index j = 0 ; j < c_cost.size() ; ++j ) {
  double cy = c_cost[ j ];
  for( Index k = 0 ; k < rows.size() ; ++k )
   cy += y[ k ] * c_A[ rows[ k ] ][ j ];
  val += cy * ( cy >= 0 ? c_lb[ j ] : c_ub[ j ] );
  }
 for( Index k = 0 ; k < rows.size() ; ++k )
  val += y[ k ] * c_b[ rows[ k ] ];

 return( val );
 }

/*--------------------------------------------------------------------------*/
/// everything a test needs: the outer and inner Block, the LagBFunction

struct Fixture {
 AbstractBlock * outer;
 std::vector< ColVariable > * y;       ///< the multipliers, in outer
 AbstractBlock * inner;
 std::vector< ColVariable > * x;       ///< the primal variables, in inner
 LagBFunction * lbf;
 FakeSolver * fake;                    ///< registered to outer

 /// the Block are built, the LagBFunction has no Lagrangian term yet

 Fixture( void ) {
  inner = new AbstractBlock();
  x = new std::vector< ColVariable >( c_cost.size() );
  inner->add_static_variable( *x , "x" );
  auto box = new std::vector< BoxConstraint >( c_cost.size() );
  inner->add_static_constraint( *box , "box" );
  LinearFunction::v_coeff_pair cp;
  for( Index j = 0 ; j < c_cost.size() ; ++j ) {
   ( *box )[ j ].set_variable( & ( *x )[ j ] , eNoMod );
   ( *box )[ j ].set_lhs( c_lb[ j ] , eNoMod );
   ( *box )[ j ].set_rhs( c_ub[ j ] , eNoMod );
   cp.push_back( { & ( *x )[ j ] , c_cost[ j ] } );
   }
  auto obj = new FRealObjective( inner , new LinearFunction( std::move( cp ) ) );
  obj->set_sense( Objective::eMin , eNoMod );
  inner->set_objective( obj , eNoMod );

  outer = new AbstractBlock();
  y = new std::vector< ColVariable >( c_A.size() );
  outer->add_static_variable( *y , "y" );

  lbf = new LagBFunction( inner );
  inner->register_Solver( new BoxSolver() );

  fake = new FakeSolver();
  outer->register_Solver( fake );
  }

 /// the i-th row of the Lagrangian term, with y_k as multiplier

 LagBFunction::dual_pair pair( Index i , Index k ) {
  LinearFunction::v_coeff_pair cp;
  for( Index j = 0 ; j < c_cost.size() ; ++j )
   cp.push_back( { & ( *x )[ j ] , c_A[ i ][ j ] } );
  return( LagBFunction::dual_pair( & ( *y )[ k ] ,
				   new LinearFunction( std::move( cp ) , c_b[ i ] ) ) );
  }

 /// makes the LagBFunction the Function of the Objective of outer

 void observe( void ) {
  auto obj = new FRealObjective( outer , lbf );
  obj->set_sense( Objective::eMax , eNoMod );
  outer->set_objective( obj , eNoMod );
  observed = true;
  }

 /// the multipliers take the given values

 void set_y( const std::vector< double > & vals ) {
  for( Index k = 0 ; k < vals.size() ; ++k )
   ( *y )[ k ].set_value( vals[ k ] );
  }

 ~Fixture() {
  outer->unregister_Solvers( true );
  inner->unregister_Solvers( true );
  // the Objective of outer only clear()s its Function when outer goes, so
  // the LagBFunction (and the inner Block with it) is deleted here, while
  // the multipliers it refers to are still there
  if( observed )
   static_cast< FRealObjective * >( outer->get_objective() )->set_function(
						  nullptr , eNoMod , true );
  else
   delete lbf;
  delete outer;
  }

 bool observed = false;
 };

/*--------------------------------------------------------------------------*/
/* Computes the LagBFunction at the current y and checks the value and the
 * last linearization against the closed form for the given rows: the value
 * is l( y ), the coefficients are g( x* ) for a minimizer x* in the box,
 * and the constant is c x*, so that the linearization at y is exact. */

static void check_at( Fixture & f , const std::vector< double > & yv ,
		      const std::vector< Index > & rows ,
		      const std::string & where )
{
 f.set_y( yv );
 assert( f.lbf->compute() == Solver::kOK );

 const double expected = closed_form( yv , rows );
 const double got = f.lbf->get_value();
 expect( equal( got , expected ) ,
	 where + ": l( y ) = " + std::to_string( got ) + ", expected " +
	 std::to_string( expected ) );

 assert( f.lbf->has_linearization() );
 const Index nv = f.lbf->get_num_active_var();
 std::vector< double > g( nv );
 if( nv )
  f.lbf->get_linearization_coefficients( g.data() );
 const double alpha = f.lbf->get_linearization_constant();

 // the minimizer is in the box and it is optimal for the Lagrangian costs
 double cx = 0;
 double cyx = 0;
 for( Index j = 0 ; j < c_cost.size() ; ++j ) {
  const double xj = ( *f.x )[ j ].get_value();
  assert( ( xj >= c_lb[ j ] - c_eps ) && ( xj <= c_ub[ j ] + c_eps ) );
  double cy = c_cost[ j ];
  for( Index k = 0 ; k < rows.size() ; ++k )
   cy += yv[ k ] * c_A[ rows[ k ] ][ j ];
  cx += c_cost[ j ] * xj;
  cyx += cy * xj;
  }
 double yb = 0;
 for( Index k = 0 ; k < rows.size() ; ++k )
  yb += yv[ k ] * c_b[ rows[ k ] ];
 expect( equal( cyx + yb , expected ) ,
	 where + ": the minimizer written in the inner Block is not optimal" );

 // the coefficients are the relaxed rows at the minimizer
 for( Index k = 0 ; k < nv ; ++k ) {
  double gk = c_b[ rows[ k ] ];
  for( Index j = 0 ; j < c_cost.size() ; ++j )
   gk += c_A[ rows[ k ] ][ j ] * ( *f.x )[ j ].get_value();
  expect( equal( g[ k ] , gk ) ,
	  where + ": g[ " + std::to_string( k ) + " ] = " +
	  std::to_string( g[ k ] ) + ", expected " + std::to_string( gk ) );
  }

 // the constant is the original cost, and the linearization is exact at y
 expect( equal( alpha , cx ) , where + ": constant = " +
	 std::to_string( alpha ) + ", expected " + std::to_string( cx ) );
 double lin = alpha;
 for( Index k = 0 ; k < nv ; ++k )
  lin += g[ k ] * yv[ k ];
 expect( equal( lin , expected ) , where + ": linearization at y = " +
	 std::to_string( lin ) + ", expected " + std::to_string( expected ) );
 }

/*--------------------------------------------------------------------------*/
/* True if no column of the Lagrangian costs refers to a multiplier: what the
 * LagBFunction has to hold when its Lagrangian term is empty. */

static bool no_multiplier_in_costs( Fixture & f )
{
 for( auto & xj : *f.x )
  if( auto col = f.lbf->get_A_by_col( & xj ) )
   if( ! col->second.empty() )
    return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- TESTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/* An empty Lagrangian term: the LagBFunction has no active Variable, no
 * nonzero, and its value is the minimum of c x over the box. */

static void test_empty_dual_pairs( void )
{
 Fixture f;
 f.lbf->set_dual_pairs( v_dual_pair() );
 f.observe();
 f.fake->get_Modification_list().clear();

 assert( f.lbf->get_num_active_var() == 0 );
 assert( f.lbf->get_A_nz() == 0 );
 assert( no_multiplier_in_costs( f ) );

 check_at( f , {} , {} , "empty Lagrangian term" );

 // setting the empty term again changes nothing, and issues nothing
 f.lbf->set_dual_pairs( v_dual_pair() );
 assert( f.lbf->get_num_active_var() == 0 );
 check_at( f , {} , {} , "empty Lagrangian term set twice" );
 assert( f.fake->get_Modification_list().empty() );

 std::cout << "empty Lagrangian term: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The Lagrangian term is added to an observed LagBFunction and removed all
 * at once with remove_variables( Subset() ): the Modification issued say
 * which multipliers came and went, the Lagrangian costs lose every
 * multiplier, and the value goes back to the minimum of c x. The same term
 * is then given again, and the value has to be the one of that term alone. */

static void test_add_then_remove_all( void )
{
 Fixture f;
 f.lbf->set_dual_pairs( v_dual_pair() );
 f.observe();
 auto & mods = f.fake->get_Modification_list();
 mods.clear();

 // add: one C05FunctionModVarsAddd with the two multipliers from 0
 f.lbf->add_dual_pairs( v_dual_pair( { f.pair( 0 , 0 ) , f.pair( 1 , 1 ) } ) );
 assert( f.lbf->get_num_active_var() == 2 );
 assert( f.lbf->get_active_var( 0 ) == & ( *f.y )[ 0 ] );
 assert( f.lbf->get_active_var( 1 ) == & ( *f.y )[ 1 ] );
 assert( f.lbf->get_A_nz() == 6 );  // the zero coefficients count too
 assert( mods.size() == 1 );
 {
  auto mod = std::dynamic_pointer_cast< C05FunctionModVarsAddd >(
							      mods.front() );
  assert( mod && ( mod->function() == f.lbf ) && ( mod->first() == 0 ) );
  assert( ( mod->vars().size() == 2 ) && ( mod->shift() == 0 ) );
  assert( ( mod->vars()[ 0 ] == & ( *f.y )[ 0 ] ) &&
	  ( mod->vars()[ 1 ] == & ( *f.y )[ 1 ] ) );
  }
 mods.clear();

 check_at( f , { 0.5 , -0.25 } , { 0 , 1 } , "after add_dual_pairs" );

 // remove all: one C05FunctionModVarsSbst with an empty subset and all the
 // multipliers
 f.lbf->remove_variables( Subset() );
 assert( f.lbf->get_num_active_var() == 0 );
 assert( mods.size() == 1 );
 {
  auto mod = std::dynamic_pointer_cast< C05FunctionModVarsSbst >(
							      mods.front() );
  assert( mod && ( mod->function() == f.lbf ) && mod->subset().empty() );
  assert( ( mod->vars().size() == 2 ) && ( mod->shift() == 0 ) );
  }
 mods.clear();

 // the Lagrangian costs refer to no multiplier any longer
 const bool clean = no_multiplier_in_costs( f );
 expect( clean , "remove_variables( Subset() ) leaves the entries of the "
	 "removed multipliers in the Lagrangian costs (get_A_by_col( x_j )"
	 "->second not empty)" );
 expect( f.lbf->get_A_nz() == 0 , "get_A_nz() = " +
	 std::to_string( f.lbf->get_A_nz() ) + " with no Lagrangian term" );

 // with no multiplier, compute() would read the stale entries past the end
 // of the (empty) vector of multipliers: only done if there are none
 if( clean )
  check_at( f , {} , {} , "after removing all the Lagrangian term" );
 else
  std::cout << "SKIPPED: compute() with no Lagrangian term, it would read "
	    << "past the end of the multipliers" << std::endl;

 // the same term again: its value is the one of the term alone
 f.lbf->add_dual_pairs( v_dual_pair( { f.pair( 0 , 0 ) , f.pair( 1 , 1 ) } ) );
 assert( f.lbf->get_num_active_var() == 2 );
 assert( mods.size() == 1 );
 expect( f.lbf->get_A_nz() == 6 , "get_A_nz() = " +
	 std::to_string( f.lbf->get_A_nz() ) + " after the term is given "
	 "again, expected 6" );
 check_at( f , { 0.5 , -0.25 } , { 0 , 1 } ,
	   "after removing all and adding the same term again" );

 std::cout << "add, remove all, add again: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* set_dual_pairs() starts from a "tabula rasa": called a second time it
 * replaces the Lagrangian term, and only the second one counts. */

static void test_set_dual_pairs_twice( void )
{
 Fixture f;
 f.lbf->set_dual_pairs( v_dual_pair( { f.pair( 0 , 0 ) , f.pair( 1 , 1 ) } ) );
 assert( f.lbf->get_num_active_var() == 2 );
 f.lbf->set_dual_pairs( v_dual_pair( { f.pair( 1 , 0 ) } ) );
 assert( f.lbf->get_num_active_var() == 1 );
 assert( f.lbf->get_active_var( 0 ) == & ( *f.y )[ 0 ] );

 const bool clean = ( f.lbf->get_A_nz() == 3 );
 expect( clean , "get_A_nz() = " + std::to_string( f.lbf->get_A_nz() ) +
	 " after set_dual_pairs() of one row of 3 entries over a term of two "
	 "rows, expected 3" );

 // the entries left of the first term refer to a multiplier that is no
 // longer there: compute() would read past the end of the multipliers
 if( clean )
  check_at( f , { 0.75 } , { 1 } , "set_dual_pairs() called twice" );
 else
  std::cout << "SKIPPED: compute() after set_dual_pairs() called twice, it "
	    << "would read past the end of the multipliers" << std::endl;

 std::cout << "set_dual_pairs twice: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The value and the linearization over a few multipliers, the one of the
 * original problem ( y = 0 ) and some on a kink, i.e., where a Lagrangian
 * cost is zero and the minimizer is not unique; the linearizations are also
 * checked to be supergradients of the (concave) l( y ) at the other points. */

static void test_values_and_kinks( void )
{
 Fixture f;
 f.lbf->set_dual_pairs( v_dual_pair( { f.pair( 0 , 0 ) , f.pair( 1 , 1 ) } ) );

 // y = 0; c^y_0 = 0; c^y_0 = c^y_2 = 0; two generic points
 const std::vector< std::vector< double > > ys = {
  { 0 , 0 } , { -1 , 0 } , { -1 , 0.25 } , { 2 , 0.5 } , { -3 , -1 } };

 for( Index p = 0 ; p < ys.size() ; ++p ) {
  const std::string where = "y = ( " + std::to_string( ys[ p ][ 0 ] ) + " , "
                            + std::to_string( ys[ p ][ 1 ] ) + " )";
  check_at( f , ys[ p ] , { 0 , 1 } , where );

  double g[ 2 ];
  f.lbf->get_linearization_coefficients( g );
  const double alpha = f.lbf->get_linearization_constant();
  for( Index q = 0 ; q < ys.size() ; ++q ) {
   const double lq = closed_form( ys[ q ] , { 0 , 1 } );
   const double lin = alpha + g[ 0 ] * ys[ q ][ 0 ] + g[ 1 ] * ys[ q ][ 1 ];
   expect( lq <= lin + c_eps , where + ": the linearization is below l at "
	   "point " + std::to_string( q ) );
   }
  }

 // compute() with no change of y gives the same value
 f.set_y( ys.back() );
 assert( f.lbf->compute( false ) == Solver::kOK );
 expect( equal( f.lbf->get_value() , closed_form( ys.back() , { 0 , 1 } ) ) ,
	 "compute( false ) changes the value" );

 std::cout << "values and kinks: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The State holds a copy of the global pool: a linearization stored and then
 * deleted comes back, with the same coefficients and constant, when the
 * State is put back. */

static void test_state_copy( void )
{
 Fixture f;
 f.lbf->set_dual_pairs( v_dual_pair( { f.pair( 0 , 0 ) , f.pair( 1 , 1 ) } ) );
 f.lbf->set_par( C05Function::intGPMaxSz , 2 );

 f.set_y( { 2 , 0.5 } );
 assert( f.lbf->compute() == Solver::kOK );
 assert( f.lbf->has_linearization() );
 double g[ 2 ];
 f.lbf->get_linearization_coefficients( g );
 const double alpha = f.lbf->get_linearization_constant();
 f.lbf->store_linearization( 0 , eNoMod );
 assert( f.lbf->is_linearization_there( 0 ) );
 assert( ! f.lbf->is_linearization_vertical( 0 ) );

 auto state = f.lbf->get_State();
 assert( state );

 f.lbf->delete_linearization( 0 , eNoMod );
 assert( ! f.lbf->is_linearization_there( 0 ) );

 // a different y changes the Solution written in the inner Block
 f.set_y( { -3 , -1 } );
 assert( f.lbf->compute() == Solver::kOK );

 f.lbf->put_State( *state );
 delete state;
 expect( f.lbf->is_linearization_there( 0 ) ,
	 "put_State() does not bring back the stored linearization" );
 if( f.lbf->is_linearization_there( 0 ) ) {
  double h[ 2 ];
  f.lbf->get_linearization_coefficients( h , Block::INFRange , 0 );
  expect( equal( h[ 0 ] , g[ 0 ] ) && equal( h[ 1 ] , g[ 1 ] ) ,
	  "the coefficients of the linearization put back differ" );
  expect( equal( f.lbf->get_linearization_constant( 0 ) , alpha ) ,
	  "the constant of the linearization put back differs" );
  }

 std::cout << "State copy of the global pool: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------- THE COLUMNS OF THE LAGRANGIAN TERM -------------------*/
/*--------------------------------------------------------------------------*/

/* The inner Block has four ColVariable x0, ..., x3 and the linear Objective
 * 1 x0 + 2 x1: x2 enters the Objective (with cost 0) as soon as a Lagrangian
 * term has it, x3 is never in any. The multipliers y are ColVariable held by
 * the Rig, as the Lagrangian pairs only point to them. */

struct Rig {
 std::vector< ColVariable > * x;
 std::vector< ColVariable > y;
 LagBFunction * f;

 Rig( void ) : y( 8 ) {
  auto block = new AbstractBlock();
  x = new std::vector< ColVariable >( 4 );
  block->add_static_variable( *x , "x" );
  block->set_objective( new FRealObjective( nullptr ,
			 new LinearFunction( { { X( 0 ) , 1 } ,
					       { X( 1 ) , 2 } } ) ) ,
			eNoMod );
  f = new LagBFunction( block );
  }

 ~Rig( void ) { delete f; }  // the LagBFunction deletes the inner Block

 ColVariable * X( Index j ) { return( & ( *x )[ j ] ); }

 // the Lagrangian pair < y[ k ] , sum_j a_j x_j > for terms { { j , a_j } }
 LagBFunction::dual_pair pair( Index k ,
			       std::vector< std::pair< Index , double > > t ) {
  LinearFunction::v_coeff_pair cp;
  for( auto & el : t )
   cp.push_back( { X( el.first ) , el.second } );
  return( LagBFunction::dual_pair( & y[ k ] ,
				    new LinearFunction( std::move( cp ) ) ) );
  }

 // the by-column representation A^j of x_j, which must be there
 const v_mon_pair & A( Index j ) {
  auto col = f->get_A_by_col( X( j ) );
  assert( col );
  return( col->second );
  }

 // the original cost c_j of x_j
 double c( Index j ) { return( f->get_A_by_col( X( j ) )->first ); }

 // the Lagrangian cost c_j + y A^j of x_j, with the names in A^j checked to
 // be those of the current Lagrangian pairs
 double lag_cost( Index j ) {
  auto col = f->get_A_by_col( X( j ) );
  assert( col );
  double cost = col->first;
  for( auto & el : col->second ) {
   assert( el.first < f->get_num_active_var() );
   cost += static_cast< ColVariable * >(
			 f->get_active_var( el.first ) )->get_value()
    * el.second;
   }
  return( cost );
  }
 };

/*--------------------------------------------------------------------------*/

static bool same( const v_mon_pair & a , const v_mon_pair & b )
{
 if( a.size() != b.size() )
  return( false );
 for( Index i = 0 ; i < a.size() ; ++i )
  if( ( a[ i ].first != b[ i ].first ) || ( a[ i ].second != b[ i ].second ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

// the two pairs < y0 , x0 + 2 x1 > and < y1 , 3 x1 + 4 x2 >

static void set_two_pairs( Rig & r )
{
 v_dual_pair dp;
 dp.push_back( r.pair( 0 , { { 0 , 1 } , { 1 , 2 } } ) );
 dp.push_back( r.pair( 1 , { { 1 , 3 } , { 2 , 4 } } ) );
 r.f->set_dual_pairs( std::move( dp ) );

 assert( r.f->get_num_active_var() == 2 );
 assert( same( r.A( 0 ) , { { 0 , 1 } } ) );
 assert( same( r.A( 1 ) , { { 0 , 2 } , { 1 , 3 } } ) );
 assert( same( r.A( 2 ) , { { 1 , 4 } } ) );
 }

/*--------------------------------------------------------------------------*/

// no Lagrangian pair left: every A^j is empty and every cost is the original

static void check_empty( Rig & r )
{
 assert( r.f->get_num_active_var() == 0 );
 for( Index j = 0 ; j < 3 ; ++j )
  assert( r.A( j ).empty() );
 assert( r.c( 0 ) == 1 );
 assert( r.c( 1 ) == 2 );
 assert( r.c( 2 ) == 0 );
 }

/*--------------------------------------------------------------------------*/

// after the Lagrangian term is emptied, the single pair < z , 5 x2 > is added:
// A^2 is { < 0 , 5 > } and nothing else, since the name 0 is now z

static void check_after_readd( Rig & r )
{
 v_dual_pair dp;
 dp.push_back( r.pair( 4 , { { 2 , 5 } } ) );
 r.f->add_dual_pairs( std::move( dp ) );

 assert( r.f->get_num_active_var() == 1 );
 assert( r.A( 0 ).empty() );
 assert( r.A( 1 ).empty() );
 assert( same( r.A( 2 ) , { { 0 , 5 } } ) );

 r.y[ 4 ].set_value( 7 );
 assert( equal( r.lag_cost( 0 ) , 1 ) );
 assert( equal( r.lag_cost( 1 ) , 2 ) );
 assert( equal( r.lag_cost( 2 ) , 35 ) );
 }

/* set_dual_pairs() builds A^j for each x_j, x2 entering the Objective with
 * cost 0, and the Lagrangian costs are c_j + y A^j; x3 has no column. */

static void test_columns_set_dual_pairs( void )
{
 Rig r;
 set_two_pairs( r );

 assert( r.c( 0 ) == 1 );
 assert( r.c( 1 ) == 2 );
 assert( r.c( 2 ) == 0 );
 assert( r.f->get_A_by_col( r.X( 3 ) ) == nullptr );

 r.y[ 0 ].set_value( 10 );
 r.y[ 1 ].set_value( -1 );
 assert( equal( r.lag_cost( 0 ) , 1 + 10 ) );
 assert( equal( r.lag_cost( 1 ) , 2 + 20 - 3 ) );
 assert( equal( r.lag_cost( 2 ) , 0 - 4 ) );
 }

/*--------------------------------------------------------------------------*/

/* set_dual_pairs() called a second time replaces the Lagrangian term: the
 * columns of the first one must not survive into the second, whose pairs
 * take the names from 0 again. */

static void test_columns_set_dual_pairs_twice( void )
{
 Rig r;
 set_two_pairs( r );

 v_dual_pair dp;
 dp.push_back( r.pair( 2 , { { 0 , 6 } } ) );
 dp.push_back( r.pair( 3 , { { 2 , 8 } } ) );
 r.f->set_dual_pairs( std::move( dp ) );

 assert( r.f->get_num_active_var() == 2 );
 assert( same( r.A( 0 ) , { { 0 , 6 } } ) );
 assert( r.A( 1 ).empty() );
 assert( same( r.A( 2 ) , { { 1 , 8 } } ) );

 r.y[ 2 ].set_value( 2 );
 r.y[ 3 ].set_value( 3 );
 assert( equal( r.lag_cost( 0 ) , 1 + 12 ) );
 assert( equal( r.lag_cost( 1 ) , 2 ) );
 assert( equal( r.lag_cost( 2 ) , 24 ) );

 // and set to no pair at all
 r.f->set_dual_pairs( v_dual_pair() );
 check_empty( r );
 }

/*--------------------------------------------------------------------------*/

/* Removing all the pairs, with a Range covering them (exactly or past the
 * end) or with an empty Subset, empties every A^j; the pairs added then
 * take the names from 0. */

static void test_columns_remove_all( void )
{
 for( int how = 0 ; how < 4 ; ++how ) {
  Rig r;
  set_two_pairs( r );

  switch( how ) {
   case( 0 ): r.f->remove_variables( Range( 0 , 2 ) ); break;
   case( 1 ): r.f->remove_variables( Range( 0 , Inf< Index >() ) ); break;
   case( 2 ): r.f->remove_variables( Subset() ); break;
   default:   r.f->remove_variables( Subset( { 1 , 0 } ) );
   }

  check_empty( r );
  check_after_readd( r );
  }
 }

/*--------------------------------------------------------------------------*/

/* Partial removals keep the pairs left, renamed, in A^j, and the pairs
 * added afterwards take the names that follow. */

static void test_columns_remove_some( void )
{
 {  // a Range with the first pair
  Rig r;
  set_two_pairs( r );
  r.f->remove_variables( Range( 0 , 1 ) );
  assert( r.f->get_num_active_var() == 1 );
  assert( r.A( 0 ).empty() );
  assert( same( r.A( 1 ) , { { 0 , 3 } } ) );
  assert( same( r.A( 2 ) , { { 0 , 4 } } ) );

  v_dual_pair dp;
  dp.push_back( r.pair( 4 , { { 0 , 5 } , { 1 , -1 } } ) );
  r.f->add_dual_pairs( std::move( dp ) );
  assert( same( r.A( 0 ) , { { 1 , 5 } } ) );
  assert( same( r.A( 1 ) , { { 0 , 3 } , { 1 , -1 } } ) );
  assert( same( r.A( 2 ) , { { 0 , 4 } } ) );

  r.y[ 1 ].set_value( 2 );
  r.y[ 4 ].set_value( 3 );
  assert( equal( r.lag_cost( 0 ) , 1 + 15 ) );
  assert( equal( r.lag_cost( 1 ) , 2 + 6 - 3 ) );
  assert( equal( r.lag_cost( 2 ) , 8 ) );
  }
 {  // a Subset with the last pair
  Rig r;
  set_two_pairs( r );
  r.f->remove_variables( Subset( { 1 } ) );
  assert( r.f->get_num_active_var() == 1 );
  assert( same( r.A( 0 ) , { { 0 , 1 } } ) );
  assert( same( r.A( 1 ) , { { 0 , 2 } } ) );
  assert( r.A( 2 ).empty() );

  v_dual_pair dp;
  dp.push_back( r.pair( 4 , { { 2 , 5 } } ) );
  r.f->add_dual_pairs( std::move( dp ) );
  assert( same( r.A( 0 ) , { { 0 , 1 } } ) );
  assert( same( r.A( 1 ) , { { 0 , 2 } } ) );
  assert( same( r.A( 2 ) , { { 1 , 5 } } ) );
  }
 {  // an unordered Subset of three pairs out of four, and remove_variable()
  Rig r;
  v_dual_pair dp;
  dp.push_back( r.pair( 0 , { { 0 , 1 } } ) );
  dp.push_back( r.pair( 1 , { { 0 , 2 } , { 1 , 2 } } ) );
  dp.push_back( r.pair( 2 , { { 1 , 3 } } ) );
  dp.push_back( r.pair( 3 , { { 0 , 4 } , { 2 , 4 } } ) );
  r.f->set_dual_pairs( std::move( dp ) );

  r.f->remove_variables( Subset( { 2 , 0 } ) );
  assert( r.f->get_num_active_var() == 2 );
  assert( r.f->get_active_var( 0 ) == & r.y[ 1 ] );
  assert( r.f->get_active_var( 1 ) == & r.y[ 3 ] );
  assert( same( r.A( 0 ) , { { 0 , 2 } , { 1 , 4 } } ) );
  assert( same( r.A( 1 ) , { { 0 , 2 } } ) );
  assert( same( r.A( 2 ) , { { 1 , 4 } } ) );

  r.f->remove_variable( 0 );
  assert( r.f->get_num_active_var() == 1 );
  assert( same( r.A( 0 ) , { { 0 , 4 } } ) );
  assert( r.A( 1 ).empty() );
  assert( same( r.A( 2 ) , { { 0 , 4 } } ) );

  // the last pair removed by index: the Lagrangian term is empty
  r.f->remove_variable( 0 );
  check_empty( r );
  check_after_readd( r );
  }
 }

/*--------------------------------------------------------------------------*/

/* The edge cases: an empty Range and a Range past the end remove nothing,
 * adding no pair does nothing, a wrong index throws, and removing all the
 * pairs when there are none leaves the columns as they are. */

static void test_columns_edge_cases( void )
{
 Rig r;
 set_two_pairs( r );

 r.f->remove_variables( Range( 1 , 1 ) );
 r.f->remove_variables( Range( 5 , 9 ) );
 r.f->add_dual_pairs( v_dual_pair() );
 assert( r.f->get_num_active_var() == 2 );
 assert( same( r.A( 0 ) , { { 0 , 1 } } ) );
 assert( same( r.A( 1 ) , { { 0 , 2 } , { 1 , 3 } } ) );
 assert( same( r.A( 2 ) , { { 1 , 4 } } ) );

 bool thrown = false;
 try { r.f->remove_variable( 2 ); }
 catch( std::invalid_argument & ) { thrown = true; }
 assert( thrown );

 thrown = false;
 try { r.f->remove_variables( Subset( { 0 , 2 } ) ); }
 catch( std::invalid_argument & ) { thrown = true; }
 assert( thrown );

 r.f->remove_variables( Subset() );
 check_empty( r );
 r.f->remove_variables( Subset() );
 r.f->remove_variables( Range( 0 , Inf< Index >() ) );
 check_empty( r );
 check_after_readd( r );
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 test_empty_dual_pairs();
 test_add_then_remove_all();
 test_set_dual_pairs_twice();
 test_values_and_kinks();
 test_state_copy();
 test_columns_set_dual_pairs();
 test_columns_set_dual_pairs_twice();
 test_columns_remove_all();
 test_columns_remove_some();
 test_columns_edge_cases();

 if( n_failures ) {
  std::cout << "LagBFunction_unit_test: " << n_failures << " check(s) failed"
	    << std::endl;
  return( 1 );
  }

 std::cout << "All tests passed!!" << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File tests_LagBFunction.cpp -------------------*/
/*--------------------------------------------------------------------------*/
