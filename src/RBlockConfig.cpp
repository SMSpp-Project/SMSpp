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

#include "RBlockConfig.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register RBlockConfig and ERBlockConfig to the Configuration factory

SMSpp_insert_in_factory_cpp_0( RBlockConfig );
SMSpp_insert_in_factory_cpp_0( ERBlockConfig );

/*--------------------------------------------------------------------------*/
/*------------------------- METHODS of RBlockConfig ------------------------*/
/*--------------------------------------------------------------------------*/

RBlockConfig::RBlockConfig( const RBlockConfig &old ) : Configuration()
{
 f_BlockConfig = nullptr;
 if( old.f_BlockConfig )
  f_BlockConfig = old.f_BlockConfig->clone();
 v_sub_BlockConfig.resize( old.v_sub_BlockConfig.size() , nullptr );
 for( std::size_t i = 0 ; i < v_sub_BlockConfig.size() ; ++i )
  if( old.v_sub_BlockConfig[ i ] )
   v_sub_BlockConfig[ i ] = old.v_sub_BlockConfig[ i ]->clone();
 }

/*--------------------------------------------------------------------------*/

void RBlockConfig::print( std::ostream &output ) const
{
 if( f_BlockConfig )
  output << *f_BlockConfig;
 for( const auto cfg : v_sub_BlockConfig )
  if( cfg )
   output << *cfg;
 output << std::endl;

 }  // end( RBlockConfig::print )

/*--------------------------------------------------------------------------*/

void RBlockConfig::load( std::istream & input ) {

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) ) {
  f_BlockConfig = nullptr;
  input.ignore( std::numeric_limits< std::streamsize >::max(),
                input.widen( '\n' ) );
 } else {
  std::string cname;
  input >> cname;
  f_BlockConfig = dynamic_cast< BlockConfig * >(
                              Configuration::new_Configuration( cname ) );
  if( f_BlockConfig)
   input >> *f_BlockConfig;
 }

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
   Configuration * tmpc = Configuration::new_Configuration( cname );
   BlockConfig * tmpbc = dynamic_cast<BlockConfig *>( tmpc );
   if( !tmpbc )
    throw ( std::invalid_argument( "not a BlockConfig object" ) );
   v_sub_BlockConfig[ i ] = tmpbc;
   input >> *tmpbc;
  }
 }
}  // end( RBlockConfig::load )

/*--------------------------------------------------------------------------*/

void RBlockConfig::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );

 if( f_BlockConfig ) {
  auto cg =  group.addGroup( "BlockConfig" );
  f_BlockConfig->serialize( cg );
 }

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

 if( f_BlockConfig || ! v_sub_BlockConfig.empty() )
  throw( std::logic_error( "deserializing a non-empty RBlockConfig" ) );

 auto cg = group.getGroup( "BlockConfig" );
 f_BlockConfig = dynamic_cast< BlockConfig *> ( new_Configuration( cg ) );

 size_t size = ( group.getDim( "n_sub_Block" ) ).getSize();

 v_sub_BlockConfig.resize( size );

 for( size_t i = 0 ; i < size ; ++i ) {
  auto cg = group.getGroup( "sub-BlockConfig_" + std::to_string( i ) );
  v_sub_BlockConfig[ i ] = dynamic_cast< BlockConfig *> (
						   new_Configuration( cg ) );
  }
 }  // end( RBlockConfig::deserialize( group ) )


/*--------------------------------------------------------------------------*/
/*------------------------ METHODS of ERBlockConfig ------------------------*/
/*--------------------------------------------------------------------------*/

ERBlockConfig::ERBlockConfig( const ERBlockConfig &old ) : RBlockConfig( old )
{
 v_ConstraintID.resize( old.v_ConstraintID.size() );
 for( std::size_t i = 0 ; i < v_ConstraintID.size() ; ++i )
   v_ConstraintID[ i ] = old.v_ConstraintID[ i ];

 v_BlockConfig_Constraints.resize
  ( old.v_BlockConfig_Constraints.size() , nullptr );
 for( std::size_t i = 0 ; i < v_BlockConfig_Constraints.size() ; ++i ) {
  v_BlockConfig_Constraints[ i ] = nullptr;
  if( old.v_BlockConfig_Constraints[ i ] )
   v_BlockConfig_Constraints[ i ] =
    old.v_BlockConfig_Constraints[ i ]->clone();
  }

 f_BlockConfig_Objective = nullptr;
 if( old.f_BlockConfig_Objective )
  f_BlockConfig_Objective = old.f_BlockConfig_Objective->clone();
 }

/*--------------------------------------------------------------------------*/

void ERBlockConfig::print( std::ostream &output ) const
{
 RBlockConfig::print( output );

 for( const auto cfgcstr : v_BlockConfig_Constraints )
  if( cfgcstr )
   output << *cfgcstr;
 if( f_BlockConfig_Objective )
  output << *f_BlockConfig_Objective;
 output << std::endl;
 }  // end( ERBlockConfig::print )

/*--------------------------------------------------------------------------*/

void ERBlockConfig::load( std::istream & input ) {
 RBlockConfig::load( input );

 // BlockConfig for Constraint

 int k;
 input >> eatcomments >> k;
 v_BlockConfig_Constraints.resize( k );
 v_ConstraintID.resize( k );
 for( int i = 0 ; i < k ; ++i ) {
  Block::Index group_index, constraint_index;
  input >> eatcomments;
  input >> group_index >> constraint_index;
  v_ConstraintID[ i ] = Block::ConstraintID( group_index , constraint_index );
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   v_BlockConfig_Constraints[ i ] = nullptr;
  else {
   std::string cname;
   input >> cname;
   Configuration *tmpc = Configuration::new_Configuration( cname );
   BlockConfig *tmpbc = dynamic_cast<BlockConfig *>( tmpc );
   if( ! tmpbc )
    throw( std::invalid_argument( "not a BlockConfig object" ) );
   v_BlockConfig_Constraints[ i ] = tmpbc;
   input >> *tmpbc;
   }
  }

 // BlockConfig for Objective

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_BlockConfig_Objective = nullptr;
 else {
  std::string cname;
  input >> cname;
  Configuration *tmpc = Configuration::new_Configuration( cname );
  BlockConfig *tmpbc = dynamic_cast<BlockConfig *>( tmpc );
  if( ! tmpbc )
   throw( std::invalid_argument( "not a BlockConfig object" ) );
  f_BlockConfig_Objective = tmpbc;
  input >> *tmpbc;
  }
 }  // end( ERBlockConfig::load )

/*--------------------------------------------------------------------------*/

void ERBlockConfig::serialize( netCDF::NcGroup & group ) const
{
 RBlockConfig::serialize( group );

 // BlockConfig for Constraint

 if( ! v_BlockConfig_Constraints.empty() ) {

  group.addDim( "n_BlockConfig_Constraint" ,
                v_BlockConfig_Constraints.size() );

  for( size_t i = 0 ; i < v_BlockConfig_Constraints.size() ; ++i ) {
   if( v_BlockConfig_Constraints[ i ] ) {
    auto bscc = group.addGroup( "BlockConfig_Constraint_"
                                + std::to_string( i ) );
    v_BlockConfig_Constraints[ i ]->serialize( bscc );
    }
   }

  auto ConstraintID_dim = group.addDim( "ConstraintID_dim" ,
                                        2 * v_ConstraintID.size() );

  auto ConstraintID_var = group.addVar( "ConstraintID" , netCDF::NcUint() ,
                                        ConstraintID_dim );

  for( size_t i = 0 ; i < v_ConstraintID.size() ; ++i ) {
   ConstraintID_var.putVar( { 2 * i } , { 2 } , std::vector<Block::Index>
                            { v_ConstraintID[ i ].first ,
                              v_ConstraintID[ i ].second }.data() );
   }
  }

 // BlockConfig for Objective

 if( f_BlockConfig_Objective ) {
  auto bscobj = group.addGroup( "BlockConfig_Objective" );
  f_BlockConfig_Objective->serialize( bscobj );
  }
 }  // end( ERBlockConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void ERBlockConfig::deserialize( netCDF::NcGroup & group )
{
 if( v_BlockConfig_Constraints.size() || v_ConstraintID.size() ||
     f_BlockConfig_Objective )
  throw( std::logic_error( "deserializing a non-empty BlockConfig" ) );

 RBlockConfig::deserialize( group );

 // BlockConfig for Constraint

 auto constrdim = group.getDim( "n_BlockConfig_Constraint" );
 size_t constrsize = constrdim.isNull() ? 0 : constrdim.getSize();

 v_BlockConfig_Constraints.resize( constrsize );
 v_ConstraintID.resize( constrsize );

 std::vector<Block::Index> var_ConstraintID;
 if( constrsize > 0 ) {
  ::deserialize( group , "ConstraintID" , 2 * constrsize ,
                 var_ConstraintID , false , false );
  }

 for( size_t i = 0 ; i < constrsize ; ++i ) {
  auto bscc = group.getGroup( "BlockConfig_Constraint_" +
                              std::to_string( i ) );
  v_BlockConfig_Constraints[ i ] = dynamic_cast< BlockConfig * >(
                                                  new_Configuration( bscc ) );
  v_ConstraintID[ i ] = Block::ConstraintID( var_ConstraintID[ 2 * i ] ,
                                             var_ConstraintID[ 2 * i + 1 ] );
  }

 // BlockConfig for Objective

 auto bscobj = group.getGroup( "BlockConfig_Objective" );
 if( ! bscobj.isNull() ) {
  f_BlockConfig_Objective = dynamic_cast< BlockConfig * >(
                                                new_Configuration( bscobj ) );
  }
 }  // end( ERBlockConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Block.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
