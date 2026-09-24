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

#include <iostream>
#include <list>
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

 std::cout << "all tests passed" << std::endl;

 return( 0 );
 }
