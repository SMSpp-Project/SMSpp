/*--------------------------------------------------------------------------*/
/*------------------------------ File Group.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the BaseGroup class, the abstract base class of the groups
 * of Variable and Constraint of a Block, and for the StaticGroup and
 * CellGroup classes, which implement it for the storage forms that a :Block
 * declares as its own members.
 *
 * A group is a homogeneous collection of Variable or Constraint: all its
 * elements have the same type, which is why it can be asked once, and not
 * once per element, whether they derive from any given base class. The
 * elements are laid out on a rectangular grid of any rank, rank 0 being a
 * single element and rank 1 an array, and each cell holds either one element
 * or a collection of them. The group may hold the elements themselves or
 * pointers to them.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __Group
 #define __Group  /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Constraint.h"
#include "Variable.h"

#include <boost/multi_array.hpp>

#include <array>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{
 class Block;  // forward definition of Block

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Group_CLASSES Classes in Group.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*-------------------------- AUXILIARY STUFF -------------------------------*/
/*--------------------------------------------------------------------------*/
/// the element of a group, whether the group holds it or a pointer to it

template< class T >
T & group_element( T & element ) { return( element ); }

template< class T >
T & group_element( T * element ) { return( *element ); }

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BaseGroup -------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a group of Variable or Constraint of a Block
/** BaseGroup is the type-agnostic face of a group: it says which Block the
 * group belongs to, which index it has there and under which name, what its
 * shape is, and it gives access to the elements as Variable or Constraint,
 * i.e., in terms of the base classes alone.
 *
 * The elements of a group all have the same type. This is what makes the
 * question "do these derive from RowConstraint?" answerable once for the
 * whole group, with elements_are(), rather than once per element.
 *
 * The shape is a rectangular grid of get_rank() dimensions, get_size( d )
 * wide along the d-th one; rank 0 is a single cell. Each cell holds one
 * element in a contiguous group, a std::vector of them in a jagged one and a
 * std::list of them in a dynamic one, where elements can be added and
 * removed while the addresses of the others stay where they are. In each of
 * the three the storage may hold the elements or pointers to them.
 *
 * A group views the container that a :Block registered, and reads its
 * storage and its shape out of it at each access, so that it stays valid if
 * the container is resized. The type of the elements, the layout and the rank
 * are plain fields, so that for_each_as() and for_each_cell_as() reach the
 * storage with one indirect call and one switch per group and then run a loop
 * typed on the element, which the compiler can inline: this is the access to
 * use on hot paths. for_each() goes through a std::function per element and
 * is meant for the cold ones.
 *
 * THE ORDER IN WHICH THE ELEMENTS COME OUT IS PART OF THE CONTRACT. It is the
 * storage order: cell by cell along the grid, the last index running fastest,
 * and inside a cell the order of the collection. Callers pair the i-th
 * element of a group with the i-th entry of a vector of their own, and the
 * pairing has to hold between one walk and the next: the columns of a
 * :MILPSolver, the Lagrangian multipliers of a LagrangianDualSolver and the
 * re-synchronisation of a PrimalProximalHeur all rest on it. Whoever changes
 * the order of any of the forms breaks them, and tests_Group.cpp is there to
 * say so.
 *
 * A const group hands out modifiable elements, and this is meant: the group
 * is a view, and its own constness is that of the view, not that of the
 * elements, which belong to the :Block. Reading a Block and writing in its
 * Variable, as a Solver does when it writes a solution, is one const group
 * and elements that change. */

class BaseGroup {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/

 using Index = unsigned int;   ///< type for indices of elements and cells

 using c_Index = const Index;  ///< a const Index

 static constexpr unsigned char max_rank = 8;  ///< the largest rank

 /// what the elements of a group are
 enum kind_type {
  eVariable = 0 ,   ///< the elements derive from Variable
  eConstraint = 1   ///< the elements derive from Constraint
  };

 /// what a cell of the grid holds
 enum layout_type {
  eContiguous = 0 ,  ///< one element per cell
  eJagged = 1 ,      ///< a std::vector of elements per cell
  eDynamic = 2       ///< a std::list of elements per cell
  };

/*------------------------ CONSTRUCTOR AND DESTRUCTOR ----------------------*/

 /// destructor of BaseGroup, virtual for a base class

 virtual ~BaseGroup() = default;

/*----------------------- METHODS FOR THE IDENTITY -------------------------*/
 /** @name Where the group lives
 *  @{ */

 /// returns the Block the group belongs to

 [[nodiscard]] Block * get_Block( void ) const { return( f_Block ); }

/*--------------------------------------------------------------------------*/
 /// returns the index of the group among those of its kind in the Block

 [[nodiscard]] Index get_index( void ) const { return( f_index ); }

/*--------------------------------------------------------------------------*/
 /// returns the name of the group, the empty string if it has none

 [[nodiscard]] const std::string & get_name( void ) const {
  return( f_name );
  }

/*--------------------------------------------------------------------------*/
 /// returns the container the group views, as it was registered

 [[nodiscard]] void * get_container( void ) const { return( f_container ); }

/*--------------------------------------------------------------------------*/
 /// sets the name of the group

 void set_name( std::string name ) { f_name = std::move( name ); }

/*--------------------------------------------------------------------------*/
 /// sets the Block and the index, as the Block registers the group

 void set_Block( Block * block , Index index ) {
  f_Block = block;
  f_index = index;
  }

/*--------------------------------------------------------------------------*/
 /// says how the storage of the group is deleted
 /** A group is a view and deletes nothing of its own. Whoever builds it out
  * of a storage it is entitled to dispose of says here how that is done, the
  * container being of a type that the group does not know; delete_storage()
  * then does it. */

 void set_storage_deleter( std::function< void( void ) > deleter ) {
  f_deleter = std::move( deleter );
  }

/*--------------------------------------------------------------------------*/
 /// deletes the storage of the group, if it was said how
 /** Deletes the storage of the group with what set_storage_deleter() was
  * given, and forgets it, so that a second call does nothing. Returns true
  * if there was something to do. The group must not be used afterwards. */

 bool delete_storage( void ) {
  if( ! f_deleter )
   return( false );
  auto deleter = std::move( f_deleter );
  f_deleter = nullptr;
  deleter();
  return( true );
  }

/** @} ---------------------------------------------------------------------*/
/*------------------------- METHODS FOR THE SHAPE --------------------------*/
/** @name The shape of the group
 *  @{ */

 /// returns the number of dimensions of the grid of cells, 0 for a single one

 [[nodiscard]] unsigned char get_rank( void ) const { return( f_rank ); }

/*--------------------------------------------------------------------------*/
 /// returns the extent of the grid along the d-th dimension

 [[nodiscard]] Index get_size( unsigned char d ) const {
  if( d >= f_rank )
   return( 1 );
  std::array< Index , max_rank > size;
  f_view( f_container , size.data() );
  return( size[ d ] );
  }

/*--------------------------------------------------------------------------*/
 /// returns the number of cells of the grid, 1 for rank 0

 [[nodiscard]] Index get_num_cells( void ) const {
  return( get_storage().num_cells );
  }

/*--------------------------------------------------------------------------*/
 /// writes the indices of the c-th cell of the grid into \p index
 /** Writes into index[ 0 ] ... index[ get_rank() - 1 ] the position of the
  * c-th cell of the grid along each dimension, the cells being numbered in
  * storage order, so that an element can be named after where it sits rather
  * than after its position in the sequence: the 7-th cell of a 3 x 4 grid is
  * ( 1 , 3 ). For a group of rank 0 there is nothing to write. */

 void get_multi_index( Index c , Index * index ) const {
  get_grid().get_multi_index( c , index );
  }

/*--------------------------------------------------------------------------*/
 /// the shape of the grid of cells, read once
 /** The rank and the extents of the grid, read out of the container once, so
  * that a caller that has to name many cells does not pay a read for each of
  * them: get_grid() is the shape as it is now, get_multi_index() is the
  * indices of one cell in it. */

 struct grid {
  unsigned char rank;                 ///< the number of dimensions
  std::array< Index , max_rank > size; ///< the extent along each of them

  /// writes the indices of the c-th cell of this grid into \p index
  void get_multi_index( Index c , Index * index ) const {
   for( unsigned char d = rank ; d-- ; ) {
    index[ d ] = size[ d ] ? c % size[ d ] : 0;
    c = size[ d ] ? c / size[ d ] : 0;
    }
   }
  };

/*--------------------------------------------------------------------------*/
 /// returns the shape of the grid of cells

 [[nodiscard]] grid get_grid( void ) const {
  grid g;
  g.rank = f_rank;
  g.size.fill( 1 );
  if( f_rank )
   f_view( f_container , g.size.data() );
  return( g );
  }

/*--------------------------------------------------------------------------*/
 /// returns how many elements the group holds now
 /** Returns the number of elements of the group: get_num_cells() for a
  * contiguous group, the sum of the sizes of the cells otherwise, which for a
  * dynamic group changes as elements are added and removed. */

 [[nodiscard]] virtual Index get_num_elements( void ) const = 0;

/*--------------------------------------------------------------------------*/
 /// returns what a cell of the grid holds

 [[nodiscard]] layout_type get_layout( void ) const { return( f_layout ); }

/*--------------------------------------------------------------------------*/
 /// returns true if each cell holds a list of elements rather than one

 [[nodiscard]] bool is_dynamic( void ) const {
  return( f_layout == eDynamic );
  }

/*--------------------------------------------------------------------------*/
 /// returns true if a cell holds a collection of elements rather than one

 [[nodiscard]] bool cells_are_collections( void ) const {
  return( f_layout != eContiguous );
  }

/*--------------------------------------------------------------------------*/
 /// returns true if the storage holds pointers to the elements
 /** Returns true if the storage of the group holds pointers to elements that
  * live elsewhere, false if it holds the elements themselves. */

 [[nodiscard]] bool is_indirect( void ) const { return( f_indirect ); }

/*--------------------------------------------------------------------------*/
 /// returns whether the elements are Variable or Constraint

 [[nodiscard]] kind_type get_kind( void ) const { return( f_kind ); }

/*--------------------------------------------------------------------------*/
 /// returns the type of the elements, the same for all of them

 [[nodiscard]] std::type_index get_element_type( void ) const {
  return( f_type );
  }

/** @} ---------------------------------------------------------------------*/
/*----------------------- METHODS FOR THE ELEMENTS -------------------------*/
/** @name Reading the elements
 *  @{ */

 /// calls f() on each element of the group, in storage order
 /** Calls f() on each element of the group, in storage order, which for a
  * group whose cells are collections means cell by cell and, inside a cell,
  * in the order of the collection. The group must be one of Variable, which
  * get_kind() says. This goes through a std::function per element, and
  * for_each_as() is the method for hot paths. */

 virtual void for_each( const std::function< void( Variable & ) > & f )
  const = 0;

/*--------------------------------------------------------------------------*/
 /// calls f() on each element of the group, in storage order
 /** The Constraint counterpart of the previous method. */

 virtual void for_each( const std::function< void( Constraint & ) > & f )
  const = 0;

/*--------------------------------------------------------------------------*/
 /// returns the i-th element of a group of Variable, nullptr if there is none
 /** Returns the i-th element of the group in storage order, or nullptr if the
  * group is not one of Variable or i is out of range. Reaching the i-th
  * element of a group whose cells are collections costs as many steps as
  * there are cells before it, and for_each_as() is there to iterate. */

 [[nodiscard]] virtual Variable * get_Variable( Index i ) const = 0;

/*--------------------------------------------------------------------------*/
 /// returns the i-th element of a group of Constraint, nullptr if none
 /** The Constraint counterpart of the previous method. */

 [[nodiscard]] virtual Constraint * get_Constraint( Index i ) const = 0;

/*--------------------------------------------------------------------------*/
 /// returns true if the elements of the group derive from T
 /** Returns true if the elements of the group derive from T. If T is the
  * type of the elements this is a comparison of types; otherwise it is asked
  * of one element only, the group being homogeneous, with one dynamic_cast
  * for the whole group, and an empty group answers false, having no element
  * to ask. */

 template< class T >
 [[nodiscard]] bool elements_are( void ) const {
  if( f_type == typeid( T ) )
   return( true );

  if constexpr( std::is_base_of_v< Variable , T > ) {
   if( f_kind != eVariable )
    return( false );
   return( dynamic_cast< T * >( get_Variable( 0 ) ) );
   }
  else {
   static_assert( std::is_base_of_v< Constraint , T > ,
		  "T must derive from either Variable or Constraint" );
   if( f_kind != eConstraint )
    return( false );
   return( dynamic_cast< T * >( get_Constraint( 0 ) ) );
   }
  }

/*--------------------------------------------------------------------------*/
 /// returns the i-th element of the group as a T
 /** Returns the i-th element of the group as a T, without checking that the
  * elements are T: elements_are< T >() answers that once for the whole
  * group, and this method is what one uses afterwards. */

 template< class T >
 [[nodiscard]] T * get_as( Index i ) const {
  if constexpr( std::is_base_of_v< Variable , T > )
   return( static_cast< T * >( get_Variable( i ) ) );
  else {
   static_assert( std::is_base_of_v< Constraint , T > ,
		  "T must derive from either Variable or Constraint" );
   return( static_cast< T * >( get_Constraint( i ) ) );
   }
  }

/*--------------------------------------------------------------------------*/
 /// calls f() on each element of the group seen as a T, if they are T
 /** Calls f( T & ) on each element of the group, in storage order, and
  * returns true, if the elements of the group derive from T; does nothing
  * and returns false otherwise, which is what tells a caller sifting a Block
  * for the groups it can handle that this one is not among them.
  *
  * If T is exactly the type of the elements, the storage is reached with one
  * switch on the layout and walked with a loop typed on T, with no call per
  * element other than f() itself, and this holds also for an empty group. If
  * T is a base class of it, the elements are reached through for_each(). */

 template< class T , class F >
 bool for_each_as( F f ) const;

/*--------------------------------------------------------------------------*/
 /// calls f() on each cell of the group, if its elements are exactly T
 /** Calls f( c , cell ) for each cell c of a group whose cells are
  * collections, in storage order, and returns true, if the elements are
  * exactly of type T; returns false and does nothing otherwise, and also if
  * the group is contiguous. The cell is the collection itself, a std::list
  * or a std::vector of T or of T *, so that f() can see and change its
  * size; group_element() turns each item of it into a T &. */

 template< class T , class F >
 bool for_each_cell_as( F f ) const;

/*--------------------------------------------------------------------------*/
 /// calls f() on each run of contiguous elements of the group
 /** Calls f( first , n ) on each maximal run of n elements of the group that
  * are contiguous in memory, in storage order, and returns true, if the
  * elements are T; does nothing and returns false otherwise. A group that is
  * a single array is one run, a group made of many arrays is one run per
  * array, and a group that holds pointers, or whose cells are collections,
  * can be one run per element. This is what a caller needs to map an element
  * back to its position from its address alone, which is a subtraction
  * inside a run and says nothing across two of them. */

 template< class T , class F >
 bool for_each_run_as( F f ) const;

/** @} ---------------------------------------------------------------------*/
/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*----------------------- PROTECTED TYPES ----------------------------------*/

 /// the function reading the storage of a container
 /** Given the container, returns the address of its first item, or of its
  * first cell if the cells are collections, and writes the extents of the
  * grid into size[ 0 ] ... size[ rank - 1 ]. */

 using view_function = void * (*)( void * container , Index * size );

 /// where the storage of the group is now, and how many cells it has

 struct storage {
  void * first;     ///< the first item, or the first cell
  Index num_cells;  ///< the number of cells of the grid
  };

/*----------------------- PROTECTED CONSTRUCTOR ----------------------------*/

 /// constructor of BaseGroup, taking the layout, the container and the rest
 /** Builds the group of elements of type T, laid out as \p layout on a grid
  * of \p rank dimensions, and held by pointer if \p indirect, whose storage
  * is read out of \p container by \p view. Only the leaves call it, and
  * for_each_as() relies on these fields naming the storage correctly. */

 template< class T >
 BaseGroup( T * , layout_type layout , bool indirect , unsigned char rank ,
	    void * container , view_function view , Block * block ,
	    Index index , std::string name )
  : f_Block( block ) , f_index( index ) , f_name( std::move( name ) ) ,
    f_type( typeid( T ) ) ,
    f_kind( std::is_base_of_v< Variable , T > ? eVariable : eConstraint ) ,
    f_layout( layout ) , f_indirect( indirect ) , f_rank( rank ) ,
    f_container( container ) , f_view( view ) {
  if( rank > max_rank )
   throw( std::invalid_argument( "BaseGroup::BaseGroup: rank " +
				 std::to_string( rank ) + " too large" ) );
  }

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// reads the storage of the group out of its container
 /** The container is read every time, and not once when the group is built,
  * because whoever owns it may have resized it since. */

 [[nodiscard]] storage get_storage( void ) const {
  std::array< Index , max_rank > size;
  storage result{ f_view( f_container , size.data() ) , 1 };
  for( unsigned char d = 0 ; d < f_rank ; ++d )
   result.num_cells *= size[ d ];
  return( result );
  }

/*-------------------------- PROTECTED FIELDS ------------------------------*/

 Block * f_Block;      ///< the Block the group belongs to

 Index f_index;        ///< the index of the group in the Block

 std::string f_name;   ///< the name of the group, may be empty

 std::function< void( void ) > f_deleter;  ///< how the storage is deleted
                                           /**< empty unless whoever built
  * the group is entitled to dispose of the storage and said how. */

 std::type_index f_type;   ///< the type of the elements

 kind_type f_kind;         ///< whether the elements are Variable or Constraint

 layout_type f_layout;     ///< what a cell holds

 bool f_indirect;          ///< true if the storage holds pointers

 unsigned char f_rank;     ///< the number of dimensions of the grid

 void * f_container;       ///< the container the group views

 view_function f_view;     ///< how the storage is read out of the container

/*--------------------------------------------------------------------------*/

 };  // end( class( BaseGroup ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- STRUCT group_form -----------------------------*/
/*--------------------------------------------------------------------------*/
/// what a group needs to know of a container of type C
/** group_form< C > says, for each container a :Block can register, the type
 * S of its items (an element type T or T *), the type of its cells when they
 * are collections, the layout, the rank, and view(), the view_function that
 * reads its storage. The primary template is the single item; the
 * specialisations are std::vector, std::vector of std::vector, std::list,
 * std::vector of std::list, and boost::multi_array of the item, of
 * std::vector and of std::list. */

template< class C >
struct group_form {
 using item_type = C;     ///< the type of the items
 using cell_type = void;  ///< the cells hold one item
 static constexpr BaseGroup::layout_type layout = BaseGroup::eContiguous;
 static constexpr unsigned char rank = 0;
 static void * view( void * c , BaseGroup::Index * ) { return( c ); }
 };

template< class S >
struct group_form< std::vector< S > > {
 using item_type = S;
 using cell_type = void;
 static constexpr BaseGroup::layout_type layout = BaseGroup::eContiguous;
 static constexpr unsigned char rank = 1;
 static void * view( void * c , BaseGroup::Index * size ) {
  auto v = static_cast< std::vector< S > * >( c );
  size[ 0 ] = v->size();
  return( v->data() );
  }
 };

template< class S >
struct group_form< std::vector< std::vector< S > > > {
 using item_type = S;
 using cell_type = std::vector< S >;
 static constexpr BaseGroup::layout_type layout = BaseGroup::eJagged;
 static constexpr unsigned char rank = 1;
 static void * view( void * c , BaseGroup::Index * size ) {
  auto v = static_cast< std::vector< cell_type > * >( c );
  size[ 0 ] = v->size();
  return( v->data() );
  }
 };

template< class S >
struct group_form< std::list< S > > {
 using item_type = S;
 using cell_type = std::list< S >;
 static constexpr BaseGroup::layout_type layout = BaseGroup::eDynamic;
 static constexpr unsigned char rank = 0;
 static void * view( void * c , BaseGroup::Index * ) { return( c ); }
 };

template< class S >
struct group_form< std::vector< std::list< S > > > {
 using item_type = S;
 using cell_type = std::list< S >;
 static constexpr BaseGroup::layout_type layout = BaseGroup::eDynamic;
 static constexpr unsigned char rank = 1;
 static void * view( void * c , BaseGroup::Index * size ) {
  auto v = static_cast< std::vector< cell_type > * >( c );
  size[ 0 ] = v->size();
  return( v->data() );
  }
 };

/// the boost::multi_array forms, whose cells are X
template< class X , std::size_t K , BaseGroup::layout_type L ,
	  class Cell >
struct group_form_multi_array {
 using cell_type = Cell;
 static constexpr BaseGroup::layout_type layout = L;
 static constexpr unsigned char rank = K;
 static void * view( void * c , BaseGroup::Index * size ) {
  auto a = static_cast< boost::multi_array< X , K > * >( c );
  for( std::size_t d = 0 ; d < K ; ++d )
   size[ d ] = a->shape()[ d ];
  return( a->data() );
  }
 };

template< class S , std::size_t K >
struct group_form< boost::multi_array< S , K > >
 : group_form_multi_array< S , K , BaseGroup::eContiguous , void > {
 using item_type = S;
 };

template< class S , std::size_t K >
struct group_form< boost::multi_array< std::vector< S > , K > >
 : group_form_multi_array< std::vector< S > , K , BaseGroup::eJagged ,
			   std::vector< S > > {
 using item_type = S;
 };

template< class S , std::size_t K >
struct group_form< boost::multi_array< std::list< S > , K > >
 : group_form_multi_array< std::list< S > , K , BaseGroup::eDynamic ,
			   std::list< S > > {
 using item_type = S;
 };

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS StaticGroup ------------------------------*/
/*--------------------------------------------------------------------------*/
/// a group of one element per cell, laid out contiguously in memory
/** StaticGroup is the group of one element per cell, and covers the static
 * forms a :Block declares whose storage is contiguous: a single item, a
 * std::vector and a boost::multi_array. S is the type of the items, either
 * the element type T or T *. The group views the container and owns
 * nothing. */

template< class S >
class StaticGroup : public BaseGroup {

 public:

 using element_type = std::remove_pointer_t< S >;  ///< the type T

 /// constructor of StaticGroup, taking the container of the items

 template< class C >
 explicit StaticGroup( C * container , Block * block = nullptr ,
		       Index index = 0 , std::string name = "" )
  : BaseGroup( static_cast< element_type * >( nullptr ) , eContiguous ,
	       std::is_pointer_v< S > , group_form< C >::rank , container ,
	       & group_form< C >::view , block , index , std::move( name ) ) {
  static_assert( std::is_same_v< typename group_form< C >::item_type , S > &&
		 ( group_form< C >::layout == eContiguous ) ,
		 "the container does not hold one S per cell" );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] Index get_num_elements( void ) const override {
  return( get_num_cells() );
  }

/*--------------------------------------------------------------------------*/

 void for_each( const std::function< void( Variable & ) > & f )
  const override {
  if constexpr( std::is_base_of_v< Variable , element_type > ) {
   auto s = get_storage();
   auto items = static_cast< S * >( s.first );
   for( Index i = 0 ; i < s.num_cells ; ++i )
    f( group_element( items[ i ] ) );
   }
  }

 void for_each( const std::function< void( Constraint & ) > & f )
  const override {
  if constexpr( std::is_base_of_v< Constraint , element_type > ) {
   auto s = get_storage();
   auto items = static_cast< S * >( s.first );
   for( Index i = 0 ; i < s.num_cells ; ++i )
    f( group_element( items[ i ] ) );
   }
  }

 [[nodiscard]] Variable * get_Variable( Index i ) const override {
  if constexpr( std::is_base_of_v< Variable , element_type > )
   return( get( i ) );
  else
   return( nullptr );
  }

 [[nodiscard]] Constraint * get_Constraint( Index i ) const override {
  if constexpr( std::is_base_of_v< Constraint , element_type > )
   return( get( i ) );
  else
   return( nullptr );
  }

/*--------------------------------------------------------------------------*/
 /// returns the first item of the storage

 [[nodiscard]] S * data( void ) const {
  return( static_cast< S * >( get_storage().first ) );
  }

/*--------------------------------------------------------------------------*/
 /// returns the i-th element with its own type

 [[nodiscard]] element_type * get( Index i ) const {
  auto s = get_storage();
  return( i < s.num_cells ?
	  & group_element( static_cast< S * >( s.first )[ i ] ) : nullptr );
  }

/*--------------------------------------------------------------------------*/
 /// returns the index of the cell of an element of the group
 /** Returns the position of *element in the grid, in storage order, or
  * get_num_cells() if it does not belong to the group: this is what makes an
  * element tell where it is from its address alone. It is a subtraction when
  * the group holds the elements, and a scan when it holds pointers. */

 [[nodiscard]] Index get_position( const element_type * element ) const {
  auto s = get_storage();
  auto items = static_cast< S * >( s.first );
  if constexpr( std::is_pointer_v< S > ) {
   for( Index i = 0 ; i < s.num_cells ; ++i )
    if( items[ i ] == element )
     return( i );
   return( s.num_cells );
   }
  else {
   if( ( element < items ) || ( element >= items + s.num_cells ) )
    return( s.num_cells );
   return( element - items );
   }
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( StaticGroup ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS CellGroup -------------------------------*/
/*--------------------------------------------------------------------------*/
/// a group whose cells hold a collection of elements each
/** CellGroup is the group of one collection per cell, C being the type of
 * that collection and S the type of its items, either the element type T or
 * T *. With C = std::list< S > it covers the dynamic forms a :Block declares,
 * a single std::list, a std::vector of std::list and a boost::multi_array of
 * std::list, where elements come and go inside a cell without the others
 * moving; with C = std::vector< S > it covers the irregular static forms, a
 * std::vector of std::vector and a boost::multi_array of std::vector, whose
 * cells have different lengths. The group views the container and owns
 * nothing. */

template< class S , class C = std::list< S > >
class CellGroup : public BaseGroup {

 public:

 using element_type = std::remove_pointer_t< S >;  ///< the type T

 using cell_type = C;  ///< the collection held by a cell

 /// constructor of CellGroup, taking the container of the cells

 template< class Container >
 explicit CellGroup( Container * container , Block * block = nullptr ,
		     Index index = 0 , std::string name = "" )
  : BaseGroup( static_cast< element_type * >( nullptr ) ,
	       std::is_same_v< C , std::list< S > > ? eDynamic : eJagged ,
	       std::is_pointer_v< S > , group_form< Container >::rank ,
	       container , & group_form< Container >::view , block , index ,
	       std::move( name ) ) {
  static_assert(
   std::is_same_v< typename group_form< Container >::cell_type , C > ,
   "the container does not hold cells of type C" );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] Index get_num_elements( void ) const override {
  auto s = get_storage();
  auto cells = static_cast< C * >( s.first );
  Index n = 0;
  for( Index c = 0 ; c < s.num_cells ; ++c )
   n += cells[ c ].size();
  return( n );
  }

/*--------------------------------------------------------------------------*/

 void for_each( const std::function< void( Variable & ) > & f )
  const override {
  if constexpr( std::is_base_of_v< Variable , element_type > ) {
   auto s = get_storage();
   auto cells = static_cast< C * >( s.first );
   for( Index c = 0 ; c < s.num_cells ; ++c )
    for( auto & item : cells[ c ] )
     f( group_element( item ) );
   }
  }

 void for_each( const std::function< void( Constraint & ) > & f )
  const override {
  if constexpr( std::is_base_of_v< Constraint , element_type > ) {
   auto s = get_storage();
   auto cells = static_cast< C * >( s.first );
   for( Index c = 0 ; c < s.num_cells ; ++c )
    for( auto & item : cells[ c ] )
     f( group_element( item ) );
   }
  }

 [[nodiscard]] Variable * get_Variable( Index i ) const override {
  if constexpr( std::is_base_of_v< Variable , element_type > )
   return( get( i ) );
  else
   return( nullptr );
  }

 [[nodiscard]] Constraint * get_Constraint( Index i ) const override {
  if constexpr( std::is_base_of_v< Constraint , element_type > )
   return( get( i ) );
  else
   return( nullptr );
  }

/*--------------------------------------------------------------------------*/
 /// returns the first cell of the grid

 [[nodiscard]] C * data( void ) const {
  return( static_cast< C * >( get_storage().first ) );
  }

/*--------------------------------------------------------------------------*/
 /// returns the i-th element with its own type, in storage order

 [[nodiscard]] element_type * get( Index i ) const {
  auto s = get_storage();
  auto cells = static_cast< C * >( s.first );
  for( Index c = 0 ; c < s.num_cells ; ++c ) {
   auto & cell = cells[ c ];
   if( i < cell.size() )
    return( & group_element( * std::next( cell.begin() , i ) ) );
   i -= cell.size();
   }
  return( nullptr );
  }

/*--------------------------------------------------------------------------*/
 /// returns the collection of the c-th cell of the grid

 [[nodiscard]] C * get_cell( Index c ) const {
  auto s = get_storage();
  return( c < s.num_cells ? static_cast< C * >( s.first ) + c : nullptr );
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( CellGroup ) )

/*--------------------------------------------------------------------------*/
 /// a group whose cells are lists, the form of the dynamic ones

template< class S >
using DynamicGroup = CellGroup< S , std::list< S > >;

/*--------------------------------------------------------------------------*/
 /// a group whose cells are vectors, the irregular static form

template< class S >
using JaggedGroup = CellGroup< S , std::vector< S > >;

/*--------------------------------------------------------------------------*/
 /// the groups of one kind of a Block, owned by it

using Vec_Group = std::vector< std::unique_ptr< BaseGroup > >;

/*--------------------------------------------------------------------------*/
 /// the group viewing a container of type C

template< class C >
using group_for = std::conditional_t<
 group_form< C >::layout == BaseGroup::eContiguous ,
 StaticGroup< typename group_form< C >::item_type > ,
 CellGroup< typename group_form< C >::item_type ,
	    typename group_form< C >::cell_type > >;

/*--------------------------------------------------------------------------*/
 /// builds the group viewing \p container

template< class C >
std::unique_ptr< BaseGroup > make_group( C & container ,
					 Block * block = nullptr ,
					 BaseGroup::Index index = 0 ,
					 std::string name = "" ) {
 return( std::make_unique< group_for< C > >( & container , block , index ,
					     std::move( name ) ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- FREE FUNCTIONS ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading a group whose elements are one of a set of types
 *  @{ */

/// calls f() on the elements of the group if these are any of T...
/** Calls f() on each element of the group, in storage order, and returns
 * true, if the elements of the group are of any of the types T..., which are
 * tried in the order in which they are given; does nothing and returns false
 * otherwise. This is the pattern of a caller that can treat a handful of
 * concrete types, say the :RowConstraint of the core, and leaves the other
 * groups of a Block alone. */

template< class... T , class F >
bool for_each_as_any_of( const BaseGroup & group , F f )
{
 return( ( group.template for_each_as< T >( f ) || ... ) );
 }

/** @} ---------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------- METHODS OF BaseGroup NEEDING THE LEAVES --------------*/
/*--------------------------------------------------------------------------*/

template< class T , class F >
bool BaseGroup::for_each_as( F f ) const
{
 if( f_type == typeid( T ) ) {
  switch( f_layout ) {
   case( eContiguous ): {
    auto s = get_storage();
    if( f_indirect ) {
     auto items = static_cast< T ** >( s.first );
     for( Index i = 0 ; i < s.num_cells ; ++i )
      f( *items[ i ] );
     }
    else {
     auto items = static_cast< T * >( s.first );
     for( Index i = 0 ; i < s.num_cells ; ++i )
      f( items[ i ] );
     }
    return( true );
    }
   case( eJagged ):
   case( eDynamic ):
    for_each_cell_as< T >( [ & f ]( Index , auto & cell ) {
     for( auto & item : cell )
      f( group_element( item ) );
     } );
    return( true );
   }
  }

 if( ! elements_are< T >() )
  return( false );

 if constexpr( std::is_base_of_v< Variable , T > )
  for_each( [ & f ]( Variable & v ) { f( static_cast< T & >( v ) ); } );
 else
  for_each( [ & f ]( Constraint & c ) { f( static_cast< T & >( c ) ); } );

 return( true );
 }

/*--------------------------------------------------------------------------*/

template< class T , class F >
bool BaseGroup::for_each_cell_as( F f ) const
{
 if( ( f_type != typeid( T ) ) || ( f_layout == eContiguous ) )
  return( false );

 auto s = get_storage();
 auto walk = [ & f , & s ]( auto * cells ) {
  for( Index c = 0 ; c < s.num_cells ; ++c )
   f( c , cells[ c ] );
  };

 if( f_layout == eDynamic ) {
  if( f_indirect )
   walk( static_cast< std::list< T * > * >( s.first ) );
  else
   walk( static_cast< std::list< T > * >( s.first ) );
  }
 else {
  if( f_indirect )
   walk( static_cast< std::vector< T * > * >( s.first ) );
  else
   walk( static_cast< std::vector< T > * >( s.first ) );
  }

 return( true );
 }

/*--------------------------------------------------------------------------*/

template< class T , class F >
bool BaseGroup::for_each_run_as( F f ) const
{
 if( f_type != typeid( T ) )
  return( for_each_as< T >( [ & f ]( T & element ) { f( & element , 1 ); } ) );

 // where the runs are is known from the shape alone, and the elements are
 // not looked at one by one: an array is one run, a collection of arrays is
 // one run per array, and elements reached through pointers, or sitting in
 // the nodes of a list, are one run each
 if( f_indirect || ( f_layout == eDynamic ) )
  return( for_each_as< T >( [ & f ]( T & element ) { f( & element , 1 ); } ) );

 if( f_layout == eContiguous ) {
  auto s = get_storage();
  if( s.num_cells )
   f( static_cast< T * >( s.first ) , s.num_cells );
  return( true );
  }

 return( for_each_cell_as< T >( [ & f ]( Index , auto & cell ) {
   if constexpr( std::is_same_v< std::decay_t< decltype( cell ) > ,
				 std::vector< T > > )
    if( ! cell.empty() )
     f( cell.data() , Index( cell.size() ) );
   } ) );
 }

/*--------------------------------------------------------------------------*/

/** @}  end( group( Group_CLASSES ) ) --------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* Group.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Group.h -----------------------------*/
/*--------------------------------------------------------------------------*/
