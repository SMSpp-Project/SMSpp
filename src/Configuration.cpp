/*--------------------------------------------------------------------------*/
/*------------------------ File Configuration.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the Configuration class.
 *
 * \version 0.11
 *
 * \date 27 - 10 - 2019
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Configuration.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register various versions of SimpleConfiguration<> to the factory

SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration< int > );

SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration< double > );

SMSpp_insert_in_factory_cpp_0_t(
 ( SimpleConfiguration< std::pair< int, int > > ) );

SMSpp_insert_in_factory_cpp_0_t(
 ( SimpleConfiguration< std::pair< double, double > > ) );

SMSpp_insert_in_factory_cpp_0_t(
 ( SimpleConfiguration< std::pair< int, double > > ) );

SMSpp_insert_in_factory_cpp_0_t(
 ( SimpleConfiguration< std::pair< double, int > > ) );

SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration< std::vector< int > > );

SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration< std::vector< double > > );

SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration< std::list< int > > );

SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration< std::list< double > > );

SMSpp_insert_in_factory_cpp_0_t(
 ( SimpleConfiguration< std::pair< Configuration *, Configuration * > > ) );

SMSpp_insert_in_factory_cpp_0_t(
 SimpleConfiguration< std::vector< Configuration * > > );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

std::string Configuration::f_prefix;  // the filename prefix

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------- METHODS of Configuration -----------------------*/
/*--------------------------------------------------------------------------*/

Configuration * Configuration::deserialize( const std::string & filename )
{
 try {
  if( ( filename.size() > 4 ) &&
      ( ! filename.compare( filename.size() - 4 , 4 , ".txt" ) ) ) {
   std::ifstream f( f_prefix.empty() ? filename : f_prefix + filename ,
		    std::fstream::in );
   if( ! f.is_open() ) {
    std::cerr << "Error: cannot open text file " <<  f_prefix + filename
	      << std::endl;
    return( nullptr );
    }
   return( Configuration::deserialize( f ) );
   }
  else {
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
   netCDF::NcFile f( fn.c_str() , netCDF::NcFile::read );
   return( Configuration::deserialize( f , idx ) );
   }
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

 }  // end( Configuration::deserialize( const std::string ) )

/*--------------------------------------------------------------------------*/

Configuration * Configuration::deserialize( const netCDF::NcFile & f ,
					    int idx )
{
 try {
  auto gtype = f.getAtt( "SMS++_file_type" );
  if( gtype.isNull() )
   return( nullptr );

  int type;
  gtype.getValues( & type );

  if( ( type != eProbFile ) && ( type != eConfigFile ) )
   return( nullptr );

  netCDF::NcGroup cg;
  if( type == eProbFile ) {
   netCDF::NcGroup dg = f.getGroup( "Prob_" +
			        std::to_string( idx >= 0 ? idx : 1 - idx ) );
   if( dg.isNull() )
    return( nullptr );

   cg = dg.getGroup( ( idx >= 0 ? "BlockConfig" : "BlockSolver" ) );
   }
  else
   cg = f.getGroup( "Config_" + std::to_string( idx ) );

  return( new_Configuration( cg ) );
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

 }  // end( Configuration::deserialize( netCDF::NcFile ) )

/*--------------------------------------------------------------------------*/

Configuration * Configuration::new_Configuration(
					     const netCDF::NcGroup & group )
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
  auto result = new_Configuration( tmp );
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

 }  // end( Configuration::new_Configuration( netCDF::NcGroup ) )

/*--------------------------------------------------------------------------*/

Configuration * Configuration::deserialize( std::istream & input )
{
 input >> eatcomments;
 if( input.eof() )
  return( nullptr );
 
 static std::string sre( "Configuration::deserialize: stream read error" );

 if( input.fail() )
  throw( std::invalid_argument( sre ) );

 std::string tmp;
 if( input.peek() == input.widen( '*' ) ) {
  input.get();

  if( input.eof() )
   return( nullptr );
  
  if( input.fail() )
   throw( std::invalid_argument( sre ) );

  if( std::isspace( input.peek() ) )
   return( nullptr );
 
  input >> tmp;
  if( input.fail() )
    throw( std::invalid_argument( sre ) );

  return( Configuration::deserialize( tmp ) );
  }
 else {
  input >> tmp;
  if( input.fail() )
    throw( std::invalid_argument( sre ) );

  auto cfg = Configuration::new_Configuration( tmp );
  input >> *cfg;
  return( cfg );
  }
 }  // end( Configuration::deserialize( std::istream ) )

/*--------------------------------------------------------------------------*/

Configuration::ConfigurationFactoryMap & Configuration::f_factory( void ) {
 static ConfigurationFactoryMap s_factory;
 return( s_factory );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- Methods of SimpleConfiguration ---------------------*/
/*--------------------------------------------------------------------------*/
// these are the variants of serialize() and deserialize() for all the
// "basic" versions of SimpleConfiguration

template<>
void SimpleConfiguration< std::pair< Configuration * , Configuration * >
			  >::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );
 if( f_value.first ) {
  auto cg = group.addGroup( "value_f" );
  f_value.first->serialize( cg );
  }
 if( f_value.second ) {
  auto cg = group.addGroup( "value_s" );
  f_value.second->serialize( cg );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::pair< Configuration * , Configuration * >
			  >::deserialize( const netCDF::NcGroup & group )
{
 Configuration::deserialize( group );
 auto sc = group.getGroup( "value_f" );
 f_value.first = new_Configuration( sc );
 sc = group.getGroup( "value_s" );
 f_value.second = new_Configuration( sc );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::pair< Configuration * , Configuration * >
			  >::clear( void )
{
 if( f_value.first )
  f_value.first->clear();
 if( f_value.second )
  f_value.second->clear();
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::vector< Configuration * > >::clear( void ) {
 for( auto config : f_value )
  if( config )
   config->clear();
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File Configuration.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
