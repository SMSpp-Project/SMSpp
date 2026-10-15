/*--------------------------------------------------------------------------*/
/*---------------------------- File SMSnetCDF.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file with what SMS++ uses to read and write netCDF files: the
 * template functions that serialize and deserialize values, std::pair,
 * std::vector, matrices and boost::multi_array to/from the variables of a
 * netCDF::NcGroup, those that check its dimensions and variables, and the
 * class NcMemFile, a netCDF::NcFile whose content lives in memory rather
 * than on disk.
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
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato, Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ DEFINITIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __SMSnetCDF
 #define __SMSnetCDF
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

// standard C++ libraries (alphabetical order)
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <numeric>
#include <ostream>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// boost libraries
#include <boost/multi_array.hpp>

// NetCDF
#include <netcdf>

// SMS++ (Inf<>)
#include "SMSTypedefs.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/
/// namespace for the Structured Modeling System++ (SMS++)

namespace SMSpp_di_unipi_it
{
/*--------------------------------------------------------------------------*/
/*------------------ DE/SERIALIZING FROM/TO netCDF FILES -------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup helper functions for serializing/deserializing to/from netCDF
 *  files.
 *
 * The following functions are intended to help in the execution of typical
 * tasks that must be performed when serializing and deserializing objects.
 *  @{ */

template< typename T >
netCDF::NcType typ2nCDF( void ) { return( netCDF::NcOpaqueType() ); }

template<>
inline netCDF::NcType typ2nCDF< char >( void ) {
 return( netCDF::NcChar() );
 }

template<>
inline netCDF::NcType typ2nCDF< int >( void ) { return( netCDF::NcInt() ); }

template<>
inline netCDF::NcType typ2nCDF< double >( void ) {
 return( netCDF::NcDouble() );
 }

template<>
inline netCDF::NcType typ2nCDF< std::string >( void ) {
 return( netCDF::NcString() );
 }

template< class T >
inline constexpr bool is_netCDF_type_v =
 std::is_arithmetic_v< T > || std::is_enum_v< T > ||
 std::is_same_v< T , std::string >;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - - - SERIALIZING AND DESERIALIZING BASIC TYPES - - - - - - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// read the values of a netCDF variable, std::string ones included
/** Reads into \p data the values of the netCDF variable \p ncVar in the
 * hyperslab given by \p start and \p count, or all of them if \p count is
 * empty. For any T but std::string this is just NcVar::getVar(). For
 * T = std::string the variable is a netCDF::NcString one, for which the
 * netCDF library writes one char * per string, allocated by itself: these
 * are read into an array of char *, copied into the std::string of \p data
 * and given back to the library with nc_free_string(). Passing an array of
 * std::string to NcVar::getVar() would instead make the library write the
 * char * over the std::string objects.
 *
 * @param[in] ncVar The netCDF variable to be read.
 *
 * @param[out] data A pointer to the first of the elements that receive the
 *                  values, as many as the product of \p count (of the sizes
 *                  of the dimensions of \p ncVar if \p count is empty, 1 if
 *                  \p ncVar is a scalar).
 *
 * @param[in] start The index of the first value to be read in each
 *                  dimension, ignored if \p count is empty.
 *
 * @param[in] count The number of values to be read in each dimension; if
 *                  empty, the whole variable is read. */

template< class T >
void get_var_values( const netCDF::NcVar & ncVar , T * data ,
                     const std::vector< std::size_t > & start = {} ,
                     const std::vector< std::size_t > & count = {} ) {
 if constexpr( std::is_same_v< T , std::string > ) {
  std::size_t n = 1;
  if( count.empty() )
   for( const auto & dim : ncVar.getDims() )
    n *= dim.getSize();
  else
   for( auto c : count )
    n *= c;

  if( ! n )
   return;

  std::vector< char * > tmp( n , nullptr );
  if( count.empty() )
   ncVar.getVar( tmp.data() );
  else
   ncVar.getVar( start , count , tmp.data() );

  for( std::size_t i = 0 ; i < n ; ++i )
   data[ i ] = tmp[ i ] ? tmp[ i ] : "";

  nc_free_string( n , tmp.data() );
  }
 else {
  if( count.empty() )
   ncVar.getVar( data );
  else
   ncVar.getVar( start , count , data );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a simple value out of a given group
/** Deserialize a "simple" value, one for which NcGroup::getVar() is defined,
 * out of the given \p group and into \p data. This is supposed to live in
 * the netCDF variable with name \p name. If a variable with the given name
 * is not present within the given \p group, an exception is thrown unless
 * the variable is declared as optional.
 *
 * If \p group has more than one variable with the same name, then the
 * variable that is deserialized follows the rule defined by the netCDF method
 * NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens, then the
 * variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the value will be obtained
 *                  from.
 *
 * @param[out] data A reference to the object that will store the desired
 *                  value.
 *
 * @param[in] name The name of the variable within the given \p group that
 *                 contains the desired value, default "value".
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup and \p optional is
 *                     false, then an exception is thrown. Default is true.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< typename T >
std::enable_if_t< is_netCDF_type_v< T > , bool >
deserialize( const netCDF::NcGroup & group , T & data ,
             const std::string & name = "value" , bool optional = true ) {
 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( ! optional )
   throw( std::invalid_argument( "deserialize(): " + name +
                                 " not present in group " +
                                 group.getName() ) );
  return( false );
  }

 get_var_values( ncVar , &data );
 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a std::string out of a netCDF NcGroup
/** Deserialize a std::string value for which deserialize() is defined as a
 * double pointer to char out of the given \p group and into \p data. This is
 * supposed to live in the netCDF variable with name \p name.
 *
 * If the variable with the given name is not present within the given \p
 * group, an exception is thrown unless the variable is declared as optional.
 *
 * If \p group has more than one variable with the same name, then the
 * variable that is deserialized follows the rule defined by the netCDF method
 * NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens, then the
 * variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the value will be obtained
 *                  from.
 *
 * @param[out] data A reference to the std::string object that will store the
 *                  desired value.
 *
 * @param[in] name The name of the variable within the given \p group that
 *                 contains the desired value, default "value".
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup and \p optional is
 *                     false, then an exception is thrown. Default is true.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

inline bool deserialize( const netCDF::NcGroup & group , std::string & data ,
                         const std::string & name = "value" ,
                         bool optional = true ) {
 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( ! optional )
   throw( std::invalid_argument( "deserialize(): " + name +
                                 " not present in group " +
                                 group.getName() ) );
  return( false );
  }

 char * data_tmp;
 ncVar.getVar( &data_tmp );
 data = std::string( data_tmp );
 free( data_tmp );
 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a std::vector< std::string > out of a netCDF NcGroup
/** This function reads a std::vector of std::string from a netCDF variable
 * with name \p name within the given netCDF NcGroup \p group. The values
 * read are stored in the given std::vector \p data.
 *
 * The variable is expected to have a single dimension and contain C-style
 * strings (char*). For each element, the corresponding C-string is converted
 * to std::string and freed after use.
 *
 * If the variable is not present in the given \p group, then the vector \p
 * data is resized to zero if \p optional == true, while an
 * std::invalid_argument exception is thrown if \p optional == false.
 *
 * @param[in] group The netCDF NcGroup from which the array will be obtained.
 *
 * @param[in] name  The name of the variable within the given group.
 *
 * @param[in] size  The expected size of the variable. If this is not
 *                  Inf< Index >(), the function checks that the variable's
 *                  size matches the given size.
 *
 * @param[out] data A reference to the vector that will store the values.
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. If the variable is not present and this
 *                     is false, an exception is thrown. Default is true.
 *
 * @return true if the desired variable was deserialized; false otherwise. */
inline bool deserialize( const netCDF::NcGroup & group ,
                         const std::string & name ,
                         std::size_t size ,
                         std::vector< std::string > & data ,
                         bool optional = true ) {
 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( optional ) {
   data.clear();
   return( false );
   }
  throw( std::invalid_argument( "deserialize(): variable " + name +
                                " not present in group " + group.getName() ) );
  }

 if( ncVar.getDimCount() != 1 )
  throw( std::logic_error( "deserialize(): variable " + name +
                           " must have dimension 1" ) );

 std::size_t var_size = ncVar.getDim( 0 ).getSize();
 if( ( size != Inf< std::size_t >() ) && ( var_size != size ) )
  throw( std::logic_error( "deserialize(): variable " + name +
                           " expected size " + std::to_string( size ) +
                           ", got " + std::to_string( var_size ) ) );

 std::vector< char * > data_tmp( var_size );
 ncVar.getVar( data_tmp.data() );

 data.resize( var_size );
 std::transform( data_tmp.begin() , data_tmp.end() , data.begin() ,
                 []( char * s ) { return( std::string( s ) ); } );

 for( char * s : data_tmp )
  free( s );

 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a std::vector< std::string > into a netCDF NcGroup
/** This function writes a std::vector of std::string into a netCDF variable
 * with name \p name within the given netCDF NcGroup \p group, as the
 * one-dimensional variable of type netCDF::NcString over the dimension \p
 * ncDim that deserialize( group , name , size , std::vector< std::string > )
 * reads back. An empty \p data is not serialized.
 *
 * @param[in, out] group The netCDF NcGroup in which the variable is added.
 *
 * @param[in] name  The name of the variable that will be added.
 *
 * @param[in] ncDim The netCDF dimension of the variable.
 *
 * @param[in] data  The vector of strings to be stored in the variable. */
inline void serialize( netCDF::NcGroup & group ,
                       const std::string & name ,
                       const netCDF::NcDim & ncDim ,
                       const std::vector< std::string > & data ) {
 if( data.empty() )
  return;  // nothing to be serialized

 auto ncVar = group.addVar( name , netCDF::NcString() , ncDim );
 for( std::size_t i = 0 ; i < data.size() ; ++i )
  ncVar.putVar( { i } , data[ i ] );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a single (scalar) variable into a netCDF NcGroup
/** Serialize a "simple" value, one for which NcGroup::putVar() is defined,
 * out of \p data and into of the variable with name \p name of the given
 * \p group. Note that this can be a std::string, so it's not necessarily
 * a "small" value (which is why it is passed by reference).
 *
 * @param[in, out] group The netCDF NcGroup in which the variable will be
 *                       added.
 *
 * @param[in] name       The name of the variable that will be added.
 *
 * @param[in] ncType     The netCDF type of the variable to be added.
 *
 * @param[in] data       A reference to the value to be written into the
 *                       variable. */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const std::string & name ,
           const netCDF::NcType & ncType , const T & data ) {
 ( group.addVar( name , ncType ) ).putVar( &data );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a single (scalar) std::string into a netCDF NcGroup
/** Specialization of the scalar serialize() for std::string. A netCDF
 * NcString variable is backed by a variable-length string, so putVar()
 * expects (the address of) an array of C-strings, i.e. a char ** : the
 * netCDF/HDF5 layer dereferences the buffer as a char * and then calls
 * strlen() on it. The generic template above would instead pass the address
 * of the std::string object itself, so HDF5 would read the first bytes of the
 * std::string's internal representation as a char * and crash. This overload
 * (preferred over the template for std::string arguments) passes the address
 * of the C-string pointer, mirroring the deserialize( group , std::string & )
 * overload that reads via a char ** as well. */

inline void
serialize( netCDF::NcGroup & group , const std::string & name ,
           const netCDF::NcType & ncType , const std::string & data ) {
 const char * c_str = data.c_str();
 ( group.addVar( name , ncType ) ).putVar( &c_str );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a single (scalar) variable into a netCDF NcGroup
/** Serialize a "simple" value, one for which NcGroup::putVar() is defined,
 * out of \p data and into of the variable with name \p name of the given
 * \p group. Note that this can be a std::string, so it's not necessarily
 * a "small" value (which is why it is passed by reference).
 *
 * @param[in, out] group The netCDF NcGroup in which the variable will be
 *                       added.
 *
 * @param[in] data       A reference to the value to be written into the
 *                       variable
 *
 * @param[in] name       The name of the variable that will be added,
 *                       default "value". */

template< typename T >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const T & data ,
           const std::string & name = "value" ) {
 serialize( group , name , typ2nCDF< T >() , data );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a std::pair value out of a netCDF NcGroup
/** Deserialize a std::pair of values for which deserialize() is well-defined
 * (e.g., "simple" values) out of the given \p group and into \p data. This
 * is done by calling deserialize() for data.first on "<name>_f" and for
 * data.second on"<name>_s".
 *
 * @param[in] group The netCDF NcGroup from which the value will be obtained
 *                  from.
 *
 * @param[out] data A reference to the std::pair that will store the desired
 *                  value.
 *
 * @param[in] name The initial part of the name of the two variables within
 *                 the given \p group that contains the desired values,
 *                 default "value".
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the any of the two
 *                     variables is not present in the given NcGroup and
 *                     \p optional == false, then an exception is thrown.
 *                     Default is true.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< typename T1 , typename T2 >
std::enable_if_t< is_netCDF_type_v< T1 > && is_netCDF_type_v< T2 > , bool >
deserialize( const netCDF::NcGroup & group , std::pair< T1 , T2 > & data ,
             const std::string & name = "value" , bool optional = true ) {
 bool did_it = deserialize( group , data.first , name + "_f" , optional );
 return( deserialize( group , data.second , name + "_s" , optional ) &&
         did_it );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a std::pair value into a given group
/** Serialize a std::pair of values for which deserialize() is well-defined
 * (e.g., "simple" values) out of \p data and into of the two variables with
 * name "<name>_f" and "<name>_s" of the given \p group. */

template< typename T1 , typename T2 >
std::enable_if_t< is_netCDF_type_v< T1 > && is_netCDF_type_v< T2 > , void >
serialize( netCDF::NcGroup & group , const std::pair< T1 , T2 > & data ,
           const std::string & name = "value" ) {
 serialize( group , data.first , name + "_f" );
 serialize( group , data.second , name + "_s" );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - - - - - - - - - -DESERIALIZING DIMENSIONS - - - - - - - - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a dimension out of a netCDF NcGroup
/** If the given netCDF NcGroup contains a dimension whose name is \p
 * dim_name, then its size is stored in \p data. If the group does not contain
 * a dimension with the \p dim_name name and the value of the parameter \p
 * optional is false, then an std::invalid_argument exception is thrown.
 *
 * If the given \p group has more than one dimension with the same name, then
 * the dimension that is considered follows the rule defined by the netCDF
 * method NcGroup::getDim(). As of version 4.3.1 of netCDF, if this happens,
 * then the dimension closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the dimension will be
 *                  obtained from.
 *
 * @param[in]  name A string with the name of the dimension.
 *
 * @param[out] data A reference to the object that will store the size of the
 *                  desired dimension; it's template so as to allow the user
 *                  to read it with different types of integers than
 *                  std::size_t.
 *
 * @param[in] optional This parameter informs whether the dimension is
 *                     optional. This means that if the dimension is not
 *                     present in the given NcGroup and \p optional ==
 *                     false, then an exception is thrown. Default is true.
 *
 * @return true if the desired dimension was deserialized; false, otherwise. */

template< typename T >
inline bool deserialize_dim( const netCDF::NcGroup & group ,
                             const std::string & name , T & data ,
                             bool optional = true ) {
 netCDF::NcDim ncDim = group.getDim( name );
 if( ncDim.isNull() ) {
  if( optional )
   return( false );
  throw( std::invalid_argument( "deserialize_dim(): " + name +
                                " not present in group " + group.getName() ) );
  }

 data = ncDim.getSize();
 return( true );
 }

/*--------------------------------------------------------------------------*/
/// return the sizes of the dimensions of a netCDF variable
/** This function receives a netCDF variable and returns a vector whose size
 * is the number of dimensions of the given variable. The i-th position of
 * this vector stores the size of the i-th dimension of the given netCDF
 * variable.
 *
 * @param[in] var The netCDF variable from which the sizes of the dimensions
 *                will be extracted.
 *
 * @return A vector with the sizes of the dimensions of the variable var. */

inline std::vector< std::size_t > get_sizes_dimensions(
 const netCDF::NcVar & var ) {
 std::vector< std::size_t > sizes_dimensions( var.getDimCount() );
 std::vector< std::size_t >::size_type i = 0;
 for( const auto & dim : var.getDims() )
  sizes_dimensions[ i++ ] = dim.getSize();
 return( sizes_dimensions );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - -SERIALIZING AND DESERIALIZING std::vector OF BASIC TYPES - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a std::vector out of a netCDF NcGroup
/** This function reads a std::vector of "simple" values of type \p T, for
 * which NcGroup::getVar() is defined, from a netCDF variable with name
 * \p name within the given netCDF NcGroup \p group. The values read are
 * stored in the given std::vector \p data.
 *
 * If the variable is not present in the given \p group, then the vector \p
 * data is resized to zero if \p optional == true, while an
 * std::invalid_argument exception is thrown if \p optional == false.
 *
 * If the given \p group has more than one variable with the same name, then
 * the variable that is considered follows the rule defined by the netCDF
 * method NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens,
 * then the variable closest to the given group is considered.
 *
 * If the variable in the file is a scalar (i.e., it has zero dimensions),
 * and \p allow_scalar_var is true, then the entire \p data vector will be
 * filled with the scalar value, repeated \p size times.
 *
 * This function also supports post-processing to decompress the vector using
 * change intervals. This applies when the vector size is smaller than the
 * requested \p size and matches the number of change intervals. In such a
 * case, the vector is expanded using the change interval logic (see below).
 *
 * @param[in] group The netCDF NcGroup from which the array will be obtained
 *                  from.
 *
 * @param[in] name  The name of the variable within the given group.
 *
 * @param[in] size  The size of the array to be read; note that this is
 *                  *not* checked against the actual size of the variable,
 *                  so it's the caller's responsibility to ensure they match.
 *                  If size == 0, data is clear()-ed and false is returned
 *                  with no other check (not even that the variable exists).
 *
 * @param[out] data A reference to the vector that will store the values of
 *                  the array.
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup and \p optional is false,
 *                     an exception is thrown. Default is true.
 *
 * @param[in] allow_scalar_var This parameter indicates whether the desired
 *                             variable (whose name is \p name) can have
 *                             a zero number of dimensions, i.e., it can be
 *           a scalar instead of an array. Its default value is false, and this
 *           means it is a scalar then an exception is thrown. If, instead,
 *           \p allow_scalar_var == true, then if the netCDF variable is a
 *           scalar, then \p data is resized to 1, and the value of the netCDF
 *           variable is stored in the first (and only) position of \p data (in
 *           this case, \p size is ignored).
 *
 * @param[in] change_intervals The list of time intervals used to decompress
 *                             time-series data. If the variable contains
 *                             fewer than \p size elements and the number of
 *           elements matches the number of \p change_intervals, then each
 *           value will be expanded over the corresponding interval.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

// if T is enum -> underlying_type_t< T >, else -> T
template< class T , bool = std::is_enum_v< T > >
struct _nc_read_type { using type = T; };

template< class T >
struct _nc_read_type< T , true > { using type = std::underlying_type_t< T >; };

template< class T >
using read_type_t = typename _nc_read_type< T >::type;

template< class T >
std::enable_if_t< is_netCDF_type_v< read_type_t< T > > , bool >
deserialize( const netCDF::NcGroup & group , const std::string & name ,
             const std::size_t size , std::vector< T > & data ,
             bool optional = true , bool allow_scalar_var = false ,
             const std::vector< std::size_t > & change_intervals = {} ) {
 if( ! size ) {
  data.clear();
  return( false );
  }

 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( optional ) {
   data.clear();
   return( false );
   }

  throw( std::invalid_argument( "deserialize(): " + name +
                                " not present in group " + group.getName() ) );
  }

 auto dc = ncVar.getDimCount();
 if( ( ( dc == 0 ) && ( ! allow_scalar_var ) ) || ( dc > 1 ) )
  throw( std::invalid_argument( "deserialize(): netCDF variable " +
                                name + " of group " + group.getName() +
                                " has " + std::to_string( dc ) +
                                " != 1 dimensions" ) );

 using R = read_type_t< T >;
 std::vector< R > buf;

 if( dc == 0 ) {
  R value;
  get_var_values( ncVar , &value );
  buf.assign( size , value );
  }

 else {
  buf.resize( size );
  get_var_values( ncVar , buf.data() , { 0 } , { size } );
  }

 // Apply decompression if requested
 if( ! change_intervals.empty() ) {
  if( buf.empty() )
   return( true );

  if( buf.size() == 1 ) {
   // Fill with constant value
   buf.resize( size , buf[ 0 ] );
   }
  else if( buf.size() < size ) {
   // Must match change_intervals
   if( buf.size() != change_intervals.size() ) {
    throw( std::logic_error(
     "deserialize(): invalid number of elements (" +
     std::to_string( buf.size() ) + ") for variable " + name +
     ". It should be equal to the number of change intervals (" +
     std::to_string( change_intervals.size() ) + ")" ) );
    }

   std::vector< R > given_vector = buf;
   buf.resize( size );

   auto t = 0;
   for( auto k = 0 ; k < change_intervals.size() ; ++k ) {
    auto upper_endpoint = change_intervals[ k ];
    if( k == change_intervals.size() - 1 )
     upper_endpoint = size - 1;

    for( ; t <= upper_endpoint ; ++t )
     buf[ t ] = given_vector[ k ];
    }
   }
  }

 // Cast back to T if enum, else copy
 data.resize( buf.size() );
 if constexpr( std::is_enum_v< T > ) {
  std::transform( buf.begin() , buf.end() , data.begin() ,
                  []( const R & v ) { return( static_cast< T >( v ) ); } );
  }
 else {
  std::copy( buf.begin() , buf.end() , data.begin() );
  }

 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a std::vector out of a netCDF NcGroup
/** This function reads a std::vector of "simple" values of type \p T, for
 * which NcGroup::getVar() is defined, from a netCDF variable with name
 * \p name within the given netCDF NcGroup \p group. The values read are
 * stored in the given std::vector \p data.
 *
 * If the variable is not present in the given \p group, then the vector \p
 * data is resized to zero if \p optional == true, while an
 * std::invalid_argument exception is thrown if \p optional == false.
 *
 * If the given \p group has more than one variable with the same name, then
 * the variable that is considered follows the rule defined by the netCDF
 * method NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens,
 * then the variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the array will be obtained
 *                  from.
 *
 * @param[out] data A reference to the vector that will store the values of
 *                  the array.
 *
 * @param[in] name  The name of the variable within the given group. The
 *                  default is "value".
 *
 * @param[in] size  The name of the dimension of the variable within the
 *                  given group. If there is no dimension with that name
 *                  and \p optional == false then an exception is thrown,
 *                  while if \p optional == true then data is clear()-ed and
 *                  false is returned with no other check (not even that the
 *                  variable exists). The default is "size".
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup and \p optional == false
 *                     then an exception is thrown. The default is true.
 *
 * @param[in] allow_scalar_var This parameter indicates whether the desired
 *                             variable (whose name is \p name) can have
 *                             a zero number of dimensions, i.e., it can be
 *           a scalar instead of an array. Its default value is false, and this
 *           means it is a scalar then an exception is thrown. If, instead, \p
 *           allow_scalar_var == true, then if the netCDF variable is a scalar,
 *           then \p data is resized to 1, and the value of the netCDF variable
 *           is stored in the first (and only) position of \p data (in this
 *           case, \p size is ignored).
 *
 * @param[in] change_intervals The list of time intervals used to decompress
 *                             time-series data. If the variable contains
 *                             fewer than \p size elements and the number of
 *           elements matches the number of \p change_intervals, then each
 *           value will be expanded over the corresponding interval.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< typename T >
std::enable_if_t< is_netCDF_type_v< T > , bool >
deserialize( const netCDF::NcGroup & group , std::vector< T > & data ,
             const std::string & name = "value" ,
             const std::string & size = "size" ,
             bool optional = true , bool allow_scalar_var = false ,
             const std::vector< std::size_t > & change_intervals = {} ) {
 auto dim = group.getDim( size );
 if( dim.isNull() ) {
  if( optional ) {
   data.clear();
   return( false );
   }

  throw( std::invalid_argument( "deserialize(): dimension " + size +
                                 " is not present" ) );
  }

 return( deserialize( group , name , dim.getSize() , data , optional ,
                      allow_scalar_var , change_intervals ) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a std::vector into a given group
/** serialize a std::vector of "simple" values, for which NcGroup::putVar()
 * is defined out of \p data and into the variable with name \p name of
 * netCDF type \p ncType with dimension \p ncDim. If \p data is empty,
 * \p group is left unchanged (no netCDF variable is created).
 *
 * @param[in, out] group The netCDF NcGroup in which the variable will be
 *                       added.
 *
 * @param[in] name       The name of the variable that will be added.
 *
 * @param[in] ncType     The type of the elements of the array.
 *
 * @param[in] ncDim      The netCDF dimension of the array.
 *
 * @param[in] data       A vector containing the data to be stored in the
 *                       variable.
 *
 * @param[in] allow_scalar_var Although this function is supposed to serialize
 *                             an array, it can also be used to serialize a
 *                             scalar. If the \p data has size 1 and \p
 *            allow_scalar_var == true, then a netCDF scalar variable is
 *            created instead of a multi-dimensional one (notice that, in
 *            this case, the argument \p ncDim is completely ignored). The
 *            default is false. */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const std::string & name ,
           const netCDF::NcType & ncType , const netCDF::NcDim & ncDim ,
           const std::vector< T > & data , bool allow_scalar_var = false ) {
 if( data.empty() )
  return; // Nothing to be serialized.

 if( allow_scalar_var && ( data.size() == 1 ) ) {
  serialize( group , name , ncType , data[ 0 ] );
  return;
  }

 group.addVar( name , ncType , ncDim ).putVar( { 0 } , { data.size() } ,
                                               data.data() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/** serialize a std::vector of "simple" values, for which NcGroup::putVar()
 * is defined out of \p data and into the variable with name \p name of
 * netCDF type \p ncType with dimension with name \p size. If a dimension
 * with name \p size is already present in \p group then it is used,
 * otherwise it is added. If \p data is empty, \p group is left unchanged
 * (no netCDF variable is created) except possibly for adding \p size.
 *
 * @param[in, out] group The netCDF NcGroup in which the variable will be
 *                       added.
 *
 * @param[in] data       A vector containing the data to be stored in the
 *                       variable.
 *
 * @param[in] name       The name of the variable that will be added,
 *                       default "value".
 *
 * @param[in] size       The name of the dimension of the array, default
 *                       "size".
 *
 * @param[in] allow_scalar_var Although this function is supposed to serialize
 *                             an array, it can also be used to serialize a
 *                             scalar. If the \p data has size 1 and
 *           \p allow_scalar_var == true, then a netCDF scalar variable is
 *           created instead of a multi-dimensional one (notice that, in this
 *           case, the argument \p ncDim is completely ignored).
 *           The default is false. */

template< typename T >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const std::vector< T > & data ,
           const std::string & name = "value" ,
           const std::string & size = "size" ,
           bool allow_scalar_var = false ) {
 auto sz = group.getDim( size );
 if( sz.isNull() )
  sz = group.addDim( size , data.size() );

 serialize( group , name , typ2nCDF< T >() , sz , data , allow_scalar_var );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - -SERIALIZING AND DESERIALIZING std::vector OF std::pair - - - - - -*/
/*- - - - OF BASIC TYPES- - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a std::vector< std::pair > > out of a netCDF NcGroup
/** This function reads a std::vector< std::pair< T1 , T2 > > where both T1
 * and T2 are "simple" types (for which NcGroup::getVar() is defined) from
 * a netCDF variable with name \p name within the given netCDF NcGroup
 * \p group. The values read are stored in the given
 * std::vector< std::pair >\p data.
 *
 * If the variable is not present in the given \p group, then the vector \p
 * data is resized to zero if \p optional == true, while an
 * std::invalid_argument exception is thrown if \p optional == false.
 *
 * If the given \p group has more than one variable with the same name, then
 * the variable that is considered follows the rule defined by the netCDF
 * method NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens,
 * then the variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the array will be obtained
 *                  from.
 *
 * @param[in] name  For efficiency, the data is assumed to be stored in
 *                  \p group under the form of *two* one-dimensional
 *                  variables of name \p name + "_f" and \p name + "_s",
 *                  of type T1 and T2 and indexed on the same dimension, with
 *                  the obvious format. If any of the two variables is not
 *                  present and \p optional == true then data is clear()-ed
 *                  and false returned, otherwise exception is throw.
 *
 * @param[in] size  The size of the array to be read; note that this is
 *                  *not* checked against the actual size of the variables,
 *                  so it's the caller's responsibility to ensure they match.
 *                  If size == 0, data is clear()-ed and false is returned
 *                  with no other check (not even that the variable exists).
 *
 * @param[out] data A reference to the vector that will store the values of
 *                  the array.
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if any of the two required
 *                     variables is not present in the given NcGroup and
 *                     \p optional is false, an exception is thrown.
 *                     Default is true.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< typename T1 , typename T2 >
std::enable_if_t< is_netCDF_type_v< T1 > && is_netCDF_type_v< T2 > , bool >
deserialize( const netCDF::NcGroup & group , const std::string & name ,
             const std::size_t & size ,
             std::vector< std::pair< T1 , T2 > > & data ,
             bool optional = true ) {
 if( ! size ) {
  data.clear();
  return( false );
  }

 std::vector< T1 > f;
 if( ! deserialize( group , name + "_f" , size , f , optional ) ) {
  data.clear();
  return( false );
  }

 std::vector< T2 > s;
 if( ! deserialize( group , name + "_s" , size , s , optional ) ) {
  data.clear();
  return( false );
  }

 data.resize( size );

 auto fit = f.begin();
 auto sit = s.begin();
 for( auto & el : data )
  el = std::pair( *( fit++ ) , *( sit++ ) );

 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a std::vector< std::pair > > out of a netCDF NcGroup
/** This function reads a std::vector< std::pair< T1 , T2 > > where both T1
 * and T2 are "simple" types (for which NcGroup::getVar() is defined) from
 * a netCDF variable with name \p name within the given netCDF NcGroup
 * \p group. The values read are stored in the given
 * std::vector< std::pair >\p data.
 *
 * If the variable is not present in the given \p group, then the vector \p
 * data is resized to zero if \p optional == true, while an
 * std::invalid_argument exception is thrown if \p optional == false.
 *
 * If the given \p group has more than one variable with the same name, then
 * the variable that is considered follows the rule defined by the netCDF
 * method NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens,
 * then the variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the array will be obtained
 *                  from.
 *
 * @param[out] data A reference to the std::vector< std::pair< T1 , T2 > >
 *                  that will store the values.
 *
 * @param[in] name  For efficiency, the data is assumed to be stored in
 *                  \p group under the form of *two* one-dimensional
 *                  variables of name \p name + "_f" and \p name + "_s",
 *                  of type T1 and T2 and indexed on the same dimension, with
 *                  the obvious format. If any of the two variables is not
 *                  present and \p optional == true then data is clear()-ed
 *                  and false returned, otherwise exception is throw.
 *                  The default is "value".
 *
 * @param[in] size  The name of the dimension of the variable within the
 *                  given group. If there is no dimension with that name
 *                  and \p optional == false then an exception is thrown,
 *                  while if \p optional == true then data is clear()-ed and
 *                  false is returned with no other check (not even that the
 *                  variable exists). The default is "size".
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup and \p optional == false
 *                     then an exception is thrown. The default is true.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< typename T1 , typename T2 >
std::enable_if_t< is_netCDF_type_v< T1 > && is_netCDF_type_v< T2 > , bool >
deserialize( const netCDF::NcGroup & group ,
             std::vector< std::pair< T1 , T2 > > & data ,
             const std::string & name = "value" ,
             const std::string & size = "size" , bool optional = true ) {
 auto dim = group.getDim( size );
 if( dim.isNull() ) {
  if( optional ) {
   data.clear();
   return( false );
   }

  throw( std::invalid_argument( "deserialize(): dimension " + size +
                                 " is not present" ) );
  }

 return( deserialize( group , name , dim.getSize() , data , optional ) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a std::vector< std::pair > > into a given group
/** serialize a std::vector< std::pair< T1 , T2 > > where both T1 and T2 are
 * "simple" types (for which NcGroup::getVar() is defined) out of \p data and
 * into two variable with name \p name + "_f" and \p name + "_s", both of the
 * same dimension \p ncDim. If \p data is empty, \p group is left unchanged
 * (no netCDF variable is created).
 *
 * @param[in, out] group The netCDF NcGroup in which the variable will be
 *                       added.
 *
 * @param[in] name       The common part of the names of the two variables
 *                       that will be added.
 *
 * @param[in] ncDim      The netCDF dimension of the array.
 *
 * @param[in] data       A vector containing the data to be stored in the
 *                       variable. */

template< typename T1 , typename T2 >
std::enable_if_t< is_netCDF_type_v< T1 > && is_netCDF_type_v< T2 > , void >
serialize( netCDF::NcGroup & group , const std::string & name ,
           const netCDF::NcDim & ncDim ,
           const std::vector< std::pair< T1 , T2 > > & data ) {
 if( data.empty() )
  return;  // nothing to be serialized

 auto size = ncDim.getSize();
 std::vector< T1 > f( size );
 std::vector< T2 > s( size );

 auto fit = f.begin();
 auto sit = s.begin();
 for( auto & el : data ) {
  *( fit++ ) = el.first;
  *( sit++ ) = el.second;
  }

 serialize( group , name + "_f" , typ2nCDF< T1 >() , ncDim , f );
 serialize( group , name + "_s" , typ2nCDF< T2 >() , ncDim , s );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a std::vector< std::pair > > into a given group
/** serialize a std::vector< std::pair< T1 , T2 > > where both T1 and T2 are
 * "simple" types (for which NcGroup::getVar() is defined) out of \p data and
 * into two variable with name \p name + "_f" and \p name + "_s", both of the
 * same dimension whose name is \p size. If \p data is empty, \p group is
 * left unchanged (no netCDF variable is created).
 *
 * @param[in, out] group The netCDF NcGroup in which the variables will be
 *                       added.
 *
 * @param[in] data       A std::vector< std::pair< T1 , T2 > > containing the
 *                       data to be stored
 *
 * @param[in] name       The common part of the names of the two variables
 *                       that will be added, default "value".
 *
 * @param[in] size       The name of the dimension of the array, default
 *                       "size". */

template< typename T1 , typename T2 >
std::enable_if_t< is_netCDF_type_v< T1 > && is_netCDF_type_v< T2 > , void >
serialize( netCDF::NcGroup & group ,
           const std::vector< std::pair< T1 , T2 > > & data ,
           const std::string & name = "value" ,
           const std::string & size = "size" ) {
 auto sz = group.getDim( size );
 if( sz.isNull() )
  sz = group.addDim( size , data.size() );

 serialize( group , name , sz , data );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - DESERIALIZING MULTI-DIMENSIONAL VARIABLES OF BASIC TYPES- - - - -*/
/*- - - - STORED ROW-MAJOR INTO A SINGLE std::vector- - - - - - - - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a multi-dimensional variable out of a netCDF NcGroup
/** This function reads a multi-dimensional array of values of type \p T from
 * a netCDF variable with name \p name within the given netCDF NcGroup \p
 * group. The number of dimensions of the multi-dimensional array is given by
 * the size of the vector \p sizes and the i-th entry of \p sizes provides the
 * size of the i-th dimension. The values read are stored in the given vector
 * \p data in row-major layout.
 *
 * If the size of any dimension is zero, then \p data is resized to zero. If
 * the variable is not present in the given \p group, then the vector \p data
 * is resized to zero if the value of the parameter \p optional is true or an
 * std::invalid_argument exception is thrown if \p optional is false.
 *
 * If the given \p group has more than one variable with the same name, then
 * the variable that is considered follows the rule defined by the netCDF
 * method NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens,
 * then the variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the array will be obtained
 *                  from.
 *
 * @param[in] name  The name of the variable within the given \p group.
 *
 * @param[in] sizes A vector containing the sizes of each dimension of the
 *                  multi-dimensional array.
 *
 * @param[out] data A reference to the vector that will store the
 *                  multi-dimensional array in row-major layout.
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup and \p optional is
 *                     false, an exception is thrown.
 *
 * @param[in] allow_scalar_var This parameter indicates whether the desired
 *                             variable (whose name is \p name) can have
 *                             dimension zero (i.e., it could be a scalar
 *            instead of an array). Its default value is false, and this
 *            means that if the size of the given \p sizes vector is not
 *            the same as the number of dimensions of the netCDF variable or
 *            the sizes of the dimensions specified by \p sizes do not match
 *            that of the netCDF variable, an exception is thrown. If \p
 *            allow_scalar_var is true, this means that if the netCDF
 *            variable has dimension zero (i.e., it is a scalar), then the
 *            given vector \p data is resized to 1 and the value of the
 *            netCDF variable is stored in the first position of \p data
 *            (notice that, in this case, the given vector \p sizes is
 *            completely ignored).
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , bool >
deserialize( const netCDF::NcGroup & group , const std::string & name ,
             const std::vector< std::size_t > & sizes ,
             std::vector< T > & data , bool optional = true ,
             bool allow_scalar_var = false ) {
 if( sizes.empty() ) {
  data.resize( 0 );
  return( false );
  }

 auto total_size = std::accumulate( begin( sizes ) , end( sizes ) , 1 ,
                                    std::multiplies< std::size_t >() );
 if( total_size == 0 ) {
  data.resize( 0 );
  return( false );
  }

 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( optional ) {
   data.resize( 0 );
   return( false );
  }

  throw( std::invalid_argument( "deserialize(): " + name +
                                " not present in group " + group.getName() ) );
  }

 if( ( ( ncVar.getDimCount() < 0 ) ||
       ( sizes.size() != static_cast< decltype( sizes.size() ) >(
        ncVar.getDimCount() ) ) ) &&
     ( ( ncVar.getDimCount() != 0 ) || ( ! allow_scalar_var ) ) )
  throw( std::invalid_argument( "deserialize(): netCDF variable " +
                                 name + " of group " + group.getName() +
                                 " has " +
                                 std::to_string( ncVar.getDimCount() ) +
                                 " dimension(s), but provided argument has " +
                                 std::to_string( sizes.size() ) +
                                 " dimension(s)" ) );

 if( ncVar.getDimCount() == 0 ) {
  data.resize( 1 );
  get_var_values( ncVar , &data[ 0 ] );
  return( true );
  }

 auto var_sizes = get_sizes_dimensions( ncVar );

 if( sizes != var_sizes )
  throw( std::invalid_argument(
   "deserialize(): given sizes of dimensions and the sizes of "
   "dimensions of netCDF variable " + name + " of group " +
   group.getName() + " do not match" ) );

 data.resize( total_size );

 std::vector< std::size_t > start( sizes.size() , 0 );

 get_var_values( ncVar , data.data() , start , sizes );

 return( true );
 }

/*--------------------------------------------------------------------------*/
/// deserialize a multi-dimensional variable out of a netCDF NcGroup
/** This function reads a multi-dimensional array of values of type \p T from
 * a netCDF variable with name \p name within the given netCDF NcGroup \p
 * group. The values that are read are stored in the given vector \p data in
 * row-major layout.
 *
 * If the netCDF variable has no dimension (i.e., it is a scalar variable),
 * then \p data is resized to 1 and its only element receives the value of the
 * netCDF variable.
 *
 * If the netCDF variable has at least one dimension and the size of any of
 * its dimension is zero, then \p data is resized to zero.
 *
 * If the variable is not present in the given \p group, then the vector \p
 * data is resized to zero if the value of the parameter \p optional is true
 * or an std::invalid_argument exception is thrown if \p optional is false.
 *
 * If the given \p group has more than one variable with the same name, then
 * the variable that is considered follows the rule defined by the netCDF
 * method NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens,
 * then the variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the array will be obtained
 *                  from.
 *
 * @param[in] name  The name of the variable within the given \p group.
 *
 * @param[out] data A reference to the vector that will store the
 *                  multi-dimensional array in row-major layout.
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup and \p optional is false,
 *                     an exception is thrown.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , bool >
deserialize( const netCDF::NcGroup & group , const std::string & name ,
             std::vector< T > & data , bool optional = true ) {
 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( optional ) {
   data.resize( 0 );
   return( false );
   }

  throw( std::invalid_argument( "deserialize(): " + name +
                                " not present in group " + group.getName() ) );
  }

 auto sizes_dimensions = get_sizes_dimensions( ncVar );
 if( sizes_dimensions.empty() ) {
  // The variable is a scalar one.
  data.resize( 1 );
  get_var_values( ncVar , data.data() );
  return( true );
  }

 auto total_size = std::accumulate( begin( sizes_dimensions ) ,
                                    end( sizes_dimensions ) , 1 ,
                                    std::multiplies< std::size_t >() );
 if( total_size == 0 ) {
  data.resize( 0 );
  return( false );
  }

 data.resize( total_size );

 std::vector< std::size_t > start( sizes_dimensions.size() , 0 );

 get_var_values( ncVar , data.data() , start , sizes_dimensions );

 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - SERIALIZING AND DESERIALIZING BI-DIMENSIONAL VARIABLES OF - - - -*/
/*- - - - BASIC TYPES WITH VARIABLE-LENGTH ROWS, I.E.,- - - - - - - - - - -*/
/*- - - - std::vector< std::vector< T > > - - - - - - - - - - - - - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a matrix with variable-length rows
/** This function reads a matrix with variable-length rows, i.e., a
 * std::vector< std::vector< T > >. This is obtained in the netCDF by having
 * two distinct netCDF::NcVars: a one-dimensional T array val[] (whose name is
 * \p name), and a one-dimensional int array start[] (whose name is \p
 * start_name). The length of start[] gives the number of rows in \p array;
 * the elements of start[] are supposed to be non-negative, ordered in
 * non-decreasing sense, and smaller than the number of elements of val[].
 * Then, the i-th row of \p array will contain the elements found in val[]
 * with indices in the closed-open interval [ start[ i ] , start[ i + 1 ] ),
 * except for the last row for which start[ i + 1 ] is undefined and the total
 * number of elements in val[] is used instead. Note that one typically expect
 * start[ 0 ] == 0, but this is not enforced for the slight chance that this
 * may be useful to the user; clearly, the elements of val[] before start[ 0 ]
 * are ignored.
 *
 * @param[in] group The netCDF::NcGroup from which the matrix with
 *                  variable-length rows will be obtained.
 *
 * @param[in] name  The name of the one-dimensional netCDF::NcVar of type
 *                  T within the given \p group in which the values to be
 *                  stored in a 1D array will be found;
 *
 * @param[in] start_name The name of the one-dimensional netCDF::NcVar (whose
 *                       type is compatible with T) within the given \p group
 *                       which tells how the elements of name are subdivided
 *            between the rows of \p array; start_name is optional if \p name
 *            is not found in \p group;
 *
 * @param[out] array A reference to the std::vector< std::vector< T > > to
 *                   be deserialized;
 *
 * @param[in] optional A bool stating if it is allowed for \p name not to be
 *                     a netCDF::NcVar in \p group, in which case an empty \p
 *                     array is returned as opposed to throwing exception.
 *                     Default is true.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , bool >
deserialize( const netCDF::NcGroup & group , const std::string & name ,
             const std::string & start_name ,
             std::vector< std::vector< T > > & matrix ,
             bool optional = true ) {
 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( optional ) {
   matrix.clear();
   return( false );
   }

  throw( std::invalid_argument( "deserialize(): " + name +
                                " is not present in group " +
                                group.getName() ) );
  }

 if( ncVar.getDimCount() != 1 )
  throw( std::invalid_argument( "deserialize(): " + name +
                                " has wrong number of dimensions" ) );

 auto ncVar_S = group.getVar( start_name );
 if( ncVar_S.isNull() )
  throw( std::invalid_argument( "deserialize(): " + start_name +
                                " is not present in group " +
                                group.getName() ) );

 if( ncVar_S.getDimCount() != 1 )
  throw( std::invalid_argument( "deserialize(): " + start_name +
                                " has wrong number of dimensions" ) );

 auto nrows = ncVar_S.getDim( 0 ).getSize();
 std::vector< int > strt( nrows + 1 );
 ncVar_S.getVar( strt.data() );

 strt[ nrows ] = ncVar.getDim( 0 ).getSize();

 for( decltype( nrows ) i = 0 ; i < nrows ; ++i ) {
  strt[ i ] = strt[ i + 1 ] - strt[ i ];
  if( strt[ i ] < 0 )
   throw( std::invalid_argument( "deserialize(): wrong indices in " +
                                 start_name ) );
  }

 std::vector< T > tmp( ncVar.getDim( 0 ).getSize() );
 get_var_values( ncVar , tmp.data() );

 matrix.resize( nrows );

 auto tit = tmp.begin();
 for( decltype( nrows ) i = 0 ; i < nrows ; ++i ) {
  matrix[ i ].resize( strt[ i ] );
  auto aiit = matrix[ i ].begin();
  for( const auto aiend = matrix[ i ].end() ; aiit != aiend ; )
   *( aiit++ ) = *( tit++ );
  }

 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a matrix with variable-length rows
/** This function serializes a matrix with variable-length rows, i.e., a
 * std::vector< std::vector< T > >. This matrix is serialized by adding two
 * distinct netCDF::NcVars: a one-dimensional array val[] (with name \p name,
 * type \p ncType, and size equal to the total number of \p T elements in
 * \p array) and a one-dimensional array start[] (with name \p start_name,
 * type netCDF::NcUint, and size equal to the number of rows of \p array).
 * The array val[] will contain the elements of \p matrix in row-major layout.
 * The i-th element of start[] will contain the index in val[] of the first
 * element of the i-th row of \p array. That is,
 *
 *   start[ i ] = sum_{j = 0}^{i - 1} array[ i ].size().
 *
 * Thus, the i-th row of \p array will be saved into the elements of val[]
 * with indices in the closed-open interval [ start[ i ] , start[ i + 1 ] ),
 * except for the last row for which start[ i + 1 ] is undefined and whose
 * elements are saved into the elements of val[] with indices in the
 * closed-open interval [ start[ i ] , | val | ), where | val | is the size
 * of val[].
 *
 * @param[in] group The netCDF::NcGroup into which the matrix with
 *                   variable-length rows will be serialized.
 *
 * @param[in] name  The name of the one-dimensional netCDF::NcVar of type
 *                  \p ncType to be added to the given \p group and into
 *                  which the values of \p array will be saved;
 *
 * @param[in] ncType The type of the netCDF variable whose name is \p name.
 *
 * @param[in] start_name The name of the one-dimensional netCDF::NcVar to be
 *                       added to the given \p group that represents the
 *                       start[] array;
 *
 * @param[in] array A reference to the std::vector< std::vector< T > > to
 *                  be serialised;
 *
 * @param[in] array_dim The netCDF dimension of the netCDF variable whose name
 *                      is \p name. This dimension must already be part of \p
 *                      group. If it is not provided, then a dimension with
 *                      name given by the concatenation of \p name and "_dim"
 *                      will be added to \p group. For instance, if it is not
 *                      provided and \p name is the string "matrix", then a
 *                      dimension with name "matrix_dim" will be added to \p
 *                      group.
 *
 * @param[in] start_dim The netCDF dimension of the netCDF variable whose name
 *                      is \p start_name. This dimension must already be part
 *                      of \p group. If it is not provided, then a dimension
 *                      with name given by the concatenation of \p start_name
 *                      and "_dim" will be added to \p group. */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const std::string & name ,
           const netCDF::NcType & ncType , const std::string & start_name ,
           const std::vector< std::vector< T > > & array ,
           const netCDF::NcDim & array_dim = netCDF::NcDim() ,
           const netCDF::NcDim & start_dim = netCDF::NcDim() ) {
 if( array.empty() )
  return; // Nothing to be serialized.

 // Compute the total number of elements in the given array
 decltype( array.size() ) num_elements = 0;
 for( decltype( array.size() ) i = 0 ; i < array.size() ; ++i ) {
  num_elements += array[ i ].size();
  }

 // Add the netCDF variable that will store the elements of the given array
 netCDF::NcVar array_var;

 if( ! array_dim.isNull() ) {
  if( num_elements != array_dim.getSize() )
   throw( std::invalid_argument(
    "serialize(): variable " + name + " of group " + group.getName() +
    ", the given dimension is not compatible with the given array" ) );
  array_var = group.addVar( name , ncType , { array_dim } );
  }
 else {
  // Add a default dimension for variable "name"
  auto dim = group.addDim( name + "_dim" , num_elements );
  array_var = group.addVar( name , ncType , { dim } );
  }

 // Save the elements of the given array into the netCDF variable "name"
 decltype( array.size() ) offset = 0;
 for( decltype( array.size() ) i = 0 ; i < array.size() ; ++i ) {
  array_var.putVar( { offset } , { array[ i ].size() } , array[ i ].data() );
  offset += array[ i ].size();
  }

 // Create the "start" vector
 std::vector< unsigned int > start( array.size() );
 start.front() = 0;
 for( decltype( array.size() ) i = 1 ; i < array.size() ; ++i )
  start[ i ] = start[ i - 1 ] + array[ i - 1 ].size();

 // Add the netCDF variable "start"
 netCDF::NcVar start_var;

 if( ! start_dim.isNull() ) {
  if( start_dim.getSize() != array.size() )
   throw( std::invalid_argument(
    "serialize(): variable " + name + " of group " + group.getName() +
    ", the size of the given dimension to the 'start' netCDF variable is "
    "different from the number of rows of the given array" ) );
  start_var = group.addVar( start_name , netCDF::NcUint() , { start_dim } );
  }
 else {
  // Add a default dimension for variable "start_name".
  auto dim = group.addDim( start_name + "_dim" , array.size() );
  start_var = group.addVar( start_name , netCDF::NcUint() , { dim } );
  }

 // Save the start vector into the netCDF variable "start_name"
 start_var.putVar( start.data() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - SERIALIZING AND DESERIALIZING BI-DIMENSIONAL VARIABLES OF - - - -*/
/*- - - - BASIC TYPES WITH FIXED-LENGTH ROWS STORED INTO A- - - - - - - - -*/
/*- - - - std::vector< std::vector< T > > - - - - - - - - - - - - - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a matrix (with fixed-length rows)
/** This function reads a matrix with fixed-length rows stored into a
 * std::vector< std::vector< T > > where each "inner" vector has the same
 * size. This is obtained from the two-dimensional netCDF variable whose
 * name is \p name in the given netCDF \p group.
 *
 * @param[in] group The netCDF::NcGroup from which the matrix will be
 *                  obtained.
 *
 * @param[in] name The name of the two-dimensional netCDF::NcVar within the
 *                 given \p group which contains the data to be stored in \p
 *                 matrix;
 *
 * @param[out] matrix A reference to the std::vector< std::vector< T > > to be
 *                    deserialized;
 *
 * @param[in] optional A bool stating if it is allowed for \p name not to be
 *                     be present in the netCDF::NcVar in \p group, in which
 *                     case an empty \p matrix is returned as opposed to
 *                     throwing an exception. Default is true.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , bool >
deserialize( const netCDF::NcGroup & group , const std::string & name ,
             std::vector< std::vector< T > > & matrix ,
             bool optional = true ) {
 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( optional ) {
   matrix.clear();
   return( false );
   }

  throw( std::invalid_argument( "deserialize(): " + name +
                                " is not present in group " +
                                group.getName() ) );
  }

 if( ncVar.getDimCount() != 2 )
  throw( std::invalid_argument( "deserialize(): " + name +
                                " has wrong number of dimensions" ) );

 auto num_rows = ncVar.getDim( 0 ).getSize();
 auto num_cols = ncVar.getDim( 1 ).getSize();

 matrix.resize( num_rows );
 for( decltype( num_rows ) i = 0 ; i < num_rows ; ++i ) {
  matrix[ i ].resize( num_cols );
  get_var_values( ncVar , matrix[ i ].data() , { i , 0 } ,
                  { 1 , num_cols } );
  }

 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a matrix (with fixed-length rows)
/** This function serializes a matrix with fixed-length rows stored into a
 * std::vector< std::vector< T > > where each "inner" vector has the same
 * size. A netCDF variable with name \p name and type \p ncType is created
 * in the given netCDF \p group and the given \p matrix is serialized into
 * this variable. The pair \p dimensions must contain the dimensions of this
 * matrix: the first dimension is the number of rows and the second one is the
 * number of columns of the matrix. Note that these dimensions must either
 * already be present in the given \p group or be null, in which case two
 * new dimensions with "default" names are added to \p group.
 *
 * @param[in] group The netCDF::NcGroup into which the matrix will be
 *                  serialized.
 *
 * @param[in] name  The name of the two-dimensional netCDF::NcVar which
 *                  will be added to the given \p group and store the data of
 *                  the given \p matrix.
 *
 * @param[in] ncType The type of the netCDF variable whose name is \p name.
 *
 * @param[out] matrix A reference to the std::vector< std::vector< T > > to be
 *                    serialised;
 *
 * @param[in] dimensions The netCDF dimensions of the netCDF variable whose
 *                       name is \p name. If the first dimension is null, then
 *                       a dimension with name given by the concatenation of
 *                       \p name and "_rows" will be added to the given \p
 *                       group. If the second dimension is null, then a
 *                       dimension with name given by the concatenation of \p
 *                       name and "_cols" will be added to the given \p group.
 */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const std::string & name ,
           const netCDF::NcType & ncType ,
           const std::vector< std::vector< T > > & matrix ,
           std::pair< netCDF::NcDim , netCDF::NcDim > dimensions = {} ) {
 if( matrix.empty() )
  return;  // nothing to be serialized

 auto num_rows = matrix.size();
 auto num_cols = matrix.front().size();

 if( ! num_cols )
  return;  // nothing to be serialized

 if( std::any_of( matrix.begin() , matrix.end() ,
                  [ num_cols ]( auto & ci ) {
                   return( ci.size() != num_cols );
                   } ) )
  throw( std::invalid_argument(
   "serialize(): matrix rows are not all of the same length" ) );

 if( dimensions.first.isNull() )
  dimensions.first = group.addDim( name + "_rows" , num_rows );
 else
  if( num_rows != dimensions.first.getSize() )
   throw( std::invalid_argument(
     "serialize(): variable " + name + " of group " + group.getName() +
     ", matrix rows do not match with first dimension" ) );

 if( dimensions.second.isNull() )
  dimensions.second = group.addDim( name + "_cols" , matrix.front().size() );
 else
  if( num_cols != dimensions.second.getSize() )
   throw( std::invalid_argument(
     "serialize(): variable " + name + " of group " + group.getName() +
     ", matrix columns do not match with second dimension" ) );

 // add the netCDF variable
 auto ncVar = group.addVar( name , ncType ,
                            { dimensions.first , dimensions.second } );

 // set the data of the netCDF variable
 for( decltype( num_rows ) i = 0 ; i < num_rows ; ++i )
  ncVar.putVar( { i , 0 } , { 1 , num_cols } , matrix[ i ].data() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - SERIALIZING AND DESERIALIZING MULTI-DIMENSIONAL VARIABLES - - - -*/
/*- - - - OF BASIC TYPES STORED INTO A boost::multi_array< T > >- - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a multi-dimensional variable out of a netCDF NcGroup
/** This function reads a multi-dimensional array of values of type \p T, from
 * a netCDF variable with name \p name within the given netCDF NcGroup \p
 * group. The number of dimensions of the multi-dimensional array is given by
 * the template parameter \p N. The values read are stored in the given \p
 * array in row-major layout.
 *
 * If the variable is not present in the given \p group, then the array is
 * emptied (every dimension becomes zero-sized) if the value of the parameter
 * \p optional is true or an std::invalid_argument exception is thrown if \p
 * optional is false.
 *
 * If the given \p group has more than one variable with the same name, then
 * the variable that is considered follows the rule defined by the netCDF
 * method NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens,
 * then the variable closest to the given group is considered.
 *
 * If the variable in the file is a scalar (i.e., it has zero dimensions),
 * and \p allow_scalar_var is true, then the entire \p multi_array will be
 * filled with the scalar value, resized according to the given \p sizes.
 *
 * In the specific case when N == 2, this function also supports common
 * decompression patterns used in time-dependent variables:
 *  - If the first dimension is 1, the value is broadcast over all rows.
 *  - If the second dimension is 1, the value is broadcast over all columns.
 *  - If the first dimension equals the number of change intervals, the data
 *    is expanded row-wise using the logic of the provided \p change_intervals.
 *
 * @param[in] group The netCDF NcGroup from which the array will be obtained
 *                  from.
 *
 * @param[in] name  The name of the variable within the given \p group.
 *
 * @param[in] sizes This parameter specifies the expected ordering of netCDF
 *                  dimensions. If the variable in the file has dimensions in
 *                  the reverse order with respect to what is expected (e.g.,
 *                  [N,1] instead of [1,N]), and N == 2, the array will be
 *                  reshaped to match the expected layout.
 *
 * @param[out] multi_array A reference to the boost::multi_array that will
 *                         store the multi-dimensional array in row-major
 *                         layout.
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup then: (i) an exception is
 *                     thrown if \p optional == false; (ii) every dimension of
 *                     \p multi_array will have size 0 if \p optional == true.
 *                     Default is true.
 *
 * @param[in] allow_scalar_var This parameter indicates whether the desired
 *                             variable (whose name is \p var_name) can have
 *                             dimension zero (i.e., it could be a scalar
 *           instead of an array). Its default value is false, and this means
 *           that if the variable has dimension zero (i.e., it is a scalar), an
 *           exception is thrown. If, instead, \p allow_scalar_var == true,
 *           then if the netCDF variable is a scalar, then the given
 *           boost::multi_array \p multi_array will have a single element (the
 *           origin) whose value will be that of the netCDF variable.
 *
 * @param[in] change_intervals The list of time intervals used to decompress
 *                             time-series data when the number of rows in the
 *                             variable equals the number of change intervals.
 *           This only applies when N == 2 and enables row-wise expansion of
 *           the input array. If empty, this logic is skipped.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< class T , std::size_t N >
std::enable_if_t< is_netCDF_type_v< T > , bool >
deserialize( const netCDF::NcGroup & group , const std::string & name ,
             const std::vector< std::size_t > & sizes ,
             boost::multi_array< T , N > & multi_array ,
             bool optional = true , bool allow_scalar_var = false ,
             const std::vector< std::size_t > & change_intervals = {} ) {
 using index = typename boost::multi_array< T , N >::index;

 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( optional ) {
   std::vector< index > new_sizes( N , 0 );
   multi_array.resize( new_sizes );
   return( false );
   }

  throw( std::invalid_argument( "deserialize(): " + name +
                                " not present in group " + group.getName() ) );
  }

 if( ncVar.getDimCount() == 0 ) {
  if( allow_scalar_var ) {
   std::vector< index > new_sizes = sizes.empty()
                                     ? std::vector< index >( N , 1 )
                                     : std::vector< index >( sizes.begin() ,
                                                             sizes.end() );
   multi_array.resize( new_sizes );

   T value;
   get_var_values( ncVar , & value );

   std::fill( multi_array.data() ,
              multi_array.data() + multi_array.num_elements() ,
              value );
   return( true );
   }

  throw( std::invalid_argument(
   "deserialize(): netCDF variable " + name + " is a scalar,"
   " but a multi-dimensional array with " + std::to_string( N ) +
   " dimensions was expected in group " + group.getName() ) );
  }

 auto sizes_dimensions = get_sizes_dimensions( ncVar );
 if( sizes_dimensions.size() < multi_array.num_dimensions() )
  sizes_dimensions.resize( multi_array.num_dimensions() , 1 );

 multi_array.resize( sizes_dimensions );

 std::vector< std::size_t > start( sizes_dimensions.size() , 0 );
 get_var_values( ncVar , multi_array.data() , start ,
                 sizes_dimensions );

 // Post-processing only for 2D arrays
 if constexpr( N == 2 ) {
  if( sizes.size() == 2 ) {
   index expected_dim0 = sizes[ 0 ];
   index expected_dim1 = sizes[ 1 ];

   auto dim0 = multi_array.shape()[ 0 ];
   auto dim1 = multi_array.shape()[ 1 ];

   // auto-transpose if needed
   if( ( dim0 == expected_dim1 ) && ( dim1 == 1 ) ) {
    multi_array.reshape( boost::array< index , N >{ { 1 , expected_dim1 } } );
    std::swap( dim0 , dim1 );
   } else if( ( dim1 == expected_dim0 ) && ( dim0 == 1 ) ) {
    multi_array.reshape( boost::array< index , N >{ { expected_dim0 , 1 } } );
    std::swap( dim0 , dim1 );
   }

   const bool dim0_expandable = ( dim0 == 1 ||
                                  dim0 == change_intervals.size() );
   const bool dim1_expandable = ( dim1 == 1 );

   const bool shape_mismatch =
    ( dim0 != expected_dim0 && ! dim0_expandable ) ||
    ( dim1 != expected_dim1 && ! dim1_expandable );

   if( shape_mismatch )
    throw( std::logic_error(
     "deserialize(): shape mismatch in variable " + name +
     " in group " + group.getName() +
     ": got [" + std::to_string( dim0 ) + " x " + std::to_string( dim1 ) +
     "], expected [" + std::to_string( expected_dim0 ) + " x " +
     std::to_string( expected_dim1 ) + "]" ) );

   if( dim0 == 1 ) {
    // For each column j, the value is constant over all rows i
    boost::multi_array< T , 2 > given_array = multi_array;
    multi_array.resize( boost::extents[ expected_dim0 ][ expected_dim1 ] );
    if( dim1 == 1 ) {
     // replicate on all rows and columns
     const T value = given_array[ 0 ][ 0 ];
     for( auto i = 0 ; i < expected_dim0 ; ++i )
      for( auto j = 0 ; j < expected_dim1 ; ++j )
       multi_array[ i ][ j ] = value;
     }
    else {
     // replicate on rows
     for( auto i = 0 ; i < expected_dim0 ; ++i )
      for( auto j = 0 ; j < expected_dim1 ; ++j )
       multi_array[ i ][ j ] = given_array[ 0 ][ j ];
     }
    }
   else if( dim1 == 1 ) {
    // For row i, the value is constant over all columns j
    boost::multi_array< T , 2 > given_array = multi_array;
    multi_array.resize( boost::extents[ expected_dim0 ][ expected_dim1 ] );
    for( auto i = 0 ; i < expected_dim0 ; ++i )
     for( auto j = 0 ; j < expected_dim1 ; ++j )
      multi_array[ i ][ j ] = given_array[ i ][ 0 ];
    }
   else if( ( dim0 == change_intervals.size() ) &&
            ( ! change_intervals.empty() ) ) {
    // The number of rows is equal to the number of change intervals.
    // For row i and column j, the value is determined by the interval k
    boost::multi_array< T , 2 > given_array = multi_array;
    multi_array.resize( boost::extents[ expected_dim0 ][ expected_dim1 ] );
    for( auto j = 0 ; j < expected_dim1 ; ++j ) {
     auto i = 0;
     for( auto k = 0 ; k < change_intervals.size() ; ++k ) {
      auto upper = ( k == change_intervals.size() - 1 )
                    ? expected_dim0 - 1 : change_intervals[ k ];
      for( ; i <= upper ; ++i )
       multi_array[ i ][ j ] = given_array[ k ][ j ];
      }
     }
    }
   }
  }

 // Final shape check
 if( ( ! sizes.empty() ) &&
     ( ( multi_array.num_dimensions() != sizes.size() ) ||
       ( ! std::equal( multi_array.shape() ,
                       multi_array.shape() + multi_array.num_dimensions() ,
                       sizes.begin() ) ) ) )
  throw( std::logic_error( "deserialize(): shape mismatch for variable " +
                           name + " in group " + group.getName() ) );

 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a multi-dimensional variable into a netCDF NcGroup
/** Add a new multi-dimensional netCDF variable with the given \p name in
 * the given netCDF NcGroup \p group. Moreover, it stores the given data into
 * that variable in row-major layout.
 *
 * @param[in, out] group The netCDF NcGroup in which the variable will be
 *                       added.
 *
 * @param[in] name       The name of the variable that will be added.
 *
 * @param[in] ncType     The type of the elements of the array.
 *
 * @param[in] ncDim      A vector with the netCDF dimensions of the array.
 *                       Note that uninitialised dimensions (.isNull()) are
 *                       supported, being considered as 1.
 *
 * @param[in] data       A vector containing the data to be stored in the
 *                       variable in row-major layout.
 *
 * @param[in] sizes This is an optional parameter that indicates the sizes of
 *                  each dimension of the multi-dimensional array to be
 *                  serialized. If the number of dimensions is greater than 1,
 *            then (a) if \p sizes is not provided then unlimited dimensions
 *            are not supported (an exception is thrown in this case); (b) if
 *            \p sizes is provided, it must have the same number of elements
 *            as \p ncDim (otherwise, an exception is thrown).
 *
 * @param[in] allow_scalar_var Although this function is supposed to serialize
 *                             a multi-dimensional array, it can also be used
 *                             to serialize a scalar. If the given vector \p
 *            data has size 1 and \p allow_scalar_var is true, then a netCDF
 *            scalar variable is created instead of a multi-dimensional one
 *            (notice that, in this case, the argument \p ncDim is completely
 *            ignored). */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const std::string & name ,
           const netCDF::NcType & ncType ,
           const std::vector< netCDF::NcDim > & ncDim ,
           const std::vector< T > & data ,
           const std::vector< std::size_t > & sizes = {} ,
           bool allow_scalar_var = false ) {
 if( data.empty() )
  return;  // nothing to be serialized

 if( ( ncDim.size() == 1 ) && ( sizes.size() <= 1 ) )
  serialize( group , name , ncType , ncDim[ 0 ] , data , allow_scalar_var );

 if( allow_scalar_var && ( data.size() == 1 ) ) {
  // serialize the only element of the given vector as a scalar variable
  serialize( group , name , ncType , data[ 0 ] );
  return;
  }

 if( sizes.empty() ) {
  // The sizes of the dimensions of the multidimensional array to be
  // serialized were not provided. We thus consider the sizes of the given
  // netCDF dimensions. In this case, unlimited dimensions are not allowed.

  std::vector< std::size_t > sz( ncDim.size() );
  for( std::size_t i = 0 ; i < ncDim.size() ; ++i )
   if( ncDim[ i ].isNull() )
    sz[ i ] = 1;
   else {
    sz[ i ] = ncDim[ i ].getSize();

    if( ncDim[ i ].isUnlimited() )
     throw( std::invalid_argument(
      "serialize(): error when serializing variable " + name +
      " of group " + group.getName() + ". The given netCDF dimension " +
      std::to_string( i ) +
      " is unlimited, but unlimited dimension is not supported when"
      " the sizes of each dimension of the multi-dimensional array"
      " represented by the vector parameter 'data' are not provided" ) );
    }

  std::vector< std::size_t > start( ncDim.size() , 0 );
  group.addVar( name , ncType , ncDim ).putVar( start , sz , data.data() );
  }
 else
  if( ncDim.size() != sizes.size() )
   throw( std::invalid_argument(
   "serialize(): error when serializing variable " + name +
   " of group " + group.getName() + ". The vector 'sizes' has size " +
   std::to_string( sizes.size() ) + ", while the vector 'ncDim' has size "
   + std::to_string( ncDim.size() ) +
   ". When the optional vector parameter 'sizes' is present,"
   " it must have the same size as the vector 'ncDim'" ) );
  else {
   std::vector< std::size_t > start( ncDim.size() , 0 );
   group.addVar( name , ncType , ncDim ).putVar( start , sizes ,
                                                 data.data() );
   }
 }

/*--------------------------------------------------------------------------*/
/// serialize a multi-dimensional variable into a netCDF NcGroup
/** Add a new multi-dimensional netCDF variable with the given name in the
 * given netCDF NcGroup. Moreover, it stores the given data into that variable
 * in row-major layout. The data is given by the boost::multi_array \p
 * multi_array. The type of the elements in \p multi_array is determined by
 * the template parameter \p T, while the number of dimensions of the \p
 * multi_array is given by the template parameter \p N.
 *
 * The netCDF dimensions of the array are given by the \p expected_ncDim
 * parameter. In general, the size of each of these dimensions should be the
 * same as the size of the corresponding dimension of the given \p multi_array
 * (i.e., size of dimension i of the given netCDF dimensions should be equal
 * to the size of dimension i of \p multi_array). There are three cases in
 * which those sizes do not need agree:
 *
 * 1) If the given \p multi_array has a single element and \p
 *    allow_scalar_var is true, then this element is serialized into a
 *    netCDF scalar variable. In this case, the given \p expected_ncDim is
 *    ignored.
 *
 * 2) If a netCDF dimension given in \p expected_ncDim has size greater than
 *    1 but the size of the corresponding dimension in the given \p
 *    multi_array is 1 and \p allow_singleton_dim is true, then that
 *    dimension will have size 1 in the netCDF variable. This artificial
 *    dimension with size 1 is referred to as the singleton dimension. The
 *    name of the singleton dimension is specified by the \p
 *    singleton_dim_name parameter. If the singleton dimension is not
 *    present in the given \p group, one is created with the name given by
 *    \p singleton_dim_name.
 *
 * 3) If a netCDF dimension is unlimited, then the corresponding dimension of
 *    \p multi_array can have any size.
 *
 * Notice that the given \p multi_array will not be serialized in the given \p
 * group if some of its dimensions has size 0 (i.e., empty arrays are not
 * serialized).
 *
 * If the given \p multi_array is not serialized into a netCDF *scalar*
 * variable, then an exception is thrown in each of the following cases:
 *
 * - The size of the \p expected_ncDim vector is different from the size of
 *   the template parameter \p N.
 *
 * - The size of some dimension i in the \p expected_ncDim vector is positive,
 *   it is different from the size s_i of dimension i of \p multi_array and
 *   either \p allow_singleton_dim is false or s_i != 1
 *
 * @param[in, out] group The netCDF NcGroup in which the variable will be
 *                       added.
 *
 * @param[in] name The name of the variable that will be added.
 *
 * @param[in] ncType The type of the elements of the \p multi_array.
 *
 * @param[in] expected_ncDim A vector with the (expected) netCDF dimensions of
 *                           the array. If the given \p multi_array is not
 *                           serialized into a netCDF *scalar* variable, then
 *            the size of this vector must necessarily be equal to the value
 *            of the template parameter \p N (otherwise, an exception is
 *            thrown). Note that some of the elements of this vector may be
 *            uninitialised (.isNull()), in which case they are treated as
 *            being dimensions with value 1.
 *
 * @param[in] multi_array A boost::multi_array containing the data to be
 *                        stored in the netCDF variable in row-major layout.
 *
 * @param[in] allow_scalar_var Although this function is supposed to serialize
 *                             an array, it can also be used to serialize a
 *                             scalar. If the given \p multi_array has size 1
 *           and \p allow_scalar_var == true, then a netCDF *scalar* variable
 *           is created instead of a multi-dimensional one (notice that, in
 *           this case, the argument \p expected_ncDim is completely ignored).
 *
 * @param[in] allow_singleton_dim This parameter indicates whether singleton
 *                                dimensions are allowed. If
 *                                \p allow_singleton_dim == true, then a
 *            dimension given in \p expected_ncDim may be replaced by a
 *            singleton dimension as explained above. If
 *            \p allow_singleton_dim == false, then, for each i in {0, ...,
 *            N-1}, the size of the i-th dimension in \p expected_ncDim must
 *            be equal to the size of the i-th dimension of \p multi_array,
 *            except when (i) the \p multi_array is serialized into a netCDF
 *            *scalar* variable or (ii) the i-th dimension in \p
 *            expected_ncDim is unlimited. If \p allow_singleton_dim == false
 *            and neither condition (i) nor condition (ii) is met, then an
 *            exception is thrown when the i-th dimension has different sizes
 *            in \p expected_ncDim and \p multi_array.
 *
 * @param[in] singleton_dim_name The name of the singleton dimension. If the
 *                                singleton dimension is used (see
 *                                \p allow_singleton_dim parameter), then the
 *            dimension whose name is given by \p singleton_dim_name is
 *            considered. If this dimension is present in the given \p
 *            group, it must have size 1 (otherwise, an exception is
 *            thrown). If a dimension with this name is not found in
 *            \p group, a dimension with this name is added to the \p group. */

template< class T , std::size_t N >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const std::string & name ,
           const netCDF::NcType & ncType ,
           const std::vector< netCDF::NcDim > & expected_ncDim ,
           const boost::multi_array< T , N > & multi_array ,
           bool allow_scalar_var = false , bool allow_singleton_dim = false ,
           const std::string & singleton_dim_name = "__Singleton__" ) {
 if( multi_array.num_elements() == 0 )
  return;  // nothing to be serialized

 if( allow_scalar_var && multi_array.num_elements() == 1 ) {
  // serialize the only value of multi_array into a netCDF scalar variable
  serialize( group , name , ncType , *multi_array.origin() );
  return;
  }

 if( expected_ncDim.size() != N )
  throw( std::invalid_argument(
   "serialize(): variable " + name + " of group " + group.getName() +
   ", the given boost::multi_array has " + std::to_string( N ) +
   " dimensions, but " + std::to_string( expected_ncDim.size() ) +
   " netCDF::NcDim were provided" ) );

 auto ncDim = expected_ncDim;

 for( std::vector< netCDF::NcDim >::size_type i = 0 ; i < N ; ++i ) {
  if( ( ! ncDim[ i ].isNull() ) && ncDim[ i ].isUnlimited() )
   continue;

  auto ncdim_size = ncDim[ i ].isNull() ? 1 : ncDim[ i ].getSize();
  auto multi_array_dim_size = multi_array.shape()[ i ];

  if( ncDim[ i ].isNull() || ( ncdim_size != multi_array_dim_size ) ) {
   if( allow_singleton_dim && multi_array_dim_size == 1 ) {
    // Ignore the given netCDF::NcDim for the i-th dimension and consider
    // the singleton dimension instead.

    auto singleton_dim = group.getDim( singleton_dim_name );

    if( singleton_dim.isNull() )
     // The singleton dimension is currently not present in the given
     // group. So, we add the singleton dimension to the given group.
     singleton_dim = group.addDim( singleton_dim_name , 1 );
    else
     if( singleton_dim.getSize() != 1 )
      throw( std::invalid_argument(
	      "serialize(): error when serializing variable " + name +
	      " of group '" + group.getName() +
	      "': the singleton dimension " + singleton_dim_name +
	      " must have size 1 but has size " +
	      std::to_string( singleton_dim.getSize() ) ) );

    ncDim[ i ] = singleton_dim;
    }
   else {
    throw( std::invalid_argument(
     "serialize(): variable " + name + " of group " + group.getName() +
     ", the size of dimension " + std::to_string( i ) +
     " of the given boost::multi_array is " +
     std::to_string( multi_array_dim_size ) +
     " but the size of the provided netCDF dimension is " +
     std::to_string( ncdim_size ) +
     ". At least one of the following requirements must be met:"
     " (1) these sizes are equal;"
     " (2) the given boost::multi_array has a single element and"
     " 'allow_scalar_var' is true;"
     " (3) the size of the dimension of the given boost::"
     "multi_array is 1 and 'allow_singleton_dim' is true;"
     " (4) provided netCDF dimension is unlimited" ) );
    }
   }
  }

 std::vector< std::size_t > start( N , 0 );
 std::vector< std::size_t > dim_sizes( N );
 for( std::vector< std::size_t >::size_type i = 0 ; i < N ; ++i )
  dim_sizes[ i ] = multi_array.shape()[ i ];

 group.addVar( name , ncType , ncDim ).putVar( start , dim_sizes ,
                                               multi_array.data() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - CHECKING netCDF FILES FOR (UN)EXPECTED STUFF  - - - - - - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// check if all the expected netCDF variables are present in a group
/** Check if all the expected netCDF variables are present in a group
 *
 * @param group    The group to be checked
 * @param expected The names of the expected variables
 * @param stream   The output stream for putting out the unexpected variables
 */

inline void check_variables( const netCDF::NcGroup & group ,
                             const std::vector< std::string > & expected ,
                             std::ostream & stream ) {
 auto vars = group.getVars();
 for( const auto & e : expected ) {
  auto search = vars.find( e );
  if( search != vars.end() )
   vars.erase( search );
  }

 if( ! vars.empty() ) {
  stream << "Unexpected netCDF Vars found in group: ";
  for( const auto & v : vars )
   stream << v.first << "; ";
  stream << std::endl;
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// check if all the expected netCDF dimensions are present in a group
/** Check if all the expected netCDF dimensions are present in a group
 *
 * @param group    The group to be checked
 * @param expected The names of the expected dimensions
 * @param stream   The output stream for putting out the unexpected dimensions
 */

inline void check_dimensions( const netCDF::NcGroup & group ,
                              const std::vector< std::string > & expected ,
                              std::ostream & stream ) {
 auto dims = group.getDims();
 for( const auto & e : expected ) {
  auto search = dims.find( e );
  if( search != dims.end() )
   dims.erase( search );
  }

 if( ! dims.empty() ) {
  stream << "Unexpected netCDF Dims found in group: ";
  for( const auto & d : dims )
   stream << d.first << "; ";
  stream << std::endl;
  }
 }

/** @} ---------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*-------------------------------- CLASSES ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup SMSnetCDF_CLASSES Classes in SMSnetCDF.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS NcMemFile ------------------------------*/
/*--------------------------------------------------------------------------*/
/// a netCDF-4 file held in memory
/** NcMemFile is a netCDF::NcFile whose content is a netCDF-4 file image in
 * memory, built on the in-memory support of the netCDF-C library
 * (nc_create_mem(), nc_open_memio() and nc_close_memio()). An NcMemFile is
 * either
 *
 * - created empty and open for writing, by the default constructor;
 *
 * - opened read-only on an existing image, by NcMemFile( std::span ),
 *   working on the bytes of the caller, which are not copied and have to
 *   outlive the NcMemFile.
 *
 * close( Image & ) closes the file and gives its image, a netCDF-4 file in
 * its own right: it can be written to disk and opened from there by any
 * netCDF reader, sent elsewhere, or opened again by an NcMemFile, and the
 * bytes of a netCDF-4 file read from disk can be opened by an NcMemFile.
 * The image need not be byte for byte the file that a netCDF::NcFile on
 * disk with the same content would hold, since the HDF5 layer writes a
 * file in memory with an older version of its format and pads it to the
 * size it has allocated, but the same content written in the same order
 * with the same netCDF and HDF5 libraries gives the same bytes. close() and
 * the destructor close the file discarding the image.
 *
 * An image cannot be opened for writing, since the netCDF-C library does
 * not write in a netCDF-4 image it has not created (nor, on disk, in the
 * file of such an image): what has to go in an image is written in the
 * NcMemFile that creates it, before it is closed.
 *
 * The in-memory files need netCDF-C 4.6.2 or later; with an older library
 * the constructors throw std::logic_error. */

class NcMemFile : public netCDF::NcFile {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the image of a netCDF file: its bytes
 using Image = std::vector< unsigned char >;

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

 /// create an empty netCDF-4 file in memory, open for writing
 /** The buffer of the image belongs to the library: with netCDF-C from
  * 4.6.2 to (at least) 4.9.3 it starts at 64 KiB, and the HDF5 "core"
  * driver grows it by realloc() in steps of 64 KiB while the file is
  * written, so a large image is reallocated about once every 64 KiB of
  * growth and its bytes come out padded to the last multiple of 64 KiB
  * [see close( Image & )]. No size can be asked for in advance, since
  * nc_create_mem() takes one but does not use it for a netCDF-4 file. */

 NcMemFile( void );

/*--------------------------------------------------------------------------*/
 /// open the netCDF image \p image read-only
 /** The file works on the bytes of \p image themselves, which have to stay
  * there unchanged until the NcMemFile is closed. An \p image that is not a
  * netCDF file throws. */

 explicit NcMemFile( std::span< const unsigned char > image );

/*--------------------------------------------------------------------------*/
 /// destructor: closes the file, if open, discarding its image

 ~NcMemFile() override = default;

/*--------------------------------------------------------------------------*/
/*-------------------------- OTHER METHODS ---------------------------------*/
/*--------------------------------------------------------------------------*/

 /// close the file, discarding its image

 using netCDF::NcFile::close;

/*--------------------------------------------------------------------------*/
 /// close the file and write its image in \p image
 /** Closes the file and replaces the content of \p image with the image of
  * the file as it is when closed. The NcMemFile is then a null object, as
  * after netCDF::NcFile::close(). Throws if the file is not open.
  *
  * The bytes are copied once into \p image from the buffer of the library,
  * which is then freed: the buffer is a block of malloc(), which a
  * std::vector cannot take over. */

 void close( Image & image );

/*--------------------------------------------------------------------------*/
/*------------------------ PRIVATE PART OF THE CLASS -----------------------*/
/*--------------------------------------------------------------------------*/

 private:

 bool f_read = false;  ///< true if the bytes are those of the caller

/*--------------------------------------------------------------------------*/

 };  // end( class( NcMemFile ) )

/** @} end( group( SMSnetCDF_CLASSES ) ) -----------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* SMSnetCDF.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File SMSnetCDF.h --------------------------*/
/*--------------------------------------------------------------------------*/
