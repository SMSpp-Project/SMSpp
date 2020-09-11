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

static const char VarAreDir = 0;   // a direction is stored
static const char VarAreSol = 1;   // a solution is stored

static const char VarToBeChckd = 1;  // Variable must be checked for feasibility

// register MCFBlock to the Block factory
SMSpp_insert_in_factory_cpp_1( LagBFunction );

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

LagBFunction::LagBFunction( Block* innerblock , Observer * const observer )
 :  C05Function()
{

 /* So far, the last computed solution is unknown
  The last computed solution is encoded in the field LastSolution
  as follows:

   - NaN : the last solution is unknown

   - Inf : the last solution is that of the local pool

   - \in [ 0 , GPMaxSz ) : is the index of a solution of the global pool   */

 LastSolution = NaNLinName;

 // set the pointer to the sub-Block (B) - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( innerblock )
  set_inner_block( innerblock );

 // set the observer pointer - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( observer )
  register_Observer( observer );

 } // end ( LagBFunction::LagBFunction( ) )  - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::clear( ) {

 for( const auto & dp : LagPairs )
  delete[] dp.second;
 LagPairs.clear();

 } //end ( LagBFunction::clear( ) )  - - - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::set_inner_block( Block* innerblock ) {

 // set the pointer to the sub-Block (B) - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( v_Block.size() )
  v_Block.clear();

 v_Block.push_back( innerblock );

 // set the objective : the Lagrangian function (obj_B) is the objective of
 // sub-block (B)
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 set_objective_and_solver( );
 initialize_cost_matrix();  // construct CostMatrix whose size is that of
                      // active variables in (obj_B)

 } // end ( LagBFunction::set_inner_block( Block* )  ) - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::set_dual_pairs( v_dual_pair && lp ,
		  ModParam issueMod ) { // this function is used to initialize
	                      // a bunch of relaxed constraints along with their
	                      // Lagrangian multipliers

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
    the second field is the pair < c_j , j-th column >, being the j-th column the
    vector < y, A_j>. It is assumed that map is ordered by the primal
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

 // the vector LagPairs is empty, so initialize it adding the relaxed constraints
 // (RCs)={g_i(x): for some i} - - - - - - - - - - - - - - - - - - - - - - - -
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

void LagBFunction::set_ComputeConfig( ComputeConfig *scfg )
{
 if( ! scfg )
  return;
 
 ThinComputeInterface::set_ComputeConfig( scfg );

 auto cc = dynamic_cast< SimpleConfiguration< std::pair< Configuration * ,
							 Configuration * > >
			 * >( scfg->f_extra_Configuration );
 if( ! cc )
  return;

 auto bsc = dynamic_cast< BlockSolverConfig * >( cc->f_value.first );
 if( bsc )
  bsc->apply( v_Block[ 0 ] );

 auto bc = dynamic_cast< BlockConfig * >( cc->f_value.second );
 if( bc )
  bc->apply( v_Block[ 0 ] );

 }  // end ( LagBFunction::set_relaxed_function( ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::set_par( const idx_type par , const int value )
{
 switch( par ) {
  case( intLPMaxSz ):
   if( LPMaxSz != value ) {
   LPMaxSz = value;
   if( !svcc ) {
    svcc = new BlockSolverConfig;
    ComputeConfig* cc = new ComputeConfig;
    cc->int_pars.push_back(
    		 std::make_pair( int_par_idx2str(intLPMaxSz) , value ) );
    svcc->set_SolverConfigs( { cc } );
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
   GPMaxSz = value;
   if( g_pool.size() > GPMaxSz )
	for( auto it = g_pool.begin() + GPMaxSz ; it != g_pool.end() ; ++it  )
	 delete[] std::get<0>(*it);
   g_pool.resize( GPMaxSz );
   break;
  default: Function::set_par( par , value );
  }
 }  // end ( LagBFunction::set_par( int ) )  - - - - - - - - - - - - - - - - -

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
     svcc->set_SolverConfigs( { cc } );
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
     svcc->set_SolverConfigs( { cc } );
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
 }  // end ( LagBFunction::set_par( double ) )

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

 } // end ( LagBFunction::deserialize )  - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::add_dual_pairs( v_dual_pair && lp , ModParam issueMod ) {

 for( const auto l_pair : lp ) { // for each relaxed constraints
  auto LinFunc = dynamic_cast<const LinearFunction *>( l_pair.second );
  if( LinFunc == nullptr )
   throw( std::logic_error( "the objective is not a linear function" ) );
  }

 /* If not already ordered by ColVariable "name = pointer", sort the vector of
	dual Lagrangian pairs <y_i,g_i(x)=a_i^T x > and update the structure
	CostMatrix which is used to update the Lagrangian cost vector.

	CostMatrix is a map whose the key is the pointer to the primal variable x_j,
	the second field is the pair < c_j , j-th column >, being the j-th column the
	vector < y, A_j>. It is assumed that map is ordered by the primal
	variable name and a column < y, A_j> is ordered by Lagrangian multiplier
	name (= pointer).

	Copy the coefficients c of (obj_B) in CostMatrix in order to allow
    the modifications of the Lagrangian cost vector c^y = c + yA,
    the original costs c will be unavailable unless have been stored
    somewhere, the issue is that -in (obj_B)- vector c must be
    replaced by c^y.  */

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

 // clear lp, because already merged with LagPairs   - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 lp.clear();

 } // end ( LagBFunction::add_dual_pairs( ) )  - - - - - - - - - - - - - - - -

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

 } // end( LagBFunction::remove_variable() ) - - - - - - - - - - - - - - - - -

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
 if( mod->concerns_Block() ) {
  mod->concerns_Block( false );
  guts_of_add_Modification( mod , chnl );
  }

 Block::add_Modification( mod , chnl );

 }  // end( LagBFunction::add_Modification() ) - - - - - - - - - - - - - - - -

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

 auto lf_obj = static_cast<LinearFunction *>( obj->get_function() );
 LinearFunction::v_c_coeff_pair & ov_pair = lf_obj->get_v_var();

 Vec_FunctionValue NCoef1, NCoef2;
 Subset nms1, nms2;

 for( Index i = 0 ; i < CostMatrix.size() ; ++i ) {
  nms1.push_back( i );
  nms2.push_back( i );
  NCoef1.push_back( CostMatrix[i].first );
  NCoef2.push_back( ov_pair[i].second );
  }

 // put the original costs into (obj_B)  - - - - - - - - - - - - - - - - - - -

 lf_obj->modify_coefficients( std::move(NCoef1) , std::move(nms1) );

 // serialize the sub-block  - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 netCDF::NcGroup sb = group.addGroup( "B" );
 v_Block[0]->serialize( sb );

 // put back the Lagrangian costs into (obj_B) - - - - - - - - - - - - - - - -

 lf_obj->modify_coefficients( std::move(NCoef2) , std::move(nms2) );

 } // end( LagBFunction::serialize() ) - - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE LagBFunction ------------*/
/*--------------------------------------------------------------------------*/

bool LagBFunction::has_linearization( const bool diagonal )
{
 bool SlvHasNewLin;  // true if a linearization of the related type exists

 if( diagonal ) {
  SlvHasNewLin = slv->has_var_solution();
  if( SlvHasNewLin )
   VarType = VarAreSol;  // set the type of the solution
  }
 else {
  SlvHasNewLin = slv->has_var_direction();
  if( SlvHasNewLin )
   VarType = VarAreDir;  // set the type of the solution
  }

 return( SlvHasNewLin );

 }  // end LagBFunction::has_linearization( )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

bool LagBFunction::compute_new_linearization( const bool diagonal )
{
 bool SlvHasNewLin; // true if a new linearization of the related type exists
                    // in the local pool

 if( diagonal ) {
  SlvHasNewLin = slv->new_var_solution();
  if( SlvHasNewLin )
   VarType = VarAreSol; // set the type of the solution
  }
 else {
  SlvHasNewLin = slv->new_var_direction();
  if( SlvHasNewLin )
   VarType = VarAreDir; // set the type of the solution
  }

 // one cannot access to the previous solution of the local pool unless
 // no additional solution was produced  - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 return( SlvHasNewLin );

 } // end LagBFunction::compute_new_linearization( ) - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::store_linearization( Index name , ModParam issueMod )
{
 // TODO: handle issueMod !!

 // throw exception if the solution does not exist or has been already stored

 if( std::isnan( LastSolution ) ||
		 LastSolution < Inf<Index>() )
  throw( std::logic_error( "the linearization is unvailable" ) );

 // throw exception if name is greater thatn the dimension of the global pool

 if( name >= GPMaxSz )
  throw( std::logic_error( "the max size of global pool has been already exceed" ) );

 // get the current solution   - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( std::get<0>(g_pool[ name ]) == nullptr )
  std::get<0>(g_pool[ name ]) = v_Block[0]->get_Solution();
 std::get<0>(g_pool[ name ])->read( v_Block[0] );

 // set the solution type  - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( VarType == VarAreSol )
  std::get<1>(g_pool[ name ]) = VarAreSol;
 else
  std::get<1>(g_pool[ name ]) = VarAreDir;

 // the last computed solution is feasible - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 std::get<2>(g_pool[ name ]) = !VarToBeChckd;

 } // end LagBFunction::store_linearization( Index ) - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::delete_linearization( Index name , ModParam issueMod )
{
 // TODO: handle issueMod !!

 if( name >= GPMaxSz )
  throw( std::logic_error( "max size of global pool already exceed" ) );

 if( std::get<0>(g_pool[ name ]) )
  delete[] std::get<0>(g_pool[ name ]);

 } // end LagBFunction::delete_linearization( Index )  - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::store_combination_of_linearizations(
	LinearCombination & coefficients , Index name , ModParam issueMod )
{
 // TODO: handle issueMod !!

 if( name >= GPMaxSz )
  throw( std::logic_error( "max size of global pool already exceed" ) );

 if( coefficients.empty() )
  throw( std::invalid_argument( "the convex combination is empty" ) );

 bool convex_combination_type = VarAreDir;
 bool convex_combination_is_feasible = !VarToBeChckd;

 p_Solution convex_combination = v_Block[0]->get_Solution();
 for( auto & pair : coefficients ) {
  convex_combination->sum( std::get<0>( g_pool[ pair.first ] ) , pair.second );
  if( convex_combination_type == VarAreSol )
   convex_combination_type = VarAreSol;
  if( std::get<2>( g_pool[ pair.first ] ) == VarToBeChckd )
   convex_combination_is_feasible = VarToBeChckd;
  }

 if( std::get<0>(g_pool[ name ]) )
  delete std::get<0>(g_pool[ name ]);
 std::get<0>(g_pool[ name ]) = convex_combination;

 std::get<1>(g_pool[ name ]) = convex_combination_type;
 std::get<2>(g_pool[ name ]) = convex_combination_is_feasible;

 } // end LagBFunction::store_convex_combination_of_linearizations(  ) - - - -

/*--------------------------------------------------------------------------*/

int LagBFunction::compute( bool changedvars )
{
 // update the Lagrangian cost vector  - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( changedvars ) // ?? else do I have to exit ??
  compute_Lagrangian_costs();

 // if some parameters have been changed, set BlockSolverConfig
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( svcc )
  svcc->apply( v_Block[0] );

 delete svcc;

 /* It is assumed that the sub-Block (B) does not have Variable defined
    in other Blocks. Then, the re-optimization of (B) can be performed starting
    from the warm-start (the old solution) and any problem shouldn't occur.
    No relevant Variable are defined in (B). Return the status of the
    optimization process */

 return( slv->compute( false ) );

 } // end ( LagBFunction::compute( ) ) - - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

Function::FunctionValue LagBFunction::get_value( void ) const
{
 FunctionValue objvalue;

 if( obj->get_sense() == Objective::eMax )
  objvalue = slv->get_ub();
 else
  objvalue = slv->get_lb();

 // add zero-linearization - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( objvalue < Inf<FunctionValue>() &&  objvalue > -Inf<FunctionValue>() )
  for( const auto & lagdual : LagPairs ) {
   auto lfrel = static_cast<const LinearFunction *>( lagdual.second );
   objvalue += lfrel->get_constant_term() * (lagdual.first)->get_value();
   }

 return( objvalue );

 } // end ( LagBFunction::get_value( ) ) - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::get_linearization_coefficients( FunctionValue * g ,
						   Range range , Index name )

{
 range.second = std::min( range.second , get_num_active_var() );
 if( range.second <= range.first )
  return;

 // the solution shall be written in the Variable of the Block - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( name == Inf<Index>() ) { // asking for the last computed linearization

  // get solution/direction from the solver  - - - - - - - - - - - - - - - - -
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( LastSolution != Inf<Index>() ) {
   if( VarType == VarAreSol )
    slv->get_var_solution();
   else
    slv->get_var_direction();
   LastSolution = Inf<Index>();
   }

  }
 else {  // asking for a linearization of the global pool  - - - - - - - - - -

  // assign Solution to the sub-Block in such a way the linearization
  // associated with the given name will be retrieved from the global pool

  if( std::get<0>(g_pool[ name ]) == nullptr )
   throw( std::logic_error( "the linearization is not available" ) );

  if( LastSolution != name ) {
   std::get<0>(g_pool[ name ])->write( v_Block[0] );
   LastSolution = name ;
   }

  } // end else  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


 // for each Lagrangian multiplier y_i, the objective value of the relaxed constraint
 // (RCs)_i is the corresponding entry of the linearization - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( Index i = range.first ; i < range.second ; i++ )
   *(g++) = LagPairs[ i ].second->get_value();

 } // end( LagBFunction::get_linearization_coefficients( * , range ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::get_linearization_coefficients( FunctionValue * g ,
						   c_Subset & subset ,
						   const bool ordered ,
						   Index name )

{

 // the solution shall be written in the Variable of the Block - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( name == Inf<Index>() ) { // asking for the last computed
	                                      // linearization   - - - - - - - - -

  // get solution/direction from the solver  - - - - - - - - - - - - - - - - -
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( LastSolution != Inf<Index>() ) {
   if( VarType == VarAreSol )
    slv->get_var_solution();
   else
    slv->get_var_direction();
   LastSolution = Inf<Index>();
   }

  }
 else {  // asking for a linearization of the global pool  - - - - - - - - - -

  // assign Solution to the sub-Block in such a way the linearization
  // associated with the given name will be retrieved from the global pool

  if( std::get<0>(g_pool[ name ]) == nullptr )
   throw( std::logic_error( "the linearization is not available" ) );

  if( LastSolution != name ) {
   std::get<0>(g_pool[ name ])->write( v_Block[0] );
   LastSolution = name ;
   }

  } // end else  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // for each Lagrangian multiplier y_i, the objective value of the relaxed constraint
 // (RCs)_i is the corresponding entry of the linearization - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 c_Index num_active_var = get_num_active_var();
 for( const auto & i : subset ) {
  if( i >= get_num_active_var() )
   throw( std::invalid_argument( "wrong index in subset" ) );
  *(g++) = LagPairs[ i ].second->get_value();
  }

 } // end( LagBFunction::get_linearization_coefficients( * , range ) )

/*--------------------------------------------------------------------------*/

Function::FunctionValue LagBFunction::get_linearization_constant(
		const Index name )
{

 if( name == Inf<Index>() ) {

  // get solution/direction from the solver  - - - - - - - - - - - - - - - - -
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( LastSolution != Inf<Index>() ) {
   if( VarType == VarAreSol )
    slv->get_var_solution();
   else
    slv->get_var_direction();
   LastSolution = Inf<Index>();
   }

  }
 else {  // asking for a linearization of the global pool  - - - - - - - - - -

  // assign Solution to the sub-Block in such a way the linearization
  // associated with the given name will be recovered from the global pool

  if( name != LastSolution ) {

   if( std::get<0>(g_pool[ name ]) == nullptr )
    throw( std::logic_error( "the linearization is not available" ) );

   std::get<0>(g_pool[ name ])->write( v_Block[0] );
   LastSolution = name ;
   }

  // if the solution must be checked and is proved to be not feasible, the method
  // returns Inf

  if( std::get<2>(g_pool[ name ]) == VarToBeChckd )
   if( ( ( std::get<1>(g_pool[ name ]) == VarAreSol ) && (!v_Block[0]->is_feasible( ) ) )
	|| ( ( std::get<1>(g_pool[ name ]) == VarAreDir ) && (!v_Block[0]->is_unbounded( ) ) ) )
    return( Inf< Function::FunctionValue >() );

  }

 // return the constant c^Tx + c_0 unless the solution is no longer feasible
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 auto lfobj = static_cast<LinearFunction *>( obj->get_function() );
 Function::FunctionValue alpha = lfobj->get_constant_term();

 // in CostMatrix the coefficient of the variable is the Lagrangian cost
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LinearFunction::v_c_coeff_pair & ov_pair = lfobj->get_v_var();
 for( auto itb = ov_pair.begin() ; itb != ov_pair.end() ; itb++ )
  alpha += (itb->first)->get_value() * (itb->second);

 return( alpha );

 } // end( LagBFunction::get_linearization_constant() )  - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

Block* LagBFunction::get_inner_block( void ) const {
 return( v_Block[0] );
 }

/*--------------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/

ComputeConfig * LagBFunction::get_ComputeConfig( bool all ,
		ComputeConfig * ocfg ) const {

 ComputeConfig* ccfg = ThinComputeInterface::get_ComputeConfig( all , ocfg );

 auto cc = new
     SimpleConfiguration< std::pair< Configuration * , Configuration * > >();

 auto bsc = new RBlockSolverConfig();  // TODO: improve on this
 bsc->get( v_Block[ 0 ] );
 cc->f_value.first = bsc;

 auto bc = new OCRBlockConfig();  // TODO: improve on this
 bsc->get( v_Block[ 0 ] );
 cc->f_value.second = bsc;

 return( ccfg );

 }  // end( LagBFunction::get_ComputeConfig() )

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int LagBFunction::get_NzMat( void ) {

 Index count = 0;
 for( Index j = 0 ; j < CostMatrix.size() ; j++ )
  for( auto it = CostMatrix[j].second.begin() ; it != CostMatrix[j].second.end()  ; )
   count += CostMatrix[j].second.size();

 return( count );
 } // end( LagBFunction::get_NzMat() )  - - - - - - - - - - - - - - - - - - -

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void LagBFunction::get_MatDesc( int *Abeg , int *Aind , double *Aval , const int strt ,
		   int stp ) {

 Index count = 0;
 for( Index j = 0 ; j < CostMatrix.size() ; j++ ) {
  Abeg[ j ] = count;
  for( auto it = CostMatrix[j].second.begin() ; it != CostMatrix[j].second.end()  ; ) {
   Index xIdx = is_active( it->first );
   if( xIdx >= strt && xIdx < stp ) {
	Aind[ count ] = xIdx;
	Aval[ count++ ] = it->second;
    }
   }
  }
 Abeg[ CostMatrix.size() ] = count;

 } // end( LagBFunction::get_MatDesc() )  - - - - - - - - - - - - - - - - - -

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int LagBFunction::get_int_par( const idx_type par ) const {

 switch( par ) {
  case( intLPMaxSz ):
   return( RAccLin );
   break;
  case( intGPMaxSz ):
   return( RAccLin );
   break;
  default:
   return( C05Function::get_dflt_int_par( par ) ) ;
  }

 } // end( LagBFunction::get_int_par( idx_type ) )  - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

double LagBFunction::get_dbl_par( const idx_type par ) const {

 switch( par ) {
  case( dblRAccLin ):
    return( RAccLin );
    break;
  case( intGPMaxSz ):
   return( RAccLin );
   break;
  default:
   return( C05Function::get_dflt_dbl_par( par ) ) ;
  }

 } // end( LagBFunction::get_dbl_par( idx_type ) ) - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

int LagBFunction::get_dflt_int_par( const idx_type par ) const
{
 return( C05Function::get_dflt_int_par( par ) ) ;
 } // end( LagBFunction::get_dflt_int_par( idx_type ) ) - - - - - - - - - - -

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

double LagBFunction::get_dflt_dbl_par( const idx_type par ) const
{
 return( C05Function::get_dflt_dbl_par( par ) ) ;
 } // end( LagBFunction::get_dflt_dbl_par( idx_type ) ) - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE LagBFunction ---------*/
/*--------------------------------------------------------------------------*/

ThinVarDepInterface::Index LagBFunction::is_active( const Variable * const var )
const
{
 auto idx = std::find_if( LagPairs.begin() , LagPairs.end() ,
				  [ & var ]( const auto & p ) -> bool {
				    return( p.first == var );
				    } );

 return( idx != LagPairs.end() ? std::distance( LagPairs.begin(), idx )
 	                       : Inf< Index >() );

 } // end( LagBFunction::is_active( Variable* ) )  - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::map_active( c_Vec_p_Var & vars , Subset & map ,
				 const bool ordered ) const
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
 }  // end( LagBFunction::map_active( Variable* ) )  - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE LagBFunction --------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::print( std::ostream &output ) const {

 C05Function::print( output );

 output << "LagBFunction [" << this << "]"
 	 << " with MaxPoll = ( " << LPMaxSz << " ~ " << GPMaxSz << " ) ";
 output << std::endl << " and tol. = ( " << AAccLin << " , " << RAccLin
		<< " ) " << std::endl;

 } // end LagBFunction::print( ) - - - - - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::load( std::istream &input ) {

 input >> LPMaxSz;
 input >> GPMaxSz;
 input >> AAccLin;
 input >> RAccLin;

 } // end LagBFunction::load( ) - - - - - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::initialize_cost_matrix( void ) {

 auto lf_obj = static_cast<LinearFunction *>( obj->get_function() );
 LinearFunction::v_c_coeff_pair & rp = lf_obj->get_v_var();

 for( const auto & monomial : rp )
  CostMatrix.push_back( std::make_pair( monomial.second , v_coeff_pair() ) );

 } // end ( LagBFunction::init_lag_matrix() )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

LagBFunction::Subset LagBFunction::add_columns( v_dual_pair & v_LagPairsair )
{
 // update CostMatrix which provides the information used to compute the
 // Lagrangian costs - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 Index var_name;
 std::set<Index> XToAdd; // this array is used to indicate the
                         // x_j to add to (obj_B)

 auto lf_obj = static_cast<LinearFunction *>( obj->get_function() );

 for( const auto & l_pair : v_LagPairsair ) { // read the dual vector LagPairs,
	          // getting the relaxed constraint <a_i,x> and the dual variable
	          // y_i thereof

  auto lf_rc = static_cast<const LinearFunction *>( l_pair.second );
  LinearFunction::v_c_coeff_pair & rp = lf_rc->get_v_var();

  for( const auto & monomial : rp ) { // for each Variable x_j
                        // of the relaxed constraint, add the pair
	                    // < y_i , a_{ij} > to CostMatrix

   // construct the pair of the form < y_i , a_{ij} > to be added to
   // the related column of x_j  - - - - - - - - - - - - - - - - - - - - - - - -

   const auto y_pair = std::make_pair( l_pair.first , monomial.second );

   // find the position of x_j in (obj_B), that is the position of x_j
   // in CostMatrix - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   var_name = lf_obj->is_active( monomial.first );

   if( var_name >= lf_obj->get_num_active_var() ) { // a new variable x_j
	                                             // was inserted   - - - - - - -

    var_name = lf_obj->get_num_active_var();
    lf_obj->add_variable( monomial.first , LinearFunction::Coefficient(0) , true );

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
   else { // variable x_j already existed - - - - - - - - - - - - - - - - - - -

	auto itB = std::lower_bound( (CostMatrix[ var_name ].second).begin() ,
	   			  (CostMatrix[ var_name ].second).end() , std::make_pair( l_pair.first , 0 ) ,
	   	 		  []( const LinearFunction::coeff_pair &a ,
	   	 		  const LinearFunction::coeff_pair &b )
	   	 		  { return( a.first < b.first ); } );

    (CostMatrix[ var_name ].second).insert( itB , y_pair ); // add < y_i , a_{ij} >
    }

   }
  } // end for each relaxed constraints  - - - - - - - - - - - - - - - - - - -


 Subset vars(XToAdd.begin(), XToAdd.end());
 std::sort( vars.begin() , vars.end() ,
	  []( const auto & p1, const auto & p2 ) { return( p1 < p2 ); } );

 return( vars );

 } // end ( LagBFunction::add_columns() )  - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

LagBFunction::Subset LagBFunction::update_columns( v_dual_pair & v_LagPairsair )
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

 auto lf_obj = static_cast<LinearFunction *>( obj->get_function() );

 for( const auto & l_pair : v_LagPairsair ) { // read the dual vector LagPairs,
	          // getting the relaxed constraint <a_i,x> and the dual variable
	          // y_i thereof

  auto lf_rc = static_cast<const LinearFunction *>( l_pair.second );
  LinearFunction::v_c_coeff_pair & rp = lf_rc->get_v_var();

  for( const auto & monomial : rp ) { // for each Variable x_j
                        // of the relaxed constraint, add the pair
	                    // < y_i , a_{ij} > to CostMatrix

   // construct the pair of the form < y_i , a_{ij} > to be added to
   // the related column of x_j  - - - - - - - - - - - - - - - - - - - - - - - -

   const auto y_pair = std::make_pair( l_pair.first , monomial.second );

   // find the position of x_j in (obj_B), that is the position of x_j
   // in CostMatrix - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   var_name = lf_obj->is_active( monomial.first );

   if( var_name >= lf_obj->get_num_active_var() ) { // a new variable x_j
	                                             // was inserted   - - - - - - -

	lf_obj->add_variable( monomial.first , LinearFunction::Coefficient(0) , true );
	var_name = lf_obj->is_active( monomial.first );

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
   else { // variable x_j already existed - - - - - - - - - - - - - - - - - - -

	auto itB = std::lower_bound( (CostMatrix[ var_name ].second).begin() ,
	   	   	  (CostMatrix[ var_name ].second).end() , std::make_pair( l_pair.first , 0 ) ,
	   	   	  []( const LinearFunction::coeff_pair &a ,
	   	   	  const LinearFunction::coeff_pair &b )
	   	   	  { return( a.first < b.first ); } );

	if( itB->first == y_pair.first )
	 itB->second = y_pair.second;
	else
     (CostMatrix[ var_name ].second).insert( itB , y_pair ); // add < y_i , a_{ij} >

    }

   }
  } // end for each relaxed constraints  - - - - - - - - - - - - - - - - - - -

 // check for remotion of inactive variables: some variables
 // x_j may become inactive in the contraint (RC)_i, in this case the relative
 // column < y_i, a_{ij}> must be removed from CostMatrix, in addition
 // if a column < y, A_j> associated to a variable x_j is empty, copy the pointer
 // to that Variable x_j and the coefficient c_j thereof - - - - - - - - - -

 for( Index i = 0 ; i < CostMatrix.size() ; i++ ) {

  const Variable * xvar = lf_obj->get_active_var( i );
  for( auto it = CostMatrix[i].second.begin() ; it != CostMatrix[i].second.end()  ; ) {

   auto itB = std::lower_bound( LagPairs.begin() ,
		        LagPairs.end() , std::make_pair( it->first , nullptr ) ,
		 		[]( const dual_pair &a , const dual_pair &b )
		 		{ return( a.first < b.first ); } );


   if( xvar->is_active( itB->second ) >= xvar->get_num_active() )
    CostMatrix[i].second.erase( it );
   else
    it++;
   }

  if( CostMatrix[i].second.empty() ) {
   nms.push_back( i );
   NCoef.push_back( CostMatrix[i].first );
   }

  }

 // save the original coefficients c_j of the variables x_j which no
 // longer are active in (RCs), write them in (obj_B).  - - - - - - - - - -

 lf_obj->modify_coefficients( std::move(NCoef) , std::move(nms) );

 Subset vars( XToAdd.begin(), XToAdd.end() );
 std::sort( vars.begin() , vars.end() ,
	  []( const auto & p1, const auto & p2 ) { return( p1 < p2 ); } );

 return( vars );

 } // end ( LagBFunction::update_columns() )  - - - - - -  - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::rm_columns( c_Range & range ) {

 // update the map which provides the information used to compute the
 // Lagrangian costs - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 Vec_FunctionValue NCoef;// primal variables which are no longer
 Subset nms;             //  active in any constraint

 auto lf_obj = static_cast<LinearFunction *>( obj->get_function() );

 // remove the Lagrangian pairs from the map CostMatrix, in addition
 // if a column < y, A_j> associated to a variable x_j is empty, copy the pointer
 // to that Variable x_j and the coefficient c_j thereof - - - - - - - - - -

for( Index j = 0 ; j < CostMatrix.size() ; j++ ) {
 for( Index i = 0 ; i < CostMatrix[i].second.size() && i < range.second ; ++i)
  if( i < range.first )
   i++;
  else   // erasing the element
   CostMatrix[j].second.erase( CostMatrix[j].second.begin() + i );

 if( CostMatrix[j].second.empty() ) {
  nms.push_back( j );
  NCoef.push_back( CostMatrix[j].first );
  }

 } // end remotion - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // save the original coefficients c_j of he variables x_j which no
 // longer are active in (RCs), write them in (obj_B).  - - - - - - - - - -

 lf_obj->modify_coefficients( std::move(NCoef) , std::move(nms) );

 } // end ( LagBFunction::rm_columns() )  - - - - - - - -  - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::rm_columns( c_Subset & subset )
{

 // update the map which provides the information used to compute the
 // Lagrangian costs - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 Vec_FunctionValue NCoef;// primal variables which are no longer
 Subset nms;             //  active in any constraint

 auto lf_obj = static_cast<LinearFunction *>( obj->get_function() );

 // remove the Lagrangian pairs from the map CostMatrix, in addition
 // if a column < y, A_j> associated to a variable x_j is empty, copy the pointer
 // to that Variable x_j and the coefficient c_j thereof - - - - - - - - - -

 for( Index j = 0 ; j < CostMatrix.size() ; j++ ) {
  auto itv1 = nms.begin();
  for( Index i = 0 ; i < CostMatrix[i].second.size() && itv1 != nms.end() ; )
   if( i < *itv1 )
    i++;
   else
	if( i == *itv1 ) { // erasing the element,
	 CostMatrix[j].second.erase( CostMatrix[j].second.begin() + i );
	 i++;   // the iterator is moved to the next entry
	 itv1++;
	 }
	else
	 itv1++;

  if( CostMatrix[j].second.empty() ) {
   nms.push_back( j );
   NCoef.push_back( CostMatrix[j].first );
   }

  } // end remotion - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // save the original coefficients c_j of he variables x_j which no
 // longer are active in (RCs), write them in (obj_B).  - - - - - - - - - -

 lf_obj->modify_coefficients( std::move(NCoef) , std::move(nms) );

 } // end ( LagBFunction::rm_columns() ) - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::set_objective_and_solver( )
{
 assert( v_Block[ 0 ]->get_objective() );  // ... which must exist

 // the objective function of the inner block must be linear - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 obj = dynamic_cast<FRealObjective *>( v_Block[0]->get_objective() );
 if( obj == nullptr )
  throw( std::logic_error( "the objective is not a real function" ) );

 auto LFInnBlck = dynamic_cast<LinearFunction *>( (obj)->get_function() );
 if( LFInnBlck == nullptr )
  throw( std::logic_error( "the objective is not a linear function" ) );

 // get the Solver of the sub-Block  - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 Solver* slv = v_Block[0]->get_registered_solvers().back();
 assert( ! slv );

 } // end ( LagBFunction::set_objective_and_solver( ) )  - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::set_original_costs( c_Subset & subset )
{
 /* The original costs have to be replaced by Lagrangian ones in (obj_B)
	allowing B to be the Lagrangian relaxation sub-problem. To avoid
	to lose the original coefficients of the costs, they have to be
	saved in LagBFunction itself, actually it is done inside CostMatrix. */

 auto lf_obj = static_cast<LinearFunction *>( obj->get_function() );
 LinearFunction::v_c_coeff_pair & rp = lf_obj->get_v_var();

 if( subset.size() )
  for( const auto idx : subset )
   CostMatrix[idx].first = rp[idx].second;
 else
  for( Index i = 0; i < rp.size() ; i++ )
   CostMatrix.push_back( std::make_pair( rp[i].second , v_coeff_pair() ) );

 } // end ( LagBFunction::set_original_costs( c_Vec_p_Var ) )  - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::compute_Lagrangian_costs( )
{
 // get the objective function pointer of the inner block   - - - - - - - - -

 auto lf_obj = static_cast<LinearFunction *>( obj->get_function() );

 Vec_FunctionValue NCoef( CostMatrix.size() ); // this array is created
 Subset nms( CostMatrix.size() );     // to change the pairs in (obj_B)

 for( Index i = 0 ; i < CostMatrix.size() ; ++i ) {

  nms[ i ] = i;
  NCoef[ i ] = CostMatrix[i].first;
  for( const auto & el : CostMatrix[i].second )
   NCoef[ i ] -= el.first->get_value() * el.second;

  } // end for - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 lf_obj->modify_coefficients( std::move(NCoef) , std::move(nms) );

 } // end ( LagBFunction::update_function() )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::guts_of_destructor( )
{
 // delete the Function objects  - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 clear();

 // clear the map handling the data structure to compute linearizations and
 // updating the Lagrangian cost vector  - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 CostMatrix.clear();

 // delete the global pool - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( auto tpl : g_pool )
  delete[] std::get<0>(tpl);
 g_pool.clear();

 // delete the inner Block - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( ! v_Block.empty() )
  delete v_Block[ 0 ];
 v_Block.clear();

 } // end ( LagBFunction::guts_of_destructor() ) - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::guts_of_add_Modification( sp_Mod mod , ChnlName chnl )
{
 // process abstract Modification- - - - - - - - - - - - - - - - - - - - - - -
 /* This requires to patiently sift through the possible Modification types
    to find what this Modification exactly is and appropriately mirror the
    changes to the "abstract representation" to the "physical one". */

 // FunctionMod: a function has been changed, three kinds of functions have
 // to be taken into account : (obj_B), (RCs) and the constraints of (B), all
 // of these are assumed to be linear
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<FunctionMod>( mod );
  if( tmod ) {
   auto lfmod = static_cast<LinearFunction * const>( tmod->function() );
   if( !lfmod )
	throw( std::logic_error( "the function must be linear" ) );

   // let's start considering a modification of (obj_B) - - - - - - - - - - -

   auto lfobj = static_cast<LinearFunction *>( obj->get_function() );

   // distinguish between predictable and unpredictable changes, take into account
   // that because of the linearity of the function a predictable change
   // has to involve the constant term only and all the coefficients remain unchanged,
   // the function shall be shifted - - - - - - - - - - - - - - - - - - - - -

   if( lfmod == lfobj ) // if the Linear Function is (obj_B)
    if( !std::isnan( tmod->shift() ) &&
	( tmod->shift() < FunctionMod::INFshift &&
	  tmod->shift() > -FunctionMod::INFshift ) ) { // a predictable change

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

      Function::Vec_FunctionValue delta( {lfmod->get_constant_term()} );

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
    else { // the changes of the constraints of (B) may violate the
     // solutions kept in the global pool, irrespectively to the fact that the
     // changes are of the type "predictable" or "unpredictable" and  signal
     // that the feasibility of the solutions has to be checked

     for( auto tpl : g_pool )
      std::get<2>( tpl ) = VarToBeChckd;

     // issue C05FunctionMod modification of the type AlphaChanged: the
     // feasible region of (B) has been changed, then the original linearizations
     // (even the g part) can no longer be used
     //!! check if C05FunctionMod::AlphaChanged is still right
     // TODO: check if a better which is possible
     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
					      C05FunctionMod::AlphaChanged ,
					      Subset( {} ) ,
					      FunctionMod::NaNshift ) ,
				    chnl );
     }
   }
  } // end FunctionMod - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // C05FunctionModLin: a linear part of a function has been changed
 // three kinds of functions have to be taken into account :
 // (obj_B), (RCs) and the constraints of (B), all
 // of these are assumed to be linear
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<C05FunctionModLin>( mod );
  if( tmod ) {
   auto lfmod = static_cast<LinearFunction * const>( tmod->function() );
   if( !lfmod )
	throw( std::logic_error( "the function must be linear" ) );

   // let's start considering a modification of (obj_B) - - - - - - - - - - -

   auto lf_obj = static_cast<LinearFunction *>( obj->get_function() );

   if( lfmod == lf_obj ) { // if the Linear Function is (obj_B)

    // the coefficients c_j of variables v_vars, changed and have to
    // to be rewritten in CostMatrix - - - - - - - - - - - - - - - - - - - - -

	Subset nms( tmod->vars().size() );
	for( Index i = 0 ; i < tmod->vars().size() ; ++i )
	 nms[ i ] = lf_obj->is_active( LagPairs[ i ].first );

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
    else { // the changes of the constraints of (B) may violate the
     // solutions kept in the global pool, irrespectively to the fact that the
     // changes are of the type "predictable" or "unpredictable" and  signal
     // that the feasibility of the solutions has to be checked

     for( auto tpl : g_pool )
      std::get<2>( tpl ) = VarToBeChckd;

     // issue C05FunctionMod modification of the type AlphaChanged: the
     // feasible region of (B) has been changed, then the original linearizations
     // (even the g part) can no longer be used
     //!! check if C05FunctionMod::AlphaChanged is still right
     // TODO: check if a better which is possible
     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
					      C05FunctionMod::AlphaChanged ,
					      Subset( {} ) ,
					      FunctionMod::NaNshift ) ,
				    chnl );
     }
   }
  } // end C05FunctionModLin - - - - - - - - - - - - - - - - - - - - - - - - -

 // FunctionModVars: some x variables have been added/removed
 // three kinds of functions have to be taken into account :
 // (obj_B), (RCs) and the constraints of (B), all
 // of these are assumed to be linear
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<FunctionModVars>( mod );
  if( tmod ) {
   auto lfmod = static_cast<LinearFunction * const>( tmod->function() );
   if( !lfmod )
   throw( std::logic_error( "the function must be linear" ) );

   // because of the linearity of the function the modification must be
   // quasi-additive one  and the shift is zero - - - - - - - - - - - - - - -
   if( tmod->shift() != 0 )
    throw( std::logic_error( "the function must be linear" ) );

   auto lf_obj = static_cast<LinearFunction *>( obj->get_function() );
   if( lfmod == lf_obj ) { // if the Linear Function is (obj_B)

	// variables x_j, for some j, have been added to (remove from) (obj_B)
	// and the new coefficients have to to be rewritten in (deleted from) CostMatrix

    Subset nms( tmod->vars().size() );
    for( Index i = 0 ; i < tmod->vars().size() ; ++i )
     nms[ i ] = lf_obj->is_active( LagPairs[ i ].first );

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
    else { // the changes of the constraints of (B) may violate the
       // solutions kept in the global pool,  signal
       // that the feasibility of the solutions has to be checked

       for( auto tpl : g_pool )
        std::get<2>( tpl ) = VarToBeChckd;

       // issue C05FunctionMod modification of the type AlphaChanged: the
       // feasible region of (B) changed and the original linearizations
       // (even the g part) can no longer be used
       //!! check if C05FunctionMod::AlphaChanged is still right
       // TODO: check if a better which is possible
       if( f_Observer )
        f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
						C05FunctionMod::AlphaChanged ,
						Subset( {} ) ,
						FunctionMod::NaNshift ) ,
				      chnl );
     }
   }
  } // end FunctionModVars   - - - - - - - - - - - - - - - - - - - - - - - - -

 // VariableMod: some variables of (B) changed the status
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<VariableMod>( mod );
  if( tmod ) {
   auto xj = dynamic_cast<ColVariable * const>( tmod->variable() );

   // if the variable is both free and continuous, the modification can be
   // ignored  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   if( ( ! xj->is_fixed() ) && ( xj->get_type() == ColVariable::kContinuous ) )
    return;

   // the new status of the Variable may violate the feasibility of the global
   // pool, signal that the fesibility of the solutions must be checked

   for( auto tpl : g_pool )
    std::get<2>( tpl ) = VarToBeChckd;

   // issue C05FunctionMod modification of the type AlphaChanged: the
   // feasible region of (B) changed and the original linearizations
   // (even the g part) can no longer be used
   //!! check if C05FunctionMod::AlphaChanged is still right
   // TODO: check if a better which is possible
   if( f_Observer )
    f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
						C05FunctionMod::AlphaChanged ,
						Subset( {} ) ,
					        FunctionMod::NaNshift ) ,
				  chnl );
   }
  }  // end VariableMod- - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // BlockModAD - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // BlockModAD: is_variable() && is_added() keep feasibility
 // BlockModAD: ( ! is_variable() ) && ( ! is_added() ) keep feasibility
 {
  const auto tmod = std::dynamic_pointer_cast< BlockModAD >( mod );
  if( tmod ) {
   if( ( tmod->is_variable() && ( ! tmod->is_added() ) ) ||
       ( ( ! tmod->is_variable() ) && tmod->is_added() ) ) {
    // the remotion of variables and addition of constraints of (B) may
    // violate
    // the feasibility of the global pool, signal that the feasibility of the
    // solutions must be checked  - - - - - - - - - - - - - - - - - - - - - -

    for( auto tpl : g_pool )
     std::get<2>( tpl ) = VarToBeChckd;

    // issue C05FunctionMod modification of the type AlphaChanged: the
    // feasible region of (B) changed and the original linearizations
    // (even the g part) can no longer be used
    //!! check if C05FunctionMod::AlphaChanged is still right
    // TODO: check if a better which is possible

    if( f_Observer )
     f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
						C05FunctionMod::AlphaChanged ,
						Subset( {} ) ,
						FunctionMod::NaNshift ) ,
				   chnl );
    }
   }
  }  // end BlockModAdd- - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // BlockMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<BlockMod>( mod );
  if( tmod ) {


   // the changes of (B) may violate the feasibility
   // of the global pool, signal that the feasibility of the solutions must
   // be checked   - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   for( auto tpl : g_pool )
    std::get<2>( tpl ) = VarToBeChckd;

   // issue C05FunctionMod modification of the type AlphaChanged: the
   // feasible region of (B) changed and the original linearizations
   // (even the g part) can no longer be used
   //!! check if C05FunctionMod::AlphaChanged is still right
   // TODO: check if a better which is possible

    if( f_Observer )
     f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
						C05FunctionMod::AlphaChanged ,
						Subset( {} ) ,
						FunctionMod::NaNshift ) ,
				   chnl );
   }
  }  // end BlockMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 }  // end( LagBFunction::guts_of_add_Modification( sp_Mod ) ) - - - - - - - -

/*--------------------------------------------------------------------------*/
/*---------------------- End File LagBFunction.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
