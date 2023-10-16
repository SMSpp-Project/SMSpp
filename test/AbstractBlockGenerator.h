/*--------------------------------------------------------------------------*/
/*---------------------- File AbstractBlockGenerator -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of AbstractBlockGenerator, a class for generating random
 * AbstractBlock.
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
   return( num_groups_dist( generator ) );
 }

 Int gen_group_type() override {
   return( group_type_dist( generator ) );
 }

 Int gen_vector_size() override {
   return( vector_size_dist( generator ) );
 }

 Int gen_list_size() override {
   return( list_size_dist( generator ) );
 }

 Int gen_multi_array_num_dim() override {
  return( multi_array_num_dim_dist( generator ) );
 }

 Int gen_multi_array_extent() override {
  return( multi_array_extent_dist( generator ) );
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
  return( function_type_dist( generator ) );
 }

private:

 std::uniform_int_distribution< Int > function_type_dist;
 Generator generator;
};

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS IBlockGenerator ---------------------------*/
/*--------------------------------------------------------------------------*/

class INumNestedBlockGenerator {
public:
 virtual ~INumNestedBlockGenerator() {}
 virtual Int gen_num_nested_blocks() = 0;
};

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS NumNestedBlockGenerator -----------------------*/
/*--------------------------------------------------------------------------*/

template< class Generator = std::mt19937 , class S = Int >
class NumNestedBlockGenerator final : public INumNestedBlockGenerator {

public:

 using Interval = std::pair< Int , Int >;

 NumNestedBlockGenerator( Interval num_nested_blocks , S seed ) :
  num_nested_blocks_dist( num_nested_blocks.first , num_nested_blocks.second ) ,
  generator ( seed ) {}

 ~NumNestedBlockGenerator() {}

 virtual Int gen_num_nested_blocks() override {
  return( num_nested_blocks_dist( generator ) );
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
   INumNestedBlockGenerator * num_nested_block_generator = nullptr ) {
  this->static_constraint_generator = static_constraint_generator;
  this->static_variable_generator = static_variable_generator;
  this->dynamic_constraint_generator = dynamic_constraint_generator;
  this->dynamic_variable_generator = dynamic_variable_generator;
  this->function_generator = function_generator;
  this->num_nested_block_generator = num_nested_block_generator;
 }

 ~AbstractBlockRandomNumberGenerator() {
  delete static_constraint_generator;
  delete static_variable_generator;
  delete dynamic_constraint_generator;
  delete dynamic_variable_generator;
  delete function_generator;
  delete num_nested_block_generator;
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
 INumNestedBlockGenerator * num_nested_block_generator;
};

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS AbstractBlockGenerator ------------------------*/
/*--------------------------------------------------------------------------*/

class AbstractBlockGenerator final {

public:

 enum GroupType { eSingleton = 0 , eVector = 1 , eMultiArray = 2 };
 enum FunctionType { eLinear = 0 , eBenders = 1 , eLag = 2 };

/*--------------------------------------------------------------------------*/

 AbstractBlockGenerator( AbstractBlockRandomNumberGenerator * generator ) {
  this->generator = generator;
 }

/*--------------------------------------------------------------------------*/

 AbstractBlock * generate( int depth ) {
  auto block = new AbstractBlock();
  generate( block , depth );
  return( block );
 }

/*--------------------------------------------------------------------------*/

 void * generate( AbstractBlock * block , int depth ) {
  generate_objective( block , std::max( 0 , depth - 1 ) );
  generate_groups( block , std::max( 0 , depth - 1 ) );
  if( depth > 0 )
   generate_nested_blocks( block , depth - 1 );
  return( block );
 }

/*--------------------------------------------------------------------------*/

private:

 template< class T >
 typename std::enable_if_t< ! has_function_v< T > >
 generate_function( T * , int ) {}

 template< class T >
 typename std::enable_if_t< has_function_v< T > >
 generate_function( T * t , int depth ) {
  if( ( ! generator ) || ( ! generator->function_generator ) ) return;
  Function * function = nullptr;
  auto function_type = generator->function_generator->gen_function_type();
  if constexpr( ! std::is_base_of_v< FRealObjective , T > ) {
   if( function_type == eLag || function_type == eBenders ) {
    // The Observer of a LagBFunction must be an FRealObjective. So, we try to
    // use a different function for the given T element.
    for( int i = 0 ; i < 100 && (function_type == eLag ||
                                 function_type == eBenders) ; ++i )
     function_type = generator->function_generator->gen_function_type();
    if( function_type == eLag || function_type == eBenders )
     function_type = eLinear; // Use a LinearFunction
   }
  }
  switch( function_type ) {
   case( eLinear ):
    function = new LinearFunction();
    break;
   case( eBenders ): {
    if( depth > 0 ) {
     function = new BendersBFunction();
     auto block = new AbstractBlock();
     static_cast< BendersBFunction * >( function )->set_inner_block( block );
     generate( block , depth );
    }
    else {
     function = new LinearFunction();
    }

    break;
   }
   case( eLag ): {
    if( depth > 0 ) {
     function = new LagBFunction( nullptr , t );
     auto block = new AbstractBlock();
     block->set_objective( new FRealObjective( block , new LinearFunction() ) );
     static_cast< LagBFunction * >( function )->set_inner_block( block );
     block->set_f_Block( static_cast< LagBFunction * >( function ) );
     generate( block , depth );
    }
    else {
     function = new LinearFunction();
    }
    break;
   }
  }
  t->set_function( function );
 }

/*--------------------------------------------------------------------------*/

 template< template< class , class > class C ,
  class T , class A = std::allocator< T > >
 typename std::enable_if_t< !has_function_v< T > >
 generate_functions( const C< T , A > & , int ) {}

 template< template< class , class > class C ,
  class T , class A = std::allocator< T > >
 typename std::enable_if_t< has_function_v< T > >
 generate_functions( const C< T , A > & c , int depth ) {
  for( auto & e : c )
   generate_function( const_cast< T * >( & e ) , depth );
 }

/*--------------------------------------------------------------------------*/

 void generate_objective( AbstractBlock * block , int depth ) {
  if( ! block->get_objective() ) {
   auto objective = new FRealObjective( block );
   generate_function( objective , depth );
   block->set_objective( objective );
  }
 }

/*--------------------------------------------------------------------------*/

 void generate_groups( AbstractBlock * block , int depth ) {
  generate_groups< ColVariable , false >
   ( [ block ]( auto * e ) { block->add_static_variable( * e ); } ,
     generator->static_variable_generator , depth );

  generate_groups< std::list< ColVariable > , true >
   ( [ block ]( auto * e ) { block->add_dynamic_variable( * e ); } ,
     generator->dynamic_variable_generator , depth );

  generate_groups< FRowConstraint , false >
   ( [ block ]( auto * e ) { block->add_static_constraint( * e ); } ,
     generator->static_constraint_generator , depth );

  generate_groups< std::list< FRowConstraint > , true >
   ( [ block ]( auto * e ) { block->add_dynamic_constraint( * e ); } ,
     generator->dynamic_constraint_generator , depth );
 }

/*--------------------------------------------------------------------------*/

 template< class T , bool dynamic , class F >
 void generate_groups( const F & add , IElementGenerator * generator ,
                       int depth ) {
  if( ! generator ) return;
  auto num_groups = generator->gen_num_groups();
  for( decltype( num_groups ) i = 0 ; i < num_groups ; ++i ) {
   auto group_type = GroupType( generator->gen_group_type() );
   generate_group< T , dynamic , F >( group_type , add , generator , depth );
  }
 }

/*--------------------------------------------------------------------------*/

 template< class T , bool dynamic , class F >
 void generate_group( const GroupType group_type , const F & add ,
                      IElementGenerator * generator , int depth ) {
  if( ! generator ) return;
  switch( group_type ) {
   case( eSingleton ):
    generate_singleton_group< T , dynamic , F >( add , generator , depth );
    break;

   case( eVector ):
    generate_vector_group< T , dynamic , F >( add , generator , depth );
    break;

   case( eMultiArray ): {
    auto k = generator->gen_multi_array_num_dim();
    switch( k ) {
     case(1):
      generate_multi_array_group< T , dynamic , 1 >( generator , add , depth );
      break;
     case(2):
      generate_multi_array_group< T , dynamic , 2 >( generator , add , depth );
      break;
     case(3):
      generate_multi_array_group< T , dynamic , 3 >( generator , add , depth );
      break;
     case(4):
      generate_multi_array_group< T , dynamic , 4 >( generator , add , depth );
      break;
     case(5):
      generate_multi_array_group< T , dynamic , 5 >( generator , add , depth );
      break;
     case(6):
      generate_multi_array_group< T , dynamic , 6 >( generator , add , depth );
      break;
     case(7):
      generate_multi_array_group< T , dynamic , 7 >( generator , add , depth );
      break;
     default:
      generate_multi_array_group< T , dynamic , 8 >( generator , add , depth );
      break;
    }
   }
  }
 }

/*--------------------------------------------------------------------------*/

 template< class T , bool dynamic , class F >
 void generate_singleton_group( const F & add ,
                                IElementGenerator * generator , int depth ) {
  if( ! generator ) return;
  auto t = new T();
  if constexpr( dynamic ) {
   auto num_elements = generator->gen_list_size();
   t->resize( num_elements );
   add( t );
   generate_functions( * t , depth );
  }
  else {
   add( t );
   generate_function( t , depth );
  }
 }

/*--------------------------------------------------------------------------*/

 template< class T , bool dynamic , class F >
 void generate_vector_group( const F & add , IElementGenerator * generator ,
                             int depth ) {
  if( ! generator ) return;
  auto size = generator->gen_vector_size();
  auto vector = new std::vector< T >( size );
  if constexpr( dynamic ) {
   for( auto & e : * vector ) {
    auto num_elements = generator->gen_list_size();
    e.resize( num_elements );
   }
   add( vector );
   for( auto & e : * vector ) {
    generate_functions( e , depth );
   }
  }
  else {
   add( vector );
   generate_functions( * vector , depth );
  }
 }

/*--------------------------------------------------------------------------*/

 template< class T , bool dynamic , std::size_t K , class F >
 void generate_multi_array_group( IElementGenerator * generator ,
                                  const F & add , int depth ) {
  using array_type = typename boost::multi_array< T , K >;
  boost::array< typename array_type::index , K > shape;

  if( ! generator ) return;

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
   add( multi_array );
   for( boost::multi_array_types::size_type i = 0 ;
        i < multi_array->num_elements() ; ++i ) {
    generate_functions( p[ i ] , depth );
   }
  }
  else {
   add( multi_array );
   if constexpr( has_function_v< T > ) {
    const auto p = multi_array->data();
    for( boost::multi_array_types::size_type i = 0 ;
         i < multi_array->num_elements() ; ++i ) {
     generate_function( & p[ i ] , depth );
    }
   }
  }
 }

/*--------------------------------------------------------------------------*/

 void generate_nested_blocks( AbstractBlock * block , int depth ) {
  if( ( ! generator ) || ( ! generator->num_nested_block_generator ) ) return;
  auto & v_Block = block->access_nested_Blocks();
  auto num_nested_blocks = generator->num_nested_block_generator->
   gen_num_nested_blocks();
  v_Block.reserve( num_nested_blocks );
  for( decltype( num_nested_blocks ) i = 0 ; i < num_nested_blocks ; ++i ) {
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
