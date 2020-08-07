/*--------------------------------------------------------------------------*/
/*------------------------- File RBlockConfig.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the RBlockConfig class.
 *
 * \version 0.10
 *
 * \date 06 - 08 - 2020
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
 v_sub_BlockConfig.resize( k );
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

  group.addDim( "n_Config_Constraint" , v_Config_Constraints.size() );

  for( size_t i = 0 ; i < v_Config_Constraints.size() ; ++i ) {
   if( v_Config_Constraints[ i ] ) {
    auto config_group = group.addGroup( "Config_Constraint_" +
                                        std::to_string( i ) );
    v_Config_Constraints[ i ]->serialize( config_group );
    }
   }

  auto ConstraintID_dim = group.addDim( "ConstraintID_dim" ,
                                        2 * v_ConstraintID.size() );

  auto ConstraintID_var = group.addVar( "ConstraintID" , netCDF::NcUint() ,
                                        ConstraintID_dim );

  for( size_t i = 0 ; i < v_ConstraintID.size() ; ++i ) {
   std::vector<Block::Index> id = { v_ConstraintID[ i ].first ,
                                    v_ConstraintID[ i ].second };
   ConstraintID_var.putVar( { 2 * i } , { 2 } , id.data() );
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

 std::vector<Block::Index> var_ConstraintID;
 if( constrsize > 0 ) {
  ::deserialize( group , "ConstraintID" , 2 * constrsize ,
                 var_ConstraintID , false , false );
  }

 for( size_t i = 0 ; i < constrsize ; ++i ) {
  auto config_group = group.getGroup( "Config_Constraint_" +
                                      std::to_string( i ) );
  v_Config_Constraints[ i ] =
   dynamic_cast< ComputeConfig * >( new_Configuration( config_group ) );
  v_ConstraintID[ i ] = Block::ConstraintID( var_ConstraintID[ 2 * i ] ,
                                             var_ConstraintID[ 2 * i + 1 ] );
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
   throw( std::invalid_argument( "OBlockConfig::load: not a ComputeConfig "
                                 " object" ) );
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
/*--------------------- End File RBlockConfig.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
