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
}

/*--------------------------------------------------------------------------*/

int main() {
    std::cout << "Running tests for QuadFunction\n";
    runAllTests();
    return( 0 );
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_QuadFunction.cpp -------------------*/
/*--------------------------------------------------------------------------*/
