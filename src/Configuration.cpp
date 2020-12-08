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
using namespace std;

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
   std::ifstream f( filename , std::fstream::in );
   if( ! f.is_open() ) {
    std::cerr << "Error: cannot open text file " << filename << std::endl;
    return( nullptr );
    }
   return( Configuration::deserialize( f ) );
   }
  else {
   int idx = 0;
   std::string fn;
   if( filename.back() == ']' ) {
    auto pos = filename.find_last_of( '[' );
    if( pos != std::string::npos ) {
     try {
      idx = std::stoi( filename.substr( pos + 1 ) );
      fn = filename.substr( 0 , pos );
      }
     catch( ... ) { idx = 0; }
     }
    }
   netCDF::NcFile f( fn.empty() ? filename.c_str() : fn.c_str() ,
		     netCDF::NcFile::read );
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

Configuration * Configuration::deserialize( netCDF::NcFile & f , int idx )
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

Configuration * Configuration::new_Configuration( netCDF::NcGroup & group )
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
void SimpleConfiguration< int >::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );
 ( group.addVar( "value" , netCDF::NcInt() ) ).putVar( &f_value );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< int >::deserialize( netCDF::NcGroup & group )
{
 Configuration::deserialize( group );
 ( group.getVar( "value" ) ).getVar( &f_value );
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< double >::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );
 ( group.addVar( "value" , netCDF::NcDouble() ) ).putVar( &f_value );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< double >::deserialize( netCDF::NcGroup & group )
{
 Configuration::deserialize( group );
 ( group.getVar( "value" ) ).getVar( &f_value );
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::pair< int , int >
			  >::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );
 ( group.addVar( "value_f" , netCDF::NcInt() ) ).putVar( &f_value.first );
 ( group.addVar( "value_s" , netCDF::NcInt() ) ).putVar( &f_value.second );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::pair< int , int >
			  >::deserialize( netCDF::NcGroup & group )
{
 Configuration::deserialize( group );
 ( group.getVar( "value_f" ) ).getVar( &f_value.first );
 ( group.getVar( "value_s" ) ).getVar( &f_value.second );
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::pair< double , double >
			  >::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );
 ( group.addVar( "value_f" , netCDF::NcDouble() ) ).putVar( &f_value.first );
 ( group.addVar( "value_s" , netCDF::NcDouble() ) ).putVar( &f_value.second );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::pair< double , double >
			  >::deserialize( netCDF::NcGroup & group )
{
 Configuration::deserialize( group );
 ( group.getVar( "value_f" ) ).getVar( &f_value.first );
 ( group.getVar( "value_s" ) ).getVar( &f_value.second );
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::pair< int, double >
			  >::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );
 ( group.addVar( "value_f" , netCDF::NcInt() ) ).putVar( &f_value.first );
 ( group.addVar( "value_s" , netCDF::NcDouble() ) ).putVar( &f_value.second );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::pair< int , double >
			  >::deserialize( netCDF::NcGroup & group )
{
 Configuration::deserialize( group );
 ( group.getVar( "value_f" ) ).getVar( &f_value.first );
 ( group.getVar( "value_s" ) ).getVar( &f_value.second );
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::pair< double , int >
			  >::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );
 ( group.addVar( "value_f", netCDF::NcDouble() ) ).putVar( &f_value.first );
 ( group.addVar( "value_s", netCDF::NcInt() ) ).putVar( &f_value.second );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::pair< double , int >
			  >::deserialize( netCDF::NcGroup & group )
{
 Configuration::deserialize( group );
 ( group.getVar( "value_f" ) ).getVar( &f_value.first );
 ( group.getVar( "value_s" ) ).getVar( &f_value.second );
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::vector< int >
			  >::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );
 auto sz = group.addDim( "size", f_value.size() );
 std::vector< size_t > startp = { 0 };
 std::vector< size_t > countp = { f_value.size() };
 ( group.addVar( "value", netCDF::NcInt(), sz ) ).putVar( startp , countp ,
                                                          f_value.data() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::vector< int >
			  >::deserialize( netCDF::NcGroup & group )
{
 Configuration::deserialize( group );
 auto dim = group.getDim( "size" );
 if( dim.isNull() ) {
  f_value.clear();
  return;
  }
 size_t size = dim.getSize();
 f_value.resize( size );
 std::vector< size_t > start = { 0 };
 std::vector< size_t > count = { size };
 ( group.getVar( "value" ) ).getVar( start , count , f_value.data() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::vector< int > >::clear( void ) {
 f_value.clear();
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::vector< double >
			  >::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );
 auto sz = group.addDim( "size" , f_value.size() );
 std::vector< size_t > startp = { 0 };
 std::vector< size_t > countp = { f_value.size() };
 ( group.addVar( "value", netCDF::NcDouble(), sz ) ).putVar( startp , countp ,
                                                             f_value.data() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::vector< double >
			  >::deserialize( netCDF::NcGroup & group )
{
 Configuration::deserialize( group );
 auto dim = group.getDim( "size" );
 if( dim.isNull() ) {
  f_value.clear();
  return;
  }
 size_t size = dim.getSize();
 f_value.resize( size );
 std::vector< size_t > start = { 0 };
 std::vector< size_t > count = { size };
 ( group.getVar( "value" ) ).getVar( start , count , f_value.data() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::vector< double > >::clear( void ) {
 f_value.clear();
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::list< int >
			  >::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );
 auto sz = group.addDim( "size" , f_value.size() );
 auto it = f_value.begin();
 auto var = group.addVar( "value" , netCDF::NcInt() , sz );
 for( size_t i = 0 ; i < f_value.size() ; )
  var.putVar( { i++ }, *( it++ ) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::list< int >
			  >::deserialize( netCDF::NcGroup & group )
{
 Configuration::deserialize( group );
 f_value.clear();
 auto dim = group.getDim( "size" );
 if( dim.isNull() )
  return;
 size_t size = dim.getSize();
 auto var = group.getVar( "value" );
 for( size_t i = 0 ; i < size ; ) {
  int val;
  var.getVar( { i++ }, &val );
  f_value.push_back( val );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::list< int > >::clear( void ) {
 f_value.clear();
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::list< double >
			  >::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );
 auto sz = group.addDim( "size" , f_value.size() );
 auto it = f_value.begin();
 auto var = group.addVar( "value" , netCDF::NcDouble() , sz );
 for( size_t i = 0 ; i < f_value.size() ; )
  var.putVar( { i++ }, *( it++ ) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::list< double >
			  >::deserialize( netCDF::NcGroup & group )
{
 Configuration::deserialize( group );
 f_value.clear();
 auto dim = group.getDim( "size" );
 if( dim.isNull() )
  return;
 size_t size = dim.getSize();
 auto var = group.getVar( "value" );
 for( size_t i = 0 ; i < size ; ) {
  double val;
  var.getVar( { i++ } , &val );
  f_value.push_back( val );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::list< double > >::clear( void ) {
 f_value.clear();
 }

/*--------------------------------------------------------------------------*/

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
			  >::deserialize( netCDF::NcGroup & group )
{
 Configuration::deserialize( group );
 auto sc = group.getGroup( "value_f" );
 f_value.first = new_Configuration( sc );
 sc = group.getGroup( "value_s" );
 f_value.second = new_Configuration( sc );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/*!!
template<>
void SimpleConfiguration< std::pair< Configuration * , Configuration * >
			  >::load( std::istream & input )
{
 f_value.first = Configuration::deserialize( input );
 f_value.second = input.eof() ? nullptr : Configuration::deserialize( input );
 }
 !!*/

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

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::vector< Configuration * >
			  >::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );
 auto sz = group.addDim( "size", f_value.size() );

 for( size_t i = 0 ; i < f_value.size() ; ++i ) {
  auto ci = group.addGroup( "Config_" + std::to_string( i ) );
  f_value[ i ]->serialize( ci );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::vector< Configuration * >
			  >::deserialize( netCDF::NcGroup & group )
{
 Configuration::deserialize( group );
 auto dim = group.getDim( "size" );
 for( auto ptr : f_value )
  delete ptr;
 if( dim.isNull() ) {
  f_value.clear();
  return;
  }
 size_t size = dim.getSize();
 f_value.resize( size );
 for( size_t i = 0 ; i < f_value.size() ; ++i ) {
  auto ci = group.getGroup( "Config_" + std::to_string( i ) );
  f_value[ i ] = new_Configuration( ci );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/*!!
template<>
void SimpleConfiguration< std::vector< Configuration * >
			  >::load( std::istream & input )
{
 if( input.fail() )
  throw( std::invalid_argument( "SimpleConfiguration::load: stream read error"
				) );
 for( auto ptr : f_value )
  delete ptr;

 input >> eatcomments;

 int dim = 0;
 if( ! input.eof() )
  input >> dim;

 if( ! dim ) {
  f_value.clear();
  return;
  }

 f_value.resize( dim , nullptr );
 for( auto & el : f_value ) {
  input >> eatcomments;
  if( input.eof() )
   return;

  el = Configuration::deserialize( input );
  }
 }
 !!*/

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
