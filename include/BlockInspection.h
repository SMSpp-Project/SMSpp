/*--------------------------------------------------------------------------*/
/*------------------------- File BlockInspection.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 *
 * This file defines the namespace "inspection" within
 * "SMSpp_di_unipi_it". This namespace contains a number of functions that are
 * useful for inspecting and obtaining information from a Block. In
 * particular, it provides functions that scan the "abstract" representation
 * of a Block to retrieve, for instance
 *
 * - a Constraint or a Variable given their location in their Block;
 *
 * - the location of a Constraint or a Variable in their Block.
 *
 * For the scan of the "abstract" representation to work, it is necessary to
 * boost::any_cast<> (in particular, Constraint and Variable), and therefore
 * it has to have a list of the kind of types thay they may have. Hence, some
 * functions at any point in time works only with a specific subset of those
 * classes, and if new types need be handled then the class has to be manually
 * updated. This is made a bit easier by the two macros
 *
 *     Constraint_Derived_Classes
 *     Variable_Derived_Classes
 *
 * defined in this header file (and immediately un-defined at the end).
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Rafael Durbano Lobato
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

 template< class S , class T , std::size_t K >
 static Index get_static_index_( const S * s ,
                                 const boost::multi_array< T , K > & var ) {
  const auto p = var.data();
  if( ( s >= & p[ 0 ] ) && ( s <= & p[ var.num_elements() - 1 ] ) )
   return( static_cast< const T * >( s ) - & p[ 0 ] );
  return( Inf< Index >() );
 }

 template< class S , class T >
 static Index get_static_index_( const S * s , const std::vector< T > & var ) {
  if( ( s >= & var.front() ) && ( s <= & var.back() ) )
   return( static_cast< const T * >( s ) - & var.front() );
  return( Inf< Index >() );
 }

 template< class S , class T >
 static std::enable_if_t< std::is_base_of_v< S , T > ||
                          std::is_base_of_v< T , S > , Index >
 get_static_index_( const S * s , const T & var ) {
  if( s == & var )
   return( 0 );
  return( Inf< Index >() );
 }

/*--------------------------------------------------------------------------*/

 template< class S , class T , std::size_t K >
 static Index get_dynamic_index_
 ( const S * s , const boost::multi_array< std::list< T > , K > & var ) {
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
  return( Inf< Index >() );
 }

 template< class S , class T >
 static Index get_dynamic_index_
 ( const S * s , const std::vector< std::list< T > > & var ) {
  Index index = 0;
  for( typename std::vector< std::list< T > >::size_type i = 0 ;
       i < var.size() ; ++i ) {
   for( auto it = var[ i ].cbegin(); it != var[ i ].cend() ; ++it , ++index )
    if( s == &*it )
     return( index );
  }
  return( Inf< Index >() );
 }

 template< class S , class T >
 static Index get_dynamic_index_( const S * s , const std::list< T > & list ) {
  Index index = 0;
  for( auto it = list.cbegin(); it != list.end() ; ++it , ++index )
   if( s == &*it )
    return( index );
  return( Inf< Index >() );
 }

/*--------------------------------------------------------------------------*/

 template< typename T , std::size_t K >
 static T * get_static_element_( const boost::multi_array< T , K > & array ,
                                 Index index ) {
  if( index >= array.num_elements() )
   return( nullptr );
  return( const_cast< T *>( & array.data()[ index ] ) );
 }

 template< typename T >
 static T * get_static_element_( const std::vector< T > & vec , Index index ) {
  if( index >= vec.size() )
   return( nullptr );
  return( const_cast< T *>( & vec[ index ] ) );
 }

 template< typename T >
 static T * get_static_element_( const T & t , Index index ) {
  assert( index == 0 );
  return( const_cast< T *>( &t ) );
 }

/*--------------------------------------------------------------------------*/

 template< typename T , std::size_t K >
 static T * get_dynamic_element_
 ( const boost::multi_array< std::list< T > , K > & multi_array , Index index ) {
  Index past_size = 0;
  for( auto list = multi_array.origin();
       list < ( multi_array.origin() + multi_array.num_elements() ); ++list ) {
   if( index < past_size + list->size() ) {
    auto it = list->begin();
    for( ; past_size < index ; ++past_size , ++it );
    return( const_cast< T * >( &*it ) );
   }
   past_size += list->size();
  }
  return( nullptr );
 }

 template< typename T >
 static T * get_dynamic_element_( const std::vector< std::list< T > > & lists ,
                                  Index index ) {
  Index past_size = 0;
  for( auto & list : lists ) {
   if( past_size + list.size() > index ) {
    auto it = list.begin();
    for( ; past_size < index ; ++it , ++past_size );
    return( const_cast< T * >( &*it ) );
   }
   past_size += list.size();
  }
  return( nullptr );
 }

 template< typename T >
 static T * get_dynamic_element_( const std::list< T > & list , Index index ) {
  auto it = list.begin();
  for( Index i = 0 ; i < index && it != list.cend(); ++i , ++it );
  if( it != list.end() )
   return( const_cast< T * >( &*it ) );
  return( nullptr );
 }

/*--------------------------------------------------------------------------*/

 template< class S , class T , class... Rest>
 static Index get_static_index( const S * s , const boost::any & group ) {
  if constexpr( std::is_base_of_v< S , T > ) {
   Index index = Inf< Index >();
   bool group_found =
    un_any_thing( T , group , { index = get_static_index_( s , var ); } );
   if( group_found && index < Inf< Index >()  )
    return( index );
  }
  if constexpr( sizeof...(Rest) != 0 )
   return( get_static_index< S , Rest... >( s , group ) );
  return( Inf< Index >() );
 }

/*--------------------------------------------------------------------------*/

 template< class S , class T , class... Rest >
 static Index get_dynamic_index( const S * s , const boost::any & group ) {
  if constexpr( std::is_base_of_v< S , T > ) {
   Index index = Inf< Index >();
   bool group_found = un_any_thing
    ( std::list< T > , group , { index = get_dynamic_index_( s , var ); } );
   if( group_found && index < Inf< Index >()  )
    return( index );
  }
  if constexpr( sizeof...(Rest) != 0 )
   return( get_dynamic_index< S , Rest... >( s , group ) );
  return( Inf< Index >() );
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
   return( inspection::get_static_index< T , T >( element , group ) );
  else
   return( inspection::get_dynamic_index< T , T >( element , group ) );
 }

/*--------------------------------------------------------------------------*/

 template< class S , class T , class... Rest >
 static S * get_static_element( const boost::any & group , Index index ) {
  if constexpr( std::is_base_of_v< S , T > ) {
   S * element = nullptr;
   bool group_found = un_any_thing
    ( T , group , { element = get_static_element_( var , index ); } );
   if( group_found )
    return( element );
  }
  if constexpr( sizeof...(Rest) != 0 )
   return( get_static_element< S , Rest... >( group , index ) );
  return( nullptr );
 }

/*--------------------------------------------------------------------------*/

 template< class S , class T , class... Rest >
 static S * get_dynamic_element( const boost::any & group , Index index ) {
  if constexpr( std::is_base_of_v< S , T > ) {
   S * element = nullptr;
   bool group_found = un_any_thing
    ( std::list< T > , group ,
      { element = get_dynamic_element_( var , index ); } );
   if( group_found )
    return( element );
  }
  if constexpr( sizeof...(Rest) != 0 )
   return( get_dynamic_element< S , Rest... >( group , index ) );
  return( nullptr );
 }

/*--------------------------------------------------------------------------*/

 template< class T >
 static const boost::any & get_group( const Block * block , Index group_index ,
                                      bool is_static ) {
  if( std::is_base_of_v< Constraint , T > ) {
   if( is_static ) {
    const auto & group = block->get_static_constraints();
    if( group_index >= group.size() )
     throw( std::invalid_argument( "get_group: invalid group index: " +
                                   std::to_string( group_index ) ) );
    return( group[ group_index ] );
   }
   else {
    const auto & group = block->get_dynamic_constraints();
    if( group_index >= group.size() )
     throw( std::invalid_argument( "get_group: invalid group index: " +
                                   std::to_string( group_index ) ) );
    return( group[ group_index ] );
   }
  }
  else if( std::is_base_of_v< Variable , T > ) {
   if( is_static ) {
    const auto & group = block->get_static_variables();
    if( group_index >= group.size() )
     throw( std::invalid_argument( "get_group: invalid group index: " +
                                   std::to_string( group_index ) ) );
    return( group[ group_index ] );
   }
   else {
    const auto & group = block->get_dynamic_variables();
    if( group_index >= group.size() )
     throw( std::invalid_argument( "get_group: invalid group index: " +
                                   std::to_string( group_index ) ) );
    return( group[ group_index ] );
   }
  }
  else
   throw( std::invalid_argument( "get_group: group not found: " ) );
 }

/*--------------------------------------------------------------------------*/

 /// returns a pointer to the element of the \p block at the specified position
 /** This function returns a pointer to the element of the given Block located
  * at the specified position. If no such an element is found, nullptr is
  * returned. \p T must be the type of a Constraint or a Variable.
  *
  * @param block A pointer to a Block.
  *
  * @param is_static Indicates whether the element is static.
  *
  * @param group_index The index of the group to which the element belongs.
  *
  * @param element_index The index of the desired element within its group.
  *
  * @return If an element of type \p T (or derived from \p T) is found at the
  *         specified location, a pointer to this element is
  *         returned. Otherwise, nullptr is returned. */

 template< class T >
 static T * get_element( const Block * block , bool is_static ,
                         Index group_index , Index element_index ) {
  auto group = get_group< T >( block , group_index , is_static );
  constexpr bool is_variable = std::is_base_of_v< Variable , T >;
  if constexpr ( is_variable ) {
   if( is_static )
    return( get_static_element< T , Variable_Derived_Classes >
     ( group , element_index ) );
   else
    return( get_dynamic_element< T , Variable_Derived_Classes >
     ( group , element_index ) );
  }
  else {
   if( is_static )
    return( get_static_element< T , Constraint_Derived_Classes >
     ( group , element_index ) );
   else
    return( get_dynamic_element< T , Constraint_Derived_Classes >
     ( group , element_index ) );
  }
 }

/*--------------------------------------------------------------------------*/

 template< class T >
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

   if( index < Inf< Index >() )
    return( std::make_pair( group_index , index ) );
  }
  return( std::make_pair( Inf< Index >() , Inf< Index >() ) );
 }

/*--------------------------------------------------------------------------*/

 template< class T >
 static std::pair< Index , Index > get_static_element_index( T * t ) {

  const auto block = t->get_Block();

  if( ! block )
   return( std::make_pair( Inf< Index >() , Inf< Index >() ) );

  if constexpr( std::is_base_of_v< Constraint , T > )
   return( get_element_index( t , block->get_static_constraints() , true ) );
  else if constexpr( std::is_base_of_v< Variable , T > )
   return( get_element_index( t , block->get_static_variables() , true ) );
  else
   return( std::make_pair( Inf< Index >() , Inf< Index >() ) );
 }

/*--------------------------------------------------------------------------*/

 template< class T >
 static std::pair< Index , Index > get_dynamic_element_index( T * t ) {

  const auto block = t->get_Block();

  if( ! block )
   return( std::make_pair( Inf< Index >() , Inf< Index >() ) );

  if constexpr( std::is_base_of_v< Constraint , T > )
   return( get_element_index( t , block->get_dynamic_constraints() , false ) );
  else if constexpr( std::is_base_of_v< Variable , T > )
   return( get_element_index( t , block->get_dynamic_variables() , false ) );
  else
   return( std::make_pair( Inf< Index >() , Inf< Index >() ) );
 }

/*--------------------------------------------------------------------------*/

 /** This function returns information about the position of the given \p
  * element in its father Block. The given \p element must be either a pointer
  * to a Constraint or a pointer to a Variable. The information returned by
  * this function consists of the following:
  *
  * - The first element of the tuple indicates whether the given \p element
  *   belongs to a static group (i.e., it indicates whether it is a static
  *   Variable or static Constraint).
  *
  * - The second element of the tuple contains the index of the group to which
  *   the given \p element belongs.
  *
  * - The third element ot the tuple is the index of the given \p element in
  *   its group.
  *
  * @param element A pointer to a Constraint or to a Variable.
  *
  * @return A tuple containing the location of the given element in its father
  *         Block. */

 template< class T >
 static std::tuple< bool , Index , Index > get_element_index( T * element ) {
  auto index_pair = get_static_element_index( element );
  if( index_pair.first < Inf< Index >() )
   return( std::make_tuple( true , index_pair.first , index_pair.second ) );
  else {
   index_pair = get_dynamic_element_index( element );
   return( std::make_tuple( false , index_pair.first , index_pair.second ) );
  }
 }

/*--------------------------------------------------------------------------*/

 /// returns the index of the given Block in the list of sub-Block of its father
 /** This function returns the index of the given Block in the list of
  * sub-Block of its father (if any). If the given Block has no father Block,
  * Inf< Index >() is returned.
  *
  * @param block a A pointer to a Block.
  *
  * @return The index of the given Block in the list of sub-Block of its
  *         father. If the given Block has no father, Inf< Index >() is
  *         returned. */

 static inline Index get_block_index( const Block * block ) {
  auto father = block->get_f_Block();
  if( father ) {
   const auto & nb = father->get_nested_Blocks();
   const auto nbit = std::find( nb.begin() , nb.end() , block );
   if( nbit != nb.end() )
    return( std::distance( nb.begin() , nbit ) );
  }
  return( Inf< Index >() );
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
   return( get_static_element< T , T >( group , index ) );
  else
   return( get_dynamic_element< T , T >( group , index ) );
 }

/*--------------------------------------------------------------------------*/

 /// returns a pointer to the Constraint identified by the given \p id
 /** Returns a pointer to the Constraint identified by the given \p id in the
  * given \p block. If no Constraint with the given \p id is found, nullptr is
  * returned.
  *
  * @param block The Block to which the desired Constraint belongs.
  *
  * @param id The Block::ConstraintID identifying the Constraint in the given
  *        \p block.
  *
  * @return If there is a Constraint with the given \p id in the given \p
  *         Block, then a pointer to this Constraint is returned. Otherwise,
  *         nullptr is returned.
  */
 static inline Constraint * get_Constraint( const Block * const block ,
                                            const Block::ConstraintID id ) {
  const auto & static_constraints = block->get_static_constraints();
  const auto num_static_groups = static_constraints.size();
  auto group_index = id.first;
  auto constraint_index = id.second;

  if( group_index < num_static_groups ) {
   // A static Constraint
   auto any_group = static_constraints[ group_index ];
   return( get_static_element< Constraint , Constraint_Derived_Classes >
    ( any_group , constraint_index ) );
  }
  else {
   // A dynamic Constraint
   group_index = id.first - num_static_groups;
   const auto & dynamic_constraints = block->get_dynamic_constraints();

   if( group_index >= dynamic_constraints.size() )
    throw( std::logic_error( "get_Constraint: invalid dynamic Constraint group "
                             "index: " + std::to_string( group_index ) ) );

   auto any_group = dynamic_constraints[ group_index ];

   return( get_dynamic_element< Constraint , Constraint_Derived_Classes >
    ( any_group , constraint_index ) );
  }
 }

/*--------------------------------------------------------------------------*/

 template< class P1 , class P2 , class T , class... Rest >
 static void fill_static_ComputeConfig
 ( boost::any & group , const P1 group_index ,
   std::vector< ComputeConfig * > & configs ,
   std::vector< std::pair< P1 , P2 > > & ids ) {

  Index constraint_index = 0;
  bool group_found = un_any_static
   ( group , [ & ]( T & constraint ) {
              auto config = constraint.get_ComputeConfig();
              if( config ) {
               configs.push_back( config );
               ids.push_back( std::make_pair( group_index ,
                                              P2( constraint_index ) ) );
              }
              ++constraint_index;
             } , un_any_type< T >() );
  if( group_found )
   return;
  else if constexpr( sizeof...(Rest) != 0 )
   fill_static_ComputeConfig< P1 , P2 , Rest... >( group , group_index ,
                                                   configs , ids );
 }

/*--------------------------------------------------------------------------*/

 template< class P1 , class P2 , class T , class... Rest >
 static void fill_dynamic_ComputeConfig
 ( boost::any & group , const P1 group_index ,
   std::vector< ComputeConfig * > & configs ,
   std::vector< std::pair< P1 , P2 > > & ids ) {

  Index constraint_index = 0;
  bool group_found = un_any_dynamic
   ( group , [ & ]( T & constraint ) {
              auto config = constraint.get_ComputeConfig();
              if( config ) {
               configs.push_back( config );
               ids.push_back( std::make_pair( group_index ,
                                              P2( constraint_index ) ) );
              }
              ++constraint_index;
             } , un_any_type< T >() );
  if( group_found )
   return;
  else if constexpr( sizeof...(Rest) != 0 )
   fill_dynamic_ComputeConfig< P1 , P2 , Rest... >( group , group_index ,
                                                    configs , ids );
 }

/*--------------------------------------------------------------------------*/

 /// get the ComputeConfig of the Constraint of the given Block
 /** This method scans all Constraint of the given \p block and adds the
  * (non-default) ComputeConfig of the Constraint to \p configs and the
  * ConstraintID of those Constraint to \p ids. The correspondence between \p
  * configs and \p ids is positional: the i-th ConstraintID in \p ids
  * identifies the Constraint whose ComputeConfig is the i-th element of \p
  * configs. It is important to notice that both \p configs and \p ids are not
  * cleared before the pointers to the ComputeConfig are obtained. That is,
  * this function preserves the initial contents of the given vectors and may
  * only add elements to them.
  *
  * @param block A pointer to a Block.
  *
  * @param configs A vector of pointers to ComputeConfig. All ComputeConfig
  *        extracted from the Constraint of the given Block will be added to
  *        this vector.
  *
  * @param ids The ConstraintID of the Constraint whose ComputeConfig were
  *        retrieved will be added to this vector. */

 template< class P1 , class P2 >
 static void fill_ComputeConfig_Constraint
 ( Block * block , std::vector< ComputeConfig * > & configs ,
   std::vector< std::pair< P1 , P2 > > & ids ) {

  if( ! block )
   return;

  auto convert_index_type =
   []( Index index ) -> P1 {
    if constexpr( std::is_same_v< P1 , std::string > )
     return( std::to_string( index ) );
    else
     return( index );
   };

  // Static Constraint
  auto static_constraints = block->get_static_constraints();
  for( Index group_index = 0 ; group_index < static_constraints.size() ;
       ++group_index ) {
   fill_static_ComputeConfig< P1 , P2 , Constraint_Derived_Classes >
    ( static_constraints[ group_index ] , convert_index_type( group_index ) ,
      configs , ids );
  }

  const auto group_index_offset = static_constraints.size();

  // Dynamic Constraint
  auto dynamic_constraints = block->get_dynamic_constraints();
  for( Index group_index = 0 ; group_index < dynamic_constraints.size() ;
       ++group_index ) {
   fill_dynamic_ComputeConfig< P1 , P2 , Constraint_Derived_Classes >
    ( dynamic_constraints[ group_index ] ,
      convert_index_type( group_index + group_index_offset ) , configs , ids );
  }
 }

/*--------------------------------------------------------------------------*/

 /// returns the indices of the Constraint that are not satisfied
 /** The given \p group must contain a group of Constraint (static or dynamic,
  * according to \p static_constraints). This function returns a vector
  * containing the indices of the Constraint in \p group that are not
  * satisfied at the current solution. If the given \p group is not a valid
  * group of static or dynamic Constraint, then this function returns an empty
  * vector.
  *
  * @param group A boost::any containing a group of Constraint.
  *
  * @param static_constraints If it is true, then the given \p group contains
  *        a group of static Constraint. Otherwise, it indicates that \p group
  *        contains a group of dynamic Constraint. */

 template< class T , class... Rest >
 static std::vector< Index > get_infeasibility
 ( boost::any & group , const bool static_constraints ) {

  std::vector< Index > indices;
  Index constraint_index = 0;
  const auto fill_indices =
   [ &indices , &constraint_index ]( T & constraint ) {
    constraint.compute();
    if( ! constraint.feasible() )
     indices.push_back( constraint_index );
    ++constraint_index;
   };
  bool group_found;
  if( static_constraints )
   group_found = un_any_static( group , fill_indices , un_any_type< T >() );
  else
   group_found = un_any_dynamic( group , fill_indices , un_any_type< T >() );
  if( group_found )
   return( indices );
  if constexpr( sizeof...(Rest) != 0 )
   return( get_infeasibility< Rest... >( group , static_constraints ) );
  return {};
 }

/*--------------------------------------------------------------------------*/

 /// returns the static or dynamic Constraint of \p block
 /** This function returns a vector of boost::any containing the static or
  * dynamic Constraint of \p block. If \p static_constraints is true, then the
  * static Constraint are returned. Otherwise, the dynamic Constraint are
  * returned.
  *
  * @param block A pointer to a Block whose Constraint will be checked.
  *
  * @param static_constraints If it is true, then the static Constraint of the
  *        given Block are returned. Otherwise, the dynamic Constraint are
  *        returned.
  *
  * @return A vector of boost::any containing the static or dynamic Constraint
  *         of the given Block. */

 static auto & get_constraints( const Block * block ,
                                const bool static_constraints ) {
  if( static_constraints )
   return( const_cast< Vec_any & >( block->get_static_constraints() ) );
  return( const_cast< Vec_any & >( block->get_dynamic_constraints() ) );
 }

/*--------------------------------------------------------------------------*/

 /// displays all Constraint of \p block that are not satisfied
 /** This function displays the indices of the Constraint of \p block
  * (ignoring those of its sub-Blocks) that are not satisfied at the current
  * solution (given by the values of the Variable of \p block). The parameter
  * \p static_constraints indicates which constraints should be checked
  * (static or dynamic ones). If a Constraint is not satisfied at the current
  * solution, its index together with the name of the group to which this
  * Constraint belongs and the name of \p block (if not empty) are
  * displayed. If the name of \p block (see Block::name()) is empty, then the
  * name of the class of the \p block is displayed instead (see
  * Block::classname()).
  *
  * @param block A pointer to a Block whose Constraint will be checked.
  *
  * @param static_constraints If it is true, then the static Constraint of the
  *        given Block are checked. Otherwise, the dynamic Constraint are
  *        checked.
  *
  * @param stream An stream to which the description of the unsatisfied
  *        Constraint will be output. */

 static inline void show_infeasibility( const Block * block ,
                                        const bool static_constraints ,
                                        std::ostream & stream = std::cout ) {
  std::string constraint_type = static_constraints ? "static" : "dynamic";
  std::string block_name = block->name();
  if( block_name.empty() )
   block_name = block->classname();
  Index i = 0;
  for( auto & group : get_constraints( block , static_constraints ) ) {
   auto indices = get_infeasibility< Constraint_Derived_Classes >
    ( group , static_constraints );
   if( ! indices.empty() ) {
    const auto & group_name = block->get_s_const_name( i );
    stream << "Unsatisfied " << constraint_type << " constraints in '"
           << group_name << "' of Block '" << block_name << "': ";
    bool add_comma = false;
    for( const auto & index : indices ) {
     if( add_comma ) stream << ", ";
     stream << index;
     add_comma = true;
    }
    stream << std::endl;
   }
   ++i;
  }
 }

/*--------------------------------------------------------------------------*/

 /// displays all Constraint of \p block that are not satisfied
 /** This function displays the indices of the Constraint of \p block (and
  * those of its sub-Blocks, recursively) that are not satisfied at the
  * current solution (given by the values of the Variable of \p block). If a
  * Constraint is not satisfied at the current solution, its index together
  * with the name of the group to which this Constraint belongs and the name
  * (if not empty) of the Block in which it is defined are displayed. If the
  * name of the Block (see Block::name()) in which the Constraint is defined
  * is empty, then the name of the class of the Block is displayed instead
  * (see Block::classname()).
  *
  * @param block A pointer to a Block whose Constraint will be checked.
  *
  * @param stream An stream to which the description of the unsatisfied
  *        Constraint will be output. */

 static inline void show_infeasibility( const Block * block ,
                                        std::ostream & stream = std::cout ) {
  show_infeasibility( block , true , stream );
  show_infeasibility( block , false , stream );
  for( auto sub_block : block->get_nested_Blocks() )
   show_infeasibility( sub_block , stream );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it::inspection )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#undef Constraint_Derived_Classes
#undef Variable_Derived_Classes

#endif  /* BlockInspection.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File BlockInspection.h ------------------------*/
/*--------------------------------------------------------------------------*/
