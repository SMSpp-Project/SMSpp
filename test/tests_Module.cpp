/** @file
 * Unit test of the C++20 module smspp, built when SMSpp_BUILD_MODULE is ON.
 *
 * The program includes no header of SMS++: it imports the module, and with
 * what that exports it builds an AbstractBlock (Variable, a row with a
 * LinearFunction, a box, an objective), checks the feasibility of a point,
 * writes the Block to the image of a netCDF file in memory and reads it
 * back, and gets a Block from the factory by its classname. Each of these
 * steps names a different part of the core, so a name that the module
 * fails to export, or a class whose factory registration does not survive
 * the import, stops the build or the run.
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

#include <iostream>
#include <memory>
#include <span>
#include <vector>

#undef NDEBUG
#include <cassert>

import smspp;

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( void )
{
 // min x_0 + x_1  s.t.  x_0 + 2 x_1 <= 4 ,  -1 <= x_0 <= 5
 auto block = std::make_unique< AbstractBlock >();

 auto x = new std::vector< ColVariable >( 2 );
 block->add_static_variable( *x , "x" );

 auto rows = new std::vector< FRowConstraint >( 1 );
 ( *rows )[ 0 ].set_function( new LinearFunction(
			  { { & ( *x )[ 0 ] , 1 } , { & ( *x )[ 1 ] , 2 } } ) );
 ( *rows )[ 0 ].set_lhs( - Inf< double >() );
 ( *rows )[ 0 ].set_rhs( 4 );
 block->add_static_constraint( *rows , "r" );

 auto box = new BoxConstraint( block.get() , & ( *x )[ 0 ] , -1 , 5 );
 block->add_static_constraint( *box , "box" );

 block->set_objective( new FRealObjective( block.get() , new LinearFunction(
			  { { & ( *x )[ 0 ] , 1 } , { & ( *x )[ 1 ] , 1 } } ) )
		       , eNoMod );

 // a feasible point and an infeasible one
 ( *x )[ 0 ].set_value( 2 );
 ( *x )[ 1 ].set_value( 1 );
 assert( RowConstraint::is_feasible( ( *rows )[ 0 ] ) );
 ( *x )[ 1 ].set_value( 2 );
 assert( ! RowConstraint::is_feasible( ( *rows )[ 0 ] ) );

 // through the image of a netCDF file in memory and back
 std::vector< unsigned char > image;
 block->Block::serialize( image , eBlockFile );
 assert( ! image.empty() );
 std::unique_ptr< Block > back( Block::deserialize(
			  std::span< const unsigned char >( image ) , 0 ) );
 auto ab = dynamic_cast< AbstractBlock * >( back.get() );
 assert( ab );
 assert( ab->get_number_static_variables() == 1 );
 assert( ab->get_number_static_constraints() == 2 );

 // the factory, by classname
 std::unique_ptr< Block > fresh( Block::new_Block( "AbstractBlock" ) );
 assert( dynamic_cast< AbstractBlock * >( fresh.get() ) );

 std::cout << "module smspp: OK" << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File tests_Module.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
