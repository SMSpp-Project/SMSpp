/*--------------------------------------------------------------------------*/
/*--------------------------- File SMSnetCDF.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the NcMemFile class of SMSnetCDF.h and of the functions
 * copying netCDF groups, copy_group() and write_via_memory().
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
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "SMSnetCDF.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#if __has_include( <netcdf_meta.h> )
 #include <netcdf_meta.h>
#endif

// nc_create_mem(), nc_open_memio() and nc_close_memio() are in 4.6.2 on
#if defined( NC_VERSION_MAJOR ) && \
    ( ( NC_VERSION_MAJOR > 4 ) || \
      ( ( NC_VERSION_MAJOR == 4 ) && \
	( ( NC_VERSION_MINOR > 6 ) || \
	  ( ( NC_VERSION_MINOR == 6 ) && ( NC_VERSION_PATCH >= 2 ) ) ) ) )
 #define SMSpp_NC_MEMIO 1
 #include <netcdf_mem.h>
#else
 #define SMSpp_NC_MEMIO 0
#endif

#ifdef SMSpp_HAVE_HDF5
 #include <H5Epublic.h>
#endif

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#if SMSpp_NC_MEMIO
// the path of a file in memory is only a label, which no file on disk gets
static const char * const mem_path = "SMS++_NcMemFile";
#endif

/*--------------------------------------------------------------------------*/

// the most groups (root included) netCDF-C can have in a file
static constexpr long max_groups = 32767;

/*--------------------------------------------------------------------------*/
// throws a netCDF exception if status is an error
static void nc_check( int status )
{
 netCDF::ncCheck( status , __FILE__ , __LINE__ );
 }

/*--------------------------------------------------------------------------*/
// a variable to be copied: where it is, and where its copy is

namespace {

struct VarCopy {
 int from_grp;
 int from_var;
 int to_grp;
 int to_var;
 };

}  // end( namespace )

/*--------------------------------------------------------------------------*/
// copy the attributes of the variable from_var of from_grp (of the group
// itself if NC_GLOBAL) to the variable to_var of to_grp

static void copy_atts( int from_grp , int from_var , int to_grp , int to_var )
{
 int na = 0;
 nc_check( nc_inq_varnatts( from_grp , from_var , & na ) );
 char name[ NC_MAX_NAME + 1 ];
 for( int i = 0 ; i < na ; ++i ) {
  nc_check( nc_inq_attname( from_grp , from_var , i , name ) );
  nc_check( nc_copy_att( from_grp , from_var , name , to_grp , to_var ) );
  }
 }

/*--------------------------------------------------------------------------*/
// define in the group to everything of the group from, recursively, without
// writing any value: dims maps the dimensions of from to those of to, and
// vars gets the variables whose values are to be copied

static void define_group( int from , int to , std::map< int , int > & dims ,
			  std::vector< VarCopy > & vars )
{
 char name[ NC_MAX_NAME + 1 ];

 // the dimensions, the unlimited ones as such - - - - - - - - - - - - - - -
 int nd = 0;
 nc_check( nc_inq_dimids( from , & nd , nullptr , 0 ) );
 std::vector< int > dimids( nd );
 if( nd )
  nc_check( nc_inq_dimids( from , & nd , dimids.data() , 0 ) );

 int nu = 0;
 nc_check( nc_inq_unlimdims( from , & nu , nullptr ) );
 std::vector< int > unlim( nu );
 if( nu )
  nc_check( nc_inq_unlimdims( from , & nu , unlim.data() ) );

 for( auto d : dimids ) {
  std::size_t len;
  nc_check( nc_inq_dim( from , d , name , & len ) );
  if( std::ranges::find( unlim , d ) != unlim.end() )
   len = NC_UNLIMITED;
  nc_check( nc_def_dim( to , name , len , & dims[ d ] ) );
  }

 // the attributes of the group- - - - - - - - - - - - - - - - - - - - - - -
 copy_atts( from , NC_GLOBAL , to , NC_GLOBAL );

 // the variables- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 int nv = 0;
 nc_check( nc_inq_varids( from , & nv , nullptr ) );
 std::vector< int > varids( nv );
 if( nv )
  nc_check( nc_inq_varids( from , & nv , varids.data() ) );

 for( auto v : varids ) {
  nc_type type;
  int ndims;
  nc_check( nc_inq_var( from , v , name , & type , & ndims , nullptr ,
			nullptr ) );
  if( type > NC_STRING )
   throw( std::invalid_argument( std::string( "copy_group: variable " ) +
				 name + " has a user-defined type" ) );

  std::vector< int > vdims( ndims );
  if( ndims )
   nc_check( nc_inq_vardimid( from , v , vdims.data() ) );
  for( auto & d : vdims )
   if( auto it = dims.find( d ) ; it != dims.end() )
    d = it->second;
   else {  // defined outside the copied group: by name, in to and above
    char dname[ NC_MAX_NAME + 1 ];
    nc_check( nc_inq_dimname( from , d , dname ) );
    nc_check( nc_inq_dimid( to , dname , & d ) );
    }

  int tv;
  nc_check( nc_def_var( to , name , type , ndims , vdims.data() , & tv ) );

  if( ndims ) {
   int storage;
   std::vector< std::size_t > chunks( ndims );
   nc_check( nc_inq_var_chunking( from , v , & storage , chunks.data() ) );
   if( storage != NC_CONTIGUOUS )
    nc_check( nc_def_var_chunking( to , tv , storage ,
				   storage == NC_CHUNKED ? chunks.data()
				                         : nullptr ) );
   }

  int shuffle , deflate , level;
  nc_check( nc_inq_var_deflate( from , v , & shuffle , & deflate , & level ) );
  if( shuffle || deflate )
   nc_check( nc_def_var_deflate( to , tv , shuffle , deflate , level ) );

  int no_fill;
  nc_check( nc_inq_var_fill( from , v , & no_fill , nullptr ) );
  if( no_fill )
   nc_check( nc_def_var_fill( to , tv , 1 , nullptr ) );

  copy_atts( from , v , to , tv );
  vars.push_back( { from , v , to , tv } );
  }

 // the sub-groups - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 int ng = 0;
 nc_check( nc_inq_grps( from , & ng , nullptr ) );
 std::vector< int > grps( ng );
 if( ng )
  nc_check( nc_inq_grps( from , & ng , grps.data() ) );

 for( auto g : grps ) {
  nc_check( nc_inq_grpname( g , name ) );
  int tg;
  nc_check( nc_def_grp( to , name , & tg ) );
  define_group( g , tg , dims , vars );
  }
 }

/*--------------------------------------------------------------------------*/
// copy the values of the variable c.from_var into the variable c.to_var

static void copy_values( const VarCopy & c )
{
 nc_type type;
 int ndims;
 nc_check( nc_inq_var( c.from_grp , c.from_var , nullptr , & type , & ndims ,
		       nullptr , nullptr ) );
 std::vector< int > vdims( ndims );
 if( ndims )
  nc_check( nc_inq_vardimid( c.from_grp , c.from_var , vdims.data() ) );

 // a scalar is read and written with a start and a count of one element
 std::vector< std::size_t > start( std::max( ndims , 1 ) , 0 );
 std::vector< std::size_t > count( std::max( ndims , 1 ) , 1 );
 std::size_t n = 1;
 for( int i = 0 ; i < ndims ; ++i ) {
  nc_check( nc_inq_dimlen( c.from_grp , vdims[ i ] , & count[ i ] ) );
  n *= count[ i ];
  }
 if( ! n )
  return;

 if( type == NC_STRING ) {
  std::vector< char * > buf( n , nullptr );
  nc_check( nc_get_vara_string( c.from_grp , c.from_var , start.data() ,
				count.data() , buf.data() ) );
  int status = nc_put_vara_string( c.to_grp , c.to_var , start.data() ,
				   count.data() ,
				   const_cast< const char ** >( buf.data() ) );
  nc_free_string( n , buf.data() );
  nc_check( status );
  }
 else {
  std::size_t size;
  nc_check( nc_inq_type( c.from_grp , type , nullptr , & size ) );
  std::vector< unsigned char > buf( n * size );
  nc_check( nc_get_vara( c.from_grp , c.from_var , start.data() ,
			 count.data() , buf.data() ) );
  nc_check( nc_put_vara( c.to_grp , c.to_var , start.data() , count.data() ,
			 buf.data() ) );
  }
 }

/*--------------------------------------------------------------------------*/

// the number of the groups in the group grp, itself included

static int count_groups( int grp )
{
 int ng = 0;
 nc_check( nc_inq_grps( grp , & ng , nullptr ) );
 std::vector< int > grps( ng );
 if( ng )
  nc_check( nc_inq_grps( grp , & ng , grps.data() ) );

 int n = 1;
 for( auto g : grps )
  n += count_groups( g );
 return( n );
 }

/*--------------------------------------------------------------------------*/
// the root group of the file of the group grp

static int root_group( int grp )
{
 for( int parent ; nc_inq_grp_parent( grp , & parent ) == NC_NOERR ; )
  grp = parent;
 return( grp );
 }

/*--------------------------------------------------------------------------*/
// copy each group pairs[ k ].first into the group pairs[ k ].second, all
// the definitions first and all the values after, so that the files of the
// copies pass from the define mode to the data mode once

static void copy_groups( const std::vector< std::pair< int , int > > & pairs )
{
 // netCDF-C (up to 4.9.3 at least) numbers the groups of a file with a
 // short, and crashes at the 32768-th one instead of returning an error
 std::map< int , long > groups;  // the groups each file of a to will have
 for( auto [ from , to ] : pairs ) {
  auto root = root_group( to );
  auto it = groups.try_emplace( root , 0 ).first;
  if( it->second == 0 )
   it->second = count_groups( root );
  it->second += count_groups( from ) - 1;
  if( it->second > max_groups )
   throw( std::length_error( "copy_group: more than " +
			     std::to_string( max_groups ) +
			     " groups in a netCDF file" ) );
  }

 std::vector< VarCopy > vars;
 for( auto [ from , to ] : pairs ) {
  // the ids of the dimensions are those of the file of from
  std::map< int , int > dims;
  define_group( from , to , dims , vars );
  }

 for( const auto & c : vars )
  copy_values( c );
 }

/*--------------------------------------------------------------------------*/

void SMSpp_di_unipi_it::copy_group( const netCDF::NcGroup & from ,
				    netCDF::NcGroup & to )
{
 copy_groups( { { from.getId() , to.getId() } } );
 }

/*--------------------------------------------------------------------------*/

void SMSpp_di_unipi_it::write_via_memory(
	   netCDF::NcGroup & group , const std::vector< std::string > & names ,
	   const std::function< void( std::size_t , netCDF::NcGroup & ) > &
	   write )
{
#if SMSpp_NC_MEMIO
 // each part in an NcMemFile of its own, closed before the next one is
 // written, so that the only datasets open are those of the part; they
 // written are in a sub-group, since the root group of an NcMemFile gives
 // its content in the order of the names once its image is opened again
 std::vector< NcMemFile::Image > images( names.size() );
 for( std::size_t i = 0 ; i < names.size() ; ++i ) {
  NcMemFile mem;
  auto part = mem.addGroup( "part" );
  write( i , part );
  mem.close( images[ i ] );
  }

 // then all of them copied together
 std::vector< std::unique_ptr< NcMemFile > > files;
 files.reserve( names.size() );
 std::vector< std::pair< int , int > > pairs;
 pairs.reserve( names.size() );
 for( std::size_t i = 0 ; i < names.size() ; ++i ) {
  files.push_back( std::make_unique< NcMemFile >(
		     std::span< const unsigned char >( images[ i ] ) ) );
  pairs.emplace_back( files.back()->getGroup( "part" ).getId() ,
		      group.addGroup( names[ i ] ).getId() );
  }
 copy_groups( pairs );
#else
 for( std::size_t i = 0 ; i < names.size() ; ++i ) {
  auto g = group.addGroup( names[ i ] );
  write( i , g );
  }
#endif
 }

/*--------------------------------------------------------------------------*/

#if SMSpp_NC_MEMIO

// the ids of the sub-groups of the group grp, in their order

static std::vector< int > sub_groups( int grp )
{
 int ng = 0;
 nc_check( nc_inq_grps( grp , & ng , nullptr ) );
 std::vector< int > grps( ng );
 if( ng )
  nc_check( nc_inq_grps( grp , & ng , grps.data() ) );
 return( grps );
 }

/*--------------------------------------------------------------------------*/
// the ids of the variables of the group grp, in their order

static std::vector< int > variables( int grp )
{
 int nv = 0;
 nc_check( nc_inq_varids( grp , & nv , nullptr ) );
 std::vector< int > vars( nv );
 if( nv )
  nc_check( nc_inq_varids( grp , & nv , vars.data() ) );
 return( vars );
 }

/*--------------------------------------------------------------------------*/
// the ids of the unlimited dimensions of the group grp

static std::vector< int > unlimited( int grp )
{
 int nu = 0;
 nc_check( nc_inq_unlimdims( grp , & nu , nullptr ) );
 std::vector< int > unlim( nu );
 if( nu )
  nc_check( nc_inq_unlimdims( grp , & nu , unlim.data() ) );
 return( unlim );
 }

/*--------------------------------------------------------------------------*/
// append to sig the attributes of the variable var of grp (of grp itself
// if NC_GLOBAL), names, types and values

static void attributes_signature( int grp , int var , std::string & sig )
{
 int na = 0;
 nc_check( nc_inq_varnatts( grp , var , & na ) );
 char name[ NC_MAX_NAME + 1 ];
 for( int i = 0 ; i < na ; ++i ) {
  nc_check( nc_inq_attname( grp , var , i , name ) );
  nc_type type;
  std::size_t len;
  nc_check( nc_inq_att( grp , var , name , & type , & len ) );
  sig += std::string( "@" ) + name + ":" + std::to_string( type ) + ":" +
         std::to_string( len ) + "=";
  if( type == NC_STRING ) {
   std::vector< char * > s( len , nullptr );
   nc_check( nc_get_att_string( grp , var , name , s.data() ) );
   for( auto c : s )
    sig += std::string( c ? c : "" ) + '\0';
   nc_free_string( len , s.data() );
   }
  else {
   std::size_t size;
   nc_check( nc_inq_type( grp , type , nullptr , & size ) );
   std::string buf( len * size , '\0' );
   if( len )
    nc_check( nc_get_att( grp , var , name , buf.data() ) );
   sig += buf;
   }
  }
 }

/*--------------------------------------------------------------------------*/
// the structure of the tree of the group grp, values of the variables
// apart, as a string: two trees are identical if their strings are

static std::string tree_signature( int grp )
{
 std::string sig;
 char name[ NC_MAX_NAME + 1 ];

 int nd = 0;
 nc_check( nc_inq_dimids( grp , & nd , nullptr , 0 ) );
 std::vector< int > dims( nd );
 if( nd )
  nc_check( nc_inq_dimids( grp , & nd , dims.data() , 0 ) );
 for( auto d : dims ) {
  std::size_t len;
  nc_check( nc_inq_dim( grp , d , name , & len ) );
  const auto unlim = unlimited( grp );
  sig += std::string( "D" ) + name + ":" + std::to_string( len ) +
         ( std::ranges::find( unlim , d ) != unlim.end() ? "U;" : ";" );
  }

 attributes_signature( grp , NC_GLOBAL , sig );

 for( auto v : variables( grp ) ) {
  nc_type type;
  int ndims;
  nc_check( nc_inq_var( grp , v , name , & type , & ndims , nullptr ,
			nullptr ) );
  sig += std::string( "V" ) + name + ":" + std::to_string( type ) + "(";
  std::vector< int > vdims( ndims );
  if( ndims )
   nc_check( nc_inq_vardimid( grp , v , vdims.data() ) );
  for( auto d : vdims ) {
   char dname[ NC_MAX_NAME + 1 ];
   nc_check( nc_inq_dimname( grp , d , dname ) );
   sig += std::string( dname ) + ",";
   }
  sig += ")";
  attributes_signature( grp , v , sig );
  }

 for( auto g : sub_groups( grp ) ) {
  nc_check( nc_inq_grpname( g , name ) );
  sig += std::string( "G" ) + name + "{" + tree_signature( g ) +
         "}";
  }
 return( sig );
 }

/*--------------------------------------------------------------------------*/
// a variable of a stack: where it is, and where it is in each part

namespace {

struct StackedVar {
 int grp;                     ///< the group of the variable in the stack
 int var;                     ///< the variable in the stack
 std::vector< int > from_grp; ///< its group in each part
 std::vector< int > from_var; ///< the variable in each part
 };

}  // end( namespace )

/*--------------------------------------------------------------------------*/
// define in the group to the tree of the groups from (identical trees, one
// per part), each variable with the dimension stack in front of its own

static void define_stacked( const std::vector< int > & from , int to ,
			    int stack , std::vector< StackedVar > & vars )
{
 char name[ NC_MAX_NAME + 1 ];
 const auto first = from.front();

 // the dimensions, by their names, and the attributes of the group
 int nd = 0;
 nc_check( nc_inq_dimids( first , & nd , nullptr , 0 ) );
 std::vector< int > dims( nd );
 if( nd )
  nc_check( nc_inq_dimids( first , & nd , dims.data() , 0 ) );
 const auto unlim = unlimited( first );
 for( auto d : dims ) {
  std::size_t len;
  nc_check( nc_inq_dim( first , d , name , & len ) );
  if( std::ranges::find( unlim , d ) != unlim.end() )
   len = NC_UNLIMITED;
  int td;
  nc_check( nc_def_dim( to , name , len , & td ) );
  }
 copy_atts( first , NC_GLOBAL , to , NC_GLOBAL );

 // the variables, the dimension stack first
 const auto vids = variables( first );
 for( std::size_t j = 0 ; j < vids.size() ; ++j ) {
  nc_type type;
  int ndims;
  nc_check( nc_inq_var( first , vids[ j ] , name , & type , & ndims ,
			nullptr , nullptr ) );
  if( type > NC_STRING )
   throw( std::invalid_argument( std::string( "write_stacked: variable " ) +
				 name + " has a user-defined type" ) );
  std::vector< int > vdims( ndims );
  if( ndims )
   nc_check( nc_inq_vardimid( first , vids[ j ] , vdims.data() ) );
  std::vector< int > sdims( 1 , stack );
  for( auto d : vdims ) {  // by name, in to or above
   char dname[ NC_MAX_NAME + 1 ];
   nc_check( nc_inq_dimname( first , d , dname ) );
   int td;
   nc_check( nc_inq_dimid( to , dname , & td ) );
   sdims.push_back( td );
   }
  int tv;
  nc_check( nc_def_var( to , name , type , sdims.size() , sdims.data() ,
			& tv ) );
  copy_atts( first , vids[ j ] , to , tv );

  StackedVar sv{ to , tv , {} , {} };
  for( auto f : from ) {
   sv.from_grp.push_back( f );
   sv.from_var.push_back( variables( f )[ j ] );
   }
  vars.push_back( std::move( sv ) );
  }

 // the sub-groups, the same in every part
 std::vector< std::vector< int > > subs;
 for( auto f : from )
  subs.push_back( sub_groups( f ) );
 for( std::size_t j = 0 ; j < subs.front().size() ; ++j ) {
  nc_check( nc_inq_grpname( subs.front()[ j ] , name ) );
  int tg;
  nc_check( nc_def_grp( to , name , & tg ) );
  std::vector< int > fj;
  for( const auto & s : subs )
   fj.push_back( s[ j ] );
  define_stacked( fj , tg , stack , vars );
  }
 }

/*--------------------------------------------------------------------------*/
// the shape of the variable var of grp, and the number of its elements

static std::size_t var_shape( int grp , int var ,
			      std::vector< std::size_t > & count )
{
 int ndims;
 nc_check( nc_inq_varndims( grp , var , & ndims ) );
 std::vector< int > vdims( ndims );
 if( ndims )
  nc_check( nc_inq_vardimid( grp , var , vdims.data() ) );
 count.resize( ndims );
 std::size_t n = 1;
 for( int i = 0 ; i < ndims ; ++i ) {
  nc_check( nc_inq_dimlen( grp , vdims[ i ] , & count[ i ] ) );
  n *= count[ i ];
  }
 return( n );
 }

/*--------------------------------------------------------------------------*/
// copy the values of the variable from_var of from_grp, whose shape is
// count, into the variable to_var of to_grp at start (the element of the
// stack first), or the slice at start of from_var into to_var if unstack

static void copy_slice( int from_grp , int from_var , int to_grp ,
			int to_var , std::size_t k ,
			const std::vector< std::size_t > & count ,
			bool unstack )
{
 std::size_t n = 1;
 for( auto c : count )
  n *= c;
 if( ! n )
  return;

 // the stacked side, with k in front, and the plain one
 std::vector< std::size_t > sstart( count.size() + 1 , 0 );
 std::vector< std::size_t > scount( 1 , 1 );
 sstart[ 0 ] = k;
 scount.insert( scount.end() , count.begin() , count.end() );
 std::vector< std::size_t > pstart( std::max< std::size_t >( count.size() ,
							       1 ) , 0 );
 std::vector< std::size_t > pcount( count.begin() , count.end() );
 if( pcount.empty() )
  pcount.push_back( 1 );

 const auto & fstart = unstack ? sstart : pstart;
 const auto & fcount = unstack ? scount : pcount;
 const auto & tstart = unstack ? pstart : sstart;
 const auto & tcount = unstack ? pcount : scount;

 nc_type type;
 nc_check( nc_inq_vartype( from_grp , from_var , & type ) );
 if( type == NC_STRING ) {
  std::vector< char * > buf( n , nullptr );
  nc_check( nc_get_vara_string( from_grp , from_var , fstart.data() ,
				fcount.data() , buf.data() ) );
  int status = nc_put_vara_string( to_grp , to_var , tstart.data() ,
				   tcount.data() ,
				   const_cast< const char ** >( buf.data() ) );
  nc_free_string( n , buf.data() );
  nc_check( status );
  }
 else {
  std::size_t size;
  nc_check( nc_inq_type( from_grp , type , nullptr , & size ) );
  std::vector< unsigned char > buf( n * size );
  nc_check( nc_get_vara( from_grp , from_var , fstart.data() ,
			 fcount.data() , buf.data() ) );
  nc_check( nc_put_vara( to_grp , to_var , tstart.data() , tcount.data() ,
			 buf.data() ) );
  }
 }

/*--------------------------------------------------------------------------*/
// define in the group to the tree of a part of the stack from (the
// dimension stack of its root left out, while that of a stack nested in it
// is kept), and append to vars the pairs of variables

static void define_unstacked( int from , int to , const std::string & stack ,
			      std::vector< VarCopy > & vars )
{
 char name[ NC_MAX_NAME + 1 ];

 int nd = 0;
 nc_check( nc_inq_dimids( from , & nd , nullptr , 0 ) );
 std::vector< int > dims( nd );
 if( nd )
  nc_check( nc_inq_dimids( from , & nd , dims.data() , 0 ) );
 const auto unlim = unlimited( from );
 for( auto d : dims ) {
  std::size_t len;
  nc_check( nc_inq_dim( from , d , name , & len ) );
  if( stack == name )
   continue;
  if( std::ranges::find( unlim , d ) != unlim.end() )
   len = NC_UNLIMITED;
  int td;
  nc_check( nc_def_dim( to , name , len , & td ) );
  }
 copy_atts( from , NC_GLOBAL , to , NC_GLOBAL );

 for( auto v : variables( from ) ) {
  nc_type type;
  int ndims;
  nc_check( nc_inq_var( from , v , name , & type , & ndims , nullptr ,
			nullptr ) );
  std::vector< int > vdims( ndims );
  if( ndims )
   nc_check( nc_inq_vardimid( from , v , vdims.data() ) );
  std::vector< int > pdims;
  for( std::size_t i = 1 ; i < vdims.size() ; ++i ) {
   char dname[ NC_MAX_NAME + 1 ];
   nc_check( nc_inq_dimname( from , vdims[ i ] , dname ) );
   int td;
   nc_check( nc_inq_dimid( to , dname , & td ) );
   pdims.push_back( td );
   }
  int tv;
  nc_check( nc_def_var( to , name , type , pdims.size() , pdims.data() ,
			& tv ) );
  copy_atts( from , v , to , tv );
  vars.push_back( { from , v , to , tv } );
  }

 for( auto g : sub_groups( from ) ) {
  nc_check( nc_inq_grpname( g , name ) );
  int tg;
  nc_check( nc_def_grp( to , name , & tg ) );
  define_unstacked( g , tg , "" , vars );
  }
 }

#endif  // SMSpp_NC_MEMIO

/*--------------------------------------------------------------------------*/
// the level of a stack whose dimension is called name: 0 for
// stacked_dimension, n for stacked_dimension + "_n", -1 if name is not the
// dimension of a stack

static int stack_level( const std::string & name )
{
 const std::string base( stacked_dimension );
 if( name == base )
  return( 0 );
 if( ( name.size() <= base.size() + 1 ) || ( ! name.starts_with( base ) ) ||
     ( name[ base.size() ] != '_' ) )
  return( -1 );
 int n = 0;
 for( auto c : name.substr( base.size() + 1 ) ) {
  if( ( c < '0' ) || ( c > '9' ) || ( n > 100000 ) )
   return( -1 );
  n = 10 * n + ( c - '0' );
  }
 return( n > 0 ? n : -1 );
 }

/*--------------------------------------------------------------------------*/
// the name of the dimension of a stack of level n [see stack_level()]

static std::string stack_name( int n )
{
 return( n ? std::string( stacked_dimension ) + "_" + std::to_string( n )
           : std::string( stacked_dimension ) );
 }

/*--------------------------------------------------------------------------*/
// the name of the dimension of the stack of the group grp (among those of
// grp alone), empty if grp is not the root of a stack

static std::string stack_dimension( int grp )
{
 int nd = 0;
 nc_check( nc_inq_dimids( grp , & nd , nullptr , 0 ) );
 std::vector< int > dims( nd );
 if( nd )
  nc_check( nc_inq_dimids( grp , & nd , dims.data() , 0 ) );
 char name[ NC_MAX_NAME + 1 ];
 for( auto d : dims ) {
  nc_check( nc_inq_dimname( grp , d , name ) );
  if( stack_level( name ) >= 0 )
   return( name );
  }
 return( std::string() );
 }

#if SMSpp_NC_MEMIO

/*--------------------------------------------------------------------------*/
// the highest level of the stacks in the tree of the group grp, -1 if none

static int stack_depth( int grp )
{
 auto name = stack_dimension( grp );
 int depth = name.empty() ? -1 : stack_level( name );
 for( auto g : sub_groups( grp ) )
  depth = std::max( depth , stack_depth( g ) );
 return( depth );
 }

/*--------------------------------------------------------------------------*/
// write in the group to the parts (whose trees are identical) stacked along
// the dimension name of to

static void write_one_stack( const std::vector< int > & parts , int to ,
			     const std::string & name )
{
 // the tree once, all the definitions first and all the values after
 int sdim;
 nc_check( nc_def_dim( to , name.c_str() , parts.size() , & sdim ) );
 std::vector< StackedVar > vars;
 define_stacked( parts , to , sdim , vars );

 std::vector< std::size_t > count;
 for( const auto & v : vars ) {
  var_shape( v.from_grp.front() , v.from_var.front() , count );
  for( std::size_t k = 0 ; k < parts.size() ; ++k )
   copy_slice( v.from_grp[ k ] , v.from_var[ k ] , v.grp , v.var , k ,
	       count , false );
  }
 }

#endif  // SMSpp_NC_MEMIO

/*--------------------------------------------------------------------------*/

bool SMSpp_di_unipi_it::write_stacked(
	   netCDF::NcGroup & group , const std::string & stack ,
	   const std::vector< std::string > & names ,
	   const std::function< void( std::size_t , netCDF::NcGroup & ) > &
	   write )
{
#if SMSpp_NC_MEMIO
 // each part in an NcMemFile of its own, as in write_via_memory()
 std::vector< NcMemFile::Image > images( names.size() );
 for( std::size_t i = 0 ; i < names.size() ; ++i ) {
  NcMemFile mem;
  auto part = mem.addGroup( "part" );
  write( i , part );
  mem.close( images[ i ] );
  }

 std::vector< std::unique_ptr< NcMemFile > > files;
 files.reserve( names.size() );
 std::vector< int > parts;
 parts.reserve( names.size() );
 for( std::size_t i = 0 ; i < names.size() ; ++i ) {
  files.push_back( std::make_unique< NcMemFile >(
		     std::span< const unsigned char >( images[ i ] ) ) );
  parts.push_back( files.back()->getGroup( "part" ).getId() );
  }

 // the root of a part may not have the dimension, which a stack nested
 // deeper in it may have
 bool can = ! parts.empty();
 int depth = -1;
 for( std::size_t i = 0 ; can && ( i < parts.size() ) ; ++i ) {
  can = stack_dimension( parts[ i ] ).empty();
  depth = std::max( depth , stack_depth( parts[ i ] ) );
  }
 // the dimension of the stack, one level above those nested in the parts,
 // since a variable of a nested stack is indexed by both
 const auto name = stack_name( depth + 1 );

 if( ! can ) {  // as write_via_memory()
  std::vector< std::pair< int , int > > pairs;
  pairs.reserve( names.size() );
  for( std::size_t i = 0 ; i < names.size() ; ++i )
   pairs.emplace_back( parts[ i ] , group.addGroup( names[ i ] ).getId() );
  copy_groups( pairs );
  return( false );
  }

 // the classes of the parts with identical trees, in the order of their
 // first part, and where each part is in its class
 std::vector< std::string > sigs;
 std::vector< std::vector< int > > classes;
 std::vector< int > cls( parts.size() );
 std::vector< int > pos( parts.size() );
 for( std::size_t i = 0 ; i < parts.size() ; ++i ) {
  auto sig = tree_signature( parts[ i ] );
  auto it = std::ranges::find( sigs , sig );
  cls[ i ] = int( it - sigs.begin() );
  if( it == sigs.end() ) {
   sigs.push_back( std::move( sig ) );
   classes.emplace_back();
   }
  pos[ i ] = int( classes[ cls[ i ] ].size() );
  classes[ cls[ i ] ].push_back( parts[ i ] );
  }

 auto to = group.addGroup( stack ).getId();
 if( classes.size() == 1 ) {  // one stack
  write_one_stack( parts , to , name );
  return( true );
  }

 // one stack per class, and the index of the parts
 int ndim , vcls , vpos;
 nc_check( nc_def_dim( to , stacked_parts_dimension , parts.size() ,
		       & ndim ) );
 nc_check( nc_def_var( to , stacked_class_variable , NC_INT , 1 , & ndim ,
		       & vcls ) );
 nc_check( nc_def_var( to , stacked_position_variable , NC_INT , 1 ,
		       & ndim , & vpos ) );
 nc_check( nc_put_var_int( to , vcls , cls.data() ) );
 nc_check( nc_put_var_int( to , vpos , pos.data() ) );
 for( std::size_t c = 0 ; c < classes.size() ; ++c ) {
  int tg;
  nc_check( nc_def_grp( to , ( stacked_class_group + std::to_string( c ) )
			.c_str() , & tg ) );
  write_one_stack( classes[ c ] , tg , name );
  }
 return( true );
#else
 write_via_memory( group , names , write );
 return( false );
#endif
 }

/*--------------------------------------------------------------------------*/

std::size_t SMSpp_di_unipi_it::stacked_size( const netCDF::NcGroup & group )
{
 if( group.isNull() )
  return( 0 );
 if( auto name = stack_dimension( group.getId() ) ; ! name.empty() )
  return( group.getDim( name , netCDF::NcGroup::Current ).getSize() );
 auto dim = group.getDim( stacked_parts_dimension , netCDF::NcGroup::Current );
 if( dim.isNull() || group.getVar( stacked_class_variable ).isNull() ||
     group.getVar( stacked_position_variable ).isNull() )
  return( 0 );
 return( dim.getSize() );
 }

/*--------------------------------------------------------------------------*/

void SMSpp_di_unipi_it::read_stacked(
	   const netCDF::NcGroup & stack , std::size_t i ,
	   const std::function< void( const netCDF::NcGroup & ) > & read )
{
 if( i >= stacked_size( stack ) )
  throw( std::out_of_range( "read_stacked: part " + std::to_string( i ) +
			    " out of the stack" ) );

 // one stack per class: the part is in that of its class
 const auto name = stack_dimension( stack.getId() );
 if( name.empty() ) {
  int c , p;
  stack.getVar( stacked_class_variable ).getVar( { i } , & c );
  stack.getVar( stacked_position_variable ).getVar( { i } , & p );
  auto sub = stack.getGroup( stacked_class_group + std::to_string( c ) ,
			     netCDF::NcGroup::ChildrenGrps );
  if( sub.isNull() || ( p < 0 ) ||
      ( std::size_t( p ) >= stacked_size( sub ) ) )
   throw( std::invalid_argument( "read_stacked: part " +
				 std::to_string( i ) + " in " +
				 stacked_class_group + std::to_string( c ) +
				 " at " + std::to_string( p ) +
				 ", which is not there" ) );
  read_stacked( sub , std::size_t( p ) , read );
  return;
  }

#if SMSpp_NC_MEMIO
 NcMemFile mem;
 auto part = mem.addGroup( "part" );
 std::vector< VarCopy > vars;
 define_unstacked( stack.getId() , part.getId() , name , vars );

 // the shape of a part is that of the stack without its first dimension,
 // since an unlimited one of the part is still empty
 std::vector< std::size_t > count;
 for( const auto & c : vars ) {
  var_shape( c.from_grp , c.from_var , count );
  count.erase( count.begin() );
  copy_slice( c.from_grp , c.from_var , c.to_grp , c.to_var , i , count ,
	      true );
  }
 read( part );
#else
 throw( std::logic_error( "read_stacked: a netCDF-C earlier than 4.6.2 has "
			  "no in-memory files" ) );
#endif
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- METHODS of NcMemFile --------------------------*/
/*--------------------------------------------------------------------------*/

NcMemFile::NcMemFile( void ) : netCDF::NcFile()
{
#if SMSpp_NC_MEMIO
 netCDF::ncCheck( nc_create_mem( mem_path , NC_NETCDF4 , 0 ,
				 & myId ) , __FILE__ , __LINE__ );
 nullObject = false;
#else
 throw( std::logic_error( "NcMemFile::NcMemFile: in-memory files need "
			  "netCDF-C 4.6.2 or later" ) );
#endif
 }

/*--------------------------------------------------------------------------*/

NcMemFile::NcMemFile( std::span< const unsigned char > image )
 : netCDF::NcFile()
{
#if SMSpp_NC_MEMIO
 if( image.empty() )
  throw( std::invalid_argument( "NcMemFile::NcMemFile: empty image" ) );

 #ifdef SMSpp_HAVE_HDF5
  // netCDF turns off the printing of the errors of HDF5 only in the thread
  // that first uses it, while a thread-safe HDF5 keeps it per thread: in
  // another thread (say, one of those of an InvestmentFunction, each reading
  // a copy of an SDDPBlock) every attribute that netCDF looks for and does
  // not find (the _Quantize ones of each variable) would print an error
  H5Eset_auto2( H5E_DEFAULT , nullptr , nullptr );
 #endif

 // the bytes of the caller, never written, reallocated or freed
 NC_memio info;
 info.size = image.size();
 info.memory = const_cast< unsigned char * >( image.data() );
 info.flags = NC_MEMIO_LOCKED;

 netCDF::ncCheck( nc_open_memio( mem_path , NC_NOWRITE , & info ,
				 & myId ) , __FILE__ , __LINE__ );
 nullObject = false;
 f_read = true;
#else
 (void) image;
 throw( std::logic_error( "NcMemFile::NcMemFile: in-memory files need "
			  "netCDF-C 4.6.2 or later" ) );
#endif
 }

/*--------------------------------------------------------------------------*/

void NcMemFile::close( Image & image )
{
 if( nullObject )
  throw( std::logic_error( "NcMemFile::close: the file is not open" ) );

#if SMSpp_NC_MEMIO
 NC_memio info{};
 // the file is closed whatever happens, as netCDF::NcFile::close() does
 nullObject = true;
 netCDF::ncCheck( nc_close_memio( myId , & info ) , __FILE__ , __LINE__ );

 auto bytes = static_cast< const unsigned char * >( info.memory );
 try {
  image.assign( bytes , bytes + info.size );
  }
 catch( ... ) {
  if( ! f_read )
   std::free( info.memory );
  throw;
  }
 if( ! f_read )
  std::free( info.memory );
#else
 (void) image;
#endif
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File SMSnetCDF.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
