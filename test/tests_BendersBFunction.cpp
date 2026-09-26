/*--------------------------------------------------------------------------*/
/*---------------------- File tests_BendersBFunction.cpp -------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Unit tests for BendersBFunction and BendersBlock.
 *
 * The inner Block is a box: five ColVariable z, each with a BoxConstraint,
 * and a linear Objective. The mapping M( x ) = A x + b sets one side of some
 * of the BoxConstraint, so that with BoxSolver attached to the inner Block
 * the Benders function has a closed form: with the first two rows setting
 * the upper bound of z_0 and the lower bound of z_1,
 *
 *     phi( x ) = - ( A_0 x + b_0 ) + 2 ( A_1 x + b_1 )
 *
 * which is affine, so that value and linearization are both known.
 *
 * The tests go over the bookkeeping of the rows (added, modified and
 * deleted, one at a time, by range and by unordered subset) and the
 * Modification it issues, which need no Solver; over the sides of the
 * RowConstraint, which are only written by compute(); and over the
 * serialization of a BendersBlock, with the matrix A in both the dense and
 * the sparse format.
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
#include "BendersBFunction.h"
#include "BendersBlock.h"
#include "BoxSolver.h"
#include "DQuadFunction.h"
#include "FakeSolver.h"
#include "FRealObjective.h"
#include "FRowConstraint.h"
#include "LinearFunction.h"
#include "OneVarConstraint.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>
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
using BBF = BendersBFunction;

/*--------------------------------------------------------------------------*/
/*------------------------------- CONSTANTS --------------------------------*/
/*--------------------------------------------------------------------------*/

// the inner Block: min d z, l <= z <= u
static const std::vector< double > c_cost = { -1 , 2 , 0 , 0 , 0 };
static const std::vector< double > c_lb = { 0 , -7 , 0 , 0 , 0 };
static const std::vector< double > c_ub = { 10 , 5 , 1 , 1 , 1 };

// the quadratic one: sum c_quad z^2 + c_qlin z, stationary at 10 and -10
static const std::vector< double > c_quad = { 1 , 1 , 0 , 0 , 0 };
static const std::vector< double > c_qlin = { -20 , 20 , 0 , 0 , 0 };

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
/// the kinds of inner Block of the tests

enum InnerKind {
 eBoxLinear ,  ///< the box one, with the linear Objective d z
 eBoxQuad ,    ///< the box one, with a separable quadratic Objective
 eRows         ///< two ColVariable and two FRowConstraint, no box
 };

/*--------------------------------------------------------------------------*/
/// the inner Block of the tests; in c the RowConstraint a row can refer to

static AbstractBlock * make_inner( InnerKind kind ,
				   std::vector< RowConstraint * > & c )
{
 auto inner = new AbstractBlock();
 c.clear();

 if( kind == eRows ) {
  // min z0 + 2 z1 , z0 + z1 <= 10 , z0 - z1 >= -7; every column has a
  // nonzero cost, as a column appearing nowhere is not read back
  auto z = new std::vector< ColVariable >( 2 );
  inner->add_static_variable( *z , "z" );
  auto rows = new std::vector< FRowConstraint >( 2 );
  inner->add_static_constraint( *rows , "rows" );
  ( *rows )[ 0 ].set_function( new LinearFunction(
   LinearFunction::v_coeff_pair( { { & ( *z )[ 0 ] , 1.0 } ,
				   { & ( *z )[ 1 ] , 1.0 } } ) ) , eNoMod );
  ( *rows )[ 0 ].set_lhs( - Inf< double >() , eNoMod );
  ( *rows )[ 0 ].set_rhs( 10 , eNoMod );
  ( *rows )[ 1 ].set_function( new LinearFunction(
   LinearFunction::v_coeff_pair( { { & ( *z )[ 0 ] , 1.0 } ,
				   { & ( *z )[ 1 ] , -1.0 } } ) ) , eNoMod );
  ( *rows )[ 1 ].set_lhs( -7 , eNoMod );
  ( *rows )[ 1 ].set_rhs( Inf< double >() , eNoMod );
  for( auto & r : *rows )
   c.push_back( & r );
  auto obj = new FRealObjective( inner , new LinearFunction(
   LinearFunction::v_coeff_pair( { { & ( *z )[ 0 ] , 1.0 } ,
				   { & ( *z )[ 1 ] , 2.0 } } ) ) );
  obj->set_sense( Objective::eMin , eNoMod );
  inner->set_objective( obj , eNoMod );
  return( inner );
  }

 auto z = new std::vector< ColVariable >( c_cost.size() );
 inner->add_static_variable( *z , "z" );
 auto box = new std::vector< BoxConstraint >( c_cost.size() );
 inner->add_static_constraint( *box , "box" );
 for( Index j = 0 ; j < c_cost.size() ; ++j ) {
  ( *box )[ j ].set_variable( & ( *z )[ j ] , eNoMod );
  ( *box )[ j ].set_lhs( c_lb[ j ] , eNoMod );
  ( *box )[ j ].set_rhs( c_ub[ j ] , eNoMod );
  c.push_back( & ( *box )[ j ] );
  }

 Function * f;
 if( kind == eBoxLinear ) {
  LinearFunction::v_coeff_pair cp;
  for( Index j = 0 ; j < c_cost.size() ; ++j )
   cp.push_back( { & ( *z )[ j ] , c_cost[ j ] } );
  f = new LinearFunction( std::move( cp ) );
  }
 else {
  DQuadFunction::v_coeff_triple ct;
  for( Index j = 0 ; j < c_cost.size() ; ++j )
   ct.push_back( { & ( *z )[ j ] , c_qlin[ j ] , c_quad[ j ] } );
  f = new DQuadFunction( std::move( ct ) );
  }
 auto obj = new FRealObjective( inner , f );
 obj->set_sense( Objective::eMin , eNoMod );
 inner->set_objective( obj , eNoMod );
 return( inner );
 }

/*--------------------------------------------------------------------------*/
/// the ColVariable of a BendersBlock, as the active Variable of its function

static BBF::VarVector variables_of( BendersBlock & bb )
{
 // the BendersBlock only gives them as const, but they are its own
 BBF::VarVector x;
 for( auto & var : bb.get_variables() )
  x.push_back( const_cast< ColVariable * >( & var ) );
 return( x );
 }

/*--------------------------------------------------------------------------*/
/// a BendersBlock whose BendersBFunction has the given mapping

struct Fixture {
 BendersBlock * bb;
 BBF * bbf;
 AbstractBlock * inner;
 std::vector< RowConstraint * > c;     ///< the RowConstraint rows refer to
 FakeSolver * fake;                    ///< registered to bb

 Fixture( BBF::MultiVector && A , BBF::RealVector && b ,
	  const std::vector< Index > & rows ,
	  BBF::ConstraintSideVector && sides , Index nvar = 2 ,
	  InnerKind kind = eBoxLinear ) {
  bb = new BendersBlock( nullptr , nvar );
  inner = make_inner( kind , c );
  BBF::ConstraintVector cons;
  for( auto i : rows )
   cons.push_back( c[ i ] );
  bbf = new BBF( inner , variables_of( *bb ) , std::move( A ) ,
		 std::move( b ) , std::move( cons ) , std::move( sides ) );
  bb->set_function( bbf , eNoMod );
  bbf->set_f_Block( bb );
  fake = new FakeSolver();
  bb->register_Solver( fake );
  }

 /// attaches to the inner Block a BoxSolver computing the dual values

 void attach_BoxSolver( void ) {
  auto bs = new BoxSolver();
  bs->set_par( BoxSolver::intPDSol , 3 );
  inner->register_Solver( bs );
  }

 void set_x( const std::vector< double > & vals ) {
  auto x = variables_of( *bb );
  for( Index i = 0 ; i < vals.size() ; ++i )
   x[ i ]->set_value( vals[ i ] );
  }

 ~Fixture() {
  inner->unregister_Solvers( true );
  bb->unregister_Solvers( true );
  delete bb;  // which deletes the BendersBFunction and the inner Block
  }
 };

/*--------------------------------------------------------------------------*/
/// the only Modification in the list, as a T, which must be there

template< class T >
static std::shared_ptr< T > the_mod( FakeSolver * fake )
{
 auto & mods = fake->get_Modification_list();
 assert( mods.size() == 1 );
 auto mod = std::dynamic_pointer_cast< T >( mods.front() );
 assert( mod );
 mods.clear();
 return( mod );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- TESTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/* Rows added, modified and deleted without any Solver: A, b, the
 * RowConstraint and their sides stay aligned, the Modification say what
 * happened, and the RowConstraint themselves are not touched. */

static void test_rows_bookkeeping( void )
{
 Fixture f( { { 1 , 2 } , { 0 , -1 } } , { 1 , 0.5 } , { 0 , 1 } ,
	    { BBF::eRHS , BBF::eLHS } );
 auto & mods = f.fake->get_Modification_list();
 mods.clear();
 auto & box = f.c;

 assert( f.bbf->get_num_active_var() == 2 );
 assert( f.bbf->get_A().size() == 2 );
 assert( ( f.bbf->get_b() == BBF::RealVector( { 1 , 0.5 } ) ) );
 assert( ( f.bbf->get_constraints() ==
	   BBF::ConstraintVector( { box[ 0 ] , box[ 1 ] } ) ) );
 assert( ( f.bbf->get_sides() ==
	   BBF::ConstraintSideVector( { BBF::eRHS , BBF::eLHS } ) ) );

 // a row with b = 0: the constants of the linearizations do not change
 f.bbf->add_row( { 1 , -1 } , 0 , box[ 2 ] , BBF::eBoth );
 {
  auto mod = the_mod< BendersBFunctionModAddd >( f.fake );
  assert( ( mod->addedrows() == 1 ) &&
	  ( mod->type() == C05FunctionMod::AllEntriesChanged ) );
  }

 // two rows with b != 0: everything changes
 f.bbf->add_rows( { { 2 , 0 } , { 0 , 3 } } , { 1 , -2 } ,
		  { box[ 3 ] , box[ 4 ] } , { BBF::eLHS , BBF::eRHS } );
 {
  auto mod = the_mod< BendersBFunctionModAddd >( f.fake );
  assert( ( mod->addedrows() == 2 ) &&
	  ( mod->type() == C05FunctionMod::AllLinearizationChanged ) );
  }
 assert( f.bbf->get_A().size() == 5 );
 assert( ( f.bbf->get_b() == BBF::RealVector( { 1 , 0.5 , 0 , 1 , -2 } ) ) );
 assert( ( f.bbf->get_sides() == BBF::ConstraintSideVector(
   { BBF::eRHS , BBF::eLHS , BBF::eBoth , BBF::eLHS , BBF::eRHS } ) ) );
 assert( f.bbf->get_constraints()[ 4 ] == box[ 4 ] );

 // a row of the wrong size, or with no RowConstraint, changes nothing
 assert( throws( [ & ]() {
	  f.bbf->add_row( { 1 } , 0 , box[ 2 ] , BBF::eLHS ); } ) );
 assert( throws( [ & ]() {
	  f.bbf->add_row( { 1 , 1 } , 0 , nullptr , BBF::eLHS ); } ) );
 assert( f.bbf->get_A().size() == 5 );
 assert( mods.empty() );

 // one row modified
 f.bbf->modify_row( 3 , { 4 , 4 } , 0.5 );
 {
  auto mod = the_mod< BendersBFunctionModRngd >( f.fake );
  assert( ( mod->BFtype() == BendersBFunctionMod::ModifyRows ) &&
	  ( mod->range() == Range( 3 , 4 ) ) &&
	  ( mod->type() == C05FunctionMod::AllLinearizationChanged ) );
  }
 assert( ( f.bbf->get_A()[ 3 ] == BBF::RealVector( { 4 , 4 } ) ) &&
	 ( f.bbf->get_b()[ 3 ] == 0.5 ) );
 assert( throws( [ & ]() { f.bbf->modify_row( 5 , { 0 , 0 } , 0 ); } ) );

 // a constant set to the value it has changes nothing, and issues nothing
 f.bbf->modify_constant( 3 , 0.5 );
 f.bbf->modify_constants( BBF::RealVector( { 0.5 , -2 } ) , Range( 3 , 5 ) );
 assert( mods.empty() );

 // one constant changed: only the constants of the linearizations change
 f.bbf->modify_constant( 3 , 1.5 );
 {
  auto mod = the_mod< BendersBFunctionModRngd >( f.fake );
  assert( ( mod->BFtype() == BendersBFunctionMod::ModifyCnst ) &&
	  ( mod->range() == Range( 3 , 4 ) ) &&
	  ( mod->type() == C05FunctionMod::AlphaChanged ) );
  }

 // rows modified by an unordered subset: each row gets its own data
 f.bbf->modify_rows( { { 7 , 7 } , { 8 , 8 } } , { 70 , 80 } ,
		     Subset( { 4 , 0 } ) );
 {
  auto mod = the_mod< BendersBFunctionModSbst >( f.fake );
  // the rows are either ordered or said not to be
  Subset rows = mod->rows();
  if( ! mod->ordered() )
   std::sort( rows.begin() , rows.end() );
  assert( ( mod->BFtype() == BendersBFunctionMod::ModifyRows ) &&
	  ( rows == Subset( { 0 , 4 } ) ) );
  }
 assert( ( f.bbf->get_A()[ 4 ] == BBF::RealVector( { 7 , 7 } ) ) &&
	 ( f.bbf->get_b()[ 4 ] == 70 ) );
 assert( ( f.bbf->get_A()[ 0 ] == BBF::RealVector( { 8 , 8 } ) ) &&
	 ( f.bbf->get_b()[ 0 ] == 80 ) );

 // one row deleted: the ones after it shift down, all four vectors alike
 f.bbf->delete_row( 2 );
 {
  auto mod = the_mod< BendersBFunctionModRngd >( f.fake );
  assert( ( mod->BFtype() == BendersBFunctionMod::DeleteRows ) &&
	  ( mod->range() == Range( 2 , 3 ) ) &&
	  ( mod->type() == C05FunctionMod::AllEntriesChanged ) );
  }
 assert( ( f.bbf->get_b() == BBF::RealVector( { 80 , 0.5 , 1.5 , 70 } ) ) );
 assert( ( f.bbf->get_constraints() == BBF::ConstraintVector(
   { box[ 0 ] , box[ 1 ] , box[ 3 ] , box[ 4 ] } ) ) );
 assert( ( f.bbf->get_sides() == BBF::ConstraintSideVector(
   { BBF::eRHS , BBF::eLHS , BBF::eLHS , BBF::eRHS } ) ) );
 assert( throws( [ & ]() { f.bbf->delete_row( 4 ); } ) );

 // rows deleted by an unordered subset
 f.bbf->delete_rows( Subset( { 3 , 0 } ) );
 {
  auto mod = the_mod< BendersBFunctionModSbst >( f.fake );
  assert( ( mod->BFtype() == BendersBFunctionMod::DeleteRows ) &&
	  ( mod->rows() == Subset( { 0 , 3 } ) ) );
  }
 assert( ( f.bbf->get_A() ==
	   BBF::MultiVector( { { 0 , -1 } , { 4 , 4 } } ) ) );
 assert( ( f.bbf->get_b() == BBF::RealVector( { 0.5 , 1.5 } ) ) );
 assert( ( f.bbf->get_constraints() ==
	   BBF::ConstraintVector( { box[ 1 ] , box[ 3 ] } ) ) );
 assert( ( f.bbf->get_sides() ==
	   BBF::ConstraintSideVector( { BBF::eLHS , BBF::eLHS } ) ) );
 assert( throws( [ & ]() { f.bbf->delete_rows( Subset( { 0 , 2 } ) ); } ) );

 // none of this has touched the RowConstraint: only compute() writes them
 for( Index j = 0 ; j < c_cost.size() ; ++j )
  assert( ( box[ j ]->get_lhs() == c_lb[ j ] ) &&
	  ( box[ j ]->get_rhs() == c_ub[ j ] ) );

 // all rows deleted: the "nuclear" FunctionMod
 f.bbf->delete_rows();
 {
  auto mod = the_mod< FunctionMod >( f.fake );
  assert( std::isnan( mod->shift() ) );
  }
 assert( f.bbf->get_A().empty() && f.bbf->get_b().empty() &&
	 f.bbf->get_constraints().empty() && f.bbf->get_sides().empty() );

 std::cout << "rows bookkeeping: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A mapping with no active Variable, i.e., each row of A empty and M( x ) = b
 * constant: deleting two rows out of three by subset leaves the third one in
 * A as in b. */

static void test_rows_with_no_variable( void )
{
 Fixture f( { {} , {} , {} } , { 1 , 2 , 3 } , { 0 , 1 , 2 } ,
	    { BBF::eRHS , BBF::eLHS , BBF::eRHS } , 0 );
 assert( f.bbf->get_num_active_var() == 0 );
 assert( f.bbf->get_A().size() == 3 );

 f.bbf->delete_rows( Subset( { 0 , 2 } ) );
 assert( ( f.bbf->get_b() == BBF::RealVector( { 2 } ) ) );
 assert( f.bbf->get_sides().size() == 1 );
 expect( f.bbf->get_A().size() == 1 ,
	 "delete_rows( { 0 , 2 } ) on 3 rows with no active Variable leaves " +
	 std::to_string( f.bbf->get_A().size() ) + " rows in A and 1 in b" );

 std::cout << "rows with no active Variable: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* With BoxSolver on the inner Block, compute() writes M( x ) into the side of
 * each RowConstraint it is mapped to, and nothing into the other side; the
 * value and the linearization are those of phi( x ). With the linear
 * Objective, u_0 = M_0( x ) and l_1 = M_1( x ),
 *
 *     phi( x ) = - u_0 + 2 l_1 ,
 *
 * with the quadratic one, whose stationary points 10 and -10 are outside
 * the boxes at the points used,
 *
 *     phi( x ) = u_0^2 - 20 u_0 + l_1^2 + 20 l_1 ,
 *
 * and in both cases the gradient is phi'( u_0 ) A_0 + phi'( l_1 ) A_1. */

static void test_sides_and_value( InnerKind kind )
{
 const std::string what = kind == eBoxLinear ? "linear" : "quadratic";
 Fixture f( { { 1 , 2 } , { 0 , -1 } } , { 1 , 0.5 } , { 0 , 1 } ,
	    { BBF::eRHS , BBF::eLHS } , 2 , kind );
 f.attach_BoxSolver();
 auto & box = f.c;

 const std::vector< std::vector< double > > xs = { { 1 , 1 } , { 2 , -1 } ,
						   { 0 , 0 } };
 for( const auto & x : xs ) {
  f.set_x( x );
  assert( f.bbf->compute() == Solver::kOK );

  const double m0 = x[ 0 ] + 2 * x[ 1 ] + 1;
  const double m1 = - x[ 1 ] + 0.5;
  assert( equal( box[ 0 ]->get_rhs() , m0 ) &&
	  ( box[ 0 ]->get_lhs() == c_lb[ 0 ] ) );
  assert( equal( box[ 1 ]->get_lhs() , m1 ) &&
	  ( box[ 1 ]->get_rhs() == c_ub[ 1 ] ) );

  double phi , d0 , d1;
  if( kind == eBoxLinear ) {
   phi = - m0 + 2 * m1;
   d0 = -1;
   d1 = 2;
   }
  else {
   phi = m0 * m0 - 20 * m0 + m1 * m1 + 20 * m1;
   d0 = 2 * m0 - 20;
   d1 = 2 * m1 + 20;
   }
  const double g0 = d0 , g1 = 2 * d0 - d1;

  const std::string where = what + " inner Objective, x = ( " +
                            std::to_string( x[ 0 ] ) + " , " +
                            std::to_string( x[ 1 ] ) + " ): ";
  expect( equal( f.bbf->get_value() , phi ) , where + "phi( x ) = " +
	  std::to_string( f.bbf->get_value() ) + ", expected " +
	  std::to_string( phi ) );

  assert( f.bbf->has_linearization() );
  double g[ 2 ];
  f.bbf->get_linearization_coefficients( g );
  expect( equal( g[ 0 ] , g0 ) && equal( g[ 1 ] , g1 ) ,
	  where + "linearization coefficients ( " + std::to_string( g[ 0 ] ) +
	  " , " + std::to_string( g[ 1 ] ) + " ), expected ( " +
	  std::to_string( g0 ) + " , " + std::to_string( g1 ) + " )" );
  const double alpha = f.bbf->get_linearization_constant();
  expect( equal( alpha + g[ 0 ] * x[ 0 ] + g[ 1 ] * x[ 1 ] , phi ) ,
	  where + "the linearization is not exact at x: constant " +
	  std::to_string( alpha ) );
  }

 // a changed constant is written by the next compute()
 f.bbf->modify_constant( 0 , 3 );
 assert( f.bbf->compute( false ) == Solver::kOK );
 assert( equal( box[ 0 ]->get_rhs() , 3 ) );

 std::cout << "sides and value, " << what << " inner Objective: done"
	   << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A BendersBlock is serialized and read back: the number of Variable, the
 * sense, A, b and the sides are the same, and the RowConstraint are found
 * in the inner Block that was read the first time compute() needs them
 * (the Solver of that inner Block does not solve it, what is checked is
 * that M( x ) is written in the right side of the right rows). The rows are
 * FRowConstraint because what an AbstractBlock writes is an LP file, which
 * gives back the bounds of the columns in a group of their own. */

static void test_BendersBlock_round_trip( void )
{
 Fixture f( { { 1 , 2 } , { 0 , -1 } } , { 1 , 0.5 } , { 0 , 1 } ,
	    { BBF::eRHS , BBF::eLHS } , 2 , eRows );

 const char * file = "BendersBFunction_test_dense.nc4";
 {
  netCDF::NcFile nc( file , netCDF::NcFile::replace );
  auto g = nc.addGroup( "BendersBlock" );
  f.bb->serialize( g );
  }

 auto read = new BendersBlock();
 try {
  netCDF::NcFile nc( file , netCDF::NcFile::read );
  read->deserialize( nc.getGroup( "BendersBlock" ) );
  }
 catch( std::exception & e ) {
  expect( false , std::string( "BendersBlock::deserialize() throws: " ) +
	  e.what() );
  delete read;
  std::remove( file );
  return;
  }
 std::remove( file );

 assert( read->get_number_variables() == 2 );
 assert( read->get_objective_sense() == Objective::eMin );
 auto rf = dynamic_cast< BBF * >(
	     static_cast< FRealObjective * >( read->get_objective() )->
	     get_function() );
 assert( rf && ( rf->get_num_active_var() == 2 ) );
 assert( rf->get_A() == f.bbf->get_A() );
 assert( rf->get_b() == f.bbf->get_b() );
 assert( rf->get_sides() == f.bbf->get_sides() );

 auto rinner = rf->get_inner_block();
 assert( rinner );
 rinner->register_Solver( new FakeSolver() );
 auto rx = variables_of( *read );
 rx[ 0 ]->set_value( 1 );
 rx[ 1 ]->set_value( 1 );
 rf->compute();  // FakeSolver: kError, but the rows are written before

 const auto & rc = rf->get_constraints();
 assert( ( rc.size() == 2 ) && rc[ 0 ] && rc[ 1 ] && ( rc[ 0 ] != rc[ 1 ] ) );
 assert( ( rc[ 0 ]->get_Block() == rinner ) &&
	 ( rc[ 1 ]->get_Block() == rinner ) );
 assert( equal( rc[ 0 ]->get_rhs() , 4 ) &&
	 ( rc[ 0 ]->get_lhs() == - Inf< double >() ) );
 assert( equal( rc[ 1 ]->get_lhs() , -0.5 ) &&
	 ( rc[ 1 ]->get_rhs() == Inf< double >() ) );

 rinner->unregister_Solvers( true );
 delete read;

 std::cout << "BendersBlock round trip, dense A: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A with at most 25% nonzeros is written in the sparse format: the file has
 * the nonzeros of each row, their columns and their values, one entry per
 * row of A also for a row with no nonzero, and reading it back gives the
 * same A. The reading is done into a BendersBFunction that already has a
 * mapping of the right shape. */

static void test_sparse_round_trip( void )
{
 // 2 x 4 with 2 nonzeros, the second row nonzero only in the last column
 Fixture f( { { 1 , 0 , 0 , 0 } , { 0 , 0 , 0 , 2 } } , { 1 , 0.5 } ,
	    { 0 , 1 } , { BBF::eRHS , BBF::eLHS } , 4 , eRows );

 const char * file = "BendersBFunction_test_sparse.nc4";
 {
  netCDF::NcFile nc( file , netCDF::NcFile::replace );
  auto g = nc.addGroup( "BendersBlock" );
  f.bb->serialize( g );
  }
 {
  netCDF::NcFile nc( file , netCDF::NcFile::read );
  auto g = nc.getGroup( "BendersBlock" ).getGroup( "BendersBFunction" );
  assert( ! g.getDim( "NumNonzero" ).isNull() );
  assert( g.getDim( "NumNonzero" ).getSize() == 2 );
  std::vector< unsigned int > nnz( 2 ) , col( 2 );
  std::vector< double > val( 2 );
  g.getVar( "NumNonzeroAtRow" ).getVar( nnz.data() );
  g.getVar( "Column" ).getVar( col.data() );
  g.getVar( "A" ).getVar( val.data() );
  assert( ( nnz == std::vector< unsigned int >( { 1 , 1 } ) ) );
  assert( ( col == std::vector< unsigned int >( { 0 , 3 } ) ) );
  assert( ( val == std::vector< double >( { 1 , 2 } ) ) );

  // read it back into a BendersBFunction whose A is already 2 x 4: were it
  // empty, what is read would be written out of its bounds
  BendersBlock holder( nullptr , 4 );
  auto rf = new BBF();
  rf->set_variables( variables_of( holder ) );
  rf->set_mapping( BBF::MultiVector( 2 , BBF::RealVector( 4 , 0 ) ) ,
		   BBF::RealVector( 2 , 0 ) , BBF::ConstraintVector( 2 ) ,
		   BBF::ConstraintSideVector( 2 , BBF::eBoth ) , eNoMod );
  rf->deserialize( g );
  expect( rf->get_A() == f.bbf->get_A() ,
	  "A read back from the sparse format is not the A written: "
	  "A[ 0 ][ 0 ] = " + std::to_string( rf->get_A()[ 0 ][ 0 ] ) +
	  ", A[ 1 ][ 3 ] = " + std::to_string( rf->get_A()[ 1 ][ 3 ] ) +
	  ", expected 1 and 2" );
  assert( rf->get_b() == f.bbf->get_b() );
  assert( rf->get_sides() == f.bbf->get_sides() );
  delete rf;
  }
 std::remove( file );

 // the last row of A with no nonzero: its count is still in the file
 f.bbf->modify_row( 1 , { 0 , 0 , 0 , 0 } , 0.5 );
 f.bbf->modify_row( 0 , { 0 , 0 , 3 , 0 } , 1 );
 bool written = true;
 try {
  netCDF::NcFile nc( file , netCDF::NcFile::replace );
  auto g = nc.addGroup( "BendersBlock" );
  f.bb->serialize( g );
  }
 catch( std::exception & e ) {
  written = false;
  expect( false , std::string( "serialize() of A = { { 0 , 0 , 3 , 0 } , "
			       "{ 0 , 0 , 0 , 0 } } throws: " ) + e.what() );
  }
 if( written ) {
  netCDF::NcFile nc( file , netCDF::NcFile::read );
  auto g = nc.getGroup( "BendersBlock" ).getGroup( "BendersBFunction" );
  std::vector< unsigned int > nnz( 2 );
  g.getVar( "NumNonzeroAtRow" ).getVar( nnz.data() );
  expect( ( nnz == std::vector< unsigned int >( { 1 , 0 } ) ) ,
	  "NumNonzeroAtRow of A = { { 0 , 0 , 3 , 0 } , { 0 , 0 , 0 , 0 } } "
	  "is { " + std::to_string( nnz[ 0 ] ) + " , " +
	  std::to_string( nnz[ 1 ] ) + " }, expected { 1 , 0 }" );
  }
 std::remove( file );

 std::cout << "BendersBlock round trip, sparse A: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 test_rows_bookkeeping();
 test_rows_with_no_variable();
 test_sides_and_value( eBoxLinear );
 test_sides_and_value( eBoxQuad );
 test_BendersBlock_round_trip();
 test_sparse_round_trip();

 if( n_failures ) {
  std::cout << "BendersBFunction_unit_test: " << n_failures
	    << " check(s) failed"
	    << std::endl;
  return( 1 );
  }

 std::cout << "All tests passed!!" << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_BendersBFunction.cpp -----------------*/
/*--------------------------------------------------------------------------*/
