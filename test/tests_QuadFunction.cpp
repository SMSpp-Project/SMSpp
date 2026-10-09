/** @file
 * Unit tests for QuadFunction and DQuadFunction.
 * They test stuff not already tested with Function: DQuadFunction is tested
 * on its own as well as the base of QuadFunction.
 *
* \author Wim van Ackooij \n
 *         EDF Lab Paris-Saclay \n
 *
 * \copyright &copy; by Wim van Ackooij
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "AbstractBlock.h"
#include "FRowConstraint.h"
#include "FakeSolver.h"
#include "QuadFunction.h"

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
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
using Coefficient = QuadFunction::Coefficient;

void runAllTests()
{
    // QuadFunctions are particularly DQuads if no non-diagonals get added ; 
    // Let us make a 1d QuadFunction q1 x^2 + c1 x

    QuadFunction add_fun;
    ColVariable v;
    Coefficient c1 = 2.0;
    Coefficient q1 = 1.0;

    add_fun.add_variable( &v, c1, q1 );

    // set a value to v 
    v.set_value(2.0);

    assert( add_fun.get_num_active_var() == 1 );
    assert( add_fun.is_active( &v ) == 0 );
    assert( add_fun.get_active_var( 0 ) == &v );
    assert( add_fun.get_linear_coefficient( 0 ) == c1 );
    assert( add_fun.get_quadratic_coefficient( 0, 0 ) == q1 );
    assert( add_fun.compute( true ) == QuadFunction::kOK );
    assert( add_fun.get_value() == (2.0 * c1 + 4.0*q1) );
    
    // Now let us make a 3 x 3 PSD function
    // 2 x1^2 - 2 x1x2 + 2 x2^2 - 2x2x3 + 2 x3^2
    //
    ColVariable x1, x2, x3;
    DQuadFunction::v_coeff_triple v_vars;
    DQuadFunction::coeff_triple t1( &x1 , 0.0 , 2.0 );
    DQuadFunction::coeff_triple t2( &x2 , 0.0 , 2.0 );
    DQuadFunction::coeff_triple t3( &x3 , 0.0 , 2.0 );
    v_vars.reserve( 3 );
    v_vars.push_back( t1 );
    v_vars.push_back( t2 );
    v_vars.push_back( t3 );

    // Off diagonal
    QuadFunction::v_off_diag_term v_nd_vars;
    v_nd_vars.reserve(2);
    QuadFunction::off_diag_term od1( 1 , 0 , -2.0 );
    QuadFunction::off_diag_term od2( 2 , 1 , -2.0 );
    v_nd_vars.push_back( od1 );
    v_nd_vars.push_back( od2 );

    QuadFunction nw_quad( std::move( v_vars ) , std::move( v_nd_vars ) );

    x1.set_value( 1.0 );
    x2.set_value( 2.0 );
    x3.set_value( 3.0 );    

    // We need to do a compute to ensure actual computation
    assert( nw_quad.compute( true ) == QuadFunction::kOK );   
    assert( nw_quad.get_value() == 12.0 );
    assert( nw_quad.is_convex() ) ;

    // Let us modify a coefficient and check that convexity was not retained, 
    // nor is the map concave
    nw_quad.modify_term(0, 1, -4.0);
    assert( !nw_quad.is_convex() ) ;
    assert( !nw_quad.is_concave() ) ;

    // Let us delete the whole first variable
    nw_quad.remove_variable(0);
    assert( nw_quad.compute( true ) == QuadFunction::kOK );   
    assert( nw_quad.get_value() == 14.0 );
    assert( nw_quad.is_convex() ) ;
    /* Removing a Range and removing a Subset have to leave the very same
     * function that would have been built on the Variable that stay: the
     * non-diagonal terms of a removed Variable go with it, and those of the
     * ones that stay follow them to their new index. */

    ColVariable y[ 5 ];
    for( QuadFunction::Index i = 0 ; i < 5 ; ++i )
        y[ i ].set_value( 1.0 + i );

    // the function on the Variable whose index is in "keep", with the same
    // coefficients whichever those are
    auto build = [ &y ]( const std::vector< QuadFunction::Index > & keep ) {
        DQuadFunction::v_coeff_triple tr( keep.size() );
        for( QuadFunction::Index t = 0 ; t < keep.size() ; ++t )
            tr[ t ] = std::make_tuple( &y[ keep[ t ] ] ,
                                       Coefficient( keep[ t ] + 1 ) ,
                                       Coefficient( 2 ) );

        QuadFunction::v_off_diag_term od;
        for( QuadFunction::Index t = 1 ; t < keep.size() ; ++t )
            for( QuadFunction::Index l = 0 ; l < t ; ++l )
                od.emplace_back( t , l ,
                                 Coefficient( 1 + keep[ t ] * keep[ l ] ) );

        return( new QuadFunction( std::move( tr ) , std::move( od ) ) );
        };

    {   // the Range [ 1 , 3 )
        auto f = build( { 0 , 1 , 2 , 3 , 4 } );
        f->remove_variables( QuadFunction::Range( 1 , 3 ) , eNoMod );
        auto g = build( { 0 , 3 , 4 } );

        assert( f->get_num_active_var() == g->get_num_active_var() );
        assert( f->compute( true ) == QuadFunction::kOK );
        assert( g->compute( true ) == QuadFunction::kOK );
        assert( f->get_value() == g->get_value() );

        delete f;
        delete g;
        }

    {   // the Subset { 0 , 2 , 4 }
        auto f = build( { 0 , 1 , 2 , 3 , 4 } );
        f->remove_variables( QuadFunction::Subset( { 0 , 2 , 4 } ) , true ,
                             eNoMod );
        auto g = build( { 1 , 3 } );

        assert( f->get_num_active_var() == g->get_num_active_var() );
        assert( f->compute( true ) == QuadFunction::kOK );
        assert( g->compute( true ) == QuadFunction::kOK );
        assert( f->get_value() == g->get_value() );

        delete f;
        delete g;
        }

    {   // an empty Subset is "all of them"
        auto f = build( { 0 , 1 , 2 , 3 , 4 } );
        f->remove_variables( QuadFunction::Subset() , true , eNoMod );
        assert( f->get_num_active_var() == 0 );
        delete f;
        }

}

/*--------------------------------------------------------------------------*/
/* What the tests above never touch: a function with nothing in it, the
 * constant term, the matrix read on the side it was not written on, a pair
 * of Variable that carries no term at all, and the two edges of a Range.
 * The matrix being symmetric is the one thing every caller assumes of a
 * quadratic function and the one an index swapped somewhere breaks. */

static void test_edge_cases( void )
{
    // ---- a function with no Variable at all ------------------------

    QuadFunction empty;
    assert( empty.get_num_active_var() == 0 );
    assert( empty.get_constant_term() == 0 );
    assert( empty.compute( true ) == QuadFunction::kOK );
    assert( empty.get_value() == 0 );

    ColVariable stranger;
    assert( empty.is_active( &stranger ) == Inf< QuadFunction::Index >() );

    // the constant term is the value of a function of no Variable, and it
    // is added to the value of one that has some
    empty.set_constant_term( 2.5 );
    assert( empty.compute( true ) == QuadFunction::kOK );
    assert( empty.get_value() == 2.5 );

    ColVariable w;
    w.set_value( 3.0 );
    empty.add_variable( &w , 1.0 , 2.0 );      // 2 w^2 + w + 2.5
    assert( empty.compute( true ) == QuadFunction::kOK );
    assert( empty.get_value() == 2.0 * 9.0 + 3.0 + 2.5 );

    // ---- the matrix is symmetric -----------------------------------

    ColVariable a , b , c;
    a.set_value( 1.0 ); b.set_value( 1.0 ); c.set_value( 1.0 );

    DQuadFunction::v_coeff_triple tr;
    tr.emplace_back( &a , 0.0 , 1.0 );
    tr.emplace_back( &b , 0.0 , 1.0 );
    tr.emplace_back( &c , 0.0 , 1.0 );

    QuadFunction::v_off_diag_term od;
    od.emplace_back( 1 , 0 , 3.0 );            // the only pair with a term
    QuadFunction sym( std::move( tr ) , std::move( od ) );

    // read on the side it was written on and on the other one: the same
    assert( sym.get_quadratic_coefficient( 1 , 0 ) == 3.0 );
    assert( sym.get_quadratic_coefficient( 0 , 1 ) == 3.0 );

    // a pair that carries no term at all is 0, not something undefined
    assert( sym.get_quadratic_coefficient( 2 , 0 ) == 0 );
    assert( sym.get_quadratic_coefficient( 0 , 2 ) == 0 );
    assert( sym.get_quadratic_coefficient( 2 , 1 ) == 0 );

    // and asking for i == j is the diagonal, which is the DQuadFunction one
    for( QuadFunction::Index i = 0 ; i < 3 ; ++i )
        assert( sym.get_quadratic_coefficient( i , i ) == 1.0 );

    /* ---- a function that is linear ---------------------------------
     *
     * A linear function is convex AND concave, its Hessian being the zero
     * matrix, which is both positive and negative semidefinite. The class
     * keeps ONE state out of Convex, Concave and NotConvex, and decides it
     * by asking Eigen for positive semidefiniteness first, so a linear
     * QuadFunction comes out Convex and is_concave() answers false. That is
     * what it does today and it is pinned here so that nobody changes it by
     * accident; a caller that has to tell this case apart asks is_linear(),
     * which is what it is for. */

    DQuadFunction::v_coeff_triple flat;
    flat.emplace_back( &a , 2.0 , 0.0 );
    flat.emplace_back( &b , -1.0 , 0.0 );
    QuadFunction line( std::move( flat ) , QuadFunction::v_off_diag_term() );
    assert( line.is_linear() );
    assert( line.is_convex() );
    assert( ! line.is_concave() );

    // and one that curves downwards is concave and not convex
    DQuadFunction::v_coeff_triple down;
    down.emplace_back( &a , 0.0 , -2.0 );
    down.emplace_back( &b , 0.0 , -2.0 );
    QuadFunction cap( std::move( down ) , QuadFunction::v_off_diag_term() );
    assert( cap.is_concave() );
    assert( ! cap.is_convex() );

    // ---- the two edges of a Range ----------------------------------
    // the off-diagonal terms are what makes this worth checking: they are
    // named by index, so removing nothing must move nothing

    auto three = [ &a , &b , &c ]() {
        DQuadFunction::v_coeff_triple t;
        t.emplace_back( &a , 1.0 , 1.0 );
        t.emplace_back( &b , 2.0 , 1.0 );
        t.emplace_back( &c , 3.0 , 1.0 );
        QuadFunction::v_off_diag_term o;
        o.emplace_back( 1 , 0 , 5.0 );
        o.emplace_back( 2 , 1 , 7.0 );
        return( new QuadFunction( std::move( t ) , std::move( o ) ) );
        };

    {   // an empty Range removes nothing, and moves no term
        auto f = three();
        f->remove_variables( QuadFunction::Range( 1 , 1 ) , eNoMod );
        assert( f->get_num_active_var() == 3 );
        assert( f->get_quadratic_coefficient( 1 , 0 ) == 5.0 );
        assert( f->get_quadratic_coefficient( 2 , 1 ) == 7.0 );
        delete f;
        }

    {   // a Range past the end stops at the end
        auto f = three();
        f->remove_variables( QuadFunction::Range( 2 , 1000 ) , eNoMod );
        assert( f->get_num_active_var() == 2 );
        assert( f->get_quadratic_coefficient( 1 , 0 ) == 5.0 );
        delete f;
        }

    {   // and one that covers it all empties it
        auto f = three();
        f->remove_variables( QuadFunction::Range( 0 , 3 ) , eNoMod );
        assert( f->get_num_active_var() == 0 );
        assert( f->compute( true ) == QuadFunction::kOK );
        assert( f->get_value() == 0 );
        delete f;
        }
    }

/*--------------------------------------------------------------------------*/
/*------------------------ DQuadFunction ON ITS OWN ------------------------*/
/*--------------------------------------------------------------------------*/

using DIndex = DQuadFunction::Index;
using DRange = DQuadFunction::Range;
using DSubset = DQuadFunction::Subset;

/// the number of soft checks that failed
static int n_failed = 0;

/// a check that reports what fails and lets the other ones run
/** Used where a failure is a defect of the library that the test documents:
 * it prints what is wrong, and main() returns non-zero at the end. */

static void check( bool ok , const char * what )
{
 if( ok )
  return;
 std::cerr << "QuadFunction_test FAILED: " << what << std::endl;
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

/// c + sum_i ( a_i x_i^2 + b_i x_i ), from what the function says it holds

static double value_of( DQuadFunction & f )
{
 double v = f.get_constant_term();
 for( DIndex i = 0 ; i < f.get_num_active_var() ; ++i ) {
  const double x = static_cast< ColVariable * >(
					  f.get_active_var( i ) )->get_value();
  v += x * ( f.get_linear_coefficient( i ) +
	     f.get_quadratic_coefficient( i ) * x );
  }
 return( v );
 }

/*--------------------------------------------------------------------------*/

/// f on x, with b_i = i + 1, a_i = 1 and x_i = ( -1 )^i ( i + 1 )

static DQuadFunction * make_dquad( std::vector< ColVariable > & x )
{
 DQuadFunction::v_coeff_triple t( x.size() );
 for( DIndex i = 0 ; i < x.size() ; ++i ) {
  x[ i ].set_value( ( i % 2 ? -1.0 : 1.0 ) * ( i + 1 ) );
  t[ i ] = std::make_tuple( & x[ i ] , Coefficient( i + 1 ) , 1.0 );
  }
 return( new DQuadFunction( std::move( t ) , 0.5 ) );
 }

/*--------------------------------------------------------------------------*/
/* The linear coefficients, alone or with the quadratic ones, changed over a
 * single index, a Range and a Subset: the ones named change, the others do
 * not, the value follows, and a wrong index throws. An empty Subset changes
 * nothing. */

static void test_DQuad_modify( void )
{
 std::vector< ColVariable > x( 4 );
 auto f = make_dquad( x );
 assert( f->compute( true ) == DQuadFunction::kOK );
 assert( f->get_value() == value_of( *f ) );
 assert( f->get_value() != 0 );

 // one linear coefficient: the quadratic one stays
 f->modify_linear_coefficient( 1 , 5 );
 assert( f->get_linear_coefficient( 1 ) == 5 );
 assert( f->get_quadratic_coefficient( 1 ) == 1 );

 // linear ones over a Range, and over one past the end
 f->modify_linear_coefficients( { 7 , 8 } , DRange( 2 , 4 ) );
 assert( ( f->get_linear_coefficient( 2 ) == 7 ) &&
	 ( f->get_linear_coefficient( 3 ) == 8 ) );
 f->modify_linear_coefficients( { 9 } , DRange( 3 , 100 ) );
 assert( f->get_linear_coefficient( 3 ) == 9 );

 // linear ones over an unordered Subset, and over an empty one
 f->modify_linear_coefficients( { -2 , -1 } , DSubset( { 3 , 0 } ) , false );
 assert( ( f->get_linear_coefficient( 0 ) == -1 ) &&
	 ( f->get_linear_coefficient( 3 ) == -2 ) );
 f->modify_linear_coefficients( {} , DSubset() );
 assert( f->get_linear_coefficient( 0 ) == -1 );
 assert( f->compute( true ) == DQuadFunction::kOK );
 assert( f->get_value() == value_of( *f ) );

 // one term
 f->modify_term( 2 , 0.25 , 3 );
 assert( ( f->get_linear_coefficient( 2 ) == 0.25 ) &&
	 ( f->get_quadratic_coefficient( 2 ) == 3 ) );

 // terms over a Range: NQuadCoef first, NLinCoef second
 std::vector< double > quad{ 4 , 5 } , lin{ -4 , -5 };
 f->modify_terms( quad.cbegin() , lin.cbegin() , DRange( 0 , 2 ) );
 assert( ( f->get_quadratic_coefficient( 0 ) == 4 ) &&
	 ( f->get_linear_coefficient( 0 ) == -4 ) &&
	 ( f->get_quadratic_coefficient( 1 ) == 5 ) &&
	 ( f->get_linear_coefficient( 1 ) == -5 ) );
 assert( f->get_quadratic_coefficient( 2 ) == 3 );

 // terms over an unordered Subset
 std::vector< double > quad2{ 6 , 7 } , lin2{ 0.5 , 1.5 };
 f->modify_terms( quad2.cbegin() , lin2.cbegin() , DSubset( { 3 , 1 } ) ,
		  false );
 assert( ( f->get_quadratic_coefficient( 3 ) == 6 ) &&
	 ( f->get_linear_coefficient( 3 ) == 0.5 ) &&
	 ( f->get_quadratic_coefficient( 1 ) == 7 ) &&
	 ( f->get_linear_coefficient( 1 ) == 1.5 ) );
 assert( f->compute( true ) == DQuadFunction::kOK );
 assert( f->get_value() == value_of( *f ) );

 // wrong indices throw
 assert( throws< std::invalid_argument >( [ f ]() {
  f->modify_linear_coefficient( 4 , 1 ); } ) );
 assert( throws< std::invalid_argument >( [ f ]() {
  f->modify_term( 4 , 1 , 1 ); } ) );
 assert( throws< std::invalid_argument >( [ f ]() {
  f->modify_linear_coefficients( { 1 } , DSubset( { 4 } ) ); } ) );
 assert( throws< std::invalid_argument >( [ f , & quad ]() {
  f->modify_terms( quad.cbegin() , quad.cbegin() , DSubset( { 4 } ) ); } ) );

 delete f;
 }

/*--------------------------------------------------------------------------*/
/* The sign of the quadratic coefficients decides convexity: all of them
 * >= 0 is convex, all <= 0 concave, a mix neither, all zero both (and
 * linear); a change of sign of one diagonal term flips it either way. */

static void test_DQuad_convexity( void )
{
 std::vector< ColVariable > x( 3 );
 auto f = make_dquad( x );         // a = 1 1 1
 assert( f->is_convex() && ( ! f->is_concave() ) && ( ! f->is_linear() ) );

 f->modify_term( 1 , 2 , -1 );     // a = 1 -1 1
 assert( ( ! f->is_convex() ) && ( ! f->is_concave() ) );

 std::vector< double > neg{ -2 , -3 } , lin{ 0 , 0 };
 f->modify_terms( neg.cbegin() , lin.cbegin() , DSubset( { 0 , 2 } ) );
 assert( ( ! f->is_convex() ) && f->is_concave() );   // a = -2 -1 -3

 f->modify_term( 1 , 2 , 1 );      // a = -2 1 -3
 assert( ( ! f->is_convex() ) && ( ! f->is_concave() ) );

 std::vector< double > zero{ 0 , 0 , 0 };
 f->modify_terms( zero.cbegin() , lin.cbegin() , DRange( 0 , 2 ) );
 f->modify_term( 2 , 0 , 0 );      // a = 0 0 0
 assert( f->is_convex() && f->is_concave() && f->is_linear() );

 f->modify_term( 0 , 1 , 0.5 );    // a = 0.5 0 0
 assert( f->is_convex() && ( ! f->is_concave() ) );

 delete f;
 }

/*--------------------------------------------------------------------------*/
/* A Variable added, removed and added again: it is active, then not, then
 * active at the end, with the coefficients given the second time. */

static void test_DQuad_add_remove_readd( void )
{
 std::vector< ColVariable > x( 3 );
 auto f = make_dquad( x );
 ColVariable y;
 y.set_value( -2.5 );

 f->add_variable( & y , 3 , 2 );
 assert( f->get_num_active_var() == 4 );
 assert( f->is_active( & y ) == 3 );
 assert( f->compute( true ) == DQuadFunction::kOK );
 assert( f->get_value() == value_of( *f ) );

 f->remove_variable( 3 );
 assert( f->get_num_active_var() == 3 );
 assert( f->is_active( & y ) >= f->get_num_active_var() );
 assert( f->compute( true ) == DQuadFunction::kOK );
 assert( f->get_value() == value_of( *f ) );

 // removed from the middle, the others move to the left
 f->add_variable( & y , -1 , 4 );
 f->remove_variable( 0 );
 assert( f->get_active_var( 0 ) == & x[ 1 ] );
 assert( f->is_active( & y ) == 2 );

 // and back again at the end, with the new coefficients
 f->remove_variables( DSubset( { 2 } ) );
 f->add_variables( { std::make_tuple( & y , 6.0 , 0.5 ) } );
 assert( f->is_active( & y ) == 2 );
 assert( ( f->get_linear_coefficient( 2 ) == 6 ) &&
	 ( f->get_quadratic_coefficient( 2 ) == 0.5 ) );
 assert( f->compute( true ) == DQuadFunction::kOK );
 assert( f->get_value() == value_of( *f ) );

 // no Variable at the index throws
 assert( throws< std::logic_error >( [ f ]() { f->remove_variable( 3 ); } ) );

 delete f;
 }

/*--------------------------------------------------------------------------*/
/* The linearization at a point where every Variable is non-zero: the
 * coefficients are the gradient 2 a_i x_i + b_i over any Range or Subset,
 * dense or sparse, and the constant is c - sum_i a_i x_i^2 [see the comments
 * of get_linearization_constant()], i.e., the one that makes the tangent
 * plane go through the value at the point. */

static void test_DQuad_linearization( void )
{
 std::vector< ColVariable > x( 4 );
 auto f = make_dquad( x );
 f->modify_term( 1 , -3 , 2.5 );
 f->modify_term( 3 , 0.75 , 3 );
 assert( f->compute( true ) == DQuadFunction::kOK );

 std::vector< double > grad( 4 );
 for( DIndex i = 0 ; i < 4 ; ++i )
  grad[ i ] = 2 * f->get_quadratic_coefficient( i ) * x[ i ].get_value() +
              f->get_linear_coefficient( i );

 // dense, all and a Range
 std::vector< double > g( 4 , 1e30 );
 f->get_linearization_coefficients( g.data() );
 assert( g == grad );

 std::vector< double > gr( 2 , 1e30 );
 f->get_linearization_coefficients( gr.data() , DRange( 1 , 3 ) );
 assert( ( gr[ 0 ] == grad[ 1 ] ) && ( gr[ 1 ] == grad[ 2 ] ) );

 // dense, a Subset in the order it is given
 std::vector< double > gs( 2 , 1e30 );
 f->get_linearization_coefficients( gs.data() , DSubset( { 3 , 0 } ) );
 assert( ( gs[ 0 ] == grad[ 3 ] ) && ( gs[ 1 ] == grad[ 0 ] ) );

 // sparse, a Range and a Subset
 DQuadFunction::SparseVector sg;
 f->get_linearization_coefficients( sg , DRange( 0 , 2 ) );
 assert( ( sg.size() == 4 ) && ( sg.nonZeros() == 2 ) );
 assert( ( sg.coeff( 0 ) == grad[ 0 ] ) && ( sg.coeff( 1 ) == grad[ 1 ] ) );

 DQuadFunction::SparseVector ss;
 f->get_linearization_coefficients( ss , DSubset( { 2 } ) );
 assert( ( ss.nonZeros() == 1 ) && ( ss.coeff( 2 ) == grad[ 2 ] ) );

 // a wrong index throws
 assert( throws< std::invalid_argument >( [ f , & gs ]() {
  f->get_linearization_coefficients( gs.data() , DSubset( { 4 } ) ); } ) );

 // the constant: c - sum_i a_i x_i^2, and constant + g x == f( x )
 double expected = f->get_constant_term();
 for( DIndex i = 0 ; i < 4 ; ++i )
  expected -= f->get_quadratic_coefficient( i ) * x[ i ].get_value() *
              x[ i ].get_value();
 const double lc = f->get_linearization_constant();
 if( std::abs( lc - expected ) > 1e-12 * std::abs( expected ) )
  std::cout << "DQuadFunction::get_linearization_constant(): expected "
	    << expected << ", got " << lc << std::endl;
 check( std::abs( lc - expected ) <= 1e-12 * std::abs( expected ) ,
	"DQuadFunction::get_linearization_constant() == c - sum_i a_i x_i^2" );

 double tangent = lc;
 for( DIndex i = 0 ; i < 4 ; ++i )
  tangent += g[ i ] * x[ i ].get_value();
 check( std::abs( tangent - f->get_value() ) <=
	1e-12 * std::abs( f->get_value() ) ,
	"DQuadFunction: constant + g x == f( x ) at the point" );

 delete f;
 }

/*--------------------------------------------------------------------------*/
/* The Hessian is the diagonal matrix with entries 2 a_i, dense and sparse
 * alike. */

static void test_DQuad_hessian( void )
{
 std::vector< ColVariable > x( 3 );
 auto f = make_dquad( x );
 f->modify_term( 1 , 0 , -2 );
 f->modify_term( 2 , 0 , 3 );      // a = 1 -2 3
 f->compute_hessian_approximation();

 C15Function::DenseHessian dh;
 f->get_hessian_approximation( dh );
 assert( ( dh.rows() == 3 ) && ( dh.cols() == 3 ) );
 for( DIndex i = 0 ; i < 3 ; ++i )
  for( DIndex j = 0 ; j < 3 ; ++j )
   assert( dh( i , j ) ==
	   ( i == j ? 2 * f->get_quadratic_coefficient( i ) : 0 ) );

 C15Function::SparseHessian sh( 3 , 3 );
 f->get_hessian_approximation( sh );
 bool same = ( sh.rows() == 3 ) && ( sh.cols() == 3 );
 for( DIndex i = 0 ; same && ( i < 3 ) ; ++i )
  for( DIndex j = 0 ; j < 3 ; ++j )
   if( sh.coeff( i , j ) != dh( i , j ) )
    same = false;
 if( ! same )
  std::cout << "DQuadFunction sparse Hessian: expected diag( 2 -4 6 ), got "
	    << "H( 0 , 0 ) = " << sh.coeff( 0 , 0 ) << ", H( 1 , 1 ) = "
	    << sh.coeff( 1 , 1 ) << ", H( 2 , 2 ) = " << sh.coeff( 2 , 2 )
	    << std::endl;
 check( same , "DQuadFunction::get_hessian_approximation( SparseHessian ) "
	"== diag( 2 a_i )" );

 delete f;
 }

/*--------------------------------------------------------------------------*/
/* A copy of an iterator of the "active" Variable, made by clone(), walks on
 * its own; begin() is end() on an empty function. There is no clone() of the
 * function itself. */

static void test_DQuad_iterator_clone( void )
{
 std::vector< ColVariable > x( 3 );
 auto f = make_dquad( x );

 auto b = f->v_begin();
 auto c = b->clone();
 ++( *c );
 assert( &( **b ) == & x[ 0 ] );
 assert( &( **c ) == & x[ 1 ] );
 assert( *b != *c );
 ++( *b );
 assert( *b == *c );
 delete b;
 delete c;

 DIndex i = 0;
 for( auto & v : *f )
  assert( & v == & x[ i++ ] );
 assert( i == 3 );

 DQuadFunction empty;
 assert( empty.begin() == empty.end() );

 delete f;
 }

/*--------------------------------------------------------------------------*/
/* The Modification issued by a DQuadFunction in an FRowConstraint of a
 * Block, as a FakeSolver sees them: type, Variable, indices and deltas. */

static std::vector< sp_Mod > take( FakeSolver * solver )
{
 auto & l = solver->get_Modification_list();
 std::vector< sp_Mod > v( l.begin() , l.end() );
 l.clear();
 return( v );
 }

static void test_DQuad_Modification( void )
{
 auto block = new AbstractBlock();
 auto x = new std::vector< ColVariable >( 4 );
 block->add_static_variable( *x , "x" );
 auto rows = new std::vector< FRowConstraint >( 1 );
 block->add_static_constraint( *rows , "c" );
 auto & row = ( *rows )[ 0 ];

 auto f = new DQuadFunction();
 row.set_function( f , eNoMod );

 auto solver = new FakeSolver();
 block->register_Solver( solver );
 take( solver );

 auto X = [ x ]( DIndex i ) { return( &( *x )[ i ] ); };

 // add_variable and add_variables
 f->add_variable( X( 0 ) , 1 , 2 );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto a = std::dynamic_pointer_cast< DQuadFunctionModVarsAddd >( m[ 0 ] );
  assert( a && ( a->function() == f ) && ( a->first() == 0 ) );
  assert( ( a->vars().size() == 1 ) && ( a->vars()[ 0 ] == X( 0 ) ) );
  assert( a->coeff()[ 0 ] == DQuadFunction::coeff_pair( 1 , 2 ) );
  assert( X( 0 )->is_active( & row ) < X( 0 )->get_num_active() );
 }
 f->add_variables( { std::make_tuple( X( 1 ) , 3.0 , 4.0 ) ,
		     std::make_tuple( X( 2 ) , 5.0 , 6.0 ) ,
		     std::make_tuple( X( 3 ) , 7.0 , 8.0 ) } );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto a = std::dynamic_pointer_cast< DQuadFunctionModVarsAddd >( m[ 0 ] );
  assert( a && ( a->first() == 1 ) && ( a->vars().size() == 3 ) );
  assert( a->coeff()[ 2 ] == DQuadFunction::coeff_pair( 7 , 8 ) );
 }

 // a linear coefficient: C05FunctionModLinRngd with the delta
 f->modify_linear_coefficient( 1 , 10 );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto l = std::dynamic_pointer_cast< C05FunctionModLinRngd >( m[ 0 ] );
  assert( l && ( l->range() == DRange( 1 , 2 ) ) );
  assert( ( l->vars()[ 0 ] == X( 1 ) ) && ( l->delta()[ 0 ] == 10 - 3 ) );
 }

 // linear coefficients over a Subset: C05FunctionModLinSbst, sorted
 f->modify_linear_coefficients( { 0 , 1 } , DSubset( { 3 , 0 } ) , false );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto l = std::dynamic_pointer_cast< C05FunctionModLinSbst >( m[ 0 ] );
  assert( l && ( l->subset() == DSubset( { 0 , 3 } ) ) );
  assert( ( l->delta()[ 0 ] == 1 - 1 ) && ( l->delta()[ 1 ] == 0 - 7 ) );
 }

 // linear coefficients over an empty Subset: nothing
 f->modify_linear_coefficients( {} , DSubset() );
 assert( take( solver ).empty() );

 // one term: DQuadFunctionModRngd with the two deltas
 f->modify_term( 2 , 5.5 , 1 );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto d = std::dynamic_pointer_cast< DQuadFunctionModRngd >( m[ 0 ] );
  assert( d && ( d->function() == f ) );
  assert( d->type() == C05FunctionMod::AllLinearizationChanged );
  assert( d->range() == DRange( 2 , 3 ) );
  assert( d->vars()[ 0 ] == X( 2 ) );
  assert( d->delta()[ 0 ] == DQuadFunction::coeff_pair( 0.5 , -5 ) );
 }

 // terms over a Range and over a Subset
 std::vector< double > quad{ 1 , 1 } , lin{ 1 , 1 };
 f->modify_terms( quad.cbegin() , lin.cbegin() , DRange( 0 , 2 ) );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto d = std::dynamic_pointer_cast< DQuadFunctionModRngd >( m[ 0 ] );
  assert( d && ( d->range() == DRange( 0 , 2 ) ) );
  // x0 had ( 1 , 2 ), x1 had ( 10 , 4 )
  assert( d->delta()[ 0 ] == DQuadFunction::coeff_pair( 0 , -1 ) );
  assert( d->delta()[ 1 ] == DQuadFunction::coeff_pair( -9 , -3 ) );
 }
 f->modify_terms( quad.cbegin() , lin.cbegin() , DSubset( { 3 , 2 } ) ,
		  false );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto d = std::dynamic_pointer_cast< DQuadFunctionModSbst >( m[ 0 ] );
  assert( d && ( d->subset() == DSubset( { 2 , 3 } ) ) );
  assert( ( d->vars()[ 0 ] == X( 2 ) ) && ( d->vars()[ 1 ] == X( 3 ) ) );
  // x2 had ( 5.5 , 1 ), x3 had ( 0 , 8 ): delta()[ k ] is that of vars()[ k ],
  // hence the deltas have to follow the sorting of the Subset
  const bool follow =
   ( d->delta()[ 0 ] == DQuadFunction::coeff_pair( 1 - 5.5 , 0 ) ) &&
   ( d->delta()[ 1 ] == DQuadFunction::coeff_pair( 1 , -7 ) );
  if( ! follow )
   std::cout << "DQuadFunctionModSbst on the unordered Subset { 3 , 2 }: "
	     << "vars() = { x2 , x3 }, expected delta() = { ( -4.5 , 0 ) , "
	     << "( 1 , -7 ) }, got { ( " << d->delta()[ 0 ].first << " , "
	     << d->delta()[ 0 ].second << " ) , ( " << d->delta()[ 1 ].first
	     << " , " << d->delta()[ 1 ].second << " ) }" << std::endl;
  check( follow , "DQuadFunctionModSbst: delta() sorted with the Subset" );
 }

 // the constant term
 f->set_constant_term( -3 );
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto c = std::dynamic_pointer_cast< C05FunctionMod >( m[ 0 ] );
  assert( c && ( c->type() == C05FunctionMod::NothingChanged ) &&
	  ( c->shift() == -3 ) );
 }

 // eNoMod issues nothing
 f->modify_term( 0 , 9 , 9 , eNoMod );
 assert( take( solver ).empty() );

 // removing: a Range of one, then a Subset
 f->remove_variable( 1 );          // x0 x2 x3
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto r = std::dynamic_pointer_cast< C05FunctionModVarsRngd >( m[ 0 ] );
  assert( r && ( r->range() == DRange( 1 , 2 ) ) &&
	  ( r->vars()[ 0 ] == X( 1 ) ) );
  assert( X( 1 )->is_active( & row ) >= X( 1 )->get_num_active() );
 }
 f->remove_variables( DSubset( { 2 , 0 } ) , false );   // x2
 {
  auto m = take( solver );
  assert( m.size() == 1 );
  auto r = std::dynamic_pointer_cast< C05FunctionModVarsSbst >( m[ 0 ] );
  assert( r && ( r->subset() == DSubset( { 0 , 2 } ) ) );
  assert( ( r->vars()[ 0 ] == X( 0 ) ) && ( r->vars()[ 1 ] == X( 3 ) ) );
  assert( f->get_active_var( 0 ) == X( 2 ) );
 }

 block->unregister_Solvers( true );
 delete block;
 }

/*--------------------------------------------------------------------------*/
/* The off-diagonal terms of a QuadFunction named by index: i == j is not an
 * off-diagonal term (the constructor rejects one with its checks on, the
 * diagonal being the one of DQuadFunction), and an index out of range
 * throws. The Hessian, dense and sparse, has 2 a_i on the diagonal and
 * a_ij off it. */

static void test_Quad_off_diagonal_indices( void )
{
 ColVariable a , b , c;
 a.set_value( 1 ); b.set_value( -2 ); c.set_value( 3 );

 auto build = [ & ]() {
  DQuadFunction::v_coeff_triple t;
  t.emplace_back( &a , 1.0 , 1.0 );
  t.emplace_back( &b , 2.0 , 2.0 );
  t.emplace_back( &c , 3.0 , 3.0 );
  QuadFunction::v_off_diag_term o;
  o.emplace_back( 1 , 0 , 0.5 );
  return( new QuadFunction( std::move( t ) , std::move( o ) ) );
  };

 // out of range, on either side
 {
  auto q = build();
  assert( throws< std::invalid_argument >( [ q ]() {
   q->modify_term( 3 , 0 , 1.0 ); } ) );
  assert( throws< std::invalid_argument >( [ q ]() {
   q->modify_term( 0 , 3 , 1.0 ); } ) );
  #ifndef LIB_NDEBUG
   assert( throws< std::invalid_argument >( [ q ]() {
    ( void ) q->get_quadratic_coefficient( 0 , 3 ); } ) );
   assert( throws< std::invalid_argument >( [ q ]() {
    ( void ) q->get_quadratic_coefficient( 3 , 3 ); } ) );
  #endif
  assert( q->get_quadratic_coefficient( 1 , 0 ) == 0.5 );
  delete q;
 }

 // the constructor rejects i == j when its checks are on
 #ifndef LIB_NDEBUG
 {
  assert( throws< std::invalid_argument >( [ & ]() {
   DQuadFunction::v_coeff_triple t;
   t.emplace_back( &a , 1.0 , 1.0 );
   t.emplace_back( &b , 2.0 , 2.0 );
   QuadFunction::v_off_diag_term o;
   o.emplace_back( 1 , 1 , 0.5 );
   QuadFunction q( std::move( t ) , std::move( o ) ); } ) );

  // and a term out of range, whichever of the two indices is
  assert( throws< std::invalid_argument >( [ & ]() {
   DQuadFunction::v_coeff_triple t;
   t.emplace_back( &a , 1.0 , 1.0 );
   t.emplace_back( &b , 2.0 , 2.0 );
   QuadFunction::v_off_diag_term o;
   o.emplace_back( 5 , 0 , 0.5 );
   QuadFunction q( std::move( t ) , std::move( o ) ); } ) );
 }
 #endif

 /* modify_term( i , i ) is no off-diagonal term either: it has to be
  * rejected as the constructor does, or else be the diagonal term that
  * get_quadratic_coefficient( i , i ), the value and the gradient all
  * agree upon. */
 {
  auto q = build();
  bool threw = false;
  try {
   q->modify_term( 1 , 1 , 5.0 );
   }
  catch( std::invalid_argument & ) {
   threw = true;
   }
  if( ! threw ) {
   // 2 b^2 + 2 b + 0.5 a b + the rest, with the new term read as
   // get_quadratic_coefficient( 1 , 1 ) says
   const double q11 = q->get_quadratic_coefficient( 1 , 1 );
   const double expected = 1 * 1 + 1 * 1 + q11 * 4 + 2 * ( -2 ) +
			   3 * 9 + 3 * 3 + 0.5 * 1 * ( -2 );
   assert( q->compute( true ) == QuadFunction::kOK );
   std::cout << "QuadFunction::modify_term( 1 , 1 , 5 ): "
	     << "get_quadratic_coefficient( 1 , 1 ) = " << q11
	     << ", value = " << q->get_value() << " ( " << expected
	     << " with that coefficient )" << std::endl;
   check( q->get_value() == expected , "QuadFunction::modify_term( i , i ) "
	  "is rejected, or the value agrees with the coefficient read" );
   }
  delete q;
 }

 // the Hessian
 {
  auto q = build();
  C15Function::DenseHessian dh;
  q->get_hessian_approximation( dh );
  assert( ( dh( 0 , 0 ) == 2 ) && ( dh( 1 , 1 ) == 4 ) &&
	  ( dh( 2 , 2 ) == 6 ) );
  assert( ( dh( 1 , 0 ) == 0.5 ) && ( dh( 0 , 1 ) == 0.5 ) &&
	  ( dh( 2 , 0 ) == 0 ) && ( dh( 2 , 1 ) == 0 ) );

  C15Function::SparseHessian sh( 3 , 3 );
  q->get_hessian_approximation( sh );
  bool same = true;
  for( DIndex i = 0 ; i < 3 ; ++i )
   for( DIndex j = 0 ; j < 3 ; ++j )
    if( sh.coeff( i , j ) != dh( i , j ) )
     same = false;
  if( ! same )
   std::cout << "QuadFunction sparse Hessian: expected H( 0 , 0 ) = 2, "
	     << "H( 1 , 1 ) = 4, H( 2 , 2 ) = 6, got " << sh.coeff( 0 , 0 )
	     << ", " << sh.coeff( 1 , 1 ) << ", " << sh.coeff( 2 , 2 )
	     << std::endl;
  check( same , "QuadFunction::get_hessian_approximation( SparseHessian ) "
	 "== the dense one" );
  delete q;
 }
 }

/*--------------------------------------------------------------------------*/
/* The Modification of a change of an off-diagonal term of a QuadFunction in
 * an FRowConstraint: a QuadFunctionModSbst naming the two Variable, whose
 * Subset is said to be ordered and hence has to be increasing. */

static void test_Quad_Modification( void )
{
 auto block = new AbstractBlock();
 auto x = new std::vector< ColVariable >( 3 );
 block->add_static_variable( *x , "x" );
 auto rows = new std::vector< FRowConstraint >( 1 );
 block->add_static_constraint( *rows , "c" );

 DQuadFunction::v_coeff_triple t;
 for( auto & v : *x )
  t.emplace_back( & v , 1.0 , 1.0 );
 QuadFunction::v_off_diag_term o;
 o.emplace_back( 2 , 0 , 0.5 );
 auto q = new QuadFunction( std::move( t ) , std::move( o ) );
 ( *rows )[ 0 ].set_function( q , eNoMod );

 auto solver = new FakeSolver();
 block->register_Solver( solver );
 take( solver );

 bool threw = false;
 try {
  q->modify_term( 2 , 0 , 1.5 );
  }
 catch( std::exception & e ) {
  std::cout << "QuadFunction::modify_term( 2 , 0 ) with an Observer throws: "
	    << e.what() << std::endl;
  threw = true;
  }
 check( ! threw , "QuadFunction::modify_term( 2 , 0 ) with an Observer "
	"does not throw" );
 assert( q->get_quadratic_coefficient( 0 , 2 ) == 1.5 );

 auto m = take( solver );
 if( ! threw ) {
  assert( m.size() == 1 );
  auto d = std::dynamic_pointer_cast< QuadFunctionModSbst >( m[ 0 ] );
  assert( d && ( d->function() == q ) && ( d->delta() == 1 ) );
  assert( d->vars().size() == 2 );
  const auto & sb = d->subset();
  if( sb != DSubset( { 0 , 2 } ) )
   std::cout << "QuadFunctionModSbst of modify_term( 2 , 0 ): expected "
	     << "subset() = { 0 , 2 }, got { " << sb[ 0 ] << " , " << sb[ 1 ]
	     << " }" << std::endl;
  check( sb == DSubset( { 0 , 2 } ) ,
	 "QuadFunctionModSbst: the Subset said ordered is increasing" );
  check( ( d->vars()[ 0 ] == &( *x )[ sb[ 0 ] ] ) &&
	 ( d->vars()[ 1 ] == &( *x )[ sb[ 1 ] ] ) ,
	 "QuadFunctionModSbst: vars()[ k ] is the Variable of subset()[ k ]" );
  }

 // a new term by Variable, given in decreasing order of index
 q->add_nd_term( &( *x )[ 1 ] , &( *x )[ 0 ] , 2.5 );
 assert( q->get_quadratic_coefficient( 0 , 1 ) == 2.5 );
 m = take( solver );
 assert( m.size() == 1 );
 {
  auto d = std::dynamic_pointer_cast< QuadFunctionModSbst >( m[ 0 ] );
  assert( d && ( d->delta() == 2.5 ) );
  check( ( d->subset() == DSubset( { 0 , 1 } ) ) &&
	 ( d->vars()[ 0 ] == &( *x )[ 0 ] ) &&
	 ( d->vars()[ 1 ] == &( *x )[ 1 ] ) ,
	 "QuadFunctionModSbst of add_nd_term( x1 , x0 ): subset() = { 0 , 1 },"
	 " vars() = { x0 , x1 }" );
 }

 // and one with the same Variable twice is not a non-diagonal term
 assert( throws< std::logic_error >( [ & ]() {
  q->add_nd_term( &( *x )[ 1 ] , &( *x )[ 1 ] , 1.0 ); } ) );

 block->unregister_Solvers( true );
 delete block;
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
    std::cout << "Running tests for QuadFunction\n";
    runAllTests();
    test_edge_cases();
    test_DQuad_modify();
    test_DQuad_convexity();
    test_DQuad_add_remove_readd();
    test_DQuad_linearization();
    test_DQuad_hessian();
    test_DQuad_iterator_clone();
    test_DQuad_Modification();
    test_Quad_off_diagonal_indices();
    test_Quad_Modification();

    if( n_failed ) {
     std::cerr << "QuadFunction_test: " << n_failed << " check(s) failed"
	       << std::endl;
     return( 1 );
     }

    std::cout << "All tests passed!!" << std::endl;
    return( 0 );
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_QuadFunction.cpp -------------------*/
/*--------------------------------------------------------------------------*/
