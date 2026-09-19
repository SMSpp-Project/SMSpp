/*--------------------------------------------------------------------------*/
/*------------------------- File BlockInspection.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the functions that look into a Block for one of its
 * Variable or Constraint, and for the ComputeConfig of them, without knowing
 * which concrete class they are.
 *
 * The Block hands its Variable and its Constraint over as groups, each of
 * which says the type of its elements and their shape, so that the functions
 * here are a matter of asking the right group [see BaseGroup]; what they add
 * is the numbering of the elements INSIDE a group, which is the one a
 * ConstraintID carries and which therefore cannot change: the elements of a
 * group that is one array are numbered in storage order, while those of a
 * group made of many arrays are numbered cell by cell, i.e., the i-th
 * element of the c-th cell of a group of n cells is the ( c + i * n )-th.
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Rafael Durbano Lobato, Donato Meoli
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
#include "FRealObjective.h"
#include "FRowConstraint.h"
#include "OneVarConstraint.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace SMSpp_di_unipi_it::inspection
{
/*--------------------------------------------------------------------------*/

 using Index = Block::Index;

/*--------------------------------------------------------------------------*/
/*------------------- NUMBERING THE ELEMENTS OF A GROUP --------------------*/
/*--------------------------------------------------------------------------*/
 /// calls f< T >() for each concrete type a group of C can hold, until true
 /** Calls f with a pointer tag of each concrete type that a group of
  * elements deriving from C can hold, in a fixed order, and stops at the
  * first call that returns true; this is how a caller that is given a base
  * class finds out which type the elements of a group really are. */

 template< class C , class F >
 static bool for_each_concrete( F f )
 {
  if constexpr( std::is_base_of_v< Variable , C > )
   return( f( static_cast< ColVariable * >( nullptr ) ) );
  else
   return( f( static_cast< FRowConstraint * >( nullptr ) ) ||
	   f( static_cast< BoxConstraint * >( nullptr ) ) ||
	   f( static_cast< LB0Constraint * >( nullptr ) ) ||
	   f( static_cast< UB0Constraint * >( nullptr ) ) ||
	   f( static_cast< LBConstraint * >( nullptr ) ) ||
	   f( static_cast< UBConstraint * >( nullptr ) ) ||
	   f( static_cast< NNConstraint * >( nullptr ) ) ||
	   f( static_cast< NPConstraint * >( nullptr ) ) ||
	   f( static_cast< ZOConstraint * >( nullptr ) ) );
  }

/*--------------------------------------------------------------------------*/

 /// the index of an element inside its group, Inf< Index >() if not there
 /** The index the functions here give an element of \p group, which is the
  * one a ConstraintID carries: the position in storage order for a group
  * that is one array, or one collection of them, and c + i * n for the i-th
  * element of the c-th cell of a group of n cells whose cells are vectors,
  * which is how such a group has always been numbered here. */

 template< class C >
 static Index index_in_group( const BaseGroup & group , const C * element )
 {
  using T = std::remove_const_t< C >;
  Index found = Inf< Index >();

  if( group.get_layout() != BaseGroup::eJagged ) {
   Index i = 0;
   group.for_each_as< T >( [ & ]( T & candidate ) {
     if( & candidate == element )
      found = i;
     ++i;
     } );
   return( found );
   }

  const Index cells = group.get_num_cells();

  for_each_concrete< T >( [ & ]( auto * tag ) {
    using X = std::remove_pointer_t< decltype( tag ) >;
    if constexpr( std::is_base_of_v< T , X > )
     return( group.for_each_cell_as< X >( [ & ]( Index c , auto & cell ) {
       Index i = 0;
       for( auto & item : cell ) {
	if( & group_element( item ) == element )
	 found = c + i * cells;
	++i;
	}
       } ) );
    else
     return( false );
    } );

  return( found );
  }

/*--------------------------------------------------------------------------*/
 /// the element of \p group at \p index, nullptr if there is none
 /** The inverse of index_in_group(): the two number the elements of a group
  * in the same way. */

 template< class C >
 static C * element_at( const BaseGroup & group , Index index )
 {
  using T = std::remove_const_t< C >;

  if( ! group.elements_are< T >() )
   return( nullptr );

  if( group.get_layout() != BaseGroup::eJagged )
   return( group.get_as< T >( index ) );

  const Index cells = group.get_num_cells();
  if( ! cells )
   return( nullptr );

  C * found = nullptr;
  const Index wanted_cell = index % cells;
  const Index wanted = index / cells;

  for_each_concrete< T >( [ & ]( auto * tag ) {
    using X = std::remove_pointer_t< decltype( tag ) >;
    if constexpr( std::is_base_of_v< T , X > )
     return( group.for_each_cell_as< X >( [ & ]( Index c , auto & cell ) {
       if( ( c != wanted_cell ) || ( wanted >= cell.size() ) )
	return;
       found = & group_element( * std::next( cell.begin() , wanted ) );
       } ) );
    else
     return( false );
    } );

  return( found );
  }

/*--------------------------------------------------------------------------*/
/*--------------------- LOOKING INTO A Block -------------------------------*/
/*--------------------------------------------------------------------------*/
 /// returns the index of the given Block among the sub-Block of its father
 /** Returns the index of the given Block in the list of sub-Block of its
  * father, or Inf< Index >() if it has no father. */

 static Index get_block_index( const Block * block )
 {
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
 /// returns the groups of \p block of the kind T, static or dynamic
 /** The vector of groups of Variable of \p block if T is a Variable, and the
  * one of its Constraint otherwise, the static ones if \p is_static. */

 template< class T >
 static const Vec_Group & get_groups( const Block * block , bool is_static )
 {
  if constexpr( std::is_base_of_v< Variable , T > )
   return( is_static ? block->get_static_variable_groups()
	             : block->get_dynamic_variable_groups() );
  else
   return( is_static ? block->get_static_constraint_groups()
	             : block->get_dynamic_constraint_groups() );
  }

/*--------------------------------------------------------------------------*/
 /// says how many groups of a kind a Block has, and what they are named
 /** The sentence a caller puts in the message when it is given the index, or
  * the name, of a group that is not there. */

 inline static std::string describe_groups( const Block * block ,
					    bool is_static ,
					    bool is_variable )
 {
  const Vec_Group & groups = is_variable
   ? ( is_static ? block->get_static_variable_groups()
	         : block->get_dynamic_variable_groups() )
   : ( is_static ? block->get_static_constraint_groups()
	         : block->get_dynamic_constraint_groups() );

  std::string names;
  for( const auto & group : groups ) {
   if( ! names.empty() )
    names += " , ";
   names += group ? ( group->get_name().empty() ? std::string( "<unnamed>" )
		                                : group->get_name() )
	          : std::string( "<empty>" );
   }

  const auto n = groups.size();

  return( std::string( is_static ? "static " : "dynamic " ) +
	  ( is_variable ? "Variable" : "Constraint" ) + " groups of the Block (" +
	  std::to_string( n ) + " group" + ( n == 1 ? "" : "s" ) +
	  ( n ? ": " + names : std::string() ) + ")" );
  }

/*--------------------------------------------------------------------------*/
 /// returns the element of \p block at the given position, nullptr if none
 /** Returns the element of type \p T that sits at \p element_index in the
  * \p group_index-th group of \p block, static or dynamic as \p is_static
  * says, or nullptr if the group is not there, is of another type, or is not
  * that long. */

 template< class T >
 static T * get_element( const Block * block , bool is_static ,
                         Index group_index , Index element_index )
 {
  if constexpr( ( ! std::is_base_of_v< Variable , T > ) &&
		( ! std::is_base_of_v< Constraint , T > ) )
   return( nullptr );   // the elements are neither Variable nor Constraint
  else {
   const auto & groups = get_groups< T >( block , is_static );

   if( group_index >= groups.size() )
    throw( std::invalid_argument( "inspection::get_element: invalid group "
				  "index " + std::to_string( group_index ) ) );

   const auto & group = groups[ group_index ];
   if( ! group )
    return( nullptr );

   return( element_at< T >( *group , element_index ) );
   }
  }

/*--------------------------------------------------------------------------*/
 /// returns how many elements of type \p T the given group of \p block holds

 template< class T >
 static Index get_element_size( const Block * block , bool is_static ,
                                Index group_index )
 {
  if constexpr( ( ! std::is_base_of_v< Variable , T > ) &&
		( ! std::is_base_of_v< Constraint , T > ) )
   return( Inf< Index >() );  // neither Variable nor Constraint
  else {
   const auto & groups = get_groups< T >( block , is_static );

   if( group_index >= groups.size() )
    throw( std::invalid_argument( "inspection::get_element_size: invalid "
				  "group index " +
				  std::to_string( group_index ) ) );

   const auto & group = groups[ group_index ];
   if( ! group )
    return( Inf< Index >() );

   // an empty group holds no element of any type, and says nothing about
   // the type it will hold: it is of length zero whatever is asked of it
   if( ! group->get_num_elements() )
    return( 0 );

   if( ! group->template elements_are< T >() )
    return( Inf< Index >() );

   return( group->get_num_elements() );
   }
  }

/*--------------------------------------------------------------------------*/
 /// where the given element sits in the Block it belongs to
 /** Returns whether \p element is static, the index of the group it belongs
  * to and its index inside that group; the three are Inf< Index >() if it
  * belongs to no group of its Block, or to no Block at all. */

 template< class T >
 static std::tuple< bool , Index , Index > get_element_index( T * element )
 {
  if constexpr( ( ! std::is_base_of_v< Variable , T > ) &&
		( ! std::is_base_of_v< Constraint , T > ) )
   return( std::make_tuple( true , Inf< Index >() , Inf< Index >() ) );
  else {
  const auto block = element->get_Block();
  if( ! block )
   return( std::make_tuple( true , Inf< Index >() , Inf< Index >() ) );

  for( const bool is_static : { true , false } ) {
   const auto & groups = get_groups< T >( block , is_static );

   for( Index g = 0 ; g < groups.size() ; ++g ) {
    if( ! groups[ g ] )
     continue;
    const auto i = index_in_group< T >( *groups[ g ] , element );
    if( i < Inf< Index >() )
     return( std::make_tuple( is_static , g , i ) );
    }
   }

  return( std::make_tuple( true , Inf< Index >() , Inf< Index >() ) );
  }
  }

/*--------------------------------------------------------------------------*/
 /// returns the Constraint of \p block with the given id, nullptr if none
 /** The groups of Constraint are numbered with the static ones first and the
  * dynamic ones after them, which is what a Block::ConstraintID says. */

 static Constraint * get_Constraint( const Block * const block ,
                                     const Block::ConstraintID id )
 {
  const auto & statics = block->get_static_constraint_groups();
  auto group_index = id.first;

  const Vec_Group * groups = & statics;
  if( group_index >= statics.size() ) {
   group_index -= statics.size();
   groups = & block->get_dynamic_constraint_groups();

   if( group_index >= groups->size() )
    throw( std::logic_error( "inspection::get_Constraint: invalid dynamic "
			     "Constraint group index: " +
			     std::to_string( group_index ) ) );
   }

  const auto & group = ( *groups )[ group_index ];
  if( ! group )
   return( nullptr );

  // the concrete type of the elements is asked of the group, once for all of
  // them, and the numbering is the same whichever it is
  Constraint * found = nullptr;

  auto pick = [ & found , index = id.second ]( const BaseGroup & g ) {
   found = element_at< FRowConstraint >( g , index );
   if( found )
    return;
   found = element_at< BoxConstraint >( g , index );
   if( found )
    return;
   found = element_at< LB0Constraint >( g , index );
   if( found )
    return;
   found = element_at< UB0Constraint >( g , index );
   if( found )
    return;
   found = element_at< LBConstraint >( g , index );
   if( found )
    return;
   found = element_at< UBConstraint >( g , index );
   if( found )
    return;
   found = element_at< NNConstraint >( g , index );
   if( found )
    return;
   found = element_at< NPConstraint >( g , index );
   if( found )
    return;
   found = element_at< ZOConstraint >( g , index );
   };

  pick( *group );

  return( found );
  }

/*--------------------------------------------------------------------------*/
 /// collects the ComputeConfig of the Constraint of \p block
 /** Adds to \p configs the (non-default) ComputeConfig of the Constraint of
  * \p block, and to \p ids the identifier of each of them, the two being
  * paired by position. The groups are numbered with the static ones first,
  * and the elements inside a group as get_element() numbers them, so that
  * the identifiers can be given back to get_Constraint(). */

 template< class P1 , class P2 >
 static void fill_ComputeConfig_Constraint( Block * block ,
			      std::vector< ComputeConfig * > & configs ,
                              std::vector< std::pair< P1 , P2 > > & ids )
 {
  if( ! block )
   return;

  auto index_of = []( Index index ) -> P1 {
   if constexpr( std::is_same_v< P1 , std::string > )
    return( std::to_string( index ) );
   else
    return( index );
   };

  auto take = [ & ]( const BaseGroup & group , Index group_index ) {
   auto one = [ & ]( Constraint & cnst , Index i ) {
    if( auto config = cnst.get_ComputeConfig() ) {
     configs.push_back( config );
     ids.push_back( std::make_pair( index_of( group_index ) , P2( i ) ) );
     }
    };

   if( group.get_layout() != BaseGroup::eJagged ) {
    Index i = 0;
    group.for_each( [ & ]( Constraint & cnst ) { one( cnst , i++ ); } );
    return;
    }

   // the cells of a jagged group are walked one by one, its elements being
   // numbered cell by cell; which concrete type they are is asked of the
   // group once, as everywhere else
   const Index cells = group.get_num_cells();

   auto by_cell = [ & ]( auto * tag ) {
    using T = std::remove_pointer_t< decltype( tag ) >;
    return( group.for_each_cell_as< T >( [ & ]( Index c , auto & cell ) {
      Index i = 0;
      for( auto & item : cell )
       one( group_element( item ) , c + ( i++ ) * cells );
      } ) );
    };

   ( by_cell( static_cast< FRowConstraint * >( nullptr ) ) ||
     by_cell( static_cast< BoxConstraint * >( nullptr ) ) ||
     by_cell( static_cast< LB0Constraint * >( nullptr ) ) ||
     by_cell( static_cast< UB0Constraint * >( nullptr ) ) ||
     by_cell( static_cast< LBConstraint * >( nullptr ) ) ||
     by_cell( static_cast< UBConstraint * >( nullptr ) ) ||
     by_cell( static_cast< NNConstraint * >( nullptr ) ) ||
     by_cell( static_cast< NPConstraint * >( nullptr ) ) ||
     by_cell( static_cast< ZOConstraint * >( nullptr ) ) );
   };

  const auto & statics = block->get_static_constraint_groups();
  for( Index g = 0 ; g < statics.size() ; ++g )
   if( statics[ g ] )
    take( *statics[ g ] , g );

  const auto & dynamics = block->get_dynamic_constraint_groups();
  for( Index g = 0 ; g < dynamics.size() ; ++g )
   if( dynamics[ g ] )
    take( *dynamics[ g ] , g + statics.size() );
  }

/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it::inspection )

/*--------------------------------------------------------------------------*/

#endif  /* BlockInspection.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File BlockInspection.h ------------------------*/
/*--------------------------------------------------------------------------*/
