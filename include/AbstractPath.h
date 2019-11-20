/*--------------------------------------------------------------------------*/
/*-------------------------- File AbstractPath.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the AbstractPath class,
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
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __AbstractPath
#define __AbstractPath
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractPath.h"
#include "Block.h"
#include "BendersBFunction.h"
#include "Constraint.h"
#include "FRealObjective.h"
#include "FRowConstraint.h"
#include "LagBFunction.h"
#include "OneVarConstraint.h"
#include "RowConstraint.h"
#include "SMSTypedefs.h"

#include <cstddef>
#include <iterator>
#include "netcdf"
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- OTHER DEFINITIONS ----------------------------*/
/*--------------------------------------------------------------------------*/

/* The following are the types derived from Constraint and Variable. At the
 * end of this file, both Constraint_Derived_Classes and
 * Variable_Derived_Classes are undefined (#undef).
 */
#define Constraint_Derived_Classes FRowConstraint , BoxConstraint , \
  ZOConstraint , NPConstraint , NNConstraint , UBConstraint , \
  LBConstraint , UB0Constraint , LB0Constraint

#define Variable_Derived_Classes ColVariable

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS AbstractPath ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
///
/**
 */

class AbstractPath {

/*--------------------------------------------------------------------------*/
/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*---------------------------- PRIVATE TYPES -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Private Types
    @{ */

 using Index = unsigned int;

/**@}-----------------------------------------------------------------------*/
/*-------------------------- PRIVATE CLASSES -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Private Classes
    @{ */

 class Node {
 public:
  enum NodeType { eBlock = 'B' , eConstraint = 'C' ,
                  eVariable = 'V' , eObjective = 'O' };
  char type;
  bool is_static;
  Index index;
  Index element_index;
 };

/**@} ----------------------------------------------------------------------*/
/*----------------------------- PRIVATE METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

 template< class S , class T , unsigned long K >
 static Index get_static_index_( const S * s ,
                                 const boost::multi_array< T , K > & var ) {
  const auto p = var.data();
  if( ( s >= & p[ 0 ] ) && ( s <= & p[ var.num_elements() - 1 ] ) )
   return( static_cast< const T * >( s ) - & p[ 0 ] );
  return( Inf<Index>() );
 }

 template< class S , class T >
 static Index get_static_index_( const S * s , const std::vector< T > & var ) {
  if( ( s >= & var.front() ) && ( s <= & var.back() ) )
   return( static_cast< const T * >( s ) - & var.front() );
  return( Inf<Index>() );
 }

 template< class S , class T >
 static std::enable_if_t< std::is_base_of_v< S , T > ||
                          std::is_base_of_v< T , S > , Index >
 get_static_index_( const S * s , const T & var ) {
  if( s == & var )
   return 0;
  return( Inf<Index>() );
 }

/*--------------------------------------------------------------------------*/

 template< class S , class T , unsigned long K >
 static Index get_dynamic_index_
 ( const S * s , const boost::multi_array< std::list<T> , K > & var ) {
  auto p = var.data();
  Index index = 0;
  for( boost::multi_array_types::size_type i = var.num_elements() ;
       i-- ; ++p ) {
   for( const auto & ell : *p ) {
    if( s == & ell )
     return( index );
    ++index;
   }
  }
  return( Inf<Index>() );
 }

 template< class S , class T >
 static Index get_dynamic_index_
 ( const S * s , const std::vector< std::list<T> > & var ) {
  Index index = 0;
  for( typename std::vector< std::list<T> >::size_type i = 0 ;
       i < var.size() ; ++i ) {
   for( auto it = var[ i ].cbegin(); it != var[ i ].cend() ; ++it , ++index )
    if( s == &*it )
     return( index );
  }
  return( Inf<Index>() );
 }

 template< class S , class T >
 static Index get_dynamic_index_( const S * s , const std::list< T > & list ) {
  Index index = 0;
  for( auto it = list.cbegin(); it != list.end() ; ++it , ++index )
   if( s == &*it )
    return( index );
  return( Inf<Index>() );
 }

/*--------------------------------------------------------------------------*/

 template< typename T , unsigned long K >
 static T * get_static_element_( const boost::multi_array<T , K> & array ,
                                 Index index ) {
  if( index >= array.num_elements() )
   return nullptr;
  return const_cast< T *>( & array.data()[ index ] );
 }

 template< typename T >
 static T * get_static_element_( const std::vector< T > & vec , Index index ) {
  if( index >= vec.size() )
   return nullptr;
  return const_cast< T *>( & vec[ index ] );
 }

 template< typename T >
 static T * get_static_element_( const T & t , Index index ) {
  assert( index == 0 );
  return const_cast< T *>( & t );
 }

/*--------------------------------------------------------------------------*/

 template< typename T , unsigned long K >
 static T * get_dynamic_element_
 ( const boost::multi_array< std::list<T> , K > & multi_array , Index index ) {
  Index past_size = 0;
  for( auto list = multi_array.origin();
       list < ( multi_array.origin() + multi_array.num_elements() ); ++list )  {
   if( index < past_size + list->size() ) {
    auto it = list->begin();
    for( ; past_size < index ; ++past_size , ++it );
    return const_cast< T * >( &*it );
   }
   past_size += list->size();
  }
  return nullptr;
 }

 template< typename T >
 static T * get_dynamic_element_( const std::vector< std::list<T> > & lists ,
                                  Index index ) {
  Index past_size = 0;
  for( auto & list : lists ) {
   if( past_size + list.size() > index ) {
    auto it = list.begin();
    for( ; past_size < index ; ++it , ++past_size );
    return const_cast< T * >( &*it );
   }
   past_size += list.size();
  }
  return nullptr;
 }

 template< typename T >
 static T * get_dynamic_element_( const std::list<T> & list , Index index ) {
  auto it = list.begin();
  for( Index i = 0 ; i < index && it != list.cend(); ++i , ++it );
  if( it != list.end() )
   return const_cast< T * >( &*it );
  return nullptr;
 }

/*--------------------------------------------------------------------------*/

 template<class S , class T , class... Rest>
 static S * get_static_element( const boost::any & group , Index index ) {
  if constexpr( std::is_base_of_v< S , T > ) {
   S * element = nullptr;
   bool group_found = un_any_thing
    ( T , group , { element = get_static_element_( var , index ); } );
   if( group_found )
    return element;
  }
  else if constexpr( sizeof...(Rest) != 0 )
   return get_static_element<S , Rest...>( group , index );
  return nullptr;
 }

/*--------------------------------------------------------------------------*/

 template<class S , class T , class... Rest>
 static S * get_dynamic_element( const boost::any & group , Index index ) {
  if constexpr( std::is_base_of_v< S , T > ) {
   S * element = nullptr;
   bool group_found = un_any_thing
    ( std::list<T> , group ,
      { element = get_dynamic_element_( var , index ); } );
   if( group_found )
    return element;
  }
  else if constexpr( sizeof...(Rest) != 0 )
   return get_dynamic_element<S , Rest...>( group , index );
  return nullptr;
 }

/*--------------------------------------------------------------------------*/

 template<class T>
 static const boost::any & get_group( const Block * block , Index group_index ,
                                      bool is_static ) {
  if( std::is_base_of_v< Constraint , T > ) {
   if( is_static ) {
    const auto & group = block->get_static_constraints();
    if( group_index >= group.size() )
     throw( std::invalid_argument( "get_group: invalid group index: " +
                                   std::to_string( group_index ) ) );
    return group[ group_index ];
   }
   else {
    const auto & group = block->get_dynamic_constraints();
    if( group_index >= group.size() )
     throw( std::invalid_argument( "get_group: invalid group index: " +
                                   std::to_string( group_index ) ) );
    return group[ group_index ];
   }
  }
  else if( std::is_base_of_v< Variable , T > ) {
   if( is_static ) {
    const auto & group = block->get_static_variables();
    if( group_index >= group.size() )
     throw( std::invalid_argument( "get_group: invalid group index: " +
                                   std::to_string( group_index ) ) );
    return group[ group_index ];
   }
   else {
    const auto & group = block->get_dynamic_variables();
    if( group_index >= group.size() )
     throw( std::invalid_argument( "get_group: invalid group index: " +
                                   std::to_string( group_index ) ) );
    return group[ group_index ];
   }
  }
  else
   throw( std::invalid_argument( "get_group: group not found: " ) );
 }

/*--------------------------------------------------------------------------*/

 template<class T>
 static T * get_element( const Block * block , bool is_static ,
                         Index group_index , Index element_index ) {
  auto group = get_group<T>( block , group_index , is_static );
  constexpr bool is_variable = std::is_base_of_v< Variable , T >;
  if constexpr ( is_variable ) {
   if( is_static )
    return get_static_element< T , Variable_Derived_Classes >
     ( group , element_index );
   else
    return get_dynamic_element< T , Variable_Derived_Classes >
     ( group , element_index );
  }
  else {
   if( is_static )
    return get_static_element< T , Constraint_Derived_Classes >
     ( group , element_index );
   else
    return get_dynamic_element< T , Constraint_Derived_Classes >
     ( group , element_index );
  }
 }

/*--------------------------------------------------------------------------*/

 template<class S , class T , class... Rest>
 static Index get_static_index( const S * s , const boost::any & group ) {
  if constexpr( std::is_base_of_v< S , T > ) {
   Index index = Inf<Index>();
   bool group_found =
    un_any_thing( T , group , { index = get_static_index_( s , var ); } );
   if( group_found && index < Inf<Index>()  )
    return index;
  }
  else if constexpr( sizeof...(Rest) != 0 )
   return get_static_index<S , Rest...>( s , group );
  return Inf<Index>();
 }

/*--------------------------------------------------------------------------*/

 template<class S , class T , class... Rest>
 static Index get_dynamic_index( const S * s , const boost::any & group ) {
  if constexpr( std::is_base_of_v< S , T > ) {
   Index index = Inf<Index>();
   bool group_found = un_any_thing
    ( std::list<T> , group , { index = get_dynamic_index_( s , var ); } );
   if( group_found && index < Inf<Index>()  )
    return index;
  }
  else if constexpr( sizeof...(Rest) != 0 )
   return get_dynamic_index<S , Rest...>( s , group );
  return Inf<Index>();
 }

/*--------------------------------------------------------------------------*/

 template<class T>
 static std::pair< Index , Index > get_element_index
 ( T * t , const Vec_any & groups , bool is_static ) {

  constexpr bool is_variable = std::is_base_of_v< Variable , T >;

  for( Index group_index = 0 ; group_index < groups.size() ;
       ++group_index  ) {

   Index index;

   if constexpr ( is_variable ) {
    if( is_static )
     index = get_static_index< T , Variable_Derived_Classes >
      ( t , groups[ group_index ] );
    else
     index = get_dynamic_index< T , Variable_Derived_Classes >
      ( t , groups[ group_index ] );
   }
   else {
    if( is_static )
     index = get_static_index< T , Constraint_Derived_Classes >
      ( t , groups[ group_index ] );
    else
     index = get_dynamic_index< T , Constraint_Derived_Classes >
      ( t , groups[ group_index ] );
   }

   if( index < Inf<Index>() )
    return std::make_pair( group_index , index );
  }
  return std::make_pair( Inf<Index>() , Inf<Index>() );
 }

/*--------------------------------------------------------------------------*/

 template<class T>
 static std::pair< Index , Index > get_static_element_index( T * t ) {

  const auto block = t->get_Block();

  if( ! block )
   return std::make_pair( Inf<Index>() , Inf<Index>() );

  if constexpr( std::is_base_of_v< Constraint , T > )
   return get_element_index( t , block->get_static_constraints() , true );
  else if constexpr( std::is_base_of_v< Variable , T > )
   return get_element_index( t , block->get_static_variables() , true );
  else
   return std::make_pair( Inf<Index>() , Inf<Index>() );
 }

/*--------------------------------------------------------------------------*/

 template<class T>
 static std::pair< Index , Index > get_dynamic_element_index( T * t ) {

  const auto block = t->get_Block();

  if( ! block )
   return std::make_pair( Inf<Index>() , Inf<Index>() );

  if constexpr( std::is_base_of_v< Constraint , T > )
   return get_element_index( t , block->get_dynamic_constraints() , false );
  else if constexpr( std::is_base_of_v< Variable , T > )
   return get_element_index( t , block->get_dynamic_variables() , false );
  else
   return std::make_pair( Inf<Index>() , Inf<Index>() );
 }

/*--------------------------------------------------------------------------*/

 template<class T>
 static std::tuple< Index , Index , bool > get_element_index( T * t ) {
  auto index_pair = get_static_element_index( t );
  if( index_pair.first < Inf<Index>() )
   return std::make_tuple( index_pair.first , index_pair.second , true );
  else {
   index_pair = get_dynamic_element_index( t );
   return std::make_tuple( index_pair.first , index_pair.second , false );
  }
 }

/*--------------------------------------------------------------------------*/

 static Index get_block_index( const Block * block ) {
  auto father = block->get_f_Block();
  if( father ) {
   const auto & nb = father->get_nested_Blocks();
   const auto nbit = std::find( nb.begin() , nb.end() , block );
   assert( nbit != nb.end() );
   return( std::distance( nb.begin() , nbit ) );
  }
  return Inf<Index>();
 }

/*--------------------------------------------------------------------------*/

 template<class T>
 static Node build_node( const T * t ) {
  auto triple = get_element_index( t );
  if( std::get< 0 >( triple ) < Inf<Index>() ) {
   Node node;
   node.index = std::get< 0 >( triple );
   node.element_index = std::get< 1 >( triple );
   node.is_static = std::get< 2 >( triple );
   return node;
  }
  else
   throw( std::logic_error( "build_node: Element not found." ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

protected:

/*--------------------------------------------------------------------------*/
/*---------------------------- PROTECTED FIELDS  ---------------------------*/
/*--------------------------------------------------------------------------*/

 std::vector< Node > nodes;

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

 template<class T>
 static AbstractPath build_path( const T * t ,
                                 const Block * reference_block ) {

  static_assert( std::is_base_of_v< Block , T > ||
                 std::is_base_of_v< Constraint , T > ||
                 std::is_base_of_v< Function , T > ||
                 std::is_base_of_v< Objective , T > ||
                 std::is_base_of_v< Variable , T > ,
                 "build_path: the element type must be one of: "
                 "Block, Constraint, Function, Objective, Variable. " );

  AbstractPath path;

  if( ! t )
   return path;

  if constexpr( std::is_base_of_v< Constraint , T > ) {
   path.nodes.push_back( build_node( t ) );
   path.nodes.back().type = Node::eConstraint;
  }
  else if constexpr( std::is_base_of_v< Variable , T > ) {
   path.nodes.push_back( build_node( t ) );
   path.nodes.back().type = Node::eVariable;
  }
  else if constexpr( std::is_base_of_v< Objective , T > ) {
   path.nodes.emplace_back().type = Node::eObjective;
  }
  else if constexpr( std::is_base_of_v< Function , T > ) {
   auto observer = t->get_Observer();
   if( auto constraint = dynamic_cast< FRowConstraint * >( observer ) ) {
    path.nodes.push_back( build_node( constraint ) );
    path.nodes.back().type = Node::eConstraint;
   }
   else if( dynamic_cast< FRealObjective * >( observer ) ) {
    path.nodes.emplace_back().type = Node::eObjective;
   }
   else
    throw( std::logic_error( "build_path: Unknown Observer of "
                             "given Function." ) );
  }
  else if constexpr( std::is_base_of_v< Block , T > ) {
   auto & node = path.nodes.emplace_back();
   node.type = Node::eBlock;
   node.index = get_block_index( t );
   if( t == reference_block )
    return path;
  }

  Block * block;
  if constexpr ( std::is_base_of_v< Block , T > )
   block = t->get_f_Block();
  else if constexpr ( std::is_base_of_v< Function , T > ) {
   auto observer = t->get_Observer();
   if( ! observer )
    throw( std::logic_error( "build_path: Path not found. Function has no "
                             "Observer." ) );
   block = observer->get_Block();
  }
  else
   block = t->get_Block();

  while( block != reference_block ) {

   auto index = get_block_index( block );

   if( index < Inf<Index>() ) {
    // block has a father Block
    auto & node = path.nodes.emplace_back();
    node.type = Node::eBlock;
    node.index = index;
    block = block->get_f_Block();
   }

   else {
    // block has no father. So, block must be either a BendersBFunction or a
    // LagBFunction.

    Observer * observer;

    if( const auto benders = dynamic_cast< BendersBFunction * >( block ) )
     observer = benders->get_Observer();
    else if( const auto lag = dynamic_cast< LagBFunction * >( block ) )
     observer = lag->get_Observer();
    else
     throw( std::logic_error( "build_path: Path not found." ) );

    if( const auto frc = dynamic_cast< FRowConstraint * >( observer ) ) {
     path.nodes.push_back( build_node( frc ) );
     path.nodes.back().type = Node::eConstraint;
     block = frc->get_Block();
    }
    else if( const auto fro = dynamic_cast< FRealObjective * >( observer ) ) {
     path.nodes.emplace_back().type = Node::eObjective;
     block = fro->get_Block();
    }
    else
     throw( std::logic_error( "build_path: Path not found." ) );
   }
  }

  std::reverse( std::begin( path.nodes ), std::end( path.nodes ) );
  return path;
 }

/*--------------------------------------------------------------------------*/

 template<class T>
 static T * get_element( const AbstractPath & path , Block * reference ) {

  if( path.nodes.empty() )
   return nullptr;

  auto block = reference;

  for( std::vector<Node>::size_type i = 0 ; i < path.nodes.size() - 1 ; ++i ) {
   const auto & node = path.nodes[ i ];

   // Intermediate nodes can be: Block, Constraint, or Objective.

   if( node.type == Node::eBlock ) {
    block = block->get_nested_Blocks()[ node.index ];
   }
   else if( node.type == Node::eConstraint ) {
    auto constraint = get_element< Constraint >
     ( block , node.is_static , node.index , node.element_index );

    if( const auto frowc = dynamic_cast<const FRowConstraint *>( constraint ) ) {
     auto function = frowc->get_function();

     if( const auto benders = dynamic_cast<const BendersBFunction *>( function ) )
      block = benders->get_inner_block();
     else if( auto lag = dynamic_cast< LagBFunction  *>( function ) )
      block = lag->get_inner_block();
     else // not found
      return nullptr;
    }
    else // not found
     return nullptr;
   }
   else if( node.type == Node::eObjective ) {
    auto objective = block->get_objective();

    if( const auto fro = dynamic_cast< const FRealObjective * >( objective ) ) {
     auto function = fro->get_function();

     if( const auto benders = dynamic_cast<const BendersBFunction *>( function ) )
      block = benders->get_inner_block();
     else if( auto lag = dynamic_cast< LagBFunction * >( function ) )
      block = lag->get_inner_block();
     else // not found
      return nullptr;
    }
    else // not found
     return nullptr;
   }
   else // not found
    return nullptr;
  }

  const auto & node = path.nodes.back();

  if constexpr( std::is_base_of_v< Constraint , T > ) {
   assert( node.type == Node::eConstraint );
   return get_element< T >( block , node.is_static , node.index ,
                            node.element_index );
  }
  else if constexpr( std::is_base_of_v< Variable , T > ) {
   assert( node.type == Node::eVariable );
   return get_element< T >( block , node.is_static , node.index ,
                            node.element_index );
  }
  else if constexpr( std::is_base_of_v< Objective , T > ) {
   assert( node.type == Node::eObjective );
   return block->get_objective();
  }
  else if constexpr( std::is_base_of_v< Function , T > ) {
   if( node.type == Node::eConstraint ) {
    // It must be an FRowConstraint
    auto constraint = get_element< FRowConstraint >
     ( block , node.is_static , node.index , node.element_index );
    if( ! constraint )
     return nullptr;
    else
     return constraint->get_function();
   }
   else if( node.type == Node::eObjective ) {
    // It must be an FRealObjective
    auto objective = dynamic_cast< FRealObjective * >( block->get_objective() );
    if( ! objective )
     return nullptr;
    else
     return objective->get_function();
   }
   else
    return nullptr;
  }
  else if constexpr( std::is_base_of_v< Block , T > ) {
   assert( node.type == Node::eBlock );

   if( node.index == Inf< Index >() )
    return block;

   const auto & nested_blocks = block->get_nested_Blocks();
   if( node.index >= nested_blocks.size() )
    return nullptr;
   return nested_blocks[ node.index ];
  }
  else
   return nullptr;
 }

/*--------------------------------------------------------------------------*/

 static void serialize( const AbstractPath & path , netCDF::NcGroup & group ) {
  Index i = 0;
  for( const auto & node : path.nodes ) {
   auto node_group = group.addGroup( "Node_" + std::to_string( i++ ) );
   SMSpp_di_unipi_it::serialize( node_group , "Type" ,
                                 netCDF::NcChar() , node.type );
   switch( node.type ) {
    case( Node::eBlock ):
     SMSpp_di_unipi_it::serialize( node_group , "Index" ,
                                   netCDF::NcUint64() , node.index );
     break;
    case( Node::eConstraint ):
    case( Node::eVariable ):
     SMSpp_di_unipi_it::serialize( node_group , "Index" ,
                                   netCDF::NcUint64() , node.index );
     SMSpp_di_unipi_it::serialize( node_group , "ElementIndex" ,
                                   netCDF::NcUint64() , node.element_index );
     SMSpp_di_unipi_it::serialize( node_group , "Static" ,
                                   netCDF::NcUbyte() , node.is_static );
     break;
   }
  }
 }

/*--------------------------------------------------------------------------*/

 static AbstractPath deserialize( const netCDF::NcGroup & group ) {

  AbstractPath path;

  for( Index i = 0 ; i < group.getGroupCount() ; ++i ) {
   auto node_group = group.getGroup( "Node_" + std::to_string( i ) );
   auto & node = path.nodes.emplace_back();
   SMSpp_di_unipi_it::deserialize( node_group , "Type" , & node.type , false );
   switch( node.type ) {
    case( Node::eBlock ):
     SMSpp_di_unipi_it::deserialize( node_group , "Index" ,
                                     & node.index , false );
     break;
    case( Node::eConstraint ):
    case( Node::eVariable ):
     SMSpp_di_unipi_it::deserialize( node_group , "Index" ,
                                     & node.index , false );
     SMSpp_di_unipi_it::deserialize( node_group , "ElementIndex" ,
                                     & node.element_index , false );
     SMSpp_di_unipi_it::deserialize( node_group , "Static" ,
                                     & node.is_static , false );
     break;
   }
  }
  return path;
 }

/*--------------------------------------------------------------------------*/

};  // end( class( AbstractPath ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#undef Constraint_Derived_Classes
#undef Variable_Derived_Classes

#endif  /* AbstractPath.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File AbstractPath.h --------------------------*/
/*--------------------------------------------------------------------------*/
