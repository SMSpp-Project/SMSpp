/*--------------------------------------------------------------------------*/
/*------------------------- File DataMapping.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the SimpleDataMappingBase class.
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "BendersBFunction.h"
#include "DataMapping.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*---------- CONSTRUCTING AND DESTRUCTING SimpleDataMappingBase ------------*/
/*--------------------------------------------------------------------------*/

void SimpleDataMappingBase::deserialize( const netCDF::NcGroup & group ,
  std::vector< std::unique_ptr< SimpleDataMappingBase > > & data_mappings ,
  Block * block_reference )
{
 auto sdmb_netCDF = pre_deserialize( group );

 if( sdmb_netCDF.NumberDataMappings.isNull() )
  return;

 auto num_data_mappings = sdmb_netCDF.NumberDataMappings.getSize();

 Index set_elements_start_index = 0;
 for( Index i = 0 ; i < num_data_mappings ; ++i ) {

  auto set_from_type = get_id< Block::Range >();
  auto set_to_type = get_id< Block::Range >();
  if( ! sdmb_netCDF.SetSize.isNull() )
   get_sets_type( sdmb_netCDF.SetSize , set_from_type, set_to_type , i );

  // DataType
  auto data_type = get_id< double >();
  if( ! sdmb_netCDF.DataType.isNull() )
   sdmb_netCDF.DataType.getVar( { i } , { 1 } , & data_type );

  // Caller type
  char caller_type = 'B';
  if( ! sdmb_netCDF.Caller.isNull() )
   sdmb_netCDF.Caller.getVar( { i } , { 1 } , & caller_type );

  auto data_mapping = SimpleDataMappingFactory::new_SimpleDataMapping
   ( { set_from_type , set_to_type , data_type , caller_type } );

  data_mapping->deserialize( sdmb_netCDF , i , set_elements_start_index ,
                             block_reference );

  data_mappings.emplace_back( data_mapping );
  }
 }

/*--------------------------------------------------------------------------*/

SimpleDataMappingBase * SimpleDataMappingFactory::new_SimpleDataMapping(
						  const std::string & types )
{
 if( types.size() == 4 && types[ 3 ] == 'F' ) {
  const auto t = types.substr( 0, 3 );
  if( t == "RRD" )
   return( new SimpleDataMapping< Range , Range , double , BendersBFunction > );
  else if( t == "RRI" )
   return( new SimpleDataMapping< Range , Range , int , BendersBFunction > );
  else if( t == "RSD" )
   return( new SimpleDataMapping< Range , Subset , double , BendersBFunction > );
  else if( t == "RSI" )
   return( new SimpleDataMapping< Range , Subset , int , BendersBFunction > );
  else if( t == "SRD" )
   return( new SimpleDataMapping< Subset , Range , double , BendersBFunction > );
  else if( t == "SRI" )
   return( new SimpleDataMapping< Subset , Range , int , BendersBFunction > );
  else if( t == "SSD" )
   return( new SimpleDataMapping< Subset , Subset , double , BendersBFunction > );
  else if( t == "SSI" )
   return( new SimpleDataMapping< Subset , Subset , int , BendersBFunction > );
  else
   throw( std::invalid_argument( "new_SimpleDataMapping: invalid template "
                                 "parameter types string: " + types ) );
 }
 else if( types.size() == 3 || ( types.size() == 4 && types[ 3 ] == 'B' ) ) {
  const auto t = types.substr( 0, 3 );
       if( t == "RRD" ) return( new SimpleDataMapping< Range , Range , double > );
  else if( t == "RRI" ) return( new SimpleDataMapping< Range , Range , int > );
  else if( t == "RSD" ) return( new SimpleDataMapping< Range , Subset , double > );
  else if( t == "RSI" ) return( new SimpleDataMapping< Range , Subset , int > );
  else if( t == "SRD" ) return( new SimpleDataMapping< Subset , Range , double > );
  else if( t == "SRI" ) return( new SimpleDataMapping< Subset , Range , int > );
  else if( t == "SSD" ) return( new SimpleDataMapping< Subset , Subset , double > );
  else if( t == "SSI" ) return( new SimpleDataMapping< Subset , Subset , int > );
  else
   throw( std::invalid_argument( "new_SimpleDataMapping: invalid template "
                                 "parameter types string: " + types ) );
  }
 else
  throw( std::invalid_argument( "new_SimpleDataMapping: invalid template "
                                "parameter types string: " + types ) );
 }

/*--------------------------------------------------------------------------*/

DataMapping * SimpleDataMappingFactory::deserialize(
		    const netCDF::NcGroup & group , Block * block_reference )
{
 // DataType

 char data_type;
 if( ! ::SMSpp_di_unipi_it::deserialize( group , data_type , "DataType" ,
                                         true ) )
  data_type = SimpleDataMappingBase::get_id< double >();

 // Caller type

 char caller_type;
 if( ! ::SMSpp_di_unipi_it::deserialize( group , caller_type , "Caller" ,
                                         true ) )
  caller_type = 'B';

 char set_from_type, set_to_type;
 get_sets_type( group , set_from_type , set_to_type );

 auto data_mapping = new_SimpleDataMapping( { set_from_type , set_to_type ,
                                              data_type , caller_type } );

 data_mapping->deserialize( group , block_reference );

 return( data_mapping );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File DataMapping.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
