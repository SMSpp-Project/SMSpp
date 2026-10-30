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
