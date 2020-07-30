/*--------------------------------------------------------------------------*/
/*------------------------- File RBlockConfig.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the RBlockConfig class.
 *
 * \version 0.10
 *
 * \date 15 - 07 - 2020
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "BlockInspection.h"
#include "RBlockConfig.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register RBlockConfig to the Configuration factory

SMSpp_insert_in_factory_cpp_0( RBlockConfig );

/*--------------------------------------------------------------------------*/
/*------------------------- METHODS of RBlockConfig ------------------------*/
/*--------------------------------------------------------------------------*/

RBlockConfig::RBlockConfig( const RBlockConfig &old ) : BlockConfig( old )
{
 v_sub_BlockConfig.resize( old.v_sub_BlockConfig.size() , nullptr );
 for( std::size_t i = 0 ; i < v_sub_BlockConfig.size() ; ++i )
  if( old.v_sub_BlockConfig[ i ] )
   v_sub_BlockConfig[ i ] = old.v_sub_BlockConfig[ i ]->clone();
 }

/*--------------------------------------------------------------------------*/

void RBlockConfig::get( Block * block ) {

 BlockConfig::get( block );

 for( auto & sBC : v_sub_BlockConfig )
  delete sBC;

 if( ! block ) {
  v_sub_BlockConfig.clear();
  return;
  }

 auto & nested_blocks = block->get_nested_Blocks();
 v_sub_BlockConfig.resize( nested_blocks.size() );

 auto nbit = nested_blocks.begin();
 for( c_Vec_Block::size_type i = 0 ; i < nested_blocks.size() ; ++i )
  v_sub_BlockConfig[ i ] = new RBlockConfig( *(nbit++) );
 }  // end( RBlockConfig::get )

/*--------------------------------------------------------------------------*/

void RBlockConfig::apply( Block * block , bool deleteold ) {

 if( ! block )
  return;

 // set the configurations for the Block -------------------------------------

 BlockConfig::apply( block , deleteold );

 // set the configurations for the sub-Block ---------------------------------

 auto & nb = block->get_nested_Blocks();
 auto bit = nb.begin();
 auto sbcit = v_sub_BlockConfig.begin();

 // only set non-nullptr configurations, hence only up until the list of
 // BlockConfigs ends
 for( ; ( bit != nb.end() ) &&
        ( sbcit != v_sub_BlockConfig.end() ) ;
        ++bit , ++sbcit )
  if( *sbcit )
   ( *sbcit )->apply( *bit , deleteold );
 }  // end( RBlockConfig::apply )

/*--------------------------------------------------------------------------*/

void RBlockConfig::print( std::ostream &output ) const
{
 BlockConfig::print( output );
 for( const auto cfg : v_sub_BlockConfig )
  if( cfg )
   output << *cfg;
 output << std::endl;

 }  // end( RBlockConfig::print )

/*--------------------------------------------------------------------------*/

void RBlockConfig::load( std::istream & input ) {

 BlockConfig::load( input );

 input >> eatcomments >> f_diff;

 int k;
 input >> eatcomments >> k;
 v_sub_BlockConfig.resize( k );
 for( int i = 0; i < k; ++i ) {
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) ) {
   v_sub_BlockConfig[ i ] = nullptr;
   input.ignore( std::numeric_limits< std::streamsize >::max(),
                 input.widen( '\n' ) );
  } else {
   std::string cname;
   input >> cname;
   v_sub_BlockConfig[ i ] =
    dynamic_cast< BlockConfig * >( Configuration::new_Configuration( cname ) );
   if( ! v_sub_BlockConfig[ i ] )
    throw ( std::invalid_argument( "RBlockConfig::load: invalid Configuration"
                                   " for the sub-Block " +
                                   std::to_string( i ) + "." ) );
   input >> *v_sub_BlockConfig[ i ];
  }
 }
}  // end( RBlockConfig::load )

/*--------------------------------------------------------------------------*/

void RBlockConfig::serialize( netCDF::NcGroup & group ) const
{
 BlockConfig::serialize( group );

 group.putAtt( "diff" , netCDF::NcInt() , f_diff );

 group.addDim( "n_sub_Block" , v_sub_BlockConfig.size() );

 for( size_t i = 0 ; i < v_sub_BlockConfig.size() ; ++i )
  if( v_sub_BlockConfig[ i ] ) {
   auto cg =  group.addGroup( "sub-BlockConfig_" + std::to_string( i ) );
   v_sub_BlockConfig[ i ]->serialize( cg );
   }

 }  // end( RBlockConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void RBlockConfig::deserialize( netCDF::NcGroup & group )
{

 if( ! v_sub_BlockConfig.empty() )
  throw( std::logic_error( "deserializing a non-empty RBlockConfig" ) );

 BlockConfig::deserialize( group );

 auto diff_att = group.getAtt( "diff" );
 if( ! diff_att.isNull() )
  diff_att.getValues( & f_diff );
 else
  f_diff = false;

 group.addDim( "n_sub_Block" , v_sub_BlockConfig.size() );

 size_t size = ( group.getDim( "n_sub_Block" ) ).getSize();

 v_sub_BlockConfig.resize( size );

 for( size_t i = 0 ; i < size ; ++i ) {
  auto cg = group.getGroup( "sub-BlockConfig_" + std::to_string( i ) );
  v_sub_BlockConfig[ i ] =
   dynamic_cast< BlockConfig * >( new_Configuration( cg ) );
  }
 }  // end( RBlockConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Block.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
