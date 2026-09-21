/** @file
 * Unit tests for the groups of Variable and Constraint of a Block.
 *
 * The groups are what a Block gives of its Variable and Constraint without
 * the caller having to know the shape of the container they sit in, so the
 * tests go over every shape a :Block can register, and over the one thing
 * the consumers of a group rely on, which is the order in which its elements
 * come out and the runs of contiguous ones they form. The groups whose cells
 * are vectors are also taken through the two consumers that copy a group,
 * the Solution and the abstract copy of a Block.
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
#include "BlockInspection.h"
#include "ColVariableSolution.h"
#include "FRowConstraint.h"
#include "LinearFunction.h"
#include "OneVarConstraint.h"
#include "RowConstraintSolution.h"

#include <array>
#include <iostream>
#include <list>
#include <vector>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

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

 // a single element, a vector, a grid, a vector of vectors, a grid of
 // vectors, a list and a vector of lists: the shapes a :Block can register
 // its Variable in, the same ones it has for its Constraint, filled with the
 // numbers 0, 1, 2, ... in storage order
 double next = 0;
 auto fill = [ & next ]( ColVariable & var ) { var.set_value( next++ ); };

 auto single = new ColVariable;
 auto array = new std::vector< ColVariable >( 4 );
 auto grid = new boost::multi_array< ColVariable , 2 >(
					       boost::extents[ 2 ][ 3 ] );
 auto jagged = new std::vector< std::vector< ColVariable > >( 2 );
 ( *jagged )[ 0 ].resize( 3 );
 ( *jagged )[ 1 ].resize( 2 );
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
 for( auto & cell : *jagged )
  for( auto & var : cell )
   fill( var );
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
 block.add_static_variable( *jagged , "jagged" );
 block.add_static_variable( *cells , "cells" );
 block.add_dynamic_variable( *list , "list" );
 block.add_dynamic_variable( *lists , "lists" );

 const auto & statics = block.get_static_variable_groups();
 const auto & dynamics = block.get_dynamic_variable_groups();

 assert( statics.size() == 5 );
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
 assert( statics[ 3 ]->get_num_elements() == 5 );
 assert( statics[ 4 ]->get_num_elements() == 8 );
 assert( dynamics[ 0 ]->get_num_elements() == 3 );
 assert( dynamics[ 1 ]->get_num_elements() == 3 );

 // the elements come out in storage order, which is the order in which they
 // were filled: this is what the callers that pair an element with the i-th
 // entry of a vector of their own depend on
 double first = 0;
 for( const auto & group : { statics[ 0 ].get() , statics[ 1 ].get() ,
			     statics[ 2 ].get() , statics[ 3 ].get() ,
			     statics[ 4 ].get() ,
			     dynamics[ 0 ].get() , dynamics[ 1 ].get() } ) {
  const auto values = values_of( *group );
  assert( values.size() == group->get_num_elements() );
  for( decltype( values.size() ) i = 0 ; i < values.size() ; ++i )
   assert( values[ i ] == first + i );
  first += values.size();
  }

 // the indices of a cell of the grid, which is how an element gets a name
 // that says where it sits rather than how far along it is
 std::array< Block::Index , BaseGroup::max_rank > index;
 statics[ 2 ]->get_multi_index( 0 , index.data() );
 assert( ( index[ 0 ] == 0 ) && ( index[ 1 ] == 0 ) );
 statics[ 2 ]->get_multi_index( 4 , index.data() );
 assert( ( index[ 0 ] == 1 ) && ( index[ 1 ] == 1 ) );
 statics[ 2 ]->get_multi_index( 5 , index.data() );
 assert( ( index[ 0 ] == 1 ) && ( index[ 1 ] == 2 ) );
 statics[ 4 ]->get_multi_index( 3 , index.data() );
 assert( ( index[ 0 ] == 1 ) && ( index[ 1 ] == 1 ) );

 // an element is found back from its address, and a foreigner is not
 assert( statics[ 1 ]->get_Variable( 2 ) == & ( *array )[ 2 ] );
 assert( statics[ 2 ]->get_Variable( 4 ) == & ( *grid )[ 1 ][ 1 ] );
 assert( statics[ 3 ]->get_Variable( 3 ) == & ( *jagged )[ 1 ][ 0 ] );
 assert( statics[ 4 ]->get_Variable( 2 ) == & ( *cells )[ 0 ][ 1 ][ 0 ] );
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
	 std::vector< Block::Index >( { 3 , 2 } ) );
 assert( runs_of( *statics[ 4 ] ) ==
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

static void test_cells_of_vectors( void )
{
 auto block = new AbstractBlock;

 // a grid whose cells are vectors of different lengths, one of them empty,
 // a vector of vectors with an empty one, and a grid of vectors of rows,
 // one row per Variable of the first grid, sitting in the same cell
 const std::array< Block::Index , 4 > length = { 3 , 0 , 1 , 2 };

 auto cells = new boost::multi_array< std::vector< ColVariable > , 2 >(
						       boost::extents[ 2 ][ 2 ] );
 auto jagged = new std::vector< std::vector< ColVariable > >( 3 );
 ( *jagged )[ 0 ].resize( 2 );
 ( *jagged )[ 2 ].resize( 3 );
 auto rows = new boost::multi_array< std::vector< FRowConstraint > , 2 >(
						       boost::extents[ 2 ][ 2 ] );

 double next = 1;
 for( Block::Index c = 0 ; c < 4 ; ++c ) {
  cells->data()[ c ].resize( length[ c ] );
  for( auto & var : cells->data()[ c ] )
   var.set_value( next++ );
  }
 for( auto & cell : *jagged )
  for( auto & var : cell )
   var.set_value( next++ );

 double dual = 100;
 for( Block::Index c = 0 ; c < 4 ; ++c ) {
  rows->data()[ c ].resize( length[ c ] );
  for( Block::Index j = 0 ; j < length[ c ] ; ++j ) {
   auto & row = rows->data()[ c ][ j ];
   row.set_function( new LinearFunction( { { & cells->data()[ c ][ j ] ,
					       1.0 } } ) , eNoMod );
   row.set_lhs( 0 , eNoMod );
   row.set_rhs( 10 + j , eNoMod );
   row.set_dual( dual++ );
   }
  }

 block->add_static_variable( *cells , "cells" );
 block->add_static_variable( *jagged , "jagged" );
 block->add_static_constraint( *rows , "rows" );

 // the two Solution give back what they took, element by element, with the
 // empty cells in between not shifting anything
 ColVariableSolution primal;
 primal.read( block );
 for( Block::Index c = 0 ; c < 4 ; ++c )
  for( auto & var : cells->data()[ c ] )
   var.set_value( -1 );
 for( auto & cell : *jagged )
  for( auto & var : cell )
   var.set_value( -1 );
 primal.write( block );

 next = 1;
 for( Block::Index c = 0 ; c < 4 ; ++c )
  for( auto & var : cells->data()[ c ] )
   assert( var.get_value() == next++ );
 for( auto & cell : *jagged )
  for( auto & var : cell )
   assert( var.get_value() == next++ );

 RowConstraintSolution duals;
 duals.read( block );
 for( Block::Index c = 0 ; c < 4 ; ++c )
  for( auto & row : rows->data()[ c ] )
   row.set_dual( 0 );
 duals.write( block );

 dual = 100;
 for( Block::Index c = 0 ; c < 4 ; ++c )
  for( auto & row : rows->data()[ c ] )
   assert( row.get_dual() == dual++ );

 // the abstract copy keeps the shape: a grid of vectors becomes a grid of
 // vectors of the same lengths, not a grid of single elements, and each
 // element of the copy is paired with the one in the same place
 auto copy = new AbstractBlock;
 copy->mirror( block );
 assert( copy->get_mirror_issues().empty() );

 const auto & vars = copy->get_static_variable_groups();
 const auto & cons = copy->get_static_constraint_groups();
 assert( ( vars.size() == 2 ) && ( cons.size() == 1 ) );

 assert( vars[ 0 ]->get_layout() == BaseGroup::eJagged );
 assert( vars[ 0 ]->get_rank() == 2 );
 assert( cons[ 0 ]->get_layout() == BaseGroup::eJagged );
 assert( cons[ 0 ]->get_rank() == 2 );

 auto copy_cells = static_cast< boost::multi_array<
  std::vector< ColVariable > , 2 > * >( vars[ 0 ]->get_container() );
 auto copy_jagged = static_cast< std::vector< std::vector< ColVariable > > *
				 >( vars[ 1 ]->get_container() );
 auto copy_rows = static_cast< boost::multi_array<
  std::vector< FRowConstraint > , 2 > * >( cons[ 0 ]->get_container() );

 assert( copy_cells->num_elements() == 4 );
 assert( copy_rows->num_elements() == 4 );
 assert( copy_jagged->size() == 3 );
 for( Block::Index c = 0 ; c < 3 ; ++c )
  assert( ( *copy_jagged )[ c ].size() == ( *jagged )[ c ].size() );

 for( Block::Index c = 0 ; c < 4 ; ++c ) {
  assert( copy_cells->data()[ c ].size() == length[ c ] );
  assert( copy_rows->data()[ c ].size() == length[ c ] );
  for( Block::Index j = 0 ; j < length[ c ] ; ++j ) {
   const auto & var = cells->data()[ c ][ j ];
   const auto & copy_var = copy_cells->data()[ c ][ j ];
   const auto & copy_row = copy_rows->data()[ c ][ j ];
   assert( copy->mirror_of( & var ) == & copy_var );
   assert( copy_var.get_value() == var.get_value() );
   assert( copy->mirror_of( & rows->data()[ c ][ j ] ) == & copy_row );
   assert( copy_row.get_rhs() == 10 + j );

   // the row of the copy is written in the Variable of the copy
   auto f = static_cast< LinearFunction * >( copy_row.get_function() );
   assert( f->get_num_active_var() == 1 );
   assert( f->get_active_var( 0 ) == & copy_var );
   }
  }

 // a Solution works on the copy as it does on the original
 ColVariableSolution copy_primal;
 copy_primal.read( copy );
 copy_primal.write( copy );

 // the copy disposes of the containers it made, those of the original are
 // of whoever registered them, the rows going before the Variable they use
 delete copy;
 delete block;
 delete rows;
 delete jagged;
 delete cells;

 std::cout << "cells of vectors: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

static void test_multi_array_layout( void )
{
 // a grid is read with the last index running fastest and its indices
 // starting at 0, which is how its cells are named and how it is copied: one
 // stored in the order of Fortran, or with its indices starting elsewhere,
 // is refused rather than read wrongly
 auto fortran = new boost::multi_array< ColVariable , 2 >(
			  boost::extents[ 2 ][ 3 ] , boost::fortran_storage_order() );
 auto based = new boost::multi_array< std::vector< ColVariable > , 2 >(
						       boost::extents[ 2 ][ 3 ] );
 based->reindex( 1 );
 auto plain = new boost::multi_array< ColVariable , 2 >(
						       boost::extents[ 2 ][ 3 ] );

 auto refused = []( auto && make ) {
  try {
   make();
   }
  catch( std::invalid_argument & ) {
   return( true );
   }
  return( false );
  };

 assert( refused( [ & ] { StaticGroup< ColVariable > group( fortran ); } ) );
 assert( refused( [ & ] {
   CellGroup< ColVariable , std::vector< ColVariable > > group( based ); } ) );
 assert( ! refused( [ & ] { StaticGroup< ColVariable > group( plain ); } ) );

 delete plain;
 delete based;
 delete fortran;

 std::cout << "multi_array layout: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- MAIN -----------------------------------*/
/*--------------------------------------------------------------------------*/

static void test_names( void )
{
 // the name a group gives to an element is the name of the group, or its
 // index between angle brackets when it has none, followed by the indices
 // of its cell in the grid and, when the cells are collections, by the
 // position of the element inside its own cell
 AbstractBlock b;

 std::vector< ColVariable > v( 3 );
 b.add_static_variable( v , "x" );

 boost::multi_array< ColVariable , 2 > m( boost::extents[ 2 ][ 3 ] );
 b.add_static_variable( m , "M" );

 std::vector< std::vector< ColVariable > > w( 2 );
 w[ 0 ].resize( 1 );
 w[ 1 ].resize( 2 );
 b.add_static_variable( w );   // no name: the index answers for it

 assert( inspection::name_of( & b , & v[ 2 ] ) == "x[ 2 ]" );
 assert( inspection::name_of( & b , & m[ 1 ][ 2 ] ) == "M[ 1 ][ 2 ]" );
 assert( inspection::name_of( & b , & w[ 1 ][ 1 ] ) == "<2>[ 1 ][ 1 ]" );

 // an element that is not in the Block has no name
 ColVariable stranger;
 assert( inspection::name_of( & b , & stranger ).empty() );

 // the walk names every element, in storage order
 std::vector< std::string > seen;
 assert( inspection::for_each_named_as< ColVariable >(
	       *b.get_static_variable_groups()[ 2 ] ,
	       [ & seen ]( const std::string & name , ColVariable & ) {
		seen.push_back( name ); } ) );
 assert( seen.size() == 3 );
 assert( seen[ 0 ] == "<2>[ 0 ][ 0 ]" );
 assert( seen[ 1 ] == "<2>[ 1 ][ 0 ]" );
 assert( seen[ 2 ] == "<2>[ 1 ][ 1 ]" );

 // a caller writing the name into a file whose format takes neither brackets
 // nor spaces asks for the separators it can afford, the marker of a group
 // with no name being one of them
 const inspection::name_format lp = { "_" , "" , "v" , "" };

 assert( inspection::name_of_cell(
	       *b.get_static_variable_groups()[ 1 ] , 5 ,
	       Inf< Block::Index >() , lp ) == "M_1_2" );

 seen.clear();
 assert( inspection::for_each_named_as< ColVariable >(
	       *b.get_static_variable_groups()[ 2 ] ,
	       [ & seen ]( const std::string & name , ColVariable & ) {
		seen.push_back( name ); } , lp ) );
 assert( seen[ 0 ] == "v2_0_0" );
 assert( seen[ 2 ] == "v2_1_1" );

 std::cout << "names: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A group with no element at all. It is not a curiosity: a dynamic group is
 * born empty, and what the group answers then has already broken a Solver
 * once, MILPSolver refusing a model over a dynamic group that held nothing
 * yet. What holds is written here so that it keeps holding. */

static void test_empty_group( void )
{
 AbstractBlock b;

 auto none = new std::vector< ColVariable >( 0 );
 b.add_static_variable( *none , "none" );

 auto cells = new std::vector< std::list< FRowConstraint > >( 2 );
 b.add_dynamic_constraint( *cells , "born_empty" );

 const auto & sv = b.get_static_variable_groups();
 const auto & dc = b.get_dynamic_constraint_groups();

 assert( sv[ 0 ]->get_num_elements() == 0 );
 assert( dc[ 0 ]->get_num_elements() == 0 );

 /* The type of the elements is a field of the group, so asking for the
  * EXACT type is answered without looking at any element and holds on an
  * empty group; asking for a BASE class is answered by casting an element,
  * and on an empty group there is none to cast, so it answers false. That
  * is the trap: whoever asks elements_are() of a base class has to say what
  * an empty group means for them, since the answer is not "no elements of
  * that type" but "no elements". */

 assert( sv[ 0 ]->elements_are< ColVariable >() );
 assert( ! sv[ 0 ]->elements_are< Variable >() );
 assert( dc[ 0 ]->elements_are< FRowConstraint >() );
 assert( ! dc[ 0 ]->elements_are< RowConstraint >() );

 /* And the size the inspection reports is 0, not something undefined, and
  * it is 0 whatever type is asked of it: an empty group holds no element of
  * any type and says nothing about the type it will hold. */

 assert( inspection::get_element_size< ColVariable >( & b , true , 0 ) == 0 );
 assert( inspection::get_element_size< Variable >( & b , true , 0 ) == 0 );
 assert( inspection::get_element_size< FRowConstraint >( & b , false , 0 )
	 == 0 );
 assert( inspection::get_element_size< RowConstraint >( & b , false , 0 )
	 == 0 );

 // walking one calls nobody, and says it walked it
 Block::Index seen = 0;
 assert( sv[ 0 ]->for_each_as< ColVariable >(
	       [ & seen ]( ColVariable & ) { ++seen; } ) );
 assert( seen == 0 );

 // naming one names nobody, rather than naming a cell that is not there
 std::vector< std::string > names;
 assert( inspection::for_each_named_as< ColVariable >( *sv[ 0 ] ,
	       [ & names ]( const std::string & n , ColVariable & ) {
		names.push_back( n ); } ) );
 assert( names.empty() );

 // an element of another Block has no name here either
 ColVariable stranger;
 assert( inspection::name_of( & b , & stranger ).empty() );

 /* The two containers are NOT deleted here: the Block still has them
  * registered, and its destructor walks its groups, so freeing what a group
  * views is a read of memory that is gone. */

 std::cout << "empty group: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

int main( void )
{
 test_shapes();
 test_constraints();
 test_cells_of_vectors();
 test_multi_array_layout();
 test_names();

 test_empty_group();

 std::cout << "All tests passed!!" << std::endl;

 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File tests_Group.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
