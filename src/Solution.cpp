/*--------------------------------------------------------------------------*/
/*--------------------------- File Solution.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the Solution class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Solution.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

std::string Solution::f_prefix;  // the filename prefix

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*---------------------------- METHODS of Solution -------------------------*/
/*--------------------------------------------------------------------------*/

Solution * Solution::deserialize( const std::string & filename )
{
 int idx = 0;
 std::string fn = f_prefix + filename;
 if( fn.back() == ']' ) {
  auto pos = fn.find_last_of( '[' );
  if( pos != std::string::npos ) {
   try {
    idx = std::stoi( fn.substr( pos + 1 ) );
    fn.erase( pos );
    }
   catch( ... ) { idx = 0; }
   }
  }
 try {
  netCDF::NcFile f( fn.c_str() , netCDF::NcFile::read );
  return( Solution::deserialize( f , idx ) );
  }
 catch( netCDF::exceptions::NcException & e ) {
  std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
  }
 catch( std::exception & e ) {
  std::cerr << "error " << e.what() << " in deserialize" << std::endl;
  }
 catch( ... ) {
  std::cerr << "unknown error in deserialize" << std::endl;
  }

 return( nullptr );

 }  // end( Solution::deserialize( const std::string ) )

/*--------------------------------------------------------------------------*/

Solution * Solution::deserialize( const netCDF::NcFile & f , int idx )
{
 try {
  auto gtype = f.getAtt( "SMS++_file_type" );
  if( gtype.isNull() )
   return( nullptr );

  int type;
  gtype.getValues( & type );

  if( type != eSolutionFile )
   return( nullptr );

  auto cg = f.getGroup( "Solution_" + std::to_string( idx ) );

  return( new_Solution( cg ) );
  }
 catch( netCDF::exceptions::NcException & e ) {
  std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
  }
 catch( std::exception & e ) {
  std::cerr << "error " << e.what() << " in deserialize" << std::endl;
  }
 catch( ... ) {
  std::cerr << "unknown error in deserialize" << std::endl;
  }

 return( nullptr );

 }  // end( Solution::deserialize( netCDF::NcFile ) )

/*--------------------------------------------------------------------------*/

Solution * Solution::new_Solution( const netCDF::NcGroup & group )
{
 try {
  if( group.isNull() )
   return( nullptr );

  std::string tmp;
  auto gtype = group.getAtt( "type" );
  if( gtype.isNull() ) {
   auto gfile = group.getAtt( "filename" );
   if( gfile.isNull() )
    return( nullptr );

   gfile.getValues( tmp );

   return( deserialize( tmp ) );
   }

  gtype.getValues( tmp );
  auto result = new_Solution( tmp );
  result->deserialize( group );
  return( result );
  }
 catch( netCDF::exceptions::NcException & e ) {
  std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
  }
 catch( std::exception & e ) {
  std::cerr << "error " << e.what() << " in deserialize" << std::endl;
  }
 catch( ... ) {
  std::cerr << "unknown error in deserialize" << std::endl;
  }

 return( nullptr );

 }  // end( Solution::new_Solution( netCDF::NcGroup ) )

/*----------------------------------------------------------------------------

Solution::SolutionFactoryMap & Solution::f_factory( void ) {
 static SolutionFactoryMap s_factory;
 return( s_factory );
 }

----------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*----------------------- End File Solution.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
