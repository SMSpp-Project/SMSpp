/*--------------------------------------------------------------------------*/
/*------------------------- File RBlockConfig.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the RBlockConfig class.
 *
 * \version 0.10
 *
 * \date 31 - 08 - 2020
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

// register RBlockConfig, CBlockConfig, and OBlockConfig to the Configuration
// factory

SMSpp_insert_in_factory_cpp_0( RBlockConfig );
SMSpp_insert_in_factory_cpp_0( CBlockConfig );
SMSpp_insert_in_factory_cpp_0( OBlockConfig );
SMSpp_insert_in_factory_cpp_0( CRBlockConfig );
SMSpp_insert_in_factory_cpp_0( ORBlockConfig );
SMSpp_insert_in_factory_cpp_0( OCBlockConfig );
SMSpp_insert_in_factory_cpp_0( OCRBlockConfig );

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
 for( const auto config : v_sub_BlockConfig )
  if( config )
   output << *config;
 output << std::endl;

 }  // end( RBlockConfig::print )

/*--------------------------------------------------------------------------*/

void RBlockConfig::load( std::istream & input ) {

 BlockConfig::load( input );

 int k;
 input >> eatcomments >> k;
 v_sub_BlockConfig.resize( k , nullptr );
 for( int i = 0 ; i < k ; ++i ) {
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
  throw( std::logic_error( "RBlockConfig::deserialize: deserializing a "
                           "non-empty RBlockConfig" ) );

 BlockConfig::deserialize( group );

 auto n_sub_Block = group.getDim( "n_sub_Block" );

 if( n_sub_Block.isNull() ) {

  v_sub_BlockConfig.resize( n_sub_Block.getSize() );

  for( decltype( v_sub_BlockConfig )::size_type i = 0 ;
       i < v_sub_BlockConfig.size() ; ++i ) {
   auto cg = group.getGroup( "sub-BlockConfig_" + std::to_string( i ) );
   v_sub_BlockConfig[ i ] =
    dynamic_cast< BlockConfig * >( new_Configuration( cg ) );
   }
  }
 }  // end( RBlockConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*------------------------ METHODS of CBlockConfig ------------------------*/
/*--------------------------------------------------------------------------*/

CBlockConfig::CBlockConfig( const CBlockConfig &old ) : BlockConfig( old )
{
 v_ConstraintID = old.v_ConstraintID;

 v_Config_Constraints.resize( old.v_Config_Constraints.size() , nullptr );
 auto this_it = v_Config_Constraints.begin();
 auto old_it = old.v_Config_Constraints.cbegin();
 for( ; this_it != v_Config_Constraints.end() ; ++this_it , ++old_it )
  if( *old_it )
   *this_it = ( *old_it )->clone();
 }

/*--------------------------------------------------------------------------*/

void CBlockConfig::get( Block * block ) {

 BlockConfig::get( block );

 for( auto config : v_Config_Constraints )
  delete config;
 v_Config_Constraints.clear();

 if( ! block ) {
  return;
  }

 // ComputeConfig for the Constraint

 if( ! v_ConstraintID.empty() ) {

  // v_ConstraintID is not empty. Consider only these Constraint.

  v_Config_Constraints.reserve( v_ConstraintID.size() );

  for( const auto & id : v_ConstraintID ) {
   auto constraint = inspection::get_Constraint( block , id );
   if( constraint )
    v_Config_Constraints.push_back( constraint->get_ComputeConfig( true ) );
   else
    throw ( std::logic_error( "CBlockConfig::get: Constraint with ConstraintID"
                              " ( " + std::to_string( id.first ) + " , " +
                              std::to_string( id.second ) +
                              ") was not found." ) );
   }
  }
 else {
  // v_ConstraintID is empty. Now we scan all Constraint.
  inspection::fill_ComputeConfig_Constraint( block , v_Config_Constraints ,
                                             v_ConstraintID );
  }
 }  // end( CBlockConfig::get )

/*--------------------------------------------------------------------------*/

void CBlockConfig::apply( Block * block , bool deleteold ) {

 if( ! block )
  return;

 BlockConfig::apply( block , deleteold );

 // set the ComputeConfig of the Constraint

 for( std::size_t i = 0 ; i < v_Config_Constraints.size() ; ++i ) {
  const auto & id = v_ConstraintID[ i ];
  auto constraint = inspection::get_Constraint( block , id );
  if( constraint ) {
   if( ( ! f_diff ) || v_Config_Constraints[ i ] )
    constraint->set_ComputeConfig( v_Config_Constraints[ i ] );
   }
  else
   throw ( std::logic_error( "CBlockConfig::apply: Constraint with ConstraintID"
                             " ( " + std::to_string( id.first ) + " , " +
                             std::to_string( id.second ) +
                             ") was not found." ) );
  }
 }  // end( CBlockConfig::apply )

/*--------------------------------------------------------------------------*/

void CBlockConfig::print( std::ostream &output ) const
{
 BlockConfig::print( output );

 for( const auto config : v_Config_Constraints )
  if( config )
   output << *config;
 output << std::endl;
 }  // end( CBlockConfig::print )

/*--------------------------------------------------------------------------*/

void CBlockConfig::load( std::istream & input ) {
 BlockConfig::load( input );

 // Configuration for the Constraint

 int k;
 input >> eatcomments >> k;
 v_Config_Constraints.resize( k );
 v_ConstraintID.resize( k );
 for( int i = 0 ; i < k ; ++i ) {
  Block::Index group_index, constraint_index;
  input >> eatcomments;
  input >> group_index >> constraint_index;
  v_ConstraintID[ i ] = Block::ConstraintID( group_index , constraint_index );
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   v_Config_Constraints[ i ] = nullptr;
  else {
   std::string cname;
   input >> cname;
   v_Config_Constraints[ i ] =
    dynamic_cast<ComputeConfig *>( Configuration::new_Configuration( cname ) );
   if( ! v_Config_Constraints[ i ] )
    throw ( std::invalid_argument( "CBlockConfig::load: invalid Configuration"
                                   " for the Constraint " +
                                   std::to_string( i ) + "." ) );
   input >> *v_Config_Constraints[ i ];
   }
  }
 }  // end( CBlockConfig::load )

/*--------------------------------------------------------------------------*/

void CBlockConfig::serialize( netCDF::NcGroup & group ) const
{
 BlockConfig::serialize( group );

 // Configuration for the Constraint

 if( ! v_Config_Constraints.empty() ) {

  auto n_Config_Constraint = group.addDim( "n_Config_Constraint" ,
                                           v_Config_Constraints.size() );

  for( size_t i = 0 ; i < v_Config_Constraints.size() ; ++i ) {
   if( v_Config_Constraints[ i ] ) {
    auto config_group = group.addGroup( "Config_Constraint_" +
                                        std::to_string( i ) );
    v_Config_Constraints[ i ]->serialize( config_group );
    }
   }

  auto two_dim = group.addDim( "two_dim" , 2 );

  auto ConstraintID_var = group.addVar( "ConstraintID" , netCDF::NcUint() ,
                                        { n_Config_Constraint , two_dim } );

  for( size_t i = 0 ; i < v_ConstraintID.size() ; ++i ) {
   ConstraintID_var.putVar( { i , 0 } , v_ConstraintID[ i ].first );
   ConstraintID_var.putVar( { i , 1 } , v_ConstraintID[ i ].second );
   }
  }
 }  // end( CBlockConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void CBlockConfig::deserialize( netCDF::NcGroup & group )
{
 if( v_Config_Constraints.size() || v_ConstraintID.size() )
  throw( std::logic_error( "CBlockConfig::deserialize: deserializing a "
                           "non-empty CBlockConfig" ) );

 BlockConfig::deserialize( group );

 // Configuration for Constraint

 auto constrdim = group.getDim( "n_Config_Constraint" );
 size_t constrsize = constrdim.isNull() ? 0 : constrdim.getSize();

 v_Config_Constraints.resize( constrsize );
 v_ConstraintID.resize( constrsize );

 auto var_ConstraintID = group.getVar( "ConstraintID" );
 if( constrsize > 0 ) {
  if( var_ConstraintID.isNull() )
   throw( std::invalid_argument( "CBlockConfig::deserialize: netCDF variable "
                                 "'ConstraintID' was not provided." ) );
  else {
   auto dimensions = ::get_sizes_dimensions( var_ConstraintID );
   if( dimensions.size() != 2 || dimensions[ 0 ] != constrsize
       || dimensions[ 1 ] != 2 )
    throw( std::invalid_argument
           ( "CBlockConfig::deserialize: invalid dimensions of netCDF variable "
             "'ConstraintID'. Its dimensions should be (" +
             std::to_string( constrsize ) +  ", 2)" ) );
   }
  }

 for( size_t i = 0 ; i < constrsize ; ++i ) {
  auto config_group = group.getGroup( "Config_Constraint_" +
                                      std::to_string( i ) );
  v_Config_Constraints[ i ] =
   dynamic_cast< ComputeConfig * >( new_Configuration( config_group ) );

  var_ConstraintID.getVar( { i , 0 } , & v_ConstraintID[ i ].first );
  var_ConstraintID.getVar( { i , 1 } , & v_ConstraintID[ i ].second );
  }
 }  // end( CBlockConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*------------------------ METHODS of OBlockConfig ------------------------*/
/*--------------------------------------------------------------------------*/

OBlockConfig::OBlockConfig( const OBlockConfig &old ) :
 BlockConfig( old ) , f_Config_Objective( nullptr )
{
 if( old.f_Config_Objective )
  f_Config_Objective = old.f_Config_Objective->clone();
 }

/*--------------------------------------------------------------------------*/

void OBlockConfig::get( Block * block ) {

 BlockConfig::get( block );

 delete f_Config_Objective;
 f_Config_Objective = nullptr;

 if( ! block ) {
  return;
  }

 // ComputeConfig for the Objective

 if( auto objective = block->get_objective() )
  f_Config_Objective = objective->get_ComputeConfig();

 }  // end( OBlockConfig::get )

/*--------------------------------------------------------------------------*/

void OBlockConfig::apply( Block * block , bool deleteold ) {

 if( ! block )
  return;

 BlockConfig::apply( block , deleteold );

 // set the configurations for the Block associated with Objective -----------
 //---------------------------------------------------------------------------

 if( auto objective = block->get_objective() )
  if( ( ! f_diff ) || f_Config_Objective )
   objective->set_ComputeConfig( f_Config_Objective );
 }  // end( OBlockConfig::apply )

/*--------------------------------------------------------------------------*/

void OBlockConfig::print( std::ostream &output ) const
{
 BlockConfig::print( output );

 if( f_Config_Objective )
  output << *f_Config_Objective;
 output << std::endl;
 }  // end( OBlockConfig::print )

/*--------------------------------------------------------------------------*/

void OBlockConfig::load( std::istream & input ) {
 BlockConfig::load( input );

 // Configuration for the Objective

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_Config_Objective = nullptr;
 else {
  std::string cname;
  input >> cname;
  auto config = Configuration::new_Configuration( cname );
  auto compute_config = dynamic_cast<ComputeConfig *>( config );
  if( ! compute_config )
   throw ( std::invalid_argument( "OBlockConfig::load: invalid Configuration"
                                  " for the Objective." ) );
  f_Config_Objective = compute_config;
  input >> *compute_config;
  }
 }  // end( OBlockConfig::load )

/*--------------------------------------------------------------------------*/

void OBlockConfig::serialize( netCDF::NcGroup & group ) const
{
 BlockConfig::serialize( group );

 // Configuration for the Objective

 if( f_Config_Objective ) {
  auto obj_group = group.addGroup( "Config_Objective" );
  f_Config_Objective->serialize( obj_group );
  }
 }  // end( OBlockConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void OBlockConfig::deserialize( netCDF::NcGroup & group )
{
 if( f_Config_Objective )
  throw( std::logic_error( "OBlockConfig::deserialize: deserializing a "
                           "non-empty OBlockConfig." ) );

 BlockConfig::deserialize( group );

 // ComputeConfig for the Objective

 auto obj_group = group.getGroup( "Config_Objective" );
 if( ! obj_group.isNull() )
  f_Config_Objective = dynamic_cast< ComputeConfig * >
   ( new_Configuration( obj_group ) );
 }  // end( OBlockConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*------------------------ METHODS of ORBlockConfig -----------------------*/
/*--------------------------------------------------------------------------*/

ORBlockConfig::ORBlockConfig( const ORBlockConfig &old ) :
 RBlockConfig( old ) , f_Config_Objective( nullptr )
{
 if( old.f_Config_Objective )
  f_Config_Objective = old.f_Config_Objective->clone();
 }

/*--------------------------------------------------------------------------*/

void ORBlockConfig::get( Block * block ) {

 RBlockConfig::get( block );

 delete f_Config_Objective;
 f_Config_Objective = nullptr;

 if( ! block ) {
  return;
  }

 // ComputeConfig for the Objective

 if( auto objective = block->get_objective() )
  f_Config_Objective = objective->get_ComputeConfig();

 }  // end( ORBlockConfig::get )

/*--------------------------------------------------------------------------*/

void ORBlockConfig::apply( Block * block , bool deleteold ) {

 if( ! block )
  return;

 RBlockConfig::apply( block , deleteold );

 // set the configurations for the Block associated with Objective -----------
 //---------------------------------------------------------------------------

 if( auto objective = block->get_objective() )
  if( ( ! f_diff ) || f_Config_Objective )
   objective->set_ComputeConfig( f_Config_Objective );
 }  // end( ORBlockConfig::apply )

/*--------------------------------------------------------------------------*/

void ORBlockConfig::print( std::ostream &output ) const
{
 RBlockConfig::print( output );

 if( f_Config_Objective )
  output << *f_Config_Objective;
 output << std::endl;
 }  // end( ORBlockConfig::print )

/*--------------------------------------------------------------------------*/

void ORBlockConfig::load( std::istream & input ) {
 RBlockConfig::load( input );

 // Configuration for the Objective

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_Config_Objective = nullptr;
 else {
  std::string cname;
  input >> cname;
  auto config = Configuration::new_Configuration( cname );
  auto compute_config = dynamic_cast<ComputeConfig *>( config );
  if( ! compute_config )
   throw ( std::invalid_argument( "ORBlockConfig::load: invalid "
                                  "Configuration for the Objective." ) );
  f_Config_Objective = compute_config;
  input >> *compute_config;
  }
 }  // end( ORBlockConfig::load )

/*--------------------------------------------------------------------------*/

void ORBlockConfig::serialize( netCDF::NcGroup & group ) const
{
 RBlockConfig::serialize( group );

 // Configuration for the Objective

 if( f_Config_Objective ) {
  auto obj_group = group.addGroup( "Config_Objective" );
  f_Config_Objective->serialize( obj_group );
  }
 }  // end( ORBlockConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void ORBlockConfig::deserialize( netCDF::NcGroup & group )
{
 if( f_Config_Objective )
  throw( std::logic_error( "ORBlockConfig::deserialize: deserializing a "
                           "non-empty ORBlockConfig." ) );

 RBlockConfig::deserialize( group );

 // ComputeConfig for the Objective

 auto obj_group = group.getGroup( "Config_Objective" );
 if( ! obj_group.isNull() )
  f_Config_Objective = dynamic_cast< ComputeConfig * >
   ( new_Configuration( obj_group ) );
 }  // end( ORBlockConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*------------------------ METHODS of CRBlockConfig ------------------------*/
/*--------------------------------------------------------------------------*/

CRBlockConfig::CRBlockConfig( const CRBlockConfig &old ) : RBlockConfig( old )
{
 v_ConstraintID = old.v_ConstraintID;

 v_Config_Constraints.resize( old.v_Config_Constraints.size() , nullptr );
 auto this_it = v_Config_Constraints.begin();
 auto old_it = old.v_Config_Constraints.cbegin();
 for( ; this_it != v_Config_Constraints.end() ; ++this_it , ++old_it )
  if( *old_it )
   *this_it = ( *old_it )->clone();
 }

/*--------------------------------------------------------------------------*/

void CRBlockConfig::get( Block * block ) {

 RBlockConfig::get( block );

 for( auto config : v_Config_Constraints )
  delete config;
 v_Config_Constraints.clear();

 if( ! block ) {
  return;
  }

 // ComputeConfig for the Constraint

 if( ! v_ConstraintID.empty() ) {

  // v_ConstraintID is not empty. Consider only these Constraint.

  v_Config_Constraints.reserve( v_ConstraintID.size() );

  for( const auto & id : v_ConstraintID ) {
   auto constraint = inspection::get_Constraint( block , id );
   if( constraint )
    v_Config_Constraints.push_back( constraint->get_ComputeConfig( true ) );
   else
    throw ( std::logic_error( "CRBlockConfig::get: Constraint with ConstraintID"
                              " ( " + std::to_string( id.first ) + " , " +
                              std::to_string( id.second ) +
                              ") was not found." ) );
   }
  }
 else {
  // v_ConstraintID is empty. Now we scan all Constraint.
  inspection::fill_ComputeConfig_Constraint( block , v_Config_Constraints ,
                                             v_ConstraintID );
  }
 }  // end( CRBlockConfig::get )

/*--------------------------------------------------------------------------*/

void CRBlockConfig::apply( Block * block , bool deleteold ) {

 if( ! block )
  return;

 RBlockConfig::apply( block , deleteold );

 // set the ComputeConfig of the Constraint

 for( std::size_t i = 0 ; i < v_Config_Constraints.size() ; ++i ) {
  const auto & id = v_ConstraintID[ i ];
  auto constraint = inspection::get_Constraint( block , id );
  if( constraint ) {
   if( ( ! f_diff ) || v_Config_Constraints[ i ] )
    constraint->set_ComputeConfig( v_Config_Constraints[ i ] );
   }
  else
   throw ( std::logic_error( "CRBlockConfig::apply: Constraint with "
                             "ConstraintID ( " + std::to_string( id.first ) +
                              " , " + std::to_string( id.second ) +
                             ") was not found." ) );
  }
 }  // end( CRBlockConfig::apply )

/*--------------------------------------------------------------------------*/

void CRBlockConfig::print( std::ostream &output ) const
{
 RBlockConfig::print( output );

 for( const auto config : v_Config_Constraints )
  if( config )
   output << *config;
 output << std::endl;
 }  // end( CRBlockConfig::print )

/*--------------------------------------------------------------------------*/

void CRBlockConfig::load( std::istream & input ) {
 RBlockConfig::load( input );

 // Configuration for the Constraint

 int k;
 input >> eatcomments >> k;
 v_Config_Constraints.resize( k );
 v_ConstraintID.resize( k );
 for( int i = 0 ; i < k ; ++i ) {
  Block::Index group_index, constraint_index;
  input >> eatcomments;
  input >> group_index >> constraint_index;
  v_ConstraintID[ i ] = Block::ConstraintID( group_index , constraint_index );
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   v_Config_Constraints[ i ] = nullptr;
  else {
   std::string cname;
   input >> cname;
   v_Config_Constraints[ i ] =
    dynamic_cast<ComputeConfig *>( Configuration::new_Configuration( cname ) );
   if( ! v_Config_Constraints[ i ] )
    throw ( std::invalid_argument( "CRBlockConfig::load: invalid Configuration"
                                   " for the Constraint " +
                                   std::to_string( i ) + "." ) );
   input >> *v_Config_Constraints[ i ];
   }
  }
 }  // end( CRBlockConfig::load )

/*--------------------------------------------------------------------------*/

void CRBlockConfig::serialize( netCDF::NcGroup & group ) const
{
 RBlockConfig::serialize( group );

 // Configuration for the Constraint

 if( ! v_Config_Constraints.empty() ) {

  auto n_Config_Constraint = group.addDim( "n_Config_Constraint" ,
                                           v_Config_Constraints.size() );

  for( size_t i = 0 ; i < v_Config_Constraints.size() ; ++i ) {
   if( v_Config_Constraints[ i ] ) {
    auto config_group = group.addGroup( "Config_Constraint_" +
                                        std::to_string( i ) );
    v_Config_Constraints[ i ]->serialize( config_group );
    }
   }

  auto two_dim = group.addDim( "two_dim" , 2 );

  auto ConstraintID_var = group.addVar( "ConstraintID" , netCDF::NcUint() ,
                                        { n_Config_Constraint , two_dim } );

  for( size_t i = 0 ; i < v_ConstraintID.size() ; ++i ) {
   ConstraintID_var.putVar( { i , 0 } , v_ConstraintID[ i ].first );
   ConstraintID_var.putVar( { i , 1 } , v_ConstraintID[ i ].second );
   }
  }
 }  // end( CRBlockConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void CRBlockConfig::deserialize( netCDF::NcGroup & group )
{
 if( v_Config_Constraints.size() || v_ConstraintID.size() )
  throw( std::logic_error( "CRBlockConfig::deserialize: deserializing a "
                           "non-empty CRBlockConfig" ) );

 RBlockConfig::deserialize( group );

 // Configuration for Constraint

 auto constrdim = group.getDim( "n_Config_Constraint" );
 size_t constrsize = constrdim.isNull() ? 0 : constrdim.getSize();

 v_Config_Constraints.resize( constrsize );
 v_ConstraintID.resize( constrsize );

 auto var_ConstraintID = group.getVar( "ConstraintID" );
 if( constrsize > 0 ) {
  if( var_ConstraintID.isNull() )
   throw( std::invalid_argument( "CRBlockConfig::deserialize: netCDF variable "
                                 "'ConstraintID' was not provided." ) );
  else {
   auto dimensions = ::get_sizes_dimensions( var_ConstraintID );
   if( dimensions.size() != 2 || dimensions[ 0 ] != constrsize
       || dimensions[ 1 ] != 2 )
    throw( std::invalid_argument
           ( "CRBlockConfig::deserialize: invalid dimensions of netCDF variable "
             "'ConstraintID'. Its dimensions should be (" +
             std::to_string( constrsize ) +  ", 2)" ) );
   }
  }

 for( size_t i = 0 ; i < constrsize ; ++i ) {
  auto config_group = group.getGroup( "Config_Constraint_" +
                                      std::to_string( i ) );
  v_Config_Constraints[ i ] =
   dynamic_cast< ComputeConfig * >( new_Configuration( config_group ) );

  var_ConstraintID.getVar( { i , 0 } , & v_ConstraintID[ i ].first );
  var_ConstraintID.getVar( { i , 1 } , & v_ConstraintID[ i ].second );
  }
 }  // end( CRBlockConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*------------------------ METHODS of OCBlockConfig ------------------------*/
/*--------------------------------------------------------------------------*/

OCBlockConfig::OCBlockConfig( const OCBlockConfig &old ) :
 CBlockConfig( old ) , f_Config_Objective( nullptr )
{
 if( old.f_Config_Objective )
  f_Config_Objective = old.f_Config_Objective->clone();
 }

/*--------------------------------------------------------------------------*/

void OCBlockConfig::get( Block * block ) {

 CBlockConfig::get( block );

 delete f_Config_Objective;
 f_Config_Objective = nullptr;

 if( ! block ) {
  return;
  }

 // ComputeConfig for the Objective

 if( auto objective = block->get_objective() )
  f_Config_Objective = objective->get_ComputeConfig();

 }  // end( OCBlockConfig::get )

/*--------------------------------------------------------------------------*/

void OCBlockConfig::apply( Block * block , bool deleteold ) {

 if( ! block )
  return;

 CBlockConfig::apply( block , deleteold );

 // set the configurations for the Block associated with Objective -----------
 //---------------------------------------------------------------------------

 if( auto objective = block->get_objective() )
  if( ( ! f_diff ) || f_Config_Objective )
   objective->set_ComputeConfig( f_Config_Objective );
 }  // end( OCBlockConfig::apply )

/*--------------------------------------------------------------------------*/

void OCBlockConfig::print( std::ostream &output ) const
{
 CBlockConfig::print( output );

 if( f_Config_Objective )
  output << *f_Config_Objective;
 output << std::endl;
 }  // end( OCBlockConfig::print )

/*--------------------------------------------------------------------------*/

void OCBlockConfig::load( std::istream & input ) {
 CBlockConfig::load( input );

 // Configuration for the Objective

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_Config_Objective = nullptr;
 else {
  std::string cname;
  input >> cname;
  auto config = Configuration::new_Configuration( cname );
  auto compute_config = dynamic_cast<ComputeConfig *>( config );
  if( ! compute_config )
   throw ( std::invalid_argument( "OCBlockConfig::load: invalid Configuration"
                                  " for the Objective." ) );
  f_Config_Objective = compute_config;
  input >> *compute_config;
  }
 }  // end( OCBlockConfig::load )

/*--------------------------------------------------------------------------*/

void OCBlockConfig::serialize( netCDF::NcGroup & group ) const
{
 CBlockConfig::serialize( group );

 // Configuration for the Objective

 if( f_Config_Objective ) {
  auto obj_group = group.addGroup( "Config_Objective" );
  f_Config_Objective->serialize( obj_group );
  }
 }  // end( OCBlockConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void OCBlockConfig::deserialize( netCDF::NcGroup & group )
{
 if( f_Config_Objective )
  throw( std::logic_error( "OCBlockConfig::deserialize: deserializing a "
                           "non-empty OCBlockConfig." ) );

 CBlockConfig::deserialize( group );

 // ComputeConfig for the Objective

 auto obj_group = group.getGroup( "Config_Objective" );
 if( ! obj_group.isNull() )
  f_Config_Objective = dynamic_cast< ComputeConfig * >
   ( new_Configuration( obj_group ) );
 }  // end( OCBlockConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*----------------------- METHODS of OCRBlockConfig ------------------------*/
/*--------------------------------------------------------------------------*/

OCRBlockConfig::OCRBlockConfig( const OCRBlockConfig &old ) :
 CRBlockConfig( old ) , f_Config_Objective( nullptr )
{
 if( old.f_Config_Objective )
  f_Config_Objective = old.f_Config_Objective->clone();
 }

/*--------------------------------------------------------------------------*/

void OCRBlockConfig::get( Block * block ) {

 CRBlockConfig::get( block );

 delete f_Config_Objective;
 f_Config_Objective = nullptr;

 if( ! block ) {
  return;
  }

 // ComputeConfig for the Objective

 if( auto objective = block->get_objective() )
  f_Config_Objective = objective->get_ComputeConfig();

 }  // end( OCRBlockConfig::get )

/*--------------------------------------------------------------------------*/

void OCRBlockConfig::apply( Block * block , bool deleteold ) {

 if( ! block )
  return;

 CRBlockConfig::apply( block , deleteold );

 // set the configurations for the Block associated with Objective -----------
 //---------------------------------------------------------------------------

 if( auto objective = block->get_objective() )
  if( ( ! f_diff ) || f_Config_Objective )
   objective->set_ComputeConfig( f_Config_Objective );
 }  // end( OCRBlockConfig::apply )

/*--------------------------------------------------------------------------*/

void OCRBlockConfig::print( std::ostream &output ) const
{
 CRBlockConfig::print( output );

 if( f_Config_Objective )
  output << *f_Config_Objective;
 output << std::endl;
 }  // end( OCRBlockConfig::print )

/*--------------------------------------------------------------------------*/

void OCRBlockConfig::load( std::istream & input ) {
 CRBlockConfig::load( input );

 // Configuration for the Objective

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_Config_Objective = nullptr;
 else {
  std::string cname;
  input >> cname;
  auto config = Configuration::new_Configuration( cname );
  auto compute_config = dynamic_cast<ComputeConfig *>( config );
  if( ! compute_config )
   throw ( std::invalid_argument( "OCRBlockConfig::load: invalid "
                                  "Configuration for the Objective." ) );
  f_Config_Objective = compute_config;
  input >> *compute_config;
  }
 }  // end( OCRBlockConfig::load )

/*--------------------------------------------------------------------------*/

void OCRBlockConfig::serialize( netCDF::NcGroup & group ) const
{
 CRBlockConfig::serialize( group );

 // Configuration for the Objective

 if( f_Config_Objective ) {
  auto obj_group = group.addGroup( "Config_Objective" );
  f_Config_Objective->serialize( obj_group );
  }
 }  // end( OCRBlockConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void OCRBlockConfig::deserialize( netCDF::NcGroup & group )
{
 if( f_Config_Objective )
  throw( std::logic_error( "OCRBlockConfig::deserialize: deserializing a "
                           "non-empty OCRBlockConfig." ) );

 CRBlockConfig::deserialize( group );

 // ComputeConfig for the Objective

 auto obj_group = group.getGroup( "Config_Objective" );
 if( ! obj_group.isNull() )
  f_Config_Objective = dynamic_cast< ComputeConfig * >
   ( new_Configuration( obj_group ) );
 }  // end( OCRBlockConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*--------------------- End File RBlockConfig.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
