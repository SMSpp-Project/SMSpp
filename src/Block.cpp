/*--------------------------------------------------------------------------*/
/*---------------------------- File Block.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the Block class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
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

#include "Block.h"
#include "Constraint.h"
#include "Objective.h"
#include "Variable.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------ STATIC MEMBERS OF Observer ----------------------*/
/*--------------------------------------------------------------------------*/

std::atomic_flag Observer::f_ch_lock;

Observer::ChnlName Observer::f_next_chnl = 1;

std::set< Observer::ChnlName > Observer::v_free_chnl;

/*--------------------------------------------------------------------------*/
/*------------------------- STATIC MEMBERS OF Block ------------------------*/
/*--------------------------------------------------------------------------*/

// register BlockConfig to the Configuration factory

SMSpp_insert_in_factory_cpp_0( BlockConfig );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

std::string Block::f_prefix;  // the filename prefix

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/
// Auxiliary functions for Block.cpp not exported as methods of the class


/*--------------------------------------------------------------------------*/
/*-------------------------- METHODS of Block ------------------------------*/
/*--------------------------------------------------------------------------*/

Block * Block::deserialize( const std::string & filename , Block * father )
{
 try {
  if( ( filename.size() > 4 ) &&
      ( filename.substr( filename.size() - 4 , 4 ) == ".txt" ) ) {
   std::ifstream f( f_prefix.empty() ? filename : f_prefix + filename ,
		    std::fstream::in );
   if( ! f.is_open() ) {
    std::cerr << "Error: cannot open text file " << f_prefix + filename
	      << std::endl;
    return( nullptr );
    }
   return( Block::deserialize( f , father ) );
   }
  else {
   int idx = 0;
   std::string fn = f_prefix + filename;
   if( fn.back() == ']' ) {
    auto pos = fn.find_last_of( '[' );
    if( pos != std::string::npos ) {
     try {
      idx = std::stoi( fn.substr( pos + 1 ) );
      fn.erase( pos );
      }
     catch( ... ) { idx = 0; }
     }
    }
   netCDF::NcFile f( fn.c_str() , netCDF::NcFile::read );
   return( Block::deserialize( f , idx , father ) );
   }
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

 }  // end( Block::deserialize( const std::string ) )

/*--------------------------------------------------------------------------*/

Block * Block::deserialize( const netCDF::NcFile & f , unsigned int idx ,
			    Block * father )
{
 try {
  netCDF::NcGroupAtt gtype = f.getAtt( "SMS++_file_type" );
  if( gtype.isNull() )
   return( nullptr );

  int type;
  gtype.getValues( & type );

  if( ( type != eProbFile ) && ( type != eBlockFile ) )
   return( nullptr );

  netCDF::NcGroup bg;
  if( type == eProbFile ) {
   netCDF::NcGroup dg = f.getGroup( "Prob_" + std::to_string( idx ) );
   if( dg.isNull() )
    return( nullptr );

   bg = dg.getGroup( "Block" );
   }
  else
   bg = f.getGroup( "Block_" + std::to_string( idx ) );

  return( new_Block( bg , father ) );
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

 }  // end( Block::deserialize( netCDF::NcFile ) )

/*--------------------------------------------------------------------------*/

Block * Block::new_Block( const netCDF::NcGroup & group , Block * father )
{
 try {
  if( group.isNull() )
   return( nullptr );

  std::string tmp;
  auto gtype = group.getAtt( "type" );
  if( gtype.isNull() ) {
   auto gfile = group.getAtt( "filename" );
   if( gfile.isNull() )
    return( nullptr );

   gfile.getValues( tmp );

   return( deserialize( tmp , father ) );
   }

  gtype.getValues( tmp );
  auto result = new_Block( tmp , father );
  result->deserialize( group );
  return( result );
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

 }  // end( Block::new_Block( netCDF::NcGroup ) )

/*--------------------------------------------------------------------------*/

Block * Block::deserialize( std::istream & input , Block * father )
{
 input >> eatcomments;
 if( input.eof() )
  return( nullptr );
 
 static std::string sre( "Block::deserialize: stream read error" );

 if( input.fail() )
  throw( std::invalid_argument( sre ) );

 std::string tmp;
 if( input.peek() == input.widen( '*' ) ) {
  input.get();

  if( input.eof() )
   return( nullptr );
  
  if( input.fail() )
   throw( std::invalid_argument( sre ) );

  if( std::isspace( input.peek() ) )
   return( nullptr );
 
  input >> tmp;
  if( input.fail() )
    throw( std::invalid_argument( sre ) );

  return( Block::deserialize( tmp , father ) );
  }
 else {
  input >> tmp;
  if( input.fail() )
    throw( std::invalid_argument( sre ) );

  auto block = Block::new_Block( tmp , father );
  // note: block surely is not nullptr, as new_Block() throws exception
  // on failure

  char frmt = 0;
  input >> eatcomments;
  if( input.fail() )
    throw( std::invalid_argument( sre ) );

  if( input.peek() == input.widen( '[' ) ) {
   input >> tmp;
   if( tmp.size() > 1 )
    frmt = tmp[ 1 ];
   input >> eatcomments;
   if( input.fail() )
    throw( std::invalid_argument( sre ) );
   }

  if( input.peek() == input.widen( '*' ) ) {
   input.get();

   if( input.eof() || input.fail() )
    throw( std::invalid_argument( sre ) );

   input >> eatcomments;
   if( input.eof() || input.fail() )
    throw( std::invalid_argument( sre ) );

   input >> tmp;
   if( input.eof() || input.fail() )
    throw( std::invalid_argument( sre ) );

   block->load( tmp , frmt );
   }
  else
   block->load( input , frmt );

  return( block );
  }
 }  // end( Block::deserialize( std::istream ) )

/*--------------------------------------------------------------------------*/
/*----------------- Methods for reading the data of the Block --------------*/
/*--------------------------------------------------------------------------*/

int Block::get_objective_sense( void ) const
{
 return( f_Objective ? f_Objective->get_sense() : Objective::eUndef );
 }

/*--------------------------------------------------------------------------*/

void Block::set_objective( Objective * newOF , c_ModParam issueMod )
{
 f_Objective = newOF;
 newOF->set_Block( this );

 if( issue_mod( issueMod ) )
  add_Modification( std::make_shared< BlockMod >(
   this , Observer::par2concern( issueMod ) ) );
}

/*--------------------------------------------------------------------------*/
/*------------- METHODS DESCRIBING THE BEHAVIOR OF AN Observer -------------*/
/*--------------------------------------------------------------------------*/

void Block::anyone_there( bool isthere )
{
 if( isthere ) {              // somebody is listening to father now
  if( f_at )                  // it was already so
   return;                    // nothing changes
  f_at = true;                // now I know it
  if( ! v_Solver.empty() )    // but my sons don't care because there
   return;                    // was already someone listening to me
  for( auto el : v_Block )    // now someone is listening to all my sons
   el->anyone_there( true );
  }
 else {                       // nobody is listening to father now
  if( ! f_at )                // it was already so
   return;                    // nothing changes
  f_at = false;               // now I know it
  if( ! v_Solver.empty() )    // but my sons don't care because there
   return;                    // is still someone listening to me
  for( auto el : v_Block )    // now no one is listening to all my sons
   el->anyone_there( false );
  }
 }  // end( Block::anyone_there )

/*--------------------------------------------------------------------------*/

void Block::add_Modification( sp_Mod mod , ChnlName chnl )
{
 // Modification should generally not even be issued if ! anyone_there(), but
 // there is an exception: "abstract" Modification under the default eModBlck
 // mode are still issued in order to keep the "abstract" representation in
 // synch with the "phisical" one. this is supposed to happen in the
 // :Block::add_Modification(), that then should call this method. without
 // the following check this would result in the Modification being uslessly
 // passed up to all the chain of ancestors of the Block, the check avoids it
 if( ! anyone_there() )
  return;

 if( ! chnl )                           // the default channel
  chnl = f_channel;                     // possibly silently hijack it

 if( chnl ) {                           // a channel is specified
  if( ! v_GroupMod.empty() ) {
   // check if it is one of the "local channels"
   auto GMit = std::find_if( v_GroupMod.begin() , v_GroupMod.end() ,
			     [ chnl ] ( auto & a ) -> bool {
			      return( a.first == chnl );
			      } );

   if( GMit != v_GroupMod.end() ) {     // if so
    GMit->second->add( mod );           // add it to the GroupModification
    return;                             // all done
    }
   }

  // it is an error if it is not a "local" channel and there is no father,
  // since it cannot be a channel of the father (any ancestor) too
  if( ! f_Block )
   throw( std::invalid_argument( "wrong channel name" ) );
  }

 if( f_Block )                                // if there is a father
  f_Block->add_Modification( mod , chnl );    // pass it above (on chnl)

 for( Solver * slv : v_Solver )               // if there is any Solver
  slv->add_Modification( mod );               // also pass it to them

 }  // end( Block::add_Modification )

/*--------------------------------------------------------------------------*/

Observer::ChnlName Block::open_channel( ChnlName chnl ,
					GroupModification * gmpmod )
{
 if( ! gmpmod )                    // if a GroupModification is not provided
  gmpmod = new GroupModification;  // create one

 if( ! chnl ) {  // opening a new channel
  chnl = Observer::new_channel_name();
  v_GroupMod.push_back( std::pair( chnl , gmpmod ) );
  return( chnl );
  }

 // nesting into an existing channel
 if( ! v_GroupMod.empty() ) {      // if there are "local channels"
  auto GMit = std::find_if( v_GroupMod.begin() , v_GroupMod.end() ,
			    [ chnl ]( auto & a ) -> bool {
			     return( a.first == chnl );
			     } );

  if( GMit != v_GroupMod.end() ) {  // if it's one of them
   // set the father of the GroupModification to the current channel
   gmpmod->set_father( GMit->second );

   // add the new GroupModification to the current channel
   GMit->second->add( std::shared_ptr< GroupModification >( gmpmod ) );

   // the current channel becomes the new GroupModification
   GMit->second = gmpmod;

   return( chnl );
   }
  }

 // it is an error if it is not a "local" channel and there is no father,
 // since it cannot be a channel of the father (any ancestor) too
 if( ! f_Block )
  throw( std::invalid_argument( "open_channel: " + std::to_string( chnl ) +
				" not found" ) );

 f_Block->open_channel( chnl , gmpmod );  // try to find it in the father

 return( chnl );  // unless exception is thrown, it has been found

 }  // end( Block::open_channel )

/*--------------------------------------------------------------------------*/

void Block::close_channel( ChnlName chnl , bool force )
{
 if( ! chnl )
  throw( std::invalid_argument( "cannot close default channel" ) );

 if( ! v_GroupMod.empty() ) {  // if there are "local channels"
  auto GMit = std::find_if( v_GroupMod.begin() , v_GroupMod.end() ,
			    [ chnl ] ( auto & a ) -> bool {
			     return( a.first == chnl );
			     } );

  if( GMit != v_GroupMod.end() ) {  // if chnl is one of them

   auto father = GMit->second->father();
   if( ( ! father ) || force ) {
    // if either the channel is in "root mode", or closure is forced
    if( chnl == f_channel )  // if it was the default channel
     f_channel = 0;          // reset it

    // if concerns_Block() of the current GroupModification is true, ensure
    // that the concerns_Block() of is also true up until the top
    if( GMit->second->concerns_Block() )
     while( father ) {
      father->concerns_Block( true );
      father = father->father();
      }

    // finally pass the GroupModification to the Block, on the (possibly
    // freshly reset) default channel
    Block::add_Modification( std::shared_ptr< GroupModification
			                      >( GMit->second ) );
    Observer::release_channel_name( chnl );  // give back the channel name
    v_GroupMod.erase( GMit );                // delete the local channel
    }
   else {
    // the channel is not in "root mode" and closure is not forced, just
    // un-nest the GroupModification by one level
    // if concerns_Block() of the current GroupModification is true, ensure
    // that the concerns_Block() of father is also true
    if( GMit->second->concerns_Block() )
     father->concerns_Block( true );

    GMit->second = father; // move back the channel to being the father
    }

   return;
   }
  }

 // it is an error if it is not a "local" channel and there is no father,
 // since it cannot be a channel of the father (any ancestor), too
 if( ! f_Block )
  throw( std::invalid_argument( "close_channel: " + std::to_string( chnl ) +
				" not found" ) );

 // pass the message up to the father
 f_Block->close_channel( chnl , force );

 }  // end( Block::close_channel )

/*--------------------------------------------------------------------------*/
/*------------ METHODS FOR LOADING, PRINTING & SAVING THE Block ------------*/
/*--------------------------------------------------------------------------*/

void Block::set_BlockConfig( BlockConfig * newBC, bool deleteold )
{
 if( f_BlockConfig == newBC )
  return;

 if( ! newBC ) {
  if( deleteold )
   delete f_BlockConfig;
  f_BlockConfig = nullptr;
  return;
  }

 if( newBC->is_diff() ) {  // "differential mode"
  if( ! f_BlockConfig )
   f_BlockConfig = newBC;
  else {
   newBC->move_non_null_configuration_to( f_BlockConfig, deleteold );
   delete newBC;
   }
  }
 else {                    // "setting mode"
  if( deleteold )
   delete f_BlockConfig;
  f_BlockConfig = newBC;
  }
 }  // end( set_BlockConfig )

/*--------------------------------------------------------------------------*/

void Block::print( std::ostream & output , char vlvl ) const
{
 // the base Block class cannot save itself completely, so 'C' is ignored
 // (rather than raising an exception) for the case that a derived class
 // decides to call the base class method
 if( vlvl == 'C' )
  return;

 output << std::endl << classname() << " with: ";
 output << std::endl << v_s_Variable.size() << " groups of static Variable, "
        << v_d_Variable.size() << " groups of dynamic Variable, "
        << std::endl << v_s_Constraint.size()
	<< " groups of static Constraint, "
        << v_d_Constraint.size() << " groups of dynamic Constraint, "
        << std::endl << v_Block.size() << " nested Blocks, and "
        << v_Solver.size() << " registered Solvers"
        << std::endl;

 if( ! vlvl ) {
  /*
  // the static Constraints of the Block- - - - - - - - - - - - - - - - - - -
  output << "Static Constraints:" << std::endl;
  for( unsigned int i = 0 ; i < v_s_Constraint.size() ; ++i ) {
   output << i;
   if( ! v_s_Constraint_names[ i ].empty() )
    output << " (" << v_s_Constraint_names[ i ] << "): ";
   else
    output << ": ";

   un_any_static_constraint( v_s_Constraint[ i ] , { output << *var; } );
   output << std::endl;
   }

  // the static Variables of the Block- - - - - - - - - - - - - - - - - - - -
  output << "Static Variables:" << std::endl;
  for( unsigned int i = 0 ; i < v_s_Variable.size() ; ++i ) {
   output << i;
   if( ! v_s_Variable_names[ i ].empty() )
    output << " (" << v_s_Variable_names[ i ] << "): ";
   else
    output << ": ";

   un_any_static_Variable( v_s_Variable[ i ] , { output << *var; } );
   output << std::endl;
   }

  // the dynamic Constraints of the Block- - - - - - - - - - - - - - - - - -
  output << "Dynamic Constraints:" << std::endl;
  for( unsigned int i = 0 ; i < v_d_Constraint.size() ; ++i ) {
   output << i;
   if( ! v_d_Constraint_names[ i ].empty() )
    output << " (" << v_d_Constraint_names[ i ] << "): ";
   else
    output << ": ";

   un_any_static_Constraint( v_d_Constraint[ i ] , { output << *var; } );
   output << std::endl;
   }

  // the dynamic Variables of the Block - - - - - - - - - - - - - - - - - - -
  output << "Dynamic Variables:" << std::endl;
  for( unsigned int i = 0 ; i < v_d_Variable.size() ; ++i ) {
   output << i;
   if( ! v_d_Variable_names[ i ].empty() )
    output << " (" << v_d_Variable_names[ i ] << "): ";
   else
    output << ": ";

   un_any_static_Variable( v_d_Variable[ i ] , { output << *var; } );
   output << std::endl;
   }
  */

  // the inner Blocks - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  output << std::endl << "Nested Blocks:" << std::endl;
  for( auto blk : v_Block )
   blk->print( output , vlvl );
  }
 }  // end( Block::print )

/*--------------------------------------------------------------------------*/

Block::BlockFactoryMap & Block::f_factory( void )
{
 static BlockFactoryMap s_factory;
 return( s_factory );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

void Block::remove_constraint_from_variables( Constraint * constraint )
{
 for( Constraint::Index i = 0 ; i < constraint->get_num_active_var() ; )
  constraint->get_active_var( i++ )->remove_active( constraint );
 }

/*--------------------------------------------------------------------------*/

void Block::remove_variable_from_stuff( Variable * const variable ,
                                        int issueindMod )
{
 for( Variable::Index i = 0 ; i < variable->get_num_active() ; ) {
  auto si = variable->get_active( i++ );
  auto ivar = si->is_active( variable );
  if( ivar >= si->get_num_active_var() )
   throw( std::logic_error( "inconsistency between active lists" ) );

  si->remove_variable( ivar , issueindMod );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------- METHODS of BlockConfig -------------------------*/
/*--------------------------------------------------------------------------*/

BlockConfig::BlockConfig( const BlockConfig & old ) : BlockConfig()
{
 f_diff = old.f_diff;
 clone_sub_Configuration( old );
 }

/*--------------------------------------------------------------------------*/

BlockConfig::BlockConfig( BlockConfig && old )
{
 f_diff = old.f_diff;
 f_static_constraints_Configuration = old.f_static_constraints_Configuration;
 old.f_static_constraints_Configuration = nullptr;

 f_dynamic_constraints_Configuration =
  old.f_dynamic_constraints_Configuration;
 old.f_dynamic_constraints_Configuration = nullptr;

 f_static_variables_Configuration = old.f_static_variables_Configuration;
 old.f_static_variables_Configuration = nullptr;

 f_dynamic_variables_Configuration = old.f_dynamic_variables_Configuration;
 old.f_dynamic_variables_Configuration = nullptr;

 f_objective_Configuration = old.f_objective_Configuration;
 old.f_objective_Configuration = nullptr;

 f_is_feasible_Configuration = old.f_is_feasible_Configuration;
 old.f_is_feasible_Configuration = nullptr;

 f_is_optimal_Configuration = old.f_is_optimal_Configuration;
 old.f_is_optimal_Configuration = nullptr;

 f_solution_Configuration = old.f_solution_Configuration;
 old.f_solution_Configuration = nullptr;

 f_extra_Configuration = old.f_extra_Configuration;
 old.f_extra_Configuration = nullptr;
 }

/*--------------------------------------------------------------------------*/

void BlockConfig::get( Block * block )
{
 // clear this BlockConfig
 delete_sub_Configuration();

 if( ! block )
  return;

 if( auto block_config = block->get_BlockConfig() ) {
  set_diff( block_config->is_diff() );
  // clone the Configuration from Block
  clone_sub_Configuration( *block_config );
  }
 }  // end( BlockConfig::get )

/*--------------------------------------------------------------------------*/

void BlockConfig::serialize( netCDF::NcFile & f , int type ) const
{
 if( type == eConfigFile ) {
  Configuration::serialize( f , type );
  return;
  }

 auto cg = ( f.addGroup( "Config_" + std::to_string( f.getGroupCount() )
 ) ).addGroup( "BlockConfig" );
 serialize( cg );

 }  // end( BlockConfig::serialize( file ) )

/*--------------------------------------------------------------------------*/

void BlockConfig::print( std::ostream & output ) const
{
 output << private_name();
 if( f_diff ) output << "[diff]";
 output << ": " << std::endl;
 if( f_static_constraints_Configuration )
  output << *f_static_constraints_Configuration;
 if( f_dynamic_constraints_Configuration )
  output << *f_dynamic_constraints_Configuration;
 if( f_static_variables_Configuration )
  output << *f_static_variables_Configuration;
 if( f_dynamic_variables_Configuration )
  output << *f_dynamic_variables_Configuration;
 if( f_objective_Configuration )
  output << *f_objective_Configuration;
 if( f_is_feasible_Configuration )
  output << *f_is_feasible_Configuration;
 if( f_is_optimal_Configuration )
  output << *f_is_optimal_Configuration;
 if( f_solution_Configuration )
  output << *f_solution_Configuration;
 if( f_extra_Configuration )
  output << *f_extra_Configuration;
 output << std::endl;

 }  // end( BlockConfig::print )

/*--------------------------------------------------------------------------*/

void BlockConfig::load( std::istream & input )
{
 if( ! empty() )
  clear();

 input >> eatcomments;
 if( input.eof() )
  return;

 input >> f_diff;
 if( input.fail() )
  throw( std::invalid_argument( "BlockConfig::load: stream read error" ) );

 f_static_constraints_Configuration = Configuration::deserialize( input );
 input >> eatcomments;
 if( input.eof() )
  return;

 f_dynamic_constraints_Configuration = Configuration::deserialize( input );
 input >> eatcomments;
 if( input.eof() )
  return;

 f_static_variables_Configuration = Configuration::deserialize( input );
 input >> eatcomments;
 if( input.eof() )
  return;

 f_dynamic_variables_Configuration = Configuration::deserialize( input );
 input >> eatcomments;
 if( input.eof() )
  return;

 f_objective_Configuration = Configuration::deserialize( input );
 input >> eatcomments;
 if( input.eof() )
  return;

 f_is_feasible_Configuration = Configuration::deserialize( input );
 input >> eatcomments;
 if( input.eof() )
  return;

 f_is_optimal_Configuration = Configuration::deserialize( input );
 input >> eatcomments;
 if( input.eof() )
  return;

 f_solution_Configuration = Configuration::deserialize( input );
 input >> eatcomments;
 if( input.eof() )
  return;

 f_extra_Configuration = Configuration::deserialize( input );

 }  // end( BlockConfig::load )

/*--------------------------------------------------------------------------*/

void BlockConfig::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );

 group.putAtt( "diff", netCDF::NcInt() , int( f_diff ) );

 if( f_static_constraints_Configuration ) {
  auto cg = group.addGroup( "static_constraints" );
  f_static_constraints_Configuration->serialize( cg );
  }

 if( f_dynamic_constraints_Configuration ) {
  auto cg = group.addGroup( "dynamic_constraints" );
  f_dynamic_constraints_Configuration->serialize( cg );
  }

 if( f_static_variables_Configuration ) {
  auto cg = group.addGroup( "static_variables" );
  f_static_variables_Configuration->serialize( cg );
  }

 if( f_dynamic_variables_Configuration ) {
  auto cg = group.addGroup( "dynamic_variables" );
  f_dynamic_variables_Configuration->serialize( cg );
  }

 if( f_objective_Configuration ) {
  auto cg = group.addGroup( "objective" );
  f_objective_Configuration->serialize( cg );
  }

 if( f_is_feasible_Configuration ) {
  auto cg = group.addGroup( "is_feasible" );
  f_is_feasible_Configuration->serialize( cg );
  }

 if( f_is_optimal_Configuration ) {
  auto cg = group.addGroup( "is_optimal" );
  f_is_optimal_Configuration->serialize( cg );
  }

 if( f_solution_Configuration ) {
  auto cg = group.addGroup( "solution" );
  f_solution_Configuration->serialize( cg );
  }

 if( f_extra_Configuration ) {
  auto cg = group.addGroup( "extra" );
  f_extra_Configuration->serialize( cg );
  }
 }  // end( BlockConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void BlockConfig::deserialize( const netCDF::NcGroup & group )
{
 if( ! empty() )
  throw( std::logic_error( "deserializing a non-empty BlockConfig" ) );

 netCDF::NcGroupAtt diff = group.getAtt( "diff" );
 if( diff.isNull() )
  f_diff = true;
 else {
  int diffint;
  diff.getValues( &diffint );
  f_diff = ( diffint > 0 );
 }

 auto cg = group.getGroup( "static_constraints" );
 f_static_constraints_Configuration = new_Configuration( cg );

 cg = group.getGroup( "dynamic_constraints" );
 f_dynamic_constraints_Configuration = new_Configuration( cg );

 cg = group.getGroup( "static_variables" );
 f_static_variables_Configuration = new_Configuration( cg );

 cg = group.getGroup( "dynamic_variables" );
 f_dynamic_variables_Configuration = new_Configuration( cg );

 cg = group.getGroup( "objective" );
 f_objective_Configuration = new_Configuration( cg );

 cg = group.getGroup( "is_feasible" );
 f_is_feasible_Configuration = new_Configuration( cg );

 cg = group.getGroup( "is_optimal" );
 f_is_optimal_Configuration = new_Configuration( cg );

 cg = group.getGroup( "solution" );
 f_solution_Configuration = new_Configuration( cg );

 cg = group.getGroup( "extra" );
 f_extra_Configuration = new_Configuration( cg );

 }  // end( BlockConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Block.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
