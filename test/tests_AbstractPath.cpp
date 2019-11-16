/*--------------------------------------------------------------------------*/
/*---------------------- File tests_AbstractPath.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Main for testing AbstractPath
 *
 * \version 0.10
 *
 * \date 16 - 11 - 2019
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

#include <iostream>
#include <random>
#include <utility>

#include "AbstractBlock.h"
#include "AbstractPath.h"
#include "Block.h"
#include "BendersBFunction.h"
#include "Constraint.h"
#include "FRealObjective.h"
#include "FRowConstraint.h"
#include "LagBFunction.h"
#include "OneVarConstraint.h"
#include "RowConstraint.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*-------------------------------- TYPES -----------------------------------*/
/*--------------------------------------------------------------------------*/

using Int = std::size_t;

/*--------------------------------------------------------------------------*/

class IElementGenerator {

public:

 virtual ~IElementGenerator() {}

 virtual Int gen_num_groups() = 0;
 virtual Int gen_group_type() = 0;
 virtual Int gen_vector_size() = 0;
 virtual Int gen_list_size() = 0;
 virtual Int gen_multi_array_num_dim() = 0;
 virtual Int gen_multi_array_extent() = 0;
};

/*--------------------------------------------------------------------------*/

template< class Generator = std::mt19937 , class S = Int >
class ElementGenerator final : public IElementGenerator {

public:

 using Interval = std::pair< Int , Int >;

 ElementGenerator( Interval num_groups , Interval group_type ,
                   Interval vector_size , Interval multi_array_num_dim ,
                   Interval multi_array_extent ,
                   Interval list_size , S seed ) :
  num_groups_dist( num_groups.first , num_groups.second ) ,
  group_type_dist( group_type.first , group_type.second ) ,
  vector_size_dist( vector_size.first , vector_size.second ) ,
  multi_array_num_dim_dist( multi_array_num_dim.first ,
                            multi_array_num_dim.second ) ,
  multi_array_extent_dist( multi_array_extent.first ,
                           multi_array_extent.second ) ,
  list_size_dist( list_size.first , list_size.second ) , generator ( seed ) {}

 ~ElementGenerator() {}

 Int gen_num_groups() override {
   return num_groups_dist( generator );
 }

 Int gen_group_type() override {
   return group_type_dist( generator );
 }

 Int gen_vector_size() override {
   return vector_size_dist( generator );
 }

 Int gen_list_size() override {
   return list_size_dist( generator );
 }

 Int gen_multi_array_num_dim() override {
  return multi_array_num_dim_dist( generator );
 }

 Int gen_multi_array_extent() override {
  return multi_array_extent_dist( generator );
 }

private:

 std::uniform_int_distribution< Int > num_groups_dist;
 std::uniform_int_distribution< Int > group_type_dist;
 std::uniform_int_distribution< Int > vector_size_dist;
 std::uniform_int_distribution< Int > multi_array_num_dim_dist;
 std::uniform_int_distribution< Int > multi_array_extent_dist;
 std::uniform_int_distribution< Int > list_size_dist;
 Generator generator;
};

/*--------------------------------------------------------------------------*/

class IBlockGenerator {
public:
 virtual ~IBlockGenerator() {}
 virtual Int gen_num_nested_blocks() = 0;
};

/*--------------------------------------------------------------------------*/

template< class Generator = std::mt19937 , class S = Int >
class BlockGenerator final : public IBlockGenerator {

public:

 using Interval = std::pair< Int , Int >;

 BlockGenerator( Interval num_nested_blocks , S seed ) :
  num_nested_blocks_dist( num_nested_blocks.first , num_nested_blocks.second ) ,
  generator ( seed ) {}

 ~BlockGenerator() {}

 virtual Int gen_num_nested_blocks() override {
  return num_nested_blocks_dist( generator );
 }

private:

 std::uniform_int_distribution< Int > num_nested_blocks_dist;
 Generator generator;
};

/*--------------------------------------------------------------------------*/

class AbstractBlockRandomNumberGenerator final {

public:

 ~AbstractBlockRandomNumberGenerator() {
  delete static_constraint_generator;
  delete static_variable_generator;
  delete dynamic_constraint_generator;
  delete dynamic_variable_generator;
  delete block_generator;
 }

 IElementGenerator * static_constraint_generator;
 IElementGenerator * static_variable_generator;
 IElementGenerator * dynamic_constraint_generator;
 IElementGenerator * dynamic_variable_generator;
 IBlockGenerator * block_generator;
};


/*--------------------------------------------------------------------------*/

class AbstractBlockGenerator final {

public:

 enum GroupType { eSingle , eVector , eMultiArray };

 AbstractBlockGenerator( AbstractBlockRandomNumberGenerator * generator ) {
  this->generator = generator;
 }

 AbstractBlock * generate( int depth ) {
  auto block = new AbstractBlock();

  block->set_objective( new FRealObjective() );

  generate_elements( block );

  if( depth > 0 )
   generate_nested_blocks( block , depth - 1 );
  return block;
 }

 void generate_elements( AbstractBlock * block ) {
  generate_elements< ColVariable , false >
   ( [ block ]( auto * e ) { block->add_static_variable( * e ); } ,
     generator->static_variable_generator );


  generate_elements< std::list< ColVariable > , true >
   ( [ block ]( auto * e ) { block->add_dynamic_variable( * e ); } ,
     generator->dynamic_variable_generator );

  generate_elements< FRowConstraint , false >
   ( [ block ]( auto * e ) { block->add_static_constraint( * e ); } ,
     generator->static_constraint_generator  );

  generate_elements< std::list< FRowConstraint > , true >
   ( [ block ]( auto * e ) { block->add_dynamic_constraint( * e ); } ,
     generator->dynamic_constraint_generator  );
 }

 template< class T , bool dynamic , class F >
 void generate_elements( F add , IElementGenerator * generator ) {

  int num_groups = generator->gen_num_groups();

  for( int i = 0 ; i < num_groups ; ++i ) {

   auto group_type = generator->gen_group_type() % 3;

   switch( group_type ) {

    case( eSingle ): {

     auto t = new T();

     if constexpr( dynamic ) {
      auto num_elements = generator->gen_list_size();
      t->resize( num_elements );
     }

     add( t );
     break;
    }

    case( eVector ): {

     auto size = generator->gen_vector_size();
     auto vector = new std::vector< T >( size );

     if constexpr( dynamic ) {
       for( auto & e : * vector ) {
        auto num_elements = generator->gen_list_size();
        e.resize( num_elements );
       }
     }

     add( vector );
     break;
    }

    case( eMultiArray ): {
     auto k = generator->gen_multi_array_num_dim();
     switch( k ) {
      case(1): add( create_multi_array< T , dynamic , 1 >( generator ) ); break;
      case(2): add( create_multi_array< T , dynamic , 2 >( generator ) ); break;
      case(3): add( create_multi_array< T , dynamic , 3 >( generator ) ); break;
      case(4): add( create_multi_array< T , dynamic , 4 >( generator ) ); break;
      case(5): add( create_multi_array< T , dynamic , 5 >( generator ) ); break;
      case(6): add( create_multi_array< T , dynamic , 6 >( generator ) ); break;
      case(7): add( create_multi_array< T , dynamic , 7 >( generator ) ); break;
      default: add( create_multi_array< T , dynamic , 8 >( generator ) ); break;
     }
    }
   }
  }
 }

 template< class T , bool dynamic , std::size_t K >
 static boost::multi_array< T , K > * create_multi_array
 ( IElementGenerator * generator ) {
  using array_type = typename boost::multi_array< T , K >;
  boost::array< typename array_type::index , K > shape;
  for( std::size_t i = 0 ; i < K ; ++i )
   shape[ i ] = generator->gen_multi_array_extent();
  auto multi_array = new array_type( shape );

  if constexpr( dynamic ) {
   const auto p = multi_array->data();
   for( boost::multi_array_types::size_type i = 0 ;
        i < multi_array->num_elements() ; ++i ) {
    auto num_elements = generator->gen_list_size();
    p[ i ].resize( num_elements );
   }
  }
  return multi_array;
 }

 void generate_nested_blocks( AbstractBlock * block , int depth ) {
  auto & v_Block = block->access_nested_Blocks();
  auto num_nested_blocks = generator->block_generator->gen_num_nested_blocks();
  v_Block.reserve( num_nested_blocks );
  for( int i = 0 ; i < num_nested_blocks ; ++i ) {
   auto nested_block = generate( depth );
   nested_block->set_f_Block( block );
   v_Block.push_back( nested_block );
  }
 }

private:

 AbstractBlockRandomNumberGenerator * generator;

};

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

void test_paths( Block * block , Block * reference_block ) {

 for( const auto & group : block->get_static_variables() )
  assert( un_any_const_static
          ( group ,
            [ reference_block ]( ColVariable & v ) {
             auto path = AbstractPath::build_path( & v , reference_block );
             auto e = AbstractPath::get_element< Variable >( reference_block ,
                                                             path );
             assert( e == & v);
            } ,
            un_any_type< ColVariable >() ) );

 for( const auto & group : block->get_static_constraints() )
  assert( un_any_const_static
          ( group ,
            [ reference_block ]( FRowConstraint & v ) {
             auto path = AbstractPath::build_path( & v , reference_block );
             auto e = AbstractPath::get_element< Constraint >( reference_block ,
                                                               path );
             assert( e == & v);
            } ,
            un_any_type< FRowConstraint >() ) );

 for( const auto & group : block->get_dynamic_variables() )
  assert( un_any_const_dynamic
          ( group ,
            [ reference_block ]( ColVariable & v ) {
             auto path = AbstractPath::build_path( & v , reference_block );
             auto e = AbstractPath::get_element< Variable >( reference_block ,
                                                             path );
             assert( e == & v);
            } ,
            un_any_type< ColVariable >() ) );

 for( const auto & group : block->get_dynamic_constraints() )
  assert( un_any_const_dynamic
          ( group ,
            [ reference_block ]( FRowConstraint & v ) {
             auto path = AbstractPath::build_path( & v , reference_block );
             auto e = AbstractPath::get_element< Constraint >( reference_block ,
                                                               path );
             assert( e == & v);
            } ,
            un_any_type< FRowConstraint >() ) );


 {
  auto objective = block->get_objective();
  auto path = AbstractPath::build_path( objective , reference_block );
  auto e = AbstractPath::get_element< Objective >( reference_block , path );
  assert( objective == e );
 }
 
 for( const auto nested_block : block->get_nested_Blocks() ) {
  auto path = AbstractPath::build_path( nested_block , reference_block );
  const auto retrieved_block = AbstractPath::get_element< Block >
   ( reference_block , path );
  assert( retrieved_block == nested_block );
 }

 // TODO Test with Functions. In particular, with BendersBFunction and
 // LagBFunction.
}

/*--------------------------------------------------------------------------*/

void test( Block * block , Block * reference_block ) {
 test_paths( block , reference_block );
 for( const auto nested_block : block->get_nested_Blocks() )
  test( nested_block , reference_block );
}

/*--------------------------------------------------------------------------*/

int main( int argc, char ** argv ) {

 AbstractBlockRandomNumberGenerator generator;

 using Interval = ElementGenerator<>::Interval;

 generator.static_constraint_generator =
  new ElementGenerator( { 5 , 10 } , { 0 , 2 } , { 5 , 10 } , { 2 , 4 } ,
                        { 3 , 6 } , { 5 , 10 } , 0 );

 generator.static_variable_generator =
  new ElementGenerator( { 5 , 10 } , { 0 , 2 } , { 5 , 10 } , { 2 , 4 } ,
                        { 3 , 6 } , { 5 , 10 } , 2 );

 generator.dynamic_constraint_generator =
  new ElementGenerator( { 5 , 10 } , { 0 , 2 } , { 5 , 10 } , { 2 , 4 } ,
                        { 3 , 6 } , { 5 , 10 } , 3 );

 generator.dynamic_variable_generator =
  new ElementGenerator( { 5 , 10 } , { 0 , 2 } , { 5 , 10 } , { 2 , 4 } ,
                        { 3 , 6 } , { 5 , 10 } , 4 );

 generator.block_generator = new BlockGenerator( { 4 , 7 } , 5 );

 AbstractBlockGenerator ab_generator( & generator );
 auto block = ab_generator.generate( 3 );

 test( block , block );

 delete block;

 return 0;
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_AbstractPath.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
