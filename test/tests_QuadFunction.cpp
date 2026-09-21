/** @file
 * Unit tests for Quadfunction.
 * They test stuff not already tested with Function.
 *
* \author Wim van Ackooij \n
 *         EDF Lab Paris-Saclay \n
 *
 * \copyright &copy; by Wim van Ackooij
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "QuadFunction.h"

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

int main( int argc , char ** argv )
{
    std::cout << "Running tests for QuadFunction\n";
    runAllTests();
    test_edge_cases();
    return( 0 );
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_QuadFunction.cpp -------------------*/
/*--------------------------------------------------------------------------*/
