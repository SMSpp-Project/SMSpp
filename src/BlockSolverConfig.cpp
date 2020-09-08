/*--------------------------------------------------------------------------*/
/*---------------------- File BlockSolverConfig.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the BlockSolverConfig and RBlockSolverConfig classes.
 *
 * \version 0.10
 *
 * \date 07 - 09 - 2020
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
#include "BlockSolverConfig.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register BlockSolverConfig and RBlockSolverConfig to the Configuration
// factory

SMSpp_insert_in_factory_cpp_0( BlockSolverConfig );
SMSpp_insert_in_factory_cpp_0( RBlockSolverConfig );

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
}

/*--------------------------------------------------------------------------*/
/*-------------------- METHODS of BlockSolverConfig ------------------------*/
/*--------------------------------------------------------------------------*/
/*------------ CONSTRUCTING AND DESTRUCTING BlockSolverConfig --------------*/
/*--------------------------------------------------------------------------*/

BlockSolverConfig::BlockSolverConfig( const BlockSolverConfig &old )
 : Configuration() {

 f_diff = old.f_diff;
 v_SolverNames = old.v_SolverNames;

 v_SolverConfigs.resize( old.v_SolverConfigs.size() );
 for( std::size_t i = 0 ; i < v_SolverConfigs.size() ; ++i ) {
  v_SolverConfigs[ i ] = nullptr;
  if( old.v_SolverConfigs[ i ] )
   v_SolverConfigs[ i ] = old.v_SolverConfigs[ i ]->clone();
  }
 }

/*--------------------------------------------------------------------------*/

BlockSolverConfig * BlockSolverConfig::deserialize( netCDF::NcFile & f ,
                                                    const unsigned int idx )
{
 try {
  netCDF::NcGroupAtt ftype = f.getAtt( "SMS++_file_type" );
  if( ftype.isNull() )
   return( nullptr );

  int type;
  ftype.getValues( & type );

  if( ( type != eProbFile ) && ( type != eConfigFile ) )
   return( nullptr );

  netCDF::NcGroup cg;

  if( type == eProbFile ) {
   netCDF::NcGroup dg = f.getGroup( "Config_" + std::to_string( idx ) );
   if( dg.isNull() )
    return( nullptr );

   cg = dg.getGroup( "SolverConfig" );
   }
  else
   cg = f.getGroup( "Config_" + std::to_string( idx ) );

  auto result = new_Configuration( cg );
  auto bcresult = dynamic_cast< BlockSolverConfig * >( result );
  if( ! bcresult ) {
   delete result;
   return( nullptr );
   }

  return( bcresult );
  }
 catch( netCDF::exceptions::NcException & e ) {
  std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
  }
 catch( std::exception & e ) {
  std::cerr << "error " << e.what() << " in deserialize" << std::endl;
  }
 catch( ... ) {
  std::cerr << "unknown error in deserialize" << std::endl;
  }

 return( nullptr );

 } // end( BlockSolverConfig::deserialize( file ) )

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::deserialize( netCDF::NcGroup & group )
{
 if( v_SolverNames.size() || v_SolverConfigs.size() )
  throw( std::logic_error( "deserializing a non-empty BlockSolverConfig" ) );

 Configuration::deserialize( group );

 netCDF::NcGroupAtt diff = group.getAtt( "diff" );
 if( diff.isNull() )
  f_diff = false;
 else {
  int diffint;
  diff.getValues( &diffint );
  f_diff = diffint > 0;
 }

 size_t slvsize = 0;
 auto dim = group.getDim( "n_SolverConfig" );
 if( ! dim.isNull() )
  slvsize = dim.getSize();

 v_SolverNames.resize( slvsize );
 v_SolverConfigs.resize( slvsize );

 netCDF::NcVar slvnms = group.getVar( "SolverNames" );

 if( slvsize > 0 && slvnms.isNull() )
  throw( std::logic_error( "BlockSolverConfig::deserialize: 'SolverNames' "
                           "was not provided." ) );

 std::vector<size_t> idx( 1 );
 for( size_t i = 0 ; i < slvsize ; ++i ) {
  idx[ 0 ] = i;
  char * str;
  slvnms.getVar( idx , & str );
  v_SolverNames[ i ] = std::string( str );
  auto sc = group.getGroup( "SolverConfig_" + std::to_string( i ) );
  v_SolverConfigs[ i ] = dynamic_cast< ComputeConfig * >
   ( new_Configuration( sc ) );
  }
 }  // end( BlockSolverConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*----------------- OTHER INITIALIZATIONS BlockSolverConfig ----------------*/
/*--------------------------------------------------------------------------*/

void BlockSolverConfig::get( Block * block , bool clear ) {

 if( clear ) {
  this->clear();
  return;
  }

 for( auto sSC : v_SolverConfigs )
  delete sSC;

 if( ! block ) {
  v_SolverConfigs.clear();
  return;
  }

 c_Lst_Solver & ls = block->get_registered_solvers();

 v_SolverNames.resize( ls.size() );
 v_SolverConfigs.resize( ls.size() );

 auto lsit = ls.begin();
 for( c_Lst_Solver::size_type i = 0 ; i < ls.size() ; ++i , ++lsit ) {
  if( *lsit ) {
   v_SolverNames[ i ] = (*lsit)->classname();
   v_SolverConfigs[ i ] = (*lsit)->get_ComputeConfig();
   }
  else {
   v_SolverNames[ i ] = "";
   v_SolverConfigs[ i ] = nullptr;
   }
  }
 }  // end( BlockSolverConfig::get )

/*--------------------------------------------------------------------------*/
/*-------- METHODS DESCRIBING THE BEHAVIOR OF THE BlockSolverConfig --------*/
/*--------------------------------------------------------------------------*/

void BlockSolverConfig::apply( Block * block ) const
{
 if( ! block )
  return;

 // set the configurations for the Solver of the Block -----------------------
 //---------------------------------------------------------------------------
 auto & solvers = block->get_registered_solvers();
 auto sit = solvers.begin();
 auto nit = v_SolverNames.begin();
 auto cit = v_SolverConfigs.begin();

 // process existing Solvers -------------------------------------------------

 if( f_diff ) {  // differential mode ----------------------------------
  for( ; ( sit != solvers.end() ) && ( nit != v_SolverNames.end() ) ;
       ++sit , ++nit ) {

   if( ! (*nit).empty() ) {  // if the name is empty do nothing
    Solver *oldS = *sit;
    block->replace_Solver( Solver::new_Solver( *nit ) , sit );
    delete oldS;
    }

   if( cit != v_SolverConfigs.end() ) {
    if( *cit )  // if the configuration is empty, do nothing
     (*sit)->set_ComputeConfig( *cit );
    ++cit;
    }
   }
  }
 else {                // setting mode ---------------------------------------
  for( ; ( sit != solvers.end() ) && ( nit != v_SolverNames.end() ) ;
       ++nit ) {

   Solver *oldS = *sit;

   if( (*nit).empty() ) {  // empty name
    block->unregister_Solver( sit );
    if( cit != v_SolverConfigs.end() )
     ++cit;  // ignore corresponding configuration
    // note: sit is not increased because the list is shortened
    }
   else {                   // non-empty name
    block->replace_Solver( Solver::new_Solver( *nit ) , sit );
    if( cit != v_SolverConfigs.end() ) {
     (*sit)->set_ComputeConfig( *cit );
     ++cit;
     }
    ++sit;
    }

   delete oldS;
   }
  }                    // end setting mode -----------------------------------

 // process Solvers in the Configuration but not in the Block-----------------

 for( ; nit != v_SolverNames.end() ; ++nit )
  if( (*nit).empty() ) {  // if the name is empty
   if( cit != v_SolverConfigs.end() )
    ++cit;
   }
  else {                   // the name is non-empty
   // important note: the order is
   // - first the Solver is created;
   // - then it is ComputeConfig-ured
   // - only then it is passed to the Block

   auto slvr = Solver::new_Solver( *nit );

   if( cit != v_SolverConfigs.end() ) {
    if( *cit )  // if the configurtion is empty, do nothing
     slvr->set_ComputeConfig( *cit );
    ++cit;
    }

   block->register_Solver( slvr );
   }
 }  // end( BlockSolverConfig::apply )

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::reset_Solver( Block * block ) const {
 // unregister Solver in reverse order
 auto & solvers = block->get_registered_solvers();
 for( auto it = solvers.rbegin() ; it != solvers.rend() ; ++it ) {
  Solver *oldS = *it;
  block->unregister_Solver( --(it.base()) );  // convert backward into forward
  delete oldS;
  }
 }  // end( BlockSolverConfig::reset_Solver )

/*--------------------------------------------------------------------------*/
/*------ METHODS FOR LOADING, PRINTING & SAVING THE BlockSolverConfig ------*/
/*--------------------------------------------------------------------------*/

void BlockSolverConfig::serialize( netCDF::NcFile & f , const int type )
 const
{
 if( type == eConfigFile ) {
  Configuration::serialize( f , type );
  return;
  }

 auto cg = ( f.addGroup( "Config_" + std::to_string( f.getGroupCount() )
                         ) ).addGroup( "SolverConfig" );
 serialize( cg );

 }  // end( BlockSolverConfig::serialize( file ) )

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );

 group.putAtt( "diff" , netCDF::NcInt() , int( f_diff ) );

 netCDF::NcDim sd = group.addDim( "n_SolverConfig" , v_SolverConfigs.size() );

 netCDF::NcVar slvnms = group.addVar( "SolverNames" , netCDF::NcString() ,
                                      sd );
 std::vector<size_t> idx( 1 );
 for( size_t i = 0 ; i < v_SolverConfigs.size() ; ++i ) {
  idx[ 0 ] = i;
  slvnms.putVar( idx , v_SolverNames[ i ] );
  if( v_SolverConfigs[ i ] ) {
   auto sc = group.addGroup( "SolverConfig_" + std::to_string( i ) );
   v_SolverConfigs[ i ]->serialize( sc );
   }
  }
 }  // end( BlockSolverConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::print( std::ostream &output ) const
{
 output << private_name();
 if( f_diff ) output << "[diff]";
 output << ": " << std::endl;
 for( std::size_t i = 0 ; i < v_SolverNames.size() ; ++i )
  output << v_SolverNames[ i ] << ": " << v_SolverConfigs[ i ];
 output << std::endl;
 }

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::load( std::istream &input )
{
 input >> eatcomments >> f_diff;

 int k;
 input >> eatcomments >> k;
 v_SolverNames.resize( k );
 for( int i = 0 ; i < k ; ++i ) {
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   v_SolverNames[ i ] = nullptr;
  else
   input >> v_SolverNames[ i ];
  }

 input >> eatcomments >> k;
 v_SolverConfigs.resize( k );
 for( int i = 0 ; i < k ; ++i ) {
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   v_SolverConfigs[ i ] = nullptr;
  else {
   std::string cname;
   input >> cname;
   Configuration *tmpc = Configuration::new_Configuration( cname );
   ComputeConfig *tmpbc = dynamic_cast<ComputeConfig *>( tmpc );
   if( ! tmpbc )
    throw( std::invalid_argument( "not a ComputeConfig object" ) );
   v_SolverConfigs[ i ] = tmpbc;
   input >> *tmpbc;
   }
  }
 }  // end( BlockSolverConfig::load )


/*--------------------------------------------------------------------------*/
/*-------------------- METHODS of RBlockSolverConfig -----------------------*/
/*--------------------------------------------------------------------------*/
/*------------ CONSTRUCTING AND DESTRUCTING RBlockSolverConfig -------------*/
/*--------------------------------------------------------------------------*/

RBlockSolverConfig::RBlockSolverConfig( const RBlockSolverConfig &old )
 : BlockSolverConfig( old ) {

 v_BlockSolverConfig.resize( old.v_BlockSolverConfig.size() );
 for( std::size_t i = 0 ; i < v_BlockSolverConfig.size() ; ++i ) {
  v_BlockSolverConfig[ i ] = nullptr;
  if( old.v_BlockSolverConfig[ i ] )
   v_BlockSolverConfig[ i ] = old.v_BlockSolverConfig[ i ]->clone();
  }

 v_sub_Block_id = old.v_sub_Block_id;
 }  // end( RBlockSolverConfig::RBlockSolverConfig )

/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::deserialize( netCDF::NcGroup & group )
{
 if( v_BlockSolverConfig.size() )
  throw( std::logic_error( "deserializing a non-empty RBlockSolverConfig" ) );

 BlockSolverConfig::deserialize( group );

 // BlockSolverConfig for sub-Block

 auto dim = group.getDim( "n_BlockSolverConfig" );
 if( dim.isNull() )
  return;
 size_t num_config = dim.getSize();

 v_BlockSolverConfig.resize( num_config );

 for( size_t i = 0 ; i < v_BlockSolverConfig.size() ; ++i ) {
  auto bc = group.getGroup( "BlockSolverConfig_" + std::to_string( i ) );
  v_BlockSolverConfig[ i ] = dynamic_cast< BlockSolverConfig * >(
                                                    new_Configuration( bc ) );
  }

 auto var_sub_Block_id = group.getVar( "sub-Block-id" );
 if( ! var_sub_Block_id.isNull() ) {
  assert( var_sub_Block_id.getDimCount() == 1 );
  assert( var_sub_Block_id.getDim( 0 ).getSize() == num_config );
  v_sub_Block_id.resize( var_sub_Block_id.getDim( 0 ).getSize() );
  var_sub_Block_id.getVar( v_sub_Block_id.data() );
  }
 else {
  v_sub_Block_id.resize( num_config );
  for( decltype( v_sub_Block_id )::size_type i = 0 ;
       i < v_sub_Block_id.size() ; ++i )
   v_sub_Block_id[ i ] = std::to_string( i );
  }
 }  // end( RBlockSolverConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*----------------- OTHER INITIALIZATIONS RBlockSolverConfig ---------------*/
/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::get( Block * block , bool clear ) {

 BlockSolverConfig::get( block , clear );

 for( auto sBSC : v_BlockSolverConfig )
  delete sBSC;

 if( ! block ) {
  v_BlockSolverConfig.clear();
  return;
  }

 const auto number_nested_Blocks = block->get_number_nested_Blocks();

 if( v_sub_Block_id.empty() ) {
  v_sub_Block_id.resize( number_nested_Blocks );
  for( decltype( v_sub_Block_id )::size_type i = 0 ;
       i < v_sub_Block_id.size() ; ++i )
   v_sub_Block_id[ i ] = std::to_string( i );
  }

 v_BlockSolverConfig.resize( v_sub_Block_id.size() );

 for( decltype( v_sub_Block_id )::size_type i = 0 ;
      i < v_sub_Block_id.size() ; ++i ) {

  const auto id = v_sub_Block_id[ i ];
  Block::Index sub_Block_index = ::get_nested_Block_index( id , block );

  if( sub_Block_index >= block->get_number_nested_Blocks() )
   throw ( std::logic_error( "RBlockSolverConfig::get: "
                             "invalid sub-Block id: " + id + "." ) );

  v_BlockSolverConfig[ i ] = new RBlockSolverConfig // TODO
   ( block->get_nested_Block( sub_Block_index ) , false , clear );
  }
 }  // end( RBlockSolverConfig::get )

/*--------------------------------------------------------------------------*/
/*-------- METHODS DESCRIBING THE BEHAVIOR OF THE RBlockSolverConfig -------*/
/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::apply( Block * block ) const
{
 if( ! block )
  return;

 // set the configurations for the Solver of the Block -----------------------
 //---------------------------------------------------------------------------

 BlockSolverConfig::apply( block );

 // set the configurations for the sub-Block ---------------------------------
 //---------------------------------------------------------------------------

 assert( v_BlockSolverConfig.size() == v_sub_Block_id.size() );

 for( decltype( v_BlockSolverConfig )::size_type i = 0 ;
      i < v_BlockSolverConfig.size() ; ++i ) {

  const auto & id = v_sub_Block_id[ i ];
  Block::Index sub_Block_index = ::get_nested_Block_index( id , block );
  if( sub_Block_index >= block->get_number_nested_Blocks() )
   throw ( std::logic_error( "RBlockConfig::apply: invalid sub-Block id: "
                             + id + "." ) );

  auto sub_Block = block->get_nested_Block( sub_Block_index );

  if( v_BlockSolverConfig[ i ] && sub_Block )
   v_BlockSolverConfig[ i ]->apply( sub_Block );
  }
 }  // end( RBlockSolverConfig::apply )

/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::reset_Solver( Block * block ) const {
 BlockSolverConfig::reset_Solver( block );

 // reset all Solver in the sub-Block

 assert( v_BlockSolverConfig.size() == v_sub_Block_id.size() );

 for( decltype( v_BlockSolverConfig )::size_type i = 0 ;
      i < v_BlockSolverConfig.size() ; ++i ) {

  const auto & id = v_sub_Block_id[ i ];
  Block::Index sub_Block_index = ::get_nested_Block_index( id , block );
  if( sub_Block_index >= block->get_number_nested_Blocks() )
   throw ( std::logic_error( "RBlockConfig::apply: invalid sub-Block id: "
                             + id + "." ) );

  auto sub_Block = block->get_nested_Block( sub_Block_index );

  if( v_BlockSolverConfig[ i ] && sub_Block )
   v_BlockSolverConfig[ i ]->reset_Solver( sub_Block );
  }
 }  // end( RBlockSolverConfig::reset_Solver )

/*--------------------------------------------------------------------------*/
/*------ METHODS FOR LOADING, PRINTING & SAVING THE RBlockSolverConfig -----*/
/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::serialize( netCDF::NcGroup & group ) const
{
 BlockSolverConfig::serialize( group );

 // BlockSolverConfig for sub-Block

 auto n_BlockSolverConfig =
  group.addDim( "n_BlockSolverConfig" , v_BlockSolverConfig.size() );

 for( size_t i = 0 ; i < v_BlockSolverConfig.size() ; ++i ) {
  if( v_BlockSolverConfig[ i ] ) {
   auto bc = group.addGroup( "BlockSolverConfig_" + std::to_string( i ) );
   v_BlockSolverConfig[ i ]->serialize( bc );
   }
  }

 netCDF::NcDim sub_Block_id_dim;
 if( v_sub_Block_id.size() == v_BlockSolverConfig.size() )
  sub_Block_id_dim = n_BlockSolverConfig;
 else
  sub_Block_id_dim = group.addDim( "n_sub_Block_id" , v_sub_Block_id.size() );

 auto sub_Block_id_var = group.addVar( "sub-Block-id" , netCDF::NcString() ,
                                       { sub_Block_id_dim } );

 sub_Block_id_var.putVar( v_sub_Block_id.data() );

 }  // end( RBlockSolverConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::print( std::ostream &output ) const
{
 BlockSolverConfig::print( output );

 decltype( v_BlockSolverConfig )::size_type i = 0;
 for( const auto config : v_BlockSolverConfig ) {
  std::string id = ( i < v_sub_Block_id.size() ) ? v_sub_Block_id[ i ] : "?";
  ++i;
  output << "BlockSolverConfig for sub-Block " << id << std::endl;
  if( config )
   output << *config;
  else
   output << "nullptr" << std::endl;
  }
 output << std::endl;
 }

/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::load( std::istream &input )
{

 BlockSolverConfig::load( input );

 // BlockSolverConfig for sub-Block

 int k;
 input >> eatcomments >> k;
 const bool id_is_provided = ( k < 0 );
 k = std::abs( k );
 v_BlockSolverConfig.resize( k , nullptr );
 v_sub_Block_id.resize( k );

 for( int i = 0 ; i < k ; ++i ) {

  if( id_is_provided ) {
   input >> eatcomments >> v_sub_Block_id[ i ];
   }
  else
   v_sub_Block_id[ i ] = std::to_string( i );

  input >> eatcomments;

  if( input.peek() == input.widen( '*' ) ) {
   v_BlockSolverConfig[ i ] = nullptr;
   input.ignore( std::numeric_limits< std::streamsize >::max() ,
                 input.widen( '\n' ) );
   }
  else {
   std::string cname;
   input >> cname;
   v_BlockSolverConfig[ i ] = dynamic_cast< BlockSolverConfig * >
    ( Configuration::new_Configuration( cname ) );
   if( ! v_BlockSolverConfig[ i ] )
    throw ( std::invalid_argument("RBlockSolverConfig::load: invalid "
                                  "BlockSolverConfig for sub-Block " +
                                  v_sub_Block_id[ i ] + "." ) );
   input >> *v_BlockSolverConfig[ i ];
   }
  }
 }  // end( RBlockSolverConfig::load )

/*--------------------------------------------------------------------------*/
/*------------------ End File BlockSolverConfig.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
