/*--------------------------------------------------------------------------*/
/*--------------------------- File SMSStreams.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file with the operator<<() that print std::pair, std::vector,
 * std::list and boost::multi_array, the operator>>() that load std::pair
 * and std::vector / std::list in dense or sparse format, and the
 * eatcomments manipulator that skips the comments of a std::istream.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ DEFINITIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __SMSStreams
 #define __SMSStreams
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

// standard C++ libraries (alphabetical order)
#include <istream>
#include <limits>
#include <list>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// boost libraries
#include <boost/multi_array.hpp>

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/
/// namespace for the Structured Modeling System++ (SMS++)

namespace SMSpp_di_unipi_it
{
/*--------------------------------------------------------------------------*/
/*----------------- PRINTING list, array and multi_array -------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup print_multi_arrays Printing list, pair, array and multi_array
 *
 * A few versions of operator<< for printing std::list, std::pair,
 * std::array and boost::multi_array.
 *  @{ */

template< class T1 , class T2 >
std::ostream &
operator<<( std::ostream & os , const std::pair< T1 , T2 > & p ) {
 os << "( " << p.first << ", " << p.second << " )";
 return( os );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Trait detecting a std::pair, used to disambiguate its printing below and by
 * SimpleConfiguration::print(). */

template< typename T >
struct is_std_pair { static constexpr bool value = false; };

template< typename T1 , typename T2 >
struct is_std_pair< std::pair< T1 , T2 > > {
 static constexpr bool value = true;
 };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Output a single element of a container. A std::pair value is associated with
 * namespace std, so an unqualified operator<< is found there by ADL too; for a
 * std::pair the call is qualified to the SMS++ operator<< above to stay
 * unambiguous when a std-namespace operator<< for std::pair is in scope (as
 * injected, e.g., by some external libraries' logging headers). */

template< typename T >
std::ostream & out_el( std::ostream & os , const T & x ) {
 if constexpr( is_std_pair< T >::value )
  return( SMSpp_di_unipi_it::operator<<( os , x ) );
 else
  return( os << x );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template< typename T , std::size_t K >
std::ostream & operator<<( std::ostream & os ,
                           const boost::multi_array< T , K > & A ) {
 const T * p = A.data();
 for( boost::multi_array_types::size_type i = A.num_elements() ; i-- ; ++p ) {
  os << "[ ";
  for( boost::multi_array_types::size_type k = 0 ; k < K ; ) {
   os << ( p - A.origin() ) / A.strides()[ k ] % A.shape()[ k ]
         + A.index_bases()[ k ];
   if( ++k < K )
    os << ", ";
   }
  out_el( os << " ] = " , *p ) << std::endl;
 }

 return( os );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template< typename T , std::size_t K >
std::ostream & operator<<( std::ostream & os ,
                           const boost::multi_array< T * , K > & A ) {
 typedef T * TP;
 const TP * p = A.data();
 for( boost::multi_array_types::size_type i = A.num_elements() ; i-- ; ++p ) {
  os << "[ ";
  for( boost::multi_array_types::size_type k = 0 ; k < K ; ) {
   os << ( p - A.origin() ) / A.strides()[ k ] % A.shape()[ k ]
         + A.index_bases()[ k ];
   if( ++k < K )
    os << ", ";
   }
  out_el( os << " ] = " , **p ) << std::endl;
  }

 return( os );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template< typename T >
std::ostream & operator<<( std::ostream & os , const std::vector< T > & l ) {
 for( unsigned int i = 0 ; i < l.size() ; ++i )
  out_el( os << "[ " << i << " ] = " , l[ i ] ) << std::endl;

 return( os );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template< typename T >
std::ostream & operator<<( std::ostream & os , const std::vector< T * > & l ) {
 for( unsigned int i = 0 ; i < l.size() ; ++i )
  out_el( os << "[ " << i << " ] = " , *l[ i ] ) << std::endl;

 return( os );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template< typename T >
std::ostream & operator<<( std::ostream & os , const std::list< T > & l ) {
 auto it = l.begin();
 for( unsigned int i = 0 ; i < l.size() ; ++i , ++it )
  out_el( os << i << " ) = " , *it );

 return( os );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template< typename T >
std::ostream & operator<<( std::ostream & os , const std::list< T * > & l ) {
 auto it = l.begin();
 for( unsigned int i = 0 ; i < l.size() ; ++i , ++it )
  out_el( os << i << " ) = " , **it );

 return( os );
 }

/** @} ---------------------------------------------------------------------*/
/*----------------- LOADING things while skipping comments -----------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup eatcomments simple operator which eats up comments in a istream
 *  @{ */

inline std::istream & eatcomments( std::istream & is ) {
 for( ;; ) {
  if( is.eof() )  // never do any other reading if eof
   break;
  is >> std::ws;  // skip whitespaces
  if( is.eof() )  // never do any other reading if eof
   break;
  if( is.peek() == is.widen( '#' ) )
   // a comment: skip the rest of line and move to next
   is.ignore( std::numeric_limits< std::streamsize >::max() ,
              is.widen( '\n' ) );
  else
   break;
  }

 return( is );
 }

/** @} ---------------------------------------------------------------------*/
/*------------------ LOADING list, array and multi_array -------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup load_composite_data structures Loading lists and vectors
 *
 * A few versions of operator>> for loading pairs, lists and vectors out of
 * a std::istream. For pairs, the format is just
 *
 *     first element of the pair
 *     second element of the pair
 *
 * List and vectors can instead be either in dense or sparse format. That is,
 * the format always starts with a signed integer
 *
 *     +/- number of elements k
 *
 * If k == 0, the empty vector is returned. If k > 0, the vector has k
 * elements, and the format is the obvious "dense" one, i.e.,
 *
 *     for i = 1 to k
 *     - i-th element of the array/list
 *
 * If k < 0, instead, the vector/list will have abs( k ) elements. The format
 * is instead the "sparse" one, i.e., (after k)
 *
 *     number of non-default elements h (<= k)
 *     for i = 1 to h
 *     - index 0 <= j < k of the i-th non-default element
 *     - value of the i-th non-default element
 *
 *     IMPORTANT NOTE: THE VALUES OF THE INDICES MUST BE IN THE RIGHT RANGE,
 *     ORDERED IN INCREASING SENSE AND NOT REPLICATED
 *
 * It is assumed that each element can be read with >> itself, which rules
 * out pointers. Elements are separated by whitespaces and comments (see
 * eatcomments above). This creates a problem with dense vectors/lists of
 * std::strings, because there is no way in which one can have any of their
 * elements to be empty; if this is the case, use the sparse input instead. */

template< class T1 , class T2 >
std::istream & operator>>( std::istream & is , std::pair< T1 , T2 > & p ) {
 is >> eatcomments >> p.first;
 is >> eatcomments >> p.second;
 return( is );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// note the template wizardry in the third parameter which prevents the
// templated operator>> for "any" std container to clash with the predefined
// one for std:: string

template< template< class ... > class C , typename T ,
 typename std::enable_if< ! std::is_same< C< T > , std::string >::value
                                         >::type * = nullptr >
std::istream & operator>>( std::istream & is , C< T > & l ) {
 int k = 0;
 is >> eatcomments;
 if( ! is.eof() )
  is >> k;

 if( ! k ) {
  l.clear();
  return( is );
  }

 if( k > 0 ) {
  l.resize( k );
  for( auto & li : l )
   is >> eatcomments >> li;
  }
 else {
  l.resize( -k );
  unsigned int h;
  is >> eatcomments >> h;
  auto lit = l.begin();
  for( unsigned int p = 0 ; h-- ; ++p ) {
   unsigned int j;
   is >> eatcomments >> j;
   for( ; p < j ; ++p )
    *( lit++ ) = T();
   is >> eatcomments >> *( lit++ );
   }
  while( lit != l.end() )
   *( lit++ ) = T();
  }

 return( is );
 }

/** @} ---------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* SMSStreams.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SMSStreams.h --------------------------*/
/*--------------------------------------------------------------------------*/
