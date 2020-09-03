/*--------------------------------------------------------------------------*/
/*------------------------- File RBlockConfig.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the RBlockConfig class.
 *
 * \version 0.10
 *
 * \date 03 - 09 - 2020
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
/*--------------------------- AUXILIARY FUNCTIONS --------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

 /// returns the index of the sub-Block with the given \p id
 Block::Index get_nested_Block_index( const std::string & id ,
                                      const Block * block ) {

  if( ( ! id.empty() ) && std::isdigit( id.front() ) ) {
   // The id is the index of the sub-Block
   try {
    return std::stoi( id );
    }
   catch( std::exception & e ) {
    std::cerr << "get_nested_Block_index: invalid sub-Block id "
              << id << ": " << e.what() << std::endl;
    return Inf<Block::Index>();
    }
   }
  else {
   // The id is the name of the sub-Block
   return block->get_nested_Block_index( id );
   }
 }

/*--------------------------------------------------------------------------*/

 /// returns the index of the group of Constraint
 Block::Index get_Constraint_group_index
 ( const std::string & constraint_group_id , const Block * block ) {

  if( ( ! constraint_group_id.empty() ) &&
      std::isdigit( constraint_group_id.front() ) ) {
   // The group id is the index of the group of Constraint
   try {
    return std::stoi( constraint_group_id );
   }
   catch( std::exception & e ) {
    std::cerr << "get_Constraint_group_index: invalid Constraint group id "
              << constraint_group_id << ": " << e.what() << std::endl;
    return Inf<Block::Index>();
   }
  }
  else {
   // The group id is the name of the group of Constraint
   auto constraint_group_index = block->get_s_const_index( constraint_group_id );
   if( constraint_group_index >= block->get_number_static_constraints() ) {
    // It must be a group of dynamic Constraint
    constraint_group_index = block->get_d_const_index( constraint_group_id );
   }
   return constraint_group_index;
  }
 }
}

/*--------------------------------------------------------------------------*/
/*------------------------- METHODS of RBlockConfig ------------------------*/
/*--------------------------------------------------------------------------*/

RBlockConfig::RBlockConfig( const RBlockConfig &old ) : BlockConfig( old )
{
 v_sub_Block_id = old.v_sub_Block_id;

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

 const auto number_nested_Blocks = block->get_number_nested_Blocks();

 if( v_sub_Block_id.empty() ) {
  v_sub_Block_id.resize( number_nested_Blocks );
  for( decltype( v_sub_Block_id )::size_type i = 0 ;
       i < v_sub_Block_id.size() ; ++i )
   v_sub_Block_id[ i ] = std::to_string( i );
  }

 v_sub_BlockConfig.resize( v_sub_Block_id.size() );

 for( decltype( v_sub_Block_id )::size_type i = 0 ;
      i < v_sub_Block_id.size() ; ++i ) {

  const auto id = v_sub_Block_id[ i ];
  Block::Index sub_Block_index = ::get_nested_Block_index( id , block );

  if( sub_Block_index >= block->get_number_nested_Blocks() )
   throw ( std::logic_error( "RBlockConfig::get: invalid sub-Block id: "
                             + id + "." ) );

  v_sub_BlockConfig[ i ] = new OCRBlockConfig // TODO
   ( block->get_nested_Block( sub_Block_index ) );
  }
 }  // end( RBlockConfig::get )

/*--------------------------------------------------------------------------*/

void RBlockConfig::apply( Block * block , bool deleteold ) {

 if( ! block )
  return;

 if( v_sub_BlockConfig.size() > v_sub_Block_id.size() )
  throw ( std::logic_error
          ( "RBlockConfig::apply: missing sub-Block identification. There are "
            + std::to_string( v_sub_BlockConfig.size() ) + " BlockConfig but on"
            "ly " + std::to_string( v_sub_Block_id.size() ) +
            " sub-Block ids." ) );

 // set the configurations for the Block -------------------------------------

 BlockConfig::apply( block , deleteold );

 // set the configurations for the sub-Block ---------------------------------

 for( decltype( v_sub_BlockConfig )::size_type i = 0 ;
      i < v_sub_BlockConfig.size() ; ++i ) {

  const auto & id = v_sub_Block_id[ i ];
  Block::Index sub_Block_index = ::get_nested_Block_index( id , block );
  if( sub_Block_index >= block->get_number_nested_Blocks() )
   throw ( std::logic_error( "RBlockConfig::apply: invalid sub-Block id: "
                             + id + "." ) );

  auto sub_Block = block->get_nested_Block( sub_Block_index );

  if( v_sub_BlockConfig[ i ] && sub_Block )
   v_sub_BlockConfig[ i ]->apply( sub_Block , deleteold );
 }
} // end( RBlockConfig::apply )

/*--------------------------------------------------------------------------*/

void RBlockConfig::print( std::ostream &output ) const
{
 BlockConfig::print( output );

 decltype( v_sub_BlockConfig )::size_type i = 0;
 for( const auto config : v_sub_BlockConfig ) {
  std::string id = ( i < v_sub_Block_id.size() ) ? v_sub_Block_id[ i ] : "?";
  ++i;
  output << "BlockConfig for sub-Block " << id << std::endl;
  if( config )
   output << *config;
  else
   output << "nullptr" << std::endl;
 }
 output << std::endl;

 }  // end( RBlockConfig::print )

/*--------------------------------------------------------------------------*/

void RBlockConfig::load( std::istream & input ) {

 BlockConfig::load( input );

 int k;
 input >> eatcomments >> k;
 const bool id_is_provided = ( k < 0 );
 k = std::abs( k );
 v_sub_BlockConfig.resize( k , nullptr );
 v_sub_Block_id.resize( k );

 for( int i = 0 ; i < k ; ++i ) {

  if( id_is_provided ) {
   input >> eatcomments >> v_sub_Block_id[ i ];
  }
  else
   v_sub_Block_id[ i ] = std::to_string( i );

  input >> eatcomments;

  if( input.peek() == input.widen( '*' ) ) {
   v_sub_BlockConfig[ i ] = nullptr;
   input.ignore( std::numeric_limits< std::streamsize >::max() ,
                 input.widen( '\n' ) );
  } else {
   std::string cname;
   input >> cname;
   v_sub_BlockConfig[ i ] =
    dynamic_cast< BlockConfig * >( Configuration::new_Configuration( cname ) );
   if( ! v_sub_BlockConfig[ i ] )
    throw ( std::invalid_argument("RBlockConfig::load: invalid BlockConfig for "
                                  "sub-Block " + v_sub_Block_id[ i ] + "." ) );
   input >> *v_sub_BlockConfig[ i ];
  }
 }
}  // end( RBlockConfig::load )

/*--------------------------------------------------------------------------*/

void RBlockConfig::serialize( netCDF::NcGroup & group ) const
{
 BlockConfig::serialize( group );

 auto n_sub_Block = group.addDim( "n_sub_Block" , v_sub_BlockConfig.size() );

 for( size_t i = 0 ; i < v_sub_BlockConfig.size() ; ++i )
  if( v_sub_BlockConfig[ i ] ) {
   auto cg =  group.addGroup( "sub-BlockConfig_" + std::to_string( i ) );
   v_sub_BlockConfig[ i ]->serialize( cg );
   }

 netCDF::NcDim sub_Block_id_dim;
 if( v_sub_Block_id.size() == v_sub_BlockConfig.size() )
  sub_Block_id_dim = n_sub_Block;
 else
  sub_Block_id_dim = group.addDim( "n_sub_Block_id" , v_sub_Block_id.size() );

 auto sub_Block_id_var = group.addVar( "sub-Block-id" , netCDF::NcString() ,
                                       { sub_Block_id_dim } );

 sub_Block_id_var.putVar( v_sub_Block_id.data() );
 }  // end( RBlockConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void RBlockConfig::deserialize( netCDF::NcGroup & group )
{

 if( ! ( v_sub_BlockConfig.empty() && v_sub_Block_id.empty() ) )
  throw( std::logic_error( "RBlockConfig::deserialize: deserializing a "
                           "non-empty RBlockConfig" ) );

 BlockConfig::deserialize( group );

 auto n_sub_Block = group.getDim( "n_sub_Block" );

 if( ! n_sub_Block.isNull() ) {

  v_sub_BlockConfig.resize( n_sub_Block.getSize() );

  for( decltype( v_sub_BlockConfig )::size_type i = 0 ;
       i < v_sub_BlockConfig.size() ; ++i ) {
   auto cg = group.getGroup( "sub-BlockConfig_" + std::to_string( i ) );
   v_sub_BlockConfig[ i ] =
    dynamic_cast< BlockConfig * >( new_Configuration( cg ) );
   }
  }

 auto var_sub_Block_id = group.getVar( "sub-Block-id" );
 if( ! var_sub_Block_id.isNull() ) {
  assert( var_sub_Block_id.getDimCount() == 1 );
  v_sub_Block_id.resize( var_sub_Block_id.getDim( 0 ).getSize() );
  var_sub_Block_id.getVar( v_sub_Block_id.data() );
  }
 else if( ! n_sub_Block.isNull() ) {
  decltype( v_sub_Block_id )::size_type n = n_sub_Block.getSize();
  v_sub_Block_id.resize( n );
  for( decltype( n ) i = 0 ; i < n ; ++i )
   v_sub_Block_id[ i ] = std::to_string( i );
  }
 }  // end( RBlockConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*------------------------ METHODS of CBlockConfig ------------------------*/
/*--------------------------------------------------------------------------*/

CBlockConfig::CBlockConfig( const CBlockConfig &old ) : BlockConfig( old )
{
 v_Constraint_id = old.v_Constraint_id;

 v_Config_Constraint.resize( old.v_Config_Constraint.size() , nullptr );
 auto this_it = v_Config_Constraint.begin();
 auto old_it = old.v_Config_Constraint.cbegin();
 for( ; this_it != v_Config_Constraint.end() ; ++this_it , ++old_it )
  if( *old_it )
   *this_it = ( *old_it )->clone();
 }

/*--------------------------------------------------------------------------*/

void CBlockConfig::get( Block * block ) {

 BlockConfig::get( block );

 for( auto config : v_Config_Constraint )
  delete config;
 v_Config_Constraint.clear();

 if( ! block ) {
  return;
  }

 // ComputeConfig for the Constraint

 if( ! v_Constraint_id.empty() ) {

  // v_Constraint_id is not empty. Consider only these Constraint.

  v_Config_Constraint.reserve( v_Constraint_id.size() );

  for( const auto [ constraint_group_id , constraint_index ] :
        v_Constraint_id ) {

   auto constraint_group_index =
    ::get_Constraint_group_index( constraint_group_id , block );

   auto constraint = inspection::get_Constraint
    ( block , Block::ConstraintID( constraint_group_index , constraint_index ) );
   if( constraint )
    v_Config_Constraint.push_back( constraint->get_ComputeConfig( true ) );
   else
    throw ( std::logic_error
            ( "CBlockConfig::get: Constraint with Constraint id ( " +
              constraint_group_id + " , " + std::to_string( constraint_index ) +
              ") was not found." ) );
   }
  }
 else {
  // v_Constraint_id is empty. Now we scan all Constraint.
  inspection::fill_ComputeConfig_Constraint( block , v_Config_Constraint ,
                                             v_Constraint_id );
  }
 }  // end( CBlockConfig::get )

/*--------------------------------------------------------------------------*/

void CBlockConfig::apply( Block * block , bool deleteold ) {

 if( ! block )
  return;

 BlockConfig::apply( block , deleteold );

 // set the ComputeConfig of the Constraint

 for( std::size_t i = 0 ; i < v_Config_Constraint.size() ; ++i ) {

  const auto [ constraint_group_id , constraint_index ] =
   v_Constraint_id[ i ];

  auto constraint_group_index =
   ::get_Constraint_group_index( constraint_group_id , block );

  auto constraint = inspection::get_Constraint
   ( block , Block::ConstraintID( constraint_group_index , constraint_index ) );

  if( constraint ) {
   if( ( ! f_diff ) || v_Config_Constraint[ i ] )
    constraint->set_ComputeConfig( v_Config_Constraint[ i ] );
   }
  else
   throw ( std::logic_error( "CBlockConfig::apply: Constraint with Constraint "
                             "id ( " + constraint_group_id + " , " +
                             std::to_string( constraint_index ) +
                             ") was not found." ) );
  }
 }  // end( CBlockConfig::apply )

/*--------------------------------------------------------------------------*/

void CBlockConfig::print( std::ostream &output ) const
{
 BlockConfig::print( output );

 decltype( v_Constraint_id )::size_type i = 0;

 for( const auto config : v_Config_Constraint ) {
  output << "ComputeConfig for Constraint (" << v_Constraint_id[ i ].first
         << " , " << v_Constraint_id[ i ].second << ")" << std::endl;
  ++i;
  if( config )
   output << *config;
  else
   output << "nullptr" << std::endl;
  }
 output << std::endl;
 }  // end( CBlockConfig::print )

/*--------------------------------------------------------------------------*/

void CBlockConfig::load( std::istream & input ) {
 BlockConfig::load( input );

 // Configuration for the Constraint

 int k;
 input >> eatcomments >> k;
 v_Config_Constraint.resize( k );
 v_Constraint_id.resize( k );

 for( int i = 0 ; i < k ; ++i ) {
  std::string constraint_group_id;
  Block::Index constraint_index;

  input >> eatcomments;
  input >> v_Constraint_id[ i ].first >> v_Constraint_id[ i ].second;

  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   v_Config_Constraint[ i ] = nullptr;
  else {
   std::string cname;
   input >> cname;
   v_Config_Constraint[ i ] =
    dynamic_cast<ComputeConfig *>( Configuration::new_Configuration( cname ) );
   if( ! v_Config_Constraint[ i ] )
    throw ( std::invalid_argument( "CBlockConfig::load: invalid Configuration"
                                   " for the Constraint " +
                                   std::to_string( i ) + "." ) );
   input >> *v_Config_Constraint[ i ];
   }
  }
 }  // end( CBlockConfig::load )

/*--------------------------------------------------------------------------*/

void CBlockConfig::serialize( netCDF::NcGroup & group ) const
{
 BlockConfig::serialize( group );

 // Configuration for the Constraint

 if( ! v_Config_Constraint.empty() ) {

  auto n_Config_Constraint = group.addDim( "n_Config_Constraint" ,
                                           v_Config_Constraint.size() );

  for( size_t i = 0 ; i < v_Config_Constraint.size() ; ++i ) {
   if( v_Config_Constraint[ i ] ) {
    auto config_group = group.addGroup( "Config_Constraint_" +
                                        std::to_string( i ) );
    v_Config_Constraint[ i ]->serialize( config_group );
    }
   }

  auto Constraint_group_id_var = group.addVar
   ( "Constraint_group_id" , netCDF::NcString() , n_Config_Constraint );

  auto Constraint_index_var = group.addVar
   ( "Constraint_index" , netCDF::NcUint() , n_Config_Constraint );

  for( size_t i = 0 ; i < v_Constraint_id.size() ; ++i ) {
   Constraint_group_id_var.putVar( { i } , v_Constraint_id[ i ].first );
   Constraint_index_var.putVar( { i } , v_Constraint_id[ i ].second );
   }
  }
 }  // end( CBlockConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void CBlockConfig::deserialize( netCDF::NcGroup & group )
{
 if( v_Config_Constraint.size() || v_Constraint_id.size() )
  throw( std::logic_error( "CBlockConfig::deserialize: deserializing a "
                           "non-empty CBlockConfig" ) );

 BlockConfig::deserialize( group );

 // Configuration for Constraint

 auto constrdim = group.getDim( "n_Config_Constraint" );
 size_t constrsize = constrdim.isNull() ? 0 : constrdim.getSize();

 v_Config_Constraint.resize( constrsize );
 v_Constraint_id.resize( constrsize );

 auto var_Constraint_group_id = group.getVar( "Constraint_group_id" );
 auto var_Constraint_index = group.getVar( "Constraint_index" );
 if( constrsize > 0 ) {
  if( var_Constraint_group_id.isNull() )
   throw( std::invalid_argument( "CBlockConfig::deserialize: netCDF variable "
                                 "'Constraint_group_id' was not provided." ) );
  else {
   auto dimensions = ::get_sizes_dimensions( var_Constraint_group_id );
   if( dimensions.size() != 1 || dimensions[ 0 ] != constrsize )
    throw( std::invalid_argument
           ( "CBlockConfig::deserialize: invalid dimensions of netCDF "
             "variable 'Constraint_group_id'. Its dimensions should be " +
             std::to_string( constrsize ) ) );

   if( ! var_Constraint_index.isNull() ) {
    auto dimensions = ::get_sizes_dimensions( var_Constraint_index );
    if( dimensions.size() != 1 || dimensions[ 0 ] != constrsize )
     throw( std::invalid_argument
            ( "CBlockConfig::deserialize: invalid dimensions of netCDF "
              "variable 'Constraint_index'. Its dimensions should be " +
              std::to_string( constrsize ) ) );
    }
   }
  }

 for( size_t i = 0 ; i < constrsize ; ++i ) {
  auto config_group = group.getGroup( "Config_Constraint_" +
                                      std::to_string( i ) );
  v_Config_Constraint[ i ] =
   dynamic_cast< ComputeConfig * >( new_Configuration( config_group ) );

  var_Constraint_group_id.getVar( { i } , & v_Constraint_id[ i ].first );

  if( ! var_Constraint_index.isNull() )
   var_Constraint_index.getVar( { i } , & v_Constraint_id[ i ].second );
  else
   v_Constraint_id[ i ].second = i;
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
/*------------------------ METHODS of CRBlockConfig ------------------------*/
/*--------------------------------------------------------------------------*/

CRBlockConfig::CRBlockConfig( const CRBlockConfig &old ) : RBlockConfig( old )
{
 v_Constraint_id = old.v_Constraint_id;

 v_Config_Constraint.resize( old.v_Config_Constraint.size() , nullptr );
 auto this_it = v_Config_Constraint.begin();
 auto old_it = old.v_Config_Constraint.cbegin();
 for( ; this_it != v_Config_Constraint.end() ; ++this_it , ++old_it )
  if( *old_it )
   *this_it = ( *old_it )->clone();
 }

/*--------------------------------------------------------------------------*/

void CRBlockConfig::get( Block * block ) {

 RBlockConfig::get( block );

 for( auto config : v_Config_Constraint )
  delete config;
 v_Config_Constraint.clear();

 if( ! block ) {
  return;
  }

 // ComputeConfig for the Constraint

 if( ! v_Constraint_id.empty() ) {

  // v_Constraint_id is not empty. Consider only these Constraint.

  v_Config_Constraint.reserve( v_Constraint_id.size() );

  for( const auto [ constraint_group_id , constraint_index ] :
        v_Constraint_id ) {

   auto constraint_group_index =
    ::get_Constraint_group_index( constraint_group_id , block );

   auto constraint = inspection::get_Constraint
    ( block , Block::ConstraintID( constraint_group_index , constraint_index ) );
   if( constraint )
    v_Config_Constraint.push_back( constraint->get_ComputeConfig( true ) );
   else
    throw ( std::logic_error
            ( "CRBlockConfig::get: Constraint with Constraint id ( " +
              constraint_group_id + " , " + std::to_string( constraint_index ) +
              ") was not found." ) );
   }
  }
 else {
  // v_Constraint_id is empty. Now we scan all Constraint.
  inspection::fill_ComputeConfig_Constraint( block , v_Config_Constraint ,
                                             v_Constraint_id );
  }
 }  // end( CRBlockConfig::get )

/*--------------------------------------------------------------------------*/

void CRBlockConfig::apply( Block * block , bool deleteold ) {

 if( ! block )
  return;

 RBlockConfig::apply( block , deleteold );

 // set the ComputeConfig of the Constraint

 for( std::size_t i = 0 ; i < v_Config_Constraint.size() ; ++i ) {

  const auto [ constraint_group_id , constraint_index ] =
   v_Constraint_id[ i ];

  auto constraint_group_index =
   ::get_Constraint_group_index( constraint_group_id , block );

  auto constraint = inspection::get_Constraint
   ( block , Block::ConstraintID( constraint_group_index , constraint_index ) );

  if( constraint ) {
   if( ( ! f_diff ) || v_Config_Constraint[ i ] )
    constraint->set_ComputeConfig( v_Config_Constraint[ i ] );
   }
  else
   throw ( std::logic_error( "CRBlockConfig::apply: Constraint with Constraint "
                             "id ( " + constraint_group_id + " , " +
                             std::to_string( constraint_index ) +
                             ") was not found." ) );
  }
 }  // end( CRBlockConfig::apply )

/*--------------------------------------------------------------------------*/

void CRBlockConfig::print( std::ostream &output ) const
{
 RBlockConfig::print( output );

 decltype( v_Constraint_id )::size_type i = 0;

 for( const auto config : v_Config_Constraint ) {
  output << "ComputeConfig for Constraint (" << v_Constraint_id[ i ].first
         << " , " << v_Constraint_id[ i ].second << ")" << std::endl;
  ++i;
  if( config )
   output << *config;
  else
   output << "nullptr" << std::endl;
  }
 output << std::endl;
 }  // end( CRBlockConfig::print )

/*--------------------------------------------------------------------------*/

void CRBlockConfig::load( std::istream & input ) {
 RBlockConfig::load( input );

 // Configuration for the Constraint

 int k;
 input >> eatcomments >> k;
 v_Config_Constraint.resize( k );
 v_Constraint_id.resize( k );

 for( int i = 0 ; i < k ; ++i ) {
  std::string constraint_group_id;
  Block::Index constraint_index;

  input >> eatcomments;
  input >> v_Constraint_id[ i ].first >> v_Constraint_id[ i ].second;

  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   v_Config_Constraint[ i ] = nullptr;
  else {
   std::string cname;
   input >> cname;
   v_Config_Constraint[ i ] =
    dynamic_cast<ComputeConfig *>( Configuration::new_Configuration( cname ) );
   if( ! v_Config_Constraint[ i ] )
    throw ( std::invalid_argument( "CRBlockConfig::load: invalid Configuration"
                                   " for the Constraint " +
                                   std::to_string( i ) + "." ) );
   input >> *v_Config_Constraint[ i ];
   }
  }
 }  // end( CRBlockConfig::load )

/*--------------------------------------------------------------------------*/

void CRBlockConfig::serialize( netCDF::NcGroup & group ) const
{
 RBlockConfig::serialize( group );

 // Configuration for the Constraint

 if( ! v_Config_Constraint.empty() ) {

  auto n_Config_Constraint = group.addDim( "n_Config_Constraint" ,
                                           v_Config_Constraint.size() );

  for( size_t i = 0 ; i < v_Config_Constraint.size() ; ++i ) {
   if( v_Config_Constraint[ i ] ) {
    auto config_group = group.addGroup( "Config_Constraint_" +
                                        std::to_string( i ) );
    v_Config_Constraint[ i ]->serialize( config_group );
    }
   }

  auto Constraint_group_id_var = group.addVar
   ( "Constraint_group_id" , netCDF::NcString() , n_Config_Constraint );

  auto Constraint_index_var = group.addVar
   ( "Constraint_index" , netCDF::NcUint() , n_Config_Constraint );

  for( size_t i = 0 ; i < v_Constraint_id.size() ; ++i ) {
   Constraint_group_id_var.putVar( { i } , v_Constraint_id[ i ].first );
   Constraint_index_var.putVar( { i } , v_Constraint_id[ i ].second );
   }
  }
 }  // end( CRBlockConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void CRBlockConfig::deserialize( netCDF::NcGroup & group )
{
 if( v_Config_Constraint.size() || v_Constraint_id.size() )
  throw( std::logic_error( "CRBlockConfig::deserialize: deserializing a "
                           "non-empty CRBlockConfig" ) );

 RBlockConfig::deserialize( group );

 // Configuration for Constraint

 auto constrdim = group.getDim( "n_Config_Constraint" );
 size_t constrsize = constrdim.isNull() ? 0 : constrdim.getSize();

 v_Config_Constraint.resize( constrsize );
 v_Constraint_id.resize( constrsize );

 auto var_Constraint_group_id = group.getVar( "Constraint_group_id" );
 auto var_Constraint_index = group.getVar( "Constraint_index" );
 if( constrsize > 0 ) {
  if( var_Constraint_group_id.isNull() )
   throw( std::invalid_argument( "CRBlockConfig::deserialize: netCDF variable "
                                 "'Constraint_group_id' was not provided." ) );
  else {
   auto dimensions = ::get_sizes_dimensions( var_Constraint_group_id );
   if( dimensions.size() != 1 || dimensions[ 0 ] != constrsize )
    throw( std::invalid_argument
           ( "CRBlockConfig::deserialize: invalid dimensions of netCDF "
             "variable 'Constraint_group_id'. Its dimensions should be " +
             std::to_string( constrsize ) ) );

   if( ! var_Constraint_index.isNull() ) {
    auto dimensions = ::get_sizes_dimensions( var_Constraint_index );
    if( dimensions.size() != 1 || dimensions[ 0 ] != constrsize )
     throw( std::invalid_argument
            ( "CRBlockConfig::deserialize: invalid dimensions of netCDF "
              "variable 'Constraint_index'. Its dimensions should be " +
              std::to_string( constrsize ) ) );
    }
   }
  }

 for( size_t i = 0 ; i < constrsize ; ++i ) {
  auto config_group = group.getGroup( "Config_Constraint_" +
                                      std::to_string( i ) );
  v_Config_Constraint[ i ] =
   dynamic_cast< ComputeConfig * >( new_Configuration( config_group ) );

  var_Constraint_group_id.getVar( { i } , & v_Constraint_id[ i ].first );

  if( ! var_Constraint_index.isNull() )
   var_Constraint_index.getVar( { i } , & v_Constraint_id[ i ].second );
  else
   v_Constraint_id[ i ].second = i;
  }
 }  // end( CRBlockConfig::deserialize( group ) )

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
