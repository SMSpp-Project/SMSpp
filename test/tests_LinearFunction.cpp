/** @file
 * Unit tests for LinearFunction.
 * They test stuff not already tested with Function.
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

#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

#include "AbstractBlock.h"
#include "FRealObjective.h"
#include "FRowConstraint.h"
#include "FakeSolver.h"
#include "LinearFunction.h"
#include "Observer.h"

// the checks compiled into the library headers exist only without NDEBUG
#ifdef NDEBUG
 #define LIB_NDEBUG
#endif

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASSES -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// an Observer that records every Modification it is sent

class Recorder : public Observer
{
 public:

 Block * get_Block( void ) const override { return( nullptr ); }

 bool anyone_there( void ) const override { return( true ); }

 void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override {
  mods.push_back( mod );
  }

 ChnlName open_channel( ChnlName chnl = 0 ,
			GroupModification * gmpmod = nullptr ) override {
  return( 0 );
  }

 void close_channel( ChnlName chnl , bool force = false ) override {}

 void set_default_channel( ChnlName chnl = 0 ) override {}

 Lst_sp_Mod mods;  ///< what has been sent, in order
 };

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

using Index = LinearFunction::Index;
using Range = LinearFunction::Range;
using Subset = LinearFunction::Subset;
using Coefficient = LinearFunction::Coefficient;

/// the generator of the coefficients and values, seeded once and for all
static std::mt19937 rng( 1234 );

static LinearFunction::Coefficient get_random_coeff()
{
 std::uniform_real_distribution< double > unif( -100 , 100 );

 return( unif( rng ) );
}

/*--------------------------------------------------------------------------*/

/// the number of soft checks that failed
static int n_failed = 0;

/// a check that reports what fails and lets the other ones run
/** Used where a failure is a defect of the library that the test documents:
 * it prints what is wrong, and main() returns non-zero at the end. */

static void check( bool ok , const char * what )
{
 if( ok )
  return;
 std::cerr << "LinearFunction_test FAILED: " << what << std::endl;
 ++n_failed;
 }

/*--------------------------------------------------------------------------*/

/// true if calling f() throws an exception of type E

template< class E , class F >
static bool throws( F f )
{
 try {
  f();
  }
 catch( E & ) {
  return( true );
  }
 catch( ... ) {
  return( false );
  }
 return( false );
 }

/*--------------------------------------------------------------------------*/

/// f with the Variable of vars, coefficient i + 1 and value 2 i - 3 each

static void fill( LinearFunction & f , std::vector< ColVariable > & vars )
{
 LinearFunction::v_coeff_pair p( vars.size() );
 for( Index i = 0 ; i < vars.size() ; ++i ) {
  vars[ i ].set_value( 2.0 * i - 3 );
  p[ i ] = { & vars[ i ] , Coefficient( i + 1 ) };
  }
 f.add_variables( std::move( p ) );
 }

/*--------------------------------------------------------------------------*/

/// the value of c + sum_i a_i x_i summed in the order of the Variable

static double value_of( LinearFunction & f )
{
 double v = f.get_constant_term();
 for( Index i = 0 ; i < f.get_num_active_var() ; ++i )
  v += static_cast< ColVariable * >( f.get_active_var( i ) )->get_value() *
       f.get_coefficient( i );
 return( v );
 }

/*--------------------------------------------------------------------------*/
/* Adding and removing single Variable and sets of them, the value being
 * checked at a point where every Variable is non-zero. */

void runAllTests()
{
 // test AddsVariable
 LinearFunction add_fun;
 ColVariable v;
 v.set_value( get_random_coeff() );
 LinearFunction::Coefficient c = get_random_coeff();

 add_fun.add_variable( &v , c );
 assert( add_fun.get_num_active_var() == 1 );
 assert( add_fun.is_active( &v ) == 0 );
 assert( add_fun.get_active_var( 0 ) == &v );
 assert( add_fun.get_coefficient( 0 ) == c );
 assert( add_fun.compute( true ) == LinearFunction::kOK );
 assert( add_fun.get_value() == v.get_value() * c );

 // test AddsVariables
 LinearFunction::v_coeff_pair add_vars( 10 );
 LinearFunction adds_fun;

 for( auto & p : add_vars ) {
  p.first = new ColVariable();
  p.first->set_value( get_random_coeff() );
  p.second = get_random_coeff();
 }

 LinearFunction::v_coeff_pair add_check = add_vars;
 adds_fun.add_variables( std::move( add_vars ) );
 assert( adds_fun.get_num_active_var() == add_check.size() );
 for( int i = 0 ; i < add_check.size() ; ++i ) {
  assert( adds_fun.is_active( add_check[ i ].first ) == i );
  assert( adds_fun.get_active_var( i ) == add_check[ i ].first );
  assert( adds_fun.get_coefficient( i ) == add_check[ i ].second );
 }
 assert( adds_fun.compute( true ) == LinearFunction::kOK );

 LinearFunction::FunctionValue sum = 0;
 for( auto & i : add_check )
  sum += i.first->get_value() * i.second;
 assert( sum != 0 );
 assert( adds_fun.get_value() == sum );

 // test RemovesVariable
 LinearFunction del_fun;
 ColVariable v1 , v2;
 v1.set_value( get_random_coeff() );
 v2.set_value( get_random_coeff() );
 LinearFunction::Coefficient c1 = get_random_coeff();
 LinearFunction::Coefficient c2 = get_random_coeff();

 del_fun.add_variable( &v1 , c1 );
 del_fun.add_variable( &v2 , c2 );

 assert( del_fun.get_num_active_var() == 2 );
 assert( del_fun.is_active( &v1 ) == 0 );
 assert( del_fun.is_active( &v2 ) == 1 );
 assert( del_fun.get_active_var( 0 ) == &v1 );
 assert( del_fun.get_coefficient( 0 ) == c1 );
 assert( del_fun.get_active_var( 1 ) == &v2 );
 assert( del_fun.get_coefficient( 1 ) == c2 );

 del_fun.remove_variable( 0 );
 assert( del_fun.is_active( &v1 ) == Inf< LinearFunction::Index >() );

 assert( del_fun.is_active( &v2 ) == 0 );
 assert( del_fun.get_active_var( 0 ) == &v2 );
 assert( del_fun.get_coefficient( 0 ) == c2 );
 assert( del_fun.get_num_active_var() == 1 );
 assert( del_fun.compute( true ) == LinearFunction::kOK );
 assert( del_fun.get_value() == v2.get_value() * c2 );

 // test RemovesVariables
 LinearFunction::v_coeff_pair del_vars( 10 );
 LinearFunction dels_fun;

 for( auto & p : del_vars ) {
  p.first = new ColVariable();
  p.first->set_value( get_random_coeff() );
  p.second = get_random_coeff();
 }

 LinearFunction::v_coeff_pair del_check = del_vars;
 dels_fun.add_variables( std::move( del_vars ) );
 assert( dels_fun.get_num_active_var() == 10 );

 LinearFunction::Range range{ 1 , 9 };
 dels_fun.remove_variables( range );
 assert( dels_fun.get_num_active_var() == 2 );

 assert( dels_fun.is_active( del_check[ 0 ].first ) == 0 );
 assert( dels_fun.get_active_var( 0 ) == del_check[ 0 ].first );
 assert( dels_fun.get_coefficient( 0 ) == del_check[ 0 ].second );

 assert( dels_fun.is_active( del_check[ 9 ].first ) == 1 );
 assert( dels_fun.get_active_var( 1 ) == del_check[ 9 ].first );
 assert( dels_fun.get_coefficient( 1 ) == del_check[ 9 ].second );

 assert( dels_fun.compute( true ) == LinearFunction::kOK );
 assert( dels_fun.get_value() ==
	 del_check[ 0 ].first->get_value() * del_check[ 0 ].second +
	 del_check[ 9 ].first->get_value() * del_check[ 9 ].second );

 for( auto & p : add_check )
  delete p.first;
 for( auto & p : del_check )
  delete p.first;
}

/*--------------------------------------------------------------------------*/
/* What the tests above never touch: a LinearFunction with nothing in it, the
 * constant term, a coefficient that is zero, and the three ways of removing
 * Variable at their own edges, one of which means the opposite of what it
 * looks like. */

static void test_edge_cases( void )
{
 // ---- a function with no Variable at all ----------------------------

 LinearFunction empty;
 assert( empty.get_num_active_var() == 0 );
 assert( empty.get_constant_term() == 0 );
 assert( empty.compute( true ) == LinearFunction::kOK );
 assert( empty.get_value() == 0 );

 // a Variable that was never added is not active, and answering with Inf
 // is what tells it apart from the one that sits at index 0
 ColVariable stranger;
 assert( empty.is_active( & stranger ) ==
	 Inf< LinearFunction::Index >() );

 // ---- the constant term ---------------------------------------------
 // it is the value of a function of no Variable, and it is added to the
 // value of one that has some: an affine function, not a linear one

 empty.set_constant_term( 3.5 );
 assert( empty.get_constant_term() == 3.5 );
 assert( empty.compute( true ) == LinearFunction::kOK );
 assert( empty.get_value() == 3.5 );

 ColVariable v;
 v.set_value( 2 );
 empty.add_variable( & v , 4 );
 assert( empty.compute( true ) == LinearFunction::kOK );
 assert( empty.get_value() == 3.5 + 8 );

 // ---- a coefficient of zero -----------------------------------------
 // the Variable is active all the same: it is in the function, it just
 // does not move its value, and whoever walks the pairs sees it

 LinearFunction zero;
 ColVariable z;
 z.set_value( 7 );
 zero.add_variable( & z , 0 );
 assert( zero.get_num_active_var() == 1 );
 assert( zero.is_active( & z ) == 0 );
 assert( zero.get_coefficient( 0 ) == 0 );
 assert( zero.compute( true ) == LinearFunction::kOK );
 assert( zero.get_value() == 0 );

 // and a coefficient can be changed to something that does move it
 zero.modify_coefficient( 0 , 2 );
 assert( zero.get_coefficient( 0 ) == 2 );
 assert( zero.compute( true ) == LinearFunction::kOK );
 assert( zero.get_value() == 14 );

 // ---- removing by range ---------------------------------------------

 auto ten = []( LinearFunction & f , std::vector< ColVariable > & vars ) {
  LinearFunction::v_coeff_pair p( vars.size() );
  for( LinearFunction::Index i = 0 ; i < vars.size() ; ++i )
   p[ i ] = { & vars[ i ] , double( i + 1 ) };
  f.add_variables( std::move( p ) );
  };

 std::vector< ColVariable > vars( 10 );

 {                              // an empty range removes nothing
  LinearFunction f;
  ten( f , vars );
  f.remove_variables( LinearFunction::Range{ 4 , 4 } );
  assert( f.get_num_active_var() == 10 );
 }
 {                              // a range past the end stops at the end
  LinearFunction f;
  ten( f , vars );
  f.remove_variables( LinearFunction::Range{ 8 , 1000 } );
  assert( f.get_num_active_var() == 8 );
  assert( f.get_active_var( 7 ) == & vars[ 7 ] );
 }
 {                              // and one that covers it all empties it
  LinearFunction f;
  ten( f , vars );
  f.remove_variables( LinearFunction::Range{ 0 , 10 } );
  assert( f.get_num_active_var() == 0 );
  assert( f.compute( true ) == LinearFunction::kOK );
  assert( f.get_value() == 0 );
 }

 // ---- removing by subset --------------------------------------------

 {                              // the last one, which leaves it empty
  LinearFunction f;
  ColVariable only;
  f.add_variable( & only , 1 );
  f.remove_variable( 0 );
  assert( f.get_num_active_var() == 0 );
  assert( f.is_active( & only ) == Inf< LinearFunction::Index >() );
 }
 {                              // an unordered subset, said to be unordered
  LinearFunction f;
  ten( f , vars );
  f.remove_variables( LinearFunction::Subset{ 7 , 1 , 4 } , false );
  assert( f.get_num_active_var() == 7 );
  assert( f.is_active( & vars[ 1 ] ) == Inf< LinearFunction::Index >() );
  assert( f.is_active( & vars[ 4 ] ) == Inf< LinearFunction::Index >() );
  assert( f.is_active( & vars[ 7 ] ) == Inf< LinearFunction::Index >() );
  assert( f.is_active( & vars[ 0 ] ) == 0 );
 }
 {
  /* ⚠️ An EMPTY subset removes EVERY Variable, which is the opposite of
   * what an empty range does and of what the word suggests: it is written
   * in the comments of remove_variables( Subset ), and a caller that
   * builds the subset and finds it empty has to know it. */

  LinearFunction f;
  ten( f , vars );
  f.remove_variables( LinearFunction::Subset{} );
  assert( f.get_num_active_var() == 0 );
 }

 // ---- modifying coefficients with someone listening ------------------
 // the part of NCoef that is not used does not reach the Modification,
 // whose delta() has one entry per Variable changed

 {                              // a Range past the end stops at the end
  Recorder rec;
  LinearFunction f;
  ten( f , vars );
  f.register_Observer( & rec );
  f.modify_coefficients( { 20 , 30 , 40 } ,
			 LinearFunction::Range{ 9 , 12 } );
  assert( f.get_coefficient( 9 ) == 20 );
  assert( rec.mods.size() == 1 );
  auto mod = std::dynamic_pointer_cast< C05FunctionModLinRngd >(
						      rec.mods.front() );
  assert( mod );
  assert( mod->range() == LinearFunction::Range( 9 , 10 ) );
  assert( mod->delta() == std::vector< double >( { 10 } ) );
 }
 {                              // an NCoef longer than the Subset
  Recorder rec;
  LinearFunction f;
  ten( f , vars );
  f.register_Observer( & rec );
  f.modify_coefficients( { 20 , 30 , 40 } ,
			 LinearFunction::Subset{ 3 , 0 } );
  assert( f.get_coefficient( 3 ) == 20 );
  assert( f.get_coefficient( 0 ) == 30 );
  assert( rec.mods.size() == 1 );
  auto mod = std::dynamic_pointer_cast< C05FunctionModLinSbst >(
						      rec.mods.front() );
  assert( mod );
  assert( mod->subset() == LinearFunction::Subset( { 0 , 3 } ) );
  assert( mod->delta() == std::vector< double >( { 29 , 16 } ) );
 }
 {                              // a wrong index changes nothing
  Recorder rec;
  LinearFunction f;
  ten( f , vars );
  f.register_Observer( & rec );
  bool thrown = false;
  try {
   f.modify_coefficients( { 20 , 30 } , LinearFunction::Subset{ 0 , 10 } );
   }
  catch( std::invalid_argument & ) {
   thrown = true;
   }
  assert( thrown );
  assert( f.get_coefficient( 0 ) == 1 );
  assert( rec.mods.empty() );
 }
 }

/*--------------------------------------------------------------------------*/
/* modify_coefficients() over a Range, a Subset and an empty Subset: the
 * coefficients named change and the others do not, and the value follows.
 * An empty Subset changes nothing (it is not "all of them", as it is for
 * remove_variables()), and an index out of range throws. */

static void test_modify_coefficients( void )
{
 std::vector< ColVariable > vars( 5 );
 LinearFunction f;
 fill( f , vars );              // coefficients 1 2 3 4 5

 // a Range
 f.modify_coefficients( { 10 , 20 } , Range( 1 , 3 ) );
 assert( f.get_coefficient( 0 ) == 1 );
 assert( f.get_coefficient( 1 ) == 10 );
 assert( f.get_coefficient( 2 ) == 20 );
 assert( f.get_coefficient( 3 ) == 4 );
 assert( f.compute( true ) == LinearFunction::kOK );
 assert( f.get_value() == value_of( f ) );

 // a Range past the end stops at the end
 f.modify_coefficients( { 40 , 50 } , Range( 3 , 1000 ) );
 assert( f.get_coefficient( 3 ) == 40 );
 assert( f.get_coefficient( 4 ) == 50 );

 // an empty Range changes nothing
 f.modify_coefficients( {} , Range( 2 , 2 ) );
 assert( f.get_coefficient( 2 ) == 20 );

 // an unordered Subset: NCoef[ k ] goes to nms[ k ]
 f.modify_coefficients( { -4 , 0.5 } , Subset( { 4 , 0 } ) , false );
 assert( f.get_coefficient( 0 ) == 0.5 );
 assert( f.get_coefficient( 4 ) == -4 );
 assert( f.get_coefficient( 1 ) == 10 );
 assert( f.compute( true ) == LinearFunction::kOK );
 assert( f.get_value() == value_of( f ) );

 // an empty Subset changes nothing
 f.modify_coefficients( {} , Subset() );
 assert( f.get_num_active_var() == 5 );
 assert( f.get_coefficient( 0 ) == 0.5 );
 assert( f.get_coefficient( 4 ) == -4 );

 // a wrong index, a short NCoef and a wrong single index throw
 assert( throws< std::invalid_argument >( [ & f ]() {
  f.modify_coefficients( { 1 } , Subset( { 5 } ) ); } ) );
 assert( throws< std::invalid_argument >( [ & f ]() {
  f.modify_coefficients( { 1 } , Subset( { 0 , 1 } ) ); } ) );
 assert( throws< std::invalid_argument >( [ & f ]() {
  f.modify_coefficients( { 1 } , Range( 0 , 2 ) ); } ) );
 assert( throws< std::invalid_argument >( [ & f ]() {
  f.modify_coefficient( 5 , 1 ); } ) );
 }

/*--------------------------------------------------------------------------*/
/* The linearization of a LinearFunction is the function itself: the
 * coefficients over any Range or Subset, dense or sparse, are the
 * coefficients whatever the point, the constant is the constant term, and
 * the Hessian is zero. */

static void test_linearization( void )
{
 std::vector< ColVariable > vars( 5 );
 LinearFunction f;
 fill( f , vars );
 f.modify_coefficient( 2 , 0 );  // a zero one, left out of a sparse g
 f.set_constant_term( -7.25 );
 assert( f.compute( true ) == LinearFunction::kOK );

 // dense, the whole Range and a part of it
 std::vector< double > g( 5 , 1e30 );
 f.get_linearization_coefficients( g.data() );
 for( Index i = 0 ; i < 5 ; ++i )
  assert( g[ i ] == f.get_coefficient( i ) );

 std::vector< double > gr( 2 , 1e30 );
 f.get_linearization_coefficients( gr.data() , Range( 3 , 5 ) );
 assert( ( gr[ 0 ] == f.get_coefficient( 3 ) ) &&
	 ( gr[ 1 ] == f.get_coefficient( 4 ) ) );

 // dense, a Subset, in the order it is given
 std::vector< double > gs( 3 , 1e30 );
 f.get_linearization_coefficients( gs.data() , Subset( { 4 , 0 , 2 } ) );
 assert( ( gs[ 0 ] == f.get_coefficient( 4 ) ) &&
	 ( gs[ 1 ] == f.get_coefficient( 0 ) ) && ( gs[ 2 ] == 0 ) );

 // sparse, a Range and a Subset
 LinearFunction::SparseVector sg;
 f.get_linearization_coefficients( sg , Range( 1 , 4 ) );
 assert( sg.size() == 5 );
 assert( sg.nonZeros() == 2 );  // index 2 is zero
 assert( ( sg.coeff( 1 ) == f.get_coefficient( 1 ) ) &&
	 ( sg.coeff( 3 ) == f.get_coefficient( 3 ) ) &&
	 ( sg.coeff( 0 ) == 0 ) );

 LinearFunction::SparseVector ss;
 f.get_linearization_coefficients( ss , Subset( { 0 , 4 } ) );
 assert( ss.nonZeros() == 2 );
 assert( ( ss.coeff( 0 ) == f.get_coefficient( 0 ) ) &&
	 ( ss.coeff( 4 ) == f.get_coefficient( 4 ) ) );

 // a wrong index in a Subset throws
 assert( throws< std::invalid_argument >( [ & f , & gs ]() {
  f.get_linearization_coefficients( gs.data() , Subset( { 5 } ) ); } ) );

 // the point does not matter
 for( auto & v : vars )
  v.set_value( v.get_value() * 3 + 1 );
 assert( f.compute( true ) == LinearFunction::kOK );
 std::vector< double > g2( 5 );
 f.get_linearization_coefficients( g2.data() );
 assert( g2 == g );

 // the constant is the constant term, and constant + g x is the value
 assert( f.get_linearization_constant() == -7.25 );
 double lin = f.get_linearization_constant();
 for( Index i = 0 ; i < 5 ; ++i )
  lin += g2[ i ] * vars[ i ].get_value();
 assert( lin == f.get_value() );

 // the Hessian is zero, dense of the right size and sparse
 f.compute_hessian_approximation();
 C15Function::DenseHessian dh;
 f.get_hessian_approximation( dh );
 assert( ( dh.rows() == 5 ) && ( dh.cols() == 5 ) );
 assert( dh.isZero( 0 ) );

 C15Function::SparseHessian sh( 5 , 5 );
 sh.insert( 1 , 1 ) = 3;        // whatever was there goes
 f.get_hessian_approximation( sh );
 assert( sh.nonZeros() == 0 );

 assert( f.is_convex() && f.is_concave() );
 }

/*--------------------------------------------------------------------------*/
/* A Variable given twice: add_variables() does not check it (as its comments
 * say), so the function has it twice and its value counts both, whereas the
 * constructor rejects it when the library is compiled with its checks. */

static void test_duplicate_variable( void )
{
 ColVariable x;
 x.set_value( 3 );

 LinearFunction f;
 f.add_variables( { { & x , 2 } , { & x , 5 } } );
 assert( f.get_num_active_var() == 2 );
 assert( f.is_active( & x ) == 0 );  // the first one
 assert( f.get_active_var( 1 ) == & x );
 assert( f.compute( true ) == LinearFunction::kOK );
 assert( f.get_value() == 3 * 2 + 3 * 5 );

 #ifndef LIB_NDEBUG
  assert( throws< std::invalid_argument >( [ & x ]() {
   LinearFunction g( { { & x , 2 } , { & x , 5 } } ); } ) );
 #endif
 }

/*--------------------------------------------------------------------------*/
/* remove_variable() with no Variable at the index throws, and leaves the
 * function as it was. */

static void test_remove_out_of_range( void )
{
 std::vector< ColVariable > vars( 3 );
 LinearFunction f;
 fill( f , vars );

 assert( throws< std::logic_error >( [ & f ]() { f.remove_variable( 3 ); } ) );
 assert( throws< std::logic_error >( [ & f ]() {
  f.remove_variable( Inf< Index >() ); } ) );
 assert( f.get_num_active_var() == 3 );

 LinearFunction empty;
 assert( throws< std::logic_error >( [ & empty ]() {
  empty.remove_variable( 0 ); } ) );

 // a Subset with an index out of range throws as well
 assert( throws< std::invalid_argument >( [ & f ]() {
  f.remove_variables( Subset( { 0 , 3 } ) ); } ) );
 }

/*--------------------------------------------------------------------------*/
/* map_active() gives the index of each Variable asked for, and throws if one
 * is not active; with ordered == true the Variable asked for are sorted by
 * address, a subset of the active ones, which may be active in any order.
 * map_index() gives the current index of Variable known by an old one. */

static void test_map_active_and_index( void )
{
 std::vector< ColVariable > x( 4 );   // &x[ 0 ] < &x[ 1 ] < ...

 // active in the order x2 x0 x3 x1
 LinearFunction f( { { & x[ 2 ] , 1 } , { & x[ 0 ] , 2 } ,
		     { & x[ 3 ] , 3 } , { & x[ 1 ] , 4 } } );

 // unordered
 {
  Subset map;
  f.map_active( { & x[ 1 ] , & x[ 2 ] , & x[ 0 ] } , map , false );
  assert( map.size() == 3 );
  assert( ( map[ 0 ] == 3 ) && ( map[ 1 ] == 0 ) && ( map[ 2 ] == 1 ) );
 }

 // a map longer than vars keeps its tail
 {
  Subset map( 3 , 77 );
  f.map_active( { & x[ 3 ] } , map , false );
  assert( ( map[ 0 ] == 2 ) && ( map[ 1 ] == 77 ) && ( map[ 2 ] == 77 ) );
 }

 // a Variable that is not active throws
 {
  ColVariable stranger;
  Subset map;
  assert( throws< std::invalid_argument >( [ & ]() {
   f.map_active( { & x[ 0 ] , & stranger } , map , false ); } ) );
 }

 // ordered, all of them
 {
  Subset map;
  f.map_active( { & x[ 0 ] , & x[ 1 ] , & x[ 2 ] , & x[ 3 ] } , map , true );
  check( ( map.size() == 4 ) && ( map[ 0 ] == 1 ) && ( map[ 1 ] == 3 ) &&
	 ( map[ 2 ] == 0 ) && ( map[ 3 ] == 2 ) ,
	 "map_active( ordered ) on all the active Variable" );
 }

 /* Ordered, a part of them: { x0 , x2 } is a set of active Variable, and
  * map_active() has to give { 1 , 0 }. */
 {
  Subset map;
  bool threw = false;
  try {
   f.map_active( { & x[ 0 ] , & x[ 2 ] } , map , true );
   }
  catch( std::exception & ) {
   threw = true;
   }
  check( ! threw , "map_active( { x0 , x2 } , ordered ) throws although "
	 "both are active" );
  check( ( map.size() == 2 ) && ( map[ 0 ] == 1 ) && ( map[ 1 ] == 0 ) ,
	 "map_active( { x0 , x2 } , ordered ) gives { 1 , 0 }" );
 }
 {
  Subset map;
  bool threw = false;
  try {
   f.map_active( { & x[ 0 ] , & x[ 3 ] } , map , true );
   }
  catch( std::exception & ) {
   threw = true;
   }
  check( ( ! threw ) && ( map.size() == 2 ) && ( map[ 0 ] == 1 ) &&
	 ( map[ 1 ] == 2 ) ,
	 "map_active( { x0 , x3 } , ordered ) gives { 1 , 2 }" );
 }

 // map_index(): nothing changed, each is where it was
 {
  auto map = f.map_index( { & x[ 0 ] , & x[ 3 ] } , Subset( { 1 , 2 } ) );
  assert( ( map[ 0 ] == 1 ) && ( map[ 1 ] == 2 ) );
  auto mapr = f.map_index( { & x[ 0 ] , & x[ 3 ] } , Range( 1 , 3 ) );
  assert( ( mapr[ 0 ] == 1 ) && ( mapr[ 1 ] == 2 ) );
 }

 // map_index(): x2 removed, the others moved one to the left, and a removed
 // one is Inf
 f.remove_variable( 0 , eNoMod );      // now x0 x3 x1
 {
  auto map = f.map_index( { & x[ 2 ] , & x[ 0 ] , & x[ 3 ] , & x[ 1 ] } ,
			  Subset( { 0 , 1 , 2 , 3 } ) );
  assert( map[ 0 ] >= f.get_num_active_var() );
  assert( ( map[ 1 ] == 0 ) && ( map[ 2 ] == 1 ) && ( map[ 3 ] == 2 ) );

  auto mapr = f.map_index( { & x[ 0 ] , & x[ 3 ] , & x[ 1 ] } ,
			   Range( 1 , 4 ) );
  assert( ( mapr[ 0 ] == 0 ) && ( mapr[ 1 ] == 1 ) && ( mapr[ 2 ] == 2 ) );
 }

 // map_index(): x2 re-added, it is at the end
 f.add_variable( & x[ 2 ] , 1 , eNoMod );  // now x0 x3 x1 x2
 {
  auto map = f.map_index( { & x[ 2 ] } , Subset( { 0 } ) );
  assert( map[ 0 ] == 3 );
 }

 // sizes that do not match throw
 assert( throws< std::invalid_argument >( [ & ]() {
  f.map_index( { & x[ 2 ] } , Subset( { 0 , 1 } ) ); } ) );
 assert( throws< std::invalid_argument >( [ & ]() {
  f.map_index( { & x[ 2 ] } , Range( 0 , 2 ) ); } ) );

 // on an empty function, everything is Inf
 LinearFunction empty;
 auto map = empty.map_index( { & x[ 2 ] } , Subset( { 0 } ) );
 assert( map[ 0 ] >= empty.get_num_active_var() );
 }

/*--------------------------------------------------------------------------*/
/* The iterators: on an empty function begin() is end(), both for the
 * virtual v_begin()/v_end() and the plain ones, const or not; on a full one
 * they walk the Variable in their order, and a clone() walks on its own. */

static void test_iterators( void )
{
 LinearFunction empty;
 {
  auto b = empty.v_begin();
  auto e = empty.v_end();
  assert( *b == *e );
  assert( ! ( *b != *e ) );
  delete b;
  delete e;
 }
 {
  const LinearFunction & ce = empty;
  auto b = ce.v_begin();
  auto e = ce.v_end();
  assert( *b == *e );
  delete b;
  delete e;
 }
 Index n = 0;
 for( auto & v : empty ) {
  ( void ) v;
  ++n;
  }
 assert( n == 0 );

 std::vector< ColVariable > vars( 3 );
 LinearFunction f;
 fill( f , vars );

 Index i = 0;
 for( auto it = f.begin() ; it != f.end() ; ++it , ++i )
  assert( &( *it ) == & vars[ i ] );
 assert( i == 3 );

 // a copy assigned moves on its own, and the one it held is released
 auto i1 = f.begin();
 auto i2 = f.end();
 i2 = i1;
 ++i2;
 assert( &( *i1 ) == & vars[ 0 ] );
 assert( &( *i2 ) == & vars[ 1 ] );
 i2 = std::move( i1 );
 assert( &( *i2 ) == & vars[ 0 ] );
 i2 = i2;               // assigned to itself, it is still valid
 assert( &( *i2 ) == & vars[ 0 ] );

 auto b = f.v_begin();
 auto c = b->clone();   // the clone moves on its own
 ++( *c );
 assert( &( **b ) == & vars[ 0 ] );
 assert( &( **c ) == & vars[ 1 ] );
 assert( *b != *c );
 delete b;
 delete c;
 }

/*--------------------------------------------------------------------------*/

/// a note for the maintainer: what differs from the documentation, and is
/// not (yet) required of the library, hence it does not fail the test

static void note( bool ok , const char * what )
{
 if( ! ok )
  std::cout << "LinearFunction_test NOTE: documented but not implemented: "
	    << what << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* eDryRun says that the change is not to be done, and hence that no
 * Modification is issued [see Observer::make_par()]. Here the function sits
 * in an FRowConstraint of a Block with a FakeSolver, so that a Modification
 * issued would be seen. That no Modification is issued is required; that
 * the change is not done is not implemented yet, and it is only noted. */

static void test_dry_run( void )
{
 auto block = new AbstractBlock();
 auto x = new std::vector< ColVariable >( 3 );
 block->add_static_variable( *x , "x" );
 auto rows = new std::vector< FRowConstraint >( 1 );
 block->add_static_constraint( *rows , "c" );

 auto f = new LinearFunction();
 f->add_variable( &( *x )[ 0 ] , 2 , eNoMod );
 ( *rows )[ 0 ].set_function( f , eNoMod );

 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();
 mods.clear();

 f->modify_coefficient( 0 , 9 , eDryRun );
 assert( mods.empty() );
 note( f->get_coefficient( 0 ) == 2 ,
	"modify_coefficient( eDryRun ) does not change the coefficient" );

 f->add_variable( &( *x )[ 1 ] , 1 , eDryRun );
 assert( mods.empty() );
 note( f->get_num_active_var() == 1 ,
	"add_variable( eDryRun ) does not add the Variable" );

 f->set_constant_term( 4 , eDryRun );
 assert( mods.empty() );
 note( f->get_constant_term() == 0 ,
	"set_constant_term( eDryRun ) does not change the constant term" );

 // a Variable added all the same has not been told of the FRowConstraint,
 // which would fail to leave it when the Block is deleted: take it away
 // just as silently
 if( f->get_num_active_var() > 1 )
  f->remove_variable( 1 , eNoMod );

 block->unregister_Solvers( true );
 delete block;
 }

/*--------------------------------------------------------------------------*/
/* The Modification issued by each change, as seen by a FakeSolver of the
 * Block when the function sits in an FRowConstraint: their type, the
 * Variable, the indices and the deltas they carry; eNoMod issues none, and
 * a change that is not one (an empty Subset of coefficients, the same
 * coefficient) issues none either. The FRowConstraint registers itself in
 * the Variable added and leaves those removed. */

static std::vector< sp_Mod > take( FakeSolver * solver )
{
 auto & l = solver->get_Modification_list();
 std::vector< sp_Mod > v( l.begin() , l.end() );
 l.clear();
 return( v );
 }

static void test_Modification_in_FRowConstraint( void )
{
 auto block = new AbstractBlock();
 auto x = new std::vector< ColVariable >( 6 );
 block->add_static_variable( *x , "x" );
 auto rows = new std::vector< FRowConstraint >( 1 );
 block->add_static_constraint( *rows , "c" );
 auto & row = ( *rows )[ 0 ];

 auto f = new LinearFunction();
 row.set_function( f , eNoMod );

 auto solver = new FakeSolver();
 block->register_Solver( solver );
 take( solver );

 auto X = [ x ]( Index i ) { return( &( *x )[ i ] ); };

 // add_variable
 f->add_variable( X( 0 ) , 1 );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto a = std::dynamic_pointer_cast< LinearFunctionModVarsAddd >( m[ 0 ] );
  assert( a && ( a->function() == f ) && ( a->first() == 0 ) );
  assert( ( a->vars().size() == 1 ) && ( a->vars()[ 0 ] == X( 0 ) ) );
  assert( ( a->coeff().size() == 1 ) && ( a->coeff()[ 0 ] == 1 ) );
  assert( a->concerns_Block() );
  assert( X( 0 )->is_active( & row ) < X( 0 )->get_num_active() );
 }

 // add_variables
 f->add_variables( { { X( 1 ) , 2 } , { X( 2 ) , 3 } , { X( 3 ) , 4 } ,
		     { X( 4 ) , 5 } } );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto a = std::dynamic_pointer_cast< LinearFunctionModVarsAddd >( m[ 0 ] );
  assert( a && ( a->first() == 1 ) && ( a->vars().size() == 4 ) );
  assert( ( a->vars()[ 0 ] == X( 1 ) ) && ( a->vars()[ 3 ] == X( 4 ) ) );
  assert( ( a->coeff()[ 0 ] == 2 ) && ( a->coeff()[ 3 ] == 5 ) );
  for( Index i = 1 ; i < 5 ; ++i )
   assert( X( i )->is_active( & row ) < X( i )->get_num_active() );
 }

 // modify_coefficient: the delta, over the Range of one
 f->modify_coefficient( 2 , 10 );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto l = std::dynamic_pointer_cast< C05FunctionModLinRngd >( m[ 0 ] );
  assert( l && ( l->function() == f ) );
  assert( l->range() == Range( 2 , 3 ) );
  assert( ( l->vars().size() == 1 ) && ( l->vars()[ 0 ] == X( 2 ) ) );
  assert( ( l->delta().size() == 1 ) && ( l->delta()[ 0 ] == 10 - 3 ) );
  assert( std::isnan( l->shift() ) );
 }

 // the same coefficient again is no change
 f->modify_coefficient( 2 , 10 );
 assert( take( solver ).empty() );

 // modify_coefficients over a Range
 f->modify_coefficients( { 20 , 30 } , Range( 0 , 2 ) );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto l = std::dynamic_pointer_cast< C05FunctionModLinRngd >( m[ 0 ] );
  assert( l && ( l->range() == Range( 0 , 2 ) ) );
  assert( ( l->vars()[ 0 ] == X( 0 ) ) && ( l->vars()[ 1 ] == X( 1 ) ) );
  assert( ( l->delta()[ 0 ] == 20 - 1 ) && ( l->delta()[ 1 ] == 30 - 2 ) );
 }

 // modify_coefficients over an unordered Subset: sorted in the Modification
 f->modify_coefficients( { 50 , 40 } , Subset( { 4 , 3 } ) , false );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto l = std::dynamic_pointer_cast< C05FunctionModLinSbst >( m[ 0 ] );
  assert( l && ( l->subset() == Subset( { 3 , 4 } ) ) );
  assert( ( l->vars()[ 0 ] == X( 3 ) ) && ( l->vars()[ 1 ] == X( 4 ) ) );
  assert( ( l->delta()[ 0 ] == 40 - 4 ) && ( l->delta()[ 1 ] == 50 - 5 ) );
 }

 // modify_coefficients over an empty Subset: nothing
 f->modify_coefficients( {} , Subset() );
 assert( take( solver ).empty() );

 // set_constant_term: a C05FunctionMod with the shift
 f->set_constant_term( 2.5 );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto c = std::dynamic_pointer_cast< C05FunctionMod >( m[ 0 ] );
  assert( c && ( c->type() == C05FunctionMod::NothingChanged ) );
  assert( c->which().empty() && ( c->shift() == 2.5 ) );
 }

 // eNoMod: the change is done, nothing is issued (the Variable are left
 // alone, since an FRowConstraint relies on the Modification to register
 // itself in the Variable added)
 f->modify_coefficient( 0 , -1 , eNoMod );
 f->set_constant_term( 0 , eNoMod );
 assert( take( solver ).empty() );
 assert( f->get_coefficient( 0 ) == -1 );

 // remove_variable
 f->remove_variable( 1 );       // x1 goes: x0 x2 x3 x4
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto r = std::dynamic_pointer_cast< C05FunctionModVarsRngd >( m[ 0 ] );
  assert( r && ( r->function() == f ) && ( ! r->added() ) );
  assert( r->range() == Range( 1 , 2 ) );
  assert( ( r->vars().size() == 1 ) && ( r->vars()[ 0 ] == X( 1 ) ) );
  assert( X( 1 )->is_active( & row ) >= X( 1 )->get_num_active() );
 }

 // remove_variables over a Range
 f->remove_variables( Range( 2 , 3 ) );   // x3 goes: x0 x2 x4
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto r = std::dynamic_pointer_cast< C05FunctionModVarsRngd >( m[ 0 ] );
  assert( r && ( r->range() == Range( 2 , 3 ) ) );
  assert( r->vars()[ 0 ] == X( 3 ) );
 }

 // remove_variables over a Subset
 f->remove_variables( Subset( { 2 , 0 } ) , false );   // left: x2
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto r = std::dynamic_pointer_cast< C05FunctionModVarsSbst >( m[ 0 ] );
  assert( r && ( r->subset() == Subset( { 0 , 2 } ) ) );
  assert( ( r->vars().size() == 2 ) && ( r->vars()[ 0 ] == X( 0 ) ) &&
	  ( r->vars()[ 1 ] == X( 4 ) ) );
  assert( f->get_num_active_var() == 1 );
  assert( f->get_active_var( 0 ) == X( 2 ) );
 }

 // remove_variables over an empty Subset: all of them, the Modification
 // naming every Variable with an empty Subset
 f->add_variable( X( 5 ) , 6 );                      // x2 x5
 take( solver );
 f->remove_variables( Subset() );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto r = std::dynamic_pointer_cast< C05FunctionModVarsSbst >( m[ 0 ] );
  assert( r && r->subset().empty() );
  assert( ( r->vars().size() == 2 ) && ( r->vars()[ 0 ] == X( 2 ) ) &&
	  ( r->vars()[ 1 ] == X( 5 ) ) );
  assert( f->get_num_active_var() == 0 );
  assert( X( 2 )->is_active( & row ) >= X( 2 )->get_num_active() );
 }

 // remove_variables over a Range that is all of them: a Subset one, empty
 f->add_variables( { { X( 0 ) , 1 } , { X( 1 ) , 1 } } );
 take( solver );
 f->remove_variables( Range( 0 , 2 ) );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto r = std::dynamic_pointer_cast< C05FunctionModVarsSbst >( m[ 0 ] );
  assert( r && r->subset().empty() && ( r->vars().size() == 2 ) );
 }

 block->unregister_Solvers( true );
 delete block;
 }

/*--------------------------------------------------------------------------*/
/* The same Modification reach a FakeSolver when the function is the one of
 * an FRealObjective, and the value of the Objective follows the function. */

static void test_Modification_in_FRealObjective( void )
{
 auto block = new AbstractBlock();
 auto x = new std::vector< ColVariable >( 3 );
 block->add_static_variable( *x , "x" );
 for( Index i = 0 ; i < 3 ; ++i )
  ( *x )[ i ].set_value( i + 1.0 );

 auto f = new LinearFunction( { { &( *x )[ 0 ] , 1 } } );
 block->set_objective( new FRealObjective( block , f ) , eNoMod );

 auto solver = new FakeSolver();
 block->register_Solver( solver );
 take( solver );

 f->add_variable( &( *x )[ 2 ] , 4 );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto a = std::dynamic_pointer_cast< LinearFunctionModVarsAddd >( m[ 0 ] );
  assert( a && ( a->function() == f ) && ( a->first() == 1 ) );
 }

 f->modify_coefficients( { 5 } , Subset( { 1 } ) );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto l = std::dynamic_pointer_cast< C05FunctionModLinSbst >( m[ 0 ] );
  assert( l && ( l->delta()[ 0 ] == 1 ) );
 }

 f->remove_variables( Subset( { 0 } ) );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto r = std::dynamic_pointer_cast< C05FunctionModVarsSbst >( m[ 0 ] );
  assert( r && ( r->subset() == Subset( { 0 } ) ) );
 }

 auto obj = static_cast< FRealObjective * >( block->get_objective() );
 assert( obj->compute( true ) == FRealObjective::kOK );
 assert( obj->value() == 5 * 3.0 );

 block->unregister_Solvers( true );
 delete block;
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 runAllTests();
 test_edge_cases();
 test_modify_coefficients();
 test_linearization();
 test_duplicate_variable();
 test_remove_out_of_range();
 test_map_active_and_index();
 test_iterators();
 test_dry_run();
 test_Modification_in_FRowConstraint();
 test_Modification_in_FRealObjective();

 if( n_failed ) {
  std::cerr << "LinearFunction_test: " << n_failed << " check(s) failed"
	    << std::endl;
  return( 1 );
  }

 std::cout << "All tests passed!!" << std::endl;
 return( 0 );
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_LinearFunction.cpp -------------------*/
/*--------------------------------------------------------------------------*/
