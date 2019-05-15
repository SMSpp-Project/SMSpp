/*--------------------------------------------------------------------------*/
/*------------------------ File LagBFunction.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the LagBFunction class, which is derived from
 * both a C05Function and a Block. The class is an interface for a
 * Lagrangian function.
 *
 * \version 0.04
 *
 * \date 15 - 05 - 2019
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
 * Copyright &copy by Antonio Frangioni, Enrico Gorgone
 *
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Observer.h"
#include "SMSTypedefs.h"
#include "LagBFunction.h"
#include <math.h>

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using SimpleConfig_p_p = SimpleConfiguration<
                            std::pair< Configuration * , Configuration * > >;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

static constexpr C05Function::LinearizationName NaNLinName
         = std::numeric_limits<C05Function::LinearizationName>::quiet_NaN();

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

 /* set a bunch of relaxed constraints (RCs)={g_i(x): for some i},
    each constraint is a dual pairs <y_i,g_i(x)> where g_i is the constraint
    and y_i is the Lagrangian multiplier thereof.

    Because relaxed during the construction phase these relaxed constraints
    are the *static* ones.

 // if( v_lag_pair.size() )
 //  set_dual_pairs( std::move( v_lag_pair ) , static_is_ordered );
  *
 */

 } // end ( LagBFunction::LagBFunction( ) )  - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::clear( ) {

 for( const auto & lagdual : lag_p )
  delete[] lagdual.second;
 lag_p.clear();

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

 v_Block[0] = innerblock;

 // set the objective : the Lagrangian function (obj_B) is the objective of
 // sub-block (B)
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 set_objective_and_solver( );

 /* if the *static* relaxed constraints <y, g(x)> have been accommodated
    in sequel do the following steps:

    (i)  copy the coefficients c of (obj_B) in LagMatrix in order to allow
         the modifications of the Lagrangian cost vector c^y = c + yA,
         the original costs c will be unavailable unless have been stored
         somewhere, the issue is that -in (obj_B)- vector c must be
         replaced by c^y

    (ii) since some variables, say ZeroVars, may be active in (RCs) but
         *not* active in (obj_B), the same number of coeff_pair as the
         number of ZeroVars have to be added to (obj_B) with zero-coefficient
         and Variables pointers defined by ZeroVars */

 if( lag_p.size() ) {
  set_original_costs();   // step (i)
  fix_sblock_objective(); // step (ii)
  }

 } // end ( LagBFunction::set_inner_block( Block* )  ) - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::set_dual_pairs( v_dual_pair && v_lag_pair ,
		 const bool static_is_ordered , c_ModParam issueMod ) { // this function is used to initialize
	                      // a bunch of relaxed constraints along with their
	                      // Lagrangian multipliers

 clear();

 /* If not already ordered by ColVariable "name = pointer", sort the vector of
    dual Lagrangian pairs <y_i,g_i(x)=a_i^T x > and construct the structure
    LagMatrix which is used to update the Lagrangian cost vector.

    LagMatrix is a map whose the key is the pointer to the primal variable x_j,
    the second field is the pair < c_j , j-th column >, being the j-th column the
    vector < y, A_j>. It is assumed that map is ordered by the primal
    variable name and a column < y, A_j> is ordered by Lagrangian multiplier
    name (= pointer).

    As of now, set c_j = 0 for all j.  */

 for( const auto lagdual : v_lag_pair ) { // for each relaxed constraints
  auto LinFunc = dynamic_cast<const LinearFunction *>( lagdual.second );
  if( LinFunc == nullptr )
   throw( std::logic_error( "the objective is not a linear function" ) );
  }

 if( ! static_is_ordered )
  std::sort( v_lag_pair.begin() , v_lag_pair.end() ,
	  []( const auto & p1, const auto & p2 ) { return( p1.first < p2.first ); } );

 add_columns( v_lag_pair );

 /* if the sub-Block has been already defined, in sequel do the following steps:

    (i)  copy the coefficients c of (obj_B) in LagMatrix in order to allow
         the modifications of the Lagrangian cost vector c^y = c + yA,
         the original costs c will be unavailable unless have been stored
         somewhere, the issue is that -in (obj_B)- vector c must be
         replaced by c^y

    (ii) since some variables, say ZeroVars, may be active in (RCs) but
         *not* active in (obj_B), the same number of coeff_pair as the
         number of ZeroVars have to be added to (obj_B) with zero-coefficient
         and Variables pointers defined by ZeroVars */

 if( v_Block.size() ) {
  set_original_costs();
  fix_sblock_objective();
  }

 // the vector lag_p is empty, so initialize it adding the relaxed constraints
 // (RCs)={g_i(x): for some i} - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 lag_p = std::move( v_lag_pair );

 if( f_Observer && f_Observer->issue_mod( issueMod ) )
  issue_add_variables_modification( lag_p , issueMod );

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
 auto cc = dynamic_cast<const SimpleConfig_p_p *>( scfg );
 if( cc == nullptr )
  throw( std::logic_error( "the configuration is not a SimpleConfiguration" ) );

 ThinComputeInterface::set_ComputeConfig( scfg );

 auto sc = dynamic_cast<BlockSolverConfig *>( cc->f_value.first );
 if( sc == nullptr )
  throw( std::logic_error( "the configuration is not a BlockSolverConfig" ) );
 v_Block[0]->set_SolverConfig( sc );

 auto bc = dynamic_cast<BlockConfig *>( cc->f_value.second );
 if( bc == nullptr )
  throw( std::logic_error( "the configuration is not a BlockConfig" ) );
 v_Block[0]->set_BlockConfig( bc );

 }  // end ( LagBFunction::set_relaxed_function( ) )   - - - - - - - - - - - -

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
    (svcc->v_SolverConfigs).push_back(cc);
    svcc->v_SolverConfigs[0]->int_pars.push_back(
    		 std::make_pair( int_par_idx2str(intLPMaxSz) , value ) );
    }
   else {

    auto it_v = std::find_if( svcc->v_SolverConfigs[0]->int_pars.begin() ,
    	 svcc->v_SolverConfigs[0]->int_pars.end() ,
    	 [ ]( const std::pair< std::string , int > & p ) {
    	      return( p.first == "intLPMaxSz" );  } );

    if( it_v != svcc->v_SolverConfigs[0]->int_pars.end() )
     (*it_v).second = value;
    else
     svcc->v_SolverConfigs[0]->int_pars.push_back(
            std::make_pair( "intLPMaxSz" , value ) );

    }
   }
   break;
  case( intGPMaxSz ):
   GPMaxSz = value;
   if( g_pool.size() > GPMaxSz )
	for( auto it = g_pool.begin() + GPMaxSz ; it != g_pool.end() ;   )
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
     (svcc->v_SolverConfigs).push_back(cc);
     svcc->v_SolverConfigs[0]->dbl_pars.push_back(
    		 std::make_pair( dbl_par_idx2str(dblRAccLin) , value ) );
     }
    else {

     //??? controllare []
     auto it_v = std::find_if( svcc->v_SolverConfigs[0]->dbl_pars.begin() ,
    	 svcc->v_SolverConfigs[0]->dbl_pars.end() ,
    	 [ ]( const std::pair< std::string , double > & p ) {
    	      return( p.first == "dblRAccLin" );  } );

     if( it_v != svcc->v_SolverConfigs[0]->dbl_pars.end() )
      (*it_v).second = value;
     else
      svcc->v_SolverConfigs[0]->dbl_pars.push_back(
            std::make_pair( "dblRAccLin" , value ) );

     }
    }
   break;
  case( dblAAccLin ):
   if( AAccLin != value ) {
    AAccLin = value;
    if( !svcc ) {
     svcc = new BlockSolverConfig;
     ComputeConfig* cc = new ComputeConfig;
     (svcc->v_SolverConfigs).push_back(cc);
     svcc->v_SolverConfigs[0]->dbl_pars.push_back(
     		 std::make_pair( dbl_par_idx2str(dblAAccLin) , value ) );
     }
    else {

     auto it_v = std::find_if( svcc->v_SolverConfigs[0]->dbl_pars.begin() ,
     	 svcc->v_SolverConfigs[0]->dbl_pars.end() ,
     	 [ ]( const std::pair< std::string , double > & p ) {
     	      return( p.first == "dblAAccLin" );  } );

     if( it_v != svcc->v_SolverConfigs[0]->dbl_pars.end() )
      (*it_v).second = value;
     else
      svcc->v_SolverConfigs[0]->dbl_pars.push_back(
             std::make_pair( "dblAAccLin" , value ) );

     }
    }
   break;
  default: Function::set_par( par , value );
  }
 } // end ( LagBFunction::set_par( double ) )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::deserialize( netCDF::NcGroup & group )
{

 v_Block.clear();
 netCDF::NcGroup sb = group.getGroup( "B" );
 if( sb.isNull() )
  throw( std::logic_error( "the group B is null" ) );

 v_Block.push_back( new_Block( sb , this ) );

 } // end ( LagBFunction::deserialize )  - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::add_dual_pairs( v_dual_pair && v_lag_pair ,
		 const bool static_is_ordered , c_ModParam issueMod ) {

   // ??? perchee2 ci non ci sta il parametro del channel

 /* If not already ordered by ColVariable "name = pointer", sort the vector of
	dual Lagrangian pairs <y_i,g_i(x)=a_i^T x > and update the structure
	LagMatrix which is used to update the Lagrangian cost vector.

	LagMatrix is a map whose the key is the pointer to the primal variable x_j,
	the second field is the pair < c_j , j-th column >, being the j-th column the
	vector < y, A_j>. It is assumed that map is ordered by the primal
	variable name and a column < y, A_j> is ordered by Lagrangian multiplier
	name (= pointer).

	If some new variables x_j are inserted their coefficients c_j are set
	to zero.  */

 for( const auto lagdual : v_lag_pair ) { // for each relaxed constraints
  auto LinFunc = dynamic_cast<const LinearFunction *>( lagdual.second );
  if( LinFunc == nullptr )
   throw( std::logic_error( "the objective is not a linear function" ) );
  }

 if( ! static_is_ordered )
  std::sort( v_lag_pair.begin() , v_lag_pair.end() ,
	  []( const auto & p1, const auto & p2 ) { return( p1.first < p2.first ); } );

 Vec_p_Var varsX = add_columns( v_lag_pair );

 /* copy the coefficients c of (obj_B) of the newly inserted variables vars
    and update LagMatrix in order to allow the modifications of the
    Lagrangian cost vector c^y = c + yA, the original costs c will be
    unavailable unless have been stored somewhere, the issue is that
    -in (obj_B)- vector c must be replaced by c^y

    since some variables, say ZeroVars, may be active in (RCs) but
    *not* active in (obj_B), the same number of coeff_pair as the
    number of ZeroVars have to be added to (obj_B) with zero-coefficient
    and Variables pointers defined by ZeroVars */

 set_original_costs( varsX );
 fix_sblock_objective();

 // merge the list of dual Lagrangian pairs, both containers shall already be
 // ordered  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 std::merge( lag_p.begin() , lag_p.end() , v_lag_pair.begin() , v_lag_pair.end() ,
		 lag_p.begin() ,
		 []( const auto & p1, const auto & p2 ) { return( p1.first < p2.first ); }  );

 std::move( v_lag_pair );

 if( f_Observer && f_Observer->issue_mod( issueMod ) )
  issue_add_variables_modification( v_lag_pair , issueMod );

 // clear v_lag_pair, because already merged with lag_p  - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 v_lag_pair.clear();

 } // end ( LagBFunction::add_dual_pairs( ) )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::remove_dual_pairs( v_dual_pair && v_lag_pair ,
		 const bool static_is_ordered , c_ModParam issueMod ,
		 c_ModParam issueAMod ) {

 /* update the structure LagMatrix:
	remove the pointer to variable x_j from LagMatrix if no longer the relaxed
	constraints (RCs) are active and restore the coefficient c_j in (obj_B) */

 std::move( v_lag_pair );

 for( const auto lagdual : v_lag_pair ) { // for each relaxed constraints
  auto LinFunc = static_cast<const LinearFunction *>( lagdual.second );
  if( LinFunc == nullptr )
   throw( std::logic_error( "the objective is not a linear function" ) );
  }

 if( ! static_is_ordered )
  std::sort( v_lag_pair.begin() , v_lag_pair.end() ,
	  []( const auto & p1, const auto & p2 ) { return( p1.first < p2.first ); } );


 rm_columns( v_lag_pair );

 // remove the Lagrangian pairs <y_i,g_i(x)=a_i^T x> contained in v_dual_pair
 // from lag_p - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( auto it = v_lag_pair.begin() ; it != v_lag_pair.end() ;  ++it )
  lag_p.erase( it );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 Vec_p_Var vars( v_lag_pair.size() );
 auto its = vars.begin();
 for( auto nm : v_lag_pair )
  *(its++) = nm.first;

 f_Observer->add_Modification( std::make_shared<FunctionModVars>( this ,
                                       FunctionModVars::RemoveVar ,
				       std::move( vars ) , true , 0 ,
				       Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 // clear v_lag_pair, because already removed from lag_p   - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 v_lag_pair.clear();

 }  // end( LagBFunction::remove_dual_pairs() )- - - - - - - - - - - - - - - -

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
 group.putAtt( "type" , "LagBFunction" );

 if( v_Block.size() != 1 )
  throw( std::invalid_argument( "it is expected 1 sub-block " ) );

 // The costs saved in (obj_B) are the Lagrangian ones. Hence, we need
 // to restore the original ones before serializing (B).

 auto lfobj = static_cast<LinearFunction *>( obj->get_function() );

 LinearFunction::v_c_coeff_pair & ov_pair = lfobj->get_v_var();
 LinearFunction::v_coeff_pair VarsToChange( LagMatrix.size() );  // temporary
 LinearFunction::v_coeff_pair VarsToRestore( LagMatrix.size() ); // vectors

 auto itb = ov_pair.begin();
 auto itc = VarsToChange.begin();
 auto itd = VarsToRestore.begin();
 for( auto ita = LagMatrix.begin() ; ita != LagMatrix.end() ; itb++ ) {
  // there is no variable active in (RCs) and *not active* in (obj_B)
  if( ita->first == itb->first ) {
   itc->first = ita->first;  // put the original costs into VarsToChange
   itc->second = (ita->second).first;
   itd->first = itb->first;  // put the Lagrangian costs into VarsToRestore
   itd->second = itb->second;
   ita++;
   itc++;
   itd++;
   }
  }

 // put the original costs into (obj_B)  - - - - - - - - - - - - - - - - - - -
 lfobj->modify_coefficients( VarsToChange );

 // serialize the sub-block  - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 netCDF::NcGroup sb = group.addGroup( "B" );
 v_Block[0]->serialize( sb );

 // put back the Lagrangian costs into (obj_B) - - - - - - - - - - - - - - - -
 lfobj->modify_coefficients( VarsToRestore );

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

void LagBFunction::store_linearization( const LinearizationName name )
{
 // throw exception if the solution does not exist or has been already stored

 if( std::isnan( LastSolution ) ||
		 LastSolution < Inf<LinearizationName>() )
  throw( std::logic_error( "the linearization is unvailable" ) );

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

 } // end LagBFunction::store_linearization( LinearizationName ) - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::delete_linearization( const LinearizationName name )
{
 delete[] std::get<0>(g_pool[ name ]);
 } // end LagBFunction::delete_linearization( LinearizationName )  - - - - - -

/*--------------------------------------------------------------------------*/

int LagBFunction::compute( bool changedvars )
{
 // update the Lagrangian cost vector  - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( changedvars ) //???
  compute_Lagrangian_costs();

 // if some parameters have been changed, set BlockSolverConfig
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( svcc )
  v_Block[0]->set_SolverConfig( svcc );

 delete svcc;

 /* It is assumed that the sub-Block (B) does not have Variable defined
    in other Blocks. Then, the re-optimization of (B) can be performed starting
    from the warm-start (the old solution) and any problem shouldn't occur.
    No relevant Variable are defined in (B). Return the status of the optimization
    process */

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
  for( const auto & lagdual : lag_p ) {
   auto lfrel = static_cast<const LinearFunction *>( lagdual.second );
   objvalue += lfrel->get_constant_term() * (lagdual.first)->get_value();
   }


 return( objvalue );

 } // end ( LagBFunction::get_value( ) ) - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::get_linearization_coefficients( FunctionValue * g ,
	  const LinearizationName name , const std::vector<Index> * const indices ,
      const Index start , const Index end )

{
 c_Index end_p = std::min( Index( lag_p.size()) , end );
 if( end_p <= start )
  return;

 // the solution shall be written in the Variable of the Block - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( name == Inf<LinearizationName>() ) { // asking for the last computed
	                                      // linearization   - - - - - - - - -

  // get solution/direction from the solver  - - - - - - - - - - - - - - - - -
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( LastSolution != Inf<LinearizationName>() ) {
   if( VarType == VarAreSol )
    slv->get_var_solution();
   else
    slv->get_var_direction();
   LastSolution = Inf<LinearizationName>();
   }

  }
 else {  // asking for a linearization of the global pool  - - - - - - - - - -

  // assign Solution to the sub-Block in such a way the linearization
  // associated with the given name will be recovered from the global pool

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

 if( indices != nullptr ) {
  for( const auto & i : *indices )
   if( ( i >= start ) && ( i < end_p ) )
    *(g++) = lag_p[ i ].second->get_value();
  }
 else
  for( Index i = start ; i < end_p ; ++i )
	*(g++) = lag_p[ i ].second->get_value();

 } // end( LagBFunction::get_linearization_coefficients( DenseVector ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::get_linearization_coefficients( SparseVector & g ,
			const LinearizationName name , c_Vec_Index * const indices ,
            c_Index start , c_Index end )
{
 c_Index end_p = std::min( Index( lag_p.size()) , end );
 if( end_p <= start )
  return;

 // the solution shall be written in the Variable of the Block - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( name == Inf<LinearizationName>() ) { // asking for the last computed
	                                      // linearization  - - - - - - - - -

  // get solution/direction from the solver  - - - - - - - - - - - - - - - - -
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( LastSolution != Inf<LinearizationName>() ) {
   if( VarType == VarAreSol )
	slv->get_var_solution();
   else
	slv->get_var_direction();
   LastSolution = Inf<LinearizationName>();
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

  } // end else  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // for each Lagrangian multiplier y_i, the objective value of the relaxed constraint
 // (RCs)_i is the corresponding entry of the linearization - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( g.nonZeros() == 0 ) {  // the given vector contains no non-zero element

  if( g.size() < Index( lag_p.size()) )
   g.resize( Index( lag_p.size()) );

  g.reserve( end_p - start );

  if( indices != nullptr ) {
   for( const auto & i : *indices )
    if( ( i >= start ) && ( i < end_p ) )
     g.insert( i ) = lag_p[ i ].second->get_value();
   }
  else
   for( Index i = start ; i < end_p ; ++i )
    g.insert( i ) = lag_p[ i ].second->get_value();

  }
 else {  // the given vector contains some non-zero elements
  if( g.size() != Index( lag_p.size()) )
   throw( std::invalid_argument(
      "LagBFunction::get_linearization_coefficients: "
      "the size of the sparse vector must be equal to the number "
      "of Lagrangian multipliers" ) );

  if( indices != nullptr ) {
   for( const auto & i : *indices )
    if( ( i >= start ) && ( i < end_p ) )
     g.coeffRef( i ) = lag_p[ i ].second->get_value();  //*?????
     }
    else
     for( Index i = start ; i < end_p ; ++i )
      g.coeffRef( i ) = lag_p[ i ].second->get_value();
  }

 }  // end( LagBFunction::get_linearization_coefficients( SparseVector ) )

/*--------------------------------------------------------------------------*/

Function::FunctionValue LagBFunction::get_linearization_constant(
		const LinearizationName name )
{

 auto lfobj = static_cast<LinearFunction *>( obj->get_function() );
 Function::FunctionValue alpha = lfobj->get_constant_term();

 if( name == Inf<LinearizationName>() ) {

  // get solution/direction from the solver  - - - - - - - - - - - - - - - - -
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( LastSolution != Inf<LinearizationName>() ) {
   if( VarType == VarAreSol )
    slv->get_var_solution();
   else
    slv->get_var_direction();
   LastSolution = Inf<LinearizationName>();
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

  if( (std::get<2>(g_pool[ name ]) == VarToBeChckd) && !v_Block[0]->is_feasible( ) )
   return( Inf< Function::FunctionValue >() );

  }

 // return the constant c^Tx + c_0 unless the solution is no longer feasible
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LinearFunction::v_c_coeff_pair & LFInnBCoeff = lfobj->get_v_var();

 auto it = LagMatrix.begin() ;
 auto itv = LFInnBCoeff.begin();

 while( it != LagMatrix.end() || itv != LFInnBCoeff.end() )
 {
  if( itv == LFInnBCoeff.end() ) {
   alpha += (it->first)->get_value() * (it->second).first;
   it++;
   }
  else
   if( it == LagMatrix.end() ) {
    alpha += (itv->first)->get_value() * (itv->second);
    itv++;
    }
   else {
    if( it->first < itv->first ) {
     alpha += (it->first)->get_value() * (it->second).first;
     it++;
     }
    else
     if( it->first == itv->first ) {
      alpha += (it->first)->get_value() * (it->second).first;
      it++;
      itv++;
      }
     else {
      alpha += (itv->first)->get_value() * (itv->second);
      itv++;
      }
    }
  } // end while  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 return( alpha );

 } // end( LagBFunction::get_linearization_constant() )  - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/

ComputeConfig * LagBFunction::get_ComputeConfig( bool all ,
		ComputeConfig * ocfg ) const {

 ComputeConfig* ccfg = ThinComputeInterface::get_ComputeConfig( all , ocfg );

 auto cc = dynamic_cast<SimpleConfig_p_p *>( ccfg );
 if( cc == nullptr )
  throw( std::logic_error( "the configuration is not a SimpleConfiguration" ) );

 cc->f_value.first = v_Block[0]->get_SolverConfig();
 cc->f_value.second = (v_Block[0]->get_BlockConfig())->clone();

 return( ccfg );

 } // end( LagBFunction::get_ComputeConfig() )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

int LagBFunction::get_dflt_int_par( const idx_type par ) const
{
 switch( par ) {
  case( intLPMaxSz ):
   return( RAccLin );
	break;
  case( intGPMaxSz ):
   return( RAccLin );
   break;
  default:
   return( C05Function::get_dflt_dbl_par( par ) ) ;
  }

} // end( LagBFunction::get_dflt_int_par( idx_type ) )  - - - - - - - - - - -

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

double LagBFunction::get_dflt_dbl_par( const idx_type par ) const
{
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
} // end( LagBFunction::get_dflt_dbl_par( idx_type ) ) - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE LagBFunction ---------*/
/*--------------------------------------------------------------------------*/

ThinVarDepInterface::Index LagBFunction::is_active( const Variable * const var )
const
{
 auto idx = std::lower_bound( lag_p.begin() , lag_p.end() ,
                                std::make_pair( var , 0 ) ,
                                []( const auto & p1, const auto & p2 )
                                  { return p1.first < p2.first; } );
 if( idx < lag_p.end() )
  return( std::distance( lag_p.begin() , idx ) );
 else
  return( Inf<Index>() );

 } // end( LagBFunction::is_active( Variable* ) )  - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::map_active( c_Vec_p_Var & vars , Vec_Index & map ,
		const bool ordered ) const
{
 if( ! vars.size() )
  return;

 // the basic implementation of the method is used whenever vars is not ordered
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( ! ordered ) {
  ThinVarDepInterface::map_active( vars , map );
  return;
  }

 if( map.size() < vars.size() )
  map.resize( vars.size() );

 // construct map vector - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 auto itvb = vars.begin();
 auto itvv = std::lower_bound( lag_p.begin() , lag_p.end() ,
     std::make_pair( *itvb , 0 ) , []( const auto & p1 , const auto & p2 ) {
	 return( p1.first < p2.first ); } );
 auto itve = std::upper_bound( itvv , lag_p.end() ,
	 std::make_pair( *(--vars.end()) , 0 ) , []( const auto & p1 , const auto & p2 ) {
	 return( p1.first < p2.first ); } );

 auto itm = map.begin();
 while( itvb < vars.end() ) {
  if( itvv >= itve )
   throw( std::invalid_argument( "some Variable is not active" ) );

  *(itm++) = std::distance( lag_p.begin() , itvv );
  itvv = std::lower_bound( itvv , itve , std::make_pair( *(++itvb) , 0 ) ,
     []( const auto & p1, const auto & p2 ) { return( p1.first < p2.first ); } );

  } // end while - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

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

Vec_p_Var LagBFunction::add_columns( v_dual_pair & v_lag_pair )
{
 // update the map which provides the information used to compute the
 // Lagrangian costs - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 std::set<p_Var> unique_vars;

 for( const auto & lagdual : v_lag_pair ) { // read the dual vector lag_p,
	          // getting the relaxed constraint <a,x> and the dual variable
	          // y_i thereof

  auto lfr = static_cast<const LinearFunction *>( lagdual.second );
  LinearFunction::v_c_coeff_pair & lfr_c = lfr->get_v_var();

  for( const auto & monomial : lfr_c ) { // for each Variable x_j
                        // of the relaxed constraint, add the pair
	                    // < y_i , a_{ij} > to LagMatrix

   // construct the pair of the form < y_i , a_{ij} > to be added to
   // the related column of x_j  - - - - - - - - - - - - - - - - - - - - - - - -

   const auto pair = std::make_pair( lagdual.first , monomial.second );

   auto itA = LagMatrix.insert( std::pair<ColVariable * , col_pair>
                         ( monomial.first , col_pair() ) );

   // itA.first is an iterator pointing to either the newly
   // inserted jth column or to the jth column already in the map,
   // in particular (itA.first->first) is the variable xj and
   // (itA.first->second) the pair <col_pair> thereof. This means
   // that (itA.first->second).first is c_j and (itA.first->second).second
   // is the array < y_i , a_{ij} > for all i in the active index set
   // of the Lagrangian multipliers

   auto &curr_column = (itA.first->second).second;

   // note that itA is a pair <iterator,bool>, the pair::second element
   // is set to false if an equivalent key already existed, true otherwise

   if( itA.second ) { // a new variable x_j was inserted  - - - - - - - - - -

    (itA.first->second).first = 0;           // set c_j = 0
    unique_vars.insert( itA.first->first );
	curr_column.insert( curr_column.begin() , pair );

    }
   else {  // variable x_j already existed - - - - - - - - - - - - - - - - - - -

	auto itB = std::lower_bound( curr_column.begin() ,
	 			        curr_column.end() , std::make_pair( lagdual.first , 0 ) ,
	 		 	        []( const LinearFunction::coeff_pair &a ,
	 		 	     	const LinearFunction::coeff_pair &b )
	 		 	        { return( a.first < b.first ); } );

    curr_column.insert( itB , pair );
    }

   }
  } // end for each relaxed constraints  - - - - - - - - - - - - - - - - - - -

 Vec_p_Var vars(unique_vars.begin(), unique_vars.end());
 std::sort( vars.begin() , vars.end() ,
	  []( const auto & p1, const auto & p2 ) { return( p1 < p2 ); } );

 return( vars );

 } // end ( LagBFunction::add_columns() )  - - - - - - - - - - - - - - - - - -


/*--------------------------------------------------------------------------*/

Vec_p_Var LagBFunction::update_columns( v_dual_pair & v_lag_pair )
{
 // update the map which provides the information used to compute the
 // Lagrangian costs - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 std::set<p_Var> unique_vars; // variables to be removed from LagMatrix

 // the following array has to save the variables which are no longer
 // active in any constraint - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LinearFunction::v_coeff_pair VarsToRmv( LagMatrix.size() );

 // update the map which provides the information needed to compute the
 // Lagrangian costs - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( const auto & lagdual : v_lag_pair ) { // read the dual vector lag_p,
	          // getting the relaxed constraint <a,x> and the dual variable
	          // y_i thereof

  auto lfr = static_cast<const LinearFunction *>( lagdual.second );
  LinearFunction::v_c_coeff_pair & lfr_c = lfr->get_v_var();

  for( const auto & monomial : lfr_c ) { // for each Variable x_j
                        // of the relaxed constraint, add the pair
	                    // < y_i , a_{ij} > to LagMatrix

   // construct the pair of the form < y_i , a_{ij} > to be added to
   // the related column of x_j  - - - - - - - - - - - - - - - - - - - - - - - -

   const auto pair = std::make_pair( lagdual.first , monomial.second );

   auto itA = LagMatrix.insert( std::pair<ColVariable * , col_pair>
                         ( monomial.first , col_pair() ) );

   // itA.first is an iterator pointing to either the newly
   // inserted jth column or to the jth column already in the map,
   // in particular (itA.first->first) is the variable xj and
   // (itA.first->second) the pair <col_pair> thereof. This means
   // that (itA.first->second).first is c_j and (itA.first->second).second
   // is the array < y_i , a_{ij}j > for all i in the active index set
   // of the Lagrangian multipliers

   auto &curr_column = (itA.first->second).second;

   // note that itA is a pair <iterator,bool>, the pair::second element
   // is set to false if an equivalent key already existed, true otherwise

   if( itA.second ) { // a new variable x_j was inserted  - - - - - - - - - -

	(itA.first->second).first = 0;           // set c_j = 0
	unique_vars.insert( itA.first->first );
    curr_column.insert( curr_column.begin() , pair );

    }
   else {  // variable x_j already existed - - - - - - - - - - - - - - - - - - -

	auto itB = std::lower_bound( curr_column.begin() ,
	 			        curr_column.end() , std::make_pair( lagdual.first , 0 ) ,
	 		 	        []( const LinearFunction::coeff_pair &a ,
	 		 	     	const LinearFunction::coeff_pair &b )
	 		 	        { return( a.first < b.first ); } );


	if( itB->first == lagdual.first )
	 itB->second = monomial.second;
	else
     curr_column.insert( itB , pair );

    }

   }
  } // end for each relaxed constraints  - - - - - - - - - - - - - - - - - - -

 // check for remotion of inactive variables: some variables x_j may become
 // inactive removing the Lagrangian pairs, in this case the relative
 // column < y, A_j> must be removed from LagMatrix  - - - - - - - - - - - - -

 for( auto el : LagMatrix ) {
  auto & curr_column = (el.second).second;
  for( auto it = curr_column.begin() ; it != curr_column.end()  ; ) {

   auto itB = std::lower_bound( lag_p.begin() ,
		        lag_p.end() , std::make_pair( it->first , nullptr ) ,
		 		[]( const dual_pair &a , const dual_pair &b )
		 		{ return( a.first < b.first ); } );


   if( it->first != itB->first )
 	throw( std::logic_error( "this should not be happened" ) );

   if( (el.first)->is_active( itB->second ) >= (el.first)->get_num_active() )
    curr_column.erase( it );
   else
    it++;

    }
   }

 // if a column < y, A_j> associated to a variable x_j is empty, copy the pointer
 // to that Variable x_j and the coefficient c_j thereof - - - - - - - - - -

 for( auto el : LagMatrix ) {
  const auto & curr_column = (el.second).second;
  if( curr_column.empty() ) {
   VarsToRmv.push_back ( std::make_pair( el.first , (el.second).first ) );
   LagMatrix.erase( el.first );
   }
  }

 // save the original coefficients c_j of he variables x_j which no
 // longer are active in (RCs), write them in (obj_B).  - - - - - - - - - -

 auto lfobj = static_cast<LinearFunction *>( obj->get_function() );
 LinearFunction::v_c_coeff_pair & LFInnBCoeff = lfobj->get_v_var();

 lfobj->modify_coefficients( VarsToRmv );

 Vec_p_Var vars(unique_vars.begin(), unique_vars.end());
 std::sort( vars.begin() , vars.end() ,
	  []( const auto & p1, const auto & p2 ) { return( p1 < p2 ); } );

 return( vars );

 } // end ( LagBFunction::update_columns() )  - - - - - -  - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::rm_columns( v_dual_pair & v_lag_pair )
{
 // update the map which provides the information used to compute the
 // Lagrangian costs - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // the following array has to save the variables which are no longer
 // active in any constraint - - - - - - - - - - - - - - - - - - - - - - - - -

 LinearFunction::v_coeff_pair VarsToRmv( v_lag_pair.size() );

 // remove the Lagrangian pairs from the map LagMatrix
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( auto el : LagMatrix ) {
  auto & curr_column = (el.second).second;
  auto itv1 = v_lag_pair.begin();
  for( auto it = curr_column.begin() ;
		  it != curr_column.end() && itv1 != v_lag_pair.end() ; ) {
   if( it->first < itv1->first )
    it++;
   else
    if( it->first == itv1->first ) {
     curr_column.erase( it );
     itv1++;
     }
    else
     itv1++;
   }

  } // end remotion - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // if a column < y, A_j> associated to a variable x_j is empty, copy the pointer
 // to that Variable x_j and the coefficient c_j thereof- - - - - - - - - - -

 for( auto el : LagMatrix ) {
  const auto & curr_column = (el.second).second;
  if( curr_column.empty() ) {
   VarsToRmv.push_back ( std::make_pair( el.first , (el.second).first ) );
   LagMatrix.erase( el.first );
   }
  }

 // save the original coefficients c_j of he variables x_j which no
 // longer are active in (RCs), write them in (obj_B)  - - - - - - - - - - -

 auto lfobj = static_cast<LinearFunction *>( obj->get_function() );
 LinearFunction::v_c_coeff_pair & LFInnBCoeff = lfobj->get_v_var();

 auto itv2 = LFInnBCoeff.begin();
 for( auto it = VarsToRmv.begin() ; it != VarsToRmv.end() ;   ) {

  if( itv2 != LFInnBCoeff.end() ) {
   if( it->first < itv2->first )
    it++;
   else
    if( it->first == itv2->first ) {
     lfobj->modify_coefficient( it->first , it->second );
     it++;
     itv2++;
     }
    else
     itv2++;
   }
  else
   it++;

  } // end for - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 } // end ( LagBFunction::rm_columns() ) - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::issue_add_variables_modification( v_dual_pair & pairs ,
						       c_ModParam issueMod )
{
 Vec_p_Var vars( pairs.size() );
 for( Index i = 0 ; i < pairs.size() ; ++i )
  vars[ i ] = pairs[ i ].first;

 // a Lagrangian function is strongly quasi-additive
 f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                         FunctionModVars::AddVar ,
					 std::move( vars ) , 0 , true ,
					 Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );
 } // end ( LagBFunction::issue_add_variables_modification() ) - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::set_objective_and_solver( )
{
 assert( ! ( v_Block[0]->get_objective() ).empty() );  // ... which must exist

 // the objective function of the inner block must be linear - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 obj = boost::any_cast<FRealObjective *>( v_Block[0]->get_objective() );
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

void LagBFunction::fix_sblock_objective( )
{
 auto lsb = static_cast<LinearFunction *>( obj->get_function() );
 LinearFunction::v_c_coeff_pair & ov_pair = lsb->get_v_var();

 /* This method can only be called after set_original_costs() of this Function.
    If there are some variables, say ZeroVars, which are active in (RCs)
    but *not* active in (obj_B), then the same number of coeff_pair as the number
    of ZeroVars have to be added to (obj_B) with zero-coefficient
    [ see set_original_costs() ]. */


 LinearFunction::v_coeff_pair PairsToAdd;  // this array is used to signal the
                                           // pairs to add to (obj_B)

 auto itb = ov_pair.begin();
 for( auto ita = LagMatrix.begin() ; ita != LagMatrix.end() ;   ) {
  if( itb != ov_pair.end() )
   if( ita->first <= itb->first ) {
    if( ita->first == itb->first )  // nothing to do
     itb++;
    else { // x_j is active in (RCs) but *not* active in (obj_B),
    	   // add the pair (x_j,0) to (obj_B)
     const auto pair = std::make_pair( ita->first , LinearFunction::Coefficient(0) );
     PairsToAdd.push_back( pair );
     }
    ita++;
    }
   else     // nothing to do
    itb++;
  else {
   const auto pair = std::make_pair( ita->first , LinearFunction::Coefficient(0) );
   PairsToAdd.push_back( pair );
   (ita->second).first = LinearFunction::Coefficient(0);
   ita++;
   }
  } // end for - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 lsb->add_variables( std::move(PairsToAdd) , true );

 } // end ( LagBFunction::set_original_costs( ) )  - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::set_original_costs( c_Vec_p_Var & v_xvar )
{
 /* The function stores the coefficients of (obj_B) relative to the variables
  contained in v_xvar, saving them in LagMatrix.

  LagMatrix is a map whose the key is the pointer to the primal variable x_j,
  the second field is the pair < c_j , j-th column >, being the j-th column the
  vector < y, A_j>. It is assumed that map is ordered by the primal
  variable name and a column < y, A_j> is ordered by Lagrangian multiplier
  name (= pointer).

  Any variable x_j is in LagMatrix is inserted with coefficient c_j set to
  zero, so the coefficients to be updated are the active ones in (obj_B).

   v_xpair is constructed like this:

    v_xpair(i) = { ( x_j , c_j ) : x_j =  v_xvar(i) )

  Obviously in this way v_xvar is read twice, but if v_xvar was once read
  the coefficients have to be get from (obj_B) by means of
  LinearFunction::get_coefficient, and it would be more costly. */

 auto lsb = static_cast<LinearFunction *>( obj->get_function() );
 LinearFunction::v_c_coeff_pair & ov_pair = lsb->get_v_var();
 LinearFunction::v_coeff_pair v_xpair ( v_xvar.size() );

 auto itc = v_xpair.begin();
 auto itb = ov_pair.begin();

  if( v_xvar.size() )
   for( auto ita = v_xvar.begin() ; ita != v_xvar.end() ;   ) {
    if( itb != ov_pair.end() )
	 if( *ita < itb->first ) { // the variable is not active in (obj_B)
	  //(itc)->second = 0;     // and so its coefficient is zero but
	  //(itc++)->first = itb->first; // there is no need to set the
      ita++;                         // coefficient in LagMatrix
	  }
	 else
	  if( *ita == itb->first ) {    // copy the coefficient value from (obj_B)
	   (itc)->first = itb->first;   // and insert it into LagMatrix
	   (itc++)->second = itb->second;
	   ita++;
	   itb++;
	   }
	  else  // nothing to do
	   itb++;
    else {                // the variable is not active in (obj_B)
     //(itc)->second = 0; // and so its coefficient is zero but
     //(itc++)->first = itb->first; // there is no need to set the coefficient in
     ita++;                         // LagMatrix
     }
    }

 /* The original costs have to be replaced by Lagrangian ones in (obj_B)
    allowing B to be the Lagrangian relaxation sub-problem. To avoid
    to lose the original coefficients of the costs, they have to be
    saved in LagBFunction itself, actually it is done inside LagMatrix.

    LagMatrix is even used to compute the Lagrangian costs c^y which have
    to be passed to (obj_B), four cases can occur according to the active
    variables in (RCs) and (obj_B):
     1. x_j is active in both (RCs) and (obj_B) ==> c_j has to be
        saved in LagMatrix and the Lagrangian cost is c^y_j = c_j + yA_j
     2. x_j is active in (RCs) but *not* active in (obj_B) ==> 0 has to be
        saved in LagMatrix and the Lagrangian cost is c^y_j = 0 + yA_j
     3. x_j is *not* active in (RCs) but active in (obj_B) ==> nothing
        to do, the Lagrangian cost is zero, c^y_j = c_j, don't touch the
        original cost of (obj_B)
     4. x_j is *not* active in both (RCs) and (obj_B) ==> nothing to do,
        the Lagrangian cost is zero, c^y_j = c_j = 0, don't touch the
        original cost of (obj_B)  */

  if( v_xpair.size() ) {
   itc = v_xpair.begin();
   for( auto ita = LagMatrix.begin() ; ita != LagMatrix.end() ; ) {
	if( ita->first < itc->first )
     ita++;
	else
	 if( ita->first == itc->first ) {
	  (ita->second).first = itc->second;
	  ita++;
	  itc++;
	  }
	 else
      itc++;
    }
   }
  else {
   itb = ov_pair.begin();
   for( auto ita = LagMatrix.begin() ; ita != LagMatrix.end() ; ) {
    if( ita->first < itb->first ) { // the variable is active in (RCs) but
     ita++; // not active in (obj_B), so its coefficient has to be zero but
     }      // there is no need to set the zero in LagMatrix
    else
     if( ita->first == itb->first ) { // copy the coefficient value from (obj_B)
      (ita->second).first = itb->second; // and insert it into LagMatrix
      ita++;
      itb++;
      }
     else
      itb++; // nothing to do
    }
   }

 // The job is not completed if there are some variables, say ZeroVars, which are
 // active in (RCs) but *not* active in (obj_B); the same number of coeff_pair
 // as the number of ZeroVars have to be added to (obj_B) with zero-coefficient
 // [ see fix_sblock_objective() ].

 } // end ( LagBFunction::set_original_costs( c_Vec_p_Var ) )  - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::compute_Lagrangian_costs( )
{
 // get the objective function pointer of the inner block   - - - - - - - - -

 auto LFInnBlck = static_cast<LinearFunction *>( obj->get_function() );

 LinearFunction::Coefficient FVCoeff;
 LinearFunction::v_coeff_pair PairsToAdd; // this array is created to change the
                                          // pairs in LFInnBlck

 for( auto it = LagMatrix.begin() ; it != LagMatrix.end() ; ++it ) {

  FVCoeff = (it->second).first;
  for( const auto & el : (it->second).second )
   FVCoeff -= el.first->get_value() * el.second;

  PairsToAdd.push_back( std::make_pair( it->first , FVCoeff ) );

  } // end for - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LFInnBlck->modify_coefficients( PairsToAdd , true );

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

 LagMatrix.clear();

 // delete the global pool - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( auto tpl : g_pool )
  delete[] std::get<0>(tpl);
 g_pool.clear();

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
   auto lfmod = static_cast<LinearFunction * const>( tmod->f_function );
   if( !lfmod )
	throw( std::logic_error( "the function must be linear" ) );

   // let's start considering a modification of (obj_B) - - - - - - - - - - -

   auto lfobj = static_cast<LinearFunction *>( obj->get_function() );

   // distinguish between predictable and unpredictable changes, take into account
   // that because of the linearity of the function a predictable change
   // has to involve the constant term only and all the coefficients remain unchanged,
   // the function shall be shifted - - - - - - - - - - - - - - - - - - - - -

   if( lfmod == lfobj ) // if the Linear Function is (obj_B)
    if( !std::isnan( tmod->f_shift ) &&
      ( tmod->f_shift < FunctionMod::INFshift &&
    	tmod->f_shift > -FunctionMod::INFshift ) ) { // a predictable change

     // the Lagrangian function (obj_B) is shifted by the constant term
     // c'_0 - c_0

     // issue C05FunctionMod modification of the type AlphaChanged: (obj_B)
     // changes and the new form of the Lagrangian function is
     // c'{^y}(x) = c{^y}x + c'_0 - c_0, the computation of (obj_B) can be
     // obtained just adding c'_0 - c_0 without re-evaluating it, however
     // the constant terms of the linearizations have to be computed again
     //	by calling get_linearization_constant()

     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
       		 	C05FunctionMod::AlphaChanged , tmod->f_shift , 0 ) , chnl );

     }
    else { // an unpredictable change

     // the coefficients c_j, for some j, changed and have to
     // to be rewritten in LagMatrix - - - - - - - - - - - - - - - - - - - - -

     set_original_costs(); // no variable is added/removed

     // issue C05FunctionMod modification of the type AlphaChanged:
     // the Lagrangian function unpredictably changes,
     // f_shift has to be set to NaN, however the constant terms \alpha of the
     // linearizations (g, \alpha) have to be computed again by calling
     // get_linearization_constant() while the g remains unchanged

     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
    		C05FunctionMod::AlphaChanged , FunctionMod::NaNshift , 0 ) , chnl );

     }
   else
    if( lfmod->get_Observer() == this ) { // the Linear Function belongs
    	                       // to (RCs), search for the index of the function
    	                       // which has changed  - - - - - - - - - - - - -

     auto it_v = std::find_if( lag_p.begin() , lag_p.end() ,
    		 [ lfmod ]( const dual_pair & p ) {
    		  return( p.second == lfmod );  } );

 	 // take out the multiplier y_i which has been modified  - - - - - - - - -
  	 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

     v_dual_pair vdp( {*it_v} );
     Vec_p_Var v_vars( {vdp[ 0 ].first} );

     // distinguish between predictable changes and unpredictable ones - - - -

     if( !std::isnan( tmod->f_shift ) &&
      ( tmod->f_shift < FunctionMod::INFshift &&
    	tmod->f_shift > -FunctionMod::INFshift ) ) { // a predictable change

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
			   true, FunctionMod::NaNshift , 0 ) , chnl );
      }
     else { // an unpredictable change

      // the coefficient vector a_i of the function (RCs)_i have been
      // modified  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      // however, the the Lagrangian costs should be updated as follows:
      // c^y = c + y_i a_i x + sum_{p \neq i} y_p a_p x
      // a_i has to be re-written in LagMatrix to allow LagBFunction the
      // computation of the Lagrangian costs

   	  update_columns( vdp );

      // issue C05FunctionModSbst modification of the type AllEntriesChanged:
   	  // the i-th entry of the linearization ( g , \alpha ) has to change,
   	  // namely g_i must be re-computed -by calling
   	  // get_linearization_coefficients- at the index i-th, however the
   	  // Lagrangian function changes in an unpredictable  way and f_shift id
   	  // set to NaN  - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   	  if( f_Observer )
   	   f_Observer->add_Modification( std::make_shared<C05FunctionModSbst>( this ,
   			   C05FunctionModSbst::AllEntriesChanged , std::move(v_vars) ,
   			   true, FunctionMod::NaNshift , 0 ) , chnl );
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
     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
    	  C05FunctionMod::AlphaChanged , FunctionMod::NaNshift ) , chnl );

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
   auto lfmod = static_cast<LinearFunction * const>( tmod->f_function );
   if( !lfmod )
	throw( std::logic_error( "the function must be linear" ) );

   // let's start considering a modification of (obj_B) - - - - - - - - - - -

   auto lfobj = static_cast<LinearFunction *>( obj->get_function() );

   if( lfmod == lfobj ) { // if the Linear Function is (obj_B)

    // the coefficients c_j of variables v_vars, changed and have to
    // to be rewritten in LagMatrix - - - - - - - - - - - - - - - - - - - - -

     set_original_costs( tmod->v_vars );

     // issue C05FunctionMod modification of the type AlphaChanged:
     // the Lagrangian function unpredictably changes and
     // f_shift has to be set to NaN, however the constant terms \alpha of the
     // linearizations (g, \alpha) have to be computed again by calling
     // get_linearization_constant() while g remains unchanged

     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
    		C05FunctionMod::AlphaChanged , FunctionMod::NaNshift , 0 ) , chnl );

    }
   else
    if( lfmod->get_Observer() == this ) { // the Linear Function belongs
    	                       // to (RCs), search for the index of the function
    	                       // which has changed  - - - - - - - - - - - - -

     auto it_v = std::find_if( lag_p.begin() , lag_p.end() ,
    		 [ lfmod ]( const dual_pair & p ) {
    		  return( p.second == lfmod );  } );

 	 // take out the multiplier y_i which has been modified  - - - - - - - - -
  	 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

     v_dual_pair vdp( {*it_v} );
     Vec_p_Var v_vars( {vdp[ 0 ].first} );

     // the coefficient vector a_i of the function (RCs)_i have been modified

     // however, the the Lagrangian costs should be updated as follows:
     // c^y = c + y_i a_i x + sum_{p \neq i} y_p a_p x
     // a_i has to be re-written in LagMatrix to allow LagBFunction the
     // computation of the Lagrangian costs

   	 update_columns( vdp );

     // issue C05FunctionModSbst modification of the type AllEntriesChanged:
   	 // the i-th entry of the linearization ( g , \alpha ) has to change,
   	 // namely g_i must be re-computed -by calling
   	 // get_linearization_coefficients- at the index i-th, however the
   	 // Lagrangian function unpredictably changes and f_shift is
   	 // set to NaN   - - - - - - - - - - - - - - - - - - - - - - - - - -

   	 if( f_Observer )
   	  f_Observer->add_Modification( std::make_shared<C05FunctionModSbst>( this ,
   		   C05FunctionModSbst::AllEntriesChanged , std::move(v_vars) ,
   		   true, FunctionMod::NaNshift , 0 ) , chnl );

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
     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
    	  C05FunctionMod::AlphaChanged , FunctionMod::NaNshift ) , chnl );

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
   auto lfmod = static_cast<LinearFunction * const>( tmod->f_function );
   if( !lfmod )
   throw( std::logic_error( "the function must be linear" ) );

   // because of the linearity of the function the modification must be
   // quasi-additive one  and the shift is zero - - - - - - - - - - - - - - -
   if( tmod->f_shift != 0 )
    throw( std::logic_error( "the function must be linear" ) );

   auto lfobj = static_cast<LinearFunction *>( obj->get_function() );
   if( lfmod == lfobj ) { // if the Linear Function is (obj_B)

	// variables x_j, for some j, have been added to (remove from) (obj_B)
	// and the new coefficients have to to be rewritten in (deleted from) LagMatrix

	set_original_costs( tmod->v_vars );

	// if there are some variables, say ZeroVars, which are active in (RCs) but
	// *not* active in (obj_B), then the same number of coeff_pair as the number
	// of ZeroVars have to be added to (obj_B) with zero-coefficient and
	// Variables pointers defined by ZeroVars

	if( tmod->f_type == FunctionModVars::RemoveVar )
	 fix_sblock_objective();

	// issue C05FunctionMod modification of the type AlphaChanged:
	// the Lagrangian function unpredictably changes and
	// f_shift has to be set to NaN, however the constant terms of the
	// linearizations have to be computed again by calling
	// get_linearization_constant()

	if( f_Observer )
	 f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
	    		C05FunctionMod::AlphaChanged , FunctionMod::NaNshift , 0 ) , chnl );

    }
   else
    if( lfmod->get_Observer() == this ) { // if the Linear Function belongs
      	                       // to (RCs) search for the index of the function
      	                       // which has changed  - - - - - - - - - - - - -

     auto it_v = std::find_if( lag_p.begin() , lag_p.end() ,
      		 [ lfmod ]( const dual_pair & p ) {
      		  return( p.second == lfmod );  } );

   	 // take out the multiplier y_i which has been modified  - - - - - - - - -
     //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

     v_dual_pair vdp( {*it_v} );
     Vec_p_Var v_vars( {vdp[ 0 ].first} );

     // variables x_j, for some j, have been added in (remove from) (RCs)_i
     // and their relative entries have to to be rewritten in (deleted from)
     // LagMatrix

     // however, the the Lagrangian costs should be updated as follows:
     // c^y = c + y_i a_i x + sum_{p \neq i} y_p a_p x
     // a_i has to be re-written in LagMatrix to allow LagBFunction the
     // computation of the Lagrangian costs

     Vec_p_Var varsX = update_columns( vdp );
     set_original_costs( varsX ); // set the coefficients c_j of the newly added x_j

 	 // if there are some variables, say ZeroVars, which are active in (RCs) but
 	 // *not* active in (obj_B), then the same number of coeff_pair as the number
 	 // of ZeroVars have to be added to (obj_B) with zero-coefficient and
 	 // Variables pointers defined by ZeroVars

 	 if( tmod->f_type == FunctionModVars::AddVar )
 	  fix_sblock_objective();

     // issue C05FunctionModSbst modification of the type AllEntriesChanged:
     // the i-th entry of the linearization ( g , \alpha ) has to change,
     // namely g_i must be re-computed -by calling
     // get_linearization_coefficients- at the index i-th, however the
     // Lagrangian function unpredictably changes and f_shift is
     // set to NaN   - - - - - - - - - - - - - - - - - - - - - - - - - -
     if( f_Observer )
      f_Observer->add_Modification( std::make_shared<C05FunctionModSbst>( this ,
     		   C05FunctionModSbst::AllEntriesChanged , std::move(v_vars) ,
     		   true, FunctionMod::NaNshift , 0 ) , chnl );

     }
    else { // the changes of the constraints of (B) may violate the
       // solutions kept in the global pool,  signal
       // that the feasibility of the solutions has to be checked

       for( auto tpl : g_pool )
        std::get<2>( tpl ) = VarToBeChckd;

       // issue C05FunctionMod modification of the type AlphaChanged: the
       // feasible region of (B) changed and the original linearizations
       // (even the g part) can no longer be used
       if( f_Observer )
        f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
      	  C05FunctionMod::AlphaChanged , FunctionMod::NaNshift ) , chnl );

     }
   }
  } // end FunctionModVars   - - - - - - - - - - - - - - - - - - - - - - - - -

 // VariableMod: some variables of (B) changed the status
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<VariableMod>( mod );
  if( tmod ) {
   auto xj = dynamic_cast<ColVariable * const>( tmod->f_variable );

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
   if( f_Observer )
    f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
  	  C05FunctionMod::AlphaChanged , FunctionMod::NaNshift ) , chnl );

   }
  }  // end VariableMod- - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // BlockModAD - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // BlockModAD::eAddVar keep feasibility
 // BlockModAD::eDelConst keep feasibility
 {
  const auto tmod = std::dynamic_pointer_cast<BlockModAD>( mod );
  if( tmod ) {
   if( ( tmod->f_type == BlockModAD::eAddConst ) ||
       ( tmod->f_type == BlockModAD::eDelVar ) ) {

	// the remotion of variables and addition of constraints of (B) may violate
	// the feasibility of the global pool, signal that the feasibility of the
	// solutions must be checked  - - - - - - - - - - - - - - - - - - - - - -

    for( auto tpl : g_pool )
     std::get<2>( tpl ) = VarToBeChckd;

    // issue C05FunctionMod modification of the type AlphaChanged: the
    // feasible region of (B) changed and the original linearizations
    // (even the g part) can no longer be used

    if( f_Observer )
     f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
   	  C05FunctionMod::AlphaChanged , FunctionMod::NaNshift ) , chnl );

    }
   }
  }  // end BlockModAD - - - - - - - - - - - - - - - - - - - - - - - - - - - -

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

   if( f_Observer )
    f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
  	  C05FunctionMod::AlphaChanged , FunctionMod::NaNshift ) , chnl );

   }
  }  // end BlockMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


 }  // end( LagBFunction::guts_of_add_Modification( sp_Mod ) ) - - - - - - - -

/*--------------------------------------------------------------------------*/
/*---------------------- End File LagBFunction.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
