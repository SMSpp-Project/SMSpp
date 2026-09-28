/*--------------------------------------------------------------------------*/
/*--------------------- File tests_PolyhedralFunction.cpp ------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Unit tests for PolyhedralFunction and for the abstract representation of
 * PolyhedralFunctionBlock.
 *
 * The values and linearizations of convex and concave functions are checked
 * at points where they are computed by hand, as are the empty function and
 * the one that is only a global bound. Every mutator is checked for the
 * Modification it issues to a FakeSolver attached to a Block holding the
 * function in its FRealObjective, and for issuing none with eNoMod. The
 * State, the netCDF format and the copy R3 Block are taken through a round
 * trip, and the primal and dual abstract representations of an empty
 * PolyhedralFunctionBlock are followed while rows are added and deleted.
 *
 * Each check prints what it expected when it fails, and all of them run, so
 * that one failing does not hide the others; main() returns the number of
 * failed checks.
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
#include "ColVariable.h"
#include "FakeSolver.h"
#include "FRealObjective.h"
#include "FRowConstraint.h"
#include "LinearFunction.h"
#include "OneVarConstraint.h"
#include "PolyhedralFunction.h"
#include "PolyhedralFunctionBlock.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using PF = PolyhedralFunction;
using Index = PF::Index;
using FV = PF::FunctionValue;

static const FV INF = Inf< FV >();

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

static int n_failed = 0;  ///< number of checks that failed

/// counts and prints a failed check, naming the case it belongs to

static void check( bool ok , const std::string & what )
{
 if( ok )
  return;
 ++n_failed;
 std::cout << "FAILED: " << what << std::endl;
 }

/*--------------------------------------------------------------------------*/
/// a number as printed by an ostream, for the messages of the checks

static std::string num( double v )
{
 std::ostringstream s;
 s << v;
 return( s.str() );
 }

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
/// true if a and b are equal within 1e-12

static bool eq( FV a , FV b ) { return( std::abs( a - b ) <= 1e-12 ); }

/*--------------------------------------------------------------------------*/
/// the current linearization of f, as the vector of its coefficients

static PF::RealVector coeffs( PF & f , Index name = Inf< Index >() )
{
 PF::RealVector g( f.get_num_active_var() , NAN );
 f.get_linearization_coefficients( g.data() , PF::INFRange , name );
 return( g );
 }

/*--------------------------------------------------------------------------*/
/// the values of the Variable pointed by x are set to v

static void set_x( const PF::VarVector & x , const PF::RealVector & v )
{
 for( Index i = 0 ; i < x.size() ; ++i )
  x[ i ]->set_value( v[ i ] );
 }

/*--------------------------------------------------------------------------*/
/// a Block with three ColVariable, a FRealObjective and a FakeSolver
/** The PolyhedralFunction lives in the FRealObjective of an AbstractBlock,
 * which dispatches every Modification it issues to the FakeSolver. */

struct Model {
 AbstractBlock * block;
 std::vector< ColVariable > * x;
 PF * f;
 FRealObjective * obj;
 FakeSolver * solver;

 Model( PF::MultiVector && A , PF::RealVector && b , FV bound = - INF ,
	bool convex = true , Index nvar = 2 ) {
  block = new AbstractBlock();
  x = new std::vector< ColVariable >( 3 );
  block->add_static_variable( *x , "x" );
  PF::VarVector vx;
  for( Index i = 0 ; i < nvar ; ++i )
   vx.push_back( & ( *x )[ i ] );
  f = new PF( std::move( vx ) , std::move( A ) , std::move( b ) , bound ,
	      convex );
  obj = new FRealObjective( block , f );
  block->set_objective( obj , eNoMod );
  solver = new FakeSolver();
  block->register_Solver( solver );
  mods().clear();
  }

 ~Model() {
  block->unregister_Solvers( true );
  delete block;
  }

 Lst_sp_Mod & mods( void ) { return( solver->get_Modification_list() ); }

 /// the only Modification in the list, cast to M, nullptr if not so
 template< class M >
 std::shared_ptr< M > only( void ) {
  if( mods().size() != 1 )
   return( nullptr );
  return( std::dynamic_pointer_cast< M >( mods().front() ) );
  }

 /// the last Modification in the list, cast to M, nullptr if not so
 template< class M >
 std::shared_ptr< M > last( void ) {
  if( mods().empty() )
   return( nullptr );
  return( std::dynamic_pointer_cast< M >( mods().back() ) );
  }
 };

/*--------------------------------------------------------------------------*/
/*------------------------------ THE TESTS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the three rows used by the value tests: x0 + 2 x1, 3 x0 + x1 + 1, - x0 - 1

static PF::MultiVector rowsA( void ) {
 return( PF::MultiVector{ { 1 , 2 } , { 3 , 1 } , { -1 , 0 } } ); }
static PF::RealVector rowsb( void ) {
 return( PF::RealVector{ 0 , 1 , -1 } ); }

/*--------------------------------------------------------------------------*/

static void test_value_convex( void )
{
 // the value is the largest row, the linearization is that row
 std::vector< ColVariable > x( 2 );
 PF f( { & x[ 0 ] , & x[ 1 ] } , rowsA() , rowsb() );
 check( f.is_convex() && ( ! f.is_concave() ) , "convex: verse" );

 struct Case { PF::RealVector x; FV value; PF::RealVector g; FV c; };
 const Case cases[] = { { { 1 , 1 } , 5 , { 3 , 1 } , 1 } ,
			{ { 0 , 0 } , 1 , { 3 , 1 } , 1 } ,
			{ { -2 , 0 } , 1 , { -1 , 0 } , -1 } };
 for( const auto & c : cases ) {
  set_x( { & x[ 0 ] , & x[ 1 ] } , c.x );
  f.compute();
  const std::string at = "convex at ( " + num( c.x[ 0 ] ) +
                         " , " + num( c.x[ 1 ] ) + " ): ";
  check( eq( f.get_value() , c.value ) ,
	 at + "value " + num( f.get_value() ) + " expected " +
	 num( c.value ) );
  check( f.has_linearization() , at + "has_linearization()" );
  check( ! f.has_linearization( false ) , at + "no vertical linearization" );
  check( coeffs( f ) == c.g , at + "linearization coefficients" );
  check( eq( f.get_linearization_constant() , c.c ) ,
	 at + "linearization constant" );
  }

 // a tie between rows 0 and 1 at ( 0 , 1 ): either may be returned, but the
 // linearization must be one of them, and exact at the point
 set_x( { & x[ 0 ] , & x[ 1 ] } , { 0 , 1 } );
 f.compute();
 check( eq( f.get_value() , 2 ) , "convex tie: value" );
 const auto g = coeffs( f );
 const auto c = f.get_linearization_constant();
 check( ( ( g == PF::RealVector{ 1 , 2 } ) && eq( c , 0 ) ) ||
	( ( g == PF::RealVector{ 3 , 1 } ) && eq( c , 1 ) ) ,
	"convex tie: linearization is one of the tied rows" );
 check( eq( g[ 0 ] * 0 + g[ 1 ] * 1 + c , 2 ) ,
	"convex tie: linearization is exact at the point" );
 }

/*--------------------------------------------------------------------------*/

static void test_value_concave( void )
{
 // the value is the smallest row, the linearization is that row
 std::vector< ColVariable > x( 2 );
 PF f( { & x[ 0 ] , & x[ 1 ] } , rowsA() , rowsb() , INF , false );
 check( f.is_concave() && ( ! f.is_convex() ) , "concave: verse" );

 struct Case { PF::RealVector x; FV value; PF::RealVector g; FV c; };
 const Case cases[] = { { { 1 , 1 } , -2 , { -1 , 0 } , -1 } ,
			{ { 0 , 0 } , -1 , { -1 , 0 } , -1 } ,
			{ { -2 , 0 } , -5 , { 3 , 1 } , 1 } };
 for( const auto & c : cases ) {
  set_x( { & x[ 0 ] , & x[ 1 ] } , c.x );
  f.compute();
  const std::string at = "concave at ( " + num( c.x[ 0 ] ) +
                         " , " + num( c.x[ 1 ] ) + " ): ";
  check( eq( f.get_value() , c.value ) ,
	 at + "value " + num( f.get_value() ) + " expected " +
	 num( c.value ) );
  check( coeffs( f ) == c.g , at + "linearization coefficients" );
  check( eq( f.get_linearization_constant() , c.c ) ,
	 at + "linearization constant" );
  }

 // a tie between rows 0 and 2 at ( -0.25 , -0.25 ), where the rows give
 // - 0.75, 0 and - 0.75
 set_x( { & x[ 0 ] , & x[ 1 ] } , { -0.25 , -0.25 } );
 f.compute();
 check( eq( f.get_value() , -0.75 ) , "concave tie: value" );
 const auto g = coeffs( f );
 const auto c = f.get_linearization_constant();
 check( ( ( g == PF::RealVector{ 1 , 2 } ) && eq( c , 0 ) ) ||
	( ( g == PF::RealVector{ -1 , 0 } ) && eq( c , -1 ) ) ,
	"concave tie: linearization is one of the tied rows" );
 }

/*--------------------------------------------------------------------------*/

static void test_local_pool( void )
{
 // with a local pool large enough, the linearizations come in order of
 // value (non-increasing if convex, non-decreasing if concave), and they
 // are the m rows plus the flat one of the bound only if a bound is set
 std::vector< ColVariable > x( 2 );
 set_x( { & x[ 0 ] , & x[ 1 ] } , { 1 , 1 } );  // rows give 3, 5, - 2

 for( bool convex : { true , false } ) {
  const std::string vs = convex ? "convex" : "concave";
  PF f( { & x[ 0 ] , & x[ 1 ] } , rowsA() , rowsb() ,
	convex ? - INF : INF , convex );
  f.set_par( PF::intLPMaxSz , 10 );
  f.compute();

  std::vector< FV > vals;
  do {
   const auto g = coeffs( f );
   vals.push_back( g[ 0 ] + g[ 1 ] + f.get_linearization_constant() );
   } while( f.compute_new_linearization() );

  const std::vector< FV > expected = convex ? std::vector< FV >{ 5 , 3 , -2 }
                                            : std::vector< FV >{ -2 , 3 , 5 };
  check( vals.size() == 3 ,
	 vs + " local pool without bound: " + num( vals.size() ) +
	 " linearizations produced, expected the 3 rows only" );
  for( Index i = 0 ; ( i < vals.size() ) && ( i < 3 ) ; ++i )
   check( eq( vals[ i ] , expected[ i ] ) ,
	  vs + " local pool: linearization " + num( i ) +
	  " has value " + num( vals[ i ] ) );
  }

 // with a bound the flat linearization is the last one, of value the bound
 PF f( { & x[ 0 ] , & x[ 1 ] } , rowsA() , rowsb() , -10 );
 f.set_par( PF::intLPMaxSz , 10 );
 f.compute();
 Index n = 1;
 while( f.compute_new_linearization() )
  ++n;
 check( n == 4 , "convex local pool with bound: m + 1 linearizations" );
 check( coeffs( f ) == PF::RealVector( { 0 , 0 } ) &&
	eq( f.get_linearization_constant() , -10 ) ,
	"convex local pool with bound: the last one is the flat bound" );
 }

/*--------------------------------------------------------------------------*/

static void test_coefficient_getters( void )
{
 // the four getters of the coefficients return the parts of l = ( 1 , 2 , 3 )
 // that the contract of C05Function asks for
 std::vector< ColVariable > x( 3 );
 PF f( { & x[ 0 ] , & x[ 1 ] , & x[ 2 ] } , { { 1 , 2 , 3 } } , { 0 } );
 f.compute();

 // range, array: g[ i - range.first ] = l[ i ]
 {
  FV g[ 2 ] = { NAN , NAN };
  f.get_linearization_coefficients( g , PF::Range( 1 , 3 ) );
  check( ( g[ 0 ] == 2 ) && ( g[ 1 ] == 3 ) ,
	 "get_linearization_coefficients( FunctionValue * , Range( 1 , 3 ) )"
	 ": got ( " + num( g[ 0 ] ) + " , " +
	 num( g[ 1 ] ) + " ), expected ( 2 , 3 )" );
  }

 // range, SparseVector: g[ i ] = l[ i ] for the i in the range
 {
  PF::SparseVector g;
  f.get_linearization_coefficients( g , PF::Range( 1 , 3 ) );
  check( ( g.coeff( 0 ) == 0 ) && ( g.coeff( 1 ) == 2 ) &&
	 ( g.coeff( 2 ) == 3 ) ,
	 "get_linearization_coefficients( SparseVector , Range( 1 , 3 ) )" );
  }

 // subset, array: g[ k ] = l[ subset[ k ] ]
 {
  FV g[ 3 ] = { NAN , NAN , NAN };
  f.get_linearization_coefficients( g , PF::Subset( { 2 , 0 } ) );
  check( ( g[ 0 ] == 3 ) && ( g[ 1 ] == 1 ) ,
	 "get_linearization_coefficients( FunctionValue * , { 2 , 0 } ): "
	 "got g = ( " + num( g[ 0 ] ) + " , " +
	 num( g[ 1 ] ) + " , " + num( g[ 2 ] ) +
	 " ), expected g[ 0 ] = l[ 2 ] = 3 , g[ 1 ] = l[ 0 ] = 1" );
  }

 // subset, SparseVector: g[ j ] = l[ j ] for the j in the subset
 {
  PF::SparseVector g;
  f.get_linearization_coefficients( g , PF::Subset( { 0 , 2 } ) , true );
  check( ( g.coeff( 0 ) == 1 ) && ( g.coeff( 1 ) == 0 ) &&
	 ( g.coeff( 2 ) == 3 ) ,
	 "get_linearization_coefficients( SparseVector , { 0 , 2 } ): got "
	 "( " + num( g.coeff( 0 ) ) + " , " +
	 num( g.coeff( 1 ) ) + " , " +
	 num( g.coeff( 2 ) ) + " ), expected ( 1 , 0 , 3 )" );
  }
 }

/*--------------------------------------------------------------------------*/

static void test_empty_and_bound( void )
{
 // with no rows the value is the bound, or + INF (convex) / - INF (concave)
 // if there is none, and in that case there is no linearization
 std::vector< ColVariable > x( 2 );
 x[ 0 ].set_value( 7 );
 x[ 1 ].set_value( -3 );

 PF cvx( { & x[ 0 ] , & x[ 1 ] } );
 cvx.compute();
 check( cvx.get_value() == INF , "empty convex: value is + INF" );
 check( ! cvx.is_bound_set() , "empty convex: no bound" );
 check( ! cvx.has_linearization() , "empty convex: no linearization" );
 check( cvx.get_global_lower_bound() == - INF ,
	"empty convex: global lower bound" );

 PF ccv( { & x[ 0 ] , & x[ 1 ] } , {} , {} , INF , false );
 ccv.compute();
 check( ccv.get_value() == - INF , "empty concave: value is - INF" );
 check( ! ccv.has_linearization() , "empty concave: no linearization" );

 PF bcvx( { & x[ 0 ] , & x[ 1 ] } , {} , {} , 3 );
 bcvx.compute();
 check( bcvx.get_value() == 3 , "bound-only convex: value is the bound" );
 check( bcvx.is_bound_set() && ( bcvx.get_global_lower_bound() == 3 ) &&
	( bcvx.get_global_upper_bound() == INF ) ,
	"bound-only convex: global bounds" );
 if( bcvx.has_linearization() )
  check( ( coeffs( bcvx ) == PF::RealVector( { 0 , 0 } ) ) &&
	 ( bcvx.get_linearization_constant() == 3 ) ,
	 "bound-only convex: the linearization is the flat one" );

 PF bccv( { & x[ 0 ] , & x[ 1 ] } , {} , {} , -2 , false );
 bccv.compute();
 check( bccv.get_value() == -2 , "bound-only concave: value is the bound" );
 check( bccv.get_global_upper_bound() == -2 ,
	"bound-only concave: global upper bound" );

 // with rows, the bound wins where it is above (convex) all of them
 x[ 0 ].set_value( -2 );
 x[ 1 ].set_value( 0 );  // rows give - 2, - 5, 1
 PF f( { & x[ 0 ] , & x[ 1 ] } , rowsA() , rowsb() , 10 );
 f.compute();
 check( f.get_value() == 10 , "convex with bound: the bound is the max" );

 // a bound of the wrong infinity is refused
 check( throws( [ & ]() { PF g( {} , {} , {} , INF , true ); } ) ,
	"convex with bound + INF is refused" );
 check( throws( [ & ]() { f.modify_bound( INF ); } ) ,
	"modify_bound( + INF ) of a convex function is refused" );
 }

/*--------------------------------------------------------------------------*/

static void test_AAccMlt( void )
{
 // a new function has the default dblAAccMlt of C05Function, whatever was
 // in the memory it is built in: the memory is filled with 0xFF first, the
 // bytes of a NaN
 alignas( PF ) unsigned char buffer[ sizeof( PF ) ];
 std::memset( buffer , 0xFF , sizeof( buffer ) );
 auto f = new( buffer ) PF();
 const double got = f->get_dbl_par( PF::dblAAccMlt );
 const double dflt = f->get_dflt_dbl_par( PF::dblAAccMlt );
 check( got == dflt , "a new PolyhedralFunction has dblAAccMlt = " +
	num( got ) + ", expected the default " +
	num( dflt ) );
 f->~PF();
 }

/*--------------------------------------------------------------------------*/

static void test_add_rows_Mod( void )
{
 // add_row() and add_rows() issue a PolyhedralFunctionModAddd with the
 // number of rows, NothingChanged and a shift of + INF (convex) / - INF
 Model m( { { 1 , 0 } } , { 0 } );

 m.f->add_row( { 0 , 1 } , 2 );
 auto mod = m.only< PolyhedralFunctionModAddd >();
 check( mod && ( mod->addedrows() == 1 ) &&
	( mod->type() == C05FunctionMod::NothingChanged ) &&
	( mod->shift() == FunctionMod::INFshift ) &&
	( mod->function() == m.f ) , "add_row: PolyhedralFunctionModAddd" );
 check( ( m.f->get_nrows() == 2 ) && ( m.f->get_b()[ 1 ] == 2 ) ,
	"add_row: data" );

 m.mods().clear();
 m.f->add_rows( { { 1 , 1 } , { -1 , 1 } } , { 3 , 4 } );
 mod = m.only< PolyhedralFunctionModAddd >();
 check( mod && ( mod->addedrows() == 2 ) , "add_rows: addedrows() == 2" );
 check( ( m.f->get_nrows() == 4 ) &&
	( m.f->get_A()[ 3 ] == PF::RealVector( { -1 , 1 } ) ) ,
	"add_rows: data" );

 // the value follows: at ( 1 , 1 ) the rows give 1, 3, 5, 4
 ( *m.x )[ 0 ].set_value( 1 );
 ( *m.x )[ 1 ].set_value( 1 );
 m.f->compute();
 check( m.f->get_value() == 5 , "add_rows: value" );

 // a row of the wrong size is refused
 check( throws( [ & ]() { m.f->add_row( { 1 } , 0 ); } ) ,
	"add_row of the wrong size is refused" );

 m.mods().clear();
 m.f->add_row( { 2 , 2 } , 0 , eNoMod );
 m.f->add_rows( { { 2 , 3 } } , { 0 } , eNoMod );
 check( m.mods().empty() && ( m.f->get_nrows() == 6 ) ,
	"add_row[s] with eNoMod: done, silently" );

 // concave: the shift is - INF
 Model n( { { 1 , 0 } } , { 0 } , INF , false );
 n.f->add_row( { 0 , 1 } , 2 );
 mod = n.only< PolyhedralFunctionModAddd >();
 check( mod && ( mod->shift() == - FunctionMod::INFshift ) ,
	"concave add_row: shift - INF" );
 }

/*--------------------------------------------------------------------------*/

static void test_delete_rows_Mod( void )
{
 // delete_row[s] issue a PolyhedralFunctionMod[Rngd/Sbst] with DeleteRows,
 // the rows deleted, and a shift of - INF (convex) / + INF (concave)
 PF::MultiVector A = { { 1 , 0 } , { 0 , 1 } , { 1 , 1 } , { 2 , 0 } ,
		       { 0 , 2 } };
 Model m( std::move( A ) , { 0 , 1 , 2 , 3 , 4 } );

 m.f->delete_row( 1 );
 auto rmod = m.only< PolyhedralFunctionModRngd >();
 check( rmod && ( rmod->PFtype() == PolyhedralFunctionMod::DeleteRows ) &&
	( rmod->range() == PF::Range( 1 , 2 ) ) &&
	( rmod->shift() == - FunctionMod::INFshift ) ,
	"delete_row: PolyhedralFunctionModRngd" );
 check( m.f->get_b() == PF::RealVector( { 0 , 2 , 3 , 4 } ) ,
	"delete_row: data" );

 m.mods().clear();
 m.f->delete_rows( PF::Range( 0 , 2 ) );
 rmod = m.only< PolyhedralFunctionModRngd >();
 check( rmod && ( rmod->PFtype() == PolyhedralFunctionMod::DeleteRows ) &&
	( rmod->range() == PF::Range( 0 , 2 ) ) ,
	"delete_rows( Range ): PolyhedralFunctionModRngd" );
 check( m.f->get_b() == PF::RealVector( { 3 , 4 } ) ,
	"delete_rows( Range ): data" );

 m.f->add_rows( { { 5 , 5 } , { 6 , 6 } } , { 5 , 6 } , eNoMod );
 m.mods().clear();
 m.f->delete_rows( PF::Subset( { 3 , 0 } ) , false );  // unordered
 auto smod = m.only< PolyhedralFunctionModSbst >();
 check( smod && ( smod->PFtype() == PolyhedralFunctionMod::DeleteRows ) &&
	( smod->rows() == PF::Subset( { 0 , 3 } ) ) &&
	( smod->shift() == - FunctionMod::INFshift ) ,
	"delete_rows( Subset ): PolyhedralFunctionModSbst, rows ordered" );
 check( m.f->get_b() == PF::RealVector( { 4 , 5 } ) &&
	( m.f->get_A()[ 0 ] == PF::RealVector( { 0 , 2 } ) ) ,
	"delete_rows( Subset ): data" );

 // an empty Subset deletes nothing
 m.mods().clear();
 m.f->delete_rows( PF::Subset() );
 check( m.mods().empty() && ( m.f->get_nrows() == 2 ) ,
	"delete_rows( {} ): nothing deleted, nothing issued" );

 // eNoMod
 m.f->delete_row( 0 , eNoMod );
 check( m.mods().empty() && ( m.f->get_nrows() == 1 ) ,
	"delete_row with eNoMod: done, silently" );

 // delete_rows() of all issues the "everything changed" FunctionMod, and
 // resets the bound
 m.f->modify_bound( -7 , eNoMod );
 m.f->delete_rows();
 auto fmod = m.only< FunctionMod >();
 check( fmod && std::isnan( fmod->shift() ) &&
	( ! std::dynamic_pointer_cast< C05FunctionMod >( fmod ) ) ,
	"delete_rows(): FunctionMod with NaN shift" );
 check( ( m.f->get_nrows() == 0 ) && ( ! m.f->is_bound_set() ) ,
	"delete_rows(): no row and no bound left" );

 // concave: the shift is + INF
 Model n( { { 1 , 0 } , { 0 , 1 } } , { 0 , 1 } , INF , false );
 n.f->delete_row( 0 );
 rmod = n.only< PolyhedralFunctionModRngd >();
 check( rmod && ( rmod->shift() == FunctionMod::INFshift ) ,
	"concave delete_row: shift + INF" );
 }

/*--------------------------------------------------------------------------*/

static void test_delete_bound_row( void )
{
 // in delete_rows( Subset ), the index get_nrows() is the "virtual" all-0
 // row of the global bound: deleting it resets the bound to - INF
 Model m( { { 1 , 0 } , { 0 , 1 } } , { 0 , 1 } , 5 );
 bool thrown = false;
 try {
  m.f->delete_rows( PF::Subset( { 1 , 2 } ) , true );
  }
 catch( std::exception & e ) {
  thrown = true;
  check( false , std::string( "delete_rows( { 1 , get_nrows() } ) throws: " )
	 + e.what() );
  }
 if( ! thrown )
  check( ( m.f->get_nrows() == 1 ) && ( ! m.f->is_bound_set() ) ,
	 "delete_rows( { 1 , get_nrows() } ): row 1 and the bound deleted" );
 }

/*--------------------------------------------------------------------------*/

static void test_modify_rows_Mod( void )
{
 // modify_row[s] issue a PolyhedralFunctionMod[Rngd/Sbst] with ModifyRows
 // and a NaN shift, and the new rows land where the indices say
 Model m( { { 1 , 0 } , { 0 , 1 } , { 1 , 1 } } , { 0 , 1 , 2 } );

 m.f->modify_row( 1 , { 5 , 5 } , 9 );
 auto rmod = m.only< PolyhedralFunctionModRngd >();
 check( rmod && ( rmod->PFtype() == PolyhedralFunctionMod::ModifyRows ) &&
	( rmod->range() == PF::Range( 1 , 2 ) ) && std::isnan( rmod->shift() ) ,
	"modify_row: PolyhedralFunctionModRngd" );
 check( ( m.f->get_A()[ 1 ] == PF::RealVector( { 5 , 5 } ) ) &&
	( m.f->get_b()[ 1 ] == 9 ) , "modify_row: data" );

 m.mods().clear();
 m.f->modify_rows( { { 7 , 0 } , { 8 , 0 } } , { 70 , 80 } ,
		   PF::Range( 0 , 2 ) );
 rmod = m.only< PolyhedralFunctionModRngd >();
 check( rmod && ( rmod->PFtype() == PolyhedralFunctionMod::ModifyRows ) &&
	( rmod->range() == PF::Range( 0 , 2 ) ) ,
	"modify_rows( Range ): PolyhedralFunctionModRngd" );
 check( ( m.f->get_b() == PF::RealVector( { 70 , 80 , 2 } ) ) &&
	( m.f->get_A()[ 1 ] == PF::RealVector( { 8 , 0 } ) ) ,
	"modify_rows( Range ): data" );

 // an unordered Subset: nA[ i ] and nb[ i ] go to row rows[ i ]
 m.mods().clear();
 m.f->modify_rows( { { 2 , 2 } , { 0 , 0 } } , { 22 , 0 } ,
		   PF::Subset( { 2 , 0 } ) , false );
 auto smod = m.only< PolyhedralFunctionModSbst >();
 check( smod && ( smod->PFtype() == PolyhedralFunctionMod::ModifyRows ) &&
	( smod->rows() == PF::Subset( { 0 , 2 } ) ) ,
	"modify_rows( Subset ): PolyhedralFunctionModSbst, rows ordered" );
 check( ( m.f->get_A()[ 2 ] == PF::RealVector( { 2 , 2 } ) ) &&
	( m.f->get_b()[ 2 ] == 22 ) ,
	"modify_rows( nA , nb , { 2 , 0 } , false ): row 2 should be "
	"( 2 , 2 ) + 22, got ( " + num( m.f->get_A()[ 2 ][ 0 ] ) +
	" , " + num( m.f->get_A()[ 2 ][ 1 ] ) + " ) + " +
	num( m.f->get_b()[ 2 ] ) );
 check( ( m.f->get_A()[ 0 ] == PF::RealVector( { 0 , 0 } ) ) &&
	( m.f->get_b()[ 0 ] == 0 ) ,
	"modify_rows( nA , nb , { 2 , 0 } , false ): row 0 should be "
	"( 0 , 0 ) + 0, got ( " + num( m.f->get_A()[ 0 ][ 0 ] ) +
	" , " + num( m.f->get_A()[ 0 ][ 1 ] ) + " ) + " +
	num( m.f->get_b()[ 0 ] ) );

 m.mods().clear();
 m.f->modify_row( 0 , { 1 , 1 } , 1 , eNoMod );
 m.f->modify_rows( { { 1 , 1 } } , { 1 } , PF::Range( 1 , 2 ) , eNoMod );
 check( m.mods().empty() && ( m.f->get_b()[ 1 ] == 1 ) ,
	"modify_row[s] with eNoMod: done, silently" );
 }

/*--------------------------------------------------------------------------*/

static void test_modify_constants_Mod( void )
{
 // modify_constant[s] issue a PolyhedralFunctionMod[Rngd/Sbst] with
 // ModifyCnst, and a shift of + INF if all the constants grow, - INF if all
 // decrease, NaN otherwise; nothing if nothing changes
 Model m( { { 1 , 0 } , { 0 , 1 } , { 1 , 1 } } , { 0 , 1 , 2 } );

 m.f->modify_constant( 1 , 5 );
 auto rmod = m.only< PolyhedralFunctionModRngd >();
 check( rmod && ( rmod->PFtype() == PolyhedralFunctionMod::ModifyCnst ) &&
	( rmod->range() == PF::Range( 1 , 2 ) ) &&
	( rmod->shift() == FunctionMod::INFshift ) ,
	"modify_constant up: PolyhedralFunctionModRngd, shift + INF" );

 m.mods().clear();
 m.f->modify_constant( 1 , 4 );
 rmod = m.only< PolyhedralFunctionModRngd >();
 check( rmod && ( rmod->shift() == - FunctionMod::INFshift ) ,
	"modify_constant down: shift - INF" );

 m.mods().clear();
 m.f->modify_constant( 1 , 4 );
 check( m.mods().empty() , "modify_constant to the same value: nothing" );

 m.f->modify_constants( { 10 , 11 } , PF::Range( 0 , 2 ) );
 rmod = m.only< PolyhedralFunctionModRngd >();
 check( rmod && ( rmod->PFtype() == PolyhedralFunctionMod::ModifyCnst ) &&
	( rmod->range() == PF::Range( 0 , 2 ) ) &&
	( rmod->shift() == FunctionMod::INFshift ) ,
	"modify_constants( Range ) all up: shift + INF" );
 check( m.f->get_b() == PF::RealVector( { 10 , 11 , 2 } ) ,
	"modify_constants( Range ): data" );

 m.mods().clear();
 m.f->modify_constants( { 12 , 0 } , PF::Range( 0 , 2 ) );
 rmod = m.only< PolyhedralFunctionModRngd >();
 check( rmod && std::isnan( rmod->shift() ) ,
	"modify_constants( Range ) mixed: shift NaN" );

 m.mods().clear();
 m.f->modify_constants( { 12 , 0 } , PF::Range( 0 , 2 ) );
 check( m.mods().empty() ,
	"modify_constants( Range ) to the same values: nothing" );

 // an unordered Subset: nb[ i ] goes to row rows[ i ]
 m.f->modify_constants( { 22 , 20 } , PF::Subset( { 2 , 0 } ) , false );
 auto smod = m.only< PolyhedralFunctionModSbst >();
 check( smod && ( smod->PFtype() == PolyhedralFunctionMod::ModifyCnst ) &&
	( smod->rows() == PF::Subset( { 0 , 2 } ) ) &&
	( smod->shift() == FunctionMod::INFshift ) ,
	"modify_constants( Subset ): PolyhedralFunctionModSbst" );
 check( ( m.f->get_b()[ 0 ] == 20 ) && ( m.f->get_b()[ 2 ] == 22 ) ,
	"modify_constants( { 22 , 20 } , { 2 , 0 } , false ): expected "
	"b[ 0 ] = 20 , b[ 2 ] = 22, got b[ 0 ] = " +
	num( m.f->get_b()[ 0 ] ) + " , b[ 2 ] = " +
	num( m.f->get_b()[ 2 ] ) );

 m.mods().clear();
 m.f->modify_constant( 0 , -1 , eNoMod );
 m.f->modify_constants( { -1 } , PF::Range( 1 , 2 ) , eNoMod );
 m.f->modify_constants( { -1 } , PF::Subset( { 2 } ) , true , eNoMod );
 check( m.mods().empty() &&
	( m.f->get_b() == PF::RealVector( { -1 , -1 , -1 } ) ) ,
	"modify_constant[s] with eNoMod: done, silently" );
 }

/*--------------------------------------------------------------------------*/

static void test_modify_bound_Mod( void )
{
 // modify_bound() issues a PolyhedralFunctionModRngd with the empty Range
 // < 0 , 0 >, ModifyCnst and a shift of + INF / - INF as the bound moves
 Model m( { { 1 , 0 } } , { 0 } );

 m.f->modify_bound( 3 );
 auto rmod = m.last< PolyhedralFunctionModRngd >();
 check( rmod && ( rmod->PFtype() == PolyhedralFunctionMod::ModifyCnst ) &&
	( rmod->range() == PF::Range( 0 , 0 ) ) &&
	( rmod->shift() == FunctionMod::INFshift ) ,
	"modify_bound up: PolyhedralFunctionModRngd < 0 , 0 >, + INF" );
 check( m.f->get_global_bound() == 3 , "modify_bound: data" );

 m.mods().clear();
 m.f->modify_bound( 1 );
 rmod = m.last< PolyhedralFunctionModRngd >();
 check( rmod && ( rmod->shift() == - FunctionMod::INFshift ) ,
	"modify_bound down: shift - INF" );

 m.mods().clear();
 m.f->modify_bound( 1 );
 check( m.mods().empty() , "modify_bound to the same value: nothing" );

 m.f->modify_bound( - INF , eNoMod );
 check( m.mods().empty() && ( ! m.f->is_bound_set() ) ,
	"modify_bound with eNoMod: done, silently" );
 }

/*--------------------------------------------------------------------------*/

static void test_global_pool_Mod( void )
{
 // deleting or modifying a row in the global pool says so in which(), and
 // the deleted one leaves the pool; the others are renamed in place
 Model m( { { 1 , 0 } , { 0 , 1 } , { -1 , 0 } } , { 0 , 0 , 0 } );
 m.f->set_par( PF::intGPMaxSz , 3 );
 auto & x = *m.x;

 x[ 0 ].set_value( 1 );  // row 0 is the max
 x[ 1 ].set_value( 0 );
 m.f->compute();
 m.f->store_linearization( 0 );
 x[ 0 ].set_value( 0 );  // row 1 is the max
 x[ 1 ].set_value( 1 );
 m.f->compute();
 m.f->store_linearization( 1 );
 x[ 0 ].set_value( -1 );  // row 2 is the max
 x[ 1 ].set_value( 0 );
 m.f->compute();
 m.f->store_linearization( 2 );

 m.mods().clear();
 m.f->modify_row( 1 , { 0 , 2 } , 1 );
 auto rmod = m.only< PolyhedralFunctionModRngd >();
 check( rmod && ( rmod->type() == C05FunctionMod::AllLinearizationChanged ) &&
	( rmod->which() == PF::Subset( { 1 } ) ) ,
	"modify_row of a stored row: AllLinearizationChanged, which { 1 }" );
 check( ( coeffs( *m.f , 1 ) == PF::RealVector( { 0 , 2 } ) ) &&
	( m.f->get_linearization_constant( 1 ) == 1 ) ,
	"modify_row of a stored row: the stored one follows" );

 m.mods().clear();
 m.f->modify_constant( 2 , 5 );
 rmod = m.only< PolyhedralFunctionModRngd >();
 check( rmod && ( rmod->type() == C05FunctionMod::AlphaChanged ) &&
	( rmod->which() == PF::Subset( { 2 } ) ) ,
	"modify_constant of a stored row: AlphaChanged, which { 2 }" );

 m.mods().clear();
 m.f->delete_row( 0 );
 rmod = m.only< PolyhedralFunctionModRngd >();
 check( rmod && ( rmod->which() == PF::Subset( { 0 } ) ) ,
	"delete_row of a stored row: which { 0 }" );
 check( ! m.f->is_linearization_there( 0 ) ,
	"delete_row of a stored row: it leaves the global pool" );
 check( m.f->is_linearization_there( 1 ) &&
	( coeffs( *m.f , 1 ) == PF::RealVector( { 0 , 2 } ) ) &&
	( m.f->get_linearization_constant( 2 ) == 5 ) ,
	"delete_row: the other stored rows are still themselves" );
 }

/*--------------------------------------------------------------------------*/

static void test_variables_Mod( void )
{
 // add_variable() issues a C05FunctionModVarsAddd, remove_variable[s] a
 // C05FunctionModVars[Rngd/Sbst], all with shift 0, and the FRealObjective
 // follows the active Variable
 Model m( { { 1 , 2 } , { 3 , 4 } } , { 0 , 1 } );
 auto & x = *m.x;

 m.f->add_variable( & x[ 2 ] , { 5 , 6 } );
 auto amod = m.only< C05FunctionModVarsAddd >();
 check( amod && ( amod->first() == 2 ) && ( amod->shift() == 0 ) &&
	( amod->vars().size() == 1 ) && ( amod->vars()[ 0 ] == & x[ 2 ] ) ,
	"add_variable: C05FunctionModVarsAddd" );
 check( ( m.f->get_num_active_var() == 3 ) &&
	( m.f->get_A()[ 1 ] == PF::RealVector( { 3 , 4 , 6 } ) ) ,
	"add_variable: data" );
 check( x[ 2 ].is_active( m.obj ) < x[ 2 ].get_num_active() ,
	"add_variable: the FRealObjective is active in the new Variable" );

 m.mods().clear();
 m.f->remove_variable( 0 );
 auto rmod = m.only< C05FunctionModVarsRngd >();
 check( rmod && ( rmod->range() == PF::Range( 0 , 1 ) ) &&
	( rmod->shift() == 0 ) && ( rmod->vars()[ 0 ] == & x[ 0 ] ) ,
	"remove_variable: C05FunctionModVarsRngd" );
 check( ( m.f->get_num_active_var() == 2 ) &&
	( m.f->get_active_var( 0 ) == & x[ 1 ] ) &&
	( m.f->get_A()[ 0 ] == PF::RealVector( { 2 , 5 } ) ) ,
	"remove_variable: data" );
 check( x[ 0 ].get_num_active() == 0 ,
	"remove_variable: the FRealObjective left the Variable" );

 // a Range that is all of them empties the columns but keeps b
 m.mods().clear();
 m.f->remove_variables( PF::Range( 0 , 2 ) );
 rmod = m.only< C05FunctionModVarsRngd >();
 check( rmod && ( rmod->range() == PF::Range( 0 , 2 ) ) &&
	( rmod->vars().size() == 2 ) ,
	"remove_variables( all Range ): C05FunctionModVarsRngd" );
 check( ( m.f->get_num_active_var() == 0 ) && ( m.f->get_nrows() == 2 ) &&
	m.f->get_A()[ 0 ].empty() &&
	( m.f->get_b() == PF::RealVector( { 0 , 1 } ) ) ,
	"remove_variables( all Range ): no columns, b kept" );
 m.f->compute();
 check( m.f->get_value() == 1 ,
	"no Variable left: the value is the largest constant" );

 m.mods().clear();
 m.f->add_variable( & x[ 0 ] , { 1 , 1 } , eNoMod );
 m.f->remove_variable( 0 , eNoMod );
 check( m.mods().empty() && ( m.f->get_num_active_var() == 0 ) ,
	"add_variable / remove_variable with eNoMod: done, silently" );
 }

/*--------------------------------------------------------------------------*/

static void test_remove_last_variable( void )
{
 // removing the only Variable leaves rows of 0 columns and the constants
 Model m( { { 2 } , { -1 } } , { 1 , 3 } , - INF , true , 1 );
 m.f->remove_variable( 0 );
 auto rmod = m.only< C05FunctionModVarsRngd >();
 check( rmod && ( rmod->range() == PF::Range( 0 , 1 ) ) ,
	"remove last variable: C05FunctionModVarsRngd" );
 check( ( m.f->get_num_active_var() == 0 ) && ( m.f->get_nrows() == 2 ) &&
	m.f->get_A()[ 1 ].empty() , "remove last variable: data" );
 m.f->compute();
 check( m.f->get_value() == 3 , "remove last variable: value" );
 }

/*--------------------------------------------------------------------------*/

static void test_remove_variables_subset( void )
{
 // an empty Subset removes all the Variable, i.e., resets A but not b nor
 // the bound, and issues a C05FunctionModVarsSbst with all the Variable
 {
  Model m( { { 1 , 2 , 3 } , { 4 , 5 , 6 } } , { 7 , 8 } , -9 , true , 3 );
  m.f->remove_variables( PF::Subset() );
  auto smod = m.only< C05FunctionModVarsSbst >();
  check( smod && ( smod->vars().size() == 3 ) && ( smod->shift() == 0 ) ,
	 "remove_variables( {} ): C05FunctionModVarsSbst of all Variable" );
  check( ( m.f->get_num_active_var() == 0 ) && ( m.f->get_nrows() == 2 ) &&
	 m.f->get_A()[ 0 ].empty() && m.f->get_A()[ 1 ].empty() &&
	 ( m.f->get_b() == PF::RealVector( { 7 , 8 } ) ) &&
	 ( m.f->get_global_bound() == -9 ) ,
	 "remove_variables( {} ): A reset, b and bound kept" );
  }

 // a proper Subset removes those columns
 {
  std::vector< ColVariable > x( 3 );
  PF f( { & x[ 0 ] , & x[ 1 ] , & x[ 2 ] } ,
	{ { 1 , 2 , 3 } , { 4 , 5 , 6 } } , { 7 , 8 } );
  f.remove_variables( PF::Subset( { 1 } ) , true );
  check( f.get_num_active_var() == 2 ,
	 "remove_variables( { 1 } ) of 3: " +
	 num( f.get_num_active_var() ) +
	 " active Variable left, expected 2" );
  check( ( f.get_A()[ 0 ].size() == 2 ) && ( f.get_A()[ 0 ][ 1 ] == 3 ) ,
	 "remove_variables( { 1 } ) of 3: row 0 should be ( 1 , 3 ), has " +
	 num( f.get_A()[ 0 ].size() ) + " columns" );
  if( f.get_num_active_var() == 2 )
   check( ( f.get_active_var( 0 ) == & x[ 0 ] ) &&
	  ( f.get_active_var( 1 ) == & x[ 2 ] ) ,
	  "remove_variables( { 1 } ) of 3: x[ 0 ] and x[ 2 ] left" );
  }

 // with the Observer, the Modification tells which ones went
 {
  Model m( { { 1 , 2 , 3 } } , { 0 } , - INF , true , 3 );
  m.f->remove_variables( PF::Subset( { 2 , 0 } ) , false );
  auto smod = m.only< C05FunctionModVarsSbst >();
  check( smod && ( smod->subset() == PF::Subset( { 0 , 2 } ) ) &&
	 ( smod->vars().size() == 2 ) &&
	 ( smod->vars()[ 0 ] == & ( *m.x )[ 0 ] ) ,
	 "remove_variables( { 2 , 0 } ): C05FunctionModVarsSbst" );
  check( ( m.f->get_num_active_var() == 1 ) &&
	 ( m.f->get_A()[ 0 ] == PF::RealVector( { 2 } ) ) ,
	 "remove_variables( { 2 , 0 } ) of 3 with Observer: expected x[ 1 ] "
	 "with coefficient 2 left, got " +
	 num( m.f->get_num_active_var() ) + " active Variable" );
  }
 }

/*--------------------------------------------------------------------------*/

static void test_set_is_convex_Mod( void )
{
 // flipping the verse issues a PolyhedralFunctionMod with NothingChanged
 // and a shift of + INF (to convex) / - INF (to concave), and moves an
 // unset bound to the right infinity
 Model m( { { 1 , 0 } } , { 0 } );
 m.f->set_is_convex( false );
 auto mod = m.only< PolyhedralFunctionMod >();
 check( mod && ( mod->type() == C05FunctionMod::NothingChanged ) &&
	( mod->shift() == - FunctionMod::INFshift ) ,
	"set_is_convex( false ): PolyhedralFunctionMod, - INF" );
 check( m.f->is_concave() && ( m.f->get_global_bound() == INF ) ,
	"set_is_convex( false ): verse and unset bound" );

 m.mods().clear();
 m.f->set_is_convex( false );
 check( m.mods().empty() , "set_is_convex to the same verse: nothing" );
 m.f->set_is_convex( true , eNoMod );
 check( m.mods().empty() && m.f->is_convex() ,
	"set_is_convex with eNoMod: done, silently" );
 }

/*--------------------------------------------------------------------------*/

static void test_State( void )
{
 // the State carries the global pool, original and aggregated
 // linearizations, into another function with the same data, directly and
 // through netCDF
 std::vector< ColVariable > x( 2 );
 PF::VarVector vx = { & x[ 0 ] , & x[ 1 ] };
 PF f( PF::VarVector( vx ) , rowsA() , rowsb() );
 f.set_par( PF::intGPMaxSz , 4 );

 set_x( vx , { 1 , 1 } );   // row 1
 f.compute();
 f.store_linearization( 0 );
 set_x( vx , { -2 , 0 } );  // row 2
 f.compute();
 f.store_linearization( 1 );
 f.store_combination_of_linearizations( { { 0 , 0.5 } , { 1 , 0.5 } } , 3 );
 check( ( coeffs( f , 3 ) == PF::RealVector( { 1 , 0.5 } ) ) &&
	eq( f.get_linearization_constant( 3 ) , 0 ) ,
	"store_combination_of_linearizations: the average of rows 1 and 2" );

 auto compare = [ & ]( PF & g , const std::string & how ) {
  check( g.is_linearization_there( 0 ) && g.is_linearization_there( 1 ) &&
	 ( ! g.is_linearization_there( 2 ) ) && g.is_linearization_there( 3 ) ,
	 how + ": names in the global pool" );
  for( Index n : { 0 , 1 , 3 } )
   check( ( coeffs( g , n ) == coeffs( f , n ) ) &&
	  eq( g.get_linearization_constant( n ) ,
	      f.get_linearization_constant( n ) ) ,
	  how + ": linearization " + num( n ) );
  };

 std::unique_ptr< State > s( f.get_State() );
 check( dynamic_cast< PolyhedralFunctionState * >( s.get() ) ,
	"get_State() is a PolyhedralFunctionState" );
 PF g( PF::VarVector( vx ) , rowsA() , rowsb() );
 g.set_par( PF::intGPMaxSz , 4 );
 g.put_State( *s );
 compare( g , "put_State( const & )" );

 const char * const name = "tests_PolyhedralFunction_state.nc4";
 {
  netCDF::NcFile file( name , netCDF::NcFile::replace );
  auto grp = file.addGroup( "State" );
  f.serialize_State( grp );
  }
 std::unique_ptr< State > r;
 {
  netCDF::NcFile file( name , netCDF::NcFile::read );
  r.reset( State::new_State( file.getGroup( "State" ) ) );
  }
 std::remove( name );
 check( dynamic_cast< PolyhedralFunctionState * >( r.get() ) ,
	"serialize_State(): read back as a PolyhedralFunctionState" );
 if( r ) {
  PF h( PF::VarVector( vx ) , rowsA() , rowsb() );
  h.set_par( PF::intGPMaxSz , 4 );
  h.put_State( std::move( *r ) );
  compare( h , "serialize_State() + put_State( && )" );
  }

 // a State of the wrong number of Variable is refused
 PF k( { & x[ 0 ] } , { { 1 } } , { 0 } );
 check( throws( [ & ]() { k.put_State( *s ); } ) ,
	"put_State with the wrong number of Variable is refused" );

 // after put_State() the pool is the one of the State for everything that
 // follows: deleting the row behind name 0 takes it out of the pool
 g.delete_row( 1 );
 check( ! g.is_linearization_there( 0 ) ,
	"put_State() then delete_row( 1 ): name 0 (row 1) should leave the "
	"global pool" );
 }

/*--------------------------------------------------------------------------*/

static void test_State_smaller_pool( void )
{
 // a State of a smaller global pool removes the names it does not have, as
 // the GlobalPoolRemoved Modification issued by put_State() says
 Model m( rowsA() , rowsb() );
 m.f->set_par( PF::intGPMaxSz , 3 );
 ( *m.x )[ 0 ].set_value( 1 );
 ( *m.x )[ 1 ].set_value( 1 );
 m.f->compute();
 m.f->store_linearization( 2 , eNoMod );

 std::vector< ColVariable > x( 2 );
 PF f( { & x[ 0 ] , & x[ 1 ] } , rowsA() , rowsb() );
 f.set_par( PF::intGPMaxSz , 1 );
 std::unique_ptr< State > s( f.get_State() );

 m.mods().clear();
 m.f->put_State( *s );
 auto mod = m.only< PolyhedralFunctionMod >();
 check( mod && ( mod->type() == C05FunctionMod::GlobalPoolRemoved ) &&
	( mod->which() == PF::Subset( { 2 } ) ) ,
	"put_State of a smaller pool: GlobalPoolRemoved { 2 }" );
 check( ! m.f->is_linearization_there( 2 ) ,
	"put_State of a smaller pool: name 2 reported removed but still in "
	"the global pool" );
 }

/*--------------------------------------------------------------------------*/

static void test_netCDF( void )
{
 // serialize() and deserialize() of an empty function, of a bound-only one
 // and of a concave one with rows
 const char * const name = "tests_PolyhedralFunction.nc4";
 std::vector< ColVariable > x( 2 );

 auto round = [ & ]( PF & f ) {
  {
   netCDF::NcFile file( name , netCDF::NcFile::replace );
   auto grp = file.addGroup( "PF" );
   f.serialize( grp );
   }
  auto g = std::make_unique< PF >();
  {
   netCDF::NcFile file( name , netCDF::NcFile::read );
   g->deserialize( file.getGroup( "PF" ) );
   }
  std::remove( name );
  g->set_variables( { & x[ 0 ] , & x[ 1 ] } );
  return( g );
  };

 PF empty( { & x[ 0 ] , & x[ 1 ] } );
 auto g = round( empty );
 check( ( g->get_nrows() == 0 ) && g->is_convex() &&
	( ! g->is_bound_set() ) , "netCDF: empty function" );
 g->compute();
 check( g->get_value() == INF , "netCDF: empty function, value + INF" );

 PF bound( { & x[ 0 ] , & x[ 1 ] } , {} , {} , 3 );
 g = round( bound );
 check( ( g->get_nrows() == 0 ) && g->is_convex() &&
	( g->get_global_bound() == 3 ) , "netCDF: bound-only function" );
 g->compute();
 check( g->get_value() == 3 , "netCDF: bound-only function, value" );

 PF ccv( { & x[ 0 ] , & x[ 1 ] } , rowsA() , rowsb() , 4 , false );
 g = round( ccv );
 check( g->is_concave() && ( g->get_A() == rowsA() ) &&
	( g->get_b() == rowsb() ) && ( g->get_global_bound() == 4 ) ,
	"netCDF: concave function with rows and bound" );

 PF ccve( { & x[ 0 ] , & x[ 1 ] } , {} , {} , INF , false );
 g = round( ccve );
 check( g->is_concave() && ( ! g->is_bound_set() ) &&
	( g->get_global_bound() == INF ) ,
	"netCDF: empty concave function" );
 }

/*--------------------------------------------------------------------------*/

static void test_remove_parallel_rows( void )
{
 // among parallel rows the dominated ones go: the smaller constants if
 // convex, the larger ones if concave
 Model m( { { 1 , 1 } , { 1 , 1 } , { 1 , 0 } , { 1 , 1 } } ,
	  { 0 , 2 , 0 , 1 } );
 m.f->remove_parallel_rows();
 auto smod = m.last< PolyhedralFunctionModSbst >();
 check( smod && ( smod->PFtype() == PolyhedralFunctionMod::DeleteRows ) &&
	( smod->rows() == PF::Subset( { 0 , 3 } ) ) ,
	"remove_parallel_rows convex: rows 0 and 3 deleted" );
 check( ( m.f->get_nrows() == 2 ) &&
	( m.f->get_b() == PF::RealVector( { 2 , 0 } ) ) ,
	"remove_parallel_rows convex: ( 1 , 1 ) + 2 and ( 1 , 0 ) left" );

 std::vector< ColVariable > x( 2 );
 PF f( { & x[ 0 ] , & x[ 1 ] } , { { 1 , 1 } , { 1 , 1 } , { 1 , 1.5 } } ,
       { 0 , 2 , 5 } , INF , false );
 f.remove_parallel_rows();
 check( ( f.get_nrows() == 2 ) && ( f.get_b() == PF::RealVector( { 0 , 5 } ) ),
	"remove_parallel_rows concave: the larger constant goes" );
 f.remove_parallel_rows( 0.5 , 1 );
 check( ( f.get_nrows() == 1 ) && ( f.get_b()[ 0 ] == 0 ) ,
	"remove_parallel_rows with tolerance 0.5: ( 1 , 1.5 ) is parallel" );
 }

/*--------------------------------------------------------------------------*/

static void test_R3_copy( void )
{
 // the copy R3 Block has the same rows, constants, bound and verse
 PolyhedralFunctionBlock pfb;
 pfb.get_PolyhedralFunction().set_PolyhedralFunction( rowsA() , rowsb() , 4 ,
						       false , eNoMod );
 std::unique_ptr< Block > r3( pfb.get_R3_Block() );
 auto copy = dynamic_cast< PolyhedralFunctionBlock * >( r3.get() );
 check( copy , "get_R3_Block(): a PolyhedralFunctionBlock" );
 if( ! copy )
  return;
 auto & g = copy->get_PolyhedralFunction();
 check( ( g.get_A() == rowsA() ) && ( g.get_b() == rowsb() ) &&
	( g.get_global_bound() == 4 ) && g.is_concave() ,
	"get_R3_Block(): same data" );
 check( & g != & pfb.get_PolyhedralFunction() ,
	"get_R3_Block(): a PolyhedralFunction of its own" );
 }

/*--------------------------------------------------------------------------*/
/// the father of a PolyhedralFunctionBlock, holding the Variable x

struct Father {
 AbstractBlock block;
 std::vector< ColVariable > x;
 PolyhedralFunctionBlock * pfb;

 Father( void ) : x( 2 ) {
  block.add_static_variable( x , "x" );
  pfb = new PolyhedralFunctionBlock( & block );
  block.add_nested_Block( pfb );
  PF().set_variables( { & x[ 0 ] , & x[ 1 ] } );
  }

 PolyhedralFunction & PF( void ) { return( pfb->get_PolyhedralFunction() ); }
 };

/*--------------------------------------------------------------------------*/
/// the coefficient of var in the LinearFunction of c, NaN if absent

static double coef( Function * fn , Variable * var )
{
 auto lf = dynamic_cast< LinearFunction * >( fn );
 if( ! lf )
  return( NAN );
 const auto k = lf->is_active( var );
 return( k < lf->get_num_active_var() ? lf->get_coefficient( k ) : NAN );
 }

/*--------------------------------------------------------------------------*/
/// true if the row c is b <= v - a x (the primal row of a convex function)

static bool primal_row( FRowConstraint & c , ColVariable * v ,
			std::vector< ColVariable > & x ,
			const PF::RealVector & a , FV b )
{
 auto lf = dynamic_cast< LinearFunction * >( c.get_function() );
 return( lf && ( lf->get_num_active_var() == 3 ) &&
	 ( c.get_lhs() == b ) && ( c.get_rhs() == INF ) &&
	 ( coef( lf , v ) == 1 ) && ( coef( lf , & x[ 0 ] ) == - a[ 0 ] ) &&
	 ( coef( lf , & x[ 1 ] ) == - a[ 1 ] ) );
 }

/*--------------------------------------------------------------------------*/

static void test_PFB_primal( void )
{
 // the primal representation of an empty function is v alone, free, with
 // no rows; rows added and deleted in the function are added and deleted
 // in the representation, and the bound goes on the box of v
 Father fa;
 SimpleConfiguration< int > cfg( 1 );
 fa.pfb->generate_abstract_variables( & cfg );
 fa.pfb->generate_abstract_constraints();
 fa.pfb->generate_objective();

 auto v = fa.pfb->get_static_variable< ColVariable >( 0 );
 check( v && ( v == fa.pfb->get_v() ) , "PFB primal: v is static group 0" );
 auto rows = fa.pfb->get_dynamic_constraint< FRowConstraint >( 0 );
 check( rows && rows->empty() , "PFB primal empty: no rows" );
 auto box = fa.pfb->get_static_constraint< BoxConstraint >( 0 );
 check( box && ( box->get_lhs() == - INF ) && ( box->get_rhs() == INF ) ,
	"PFB primal empty: v is free" );
 auto obj = dynamic_cast< FRealObjective * >( fa.pfb->get_objective() );
 check( obj && ( obj->get_sense() == Objective::eMin ) &&
	( coef( obj->get_function() , v ) == 1 ) ,
	"PFB primal: min v" );
 if( ! ( v && rows && box ) )
  return;

 fa.PF().add_row( { 1 , 2 } , 3 );
 check( ( rows->size() == 1 ) &&
	primal_row( rows->front() , v , fa.x , { 1 , 2 } , 3 ) ,
	"PFB primal add_row: 3 <= v - x0 - 2 x1" );

 fa.PF().add_rows( { { 3 , 1 } , { 0 , -1 } } , { -1 , 2 } );
 check( ( rows->size() == 3 ) &&
	primal_row( rows->back() , v , fa.x , { 0 , -1 } , 2 ) ,
	"PFB primal add_rows: 3 rows, the last 2 <= v + x1" );

 fa.PF().delete_row( 0 );
 check( ( rows->size() == 2 ) &&
	primal_row( rows->front() , v , fa.x , { 3 , 1 } , -1 ) ,
	"PFB primal delete_row( 0 ): the first row is now - 1 <= v - 3 x0 - x1" );

 fa.PF().modify_constant( 1 , 7 );
 check( ( rows->size() == 2 ) && ( rows->back().get_lhs() == 7 ) ,
	"PFB primal modify_constant: the lhs follows" );

 fa.PF().modify_bound( 4 );
 check( ( box->get_lhs() == 4 ) && ( box->get_rhs() == INF ) ,
	"PFB primal modify_bound: 4 <= v" );

 fa.PF().delete_rows( PF::Range( 0 , 2 ) );
 check( rows->empty() , "PFB primal delete_rows( all Range ): no rows" );
 }

/*--------------------------------------------------------------------------*/

static void test_PFB_dual( void )
{
 // the dual representation of an empty function without bound is gamma
 // fixed to 0, no theta, the normalization gamma = 1 and objective 0 gamma;
 // each row added brings its theta, in the normalization if diagonal, in the
 // objective with its constant, and each row deleted takes it away
 Father fa;
 SimpleConfiguration< int > cfg( 3 );
 fa.pfb->generate_abstract_variables( & cfg );
 fa.pfb->generate_abstract_constraints();
 fa.pfb->generate_objective();

 auto gamma = fa.pfb->get_static_variable< ColVariable >( 0 );
 auto theta = fa.pfb->get_dynamic_variable< ColVariable >( 0 );
 auto norm = fa.pfb->get_static_constraint< FRowConstraint >( 0 );
 auto obj = dynamic_cast< FRealObjective * >( fa.pfb->get_objective() );
 check( gamma && gamma->is_fixed() && ( gamma->get_value() == 0 ) &&
	gamma->is_positive() , "PFB dual empty: gamma fixed to 0" );
 check( theta && theta->empty() , "PFB dual empty: no theta" );
 check( norm && ( norm->get_lhs() == 1 ) && ( norm->get_rhs() == 1 ) &&
	( norm->get_num_active_var() == 1 ) &&
	( coef( norm->get_function() , gamma ) == 1 ) ,
	"PFB dual empty: normalization gamma = 1" );
 check( obj && ( obj->get_sense() == Objective::eMax ) &&
	( obj->get_num_active_var() == 1 ) &&
	( coef( obj->get_function() , gamma ) == 0 ) ,
	"PFB dual empty: max 0 gamma" );
 if( ! ( gamma && theta && norm && obj ) )
  return;

 fa.PF().add_row( { 1 , 2 } , 3 );
 check( ( theta->size() == 1 ) && theta->front().is_positive() ,
	"PFB dual add_row: one theta, non-negative" );
 auto t0 = theta->empty() ? nullptr : & theta->front();
 check( ( norm->get_num_active_var() == 2 ) &&
	( coef( norm->get_function() , t0 ) == 1 ) ,
	"PFB dual add_row: theta in the normalization with 1" );
 check( ( obj->get_num_active_var() == 2 ) &&
	( coef( obj->get_function() , t0 ) == 3 ) ,
	"PFB dual add_row: theta in the objective with b = 3" );

 fa.PF().add_row( { 1 , 0 } , -1 , eModBlck , true );  // vertical
 auto t1 = theta->size() < 2 ? nullptr : & theta->back();
 check( theta->size() == 2 , "PFB dual add vertical row: two theta" );
 check( ( norm->get_num_active_var() == 2 ) &&
	std::isnan( coef( norm->get_function() , t1 ) ) ,
	"PFB dual add vertical row: not in the normalization" );
 check( ( obj->get_num_active_var() == 3 ) &&
	( coef( obj->get_function() , t1 ) == -1 ) ,
	"PFB dual add vertical row: in the objective with b = - 1" );

 fa.PF().delete_row( 0 );
 check( ( theta->size() == 1 ) && ( & theta->front() == t1 ) ,
	"PFB dual delete_row( 0 ): the theta of row 1 is left" );
 check( ( norm->get_num_active_var() == 1 ) &&
	( coef( norm->get_function() , gamma ) == 1 ) ,
	"PFB dual delete_row( 0 ): normalization gamma = 1 again" );
 check( ( obj->get_num_active_var() == 2 ) &&
	( coef( obj->get_function() , t1 ) == -1 ) ,
	"PFB dual delete_row( 0 ): objective gamma and the vertical theta" );

 fa.PF().modify_bound( 5 );
 check( ( ! gamma->is_fixed() ) &&
	( coef( obj->get_function() , gamma ) == 5 ) ,
	"PFB dual modify_bound: gamma free, with 5 in the objective" );

 fa.PF().delete_row( 0 );
 check( theta->empty() && ( obj->get_num_active_var() == 1 ) ,
	"PFB dual delete the last row: gamma alone" );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- MAIN -----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( void )
{
 test_value_convex();
 test_value_concave();
 test_local_pool();
 test_coefficient_getters();
 test_empty_and_bound();
 test_AAccMlt();
 test_add_rows_Mod();
 test_delete_rows_Mod();
 test_delete_bound_row();
 test_modify_rows_Mod();
 test_modify_constants_Mod();
 test_modify_bound_Mod();
 test_global_pool_Mod();
 test_variables_Mod();
 test_remove_last_variable();
 test_remove_variables_subset();
 test_set_is_convex_Mod();
 test_State();
 test_State_smaller_pool();
 test_netCDF();
 test_remove_parallel_rows();
 test_R3_copy();
 test_PFB_primal();
 test_PFB_dual();

 if( n_failed ) {
  std::cout << n_failed << " checks FAILED" << std::endl;
  return( 1 );
  }

 std::cout << "All tests passed!!" << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------- End File tests_PolyhedralFunction.cpp ----------------*/
/*--------------------------------------------------------------------------*/
