/*--------------------------------------------------------------------------*/
/*-------------------------- File AbstractPath.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the AbstractPath class, which represent a path from a Block
 * to one of its elements: a Block, a Constraint, a Variable, an Objective, or
 * a Function. The element can directly belong to the Block itself (even be
 * the Block itself) or belong to any of its sons, recursively.
 *
 * \version 0.10
 *
 * \date 01 - 12 - 2019
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
/// A path from a Block to one of its elements
/** The AbstractPath represents a path from a Block, here referred to as the
 * reference Block, to one of its elements (the target element): a Block, a
 * Constraint, a Variable, an Objective, or a Function. The target element can
 * directly belong to the reference Block itself (even be the reference Block
 * itself) or belong to any of its sons, recursively. The reference Block is
 * not explicitly represented in the path. In fact, the representation of the
 * path is independent from the reference Block. For the path to be
 * meaningful, the reference Block should be clear from the context. The
 * reference Block must be available when the path is constructed and when the
 * target element is retrieved. Furthermore, the type of the target element
 * cannot always be inferred from the path. The type of the target element,
 * therefore, must also be known from the context.
 *
 * The AbstractPath is particularly useful for the serialization and
 * deserialization of pointers to objects. If an object stores pointers to
 * other objects (for example, a Function has a set of pointers to Variable,
 * others have pointers to Block), these pointers can be serialized and
 * deserialized, making the construction of the object easier. The fact that
 * the representation of the path is independent from the reference Block also
 * facilitates its serialization and deserialization. Another advantage of
 * this independence from the reference Block is that the same path can be
 * used to target different objects.
 *
 * The path is defined as a sequence of nodes, each one represented by a Node
 * object. Each node has one of the following types: 'B', 'C', 'V', or
 * 'O'. These types indicate that the node is associated with a Block, a
 * Constraint, a Variable, and a Object, respectively. Notice that there is no
 * node associated with a Function, but this does not prevent one from
 * constructing a path to a Function. Although the nodes in the path are
 * arranged in the natural order, i.e., the first node is the origin of the
 * path and the last one is the destination, it is easier to understand the
 * path if we look at it backwards.
 *
 * Consider the path from some reference Block to some Variable. The last node
 * in this path necessarily has the 'V' type, indicating this is a path to a
 * Variable. This Variable belongs to some Block and is either static or
 * dynamic. This last node has all information needed to retrieve this
 * Variable from its father Block: a boolean indicating whether the Variable
 * is static or dynamic, the index of the group to which it belongs, and the
 * index of the Variable within that group.
 *
 * Note: The index of a Variable (or Constraint) within a group is a single
 *       number and may not be entirely obvious which number it should be
 *       (specially for multi-dimensional arrays and (multi-dimensional)
 *       arrays of lists). Please refer to the comments of the deserialize()
 *       function for an explanation of the index of a Variable (or
 *       Constraint) within a group.
 *
 * A 'V' node is always preceded by a 'B' node, unless it is the only node in
 * the path. If the path has only a single node, which is a 'V' node, then the
 * Variable this path refers to is defined in the reference Block. In other
 * words, if the target Variable of the path belongs to the reference Block,
 * then the path is formed by a single node whose type is 'V'.
 *
 * A 'B' node is associated with a Block and may contain the index of this
 * Block in the vector of nested Blocks of its father Block. There are two
 * cases in which this node does not have this index. In each of these cases,
 * the index has the value +Inf. These cases are:
 *
 * 1. The node is associated with the reference Block. Since the reference
 *    Block is the root of the three that contains the path, no allusion to
 *    the father of the reference Block must be made.
 *
 * 2. The Block with which the node is associated does not have a father
 *    Block. In this case, if the Block is not the reference Block, then it
 *    must be the inner Block of a BendersBFunction or that of a LagBFunction.
 *
 * If the index of a 'B' node is not +Inf, then this node is necessarily
 * preceded by another 'B' node, which is associated with the father of that
 * Block. It this index is +Inf then this Block is an inner Block of a
 * Function (a BendersBFunction or a LagBFunction). This Function either
 * belongs to an FRealObjective or an FRowConstraint, so that the previous
 * node in the path has either type 'O' or 'C'.
 *
 * An 'O' node, which is associated with an Objective, is either preceded by
 * a 'B' node (which is associated with the Block that owns that Objective) or
 * is the only node in the path. If it is the last node in the path, then the
 * target element is either this Objective or the Function in that Objective
 * (in which case that Objective is an FRealObjective). The type of the target
 * element must be known from the context. If this is not the last node in the
 * path, then the type of the next node in the path is 'B' and it is
 * associated with the inner Block of either a BendersBFunction or a
 * LagBFunction which is the Function of that Objective (and thus that
 * Objective is actually an FRealObjective).
 *
 * Finally, a 'C' node, which is associated with a Constraint, has
 * characteristics pertaining both the 'V' and the 'O' nodes. Like the 'V'
 * node, it has a boolean indicating whether it is static or not, the index of
 * the group to which it belongs, and the index of the Constraint within that
 * group (exactly as defined for Variables). Like an 'O' node, a 'C' node is
 * either preceded by a 'B' node (which is associated with the Block that
 * owns that Constraint) or is the only node in the path. If it is the last
 * node in the path, then the target element is either this Constraint or the
 * Function in that Constraint (in which case that Constraint is an
 * FRowConstraint). The type of the target element must be known from the
 * context. If this is not the last node in the path, then the type of the
 * next node in the path is 'B' and it is associated with the inner Block of
 * either a BendersBFunction or a LagBFunction which is the Function of that
 * Constraint (and thus that Constraint is actually an FRowConstraint).
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

 /// Node represents a node in the path
 /** This class is used to represent a node in the path. A Node can be of
  * four types, defined by the NodeType enum:
  *
  * 1. 'B', a Block
  * 2. 'C', a Constraint
  * 3. 'V', a Variable
  * 4. 'O', an Objective.
  */
 class Node {
 public:
  enum NodeType { eBlock = 'B' , eConstraint = 'C' ,
                  eVariable = 'V' , eObjective = 'O' };

  Node() : type( 'N' ) , is_static( true ) , index( Inf<Index>() ) ,
   element_index( Inf<Index>() ) {}

  char type;
  bool is_static;
  Index index;
  Index element_index;

  bool operator==( const Node & node ) const {
   return ( type == node.type ) && ( is_static == node.is_static ) &&
    ( index == node.index ) && ( element_index == node.element_index );
  }

  bool operator!=( const Node & node ) const {
   return ! ( * this == node );
  }
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
   if( t == reference_block ) {
    node.index = Inf<Index>();
    return path;
   }
   node.index = get_block_index( t );
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
    assert( node.index < block->get_nested_Blocks().size() );
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
  } // end for

  // Now, we analyse the last node in the path.

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
     SMSpp_di_unipi_it::serialize( node_group , "GroupIndex" ,
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

 /// deserializes an AbstractPath from a netCDF::NcGroup and returns it
 /**
  * This function constructs and returns an AbstractPath by deserializing it
  * from the given \p group. This \p group has a variable of type
  * netCDF::NcUint64() whose name is "NumNodes" that contains the number N of
  * nodes in the path.
  *
  *          ALL INDICES MENTIONED HERE BELONG TO ZERO-BASED NUMBERED
  *          SEQUENCE, I.E., SEQUENCES WHOSE FIRST ELEMENT IS 0.
  *
  * This \p group also has N sub-groups. The i-th sub-group, for i in {0, ...,
  * N-1}, has name Node_i and represents the i-th Node in the path. Each of
  * these sub-groups has a variable of type netCDF::NcChar() named "Type"
  * which stores the type of the Node, i.e., a NodeType. The value of the
  * variable "Type" may be any of the following letters:
  *
  * - 'O', if the node is associated with an Objective.
  *
  * - 'B', if the node is associated with a Block;
  *
  * - 'C', if the node is associated with a Constraint;
  *
  * - 'V', if the node is associated with a Variable;
  *
  * A group whose "Type" is 'O', i.e., representing a Node associated with an
  * Objective, has no other variable. If it is the last node in the path, then
  * the path refers to an Objective. Otherwise, the next node in the path must
  * be of type 'B'.
  *
  * A group whose "Type" is 'B', i.e., representing a Node associated with a
  * Block B, may have an additional variable of type netCDF::NcUint64() whose
  * name is "Index". If such a group does not represent the last node in the
  * path (i.e., it is the last group named "Node_N"), this variable must be
  * present. In this case, this variable is the index of the sub-Block of
  * Block B which is the next node in the path. If such a group represents the
  * last node in the path (i.e., its name is "Node_N"), then the destination
  * of the path is a Block which must be
  *
  * - the Block B itself if the variable "Index" is not present;
  *
  * - the j-th sub-Block of Block B, where j is the value of the variable
  *   "Index".
  *
  * A group whose type is 'C' or 'V', representing a Constraint or a Variable,
  * respectively, necessarily has the variables named "GroupIndex",
  * "ElementIndex", and "Static". The "Static" variable has type
  * netCDF::NcUbyte(). If the value of the variable "Static" is 1, it means
  * that the element this nodes represents (either a Constraint or a Variable)
  * is static. If the value of this variable is 0, then the element is
  * dynamic. The variable "GroupIndex" stores the index of the (static or
  * dynamic) group to which the element belongs. The variable "ElementIndex"
  * is the index of the element in that group.
  *
  * A static group can be one of three types:
  *
  * 1. It is a single Constraint/Variable;
  *
  * 2. It is a vector of Constraint/Variable;
  *
  * 3. It is a multi array of Constraint/Variable.
  *
  * In the first case, in which the group is a single Constraint/Variable, the
  * value of the "ElementIndex" variable is 0. In the second case, in which
  * the group is a vector of Constraint/Variable, the value of the
  * "ElementIndex" variable is the index of the element in that vector. In the
  * last case, in which the group is a multi array of of Constraint/Variable,
  * the value of the "ElementIndex" variable is the index of the element in
  * the vectorized multi array in row-major layout. For instance, if the multi
  * array has two dimensions with sizes m and n, respectively, then the
  * element at position (i, j) would have an element index equal to n * i + j
  * (recall the indices start from 0). In general, for a multi array with k
  * dimensions with sizes (n_0, ..., n_{k-1}), the element at position
  * (i_0, ..., i_{k-1}) would have an element index equal to
  *
  * \[
  *    \sum_{p = 0}^{k-1} ( \prod_{q = p + 1}^{k-1} n_q ) i_p.
  * \]
  *
  * A dynamic group can be one of three types:
  *
  * 1. It is list of Constraint/Variable;
  *
  * 2. It is a vector of lists of Constraint/Variable;
  *
  * 3. It is a multi array of lists of Constraint/Variable.
  *
  * In the first case, in which the group is a list of Constraint/Variable,
  * the value of the "ElementIndex" variable is the index of the element in
  * that list. In the second case, in which the group is a vector of lists of
  * Constraint/Variable, the value of the "ElementIndex" variable for an
  * element at position j of the k-th list of the vector is given by
  *
  * \[
  *    j + \sum_{i = 0}^{k-1} s_i
  * \]
  *
  * where s_i is the number of elements in the i-th list of the vector. The
  * last case is analogous.
  *
  * @param group The netCDF::NcGroup containing the path.
  *
  * @return The AbstractPath corresponding to the given group.
  */
 static AbstractPath deserialize( const netCDF::NcGroup & group ) {

  AbstractPath path;

  auto group_count = group.getGroupCount();
  for( decltype( group_count ) i = 0 ; i < group_count ; ++i ) {
   auto node_group = group.getGroup( "Node_" + std::to_string( i ) );
   auto & node = path.nodes.emplace_back();
   SMSpp_di_unipi_it::deserialize( node_group , "Type" , & node.type , false );
   switch( node.type ) {
    case( Node::eBlock ):
     if( i < group_count - 1 )
      SMSpp_di_unipi_it::deserialize( node_group , "Index" ,
                                      & node.index , false );
     else {
      if( ! SMSpp_di_unipi_it::deserialize( node_group , "Index" ,
                                            & node.index , true ) )
       node.index = Inf< Index >();
     }
     break;
    case( Node::eConstraint ):
    case( Node::eVariable ):
     SMSpp_di_unipi_it::deserialize( node_group , "GroupIndex" ,
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

 bool operator==( const AbstractPath & path ) const {
  if( nodes.size() != path.nodes.size() )
   return false;

  for( decltype( nodes )::size_type i = 0 ; i < nodes.size() ; ++ i ) {
   if( nodes[ i ] != path.nodes[ i ] )
    return false;
  }

  return true;
 }

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
