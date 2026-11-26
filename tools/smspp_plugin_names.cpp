/*--------------------------------------------------------------------------*/
/*----------------------- File smspp_plugin_names.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Writes the table of a plugin, the file that tells SMSpp_load_plugin()
 * which classes the plugin registers [see SMSFactory.h]:
 *
 *     smspp_plugin_names <table> <plugin> [<library> ...]
 *
 * loads the libraries, which the plugin links and whose classes are not its
 * own, then the plugin, and writes in the table a line with the name of
 * each :Block and :Solver that the plugin has put in the factories, and the
 * path of the plugin relative to the directory of the table (its file name,
 * where the table is next to it).
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

// the paths of std::filesystem, so that nothing has to be linked for them
#define BOOST_DLL_USE_STD_FS

#include "Block.h"
#include "Solver.h"

#include <boost/dll/shared_library.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*-------------------------------- FUNCTIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the names of the :Block and of the :Solver in the factories, sorted

static std::vector< std::string > names( void ) {
 auto n = Block::get_factory_names();
 const auto s = Solver::get_factory_names();
 n.insert( n.end() , s.begin() , s.end() );
 std::sort( n.begin() , n.end() );
 return( n );
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 if( argc < 3 ) {
  std::cerr << "Usage: " << argv[ 0 ] << " <table> <plugin> [<library> ...]"
	    << std::endl;
  return( 1 );
  }

 const auto mode = boost::dll::load_mode::rtld_global |
                   boost::dll::load_mode::rtld_now;
 // never destroyed: the libraries stay loaded, as their classes are in the
 // factories until the end
 auto & libs = * new std::vector< boost::dll::shared_library >;
 try {
  for( int i = 3 ; i < argc ; ++i )
   libs.emplace_back( argv[ i ] , mode );
  const auto before = names();
  const std::filesystem::path plugin( argv[ 2 ] );
  libs.emplace_back( plugin , mode );
  const auto after = names();

  std::vector< std::string > own;
  std::set_difference( after.begin() , after.end() ,
		       before.begin() , before.end() ,
		       std::back_inserter( own ) );

  // the plugin as seen from the directory of the table, where it is read
  const std::filesystem::path path( argv[ 1 ] );
  auto file = std::filesystem::absolute( plugin ).lexically_relative(
	        std::filesystem::absolute( path ).parent_path() );
  if( file.empty() )
   file = std::filesystem::absolute( plugin );

  std::ofstream table( path );
  for( const auto & n : own )
   table << n << " " << file.generic_string() << "\n";
  if( ! table ) {
   std::cerr << argv[ 0 ] << ": cannot write " << argv[ 1 ] << std::endl;
   return( 1 );
   }
  }
 catch( const std::exception & e ) {
  std::cerr << argv[ 0 ] << ": " << e.what() << std::endl;
  return( 1 );
  }

 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File smspp_plugin_names.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
