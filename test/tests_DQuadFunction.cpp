/*--------------------------------------------------------------------------*/
/*---------------------- File tests_DQuadFunction.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Unit tests for DQuadFunction.
 *
 * The tests go over the value of the function, its linearization (the
 * gradient, whole and in part, by Range and by Subset, dense and sparse, and
 * the constant that makes the linearization exact at the point), its Hessian
 * and its convexity; over the changes of its coefficients (one term, a Range
 * of them, a Subset of them, the linear ones only, the constant term) and
 * of its Variable (added and removed, one at a time, by Range and by
 * Subset), each with the Modification it issues, read by an Observer that
 * records them. The edge cases are those of every method taking a Range or
 * a Subset: empty, to the end, past the end, covering everything,
 * unordered, and a change that changes nothing. DQuadFunction has no netCDF
 * format of its own, and an AbstractBlock writes its model as an LP file,
 * which has no room for a quadratic term: there is no round trip to test.
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

#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "ColVariable.h"
#include "DQuadFunction.h"
#include "Observer.h"

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using DQ = DQuadFunction;
using Index = DQ::Index;
using Range = DQ::Range;
using Subset = DQ::Subset;
using Coeffs = std::vector< double >;
using Pairs = DQ::v_coeff_pair;

static constexpr double INF = Inf< double >();

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASSES -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// an Observer that records every Modification it is sent
/** anyone_there() answers what listening says, which is what tells eNoBlck
 * from eModBlck; channels are not used by the tests, and all of them are the
 * default one. */

class Recorder : public Observer
{
 public:

 Block * get_Block( void ) const override { return( nullptr ); }

 bool anyone_there( void ) const override { return( listening ); }

 void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override {
  mods.push_back( mod );
  }

 ChnlName open_channel( ChnlName chnl = 0 ,
			GroupModification * gmpmod = nullptr ) override {
  return( 0 );
  }

 void close_channel( ChnlName chnl , bool force = false ) override {}

 void set_default_channel( ChnlName chnl = 0 ) override {}

 bool listening = true;  ///< what anyone_there() says
 Lst_sp_Mod mods;        ///< what has been sent, in order
 };

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// true if calling f() throws an exception derived from E

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
/// the one Modification recorded, as a T, the record being emptied

template< class T >
static std::shared_ptr< T > take( Recorder & rec )
{
 assert( rec.mods.size() == 1 );
 auto mod = std::dynamic_pointer_cast< T >( rec.mods.front() );
 assert( mod );
 rec.mods.clear();
 return( mod );
 }

/*--------------------------------------------------------------------------*/
/// the value of f at the current point

static double value( DQ & f )
{
 assert( f.compute( true ) == DQ::kOK );
 return( f.get_value() );
 }

/*--------------------------------------------------------------------------*/
/// the Variable x, with the given values

static std::vector< ColVariable > point( const Coeffs & v )
{
 std::vector< ColVariable > x( v.size() );
 for( Index i = 0 ; i < v.size() ; ++i )
  x[ i ].set_value( v[ i ] );
 return( x );
 }

/*--------------------------------------------------------------------------*/
/// the linear and the quadratic coefficients of f, in order

static Coeffs lin( const DQ & f )
{
 Coeffs c;
 for( Index i = 0 ; i < f.get_num_active_var() ; ++i )
  c.push_back( f.get_linear_coefficient( i ) );
 return( c );
 }

static Coeffs quad( const DQ & f )
{
 Coeffs c;
 for( Index i = 0 ; i < f.get_num_active_var() ; ++i )
  c.push_back( f.get_quadratic_coefficient( i ) );
 return( c );
 }

/*--------------------------------------------------------------------------*/
/// the Variable of f, in order

static Vec_p_Var vars( const DQ & f )
{
 Vec_p_Var v;
 for( Index i = 0 ; i < f.get_num_active_var() ; ++i )
  v.push_back( f.get_active_var( i ) );
 return( v );
 }

/*--------------------------------------------------------------------------*/
/// the function sum_i ( b_i x_i + a_i x_i^2 ) + c on the Variable in x

static DQ::v_coeff_triple triples( std::vector< ColVariable > & x ,
				   const Coeffs & b , const Coeffs & a )
{
 DQ::v_coeff_triple t;
 for( Index i = 0 ; i < b.size() ; ++i )
  t.emplace_back( & x[ i ] , b[ i ] , a[ i ] );
 return( t );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- TESTS ----------------------------------*/
/*--------------------------------------------------------------------------*/
/* A function with no Variable is its constant term, is convex, concave and
 * linear at once, and has a linearization of no coefficient whose constant
 * is the constant term. */

static void test_empty( void )
{
 DQ f;
 assert( f.get_num_active_var() == 0 );
 assert( f.get_constant_term() == 0 );
 assert( value( f ) == 0 );
 assert( f.is_convex() && f.is_concave() && f.is_linear() );
 assert( f.get_linearization_constant() == 0 );
 assert( f.get_ComputeConfig( true , nullptr ) == nullptr );

 ColVariable stranger;
 assert( f.is_active( & stranger ) == Inf< Index >() );

 DQ::DenseHessian h;
 f.get_hessian_approximation( h );
 assert( ( h.rows() == 0 ) && ( h.cols() == 0 ) );

 DQ g( {} , 2.5 );
 assert( value( g ) == 2.5 );
 assert( g.get_linearization_constant() == 2.5 );

 // the global pool is not there
 assert( throws< std::invalid_argument >( [ & g ]() {
  g.get_linearization_constant( 0 ); } ) );
 }

/*--------------------------------------------------------------------------*/
/* The value, the linearization and the Hessian of
 *   f( x ) = 2 x_0^2 + 3 x_0 - x_1^2 + 0.5 x_2 + 5
 * at x = ( 1 , 2 , 4 ): f = 5 - 4 + 2 + 5 = 8, the gradient is
 * ( 4 x_0 + 3 , - 2 x_1 , 0.5 ) = ( 7 , -4 , 0.5 ), and the constant that
 * makes the linearization exact is c - sum_i a_i x_i^2 = 5 - ( 2 - 4 ) = 7,
 * so that 7 + 7 - 8 + 2 = 8. A zero coefficient is still a term. */

static void test_value( void )
{
 auto x = point( { 1 , 2 , 4 } );
 DQ f( triples( x , { 3 , 0 , 0.5 } , { 2 , -1 , 0 } ) , 5 );

 assert( f.get_num_active_var() == 3 );
 assert( f.is_active( & x[ 2 ] ) == 2 );
 assert( f.get_v_var().size() == 3 );
 assert( value( f ) == 8 );
 assert( f.get_lower_estimate() == 8 );
 assert( f.get_upper_estimate() == 8 );
 assert( ( ! f.is_convex() ) && ( ! f.is_concave() ) && ( ! f.is_linear() ) );

 // compute( false ) leaves the value alone: nothing has changed for it
 x[ 0 ].set_value( 0 );
 assert( f.compute( false ) == DQ::kOK );
 assert( f.get_value() == 8 );
 x[ 0 ].set_value( 1 );

 // ---- the gradient, whole and in part ------------------------------------

 assert( f.get_linearization_coefficient( 0 ) == 7 );
 Coeffs g( 3 , NAN );
 f.get_linearization_coefficients( g.data() );
 assert( g == Coeffs( { 7 , -4 , 0.5 } ) );

 Coeffs r( 3 , NAN );                              // g[ i - first ] = l[ i ]
 f.get_linearization_coefficients( r.data() , Range( 1 , 1000 ) );
 assert( ( r[ 0 ] == -4 ) && ( r[ 1 ] == 0.5 ) && std::isnan( r[ 2 ] ) );

 Coeffs e( 1 , NAN );                              // empty: nothing written
 f.get_linearization_coefficients( e.data() , Range( 2 , 2 ) );
 assert( std::isnan( e[ 0 ] ) );

 Coeffs s( 3 , NAN );                              // g[ k ] = l[ subset[ k ] ]
 f.get_linearization_coefficients( s.data() , Subset( { 2 , 0 } ) );
 assert( ( s[ 0 ] == 0.5 ) && ( s[ 1 ] == 7 ) && std::isnan( s[ 2 ] ) );
 assert( throws< std::invalid_argument >( [ & ]() {
  f.get_linearization_coefficients( s.data() , Subset( { 3 } ) ); } ) );

 DQ::SparseVector sv;                              // g[ i ] = l[ i ]
 f.get_linearization_coefficients( sv , Range( 1 , 3 ) );
 assert( sv.size() == 3 );
 assert( sv.nonZeros() == 2 );
 assert( ( sv.coeff( 1 ) == -4 ) && ( sv.coeff( 2 ) == 0.5 ) );

 DQ::SparseVector ss;
 f.get_linearization_coefficients( ss , Subset( { 2 , 0 } ) );
 assert( ( ss.coeff( 0 ) == 7 ) && ( ss.coeff( 2 ) == 0.5 ) );
 assert( ss.coeff( 1 ) == 0 );

 // a non-empty SparseVector is updated where asked and left alone elsewhere
 f.get_linearization_coefficients( ss , Subset( { 1 } ) );
 assert( ( ss.coeff( 0 ) == 7 ) && ( ss.coeff( 1 ) == -4 ) );
 DQ::SparseVector wrong( 5 );
 wrong.insert( 4 ) = 1;
 assert( throws< std::invalid_argument >( [ & ]() {
  f.get_linearization_coefficients( wrong , Range( 0 , 1 ) ); } ) );

 // ---- the constant of the linearization ----------------------------------
 // whatever it is, it has to make the linearization exact at the point

 {
  const double c = f.get_linearization_constant();
  const double at = c + 7 * 1 - 4 * 2 + 0.5 * 4;
  // the constant is c - sum_i a_i x_i^2 = 5 - ( 2 - 4 ) = 7, which makes
  // the linearization exact at the point
  assert( c == 7 );
  assert( at == value( f ) );
 }

 // ---- the Hessian --------------------------------------------------------

 DQ::DenseHessian h;
 f.get_hessian_approximation( h );
 assert( ( h.rows() == 3 ) && ( h.cols() == 3 ) );
 assert( ( h( 0 , 0 ) == 4 ) && ( h( 1 , 1 ) == -2 ) && ( h( 2 , 2 ) == 0 ) );
 assert( ( h( 0 , 1 ) == 0 ) && ( h( 2 , 0 ) == 0 ) );

 // the sparse Hessian has each diagonal entry in its own place
 {
  DQ::SparseHessian sh( 3 , 3 );
  f.get_hessian_approximation( sh );
  assert( sh.coeff( 0 , 0 ) == 4 );
  assert( sh.coeff( 1 , 1 ) == -2 );
  assert( sh.coeff( 2 , 2 ) == 0 );
 }

 // ---- convexity ----------------------------------------------------------

 DQ cvx( triples( x , { 1 , 1 } , { 2 , 0 } ) );
 assert( cvx.is_convex() && ( ! cvx.is_concave() ) && ( ! cvx.is_linear() ) );
 DQ ccv( triples( x , { 1 , 1 } , { -2 , 0 } ) );
 assert( ccv.is_concave() && ( ! ccv.is_convex() ) );
 DQ lf( triples( x , { 1 , -1 } , { 0 , 0 } ) );
 assert( lf.is_linear() && lf.is_convex() && lf.is_concave() );

 // ---- the Variable -------------------------------------------------------

 Subset map;
 f.map_active( { & x[ 2 ] , & x[ 0 ] } , map , false );
 assert( map == Subset( { 2 , 0 } ) );
 ColVariable stranger;
 assert( throws< std::invalid_argument >( [ & ]() {
  f.map_active( { & stranger } , map , false ); } ) );

 // where a Variable is now, given where it was
 assert( f.map_index( { & x[ 1 ] , & x[ 2 ] } , Subset( { 1 , 7 } ) ) ==
	 Subset( { 1 , 2 } ) );
 assert( f.map_index( { & stranger } , Range( 0 , 1 ) ) ==
	 Subset( { Inf< Index >() } ) );
 }

/*--------------------------------------------------------------------------*/
/* The coefficients: modify_term() and modify_terms() change both of them and
 * issue a DQuadFunctionModRngd / ...Sbst with the deltas, the ..._linear_...
 * methods change the linear one only and issue the narrower
 * C05FunctionModLinRngd / ...LinSbst, and set_constant_term() issues a
 * C05FunctionMod whose shift is the change. A change that changes nothing
 * issues nothing, an empty Range or Subset changes nothing, and a Range past
 * the end stops at the end. */

static void test_coefficients( void )
{
 auto x = point( { 1 , 2 , 3 } );
 Recorder rec;
 DQ f;
 f.register_Observer( & rec );

 auto reset = [ & ]() {
  f.remove_variables( Subset() , false , eNoMod );
  f.add_variables( triples( x , { 1 , 2 , 3 } , { 10 , 20 , 30 } ) , eNoMod );
  f.set_constant_term( 0 , eNoMod );
  rec.mods.clear();
  };

 // ---- one term -----------------------------------------------------------

 reset();
 f.modify_term( 1 , 2 , 20 );                      // the same: nothing
 assert( rec.mods.empty() );

 f.modify_term( 1 , 5 , 15 );
 assert( lin( f ) == Coeffs( { 1 , 5 , 3 } ) );
 assert( quad( f ) == Coeffs( { 10 , 15 , 30 } ) );
 {
  auto mod = take< DQuadFunctionModRngd >( rec );
  assert( mod->type() == C05FunctionMod::AllLinearizationChanged );
  assert( mod->range() == Range( 1 , 2 ) );
  assert( mod->vars() == Vec_p_Var( { & x[ 1 ] } ) );
  assert( mod->delta() == Pairs( { { 3 , -5 } } ) );
  assert( std::isnan( mod->shift() ) );
 }
 assert( throws< std::invalid_argument >( [ & f ]() {
  f.modify_term( 3 , 1 , 1 ); } ) );

 f.modify_linear_coefficient( 0 , 1 );             // the same: nothing
 assert( rec.mods.empty() );
 f.modify_linear_coefficient( 0 , -1 );
 assert( lin( f ) == Coeffs( { -1 , 5 , 3 } ) );
 assert( quad( f ) == Coeffs( { 10 , 15 , 30 } ) );
 {
  auto mod = take< C05FunctionModLinRngd >( rec );
  assert( mod->range() == Range( 0 , 1 ) );
  assert( mod->delta() == Coeffs( { -2 } ) );
  assert( mod->vars() == Vec_p_Var( { & x[ 0 ] } ) );
 }
 assert( throws< std::invalid_argument >( [ & f ]() {
  f.modify_linear_coefficient( 3 , 1 ); } ) );

 // ---- a Range of terms ---------------------------------------------------

 reset();
 const Coeffs nq = { 7 , 8 , 9 };
 const Coeffs nl = { -7 , -8 , -9 };

 f.modify_terms( nq.begin() , nl.begin() , Range( 2 , 2 ) );   // empty
 f.modify_terms( nq.begin() , nl.begin() , Range( 3 , 9 ) );   // past it
 assert( rec.mods.empty() );
 assert( quad( f ) == Coeffs( { 10 , 20 , 30 } ) );

 f.modify_terms( nq.begin() , nl.begin() , Range( 1 , 1000 ) ); // to the end
 assert( quad( f ) == Coeffs( { 10 , 7 , 8 } ) );
 assert( lin( f ) == Coeffs( { 1 , -7 , -8 } ) );
 {
  auto mod = take< DQuadFunctionModRngd >( rec );
  assert( mod->range() == Range( 1 , 3 ) );        // the one done
  assert( mod->vars() == Vec_p_Var( { & x[ 1 ] , & x[ 2 ] } ) );
  assert( mod->delta() == Pairs( { { -9 , -13 } , { -11 , -22 } } ) );
 }

 f.modify_terms( nq.begin() , nl.begin() );        // INFRange: all
 assert( quad( f ) == nq );
 assert( lin( f ) == nl );
 assert( take< DQuadFunctionModRngd >( rec )->range() == Range( 0 , 3 ) );

 // ---- a Subset of terms --------------------------------------------------

 reset();
 f.modify_terms( nq.begin() , nl.begin() , Subset() );   // empty: nothing
 assert( rec.mods.empty() );
 assert( quad( f ) == Coeffs( { 10 , 20 , 30 } ) );

 // unordered: the k-th new coefficients go to the k-th index as given
 f.modify_terms( nq.begin() , nl.begin() , Subset( { 2 , 0 } ) , false );
 assert( quad( f ) == Coeffs( { 8 , 20 , 7 } ) );
 assert( lin( f ) == Coeffs( { -8 , 2 , -7 } ) );
 {
  // the Modification orders the Subset, and the Variable with it
  auto mod = take< DQuadFunctionModSbst >( rec );
  assert( mod->type() == C05FunctionMod::AllLinearizationChanged );
  assert( mod->subset() == Subset( { 0 , 2 } ) );
  assert( mod->vars() == Vec_p_Var( { & x[ 0 ] , & x[ 2 ] } ) );
  assert( mod->delta().size() == 2 );
  // and delta() with them, delta()[ k ] still being the change of
  // vars()[ k ]
  assert( mod->delta() == Pairs( { { -9 , -2 } , { -10 , -23 } } ) );
 }

 assert( throws< std::invalid_argument >( [ & ]() {
  f.modify_terms( nq.begin() , nl.begin() , Subset( { 3 } ) ); } ) );
 assert( rec.mods.empty() );

 // ---- the linear coefficients only ---------------------------------------

 reset();
 f.modify_linear_coefficients( Coeffs() , Range( 1 , 1 ) );   // empty
 f.modify_linear_coefficients( Coeffs() , Subset() );         // empty
 assert( rec.mods.empty() );

 f.modify_linear_coefficients( Coeffs( { 4 , 4 } ) , Range( 1 , 3 ) );
 assert( lin( f ) == Coeffs( { 1 , 4 , 4 } ) );
 assert( quad( f ) == Coeffs( { 10 , 20 , 30 } ) );
 {
  auto mod = take< C05FunctionModLinRngd >( rec );
  assert( mod->range() == Range( 1 , 3 ) );
  assert( mod->delta() == Coeffs( { 2 , 1 } ) );   // the changes, not values
  assert( mod->vars() == Vec_p_Var( { & x[ 1 ] , & x[ 2 ] } ) );
 }

 f.modify_linear_coefficients( Coeffs( { 0 , 9 } ) , Subset( { 2 , 0 } ) ,
			       false );
 assert( lin( f ) == Coeffs( { 9 , 4 , 0 } ) );
 {
  // ordered inside, the deltas going with their Variable
  auto mod = take< C05FunctionModLinSbst >( rec );
  assert( mod->subset() == Subset( { 0 , 2 } ) );
  assert( mod->vars() == Vec_p_Var( { & x[ 0 ] , & x[ 2 ] } ) );
  assert( mod->delta() == Coeffs( { 8 , -4 } ) );
 }

 assert( throws< std::invalid_argument >( [ & ]() {       // too few
  f.modify_linear_coefficients( Coeffs( { 1 } ) , Range( 0 , 2 ) ); } ) );
 assert( throws< std::invalid_argument >( [ & ]() {
  f.modify_linear_coefficients( Coeffs( { 1 } ) , Subset( { 5 } ) ); } ) );
 assert( rec.mods.empty() );

 // a wrong index is refused before any coefficient is changed
 reset();
 assert( throws< std::invalid_argument >( [ & ]() {
  f.modify_linear_coefficients( Coeffs( { 7 , 7 } ) , Subset( { 0 , 5 } ) );
  } ) );
 assert( throws< std::invalid_argument >( [ & ]() {
  f.modify_terms( nq.begin() , nl.begin() , Subset( { 0 , 3 } ) ); } ) );
 assert( lin( f ) == Coeffs( { 1 , 2 , 3 } ) );
 assert( quad( f ) == Coeffs( { 10 , 20 , 30 } ) );
 assert( rec.mods.empty() );

 // an NCoef longer than the Subset: the part not used does not reach the
 // Modification
 f.modify_linear_coefficients( Coeffs( { 5 , 6 , 7 } ) , Subset( { 1 } ) );
 assert( lin( f ) == Coeffs( { 1 , 5 , 3 } ) );
 assert( take< C05FunctionModLinSbst >( rec )->delta() == Coeffs( { 3 } ) );

 // a Range past the end is cut at the end, and so is NCoef: the
 // Modification has the change of the one coefficient changed
 reset();
 f.modify_linear_coefficients( Coeffs( { 5 , 6 , 7 } ) , Range( 2 , 5 ) );
 assert( lin( f ) == Coeffs( { 1 , 2 , 5 } ) );
 {
  auto mod = take< C05FunctionModLinRngd >( rec );
  assert( mod->range() == Range( 2 , 3 ) );
  assert( mod->delta() == Coeffs( { 2 } ) );
 }
 rec.mods.clear();

 // ---- the constant term --------------------------------------------------

 reset();
 f.set_constant_term( 0 );                         // the same: nothing
 assert( rec.mods.empty() );
 f.set_constant_term( 4 );
 assert( f.get_constant_term() == 4 );
 {
  auto mod = take< C05FunctionMod >( rec );
  assert( mod->type() == C05FunctionMod::NothingChanged );
  assert( mod->which().empty() );
  assert( mod->shift() == 4 );
 }
 f.set_constant_term( 1 );
 assert( take< C05FunctionMod >( rec )->shift() == -3 );
 assert( value( f ) == 1 + 1 + 10 + 4 + 80 + 9 + 270 );

 // ---- who is listening ---------------------------------------------------

 reset();
 rec.listening = false;
 f.modify_term( 0 , 0 , 0 , eNoBlck );             // done, not told
 assert( rec.mods.empty() );
 f.modify_term( 1 , 0 , 0 , eModBlck );            // done, told the Block
 assert( take< DQuadFunctionModRngd >( rec )->concerns_Block() );
 rec.listening = true;
 f.modify_term( 2 , 0 , 0 , eNoBlck );             // told the Solver only
 assert( ! take< DQuadFunctionModRngd >( rec )->concerns_Block() );
 f.set_constant_term( 3 , eNoMod );                // done, not told
 assert( rec.mods.empty() );
 assert( f.is_linear() );
 assert( value( f ) == 3 );
 }

/*--------------------------------------------------------------------------*/
/* The Variable: added at the end with a DQuadFunctionModVarsAddd carrying
 * their coefficients, removed with a C05FunctionModVarsRngd or ...Sbst, the
 * coefficients of the others following them. An empty Subset removes them
 * all, an empty Range none, and removing all of them is said with an empty
 * Subset whichever way it is asked. */

static void test_variables( void )
{
 auto x = point( { 1 , 2 , 3 , 4 } );
 Recorder rec;
 DQ f;
 f.register_Observer( & rec );

 auto reset = [ & ]() {
  f.remove_variables( Subset() , false , eNoMod );
  f.add_variables( triples( x , { 1 , 2 , 3 } , { 10 , 20 , 30 } ) , eNoMod );
  rec.mods.clear();
  };

 // ---- adding -------------------------------------------------------------

 f.add_variables( {} );                            // nothing: nothing
 f.add_variable( nullptr , 1 , 1 );
 assert( f.get_num_active_var() == 0 );
 assert( rec.mods.empty() );

 f.add_variables( triples( x , { 1 , 2 } , { 10 , 20 } ) );   // to nothing
 assert( vars( f ) == Vec_p_Var( { & x[ 0 ] , & x[ 1 ] } ) );
 {
  auto mod = take< DQuadFunctionModVarsAddd >( rec );
  assert( mod->first() == 0 );
  assert( mod->vars() == Vec_p_Var( { & x[ 0 ] , & x[ 1 ] } ) );
  assert( mod->coeff() == Pairs( { { 1 , 10 } , { 2 , 20 } } ) );
  assert( mod->shift() == 0 );                     // strongly q.-additive
 }

 f.add_variable( & x[ 2 ] , 3 , 30 );              // to something
 assert( f.is_active( & x[ 2 ] ) == 2 );
 {
  auto mod = take< DQuadFunctionModVarsAddd >( rec );
  assert( mod->first() == 2 );
  assert( mod->coeff() == Pairs( { { 3 , 30 } } ) );
 }

 DQ::v_coeff_triple more;
 more.emplace_back( & x[ 3 ] , 4 , 40 );
 f.add_variables( std::move( more ) );
 assert( lin( f ) == Coeffs( { 1 , 2 , 3 , 4 } ) );
 assert( quad( f ) == Coeffs( { 10 , 20 , 30 , 40 } ) );
 assert( take< DQuadFunctionModVarsAddd >( rec )->first() == 3 );

 // ---- removing one -------------------------------------------------------

 reset();
 f.remove_variable( 1 );
 assert( vars( f ) == Vec_p_Var( { & x[ 0 ] , & x[ 2 ] } ) );
 assert( lin( f ) == Coeffs( { 1 , 3 } ) );
 {
  auto mod = take< C05FunctionModVarsRngd >( rec );
  assert( mod->range() == Range( 1 , 2 ) );
  assert( mod->vars() == Vec_p_Var( { & x[ 1 ] } ) );
  assert( mod->shift() == 0 );
 }
 assert( throws< std::logic_error >( [ & f ]() { f.remove_variable( 2 ); } ) );
 assert( rec.mods.empty() );

 // ---- removing by Range --------------------------------------------------

 reset();
 f.remove_variables( Range( 1 , 1 ) );             // empty: nothing
 f.remove_variables( Range( 3 , 7 ) );             // past it: nothing
 assert( f.get_num_active_var() == 3 );
 assert( rec.mods.empty() );

 f.remove_variables( Range( 1 , 1000 ) , eNoMod ); // to the end, silently
 assert( vars( f ) == Vec_p_Var( { & x[ 0 ] } ) );
 assert( quad( f ) == Coeffs( { 10 } ) );
 assert( rec.mods.empty() );

 // removing a Range that is not all the Variable, with the Modification
 // issued
 reset();
 f.remove_variables( Range( 1 , 3 ) );
 assert( vars( f ) == Vec_p_Var( { & x[ 0 ] } ) );
 {
  auto mod = take< C05FunctionModVarsRngd >( rec );
  assert( mod->range() == Range( 1 , 3 ) );
  assert( mod->vars() == Vec_p_Var( { & x[ 1 ] , & x[ 2 ] } ) );
 }

 reset();
 f.remove_variables( Range( 0 , 1000 ) );          // all of them
 assert( f.get_num_active_var() == 0 );
 {
  auto mod = take< C05FunctionModVarsSbst >( rec );
  assert( mod->subset().empty() );
  assert( mod->vars() == Vec_p_Var( { & x[ 0 ] , & x[ 1 ] , & x[ 2 ] } ) );
 }

 // ---- removing by Subset -------------------------------------------------

 reset();
 f.remove_variables( Subset() );                   // empty: all of them
 assert( f.get_num_active_var() == 0 );
 assert( take< C05FunctionModVarsSbst >( rec )->subset().empty() );
 f.remove_variables( Subset() );                   // and again: still none
 assert( take< C05FunctionModVarsSbst >( rec )->vars().empty() );

 reset();
 f.remove_variables( Subset( { 2 , 0 } ) , false );   // unordered
 assert( vars( f ) == Vec_p_Var( { & x[ 1 ] } ) );
 assert( lin( f ) == Coeffs( { 2 } ) );
 assert( quad( f ) == Coeffs( { 20 } ) );
 {
  auto mod = take< C05FunctionModVarsSbst >( rec );
  assert( mod->subset() == Subset( { 0 , 2 } ) );   // ordered inside
  assert( mod->vars() == Vec_p_Var( { & x[ 0 ] , & x[ 2 ] } ) );
 }

 reset();
 f.remove_variables( Subset( { 1 } ) , true , eNoMod );
 assert( vars( f ) == Vec_p_Var( { & x[ 0 ] , & x[ 2 ] } ) );
 assert( quad( f ) == Coeffs( { 10 , 30 } ) );

 reset();
 f.remove_variables( Subset( { 0 , 1 , 2 } ) , true );   // all, by name
 assert( f.get_num_active_var() == 0 );
 assert( take< C05FunctionModVarsSbst >( rec )->vars().size() == 3 );

 reset();
 assert( throws< std::invalid_argument >( [ & f ]() {
  f.remove_variables( Subset( { 1 , 3 } ) , true ); } ) );
 assert( f.get_num_active_var() == 3 );
 assert( rec.mods.empty() );

 // the value follows the Variable: 1 + 10 is gone, 2 * 2 + 20 * 4 and
 // 3 * 3 + 30 * 9 stay
 f.remove_variable( 0 , eNoMod );
 assert( value( f ) == 4 + 80 + 9 + 270 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- MAIN -----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( void )
{
 test_empty();
 test_value();
 test_coefficients();
 test_variables();

 std::cout << "DQuadFunction_test: OK" << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- End File tests_DQuadFunction.cpp -------------------*/
/*--------------------------------------------------------------------------*/
