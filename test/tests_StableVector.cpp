/*--------------------------------------------------------------------------*/
/*------------------------ File tests_StableVector.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Unit tests for the splice() of boost::container::stable_vector, which the
 * build adds to the stable_vector of the packaged Boost [see shim/README.md].
 *
 * A dynamic group of a Block holds elements that the rest of the model refers
 * to by address, and that cannot be copied (the copy constructor of
 * Constraint throws); hence, they can only be moved into the group from a
 * temporary container, and out of it into the one that keeps them alive
 * after a removal, if neither the move nor the copy of an element is ever
 * called. The tests take the three splice() through every position (at the
 * beginning, in the middle, at the end, into an empty stable_vector), through
 * a growth of the index of the receiving stable_vector, and back, checking
 * that the order of the elements is the expected one, that their addresses do
 * not change, that no element is constructed or destroyed by a splice(), and
 * that both stable_vector work as usual afterwards.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "FRowConstraint.h"

#include <boost/container/stable_vector.hpp>

#include <iostream>
#include <vector>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------- TYPES ------------------------------------*/
/*--------------------------------------------------------------------------*/

static int n_built = 0;      ///< elements constructed so far
static int n_destroyed = 0;  ///< elements destroyed so far

/// an element that can be neither copied nor moved, counting its lifetime
struct Pinned {
 int v;
 explicit Pinned( int x ) : v( x ) { ++n_built; }
 Pinned( const Pinned & ) = delete;
 Pinned( Pinned && ) = delete;
 Pinned & operator=( const Pinned & ) = delete;
 Pinned & operator=( Pinned && ) = delete;
 ~Pinned() { ++n_destroyed; }
 };

using SV = boost::container::stable_vector< Pinned >;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/
/// checks that s holds the values in want, by iteration and by index

static void check( const SV & s , const std::vector< int > & want )
{
 assert( s.size() == want.size() );
 std::size_t i = 0;
 for( const auto & e : s )
  assert( e.v == want[ i++ ] );
 for( i = 0 ; i < want.size() ; ++i )
  assert( s[ i ].v == want[ i ] );
 }

/*--------------------------------------------------------------------------*/
/// fills s with the values from first to first + n - 1

static void fill( SV & s , int first , int n )
{
 for( int i = 0 ; i < n ; ++i )
  s.emplace_back( first + i );
 }

/*--------------------------------------------------------------------------*/
/// runs f, which has to neither construct nor destroy any element

template< class F > static void no_lifetime( F && f )
{
 const int built = n_built;
 const int destroyed = n_destroyed;
 f();
 assert( ( n_built == built ) && ( n_destroyed == destroyed ) );
 }

/*--------------------------------------------------------------------------*/
/// the three splice(), at every position and into an empty stable_vector

static void test_positions( void )
{
 SV a , b , c;
 fill( a , 0 , 5 );   // 0 1 2 3 4
 fill( b , 10 , 3 );  // 10 11 12
 Pinned * p11 = & b[ 1 ];
 Pinned * p2 = & a[ 2 ];
 Pinned * p4 = & a[ 4 ];

 // all of b, in the middle of a
 no_lifetime( [ & ] { a.splice( a.cbegin() + 2 , b ); } );
 check( a , { 0 , 1 , 10 , 11 , 12 , 2 , 3 , 4 } );
 assert( b.empty() );
 assert( ( & a[ 3 ] == p11 ) && ( & a[ 5 ] == p2 ) && ( & a[ 7 ] == p4 ) );

 // one element, into an empty stable_vector
 no_lifetime( [ & ] { c.splice( c.cend() , a , a.cbegin() + 3 ); } );
 check( c , { 11 } );
 check( a , { 0 , 1 , 10 , 12 , 2 , 3 , 4 } );
 assert( & c[ 0 ] == p11 );

 // a range at the end of a, at the beginning of c
 no_lifetime( [ & ] { c.splice( c.cbegin() , a , a.cbegin() + 4 ,
                                a.cend() ); } );
 check( c , { 2 , 3 , 4 , 11 } );
 check( a , { 0 , 1 , 10 , 12 } );
 assert( ( & c[ 0 ] == p2 ) && ( & c[ 2 ] == p4 ) );

 // an empty range changes nothing
 no_lifetime( [ & ] { a.splice( a.cend() , c , c.cbegin() , c.cbegin() ); } );
 check( a , { 0 , 1 , 10 , 12 } );
 check( c , { 2 , 3 , 4 , 11 } );

 // all of c at the end of a, and an rvalue
 no_lifetime( [ & ] { a.splice( a.cend() , c ); } );
 no_lifetime( [ & ] { a.splice( a.cend() , SV() ); } );
 check( a , { 0 , 1 , 10 , 12 , 2 , 3 , 4 , 11 } );
 assert( c.empty() );

 // both still work as usual
 a.erase( a.cbegin() + 2 );
 a.emplace( a.cbegin() , 7 );
 c.emplace_back( 8 );
 check( a , { 7 , 0 , 1 , 12 , 2 , 3 , 4 , 11 } );
 check( c , { 8 } );
 }

/*--------------------------------------------------------------------------*/
/// a splice() that makes the index of the receiving stable_vector grow, as a
/// group gaining many elements does, and the one that takes them out again,
/// as their removal does

static void test_growth( void )
{
 SV group , added , removed;
 fill( group , 0 , 3 );
 fill( added , 100 , 1000 );
 std::vector< Pinned * > address( 1000 );
 for( int i = 0 ; i < 1000 ; ++i )
  address[ i ] = & added[ i ];

 no_lifetime( [ & ] { group.splice( group.cbegin() + 1 , added ); } );
 assert( group.size() == 1003 );
 assert( ( group[ 0 ].v == 0 ) && ( group[ 1002 ].v == 2 ) );
 for( int i = 0 ; i < 1000 ; ++i )
  assert( & group[ i + 1 ] == address[ i ] );

 no_lifetime( [ & ] { removed.splice( removed.cend() , group ,
                                      group.cbegin() + 1 ,
                                      group.cbegin() + 1001 ); } );
 check( group , { 0 , 1 , 2 } );
 assert( removed.size() == 1000 );
 for( int i = 0 ; i < 1000 ; ++i )
  assert( & removed[ i ] == address[ i ] );
 }

/*--------------------------------------------------------------------------*/
/// the elements of a dynamic group of Constraint, whose copy constructor
/// throws: they go in and out of the group by splice() alone, and keep their
/// address

static void test_constraints( void )
{
 boost::container::stable_vector< FRowConstraint > group , added , removed;
 group.emplace_back();
 added.emplace_back();
 added.emplace_back();
 FRowConstraint * first = & added.front();
 FRowConstraint * second = & added.back();

 group.splice( group.cend() , added );
 assert( ( group.size() == 3 ) && added.empty() );
 assert( ( & group[ 1 ] == first ) && ( & group[ 2 ] == second ) );

 removed.splice( removed.cend() , group , group.cbegin() + 1 );
 assert( ( group.size() == 2 ) && ( & group[ 1 ] == second ) );
 assert( ( removed.size() == 1 ) && ( & removed[ 0 ] == first ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( void )
{
 test_positions();
 test_growth();
 test_constraints();

 // every element constructed has been destroyed, exactly once
 assert( n_built == n_destroyed );

 std::cout << "StableVector_unit_test: all tests passed" << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File tests_StableVector.cpp -------------------*/
/*--------------------------------------------------------------------------*/
