/** @file
 * Unit tests for what a Solution does when the Block it comes from changes.
 *
 * A Solution holds one value per element of the Block it was read from, so
 * the elements that go away take their value with them: the tests are on
 * Solution::drop_dynamic_values(), i.e., on the dual values of the dynamic
 * RowConstraint that are removed from a group, which whoever holds a dual
 * solution has to see before deciding whether what is left of it is still
 * worth anything. They go over the shapes a group of dynamic Constraint can
 * have, over the cells of a nested Block, over the positions asked for in
 * any order and past what the Solution holds, and over the two :Solution of
 * the core that hold dual values.
 *
 * Then the rest of what a Solution does: the round trip through a netCDF
 * file of RowConstraintSolution and ColRowSolution, a Block with no group
 * and one with empty groups included; clone() full and empty; scale() and
 * sum(); what is_direction() says through all of these; the refusal to be
 * written into a Block of another shape; and the factory.
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
#include "ColRowSolution.h"
#include "FRowConstraint.h"
#include "LinearFunction.h"
#include "OneVarConstraint.h"
#include "RowConstraintSolution.h"

#include <cstdio>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using Index = Block::Index;
using Subset = Block::Subset;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// gives the rows of the list a Function of their own and the given duals
/** The dual values are start, start + 1, ... in list order; a row needs a
 * Function to be a row at all, and one Variable is enough. */

static void fill( std::list< FRowConstraint > & rows , ColVariable & var ,
		  double start )
{
 for( auto & row : rows ) {
  row.set_function( new LinearFunction( { { & var , 1.0 } } ) , eNoMod );
  row.set_lhs( 0 , eNoMod );
  row.set_rhs( 1 , eNoMod );
  row.set_dual( start++ );
  }
 }

/*--------------------------------------------------------------------------*/
/// the dual values of the rows of the list, in list order

static std::vector< double > duals_of( const std::list< FRowConstraint > & rows )
{
 std::vector< double > rv;
 for( const auto & row : rows )
  rv.push_back( row.get_dual() );

 return( rv );
 }

/*--------------------------------------------------------------------------*/
/* Writes the Solution in the Block and gives back the dual values of the
 * rows of the list: what the Solution holds for them after the rows that
 * were removed have been dropped from it. */

static std::vector< double > written_duals( Solution * sol , Block * block ,
					    std::list< FRowConstraint > & rows )
{
 for( auto & row : rows )
  row.set_dual( -1 );
 sol->write( block );

 return( duals_of( rows ) );
 }

/*--------------------------------------------------------------------------*/
/// calls f(), and says which exception it threw: "" if none

static std::string throws( const std::function< void( void ) > & f )
{
 try {
  f();
  }
 catch( const std::invalid_argument & ) {
  return( "invalid_argument" );
  }
 catch( const std::logic_error & ) {
  return( "logic_error" );
  }
 catch( const std::exception & ) {
  return( "exception" );
  }
 return( "" );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- TESTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/* One group whose cell is a plain list: a subset of its rows is removed, in
 * an order of its own, and the Solution has to give back exactly the dual
 * values of those and to keep the others matching the rows that are left. */

static void test_subset_of_a_list( void )
{
 auto block = new AbstractBlock;
 auto var = new ColVariable;
 block->add_static_variable( *var , "x" );
 auto rows = new std::list< FRowConstraint >( 5 );
 fill( *rows , *var , 10 );
 block->add_dynamic_constraint( *rows , "rows" );

 RowConstraintSolution duals;
 duals.read( block );

 // the rows in positions 3 and 1 go, the Block issuing no Modification: the
 // test is on the Solution, which is told by hand what went
 block->remove_dynamic_constraints( *rows , Subset( { 3 , 1 } ) , false ,
				    eNoMod );
 assert( rows->size() == 3 );

 std::vector< double > dropped;
 assert( duals.drop_dynamic_values( block , & *rows , Subset( { 3 , 1 } ) ,
				    dropped ) );

 // the values come back in the order the positions were asked for
 assert( ( dropped == std::vector< double >( { 13 , 11 } ) ) );

 // and what is left goes back to the rows that are left
 assert( ( written_duals( & duals , block , *rows ) ==
	   std::vector< double >( { 10 , 12 , 14 } ) ) );

 delete block;
 std::cout << "subset of a list: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* A group whose cells are a vector of lists: only the cell the rows were
 * removed from is touched, the others keeping every value they had. */

static void test_one_cell_of_a_grid( void )
{
 auto block = new AbstractBlock;
 auto var = new ColVariable;
 block->add_static_variable( *var , "x" );
 auto grid = new std::vector< std::list< FRowConstraint > >( 3 );
 ( *grid )[ 0 ].resize( 2 );
 ( *grid )[ 1 ].resize( 4 );
 ( *grid )[ 2 ].resize( 1 );
 fill( ( *grid )[ 0 ] , *var , 100 );
 fill( ( *grid )[ 1 ] , *var , 200 );
 fill( ( *grid )[ 2 ] , *var , 300 );
 block->add_dynamic_constraint( *grid , "grid" );

 RowConstraintSolution duals;
 duals.read( block );

 // the first two rows of the second cell go
 block->remove_dynamic_constraints( ( *grid )[ 1 ] , Block::Range( 0 , 2 ) ,
				    eNoMod );

 std::vector< double > dropped;
 assert( duals.drop_dynamic_values( block , & ( *grid )[ 1 ] ,
				    Subset( { 0 , 1 } ) , dropped ) );
 assert( ( dropped == std::vector< double >( { 200 , 201 } ) ) );

 assert( ( written_duals( & duals , block , ( *grid )[ 1 ] ) ==
	   std::vector< double >( { 202 , 203 } ) ) );
 assert( ( written_duals( & duals , block , ( *grid )[ 0 ] ) ==
	   std::vector< double >( { 100 , 101 } ) ) );
 assert( ( written_duals( & duals , block , ( *grid )[ 2 ] ) ==
	   std::vector< double >( { 300 } ) ) );

 delete block;
 std::cout << "one cell of a grid: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* The whole cell goes, which is what an empty set of positions means, and
 * the rows that are added to it afterwards find nothing of the old values. */

static void test_whole_cell( void )
{
 auto block = new AbstractBlock;
 auto var = new ColVariable;
 block->add_static_variable( *var , "x" );
 auto rows = new std::list< FRowConstraint >( 3 );
 fill( *rows , *var , 1 );
 block->add_dynamic_constraint( *rows , "rows" );

 RowConstraintSolution duals;
 duals.read( block );

 block->remove_dynamic_constraints( *rows , Subset() , false , eNoMod );
 assert( rows->empty() );

 std::vector< double > dropped;
 assert( duals.drop_dynamic_values( block , & *rows , Subset() , dropped ) );
 assert( ( dropped == std::vector< double >( { 1 , 2 , 3 } ) ) );

 // a new row of that cell gets the default dual value, not one of the three
 std::list< FRowConstraint > more( 1 );
 fill( more , *var , 7 );
 block->add_dynamic_constraints( *rows , more , eNoMod );
 assert( ( written_duals( & duals , block , *rows ) ==
	   std::vector< double >( { 0 } ) ) );

 delete block;
 std::cout << "whole cell: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* A position past what the Solution holds, which happens when the cell was
 * shorter the last time it was read: nothing was held for it, hence zero is
 * what it dropped, and the values that are held are not shifted by it. */

static void test_position_past_the_values( void )
{
 auto block = new AbstractBlock;
 auto var = new ColVariable;
 block->add_static_variable( *var , "x" );
 auto rows = new std::list< FRowConstraint >( 2 );
 fill( *rows , *var , 50 );
 block->add_dynamic_constraint( *rows , "rows" );

 RowConstraintSolution duals;
 duals.read( block );   // two values are held

 std::list< FRowConstraint > more( 2 );
 fill( more , *var , 70 );
 block->add_dynamic_constraints( *rows , more , eNoMod );

 // the two rows that joined the cell after the Solution was read go away
 block->remove_dynamic_constraints( *rows , Block::Range( 2 , 4 ) , eNoMod );

 std::vector< double > dropped;
 assert( duals.drop_dynamic_values( block , & *rows , Subset( { 2 , 3 } ) ,
				    dropped ) );
 assert( ( dropped == std::vector< double >( { 0 , 0 } ) ) );
 assert( ( written_duals( & duals , block , *rows ) ==
	   std::vector< double >( { 50 , 51 } ) ) );

 delete block;
 std::cout << "position past the values: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* The cell of a nested Block, found walking the tree of Solution, and a cell
 * of no Block of the tree, which the Solution has to say it cannot do. */

static void test_nested_and_unknown( void )
{
 auto block = new AbstractBlock;
 auto var = new ColVariable;
 block->add_static_variable( *var , "x" );
 auto root_rows = new std::list< FRowConstraint >( 2 );
 fill( *root_rows , *var , 1000 );
 block->add_dynamic_constraint( *root_rows , "rows" );

 auto inner = new AbstractBlock( block );
 auto inner_var = new ColVariable;
 inner->add_static_variable( *inner_var , "y" );
 auto inner_rows = new std::list< FRowConstraint >( 4 );
 fill( *inner_rows , *inner_var , 2000 );
 inner->add_dynamic_constraint( *inner_rows , "rows" );
 block->add_nested_Block( inner );

 RowConstraintSolution duals;
 duals.read( block );

 inner->remove_dynamic_constraints( *inner_rows , Subset( { 2 } ) , true ,
				    eNoMod );

 // the Solution of the root Block is the one that is asked, and the cell of
 // the nested Block is found from there
 std::vector< double > dropped;
 assert( duals.drop_dynamic_values( block , & *inner_rows , Subset( { 2 } ) ,
				    dropped ) );
 assert( ( dropped == std::vector< double >( { 2002 } ) ) );
 // the Solution of the root Block is written in the root Block, which is
 // where the cells of the nested ones are reached from
 for( auto & row : *inner_rows )
  row.set_dual( -1 );
 assert( ( written_duals( & duals , block , *root_rows ) ==
	   std::vector< double >( { 1000 , 1001 } ) ) );
 assert( ( duals_of( *inner_rows ) ==
	   std::vector< double >( { 2000 , 2001 , 2003 } ) ) );

 // a list that no Block of the tree has registered is not found
 std::list< FRowConstraint > loose( 1 );
 fill( loose , *var , 0 );
 assert( ! duals.drop_dynamic_values( block , & loose , Subset( { 0 } ) ,
				      dropped ) );

 delete block;
 std::cout << "nested Block and unknown cell: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* The two :Solution of the core that hold dual values: the ColRowSolution
 * drops them through the RowConstraintSolution it is made of, while the one
 * that holds only the Variable has no value of a row to drop and says so. */

static void test_the_solutions( void )
{
 auto block = new AbstractBlock;
 auto var = new ColVariable;
 block->add_static_variable( *var , "x" );
 auto rows = new std::list< FRowConstraint >( 3 );
 fill( *rows , *var , 5 );
 block->add_dynamic_constraint( *rows , "rows" );

 ColRowSolution both;
 both.read( block );
 ColVariableSolution primal;
 primal.read( block );

 block->remove_dynamic_constraints( *rows , Subset( { 1 } ) , true , eNoMod );

 std::vector< double > dropped;
 assert( both.drop_dynamic_values( block , & *rows , Subset( { 1 } ) ,
				   dropped ) );
 assert( ( dropped == std::vector< double >( { 6 } ) ) );
 assert( ( written_duals( & both , block , *rows ) ==
	   std::vector< double >( { 5 , 7 } ) ) );

 dropped.clear();
 assert( ! primal.drop_dynamic_values( block , & *rows , Subset( { 1 } ) ,
				       dropped ) );

 delete block;
 std::cout << "the :Solution that hold dual values: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* The values of dynamic ColVariable that are removed, which is the same thing
 * one group over: what a ColVariableSolution holds for them goes with them,
 * and a ColRowSolution drops them as well, looking for the cell among the
 * groups of the Variable before those of the Constraint. */

static void test_dynamic_variables( void )
{
 auto block = new AbstractBlock;
 auto vars = new std::list< ColVariable >( 4 );
 double next = 1;
 for( auto & var : *vars )
  var.set_value( next++ );
 block->add_dynamic_variable( *vars , "x" );

 ColVariableSolution primal;
 primal.read( block );
 ColRowSolution both;
 both.read( block );

 block->remove_dynamic_variables( *vars , Subset( { 2 , 0 } ) , false ,
				  eNoMod );
 assert( vars->size() == 2 );

 std::vector< double > dropped;
 assert( primal.drop_dynamic_values( block , & *vars , Subset( { 2 , 0 } ) ,
				     dropped ) );
 assert( ( dropped == std::vector< double >( { 3 , 1 } ) ) );

 for( auto & var : *vars )
  var.set_value( -1 );
 primal.write( block );
 std::vector< double > left;
 for( auto & var : *vars )
  left.push_back( var.get_value() );
 assert( ( left == std::vector< double >( { 2 , 4 } ) ) );

 // the one that holds both parts finds the cell among the Variable
 dropped.clear();
 assert( both.drop_dynamic_values( block , & *vars , Subset( { 2 , 0 } ) ,
				   dropped ) );
 assert( ( dropped == std::vector< double >( { 3 , 1 } ) ) );

 delete block;
 std::cout << "dynamic Variable: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The Block the round trips below go through: a static group of three rows,
 * a static group with no row at all, a dynamic list of two rows, a dynamic
 * vector of two lists the second of which is empty, and a nested Block with
 * one static row; each Variable has a value and each row a dual of its own,
 * so that a value in the wrong place is seen. */

struct shaped_block {
 AbstractBlock * block;
 std::vector< ColVariable > * vars;
 std::vector< FRowConstraint > * rows;
 std::vector< FRowConstraint > * none;
 std::list< FRowConstraint > * list;
 std::vector< std::list< FRowConstraint > > * cells;
 std::list< ColVariable > * dvars;
 AbstractBlock * inner;
 std::vector< FRowConstraint > * inner_rows;
 };

static shaped_block make_shaped_block( void )
{
 shaped_block s;
 s.block = new AbstractBlock;
 s.vars = new std::vector< ColVariable >( 2 );
 ( *s.vars )[ 0 ].set_value( 1.5 );
 ( *s.vars )[ 1 ].set_value( -2 );
 s.block->add_static_variable( *s.vars , "x" );

 s.dvars = new std::list< ColVariable >( 3 );
 double v = 7;
 for( auto & var : *s.dvars )
  var.set_value( v++ );
 s.block->add_dynamic_variable( *s.dvars , "y" );

 auto & x = ( *s.vars )[ 0 ];
 s.rows = new std::vector< FRowConstraint >( 3 );
 double d = 10;
 for( auto & row : *s.rows ) {
  row.set_function( new LinearFunction( { { & x , 1.0 } } ) , eNoMod );
  row.set_rhs( 1 , eNoMod );
  row.set_dual( d++ );
  }
 s.block->add_static_constraint( *s.rows , "rows" );

 s.none = new std::vector< FRowConstraint >( 0 );
 s.block->add_static_constraint( *s.none , "none" );

 s.list = new std::list< FRowConstraint >( 2 );
 fill( *s.list , x , 20 );
 s.block->add_dynamic_constraint( *s.list , "list" );

 s.cells = new std::vector< std::list< FRowConstraint > >( 2 );
 ( *s.cells )[ 0 ].resize( 2 );
 fill( ( *s.cells )[ 0 ] , x , 30 );
 s.block->add_dynamic_constraint( *s.cells , "cells" );

 s.inner = new AbstractBlock( s.block );
 auto y = new ColVariable;
 y->set_value( 4 );
 s.inner->add_static_variable( *y , "z" );
 s.inner_rows = new std::vector< FRowConstraint >( 1 );
 ( *s.inner_rows )[ 0 ].set_function( new LinearFunction( { { y , 1.0 } } ) ,
				      eNoMod );
 ( *s.inner_rows )[ 0 ].set_dual( 40 );
 s.inner->add_static_constraint( *s.inner_rows , "inner" );
 s.block->add_nested_Block( s.inner );

 return( s );
 }

/*--------------------------------------------------------------------------*/
/// sets every dual and every value of the shaped Block to -1

static void wipe( shaped_block & s )
{
 for( auto & var : *s.vars ) var.set_value( -1 );
 for( auto & var : *s.dvars ) var.set_value( -1 );
 for( auto & row : *s.rows ) row.set_dual( -1 );
 for( auto & row : *s.list ) row.set_dual( -1 );
 for( auto & cell : *s.cells )
  for( auto & row : cell )
   row.set_dual( -1 );
 for( auto & row : *s.inner_rows ) row.set_dual( -1 );
 }

/*--------------------------------------------------------------------------*/
/// whether the shaped Block holds the duals it was made with

static bool has_its_duals( const shaped_block & s )
{
 std::vector< double > got;
 for( auto & row : *s.rows ) got.push_back( row.get_dual() );
 for( auto & row : *s.list ) got.push_back( row.get_dual() );
 for( auto & row : ( *s.cells )[ 0 ] ) got.push_back( row.get_dual() );
 got.push_back( ( *s.inner_rows )[ 0 ].get_dual() );
 return( got == std::vector< double >( { 10 , 11 , 12 , 20 , 21 , 30 , 31 ,
					 40 } ) );
 }

/*--------------------------------------------------------------------------*/
/// whether the shaped Block holds the values it was made with

static bool has_its_values( const shaped_block & s )
{
 std::vector< double > got;
 for( auto & var : *s.vars ) got.push_back( var.get_value() );
 for( auto & var : *s.dvars ) got.push_back( var.get_value() );
 return( got == std::vector< double >( { 1.5 , -2 , 7 , 8 , 9 } ) );
 }

/*--------------------------------------------------------------------------*/
/* Writes the Solution in a netCDF file and reads it back through the
 * factory, which is what new_Solution( netCDF::NcGroup ) and the "type"
 * attribute are for: the Solution read is a new object, of the dynamic type
 * of the one written. */

static Solution * through_a_file( const Solution & sol )
{
 const char * const name = "tests_Solution_round_trip.nc4";
 sol.serialize( std::string( name ) );
 auto read = Solution::deserialize( std::string( name ) );
 std::remove( name );
 return( read );
 }

/*--------------------------------------------------------------------------*/

/* A RowConstraintSolution goes through a netCDF file and comes back with
 * every dual in its place: the static groups, the one that holds no row,
 * the dynamic list, the vector of lists with its empty cell, and the nested
 * Block; written into the Block, it gives back the duals it was read from. */

static void test_row_round_trip( void )
{
 auto s = make_shaped_block();

 RowConstraintSolution duals;
 duals.read( s.block );

 auto read = through_a_file( duals );
 assert( read );
 auto back = dynamic_cast< RowConstraintSolution * >( read );
 assert( back );

 assert( back->get_static_constraint_dual_values() ==
	 duals.get_static_constraint_dual_values() );
 assert( back->get_dynamic_constraint_dual_values() ==
	 duals.get_dynamic_constraint_dual_values() );
 assert( back->get_nested_solutions().size() == 1 );
 assert( back->get_nested_solutions()[ 0 ].
	 get_static_constraint_dual_values() ==
	 duals.get_nested_solutions()[ 0 ].get_static_constraint_dual_values() );

 // the shape survives: the empty static group is there, and so is the
 // empty cell of the vector of lists
 assert( back->get_static_constraint_dual_values().size() == 2 );
 assert( back->get_static_constraint_dual_values()[ 1 ].empty() );
 assert( back->get_dynamic_constraint_dual_values().size() == 2 );
 assert( back->get_dynamic_constraint_dual_values()[ 1 ].size() == 2 );
 assert( back->get_dynamic_constraint_dual_values()[ 1 ][ 1 ].empty() );

 wipe( s );
 back->write( s.block );
 assert( has_its_duals( s ) );

 delete read;
 delete s.block;
 std::cout << "RowConstraintSolution round trip: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* The same for a ColRowSolution, whose two halves travel in two groups of
 * their own: both come back, and both are written. */

static void test_colrow_round_trip( void )
{
 auto s = make_shaped_block();

 ColRowSolution both;
 both.read( s.block );

 auto read = through_a_file( both );
 assert( read );
 auto back = dynamic_cast< ColRowSolution * >( read );
 assert( back );

 assert( back->get_variable_solution().get_static_variable_values() ==
	 both.get_variable_solution().get_static_variable_values() );
 assert( back->get_variable_solution().get_dynamic_variable_values() ==
	 both.get_variable_solution().get_dynamic_variable_values() );
 assert( back->get_constraint_solution().
	 get_static_constraint_dual_values() ==
	 both.get_constraint_solution().get_static_constraint_dual_values() );
 assert( back->get_constraint_solution().
	 get_dynamic_constraint_dual_values() ==
	 both.get_constraint_solution().get_dynamic_constraint_dual_values() );

 wipe( s );
 back->write( s.block );
 assert( has_its_duals( s ) );
 assert( has_its_values( s ) );

 delete read;
 delete s.block;
 std::cout << "ColRowSolution round trip: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* A Block with no group at all, and one whose only groups hold nothing: the
 * three :Solution read, travel and are written without complaining, and
 * come back as empty as they left. */

static void test_round_trip_of_nothing( void )
{
 AbstractBlock empty;

 for( int k = 0 ; k < 2 ; ++k ) {
  AbstractBlock * block = & empty;
  AbstractBlock hollow;
  if( k == 1 ) {  // a static group and a dynamic list, both with no row
   hollow.add_static_variable( * new std::vector< ColVariable >( 0 ) , "x" );
   hollow.add_static_constraint( * new std::vector< FRowConstraint >( 0 ) ,
				 "r" );
   hollow.add_dynamic_constraint( * new std::list< FRowConstraint > , "d" );
   block = & hollow;
   }

  RowConstraintSolution duals;
  duals.read( block );
  auto rd = through_a_file( duals );
  assert( dynamic_cast< RowConstraintSolution * >( rd ) );
  auto rback = static_cast< RowConstraintSolution * >( rd );
  assert( rback->get_static_constraint_dual_values() ==
	  duals.get_static_constraint_dual_values() );
  assert( rback->get_dynamic_constraint_dual_values() ==
	  duals.get_dynamic_constraint_dual_values() );
  rback->write( block );
  delete rd;

  ColVariableSolution primal;
  primal.read( block );
  auto pd = through_a_file( primal );
  assert( dynamic_cast< ColVariableSolution * >( pd ) );
  assert( static_cast< ColVariableSolution * >( pd )->
	  get_static_variable_values() == primal.get_static_variable_values() );
  pd->write( block );
  delete pd;

  ColRowSolution both;
  both.read( block );
  auto bd = through_a_file( both );
  assert( dynamic_cast< ColRowSolution * >( bd ) );
  bd->write( block );
  delete bd;
  }

 std::cout << "round trip of an empty Block: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* A dynamic group whose grid has no cell at all, i.e., a vector of lists of
 * size zero: there is no value to write, but there is a group, and the
 * Solution read back has to have it, or it cannot be written into the Block
 * it was read from. */

static void test_round_trip_of_a_cellless_group( void )
{
 AbstractBlock block;
 auto var = new ColVariable;
 block.add_static_variable( *var , "x" );
 block.add_dynamic_constraint(
		  * new std::vector< std::list< FRowConstraint > >( 0 ) , "c" );
 block.add_dynamic_variable(
		  * new std::vector< std::list< ColVariable > >( 0 ) , "v" );

 RowConstraintSolution duals;
 duals.read( & block );
 assert( duals.get_dynamic_constraint_dual_values().size() == 1 );

 auto rd = through_a_file( duals );
 auto rback = dynamic_cast< RowConstraintSolution * >( rd );
 assert( rback );
 assert( rback->get_dynamic_constraint_dual_values().size() == 1 );
 assert( rback->get_dynamic_constraint_dual_values()[ 0 ].empty() );
 rback->write( & block );
 delete rd;

 ColVariableSolution primal;
 primal.read( & block );
 assert( primal.get_dynamic_variable_values().size() == 1 );
 auto pd = through_a_file( primal );
 auto pback = dynamic_cast< ColVariableSolution * >( pd );
 assert( pback );
 assert( pback->get_dynamic_variable_values().size() == 1 );
 assert( pback->get_dynamic_variable_values()[ 0 ].empty() );
 pback->write( & block );
 delete pd;

 std::cout << "round trip of a group with no cell: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* clone( false ) is a copy, of the same type and with the same values;
 * clone( true ) is of the same type and holds nothing, hence it cannot be
 * written into the Block the original came from. */

static void test_clone( void )
{
 auto s = make_shaped_block();

 ColRowSolution both;
 both.read( s.block );

 auto full = both.clone( false );
 auto hollow = both.clone( true );

 assert( dynamic_cast< ColRowSolution * >( full ) );
 assert( dynamic_cast< ColRowSolution * >( hollow ) );

 assert( full->get_variable_solution().get_static_variable_values() ==
	 both.get_variable_solution().get_static_variable_values() );
 assert( full->get_constraint_solution().get_dynamic_constraint_dual_values()
	 == both.get_constraint_solution().
	    get_dynamic_constraint_dual_values() );
 assert( full->get_constraint_solution().get_nested_solutions().size() == 1 );

 assert( hollow->get_variable_solution().get_static_variable_values().empty() );
 assert( hollow->get_constraint_solution().
	 get_static_constraint_dual_values().empty() );
 assert( hollow->get_constraint_solution().get_nested_solutions().empty() );

 wipe( s );
 full->write( s.block );
 assert( has_its_duals( s ) && has_its_values( s ) );

 assert( ! throws( [ & ] { hollow->write( s.block ); } ).empty() );

 RowConstraintSolution duals;
 duals.read( s.block );
 auto rfull = duals.clone();
 auto rhollow = duals.clone( true );
 assert( rfull->get_static_constraint_dual_values() ==
	 duals.get_static_constraint_dual_values() );
 assert( rhollow->get_static_constraint_dual_values().empty() &&
	 rhollow->get_dynamic_constraint_dual_values().empty() );

 delete rhollow;
 delete rfull;
 delete hollow;
 delete full;
 delete s.block;
 std::cout << "clone: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* scale() gives a new Solution with every value multiplied, and sum() adds
 * a multiple of another Solution to this one, value by value, the nested
 * Block included; a sum into an empty Solution is the scaled other one, and
 * a Solution of another type or shape is refused. */

static void test_scale_and_sum( void )
{
 auto s = make_shaped_block();

 ColVariableSolution primal;
 primal.read( s.block );
 RowConstraintSolution duals;
 duals.read( s.block );

 auto p2 = primal.scale( 2 );
 assert( ( p2->get_static_variable_values()[ 0 ] ==
	   std::vector< double >( { 3 , -4 } ) ) );
 assert( ( p2->get_dynamic_variable_values()[ 0 ][ 0 ] ==
	   std::vector< double >( { 14 , 16 , 18 } ) ) );

 auto d2 = duals.scale( -1 );
 assert( ( d2->get_static_constraint_dual_values()[ 0 ] ==
	   std::vector< double >( { -10 , -11 , -12 } ) ) );
 assert( ( d2->get_dynamic_constraint_dual_values()[ 1 ][ 0 ] ==
	   std::vector< double >( { -30 , -31 } ) ) );
 assert( ( d2->get_nested_solutions()[ 0 ].
	   get_static_constraint_dual_values()[ 0 ] ==
	   std::vector< double >( { -40 } ) ) );

 // p2 + 3 primal = 5 primal, d2 + 2 duals = duals
 p2->sum( & primal , 3 );
 assert( ( p2->get_static_variable_values()[ 0 ] ==
	   std::vector< double >( { 7.5 , -10 } ) ) );
 assert( ( p2->get_dynamic_variable_values()[ 0 ][ 0 ] ==
	   std::vector< double >( { 35 , 40 , 45 } ) ) );
 d2->sum( & duals , 2 );
 assert( d2->get_static_constraint_dual_values() ==
	 duals.get_static_constraint_dual_values() );
 assert( d2->get_dynamic_constraint_dual_values() ==
	 duals.get_dynamic_constraint_dual_values() );
 assert( d2->get_nested_solutions()[ 0 ].get_static_constraint_dual_values()
	 == duals.get_nested_solutions()[ 0 ].
	    get_static_constraint_dual_values() );

 // into an empty one, the sum is the other one scaled
 ColVariableSolution acc;
 acc.sum( & primal , 0.5 );
 assert( ( acc.get_static_variable_values()[ 0 ] ==
	   std::vector< double >( { 0.75 , -1 } ) ) );
 RowConstraintSolution racc;
 racc.sum( & duals , 1 );
 assert( racc.get_static_constraint_dual_values() ==
	 duals.get_static_constraint_dual_values() );

 // another type, and another shape, are refused
 assert( throws( [ & ] { p2->sum( & duals , 1 ); } ) == "invalid_argument" );
 assert( throws( [ & ] { d2->sum( & primal , 1 ); } ) == "invalid_argument" );

 AbstractBlock other;
 other.add_static_variable( * new std::vector< ColVariable >( 5 ) , "x" );
 ColVariableSolution wrong;
 wrong.read( & other );
 assert( ! throws( [ & ] { p2->sum( & wrong , 1 ); } ).empty() );

 // what a scaled Solution holds goes back into the Block
 auto p1 = primal.scale( 1 );
 wipe( s );
 p1->write( s.block );
 assert( has_its_values( s ) );

 delete p1;
 delete d2;
 delete p2;
 delete s.block;
 std::cout << "scale and sum: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* Whether a Solution holds a direction travels with it: clone(), full or
 * empty, scale() and the netCDF round trip keep it, and sum() is a direction only
 * if both sides are one [see Solution::is_direction()]. */

static void test_is_direction( void )
{
 auto s = make_shaped_block();

 ColVariableSolution primal;
 primal.read( s.block );
 RowConstraintSolution duals;
 duals.read( s.block );
 ColRowSolution both;
 both.read( s.block );

 // read() takes it from the Block, which has no ray
 assert( ! primal.is_direction() );
 assert( ! duals.is_direction() );
 assert( ! both.is_direction() );

 for( Solution * sol : std::vector< Solution * >( { & primal , & duals ,
						   & both } ) ) {
  sol->is_direction( true );

  auto c = sol->clone( false );
  assert( c->is_direction() );
  auto sc = sol->scale( 3 );
  assert( sc->is_direction() );
  auto rt = through_a_file( *sol );
  assert( rt && rt->is_direction() );

  // an empty clone says what the original holds all the same
  auto e = sol->clone( true );
  assert( e->is_direction() );
  delete e;

  // a direction plus a direction is a direction
  c->sum( sol , 1 );
  assert( c->is_direction() );

  // a direction plus a solution is not, whichever side the solution is on
  auto plain = sol->clone( false );
  plain->is_direction( false );
  c->sum( plain , 1 );
  assert( ! c->is_direction() );
  sc->sum( plain , 1 );
  assert( ! sc->is_direction() );
  plain->sum( sol , 1 );
  assert( ! plain->is_direction() );

  // and a solution comes back from the file as a solution
  auto rt2 = through_a_file( *plain );
  assert( rt2 && ( ! rt2->is_direction() ) );

  delete rt2;
  delete plain;
  delete rt;
  delete sc;
  delete c;
  }

 delete s.block;
 std::cout << "is_direction: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* A Solution written into a Block of another shape is an error [see
 * Solution]: fewer groups, a group of another size, fewer cells in a dynamic
 * group, another number of nested Block. Each is refused with an
 * exception. */

static void test_write_into_another_shape( void )
{
 auto s = make_shaped_block();

 ColVariableSolution primal;
 primal.read( s.block );
 RowConstraintSolution duals;
 duals.read( s.block );
 ColRowSolution both;
 both.read( s.block );

 // fewer groups
 AbstractBlock bare;
 assert( ! throws( [ & ] { primal.write( & bare ); } ).empty() );
 assert( ! throws( [ & ] { duals.write( & bare ); } ).empty() );
 assert( ! throws( [ & ] { both.write( & bare ); } ).empty() );

 // the same groups, one of them of another size
 AbstractBlock sized;
 sized.add_static_variable( * new std::vector< ColVariable >( 3 ) , "x" );
 sized.add_dynamic_variable( * new std::list< ColVariable >( 3 ) , "y" );
 assert( ! throws( [ & ] { primal.write( & sized ); } ).empty() );

 AbstractBlock rsized;
 rsized.add_static_constraint( * new std::vector< FRowConstraint >( 4 ) ,
			       "rows" );
 rsized.add_static_constraint( * new std::vector< FRowConstraint >( 0 ) ,
			       "none" );
 rsized.add_dynamic_constraint( * new std::list< FRowConstraint > , "list" );
 rsized.add_dynamic_constraint(
		 * new std::vector< std::list< FRowConstraint > >( 2 ) , "cells" );
 assert( ! throws( [ & ] { duals.write( & rsized ); } ).empty() );

 // the same static groups, a dynamic group with another number of cells
 AbstractBlock celled;
 celled.add_static_constraint( * new std::vector< FRowConstraint >( 3 ) ,
			       "rows" );
 celled.add_static_constraint( * new std::vector< FRowConstraint >( 0 ) ,
			       "none" );
 celled.add_dynamic_constraint( * new std::list< FRowConstraint > , "list" );
 celled.add_dynamic_constraint(
		 * new std::vector< std::list< FRowConstraint > >( 3 ) , "cells" );
 assert( ! throws( [ & ] { duals.write( & celled ); } ).empty() );

 // the same groups, and no nested Block
 AbstractBlock flat;
 flat.add_static_constraint( * new std::vector< FRowConstraint >( 3 ) ,
			     "rows" );
 flat.add_static_constraint( * new std::vector< FRowConstraint >( 0 ) ,
			     "none" );
 flat.add_dynamic_constraint( * new std::list< FRowConstraint >( 2 ) , "list" );
 flat.add_dynamic_constraint(
		 * new std::vector< std::list< FRowConstraint > >( 2 ) , "cells" );
 assert( ! throws( [ & ] { duals.write( & flat ); } ).empty() );

 // while the Block it came from takes it
 duals.write( s.block );
 primal.write( s.block );
 both.write( s.block );

 delete s.block;
 std::cout << "write into another shape: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* The factory knows the :Solution of the core by name and makes an empty
 * one of the right type; a name it does not know is refused. A file that is
 * not a Solution file gives no Solution. */

static void test_factory( void )
{
 for( const std::string name : { "Solution" , "ColVariableSolution" ,
				 "RowConstraintSolution" , "ColRowSolution" } ) {
  auto sol = Solution::new_Solution( name );
  assert( sol );
  assert( sol->classname() == name );
  delete sol;
  }

 assert( dynamic_cast< ColRowSolution * >(
	 std::unique_ptr< Solution >( Solution::new_Solution(
				       "ColRowSolution" ) ).get() ) );

 assert( throws( [] { delete Solution::new_Solution( "NoSuchSolution" ); } )
	 == "invalid_argument" );

 // a netCDF file of another kind is not taken for a Solution file
 const char * const name = "tests_Solution_not_a_solution.nc4";
 {
  netCDF::NcFile f( name , netCDF::NcFile::replace );
  f.putAtt( "SMS++_file_type" , netCDF::NcInt() , eBlockFile );
  }
 auto none = Solution::deserialize( std::string( name ) );
 std::remove( name );
 assert( ! none );

 std::cout << "factory: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*----------------------------------- MAIN ---------------------------------*/
/*--------------------------------------------------------------------------*/

int main( void )
{
 test_subset_of_a_list();
 test_one_cell_of_a_grid();
 test_whole_cell();
 test_position_past_the_values();
 test_nested_and_unknown();
 test_the_solutions();
 test_dynamic_variables();

 test_row_round_trip();
 test_colrow_round_trip();
 test_round_trip_of_nothing();
 test_round_trip_of_a_cellless_group();
 test_clone();
 test_scale_and_sum();
 test_is_direction();
 test_write_into_another_shape();
 test_factory();

 std::cout << "All tests passed!!" << std::endl;

 return( 0 );
 }
