/*--------------------------------------------------------------------------*/
/*-------------------------- File NcMemFile.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the NcMemFile class.
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

#include "NcMemFile.h"

#include <cstdlib>
#include <stdexcept>

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
/*------------------------ End File NcMemFile.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
