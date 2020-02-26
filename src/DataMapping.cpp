/*--------------------------------------------------------------------------*/
/*------------------------- File DataMapping.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the SimpleDataMappingBase class.
 *
 * \version 0.10
 *
 * \date 25 - 02 - 2020
 *
 * \author Rafael Durbano Lobato \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "DataMapping.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*---------- CONSTRUCTING AND DESTRUCTING SimpleDataMappingBase ------------*/
/*--------------------------------------------------------------------------*/

void SimpleDataMappingBase::deserialize
( const netCDF::NcGroup & group ,
  std::vector< std::unique_ptr< SimpleDataMappingBase > > & data_mappings ,
  Block * block_reference ) {

 auto sdmb_netCDF = pre_deserialize( group );

 Index num_data_mappings = 1;
 if( ! sdmb_netCDF.NumberDataMappings.isNull() )
  num_data_mappings = sdmb_netCDF.NumberDataMappings.getSize();

 Index set_elements_start_index = 0;
 for( Index i = 0 ; i < num_data_mappings ; ++i ) {

  char set_from_type, set_to_type;
  get_sets_type( sdmb_netCDF.SetSize , set_from_type, set_to_type , i );

  // DataType
  char data_type;
  sdmb_netCDF.DataType.getVar( { i } , { 1 } , & data_type );

  // Caller type
  char caller_type;
  sdmb_netCDF.Caller.getVar( { i } , { 1 } , & caller_type );

  auto data_mapping = SimpleDataMappingFactory::new_SimpleDataMapping
   ( { set_from_type , set_to_type , data_type , caller_type } );

  data_mapping->deserialize( sdmb_netCDF , i , set_elements_start_index ,
                             block_reference );

  data_mappings.emplace_back( data_mapping );
 }
}

/*--------------------------------------------------------------------------*/
/*----------------------- End File DQuadFunction.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
