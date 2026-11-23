/*--------------------------------------------------------------------------*/
/*-------------------------- File SMSFactory.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of SMSpp_load_plugin(), which loads the shared library
 * that registers a class that the factories do not hold.
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

// the paths of std::filesystem, so that nothing has to be linked for them
#define BOOST_DLL_USE_STD_FS

#include "SMSFactory.h"

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/dll/shared_library.hpp>

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

namespace fs = std::filesystem;

/*--------------------------------------------------------------------------*/
/*----------------------------- LOCAL TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

/// the tables, read once, and the plugins loaded, under the lock of the
/// factories
struct Plugins {
 bool read = false;
 std::map< std::string , fs::path > table;  ///< class -> plugin
 std::map< fs::path , boost::dll::shared_library > loaded;
 };

/*--------------------------------------------------------------------------*/
/// the plugins, never destroyed: no plugin is unloaded while its classes,
/// in the factories and in the static objects, may still be used

Plugins & plugins( void ) {
 static Plugins * p = new Plugins;
 return( *p );
 }

/*--------------------------------------------------------------------------*/
/// the directories of the tables, in the order in which they are read

std::vector< fs::path > plugin_dirs( void ) {
 std::vector< fs::path > dirs;
 if( const char * env = std::getenv( "SMSpp_PLUGIN_PATH" ) ) {
  // the separator of PATH: ';' where the directories are separated by '\'
  const char sep = ( fs::path::preferred_separator == '\\' ) ? ';' : ':';
  std::stringstream ss( env );
  for( std::string d ; std::getline( ss , d , sep ) ; )
   if( ! d.empty() )
    dirs.emplace_back( d );
  }

 #ifdef SMSpp_PLUGIN_BUILD_DIR
  dirs.emplace_back( SMSpp_PLUGIN_BUILD_DIR );
 #endif

 // the directory of the library of SMS++, and the one of the libraries of
 // its installation, if it is in that of the programs (as a DLL is)
 std::error_code ec;
 const auto self = boost::dll::this_line_location( ec );
 if( ! ec ) {
  dirs.push_back( self.parent_path() );
  #ifdef SMSpp_PLUGIN_FROM_BIN
   const auto lib = ( self.parent_path() /
		      SMSpp_PLUGIN_FROM_BIN ).lexically_normal();
   if( lib != dirs.back().lexically_normal() )
    dirs.push_back( lib );
  #endif
  }

 return( dirs );
 }

/*--------------------------------------------------------------------------*/
/// reads the tables: a class is taken from the first directory that has it

void read_tables( Plugins & p ) {
 for( const auto & d : plugin_dirs() ) {
  std::error_code ec;
  for( fs::directory_iterator it( d , ec ) , end ;
       ( ! ec ) && ( it != end ) ; it.increment( ec ) ) {
   if( it->path().extension() != ".smspp" )
    continue;
   std::ifstream f( it->path() );
   for( std::string line ; std::getline( f , line ) ; ) {
    std::istringstream ls( line );
    std::string name , file;
    if( ! ( ls >> name ) )
     continue;
    std::getline( ls >> std::ws , file );
    while( ( ! file.empty() ) && std::isspace(
				  static_cast< unsigned char >( file.back() ) ) )
     file.pop_back();
    if( ! file.empty() )
     p.table.emplace( name , it->path().parent_path() / file );
    }
   }
  }
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- FUNCTIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

std::shared_mutex & SMSpp_di_unipi_it::SMSpp_factory_mutex( void )
{
 static std::shared_mutex m;
 return( m );
 }

/*--------------------------------------------------------------------------*/

bool SMSpp_di_unipi_it::SMSpp_load_plugin( const std::string & classname )
{
 // the plugin registers its classes in the factories while it is loaded
 std::unique_lock< std::shared_mutex > lock( SMSpp_factory_mutex() );
 auto & p = plugins();
 if( ! p.read ) {
  read_tables( p );
  p.read = true;
  }

 const auto it = p.table.find( classname );
 if( ( it == p.table.end() ) || p.loaded.count( it->second ) )
  return( false );

 // its symbols are global and resolved at once, as if it were linked
 try {
  p.loaded.emplace( it->second , boost::dll::shared_library( it->second ,
			  boost::dll::load_mode::rtld_global |
			  boost::dll::load_mode::rtld_now ) );
  }
 catch( const std::exception & e ) {
  throw( std::runtime_error( "SMSpp_load_plugin: " + classname +
			     " is in " + it->second.string() +
			     ", which cannot be loaded: " + e.what() ) );
  }

 return( true );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File SMSFactory.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
