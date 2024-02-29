/*--------------------------------------------------------------------------*/
/*-------------------------- File BendersBlock.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the BendersBlock class.
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

#include "BendersBlock.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

SMSpp_insert_in_factory_cpp_1( BendersBlock );

/*--------------------------------------------------------------------------*/
/*-------------------------- METHODS of BendersBlock -----------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------- CONSTRUCTING AND DESTRUCTING BendersBlock ----------------*/
/*--------------------------------------------------------------------------*/

void BendersBlock::deserialize( const netCDF::NcGroup & group ) {

 auto ncDim_NumVar = group.getDim( "NumVar" );

 if( ncDim_NumVar.isNull() )
  throw( std::logic_error( "BendersBlock::deserialize: "
                           "NumVar dimension is required." ) );

 const auto ObjectiveSense = group.getDim( "ObjectiveSense" );
 if( ObjectiveSense.isNull() || ( ObjectiveSense.getSize() != 0 ) )
  objective.set_sense( Objective::eMin );
 else
  objective.set_sense( Objective::eMax );

 v_variables.resize( ncDim_NumVar.getSize() );
 for( auto & variable : v_variables )
  variable.set_Block( this );

 auto benders_function_group = group.getGroup( "BendersBFunction" );
 if( benders_function_group.isNull() )
  throw( std::logic_error( "BendersBlock::deserialize: "
                           "'BendersBFunction' sub-group is required." ) );

 auto benders_function = new BendersBFunction();

 {
  std::vector< ColVariable * > p_variables;
  p_variables.reserve( v_variables.size() );
  for( auto & variable : v_variables )
   p_variables.push_back( & variable );
  benders_function->set_variables( std::move( p_variables ) );
 }

 benders_function->deserialize( benders_function_group );
 set_function( benders_function );
 benders_function->set_f_Block( this );

 Block::deserialize( group );
}

/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR Saving THE DATA OF THE BendersBlock -----------*/
/*--------------------------------------------------------------------------*/

void BendersBlock::serialize( netCDF::NcGroup & group ) const {

 Block::serialize( group );

 group.putAtt( "type" , "BendersBlock" );

 group.addDim( "NumVar" , v_variables.size() );

 if( objective.get_sense() == Objective::eMax )
  group.addDim( "ObjectiveSense" , 0 );

 auto benders_function = objective.get_function();

 if( benders_function ) {
  auto benders_function_group = group.addGroup( "BendersBFunction" );
  static_cast< BendersBFunction * >( benders_function )->
   serialize( benders_function_group );
 }
}

/*--------------------------------------------------------------------------*/
/*---------------------- End File BendersBlock.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
