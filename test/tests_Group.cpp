/** @file
 * Unit tests for the groups of Variable and Constraint of a Block.
 *
 * The groups are what a Block gives of its Variable and Constraint without
 * the caller having to know the shape of the container they sit in, so the
 * tests go over every shape a :Block can register, and over the one thing
 * the consumers of a group rely on, which is the order in which its elements
 * come out and the runs of contiguous ones they form.
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
#include "OneVarConstraint.h"

#include <cassert>
#include <iostream>
#include <list>
#include <vector>

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// returns the values of the elements of the group, in storage order

static std::vector< double > values_of( const BaseGroup & group )
{
 std::vector< double > values;
 group.for_each_as< ColVariable >( [ & values ]( ColVariable & var ) {
   values.push_back( var.get_value() ); } );
 return( values );
 }

/*--------------------------------------------------------------------------*/
/// returns the lengths of the runs of contiguous elements of the group

static std::vector< Block::Index > runs_of( const BaseGroup & group )
{
 std::vector< Block::Index > runs;
 group.for_each_run_as< ColVariable >(
  [ & runs ]( ColVariable * , Block::Index n ) { runs.push_back( n ); } );
 return( runs );
 }

/*--------------------------------------------------------------------------*/

static void test_shapes( void )
{
 AbstractBlock block;

 // a single element, a vector, a grid, a grid of vectors, a list and a
 // vector of lists: the shapes a :Block can register its Variable in,
 // filled with the numbers 0, 1, 2, ... in storage order
 double next = 0;
 auto fill = [ & next ]( ColVariable & var ) { var.set_value( next++ ); };

 auto single = new ColVariable;
 auto array = new std::vector< ColVariable >( 4 );
 auto grid = new boost::multi_array< ColVariable , 2 >(
					       boost::extents[ 2 ][ 3 ] );
 auto cells = new boost::multi_array< std::vector< ColVariable > , 2 >(
					       boost::extents[ 2 ][ 2 ] );
 for( auto cell = cells->data() ; cell != cells->data() + 4 ; ++cell )
  cell->resize( 2 );
 auto list = new std::list< ColVariable >( 3 );
 auto lists = new std::vector< std::list< ColVariable > >( 2 );
 ( *lists )[ 0 ].resize( 2 );
 ( *lists )[ 1 ].resize( 1 );

 fill( *single );
 for( auto & var : *array )
  fill( var );
 for( auto var = grid->data() ; var != grid->data() + 6 ; ++var )
  fill( *var );
 for( auto cell = cells->data() ; cell != cells->data() + 4 ; ++cell )
  for( auto & var : *cell )
   fill( var );
 for( auto & var : *list )
  fill( var );
 for( auto & cell : *lists )
  for( auto & var : cell )
   fill( var );

 block.add_static_variable( *single , "single" );
 block.add_static_variable( *array , "array" );
 block.add_static_variable( *grid , "grid" );
 block.add_static_variable( *cells , "cells" );
 block.add_dynamic_variable( *list , "list" );
 block.add_dynamic_variable( *lists , "lists" );

 const auto & statics = block.get_static_variable_groups();
 const auto & dynamics = block.get_dynamic_variable_groups();

 assert( statics.size() == 4 );
 assert( dynamics.size() == 2 );

 // each group knows its name, its index, the type of its elements, how many
 // of them there are, and the shape of its grid of cells
 assert( statics[ 0 ]->get_name() == "single" );
 assert( statics[ 2 ]->get_index() == 2 );
 assert( statics[ 0 ]->get_element_type() == typeid( ColVariable ) );
 assert( statics[ 0 ]->elements_are< ColVariable >() );
 assert( statics[ 0 ]->elements_are< Variable >() );
 assert( ! statics[ 0 ]->elements_are< Constraint >() );

 assert( statics[ 0 ]->get_rank() == 0 );
 assert( statics[ 1 ]->get_rank() == 1 );
 assert( statics[ 2 ]->get_rank() == 2 );
 assert( statics[ 2 ]->get_size( 0 ) == 2 );
 assert( statics[ 2 ]->get_size( 1 ) == 3 );

 assert( statics[ 0 ]->get_num_elements() == 1 );
 assert( statics[ 1 ]->get_num_elements() == 4 );
 assert( statics[ 2 ]->get_num_elements() == 6 );
 assert( statics[ 3 ]->get_num_elements() == 8 );
 assert( dynamics[ 0 ]->get_num_elements() == 3 );
 assert( dynamics[ 1 ]->get_num_elements() == 3 );

 // the elements come out in storage order, which is the order in which they
 // were filled: this is what the callers that pair an element with the i-th
 // entry of a vector of their own depend on
 double first = 0;
 for( const auto & group : { statics[ 0 ].get() , statics[ 1 ].get() ,
			     statics[ 2 ].get() , statics[ 3 ].get() ,
			     dynamics[ 0 ].get() , dynamics[ 1 ].get() } ) {
  const auto values = values_of( *group );
  assert( values.size() == group->get_num_elements() );
  for( decltype( values.size() ) i = 0 ; i < values.size() ; ++i )
   assert( values[ i ] == first + i );
  first += values.size();
  }

 // an element is found back from its address, and a foreigner is not
 assert( statics[ 1 ]->get_Variable( 2 ) == & ( *array )[ 2 ] );
 assert( statics[ 2 ]->get_Variable( 4 ) == & ( *grid )[ 1 ][ 1 ] );
 assert( statics[ 3 ]->get_Variable( 2 ) == & ( *cells )[ 0 ][ 1 ][ 0 ] );
 assert( ! statics[ 1 ]->get_Variable( 4 ) );

 // a group of Variable answers nothing when asked for Constraint
 assert( ! statics[ 0 ]->get_Constraint( 0 ) );
 assert( ! statics[ 0 ]->for_each_as< FRowConstraint >(
					 []( FRowConstraint & ) {} ) );

 // the runs of contiguous elements: one for an array, one per inner array
 // for the shapes made of many of them, and one per element when the cells
 // are lists, since their nodes are allocated one by one
 assert( runs_of( *statics[ 0 ] ) == std::vector< Block::Index >( { 1 } ) );
 assert( runs_of( *statics[ 1 ] ) == std::vector< Block::Index >( { 4 } ) );
 assert( runs_of( *statics[ 2 ] ) == std::vector< Block::Index >( { 6 } ) );
 assert( runs_of( *statics[ 3 ] ) ==
	 std::vector< Block::Index >( { 2 , 2 , 2 , 2 } ) );

 // the group is a view: the :Block may resize its container afterwards, and
 // the group says what is there now
 array->resize( 6 );
 assert( statics[ 1 ]->get_num_elements() == 6 );
 list->emplace_back();
 assert( dynamics[ 0 ]->get_num_elements() == 4 );

 std::cout << "shapes: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

static void test_constraints( void )
{
 AbstractBlock block;

 auto rows = new std::vector< FRowConstraint >( 3 );
 auto boxes = new std::vector< ZOConstraint >( 2 );

 block.add_static_constraint( *rows , "rows" );
 block.add_static_constraint( *boxes , "zeroone" );

 const auto & groups = block.get_static_constraint_groups();
 assert( groups.size() == 2 );

 // the question about the type is asked of the group, once for all of its
 // elements, and it is answered through the intermediate classes too
 assert( groups[ 0 ]->elements_are< FRowConstraint >() );
 assert( groups[ 0 ]->elements_are< RowConstraint >() );
 assert( ! groups[ 0 ]->elements_are< OneVarConstraint >() );
 assert( groups[ 1 ]->elements_are< ZOConstraint >() );
 assert( groups[ 1 ]->elements_are< OneVarConstraint >() );
 assert( ! groups[ 1 ]->elements_are< FRowConstraint >() );

 // a caller that treats a handful of types walks the ones it knows and
 // leaves the others alone
 Block::Index seen = 0;
 auto count = [ & seen ]( RowConstraint & ) { ++seen; };

 for( const auto & group : groups )
  for_each_as_any_of< BoxConstraint , LB0Constraint , UB0Constraint ,
		      LBConstraint , UBConstraint , NNConstraint ,
		      NPConstraint , ZOConstraint >( *group , count );

 assert( seen == 2 );

 seen = 0;
 block.for_each_constraint_group( [ & count , & seen ]
				  ( const BaseGroup & group ) {
   group.for_each_as< FRowConstraint >( count ); } );

 assert( seen == 3 );

 std::cout << "constraints: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- MAIN -----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( void )
{
 test_shapes();
 test_constraints();

 std::cout << "All tests passed!!" << std::endl;

 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File tests_Group.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
