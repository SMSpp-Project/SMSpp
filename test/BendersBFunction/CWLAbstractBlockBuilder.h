/*--------------------------------------------------------------------------*/
/*---------------------- File CWLAbstractBlockBuilder ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Reads an instance of the Capacitated Warehouse Location problem and
 * produces an AbstractBlock associated with that instance.
 *
 * \version 0.10
 *
 * \date 22 - 11 - 2019
 *
 * \author Rafael Durbano Lobato \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Rafael Durbano Lobato
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractBlock.h"
#include "BendersBFunction.h"
#include "FRealObjective.h"
#include "FRowConstraint.h"
#include "LinearFunction.h"

#include <iostream>
#include <fstream>
#include <string>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

// namespace for the tests of the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it { namespace tests {

/*--------------------------------------------------------------------------*/
/*-------------------------------- CLASSES ---------------------------------*/
/*--------------------------------------------------------------------------*/

struct CWLInstance {

 void set( int num_locations , int num_customers ) {
  this->num_locations = num_locations;
  this->num_customers = num_customers;
  capacity.resize( num_locations );
  fixed_cost.resize( num_locations );
  demand.resize( num_customers );
  cost.resize( num_locations );
  for( int i = 0 ; i < num_locations ; ++i ) {
   cost[ i ].resize( num_customers );
  }
 }

 int num_locations, num_customers;
 std::vector< double > capacity;
 std::vector< double > fixed_cost;
 std::vector< double > demand;
 std::vector< std::vector< double > > cost;
};

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

CWLInstance read_cwl_instance( std::string file_name ) {
 std::ifstream stream( file_name );
 if( ! stream.is_open() )
  throw std::invalid_argument( "File " + file_name + " could not be opened." );

 CWLInstance instance;
 int num_locations , num_customers;

 stream >> num_locations >> num_customers;

 if( stream.fail() || stream.bad() )
  throw std::runtime_error( "Error while reading file " + file_name );

 instance.set( num_locations , num_customers );

 for( int i = 0 ; i < num_locations ; ++i ) {
  stream >> instance.capacity[ i ] >> instance.fixed_cost[ i ];
  if( stream.fail() || stream.bad() )
   throw std::runtime_error( "Error while reading file " + file_name );
 }

 for( int j = 0 ; j < num_customers ; ++j ) {
  stream >> instance.demand[ j ];
  if( stream.fail() || stream.bad() )
   throw std::runtime_error( "Error while reading file " + file_name );
  for( int i = 0 ; i < num_locations ; ++i ) {
   stream >> instance.cost[ i ][ j ];
   if( stream.fail() || stream.bad() )
    throw std::runtime_error( "Error while reading file " + file_name );
  }
 }

 return instance;
}

/*--------------------------------------------------------------------------*/

AbstractBlock * build_CWL_block( std::string file_name ,
                                 bool continuous_relaxation = false ) {

 auto instance = read_cwl_instance( file_name );

 auto block = new AbstractBlock();

 // Variables

 auto y = new std::vector< ColVariable >( instance.num_locations );
 for( auto & y_i : * y ) {
  y_i.is_unitary( true );
  if( ! continuous_relaxation )
   y_i.is_integer( true );
 }

 block->add_static_variable( * y );

 using array_type = typename boost::multi_array< ColVariable , 2 >;
 boost::array< typename array_type::index , 2 > shape =
  { instance.num_locations , instance.num_customers };
 auto x = new array_type( shape );

 auto p_x = x->data();
 for( int k = 0 ; k < x->num_elements() ; ++k , ++p_x )
  p_x->is_positive( true );

 block->add_static_variable( * x );

 // Constraints

 {
  auto demand_fulfillment =
   new std::vector< FRowConstraint >( instance.num_customers );
  for( int j = 0 ; j < instance.num_customers ; ++j ) {
   auto function = new LinearFunction();
   for( int i = 0 ; i < instance.num_locations ; ++i ) {
    function->add_variable( & ( * x )[ i ][ j ] , 1 );
   }
   ( * demand_fulfillment )[ j ].set_function( function );
   ( * demand_fulfillment )[ j ].set_both( 1 );
  }
  block->add_static_constraint( * demand_fulfillment );
 }

 {
  auto capacity_constraints =
   new std::vector< FRowConstraint >( instance.num_locations );
  for( int i = 0 ; i < instance.num_locations ; ++i ) {
   auto function = new LinearFunction();
   for( int j = 0 ; j < instance.num_customers ; ++j ) {
    function->add_variable( & ( * x )[ i ][ j ] , instance.demand[ j ] );
   }
   function->add_variable( & ( * y )[ i ] , - instance.capacity[ i ] );
   ( * capacity_constraints )[ i ].set_function( function );
   ( * capacity_constraints )[ i ].set_rhs( 0 );
  }
  block->add_static_constraint( * capacity_constraints );
 }

 // Objective function

 {
  auto function = new LinearFunction();
  for( int i = 0 ; i < instance.num_locations ; ++i ) {
   function->add_variable( & ( * y )[ i ] , instance.fixed_cost[ i ] );
   for( int j = 0 ; j < instance.num_customers ; ++j ) {
    function->add_variable( & ( * x )[ i ][ j ] , instance.cost[ i ][ j ] );
   }
  }
  auto objective = new FRealObjective( block , function );
  objective->set_sense( Objective::eMin );
  block->set_objective( objective );
 }

 return block;
}

/*--------------------------------------------------------------------------*/

BendersBFunction * build_Benders_function( const CWLInstance & instance ,
                                           std::vector< ColVariable * > && y ,
                                           Solver * solver ) {

 auto block = new AbstractBlock();

 // Variables

 using array_type = typename boost::multi_array< ColVariable , 2 >;
 boost::array< typename array_type::index , 2 > shape =
  { instance.num_locations , instance.num_customers };
 auto x = new array_type( shape );

 auto p_x = x->data();
 for( int k = 0 ; k < x->num_elements() ; ++k , ++p_x )
  p_x->is_positive( true );

 block->add_static_variable( * x );

 // Constraints

 {
  auto demand_fulfillment =
   new std::vector< FRowConstraint >( instance.num_customers );
  for( int j = 0 ; j < instance.num_customers ; ++j ) {
   auto function = new LinearFunction();
   for( int i = 0 ; i < instance.num_locations ; ++i ) {
    function->add_variable( & ( * x )[ i ][ j ] , 1 );
   }
   ( * demand_fulfillment )[ j ].set_function( function );
   ( * demand_fulfillment )[ j ].set_both( 1 );
  }
  block->add_static_constraint( * demand_fulfillment );
 }

 auto capacity_constraints =
  new std::vector< FRowConstraint >( instance.num_locations );
 {
  for( int i = 0 ; i < instance.num_locations ; ++i ) {
   auto function = new LinearFunction();
   for( int j = 0 ; j < instance.num_customers ; ++j ) {
    function->add_variable( & ( * x )[ i ][ j ] , instance.demand[ j ] );
   }
   ( * capacity_constraints )[ i ].set_function( function );
   ( * capacity_constraints )[ i ].set_rhs( 0 );
  }
  block->add_static_constraint( * capacity_constraints );
 }

 // Objective function

 {
  auto function = new LinearFunction();
  for( int i = 0 ; i < instance.num_locations ; ++i )
   for( int j = 0 ; j < instance.num_customers ; ++j )
    function->add_variable( & ( * x )[ i ][ j ] , instance.cost[ i ][ j ] );
  auto objective = new FRealObjective( block , function );
  objective->set_sense( Objective::eMin );
  block->set_objective( objective );
 }

 block->register_Solver( solver );

 // BendersBFunction

 auto benders_function = new BendersBFunction();

 benders_function->set_inner_block( block );
 benders_function->set_variables( std::move( y ) );

 {
  for( int i = 0 ; i < instance.num_locations ; ++i ) {
   auto cs = BendersBFunction::ConstraintSpecifier
    ( & ( * capacity_constraints )[ i ] , BendersBFunction::eRHS );
   std::vector< double > Ai( instance.num_locations , 0 );
   Ai[ i ] = instance.capacity[ i ];
   benders_function->add_row( std::move( Ai ) , 0 , cs );
  }
 }

 return benders_function;
}

/*--------------------------------------------------------------------------*/

AbstractBlock * build_Benders_master_block
( const CWLInstance & instance , bool continuous_relaxation ,
  std::vector< ColVariable * > & p_y ) {

 auto block = new AbstractBlock();

 // Variables

 p_y.clear();
 p_y.reserve( instance.num_locations );
 auto y = new std::vector< ColVariable >( instance.num_locations );
 for( auto & y_i : * y ) {
  y_i.is_unitary( true );
  p_y.push_back( & y_i );
  if( ! continuous_relaxation )
   y_i.is_integer( true );
 }

 block->add_static_variable( * y );

 auto z = new ColVariable();
 block->add_static_variable( * z );

 // Constraints

 block->add_dynamic_constraint( * new std::list< FRowConstraint > );

 // Objective function

 {
  auto function = new LinearFunction();
  function->add_variable( z , 1 );
  for( int i = 0 ; i < instance.num_locations ; ++i )
   function->add_variable( & ( * y )[ i ] , instance.fixed_cost[ i ] );
  auto objective = new FRealObjective( block , function );
  objective->set_sense( Objective::eMin );
  block->set_objective( objective );
 }

 return block;
}

/*--------------------------------------------------------------------------*/

AbstractBlock * build_CWL_block_with_Benders_decomposition
( std::string file_name , bool continuous_relaxation = false ,
  Solver * inner_block_solver = nullptr) {

 auto instance = read_cwl_instance( file_name );
 std::vector< ColVariable * > y;
 auto master_block = build_Benders_master_block( instance ,
                                                 continuous_relaxation , y );
 auto benders_function = build_Benders_function( instance , std::move( y ) ,
                                                 inner_block_solver );
 auto & nested_blocks = master_block->access_nested_Blocks();
 assert( nested_blocks.empty() );
 nested_blocks.push_back( new AbstractBlock( master_block ) );
 auto objective = new FRealObjective( nested_blocks[ 0 ] , benders_function );
 nested_blocks[ 0 ]->set_objective( objective );

 return master_block;
}

} }   // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*------------------- End File CWLAbstractBlockBuilder.h -------------------*/
/*--------------------------------------------------------------------------*/
