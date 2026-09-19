/** @file
 * Unit tests for the size variable of a PolyhedralFunctionBlock.
 *
 * The size variable of a PolyhedralFunctionBlock is the multiplier, owned by
 * the father, that set_size_variable() writes into the normalization
 * constraint of the dual representation. The tests check that it gets there
 * whether it is given before the abstract representation exists or after,
 * that a Solver reading the model by columns finds it in that row, and that
 * a change of the global scale keeps its coefficient in step while leaving
 * the constraint and its LinearFunction the objects they were.
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
#include "FRowConstraint.h"
#include "LinearFunction.h"
#include "PolyhedralFunctionBlock.h"

#include <algorithm>
#include <cmath>
#include <iostream>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the dual representation with the global scaling on: bits 0, 1 and 3
static constexpr int dual_global = 1 + 2 + 8;

/// the father, holding the variables of the function and the size variable
struct Father {
 AbstractBlock block;
 std::vector< ColVariable > x;
 std::vector< ColVariable > lambda;
 PolyhedralFunctionBlock * pfb;

 Father( void ) : x( 2 ) , lambda( 1 ) {
  block.add_static_variable( x , "x" );
  block.add_static_variable( lambda , "lambda" );
  pfb = new PolyhedralFunctionBlock( & block );
  block.add_nested_Block( pfb );
  pfb->get_PolyhedralFunction().set_variables( { & x[ 0 ] , & x[ 1 ] } );
  pfb->get_PolyhedralFunction().set_PolyhedralFunction(
			     { { 1 , 2 } , { 3 , 1 } } , { 0 , 1 } );
  }
 };

/*--------------------------------------------------------------------------*/
/// the normalization constraint of the dual representation

static FRowConstraint * normalization( PolyhedralFunctionBlock * pfb )
{
 FRowConstraint * row = nullptr;
 pfb->for_each_constraint_group( [ & row ]( const BaseGroup & group ) {
   if( group.get_name() == "PolyF_norm" )
    row = group.get_Constraint( 0 ) ?
          dynamic_cast< FRowConstraint * >( group.get_Constraint( 0 ) ) :
          nullptr; } );
 return( row );
 }

/*--------------------------------------------------------------------------*/
/// the coefficient of var in the normalization constraint, NaN if absent

static double coefficient( FRowConstraint * row , ColVariable * var )
{
 auto lf = static_cast< LinearFunction * >( row->get_function() );
 const auto k = lf->is_active( var );
 return( k < lf->get_num_active_var() ? lf->get_coefficient( k ) : NAN );
 }

/*--------------------------------------------------------------------------*/
/// true if the Variable counts the row among its active stuff

static bool registered( ColVariable * var , FRowConstraint * row )
{
 const auto & act = var->active_stuff();
 ThinVarDepInterface * r = row;
 return( std::binary_search( act.begin() , act.end() , r ) );
 }

/*--------------------------------------------------------------------------*/
/// checks the row once the size variable is in it

static void check_in( PolyhedralFunctionBlock * pfb , ColVariable * lambda )
{
 auto row = normalization( pfb );
 assert( row );
 assert( std::abs( coefficient( row , lambda ) +
		   1 / pfb->get_v_scale() ) < 1e-12 );
 assert( row->get_lhs() == 0 );
 assert( row->get_rhs() == 0 );
 assert( registered( lambda , row ) );
 }

/*--------------------------------------------------------------------------*/

static void test_before( void )
{
 // given before the abstract representation exists, the Variable is
 // written in when the normalization constraint is built
 Father f;
 assert( f.pfb->set_size_variable( & f.lambda[ 0 ] ) );

 SimpleConfiguration< int > cfg( dual_global );
 f.pfb->generate_abstract_variables( & cfg );
 f.pfb->generate_abstract_constraints();
 check_in( f.pfb , & f.lambda[ 0 ] );

 std::cout << "given before: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

static void test_after( void )
{
 // given afterwards, and without any Modification, it is added in place
 Father f;
 SimpleConfiguration< int > cfg( dual_global );
 f.pfb->generate_abstract_variables( & cfg );
 f.pfb->generate_abstract_constraints();

 auto row = normalization( f.pfb );
 auto function = row->get_function();

 assert( f.pfb->set_size_variable( & f.lambda[ 0 ] , eNoMod ) );
 check_in( f.pfb , & f.lambda[ 0 ] );
 assert( normalization( f.pfb ) == row );
 assert( row->get_function() == function );

 // a second time changes nothing
 const auto nav =
  static_cast< LinearFunction * >( function )->get_num_active_var();
 assert( f.pfb->set_size_variable( & f.lambda[ 0 ] ) );
 assert( static_cast< LinearFunction * >( function )->get_num_active_var()
	 == nav );

 // what is not a ColVariable is refused
 assert( ! f.pfb->set_size_variable( nullptr ) );

 std::cout << "given after: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

static void test_rescale( void )
{
 // a row a thousand times larger moves the global scale, and the
 // coefficient of the size variable follows it in place
 Father f;
 SimpleConfiguration< int > cfg( dual_global );
 f.pfb->generate_abstract_variables( & cfg );
 f.pfb->generate_abstract_constraints();
 f.pfb->generate_objective();
 assert( f.pfb->set_size_variable( & f.lambda[ 0 ] ) );

 auto row = normalization( f.pfb );
 auto function = row->get_function();
 const auto scale = f.pfb->get_v_scale();

 f.pfb->get_PolyhedralFunction().add_row( { 1e4 , 1e4 } , 1e4 );

 assert( f.pfb->get_v_scale() != scale );
 assert( normalization( f.pfb ) == row );
 assert( row->get_function() == function );
 check_in( f.pfb , & f.lambda[ 0 ] );

 std::cout << "rescale: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

static void test_primal( void )
{
 // the primal representation takes no size variable
 Father f;
 SimpleConfiguration< int > cfg( 1 );
 f.pfb->generate_abstract_variables( & cfg );
 f.pfb->generate_abstract_constraints();
 assert( ! f.pfb->set_size_variable( & f.lambda[ 0 ] ) );

 // and given before, it makes the generation of the primal one throw
 Father g;
 assert( g.pfb->set_size_variable( & g.lambda[ 0 ] ) );
 g.pfb->generate_abstract_variables( & cfg );
 bool thrown = false;
 try {
  g.pfb->generate_abstract_constraints();
  }
 catch( std::logic_error & ) {
  thrown = true;
  }
 assert( thrown );

 std::cout << "primal: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- MAIN -----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( void )
{
 test_before();
 test_after();
 test_rescale();
 test_primal();

 std::cout << "All tests passed!!" << std::endl;

 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File tests_SizeVariable.cpp -------------------*/
/*--------------------------------------------------------------------------*/
