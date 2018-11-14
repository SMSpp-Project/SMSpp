/*--------------------------------------------------------------------------*/
/*---------------------------- File Block.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the Block class.
 *
 * \version 0.10
 *
 * \date 04 - 04 - 2018
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
 * Copyright &copy by Antonio Frangioni, Rafael Durbano Lobato
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

// register BlockConfig, BlockSolverConfig to the Configuration factory

SMSpp_insert_in_factory_cpp_0( BlockConfig );
SMSpp_insert_in_factory_cpp_0( BlockSolverConfig );

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/
// Auxiliary functions for Block.cpp not exported as methods of the class


/*--------------------------------------------------------------------------*/
/*-------------------------- METHODS of Block ------------------------------*/
/*--------------------------------------------------------------------------*/
/*----------------- Methods for reading the data of the Block --------------*/
/*--------------------------------------------------------------------------*/

BlockSolverConfig * Block::get_SolverConfig( BlockSolverConfig * svcc )
{
 BlockSolverConfig * scfg = svcc ? svcc : new BlockSolverConfig();

 c_Lst_Solver & ls = get_registered_solvers();

 scfg->v_SolverNames.resize( ls.size() );
 scfg->v_SolverConfigs.resize( ls.size() );

 auto lsit = ls.begin();
 for( int i = 0 ; i < ls.size() ; ++i , ++lsit ) {
  scfg->v_SolverNames[ i ] = (*lsit)->name();
  scfg->v_SolverConfigs[ i ] = (*lsit)->get_ComputeConfig();
  }

 c_Vec_Block & nb = get_nested_Blocks();
 scfg->v_BlockSolverConfigs.resize( nb.size() );

 auto nbit = nb.begin();
 for( int i = 0 ; i < nb.size() ; )
  scfg->v_BlockSolverConfigs[ i++ ] = (*(nbit++))->get_SolverConfig();

 return( scfg );

 }  // end( Block::get_SolverConfig )

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

 if( ! chnl ) {                         // not the default channel
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
 v_current_GroupMod[ chnl - 1 ]->v_sub_Modifications.push_back( mod );
  
 }  // end( Block::add_Modification )

/*--------------------------------------------------------------------------*/

Observer::ChnlName Block::open_channel( GroupModification * gmpmod ,
					c_ModParam issueMod )
{
 if( ! gmpmod )
  gmpmod = new GroupModification( Observer::par2concern( issueMod ) );

 auto it = std::find( v_current_GroupMod.begin() ,
		      v_current_GroupMod.end() , nullptr );
 ChnlName chnl;
 if( it >= v_current_GroupMod.end() ) {
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

void Block::nest_channel( c_ChnlName chnl , GroupModification * gmpmod ,
			  c_ModParam issueMod )
{
 if( ( ! chnl ) || ( chnl > v_current_GroupMod.size() ) ||
     ( v_current_GroupMod[ chnl - 1 ] == nullptr ) )
  throw( std::invalid_argument( "wrong channel name" ) );

 if( ! gmpmod )
  gmpmod = new GroupModification( Observer::par2concern( issueMod ) );

 gmpmod->f_father = v_current_GroupMod[ chnl - 1 ];
 v_current_GroupMod[ chnl - 1 ]->v_sub_Modifications.push_back(
			       std::shared_ptr<GroupModification>( gmpmod ) );
 v_current_GroupMod[ chnl - 1 ] = gmpmod;

 }  // end( Block::nest_channel )

/*--------------------------------------------------------------------------*/

void Block::un_nest_channel( c_ChnlName chnl )
{
 if( ( chnl > v_current_GroupMod.size() ) ||
     ( v_current_GroupMod[ chnl - 1 ] == nullptr ) )
  throw( std::invalid_argument( "wrong channel name" ) );

 if( ! v_current_GroupMod[ chnl - 1 ]->f_father )
  throw( std::invalid_argument( "channel is at root level" ) );

 v_current_GroupMod[ chnl - 1 ] = v_current_GroupMod[ chnl - 1 ]->f_father;

 }  // end( Block::un_nest_channel )

/*--------------------------------------------------------------------------*/

void Block::close_channel( c_ChnlName chnl )
{
 if( ( chnl > v_current_GroupMod.size() ) ||
     ( v_current_GroupMod[ chnl - 1 ] == nullptr ) )
  throw( std::invalid_argument( "wrong channel name" ) );

 add_Modification( std::shared_ptr<GroupModification>(
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

void Block::set_default_channel( c_ChnlName chnl )
{
 if( ( ! chnl ) || ( chnl > v_current_GroupMod.size() ) ||
     ( v_current_GroupMod[ chnl - 1 ] == nullptr ) )
  throw( std::invalid_argument( "wrong channel name" ) );

 f_channel = chnl;
 }

/*--------------------------------------------------------------------------*/
/*------------ METHODS FOR LOADING, PRINTING & SAVING THE Block ------------*/
/*--------------------------------------------------------------------------*/

void Block::remove_constraint_from_variables( Constraint * constraint )
{
 for( auto & var : *constraint )
  var.remove_active( constraint );
 }

/*--------------------------------------------------------------------------*/

void Block::remove_variable_from_stuff( Variable * const variable ,
					const int issueindMod )
{
 for( int i = 0 ; i < variable->get_num_active() ; ++i )
  variable->get_active( i )->remove_variable( variable , issueindMod );
 }

/*--------------------------------------------------------------------------*/

void Block::set_BlockConfig( BlockConfig *newBC , const bool safe )
{
 if( ( ! safe ) && f_Block && f_Block->f_BlockConfig ) {
  // the current BlockConfig, if any, can be "live" in the BlockConfig of the
  // father: find i such that this Block is the i-th son of its father Block
  const int i = std::distance( f_Block->v_Block.begin() ,
     std::find( f_Block->v_Block.begin() , f_Block->v_Block.end() , this ) );

  if( i >= f_Block->v_Block.size() )
   throw( std::domain_error( "This Block is not a son of its father!" ) );

  // then, this f_BlockConfig must be at the i-th position of the
  // v_sub_BlockConfig in the father's f_BlockConfig, unless the vector is
  // shorter than that; in this case it is resized

  std::vector<BlockConfig *> &fvsBC =
                                    f_Block->f_BlockConfig->v_sub_BlockConfig;

  int j = fvsBC.size();
  if( i >= j ) {  // the father did not have a i-th BlockConfig
   fvsBC.resize( i + 1 );
   for( ; j < i ; ++j )
    fvsBC[ j ] = nullptr;
   }
  else           // the father had the i-th BlockConfig
   if( fvsBC[ i ] != f_BlockConfig )
    throw( std::domain_error( "Inconsistent v_sub_BlockConfig" ) );

  fvsBC[ i ] = newBC;
  }

 if( f_BlockConfig ) {  // there is a BlockConfig already
  for( auto blck : v_Block )   // delete all sub-BlockConfig, if any
   blck->set_BlockConfig();

  delete f_BlockConfig;     // delete the previous BlockConfig
  }

 int i = 0;
 if( newBC ) {  // a new BlockConfig is being set
  std::vector<BlockConfig *> &vsBC = newBC->v_sub_BlockConfig;
  for( ; i < vsBC.size() ; ++i )
   v_Block[ i ]->set_BlockConfig( vsBC[ i ] );
  }

 // for all sub-Block that have no explicit sub-BlockConfig (all if
 // newBC == nullptr), set them to "no configuration"
 for( ; i < v_Block.size() ; ++i )
  v_Block[ i ]->set_BlockConfig();
 
 f_BlockConfig = newBC;  // finally ...

 }  // end( set_BlockConfig )

/*--------------------------------------------------------------------------*/

void Block::add_to_BlockConfig( BlockConfig *aBC , const bool safe )
{
 if( ! aBC )  // adding a void configuration to anything
  return;     // does nothing

 if( ! f_BlockConfig ) {   // adding to nothing
  set_BlockConfig( aBC );  // is setting
  return;
  }

 // add the individual fields of aBC to the corresponding ones of the
 // f_BlockConfig
 if( aBC->f_name.size() )
  f_BlockConfig->f_name = aBC->f_name;

 if( aBC->f_static_constraints_Configuration ) {
  delete f_BlockConfig->f_static_constraints_Configuration;
  f_BlockConfig->f_static_constraints_Configuration =
                                     aBC->f_static_constraints_Configuration;
  aBC->f_static_constraints_Configuration = nullptr;
  }

 if( aBC->f_dynamic_constraints_Configuration ) {
  delete f_BlockConfig->f_dynamic_constraints_Configuration;
  f_BlockConfig->f_dynamic_constraints_Configuration =
                                    aBC->f_dynamic_constraints_Configuration;
  aBC->f_dynamic_constraints_Configuration = nullptr;
  }

 if( aBC->f_static_variables_Configuration ) {
  delete f_BlockConfig->f_static_variables_Configuration;
  f_BlockConfig->f_static_variables_Configuration =
                                      aBC->f_static_variables_Configuration;
  aBC->f_static_variables_Configuration = nullptr;
  }

 if( aBC->f_dynamic_variables_Configuration ) {
  delete f_BlockConfig->f_dynamic_variables_Configuration;
  f_BlockConfig->f_dynamic_variables_Configuration =
                                     aBC->f_dynamic_variables_Configuration;
  aBC->f_dynamic_variables_Configuration = nullptr;
  }

 if( aBC->f_objective_Configuration ) {
  delete f_BlockConfig->f_objective_Configuration;
  f_BlockConfig->f_objective_Configuration = aBC->f_objective_Configuration;
  aBC->f_objective_Configuration = nullptr;
  }

 if( aBC->f_is_feasible_Configuration ) {
  delete f_BlockConfig->f_is_feasible_Configuration;
  f_BlockConfig->f_is_feasible_Configuration =
                                           aBC->f_is_feasible_Configuration;
  aBC->f_is_feasible_Configuration = nullptr;
  }
 
 if( aBC->f_is_optimal_Configuration ) {
  delete f_BlockConfig->f_is_optimal_Configuration;
  f_BlockConfig->f_is_optimal_Configuration =
                                            aBC->f_is_optimal_Configuration;
  aBC->f_is_optimal_Configuration = nullptr;
  }

 if( aBC->f_solution_Configuration ) {
  delete f_BlockConfig->f_solution_Configuration;
  f_BlockConfig->f_solution_Configuration =
                                              aBC->f_solution_Configuration;
  aBC->f_solution_Configuration = nullptr;
  }

 // now process the vectors of sub-BlockConfig
 std::vector<BlockConfig *> &vsBC = aBC->v_sub_BlockConfig;
 std::vector<BlockConfig *> &fvsBC = f_BlockConfig->v_sub_BlockConfig;

 // silently discard any component of vsBC for which there is no sub-Block
 int i = min( vsBC.size() , v_Block.size() );

 // ignore the last part of vsBC that is all made of nullptr, if any
 while( ( i > 0 ) && ( ! vsBC[ i - 1 ] ) )
  --i;

 // if necessary, resize fvsBC
 int j = fvsBC.size();
 if( i > j ) {
   fvsBC.resize( i );
   for( ; j < i ; ++j )
    fvsBC[ j ] = nullptr;
   }

 // now add (or set) each element of aBC to the corresponding one of fvsBC
 for( j = 0 ; j < i ; ++j ) {
  if( fvsBC[ j ] )
   v_Block[ j ]->add_to_BlockConfig( vsBC[ j ] );
  else
   v_Block[ j ]->set_BlockConfig( vsBC[ j ] );

  vsBC[ j ] = nullptr;  // remember it has been consumed already
  }

 delete aBC;  // finally ...

 }  // end( Block::add_to_BlockConfig )

/*--------------------------------------------------------------------------*/

void Block::set_SolverConfig( BlockSolverConfig * svcc )
{
 if( ! svcc ) {  // complete reset
  // unregister Solver in reverse order
  for( auto it = v_Solver.rbegin() ; it != v_Solver.rend() ; ++it )
   unregister_Solver( --(it.base()) );  // convert backward into forward

  // reset all Solver in all sub-Block
  for( auto it = v_Block.begin() ; it != v_Block.end() ; ++it )
   (*it)->set_SolverConfig();

  return;        // all done
  }

 // set the configurations for the Solver of this Block ----------------------
 //---------------------------------------------------------------------------
 auto sit = v_Solver.begin();
 auto nit = svcc->v_SolverNames.begin();
 auto cit = svcc->v_SolverConfigs.begin();

 // process existing Solvers -------------------------------------------------

 if( svcc->f_diff ) {  // differential mode ----------------------------------
  for( ; ( sit != v_Solver.end() ) && ( nit != svcc->v_SolverNames.end() ) ;
       ++sit , ++nit ) {

   if( ! (*nit).empty() ) {  // if the name is empty do nothing
    Solver *oldS = *sit;
    replace_Solver( Solver::new_Solver( *nit ) , sit );
    delete oldS;
    }

   if( cit != svcc->v_SolverConfigs.end() ) {
    if( *cit )  // if the configurtion is empty, do nothing
     (*sit)->set_ComputeConfig( *cit );
    ++cit;
    }
   }
  }
 else {                // setting mode ---------------------------------------
  for( ; ( sit != v_Solver.end() ) && ( nit != svcc->v_SolverNames.end() ) ;
       ++nit ) {

   Solver *oldS = *sit;

   if( (*nit).empty() ) {  // empty name
    unregister_Solver( sit );
    if( cit != svcc->v_SolverConfigs.end() )
     ++cit;  // ignore corresponding configuration
    // note: sit is not increased because the list is shortened
    }
   else {                   // non-empty name
    replace_Solver( Solver::new_Solver( *nit ) , sit );
    if( cit != svcc->v_SolverConfigs.end() ) {
     (*sit)->set_ComputeConfig( *cit );
     ++cit;
     }
    ++sit;
    }

   delete oldS;
   }
  }                    // end setting mode -----------------------------------

 // process Solvers in the Configuration but not in the Block-----------------

 for( ; nit != svcc->v_SolverNames.end() ; ++nit )   
  if( (*nit).empty() ) {  // if the name is empty
   if( cit != svcc->v_SolverConfigs.end() )
    ++cit;
   }
  else {                   // the name is non-empty
   register_Solver( Solver::new_Solver( *nit ) );

   if( cit != svcc->v_SolverConfigs.end() ) {
    if( *cit )  // if the configurtion is empty, do nothing
     v_Solver.back()->set_ComputeConfig( *cit );
    ++cit;
    }
   }

 // set the configurations for the sub-Block ---------------------------------
 //---------------------------------------------------------------------------
 auto bit = v_Block.begin();
 auto bsit = svcc->v_BlockSolverConfigs.begin();

 if( svcc->f_diff ) {  // differential mode ----------------------------------
  // only set non-nullptr configurations, hence only up until the list of
  // BlockSolverConfigs ends
  for( ; ( bit != v_Block.end() ) &&
	 ( bsit != svcc->v_BlockSolverConfigs.end() ) ;
         ++bit , ++bsit )
   if( *bsit )
    (*bit)->set_SolverConfig( *bsit );
  }
 else {                // setting mode ---------------------------------------
  // process the list of BlockSolverConfigs, don't mind of nullptr
  for( ; ( bit != v_Block.end() ) &&
	 ( bsit != svcc->v_BlockSolverConfigs.end() ) ;
         ++bit , ++bsit )
   (*bit)->set_SolverConfig( *bsit );

  // after the list ends, the remaining configurations are nullptr
  for( ; bit != v_Block.end() ; ++bit )
   (*bit)->set_SolverConfig();
  }                    // end setting mode -----------------------------------

}  // end( Block::set_SolverConfig )

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
/*------------------------- METHODS of BlockConfig -------------------------*/
/*--------------------------------------------------------------------------*/

BlockConfig::BlockConfig( BlockConfig &old ) : Configuration()
{
 f_name = old.f_name;

 f_static_constraints_Configuration =
                              old.f_static_constraints_Configuration->clone();
 f_dynamic_constraints_Configuration =
                             old.f_dynamic_constraints_Configuration->clone();
 f_static_variables_Configuration =
                              old.f_static_variables_Configuration->clone();
 f_dynamic_variables_Configuration =
                              old.f_dynamic_variables_Configuration->clone();
 f_objective_Configuration = old.f_objective_Configuration->clone();
 f_is_feasible_Configuration = old.f_is_feasible_Configuration->clone();
 f_is_optimal_Configuration = old.f_is_optimal_Configuration->clone();
 f_solution_Configuration = old.f_solution_Configuration->clone();
 f_extra_Configuration = old.f_extra_Configuration->clone();

 v_sub_BlockConfig.resize( old.v_sub_BlockConfig.size() );
 for( int i = 0 ; i < v_sub_BlockConfig.size() ; ++i )
  v_sub_BlockConfig[ i ] = old.v_sub_BlockConfig[ i ]->clone();
 }

/*--------------------------------------------------------------------------*/

BlockConfig * BlockConfig::deserialize( netCDF::NcFile && f ,
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

  auto result = new_Configuration( std::move( cg ) );
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

void BlockConfig::serialize( netCDF::NcFile && f , const int type ) const
{
 if( type == eConfigFile ) {
  Configuration::serialize( std::move( f ) , type );
  return;
  }

 serialize( ( f.addGroup( "Desc_" + std::to_string( f.getGroupCount() )
			  ) ).addGroup( "BlockConfig" ) );

 }  // end( BlockConfig::serialize( file ) )

/*--------------------------------------------------------------------------*/

void BlockConfig::print( std::ostream &output ) const
{
 output << "BlockConfig" << std::endl;
 if( ! f_name.empty() )
  output << "[" << f_name << "]";
 output << ": ";
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
 for( auto cfg : v_sub_BlockConfig )
  if( cfg )
   output << *cfg;
 output << std::endl;

 }  // end( BlockConfig::print )

/*--------------------------------------------------------------------------*/

void BlockConfig::load( std::istream &input )
{
 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_name.erase();
 else
  input >> f_name;

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_static_constraints_Configuration = nullptr;
 else {
  std::string cname;
  input >> cname;
  f_static_constraints_Configuration =
                                    Configuration::new_Configuration( cname );
  input >> *f_static_constraints_Configuration;
  }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_dynamic_constraints_Configuration = nullptr;
 else {
  std::string cname;
  input >> cname;
  f_dynamic_constraints_Configuration =
                                    Configuration::new_Configuration( cname );
  input >> *f_dynamic_constraints_Configuration;
  }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_static_variables_Configuration = nullptr;
 else {
  std::string cname;
  input >> cname;
  f_static_variables_Configuration =
                                    Configuration::new_Configuration( cname );
  input >> *f_static_variables_Configuration;
  }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_dynamic_variables_Configuration = nullptr;
 else {
  std::string cname;
  input >> cname;
  f_dynamic_variables_Configuration =
                                    Configuration::new_Configuration( cname );
  input >> *f_dynamic_variables_Configuration;
  }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_objective_Configuration = nullptr;
 else {
  std::string cname;
  input >> cname;
  f_objective_Configuration = Configuration::new_Configuration( cname );
  input >> *f_objective_Configuration;
  }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_is_feasible_Configuration = nullptr;
 else {
  std::string cname;
  input >> cname;
  f_is_feasible_Configuration = Configuration::new_Configuration( cname );
  input >> *f_is_feasible_Configuration;
  }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_is_optimal_Configuration = nullptr;
 else {
  std::string cname;
  input >> cname;
  f_is_optimal_Configuration = Configuration::new_Configuration( cname );
  input >> *f_is_optimal_Configuration;
  }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_solution_Configuration = nullptr;
 else {
  std::string cname;
  input >> cname;
  f_solution_Configuration = Configuration::new_Configuration( cname );
  input >> *f_solution_Configuration;
  }

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_extra_Configuration = nullptr;
 else {
  std::string cname;
  input >> cname;
  f_extra_Configuration = Configuration::new_Configuration( cname );
  input >> *f_extra_Configuration;
  }

 int k;
 input >> eatcomments >> k;
 v_sub_BlockConfig.resize( k );
 for( int i = 0 ; i < k ; ++i ) {
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   v_sub_BlockConfig[ i ] = nullptr;
  else {
   std::string cname;
   input >> cname;
   Configuration *tmpc = Configuration::new_Configuration( cname );
   BlockConfig *tmpbc = dynamic_cast<BlockConfig *>( tmpc );
   if( ! tmpbc )
    throw( invalid_argument( "not a BlockConfig object" ) );
   v_sub_BlockConfig[ i ] = tmpbc;
   input >> *tmpbc;
   }
  }
 }  // end( BlockConfig::load )

/*--------------------------------------------------------------------------*/

void BlockConfig::serialize( netCDF::NcGroup && group ) const
{
 Configuration::serialize( std::move( group ) );

 if( f_static_constraints_Configuration )
  f_static_constraints_Configuration->serialize(
				    group.addGroup( "static_constraints" ) );

 if( f_dynamic_constraints_Configuration )
  f_dynamic_constraints_Configuration->serialize(
                                   group.addGroup( "dynamic_constraints" ) );

 if( f_static_variables_Configuration )
  f_static_variables_Configuration->serialize(
				      group.addGroup( "static_variables" ) );

 if( f_dynamic_variables_Configuration )
  f_dynamic_variables_Configuration->serialize(
				     group.addGroup( "dynamic_variables" ) );

 if( f_objective_Configuration )
  f_objective_Configuration->serialize( group.addGroup( "objective" ) );

 if( f_is_feasible_Configuration )
  f_is_feasible_Configuration->serialize( group.addGroup( "is_feasible" ) );

 if( f_is_optimal_Configuration )
  f_is_optimal_Configuration->serialize( group.addGroup( "is_optimal" ) );

 if( f_solution_Configuration )
  f_solution_Configuration->serialize( group.addGroup( "solution" ) );

 if( f_extra_Configuration )
  f_extra_Configuration->serialize( group.addGroup( "extra" ) );

 group.addDim( "n_sub_Block" , v_sub_BlockConfig.size() );

 for( size_t i = 0 ; i < v_sub_BlockConfig.size() ; ++i )
  if( v_sub_BlockConfig[ i ] )
   v_sub_BlockConfig[ i ]->serialize(
	         group.addGroup( "sub-BlockConfig_" + std::to_string( i ) ) );

 }  // end( BlockConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void BlockConfig::deserialize( netCDF::NcGroup && group )
{
 if( f_static_constraints_Configuration ||
     f_dynamic_constraints_Configuration ||
     f_static_variables_Configuration || f_dynamic_variables_Configuration ||
     f_objective_Configuration || f_is_feasible_Configuration ||
     f_is_optimal_Configuration || f_solution_Configuration ||
     f_extra_Configuration || v_sub_BlockConfig.size() )
  throw( std::logic_error( "deserializing a non-empty BlockConfig" ) );

 netCDF::NcGroupAtt name = group.getAtt( "name" );
 if( name.isNull() )
  throw( std::invalid_argument( "missing name in netCDF group" ) );

 name.getValues( f_name );

 f_static_constraints_Configuration = new_Configuration(
				    group.getGroup( "static_constraints" ) );

 f_dynamic_constraints_Configuration = new_Configuration(
				   group.getGroup( "dynamic_constraints" ) );

 f_static_variables_Configuration = new_Configuration(
				      group.getGroup( "static_variables" ) );

 f_dynamic_variables_Configuration = new_Configuration(
				     group.getGroup( "dynamic_variables" ) );

 f_objective_Configuration = new_Configuration(
					     group.getGroup( "objective" ) );

 f_is_feasible_Configuration = new_Configuration(
					   group.getGroup( "is_feasible" ) );

 f_is_optimal_Configuration = new_Configuration(
					    group.getGroup( "is_optimal" ) );

 f_solution_Configuration = new_Configuration( group.getGroup( "solution" ) );

 f_extra_Configuration = new_Configuration( group.getGroup( "extra" ) );

 size_t size = ( group.getDim( "n_sub_Block" ) ).getSize();

 v_sub_BlockConfig.resize( size );

 for( size_t i = 0 ; i < size ; ++i )
  v_sub_BlockConfig[ i ] = dynamic_cast< BlockConfig *> (
		   new_Configuration( group.getGroup( "sub-BlockConfig_" +
						      std::to_string( i ) ) )
							 );
 }  // end( BlockConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*--------------------- METHODS of BlockSolverConfig -----------------------*/
/*--------------------------------------------------------------------------*/

BlockSolverConfig::BlockSolverConfig( BlockSolverConfig &old )
 : Configuration()
{
 f_diff = old.f_diff;
 v_SolverNames = old.v_SolverNames;

 v_SolverConfigs.resize( old.v_SolverConfigs.size() );
 for( int i = 0 ; i < v_SolverConfigs.size() ; ++i )
  v_SolverConfigs[ i ] = old.v_SolverConfigs[ i ]->clone();

 v_BlockSolverConfigs.resize( old.v_BlockSolverConfigs.size() );

 for( int i = 0 ; i < v_BlockSolverConfigs.size() ; ++i )
  v_BlockSolverConfigs[ i ] = old.v_BlockSolverConfigs[ i ]->clone();
 }

/*--------------------------------------------------------------------------*/

BlockSolverConfig * BlockSolverConfig::deserialize( netCDF::NcFile && f ,
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

  auto result = new_Configuration( std::move( cg ) );
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

void BlockSolverConfig::serialize( netCDF::NcFile && f , const int type )
 const
{
 if( type == eConfigFile ) {
  Configuration::serialize( std::move( f ) , type );
  return;
  }

 serialize( ( f.addGroup( "Desc_" + std::to_string( f.getGroupCount() )
			  ) ).addGroup( "SolverConfig" ) );

 }  // end( BlockSolverConfig::serialize( file ) )

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::print( std::ostream &output ) const
{
 output << "BlockSolverConfig";
 if( f_diff ) output << "[diff]";
 output << ": " << std::endl;
 for( int i = 0 ; i < v_SolverNames.size() ; ++i )
  output << v_SolverNames[ i ] << ": " << v_SolverConfigs[ i ];
 for( auto cfg : v_BlockSolverConfigs )
  if( cfg )
   output << *cfg;
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
    throw( invalid_argument( "not a ComputeConfig object" ) );
   v_SolverConfigs[ i ] = tmpbc;
   input >> *tmpbc;
   }
  }

 input >> eatcomments >> k;
 v_BlockSolverConfigs.resize( k );
 for( int i = 0 ; i < k ; ++i ) {
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   v_BlockSolverConfigs[ i ] = nullptr;
  else {
   std::string cname;
   input >> cname;
   Configuration *tmpc = Configuration::new_Configuration( cname );
   BlockSolverConfig *tmpbc = dynamic_cast<BlockSolverConfig *>( tmpc );
   if( ! tmpbc )
    throw( invalid_argument( "not a BlockSolverConfig object" ) );
   v_BlockSolverConfigs[ i ] = tmpbc;
   input >> *tmpbc;
   }
  }
 }  // end( BlockSolverConfig::load )

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::serialize( netCDF::NcGroup && group ) const
{
 Configuration::serialize( std::move( group ) );

 netCDF::NcDim sd = group.addDim( "n_SolverConfig" , v_SolverConfigs.size() );
 group.addDim( "n_BlockSolverConfig" , v_BlockSolverConfigs.size() );

 netCDF::NcVar slvnms = group.addVar( "SolverNames" , netCDF::NcString() ,
				      sd );
 std::vector<size_t> idx( 1 );
 for( size_t i = 0 ; i < v_SolverConfigs.size() ; ++i ) {
  idx[ 0 ] = i;
  slvnms.putVar( idx , v_SolverNames[ i ] );
  v_SolverConfigs[ i ]->serialize( group.addGroup(
		                   "SolverConfig_" + std::to_string( i ) ) );
  }

 for( size_t i = 0 ; i < v_BlockSolverConfigs.size() ; ++i )
  v_BlockSolverConfigs[ i ]->serialize( group.addGroup(
		              "BlockSolverConfig_" + std::to_string( i ) ) );

 }  // end( BlockSolverConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::deserialize( netCDF::NcGroup && group )
{
 if( v_SolverNames.size() || v_SolverConfigs.size() ||
     v_BlockSolverConfigs.size() )
  throw( std::logic_error( "deserializing a non-empty BlockSolverConfig" ) );

 Configuration::deserialize( std::move( group ) );

 size_t slvsize = ( group.getDim( "n_SolverConfig" ) ).getSize();
 size_t blkslvsize = ( group.getDim( "n_BlockSolverConfig" ) ).getSize();

 v_SolverNames.resize( slvsize );
 v_SolverConfigs.resize( slvsize );
 v_BlockSolverConfigs.resize( blkslvsize );

 netCDF::NcVar slvnms = group.getVar( "SolverNames" );
 std::vector<size_t> idx( 1 );
 for( size_t i = 0 ; i < slvsize ; ++i ) {
  idx[ 0 ] = i;
  char * str;
  slvnms.getVar( idx , & str );
  v_SolverNames[ i ] = std::string( str );
  v_SolverConfigs[ i ] = dynamic_cast< ComputeConfig * >(
                  new_Configuration( group.getGroup( "SolverConfig_" +
						     std::to_string( i ) ) )
							 );
  }

 for( size_t i = 0 ; i < blkslvsize ; ++i )
  v_BlockSolverConfigs[ i ] = dynamic_cast< BlockSolverConfig * >(
	          new_Configuration( group.getGroup( "BlockSolverConfig_" +
						     std::to_string( i ) ) )
								  );

 }  // end( BlockSolverConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Block.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
