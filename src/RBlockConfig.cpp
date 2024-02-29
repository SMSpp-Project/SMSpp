/*--------------------------------------------------------------------------*/
/*------------------------- File RBlockConfig.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the RBlockConfig class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "BlockInspection.h"

#include "RBlockConfig.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using namespace SMSpp_di_unipi_it::BlockConfigHandlers;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register all seven *BlockConfig to the Configuration factory

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

// returns the index of the sub-Block with the given \p id
Block::Index get_nested_Block_index( const std::string & id ,
                                     const Block * block )
{
 if( ( ! id.empty() ) && std::isdigit( id.front() ) ) {
  // the id is the index of the sub-Block
  try { return( std::stoi( id ) ); }
  catch( ... ) { return( Inf< Block::Index >() ); }
  }
 else  // the id is the name of the sub-Block
  return( block->get_nested_Block_index( id ) );
 }

/*--------------------------------------------------------------------------*/
// returns the sub-Block with the given \p id

Block * get_nested_Block( const std::string & id , const Block * block )
{
 if( auto bi = block->get_nested_Block( id ) )
  return( bi );

 if( id.empty() || ( ! std::isdigit( id.front() ) ) )
  return( nullptr );

 Block::Index i;
 try { i = std::stoi( id ); }
 catch( ... ) { return( nullptr ); }

 return( block->get_nested_Block( i ) );
 }

/*--------------------------------------------------------------------------*/
// returns the index of the group of Constraint

Block::Index get_Constraint_group_index( const std::string & id ,
                                         const Block * block )
{
 if( ( ! id.empty() ) && std::isdigit( id.front() ) ) {
  // the group id is the index of the group of Constraint
  try { return( std::stoi( id ) ); }
  catch( ... ) { return( Inf< Block::Index >() ); }
  }
 else {  // the group id is the name of the group of Constraint
  auto i = block->get_s_const_index( id );           // try static
  if( i >= block->get_number_static_constraints() )  // if not
   i = block->get_d_const_index( id );               // must be dynamic

  return( i );
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace )

/*--------------------------------------------------------------------------*/

static bool advance( std::istream & input )
{
 input >> eatcomments;
 return( input.eof() );
 }

/*--------------------------------------------------------------------------*/

static void checkfail( std::istream & input )
{
 if( input.fail() )
  throw( std::invalid_argument( "*BlockConfig::load: stream read error" ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- METHODS of RHandler --------------------------*/
/*--------------------------------------------------------------------------*/

void RHandler::deserialize( const netCDF::NcGroup & group )
{
 auto n_sub_Block = group.getDim( "n_sub_Block" );

 if( n_sub_Block.isNull() )
  return;

 v_sub_BlockConfig.resize( n_sub_Block.getSize() );

 for( decltype( v_sub_BlockConfig )::size_type i = 0;
      i < v_sub_BlockConfig.size(); ++i ) {
  auto cg = group.getGroup( "sub-BlockConfig_" + std::to_string( i ) );
  v_sub_BlockConfig[ i ] =
   dynamic_cast< BlockConfig * >( Configuration::new_Configuration( cg ) );
  }

 auto var_sub_Block_id = group.getVar( "sub-Block-id" );
 if( ! var_sub_Block_id.isNull() ) {
  assert( var_sub_Block_id.getDimCount() == 1 );
  assert( var_sub_Block_id.getDim( 0 ).getSize() == n_sub_Block.getSize() );
  v_sub_Block_id.resize( var_sub_Block_id.getDim( 0 ).getSize() );
  var_sub_Block_id.getVar( v_sub_Block_id.data() );
  }
 else {
  decltype( v_sub_Block_id )::size_type n = n_sub_Block.getSize();
  v_sub_Block_id.resize( n );
  for( decltype( n ) i = 0 ; i < n ; ++i )
   v_sub_Block_id[ i ] = std::to_string( i );
  }
 }  // end( RHandler::deserialize( group ) )

/*--------------------------------------------------------------------------*/

void RHandler::get( Block * block )
{
 if( ! block ) {
  for( auto & config : v_sub_BlockConfig )
   delete config;
  v_sub_BlockConfig.clear();
  v_sub_Block_id.clear();
  return;
  }

 #ifndef NDEBUG
 if( v_sub_BlockConfig.size() != v_sub_Block_id.size() )
  throw( std::logic_error( "RHandler::get: inconsistent Rhandler state" ) );
 #endif

 if( v_sub_Block_id.empty() ) {  // scan *all* the sub-Block- - - - - - - - -
  for( Block::Index i = 0 ; i < block->get_number_nested_Blocks() ; ++i ) {
   auto bi = block->get_nested_Block( i );  // get i-th sub-Block

   if( auto BC = OCRBlockConfig::get_right_BlockConfig( bi ) ) {
    std::string id = bi->name();            // get its id
    if( id.empty() ) id = std::to_string( i );
    add_sub_BlockConfig( BC, std::move( id ) );
    }
   }

  return;  // all done
  }

 // only scan the existing sub-Block- - - - - - - - - - - - - - - - - - - - -

 for( decltype( v_sub_Block_id )::size_type i = 0 ;
      i < v_sub_Block_id.size() ; ++i ) {

  const auto id = v_sub_Block_id[ i ];
  Block::Index index = ::get_nested_Block_index( id , block );

  if( index >= block->get_number_nested_Blocks() )
   throw( std::logic_error( "RHandler::get: invalid sub-Block id: " + id ) );

  auto sB = block->get_nested_Block( index );

  if( v_sub_BlockConfig[ i ] )
   v_sub_BlockConfig[ i ]->get( sB );
  else
   v_sub_BlockConfig[ i ] = OCRBlockConfig::get_right_BlockConfig( sB );
  }
 }  // end( RHandler::get )

/*--------------------------------------------------------------------------*/

void RHandler::apply( Block * block , bool deleteold , bool diff )
{
 #ifndef NDEBUG
 if( v_sub_BlockConfig.size() != v_sub_Block_id.size() )
  throw( std::logic_error( "RHandler::apply: inconsistent RHandler state" ) );
 #endif

 for( decltype( v_sub_BlockConfig )::size_type i = 0 ;
      i < v_sub_BlockConfig.size() ; ++i ) {

  const auto & id = v_sub_Block_id[ i ];
  Block::Index sub_Block_index = ::get_nested_Block_index( id , block );
  if( sub_Block_index >= block->get_number_nested_Blocks() )
   throw( std::logic_error( "RHandler::apply: invalid sub-Block id: " + id )
	  );

  auto sub_Block = block->get_nested_Block( sub_Block_index );

  if( v_sub_BlockConfig[ i ] && sub_Block )
   v_sub_BlockConfig[ i ]->apply( sub_Block, deleteold );
  }
 }  // end( RHandler::apply )

/*--------------------------------------------------------------------------*/

void RHandler::serialize( netCDF::NcGroup & group ) const
{
 auto n_sub_Block = group.addDim( "n_sub_Block", v_sub_BlockConfig.size() );

 for( size_t i = 0 ; i < v_sub_BlockConfig.size() ; ++i )
  if( v_sub_BlockConfig[ i ] ) {
   auto cg = group.addGroup( "sub-BlockConfig_" + std::to_string( i ) );
   v_sub_BlockConfig[ i ]->serialize( cg );
   }

 netCDF::NcDim sub_Block_id_dim;
 if( v_sub_Block_id.size() == v_sub_BlockConfig.size() )
  sub_Block_id_dim = n_sub_Block;
 else
  sub_Block_id_dim = group.addDim( "n_sub_Block_id", v_sub_Block_id.size() );

 auto sub_Block_id_var = group.addVar( "sub-Block-id", netCDF::NcString(),
                                       { sub_Block_id_dim } );

 sub_Block_id_var.putVar( v_sub_Block_id.data() );

 }  // end( RHandler::serialize( group ) )

/*--------------------------------------------------------------------------*/

void RHandler::print( std::ostream & output ) const
{
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

 }  // end( RHandler::print )

/*--------------------------------------------------------------------------*/

void RHandler::load( std::istream & input )
{
 int k = 0;
 if( ! advance( input ) ) {
  input >> k;
  checkfail( input );
  }

 if( ! k ) {
  v_sub_Block_id.clear();
  v_sub_BlockConfig.clear();
  return;
  }

 const bool id_is_provided = ( k < 0 );
 k = std::abs( k );
 v_sub_Block_id.resize( k );
 v_sub_BlockConfig.resize( k , nullptr );

 for( int i = 0 ; i < k ; ++i ) {
  if( id_is_provided ) {
   input >> eatcomments >> v_sub_Block_id[ i ];
   checkfail( input );
   }
  else
   v_sub_Block_id[ i ] = std::to_string( i );

  auto cfg = Configuration::deserialize( input );
  v_sub_BlockConfig[ i ] = dynamic_cast< BlockConfig * >( cfg );
   if( ! v_sub_BlockConfig[ i ] ) {
    delete cfg;
    throw( std::invalid_argument(
		    "*BlockConfig::load: invalid BlockConfig for sub-Block "
		    + v_sub_Block_id[ i ] ) );
    }
   }
  }  // end( RHandler::load )

/*--------------------------------------------------------------------------*/
/*-------------------------- METHODS of CHandler --------------------------*/
/*--------------------------------------------------------------------------*/

void CHandler::deserialize( const netCDF::NcGroup & group )
{
 // Configuration for Constraint

 auto constrdim = group.getDim( "n_Config_Constraint" );
 size_t constrsize = constrdim.isNull() ? 0 : constrdim.getSize();

 v_Config_Constraint.resize( constrsize );
 v_Constraint_id.resize( constrsize );

 auto var_Constraint_group_id = group.getVar( "Constraint_group_id" );
 auto var_Constraint_index = group.getVar( "Constraint_index" );
 if( constrsize > 0 ) {
  if( var_Constraint_group_id.isNull() )
   throw( std::invalid_argument(
		     "*BlockConfig::deserialize: no Constraint_group_id" ) );
  else {
   auto dims1 = ::get_sizes_dimensions( var_Constraint_group_id );
   if( ( dims1.size() != 1 ) || ( dims1[ 0 ] != constrsize ) )
    throw( std::invalid_argument(
     "*BlockConfig::deserialize: invalid Constraint_group_id dimensions" ) );

   if( ! var_Constraint_index.isNull() ) {
    auto dims2 = ::get_sizes_dimensions( var_Constraint_index );
    if( ( dims2.size() != 1 ) || ( dims2[ 0 ] != constrsize ) )
     throw( std::invalid_argument(
        "*BlockConfig::deserialize: invalid Constraint_index dimensions" ) );
    }
   }
  }

 for( size_t i = 0 ; i < constrsize ; ++i ) {
  auto config_group = group.getGroup( "Config_Constraint_" +
                                      std::to_string( i ) );
  v_Config_Constraint[ i ] = dynamic_cast< ComputeConfig * >(
   Configuration::new_Configuration( config_group ) );

  var_Constraint_group_id.getVar( { i } , & v_Constraint_id[ i ].first );

  if( ! var_Constraint_index.isNull() )
   var_Constraint_index.getVar( { i }, & v_Constraint_id[ i ].second );
  else
   v_Constraint_id[ i ].second = i;
  }
 }  // end( CHandler::deserialize( group ) )

/*--------------------------------------------------------------------------*/

void CHandler::get( Block * block )
{
 if( ! block ) {
  for( auto config : v_Config_Constraint )
   delete config;
  v_Config_Constraint.clear();
  v_Constraint_id.clear();
  return;
  }

 #ifndef NDEBUG
  if( v_Constraint_id.size() != v_Config_Constraint.size() )
   throw( std::logic_error( "CHandler::get: inconsistent CHandler state" ) );
 #endif

 // ComputeConfig for the Constraint

 if( ! v_Constraint_id.empty() ) {
  // v_Constraint_id is not empty. Consider only these Constraint.

  for( Block::Index i = 0 ; i < v_Constraint_id.size() ; ++i ) {
   const auto[ id , ci ] = v_Constraint_id[ i ];
   auto gi = ::get_Constraint_group_index( id , block );
   if( auto constr = inspection::get_Constraint( block ,
					  Block::ConstraintID( gi , ci ) ) )
    v_Config_Constraint[ i ] =
     constr->get_ComputeConfig( false , v_Config_Constraint[ i ] );
   else
    throw( std::logic_error( "*BlockConfig::get: Constraint with id ( " + id
			     + " , " + std::to_string( ci ) + ") not found"
			     ) );
   }
  }
 else  // v_Constraint_id is empty. Now we scan all Constraint.
  inspection::fill_ComputeConfig_Constraint( block , v_Config_Constraint ,
                                             v_Constraint_id );
 }  // end( CHandler::get )

/*--------------------------------------------------------------------------*/

void CHandler::apply( Block * block , bool deleteold , bool diff )
{
 // set the ComputeConfig of the Constraint

 for( std::size_t i = 0 ; i < v_Config_Constraint.size() ; ++i ) {
  const auto[ id , ci ] = v_Constraint_id[ i ];
  auto gi = ::get_Constraint_group_index( id , block );
  if( auto constr = inspection::get_Constraint( block ,
                                          Block::ConstraintID( gi, ci ) ) ) {
   if( ( ! diff ) || v_Config_Constraint[ i ] )
    constr->set_ComputeConfig( v_Config_Constraint[ i ] );
   }
  else
   throw( std::logic_error( "*BlockConfig::apply: Constraint with id ( " +
			    id + " , " + std::to_string( ci ) + ") not found"
			    ) );
  }
 }  // end( CHandler::apply )

/*--------------------------------------------------------------------------*/

void CHandler::serialize( netCDF::NcGroup & group ) const
{
 if( v_Config_Constraint.empty() )
  return;

 auto n_Config_Constraint = group.addDim( "n_Config_Constraint" ,
                                          v_Config_Constraint.size() );

 for( size_t i = 0 ; i < v_Config_Constraint.size() ; ++i ) {
  if( v_Config_Constraint[ i ] ) {
   auto config_group = group.addGroup( "Config_Constraint_" +
                                       std::to_string( i ) );
   v_Config_Constraint[ i ]->serialize( config_group );
   }
  }

 auto Constraint_group_id_var = group.addVar( "Constraint_group_id" ,
                                              netCDF::NcString() ,
                                              n_Config_Constraint );

 auto Constraint_index_var = group.addVar( "Constraint_index" ,
                                           netCDF::NcUint() ,
                                           n_Config_Constraint );

 for( size_t i = 0 ; i < v_Constraint_id.size() ; ++i ) {
  Constraint_group_id_var.putVar( { i } , v_Constraint_id[ i ].first );
  Constraint_index_var.putVar( { i } , v_Constraint_id[ i ].second );
  }
 }  // end( CHandler::serialize( group ) )

/*--------------------------------------------------------------------------*/

void CHandler::print( std::ostream & output ) const
{
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

 }  // end( CHandler::print )

/*--------------------------------------------------------------------------*/

void CHandler::load( std::istream & input )
{
 unsigned int k = 0;
 if( ! advance( input ) ) {
  input >> k;
  checkfail( input );
  }

 if( ! k ) {
  v_Constraint_id.clear();
  v_Config_Constraint.clear();
  return;
  }

 v_Constraint_id.resize( k );
 v_Config_Constraint.resize( k , nullptr );

 for( unsigned int i = 0 ; i < k ; ++i ) {
  input >> eatcomments >> v_Constraint_id[ i ].first >> eatcomments
	>> v_Constraint_id[ i ].second >> eatcomments;
  checkfail( input );

  auto cfg = Configuration::deserialize( input );
  v_Config_Constraint[ i ] = dynamic_cast< ComputeConfig * >( cfg );
  if( ! v_Config_Constraint[ i ] ) {
   delete cfg;
   throw( std::invalid_argument(
		  "BlockConfig::load: invalid Configuration for Constraint "
		  + std::to_string( i ) ) );
   }
  }
 }  // end( CHandler::load )

/*--------------------------------------------------------------------------*/
/*--------------------------- METHODS of OHandler --------------------------*/
/*--------------------------------------------------------------------------*/

void OHandler::deserialize( const netCDF::NcGroup & group )
{
 if( f_Config_Objective )
  throw( std::logic_error( "deserializing a non-empty *OBlockConfig." ) );

 auto obj_group = group.getGroup( "Config_Objective" );
 if( ! obj_group.isNull() )
  f_Config_Objective = dynamic_cast< ComputeConfig * >(
   Configuration::new_Configuration( obj_group ) );
 }

/*--------------------------------------------------------------------------*/

void OHandler::get( Block * block )
{
 if( ! block ) {
  delete f_Config_Objective;
  f_Config_Objective = nullptr;
  return;
  }

 if( auto objective = block->get_objective() )
  f_Config_Objective = objective->get_ComputeConfig( false ,
                                                     f_Config_Objective );
 }

/*--------------------------------------------------------------------------*/

void OHandler::apply( Block * block , bool deleteold , bool diff )
{
 if( ! block )
  return;

 if( auto objective = block->get_objective() )
  if( ( ! diff ) || f_Config_Objective )
   objective->set_ComputeConfig( f_Config_Objective );
 }

/*--------------------------------------------------------------------------*/

void OHandler::serialize( netCDF::NcGroup & group ) const
{
 if( f_Config_Objective ) {
  auto obj_group = group.addGroup( "Config_Objective" );
  f_Config_Objective->serialize( obj_group );
  }
 }

/*--------------------------------------------------------------------------*/

void OHandler::print( std::ostream & output ) const
{
 if( f_Config_Objective )
  output << *f_Config_Objective << std::endl;
 }

/*--------------------------------------------------------------------------*/

void OHandler::load( std::istream & input )
{
 if( advance( input ) ) {
  f_Config_Objective = nullptr;
  return;
  }

 checkfail( input );

 auto cfg = Configuration::deserialize( input );
 f_Config_Objective = dynamic_cast< ComputeConfig * >( cfg );
 if( ! f_Config_Objective ) {
  delete cfg;
  throw( std::invalid_argument(
	   "*BlockConfig::load: invalid Configuration for the Objective" ) );
  }
 }  // end( OBlockConfig::load )

/*--------------------------------------------------------------------------*/
/*------------------------ METHODS of OCRBlockConfig -----------------------*/
/*--------------------------------------------------------------------------*/

BlockConfig * OCRBlockConfig::get_right_BlockConfig( const Block * block )
{
 if( ! block )
  return( nullptr );

 auto OCRBC = new OCRBlockConfig( block );
 if( OCRBC->OHandler::empty() ) {
  auto CRBC = new CRBlockConfig( std::move( *OCRBC ) );
  delete OCRBC;

  if( CRBC->CHandler::empty() ) {
   auto RBC = new RBlockConfig( std::move( *CRBC ) );
   delete CRBC;

   if( RBC->RHandler::empty() ) {
    auto BC = new BlockConfig( std::move( *RBC ) );
    delete RBC;

    if( BC->empty() ) {
     delete BC;
     return( nullptr );
     }
    else
     return( BC );
    }
   else
    return( RBC );
   }
  else {
   if( CRBC->RHandler::empty() ) {
    auto CBC = new CBlockConfig( std::move( *CRBC ) );
    delete CRBC;
    return( CBC );
    }
   else
    return( CRBC );
   }
  }

 if( OCRBC->CHandler::empty() ) {
  auto ORBC = new ORBlockConfig( std::move( *OCRBC ) );
  delete OCRBC;

  if( ORBC->RHandler::empty() ) {
   auto OBC = new OBlockConfig( std::move( *ORBC ) );
   delete ORBC;
   return( OBC );
   }
  else
   return( ORBC );
  }

 if( OCRBC->RHandler::empty() ) {
  auto OCBC = new OCBlockConfig( std::move( *OCRBC ) );
  delete OCRBC;
  return( OCBC );
  }

 return( OCRBC );

 }  // end( OCRBlockConfig::get_right_BlockConfig )

/*--------------------------------------------------------------------------*/
/*--------------------- End File RBlockConfig.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
