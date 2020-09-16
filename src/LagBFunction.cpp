/*--------------------------------------------------------------------------*/
/*------------------------ File LagBFunction.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the LagBFunction class, which is derived from
 * both a C05Function and a Block. The class is an interface for a
 * Lagrangian function.
 *
 * \version 0.07
 *
 * \date 15 - 07 - 2020
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

#include "LagBFunction.h"

#include "Observer.h"

#include "RBlockConfig.h"

#include "SMSTypedefs.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using SimpleConfig_p_p = SimpleConfiguration<
                            std::pair< Configuration * , Configuration * > >;

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
    LPMaxSz( 0 ) , RAccLin( 0 ) , AAccLin( 0 ) , svcc( nullptr )
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
 for( const auto & dp : LagPairs )
  delete dp.second;
 LagPairs.clear();
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

 for( const auto l_pair : lp ) { // for each relaxed constraints
  auto LinFunc = dynamic_cast<const LinearFunction *>( l_pair.second );
  if( LinFunc == nullptr )
   throw( std::logic_error( "the objective is not a linear function" ) );
  }

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
    if( ! svcc ) {
     svcc = new BlockSolverConfig;
     ComputeConfig* cc = new ComputeConfig;
     cc->int_pars.push_back(
    		 std::make_pair( int_par_idx2str( intLPMaxSz ) , value ) );
     svcc->add_ComputeConfig( "" , cc );
     }
    else {
     auto & solver_configs = svcc->get_SolverConfigs();

     if( solver_configs.empty() )
      throw( std::logic_error( "LagBFunction::set_par: BlockSolverConfig "
                               "has no ComputeConfig" ) );

     auto & sc = solver_configs[ 0 ];

     auto it_v = std::find_if( sc->int_pars.begin() , sc->int_pars.end() ,
    	 [ ]( const std::pair< std::string , int > & p ) {
    	      return( p.first == "intLPMaxSz" );  } );

     if( it_v != sc->int_pars.end() )
      (*it_v).second = value;
     else
      sc->int_pars.push_back( std::make_pair( "intLPMaxSz" , value ) );
     }
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
    if( !svcc ) {
     svcc = new BlockSolverConfig;
     ComputeConfig* cc = new ComputeConfig;
     cc->dbl_pars.push_back( std::make_pair( dbl_par_idx2str( dblRAccLin ) ,
                                             value ) );
     svcc->add_ComputeConfig( "" , cc );
     }
    else {
     auto & solver_configs = svcc->get_SolverConfigs();

     if( solver_configs.empty() )
      throw( std::logic_error( "LagBFunction::set_par: BlockSolverConfig "
                               "has no ComputeConfig" ) );

     auto & sc = solver_configs[ 0 ];

     auto it_v = std::find_if( sc->dbl_pars.begin() , sc->dbl_pars.end() ,
    	 [ ]( const std::pair< std::string , double > & p ) {
    	      return( p.first == "dblRAccLin" );  } );

     if( it_v != sc->dbl_pars.end() )
      (*it_v).second = value;
     else
      sc->dbl_pars.push_back( std::make_pair( "dblRAccLin" , value ) );
     }
    }
   break;
  case( dblAAccLin ):
   if( AAccLin != value ) {
    AAccLin = value;
    if( !svcc ) {
     svcc = new BlockSolverConfig;
     ComputeConfig* cc = new ComputeConfig;
     cc->dbl_pars.push_back( std::make_pair( dbl_par_idx2str( dblAAccLin ) ,
                                             value ) );
     svcc->add_ComputeConfig( "" , cc );
     }
    else {
     auto & solver_configs = svcc->get_SolverConfigs();

     if( solver_configs.empty() )
      throw( std::logic_error( "LagBFunction::set_par: BlockSolverConfig "
                               "has no ComputeConfig" ) );

     auto & sc = solver_configs[ 0 ];

     auto it_v = std::find_if( sc->dbl_pars.begin() , sc->dbl_pars.end() ,
     	 [ ]( const std::pair< std::string , double > & p ) {
     	      return( p.first == "dblAAccLin" );  } );

     if( it_v != sc->dbl_pars.end() )
      (*it_v).second = value;
     else
      sc->dbl_pars.push_back( std::make_pair( "dblAAccLin" , value ) );
     }
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
 for( const auto l_pair : lp ) { // for each relaxed constraints
  auto LinFunc = dynamic_cast<const LinearFunction *>( l_pair.second );
  if( LinFunc == nullptr )
   throw( std::logic_error( "the objective is not a linear function" ) );
  }

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

 Vec_FunctionValue NCoef1, NCoef2;
 Subset nms1, nms2;

 for( Index i = 0 ; i < CostMatrix.size() ; ++i ) {
  nms1.push_back( i );
  nms2.push_back( i );
  NCoef1.push_back( CostMatrix[ i ].first );
  NCoef2.push_back( ov_pair[ i ].second );
  }

 // put the original costs into (obj_B)  - - - - - - - - - - - - - - - - - - -

 obj->modify_coefficients( std::move( NCoef1 ) , std::move( nms1 ) );

 // serialize the sub-block  - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 netCDF::NcGroup sb = group.addGroup( "B" );
 v_Block.front()->serialize( sb );

 // put back the Lagrangian costs into (obj_B) - - - - - - - - - - - - - - - -

 obj->modify_coefficients( std::move( NCoef2 ) , std::move( nms2 ) );

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

  f_linear_term = 0;           // meanwhile also compute the linear term yb
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

LagBFunction::Subset LagBFunction::add_columns( v_dual_pair & v_LagPairsair )
{
 // update CostMatrix which provides the information used to compute the
 // Lagrangian costs - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 Index var_name;
 std::set< Index > XToAdd;  // which x_j to add to (obj_B)

 for( const auto & l_pair : v_LagPairsair ) {
  // read the dual vector LagPairs, getting the relaxed constraint <a_i,x>
  // and the dual variable y_i thereof

  auto lf_rc = static_cast<const LinearFunction *>( l_pair.second );
  LinearFunction::v_c_coeff_pair & rp = lf_rc->get_v_var();

  for( const auto & monomial : rp ) {
   // for each Variable x_j of the relaxed constraint, add the pair
   // < y_i , a_{ij} > to CostMatrix

   // construct the pair of the form < y_i , a_{ij} > to be added to
   // the related column of x_j  - - - - - - - - - - - - - - - - - - - - - -

   const auto y_pair = std::make_pair( l_pair.first , monomial.second );

   // find the position of x_j in (obj_B), that is the position of x_j
   // in CostMatrix - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   var_name = obj->is_active( monomial.first );

   if( var_name >= obj->get_num_active_var() ) {
    // a new variable x_j was inserted   - - - - - - -

    var_name = obj->get_num_active_var();
    obj->add_variable( monomial.first , LinearFunction::Coefficient( 0 ) ,
		       true );

    /* The original costs have to be replaced by Lagrangian ones in (obj_B)
   	allowing B to be the Lagrangian relaxation sub-problem. To avoid to
        lose the original coefficients of the costs, they have to be saved
	in LagBFunction itself, actually it is done inside CostMatrix. */

    CostMatrix.push_back( col_pair() );
    auto itA = CostMatrix.back();
    itA.first = monomial.second; // set c_j
    itA.second.push_back( y_pair );  // add < y_i , a_{ij} >

    XToAdd.insert( var_name );
    }
   else {  // variable x_j already existed- - - - - - - - - - - - - - - - - -

    auto itB = std::lower_bound( CostMatrix[ var_name ].second.begin() ,
				 CostMatrix[ var_name ].second.end() ,
				 std::make_pair( l_pair.first , 0 ) ,
				 []( const LinearFunction::coeff_pair &a ,
				     const LinearFunction::coeff_pair &b )
	   	 		   { return( a.first < b.first ); } );

    // add < y_i , a_{ij} >
    CostMatrix[ var_name ].second.insert( itB , y_pair );
    }
   }
  } // end for each relaxed constraints  - - - - - - - - - - - - - - - - - - -

 Subset vars(XToAdd.begin(), XToAdd.end());
 std::sort( vars.begin() , vars.end() ,
	  []( const auto & p1, const auto & p2 ) { return( p1 < p2 ); } );

 return( vars );

 }  // end( LagBFunction::add_columns() )

/*--------------------------------------------------------------------------*/

LagBFunction::Subset LagBFunction::update_columns(
						v_dual_pair & v_LagPairsair )
{
 // update CostMatrix which provides the information needed to compute the
 // Lagrangian costs, the method is similar to add_column() [see above ]
 // except for the fact that the update_columns( ) can -in addition- change
 // the coefficient a_{ij} and remove the dual variable y_i whose
 // relaxed constraint (RC)_i is not longer active in x_j  - - - - - - - - - -

 Index var_name;
 std::set<Index> XToAdd; // this array is used to indicate the
                         // x_j to add to (obj_B)

 Vec_FunctionValue NCoef;// primal variables which are no longer
 Subset nms;             //  active in any constraint

 for( const auto & l_pair : v_LagPairsair ) {
  // read the dual vector LagPairs, getting the relaxed constraint <a_i,x>
  // and the dual variable y_i thereof

  auto lf_rc = static_cast<const LinearFunction *>( l_pair.second );
  LinearFunction::v_c_coeff_pair & rp = lf_rc->get_v_var();

  for( const auto & monomial : rp ) { // for each Variable x_j
   // of the relaxed constraint, add the pair < y_i , a_{ij} > to CostMatrix

   // construct the pair of the form < y_i , a_{ij} > to be added to
   // the related column of x_j  - - - - - - - - - - - - - - - - - - - - - -

   const auto y_pair = std::make_pair( l_pair.first , monomial.second );

   // find the position of x_j in (obj_B), that is the position of x_j
   // in CostMatrix - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   var_name = obj->is_active( monomial.first );

   if( var_name >= obj->get_num_active_var() ) {
    // a new variable x_j was inserted   - - - - - - -

    obj->add_variable( monomial.first , LinearFunction::Coefficient( 0 ) ,
		       true );
    var_name = obj->is_active( monomial.first );

    /* The original costs have to be replaced by Lagrangian ones in (obj_B)
       allowing B to be the Lagrangian relaxation sub-problem. To avoid
       to lose the original coefficients of the costs, they have to be
       saved in LagBFunction itself, actually it is done inside CostMatrix. */

    CostMatrix.push_back( col_pair() );
    auto itA = CostMatrix.back();
    itA.first = monomial.second; // set c_j
    itA.second.push_back( y_pair );  // add < y_i , a_{ij} >

    XToAdd.insert( var_name );
    }
   else {  // variable x_j already existed - - - - - - - - - - - - - - - - - -
    auto itB = std::lower_bound( CostMatrix[ var_name ].second.begin() ,
				 CostMatrix[ var_name ].second.end() ,
				 std::make_pair( l_pair.first , 0 ) ,
				 []( const LinearFunction::coeff_pair &a ,
				     const LinearFunction::coeff_pair &b )
				   { return( a.first < b.first ); } );

    if( itB->first == y_pair.first )
     itB->second = y_pair.second;
    else  // add < y_i , a_{ij} >
     CostMatrix[ var_name ].second.insert( itB , y_pair );
    }
   }
  } // end for each relaxed constraints  - - - - - - - - - - - - - - - - - - -

 // check for remotion of inactive variables: some variables x_j may become
 // inactive in the contraint (RC)_i, in this case the relative column
 // < y_i, a_{ij}> must be removed from CostMatrix, in addition if a column
 // < y, A_j> associated to a variable x_j is empty, copy the pointer
 // to that Variable x_j and the coefficient c_j thereof - - - - - - - - - -

 for( Index i = 0 ; i < CostMatrix.size() ; i++ ) {
  const Variable * xvar = obj->get_active_var( i );
  for( auto it = CostMatrix[ i ].second.begin() ;
       it != CostMatrix[ i ].second.end() ; ) {

   auto itB = std::lower_bound( LagPairs.begin() , LagPairs.end() ,
				std::make_pair( it->first , nullptr ) ,
		 		[]( const dual_pair &a , const dual_pair &b )
		 		  { return( a.first < b.first ); } );

   if( xvar->is_active( itB->second ) >= xvar->get_num_active() )
    CostMatrix[ i ].second.erase( it );
   else
    it++;
   }

  if( CostMatrix[ i ].second.empty() ) {
   nms.push_back( i );
   NCoef.push_back( CostMatrix[ i ].first );
   }
  }

 // save the original coefficients c_j of the variables x_j which no
 // longer are active in (RCs), write them in (obj_B).  - - - - - - - - - -

 obj->modify_coefficients( std::move( NCoef ) , std::move( nms ) );

 Subset vars( XToAdd.begin(), XToAdd.end() );
 std::sort( vars.begin() , vars.end() ,
	  []( const auto & p1, const auto & p2 ) { return( p1 < p2 ); } );

 return( vars );

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

 LinearFunction::v_c_coeff_pair & rp = obj->get_v_var();

 if( ! subset.empty() )
  for( const auto idx : subset )
   CostMatrix[ idx ].first = rp[ idx ].second;
 else
  for( Index i = 0; i < rp.size() ; i++ )
   CostMatrix.push_back( std::make_pair( rp[ i ].second , v_coeff_pair() ) );

 }  // end( LagBFunction::set_original_costs( c_Vec_p_Var ) )

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
 // delete the Function objects  - - - - - - - - - - - - - - - - - - - - - - -

 clear();

 // clear the map handling the data structure to compute linearizations and
 // updating the Lagrangian cost vector- - - - - - - - - - - - - - - - - - - -

 CostMatrix.clear();

 // delete the global pool - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( Index i = 0 ; i < f_max_glob ; ++i )
  delete g_pool[ i ].first;
 g_pool.clear();

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

 // FunctionMod: a function has been changed, three kinds of functions have
 // to be taken into account : (obj_B), (RCs) and the constraints of (B), all
 // of these are assumed to be linear
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = dynamic_cast< FunctionMod * >( mod );
  if( tmod ) {
   auto lfmod = static_cast< LinearFunction * const >( tmod->function() );
   if( ! lfmod )
    throw( std::logic_error( "the function must be linear" ) );

   // let's start considering a modification of (obj_B) - - - - - - - - - - -

   // distinguish between predictable and unpredictable changes, take into
   // account that because of the linearity of the function a predictable
   // change has to involve the constant term only and all the coefficients
   // remain unchanged, the function shall be shifted

   if( lfmod == obj ) // if the Linear Function is (obj_B)
    if( ! std::isnan( tmod->shift() ) &&
	( tmod->shift() < INF && tmod->shift() > -INF ) ) {
     // a predictable change
     // the Lagrangian function (obj_B) is shifted by the constant term
     // c'_0 - c_0

     // issue C05FunctionMod modification of the type AlphaChanged: (obj_B)
     // changes and the new form of the Lagrangian function is
     // c'{^y}(x) = c{^y}x + c'_0 - c_0, the computation of (obj_B) can be
     // obtained just adding c'_0 - c_0 without re-evaluating it, however
     // the constant terms of the linearizations have to be computed again
     //	by calling get_linearization_constant()

     //!! check if C05FunctionMod::AlphaChanged is still right
     // TODO: check if better which is possible
     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
					  C05FunctionMod::AlphaChanged ,
					  Subset( {} ) , tmod->shift() , 0 ) ,
				    chnl );

     }
    else { // an unpredictable change

     // the coefficients c_j, for some j, changed and have to
     // to be rewritten in CostMatrix - - - - - - - - - - - - - - - - - - - - -

     set_original_costs(); // no variable is added/removed

     // issue C05FunctionMod modification of the type AlphaChanged:
     // the Lagrangian function unpredictably changes,
     // f_shift has to be set to NaN, however the constant terms \alpha of the
     // linearizations (g, \alpha) have to be computed again by calling
     // get_linearization_constant() while the g remains unchanged

     //!! check if C05FunctionMod::AlphaChanged is still right
     // TODO: check if better which is possible
     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
				 C05FunctionMod::AlphaChanged , Subset( {} ) ,
				 FunctionMod::NaNshift , 0 ) ,
				    chnl );

     }
   else
    if( lfmod->get_Observer() == this ) { // the Linear Function belongs
    	                       // to (RCs), search for the index of the function
    	                       // which has changed  - - - - - - - - - - - - -

     auto it_v = std::find_if( LagPairs.begin() , LagPairs.end() ,
    		 [ lfmod ]( const dual_pair & p ) {
    		  return( p.second == lfmod );  } );

     // take out the multiplier y_i which has been modified  - - - - - - - - -
     //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

     v_dual_pair vdp( {*it_v} );
     Vec_p_Var v_vars( {vdp[ 0 ].first} );

     // distinguish between predictable changes and unpredictable ones - - - -

     if( !std::isnan( tmod->shift() ) &&
	 ( tmod->shift() < FunctionMod::INFshift &&
	   tmod->shift() > -FunctionMod::INFshift ) ) { // a predictable change

      // the constant term of the function (RCs)_i is b'_i and b_i the
      // previous value thereof, this means that the Lagrangian function
      // changes the linear part y^i b_i --> y^_i b'_i - - - - - - - - - - - -

      Vec_FunctionValue delta( {lfmod->get_constant_term()} );

      // issue C05FunctionModLin modification: the Lagrangian
      // function (obj_B) has changed in an "unpredictably",
      // f_shift has to be set to NaN, what is changed? the linear part:
      // (obj_B) -> (obj_B) + y_i( b'_i - b_i )

      if( f_Observer )
       f_Observer->add_Modification( std::make_shared<C05FunctionModLin>( this ,
    		   std::move(delta) , std::move(v_vars) ,
			   FunctionMod::NaNshift , 0 ) , chnl );

      }
     else { // an unpredictable change

      // the coefficient vector a_i of the function (RCs)_i have been
      // modified  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      // however, the the Lagrangian costs should be updated as follows:
      // c^y = c + y_i a_i x + sum_{p \neq i} y_p a_p x
      // a_i has to be re-written in CostMatrix to allow LagBFunction the
      // computation of the Lagrangian costs

   	  Subset nms = update_columns( vdp );

      // issue C05FunctionModSbst modification of the type AllEntriesChanged:
   	  // the i-th entry of the linearization ( g , \alpha ) has to change,
   	  // namely g_i must be re-computed -by calling
   	  // get_linearization_coefficients- at the index i-th, however the
   	  // Lagrangian function changes in an unpredictable  way and f_shift id
   	  // set to NaN  - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	  // TODO: check if a better which is possible
   	  if( f_Observer )
   	   f_Observer->add_Modification( std::make_shared<C05FunctionModSbst>(
			        this , C05FunctionModSbst::AllEntriesChanged ,
				std::move( v_vars ) , std::move( nms ) , true,
				Subset( {} ) , FunctionMod::NaNshift , 0 ) ,
					 chnl );
      }
     }
    else   // the changes of the constraints of (B) may violate the
           // Solution kept in the global pool, irrespectively to the fact
           // that the changes are "predictable" or "unpredictable"
     return( true );

   return( false );
   }
  } // end FunctionMod - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // C05FunctionModLin: a linear part of a function has been changed
 // three kinds of functions have to be taken into account :
 // (obj_B), (RCs) and the constraints of (B), all
 // of these are assumed to be linear
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = dynamic_cast< C05FunctionModLin * >( mod );
  if( tmod ) {
   auto lfmod = static_cast< LinearFunction * const >( tmod->function() );
   if( ! lfmod )
    throw( std::logic_error( "the function must be linear" ) );

   // let's start considering a modification of (obj_B) - - - - - - - - - - -

   if( lfmod == obj ) { // if the Linear Function is (obj_B)

    // the coefficients c_j of variables v_vars, changed and have to
    // to be rewritten in CostMatrix - - - - - - - - - - - - - - - - - - - - -

	Subset nms( tmod->vars().size() );
	for( Index i = 0 ; i < tmod->vars().size() ; ++i )
	 nms[ i ] = obj->is_active( LagPairs[ i ].first );

    set_original_costs( nms );

    // issue C05FunctionMod modification of the type AlphaChanged:
    // the Lagrangian function unpredictably changes and
    // f_shift has to be set to NaN, however the constant terms \alpha of the
    // linearizations (g, \alpha) have to be computed again by calling
    // get_linearization_constant() while g remains unchanged

     //!! check if C05FunctionMod::AlphaChanged is still right
    // TODO: check if a better which is possible
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
					      C05FunctionMod::AlphaChanged ,
					      Subset( {} ) ,
					      FunctionMod::NaNshift , 0 ) ,
				   chnl );
    }
   else
    if( lfmod->get_Observer() == this ) { // the Linear Function belongs
    	                       // to (RCs), search for the index of the function
    	                       // which has changed  - - - - - - - - - - - - -

     auto it_v = std::find_if( LagPairs.begin() , LagPairs.end() ,
    		 [ lfmod ]( const dual_pair & p ) {
    		  return( p.second == lfmod );  } );

     // take out the multiplier y_i which has been modified  - - - - - - - - -
     //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

     v_dual_pair vdp( {*it_v} );
     Vec_p_Var v_vars( {vdp[ 0 ].first} );

     // the coefficient vector a_i of the function (RCs)_i have been modified

     // however, the the Lagrangian costs should be updated as follows:
     // c^y = c + y_i a_i x + sum_{p \neq i} y_p a_p x
     // a_i has to be re-written in CostMatrix to allow LagBFunction the
     // computation of the Lagrangian costs

     Subset nms = update_columns( vdp );

     // issue C05FunctionModSbst modification of the type AllEntriesChanged:
     // the i-th entry of the linearization ( g , \alpha ) has to change,
     // namely g_i must be re-computed -by calling
     // get_linearization_coefficients- at the index i-th, however the
     // Lagrangian function unpredictably changes and f_shift is
     // set to NaN   - - - - - - - - - - - - - - - - - - - - - - - - - -

     // TODO: check if a better which is possible
     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionModSbst>( this ,
   	           C05FunctionModSbst::AllEntriesChanged , std::move( v_vars ) ,
		   std::move( nms ) , true , Subset( {} ) ,
		   FunctionMod::NaNshift , 0 ) ,
				    chnl );
     }
    else  // the changes of the constraints of (B) may violate the
           // Solution kept in the global pool, irrespectively to the fact
           // that the are "predictable" or "unpredictable"
     return( true );

   return( false );
   }
  } // end C05FunctionModLin - - - - - - - - - - - - - - - - - - - - - - - - -

 // FunctionModVars: some x variables have been added/removed
 // three kinds of functions have to be taken into account :
 // (obj_B), (RCs) and the constraints of (B), all
 // of these are assumed to be linear
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = dynamic_cast< FunctionModVars * >( mod );
  if( tmod ) {
   auto lfmod = static_cast< LinearFunction * const >( tmod->function() );
   if( ! lfmod )
   throw( std::logic_error( "the function must be linear" ) );

   // because of the linearity of the function the modification must be
   // quasi-additive one  and the shift is zero - - - - - - - - - - - - - - -
   if( tmod->shift() != 0 )
    throw( std::logic_error( "the function must be linear" ) );

   if( lfmod == obj ) { // if the Linear Function is (obj_B)

    // variables x_j, for some j, have been added to (remove from) (obj_B)
    // and the new coefficients have to to be rewritten in (deleted from) CostMatrix

    Subset nms( tmod->vars().size() );
    for( Index i = 0 ; i < tmod->vars().size() ; ++i )
     nms[ i ] = obj->is_active( LagPairs[ i ].first );

    set_original_costs( nms );

    // issue C05FunctionMod modification of the type AlphaChanged:
    // the Lagrangian function unpredictably changes and
    // f_shift has to be set to NaN, however the constant terms of the
    // linearizations have to be computed again by calling
    // get_linearization_constant()

    //!! check if C05FunctionMod::AlphaChanged is still right
    // TODO: check if a better which is possible
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
						C05FunctionMod::AlphaChanged ,
						Subset( {} ) ,
					        FunctionMod::NaNshift , 0 ) ,
				   chnl );
    }
   else
    if( lfmod->get_Observer() == this ) { // if the Linear Function belongs
      	                       // to (RCs) search for the index of the function
      	                       // which has changed  - - - - - - - - - - - - -

     auto it_v = std::find_if( LagPairs.begin() , LagPairs.end() ,
      		 [ lfmod ]( const dual_pair & p ) {
      		  return( p.second == lfmod );  } );

   	 // take out the multiplier y_i which has been modified  - - - - - - - - -
     //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

     v_dual_pair vdp( {*it_v} );
     Vec_p_Var v_vars( {vdp[ 0 ].first} );

     // variables x_j, for some j, have been added in (remove from) (RCs)_i
     // and their relative entries have to to be rewritten in (deleted from)
     // CostMatrix

     // however, the the Lagrangian costs should be updated as follows:
     // c^y = c + y_i a_i x + sum_{p \neq i} y_p a_p x
     // a_i has to be re-written in CostMatrix to allow LagBFunction the
     // computation of the Lagrangian costs

     Subset nms = update_columns( vdp );

     // issue C05FunctionModSbst modification of the type AllEntriesChanged:
     // the i-th entry of the linearization ( g , \alpha ) has to change,
     // namely g_i must be re-computed -by calling
     // get_linearization_coefficients- at the index i-th, however the
     // Lagrangian function unpredictably changes and f_shift is
     // set to NaN   - - - - - - - - - - - - - - - - - - - - - - - - - -
    // TODO: check if a better which is possible

     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionModSbst>( this ,
       	           C05FunctionModSbst::AllEntriesChanged , std::move( v_vars ) ,
		   std::move( nms ) , true , Subset( {} ) ,
		   FunctionMod::NaNshift , 0 ) ,
				    chnl );
     }
    else  // the changes of the constraints of (B) may violate the
          // Solution kept in the global pool
     return( true );

   return( false );
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

   // if the variable is both free and continuous, the modification can be
   // ignored  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   return( ( ! xj->is_fixed() ) || ( ! xj->is_integer() ) );
   }
  }  // end VariableMod- - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // BlockModAD - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // BlockModAD: is_variable() && is_added() keep feasibility
 // BlockModAD: ( ! is_variable() ) && ( ! is_added() ) keep feasibility
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
/*---------------------- End File LagBFunction.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
