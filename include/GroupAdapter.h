/*--------------------------------------------------------------------------*/
/*-------------------------- File GroupAdapter.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for make_groups(), which reads the vectors of boost::any in
 * which a Block keeps its groups of Variable and Constraint and returns the
 * same groups as BaseGroup.
 *
 * The list of the types that can be in an any, and of the shapes they can be
 * in, is written here and nowhere else: this is the one place that pays the
 * price of boost::any, and whoever reads the groups from here on works in
 * terms of Variable and Constraint alone.
 *
 * The groups are VIEWS: the elements stay where the :Block put them, and
 * nothing is copied or owned.
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

#ifndef __GroupAdapter
 #define __GroupAdapter
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Group.h"

#include "ColVariable.h"
#include "FRowConstraint.h"
#include "OneVarConstraint.h"

#include <boost/any.hpp>
#include <boost/multi_array.hpp>

#include <array>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace SMSpp_di_unipi_it
{
/** @defgroup GroupAdapter_FUNCTIONS Functions in GroupAdapter.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*-------------------------- AUXILIARY STUFF -------------------------------*/
/*--------------------------------------------------------------------------*/

namespace group_adapter_detail {

 using Index = BaseGroup::Index;

/*--------------------------------------------------------------------------*/
 /// the function that builds the group of an any known to hold a C *

 template< class Sink >
 using builder = void (*)( const boost::any & , Block * , Index ,
			   const std::string & , Sink & );

 template< class C , class Sink >
 void build( const boost::any & any , Block * block , Index index ,
	     const std::string & name , Sink & sink ) {
  C * container = * boost::unsafe_any_cast< C * >( & any );
  sink( group_for< C >( container , block , index , name ) , container );
  }

/*--------------------------------------------------------------------------*/
 /// the boost::multi_array entries of rank K of table()

 template< class S , std::size_t K , class Add >
 void add_multi_array( Add & add , bool dynamic ) {
  if( dynamic )
   add( static_cast< boost::multi_array< std::list< S > , K > * >( nullptr ) );
  else {
   add( static_cast< boost::multi_array< S , K > * >( nullptr ) );
   add( static_cast< boost::multi_array< std::vector< S > , K > * >
	( nullptr ) );
   }
  }

/*--------------------------------------------------------------------------*/
 /// the table from the type held by an any to the function building its group
 /** Lists, for each type T of \p T... and for S = T and S = T *, the
  * containers of S that a :Block can register, static or dynamic as
  * \p dynamic says, with a boost::multi_array of any rank from 2 to
  * BaseGroup::max_rank. It is built once, and only read afterwards. */

 template< class Sink , bool dynamic , class... T >
 const std::unordered_map< std::type_index , builder< Sink > > &
 table( void ) {
  static const auto the_table = [] {
   std::unordered_map< std::type_index , builder< Sink > > t;
   auto add = [ & t ]( auto * tag ) {
    using C = std::remove_pointer_t< decltype( tag ) >;
    t.emplace( std::type_index( typeid( C * ) ) , & build< C , Sink > );
    };
   auto add_all = [ & ]( auto * tag ) {
    using S = std::remove_pointer_t< decltype( tag ) >;
    using std::vector , std::list;
    if( dynamic ) {
     add( static_cast< list< S > * >( nullptr ) );
     add( static_cast< vector< list< S > > * >( nullptr ) );
     }
    else {
     add( static_cast< S * >( nullptr ) );
     add( static_cast< vector< S > * >( nullptr ) );
     add( static_cast< vector< vector< S > > * >( nullptr ) );
     }
    add_multi_array< S , 2 >( add , dynamic );
    add_multi_array< S , 3 >( add , dynamic );
    add_multi_array< S , 4 >( add , dynamic );
    add_multi_array< S , 5 >( add , dynamic );
    add_multi_array< S , 6 >( add , dynamic );
    add_multi_array< S , 7 >( add , dynamic );
    add_multi_array< S , 8 >( add , dynamic );
    };
   ( ( add_all( static_cast< T * >( nullptr ) ) ,
       add_all( static_cast< T ** >( nullptr ) ) ) , ... );
   return( t );
   }();
  return( the_table );
  }

/*--------------------------------------------------------------------------*/
 /// builds the group of an any, if its type is one of \p T..., and sinks it
 /** Looks the type held by \p any up in table(), first through a cache that
  * each thread keeps of the type_info it has already met, so that a group
  * costs a few comparisons of pointers, and calls the builder found, if
  * any. Returns true if there was one. */

 template< class Sink , bool dynamic , class... T >
 bool fits( const boost::any & any , Block * block , Index index ,
	    const std::string & name , Sink sink ) {
  if( any.empty() )
   return( false );

  const std::type_info & type = any.type();

  // a Block holds few types of group, so a short array scanned by pointer is
  // cheaper than any hash; the table is asked only on a miss
  struct entry { const std::type_info * type; builder< Sink > build; };
  thread_local std::array< entry , 16 > cache{};
  thread_local unsigned char used = 0 , next = 0;

  builder< Sink > build = nullptr;
  unsigned char k = 0;
  for( ; k < used ; ++k )
   if( cache[ k ].type == & type ) {
    build = cache[ k ].build;
    break;
    }

  if( k == used ) {
   const auto & t = table< Sink , dynamic , T... >();
   auto found = t.find( std::type_index( type ) );
   build = ( found == t.end() ) ? nullptr : found->second;
   unsigned char slot = used < cache.size() ? used++ : next++ % cache.size();
   cache[ slot ] = { & type , build };
   }

  if( ! build )
   return( false );
  build( any , block , index , name , sink );
  return( true );
  }

/*--------------------------------------------------------------------------*/
 /// a sink that keeps the group, with the deleter of its storage

 struct keep {
  std::unique_ptr< BaseGroup > & group;

  template< class G , class P >
  void operator()( G && g , P * storage ) const {
   group = std::make_unique< std::decay_t< G > >( std::forward< G >( g ) );
   group->set_storage_deleter( [ storage ]() { delete storage; } );
   }
  };

/*--------------------------------------------------------------------------*/
 /// a sink that hands the group to a function, and drops it afterwards

 template< class F >
 struct hand_to {
  F & f;

  template< class G , class P >
  void operator()( G && g , P * ) const {
   const BaseGroup & group = g;
   f( group );
   }
  };

/*--------------------------------------------------------------------------*/
 /// tries the types of Variable that a group may hold

 template< class Sink >
 bool fits_variable( bool dynamic , const boost::any & any , Block * block ,
		     Index index , const std::string & name , Sink sink ) {
  return( dynamic
	  ? fits< Sink , true , ColVariable >( any , block , index , name ,
					       sink )
	  : fits< Sink , false , ColVariable >( any , block , index , name ,
						sink ) );
  }

/*--------------------------------------------------------------------------*/
 /// tries the types of Constraint that a group may hold

 template< class Sink >
 bool fits_constraint( bool dynamic , const boost::any & any , Block * block ,
		       Index index , const std::string & name , Sink sink ) {
  if( dynamic )
   return( fits< Sink , true , FRowConstraint , BoxConstraint , LB0Constraint ,
		 UB0Constraint , LBConstraint , UBConstraint , NNConstraint ,
		 NPConstraint , ZOConstraint >( any , block , index , name ,
						sink ) );
  return( fits< Sink , false , FRowConstraint , BoxConstraint , LB0Constraint ,
		UB0Constraint , LBConstraint , UBConstraint , NNConstraint ,
		NPConstraint , ZOConstraint >( any , block , index , name ,
					       sink ) );
  }

 }  // end( namespace group_adapter_detail )

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/
/// the groups of Variable of a Block, read out of its vector of boost::any
/** Returns one BaseGroup for each slot of \p anys, in the same order, with
 * the name that \p names gives it, and with the deleter of its storage. A
 * slot whose content is of no known type or shape gives a null entry, so
 * that the caller sees that a group is there and that this function could
 * not read it. */

inline std::vector< std::unique_ptr< BaseGroup > >
make_variable_groups( Block * block , const std::vector< boost::any > & anys ,
		      const std::vector< std::string > & names ,
		      bool dynamic ) {
 using namespace group_adapter_detail;
 std::vector< std::unique_ptr< BaseGroup > > groups( anys.size() );
 for( Index i = 0 ; i < anys.size() ; ++i )
  fits_variable( dynamic , anys[ i ] , block , i ,
		 i < names.size() ? names[ i ] : std::string() ,
		 keep{ groups[ i ] } );
 return( groups );
 }

/*--------------------------------------------------------------------------*/
/// the groups of Constraint of a Block, read out of its vector of boost::any
/** The Constraint counterpart of the previous one: the types tried are the
 * concrete :Constraint of the core, in the order in which they are most
 * often met. */

inline std::vector< std::unique_ptr< BaseGroup > >
make_constraint_groups( Block * block ,
			const std::vector< boost::any > & anys ,
			const std::vector< std::string > & names ,
			bool dynamic ) {
 using namespace group_adapter_detail;
 std::vector< std::unique_ptr< BaseGroup > > groups( anys.size() );
 for( Index i = 0 ; i < anys.size() ; ++i )
  fits_constraint( dynamic , anys[ i ] , block , i ,
		   i < names.size() ? names[ i ] : std::string() ,
		   keep{ groups[ i ] } );
 return( groups );
 }

/*--------------------------------------------------------------------------*/
/// calls f() on the group of Variable held by an any, without allocating
/** Builds on the stack the group of Variable that \p any holds and calls
 * f( const BaseGroup & ) on it, returning true, or returns false if the
 * content of \p any is of no known type or shape. The group has neither a
 * name nor a deleter and lives only for the call, and the type is found with
 * one lookup keyed on a pointer: this is the form for paths that read the
 * groups of a Block often. */

template< class F >
bool visit_variable_group( const boost::any & any , bool dynamic , F f ,
			   Block * block = nullptr ,
			   BaseGroup::Index index = 0 ) {
 static const std::string no_name;
 return( group_adapter_detail::fits_variable(
	  dynamic , any , block , index , no_name ,
	  group_adapter_detail::hand_to< F >{ f } ) );
 }

/*--------------------------------------------------------------------------*/
/// calls f() on the group of Constraint held by an any, without allocating
/** The Constraint counterpart of the previous one. */

template< class F >
bool visit_constraint_group( const boost::any & any , bool dynamic , F f ,
			     Block * block = nullptr ,
			     BaseGroup::Index index = 0 ) {
 static const std::string no_name;
 return( group_adapter_detail::fits_constraint(
	  dynamic , any , block , index , no_name ,
	  group_adapter_detail::hand_to< F >{ f } ) );
 }

/** @}  end( group( GroupAdapter_FUNCTIONS ) ) -----------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* GroupAdapter.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File GroupAdapter.h --------------------------*/
/*--------------------------------------------------------------------------*/
