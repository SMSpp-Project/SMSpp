/*--------------------------------------------------------------------------*/
/*---------------------------- File Block.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the Block class.
 *
 * \version 0.10
 *
 * \date 29 - 07 - 2020
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Department of Applied Mathematics \n
 *         State University of Campinas, Brazil \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
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

#include "Block.h"
#include "Constraint.h"
#include "Objective.h"
#include "Variable.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;
using namespace std;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register BlockConfig to the Configuration factory

SMSpp_insert_in_factory_cpp_0( BlockConfig );

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/
// Auxiliary functions for Block.cpp not exported as methods of the class


/*--------------------------------------------------------------------------*/
/*-------------------------- METHODS of Block ------------------------------*/
/*--------------------------------------------------------------------------*/
/*----------------- Methods for reading the data of the Block --------------*/
/*--------------------------------------------------------------------------*/

int Block::get_objective_sense( void ) const
{
 return( f_Objective ? f_Objective->get_sense() : Objective::eMin );
 }

/*--------------------------------------------------------------------------*/

void Block::set_objective( Objective * newOF , c_ModParam issueMod )
{
 f_Objective = newOF;
 newOF->set_Block( this );

 if( issue_mod( issueMod ) )
  add_Modification(
     std::make_shared<BlockMod>( this , Observer::par2concern( issueMod ) ) );
 }
 
/*--------------------------------------------------------------------------*/
/*------------- METHODS DESCRIBING THE BEHAVIOR OF AN Observer -------------*/
/*--------------------------------------------------------------------------*/

void Block::anyone_there( bool isthere ) {
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
 if( ! chnl )                           // the default channel
  chnl = f_channel;                     // possibly silently hijack it

 if( ! chnl ) {                         // (still) the default channel
  if( f_Block )                         // if there is a father
   f_Block->add_Modification( mod );    // pass it above

  for( Solver *slv : v_Solver )         // if there is any Solver
   slv->add_Modification( mod );        // pass it to them

  return;                               // all done
  }

 if( ( chnl > v_current_GroupMod.size() ) ||
     ( v_current_GroupMod[ chnl - 1 ] == nullptr ) )
  throw( std::invalid_argument( "wrong channel name" ) );

 // append it to the sub_Modifications of the appropriate GroupModification
 v_current_GroupMod[ chnl - 1 ]->add( mod );
  
 }  // end( Block::add_Modification )

/*--------------------------------------------------------------------------*/

Observer::ChnlName Block::open_channel( GroupModification * gmpmod )
{
 if( ! gmpmod )
  gmpmod = new GroupModification();

 auto it = std::find( v_current_GroupMod.begin() ,
		      v_current_GroupMod.end() , nullptr );
 ChnlName chnl;
 if( it == v_current_GroupMod.end() ) {
  v_current_GroupMod.push_back( gmpmod );
  chnl = v_current_GroupMod.size();
  }
 else {
  *it = gmpmod;
  chnl = std::distance( it , v_current_GroupMod.begin() ) + 1;
  }

 return( chnl );

 }  // end( Block::open_channel )

/*--------------------------------------------------------------------------*/

void Block::nest_channel( c_ChnlName chnl , GroupModification * gmpmod )
{
 if( ( ! chnl ) || ( chnl > v_current_GroupMod.size() ) ||
     ( v_current_GroupMod[ chnl - 1 ] == nullptr ) )
  throw( std::invalid_argument( "wrong channel name" ) );

 // if a GroupModification is not provided, create one
 if( ! gmpmod )
  gmpmod = new GroupModification();

 // set the father of the GroupModification to the current channel
 gmpmod->set_father( v_current_GroupMod[ chnl - 1 ] );

 // add the new GroupModification to the current channel
 v_current_GroupMod[ chnl - 1 ]->add( std::shared_ptr< GroupModification >(
								   gmpmod ) );
 // the current channel becomes the new GroupModification
 v_current_GroupMod[ chnl - 1 ] = gmpmod;

 }  // end( Block::nest_channel )

/*--------------------------------------------------------------------------*/

void Block::un_nest_channel( c_ChnlName chnl )
{
 if( ! chnl )
  throw( std::invalid_argument( "cannot un-nest default channel" ) );

 if( ( chnl > v_current_GroupMod.size() ) ||
     ( v_current_GroupMod[ chnl - 1 ] == nullptr ) )
  throw( std::invalid_argument( "wrong channel name" ) );

 // the father of the current GroupModification
 auto father = v_current_GroupMod[ chnl - 1 ]->father();
 if( ! father )
  throw( std::invalid_argument( "channel is at root level" ) );

 // if concerns_Block() of the current GroupModification is true, ensure
 // that the concerns_Block() of father is also true
 if( v_current_GroupMod[ chnl - 1 ]->concerns_Block() )
  father->concerns_Block( true );

 // move back the channel to being the father
 v_current_GroupMod[ chnl - 1 ] = father;

 }  // end( Block::un_nest_channel )

/*--------------------------------------------------------------------------*/

void Block::close_channel( ChnlName chnl )
{
 if( ! chnl )
  throw( std::invalid_argument( "cannot close default channel" ) );

 if( ( chnl > v_current_GroupMod.size() ) ||
     ( v_current_GroupMod[ chnl - 1 ] == nullptr ) )
  throw( std::invalid_argument( "wrong channel name" ) );

 Block::add_Modification( std::shared_ptr< GroupModification >(
				          v_current_GroupMod[ chnl - 1 ] ) );
 // there is no longer an "open" chnl GroupModification
 v_current_GroupMod[ chnl - 1 ] = nullptr;

 if( chnl == f_channel )  // if it was the default channel
  f_channel = 0;          // reset it

 if( chnl == v_current_GroupMod.size() ) {
  ChnlName i = chnl - 1;
  while( ( i > 0 ) && ( ! v_current_GroupMod[ i - 1 ] ) )
   i--;

  if( i )
   v_current_GroupMod.resize( i );
  else
   v_current_GroupMod.clear();
  }
 }  // end( Block::close_channel )

/*--------------------------------------------------------------------------*/

void Block::set_default_channel( ChnlName chnl )
{
 if( ( ! chnl ) || ( chnl > v_current_GroupMod.size() ) ||
     ( v_current_GroupMod[ chnl - 1 ] == nullptr ) )
  throw( std::invalid_argument( "wrong channel name" ) );

 f_channel = chnl;
 }

/*--------------------------------------------------------------------------*/
/*------------ METHODS FOR LOADING, PRINTING & SAVING THE Block ------------*/
/*--------------------------------------------------------------------------*/

void Block::set_BlockConfig( BlockConfig * newBC , bool deleteold )
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
   newBC->move_non_null_configuration_to( f_BlockConfig , deleteold );
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

void Block::print( ostream &output ) const
{
 output << endl << "Block with: ";
 output << endl << v_s_Variable.size() << " types of static Variables, "
                << v_d_Variable.size() << " types of dynamic Variables, "
        << endl << v_s_Constraint.size() << " types of static Constraints, "
                << v_d_Constraint.size() << " types of dynamic Constraints, "
        << endl << v_Block.size() << " nested Blocks, and "
                << v_Solver.size() << " registered Solvers"
        << endl;

 if( verbosity_lvl == Block::medium || verbosity_lvl == Block::high ) {
  /*
  // the static Constraints of the Block- - - - - - - - - - - - - - - - - - -
  output << "Static Constraints:" << endl;
  for( unsigned int i = 0 ; i < v_s_Constraint.size() ; ++i ) {
   output << i;
   if( ! v_s_Constraint_names[ i ].empty() )
    output << " (" << v_s_Constraint_names[ i ] << "): ";
   else
    output << ": ";

   un_any_static_constraint( v_s_Constraint[ i ] , { output << *var; } );
   output << endl;
   }

  // the static Variables of the Block- - - - - - - - - - - - - - - - - - - -
  output << "Static Variables:" << endl;
  for( unsigned int i = 0 ; i < v_s_Variable.size() ; ++i ) {
   output << i;
   if( ! v_s_Variable_names[ i ].empty() )
    output << " (" << v_s_Variable_names[ i ] << "): ";
   else
    output << ": ";

   un_any_static_Variable( v_s_Variable[ i ] , { output << *var; } );
   output << endl;
   }

  // the dynamic Constraints of the Block- - - - - - - - - - - - - - - - - -
  output << "Dynamic Constraints:" << endl;
  for( unsigned int i = 0 ; i < v_d_Constraint.size() ; ++i ) {
   output << i;
   if( ! v_d_Constraint_names[ i ].empty() )
    output << " (" << v_d_Constraint_names[ i ] << "): ";
   else
    output << ": ";

   un_any_static_Constraint( v_d_Constraint[ i ] , { output << *var; } );
   output << endl;
   }

  // the dynamic Variables of the Block - - - - - - - - - - - - - - - - - - -
  output << "Dynamic Variables:" << endl;
  for( unsigned int i = 0 ; i < v_d_Variable.size() ; ++i ) {
   output << i;
   if( ! v_d_Variable_names[ i ].empty() )
    output << " (" << v_d_Variable_names[ i ] << "): ";
   else
    output << ": ";

   un_any_static_Variable( v_d_Variable[ i ] , { output << *var; } );
   output << endl;
   }
  */

  // the inner Blocks - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  output  << endl << "Nested Blocks:" << endl;
  for( p_Block blk : v_Block )
   output << *blk;
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
 /*!!
 for( auto & var : *constraint )
  var.remove_active( constraint );
  !!*/
 }

/*--------------------------------------------------------------------------*/

void Block::remove_variable_from_stuff( Variable * const variable ,
					const int issueindMod  )
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

BlockConfig::BlockConfig( const BlockConfig & old )
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

BlockConfig * BlockConfig::deserialize( netCDF::NcFile & f ,
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

   cg = dg.getGroup( "BlockConfig" );
   }
  else
   cg = f.getGroup( "Config_" + std::to_string( idx ) );

  auto result = new_Configuration( cg );
  auto bcresult = dynamic_cast< BlockConfig * >( result );
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

 } // end( BlockConfig::deserialize( file ) ) 

/*--------------------------------------------------------------------------*/

void BlockConfig::serialize( netCDF::NcFile & f , const int type ) const
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

void BlockConfig::print( std::ostream &output ) const
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

void BlockConfig::load( std::istream & input ) {
 input >> eatcomments >> f_diff;

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) ) {
  f_static_constraints_Configuration = nullptr;
  input.ignore( std::numeric_limits< std::streamsize >::max(),
                input.widen( '\n' ) );
 } else {
  std::string cname;
  input >> cname;
  f_static_constraints_Configuration =
   Configuration::new_Configuration( cname );
  input >> *f_static_constraints_Configuration;
 }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) ) {
  f_dynamic_constraints_Configuration = nullptr;
  input.ignore( std::numeric_limits< std::streamsize >::max(),
                input.widen( '\n' ) );
 } else {
  std::string cname;
  input >> cname;
  f_dynamic_constraints_Configuration =
   Configuration::new_Configuration( cname );
  input >> *f_dynamic_constraints_Configuration;
 }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) ) {
  f_static_variables_Configuration = nullptr;
  input.ignore( std::numeric_limits< std::streamsize >::max(),
                input.widen( '\n' ) );
 } else {
  std::string cname;
  input >> cname;
  f_static_variables_Configuration =
   Configuration::new_Configuration( cname );
  input >> *f_static_variables_Configuration;
 }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) ) {
  f_dynamic_variables_Configuration = nullptr;
  input.ignore( std::numeric_limits< std::streamsize >::max(),
                input.widen( '\n' ) );
 } else {
  std::string cname;
  input >> cname;
  f_dynamic_variables_Configuration =
   Configuration::new_Configuration( cname );
  input >> *f_dynamic_variables_Configuration;
 }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) ) {
  f_objective_Configuration = nullptr;
  input.ignore( std::numeric_limits< std::streamsize >::max(),
                input.widen( '\n' ) );
 } else {
  std::string cname;
  input >> cname;
  f_objective_Configuration = Configuration::new_Configuration( cname );
  input >> *f_objective_Configuration;
 }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) ) {
  f_is_feasible_Configuration = nullptr;
  input.ignore( std::numeric_limits< std::streamsize >::max(),
                input.widen( '\n' ) );
 } else {
  std::string cname;
  input >> cname;
  f_is_feasible_Configuration = Configuration::new_Configuration( cname );
  input >> *f_is_feasible_Configuration;
 }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) ) {
  f_is_optimal_Configuration = nullptr;
  input.ignore( std::numeric_limits< std::streamsize >::max(),
                input.widen( '\n' ) );
 } else {
  std::string cname;
  input >> cname;
  f_is_optimal_Configuration = Configuration::new_Configuration( cname );
  input >> *f_is_optimal_Configuration;
 }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) ) {
  f_solution_Configuration = nullptr;
  input.ignore( std::numeric_limits< std::streamsize >::max(),
                input.widen( '\n' ) );
 } else {
  std::string cname;
  input >> cname;
  f_solution_Configuration = Configuration::new_Configuration( cname );
  input >> *f_solution_Configuration;
 }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) ) {
  f_extra_Configuration = nullptr;
  input.ignore( std::numeric_limits< std::streamsize >::max(),
                input.widen( '\n' ) );
 } else {
  std::string cname;
  input >> cname;
  f_extra_Configuration = Configuration::new_Configuration( cname );
  input >> *f_extra_Configuration;
 }
}  // end( BlockConfig::load )

/*--------------------------------------------------------------------------*/

void BlockConfig::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );

 group.putAtt( "diff" , netCDF::NcInt() , int( f_diff ) );

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

void BlockConfig::deserialize( netCDF::NcGroup & group )
{
 if( f_static_constraints_Configuration ||
     f_dynamic_constraints_Configuration ||
     f_static_variables_Configuration || f_dynamic_variables_Configuration ||
     f_objective_Configuration || f_is_feasible_Configuration ||
     f_is_optimal_Configuration || f_solution_Configuration ||
     f_extra_Configuration )
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
