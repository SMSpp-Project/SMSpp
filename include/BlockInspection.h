/*--------------------------------------------------------------------------*/
/*------------------------- File BlockInspection.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 *
 * \version 0.10
 *
 * \date 18 - 07 - 2020
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

#ifndef __BlockInspection
#define __BlockInspection
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"
#include "BendersBFunction.h"
#include "FRealObjective.h"
#include "FRowConstraint.h"
#include "LagBFunction.h"
#include "OneVarConstraint.h"

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

namespace SMSpp_di_unipi_it::inspection
{

 using Index = Block::Index;

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
       list < ( multi_array.origin() + multi_array.num_elements() ); ++list ) {
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

/// returns the index of the given element in the given boost::any group
/** Returns the index of the given \p element in the given boost::any \p
 * group.
 *
 * @param element A pointer to the element whose index in \p group is desired.
 *
 * @param group The group containing the element.
 *
 * @param is_static Indicates whether the given group is static or dynamic.
 *
 * @return The index of the given element in the given group.
 */
 template< class T >
 static Index get_index( const T * element , const boost::any & group ,
                         const bool is_static ) {
  if( is_static )
   return inspection::get_static_index< T, T >( element , group );
  else
   return inspection::get_dynamic_index< T, T >( element , group );
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

 /// returns a pointer to the element at the given index in the given group
 /** Returns a pointer to the element of type \p T located at the given
  * position \p index in the given boost::any \p group. If the element is not
  * found, nullptr is returned. The parameter \p is_static indicates whether
  * the given group must be considered static or dynamic.
  *
  * @param group A boost::any.
  *
  * @param index The index of the element in the given group.
  *
  * @param is_static Indicates whether the given group is static or dynamic.
  *
  * @return If an element of type T is found at position \p index in the given
  *         \p group, then a pointer to this element is returned. Otherwise,
  *         nullptr is returned.
  */
 template< class T >
 static T * get_element( const boost::any & group , const Block::Index index ,
                         const bool is_static ) {
  if( is_static )
   return get_static_element< T, T >( group , index );
  else
   return get_dynamic_element< T, T >( group , index );
 }

/*--------------------------------------------------------------------------*/

 /// returns a pointer to the Constraint identified by the given \p id
 /** Returns a pointer to the Constraint identified by the given \p id in the
  * given \p block. The desired Constraint must have exactly the type T. If no
  * Constraint of type T with the given \p id is found, nullptr is returned.
  *
  * @param block The Block to which the desired Constraint belongs.
  *
  * @param id The Block::ConstraintID identifying the Constraint in the given
  *        \p block.
  *
  * @return If there is a Constraint of type T with the given \p id in the
  *         given \p Block, then a pointer to this Constraint is
  *         returned. Otherwise, nullptr is returned.
  */
 template< class T >
 static T * get_Constraint( const Block * const block ,
                            const Block::ConstraintID id ) {
  const auto & static_constraints = block->get_static_constraints();
  const auto num_static_groups = static_constraints.size();
  auto group_index = id.first;
  auto constraint_index = id.second;

  if( group_index < num_static_groups ) {
   // A static Constraint
   auto any_group = static_constraints[ group_index ];
   return get_element< T >( any_group , constraint_index , true );
  }
  else {
   // A dynamic Constraint
   group_index = id.first - num_static_groups;
   const auto & dynamic_constraints = block->get_dynamic_constraints();

   if( group_index >= dynamic_constraints.size() )
    throw( std::logic_error( "get_Constraint: invalid dynamic Constraint group "
                             "index: " + std::to_string( group_index ) ) );

   auto any_group = dynamic_constraints[ group_index ];
   return get_element< T >( any_group , constraint_index , false );
  }
 }

/*--------------------------------------------------------------------------*/

 /// returns a pointer to the inner Block associated with the given Function
 /** Returns a pointer to the inner Block associated with the given \p
  * function. The given Function is expected to be either a BendersBFunction or
  * a LagBFunction. If the given Function is not of any of these two types,
  * then nullptr is returned.
  *
  * @param function A pointer to the Function whose inner Block is desired.
  *
  * @return The inner Block associated with the given Function. If the given
  *         Function has no associated inner Block, then nullptr is returned.
  */
 static Block * get_indirect_sub_Block( const Function * const function ) {
  if( ! function )
   return nullptr;
  if( auto f = dynamic_cast< const BendersBFunction * >( function ) )
   return f->get_inner_block();
  else if( auto f = dynamic_cast< const LagBFunction * >( function ) )
   return f->get_inner_block();
  return nullptr;
 }

/*--------------------------------------------------------------------------*/

 /// returns the inner Block associated with the Objective of the given Block
 /** Returns a pointer to the inner Block associated with the given Objective
  * of \p block. If the Objective of the given Block is not an FRealObjective,
  * then nullptr is returned.
  *
  * @param block A pointer to the Block containing an Objective whose
  *        associated inner Block is desired.
  *
  * @return A pointer to the inner Block associated with the Objective of the
  *         given Block. If the Objective of the given Block is not an
  *         FRealObjective, nullptr is returned.
  */
 static Block * get_indirect_sub_Block( const Block * const block ) {
  if( auto objective = dynamic_cast<FRealObjective *>( block->get_objective() ) )
   return get_indirect_sub_Block( objective->get_function() );
  return nullptr;
 }

/*--------------------------------------------------------------------------*/

 /// returns a pointer to the inner Block associated with the given Constraint
 /** Returns a pointer to the inner Block associated with the given \p
  * constraint. If the given Constraint has no inner Block associated with it,
  * nullptr is returned.
  *
  * @param constraint A pointer to the Constraint whose associated inner Block
  *        is desired.
  *
  * @return The inner Block associated with the given Constraint. If the given
  *         Constraint has no associated inner Block, then nullptr is returned.
  */
 static Block * get_indirect_sub_Block( const Constraint * const constraint ) {
  if( auto frowc = dynamic_cast< const FRowConstraint * >( constraint ) )
   return get_indirect_sub_Block( frowc->get_function() );
  return nullptr;
 }

/*--------------------------------------------------------------------------*/

 /// returns a pointer to the inner Block associated with the given Constraint
 /** Returns a pointer to the inner Block associated with the Constraint,
  * belonging to \p block, specified by the given ConstraintID \p id (if any).
  *
  * @param block The Block to which the Constraint belongs.
  *
  * @param id The Block::ConstraintID identifying the Constraint in the given
  *        \p block.
  *
  * @return The inner Block associated with the given Constraint.
  */
 static Block * get_indirect_sub_Block( const Block * const block ,
                                        const Block::ConstraintID id ) {
  auto constraint = get_Constraint< FRowConstraint >( block , id );
  return get_indirect_sub_Block( constraint );
 }

}  // end( namespace SMSpp_di_unipi_it::inspection )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#undef Constraint_Derived_Classes
#undef Variable_Derived_Classes

#endif  /* BlockInspection.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File BlockInspection.h ------------------------*/
/*--------------------------------------------------------------------------*/
