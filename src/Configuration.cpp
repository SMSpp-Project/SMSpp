/*--------------------------------------------------------------------------*/
/*------------------------ File Configuration.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the Configuration class.
 *
 * \version 0.10
 *
 * \date 16 - 09 - 2018
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni
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

SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration<int> );
SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration<double> );
using SimpleConfig_i_i = SimpleConfiguration< std::pair<int,int> >;
SMSpp_insert_in_factory_cpp_0_t( SimpleConfig_i_i );
using SimpleConfig_d_d = SimpleConfiguration< std::pair<double,double> >;
SMSpp_insert_in_factory_cpp_0_t( SimpleConfig_d_d );
using SimpleConfig_i_d = SimpleConfiguration< std::pair<int,double> >;
SMSpp_insert_in_factory_cpp_0_t( SimpleConfig_i_d );
using SimpleConfig_d_i = SimpleConfiguration< std::pair<double,int> >;
SMSpp_insert_in_factory_cpp_0_t( SimpleConfig_d_i );
SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration< std::vector<int> > );
SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration< std::vector<double> > );
SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration< std::list<int> > );
SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration< std::list<double> > );

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------- METHODS of Configuration -----------------------*/
/*--------------------------------------------------------------------------*/
// only one, that of the factory

Configuration::ConfigurationFactoryMap & Configuration::f_factory( void )
{
 static ConfigurationFactoryMap s_factory;
 return( s_factory );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- Methods of SimpleConfiguration ---------------------*/
/*--------------------------------------------------------------------------*/
// these are the variants of serialize() and deserialize() for all the
// "basic" versions of SimpleConfiguration

template<>
void SimpleConfiguration<int>::serialize( netCDF::NcGroup && group ) const
{
 Configuration::serialize( std::move( group ) );
 ( group.addVar( "value" , netCDF::NcInt() ) ).putVar( & f_value );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration<int>::deserialize( netCDF::NcGroup && group )
{
 Configuration::deserialize( std::move( group ) );
 ( group.getVar( "value" ) ).getVar( & f_value );
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration<double>::serialize( netCDF::NcGroup && group ) const
{
 Configuration::serialize( std::move( group ) );
 ( group.addVar( "value" , netCDF::NcDouble() ) ).putVar( & f_value );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration<double>::deserialize( netCDF::NcGroup && group )
{
 Configuration::deserialize( std::move( group ) );
 ( group.getVar( "value" ) ).getVar( & f_value );
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::pair<int,int> >::serialize(
					      netCDF::NcGroup && group ) const
{
 Configuration::serialize( std::move( group ) );
 ( group.addVar( "value_f" , netCDF::NcInt() ) ).putVar( & f_value.first );
 ( group.addVar( "value_s" , netCDF::NcInt() ) ).putVar( & f_value.second );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::pair<int,int> >::deserialize(
						    netCDF::NcGroup && group )
 
{
 Configuration::deserialize( std::move( group ) );
 ( group.getVar( "value_f" ) ).getVar( & f_value.first );
 ( group.getVar( "value_s" ) ).getVar( & f_value.second );
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::pair<double,double> >::serialize(
					     netCDF::NcGroup && group ) const
{
 Configuration::serialize( std::move( group ) );
 ( group.addVar( "value_f" , netCDF::NcDouble() ) ).putVar( & f_value.first );
 ( group.addVar( "value_s" , netCDF::NcDouble() ) ).putVar( & f_value.second );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::pair<double,double> >::deserialize(
						    netCDF::NcGroup && group )
 
{
 Configuration::deserialize( std::move( group ) );
 ( group.getVar( "value_f" ) ).getVar( & f_value.first );
 ( group.getVar( "value_s" ) ).getVar( & f_value.second );
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::pair<int,double> >::serialize(
					     netCDF::NcGroup && group ) const
{
 Configuration::serialize( std::move( group ) );
 ( group.addVar( "value_f" , netCDF::NcInt() ) ).putVar( & f_value.first );
 ( group.addVar( "value_s" , netCDF::NcDouble() ) ).putVar( & f_value.second );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::pair<int,double> >::deserialize(
						    netCDF::NcGroup && group )
 
{
 Configuration::deserialize( std::move( group ) );
 ( group.getVar( "value_f" ) ).getVar( & f_value.first );
 ( group.getVar( "value_s" ) ).getVar( & f_value.second );
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::pair<double,int> >::serialize(
					     netCDF::NcGroup && group ) const
{
 Configuration::serialize( std::move( group ) );
 ( group.addVar( "value_f" , netCDF::NcDouble() ) ).putVar( & f_value.first );
 ( group.addVar( "value_s" , netCDF::NcInt() ) ).putVar( & f_value.second );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::pair<double,int> >::deserialize(
						    netCDF::NcGroup && group )
 
{
 Configuration::deserialize( std::move( group ) );
 ( group.getVar( "value_f" ) ).getVar( & f_value.first );
 ( group.getVar( "value_s" ) ).getVar( & f_value.second );
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::vector<int> >::serialize(
					     netCDF::NcGroup && group ) const
{
 Configuration::serialize( std::move( group ) );
 netCDF::NcDim sz = group.addDim( "size" , f_value.size() );
 std::vector<size_t> startp = { 0 };
 std::vector<size_t> countp = { f_value.size() };
 ( group.addVar( "value" , netCDF::NcInt() , sz ) ).putVar( startp , countp ,
							    f_value.data() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::vector<int> >::deserialize(
						   netCDF::NcGroup && group )
{
 Configuration::deserialize( std::move( group ) );
 size_t size = ( group.getDim( "size" ) ).getSize();
 f_value.resize( size );
 std::vector<size_t> start = { 0 };
 std::vector<size_t> count = { size };
 ( group.getVar( "value" ) ).getVar( start , count , f_value.data() );
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::vector<double> >::serialize(
					      netCDF::NcGroup && group ) const
{
 Configuration::serialize( std::move( group ) );
 netCDF::NcDim sz = group.addDim( "size" , f_value.size() );
 std::vector<size_t> startp = { 0 };
 std::vector<size_t> countp = { f_value.size() };
 ( group.addVar( "value" , netCDF::NcDouble() , sz ) ).putVar( startp ,
							       countp ,
							       f_value.data()
							       );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::vector<double> >::deserialize(
						   netCDF::NcGroup && group )
{
 Configuration::deserialize( std::move( group ) );
 size_t size = ( group.getDim( "size" ) ).getSize();
 f_value.resize( size );
 std::vector<size_t> start = { 0 };
 std::vector<size_t> count = { size };
 ( group.getVar( "value" ) ).getVar( start , count , f_value.data() );
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::list<int> >::serialize(
					     netCDF::NcGroup && group ) const
{
 Configuration::serialize( std::move( group ) );
 netCDF::NcDim sz = group.addDim( "size" , f_value.size() );
 auto it = f_value.begin();
 netCDF::NcVar var = group.addVar( "value" , netCDF::NcInt() , sz );
 for( size_t i = 0 ; i < f_value.size() ; )
  var.putVar( { i++ } , *(it++) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::list<int> >::deserialize(
						    netCDF::NcGroup && group )
{
 Configuration::deserialize( std::move( group ) );
 size_t size = ( group.getDim( "size" ) ).getSize();
 f_value.clear();
 netCDF::NcVar var = group.getVar( "value" );
 for( size_t i = 0 ; i < size ; ) {
  int val;
  var.getVar( { i++ } , &val );
  f_value.push_back( val );
  }
 }

/*--------------------------------------------------------------------------*/

template<>
void SimpleConfiguration< std::list<double> >::serialize(
					     netCDF::NcGroup && group ) const
{
 Configuration::serialize( std::move( group ) );
 netCDF::NcDim sz = group.addDim( "size" , f_value.size() );
 auto it = f_value.begin();
 netCDF::NcVar var = group.addVar( "value" , netCDF::NcDouble() , sz );
 for( size_t i = 0 ; i < f_value.size() ; )
  var.putVar( { i++ } , *(it++) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template<>
void SimpleConfiguration< std::list<double> >::deserialize(
					            netCDF::NcGroup && group )
{
 Configuration::deserialize( std::move( group ) );
 size_t size = ( group.getDim( "size" ) ).getSize();
 f_value.clear();
 netCDF::NcVar var = group.getVar( "value" );
 for( size_t i = 0 ; i < size ; ) {
  double val;
  var.getVar( { i++ } , &val );
  f_value.push_back( val );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File Configuration.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
