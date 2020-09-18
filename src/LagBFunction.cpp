/*--------------------------------------------------------------------------*/
/*------------------------ File LagBFunction.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the LagBFunction class, which is derived from
 * both a C05Function and a Block. The class is an interface for a
 * Lagrangian function.
 *
 * \version 0.20
 *
 * \date 18 - 09 - 2020
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Enrico Gorgone \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni, Enrico Gorgone
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "BlockSolverConfig.h"

#include "FRowConstraint.h"

#include "LagBFunction.h"

#include "RBlockConfig.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

static constexpr C05Function::Index NaNLinName
                      = std::numeric_limits<C05Function::Index>::quiet_NaN();

static constexpr Function::FunctionValue NaN = FunctionMod::NaNshift;

static constexpr Function::FunctionValue INF = FunctionMod::INFshift;

// register LagBFunction to the Block factory
SMSpp_insert_in_factory_cpp_1( LagBFunction );

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

LagBFunction::LagBFunction( Block* innerblock , Observer * const observer )
 :  C05Function() , obj( nullptr ) , IsConvex( true ) , f_max_glob( 0 ) ,
    LastSolution( NaNLinName ) , VarSol( true ) , f_linear_term( 0 ) ,
    f_play_dumb( false ) , LPMaxSz( 0 ) , RAccLin( 0 ) , AAccLin( 0 ) ,
    svcc( nullptr )
{
 // set the pointer to the sub-Block (B) - - - - - - - - - - - - - - - - - - -

 if( innerblock )
  set_inner_block( innerblock );

 // set the observer pointer - - - - - - - - - - - - - - - - - - - - - - - - -

 if( observer )
  register_Observer( observer );

 }  // end( LagBFunction::LagBFunction() )

/*--------------------------------------------------------------------------*/

void LagBFunction::clear( void )
{
 // delete all the ColVariable (and the Lagrangian terms with them)
 for( const auto & dp : LagPairs )
  delete dp.second;
 LagPairs.clear();

 // delete the auxiliary data structure for computing the Lagrangian costs
 CostMatrix.clear();

 // delete the global pool (do not issue any Modification because clear()
 // is only meant to be called right before destroying the Function, any
 // Solver attached to it should have been done with long ago
 for( Index i = 0 ; i < f_max_glob ; ++i )
  delete g_pool[ i ].first;
 g_pool.clear();
 f_max_glob = 0;
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::set_inner_block( Block* innerblock )
{
 // consstency checks

 if( ! innerblock )
  throw( std::invalid_argument( "empty inner Block not allowed" ) );

 auto frobj = innerblock->get_objective< FRealObjective >();
 if( ! frobj )
 throw( std::invalid_argument( "inner Block with no FRealObj not allowed" ) );

 IsConvex = ( frobj->get_sense() == Objective::eMax );

 obj = dynamic_cast< LinearFunction * >( frobj->get_function() );
 if( ! obj )
  throw( std::logic_error( "inner Block with no linear obj not allowed" ) );

 // set the inner Block
 if( ! v_Block.empty() )
  v_Block.clear();

 v_Block.push_back( innerblock );

  // construct CostMatrix whose size is that of active variables in (obj_B)
 initialize_cost_matrix();

 }  // end( LagBFunction::set_inner_block() )

/*--------------------------------------------------------------------------*/

void LagBFunction::set_dual_pairs( v_dual_pair && lp , ModParam issueMod )
{
 // this function is used to initialize a bunch of relaxed constraints along
 // with their Lagrangian multipliers

 clear();

 /* If not already ordered by ColVariable "name = pointer", sort the vector of
    dual Lagrangian pairs <y_i,g_i(x)=a_i^T x > and construct the structure
    CostMatrix which is used to update the Lagrangian cost vector.

    CostMatrix is a map whose the key is the pointer to the primal variable x_j,
    the second field is the pair < c_j , j-th column >, being the j-th column
    the vector < y, A_j>. It is assumed that map is ordered by the primal
    variable name and a column < y, A_j> is ordered by Lagrangian multiplier
    name (= pointer).

    Copy the coefficients c of (obj_B) in CostMatrix in order to allow
    the modifications of the Lagrangian cost vector c^y = c + yA,
    the original costs c will be unavailable unless have been stored
    somewhere, the issue is that -in (obj_B)- vector c must be
    replaced by c^y. */

 add_columns( lp );

 // sub-Block has been already defined - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( v_Block.empty() )
  throw( std::logic_error( "no sub-block is present" ) );

 // the vector LagPairs is empty, so initialize it adding the relaxed
 // constraints (RCs) ={ g_i(x) : for some i } - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LagPairs = std::move( lp );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 Vec_p_Var vars( LagPairs.size() );
 for( Index i = 0 ; i < LagPairs.size() ; ++i )
  vars[ i ] = LagPairs[ i ].first;

 // a Lagrangian function is strongly quasi-additive
 f_Observer->add_Modification( std::make_shared< C05FunctionModVarsAddd >(
					 this , std::move( vars ) , 0 , 0 ,
					 Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 } // end ( LagBFunction::set_dual_pairs( ) )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::set_relaxed_function( Function * const function  )
{
 // register this Objective as an Observer of the given Function (if any)

 function->register_Observer( this );

 }  // end ( LagBFunction::set_relaxed_function( ) )   - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::set_default_inner_Block_BlockConfig() {
 if( auto inner_block = get_inner_block() ) {
   auto config = new OCRBlockConfig( inner_block );
   config->clear();
   config->apply( inner_block );
  }
}

/*--------------------------------------------------------------------------*/

void LagBFunction::set_default_inner_Block_BlockSolverConfig() {
 if( auto inner_block = get_inner_block() ) {
   auto solver_config = new RBlockSolverConfig( inner_block );
   solver_config->clear();
   solver_config->apply( inner_block );
  }
}

/*--------------------------------------------------------------------------*/

void LagBFunction::set_ComputeConfig( ComputeConfig *scfg )
{
 ThinComputeInterface::set_ComputeConfig( scfg );

 auto inner_block = get_inner_block();

 if( ! inner_block )
  return;
 else if( ! scfg ) {
  // scfg is nullptr
  set_default_inner_Block_configuration();
  return;
 }
 else if( ! scfg->f_extra_Configuration ) {
  // scfg->f_extra_Configuration is nullptr
  if( ! scfg->f_diff )
   set_default_inner_Block_configuration();
  return;
 }

 // Set the BlockConfig of the inner Block

 if( auto config = dynamic_cast< SimpleConfiguration
     < std::pair< Configuration * , Configuration * > > * >
     ( scfg->f_extra_Configuration ) ) {

  if( config->f_value.first ) {
   if( auto block_config = dynamic_cast< BlockConfig * >
       ( config->f_value.first ) )
    block_config->apply( inner_block );
   else
    throw( std::invalid_argument
           ( "LagBFunction::set_ComputeConfig: the first element of "
             "the pair of the extra Configuration must be a pointer "
             "to a :BlockConfig." ) );
  }
  else {
   // A BlockConfig for the inner Block was not provided
   // scfg->f_extra_Configuration->f_value.first is nullptr
   if( ! scfg->f_diff )
    set_default_inner_Block_BlockConfig();
  }

  // Set the BlockSolverConfig of the inner Block

  if( config->f_value.second ) {
   if( auto block_config = dynamic_cast< BlockSolverConfig * >
       ( config->f_value.second ) )
    block_config->apply( inner_block );
   else
    throw( std::invalid_argument
           ( "LagBFunction::set_ComputeConfig: the second element "
             "of the pair of the extra Configuration must be a pointer "
             "to a :BlockSolverConfig." ) );
  }
  else {
   // A BlockSolverConfig for the inner Block was not provided
   // scfg->f_extra_Configuration->f_value.second is nullptr
   if( ! scfg->f_diff )
    set_default_inner_Block_BlockSolverConfig();
  }
 }
 else
  throw( std::invalid_argument( "LagBFunction::set_ComputeConfig: "
                                "invalid type of extra Configuration." ) );

 }  // end ( LagBFunction::set_relaxed_function( ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::set_par( const idx_type par , const int value )
{
 switch( par ) {
  case( intLPMaxSz ):
   if( LPMaxSz != value ) {
    LPMaxSz = value;
    add_par( std::string( int_par_idx2str( intLPMaxSz ) ) , value );
    }
   break;
  case( intGPMaxSz ):
   if( g_pool.size() > value ) {
    for( auto it = g_pool.begin() + value ; it != g_pool.end() ; ++it  )
     delete it->first;

    if( f_max_glob > value ) {
     f_max_glob = value;
     update_f_max_glob();
     }    
    }
   g_pool.resize( value , gpool_el( nullptr , true ) );
   break;
  default: Function::set_par( par , value );
  }
 }  // end ( LagBFunction::set_par( int ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::set_par( const idx_type par , const double value )
{
 switch( par ) {
  case( dblRAccLin ):
   if( RAccLin != value ) {
    RAccLin = value;
    add_par( std::string( dbl_par_idx2str( dblRAccLin ) ) , value );
    }
   break;
  case( dblAAccLin ):
   if( AAccLin != value ) {
    AAccLin = value;
    add_par( std::string( dbl_par_idx2str( dblAAccLin ) ) , value );
    }
   break;
  default: Function::set_par( par , value );
  }
 }  // end( LagBFunction::set_par( double ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::deserialize( netCDF::NcGroup & group )
{
 v_Block.clear();
 netCDF::NcGroup sb = group.getGroup( "B" );
 if( sb.isNull() )
  throw( std::logic_error( "the group B is null" ) );

 v_Block.push_back( new_Block( sb , this ) );

 // call the method of Block- - - - - - - - - - - - - - - - - - - - - - - - -
 // inside this the NBModification, the "nuclear option",  is issued

 Block::deserialize( group );

 }  // end( LagBFunction::deserialize )

/*--------------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::add_dual_pairs( v_dual_pair && lp , ModParam issueMod )
{
 /* If not already ordered by ColVariable "name = pointer", sort the vector of
    dual Lagrangian pairs <y_i,g_i(x)=a_i^T x > and update the structure
    CostMatrix which is used to update the Lagrangian cost vector.

    CostMatrix is a map whose the key is the pointer to the primal variable
    x_j, the second field is the pair < c_j , j-th column >, being the j-th
    column the vector < y , A_j >. It is assumed that map is ordered by the
    primal variable name and a column < y , A_j > is ordered by Lagrangian
    multipliername (= pointer).

    Copy the coefficients c of (obj_B) in CostMatrix in order to allow the
    modifications of the Lagrangian cost vector c^y = c + yA: the original
    costs c would be unavailable unless have been stored somewhere, since
    the vector c in (obj_B) must be replaced by c^y.  */

 add_columns( lp );

 // merge the list of dual Lagrangian pairs  - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 Index k = LagPairs.size();
 LagPairs.insert( LagPairs.end() , lp.begin() , lp.end() );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 Vec_p_Var vars( lp.size() );
 for( Index i = 0 ; i < lp.size() ; ++i )
  vars[ i ] = lp[ i ].first;

 // a Lagrangian function is strongly quasi-additive
 f_Observer->add_Modification( std::make_shared< C05FunctionModVarsAddd >(
					 this , std::move( vars ) , k , 0 ,
					 Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 // clear lp, because already merged with LagPairs - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 lp.clear();

 }  // end( LagBFunction::add_dual_pairs() )

/*--------------------------------------------------------------------------*/

void LagBFunction::remove_variable( Index i , ModParam issueMod )
{
 if( i >= LagPairs.size() )
  throw( std::logic_error( "less than i Variable are active" ) );

 auto itv = LagPairs.begin() + i;
 auto var = (*itv).first;
 LagPairs.erase( itv );       // erase it

 // remove the pointer to variable x_j from CostMatrix if no longer the relaxed
 // constraints (RCs) are active and restore the coefficient c_j in (obj_B)

 Subset vars( { i } );
 rm_columns( vars );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // a linear function is additive ==> strongly quasi-additive
 f_Observer->add_Modification( std::make_shared<C05FunctionModVarsRngd>(
					this , Vec_p_Var( { var } ) ,
			                Range( i , i + 1 ) , 0 ,
				        Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LagBFunction::remove_variable() )

/*--------------------------------------------------------------------------*/

void LagBFunction::remove_variables( Range range , ModParam issueMod )
{
 // TODO: better handling of a complete removal of Variable
 
 range.second = std::min( range.second , c_Index( LagPairs.size() ) );
 if( range.second <= range.first )
  return;

 const auto strtit = LagPairs.begin() + range.first;
 const auto stopit = LagPairs.begin() + range.second;

 rm_columns( range );

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vars( range.second - range.first );
  auto vpit = vars.begin();
  for( auto tmpit = strtit ; tmpit < stopit ; )
   *(vpit++) = (*(tmpit++)).first;

  LagPairs.erase( strtit , stopit );

  // now issue the Modification
  // a linear function is additive ==> strongly quasi-additive
  f_Observer->add_Modification( std::make_shared<C05FunctionModVarsRngd>(
				       this , std::move( vars ) , range , 0 ,
				       Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );
  }
 else  // noone is there: just do it
  LagPairs.erase( strtit , stopit );

 }  // end( LinearFunction::remove_variables( range ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::remove_variables( Subset && nms , bool ordered ,
				     ModParam issueMod )
{
 if( nms.empty() )
  throw( std::invalid_argument(
       "LagBFunction::remove_variables: empty nms not properly handled " ) );

 if( LagPairs.empty() )  // deleting from nothing
  throw( std::logic_error( "deleting from an empty set" ) );

 if( ! ordered )
  std::sort( nms.begin() , nms.end() );

 auto it = nms.begin();
 if( ( *it >= LagPairs.size() ) || ( nms.back() >= LagPairs.size() ) )
  throw( std::invalid_argument( "wrong index in LinearFunction" ) );

 auto vi = *it;    // first element to be eliminated
 auto curr = LagPairs.begin() + vi;   // position where to move stuff

 rm_columns( nms );

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification
  // (as it will be destroyed during the process)

  Vec_p_Var vars( nms.size() );
  auto its = vars.begin();

  *(its++) = LagPairs[ *(it++) ].first;
  ++vi;              // skip the first element, it will be overwritten

  for( ; it < nms.end() ; ++vi )
   if( *it == vi )                // one element to be eliminated
    *(its++) = LagPairs[ *(it++) ].first;  // skip it, but record the Variable
   else
    *(curr++) = LagPairs[ vi ];   // move in the current position

  auto itv = LagPairs.begin() + vi;
  for( ; itv < LagPairs.end(); )  // copy the last part
   *(curr++) = *(itv++);         // after the last of nms[]

  LagPairs.erase( curr, itv );    // erase the last part

  // now issue the Modification
  // a linear function is additive ==> strongly quasi-additive
  f_Observer->add_Modification( std::make_shared< C05FunctionModVarsSbst >(
					 this , std::move( vars ) ,
					 std::move( nms ) , ordered , 0 ,
                                         Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );
  }
 else {  // noone is there: just do it
  ++it;              // skip the first element
  ++vi;              // as it will be overwritten

  for( ; it < nms.end() ; ++vi )
   if( *it == vi )               // one element to be eliminated
    ++it;                        // skip it
   else
    *(curr++) = LagPairs[ vi ];   // move in the current position

  auto itv = LagPairs.begin() + vi;
  for( ; itv < LagPairs.end(); )  // copy the last part
   *(curr++) = *(itv++);         // after the last of v_var

  LagPairs.erase( curr, itv );    // erase the last part
  }
 }  // end( LinearFunction::remove_variables( subset ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::add_Modification( sp_Mod mod , ChnlName chnl )
{
 // if f_play_dumb == true, ignore any Modification coming directly from the
 // inner Block because that's a "self-inflicted Modification" that
 // LagBFunction caused by modifying the Objective of the inner Block
 if( f_play_dumb && ( mod->get_Block() == v_Block.front() ) )
  return;

 // if the Modification requires it, now check all the Solution in the global
 // pool for feasibility; any one found to be unfeasible is deleted, and if
 // this happen an appropriate C05FunctionMod is issued- - - - - - - - - - - -

 if( guts_of_add_Modification( mod.get() , chnl ) ) {
  Subset which;
  bool all = true;

  for( Index i = 0 ; i < f_max_glob ; ++i )
   if( g_pool[ i ].first ) {  // a Solution is there

    // write it in the Variable of the inner Block
    g_pool[ i ].first->write( v_Block.front() );
    LastSolution = i;  // and recall what's there

    // check it's still a feasible solution/direction
    bool feas = g_pool[ i ].second ? v_Block.front()->is_feasible()
                                   : v_Block.front()->is_unbounded();
    if( feas )              // if so
     all = false;           // not all are eliminated
    else {                  // otherwise
     delete g_pool[ i ].first;  // eliminate it
     g_pool[ i ].first = nullptr;
     which.push_back( i );      // recall its name
     }
    }

  if( all )        // all removed
   which.clear();  // has a special setting to it

  // if the C05FunctionModSbst has to be issued, do it
  // note: the Modification assumes issueMod == eModBlck and
  //       concerns_Block() == true
  if( ( all || ( ! which.empty() ) ) &&
      ( f_Observer && f_Observer->issue_mod( eModBlck ) ) )
   f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
				      C05FunctionMod::GlobalPoolRemoved ,
				      std::move( which ) , true , 0 ) ,
				 chnl );

  }  // end( if( checking is required ) )
 }  // end( LagBFunction::add_Modification() )

/*--------------------------------------------------------------------------*/
/*---------- METHODS FOR Loading/Saving THE DATA OF THE LagBFunction -------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::serialize( netCDF::NcGroup & group ) const
{
 // call the method of Block- - - - - - - - - - - - - - - - - - - - - - - - -

 Block::serialize( group );

 // now the LagBFunction data - - - - - - - - - - - - - - - - - - - - - - - -

 if( v_Block.size() != 1 )
  throw( std::invalid_argument( "it is expected 1 sub-block " ) );

 // The costs saved in (obj_B) are the Lagrangian ones. Hence, we need
 // to restore the original ones before serializing (B).

 LinearFunction::v_c_coeff_pair & ov_pair = obj->get_v_var();

 Vec_FunctionValue NCoef1( CostMatrix.size() );
 Vec_FunctionValue NCoef2( CostMatrix.size() );

 for( Index i = 0 ; i < CostMatrix.size() ; ++i ) {
  NCoef1[ i ] = CostMatrix[ i ].first;
  NCoef2[ i ] = ov_pair[ i ].second;
  }

 // temporarily put back the original costs into the Objective of the
 // inner Block before deserialising, and then restore the current one,
 // which requires locking it
 // note: one could be tempted to run modify_coefficients() with eNoMod, so
 //       that no Modification at all is issued. this would be OK if the
 //       inner Block only had the abstract representation, since then
 //       changing it is all it is needed to put its state back to the
 //       original one prior serialization. however, doing so would not
 //       update any physical representation of the inner Block, which
 //       would therefore not be serialised correctly
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 bool owned = v_Block.front()->is_owned_by( this );
 if( ( ! owned ) && ( ! v_Block.front()->lock( this ) ) )
  throw( std::logic_error( "cannot lock inner Block" ) );

 // ignore any ensuing Modification: note the horribly dirty trick of
 // explicitly const_casting away const-ness from this in order to be able
 // to (temporarily) change a field of the class inside a const method
 const_cast< LagBFunction * >( this )->f_play_dumb = true;

 // put back the original costs
 obj->modify_coefficients( std::move( NCoef1 ) ,
			   Range( 0 , CostMatrix.size() ) );

 // serialize the sub-block
 netCDF::NcGroup sb = group.addGroup( "B" );
 v_Block.front()->serialize( sb );

 // put back the Lagrangian costs
 obj->modify_coefficients( std::move( NCoef2 )  ,
			   Range( 0 , CostMatrix.size() ) );

 // back to normal operations
 const_cast< LagBFunction * >( this )->f_play_dumb = true;
 if( ! owned )
  v_Block.front()->unlock( const_cast< LagBFunction * >( this ) );  // unlock it

 }  // end( LagBFunction::serialize() )

/*--------------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE LagBFunction ------------*/
/*--------------------------------------------------------------------------*/

bool LagBFunction::has_linearization( const bool diagonal )
{
 // true if the first linearization of the related type exists
 bool newlin = diagonal
  ? v_Block.front()->get_registered_solvers().front()->has_var_solution()
  : v_Block.front()->get_registered_solvers().front()->has_var_direction();

 if( newlin )
  VarSol = diagonal;  // set the type of the solution

 return( newlin );

 }  // end( LagBFunction::has_linearization() )

/*--------------------------------------------------------------------------*/

bool LagBFunction::compute_new_linearization( const bool diagonal )
{
 // true if another linearization of the related type exists
 bool newlin = diagonal
  ? v_Block.front()->get_registered_solvers().front()->new_var_solution()
  : v_Block.front()->get_registered_solvers().front()->new_var_direction();

 if( newlin )
  VarSol = diagonal;  // set the type of the solution

 return( newlin );

 }  // end( LagBFunction::compute_new_linearization() )

/*--------------------------------------------------------------------------*/

void LagBFunction::store_linearization( Index name , ModParam issueMod )
{
 // throw exception if the solution does not exist or has been already stored

 if( std::isnan( LastSolution ) || ( LastSolution < Inf<Index>() ) )
  throw( std::logic_error( "the linearization is unvailable" ) );

 // throw exception if name is greater thatn the dimension of the global pool

 if( name >= g_pool.size() )
  throw( std::logic_error( "max size of global pool exceeded" ) );

 // get the current Solution - - - - - - - - - - - - - - - - - - - - - - - - -
     
 if( g_pool[ name ].first )     // a Solution is already there
  delete g_pool[ name ].first;  // delete it

 // get a "fully loaded" Solution out of the Block, using the default
 // f_solution_Configuration in the f_solution_Configuration
 g_pool[ name ].first = v_Block.front()->get_Solution( nullptr , false );

 // set the solution type- - - - - - - - - - - - - - - - - - - - - - - - - - -

 g_pool[ name ].second = true;

 if( name >= f_max_glob )  // update f_max_glob
  f_max_glob = name + 1;

 // if necessary, issue the Modification - - - - - - - - - - - - - - - - - - -

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;
  
 f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
				         C05FunctionMod::GlobalPoolAdded ,
					 Subset( { name } ) , 0 ,
				         Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LagBFunction::store_linearization( Index ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::store_combination_of_linearizations(
	LinearCombination & coefficients , Index name , ModParam issueMod )
{
 if( name >= g_pool.size() )
  throw( std::logic_error( "max size of global pool already exceed" ) );

 if( coefficients.empty() )
  throw( std::invalid_argument( "the convex combination is empty" ) );

 bool type = true;         // a diagonal one unless oherwise proven
 bool unfeasible = false;  // feasible unless oherwise proven

 // get an "empty" solution from the Block
 p_Solution convex_combination = v_Block.front()->get_Solution();

 for( auto & pair : coefficients ) {
  // add the new term to the convex combination
  convex_combination->sum( g_pool[ pair.first ].first , pair.second );

  // if the convex combination contains even a single direction
  if( ! g_pool[ pair.first ].second )
   type = false;  // then it is a direction
  }

 delete g_pool[ name ].first;  // delete the current Solution (if any)

 g_pool[ name ].first = convex_combination;
 g_pool[ name ].second = type;

 if( name >= f_max_glob )      // update f_max_glob
  f_max_glob = name + 1;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
					C05FunctionMod::GlobalPoolAdded ,
					Subset( { name } ) , 0 ,
					Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LagBFunction::store_convex_combination_of_linearizations )

/*--------------------------------------------------------------------------*/

void LagBFunction::delete_linearization( Index name , ModParam issueMod )
{
 if( ( name >= g_pool.size() ) || ( ! g_pool[ name ].first ) )
  throw( std::invalid_argument( "invalid linearization name" ) );

 delete g_pool[ name ].first;
 g_pool[ name ].first = nullptr;

 update_f_max_glob();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;
  
 f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
				      C05FunctionMod::GlobalPoolRemoved ,
				      Subset( { name } ) , 0 ,
				      Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LagBFunction::delete_linearization )

/*--------------------------------------------------------------------------*/

void LagBFunction::delete_linearizations( Subset && which , bool ordered ,
					  ModParam issueMod )
{
 if( which.empty() ) {  // delete them all
  for( auto & el : g_pool )
   if( el.first ) {
    delete el.first;
    el.first = nullptr;
    }

  f_max_glob = 0;

  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
				      C05FunctionMod::GlobalPoolRemoved ,
				      std::move( which ) , 0 ,
				      Observer::par2concern( issueMod ) ) ,
				 Observer::par2chnl( issueMod ) );
  return;  // all done
  }

 // here, which is not empty, so we have to delete the given subset
 if( ! ordered )
  std::sort( which.begin() , which.end() );

 if( which.back() >= g_pool.size() )
  throw( std::invalid_argument( "invalid linearization name" ) );

 for( auto i : which ) {
  if( ! g_pool[ i ].first )
   throw( std::invalid_argument( "invalid linearization name" ) );

  delete g_pool[ i ].first;
  g_pool[ i ].first = nullptr;
  }

 update_f_max_glob();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;
  
 f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
				      C05FunctionMod::GlobalPoolRemoved ,
				      std::move( which ) , 0 ,
				      Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LagBFunction::delete_linearizations )

/*--------------------------------------------------------------------------*/

int LagBFunction::compute( bool changedvars )
{
 if( v_Block.empty() )  // the Lagrangian function is not well-defined
  return( kError );     // that's clearly an error

 // if no Solver is attached to the inner Block
 if( v_Block.front()->get_registered_solvers().empty() )
  return( kError );     // that's clearly an error
 
 // update the Lagrangian cost vector  - - - - - - - - - - - - - - - - - - - -
 // ... only if the Lagrangian variables have changed, otherwise it is the
 // same as before and need not be re-computed
 if( changedvars ) {
  compute_Lagrangian_costs();  // compute c^y = c + yA
  f_linear_term = NaN;         // force to recompute the linear term yb
  }

 // if necessary, recompute the linear term- - - - - - - - - - - - - - - - - -
 if( std::isnan( f_linear_term ) ) {
  f_linear_term = 0;
  for( const auto & lagdual : LagPairs ) {
   auto lfrel = static_cast< const LinearFunction * >( lagdual.second );
   f_linear_term += lfrel->get_constant_term() * lagdual.first->get_value();
   }
  }
 
 // if some parameters have been changed, set BlockSolverConfig- - - - - - - -
 if( svcc ) {
  svcc->apply( v_Block.front() );
  delete svcc;
  svcc = nullptr;
  }

 // it is assumed that the inner Block (B) does not have Variable defined in
 // other Blocks: then, the re-optimization of (B) can be performed starting
 // from the old solution, i.e., compute( false ) can be called;
 // return the status of the Solver as the status of the LagBFunction

 return( v_Block.front()->get_registered_solvers().front()->compute( false )
	 );

 }  // end( LagBFunction::compute() )

/*--------------------------------------------------------------------------*/

void LagBFunction::get_linearization_coefficients( FunctionValue * g ,
						   Range range , Index name )
{
 range.second = std::min( range.second , get_num_active_var() );
 if( range.second <= range.first )
  return;

 // the solution shall be written in the Variable of the Block - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( name == Inf<Index>() ) {  // the last computed linearization- - - - - - -

  // get solution/direction from the solver
  if( LastSolution != Inf<Index>() ) {  // ... if necessary
   if( VarSol )
    v_Block.front()->get_registered_solvers().front()->get_var_solution();
   else
    v_Block.front()->get_registered_solvers().front()->get_var_direction();

   LastSolution = Inf<Index>();
   }
  }
 else {  // a linearization of the global pool - - - - - - - - - - - - - - - -

  // assign Solution to the sub-Block in such a way the linearization
  // associated with the given name will be retrieved from the global pool

  if( ! g_pool[ name ].first )
   throw( std::logic_error( "invalid linearization name" ) );

  if( LastSolution != name ) {
   g_pool[ name ].first->write( v_Block.front() );
   LastSolution = name;
   }
  }  // end else - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // for each Lagrangian multiplier y_i, the objective value of the relaxed
 // constraint (RCs)_i is the corresponding entry of the linearization
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( Index i = range.first ; i < range.second ; ++i ) {
  LagPairs[ i ].second->compute();
  *(g++) = LagPairs[ i ].second->get_value();
  }
 }  // end( LagBFunction::get_linearization_coefficients( * , range ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::get_linearization_coefficients( FunctionValue * g ,
			      c_Subset & subset , bool ordered , Index name )
{
 // the solution shall be written in the Variable of the Block - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( name == Inf<Index>() ) {  // the last computed linearization- - - - - - -

  // get solution/direction from the solver
  if( LastSolution != Inf<Index>() ) {  // ... if necessary
   if( VarSol )
    v_Block.front()->get_registered_solvers().front()->get_var_solution();
   else
    v_Block.front()->get_registered_solvers().front()->get_var_direction();

   LastSolution = Inf<Index>();
   }
  }
 else {  // a linearization of the global pool - - - - - - - - - - - - - - - -

  // assign Solution to the sub-Block in such a way the linearization
  // associated with the given name will be retrieved from the global pool

  if( ! g_pool[ name ].first )
   throw( std::logic_error( "invalid linearization name" ) );

  if( LastSolution != name ) {
   g_pool[ name ].first->write( v_Block.front() );
   LastSolution = name;
   }
  }  // end else - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // for each Lagrangian multiplier y_i, the objective value of the relaxed
 // constraint (RCs)_i is the corresponding entry of the linearization - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( const auto i : subset ) {
  if( i >= get_num_active_var() )
   throw( std::invalid_argument( "wrong index in subset" ) );
  LagPairs[ i ].second->compute();
  *(g++) = LagPairs[ i ].second->get_value();
  }
 }  // end( LagBFunction::get_linearization_coefficients( * , subset ) )

/*--------------------------------------------------------------------------*/

Function::FunctionValue LagBFunction::get_linearization_constant( Index name )
{
 if( name == Inf<Index>() ) {  // the last computed linearization- - - - - - -

  // get solution/direction from the solver
  if( LastSolution != Inf<Index>() ) {  // ... if necessary
   if( VarSol )
    v_Block.front()->get_registered_solvers().front()->get_var_solution();
   else
    v_Block.front()->get_registered_solvers().front()->get_var_direction();

   LastSolution = Inf<Index>();
   }
  }
 else {  // a linearization of the global pool - - - - - - - - - - - - - - - -

  if( ! g_pool[ name ].first )  // if no such linearization, return NaN
   return( std::numeric_limits<FunctionValue>::quiet_NaN() );

  // assign Solution to the sub-Block in such a way the linearization
  // associated with the given name will be recovered from the global pool

  if( name != LastSolution ) {
   g_pool[ name ].first->write( v_Block.front() );
   LastSolution = name;
   }
  }  // end else - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // return the c x^* for the chosen solution x^*, where c are the *original*
 // costs. this corresponds to the value of the Lagrangian function
 // c x^* + y ( A x^* + b ) = c x^* + y g( x^* ) in y = 0 (in fact, the
 // linear term b is not involved)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 auto alpha = obj->get_constant_term();

 auto & ov_pair = obj->get_v_var();
 #ifndef NDEBUG
  if( ov_pair.size() != CostMatrix.size() )
   throw( std::logic_error( "CostMatrix inconsistent with obj" ) );
 #endif
 for( Index i = 0 ; i < ov_pair.size() ; ++i )
  alpha += ov_pair[ i ].first->get_value() * CostMatrix[ i ].first;

 return( alpha );

 }  // end( LagBFunction::get_linearization_constant )

/*--------------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/

ComputeConfig * LagBFunction::get_ComputeConfig( bool all ,
					       ComputeConfig * ocfg ) const
{
 ComputeConfig* ccfg = ThinComputeInterface::get_ComputeConfig( all , ocfg );

 auto cc = new
     SimpleConfiguration< std::pair< Configuration * , Configuration * > >();

 auto bsc = new RBlockSolverConfig();  // TODO: improve on this
 bsc->get( v_Block.front() );
 cc->f_value.first = bsc;

 auto bc = new OCRBlockConfig();  // TODO: improve on this
 bsc->get( v_Block.front() );
 cc->f_value.second = bsc;

 return( ccfg );

 }  // end( LagBFunction::get_ComputeConfig() )

/*--------------------------------------------------------------------------*/

int LagBFunction::get_NzMat( void )
{
 Index count = 0;
 for( auto & CMj : CostMatrix )
  count += CMj.second.size();

 return( count );
 }

/*--------------------------------------------------------------------------*/

void LagBFunction::get_MatDesc( int *Abeg , int *Aind , double *Aval ,
				int strt , int stp )
{
 Index count = 0;
 for( Index j = 0 ; j < CostMatrix.size() ; ++j ) {
  Abeg[ j ] = count;
  for( auto & CMjs : CostMatrix[ j ].second ) {
   Index xIdx = is_active( CMjs.first );
   if( xIdx >= strt && xIdx < stp ) {
    Aind[ count ] = xIdx;
    Aval[ count++ ] = CMjs.second;
    }
   }
  }

 Abeg[ CostMatrix.size() ] = count;

 } // end( LagBFunction::get_MatDesc() )  - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

int LagBFunction::get_int_par( const idx_type par ) const
{
 switch( par ) {
  case( intLPMaxSz ):
   return( LPMaxSz );
   break;
  case( intGPMaxSz ):
   return( g_pool.size() );
   break;
  default:
   return( C05Function::get_dflt_int_par( par ) ) ;
  }
 }  // end( LagBFunction::get_int_par( idx_type ) )

/*--------------------------------------------------------------------------*/

double LagBFunction::get_dbl_par( const idx_type par ) const
{
 switch( par ) {
  case( dblRAccLin ):
    return( RAccLin );
    break;
  case( dblAAccLin ):
   return( AAccLin );
   break;
  default:
   return( C05Function::get_dflt_dbl_par( par ) ) ;
  }
 }  // end( LagBFunction::get_dbl_par( idx_type ) )

/*--------------------------------------------------------------------------*/
/*
int LagBFunction::get_dflt_int_par( const idx_type par ) const {
 return( C05Function::get_dflt_int_par( par ) ) ;
 }
*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*
double LagBFunction::get_dflt_dbl_par( const idx_type par ) const {
 return( C05Function::get_dflt_dbl_par( par ) ) ;
 }
*/
/*--------------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE LagBFunction ---------*/
/*--------------------------------------------------------------------------*/

ThinVarDepInterface::Index LagBFunction::is_active(
					   const Variable * const var ) const
{
 auto idx = std::find_if( LagPairs.begin() , LagPairs.end() ,
			  [ & var ]( const auto & p )
			           { return( p.first == var ); } );

 return( idx != LagPairs.end() ? std::distance( LagPairs.begin() , idx )
 	                       : Inf< Index >() );

 }  // end( LagBFunction::is_active( Variable* ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::map_active( c_Vec_p_Var & vars , Subset & map ,
			       bool ordered ) const
{
 if( vars.empty() )
  return;

 if( map.size() < vars.size() )
  map.resize( vars.size() );

 if( ordered ) {
  Index found = 0;
  for( Index i = 0 ; i < LagPairs.size() ; ++i ) {
   auto itvi = std::lower_bound( vars.begin() , vars.end() ,
		   LagPairs[ i ].first );
   if( itvi != vars.end() ) {
    map[ std::distance( vars.begin() , itvi ) ] = i;
    ++found;
    }
   }
  if( found < vars.size() )
   throw( std::invalid_argument( "map_active: some Variable is not active" )
	  );
  }
 else {
  auto it = map.begin();
  for( auto var : vars ) {
   Index i = LagBFunction::is_active( var );
   if( i >= LagPairs.size() )
    throw( std::invalid_argument( "map_active: some Variable is not active" )
	   );
   *(it++) = i;
   }
  }
 }  // end( LagBFunction::map_active( Variable* ) )

/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE LagBFunction --------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::print( std::ostream & output ) const
{
 C05Function::print( output );

 output << "LagBFunction [" << this << "]"
	<< " with MaxPoll = ( " << LPMaxSz << " ~ " << g_pool.size() << " ) "
	<< std::endl << " and tol. = ( " << AAccLin << " , " << RAccLin
	<< " ) " << std::endl;

 }  // end( LagBFunction::print() )

/*--------------------------------------------------------------------------*/

void LagBFunction::load( std::istream &input )
{
 input >> LPMaxSz;
 // input >> GPMaxSz;
 input >> AAccLin;
 input >> RAccLin;

 }  // end( LagBFunction::load() )

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::initialize_cost_matrix( void )
{
 LinearFunction::v_c_coeff_pair & rp = obj->get_v_var();

 for( const auto & monomial : rp )
  CostMatrix.push_back( std::make_pair( monomial.second , v_coeff_pair() ) );

 }  // end( LagBFunction::init_lag_matrix() )

/*--------------------------------------------------------------------------*/

void LagBFunction::add_columns( v_dual_pair & v_LagPairsair )
{
 // given a new vector of pairs < y_i , g_i(x) >, that were not a part of
 // LagPairs already, update CostMatrix, which provides the information used
 // to compute the Lagrangian costs. the new g_i(x) may contain some Variable
 // x_j that is not in the Objective of the inner Block already, in which
 // case this is added (and CostMatrix grows by one row)

 LinearFunction::v_coeff_pair toadd;  // new ColVariable to add to obj

 for( const auto & l_pair : v_LagPairsair ) {
  // for each dual pair < y_i , g_i(x) >
  auto lf_rc = dynamic_cast< const LinearFunction * >( l_pair.second );
  if( ! lf_rc )
   throw( std::invalid_argument( "Lagrangian term not a LinearFunction" ) );

  LinearFunction::v_c_coeff_pair & rp = lf_rc->get_v_var();

  for( const auto & monomial : rp ) {
   if( monomial.second == 0 )  // a "fake" monomial
    continue;                  // skip it

   // for each Variable x_j in g_i(x), add the pair < y_i , a_{ij} > to
   // CostMatrix[ j ] (if it exists, otherwise create it)

   // construct the pair < y_i , a_{ij} > to be added to CostMatrix[ j ]
   const auto y_pair = std::make_pair( l_pair.first , monomial.second );

   // find the position of x_j in (obj_B)
   Index i = obj->is_active( monomial.first );

   if( i >= obj->get_num_active_var() ) {
    // the variable x_j is not (yet) in obj, but it may be in toadd
    auto it = std::find_if( toadd.begin() , toadd.end() ,
			    [ & ]( const LinearFunction::coeff_pair & el )
			         { return( el.first == monomial.first ); } );
    if( it == toadd.end() ) {
     // it was not in toadd, it has to be added now
     toadd.push_back( std::pair( monomial.first , FunctionValue( 0 ) ) );
     CostMatrix.push_back( col_pair() );
     CostMatrix.back().first = monomial.second;     // set c_j
     CostMatrix.back().second.push_back( y_pair );  // add < y_i , a_{ij} >
     i = Inf<Index>();
     }
    else
     i = obj->get_num_active_var() + std::distance( toadd.begin() , it );
    }

   if( i < Inf<Index>() ) {
    // x_j was there already in CostMatrix, although possibly not in obj
    // find the place of < y_i , a_{ij} > in A_j
    auto itB = std::lower_bound( CostMatrix[ i ].second.begin() ,
				 CostMatrix[ i ].second.end() ,
				 std::make_pair( l_pair.first , 0 ) ,
				 []( const LinearFunction::coeff_pair & a ,
				     const LinearFunction::coeff_pair & b )
	   	 		   { return( a.first < b.first ); } );
    // add < y_i , a_{ij} > to A_j
    CostMatrix[ i ].second.insert( itB , y_pair );
    }
   }  // end for each monomial
  } // end for each relaxed constraints  - - - - - - - - - - - - - - - - - - -

 if( ! toadd.empty() ) {             // some new Variables have to be added
  bool owned = v_Block.front()->is_owned_by( this );
  if( ( ! owned ) && ( ! v_Block.front()->lock( this ) ) )
   throw( std::logic_error( "cannot lock inner Block" ) );

  f_play_dumb = true;                // ignore any ensuing Modification

  if( toadd.size() == 1 )
   obj->add_variable( toadd.front().first , toadd.front().second );
  else
   obj->add_variables( std::move( toadd ) );

  f_play_dumb = false;               // back to normal operations
  if( ! owned )
   v_Block.front()->unlock( this );  // unlock it
  }
 }  // end( LagBFunction::add_columns() )

/*--------------------------------------------------------------------------*/

void LagBFunction::update_columns( v_dual_pair & v_LagPairsair )
{
 // update CostMatrix which provides the information needed to compute the
 // Lagrangian costs. the method is similar to add_column() [see above]
 // except for the fact that the update_columns( ) can -in addition- change
 // the coefficient a_{ij} and remove the dual variable y_i whose
 // relaxed constraint (RC)_i is not longer active in x_j  - - - - - - - - - -

 Subset nms;  // names of the primal variables x_j that no longer belong to
              // CostMatrix at the end
 LinearFunction::v_coeff_pair toadd;  // new ColVariable to add to obj

 for( const auto & l_pair : v_LagPairsair ) {
  // for each dual pair < y_i , g_i(x) >
  // no need to dynamic_cast since this is only called with elements
  // already in LagPairs where the Function are only LinearFunction
  auto lf_rc = static_cast< const LinearFunction * >( l_pair.second );

  LinearFunction::v_c_coeff_pair & rp = lf_rc->get_v_var();

  for( const auto & monomial : rp ) {
   // construct the pair < y_i , a_{ij} > to be added to CostMatrix[ j ]
   const auto y_pair = std::make_pair( l_pair.first , monomial.second );

   // find the position of x_j in (obj_B)
   Index i = obj->is_active( monomial.first );

   if( i >= obj->get_num_active_var() ) {
    // the variable x_j is not (yet) in obj
    if( monomial.second == 0 )  // a "fake" monomial
     continue;                  // skip it

    // a nonzero monomial, which may be in toadd
    auto it = std::find_if( toadd.begin() , toadd.end() ,
			    [ & ]( const LinearFunction::coeff_pair & el )
			         { return( el.first == monomial.first ); } );
    if( it == toadd.end() ) {
     // it was not in toadd, it has to be added now
     toadd.push_back( std::pair( monomial.first , FunctionValue( 0 ) ) );
     CostMatrix.push_back( col_pair() );
     CostMatrix.back().first = monomial.second;     // set c_j
     CostMatrix.back().second.push_back( y_pair );  // add < y_i , a_{ij} >
     i = Inf<Index>();
     }
    else  // was already in toadd, hence in CostMatrix
     i = obj->get_num_active_var() + std::distance( toadd.begin() , it );
    }

   if( i < Inf<Index>() ) {
    // x_j was there already in CostMatrix, although possibly not in obj
    // find the place of < y_i , a_{ij} > in A_j
    auto itB = std::lower_bound( CostMatrix[ i ].second.begin() ,
				 CostMatrix[ i ].second.end() ,
				 std::make_pair( l_pair.first , 0 ) ,
				 []( const LinearFunction::coeff_pair & a ,
				     const LinearFunction::coeff_pair & b )
	   	 		   { return( a.first < b.first ); } );

    if( itB == CostMatrix[ i ].second.end() ) {
     // there is no term < y_i , a_{ij} > into A_j currently
     if( monomial.second )  // a_{ij} != 0: add < y_i , a_{ij} > to A_j
      CostMatrix[ i ].second.insert( itB , y_pair );
     // else it was not there before and it is not created
     }
    else  // there is a term < y_i , a_{ij} > into A_j currently
     if( monomial.second )                  // a_{ij} != 0
      itB->second = monomial.second;        // modify a_{ij}
     else {                                 // a_{ij} == 0
      CostMatrix[ i ].second.erase( itB );  // remove the term
      if( CostMatrix[ i ].second.empty() )  // if A_j becomes empty 
       nms.push_back( i );                  // mark it for deletion
      }
    }
   }  // end for each monomial
  }  // end for each relaxed constraint- - - - - - - - - - - - - - - - - - - -

 // check for remotion of inactive variables: some variables x_j may become
 // inactive in the contraint (RC)_i, in this case the relative column
 // < y_i, a_{ij}> must be removed from CostMatrix, and their original cost
 // c_j must be restored in obj
 //
 // however, an issue here is that at a certain iteration some A_j may become
 // empty, to be filled afterwards, to be emptied again; while this is very
 // unlikely, nms may contain duplicates. ensure it is not so.

 std::sort( nms.begin() , nms.end() );
 nms.erase( std::unique( nms.begin() , nms.end() ) , nms.end() );

 // ensure that any index in nms actually correspond to an empty A_j
 nms.erase( std::remove_if( nms.begin(), nms.end() ,
			    [ this ]( c_Index i )
			    { return( ! CostMatrix[ i ].second.empty() ); }
			    ) , nms.end() );

 // finally, remove all rows of CostMatrix still in nms, but keep their
 // original cost to be restored in obj
 Vec_FunctionValue NCoef( nms.size() );

 for( Index i = nms.size() ; i-- ; ) {
  NCoef[ i ] = CostMatrix[ nms[ i ] ].first;
  CostMatrix.erase( CostMatrix.begin() + nms[ i ] );
  }

 // if some Variables have to be added or modified, do it now
 if( ( ! toadd.empty() ) || ( ! nms.empty() ) ) {
  bool owned = v_Block.front()->is_owned_by( this );
  if( ( ! owned ) && ( ! v_Block.front()->lock( this ) ) )
   throw( std::logic_error( "cannot lock inner Block" ) );

  f_play_dumb = true;                // ignore any ensuing Modification

  if( ! toadd.empty() ) {  // add the missing Variable (if any)
   if( toadd.size() == 1 )
    obj->add_variable( toadd.front().first , toadd.front().second );
   else
    obj->add_variables( std::move( toadd ) );
   }

  if( ! nms.empty() ) {    // modify the coefficient of no-longer-active x_j
   if( nms.size() == 1 )   // modify one
    obj->modify_coefficient( NCoef.front() , nms.front() );
   else                    // modify a subset (note that nms is ordered)
    obj->modify_coefficients( std::move( NCoef ) , std::move( nms ) , true );
   }

  f_play_dumb = false;               // back to normal operations
  if( ! owned )
   v_Block.front()->unlock( this );  // unlock it
  }
 }  // end( LagBFunction::update_columns() )

/*--------------------------------------------------------------------------*/

void LagBFunction::rm_columns( c_Range & range )
{
 // update the map which provides the information used to compute the
 // Lagrangian costs - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 Vec_FunctionValue NCoef;// primal variables which are no longer
 Subset nms;             //  active in any constraint

 // remove the Lagrangian pairs from the map CostMatrix, in addition
 // if a column < y, A_j> associated to a variable x_j is empty, copy the
 // pointer to that Variable x_j and the coefficient c_j thereof

 for( Index j = 0 ; j < CostMatrix.size() ; j++ ) {
  for( Index i = 0 ; i < CostMatrix[ i ].second.size() && i < range.second ;
       ++i )
   if( i < range.first )
    i++;
   else   // erasing the element
    CostMatrix[ j ].second.erase( CostMatrix[ j ].second.begin() + i );

 if( CostMatrix[ j ].second.empty() ) {
  nms.push_back( j );
  NCoef.push_back( CostMatrix[ j ].first );
  }
 }  // end removal - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // save the original coefficients c_j of he variables x_j which no
 // longer are active in (RCs), write them in (obj_B).  - - - - - - - - - -

 obj->modify_coefficients( std::move( NCoef ) , std::move( nms ) );

 }  // end( LagBFunction::rm_columns() )

/*--------------------------------------------------------------------------*/

void LagBFunction::rm_columns( c_Subset & subset )
{
 // update the map which provides the information used to compute the
 // Lagrangian costs - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 Vec_FunctionValue NCoef;// primal variables which are no longer
 Subset nms;             //  active in any constraint

 // remove the Lagrangian pairs from the map CostMatrix, in addition
 // if a column < y, A_j> associated to a variable x_j is empty, copy the
 // pointer to that Variable x_j and the coefficient c_j thereof

 for( Index j = 0 ; j < CostMatrix.size() ; ++j ) {
  auto itv1 = nms.begin();
  for( Index i = 0 ; i < CostMatrix[ i ].second.size() && itv1 != nms.end()
	; )
   if( i < *itv1 )
    i++;
   else
    if( i == *itv1 ) { // erasing the element,
     CostMatrix[ j ].second.erase( CostMatrix[ j ].second.begin() + i );
     i++;   // the iterator is moved to the next entry
     itv1++;
     }
    else
     itv1++;

  if( CostMatrix[ j ].second.empty() ) {
   nms.push_back( j );
   NCoef.push_back( CostMatrix[ j ].first );
   }
  } // end removal - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // save the original coefficients c_j of he variables x_j which no
 // longer are active in (RCs), write them in (obj_B).  - - - - - - - - - -

 obj->modify_coefficients( std::move( NCoef ) , std::move( nms ) );

 }  // end( LagBFunction::rm_columns() )

/*--------------------------------------------------------------------------*/

void LagBFunction::set_original_costs( c_Subset & subset )
{
 /* The original costs have to be replaced by Lagrangian ones in (obj_B)
    allowing B to be the Lagrangian relaxation sub-problem. To avoid
    to lose the original coefficients of the costs, they have to be
    saved in LagBFunction itself, actually it is done inside CostMatrix. */

 const LinearFunction::v_c_coeff_pair & rp = obj->get_v_var();

 if( subset.empty() )
  for( Index i = 0; i < rp.size() ; ++i )
   CostMatrix[ i ].first = rp[ i ].second;
 else
  for( const auto i : subset )
   CostMatrix[ i ].first = rp[ i ].second;

 }  // end( LagBFunction::set_original_costs( Subset ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::set_original_costs( Range range )
{
 const LinearFunction::v_c_coeff_pair & rp = obj->get_v_var();

 for( Index i = range.first; i < range.second ; ++i )
   CostMatrix[ i ].first = rp[ i ].second;

 }  // end( LagBFunction::set_original_costs( Range ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::compute_Lagrangian_costs( void )
{
 // array of Lagrangian costs c^y = c + yA
 Vec_FunctionValue NCoef( CostMatrix.size() );

 // compute the Lagrangian costs
 for( Index i = 0 ; i < CostMatrix.size() ; ++i ) {
  NCoef[ i ] = CostMatrix[ i ].first;
  for( const auto & el : CostMatrix[ i ].second )
   NCoef[ i ] += el.first->get_value() * el.second;
  }

 // modify the coefficients in the LinearFunction
 obj->modify_coefficients( std::move( NCoef ) ,
			   Range( 0 , CostMatrix.size() ) );

 }  // end( LagBFunction::compute_Lagrangian_costs )

/*--------------------------------------------------------------------------*/

void LagBFunction::guts_of_destructor( void )
{
 // clear() the LagBFunction - - - - - - - - - - - - - - - - - - - - - - - - -

 clear();

 // delete the inner Block - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( ! v_Block.empty() )
  delete v_Block.front();
 v_Block.clear();

 }  // end( LagBFunction::guts_of_destructor )

/*--------------------------------------------------------------------------*/

bool LagBFunction::guts_of_add_Modification( p_Mod mod , ChnlName chnl )
{
 const auto tmod = dynamic_cast< GroupModification * >( mod );
 if( tmod ) {
  // process every Modification inside the GroupModification, returning true
  // if any one of those returned true
  bool to_check = false;
  for( auto & ttmod : tmod->sub_Modifications() )
   if( guts_of_add_Modification( ttmod.get() , chnl ) )
    to_check = true;
  return( to_check );  
  }
 else
  return( guts_of_guts_of_add_Modification( mod , chnl ) );

 }  // end( guts_of_add_Modification )

/*--------------------------------------------------------------------------*/

bool LagBFunction::guts_of_guts_of_add_Modification( p_Mod mod ,
						     ChnlName chnl )
{
 // process Modification - - - - - - - - - - - - - - - - - - - - - - - - - - -
 /* This requires to patiently sift through the possible Modification types
    to find what this Modification exactly is and appropriately react. */

 // C05FunctionModLin: the "linear part" of a Function has been changed
 // C05FunctionModLin can have a special treatment, and therefore need be
 // checked before FunctionMod (because C05FunctionModLin is a FunctionMod)
 // in case they come from the (LinearFunction inside the) Objective of the
 // inner Block, in which case only a part of the original costs is changed
 //
 // If the part that has changed is a Subset of a Range depends on the
 // sub-type of C05FunctionModLin, so two almost identical pieces of code
 // follow, one for each of them.
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // C05FunctionModLinRngd- - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = dynamic_cast< C05FunctionModLinRngd * >( mod );
  if( tmod &&
      ( static_cast< LinearFunction * >( tmod->function() ) == obj ) ) {
   // ... coming from the (LinearFunction inside the) Objective of the
   // inner Block: update the corresponding Range of original costs

   set_original_costs( tmod->range() );

   // issue a C05FunctionMod modification of the type AlphaChanged:
   // the Lagrangian function unpredictably changes (f_shift == NaN), and
   // the constant terms \alpha of the linearizations ( g , \alpha ) have
   // to be computed again by calling get_linearization_constant() since
   // they are c x^*, and c has changed (while g remains unchanged)

   if( f_Observer )
    f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
				  C05FunctionMod::AlphaChanged , Subset() ,
				  FunctionMod::NaNshift , true ) , chnl );
   return( false );
   }
  }  // end C05FunctionModLinRngd- - - - - - - - - - - - - - - - - - - - - - -

 // C05FunctionModLinSbst- - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = dynamic_cast< C05FunctionModLinSbst * >( mod );
  if( tmod &&
      ( static_cast< LinearFunction * >( tmod->function() ) == obj ) ) {
   // ... coming from the (LinearFunction inside the) Objective of the
   // inner Block: update the corresponding Subset of original costs

   set_original_costs( tmod->subset() );

   // issue a C05FunctionMod modification of the type AlphaChanged:
   // the Lagrangian function unpredictably changes (f_shift == NaN), and
   // the constant terms \alpha of the linearizations ( g , \alpha ) have
   // to be computed again by calling get_linearization_constant() since
   // they are c x^*, and c has changed (while g remains unchanged)

   if( f_Observer )
    f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
				  C05FunctionMod::AlphaChanged , Subset() ,
				  FunctionMod::NaNshift , true ) , chnl );
   return( false );
   }
  }  // end C05FunctionModLinSbst- - - - - - - - - - - - - - - - - - - - - - -

 // FunctionMod: a Function has been changed
 // changes in a Function can come from three different components:
 //
 // - the (LinearFunction inside the) Objective of the inner Block, or any
 //   of its sub-Block (recursively)
 //
 // - a LinearFunction that define the Lagrangian term < y_i , g_i( x ) > for
 //   some y
 //
 // - any Constraint in the inner Block, or any of its sub-Block (recursively)
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = dynamic_cast< FunctionMod * >( mod );
  if( tmod ) {
   auto f = tmod->function();  // the Function it comes from

   // let's start considering a Modification from a FRealObjective,
   // either that of the inner Block
   bool fobj = ( static_cast< LinearFunction * >( f ) == obj );

   if( fobj )
    // in this case the new costs have to be stored
    // HUGE DOUBT: WHO IS SAYING THAT ALL COSTS HAVE BEEN CHANGED?
    // SOME COSTS MAY HAVE NOT, AND IF y != 0 YOU CAN FIND THERE THE
    // LAGRANGIAN COSTS INSTEAD OF THE TRUE ONES, ERROEOUSLY MAKING
    // THEM PERMANENT! WE HAVE TO CHECK WHICH COSTS HAVE CHANGED
    set_original_costs();
   else {
    // or, if this does not work, that of a further sub-Block
    // (note that we only deal with FRealObjective)
    auto objobs = dynamic_cast< FRealObjective * >( f->get_Observer() );
    fobj = ( objobs && ( objobs == tmod->get_Block()->get_objective() ) );
    }

   if( fobj ) {  // in either case
    if( ( ! std::isnan( tmod->shift() ) ) &&
	( tmod->shift() < INF ) && ( tmod->shift() > -INF ) ) {
     // a finite shift() == a predictable change == the constant in the
     // Objective has changed from c_0 to c'_0, hence the whole Lagrangian
     // function is shifted by the constant term shift() == c'_0 - c_0,
     // hence issue C05FunctionMod modification of type NothingChanged
     // with the very same shift() == c'_0 - c_0

     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
				    C05FunctionMod::NothingChanged ,
				    Subset() , tmod->shift() , true ) ,
				    chnl );

     }
    else {  // an unpredictable change in the Objective
     // the Objective of the Lagrangian function changes unpredictably, hence
     // issue a C05FunctionMod modification of the type AlphaChanged:
     // the Lagrangian function unpredictably changes (f_shift == NaN), and
     // the constant terms \alpha of the linearizations ( g , \alpha ) have
     // to be computed again by calling get_linearization_constant() since
     // they are c x^*, and c has changed (while g remains unchanged)

     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
				    C05FunctionMod::AlphaChanged ,
				    Subset() , NaN , true ) ,
				    chnl );
     }

    return( false );  // in either case, all is done
    }

   // a second relevant, and entirely different, case is the one where f is
   // one of the LinearFunction defining the Lagrangian term < y_i , g_i(x) >
   // these are easy to spot in that are the only Function whose Observer is
   // directly the LagBFunction

   if( f->get_Observer() == this ) {
    // search for the Lagrangian term which has changed
    auto it_v = std::find_if( LagPairs.begin() , LagPairs.end() ,
		[ f ]( const dual_pair & p ) {
		 return( p.second == static_cast< LinearFunction * >( f ) ); }
			      );
    #ifndef NDEBUG
     if( it_v == LagPairs.end() )
      throw( std::logic_error( "Lagrangian term not found" ) );
    #endif

    Index i = std::distance( LagPairs.begin() , it_v );

    // distinguish between predictable and unpredictable changes
    // shift() == NaN, like in the case of an unpredictable change
    // however, an unpredictable change means that the coefficient vector
    // A_i of the LinearFunction in the Lagrangian term has been modified,
    // and therefore CostMatrix has to be updated to allow LagBFunction the
    // computation of the Lagrangian costs
    if( std::isnan( tmod->shift() ) ||
	( tmod->shift() == INF ) || ( tmod->shift() == -INF ) ) {
     v_dual_pair dp( { *it_v } );
     update_columns( dp );

     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionModRngd>(
			       this , C05FunctionMod::AllEntriesChanged ,
			       Vec_p_Var( { it_v->first } ) ,
			       Range( i , i + 1 ) , Subset() , NaN , true ) ,
				   chnl );
     }
    else {
     // a finite shift() == a predictable change == the constant term b_i of
     // the LinearFunction g_i(x) = A_i x + b_i has changed to b'_i. hence,
     // the i-th entry of all linearizations changes by shift() == b'_i - b_i,
     // which is the perfect case for a C05FunctionModLinRngd
     f_linear_term = NaN;  // since b_i has changed, the linear term has

     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionModLinRngd>(
			     this , Vec_FunctionValue( { tmod->shift() } )  ,
			     Vec_p_Var( { it_v->first } ) ,
			     Range( i , i + 1 ) , NaN , true ) ,
				   chnl );
     }

    return( false );  // the case of changes in the Lagrangian term is over
    }

   // here comes the last and final case: f belongs to some constraint
   // if the Function has changed unpredictably, then there is no way one
   // can guarantee that the previous Solutions have remained feasible
   if( std::isnan( tmod->shift() ) )
    return( true );

   // if the Constraint is a [F]RowConstraint, it is surely not violated
   // if shift() > 0 and RHS == +INF or shift() < 0 and LHS == -INF,
   // otherwise in principle it can be violated and we need to check
   auto cnsobs = dynamic_cast< FRowConstraint * >( f->get_Observer() );
   if( cnsobs )
    return( ( ( tmod->shift() > 0 ) && ( cnsobs->get_rhs() < INF ) ) ||
	    ( ( tmod->shift() < 0 ) && ( cnsobs->get_lhs() > -INF ) ) );

   // this is a Function that has changed in some way we don't understand:
   // take the safe route and re-check feasibility
   return( true );
   }
  } // end FunctionMod - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // FunctionModVars: some Variable have been added/removed from a Function
 // Again, changes in a Function can come from three different components:
 //
 // - the (LinearFunction inside the) Objective of the inner Block, or any
 //   of its sub-Block (recursively)
 //
 // - a LinearFunction that define the Lagrangian term < y_i , g_i( x ) > for
 //   some y
 //
 // - any Constraint in the inner Block, or any of its sub-Block (recursively)
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = dynamic_cast< FunctionModVars * >( mod );
  if( tmod ) {
   auto f = tmod->function();  // the Function it comes from

   // let's start considering a Modification from a FRealObjective,
   // either that of the inner Block
   bool fobj = ( static_cast< LinearFunction * >( f ) == obj );

   if( ! fobj ) {
    // in this case the new costs have to be stored
    // HUGE DOUBT: WHO IS SAYING THAT ALL COSTS HAVE BEEN CHANGED?
    // SOME COSTS MAY HAVE NOT, AND IF y != 0 YOU CAN FIND THERE THE
    // LAGRANGIAN COSTS INSTEAD OF THE TRUE ONES, ERROEOUSLY MAKING
    // THEM PERMANENT! WE HAVE TO CHECK WHICH COSTS HAVE CHANGED

    // variables x_j, for some j, have been added to (remove from) (obj_B)
    // and the new coefficients have to to be rewritten in (deleted from)
    // CostMatrix

    Subset nms( tmod->vars().size() );
    for( Index i = 0 ; i < tmod->vars().size() ; ++i )
     nms[ i ] = obj->is_active( LagPairs[ i ].first );

    set_original_costs( nms );
    }
   else {
    // or, if this does not work, that of a further sub-Block
    // (note that we only deal with FRealObjective)
    auto objobs = dynamic_cast< FRealObjective * >( f->get_Observer() );
    fobj = ( objobs && ( objobs == tmod->get_Block()->get_objective() ) );
    }

   if( fobj ) {  // in either case
    // the Objective of the Lagrangian function changes unpredictably, hence
    // issue a C05FunctionMod modification of the type AlphaChanged:
    // the Lagrangian function unpredictably changes (f_shift == NaN), and
    // the constant terms \alpha of the linearizations ( g , \alpha ) have
    // to be computed again by calling get_linearization_constant() since
    // they are c x^*, and c has changed (while g remains unchanged)

    if( f_Observer )
     f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
				C05FunctionMod::AlphaChanged , Subset( {} ) ,
				FunctionMod::NaNshift , true ) ,
				   chnl );

    return( false );  // in either case, all is done
    }

   // a second relevant, and entirely different, case is the one where f is
   // one of the LinearFunction defining the Lagrangian term < y_i , g_i(x) >
   // these are easy to spot in that are the only Function whose Observer is
   // directly the LagBFunction
   if( f->get_Observer() == this ) {
   // search for the Lagrangian term which has changed
    auto it_v = std::find_if( LagPairs.begin() , LagPairs.end() ,
		[ f ]( const dual_pair & p ) {
		 return( p.second == static_cast< LinearFunction * >( f ) ); }
			      );
    #ifndef NDEBUG
     if( it_v == LagPairs.end() )
      throw( std::logic_error( "Lagrangian term not found" ) );
    #endif

    // the coefficient vector A_i of the LinearFunction in the Lagrangian
    // term has been modified (Variable added/removed), and therefore
    // CostMatrix has to be updated to allow LagBFunction the/ computation
    // of the Lagrangian costs
    v_dual_pair dp( { *it_v } );
    update_columns( dp );

    if( f_Observer ) {
     Index i = std::distance( LagPairs.begin() , it_v );
     f_Observer->add_Modification( std::make_shared<C05FunctionModRngd>(
				   this , C05FunctionMod::AllEntriesChanged ,
				   Vec_p_Var( { it_v->first } ) ,
				   Range( i , i + 1 ) , Subset( {} ) ,
				   NaN , true ) ,
				   chnl );
     }

    return( false );  // the case of changes in the Lagrangian term is over
    }

   // here comes the last and final case: f belongs to some constraint
   // in theory, adding Variable should not violate the Constraint ...
   // but this is only true if, say, the Constraint is linear and the
   // [Col]Variable are allowed to take the value 0. since we have no
   // way of knowing wether or not this is true, we have to assume it is not
   return( true );
   }
  } // end FunctionModVars   - - - - - - - - - - - - - - - - - - - - - - - - -

 // VariableMod: some variables of (B) changed the status
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = dynamic_cast< VariableMod * >( mod );
  if( tmod ) {
   auto xj = dynamic_cast< ColVariable * const >( tmod->variable() );

   if( ! xj )        // unknown variable type
    return( true );  // no clue what is happening, take the worst case

   // if the variable is both free and continuous, the Modification can be
   // ignored
   // THIS IS NOT ENTIRELY CORRECT: THE BOUNDS MAY HAVE CHANGED AND BECOME
   // STRICTER

   return( ( ! xj->is_fixed() ) || ( ! xj->is_integer() ) );
   }
  }  // end VariableMod- - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // BlockModAD - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // BlockModAD: is_variable() && is_added() keep feasibility
 // BlockModAD: ( ! is_variable() ) && ( ! is_added() ) keep feasibility
 // THIS IS CORRECT BASED ON THE DEFINITION OF DYNAMIC Variable/Constraint IN
 // A Block, WHICH STATES THAT:
 // Similarly, dynamic Variable are "there even they are
 // not there": all dynamic Variable not explicitly generated are assumed to
 // be there in the Block set at their default value (most often, zero), and
 // it is assumed that this does not change the fact that the (explicitly
 // constructed part of the) solution is feasible.
 // THUS, GENERATING DYNAMIC Variable CANNOT MAKE A Solution UNFEASIBLE. A
 // FORTIORI NOR CAN DELETING A DYNAMIC Constraint
 {
  const auto tmod = dynamic_cast< BlockModAD * >( mod );
  if( tmod )
   return( ( tmod->is_variable() && ( ! tmod->is_added() ) ) ||
	   ( ( ! tmod->is_variable() ) && tmod->is_added() ) );

  }  // end BlockModAdd- - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // BlockMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = dynamic_cast< BlockMod * >( mod );
  if( tmod )  // arbitrary changes of (B) may violate the feasibility
   return( true );

  }  // end BlockMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 return( false );  // ignore any other Modification (BAD!!)
 // indeed, the safe return value would be true: if I don't understand it,
 // it can wreak arbitrary havok. but this would be severely over-reacting
 // in many cases, so we avoid it for the time being

 }  // end( LagBFunction::guts_of_guts_of_add_Modification )

/*--------------------------------------------------------------------------*/

template< typename par_type >
void LagBFunction::add_par( std::string && name , par_type value )
{
 if( ! svcc ) {
  svcc = new BlockSolverConfig;
  svcc->set_diff( true );
  }

 ComputeConfig * cc;
 auto & solver_configs = svcc->get_SolverConfigs();
 if( solver_configs.empty() ) {
  cc = new ComputeConfig;
  cc->f_diff = true;
  svcc->add_ComputeConfig( "" , cc );
  }
 else
  cc = solver_configs.front();

 cc->set_par( std::move( name ) , value );
 }
 
/*--------------------------------------------------------------------------*/
/*---------------------- End File LagBFunction.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
