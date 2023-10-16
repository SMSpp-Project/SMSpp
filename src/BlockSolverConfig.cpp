/*--------------------------------------------------------------------------*/
/*---------------------- File BlockSolverConfig.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the BlockSolverConfig and RBlockSolverConfig classes.
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
                                     const Block * block )
{
 if( ( ! id.empty() ) && std::isdigit( id.front() ) ) {
  // The id is the index of the sub-Block
  try { return( std::stoi( id ) ); }
  catch( ... ) { return( Inf< Block::Index >() ); }
  }
 else  // The id is the name of the sub-Block
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

}  // end( namespace )

/*--------------------------------------------------------------------------*/

static bool advance( std::istream & input )
{
 input >> eatcomments;
 return( input.eof() );
 }

/*--------------------------------------------------------------------------*/

static void checkfail( std::istream & input , const std::string & msg )
{
 if( input.fail() )
  throw( std::invalid_argument( msg ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- METHODS of BlockSolverConfig ------------------------*/
/*--------------------------------------------------------------------------*/
/*------------ CONSTRUCTING AND DESTRUCTING BlockSolverConfig --------------*/
/*--------------------------------------------------------------------------*/

BlockSolverConfig::BlockSolverConfig( const BlockSolverConfig & old )
 : Configuration()
{
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

BlockSolverConfig::BlockSolverConfig( BlockSolverConfig && old ) noexcept
 : Configuration()
{
 f_diff = old.f_diff;
 v_SolverNames = std::move( old.v_SolverNames );
 v_SolverConfigs = std::move( old.v_SolverConfigs );
 }

/*--------------------------------------------------------------------------*/

BlockSolverConfig * BlockSolverConfig::deserialize( netCDF::NcFile & f,
						    unsigned int idx )
{
 try {
  netCDF::NcGroupAtt ftype = f.getAtt( "SMS++_file_type" );
  if( ftype.isNull() )
   return( nullptr );

  int type;
  ftype.getValues( &type );

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

void BlockSolverConfig::deserialize( const netCDF::NcGroup & group )
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

 size_t num_solvers = 0;
 auto dim = group.getDim( "n_SolverConfig" );
 if( ! dim.isNull() )
  num_solvers = dim.getSize();

 v_SolverNames.resize( num_solvers );
 v_SolverConfigs.resize( num_solvers , nullptr );

 netCDF::NcVar solver_names_var = group.getVar( "SolverNames" );

 if( ( num_solvers > 0 ) && solver_names_var.isNull() )
  throw( std::invalid_argument(
	          "BlockSolverConfig::deserialize: missing SolverNames" ) );

 for( size_t i = 0 ; i < num_solvers ; ++i ) {
  char * solver_name;
  solver_names_var.getVar( { i } , &solver_name );
  v_SolverNames[ i ] = std::string( solver_name );
  auto sc = group.getGroup( "SolverConfig_" + std::to_string( i ) );
  if( sc.isNull() )
   v_SolverConfigs[ i ] = nullptr;
  else {
   auto ccfg = dynamic_cast< ComputeConfig * >( new_Configuration( sc ) );
   if( ccfg )
    v_SolverConfigs[ i ] = ccfg;
   else {
    delete ccfg;
    throw( std::invalid_argument(
	          "BlockSolverConfig::deserialize: not a ComputeConfig" ) );    
    }
   }
  }
 }  // end( BlockSolverConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*----------------- OTHER INITIALIZATIONS BlockSolverConfig ----------------*/
/*--------------------------------------------------------------------------*/

void BlockSolverConfig::get( const Block * block , bool clear )
{
 // deal with the trivial cases first
 if( clear || ( ! block ) ) {
  for( auto config : v_SolverConfigs )
   delete config;
  v_SolverConfigs.clear();
  v_SolverNames.clear();
  return;
  }

 // get the Solver (name) and the ComputeConfig (pointer) for each Solver
 auto registered_Solvers = block->get_registered_solvers();

 v_SolverNames.resize( registered_Solvers.size() );
 v_SolverConfigs.resize( registered_Solvers.size() );

 auto it = registered_Solvers.begin();

 if( f_diff )
  for( auto & el : v_SolverConfigs )
   el = (*(it++))->get_ComputeConfig();
 else
  for( decltype( registered_Solvers )::size_type i = 0 ;
       i < registered_Solvers.size() ; ++i , ++it ) {
   v_SolverNames[ i ] = ( *it )->classname();
   v_SolverConfigs[ i ] = ( *it )->get_ComputeConfig();
  }
 }  // end( BlockSolverConfig::get )

/*--------------------------------------------------------------------------*/
/*---------- METHODS DESCRIBING THE BEHAVIOR OF BlockSolverConfig ----------*/
/*--------------------------------------------------------------------------*/

void BlockSolverConfig::apply( Block * block ) const
{
 if( ! block )
  return;

 if( ( ! f_diff ) && v_SolverNames.empty() ) {
  // applying a BlockSolverConfig without Solver in setting mode means
  // unregistering and deleting all existing Solver
  block->unregister_Solvers( true );  // do it with one call
  return;                             // all done
  }

 auto & solvers = block->get_registered_solvers();
 auto sit = solvers.begin();
 auto nit = v_SolverNames.begin();
 auto cit = v_SolverConfigs.begin();

 // process existing Solvers - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( f_diff ) {  // differential mode- - - - - - - - - - - - - - - - - - - - -
  // process existing Solvers

  for( ; ( sit != solvers.end() ) && ( nit != v_SolverNames.end() ) ;
         ++sit , ++nit , ++cit ) {
   //  note: the order of operations is important

   // if the name is empty use the existing solver, otherwise create one
   auto slvr = nit->empty() ? *sit : Solver::new_Solver( *nit );

   if( *cit )                             // if the ComputeConfig is there
    slvr->set_ComputeConfig( *cit );      // ComputeConfig-ure it

   if( ! nit->empty() )                   // if it is a new one, only now it
    block->replace_Solver( slvr, sit, true );  // replaces the existing one
   }

  // if any Solver in the Block remains after that the end of v_SolverNames
  // is reached, nothing is done
  }
 else {          // setting mode - - - - - - - - - - - - - - - - - - - - - - -
  // delete extra Solvers

  while( solvers.size() > v_SolverNames.size() )
   block->unregister_Solver( --solvers.end(), true );

  // process existing Solvers

  for( ; ( sit != solvers.end() ) && ( nit != v_SolverNames.end() ) ;
         ++nit , ++sit , ++cit ) {
   //  note: the order of operations is important

   auto slvr = Solver::new_Solver( *nit );  // first create a new Solver

   if( *cit )                               // if the ComputeConfig is there
    slvr->set_ComputeConfig( *cit );        // ComputeConfig-ure it

   // only then replace the existing one
   block->replace_Solver( slvr, sit, true );
   }
  }              // end setting mode - - - - - - - - - - - - - - - - - - - - -

 // process Solvers in the Configuration but not in the Block- - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( ; nit != v_SolverNames.end() ; ++nit , ++cit ) {
  //  note: the order of operations is important

  auto slvr = Solver::new_Solver( *nit );  // first create the Solver

  if( *cit )                               // if the ComputeConfig is there
   slvr->set_ComputeConfig( *cit );        // ComputeConfig-ure it

  block->register_Solver( slvr );          // only then pass it to the Block
  }

 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 }  // end( BlockSolverConfig::apply )

/*--------------------------------------------------------------------------*/
/*------ METHODS FOR LOADING, PRINTING & SAVING THE BlockSolverConfig ------*/
/*--------------------------------------------------------------------------*/

void BlockSolverConfig::serialize( netCDF::NcFile & f , int type ) const
{
 if( type == eConfigFile )
  Configuration::serialize( f, type );
 else {
  auto cg = ( f.addGroup( "Config_" + std::to_string( f.getGroupCount() )
  ) ).addGroup( "SolverConfig" );
  serialize( cg );
  }
 }  // end( BlockSolverConfig::serialize( file ) )

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );

 group.putAtt( "diff" , netCDF::NcInt(), int( f_diff ) );

 netCDF::NcDim dim = group.addDim( "n_SolverConfig" ,
                                   v_SolverConfigs.size() );

 netCDF::NcVar solver_names_var = group.addVar( "SolverNames" ,
                                                netCDF::NcString(), dim );

 for( size_t i = 0 ; i < v_SolverConfigs.size() ; ++i ) {
  solver_names_var.putVar( { i }, v_SolverNames[ i ] );
  if( v_SolverConfigs[ i ] ) {
   auto sc = group.addGroup( "SolverConfig_" + std::to_string( i ) );
   v_SolverConfigs[ i ]->serialize( sc );
   }
  }
 }  // end( BlockSolverConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::print( std::ostream & output ) const
{
 output << private_name();
 if( f_diff ) output << "[diff]";
 output << ": " << std::endl;
 for( std::size_t i = 0 ; i < v_SolverNames.size() ; ++i )
  output << v_SolverNames[ i ] << ": " << v_SolverConfigs[ i ];
 output << std::endl;
 }

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::load( std::istream & input )
{
 static std::string sre( "BlockSolverConfig::load: stream read error" );

 unsigned int k = 0;
 if( ! advance( input ) ) {
  input >> f_diff;
  checkfail( input , sre );

  if( ! advance( input ) ) {
   input >> k;
   checkfail( input , sre );
   }
  }

 if( ! k ) {
  v_SolverNames.clear();
  v_SolverConfigs.clear();
  return;
  }

 v_SolverNames.resize( k );
 v_SolverConfigs.resize( k , nullptr );
 for( unsigned int i = 0 ; i < k ; ++i ) {
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   input.get();  // read away (and ignore) the '*' from the stream
  else
   input >> v_SolverNames[ i ];

  checkfail( input , sre );
  }

 if( advance( input ) )
  return;

 input >> k;
 checkfail( input , sre );
 
 if( k > v_SolverNames.size() )
  v_SolverNames.resize( k );
 v_SolverConfigs.resize( std::max( k , Index( v_SolverNames.size() ) ) ,
                         nullptr );

 for( unsigned int i = 0 ; i < k ; ++i ) {
  auto cfg = Configuration::deserialize( input );
  if( ! cfg )  // empty Configuration
   continue;   // that's OK, but a nonempty one must be a ComputeConfig
  v_SolverConfigs[ i ] = dynamic_cast< ComputeConfig * >( cfg );
  if( ! v_SolverConfigs[ i ] ) {
   delete cfg;
   throw( std::invalid_argument(
		         "BlockSolverConfig::load: not a ComputeConfig "
			 + std::to_string( i ) ) );
   }
  }
 }  // end( BlockSolverConfig::load )

/*--------------------------------------------------------------------------*/
/*-------------------- METHODS of RBlockSolverConfig -----------------------*/
/*--------------------------------------------------------------------------*/
/*------------ CONSTRUCTING AND DESTRUCTING RBlockSolverConfig -------------*/
/*--------------------------------------------------------------------------*/

RBlockSolverConfig::RBlockSolverConfig( const RBlockSolverConfig & old )
 : BlockSolverConfig( old )
{
 v_sub_Block_id = old.v_sub_Block_id;

 v_BlockSolverConfig.resize( old.v_BlockSolverConfig.size() );
 for( std::size_t i = 0 ; i < v_BlockSolverConfig.size() ; ++i )
  v_BlockSolverConfig[ i ] = old.v_BlockSolverConfig[ i ] ?
                             old.v_BlockSolverConfig[ i ]->clone() : nullptr;
 }

/*--------------------------------------------------------------------------*/

RBlockSolverConfig::RBlockSolverConfig( RBlockSolverConfig && old ) noexcept
 : BlockSolverConfig( old )
{
 v_sub_Block_id = std::move( old.v_sub_Block_id );
 v_BlockSolverConfig = std::move( old.v_BlockSolverConfig );
 }

/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::deserialize( const netCDF::NcGroup & group )
{
 if( v_BlockSolverConfig.size() )
  throw( std::logic_error( "deserializing a non-empty RBlockSolverConfig" ) );

 // deserialize the "root Block" information- - - - - - - - - - - - - - - - -

 BlockSolverConfig::deserialize( group );

 // deserialize the sub-Block information - - - - - - - - - - - - - - - - - -

 auto dim = group.getDim( "n_BlockSolverConfig" );
 if( dim.isNull() )
  return;
 size_t num_config = dim.getSize();

 // BlockSolverConfig
 v_BlockSolverConfig.resize( num_config, nullptr );

 for( size_t i = 0 ; i < v_BlockSolverConfig.size() ; ++i ) {
  auto bc = group.getGroup( "BlockSolverConfig_" + std::to_string( i ) );
  if( bc.isNull() )
   v_BlockSolverConfig[ i ] = nullptr;
  else {
   auto bsc = new_Configuration( bc );
   v_BlockSolverConfig[ i ] = dynamic_cast< BlockSolverConfig * >( bsc );
   if( ! v_BlockSolverConfig[ i ] ) {
    delete bsc;
    throw( std::invalid_argument(
	         "RBlockSolverConfig::deserialize: not a ComputeConfig" ) );
    }
   }
  }

 // sub-Block id
 v_sub_Block_id.resize( num_config );

 auto var_sub_Block_id = group.getVar( "sub-Block-id" );
 if( var_sub_Block_id.isNull() )
  for( decltype( v_sub_Block_id )::size_type i = 0 ;
       i < v_sub_Block_id.size() ; ++i )
   v_sub_Block_id[ i ] = std::to_string( i );
 else {
  if( var_sub_Block_id.getDimCount() != 1 )
   throw( std::invalid_argument(
    "RBlockSolverConfig::deserialize: sub-Block-id has wrong dimensions" ) );
  if( var_sub_Block_id.getDim( 0 ).getSize() != num_config )
   throw( std::invalid_argument(
   "RBlockSolverConfig::deserialize: wrong 1st dimension in sub-Block-id" ) );
  var_sub_Block_id.getVar( v_sub_Block_id.data() );
  }
 }  // end( RBlockSolverConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*----------------- OTHER INITIALIZATIONS RBlockSolverConfig ---------------*/
/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::get( const Block * block , bool clear )
{
 BlockSolverConfig::get( block, clear );

 if( ! block ) {
  for( auto config : v_BlockSolverConfig )
   delete config;
  v_BlockSolverConfig.clear();
  v_sub_Block_id.clear();
  return;
  }

 #ifndef NDEBUG
  if( v_BlockSolverConfig.size() != v_sub_Block_id.size() )
   throw( std::logic_error( "RBlockSolverConfig::get: inconsistent state" ) );
 #endif

 if( v_sub_Block_id.empty() ) {  // scan *all* the sub-Block- - - - - - - - -

  for( Index i = 0 ; i < block->get_number_nested_Blocks() ; ++i ) {
   auto bi = block->get_nested_Block( i );  // get the i-th sub-Block
   // if there is any significant configuration information in it
   if( auto bsc = RBlockSolverConfig::get_right_BlockSolverConfig( bi , true ,
                                                                   clear ) ) {
    std::string id = bi->name();                // define its id
    if( id.empty() ) id = std::to_string( i );  // (index if not name)
    add_BlockSolverConfig( bsc, id );          // add it
    }
   }

  return;  // all done
  }

 // only scan the existing sub-Block- - - - - - - - - - - - - - - - - - - - -

 auto it = v_BlockSolverConfig.begin();
 for( const auto & id : v_sub_Block_id ) {
  auto bi = block->get_nested_Block( id );
  if( *it )  // ... assuming they have a non-nullptr BlockSolverConfig
   ( *it )->get( bi, clear );
  else              // the BlockSolverConfig is nullptr
   if( ! clear ) {  // and a "full" BlockSolverConfig is required
    auto newBSC = new BlockSolverConfig( bi );   // get it
    if( ! newBSC->empty() )                      // if it is significant
     *it = newBSC;                               // record it
    else                                         // otherwise
     delete newBSC;                              // discard it
    }

  ++it;
  }
 }  // end( RBlockSolverConfig::get )

/*--------------------------------------------------------------------------*/
/*-------- METHODS DESCRIBING THE BEHAVIOR OF THE RBlockSolverConfig -------*/
/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::apply( Block * block ) const
{
 if( ! block )
  return;

 // set the configurations for the Solver of the "root" Block - - - - - - - -

 BlockSolverConfig::apply( block );

 // set the configurations for the sub-Block- - - - - - - - - - - - - - - - -

 #ifndef NDEBUG
  if( v_BlockSolverConfig.size() != v_sub_Block_id.size() )
   throw( std::logic_error( "RBlockSolverConfig::apply: inconsistent state"
			    ) );
 #endif

 auto it = v_BlockSolverConfig.begin();
 for( const auto & id : v_sub_Block_id ) {
  if( auto sub_Block = block->get_nested_Block( id ) ) {
   if( *it )
    ( *it )->apply( sub_Block );
   else
    if( ! f_diff )
     sub_Block->unregister_Solvers( true );
   }
  ++it;
  }
 }  // end( RBlockSolverConfig::apply )

/*--------------------------------------------------------------------------*/
/*------ METHODS FOR LOADING, PRINTING & SAVING THE RBlockSolverConfig -----*/
/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::serialize( netCDF::NcGroup & group ) const
{
 BlockSolverConfig::serialize( group );

 // BlockSolverConfig for sub-Block

 auto n_BlockSolverConfig =
  group.addDim( "n_BlockSolverConfig", v_BlockSolverConfig.size() );

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

 auto sub_Block_id_var = group.addVar( "sub-Block-id" , netCDF::NcString(),
                                       { sub_Block_id_dim } );

 sub_Block_id_var.putVar( v_sub_Block_id.data() );

 }  // end( RBlockSolverConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::print( std::ostream & output ) const
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

void RBlockSolverConfig::load( std::istream & input )
{
 BlockSolverConfig::load( input );

 static const std::string sre(
			    "RBlockSolverConfig::load: stream read error" );
 if( advance( input ) ) {
  v_sub_Block_id.clear();
  for( auto & el : v_BlockSolverConfig )
   delete el;
  v_BlockSolverConfig.clear();
  return;
  }

 checkfail( input , sre );

 // BlockSolverConfig for sub-Block

 int k;
 input >> k;
 checkfail( input , sre );

 const bool id_is_provided = ( k < 0 );
 k = std::abs( k );
 v_sub_Block_id.resize( k );
 v_BlockSolverConfig.resize( k , nullptr );

 for( int i = 0 ; i < k ; ++i ) {
  if( id_is_provided ) {
   input >> eatcomments >> v_sub_Block_id[ i ];
   checkfail( input , sre );
   }
  else
   v_sub_Block_id[ i ] = std::to_string( i );

  auto cfg = Configuration::deserialize( input );
  v_BlockSolverConfig[ i ] = dynamic_cast< BlockSolverConfig * >( cfg );
  if( ! v_BlockSolverConfig[ i ] ) {
   delete cfg;
   throw( std::invalid_argument(
        "RBlockSolverConfig::load: invalid BlockSolverConfig for sub-Block "
	+ v_sub_Block_id[ i ] ) );
   }
  }
 }  // end( RBlockSolverConfig::load )

/*--------------------------------------------------------------------------*/
/*------------------ End File BlockSolverConfig.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
