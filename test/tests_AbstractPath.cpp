/*--------------------------------------------------------------------------*/
/*---------------------- File tests_AbstractPath.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the tests for AbstractPath.
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Rafael Durbano Lobato
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

void test_serialization( const AbstractPath & path ) {
 netCDF::NcFile ncFile( "ncfile_path_test.txt" , netCDF::NcFile::replace );
 auto group = ncFile.addGroup("Path");
 path.serialize( group );
 AbstractPath deserialized_path( group );
 assert( path == deserialized_path );
}

/*--------------------------------------------------------------------------*/

void test_paths( Block * block , Block * reference_block ) {

 for( const auto & group : block->get_static_variables() )
  assert( un_any_const_static
          ( group ,
            [ reference_block ]( ColVariable & v ) {
             AbstractPath path( & v , reference_block );
             assert( & v == path.get_element< Variable >( reference_block ) );
             test_serialization( path );
            } ,
            un_any_type< ColVariable >() ) );

 for( const auto & group : block->get_static_constraints() )
  assert( un_any_const_static
          ( group ,
            [ reference_block ]( FRowConstraint & v ) {
             {
             AbstractPath path( & v , reference_block );
             assert( & v == path.get_element< Constraint >( reference_block ) );
             test_serialization( path );
             }

             {
             auto function = v.get_function();
             AbstractPath path( function , reference_block );
             assert( function == path.get_element< Function >
                     ( reference_block ) );
             test_serialization( path );
             }
            } ,
            un_any_type< FRowConstraint >() ) );

 for( const auto & group : block->get_dynamic_variables() )
  assert( un_any_const_dynamic
          ( group ,
            [ reference_block ]( ColVariable & v ) {
             AbstractPath path( & v , reference_block );
             assert( & v == path.get_element< Variable >( reference_block ) );
             test_serialization( path );
            } ,
            un_any_type< ColVariable >() ) );

 for( const auto & group : block->get_dynamic_constraints() )
  assert( un_any_const_dynamic
          ( group ,
            [ reference_block ]( FRowConstraint & v ) {
             {
             AbstractPath path( & v , reference_block );
             assert( & v == path.get_element< Constraint >( reference_block ) );
             test_serialization( path );
             }

             {
             auto function = v.get_function();
             AbstractPath path( function , reference_block );
             assert( function == path.get_element< Function >
                     ( reference_block ) );
             test_serialization( path );
             }
            } ,
            un_any_type< FRowConstraint >() ) );

 {
  auto objective = block->get_objective();
  AbstractPath path( objective , reference_block );
  auto e = path.get_element< Objective >( reference_block );
  assert( objective == e );
  test_serialization( path );
 }

 {
  Function * function = nullptr;
  auto objective = static_cast< FRealObjective * >( block->get_objective() );
  if( objective ) {
   function = objective->get_function();
  }
  AbstractPath path( function , reference_block );
  auto e = path.get_element< Function >( reference_block );
  assert( function == e );
  test_serialization( path );
 }

 {
  AbstractPath path( block , reference_block );
  const auto retrieved_block = path.get_element< Block >( reference_block );
  assert( retrieved_block == block );
  test_serialization( path );
 }

 for( const auto nested_block : block->get_nested_Blocks() ) {
  AbstractPath path( nested_block , reference_block );
  const auto retrieved_block = path.get_element< Block >( reference_block );
  assert( retrieved_block == nested_block );
  test_serialization( path );
 }

 if( const auto pfb = dynamic_cast< PolyhedralFunctionBlock * >( block ) ) {
  const auto & function = pfb->get_PolyhedralFunction();
  AbstractPath path( & function , reference_block );
  const auto retrieved_function =
   path.get_element< Function >( reference_block );
  assert( retrieved_function == & function );
  test_serialization( path );
 }
}

/*--------------------------------------------------------------------------*/

std::set< std::pair< Block * , Block * > > visited_blocks;

bool visited( Block * block , Block * reference_block ) {
 if( visited_blocks.find( std::make_pair( block , reference_block ) ) !=
     visited_blocks.end() )
  return( true );
 visited_blocks.insert( std::make_pair( block , reference_block ) );
 return( false );
}

/*--------------------------------------------------------------------------*/

void test( Block * block , Block * reference_block ) {
 if( visited( block , reference_block ) )
  return;

 test_paths( block , reference_block );

 if( const auto objective =
     dynamic_cast< FRealObjective * >( block->get_objective() ) ) {
  if( const auto function =
      dynamic_cast< BendersBFunction * >( objective->get_function() ) ) {
   if( const auto inner_block = function->get_inner_block() ) {
    test( inner_block , reference_block );
    test( inner_block , inner_block );
    if( reference_block != block ) {
     test( inner_block , block );
    }
   }
  }
  else if( const auto function =
           dynamic_cast< LagBFunction * >( objective->get_function() ) ) {
   if( const auto inner_block = function->get_inner_block() ) {
    test( inner_block , reference_block );
    test( inner_block , inner_block );
    if( reference_block != block ) {
     test( inner_block , block );
    }
   }
  }
 }

 for( const auto nested_block : block->get_nested_Blocks() ) {
  test( nested_block , reference_block );
  if( block != reference_block )
   test( nested_block , block );
  test( nested_block , nested_block );
 }
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
  auto objective = static_cast< FRealObjective * >( block->get_objective() );
  if( objective ) {
   assert( objective->get_function() != nullptr );
  }
 }

 for( const auto nested_block : block->get_nested_Blocks() )
  test_everyone_has_function( nested_block );
}

/*--------------------------------------------------------------------------*/

void print_tree( Block * block , std::string spaces = "" ) {
 std::cout << block << std::endl;

 if( const auto objective =
     dynamic_cast< FRealObjective * >( block->get_objective() ) ) {
  if( const auto function =
      dynamic_cast< BendersBFunction * >( objective->get_function() ) ) {
   if( const auto inner_block = function->get_inner_block() ) {
    std::cout << spaces << "-> BendersBFunction " << function << std::endl;
    std::cout << spaces + "   " << "-> ";
    print_tree( inner_block , spaces + "   "  + "   " );
   }
  }
  else if( const auto function =
           dynamic_cast< LagBFunction * >( objective->get_function() ) )
   if( const auto inner_block = function->get_inner_block() ) {
    std::cout << spaces << "-> LagBFunction " << function << std::endl;
    std::cout << spaces + "   " << "-> ";
    print_tree( inner_block , spaces + "   "  + "   " );
   }
 }

 auto & nested_blocks = block->get_nested_Blocks();
 if( ! nested_blocks.empty() ) {
  for( auto son : nested_blocks ) {
   std::cout << spaces << "-> ";
   print_tree( son , spaces + "   " );
  }
 }
}

/*--------------------------------------------------------------------------*/

void simple_full_test() {

 AbstractBlockRandomNumberGenerator generator;

 generator.static_constraint_generator =
  new ElementGenerator< std::mt19937 , Int >
  ( { 4 , 7 } , { 0 , 2 } , { 4 , 7 } , { 2 , 3 } , { 3 , 6 } , { 4 , 7 } , 0 );

 generator.static_variable_generator =
  new ElementGenerator< std::mt19937 , Int >
  ( { 4 , 7 } , { 0 , 2 } , { 4 , 7 } , { 2 , 4 } , { 3 , 6 } , { 4 , 7 } , 2 );

 generator.dynamic_constraint_generator =
  new ElementGenerator< std::mt19937 , Int >
  ( { 4 , 7 } , { 0 , 2 } , { 4 , 7 } , { 2 , 3 } , { 3 , 6 } , { 4 , 7 } , 3 );

 generator.dynamic_variable_generator =
  new ElementGenerator< std::mt19937 , Int >
  ( { 4 , 7 } , { 0 , 2 } , { 4 , 7 } , { 2 , 4 } , { 3 , 6 } , { 4 , 7 } , 4 );

 generator.function_generator =
  new FunctionGenerator< std::mt19937 , Int >( { 0 , 2 } , 5 );

 generator.num_nested_block_generator =
  new NumNestedBlockGenerator< std::mt19937 , Int >( { 4 , 7 } , 6 );

 AbstractBlockGenerator ab_generator( & generator );
 auto block = ab_generator.generate( 2 );

 test_everyone_has_function( block );

 test( block , block );

 delete block;
}

/*--------------------------------------------------------------------------*/

int main() {
 simple_full_test();
 return( 0 );
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_AbstractPath.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
