/*--------------------------------------------------------------------------*/
/*---------------------- File tests_AbstractPath.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the tests for AbstractPath.
 *
 * \version 0.10
 *
 * \date 20 - 11 - 2019
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

#include "AbstractBlockGenerator.h"
#include "AbstractPath.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;
using namespace SMSpp_di_unipi_it::tests;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

void test_paths( Block * block , Block * reference_block ) {

 for( const auto & group : block->get_static_variables() )
  assert( un_any_const_static
          ( group ,
            [ reference_block ]( ColVariable & v ) {
             auto path = AbstractPath::build_path( & v , reference_block );
             assert( & v == AbstractPath::get_element< Variable >
                     ( path , reference_block ) );
            } ,
            un_any_type< ColVariable >() ) );

 for( const auto & group : block->get_static_constraints() )
  assert( un_any_const_static
          ( group ,
            [ reference_block ]( FRowConstraint & v ) {
             {
             auto path = AbstractPath::build_path( & v , reference_block );
             assert( & v == AbstractPath::get_element< Constraint >
                     ( path , reference_block ) );
             }

             {
             auto function = v.get_function();
             auto path = AbstractPath::build_path( function , reference_block );
             assert( function == AbstractPath::get_element< Function >
                     ( path , reference_block ) );
             }
            } ,
            un_any_type< FRowConstraint >() ) );

 for( const auto & group : block->get_dynamic_variables() )
  assert( un_any_const_dynamic
          ( group ,
            [ reference_block ]( ColVariable & v ) {
             auto path = AbstractPath::build_path( & v , reference_block );
             assert( & v == AbstractPath::get_element< Variable >
                     ( path , reference_block ) );
            } ,
            un_any_type< ColVariable >() ) );

 for( const auto & group : block->get_dynamic_constraints() )
  assert( un_any_const_dynamic
          ( group ,
            [ reference_block ]( FRowConstraint & v ) {
             {
             auto path = AbstractPath::build_path( & v , reference_block );
             assert( & v == AbstractPath::get_element< Constraint >
                     ( path , reference_block ) );
             }

             {
             auto function = v.get_function();
             auto path = AbstractPath::build_path( function , reference_block );
             assert( function == AbstractPath::get_element< Function >
                     ( path , reference_block ) );
             }
            } ,
            un_any_type< FRowConstraint >() ) );

 {
  auto objective = block->get_objective();
  auto path = AbstractPath::build_path( objective , reference_block );
  auto e = AbstractPath::get_element< Objective >( path , reference_block );
  assert( objective == e );
 }

 {
  Function * function = nullptr;
  auto objective = static_cast< FRealObjective * >( block->get_objective() );
  if( objective ) {
   function = objective->get_function();
  }
  auto path = AbstractPath::build_path( function , reference_block );
  auto e = AbstractPath::get_element< Function >( path , reference_block );
  assert( function == e );
 }

 {
  auto path = AbstractPath::build_path( block , reference_block );
  const auto retrieved_block = AbstractPath::get_element< Block >
   ( path , reference_block );
  assert( retrieved_block == block );
 }

 for( const auto nested_block : block->get_nested_Blocks() ) {
  auto path = AbstractPath::build_path( nested_block , reference_block );
  const auto retrieved_block = AbstractPath::get_element< Block >
   ( path , reference_block );
  assert( retrieved_block == nested_block );
 }
}

/*--------------------------------------------------------------------------*/

void test( Block * block , Block * reference_block ) {
 test_paths( block , reference_block );
 for( const auto nested_block : block->get_nested_Blocks() )
  test( nested_block , reference_block );
}

/*--------------------------------------------------------------------------*/

void test_everyone_has_function( Block * block ) {

 for( const auto & group : block->get_static_constraints() )
  assert( un_any_const_static
          ( group ,
            []( FRowConstraint & v ) {
             assert( v.get_function() != nullptr );
            } ,
            un_any_type< FRowConstraint >() ) );

 for( const auto & group : block->get_dynamic_constraints() )
  assert( un_any_const_dynamic
          ( group ,
            []( FRowConstraint & v ) {
             assert( v.get_function() != nullptr );
            } ,
            un_any_type< FRowConstraint >() ) );


 {
  Function * function = nullptr;
  auto objective = static_cast< FRealObjective * >( block->get_objective() );
  if( objective ) {
   assert( objective->get_function() != nullptr );
  }
 }

 for( const auto nested_block : block->get_nested_Blocks() )
  test_everyone_has_function( nested_block );
}

/*--------------------------------------------------------------------------*/

void simple_full_test() {

 AbstractBlockRandomNumberGenerator generator;

 using Interval = ElementGenerator<>::Interval;

 generator.static_constraint_generator =
  new ElementGenerator( { 4 , 7 } , { 0 , 2 } , { 4 , 7 } , { 2 , 3 } ,
                        { 3 , 6 } , { 4 , 7 } , 0 );

 generator.static_variable_generator =
  new ElementGenerator( { 4 , 7 } , { 0 , 2 } , { 4 , 7 } , { 2 , 4 } ,
                        { 3 , 6 } , { 4 , 7 } , 2 );

 generator.dynamic_constraint_generator =
  new ElementGenerator( { 4 , 7 } , { 0 , 2 } , { 4 , 7 } , { 2 , 3 } ,
                        { 3 , 6 } , { 4 , 7 } , 3 );

 generator.dynamic_variable_generator =
  new ElementGenerator( { 4 , 7 } , { 0 , 2 } , { 4 , 7 } , { 2 , 4 } ,
                        { 3 , 6 } , { 4 , 7 } , 4 );

 generator.function_generator = new FunctionGenerator( { 0 , 2 } , 5 );

 generator.block_generator = new BlockGenerator( { 4 , 7 } , 6 );

 AbstractBlockGenerator ab_generator( & generator );
 auto block = ab_generator.generate( 2 );

 test_everyone_has_function( block );

 test( block , block );

 delete block;
}

/*--------------------------------------------------------------------------*/

int main( int argc, char ** argv ) {
 simple_full_test();
 return 0;
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_AbstractPath.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
