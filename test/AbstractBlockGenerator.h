/*--------------------------------------------------------------------------*/
/*---------------------- File AbstractBlockGenerator -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of AbstractBlockGenerator, a class for generating random
 * AbstractBlock.
 *
 * \version 0.10
 *
 * \date 19 - 11 - 2019
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

#include <random>
#include <utility>

#include "AbstractBlock.h"
#include "Block.h"
#include "BendersBFunction.h"
#include "FRealObjective.h"
#include "FRowConstraint.h"
#include "LagBFunction.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

// namespace for the tests of the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it { namespace tests {

/*--------------------------------------------------------------------------*/
/*-------------------------------- TYPES -----------------------------------*/
/*--------------------------------------------------------------------------*/

using Int = std::size_t;

/*--------------------------------------------------------------------------*/
/*----------------------------- TYPES TRAITS -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
struct has_function :
 std::bool_constant< std::is_base_of_v< FRealObjective , T > ||
                     std::is_base_of_v< FRowConstraint , T > >{};

template< class T >
inline constexpr bool has_function_v = has_function< T >::value;

/*--------------------------------------------------------------------------*/
/*-------------------------------- CLASSES ---------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS IElementGenerator -------------------------*/
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
/*------------------------ CLASS ElementGenerator --------------------------*/
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
/*----------------------- CLASS IFunctionGenerator -------------------------*/
/*--------------------------------------------------------------------------*/

class IFunctionGenerator {
public:
 virtual ~IFunctionGenerator() {}
 virtual Int gen_function_type() = 0;
};

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS FunctionGenerator --------------------------*/
/*--------------------------------------------------------------------------*/

template< class Generator = std::mt19937 , class S = Int >
class FunctionGenerator final : public IFunctionGenerator {

public:

 using Interval = std::pair< Int , Int >;

 FunctionGenerator( Interval function_type , S seed ) :
  function_type_dist( function_type.first , function_type.second ) ,
  generator ( seed ) {}

 ~FunctionGenerator() {}

 virtual Int gen_function_type() override {
  return function_type_dist( generator );
 }

private:

 std::uniform_int_distribution< Int > function_type_dist;
 Generator generator;
};

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS IBlockGenerator ---------------------------*/
/*--------------------------------------------------------------------------*/

class IBlockGenerator {
public:
 virtual ~IBlockGenerator() {}
 virtual Int gen_num_nested_blocks() = 0;
};

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS BlockGenerator ----------------------------*/
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
/*-------------- CLASS AbstractBlockRandomNumberGenerator ------------------*/
/*--------------------------------------------------------------------------*/

class AbstractBlockRandomNumberGenerator final {

public:

 AbstractBlockRandomNumberGenerator
 ( IElementGenerator * static_constraint_generator = nullptr ,
   IElementGenerator * static_variable_generator = nullptr ,
   IElementGenerator * dynamic_constraint_generator = nullptr ,
   IElementGenerator * dynamic_variable_generator = nullptr ,
   IFunctionGenerator * function_generator = nullptr ,
   IBlockGenerator * block_generator = nullptr ) {
  this->static_constraint_generator = static_constraint_generator;
  this->static_variable_generator = static_variable_generator;
  this->dynamic_constraint_generator = dynamic_constraint_generator;
  this->dynamic_variable_generator = dynamic_variable_generator;
  this->function_generator = function_generator;
  this->block_generator = block_generator;
 }

 ~AbstractBlockRandomNumberGenerator() {
  delete static_constraint_generator;
  delete static_variable_generator;
  delete dynamic_constraint_generator;
  delete dynamic_variable_generator;
  delete function_generator;
  delete block_generator;
 }

 AbstractBlockRandomNumberGenerator
 ( const AbstractBlockRandomNumberGenerator & ) = delete;

 AbstractBlockRandomNumberGenerator & operator=
 ( const AbstractBlockRandomNumberGenerator & ) = delete;

 AbstractBlockRandomNumberGenerator
 ( AbstractBlockRandomNumberGenerator && ) = delete;

 AbstractBlockRandomNumberGenerator & operator=
 ( AbstractBlockRandomNumberGenerator && ) = delete;

 IElementGenerator * static_constraint_generator;
 IElementGenerator * static_variable_generator;
 IElementGenerator * dynamic_constraint_generator;
 IElementGenerator * dynamic_variable_generator;
 IFunctionGenerator * function_generator;
 IBlockGenerator * block_generator;
};

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS AbstractBlockGenerator ------------------------*/
/*--------------------------------------------------------------------------*/

class AbstractBlockGenerator final {

public:

 enum GroupType { eSingleton , eVector , eMultiArray };
 enum FunctionType { eLinear , eBenders , eLag };

/*--------------------------------------------------------------------------*/

 AbstractBlockGenerator( AbstractBlockRandomNumberGenerator * generator ) {
  this->generator = generator;
 }

/*--------------------------------------------------------------------------*/

 AbstractBlock * generate( int depth ) {
  auto block = new AbstractBlock();
  generate_objective( block );
  generate_groups( block );
  if( depth > 0 )
   generate_nested_blocks( block , depth - 1 );
  return block;
 }

/*--------------------------------------------------------------------------*/

private:

 template< class T >
 typename std::enable_if_t< ! has_function_v< T > >
 generate_function( T * ) {}

 template< class T >
 typename std::enable_if_t< has_function_v< T > >
 generate_function( T * t ) {
  if( ! generator || ! generator->function_generator ) return;
  Function * function = nullptr;
  switch( generator->function_generator->gen_function_type() ) {
   case( eLinear ):
    function = new LinearFunction();
    break;
   case( eBenders ):
    function = new BendersBFunction( nullptr );
    break;
   case( eLag ):
    function = new LagBFunction();
    break;
  }
  t->set_function( function );
 }

/*--------------------------------------------------------------------------*/

 template < template < class , class > class C ,
            class T , class A = std::allocator< T > >
 typename std::enable_if_t< ! has_function_v< T > >
 generate_functions( const C< T , A > & ) {}

 template <template < class , class > class C ,
           class T , class A = std::allocator< T > >
 typename std::enable_if_t< has_function_v< T > >
 generate_functions( const C< T , A > & c ) {
  for( auto & e : c )
   generate_function( const_cast< T * >( & e ) );
 }

/*--------------------------------------------------------------------------*/

 void generate_objective( AbstractBlock * block ) {
  auto objective = new FRealObjective();
  generate_function( objective );
  block->set_objective( objective );
 }

/*--------------------------------------------------------------------------*/

 void generate_groups( AbstractBlock * block ) {
  generate_groups< ColVariable , false >
   ( [ block ]( auto * e ) { block->add_static_variable( * e ); } ,
     generator->static_variable_generator );

  generate_groups< std::list< ColVariable > , true >
   ( [ block ]( auto * e ) { block->add_dynamic_variable( * e ); } ,
     generator->dynamic_variable_generator );

  generate_groups< FRowConstraint , false >
   ( [ block ]( auto * e ) { block->add_static_constraint( * e ); } ,
     generator->static_constraint_generator  );

  generate_groups< std::list< FRowConstraint > , true >
   ( [ block ]( auto * e ) { block->add_dynamic_constraint( * e ); } ,
     generator->dynamic_constraint_generator  );
 }

/*--------------------------------------------------------------------------*/

 template< class T , bool dynamic , class F >
 void generate_groups( const F & add , IElementGenerator * generator ) {
  if( ! generator ) return;
  auto num_groups = generator->gen_num_groups();
  for( int i = 0 ; i < num_groups ; ++i ) {
   auto group_type = GroupType( generator->gen_group_type() % 3 );
   generate_group< T , dynamic , F >( group_type , add , generator );
  }
 }

/*--------------------------------------------------------------------------*/

 template< class T , bool dynamic , class F >
 void generate_group( const GroupType group_type , const F & add ,
                      IElementGenerator * generator ) {
  if( ! generator ) return;
  switch( group_type ) {
   case( eSingleton ):
    generate_singleton_group< T , dynamic , F >( add , generator );
    break;

   case( eVector ):
    generate_vector_group< T , dynamic , F >( add , generator );
    break;

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

/*--------------------------------------------------------------------------*/

 template< class T , bool dynamic , class F >
 void generate_singleton_group( const F & add ,
                                IElementGenerator * generator ) {
  if( ! generator ) return;
  auto t = new T();
  if constexpr( dynamic ) {
   auto num_elements = generator->gen_list_size();
   t->resize( num_elements );
   generate_functions( * t );
  }
  else
   generate_function( t );
  add( t );
 }

/*--------------------------------------------------------------------------*/

 template< class T , bool dynamic , class F >
 void generate_vector_group( const F & add , IElementGenerator * generator ) {
  if( ! generator ) return;
  auto size = generator->gen_vector_size();
  auto vector = new std::vector< T >( size );
  if constexpr( dynamic ) {
   for( auto & e : * vector ) {
    auto num_elements = generator->gen_list_size();
    e.resize( num_elements );
    generate_functions( e );
   }
  }
  else
   generate_functions( * vector );
  add( vector );
 }

/*--------------------------------------------------------------------------*/

 template< class T , bool dynamic , std::size_t K >
 boost::multi_array< T , K > * create_multi_array
 ( IElementGenerator * generator ) {
  using array_type = typename boost::multi_array< T , K >;
  boost::array< typename array_type::index , K > shape;

  if( ! generator ) {
   for( std::size_t i = 0 ; i < K ; ++i )
    shape[ i ] = 0;
   return new array_type( shape );
  }

  for( std::size_t i = 0 ; i < K ; ++i )
   shape[ i ] = generator->gen_multi_array_extent();
  auto multi_array = new array_type( shape );

  if constexpr( dynamic ) {
   const auto p = multi_array->data();
   for( boost::multi_array_types::size_type i = 0 ;
        i < multi_array->num_elements() ; ++i ) {
    auto num_elements = generator->gen_list_size();
    p[ i ].resize( num_elements );
    generate_functions( p[ i ] );
   }
  }
  else if constexpr( has_function_v< T > ) {
   const auto p = multi_array->data();
   for( boost::multi_array_types::size_type i = 0 ;
        i < multi_array->num_elements() ; ++i ) {
    generate_function( & p[ i ] );
   }
  }
  return multi_array;
 }

/*--------------------------------------------------------------------------*/

 void generate_nested_blocks( AbstractBlock * block , int depth ) {
  if( ! generator || ! generator->block_generator ) return;
  auto & v_Block = block->access_nested_Blocks();
  auto num_nested_blocks = generator->block_generator->gen_num_nested_blocks();
  v_Block.reserve( num_nested_blocks );
  for( int i = 0 ; i < num_nested_blocks ; ++i ) {
   auto nested_block = generate( depth );
   nested_block->set_f_Block( block );
   v_Block.push_back( nested_block );
  }
 }

/*--------------------------------------------------------------------------*/

 AbstractBlockRandomNumberGenerator * generator;

};

} }   // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*------------------- End File AbstractBlockGenerator.h --------------------*/
/*--------------------------------------------------------------------------*/
