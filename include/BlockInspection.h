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
 * ConstraintID carries: the elements of a group are numbered in storage
 * order, cell by cell, so that the i-th element of the c-th cell comes after
 * all the elements of the cells before it, whatever the cells hold.
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
  * one a ConstraintID carries: the position in storage order, i.e., the
  * position of the element inside its cell plus the sizes of the cells
  * before it, whatever the cells hold. */

 template< class C >
 static Index index_in_group( const BaseGroup & group , const C * element )
 {
  using T = std::remove_const_t< C >;
  Index found = Inf< Index >();

  // one array of elements of type exactly T is one run, where the position
  // of the element is a subtraction; anything else comes one by one
  if( group.get_layout() == BaseGroup::eContiguous ) {
   Index base = 0;
   group.for_each_run_as< T >( [ & ]( T * first , Index n ) {
     if( ( found == Inf< Index >() ) &&
	 std::less_equal<>()( first , element ) &&
	 std::less<>()( element , first + n ) )
      found = base + Index( element - first );
     base += n;
     } );
   return( found );
   }

  if( group.get_layout() != BaseGroup::eJagged ) {
   Index i = 0;
   group.for_each_as< T >( [ & ]( T & candidate ) {
     if( & candidate == element )
      found = i;
     ++i;
     } );
   return( found );
   }

  Index base = 0;

  for_each_concrete< T >( [ & ]( auto * tag ) {
    using X = std::remove_pointer_t< decltype( tag ) >;
    if constexpr( std::is_base_of_v< T , X > )
     return( group.for_each_cell_as< X >( [ & ]( Index c , auto & cell ) {
       Index i = 0;
       for( auto & item : cell ) {
	if( & group_element( item ) == element )
	 found = base + i;
	++i;
	}
       base += i;
       } ) );
    else
     return( false );
    } );

  return( found );
  }

/*--------------------------------------------------------------------------*/
 /// how the pieces of the name of an element are spelled out
 /** The name of an element is made of the name of its group and of a list of
  * indices; which characters set the indices apart, and which stand around
  * the index of a group that has no name, is what this says. The default is
  * the readable one, "x[ 2 ]" and "<2>[ 1 ][ 1 ]"; a caller writing a name
  * into a file whose format takes neither brackets nor spaces, as the LP one
  * does not, asks instead for { "_" , "" , "v" , "" } and reads "x_2" and
  * "v2_1_1". Of course, with a marker that is a letter a nameless group of
  * index 2 has the name a group truly called "v2" would have; with the
  * default one it cannot happen, an angle bracket being no part of a name.
  * A caller that names two kinds of thing into the same file, as a model
  * file names columns and rows, gives each kind its own marker: the index
  * of a group of columns and that of a group of rows are the same numbers,
  * so one marker for both would put the same name on two things. */

 struct name_format {
  const char * open = "[ ";     ///< opens the index of a cell
  const char * close = " ]";    ///< closes the index of a cell
  const char * gopen = "<";     ///< opens the index of a group with no name
  const char * gclose = ">";    ///< closes the index of a group with no name
  };

/*--------------------------------------------------------------------------*/
 /// the name \p group gives to the element of it at \p cell and \p position
 /** The name is that of the group, or its index in the Block between angle
  * brackets when it has none, followed by the multi-index of the cell, one
  * pair of square brackets per dimension of the grid, and by the position of
  * the element inside its cell when the cells are collections. A group that
  * is one array has one cell per element and no position, so the name of its
  * i-th element is just "<name>[ i ]". \p fmt says which characters play the
  * part of the brackets [see name_format]. */

 static std::string name_of_cell( const BaseGroup & group , Index cell ,
				  Index position = Inf< Index >() ,
				  name_format fmt = {} )
 {
  std::string name = group.get_name();
  if( name.empty() )
   name = fmt.gopen + std::to_string( group.get_index() ) + fmt.gclose;

  const auto g = group.get_grid();
  std::array< Index , BaseGroup::max_rank > idx;
  g.get_multi_index( cell , idx.data() );

  for( unsigned char d = 0 ; d < g.rank ; ++d )
   name += fmt.open + std::to_string( idx[ d ] ) + fmt.close;

  if( position < Inf< Index >() )
   name += fmt.open + std::to_string( position ) + fmt.close;

  return( name );
  }

/*--------------------------------------------------------------------------*/
 /// walks \p group giving each element to \p f together with its name
 /** Calls f( name , element ) for each element of \p group, in storage
  * order, with the name name_of_cell() gives it. Answers false, without
  * calling \p f, if the elements of the group are not T. The shape of the
  * grid is read once for the whole group and not once per element. */

 template< class T , class F >
 static bool for_each_named_as( const BaseGroup & group , F f ,
				name_format fmt = {} )
 {
  std::string base = group.get_name();
  if( base.empty() )
   base = fmt.gopen + std::to_string( group.get_index() ) + fmt.gclose;

  const auto g = group.get_grid();

  auto named = [ & base , & g , fmt ]( Index cell , Index position ) {
   std::array< Index , BaseGroup::max_rank > idx;
   g.get_multi_index( cell , idx.data() );
   std::string name = base;
   for( unsigned char d = 0 ; d < g.rank ; ++d )
    name += fmt.open + std::to_string( idx[ d ] ) + fmt.close;
   if( position < Inf< Index >() )
    name += fmt.open + std::to_string( position ) + fmt.close;
   return( name );
   };

  if( group.get_layout() == BaseGroup::eContiguous ) {
   Index i = 0;
   return( group.for_each_as< T >( [ & ]( T & element ) {
     f( named( i , Inf< Index >() ) , element );
     ++i;
     } ) );
   }

  bool done = false;
  for_each_concrete< T >( [ & ]( auto * tag ) {
    using X = std::remove_pointer_t< decltype( tag ) >;
    if constexpr( std::is_base_of_v< T , X > ) {
     if( group.for_each_cell_as< X >( [ & ]( Index c , auto & cell ) {
       Index i = 0;
       for( auto & item : cell )
	f( named( c , i++ ) , static_cast< T & >( group_element( item ) ) );
       } ) ) {
      done = true;
      return( true );
      }
     return( false );
     }
    else
     return( false );
    } );

  return( done );
  }

/*--------------------------------------------------------------------------*/
 /// as for_each_named_as(), trying each of T... until one of them fits
 /** Calls for_each_named_as() for each type of the list in turn, stopping at
  * the first that the elements of \p group are; answers false if they are
  * none of them, having called \p f no times. */

 template< class... T , class F >
 static bool for_each_named_as_any_of( const BaseGroup & group , F f )
 {
  return( ( for_each_named_as< T >( group , f ) || ... ) );
  }

/*--------------------------------------------------------------------------*/
 /// the name \p group gives to \p element, "" if the element is not in it
 /** Walks \p group looking for \p element and names it as name_of_cell()
  * does. This is what it takes to say which Variable a row of the model is
  * written on, which is the thing a model printed by hand needs and the
  * address of the element alone cannot give. */

 template< class C >
 static std::string name_in_group( const BaseGroup & group ,
				   const C * element )
 {
  using T = std::remove_const_t< C >;
  std::string found;

  if( group.get_layout() == BaseGroup::eContiguous ) {
   Index i = 0;
   group.for_each_as< T >( [ & ]( T & candidate ) {
     if( & candidate == element )
      found = name_of_cell( group , i );
     ++i;
     } );
   return( found );
   }

  for_each_concrete< T >( [ & ]( auto * tag ) {
    using X = std::remove_pointer_t< decltype( tag ) >;
    if constexpr( std::is_base_of_v< T , X > )
     return( group.for_each_cell_as< X >( [ & ]( Index c , auto & cell ) {
       Index i = 0;
       for( auto & item : cell ) {
	if( & group_element( item ) == element )
	 found = name_of_cell( group , c , i );
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

  if( ! group.get_num_cells() )
   return( nullptr );

  C * found = nullptr;
  Index base = 0;

  for_each_concrete< T >( [ & ]( auto * tag ) {
    using X = std::remove_pointer_t< decltype( tag ) >;
    if constexpr( std::is_base_of_v< T , X > )
     return( group.for_each_cell_as< X >( [ & ]( Index c , auto & cell ) {
       const Index n = Index( cell.size() );
       if( ( ! found ) && ( index >= base ) && ( index < base + n ) )
	found = & group_element( * std::next( cell.begin() ,
					      index - base ) );
       base += n;
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
 /// the name \p block gives to \p element, "" if the element is not in it
 /** Looks for \p element in the four vectors of groups of \p block, static
  * first and then dynamic, and gives the name the group it is in gives it. */

 template< class C >
 static std::string name_of( const Block * block , const C * element )
 {
  using T = std::remove_const_t< C >;
  if( ( ! block ) || ( ! element ) )
   return( "" );

  for( bool is_static : { true , false } )
   for( const auto & group : get_groups< T >( block , is_static ) ) {
    if( ! group )
     continue;
    auto name = name_in_group< C >( *group , element );
    if( ! name.empty() )
     return( name );
    }

  return( "" );
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
  * belongs to no group of its Block, or to no Block at all. An element that
  * knows its group [see Variable::get_Group()] is looked for there only; one
  * that does not, as an element reached through a group of pointers, is
  * looked for in all the groups of its Block. */

 template< class T >
 static std::tuple< bool , Index , Index > get_element_index( T * element )
 {
  if constexpr( ( ! std::is_base_of_v< Variable , T > ) &&
		( ! std::is_base_of_v< Constraint , T > ) )
   return( std::make_tuple( true , Inf< Index >() , Inf< Index >() ) );
  else {
  if( const auto group = element->get_Group() ) {
   // a dynamic group is one of std::list, and no static group is
   const auto i = index_in_group< T >( *group , element );
   if( i < Inf< Index >() )
    return( std::make_tuple( ! group->is_dynamic() , group->get_index() ,
			     i ) );
   }

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
