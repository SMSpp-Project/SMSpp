/*--------------------------------------------------------------------------*/
/*------------------------ File LagBFunction.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the LagBFunction class, which is derived from both a
 * C05Function and a Block and implements the concept of the Lagrangian 
 * function of some Block (the unique sub-Block of LagBFunction when "seen"
 * as a Block) w.r.t. a given set of linear terms.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Enrico Gorgone \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Enrico Gorgone, Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------- MACROS -----------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef NDEBUG
 #define CHECK_SOLUTIONS 0
 /* CHECK_SOLUTIONS, coded bit-wise, activates some checks about the solutions
  * that are generated and used to compute linearizations. This should not be
  * necessary, and it's costly, but it may be useful to catch some bugs in the
  * inner Block and/or its Solver.
  *
  * - bit 0: the feasibility of the solutions is checked
  *
  * - bit 1: the objective value returned by the Solver is compared with the
  *          value as computed by FRealObjective
  *
  * - bit 2: during store_combination_of_linearizations(), all the Solution
  *          are printed together with their coefficients, and the combined
  *          final solution is printed as well
  */
#else
 #define CHECK_SOLUTIONS 0
 // never change this
#endif

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <queue>

#include "BlockSolverConfig.h"

#include "BoxSolver.h"

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

// register LagBFunctionState to the State factory

SMSpp_insert_in_factory_cpp_0( LagBFunctionState );

/*--------------------------------------------------------------------------*/
/*------------------------------ LOCAL TYPES -------------------------------*/
/*--------------------------------------------------------------------------*/

using p_LF = LinearFunction *;

using p_QF = DQuadFunction *;

using SConf_p_p = SimpleConfiguration< std::pair< Configuration * ,
						  Configuration * > >;

/*--------------------------------------------------------------------------*/
/*----------------------- LOCAL const AND constexpr ------------------------*/
/*--------------------------------------------------------------------------*/

static constexpr Function::FunctionValue NaN = FunctionMod::NaNshift;

static constexpr Function::FunctionValue INF = FunctionMod::INFshift;

/*--------------------------------------------------------------------------*/
/*-------------------------------- FUNCTIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

template< typename T >
static void Compact( std::vector< T > & g , Function::c_Subset & B )
{
 // takes a "dense" n-vector g and "compacts" it deleting the elements whose
 // indices are in B; all elements of B must be in the range 0 .. n, B must
 // be ordered in increasing sense
 // the remaining entries in g are shifted left of the minimum possible
 // amount in order to fill the holes left by the deleted ones
 // g is *not* resized in here
 //
 // since T may be a "large" type, elements of T are moved rather than copied
 //
 // g is finally resized to the final size

 auto Bit = B.begin();
 auto i = *( Bit++ );
 auto git = g.begin() + ( i++ );

 for( ; Bit != B.end() ; ++i ) {
  auto h = *( Bit++ );
  while( i < h )
   *( git++ ) = std::move( g[ i++ ] );
  }

 std::copy( std::make_move_iterator( g.begin() + i ) ,
	    std::make_move_iterator( g.end() ) , git );

 g.resize( g.size() - B.size() );

 }  // end( Compact )

/*--------------------------------------------------------------------------*/
/*--------------------------- class LagBFunction ---------------------------*/
/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register LagBFunction to the Block factory
SMSpp_insert_in_factory_cpp_1( LagBFunction );

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

LagBFunction::LagBFunction( Block * innerblock , Observer * observer )
 : C05Function() , IsConvex( true ) , InnrSlvr( 0 ) , NoSol( false ) ,
   ChkState( false ) , PushCostToOwner( true ) , f_cost_tol( 1e-12 ) ,
   f_active_dirty( true ) , f_lazy_eval( false ) , f_max_glob( 0 ) ,
   LastSolution( 0 ) , VarSol( true ) , f_yb( -INF ) ,
   f_play_dumb( false ) , f_dirty_Lc( false ) , f_c_changed( false ) ,
   f_Lc( -1 ) , LPMaxSz( 0 ) , f_BSC( nullptr ) , f_CC( nullptr ) ,
   f_CC_changed( false ) , f_BS( nullptr ) , f_id( this )
{
 // set the pointer to the sub-Block (B) - - - - - - - - - - - - - - - - - - -
 if( innerblock )
  set_inner_block( innerblock );

 // set the observer pointer - - - - - - - - - - - - - - - - - - - - - - - - -
 if( observer )
  register_Observer( observer );

 init_CC();

 }  // end( LagBFunction::LagBFunction )

/*--------------------------------------------------------------------------*/

LagBFunction::~LagBFunction( void )
{
 guts_of_destructor();
 delete f_BS;
 }

/*--------------------------------------------------------------------------*/

void LagBFunction::clear( void )
{
 // delete all the Lagrangian terms (and the ColVariable with them)
 clear_lp();

 // delete the auxiliary data structure for computing the Lagrangian costs
 CostMatrix.clear();
 v_tmpCP.clear();

 // delete the global pool (do not issue any Modification because clear()
 // is only meant to be called right before destroying the Function, any
 // Solver attached to it should have been done with long ago
 if( ! NoSol )  // ... if there is anything to delete
  for( Index i = 0 ; i < f_max_glob ; ++i )
   delete g_pool[ i ].sol;
 g_pool.clear();
 f_max_glob = 0;
 f_yb = -INF;  // since b is empty, there are no nonzeros
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::set_inner_block( Block * innerblock , bool deleteold )
{
 // if there is an existing inner Block, cleanup it
 if( ! v_Block.empty() ) {
  if( ! deleteold ) {
   if( ! innerblock ) {           // was a cleanup
    guts_of_destructor( false );  // all done
    return;
    }
   }
  else
   guts_of_destructor( deleteold );
  }

 v_Block.resize( 1 );
 v_Block.front() = innerblock;
 innerblock->set_f_Block( this );

 // ensure the Objective of the inner Block is defined (and therefore the
 // Variable need to) because LagBFunction checks it
 innerblock->generate_abstract_variables();
 innerblock->generate_objective();

 const auto frobj = innerblock->get_objective< FRealObjective >();
 if( ! frobj )
  throw( std::invalid_argument(
			   "inner Block Objective not a FRealObjective" ) );

 IsConvex = ( frobj->get_sense() == Objective::eMax );

 if( PushCostToOwner ) {
  // Clear data structures (in case of reuse)
  v_Obj.clear();
  v_ObjIsQuad.clear();
  v_BlockBFS.clear();
  Block2Idx.clear();
  CostMatrix.clear();

  // BFS traversal of Block tree starting from innerblock
  std::queue< Block * > q;
  q.push( innerblock );

  while( ! q.empty() ) {
   Block * curr = q.front(); q.pop();

   Index h = v_BlockBFS.size();
   v_BlockBFS.push_back( curr );
   Block2Idx[ curr ] = h;

   FRealObjective * obj = static_cast< FRealObjective * >(
						     curr->get_objective() );
   v_Obj.push_back( obj );

   bool isQuad = dynamic_cast< const p_QF >( obj->get_function() ) != nullptr;
   v_ObjIsQuad.push_back( isQuad );

   // Init CostMatrix for each Block's objective
   if( ! isQuad ) {
    const auto & rp = static_cast< p_LF >( obj->get_function() )->get_v_var();
    m_column cm( rp.size() );
    for( Index i = 0 ; i < rp.size() ; ++i )
     cm[ i ].first = rp[ i ].second;
    CostMatrix.emplace_back( std::move( cm ) );
    }
   else {
    const auto & rp = static_cast< p_QF >( obj->get_function() )->get_v_var();
    m_column cm( rp.size() );
    for( Index i = 0 ; i < rp.size() ; ++i )
     cm[ i ].first = std::get< 1 >( rp[ i ] );
    CostMatrix.emplace_back( std::move( cm ) );
    }

   for( Index i = 0 ; i < curr->get_number_nested_Blocks() ; ++i )
    q.push( curr->get_nested_Block( i ) );
   }
  }
 else {
  // if we are not pushing the cost to the owner, build a single CostMatrix
  CostMatrix.clear();  // ensure empty

  m_column cm;
  auto * fn = frobj->get_function();

  if( auto * lf = dynamic_cast< p_LF >( fn ) ) {
   const auto & rp = lf->get_v_var();
   cm.resize( rp.size() );
   for( Index i = 0 ; i < rp.size() ; ++i )
    cm[ i ].first = rp[ i ].second;
   }
  else
   if( auto * qf = dynamic_cast< p_QF >( fn ) ) {
    const auto & rp = qf->get_v_var();
    cm.resize( rp.size() );
    for( Index i = 0 ; i < rp.size() ; ++i )
     cm[ i ].first = std::get< 1 >( rp[ i ] );
    }
   else
    throw( std::invalid_argument( "Unsupported objective function type" ) );

  CostMatrix.emplace_back( std::move( cm ) );

  // Also fill BFS metadata to keep the structure coherent
  v_Obj.clear();
  v_ObjIsQuad.clear();
  v_BlockBFS.clear();
  Block2Idx.clear();

  v_Obj.push_back( frobj );
  v_ObjIsQuad.push_back( dynamic_cast< const p_QF >( fn ) != nullptr );
  v_BlockBFS.push_back( innerblock );
  Block2Idx[ innerblock ] = 0;
  }

 v_tmpCP.clear();                // no terms to be stealthily added to obj yet
 v_tmpCP.resize( v_Obj.size() ); // one entry per objective block

 f_c_changed = false;  // Lagrangian costs are still == to original costs
 f_dirty_Lc = ! LagPairs.empty();  // ... hence they have to be updated,
                                   // unless the Lagrangian term is empty
 f_Lc = -1;            // the Lipschitz constant must be computed

 }  // end( LagBFunction::set_inner_block )

/*--------------------------------------------------------------------------*/

void LagBFunction::set_dual_pairs( v_dual_pair && dp )
{
 clear_lp();       // ensure we are starting from a "tabula rasa"
 for( auto & tmp : v_tmpCP )
  tmp.clear();     // no terms to be stealthily added to obj yet

 #ifndef NDEBUG
  // check that all the pairs in dp have distinct variables
  if( dp.size() > 1 ) {
   std::vector< Index > sorted( dp.size() );
   std::iota( sorted.begin() , sorted.end() , 0 );
   std::sort( sorted.begin() , sorted.end() ,
	      [ dp ]( Index i , Index j ) {
	       return( std::less< ColVariable * >{}( dp[ i ].first ,
						     dp[ j ].first ) );
	       } );
   for( Index i = 0 ; i < dp.size() - 1 ; ++i )
    if( dp[ sorted[ i ] ].first == dp[ sorted[ i + 1 ] ].first )
     throw( std::invalid_argument( "LagBFunction::set_dual_pairs: repeated "
				   "ColVariable in dp[ " +
				   std::to_string( sorted[ i ] ) +
				   " ] and dp[ "
				   + std::to_string( sorted[ i + 1 ] ) + " ]"
				   ) );
   }
 #endif

 // construct the auxiliary structure CostMatrix which is used to update the
 // Lagrangian cost vector
 //
 // CostMatrix is a vector of pairs < c_j , A_j > indexed like the primal
 // variable x_j = obj->get_active_var( j ). c_j is the original cost (which
 // would no longer be available in obj and therefore need to be stored
 // somewhere, and A_j is v_coeff_pair: a vector of pairs < i , a_{ij} >
 // (index of the ColVariable among the active ones in the LagBFunction, real
 // coefficient) describing the linear function y A^j required to compute the
 // Lagrangian cost c_j - y A^j

 add_to_CostMatrix( dp );

 // save the dual pairs in the LagPairs data structure
 LagPairs = std::move( dp );

 // ensure that LagBFunction is the Observer of the LinearFunction
 for( auto & p : LagPairs )
  p.second->register_Observer( this );

 f_yb = -INF;                       // b == 0
 for( auto const & lp : LagPairs )  // ... unless otherwise proven
  if( static_cast< p_LF >( lp.second )->get_constant_term() ) {
   f_yb = NaN; break;               // if so, yb has to be computed
   }

 // Lagrangian costs have to be updated unless by chance they are still the
 // original ones and the newly set Lagrangian term is actually empty
 f_dirty_Lc = f_c_changed || ( ! LagPairs.empty() );

 }  // end( LagBFunction::set_dual_pairs )

/*--------------------------------------------------------------------------*/

void LagBFunction::set_ComputeConfig( const ComputeConfig * scfg )
{
 auto inner_block = get_inner_block();
 if( ! inner_block )
  throw( std::logic_error( "incomplete LagBFunction configured" ) );

 if( scfg )
  if( scfg->f_extra_Configuration ) {
   BlockConfig * BC = nullptr;
   BlockSolverConfig * BSC = nullptr;

   if( auto scpp = dynamic_cast< SConf_p_p * >(
			                   scfg->f_extra_Configuration ) ) {
    if( scpp->f_value.first ) {
     BSC = dynamic_cast< BlockSolverConfig * >( scpp->f_value.first );
     if( ! BSC ) throw( std::invalid_argument(
      "LagBFunction::set_ComputeConfig: invalid extra_Configuration.fist" ) );
     }
    else
     if( ! scfg->diff() )  // if not in differential mode
      set_default_inner_BlockSolverConfig();  // reset the BlockSolverConfig

    if( scpp->f_value.second ) {
     BC = dynamic_cast< BlockConfig * >( scpp->f_value.second );
     if( ! BC ) throw( std::invalid_argument(
     "LagBFunction::set_ComputeConfig: invalid extra_Configuration.second" ) );
     }
    else
     if( ! scfg->diff() )  // if not in differential mode
      set_default_inner_BlockConfig();  // reset the BlockConfig
    }
   else
    if( ! ( BC = dynamic_cast< BlockConfig * >(
				            scfg->f_extra_Configuration ) ) )
     if( ! ( BSC = dynamic_cast< BlockSolverConfig * >(
				            scfg->f_extra_Configuration ) ) )
      throw( std::invalid_argument(
	   "LagBFunction::set_ComputeConfig: invalid extra_Configuration" ) );

   if( BC )     // set the BlockConfig of the inner Block, if any
    BC->apply( inner_block );

   if( BSC ) {  // set the BlockSolverConfig of the inner Block, if any
    if( f_BSC ) {
     // if a previous clear()-ed BSC is present, apply() it so as to "clean"
     // the Block for the arrival of the new one
     // BlockSolverConfig is in "set mode", since this would reset everything
     f_BSC->apply( inner_block );
     delete f_BSC;                     // delete the old one
     }

    BSC->apply( inner_block );         // apply the new BlockSolverConfig
    f_BSC = BSC->clone();                // keep a copy of the new one
    f_BSC->clear();                      // but clear it
    }
   }
  else {  // scfg->f_extra_Configuration is nullptr
   if( ! scfg->diff() )  // if not in differential mode
    set_default_inner_Block_configuration();  // reset everything
   }
 else     // scfg == nullptr
  set_default_inner_Block_configuration();  // reset everything

 // finally, set the parameters of LagBFunction itself: this needs to be
 // done last because it may change some of the parameters of the Solver
 // used to solve the inner Block, that may not exist before the
 // extra Configuration is apply()-ed to the inner Block
 //
 // note that the inner Solver may be changing and some other parameters
 // actually are parameters of the inner Solver; thus, ensure that the
 // change in InnrSlvr is acted upon first
 for( const auto & pair : scfg->int_pars )
  if( pair.first == "intInnrSlvr" )
   set_par( intInnrSlvr , pair.second );

 // now do all the rest
 ThinComputeInterface::set_ComputeConfig( scfg );

 }  // end( LagBFunction::set_ComputeConfig )

/*--------------------------------------------------------------------------*/

void LagBFunction::set_par( idx_type par , int value )
{
 if( par < intLastAlgParTCI ) {
  add_par( int_par_idx2str( par ) , value );
  return;
  }

 if( par >= intLastLagBFPar ) {
  if( auto is = inner_Solver() )
   add_par( is->int_par_idx2str( int_par_lbf( par ) ) , value );
  return;
  }

 switch( par ) {
  case( intLPMaxSz ):  // intLPMaxSz- - - - - - - - - - - - - - - - - - - - -
   if( LPMaxSz != value ) {
    LPMaxSz = value;
    add_par( "intMaxSol" , value );
    }
   break;
  case( intGPMaxSz ):  // intGPMaxSz- - - - - - - - - - - - - - - - - - - - -
   if( ( LastSolution< Inf< Index >() ) && ( LastSolution >= g_pool.size() ) )
    // LastSolution is undefined: ensure it remains so even if
    LastSolution = value;         // the global pool grows
   // note: if the global pool shrinks and LastSolution is one of the deleted
   //       ones it is >= value, which automatically means "undefined"
   if( g_pool.size() > Index( value ) ) {
    if( ! NoSol )  // ... if there are Solution a all
     for( auto it = g_pool.begin() + value ; it != g_pool.end() ; ++it  )
      delete it->sol;

    if( f_max_glob > Index( value ) ) {
     f_max_glob = value;
     update_f_max_glob();
     }    
    }
   g_pool.resize( value );
   break;
  case( intInnrSlvr ):  // intInnrSlvr - - - - - - - - - - - - - - - - - - -
   if( InnrSlvr != Index( value ) ) {
    InnrSlvr = Index( value );
    // ensure there is a ComputeConfig in diff mode ready
    while( f_BSC->num_ComputeConfig() <= InnrSlvr ) {
     auto cc = new ComputeConfig;
     cc->set_diff( true );
     f_BSC->add_ComputeConfig( "" , cc );
     }
    }
   break;
  case( intNoSol ):  // intNoSol - - - - - - - - - - - - - - - - - - - - - -
   if( ( value > 0 ) && ( NoSol == false ) ) {
    NoSol = true;
    // setting NoSol == true when it was false: throw away all Solution
    // currently stored in the global pool
    for( Index i = 0 ; i < f_max_glob ; ++i )
     if( g_pool[ i ].sol ) {
      delete g_pool[ i ].sol;
      // any surely nonzero address
      g_pool[ i ].sol = reinterpret_cast < Solution * >( this );
      }
    break;
    }
   if( ( value == 0 ) && ( NoSol == true ) ) {
    NoSol = false;
    // setting NoSol == false when it was true: has to cleanup all the
    // global pool since the information there is not reliable (no
    // Solution is a real Solution)
    for( Index i = 0 ; i < f_max_glob ; ++i )
     g_pool[ i ].sol = nullptr;
    f_max_glob = 0;

    // if somebody is listening (assuming issueMod == eModBlck) 
    if( f_Observer && ( f_Observer->issue_mod( eModBlck ) ) )
     // issue a LagBFunctionMod with type() == GlobalPoolRemoved and
     // which().empty(); however, shift() == 0 since the function itself
     // has not really changed
     f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                    this , C05FunctionMod::GlobalPoolRemoved ,
                                    Subset() , 64 , 0 , true ) ,
                                   eModBlck );
    }
   break;
  case( intChkState ):  // intChkState - - - - - - - - - - - - - - - - - - -
   ChkState = ( value > 0 );
   break;
  case( intPoolExtMem ):  // intPoolExtMem - - - - - - - - - - - - - - - - - -
   f_lazy_eval = ( value != 0 );  // 1 = re-read via sol->write (lazy)
   break;
  case( intPushCostToOwner ): // intPushCostToOwner - - - - - - - - - - - - -
   bool new_val = ( value != 0 );
   if( PushCostToOwner != new_val ) {
    PushCostToOwner = new_val;
    if( get_inner_block() && ( ! v_Obj.empty() ) )
     set_inner_block( get_inner_block() , false );
   }
  }
 }  // end( LagBFunction::set_par( int ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::set_par( idx_type par , double value )
{
 if( par == dblCostTol ) {
  f_cost_tol = value;
  return;
  }

 if( par < dblLastAlgParTCI ) {
  add_par( dbl_par_idx2str( par ) , value );
  return;
  }

 if( par >= dblLastLagBFPar ) {
  if( auto is = inner_Solver() )
   add_par( is->dbl_par_idx2str( dbl_par_lbf( par ) ) , value );
  return;
  }

 switch( par ) {
  case( dblRelAcc ):   add_par( "dblRelAcc"   , value ); break;
  case( dblAbsAcc ):   add_par( "dblAbsAcc"   , value ); break;
  case( dblUpCutOff ): add_par( "dblUpCutOff" , value ); break;
  case( dblLwCutOff ): add_par( "dblLwCutOff" , value ); break;
  case( dblRAccLin ):  add_par( "dblRAccSol"  , value ); break;
  case( dblAAccLin ):  add_par( "dblAAccSol"  , value );
  }
 }  // end( LagBFunction::set_par( double ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::deserialize( const netCDF::NcGroup & group )
{
 throw( std::logic_error( "LagBFunction::deserialize not implemented yet" ) );

 guts_of_destructor();  // cleanup whatever is there now

 // ensure f_CC is there (it is deleted in guts_of)
 init_CC();

 f_c_changed = false;   // Lagrangian costs are still == to original costs
 f_dirty_Lc = ! LagPairs.empty();  // ... hence they have to be updated,
                                   // unless the Lagrangian term is empty
 f_yb = INF;            // have to check if b == 0 or not
 f_Lc = -1;             // the Lipschitz constant must be computed

 // now the inner Block - - - - - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcGroup sb = group.getGroup( "B" );
 if( sb.isNull() )
  throw( std::invalid_argument( "no inner Block provided" ) );

 v_Block.push_back( new_Block( sb , this ) );

 // now the Lagrangian term < y , g( x ) >- - - - - - - - - - - - - - - - - -
 //!! not implemented yet

 // call the method of Block- - - - - - - - - - - - - - - - - - - - - - - - -
 // inside this the NBModification, the "nuclear option",  is issued

 Block::deserialize( group );

 }  // end( LagBFunction::deserialize )

/*--------------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::add_dual_pairs( v_dual_pair && dp , ModParam issueMod )
{
 if( dp.empty() )  // adding nothing
  return;          // cowardly (and silently) return

 // update CostMatrix- - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 add_to_CostMatrix( dp );

 // if b == 0, check if this remains true- - - - - - - - - - - - - - - - - - -
 if( f_yb == -INF )
  for( auto const & el : dp )
   if( static_cast< p_LF >( el.second )->get_constant_term() ) {
    f_yb = NaN; break;  // if not, yb has to be computed
    }

 f_dirty_Lc = true;  // Lagrangian costs have to be updated
 f_Lc = -1;          // the Lipschitz constant must be computed

 // ensure that LagBFunction is the Observer of the new LinearFunction
 for( auto & p : dp )
  p.second->register_Observer( this );

 // merge the list of dual Lagrangian pairs  - - - - - - - - - - - - - - - - -
 // be sure to use std::make_move_iterator() to have the contents of lp moved
 // into LagPairs rather than copied

 Index k = LagPairs.size();
 LagPairs.insert( LagPairs.end() , std::make_move_iterator( dp.begin() ) ,
		                   std::make_move_iterator( dp.end() ) );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // construct the set of added Variable for the C05FunctionModVarsAddd
 Vec_p_Var vars( LagPairs.size() - k );
 for( Index i = k ; i < LagPairs.size() ; ++i )
  vars[ i - k ] = LagPairs[ i ].first;

 // a Lagrangian function is strongly quasi-additive: shift() == 0
 f_Observer->add_Modification( std::make_shared< C05FunctionModVarsAddd >(
                                this , std::move( vars ) , k , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( LagBFunction::add_dual_pairs )

/*--------------------------------------------------------------------------*/

void LagBFunction::remove_variable( Index i , ModParam issueMod )
{
 if( i >= LagPairs.size() )
  throw( std::invalid_argument( "LagBFunction::remove_variable: wrong index"
				) );

 // update CostMatrix - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 for( Index h = 0 ; h < CostMatrix.size() ; ++h ) {
  auto & CMh = CostMatrix[ h ];

  for( auto & col : CMh ) {
   auto & Aj = col.second;  // ordered vector of mon_pair < y_name , a_{ij} >

   // search < i , * >
   auto it_i = std::lower_bound( Aj.begin() , Aj.end() ,
                                 mon_pair( i , 0 ) ,
                                 []( const auto & a , const auto & b )
                                 { return( a.first < b.first ); } );

   if( ( it_i != Aj.end() ) && ( it_i->first == i ) ) {
    // decrement all subsequent names
    for( auto jt = it_i + 1 ; jt != Aj.end() ; ++jt )
     --( jt->first );

    // and remove exactly < i , a_{ij} >
    Aj.erase( it_i );
    }
   else {
    // no < i , * >: shift only names > i
    auto jt = std::lower_bound( Aj.begin() , Aj.end() ,
                                mon_pair( i + 1 , 0 ) ,
                                []( const auto & a , const auto & b )
                                { return( a.first < b.first ); } );
    for( ; jt != Aj.end() ; ++jt )
     --( jt->first );
    }
   // NOTE: do not delete columns even if they become empty: objective
   // alignment
   }
  }

 // if b != 0 but we are eliminating a nonzero, it may have become 0 - - - - -
 if( ( f_yb > -INF ) &&
     ( static_cast< p_LF >( LagPairs[ i ].second )->get_constant_term() ) )
  f_yb = INF;  // if so, signal to check if b == 0 or not

 f_Lc = -1;    // the Lipschitz constant must be computed

 // now actually eliminate the row from LagPairs - - - - - - - - - - - - - - -
 auto itv = LagPairs.begin() + i;
 auto var = itv->first;

 if( itv->second->get_num_active_var() > 0 )
  f_dirty_Lc = true;
 // Lagrangian costs have to be updated unless in the strange case where the
 // removed variable had an empty corresponding Lagrangian term

 // delete the LinearFunction in the to-be-deleted LagPairs[ i ]
 delete itv->second;
 LagPairs.erase( itv );       // now erase it

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // a Lagrangian function is strongly quasi-additive: shift() == 0
 f_Observer->add_Modification( std::make_shared< C05FunctionModVarsRngd >(
                                this , Vec_p_Var( { var } ) ,
                                Range( i , i + 1 ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( LagBFunction::remove_variable )

/*--------------------------------------------------------------------------*/

void LagBFunction::remove_variables( Range range , ModParam issueMod )
{
 range.second = std::min( range.second , Index( LagPairs.size() ) );
 if( range.second <= range.first )  // actually nothing to remove
  return;                           // cowardly (and silently) return

 if( ( ! range.first ) && ( range.second >= LagPairs.size() ) ) {
  // removing *all* variables
  if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
   // an Observer is there: copy the names of deleted Variables (all of them)
   Vec_p_Var vars( LagPairs.size() );

   for( Index i = 0 ; i < LagPairs.size() ; ++i )
    vars[ i ] = LagPairs[ i ].first;

   clear_lp();  // then clear the LagBFunction

   // now issue the Modification: note that the subset is empty
   // a LagBFunction is strongly quasi-additive
   f_Observer->add_Modification( std::make_shared< C05FunctionModVarsSbst >(
                                  this , std::move( vars ) , Subset() , true ,
                                  0 , Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );
  }
  else          // no-one is listening
   clear_lp();  // just do it

  f_dirty_Lc = f_c_changed;  // since the Lagrangian term is now empty, the
                             // Lagrangian costs should be the original costs,
                             // so they will have to be modified unless by
                             // chance they already are so

  f_yb = -INF;  // b is empty, hence there are no nonzeros
  return;
 }

 // this is not a complete reset
 // update CostMatrix - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const Index delt = range.second - range.first;

  // for each Block/Objective, for each column A_j: remove all the pairs
  // < y_k , a_{kj} > with k in [range.first , range.second) and decrement by
  // 'delt' all the names k >= range.second
  for( Index h = 0 ; h < CostMatrix.size() ; ++h ) {
   auto & CMh = CostMatrix[ h ];
   for( auto & col : CMh ) {
    auto & Aj = col.second;  // ordered vector of mon_pair < y_name , a_{ij} >

    // first k >= range.first
    auto iit = std::lower_bound( Aj.begin() , Aj.end() ,
                                 mon_pair( range.first , 0 ) ,
                                 []( const auto & a , const auto & b )
                                 { return( a.first < b.first ); } );

    if( iit == Aj.end() )
     continue;

    // first k >= range.second
    auto eit = std::lower_bound( iit , Aj.end() ,
                                 mon_pair( range.second , 0 ) ,
                                 []( const auto & a , const auto & b )
                                 { return( a.first < b.first ); } );

    // shift subsequent names
    for( auto jt = eit ; jt != Aj.end() ; ++jt )
     ( jt->first ) -= delt;

    // delete the block [range.first , range.second)
    if( eit != iit )
     Aj.erase( iit , eit );

    // NOTE: do not delete the column from CostMatrix[ h ] even if now empty,
    // because the Objective still exists and we want to keep the alignment
    }
   }
  }

 // if b != 0 but we are eliminating nonzeros, it may have become 0- - - - - -
 const auto strtit = LagPairs.begin() + range.first;
 const auto stopit = LagPairs.begin() + range.second;

 if( f_yb > -INF )
  for( auto lpit = strtit ; lpit != stopit ; ++lpit )
   if( static_cast< p_LF >( lpit->second )->get_constant_term() ) {
    f_yb = INF; break;  // if so, signal to check if b == 0 or not
   }

 f_Lc = -1;  // the Lipschitz constant must be computed

 // now actually eliminate the rows from LagPairs - - - - - - - - - - - - - -
 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vars( range.second - range.first );
  auto vpit = vars.begin();
  for( auto tmpit = strtit ; tmpit < stopit ; ) {
   // if any of the removed Lagrangian terms is nonempty, Lagrangian costs
   // will have to be updated
   if( tmpit->second->get_num_active_var() > 0 )
    f_dirty_Lc = true;
   *( vpit++ ) = ( tmpit++ )->first;
   }

  // delete the LinearFunction(s) in the to-be-deleted LagPairs[ i ]
  for( auto LPi = strtit ; LPi < stopit ; ++LPi )
   delete LPi->second;

  LagPairs.erase( strtit , stopit );  // now erase them

  // a Lagrangian function is strongly quasi-additive: shift() == 0
  f_Observer->add_Modification( std::make_shared< C05FunctionModVarsRngd >(
                                 this , std::move( vars ) , range , 0 ,
                                 Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );
  }
 else {  // noone is there: just do it
  // if any of the removed Lagrangian terms is nonempty, Lagrangian costs
  // will have to be updated
  if( ! f_dirty_Lc )
   for( auto tmpit = strtit ; tmpit != stopit ; ++tmpit )
    if( tmpit->second->get_num_active_var() > 0 ) {
     f_dirty_Lc = true;
     break;
    }

  // delete the LinearFunction(s) in the to-be-deleted LagPairs[ i ]
  for( auto LPi = strtit ; LPi < stopit ; ++LPi )
   delete LPi->second;

  LagPairs.erase( strtit , stopit );  // now erase them
  }
 }  // end( LagBFunction::remove_variables( range ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::remove_variables( Subset && nms , bool ordered ,
				     ModParam issueMod )
{
 if( nms.empty() ) {  // removing all Variables
  if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
   // an Observer is there: copy the names of deleted Variables (all of them)
   Vec_p_Var vars( LagPairs.size() );

   for( Index i = 0 ; i < LagPairs.size() ; ++i )
    vars[ i ] = LagPairs[ i ].first;

   clear_lp();  // then clear the LagBFunction

   // now issue the Modification: note that the subset is empty
   // a LagBFunction is strongly quasi-additive
   f_Observer->add_Modification( std::make_shared< C05FunctionModVarsSbst >(
                                  this , std::move( vars ) , Subset() , true ,
                                  0 , Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );
   }
  else          // no-one is listening
   clear_lp();  // just do it

  f_dirty_Lc = f_c_changed;  // since the Lagrangian term is now empty, the
                             // Lagrangian costs should be the original costs,
                             // so they will have to be modified unless by
                             // chance they already are so
  f_yb = -INF;  // b is empty, hence there are no nonzeros
  return;
  }

 f_Lc = -1;     // the Lipschitz constant must be computed

 // this is not a complete reset
 if( ! ordered )
  std::sort( nms.begin() , nms.end() );

 if( nms.back() >= LagPairs.size() )
  throw( std::invalid_argument( "LagBFunction::remove_variables: wrong index"
				) );

 // update CostMatrix - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // for each Block/Objective, for each column A_j:
 //   - delete all pairs < y_k , a_{kj} > with k in nms
 //   - decrement the name k by how many entries of nms are < k
 {
  if( ! nms.empty() )
   for( Index h = 0 ; h < CostMatrix.size() ; ++h ) {
    auto & CMh = CostMatrix[ h ];
    for( auto & col : CMh ) {
     auto & Aj = col.second;  // ordered vector of mon_pair < y_name , a_{ij} >
     if( Aj.empty() )
      continue;

     auto nit   = nms.begin();       // iterates over indices to remove
     Index cnt  = 0;                 // how many removals seen so far (< k)
     auto wit   = Aj.begin();        // compact write position

     for( auto itp = Aj.begin() ; itp != Aj.end() ; ++itp ) {
      // advance nms while it removes indices < itp->first (only shift)
      while( ( nit != nms.end() ) && ( *nit < itp->first ) ) { ++nit; ++cnt; }

      if( ( nit != nms.end() ) && ( *nit == itp->first ) ) {
       // this term has y_k to delete: skip (and count this removal too)
       ++nit; ++cnt;
      }
      else {
       // keep the term, but shift the y_k index by the number of removals
       // seen
       wit->first  = itp->first  - cnt;
       wit->second = itp->second;
       ++wit;
      }
     }

     // erase the now-unused tail
     if( wit != Aj.end() )
      Aj.erase( wit , Aj.end() );

     // NOTE: do not delete the column from CostMatrix[ h ] even if now empty,
     // because the Objective still exists and we want to keep the alignment
    }
   }
  }

 // if b != 0 but we are eliminating nonzeros, it may have become 0 - - - - -
 if( f_yb > -INF )
  for( Index i : nms )
   if( static_cast< p_LF >( LagPairs[ i ].second )->get_constant_term() ) {
    f_yb = INF; break;  // if so, signal to check if b == 0 or not
   }

 // if any of the removed Lagrangian terms is nonempty, Lagrangian costs
 // will have to be updated
 if( ! f_dirty_Lc )
  for( auto i : nms )
   if( LagPairs[ i ].second->get_num_active_var() > 0 ) {
    f_dirty_Lc = true;
    break;
    }

 // now actually eliminate the rows from LagPairs - - - - - - - - - - - - - -
 // first of all delete the affected LinearFunction
 for( auto idx : nms )
  delete LagPairs[ idx ].second;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification
  // (as it will be destroyed during the process)

  auto it = nms.begin();
  auto vi = *it;    // first element to be eliminated
  auto curr = LagPairs.begin() + vi;   // position where to move stuff
  Vec_p_Var vars( nms.size() );
  auto its = vars.begin();

  *( its++ ) = LagPairs[ *( it++ ) ].first;
  ++vi;              // skip the first element, it will be overwritten

  for( ; it < nms.end() ; ++vi )
   if( *it == vi )                // one element to be eliminated
    *( its++ ) = LagPairs[ *( it++ ) ].first;  // skip it, but keep the Variable
   else
    *( curr++ ) = std::move( LagPairs[ vi ] );  // move in the current position

  // copy the last part after the last of nms[]
  std::copy( std::make_move_iterator( LagPairs.begin() + vi ) ,
	     std::make_move_iterator( LagPairs.end() ) , curr );

  LagPairs.resize( LagPairs.size() - nms.size() );  // erase the last part

  // a Lagrangian function is strongly quasi-additive: shift() == 0
  f_Observer->add_Modification( std::make_shared< C05FunctionModVarsSbst >(
                                 this , std::move( vars ) ,
                                 std::move( nms ) , ordered , 0 ,
                                 Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );
  }
 else    // noone is there: just do it
  Compact( LagPairs , nms );

 }  // end( LagBFunction::remove_variables( subset ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::cleanup_inner_objective( void )
{
 if( ! f_c_changed )  // Lagrangian costs are already == to original costs
  return;             // nothing to do

 f_play_dumb = true;  // ignore any ensuing Modification

 // for each Block's objective
 for( Index h = 0 ; h < CostMatrix.size() ; ++h ) {
  const auto & CMh = CostMatrix[ h ];

  // construct the vector of original coefficients
  Vec_FunctionValue NC( CMh.size() );
  for( Index i = 0 ; i < CMh.size() ; ++i )
   NC[ i ] = CMh[ i ].first;

  // modify the objective (linear or quadratic)
  if( ! v_ObjIsQuad[ h ] )
   static_cast< p_LF >( v_Obj[ h ]->get_function()
			)->modify_coefficients( std::move( NC ) );
  else
   static_cast< p_QF >( v_Obj[ h ]->get_function()
			)->modify_linear_coefficients( std::move( NC ) );
  }

 f_play_dumb = false;  // back to normal operations
 f_c_changed = false;  // Lagrangian costs are now == to original costs
 f_dirty_Lc = ! LagPairs.empty();  // ... hence they have to be updated,
 // unless the Lagrangian term is empty

 }  // end( LagBFunction::cleanup_inner_objective() )

/*--------------------------------------------------------------------------*/

void LagBFunction::add_Modification( sp_Mod mod , ChnlName chnl )
{
 // if f_play_dumb == true, ignore any Modification coming directly from the
 // inner Block because that's a "self-inflicted Modification" that
 // LagBFunction caused by modifying the Objective of the inner Block
 // THESE Modification ARE ALSO NOT FORWARDED TO THE f_Block OF THE
 // LagBFunction, IF IT HAS BEEN SET, THE IDEA BEING THAT THEY ARE "TEMPORARY"
 // AND THEY WILL BE UNDONE BY THE NEXT TIME ANYTHING ELSE WILL BE ALLOWED TO
 // LOOK AT THE sub-Block (cf. cleanup_inner_objective())
 if( f_play_dumb && ( std::find( v_BlockBFS.begin() ,
                                 v_BlockBFS.end() ,
                                 mod->get_Block() ) != v_BlockBFS.end() ) )
  return;

 // if the Modification requires it, now check all the Solution in the global
 // pool for feasibility; any one found to be unfeasible is deleted, and if
 // this happen an appropriate C05FunctionMod is issued- - - - - - - - - - - -

 if( auto what = guts_of_add_Modification( mod.get() , chnl ) ) {
  f_Lc = -1;      // the Lipschitz constant must be computed
  Index cnt = 0;  // how many linearizations are there
  Subset which;   // which ones get eliminated

  // only run the elimination loop if Solution are there, otherwise assume
  // the worst and remove everything
  if( NoSol ) {
   for( Index i = 0 ; i < f_max_glob ; ++i )
    g_pool[ i ].sol = nullptr;
   f_max_glob = 0;
   }
  else {
   for( Index i = 0 ; i < f_max_glob ; ++i ) {
    if( g_pool[ i ].sol ) {  // a Solution is there
     ++cnt;

     // write it in the Variable of the inner Block
     g_pool[ i ].sol->write( v_Block.front() );
     LastSolution = i;  // and recall what's there

     // check it's still a feasible solution/direction
     bool feas = g_pool[ i ].varsol ? v_Block.front()->is_feasible()
                                    : v_Block.front()->is_unbounded();
     if( ! feas ) {              // if not
      delete g_pool[ i ].sol;  // eliminate it
      g_pool[ i ].sol = nullptr;
      which.push_back( i );      // recall its name
      LastSolution = g_pool.size();
      // say that no Solution is saved in the Block, since the name is now
      // available again for a different Solution
      }
     }
    }
   update_f_max_glob();
   }

  // if nobody is listening (assuming issueMod == eModBlck)
  if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( eModBlck ) ) )
   return;  // all done
  
  // issue a LagBFunctionMod: if some linearizations have been removed it has
  // type() == GlobalPoolRemoved, otherwise it has type() == NothingChanged
  // note: the explicit definition of type here was originally avoided by
  //       having the ? expression directly in the constructor, but this
  //       meant that the same expression had a check if which was nonempty
  //       and a std-move of which that could make it empty, i.e., the
  //       perfect example of an expression with side-effects whose result
  //       depended on the order of the sub-expressions and therefore was
  //       compiler-dependent, meaning extremely-hard-to-find errors 
  auto type = which.empty() ? C05FunctionMod::NothingChanged
                            : C05FunctionMod::GlobalPoolRemoved;

  // in both cases it has shift() == NaN, since even if by chance none of the
  // existing linearizations is affected (but this may simply be because
  // there is none) the value of the function in general has changed
  // unpredictably if all linearizations have been removed, then pass an
  // empty Subset
  if( cnt == which.size() )
   which.clear();
 
  f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
				    this , type , std::move( which ) , what ,
				    C05FunctionMod::NaNshift , true ) ,
				chnl );

  }  // end( if( checking is required ) )

 // finally, if the father Block of LagBFunction is defined, forward the
 // Modification there; this is done, e.g., for the case where the sub-Block
 // (B) in the LagBFunction was actually a sub-Block of some other Block and
 // it has been "stolen" from it, but one still wants the Modification to
 // reach the original father. hence, this is done only for Modification
 // that actually come from the inner Block (or one of its sub-Block,
 // recursively, i.e., avoiding those coming from the LagBFunction itself,
 // which are those corresponding to changes in the linking constraints

 if( mod->get_Block() != this )
  if( auto fthr = get_f_Block() )
   fthr->add_Modification( mod , chnl );

 }  // end( LagBFunction::add_Modification() )

/*--------------------------------------------------------------------------*/
/*--------- METHODS FOR PRINTING/SAVING THE DATA OF THE LagBFunction -------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::print( std::ostream & output , char vlvl ) const
{
 output << "LagBFunction [" << this << "]" << " with "
	<< get_num_active_var() << " active variables" << std::endl;

 }  // end( LagBFunction::print )

/*--------------------------------------------------------------------------*/

void LagBFunction::serialize( netCDF::NcGroup & group ) const
{
 throw( std::logic_error( "LagBFunction::serialize not implemented yet" ) );

 // call the method of Block- - - - - - - - - - - - - - - - - - - - - - - - -

 Block::serialize( group );

 // now the Lagrangian term < y , g(x) >- - - - - - - - - - - - - - - - - - -
 //!! not implemented yet

 // now the inner Block - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( v_Block.size() != 1 )
  throw( std::invalid_argument( "exactly one sub-Block expected" ) );

 netCDF::NcGroup sb = group.addGroup( "B" );

 if( ! f_c_changed ) {  // if the costs are still the original ones
  v_Block.front()->serialize( sb );  // just do it
  return;                            // nothing else to do
  }

 // temporarily put back the original costs into the Objective of the
 // inner Block before deserializing, and then restore the current one,
 // which requires locking it
 // note: one could be tempted to run modify_coefficients() with eNoMod, so
 //       that no Modification at all is issued. this would be OK if the
 //       inner Block only had the abstract representation, since then
 //       changing it is all it is needed to put its state back to the
 //       original one prior serialization. however, doing so would not
 //       update any physical representation of the inner Block, which
 //       would therefore not be serialised correctly
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 bool owned = v_Block.front()->is_owned_by( f_id );
 if( ( ! owned ) && ( ! v_Block.front()->lock( f_id ) ) )
  throw( std::logic_error( "cannot lock inner Block" ) );

 // The costs saved in (obj_B) are the Lagrangian ones. Hence, we need
 // to restore the original ones before serializing (B).
 // ignore any ensuing Modification: note the horribly dirty trick of
 // explicitly const_casting away const-ness from this in order to be able
 // to (temporarily) change a field of the class inside a const method
 const_cast< LagBFunction * >( this )->f_play_dumb = true;

 for( Index h = 0 ; h < v_Obj.size() ; ++h ) {
  auto * f = v_Obj[ h ]->get_function();

  Vec_FunctionValue NCoef1, NCoef2;

  if( auto * lf = dynamic_cast< p_LF >( f ) ) {
   const auto & ov_pair = lf->get_v_var();
   const auto nv = lf->get_num_active_var();
   NCoef1.resize( nv );
   NCoef2.resize( nv );
   for( Index i = 0 ; i < nv ; ++i ) {
    NCoef1[ i ] = CostMatrix[ h ][ i ].first;
    NCoef2[ i ] = ov_pair[ i ].second;
   }

   lf->modify_coefficients( std::move( NCoef1 ) );
   }
  else
   if( auto * qf = dynamic_cast< p_QF >( f ) ) {
    const auto & ov_triples = qf->get_v_var();
    const auto nv = qf->get_num_active_var();
    NCoef1.resize( nv );
    NCoef2.resize( nv );
    for( Index i = 0 ; i < nv ; ++i ) {
     NCoef1[ i ] = CostMatrix[ h ][ i ].first;
     NCoef2[ i ] = std::get< 1 >( ov_triples[ i ] );
     }

    qf->modify_linear_coefficients( std::move( NCoef1 ) );
    }
   else
    throw( std::logic_error(
		    "LagBFunction::serialize: unsupported Function type" ) );
  }

 // serialize the sub-block
 v_Block.front()->serialize( sb );

 // put back the Lagrangian costs
 for( Index h = 0 ; h < v_Obj.size() ; ++h ) {
  auto * f = v_Obj[ h ]->get_function();

  Vec_FunctionValue NCoef;

  if( auto * lf = dynamic_cast< p_LF >( f ) ) {
   const auto & ov_pair = lf->get_v_var();
   const auto nv = lf->get_num_active_var();
   NCoef.resize( nv );
   for( Index i = 0 ; i < nv ; ++i )
    NCoef[ i ] = ov_pair[ i ].second;

   lf->modify_coefficients( std::move( NCoef ) );
   }
  else
   if( auto * qf = dynamic_cast< p_QF >( f ) ) {
    const auto & ov_triples = qf->get_v_var();
    const auto nv = qf->get_num_active_var();
    NCoef.resize( nv );
    for( Index i = 0 ; i < nv ; ++i )
     NCoef[ i ] = std::get< 1 >( ov_triples[ i ] );

    qf->modify_linear_coefficients( std::move( NCoef ) );
    }
  }

 // back to normal operations
 const_cast< LagBFunction * >( this )->f_play_dumb = false;

 if( ! owned )
  v_Block.front()->unlock( f_id );  // unlock it

 }  // end( LagBFunction::serialize )

/*--------------------------------------------------------------------------*/
/*----------- METHODS FOR HANDLING THE State OF THE LagBFunction -----------*/
/*--------------------------------------------------------------------------*/

State * LagBFunction::get_State( void ) const {
 return( new LagBFunctionState( this ) );
 }

/*--------------------------------------------------------------------------*/

void LagBFunction::put_State( const State & state )
{
 // restores gpool_el::sol/::varsol AND ::value/::convexified (eager/lazy);
 // ::conv_active is a rebuildable cache and is reset to empty (the restored
 // entries fall back to sol->write until re-stored).

 // if state is not a LagBFunctionState &, exception will be thrown
 const auto & s = dynamic_cast< const LagBFunctionState & >( state );

 // ensure g_pool is large enough
 if( s.f_max_glob > g_pool.size() )
  g_pool.resize( s.f_max_glob );

 // copy the important linearization information
 zLC = s.zLC;

 bool gpempty = ( f_max_glob > 0 );
 Subset Addd;

 // first void the current global pool
 if( NoSol ) {
  std::fill( g_pool.begin() , g_pool.end() , gpool_el() );
  f_max_glob = 0;
  }
 else {
  for( auto & el : g_pool ) {
   delete el.sol;
   el.sol = nullptr;
   el.varsol = true;
   el.value = 0;               // clear stale eager/lazy data on voided slots
   el.convexified = false;
   el.conv_active.clear();
   }

  // now add back all the Solution in the State (possibly after a check)
  auto gpit = g_pool.begin();

  if( ChkState )  // if Solutions are checked
   for( Index i = 0 ; i < s.g_pool.size() ; ++i ) {
    if( s.g_pool[ i ].sol ) {
     // write the Solution to the inner Block
     s.g_pool[ i ].sol->write( v_Block.front() );

     // if it's still a feasible solution/direction, copy it
     if( ( s.g_pool[ i ].varsol ? v_Block.front()->is_feasible()
	                        : v_Block.front()->is_unbounded() ) ) {
      gpit->sol = s.g_pool[ i ].sol->clone();  // clone() the Solution in
      gpit->varsol = s.g_pool[ i ].varsol;
      gpit->value = s.g_pool[ i ].value;            // eager/lazy constant
      gpit->convexified = s.g_pool[ i ].convexified;
      gpit->conv_active.clear();                    // cache: rebuilt lazily
      Addd.push_back( i );
      f_max_glob = i + 1;
      }
     }
    ++gpit;
    }
  else {        // it is trusted that Solution are correct
   for( Index i = 0 ; i < s.g_pool.size() ; ++i ) {
    if( s.g_pool[ i ].sol ) {
     gpit->sol = s.g_pool[ i ].sol->clone();  // clone() the Solution in
     gpit->varsol = s.g_pool[ i ].varsol;
     gpit->value = s.g_pool[ i ].value;            // eager/lazy constant
     gpit->convexified = s.g_pool[ i ].convexified;
     gpit->conv_active.clear();                    // cache: rebuilt lazily
     Addd.push_back( i );
     }
    ++gpit;
    }

   f_max_glob = s.f_max_glob;
   }
  }  // end( else( NoSol ) )

 // if there is no Observer, no-one is looking at what just happened
 if( ! f_Observer )
  return;

 // but if there is an Observer the Modification have to be issued *after*
 // the data change; first tell that all previous linearizations have been
 // removed, provided there was any
 // note that the GlobalPoolRemoved Modification is issued with
 // what == 0, i.e., nothing really has changed in the inner Block
 if( ! gpempty )
  f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
   this , C05FunctionMod::GlobalPoolRemoved , std::move( Subset() ) , 0 , 0 )
				);

 // then tell about additions (if there is anything to add), so that the
 // aggregated linearizations are substituted with the new ones
 if( ! Addd.empty() )
  f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
   this , C05FunctionMod::GlobalPoolAdded , std::move( Addd ) , 0 , 0 ) );

 }  // end( LagBFunction::put_State( const & ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::put_State( State && state )
{
 // if state is not a LagBFunctionState &&, exception will be thrown
 auto && s = dynamic_cast< LagBFunctionState && >( state );

 // ensure g_pool is large enough
 if( s.f_max_glob > g_pool.size() )
  g_pool.resize( s.f_max_glob );

 // move the important linearization information
 zLC = std::move( s.zLC );

 bool gpempty = ( f_max_glob > 0 );
 Subset Addd;

 // first void the current global pool
 if( NoSol ) {
  std::fill( g_pool.begin() , g_pool.end() , gpool_el() );
  f_max_glob = 0;
  }
 else {
  for( auto & el : g_pool ) {
   delete el.sol;
   el.sol = nullptr;
   el.varsol = true;
   el.value = 0;               // clear stale eager/lazy data on voided slots
   el.convexified = false;
   el.conv_active.clear();
   }

  // now add back all the Solution in the State (possibly after a check)
  auto gpit = g_pool.begin();

  if( ChkState )  // if Solutions are checked
   for( Index i = 0 ; i < s.g_pool.size() ; ++i ) {
    if( s.g_pool[ i ].sol ) {
     // write the Solution to the inner Block
     s.g_pool[ i ].sol->write( v_Block.front() );

     // if it's still a feasible solution/direction, copy it
     if( ( s.g_pool[ i ].varsol ? v_Block.front()->is_feasible()
	                        : v_Block.front()->is_unbounded() ) ) {
      gpit->sol = s.g_pool[ i ].sol;  // move the Solution in
      s.g_pool[ i ].sol = nullptr;      // delete it from the State
      gpit->varsol = s.g_pool[ i ].varsol;
      gpit->value = s.g_pool[ i ].value;            // eager/lazy constant
      gpit->convexified = s.g_pool[ i ].convexified;
      gpit->conv_active.clear();                    // cache: rebuilt lazily
      Addd.push_back( i );
      f_max_glob = i + 1;
      }
     }
    ++gpit;
    }
  else {        // it is trusted that Solution are correct
   for( Index i = 0 ; i < s.g_pool.size() ; ++i ) {
    if( s.g_pool[ i ].sol ) {
     gpit->sol = s.g_pool[ i ].sol;  // move the Solution in
     s.g_pool[ i ].sol = nullptr;      // delete it from the State
     gpit->varsol = s.g_pool[ i ].varsol;
     gpit->value = s.g_pool[ i ].value;            // eager/lazy constant
     gpit->convexified = s.g_pool[ i ].convexified;
     gpit->conv_active.clear();                    // cache: rebuilt lazily
     Addd.push_back( i );
     }
    ++gpit;
    }

   f_max_glob = s.f_max_glob;
   }
  }  // end( else( NoSol ) )

 // if there is no Observer, no-one is looking at what just happened
 if( ! f_Observer )
  return;

 // but if there is an Observer the Modification have to be issued *after*
 // the data change; first tell that all previous linearizations have been
 // removed, provided there was any
 // note that the GlobalPoolRemoved Modification is issued with
 // what == 0, i.e., nothing really has changed in the inner Block
 if( ! gpempty )
  f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
   this , C05FunctionMod::GlobalPoolRemoved , std::move( Subset() ) , 0 , 0 )
				);

 // then tell about additions (if there is anything to add), so that the
 // aggregated linearizations are substituted with the new ones
 if( ! Addd.empty() )
  f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
   this , C05FunctionMod::GlobalPoolAdded , std::move( Addd ) , 0 , 0 ) );

 }  // end( LagBFunction::put_State( && ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::serialize_State( netCDF::NcGroup & group ,
				    const std::string & sub_group_name ) const
{
 if( ! sub_group_name.empty() ) {
  auto gr = group.addGroup( sub_group_name );
  serialize_State( gr );
  return;
  }

 // do it "by hand" since there is no LagBFunctionState available
 // to call State::serialize() from
 group.putAtt( "type", "LagBFunctionState" );

 if( f_max_glob ) {
  netCDF::NcDim gs = group.addDim( "LagBFunction_MaxGlob" , f_max_glob );

  std::vector< int > typ( f_max_glob );
  for( Index i = 0 ; i < f_max_glob ; ++i )
   typ[ i ] = g_pool[ i ].varsol ? 1 : 0;
 
  ( group.addVar( "LagBFunction_Type" , netCDF::NcByte() , gs )
    ).putVar( { 0 } , {  f_max_glob } , typ.data() );

  for( Index i = 0 ; i < f_max_glob ; ++i ) {
   if( ! g_pool[ i ].sol )
    continue;

   auto gi = group.addGroup( "LagBFunction_Sol_" + std::to_string( i ) );
   g_pool[ i ].sol->serialize( gi );
   }
  }

 if( ! zLC.empty() ) {
  netCDF::NcDim cn = group.addDim( "LagBFunction_ImpCoeffNum" , zLC.size() );
  
  auto ncCI = group.addVar( "LagBFunction_ImpCoeffInd" , netCDF::NcInt() ,
			    cn );

  auto ncCV = group.addVar( "LagBFunction_ImpCoeffVal" , netCDF::NcDouble() ,
			    cn );
 
  for( Index i = 0 ; i < zLC.size() ; ++i ) {
   ncCI.putVar( { i } , zLC[ i ].first );
   ncCV.putVar( { i } , zLC[ i ].second );
   }
  }
 }  // end( LagBFunction::serialize_State )

/*--------------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE LagBFunction ------------*/
/*--------------------------------------------------------------------------*/

bool LagBFunction::has_linearization( bool diagonal )
{
 auto is = inner_Solver();
 if( ! is )
  throw( std::logic_error(
		 "LagBFunction::has_linearization called with no Solver" ) );
 // true if the first linearization of the related type exists
 bool newlin = diagonal ? is->has_var_solution() : is->has_var_direction();

 if( newlin ) {                  // the Solver has the desired stuff
  VarSol = diagonal;             // set the type of the Solution
  LastSolution = g_pool.size();  // signal it has to be read in the Block
  }

 return( newlin );

 }  // end( LagBFunction::has_linearization )

/*--------------------------------------------------------------------------*/

bool LagBFunction::compute_new_linearization( const bool diagonal )
{
 auto is = inner_Solver();
 if( ! is )
  throw( std::logic_error(
	 "LagBFunction::compute_new_linearization called with no Solver" ) );

 // true if another linearization of the related type exists
 bool newlin = diagonal ? is->new_var_solution() : is->new_var_direction();

 if( newlin ) {                  // the Solver has the desired stuff
  VarSol = diagonal;             // set the type of the Solution
  LastSolution = g_pool.size();  // signal it has to be read in the Block
  }

 return( newlin );

 }  // end( LagBFunction::compute_new_linearization )

/*--------------------------------------------------------------------------*/

void LagBFunction::store_linearization( Index name , ModParam issueMod )
{
 if( name >= g_pool.size() )
  throw( std::logic_error(
	 "LagBFunction::store_linearization: invalid linearization name" ) );

 // throw exception if the solution does not exist or has been already stored
 if( LastSolution< Inf< Index >() )
  throw( std::logic_error( "LagBFunction: unavailable linearization" ) );

 // get the current Solution from the Solver - - - - - - - - - - - - - - - - -

 if( NoSol )
  // put there any non-nullptr to mark the slot as taken
  g_pool[ name ].sol = reinterpret_cast< Solution * >( this );
 else {
  delete g_pool[ name ].sol;  // delete the Solution already there (if any)

  // get a "fully loaded" Solution out of the inner Block, using the default
  // f_solution_Configuration in the BlockConfig of the inner Block
  g_pool[ name ].sol = v_Block.front()->get_Solution( nullptr , false );
  if( ! g_pool[ name ].sol )
   throw( std::logic_error( "LagBFunction: no Solution provided by Block" ) );
  }

 g_pool[ name ].varsol = VarSol;  // record the Solution type
 // reset the slot in case it previously held a convexified linearization; the
 // epigraphic correction (if any) is computed right below
 g_pool[ name ].convexified = false;
 g_pool[ name ].value = 0;
 g_pool[ name ].conv_active.clear();
 // stale subgradient cache (repopulated below)
 LastSolution = name;             // record that the Solution has been stored

 // the stored x* need not be a genuine subproblem solution: when it is a
 // *convex combination* of subproblem solutions, re-evaluating the objective
 // then gives the value at the (fractional) combination point, f(conv),
 // rather than the value the dual actually attains, Sum_k mult_k f(x_k); the
 // two coincide only when the objective is affine. The latter (epigraphic)
 // value is the exact constant of the linearization and equals
 // value - < lambda , G > , where the right "value" is get_value(): the best
 // *bound* on Fi consistent with the recovered primal -- lower bound for a
 // minimisation, upper bound for a maximisation (see the twin comment in
 // get_linearization_constant()). Store the difference delta_na = epigraphic
 // - f(conv) as the (cost-independent) correction, so that later
 // re-evaluations f(conv) + value reconstruct the exact constant. For a
 // genuine extreme point delta_na is ~0. EAGER (default): instead store the
 // full epigraphic constant itself in value (= epigraphic when convexified,
 // else c·conv), so that get_linearization_constant() can return it without
 // re-reading the Solution; it is maintained on cost changes by
 // update_CostMatrix_*().
 if( ( ! NoSol ) && VarSol && inner_Solver() && ( ! std::isnan( f_yb ) ) ) {
  double fv = get_value();                          // = (lb|ub) + f_yb
  if( ( fv > - Inf< FunctionValue >() ) && ( fv < Inf< FunctionValue >() ) ) {
   double cx = get_linearization_constant( name );  // = f(conv) (value is 0)
   double lG = 0;                                   // = <lambda,G>
   for( auto & lp : LagPairs ) {
    lp.second->compute();
    lG += lp.first->get_value() * lp.second->get_value();
    }
   double epigraphic = fv - lG;
   bool cvx = ( std::abs( epigraphic - cx ) >
                1e-9 * std::max( double( 1 ) , std::abs( epigraphic ) ) );
   g_pool[ name ].convexified = cvx;
   g_pool[ name ].value = f_lazy_eval ? ( epigraphic - cx )        // delta_na
                                      : ( cvx ? epigraphic : cx ); // const
   }
  else
   if( ! f_lazy_eval )           // fv not finite: store the bare c·conv
    g_pool[ name ].value = get_linearization_constant( name );
  }
 else
  if( ( ! f_lazy_eval ) && ( ! NoSol ) )  // EAGER needs a stored constant
   g_pool[ name ].value = get_linearization_constant( name );  // = c·conv

 // EAGER (phase 2): cache x*_k on the dual-pair coords (v_active) so the
 // subgradient can later be rebuilt without writing the Solution into the
 // Block. The Block still holds x*_k here (the fresh solution).
 populate_conv_active( name );

 if( name >= f_max_glob )         // update f_max_glob
  f_max_glob = name + 1;

 // if necessary, issue the Modification - - - - - - - - - - - - - - - - - - -

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;
  
 f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                this , C05FunctionMod::GlobalPoolAdded ,
                                Subset( { name } ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( LagBFunction::store_linearization( Index ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::store_combination_of_linearizations(
	c_LinearCombination & coefficients , Index name , ModParam issueMod )
{
 if( name >= g_pool.size() )
  throw( std::logic_error( "max size of global pool already exceed" ) );

 if( coefficients.empty() )
  throw( std::invalid_argument( "the convex combination is empty" ) );

 if( name >= f_max_glob )      // update f_max_glob
  f_max_glob = name + 1;

 if( NoSol ) {  // only pretend you are doing it
  g_pool[ name ].sol = reinterpret_cast< Solution * >( this );
  g_pool[ name ].varsol = true;
  return;
  }

 #if CHECK_SOLUTIONS & 4
  std::cout << "LagBFunction " << this << ": computing "
	    << coefficients.size() << "-combination in "
	    << name << std::endl;
 #endif

 // get a scaled version of the first Solution
 auto first = coefficients[ 0 ].first;
 auto convex_combination = ( g_pool[ first ].sol
			     )->scale( coefficients[ 0 ].second );
 bool type = g_pool[ first ].varsol;  // diagonal unless already vertical

 // for all other Solutions in the pool
 for( Index i = 1 ; i < coefficients.size() ; ++i ) {
  auto pos = coefficients[ i ].first;
  if( pos == first )
   continue;
  auto mult = coefficients[ i ].second;

  #if CHECK_SOLUTIONS & 4
   std::cout << "pos = " << pos << ", mult = " << mult << ", sol = "
             << * g_pool[ pos ].sol;
  #endif

  // add the new term to the convex combination
  convex_combination->sum( g_pool[ pos ].sol , mult );

  // if the convex combination even contains a single direction
  if( ! g_pool[ pos ].varsol )
   type = false;  // then it is a direction
  }

 // BEFORE overwriting slot 'name' (it may itself be one of the constituents),
 // compute the EPIGRAPHIC value of the combination from the constituents:
 // each get_linearization_constant() returns f( x_k ) plus the constituent's
 // own correction, i.e. its epigraphic value, so this sum is sum_k lambda_k
 // f(x_k)
 double agg = 0;
 for( const auto & cf : coefficients )
  agg += cf.second * get_linearization_constant( cf.first );

 delete g_pool[ name ].sol;  // delete the current Solution (if any)

 g_pool[ name ].sol = convex_combination;  // store the Solution

 // EAGER subgradient for the combination: the coupled-coord values of the
 // combined Solution are the same linear combination of the
 // constituents' values, so conv_active combines element-wise exactly like
 // the Solution does (scale the first, then sum the rest, skipping a repeated
 // 'first' index as the Solution loop above). This gives the aggregate a
 // write-free subgradient too. Needs external mode, a current v_active and
 // every constituent to carry a shape-matching conv_active; otherwise leave
 // empty -> sol->write fallback. NB: read the constituents from the OLD pool
 // state (name itself may be one of them); g_pool[ name ].conv_active is not
 // overwritten until the assignment below, mirroring how agg and the Solution
 // sum read the old constituents.
 std::vector< Vec_FunctionValue > comb_ca;
 if( ( ! f_lazy_eval ) && ( ! f_active_dirty ) &&
     ( v_active.size() == CostMatrix.size() ) ) {
  bool ok = true;
  for( const auto & cf : coefficients ) {
   const auto & cca = g_pool[ cf.first ].conv_active;
   if( cca.size() != v_active.size() ) { ok = false; break; }
   for( Index h = 0 ; ok && ( h < cca.size() ) ; ++h )
    if( cca[ h ].size() != v_active[ h ].size() ) { ok = false; break; }
   if( ! ok ) break;
   }
  if( ok ) {
   comb_ca.assign( v_active.size() , Vec_FunctionValue() );
   const auto & c0 = g_pool[ first ].conv_active;
   const double m0 = coefficients[ 0 ].second;
   for( Index h = 0 ; h < v_active.size() ; ++h ) {
    comb_ca[ h ].resize( v_active[ h ].size() );
    for( Index t = 0 ; t < comb_ca[ h ].size() ; ++t )
     comb_ca[ h ][ t ] = m0 * c0[ h ][ t ];
    }
   for( Index i = 1 ; i < coefficients.size() ; ++i ) {
    const auto pos = coefficients[ i ].first;
    if( pos == first )
     continue;
    const double mult = coefficients[ i ].second;
    const auto & cc = g_pool[ pos ].conv_active;
    for( Index h = 0 ; h < v_active.size() ; ++h )
     for( Index t = 0 ; t < comb_ca[ h ].size() ; ++t )
      comb_ca[ h ][ t ] += mult * cc[ h ][ t ];
    }
   }
  }

 g_pool[ name ].varsol = type;             // store the type
 g_pool[ name ].conv_active = std::move( comb_ca );  // empty if not combinable

 if( name == LastSolution )    // if this was the Solution in the inner Block
  LastSolution = g_pool.size();  // it is no longer valid

 // EAGER (default): store the full epigraphic constant agg directly; under
 // eager get_linearization_constant( cf.first ) above already returned each
 // constituent's full constant, so agg is the exact combination constant and
 // is returned as-is (no f(conv) re-read). LAZY: store the correction
 // delta_na = agg - f(conv); with value == 0 / convexified == false, the call
 // returns the bare f(conv), so afterwards f(conv) + value == agg.
 if( ! f_lazy_eval ) {
  g_pool[ name ].value = agg;
  g_pool[ name ].convexified = true;
  }
 else {
  g_pool[ name ].convexified = false;
  g_pool[ name ].value = 0;
  g_pool[ name ].value = agg - get_linearization_constant( name );
  g_pool[ name ].convexified = true;
  }

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                this , C05FunctionMod::GlobalPoolAdded ,
                                Subset( { name } ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( LagBFunction::store_convex_combination_of_linearizations )

/*--------------------------------------------------------------------------*/

void LagBFunction::delete_linearization( Index name , ModParam issueMod )
{
 if( ( name >= g_pool.size() ) || ( ! g_pool[ name ].sol ) )
  throw( std::invalid_argument(
	 "LagBFunction::delete_linearization: invalid linearization name" ) );

 if( ! NoSol )                    // if the Solution is there
  delete g_pool[ name ].sol;    // delete it
 g_pool[ name ].sol = nullptr;  // mark that the position is empty

 if( name == LastSolution )    // if this was the Solution in the inner Block
  LastSolution = g_pool.size();  // it is no longer valid

 update_f_max_glob();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;
  
 f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                this , C05FunctionMod::GlobalPoolRemoved ,
                                Subset( { name } ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( LagBFunction::delete_linearization )

/*--------------------------------------------------------------------------*/

void LagBFunction::delete_linearizations( Subset && which , bool ordered ,
					  ModParam issueMod )
{
 if( which.empty() ) {  // delete them all
  if( NoSol )
   for( Index i = 0 ; i < f_max_glob ; ++i )
    g_pool[ i ].sol = nullptr;
  else
   for( Index i = 0 ; i < f_max_glob ; ++i )
    if( g_pool[ i ].sol ) {
     delete g_pool[ i ].sol;
     g_pool[ i ].sol = nullptr;
     }

  f_max_glob = 0;
  if( LastSolution< Inf< Index >() )  // LastSolution was in the global pool
   LastSolution = g_pool.size();     // it is no longer valid

  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                  this , C05FunctionMod::GlobalPoolRemoved ,
                                  std::move( which ) , 0 ,
                                  Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );
  return;  // all done
  }

 // here, which is not empty, so we have to delete the given subset
 if( ! ordered )
  std::sort( which.begin() , which.end() );

 if( which.back() >= g_pool.size() )
  throw( std::invalid_argument(
       "LagBFunction::delete_linearizations: invalid linearization name" ) );

 for( auto i : which ) {
  if( ! g_pool[ i ].sol )
   throw( std::invalid_argument(
       "LagBFunction::delete_linearizations: invalid linearization name" ) );

  if( i == LastSolution )    // if this was the Solution in the inner Block
   LastSolution = g_pool.size();  // it is no longer valid

  if( ! NoSol )               // if a Solution really is there
   delete g_pool[ i ].sol;  // delete it
  g_pool[ i ].sol = nullptr;
  }

 update_f_max_glob();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;
  
 f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                this , C05FunctionMod::GlobalPoolRemoved ,
                                std::move( which ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( LagBFunction::delete_linearizations )

/*--------------------------------------------------------------------------*/

int LagBFunction::compute( bool changedvars )
{
 auto is = inner_Solver();
 if( ! is )          // there is no inner Solver
  return( kError );  // that's clearly an error

 // if required, check if b == 0 or not- - - - - - - - - - - - - - - - - - - -
 if( f_yb == INF ) {
  f_yb = -INF;  // b == 0 until otherwise proven
  for( auto const & lp : LagPairs )
   if( static_cast< p_LF >( lp.second )->get_constant_term() ) {
    f_yb = NaN; break;  // if b has nonzeros, yb need be recomputed
    }
  }

 // check what needs be updated- - - - - - - - - - - - - - - - - - - - - - - -
 if( changedvars && ( ! ( LagPairs.empty() && ( ! f_c_changed ) ) ) ) {
  // if the Lagrangian variables have changed, then Lagrangian costs
  // c^y = c + yA need to be recomputed; however, this is unless there are
  // actually no Lagrangian variables and the costs are still the original
  // ones, because then c^y = c
  f_dirty_Lc = true;
  if( ( ! std::isnan( f_yb ) ) && ( f_yb > -INF ) )
                      // unless b is known to be all-0
   f_yb = NaN;        // force to recompute the linear term yb
  }

 // see if entries of the inner obj have to be "stealthily" added- - - - - - -
 // this requires locking the inner Block, which we do only once
 bool tounlock = false;

 // if there are pending additions to the inner objective, flush them now
 bool had_pending = false;
 for( const auto & bucket : v_tmpCP )
  if( ! bucket.empty() ) { had_pending = true; break; }

 if( had_pending )
  tounlock = flush_v_tmpCP();

 // flushing new variables into the objective changes costs, so mark dirty
 if( had_pending )
  f_dirty_Lc = true;

 // ensure CostMatrix has at least as many columns as the current #active var
 // in each objective (do not shrink here; we will use min(cm,nv) below)
 for( Index h = 0 ; h < CostMatrix.size() ; ++h ) {
  auto * fn = v_Obj[ h ]->get_function();
  Index nv = ! v_ObjIsQuad[ h ]
              ? static_cast< p_LF >( fn )->get_num_active_var()
              : static_cast< p_QF >( fn )->get_num_active_var();

  if( CostMatrix[ h ].size() < nv ) {
   CostMatrix[ h ].reserve( nv );
   while( CostMatrix[ h ].size() < nv )
    CostMatrix[ h ].emplace_back();  // default column (0.0, {})
  }
 }

 // if necessary, recompute the Lagrangian costs c^y = c + yA- - - - - - - - -
 if( f_dirty_Lc ) {
  // temp array of y values, more cache friendly
  Vec_FunctionValue y( LagPairs.size() );
  for( Index i = 0 ; i < LagPairs.size() ; ++i )
   y[ i ] = LagPairs[ i ].first->get_value();

  // (re)build v_active if the dual-pair / variable structure changed: per
  // objective, the sorted positions j coupled to a multiplier (non-empty
  // CostMatrix[h][j].second). Cached; rebuilt only on f_active_dirty, so the
  // per-compute loop below iterates O(|coupled|) and not O(#vars).
  if( f_active_dirty ) {
   v_active.assign( CostMatrix.size() , Subset() );
   for( Index h = 0 ; h < CostMatrix.size() ; ++h ) {
    const auto & cm = CostMatrix[ h ];
    for( Index i = 0 ; i < cm.size() ; ++i )
     if( ! cm[ i ].second.empty() )
      v_active[ h ].push_back( i );
    }
   f_active_dirty = false;
   // v_active changed shape: any conv_active stored against the old structure
   // is now misaligned. Drop them so get_linearization_coefficients() falls
   // back to sol->write() until those entries are re-stored against the new
   // v_active. Structural changes are rare, so the scan is cheap amortised.
   for( auto & el : g_pool )
    el.conv_active.clear();
   }

  // loop over all Blocks in BFS order
  for( Index h = 0 ; h < CostMatrix.size() ; ++h ) {
   const auto & cm = CostMatrix[ h ];

   // compute how many coefficients we can safely write (objective may have
   // grown)
   auto * fn = v_Obj[ h ]->get_function();
   Index nv = ! v_ObjIsQuad[ h ]
               ? static_cast< p_LF >( fn )->get_num_active_var()
               : static_cast< p_QF >( fn )->get_num_active_var();
   const Index m = std::min< Index >( nv , static_cast< Index >( cm.size() ) );

   // sparse Lagrangian-cost update
   // recompute c^y = c + yA only for coords coupled to a multiplier
   // (non-empty A_j; the rest keep c_j, already loaded), and among those
   // write only the ones that actually changed since the last write. The
   // current (last-written) value is read straight from the objective via
   // get_v_var() -- no mirror, no extra memory. COSTTOL filters numerical
   // noise; comparing against the loaded value (not the last computed) lets
   // sub-tolerance changes accumulate and so bounds the drift.
   const double tol = f_cost_tol;     // dblCostTol

   // if the Block has not been locked yet and it is not owned
   bool block_locked = false;
   Block * blk = v_BlockBFS[ h ];
   if( ! blk->is_owned_by( f_id ) ) {
    if( ! blk->lock( f_id ) )  // try to lock it; failure
     return( kError );         // clearly is an error
    block_locked = true;       // it'll have to be unlocked
    }

   // collect the coords whose c^y actually changes (reading the current
   // loaded coefficient under the lock)
   Subset chgidx;
   Vec_FunctionValue chgval;
   for( Index i : v_active[ h ] ) {  // only coords coupled to a multiplier
    if( i >= m )                     // beyond current objective (shrunk): skip
     continue;
    double newval = cm[ i ].first;
    for( const auto & el : cm[ i ].second )
     newval += y[ el.first ] * el.second;
    double oldval = ! v_ObjIsQuad[ h ]
       ? static_cast< p_LF >( fn )->get_v_var()[ i ].second
       : std::get< 1 >( static_cast< p_QF >( fn )->get_v_var()[ i ] );
    if( std::abs( newval - oldval ) >
        tol * std::max( double( 1 ) , std::abs( newval ) ) ) {
     chgidx.push_back( i );
     chgval.push_back( newval );
     }
    }

   if( ! chgidx.empty() ) {
    f_play_dumb = true;         // ignore any ensuing Modification

    // The Modification is issued on the standard channel, and therefore with
    // concerns_Block() == true: the inner Block must see it as any other
    // Objective change, so that it can fold it into its physical
    // representation and re-issue it in its own physical language to the
    // Solver registered on it (e.g. ThermalUnitBlock translating the new
    // Lagrangian costs for its DP solvers, which never look at the abstract
    // representation). An *enclosing* LagBFunction that has adopted this
    // same (shared) sub-Block Objective must still not mistake the write
    // for a real cost change [it would issue a spurious AlphaChanged that
    // keeps invalidating its bundle model -> kLowPrecision]: it recognises
    // it structurally, because the writer (this LagBFunction) holds the
    // Block lock, see the guard in guts_of_guts_of_add_Modification().
    //
    // chgidx is sorted with distinct entries (it is a subset of the sorted
    // v_active[h] pushed in order), so it represents a contiguous run iff
    // back - front + 1 == size -- an O(1) test. In that (very common) case
    // the changed set IS a Range: issue the Range overload, which avoids
    // building / carrying the O(k) index vector in the Modification (this is
    // also the original pre-sparse write path). Otherwise fall back to the
    // Subset form.
    const bool is_range =
     ( chgidx.back() - chgidx.front() + 1 == Index( chgidx.size() ) );
    if( ! v_ObjIsQuad[ h ] ) {
     auto * lf = static_cast< p_LF >( fn );
     if( is_range )
      lf->modify_coefficients( std::move( chgval ) ,
			       Range( chgidx.front() , chgidx.back() + 1 ) );
     else
      lf->modify_coefficients( std::move( chgval ) , std::move( chgidx ) );
     }
    else {
     auto * qf = static_cast< p_QF >( fn );
     if( is_range )
      qf->modify_linear_coefficients( std::move( chgval ) ,
				      Range( chgidx.front() ,
					     chgidx.back() + 1 ) );
     else
      qf->modify_linear_coefficients( std::move( chgval ) ,
				      std::move( chgidx ) );
     }

    f_play_dumb = false;        // back to normal operations
    }

   // if the Block had to be locked, for whatever reason
   if( block_locked )
    blk->unlock( f_id );      // unlock it
   }

  f_dirty_Lc = false;           // Lagrangian costs are current
  f_c_changed = true;           // ... and hence no longer original
  }

 // if the inner Block had to be locked, for whatever reason
 if( tounlock )
  v_Block.front()->unlock( f_id );  // unlock it

 // if necessary, recompute the linear term- - - - - - - - - - - - - - - - - -
 if( std::isnan( f_yb ) ) {
  f_yb = 0;
  for( const auto & lp : LagPairs )
   f_yb += static_cast< p_LF >( lp.second )->get_constant_term() *
           lp.first->get_value();
  }

 // if some parameters have been changed, set BlockSolverConfig- - - - - - - -
 if( f_CC_changed ) {
  is->set_ComputeConfig( f_CC );  // push the changes
  f_CC->clear();                  // clear the ComputeConfig
  f_CC->set_diff( true );         // but keep it in "diff mode"
  f_CC_changed = false;           // no changes so far
  }

 // if the solution in the Block was the one out of the last call to
 // compute(), signal that it is no longer correct; this is done by
 // has_linearization(), but it is possible that get_linearization_*() is
 // called (on "old" linearizations) without calling it
 if( LastSolution == Inf< Index >() )
  LastSolution = g_pool.size();

 // finally, compute() the inner Block - - - - - - - - - - - - - - - - - - - -
 // it is assumed that the inner Block (B) does not have Variable defined in
 // other Blocks: then, the re-optimization of (B) can be performed starting
 // from the old solution, i.e., compute( false ) can be called; this means
 // that in fact the Solver may not have to do anything because the inner
 // Block may not have changed (say, only b has), but this is left to the
 // Solver to properly check to avoid doing useless work

 // return the status of the Solver as the status of the LagBFunction
 return( is->compute( false ) );

 }  // end( LagBFunction::compute )

/*--------------------------------------------------------------------------*/

static RealObjective::OFValue get_recours_obj( const Block * blck )
{
 RealObjective::OFValue rv = 0;
 if( auto obj = dynamic_cast< RealObjective * >( blck->get_objective() ) )
  rv = obj->get_constant_term();
 for( const auto bk : blck->get_nested_Blocks() )
  rv += get_recours_obj( bk );
 return( rv );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

Function::FunctionValue LagBFunction::get_constant_term( void ) const
{
 if( auto bk = get_inner_block() )
  return( get_recours_obj( bk ) );
 return( 0 );
 }

/*--------------------------------------------------------------------------*/

#if CHECK_SOLUTIONS & 2

static double cptobj( Block * blck )
{
 double ov = 0;
 if( auto obj = dynamic_cast< FRealObjective * >( blck->get_objective() ) ) {
  obj->compute();
  ov = obj->value();
  }
 for( auto bk : blck->get_nested_Blocks() )
  ov += cptobj( bk );
 return( ov );
 }

#endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// is the cached conv_active of g_pool[ name ] usable to rebuild the
// subgradient without writing the Solution? It is when the external (eager)
// mode is on and conv_active was stored against the *current* v_active
// structure (same shape). Structural changes invalidate it (compute() clears
// conv_active when it rebuilds v_active, so a shape mismatch here only guards
// against an interleaving).

void LagBFunction::populate_conv_active( Index name )
{
 // read x*_name at the dual-pair coords (v_active) from the CURRENT
 // inner-Block state (caller guarantees the Block holds x*_name) into
 // conv_active. No-op, leaving it EMPTY (=> sol->write fallback), under lazy
 // / NoSol, or while v_active is not current (f_active_dirty), or on a stale
 // out-of-range pos.

 auto & ca = g_pool[ name ].conv_active;
 if( f_lazy_eval || NoSol || f_active_dirty ||
     ( v_active.size() != CostMatrix.size() ) ) {
  ca.clear();
  return;
  }
 ca.assign( v_active.size() , Vec_FunctionValue() );
 bool ok = true;
 for( Index h = 0 ; ok && ( h < v_active.size() ) ; ++h ) {
  auto * fn = v_Obj[ h ]->get_function();
  ca[ h ].reserve( v_active[ h ].size() );
  if( ! v_ObjIsQuad[ h ] ) {
   const auto & rp = static_cast< p_LF >( fn )->get_v_var();
   for( Index j : v_active[ h ] ) {
    if( j >= rp.size() ) { ok = false; break; }   // v_active stale vs obj
    ca[ h ].push_back( rp[ j ].first->get_value() );
    }
   }
  else {
   const auto & rp = static_cast< p_QF >( fn )->get_v_var();
   for( Index j : v_active[ h ] ) {
    if( j >= rp.size() ) { ok = false; break; }
    ca[ h ].push_back( std::get< 0 >( rp[ j ] )->get_value() );
    }
   }
  }
 if( ! ok )                       // give up -> empty -> sol->write fallback
  ca.clear();
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

bool LagBFunction::conv_active_usable( Index name ) const
{
 if( f_lazy_eval )
  return( false );
 // v_active is rebuilt lazily in compute() when f_active_dirty; between a
 // structural Modification (variable/dual-pair add/remove, which updates
 // CostMatrix and sets f_active_dirty) and that rebuild, v_active and the
 // stored conv_active still hold the OLD positions while CostMatrix is
 // already the new one. Their sizes may even still match, so the shape check
 // below is not enough: using conv_active here would index the new CostMatrix
 // with stale positions and produce a wrong subgradient. Fall back to
 // sol->write until the rebuild (which also clears conv_active) has run and
 // the entries are re-stored.
 if( f_active_dirty )
  return( false );
 const auto & CA = g_pool[ name ].conv_active;
 if( ( CA.size() != v_active.size() ) || ( CA.size() != CostMatrix.size() ) )
  return( false );
 for( Index h = 0 ; h < CA.size() ; ++h )
  if( CA[ h ].size() != v_active[ h ].size() )
   return( false );
 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

bool LagBFunction::coeff_from_conv_active( FunctionValue * g , Range range ,
					   Index name )
{
 if( ! conv_active_usable( name ) )
  return( false );
 const auto & CA = g_pool[ name ].conv_active;

 // g_i = const_i + sum_{j coupled} a_{ij} x*_j: const_i is the constant term
 // of the relaxed-constraint LinearFunction (what get_value() adds), the
 // a_{ij} come from CostMatrix (the transpose of the dual pairs) and x*_j
 // from conv_active. This reproduces exactly LagPairs[ i
 // ].second->get_value() at x*.
 for( Index i = range.first ; i < range.second ; ++i )
  g[ i - range.first ] =
   static_cast< p_LF >( LagPairs[ i ].second )->get_constant_term();

 for( Index h = 0 ; h < CA.size() ; ++h ) {
  const auto & cm = CostMatrix[ h ];
  const auto & va = v_active[ h ];
  const auto & ca = CA[ h ];
  for( Index idx = 0 ; idx < va.size() ; ++idx ) {
   const FunctionValue xv = ca[ idx ];
   if( xv == 0 )
    continue;
   for( const auto & mon : cm[ va[ idx ] ].second ) {
    const Index i = mon.first;
    if( ( i >= range.first ) && ( i < range.second ) )
     g[ i - range.first ] += mon.second * xv;
    }
   }
  }
 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

bool LagBFunction::coeff_from_conv_active( FunctionValue * g ,
					   c_Subset & subset , Index name )
{
 if( ! conv_active_usable( name ) )
  return( false );
 const auto & CA = g_pool[ name ].conv_active;

 // map each requested multiplier to its output position, seed with const_i
 std::unordered_map< Index , Index > pos;
 pos.reserve( subset.size() );
 for( Index k = 0 ; k < subset.size() ; ++k ) {
  if( subset[ k ] >= get_num_active_var() )
   throw( std::invalid_argument( "wrong index in subset" ) );
  pos[ subset[ k ] ] = k;
  g[ k ] = static_cast< p_LF >( LagPairs[ subset[ k ] ].second
				)->get_constant_term();
  }

 for( Index h = 0 ; h < CA.size() ; ++h ) {
  const auto & cm = CostMatrix[ h ];
  const auto & va = v_active[ h ];
  const auto & ca = CA[ h ];
  for( Index idx = 0 ; idx < va.size() ; ++idx ) {
   const FunctionValue xv = ca[ idx ];
   if( xv == 0 )
    continue;
   for( const auto & mon : cm[ va[ idx ] ].second ) {
    auto it = pos.find( mon.first );
    if( it != pos.end() )
     g[ it->second ] += mon.second * xv;
    }
   }
  }
 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void LagBFunction::get_linearization_coefficients( FunctionValue * g ,
						   Range range , Index name )
{
 range.second = std::min( range.second , get_num_active_var() );
 if( range.second <= range.first )
  return;

 // the solution shall be written in the Variable of the Block - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( name == Inf< Index >() ) {  // the last computed linearization- - - - - -

  // get solution/direction from the solver
  if( LastSolution != Inf< Index >() ) {  // ... if necessary
   auto is = inner_Solver();
   if( ! is )
    throw( std::logic_error(
    "LagBFunction:::get_linearization_coefficients called with no Solver" ) );
   if( VarSol ) {
    is->get_var_solution();
    #if CHECK_SOLUTIONS
     auto blck = v_Block.front();
     #if CHECK_SOLUTIONS & 1
      SimpleConfiguration< double > sc( 1e-6 );
      if( ! blck->is_feasible( true , & sc ) )
       std::cout << "Error: solution infeasible " << std::endl;
     #endif
     #if CHECK_SOLUTIONS & 2
      auto ov = cptobj( blck );
      auto iv = is->get_var_value();
      if( std::abs( ov - iv ) >
	  1e-6 * std::max( double( 1 ) , std::abs( iv ) ) )
       std::cout << "Error: objval = " << ov << " != isval = " << iv
		 << std::endl;
     #endif
    #endif
    }
   else
    is->get_var_direction();

   LastSolution = Inf< Index >();
   }
  }
 else {  // a linearization of the global pool - - - - - - - - - - - - - - - -
  if( NoSol )
   throw( std::logic_error( "LagBFunction: Solutions are not stored" ) );
   
  // assign Solution to the sub-Block in such a way the linearization
  // associated with the given name will be retrieved from the global pool

  if( ! g_pool[ name ].sol )
   throw( std::logic_error(
   "LagBFunction::get_linearization_coefficients: invalid linearization name"
			   ) );

  // EAGER (external subgradient): rebuild g from the stored conv_active
  // without writing the Solution into the inner Block (see
  // sparse_costs_design.md §4.4); fall back to the write below if conv_active
  // is unavailable/inconsistent.
  if( coeff_from_conv_active( g , range , name ) )
   return;

  if( LastSolution != name ) {
   g_pool[ name ].sol->write( v_Block.front() );
   LastSolution = name;
   }

  // populate-on-miss: the Block now holds x*_name; cache it so subsequent
  // subgradient queries for this entry skip the write. This self-heals the
  // conv_active cache after a structural change dropped it (one write per
  // entry instead of one per query). No-op under lazy / while v_active not
  // current.
  populate_conv_active( name );
  }  // end else - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // for each Lagrangian multiplier y_i, the objective value of the relaxed
 // constraint (RCs)_i is the corresponding entry of the linearization
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( Index i = range.first ; i < range.second ; ++i ) {
  LagPairs[ i ].second->compute();
  *( g++ ) = LagPairs[ i ].second->get_value();
  }
 }  // end( LagBFunction::get_linearization_coefficients( * , range ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::get_linearization_coefficients( FunctionValue * g ,
			      c_Subset & subset , bool ordered , Index name )
{
 // the solution shall be written in the Variable of the Block - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( name == Inf< Index >() ) {  // the last computed linearization- - - - - -
  // get solution/direction from the solver
  if( LastSolution != Inf< Index >() ) {  // ... if necessary
   auto is = inner_Solver();
   if( ! is )
    throw( std::logic_error(
    "LagBFunction:::get_linearization_coefficients called with no Solver" ) );
   if( VarSol ) {
    is->get_var_solution();
    #if CHECK_SOLUTIONS
     auto blck = v_Block.front();
     #if CHECK_SOLUTIONS & 1
      SimpleConfiguration< double > sc( 1e-6 );
      if( ! blck->is_feasible( true , & sc ) )
       std::cout << "Error: solution infeasible " << std::endl;
     #endif
     #if CHECK_SOLUTIONS & 2
      auto ov = cptobj( blck );
      auto iv = is->get_var_value();
      if( std::abs( ov - iv ) >
	  1e-6 * std::max( double( 1 ) , std::abs( iv ) ) )
       std::cout << "Error: objval = " << ov << " != isval = " << iv
		 << std::endl;
     #endif
    #endif
    }
   else
    is->get_var_direction();

   LastSolution = Inf< Index >();
   }
  }
 else {  // a linearization of the global pool - - - - - - - - - - - - - - - -
  if( NoSol )
   throw( std::logic_error( "LagBFunction: Solutions are not stored" ) );

  // assign Solution to the sub-Block in such a way the linearization
  // associated with the given name will be retrieved from the global pool

  if( ! g_pool[ name ].sol )
   throw( std::logic_error(
   "LagBFunction::get_linearization_coefficients: invalid linearization name"
			   ) );

  // EAGER (external subgradient): rebuild g from the stored conv_active
  // without writing the Solution into the inner Block; fall back to the
  // write below if conv_active is unavailable/inconsistent.
  if( coeff_from_conv_active( g , subset , name ) )
   return;

  if( LastSolution != name ) {
   g_pool[ name ].sol->write( v_Block.front() );
   LastSolution = name;
   }

  // populate-on-miss (#4); see the Range overload above.
  populate_conv_active( name );
  }  // end else - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // for each Lagrangian multiplier y_i, the objective value of the relaxed
 // constraint (RCs)_i is the corresponding entry of the linearization - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( const auto i : subset ) {
  if( i >= get_num_active_var() )
   throw( std::invalid_argument( "wrong index in subset" ) );
  LagPairs[ i ].second->compute();
  *( g++ ) = LagPairs[ i ].second->get_value();
  }
 }  // end( LagBFunction::get_linearization_coefficients( * , subset ) )

/*--------------------------------------------------------------------------*/

Function::FunctionValue LagBFunction::get_linearization_constant( Index name )
{
 if( f_play_dumb )
  return 0;

 if( name == Inf< Index >() ) {  // the last computed linearization- - - - - -

  // get solution/direction from the solver
  if( LastSolution != Inf< Index >() ) {  // ... if necessary
   auto is = inner_Solver();
   if( ! is )
    throw( std::logic_error(
      "LagBFunction:::get_linearization_constant called with no Solver" ) );
   if( VarSol ) {
    is->get_var_solution();
    #if CHECK_SOLUTIONS
     auto blck = v_Block.front();
     #if CHECK_SOLUTIONS & 1
      SimpleConfiguration< double > sc( 1e-6 );
      if( ! blck->is_feasible( true , & sc ) )
       std::cout << "Error: solution infeasible " << std::endl;
     #endif
     #if CHECK_SOLUTIONS & 2
      auto ov = cptobj( blck );
      auto iv = is->get_var_value();
      if( std::abs( ov - iv ) >
	  1e-6 * std::max( double( 1 ) , std::abs( iv ) ) )
       std::cout << "Error: objval = " << ov << " != isval = " << iv
		 << std::endl;
     #endif
    #endif
    }
   else
    is->get_var_direction();

   LastSolution = Inf< Index >();
   }
  }
 else {  // a linearization of the global pool - - - - - - - - - - - - - - - -
  if( NoSol )
   throw( std::logic_error( "LagBFunction: Solutions are not stored" ) );

  if( ! g_pool[ name ].sol )  // if no such linearization
   return( NaN );               // return NaN

  // EAGER (intPoolExtMem == 0, default): the full epigraphic constant is kept
  // up-to-date in g_pool[ name ].value (set at store time, maintained on cost
  // changes). Return it directly WITHOUT writing the stored Solution into the
  // inner Block: this query is hot (the enclosing Solver asks for the
  // constant of every linearization at every iteration), so avoiding the
  // write is also a performance win. The store-time call has name ==
  // LastSolution and falls through to recompute c·conv as the basis for the
  // stored value.
  if( ( ! f_lazy_eval ) && ( name != LastSolution ) )
   return( g_pool[ name ].value );

  // assign Solution to the sub-Block in such a way the linearization
  // associated with the given name will be recovered from the global pool
  if( name != LastSolution ) {
   g_pool[ name ].sol->write( v_Block.front() );
   LastSolution = name;
   }
  }  // end else - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // return the c x^* for the chosen solution x^*, where c are the *original*
 // costs. this corresponds to the value of the Lagrangian function
 // c x^* + y ( A x^* + b ) = c x^* + y g( x^* ) in y = 0 (in fact, the
 // linear term b is not involved)
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // compute the value of the original Objective function (excluding any
 // Lagrangian terms) this includes both the Objective of the root Block and
 // those of all sub-Blocks recursively
 OFValue alpha = 0;

 // if PushCostToOwner == 0, only the root Block's Objective is modified,
 // so we can safely rely on get_objective_value() to collect the *original*
 // values of all sub-Block Objectives recursively
 if( ! PushCostToOwner )
  alpha += get_objective_value( get_inner_block() );

 // for the root Block (and for all other Objectives when PushCostToOwner ==
 // 1), we need to compute the value "by hand" using the original coefficients
 // stored in CostMatrix, since their internal Objective may be modified
 for( Index h = 0 ; h < v_Obj.size() ; ++h ) {
  const auto * obj = v_Obj[ h ];
  const auto & cm = CostMatrix[ h ];

  if( ! v_ObjIsQuad[ h ] ) {
   alpha += obj->get_constant_term();
   const auto & rp = static_cast< p_LF >( obj->get_function() )->get_v_var();
   const auto & tp = v_tmpCP[ h ];

   #ifndef NDEBUG
   if( cm.size() < ( rp.size() + tp.size() ) )
    throw( std::logic_error( "CostMatrix inconsistent with linear objective"
			     ) );
   #endif

   for( Index i = 0 ; i < rp.size() ; ++i )
    alpha += rp[ i ].first->get_value() * cm[ i ].first;

   for( Index i = 0 ; i < tp.size() ; ++i )
    alpha += tp[ i ].first->get_value() * cm[ rp.size() + i ].first;
   }
  else {
   alpha += obj->get_constant_term();
   const auto & rp = static_cast< p_QF >( obj->get_function() )->get_v_var();
   const auto & tp = v_tmpCP[ h ];

   #ifndef NDEBUG
   if( cm.size() < ( rp.size() + tp.size() ) )
    throw( std::logic_error(
		      "CostMatrix inconsistent with quadratic objective" ) );
   #endif

   for( Index i = 0 ; i < rp.size() ; ++i ) {
    auto val = std::get< 0 >( rp[ i ] )->get_value();
    if( val ) {
     alpha += cm[ i ].first * val;
     val *= val;
     alpha += std::get< 2 >( rp[ i ] ) * val;
    }
   }

   for( Index i = 0 ; i < tp.size() ; ++i ) {
    auto val = tp[ i ].first->get_value();
    if( val )
     alpha += cm[ rp.size() + i ].first * val;
    }
   }
  }

 // epigraphic correction for a stored linearization. Under LAZY, value is the
 // cost-independent correction delta_na (0 for an original linearization,
 // epigraphic - f(conv) for a convexified one), ADDED to the re-evaluated
 // f(conv) so that f(conv) + value is the exact epigraphic constant (see
 // store_linearization()).
 if( ( name < Inf< Index >() ) && f_lazy_eval )
  // LAZY: value holds the cost-independent correction delta_na, to be ADDED
  // to the re-evaluated f(conv). (EAGER stores the FULL constant in value and
  // returns it via the early-return above; the only EAGER fall-through here
  // is the store-time call name == LastSolution, where alpha = f(conv)
  // recomputed above is exactly the basis store_linearization() wants --
  // adding value, the full constant, would double-count it.)
  alpha += g_pool[ name ].value;
 else
  if( name == Inf< Index >() ) {
   // the last computed linearization (name == Inf) has no global-pool entry,
   // so its correction (if any) is computed here. When the inner Solver is
   // itself a Lagrangian dual, x* is a convex combination of subproblem
   // solutions, and the alpha = f( conv ) computed above is the value at the
   // (fractional) combination point rather than the epigraphic value Sum_k
   // mult_k f( x_k ) the dual attains; the two coincide only when the
   // objective is affine. The exact constant equals value - <lambda,G> , where
   // the right "value" to use is get_value(): the best *bound* on Fi
   // consistent with the recovered (possibly convexified) primal -- the lower
   // bound for a minimisation, the upper bound for a maximisation (this is
   // exactly what get_value() returns via is_convex()). Using instead the
   // value attained at the incumbent would be on the wrong side of the inner
   // gap and produce an over-estimated constant, i.e. a linearization above Fi
   // (negative error). Correct alpha by the gap when it is significant (for a
   // genuine extreme point the inner Solver is exact, get_value() == c x*, and
   // the stable c x* computed above is kept).
   if( VarSol && inner_Solver() && ( ! std::isnan( f_yb ) ) ) {
    double fv = get_value();                          // = (lb|ub) + f_yb
    if( ( fv > - Inf< FunctionValue >() ) && ( fv < Inf< FunctionValue >() )
	) {
     double lG = 0;                                   // = <lambda,G>
     for( auto & lp : LagPairs ) {
      lp.second->compute();
      lG += lp.first->get_value() * lp.second->get_value();
      }
     double epigraphic = fv - lG;
     if( std::abs( epigraphic - alpha ) >
	 1e-9 * std::max( double( 1 ) , std::abs( epigraphic ) ) )
      alpha = epigraphic;
     }
    }
   }

 return( alpha );

 }  // end( LagBFunction::get_linearization_constant )

/*--------------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/

ComputeConfig * LagBFunction::get_ComputeConfig( bool all ,
					         ComputeConfig * ocfg ) const
{
 // get the "standard" part of the ComputeConfig - - - - - - - - - - - - - - -

 ComputeConfig * ccfg = C05Function::get_ComputeConfig( all , ocfg );

 if( ccfg && ccfg->f_extra_Configuration ) {
  // if an extra configuration is there (must have been in ocfg) - - - - - - -
  auto cc = dynamic_cast< SimpleConfig_p_p * >( ccfg->f_extra_Configuration );
  if( ! cc )
   throw( std::invalid_argument( "ocfg extra_Configuration is not a"
				 "SimpleConfiguration< pair< Cfg * > >" ) );

  auto bsc = dynamic_cast< BlockSolverConfig * >( cc->f_value.first );
  if( ! bsc )
   throw( std::invalid_argument(
	     "ocfg extra_Configuration first is not a BlockSolverConfig" ) );
  bsc->get( v_Block.front() , ! all );

  auto bc = dynamic_cast< BlockConfig * >( cc->f_value.second );
  if( ! bc )
   throw( std::invalid_argument(
	          "ocfg extra_Configuration second is not a BlockConfig" ) );
  bc->get( v_Block.front() );
  }
 else {  // else (the extra Configuration has to be constructed) - - - - - - -
  auto bsc = RBlockSolverConfig::get_right_BlockSolverConfig(
						   v_Block.front() , ! all );

  auto bc = OCRBlockConfig::get_right_BlockConfig( v_Block.front() );

  if( bsc || bc ) {
   auto cc = new SimpleConfig_p_p;

   cc->f_value.first = bsc;
   cc->f_value.second = bc;
   if( ! ccfg )
    ccfg = new ComputeConfig;
   ccfg->f_extra_Configuration = cc;
   }

  if( ccfg && ccfg->empty() ) {
   delete ccfg;
   ccfg = nullptr;
   }
  }

 return( ccfg );

 }  // end( LagBFunction::get_ComputeConfig )

/*--------------------------------------------------------------------------*/

int LagBFunction::get_A_nz( void )
{
 Index count = 0;
 for( const auto & CM : CostMatrix )
  for( const auto & col : CM )
   count += col.second.size();

 return( count );
}

/*--------------------------------------------------------------------------*/

void LagBFunction::get_MatDesc( int * Abeg , int * Aind , double * Aval ,
                                 int strt , int stp )
{
 // important note: the order of the variables in CostMatrix is *not* the
 // original order of the columns, which is how A here need be provided,
 // but rather the order of which they are in obj; not all of the variable
 // may be in obj, which means they do not appear in A anywhere (for
 // otherwise they would have been forcibly added to obj), which means
 // that the corresponding column in A is empty

 Index count = 0;
 Index j = 0;

 for( const auto & CM : CostMatrix )
  for( const auto & col : CM ) {
   Abeg[ j ] = count;

   for( const auto & CMjs : col.second )
    if( ( CMjs.first >= Index( strt ) ) && ( CMjs.first < Index( stp ) ) ) {
     Aind[ count ] = CMjs.first;
     Aval[ count++ ] = CMjs.second;
    }

   ++j;
  }

 Abeg[ j ] = count;

 }  // end( LagBFunction::get_MatDesc )

/*--------------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE LagBFunction ---------*/
/*--------------------------------------------------------------------------*/

ThinVarDepInterface::Index LagBFunction::is_active( const Variable * var )
 const {
 auto idx = std::find_if( LagPairs.begin() , LagPairs.end() ,
			  [ & var ]( const auto & p )
			           { return( p.first == var ); } );

 return( idx != LagPairs.end() ? std::distance( LagPairs.begin() , idx )
 	                       : Inf< Index >() );

 }  // end( LagBFunction::is_active )

/*--------------------------------------------------------------------------*/

void LagBFunction::map_active( c_Vec_p_Var & vars , Subset & map ,
			       bool ordered ) const
{
 if( vars.empty() )
  return;

 if( map.size() < vars.size() )
  map.resize( vars.size() );

 if( ordered )
  for( Index i = 0 ; i < LagPairs.size() ; ++i ) {
   auto itvi = std::lower_bound( vars.begin() , vars.end() ,
                                 LagPairs[ i ].first );
   if( itvi != vars.end() )
    map[ std::distance( vars.begin() , itvi ) ] = i;
   else
    throw( std::invalid_argument( "map_active: some Variable not active" ) );
   }
 else {
  auto it = map.begin();
  for( auto var : vars ) {
   Index i = LagBFunction::is_active( var );
   if( i >= LagPairs.size() )
    throw( std::invalid_argument( "map_active: some Variable not active" ) );
   *( it++ ) = i;
   }
  }
 }  // end( LagBFunction::map_active )

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

bool LagBFunction::flush_v_tmpCP( void )
{
 bool tounlock = false;

 if( ! v_Block.front()->is_owned_by( f_id ) ) {  // if the inner Block is free
  if( ! v_Block.front()->lock( f_id ) )          // try to lock it
   throw( std::logic_error( "LagBFunction: cannot lock inner Block" ) );
  tounlock = true;                               // it'll have to be unlocked
  }

 f_play_dumb = true;                // ignore any ensuing Modification

 // work on each Objective (inner block) — linear or quadratic
 bool all_flushed = true;
 for( Index h = 0 ; h < v_Obj.size() ; ++h ) {
  if( ! v_ObjIsQuad[ h ] ) {  // the linear case
   auto * lf = static_cast< p_LF >( v_Obj[ h ]->get_function() );
   const Index cur  = lf->get_num_active_var();
   const Index pend = v_tmpCP[ h ].size();

   // ensure CostMatrix has at least 'cur' columns even if there is nothing to
   // flush
   if( CostMatrix[ h ].size() < cur ) {
    CostMatrix[ h ].reserve( cur );
    while( CostMatrix[ h ].size() < cur )
     CostMatrix[ h ].emplace_back();  // default column (0.0, {})
   }
   // if there are pending variables, pre-grow columns up to cur + pend
   if( pend && CostMatrix[ h ].size() < cur + pend ) {
    const Index target = cur + pend;
    CostMatrix[ h ].reserve( target );
    while( CostMatrix[ h ].size() < target )
     CostMatrix[ h ].emplace_back();  // default column (0.0, {})
   }

   const Index cm_sz = CostMatrix[ h ].size();
   const Index can   = ( cm_sz > cur ) ? std::min< Index >( pend , cm_sz - cur ) : 0;

   // use eNoBlck: the inner Block need not mirror the abstract change,
   // because LagBFunction manages the consistency externally (the added
   // variable carries no immediate cost contribution: linear coeff is set
   // when the Lagrangian costs are later updated via set_linear_term).
   // Without this, leaf Blocks whose add_Modification dispatcher does not
   // recognise FunctionModVarsAddd (e.g. ThermalUnitBlock) would throw.
   if( can == 1 )
    lf->add_variable( v_tmpCP[ h ].front().first ,
                      v_tmpCP[ h ].front().second , eNoBlck );
   else if( can > 1 ) {
    v_coeff_pair to_add( v_tmpCP[ h ].begin() , v_tmpCP[ h ].begin() + can );
    lf->add_variables( std::move( to_add ) , eNoBlck );
    }

   if( can )
    v_tmpCP[ h ].erase( v_tmpCP[ h ].begin() , v_tmpCP[ h ].begin() + can );

   // after add, ensure columns cover the new number of active variables
   {
    const Index new_cur = lf->get_num_active_var();
    if( CostMatrix[ h ].size() < new_cur ) {
     CostMatrix[ h ].reserve( new_cur );
     while( CostMatrix[ h ].size() < new_cur )
      CostMatrix[ h ].emplace_back();  // default column (0.0, {})
    }
   }

   if( ! v_tmpCP[ h ].empty() )
    all_flushed = false;
   }
  else {                      // the quadratic case
   auto * qf = static_cast< p_QF >( v_Obj[ h ]->get_function() );
   const Index cur  = qf->get_num_active_var();
   const Index pend = v_tmpCP[ h ].size();

   // ensure CostMatrix has at least 'cur' columns even if there is nothing to
   // flush
   if( CostMatrix[ h ].size() < cur ) {
    CostMatrix[ h ].reserve( cur );
    while( CostMatrix[ h ].size() < cur )
     CostMatrix[ h ].emplace_back();  // default column (0.0, {})
   }
   // if there are pending variables, pre-grow columns up to cur + pend
   if( pend && CostMatrix[ h ].size() < cur + pend ) {
    const Index target = cur + pend;
    CostMatrix[ h ].reserve( target );
    while( CostMatrix[ h ].size() < target )
     CostMatrix[ h ].emplace_back();  // default column (0.0, {})
   }

   const Index cm_sz = CostMatrix[ h ].size();
   const Index can   = ( cm_sz > cur ) ? std::min< Index >( pend , cm_sz - cur ) : 0;

   // see comment in the LinearFunction branch above for why eNoBlck
   if( can == 1 )
    qf->add_variable( v_tmpCP[ h ].front().first ,
                      v_tmpCP[ h ].front().second , 0 , eNoBlck );
   else if( can > 1 ) {
    v_coeff_triple vars( can , coeff_triple( nullptr , 0 , 0 ) );
    for( Index i = 0 ; i < can ; ++i ) {
     std::get< 0 >( vars[ i ] ) = v_tmpCP[ h ][ i ].first;
     std::get< 1 >( vars[ i ] ) = v_tmpCP[ h ][ i ].second;
     }
    qf->add_variables( std::move( vars ) , eNoBlck );
    }

   if( can )
    v_tmpCP[ h ].erase( v_tmpCP[ h ].begin() , v_tmpCP[ h ].begin() + can );

   // after add, ensure columns cover the new number of active variables
   {
    const Index new_cur = qf->get_num_active_var();
    if( CostMatrix[ h ].size() < new_cur ) {
     CostMatrix[ h ].reserve( new_cur );
     while( CostMatrix[ h ].size() < new_cur )
      CostMatrix[ h ].emplace_back();  // default column (0.0, {})
    }
   }

   if( ! v_tmpCP[ h ].empty() )
    all_flushed = false;
   }
  }
 if( all_flushed )
  for( auto & bucket : v_tmpCP )
   bucket.clear();

 f_play_dumb = false;               // back to normal operations

 return( tounlock );

 }  // end( LagBFunction::flush_v_tmpCP )

/*--------------------------------------------------------------------------*/

void LagBFunction::add_to_CostMatrix( v_c_dual_pair & newdp )
{
 f_active_dirty = true;  // dual-pair coupling changes -> v_active must rebuild

 // given a new vector of pairs < y_i , g_i( x ) >, that were not a part of
 // LagPairs already, update CostMatrix, which provides the information used
 // to compute the Lagrangian costs. the new g_i( x ) may contain some
 // Variable x_j that is not in the Objective of the inner Block already, in
 // which case this is added (and CostMatrix grows by one row)

 for( Index i = 0 ; i < newdp.size() ; ++i ) {  // for each < y_i , g_i( x ) >
  const auto gi = dynamic_cast< p_LF >( newdp[ i ].second );
  if( ! gi )
   throw( std::invalid_argument( "Lagrangian term not a LinearFunction" ) );

  const auto & rp = gi->get_v_var();
  for( const auto & rpj : rp ) {
   // for each Variable x_j in g_i( x ), add the pair < y_i , a_{ij} > to
   // CMh[ j ] (if it exists, otherwise create it)

   // construct the pair < y_i , a_{ij} > to be added to CMh[ j ]
   // note that we assume that auto contains new columns that will be added
   // after the current ones in LagPairs, hence the name that the ColVariable
   // newdp[ i ].first will get is LagPairs.size() + i
   const auto y_pair = mon_pair( LagPairs.size() + i , rpj.second );

   // identify the owning Block of the variable
   Index h;
   if( v_Obj.size() == 1 )
    h = 0;
   else {
    Block * bj = rpj.first->get_Block();
    auto it = Block2Idx.find( bj );
    if( it == Block2Idx.end() )
     throw( std::logic_error( "add_to_CostMatrix: variable block not found" )
	    );
    h = it->second;
    }

   // get the Function and active index
   auto * fobj = v_Obj[ h ];
   auto * fn = fobj->get_function();
   Index nv, j;
   if( ! v_ObjIsQuad[ h ] ) {
    auto * lf = static_cast< p_LF >( fn );
    nv = lf->get_num_active_var();
    j = lf->is_active( rpj.first );
    }
   else {
    auto * qf = static_cast< p_QF >( fn );
    nv = qf->get_num_active_var();
    j = qf->is_active( rpj.first );
    }

   // ensure CostMatrix has at least nv columns for this Objective
   if( CostMatrix[ h ].size() < nv ) {
    CostMatrix[ h ].reserve( nv );
    while( CostMatrix[ h ].size() < nv )
     CostMatrix[ h ].emplace_back();  // default column (0.0, {})
    }

   if( j >= nv ) {
    // x_j is not (yet) in obj, but it may be in v_tmpCP[ h ] already
    auto itv = std::find_if( v_tmpCP[ h ].begin() , v_tmpCP[ h ].end() ,
			     [ & ]( const auto & el )
			     { return( el.first == rpj.first ); } );
    if( itv == v_tmpCP[ h ].end() ) {
     // it was not in v_tmpCP[ h ], it has to be added now
     v_tmpCP[ h ].push_back( coeff_pair( rpj.first , 0 ) );
     CostMatrix[ h ].push_back( col_pair() );
     CostMatrix[ h ].back().first = 0;                  // c_j = 0
     CostMatrix[ h ].back().second.push_back( y_pair ); // add < y_i , a_{ij} >
     j = Inf< Index >();
     }
    else
     j = nv + std::distance( v_tmpCP[ h ].begin() , itv );
    }

   if( j < Inf< Index >() ) {
    // x_j was there already in CostMatrix, although possibly not in obj
    // find the place of < y_i , a_{ij} > in A_j; again, recall that the
    // name of y_i is LagPairs.size() + i
    auto & CMh = CostMatrix[ h ][ j ];
    auto itp = std::lower_bound( CMh.second.begin() , CMh.second.end() ,
                                 mon_pair( LagPairs.size() + i , 0 ) ,
                                 []( const auto & a , const auto & b )
                                 { return( a.first < b.first ); } );
    // add < y_i , a_{ij} > to A_j
    CMh.second.insert( itp , y_pair );
    }
   }  // end( for( each monomial in g_i( x ) ) )
  }  // end( for( each Lagrangian pair < y_i , g_i( x ) > ) )

 // if needed, immediately flush the set of variables to be re-added to obj;
 // if the inner Block had to be locked for this, unlock it
 bool pend = false;
 for( const auto & bucket : v_tmpCP )
  if( ! bucket.empty() ) { pend = true; break; }

 if( pend && flush_v_tmpCP() )
  v_Block.front()->unlock( f_id );

 }  // end( LagBFunction::add_to_CostMatrix )

/*--------------------------------------------------------------------------*/

void LagBFunction::mod_CostMatrix( Index i , Index first )
{
 // in the existing Lagrangian term < y_i , g_i( x ) >, new monomials have
 // been added to g_i( x ) at position first and following: update
 // CostMatrix, which provides the information used to compute the Lagrangian
 // costs. the new monomials in g_i( x ) may contain some Variable x_j that
 // is not in the Objective of the inner Block already, in which case this
 // is added (and CostMatrix grows by one row)

 const auto gi = static_cast< p_LF >( LagPairs[ i ].second );
 const auto & rp = gi->get_v_var();

 #ifndef NDEBUG
  if( first >= rp.size() )
   throw( std::logic_error( "inconsistent first in add_columns()" ) );
 #endif

 for( Index h = first ; h < rp.size() ; ++h ) {
  // for each Variable x_j in g_i( x ) in a monomial with index >= first, add
  // the pair < y_i , a_{ij} > to CMh[ j ] (if it exists, otherwise
  // create it)

  // construct the pair < y_i , a_{ij} > to be added to CMh[ j ]
  const auto y_pair = mon_pair( i , rp[ h ].second );

  // identify the owning Block of the variable
  Index k;
  if( v_Obj.size() == 1 )
   k = 0;
  else {
   Block * bj = rp[ h ].first->get_Block();
   auto it = Block2Idx.find( bj );
   if( it == Block2Idx.end() )
    throw( std::logic_error( "mod_CostMatrix: variable block not found" ) );
   k = it->second;
   }

  // get the Function and active index
  auto * fobj = v_Obj[ k ];
  auto * fn = fobj->get_function();
  Index nv, j;
  if( ! v_ObjIsQuad[ k ] ) {
   auto * lf = static_cast< p_LF >( fn );
   nv = lf->get_num_active_var();
   j = lf->is_active( rp[ h ].first );
   }
  else {
   auto * qf = static_cast< p_QF >( fn );
   nv = qf->get_num_active_var();
   j = qf->is_active( rp[ h ].first );
   }

  if( j >= nv ) {
   // x_j is not (yet) in obj, but it may be in v_tmpCP[ k ] already
   auto itv = std::find_if( v_tmpCP[ k ].begin() , v_tmpCP[ k ].end() ,
			    [ & ]( const auto & el )
			    { return( el.first == rp[ h ].first ); } );
   if( itv == v_tmpCP[ k ].end() ) {
    // it was not in v_tmpCP[k], it has to be added now
    v_tmpCP[ k ].push_back( coeff_pair( rp[ h ].first , 0 ) );
    CostMatrix[ k ].push_back( col_pair() );
    CostMatrix[ k ].back().first = 0;                  // c_j = 0
    CostMatrix[ k ].back().second.push_back( y_pair ); // add < y_i , a_{ij} >
    j = Inf< Index >();
    }
   else
    j = nv + std::distance( v_tmpCP[ k ].begin() , itv );
   }

  if( j < Inf< Index >() ) {
   // x_j was there already in CostMatrix, although possibly not in obj
   // find the place of < y_i , a_{ij} > in A_j
   auto & CMj = CostMatrix[ k ][ j ];
   auto itp = std::lower_bound( CMj.second.begin() , CMj.second.end() ,
                                mon_pair( i , 0 ) ,
                                []( const auto & a , const auto & b )
                                { return( a.first < b.first ); } );
   // add < y_i , a_{ij} > to A_j
   CMj.second.insert( itp , y_pair );
   }
  }  // end( for( each monomial in g_i( x ) ) )
 }  // end( LagBFunction::mod_CostMatrix )

/*--------------------------------------------------------------------------*/

void LagBFunction::init_CC( void )
{
 f_CC = new ComputeConfig;  // create a new empty one
 f_CC->set_diff( true );    // set it in "diff mode"
 }

/*--------------------------------------------------------------------------*/

void LagBFunction::guts_of_destructor( bool deleteinner )
{
 // clear() all the LagBFunction - - - - - - - - - - - - - - - - - - - - - - -

 clear();

 // cleanup and possibly delete the inner Block - - - - - - - - - - - - - - -

 if( ! v_Block.empty() ) {  // ... if any
  // the inner Block is orphan
  v_Block.front()->set_f_Block( nullptr );

  // use the clear()-ed BlockSolverConfig to delete all the Solver that were
  // registered by it (hence, be sure it is in diff() == true mode)
  if( f_BSC )
   f_BSC->apply( v_Block.front() );

  // now finally the inner Block can be deleted
  if( deleteinner )
   delete v_Block.front();
  }

 v_Block.clear();
 delete f_BSC;
 f_BSC = nullptr;
 delete f_CC;
 f_CC = nullptr;

 }  // end( LagBFunction::guts_of_destructor )

/*--------------------------------------------------------------------------*/

char LagBFunction::guts_of_add_Modification( p_Mod mod , ChnlName chnl )
{
 const auto tmod = dynamic_cast< GroupModification * >( mod );
 if( tmod ) {
  // process every Modification inside the GroupModification, returning true
  // if any one of those returned true
  char what = 0;
  for( auto & ttmod : tmod->sub_Modifications() )
   what |= guts_of_add_Modification( ttmod.get() , chnl );
  return( what );
  }
 else
  return( guts_of_guts_of_add_Modification( mod , chnl ) );

 }  // end( guts_of_add_Modification )

/*--------------------------------------------------------------------------*/

char LagBFunction::guts_of_guts_of_add_Modification( p_Mod mod ,
						     ChnlName chnl )
{
 // process Modification - - - - - - - - - - - - - - - - - - - - - - - - - - -
 /* This requires to patiently sift through the possible Modification types
  * to find what this Modification exactly is and appropriately react.
  * However, fro Modification coming directly from the LagBFunction this is
  * immediately deferred to guts_of_this_add_Modification(). */

 if( mod->get_Block() == this )
  return( guts_of_this_add_Modification( mod , chnl ) );
 
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // C05FunctionModLin: the "linear part" of a Function has been changed
 // C05FunctionModLin can have a special treatment, and therefore need be
 // checked before FunctionMod (because C05FunctionModLin is a FunctionMod)
 // in case they come from:
 //
 // - the (LinearFunction or DQuadFunction inside the) Objective of the inner
 //   Block;
 //
 // There are two types of C05FunctionModLin, according to if the
 // coefficients of the LinearFunction that change are a Range or a Subset.
 // Hence, two almost identical pieces of code follow, one for each of them.
 //
 // IMPORTANT NOTE: Modification coming from obj can be "arbitrarily
 //                 delayed", since (say) they can be stored in a
 // GroupModification and only processed a lot later. In particular, the
 // indices in delta() may NO LONGER CORRESPOND TO THE CURRENT POSITION OF
 // THE ColVariable IN obj, SINCE "ACTIVE" Variable MAY HAVE BEEN ADDED OR
 // DELETED. However, if this happens it is signalled by a Modification to be
 // found *after* the current one. CostMatrix is kept parallel to obj as these
 // Modification happen, which means that the current status of CostMatrix is
 // exactly parallel to the status of obj at the time in which the
 // Modification was issued, which allows directly using range().
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 Index h = 0;
 Block * block;
 if( PushCostToOwner ) {
  block = mod->get_Block();
  auto it = Block2Idx.find( block );
  if( it == Block2Idx.end() )
   throw( std::logic_error( "LagBFunction::add_Modification: mod is from "
			    "unknown Block" ) );
  h = it->second;
  }
 else
  block = v_Block.front();

 auto & CMh = CostMatrix[ h ];
 auto * CMh_f = v_Obj[ h ]->get_function();

 // C05FunctionModLinRngd- - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( const auto tmod = dynamic_cast< const C05FunctionModLinRngd * >( mod )
     ) {
  if( const auto lf = dynamic_cast< p_LF >( tmod->function() ) ) {
   // only deal with C05FunctionModLinRngd coming from LinearFunction ...
   if( lf == CMh_f ) {  // ... inside the Objective of the inner Block - - - -

    update_CostMatrix_ModLinRngd( lf->get_v_var() , tmod->vars() ,
				  tmod->range() );

    // issue a LagBFunctionMod modification of the type AlphaChanged and
    // with what() == 1: the Lagrangian function unpredictably changes
    // (f_shift == NaN), and the constant terms \alpha = c x^* of the
    // linearizations ( g , \alpha ) have to be computed again since
    // c has changed (while g remains unchanged)
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                     this , C05FunctionMod::AlphaChanged ,
                                     Subset() , 1 , NaN , true ) ,
				   chnl );
    return( 0 );  // all done

    }  // end( coming from obj )
   }  // end( coming from a LinearFunction )

  if( const auto qf = dynamic_cast< p_QF >( tmod->function() ) )
   // only deal with C05FunctionModLinRngd coming from DQuadFunction ...
   if( qf == CMh_f ) {  // ... inside the Objective of the inner Block- - - -
    v_coeff_pair rc;
    triple_to_pair( qf->get_v_var() , rc );
    update_CostMatrix_ModLinRngd( rc , tmod->vars() , tmod->range() );

    // issue a LagBFunctionMod modification of the type AlphaChanged and
    // with what() == 1: the Lagrangian function unpredictably changes
    // (f_shift == NaN), and the constant terms \alpha = c x^* of the
    // linearizations ( g , \alpha ) have to be computed again since
    // c has changed (while g remains unchanged)
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                      this , C05FunctionMod::AlphaChanged ,
                                      Subset() , 1 , NaN , true ) ,
				   chnl );
    return( 0 );  // all done

    }  // end( coming from qobj )

  // note: since we do know this is a C05FunctionModLinRngd we should now
  //       avoid checking for all clearly incompatible types like
  //       C05FunctionModLinSbst, but this would mess up too much with the
  //       code flow, so the hell with it
  }  // end( C05FunctionModLinRngd )

 // C05FunctionModLinSbst- - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // same comments and IMPORTANT NOTESs as for C05FunctionModLinRngd, except
 // of course there is a Subset rather than a Range
 if( const auto tmod = dynamic_cast< const C05FunctionModLinSbst * >( mod )
     ) {
  if( const auto lf = dynamic_cast< p_LF >( tmod->function() ) ) {
   // only deal with C05FunctionModLinSbst coming from LinearFunction ...
   if( lf == CMh_f ) {  // ... inside the Objective of the inner Block - - - -
    update_CostMatrix_ModLinSbst( lf->get_v_var() , tmod->vars() ,
				  tmod->subset() );

    // issue a LagBFunctionMod modification of the type AlphaChanged and
    // with what() == 1: the Lagrangian function unpredictably changes
    // (f_shift == NaN), and the constant terms \alpha = c x^* of the
    // linearizations ( g , \alpha ) have to be computed again since
    // c has changed (while g remains unchanged)
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                     this , C05FunctionMod::AlphaChanged ,
                                     Subset() , 1 , NaN , true ) ,
				   chnl );
    return( 0 );  // all done

    }  // end( coming from obj )
   }  // end( coming from a LinearFunction )

  if( const auto qf = dynamic_cast< p_QF >( tmod->function() ) )
   // only deal with C05FunctionModLinSbst coming from DQuadFunction ...
   if( qf == CMh_f ) {  // ... inside the Objective of the inner Block- - - -
    v_coeff_pair rc;
    triple_to_pair( qf->get_v_var() , rc );
    update_CostMatrix_ModLinSbst( rc , tmod->vars() , tmod->subset() );

    // issue a LagBFunctionMod modification of the type AlphaChanged and
    // with what() == 1: the Lagrangian function unpredictably changes
    // (f_shift == NaN), and the constant terms \alpha = c x^* of the
    // linearizations ( g , \alpha ) have to be computed again since
    // c has changed (while g remains unchanged)
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                      this , C05FunctionMod::AlphaChanged ,
                                      Subset() , 1 , NaN , true ) ,
				   chnl );
    return( 0 );  // all done

    }  // end( coming from qobj )

  }  // end( C05FunctionModLinSbst )

 // C05FunctionModRngd - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // the interesting case for a C05FunctionModRngd is when it signals the
 // changes the *quadratic* coefficients (and, possibly the linear ones as
 // well) in the DQuadFunction inside the Objective of the inner Block
 if( const auto tmod = dynamic_cast< const C05FunctionModRngd * >( mod ) )
  if( const auto qf = dynamic_cast< p_QF >( tmod->function() ) )
   if( qf == CMh_f ) {
    v_coeff_pair rc;
    triple_to_pair( qf->get_v_var() , rc );
    update_CostMatrix_ModLinRngd( rc , tmod->vars() , tmod->range() );

    // issue a LagBFunctionMod modification of the type AlphaChanged and
    // with what() == 1: the Lagrangian function unpredictably changes
    // (f_shift == NaN), and the constant terms \alpha = c x^* of the
    // linearizations ( g , \alpha ) have to be computed again since
    // c has changed (while g remains unchanged)
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                     this , C05FunctionMod::AlphaChanged ,
                                     Subset() , 1 , NaN , true ) ,
				   chnl );
    return( 0 );  // all done
    }

 // C05FunctionModSbst - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // the interesting case for a C05FunctionModSbst is when it signals the
 // changes the *quadratic* coefficients (and, possibly the linear ones as
 // well) in the DQuadFunction inside the Objective of the inner Block
 if( const auto tmod = dynamic_cast< const C05FunctionModSbst * >( mod ) )
  if( const auto qf = dynamic_cast< p_QF >( tmod->function() ) )
   if( qf == CMh_f ) {
    v_coeff_pair rc;
    triple_to_pair( qf->get_v_var() , rc );
    update_CostMatrix_ModLinSbst( rc , tmod->vars() , tmod->subset() );

    // issue a LagBFunctionMod modification of the type AlphaChanged and
    // with what() == 1: the Lagrangian function unpredictably changes
    // (f_shift == NaN), and the constant terms \alpha = c x^* of the
    // linearizations ( g , \alpha ) have to be computed again since
    // c has changed (while g remains unchanged)
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                     this , C05FunctionMod::AlphaChanged ,
                                     Subset() , 1 , NaN , true ) ,
				   chnl );
    return( 0 );  // all done
    }

 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // FunctionMod: a Function has been changed
 // changes in a Function can come from three different components:
 //
 // - the (LinearFunction or DQuadFunction inside the) Objective of the inner
 //   Block, or any of its sub-Block (recursively); if it is obj, the only
 //   remaining FunctionMod is the C05FunctionMod with type() ==
 //   NothingChanged corresponding to the change of the constant term
 //
 // - the LinearFunction that defines a Lagrangian term < y_i , g_i( x ) >;
 //   also in this case, the only remaining FunctionMod is the C05FunctionMod
 //   with type() == NothingChanged corresponding to the change of the
 //   constant term - but this case is considered elsewhere
 //
 // - any Constraint in the inner Block, or any of its sub-Block (recursively)
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( const auto tmod = dynamic_cast< const FunctionMod * >( mod ) ) {
  auto f = tmod->function();  // the Function it comes from

  if( f == CMh_f ) {  // if it is obj- - - - - - - - - - - - - - - - - - - - -
   // the only remaining FunctionMod is the C05FunctionMod with type() ==
   // NothingChanged corresponding to the change of the constant term from
   // c_0 to c'_0; hence the whole Lagrangian function is shifted by the
   // same amount, i.e., issue a LagBFunctionMod with type() ==
   // NothingChanged, what() == 1 and the very same shift() == c'_0 - c_0

   if( f_Observer )
    f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                    this , C05FunctionMod::NothingChanged ,
                                    Subset() , 1 , tmod->shift() , true ) ,
				  chnl );
   return( 0 );  // all done

   }  // end( if( from obj ) )

  if( dynamic_cast< Objective * >( f->get_Observer() ) ) {
   //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   // if it is not obj, it may still be the Function inside the Objective of a
   // further sub-Block of the inner Block
   // note that this cannot possibly happen if PushCostToOwner == true since
   // the case in which the Modification comes from the Objective has already
   // been completely dealt with previously
   assert( ! PushCostToOwner );

   if( ( ! std::isnan( tmod->shift() ) ) &&
       ( tmod->shift() < INF ) && ( tmod->shift() > -INF ) ) {
    // a finite shift() == a predictable change == the whole Objective has
    // changed by shift(): like in the case of obj, issue a LagBFunctionMod
    // with type() == NothingChanged, what() == 1 and the very same shift()

    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                     this , C05FunctionMod::NothingChanged ,
                                     Subset() , 1 , tmod->shift() , true ) ,
				   chnl );
    }
   else {  // an unpredictable change in an Objective
    // issue a LagBFunctionMod modification of the type AlphaChanged and
    // with what() == 1: the Lagrangian function unpredictably changes
    // (f_shift == NaN), and the constant terms \alpha = c x^* of the
    // linearizations ( g , \alpha ) have to be computed again since
    // c has changed (while g remains unchanged)
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                     this , C05FunctionMod::AlphaChanged ,
                                     Subset() , 1 , NaN , true ) ,
				   chnl );
    }

   return( 0 );  // in either case, all is done

   }  // end( if( from the Objective of a further sub-Block ) )

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // here comes the last and final case: f belongs to some [FRow]Constraint
  // if the Function has changed unpredictably, then there is no way one
  // can guarantee that the previous Solutions have remained feasible
  if( std::isnan( tmod->shift() ) )
   return( 4 );

  // if the Constraint is a [F]RowConstraint, it is surely not violated
  // if shift() > 0 and RHS == +INF or shift() < 0 and LHS == -INF,
  // otherwise in principle it can be violated and we need to check
  if( auto cnsobs = dynamic_cast< FRowConstraint * >( f->get_Observer() ) )
   if( ( ( tmod->shift() > 0 ) && ( cnsobs->get_rhs() <  INF ) ) ||
       ( ( tmod->shift() < 0 ) && ( cnsobs->get_lhs() > -INF ) ) ) {
    f_Lc = -1;    // yet, the Lipschitz constant must be recomputed
    return( 0 );
    }

  // this is a Function that has changed in some way we don't understand:
  // take the safe route and re-check feasibility
  return( 4 );

  }  // end( FunctionMod )

 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // FunctionModVars: some Variable have been added/removed from a Function
 // C05FunctionModVars can have a special treatment, and therefore need be
 // checked before FunctionModVars (because C05FunctionModVars is a
 // FunctionModVars) in case they come from  the (LinearFunction or
 // DQuadFunction inside the) Objective of the inner Block
 //
 // There are three types of C05FunctionModVars, according to if the
 // Variable are added or deleted, and in the latter case if what is
 // deleted is a Range or a Subset. Hence, three similar pieces of code
 // follow, two almost being identical.
 //
 // IMPORTANT NOTE: see IMPORTANT NOTE for the C05FunctionModLin, which
 //                 apply verbatim here as well
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // C05FunctionModVarsAddd - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // this is: [Col]Variable are being added
 if( const auto tmod = dynamic_cast< const C05FunctionModVarsAddd * >( mod )
     ) {
  if( const auto lf = dynamic_cast< const p_LF >( tmod->function() ) ) {
   // only deal with C05FunctionModVarsAddd coming from LinearFunction ...
   if( lf == CMh_f ) {  // ... inside the Objective of the inner Block - - - -
    // update CostMatrix accordingly
    update_CostMatrix_ModVarsAddd( tmod->vars() , tmod->first() );

    // issue a LagBFunctionMod modification of the type AlphaChanged and
    // with what() == 1: the Lagrangian function unpredictably changes
    // (f_shift == NaN), and the constant terms \alpha = c x^* of the
    // linearizations ( g , \alpha ) have to be computed again since
    // c has changed (while g remains unchanged)
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
			                 this , C05FunctionMod::AlphaChanged ,
					 Subset() , 1 , NaN , true ) ,
				   chnl );
    return( 0 );  // all done

    }  // end( coming from obj )
   }  // end( coming from a LinearFunction )

  if( const auto qf = dynamic_cast< const p_QF >( tmod->function() ) )
   // only deal with C05FunctionModVarsAddd coming from DQuadFunction ...
   if( qf == CMh_f ) {  // ... inside the Objective of the inner Block- - - -
    // update CostMatrix accordingly
    update_CostMatrix_ModVarsAddd( tmod->vars() , tmod->first() );

    // issue a LagBFunctionMod modification of the type AlphaChanged and
    // with what() == 1: the Lagrangian function unpredictably changes
    // (f_shift == NaN), and the constant terms \alpha = c x^* of the
    // linearizations ( g , \alpha ) have to be computed again since
    // c has changed (while g remains unchanged)
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                      this , C05FunctionMod::AlphaChanged ,
				      Subset() , 1 , NaN , true ) ,
				   chnl );
    return( 0 );  // all done

     }  // end( coming from qobj )

  // note: since we do know this is a C05FunctionModVarsAddd we should now
  //       avoid checking for all clearly incompatible types like
  //       C05FunctionModVarsRngd and C05FunctionModVarsSbst, but this would
  //       mess up too much with the code flow, so the hell with it
  }  // end( C05FunctionModVarsAddd )

 // C05FunctionModVarsRngd - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // this is: a Range of [Col]Variable are being removed
 if( const auto tmod = dynamic_cast< const C05FunctionModVarsRngd * >( mod )
     ) {
  if( const auto lf = dynamic_cast< p_LF >( tmod->function() ) ) {
   // only deal with C05FunctionModVarsRngd coming from LinearFunction ...
   if( lf == CMh_f ) {  // ... inside the Objective of the inner Block - - - -
    // remove the range of rows from CostMatrix accordingly
    update_CostMatrix_ModVarsRngd( tmod->vars() , tmod->range() );

    // issue a LagBFunctionMod modification of the type AlphaChanged and
    // with what() == 1: the Lagrangian function unpredictably changes
    // (f_shift == NaN), and the constant terms \alpha = c x^* of the
    // linearizations ( g , \alpha ) have to be computed again since c has
    // changed (while g remains unchanged)
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                     this , C05FunctionMod::AlphaChanged ,
                                     Subset() , 1 , NaN , true ) ,
				   chnl );
    return( 0 );  // all done

    }  // end( coming from obj )
   }  // end( coming from a LinearFunction )

  if( const auto qf = dynamic_cast< p_QF >( tmod->function() ) )
   // only deal with C05FunctionModVarsRngd coming from DQuadFunction ...
   if( qf == CMh_f ) {  // ... inside the Objective of the inner Block- - - -
    // remove the range of rows from CostMatrix accordingly
    update_CostMatrix_ModVarsRngd( tmod->vars() , tmod->range() );

    // issue a LagBFunctionMod modification of the type AlphaChanged and
    // with what() == 1: the Lagrangian function unpredictably changes
    // (f_shift == NaN), and the constant terms \alpha = c x^* of the
    // linearizations ( g , \alpha ) have to be computed again since c has
    // changed (while g remains unchanged)
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                      this , C05FunctionMod::AlphaChanged ,
                                      Subset() , 1 , NaN , true ) ,
				   chnl );
    return( 0 );  // all done

    }  // end( coming from qobj )

  // note: we should now avoid checking for C05FunctionModVarsSbst, but
  // the hell with it
  }  // end( C05FunctionModVarsRngd )

 // C05FunctionModVarsSbst - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // this is: a Subset of [Col]Variable are being removed
 if( const auto tmod = dynamic_cast< const C05FunctionModVarsSbst * >( mod )
     ) {
  if( const auto lf = dynamic_cast< p_LF >( tmod->function() ) ) {
   // only deal with C05FunctionModVarsSbst coming from LinearFunction ...
   if( lf == CMh_f ) {  // ... inside the Objective of the inner Block - - - -
    // remove the range of subset from CostMatrix accordingly
    update_CostMatrix_ModVarsSbst( tmod->vars() , tmod->subset() );

    // issue a LagBFunctionMod modification of the type AlphaChanged and
    // with what() == 1: the Lagrangian function unpredictably changes
    // (f_shift == NaN), and the constant terms \alpha = c x^* of the
    // linearizations ( g , \alpha ) have to be computed again since
    // c has changed (while g remains unchanged)
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                     this , C05FunctionMod::AlphaChanged ,
                                     Subset() , 1 , NaN , true ) ,
				   chnl );
    return( 0 );  // all done

    }  // end( coming from obj )
   }  // end( coming from a LinearFunction )

  if( const auto qf = dynamic_cast< p_QF >( tmod->function() ) )
   // only deal with C05FunctionModVarsSbst coming from DQuadFunction ...
   if( qf == CMh_f ) {  // ... inside the Objective of the inner Block- - - -
    // remove the range of subset from CostMatrix accordingly
    update_CostMatrix_ModVarsSbst( tmod->vars() , tmod->subset() );

    // issue a LagBFunctionMod modification of the type AlphaChanged and
    // with what() == 1: the Lagrangian function unpredictably changes
    // (f_shift == NaN), and the constant terms \alpha = c x^* of the
    // linearizations ( g , \alpha ) have to be computed again since
    // c has changed (while g remains unchanged)
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                      this , C05FunctionMod::AlphaChanged ,
                                      Subset() , 1 , NaN , true ) ,
				   chnl );
    return( 0 );  // all done

    }  // end( coming from qobj )

  }  // end( C05FunctionModVarsSbst )

 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // FunctionModVars: some Variable have been added/removed from a Function
 //
 // The Function can *not* be the (LinearFunction or DQuadFunction inside
 // the) Objective of the inner Block since these have been dealt with
 // already. What remains is the Objective of some sub-Block of the inner
 // Block, or a Constraint in the inner Block (or any of its sub-Block,
 // recursively)
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( const auto tmod = dynamic_cast< const FunctionModVars * >( mod ) ) {
  auto f = tmod->function();  // the Function it comes from

  if( dynamic_cast< Objective * >( f->get_Observer() ) ) {
   //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   // the Function inside the Objective of a sub-Block of the inner Block
   // note that this cannot possibly happen if PushCostToOwner == true since
   // the case in which the Modification comes from the Objective has already
   // been completely dealt with previously
   assert( ! PushCostToOwner );

   // issue a LagBFunctionMod modification of the type AlphaChanged and
   // with what() == 1: the Lagrangian function unpredictably changes
   // (f_shift == NaN), and the constant terms \alpha =  c x^* of the
   // linearizations ( g , \alpha ) have to be computed again since
   // c has changed (while g remains unchanged)
   if( f_Observer )
    f_Observer->add_Modification( std::make_shared< LagBFunctionMod >(
                                    this , C05FunctionMod::AlphaChanged ,
                                    Subset() , 1 , NaN , true ) ,
				  chnl );
   return( 0 );  // all done

   }  // end( if( from the Objective of a further sub-Block ) )

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // here comes the last and final case: f belongs to some constraint
  // in theory, adding Variable should not violate the Constraint ...
  // but this is only true if, say, the Constraint is linear and the
  // [Col]Variable are allowed to take the value 0. since we have no
  // way of knowing whether or not this is true, we have to assume it is not
  return( 4 );

  }  // end( FunctionModVars )

 // VariableMod: some variables of (B) changed the status- - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( const auto tmod = dynamic_cast< const VariableMod * >( mod ) ) {
  const auto xj = dynamic_cast< const ColVariable * >( tmod->variable() );

  if( ! xj )     // unknown variable type
   return( 8 );  // no clue what is happening, take the worst case

  // check the current state of the ColVariable against its previous state:
  // if the change of state increased the set of values that the ColVariable
  // can have then return 0 (nothing has to be done), otherwise return 8
  // (feasibility has to be checked)
  if( ( ( xj->is_fixed() == xj->is_fixed( tmod->old_state() ) ) ||
	( ( ! xj->is_fixed() ) && xj->is_fixed( tmod->old_state() ) ) )
      && ( ( xj->is_integer() == xj->is_integer( tmod->old_state() ) ) ||
	   ( ( ! xj->is_integer() ) && xj->is_integer( tmod->old_state() ) ) )
      && ( ( xj->is_positive() == xj->is_positive( tmod->old_state() ) ) ||
	   ( ( ! xj->is_positive() ) && xj->is_positive( tmod->old_state() ) )
	   )
      && ( ( xj->is_negative() == xj->is_negative( tmod->old_state() ) ) ||
	   ( ( ! xj->is_negative() ) && xj->is_negative( tmod->old_state() ) )
	   )
      && ( ( xj->is_unitary() == xj->is_unitary( tmod->old_state() ) ) ||
	   ( ( ! xj->is_unitary() ) && xj->is_unitary( tmod->old_state() ) ) )
      ) {
   f_Lc = -1;    // the Lipschitz constant must be computed
   return( 0 );
   }
  else
   return( 8 );

  }  // end( VariableMod )

 // RowConstraintMod: the LHS/RHS of some constraints of (B) changed - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( const auto tmod = dynamic_cast< const RowConstraintMod * >( mod ) ) {
  // return true if the RHS and/or LHS have changed
  // TODO: if the RHS increases or the LHS decreases in fact the feasible
  //       region has increased and in fact false should be returned, but so
  //       far there is no way to detect this since we don't have access to
  //       the previous value; some work should be done on RowConstraintMod
  if( ( tmod->type() == RowConstraintMod::eChgLHS ) ||
      ( tmod->type() == RowConstraintMod::eChgRHS ) ||
      ( tmod->type() == RowConstraintMod::eChgBTS ) )
   return( 2 );
  // otherwise do nothing, as the case is dealt with next
  }

 // ConstraintMod: some constraints of (B) relaxed/enforced- - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( const auto tmod = dynamic_cast< const ConstraintMod * >( mod ) ) {
  // return true if a Constraint has been enforced, since this reduces the
  // feasible region, and false if a Constraint has been relaxed, since this
  // enlarges the feasible region
  f_Lc = -1;  // the Lipschitz constant must be computed
  return( tmod->type() == ConstraintMod::eEnforceConst ? 16 : 0 );
  }

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
 if( const auto tmod = dynamic_cast< const BlockModAD * >( mod ) ) {
  f_Lc = -1;  // the Lipschitz constant must be computed
  return( ( tmod->is_variable() && ( ! tmod->is_added() ) ) ||
	  ( ( ! tmod->is_variable() ) && tmod->is_added() ) ? 32 : 0 );
  }

 // BlockMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // arbitrary changes of (B) may violate the feasibility
 if( dynamic_cast< const BlockMod * >( mod ) )
  return( 64 );

 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 return( 0 );  // ignore any other Modification (BAD!!)
 // indeed, the safe return value would be 128: if I don't understand it,
 // it can wreak arbitrary havok. but this would be severely over-reacting
 // in many cases, so we avoid it for the time being
 //
 // yet another example about why we should be adding some "semantic"
 // information to Modification that give an idea of the kind of change that
 // they can exert on the model

 }  // end( LagBFunction::guts_of_guts_of_add_Modification )

/*--------------------------------------------------------------------------*/

char LagBFunction::guts_of_this_add_Modification( p_Mod mod , ChnlName chnl )
{
 // process a Modification coming directly from the LagBFunction - - - - - - -
 /* This requires to patiently sift through the possible Modification types
  * to find what this Modification exactly is and appropriately react, but
  * the list is shorter since the Modification can only come from the
  * the LinearFunction that defines one of the Lagrangian terms
  *  < y_i , g_i( x ) = A_i x + b_i >
  * where g_i is a LinearFunction, which is checked immediately. */

 p_LF lf = nullptr;
 if( auto fmod = dynamic_cast< const FunctionMod * >( mod ) )
  lf = dynamic_cast< p_LF >( fmod->function() );
 else
  if( auto fmodv = dynamic_cast< const FunctionModVars * >( mod ) )
   lf = dynamic_cast< p_LF >( fmodv->function() );
  else
   throw( std::logic_error( "LagBFunction::add_Modification: unexpected "
			    "non-FunctionMod[Vars] from this" ) );
 if( ! lf )
  throw( std::logic_error( "LagBFunction::add_Modification: mod from "
			   "Lagrangian term which is not LinearFunction" ) );

 // keep information about the current Block to which the "current
 // ColVariable being looked at" belongs to: while it may change (if
 // PushCostToOwner == true), it's likely pretty stable so that we can
 // avoid repeated searches. initialization is to the Block being the
 // inner Block of the LagBFunction, which is always right when
 // PushCostToOwner == false
 Block * block = v_Block.front();            // current block
 Index b = 0;                                // its index
 auto & CMb = CostMatrix[ b ];               // its CostMatrix[]
 auto * CMb_f = v_Obj[ b ]->get_function();  // its Objective and ...
 Index nv = CMb_f->get_num_active_var();     // its number of variables

 // small local lambda to keep the stuff above updated when a new variable
 // x is looked at; only needs be called if PushCostToOwner == true
 auto updatestuff =
  [ & block , & b , & CMb , & CMb_f , & nv , this ] ( Variable * x )
  -> void {
  if( auto cb = x->get_Block() ; cb != block ) {
   // find the sub-Block to which x belongs
   auto it = Block2Idx.find( cb );
   if( ( it == Block2Idx.end() ) || ( it->second >= v_Obj.size() ) )
    throw( std::logic_error( "Variable not found in any objective" ) );
   b = it->second;
   block = cb;
   CMb = CostMatrix[ b ];
   CMb_f = v_Obj[ b ]->get_function();
   nv = CMb_f->get_num_active_var();
   }
  };

 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // IMPORTANT NOTE: unlike the general case, Modification coming from
 //                 Lagrangian terms "immediately reach" the LagBFunction,
 // since they do not pass from any other Block before and therefore they
 // cannot ever be packed in a GroupModification and delayed (before getting
 // here, this can happen for Block further up the tree and for Solver)
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // C05FunctionModLin: the "linear part" of a Function has been changed
 // C05FunctionModLin can have a special treatment, and therefore need be
 // checked before FunctionMod (because C05FunctionModLin is a FunctionMod)
 //
 // There are two types of C05FunctionModLin, according to if the
 // coefficients of the LinearFunction that change are a Range or a Subset.
 // Hence, two almost identical pieces of code follow, one for each of them.
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // C05FunctionModLinRngd- - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( const auto tmod = dynamic_cast< const C05FunctionModLinRngd * >( mod )
     ) {
  // the corresponding entry of all the linearizations changes

  // search for the Lagrangian term which has changed
  auto it = std::find_if( LagPairs.begin() , LagPairs.end() ,
			  [ lf ]( auto & p ) { return( p.second == lf ); }
			  );
  #ifndef NDEBUG
   if( it == LagPairs.end() )
    throw( std::logic_error( "LagBFunction::add_Modification: Lagrangian "
			     "term not found" ) );
  #endif

  Index i = std::distance( LagPairs.begin() , it );
  const auto & rc = lf->get_v_var();
  auto dit = tmod->delta().begin();

  // for all the coefficients a_{ij} in A_j that have changed
  for( Index h = tmod->range().first ; h < tmod->range().second ; ++h ) {
   if( PushCostToOwner )
    updatestuff( rc[ h ].first );

   auto j = CMb_f->is_active( rc[ h ].first ); // find x_j

   // find the place of < y_i , a_{ij} > in A_j (has to be there)
   auto ajit = std::lower_bound( CMb[ j ].second.begin() ,
				 CMb[ j ].second.end() ,
				 mon_pair( i , 0 ) ,
				 []( const auto & a , const auto & b ) {
				  return( a.first < b.first ); } );
   #ifndef NDEBUG
    if( ajit == CMb[ j ].second.end() )
     throw( std::logic_error( "LagBFunction::add_Modification: "
			      "inconsistent CostMatrix" ) );
   #endif

   ajit->second += *( dit++ );  // update a_{ij}

   }  // end( for( all the changed a_{ij} ) )

  f_dirty_Lc = true;  // Lagrangian costs will have to be recomputed
  f_Lc = -1;          // the Lipschitz constant must be computed

  // issue a C05FunctionModRngd saying that the entry i of all
  // the linearizations in the global pool has changed (the value of
  // the function has changed unpredictably, i.e., shift() == NaN)
  if( f_Observer )
   f_Observer->add_Modification( std::make_shared< C05FunctionModRngd >(
			       this , C05FunctionMod::AllEntriesChanged ,
			       Vec_p_Var( { it->first } ) ,
			       Range( i , i + 1 ) , Subset() , NaN , true ) ,
				 chnl );

  return( 0 );  // all done

  // note: since we do know this is a C05FunctionModLinRngd we should now
  //       avoid checking for all clearly incompatible types like
  //       C05FunctionModLinSbst, but this would mess up too much with the
  //       code flow, so the hell with it
  }  // end( C05FunctionModLinRngd )

 // C05FunctionModLinSbst- - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // same comments and IMPORTANT NOTESs as for C05FunctionModLinRngd, except
 // of course there is a Subset rather than a Range
 if( const auto tmod = dynamic_cast< const C05FunctionModLinSbst * >( mod )
     ) {
  // the corresponding entry of all the linearizations changes

  // search for the Lagrangian term which has changed
  auto it = std::find_if( LagPairs.begin() , LagPairs.end() ,
			  [ lf ]( auto & p ) { return( p.second == lf ); } );
  #ifndef NDEBUG
   if( it == LagPairs.end() )
    throw( std::logic_error( "LagBFunction::add_Modification: Lagrangian "
			     "term not found" ) );
  #endif

  Index i = std::distance( LagPairs.begin() , it );
  const auto & rc = lf->get_v_var();
  auto dit = tmod->delta().begin();

  // for all the coefficients a_{ij} in A_j that have changed
  for( auto h : tmod->subset() ) {
   if( PushCostToOwner )
    updatestuff( rc[ h ].first );

   auto j = CMb_f->is_active( rc[ h ].first ); // find x_j

   // find the place of < y_i , a_{ij} > in A_j (has to be there)
   auto ajit = std::lower_bound( CMb[ j ].second.begin() ,
				 CMb[ j ].second.end() ,
				 mon_pair( i , 0 ) ,
				 []( const auto & a , const auto & b ) {
				  return( a.first < b.first ); } );
   #ifndef NDEBUG
    if( ajit == CMb[ j ].second.end() )
     throw( std::logic_error( "LagBFunction::add_Modification: "
			      "inconsistent CostMatrix" ) );
   #endif

   ajit->second += *( dit++ );  // update a_{ij}

   }  // end( for( all the changed a_{ij} ) )

  f_dirty_Lc = true;  // Lagrangian costs will have to be recomputed
  f_Lc = -1;          // the Lipschitz constant must be computed

  // issue a C05FunctionModRngd (yes, it is Rngd, even if the originating
  // C05FunctionModLin was a Sbst one) saying that the entry i of all
  // the linearizations in the global pool has changed (the value of
  // the function has changed unpredictably, i.e., shift() == NaN)
  if( f_Observer )
   f_Observer->add_Modification( std::make_shared< C05FunctionModRngd >(
				this , C05FunctionMod::AllEntriesChanged ,
				Vec_p_Var( { it->first } ) ,
				Range( i , i + 1 ) , Subset() , NaN , true ) ,
				 chnl );
  return( 0 );  // all done
  
  }  // end( C05FunctionModLinSbst )

 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // generic FunctionMod: the only remaining FunctionMod is the C05FunctionMod
 // with type() == NothingChanged corresponding to the change of the constant
 // term. that is, the constant term b_i of the LinearFunction g_i( x ) =
 // A_i x + b_i has changed to b'_i. hence, the i-th entry of all
 // linearizations changes by shift() == b'_i - b_i, which is the perfect
 // case for a C05FunctionModLinRngd with range() == ( i , i + 1 ) and
 // delta() == { shift() }

 if( const auto tmod = dynamic_cast< const FunctionMod * >( mod ) ) {
  // since b_i has changed, b may no longer be all-0 if it previously was,
  // and the linear term has to be recomputed (or b == 0 checked first)
  f_yb = f_yb == -INF ? INF : NaN;
  f_Lc = -1;  // the Lipschitz constant must be computed
  // in fact there could be better ways to react to this if one were to
  // keep more disaggregated information about the Lipschitz constant, but
  // this does not look to be a common occurrence so we don't bother yet

  if( f_Observer ) {
   // search for the Lagrangian term which has changed
   auto it = std::find_if( LagPairs.begin() , LagPairs.end() ,
			   [ lf ]( auto & p ) { return( p.second == lf );
			   } );
   #ifndef NDEBUG
    if( it == LagPairs.end() )
     throw( std::logic_error( "LagBFunction::add_Modification: Lagrangian "
			      "term not found" ) );
   #endif

   Index i = std::distance( LagPairs.begin() , it );
   f_Observer->add_Modification( std::make_shared< C05FunctionModLinRngd >(
			    this , Vec_FunctionValue( { tmod->shift() } ) ,
			    Vec_p_Var( { it->first } ) ,
			    Range( i , i + 1 ) , NaN , true ) ,
				 chnl );
   }

  return( 0 );

  }  // end( FunctionMod )

 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // FunctionModVars: some Variable have been added/removed from a Function
 // C05FunctionModVars can have a special treatment, and therefore need be
 // checked before FunctionModVars (because C05FunctionModVars is a
 // FunctionModVars)
 //
 // There are three types of C05FunctionModVars, according to if the
 // Variable are added or deleted, and in the latter case if what is
 // deleted is a Range or a Subset. Hence, three similar pieces of code
 // follow, two almost being identical.
 //
 // IMPORTANT NOTE: see IMPORTANT NOTE for the C05FunctionModLin, which
 //                 apply verbatim here as well
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // C05FunctionModVarsAddd - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // this is: [Col]Variable are being added
 if( const auto tmod = dynamic_cast< const C05FunctionModVarsAddd * >( mod )
     ) {
  // add the corresponding terms to CostMatrix

  // search for the Lagrangian term which has changed
  auto it = std::find_if( LagPairs.begin() , LagPairs.end() ,
			  [ lf ]( auto & p ) { return( p.second == lf ); } );
  #ifndef NDEBUG
   if( it == LagPairs.end() )
    throw( std::logic_error( "LagBFunction::add_Modification: Lagrangian "
			     "term not found" ) );
  #endif

  Index i = std::distance( LagPairs.begin() , it );
  mod_CostMatrix( i , tmod->first() );

  // issue a C05FunctionModRngd saying that the entry i of all
  // the linearizations in the global pool has changed (the value of
  // the function has changed unpredictably, i.e., shift() == NaN)
  if( f_Observer )
   f_Observer->add_Modification( std::make_shared< C05FunctionModRngd >(
			      this , C05FunctionMod::AllEntriesChanged ,
			      Vec_p_Var( { it->first } ) ,
			      Range( i , i + 1 ) , Subset() , NaN , true ) ,
				 chnl );

  f_Lc = -1;    // the Lipschitz constant must be computed
  return( 0 );  // all done

  // note: since we do know this is a C05FunctionModVarsAddd we should now
  //       avoid checking for all clearly incompatible types like
  //       C05FunctionModVarsRngd and C05FunctionModVarsSbst, but this would
  //       mess up too much with the code flow, so the hell with it
  }  // end( C05FunctionModVarsAddd )

 // C05FunctionModVarsRngd - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // this is: a Range of [Col]Variable are being removed
 if( const auto tmod = dynamic_cast< const C05FunctionModVarsRngd * >( mod )
     ) {
  // remove the corresponding terms from CostMatrix

  // search for the Lagrangian term which has changed
  auto it = std::find_if( LagPairs.begin() , LagPairs.end() ,
			  [ lf ]( auto & p ) { return( p.second == lf ); } );
  #ifndef NDEBUG
   if( it == LagPairs.end() )
    throw( std::logic_error( "Lagrangian term not found" ) );
  #endif

  Index i = std::distance( LagPairs.begin() , it );

   // for all the Variable that have been eliminated
   for( auto xj : tmod->vars() ) {
    if( PushCostToOwner )
     updatestuff( xj );

     auto j = CMb_f->is_active( xj );
     if( j >= nv ) {
      // the deleted variable is not in obj yet, but it may be in v_tmpCP
      // waiting to be added to obj
      Index h;
      if( v_Obj.size() == 1 )
       h = 0;
      else {
       auto itb = Block2Idx.find( xj->get_Block() );
       if( ( itb == Block2Idx.end() ) || ( itb->second >= v_Obj.size() ) )
	throw( std::logic_error( "LagBFunction::add_Modification: deleted "
				 "variable not found in Block2Idx" ) );
       h = itb->second;
       }

      auto tCPit = std::find_if( v_tmpCP[ h ].begin() , v_tmpCP[ h ].end() ,
				 [ & xj ]( const auto & p ) {
				  return( p.first == xj ); } );
      if( tCPit == v_tmpCP[ h ].end() )
       throw( std::logic_error( "LagBFunction::add_Modification: deleted "
				"variable not found" ) );
      j = nv + std::distance( v_tmpCP[ h ].begin() , tCPit );
      }

     auto ajit = std::lower_bound( CMb[ j ].second.begin() ,
				   CMb[ j ].second.end() ,
				   mon_pair( i , 0 ) ,
				   []( const auto & a , const auto & b ) {
				    return( a.first < b.first ); } );
     #ifndef NDEBUG
      if( ajit == CMb[ j ].second.end() )
       throw( std::logic_error( "LagBFunction::add_Modification: a_{ij} "
				"term not found in CostMatrix" ) );
     #endif

     // remove < y_i , a_{ij} > from A_j
     CMb[ j ].second.erase( ajit );

     // if this leaves the term empty and the term actually was of some
     // variable that still had to be added to obj, just don't do that:
     // rather, erase the row of CostMatrix and the corresponding one in
     // v_tmpCP
     if( CMb[ j ].second.empty() && ( j >= nv ) ) {
      CMb.erase( CMb.begin() + j );
      v_tmpCP[ b ].erase( v_tmpCP[ b ].begin() + ( j - nv ) );
      }
     }

    // issue a C05FunctionModRngd saying that the entry i of all
    // the linearizations in the global pool has changed (the value of
    // the function has changed unpredictably, i.e., shift() == NaN)
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< C05FunctionModRngd >(
			       this , C05FunctionMod::AllEntriesChanged ,
			       Vec_p_Var( { it->first } ) ,
			       Range( i , i + 1 ) , Subset() , NaN , true ) ,
				   chnl );

    f_Lc = -1;    // the Lipschitz constant must be computed
    return( 0 );  // all done

  // note: we should now avoid checking for C05FunctionModVarsSbst, but
  // the hell with it
  }  // end( C05FunctionModVarsRngd )

 // C05FunctionModVarsSbst - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // this is: a Subset of [Col]Variable are being removed
 if( const auto tmod = dynamic_cast< const C05FunctionModVarsSbst * >( mod )
     ) {
  // remove the corresponding terms from CostMatrix

  // search for the Lagrangian term which has changed
  auto it = std::find_if( LagPairs.begin() , LagPairs.end() ,
			  [ lf ]( auto & p ) { return( p.second == lf ); } );
  #ifndef NDEBUG
   if( it == LagPairs.end() )
    throw( std::logic_error( "Lagrangian term not found" ) );
  #endif

  Index i = std::distance( LagPairs.begin() , it );

  // for all the Variable that have been eliminated
  for( auto xj : tmod->vars() ) {
   if( PushCostToOwner )
     updatestuff( xj );

   auto j = CMb_f->is_active( xj );
   if( j >= nv ) {
    // the deleted variable is not in obj yet, but it may be in v_tmpCP[h]
    Index h;
    if( v_Obj.size() == 1 )
     h = 0;
    else {
     auto itb = Block2Idx.find( xj->get_Block() );
     if( ( itb == Block2Idx.end() ) || ( itb->second >= v_Obj.size() ) )
      throw( std::logic_error( "LagBFunction::add_Modification: deleted "
			       "variable not found in Block2Idx" ) );
     h = itb->second;
     }

    auto tCPit = std::find_if( v_tmpCP[ h ].begin() , v_tmpCP[ h ].end() ,
			       [ & xj ]( const auto & p ) {
				return( p.first == xj ); } );
    if( tCPit == v_tmpCP[ h ].end() )
     throw( std::logic_error( "LagBFunction::add_Modification: deleted "
			      "variable not found" ) );
    j = nv + std::distance( v_tmpCP[ h ].begin() , tCPit );
    }

   auto ajit = std::lower_bound( CMb[ j ].second.begin() ,
				 CMb[ j ].second.end() ,
				 std::make_pair( i , 0 ) ,
				 []( const auto & a , const auto & b ) {
				  return( a.first < b.first ); } );
   #ifndef NDEBUG
    if( ajit == CMb[ j ].second.end() )
     throw( std::logic_error( "LagBFunction::add_Modification: a_{ij} "
			      "term not found in CostMatrix" ) );
   #endif

   // remove < y_i , a_{ij} > from A_j
   CMb[ j ].second.erase( ajit );

   // if this leaves the term empty and the term actually was of some
   // variable that still had to be added to obj, just don't do that:
   // rather, erase the row of CostMatrix and the corresponding one in
   // v_tmpCP
   if( CMb[ j ].second.empty() && ( j >= nv ) ) {
    CMb.erase( CMb.begin() + j );
    v_tmpCP[ b ].erase( v_tmpCP[ b ].begin() + ( j - nv ) );
    }
   }

  // issue a C05FunctionModRngd saying that the entry i of all
  // the linearizations in the global pool has changed (the value of
  // the function has changed unpredictably, i.e., shift() == NaN)
  if( f_Observer )
   f_Observer->add_Modification( std::make_shared< C05FunctionModRngd >(
			       this , C05FunctionMod::AllEntriesChanged ,
			       Vec_p_Var( { it->first } ) ,
			       Range( i , i + 1 ) , Subset() , NaN , true ) ,
				 chnl );

  f_Lc = -1;    // the Lipschitz constant must be computed
  return( 0 );  // all done

  }  // end( C05FunctionModVarsSbst )

 // this should never happen, but just in case ...
 return( 0 );  // all done

 }  // end( LagBFunction::guts_of_this_add_Modification )

/*--------------------------------------------------------------------------*/

void LagBFunction::update_CostMatrix_ModLinRngd( const v_coeff_pair & rc ,
                                                 c_Vec_p_Var & vars ,
                                                 c_Range & rng )
{
 // note that one may think to update the corresponding Range of original
 // costs by adding delta(), but this would be wrong because delta() is
 // computed w.r.t. the last value of the costs, which is in general a
 // Lagrangian one. one may alternatively think to just mark the cost as in
 // need for refreshing and do it later (say, in compute()), but this would
 // be wrong as well because the costs that have *not* been changed are the
 // Lagrangian ones, and one would end up "fixing" them as the true costs.
 // hence, the only option is to read the costs now (since the Modification
 // is the only place where the information about which costs have actually
 // changed is), although reading the costs from obj is nontrivial since the
 // Range may not be current if Variable have been added/deleted

 f_dirty_Lc = true;  // Lagrangian costs will have to be recomputed

 if( vars.empty() || ( rng.second <= rng.first ) )
  return;

 Index b;
 if( v_Obj.size() == 1 )
  b = 0;
 else {
  auto it = Block2Idx.find( vars.front()->get_Block() );
  if( ( it == Block2Idx.end() ) || ( it->second >= v_Obj.size() ) )
   throw( std::logic_error( "Variable not found in any objective" ) );
  b = it->second;
  }

 m_column & CM = CostMatrix[ b ];

 // first check if by chance the Range is still current
 bool current = true;
 auto it = vars.begin();
 for( Index j = rng.first ; j < rng.second ; ++j )
  if( rc[ j ].first != *( it++ ) ) { current = false; break; }

 if( current ) {  // if Range is still current, it's easy
  std::vector< std::pair< Index , double > > jdeltas;  // EAGER: (j, Delta_c_j)
  for( Index j = rng.first ; j < rng.second ; ++j ) {
   if( ! f_lazy_eval ) {
    double d = rc[ j ].second - CM[ j ].first;
    if( d != 0 )
     jdeltas.emplace_back( j , d );
    }
   CM[ j ].first = rc[ j ].second;
   }
  eager_pool_cost_delta( rc , jdeltas );
  return;
  }

 // if Range is no longer current, it's complicated
 // however, note that CostMatrix is still "aligned" with the indices
 // found in the Modification, since if Variable have been added/removed
 // in obj by changes occurring prior to this the corresponding
 // Modification have been already "seen" before this and acted upon
 it = vars.begin();
 for( Index j = rng.first ; j < rng.second ; ++j ) {
  auto vi = *( it++ );          // the Variable whose cost has changed
  if( rc[ j ].first == vi )     // if vi is still in the same place
   CM[ j ].first = rc[ j ].second;  // still easy
  else {                        // if not, look up for it
   // start looking up before j, since it's where it most likely is
   Index h = j;
   while( h )
    if( rc[ --h ].first == vi )
     break;

   if( rc[ h ].first == vi )    // if it is found
    CM[ j ].first = rc[ h ].second;  // all done
   else {                       // otherwise
    // look up after j, it still could be there
    for( h = j + 1 ; h < rc.size() ; ++h )
     if( rc[ h ].first == vi )
      break;

    if( h < rc.size() )         // if it is found
     CM[ j ].first = rc[ h ].second;  // all done
    // otherwise, do nothing: the changed Variable is no longer in obj
    // at this point, which means that the corresponding Modification
    // is waiting in the queue to be discovered and acted upon
    }
   }
  }
 }  // end( LagBFunction::update_CostMatrix_ModLinRngd )

/*--------------------------------------------------------------------------*/

void LagBFunction::update_CostMatrix_ModLinSbst( const v_coeff_pair & rc ,
                                                 c_Vec_p_Var & vars ,
                                                 c_Subset & sbst )
{
 // note that one may think to update the corresponding Subset of original
 // costs by adding delta(), but this would be wrong because delta() is
 // computed w.r.t. the last value of the costs, which is in general a
 // Lagrangian one. one may alternatively think to just mark the cost as in
 // need for refreshing and do it later (say, in compute()), but this would
 // be wrong as well because the costs that have *not* been changed are the
 // Lagrangian ones, and one would end up "fixing" them as the true costs.
 // hence, the only option is to read the costs now (since the Modification
 // is the only place where the information about which costs have actually
 // changed is), although reading the costs from obj is nontrivial since the
 // Subset may not be current if Variable have been added/deleted

 f_dirty_Lc = true;  // Lagrangian costs will have to be recomputed

 if( vars.empty() || sbst.empty() )
  return;

 Index b;
 if( v_Obj.size() == 1 )
  b = 0;
 else {
  auto it = Block2Idx.find( vars.front()->get_Block() );
  if( ( it == Block2Idx.end() ) || ( it->second >= v_Obj.size() ) )
   throw( std::logic_error( "Variable not found in any objective" ) );
  b = it->second;
  }

 m_column & CM = CostMatrix[ b ];

 // first check if by chance the Subset is still current
 bool current = true;
 auto it = vars.begin();
 for( Index j : sbst )
  if( rc[ j ].first != *( it++ ) ) { current = false; break; }

 if( current ) {  // if Subset is still current, it's easy
  std::vector< std::pair< Index , double > > jdeltas;  // EAGER: (j, Delta_c_j)
  for( Index j : sbst ) {
   if( ! f_lazy_eval ) {
    double d = rc[ j ].second - CM[ j ].first;
    if( d != 0 )
     jdeltas.emplace_back( j , d );
    }
   CM[ j ].first = rc[ j ].second;
   }
  eager_pool_cost_delta( rc , jdeltas );
  return;
  }

 // if Subset is no longer current, it's complicated
 // however, note that CostMatrix is still "aligned" with the indices found
 // in the Modification, since if Variable have been added/removed in obj by
 // changes occurring prior to this the corresponding Modification have been
 // already "seen" before this and acted upon
 it = vars.begin();
 for( Index j : sbst ) {
  auto vi = *( it++ );          // the Variable whose cost has changed
  if( rc[ j ].first == vi )   // if vi is still in the same place
   CM[ j ].first = rc[ j ].second;  // still easy
  else {                      // if not, look up for it
   // start looking up before j, since it's where it most likely is
   Index h = j;
   while( h )
    if( rc[ --h ].first == vi )
     break;

   if( rc[ h ].first == vi )  // if it is found
    CM[ j ].first = rc[ h ].second;  // all done
   else {                     // otherwise
    // look up after j, it still could be there
    for( h = j + 1 ; h < rc.size() ; ++h )
     if( rc[ h ].first == vi )
      break;

    if( h < rc.size() )       // if it is found
     CM[ j ].first = rc[ h ].second;  // all done
    // otherwise, do nothing: the changed Variable is no longer in obj
    // at this point, which means that the corresponding Modification
    // is waiting in the queue to be discovered and acted upon
    }
   }
  }
 }  // end( LagBFunction::update_CostMatrix_ModLinSbst )

/*--------------------------------------------------------------------------*/

void LagBFunction::eager_pool_cost_delta( const v_coeff_pair & rc ,
		 const std::vector< std::pair< Index , double > > & jdeltas )
{
 if( f_lazy_eval || NoSol || jdeltas.empty() )
  return;

 // for each stored linearization, write its Solution into the inner Block so
 // that the changed Variables hold x*_k, then add < Delta_c , x*_k > to the
 // (full epigraphic) constant kept in value. delta_na, being
 // cost-independent, is left untouched. The write is done at this
 // Modification-handling point (not at query time, which would disturb the
 // Block state and the cost propagation -- the very bug eager fixes).
 for( Index k = 0 ; k < f_max_glob ; ++k ) {
  if( ! g_pool[ k ].sol )
   continue;
  g_pool[ k ].sol->write( v_Block.front() );
  double dv = 0;
  for( const auto & jd : jdeltas )
   dv += jd.second * rc[ jd.first ].first->get_value();
  g_pool[ k ].value += dv;
  }

 // the inner Block now holds the last pool Solution; mark that no pool entry
 // is "the current one" so later queries take the eager fast path
 LastSolution = g_pool.size();

 }  // end( LagBFunction::eager_pool_cost_delta )

/*--------------------------------------------------------------------------*/

void LagBFunction::update_CostMatrix_ModVarsAddd( c_Vec_p_Var & vars ,
                                                  Index first )
{
 f_active_dirty = true;  // variable structure changes -> v_active must rebuild

 // update CostMatrix for the addition of new variables. note that the new
 // rows are empty, because if a new term is added, it means it was not
 // there before
 // IN FACT WE ARE SHIFTING ON WHO IS DOING THE BURDEN OF NOT
 // INTERFERING WITH THE "AUTOMATIC" ADDITION OF Variable TO obj

 if( vars.empty() )
  return;

 Index b;
 if( v_Obj.size() == 1 )
  b = 0;
 else {
  auto it = Block2Idx.find( vars.front()->get_Block() );
  if( ( it == Block2Idx.end() ) || ( it->second >= v_Obj.size() ) )
   throw( std::logic_error( "Variable not found in any objective" ) );
  b = it->second;
  }

 m_column & CM = CostMatrix[ b ];

 if( v_tmpCP.empty() || v_tmpCP[ b ].empty() ) {
  // there are no variables to be "stealthily" added to obj, hence
  // CostMatrix.size() == [q]obj->gen_num_active_var()

  #ifndef NDEBUG
   if( first != CM.size() )
    throw( std::logic_error( "inconsistent CostMatrix[" +
			     std::to_string( b ) + "]" ) );
  #endif

  CM.resize( CM.size() + vars.size() );
  }
 else {
  // some variables must be "stealthily" added to obj: thus, the entries
  // of CostMatrix have not to be added at the end, and we have to check
  // that the thusly added variables are not among these, because in case
  // the list of variables to be added has to be changed, and CostMatrix
  // has to be changed accordingly

  c_Index nv = vars.size();
  CM.insert( CM.begin() + first , nv , col_pair() );

  auto & tmpCP = v_tmpCP[ b ];
  for( Index i = 0 ; i < tmpCP.size() ; ++i )
   for( Index j = 0 ; j < nv ; ++j )
    if( vars[ j ] == tmpCP[ i ].first ) {
     tmpCP.erase( tmpCP.begin() + i );
     CM[ first + j ] = std::move( CM[ first + nv + j ] );
     CM.erase( CM.begin() + first + nv + i );
     break;
    }
 }
}  // end( LagBFunction::update_CostMatrix_ModVarsAddd )

/*--------------------------------------------------------------------------*/

void LagBFunction::update_CostMatrix_ModVarsRngd( c_Vec_p_Var & vars ,
                                                  c_Range & rng )
{
 f_active_dirty = true;  // variable structure changes -> v_active must rebuild

 // remove the range of rows from CostMatrix corresponding to receiving a
 // C05FunctionModVarsRngd; however, if the Lagrangian term y A^j in a
 // removed CostMatrix entry is not empty, then the corresponding variable
 // x_j is immediately re-added, with 0 coefficient, at the back of the
 // objective of the inner Block, and therefore the corresponding row of
 // CostMatrix shares the same fate
 //
 // note that there is no need to check if the variables being deleted are
 // those in the part of CostMatrix holding information about the variables
 // still to be added, because if they are still to be added they cannot
 // have been deleted

 if( vars.empty() || ( rng.second <= rng.first ) )
  return;

 Index b;
 if( v_Obj.size() == 1 )
  b = 0;
 else {
  auto it = Block2Idx.find( vars.front()->get_Block() );
  if( ( it == Block2Idx.end() ) || ( it->second >= v_Obj.size() ) )
   throw( std::logic_error( "Variable not found in any objective" ) );
  b = it->second;
 }

 m_column & CM = CostMatrix[ b ];

 #ifndef NDEBUG
  if( rng.second > CM.size() )
   throw( std::logic_error( "inconsistent CostMatrix" ) );
 #endif

 m_column tempCM;     // CostMatrix elements to be re-added

 // check if are nonempty elements of CostMatrix are being deleted, if so
 // save the corresponding information to sneakily add them back
 auto strtit = CM.begin() + rng.first;
 auto stpit = CM.begin() + rng.second;
 for( auto it = strtit ; it != stpit ; ++it )
  if( ! it->second.empty() ) {
   tempCM.push_back( std::move( *it ) );
   tempCM.back().first = 0;
   v_tmpCP[ b ].push_back( coeff_pair(
     static_cast< ColVariable * >( vars[ std::distance( strtit , it ) ] ) ,
     Coefficient( 0 ) ) );
  }

 CM.erase( strtit , stpit );

 if( ! tempCM.empty() )  // some nonempty elements have been deleted
                         // add them back at the end
  CM.insert( CM.end() ,
             std::make_move_iterator( tempCM.begin() ),
             std::make_move_iterator( tempCM.end() ) );

}  // end( LagBFunction::update_CostMatrix_ModVarsRngd )

/*--------------------------------------------------------------------------*/

void LagBFunction::update_CostMatrix_ModVarsSbst( c_Vec_p_Var & vars ,
                                                  c_Subset & sbst )
{
 f_active_dirty = true;  // variable structure changes -> v_active must rebuild

 // remove the range of rows from CostMatrix corresponding to receiving a
 // C05FunctionModVarsSbst; however, if the Lagrangian term y A^j in a
 // removed CostMatrix entry is not empty, then the corresponding variable
 // x_j is immediately re-added, with 0 coefficient, at the back of the
 // objective of the inner Block, and therefore the corresponding row of
 // CostMatrix shares the same fate
 //
 // note that there is no need to check if the variables being deleted are
 // those in the part of CostMatrix holding information about the variables
 // still to be added, because if they are still to be added they cannot
 // have been deleted

 if( vars.empty() || sbst.empty() )
  return;

 Index b;
 if( v_Obj.size() == 1 )
  b = 0;
 else {
  auto it = Block2Idx.find( vars.front()->get_Block() );
  if( ( it == Block2Idx.end() ) || ( it->second >= v_Obj.size() ) )
   throw( std::logic_error( "Variable not found in any objective" ) );
  b = it->second;
 }

 m_column & CM = CostMatrix[ b ];

 #ifndef NDEBUG
  if( sbst.back() >= CM.size() )
   throw( std::logic_error( "inconsistent CostMatrix" ) );
 #endif

 m_column tempCM;     // CostMatrix elements to be re-added

 // check if are nonempty elements of CostMatrix are being deleted, if so
 // save the corresponding information to sneakily add them back
 for( Index i = 0 ; i < sbst.size() ; ++i )
  if( ! CM[ sbst[ i ] ].second.empty() ) {
   tempCM.push_back( std::move( CM[ sbst[ i ] ] ) );
   tempCM.back().first = 0;
   v_tmpCP[ b ].push_back( coeff_pair(
     static_cast< ColVariable * >( vars[ i ] ) , Coefficient( 0 ) ) );
  }

 Compact( CM , sbst );

 if( ! tempCM.empty() )  // some nonempty elements have been deleted
                         // add them back at the end
  CM.insert( CM.end() ,
             std::make_move_iterator( tempCM.begin() ),
             std::make_move_iterator( tempCM.end() ) );

}  // end( LagBFunction::update_CostMatrix_ModVarsSbst )

/*--------------------------------------------------------------------------*/

void LagBFunction::set_default_inner_BlockConfig( void )
{
 if( auto ib = get_inner_block() ) {
  auto config = new OCRBlockConfig( ib );
  config->clear();
  config->apply( ib );
  }
 }

/*--------------------------------------------------------------------------*/

void LagBFunction::set_default_inner_BlockSolverConfig( void )
{
 if( auto ib = get_inner_block() ) {
  auto solver_config = new RBlockSolverConfig( ib );
  solver_config->clear();
  solver_config->apply( ib );
  }
 }

/*--------------------------------------------------------------------------*/

void LagBFunction::compute_Lipschitz_constant( void )
{
 auto ib = get_inner_block();
 if( ! ib ) {                     // if there is no inner Block
  f_Lc = Inf< FunctionValue >();  // there is no finite constant
  return;
  }

 if( ! f_BS )                     // if the BoxSolver is not there yet
  f_BS = new BoxSolver;           // do it now

 ib->register_Solver( f_BS );     // register the BoxSolver to the inner Block

 // now start a loop: for each LinearFunction in the vector of Lagrangian
 // pairs compute (an upper/lower estimate of) the maximum and minimum of
 // its value on the feasible region of the inner Block, take the max of
 // the absolute values of the two, and take the sum of the squares of
 // these: this is an upper estimate of the (square of) the Lipschitz constant
 f_Lc = 0;
 for( auto el : LagPairs ) {
  f_BS->set_Objective_Function( el.second );
  f_BS->compute();
  auto vv = std::abs( f_BS->get_var_value() );
  if( vv == Inf< FunctionValue >() ) {
   f_Lc = Inf< FunctionValue >();
   break;
   }
  auto ivv = std::abs( f_BS->get_opposite_value() );
  if( vv == Inf< FunctionValue >() ) {
   f_Lc = Inf< FunctionValue >();
   break;
   }
  auto mx = std::max( vv , ivv );
  f_Lc += mx * mx;
  }

 if( f_Lc < Inf< FunctionValue >() )
  f_Lc = std::sqrt( f_Lc );          // the true Lipschitz constant

 // unregister the BoxSolver from the inner Block
 ib->unregister_Solver( f_BS );

 }  // end( LagBFunction::compute_Lipschitz_constant )

/*--------------------------------------------------------------------------*/

template< typename par_type >
void LagBFunction::add_par( std::string && name , par_type && value )
{
 if( f_CC->set_par( std::move( name ) , std::move( value ) ) )
  f_CC_changed = true;
 }

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS LagBFunctionState -------------------------*/
/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunctionState::deserialize( const netCDF::NcGroup & group )
{
 auto gs = group.getDim( "LagBFunction_MaxGlob" );
 f_max_glob = gs.isNull() ? 0 : gs.getSize();

 g_pool.resize( f_max_glob );

 if( f_max_glob ) {
  auto nct = group.getVar( "LagBFunction_Type" );
  if( nct.isNull() )
   throw( std::logic_error( "LagBFunction_Type not found" ) );

  // eager/lazy constant and convexified flag; both optional for backward
  // compatibility with States written before they were serialized (default
  // value = 0, convexified = false, matching the old behaviour)
  auto ncv = group.getVar( "LagBFunction_Value" );
  auto ncc = group.getVar( "LagBFunction_Convexified" );

  for( Index i = 0 ; i < f_max_glob ; ++i ) {
   int ti;
   nct.getVar( { i } , &ti );
   g_pool[ i ].varsol = ( ti != 0 );

   if( ! ncv.isNull() )
    ncv.getVar( { i } , &( g_pool[ i ].value ) );
   else
    g_pool[ i ].value = 0;

   if( ! ncc.isNull() ) {
    int ci;
    ncc.getVar( { i } , &ci );
    g_pool[ i ].convexified = ( ci != 0 );
    }
   else
    g_pool[ i ].convexified = false;

   g_pool[ i ].conv_active.clear();  // cache, not serialized -> rebuilt lazily

   auto gi = group.getGroup( "LagBFunction_Sol_" + std::to_string( i ) );
   if( gi.isNull() )
    g_pool[ i ].sol = nullptr;
   else
    g_pool[ i ].sol = Solution::new_Solution( gi );
   }
  }

 auto nic = group.getDim( "LagBFunction_ImpCoeffNum" );
 if( ( ! nic.isNull() ) && ( nic.getSize() ) ) {
  zLC.resize( nic.getSize() );

  auto ncCI = group.getVar( "LagBFunction_ImpCoeffInd" );
  if( ncCI.isNull() )
   throw( std::logic_error( "LagBFunction_ImpCoeffInd not found" ) );

  auto ncCV = group.getVar( "LagBFunction_ImpCoeffVal" );
  if( ncCV.isNull() )
   throw( std::logic_error( "LagBFunction_ImpCoeffVal not found" ) );

  for( LagBFunction::Index i = 0 ; i < zLC.size() ; ++i ) {
   ncCI.getVar( { i } , &(zLC[ i ].first) );
   ncCV.getVar( { i } , &(zLC[ i ].second) );
   }
  }
 else
  zLC.clear();
 
 }  // end( LagBFunctionState::deserialize )

/*--------------------------------------------------------------------------*/

void LagBFunctionState::serialize( netCDF::NcGroup & group ) const
{
 // always call the method of the base class first
 State::serialize( group );

 if( f_max_glob ) {
  netCDF::NcDim gs = group.addDim( "LagBFunction_MaxGlob" , f_max_glob );

  std::vector< int > typ( f_max_glob );
  for( Index i = 0 ; i < f_max_glob ; ++i )
   typ[ i ] = g_pool[ i ].varsol ? 1 : 0;
 
  ( group.addVar( "LagBFunction_Type" , netCDF::NcByte() , gs ) ).putVar(
				      { 0 } , {  f_max_glob } , typ.data() );

  // the eager/lazy linearization constant (gpool_el::value) and the
  // convexified flag, indexed over LagBFunction_MaxGlob (conv_active is a
  // cache, not saved). Empty slots (no Solution) are written as 0/false:
  // their value/convexified are meaningless (never read while sol == nullptr)
  // and may carry stale data left by delete_linearization, so canonicalising
  // them keeps the round-trip exact.
  std::vector< double > val( f_max_glob );
  std::vector< int > cvx( f_max_glob );
  for( Index i = 0 ; i < f_max_glob ; ++i ) {
   val[ i ] = g_pool[ i ].sol ? g_pool[ i ].value : 0;
   cvx[ i ] = ( g_pool[ i ].sol && g_pool[ i ].convexified ) ? 1 : 0;
   }
  ( group.addVar( "LagBFunction_Value" , netCDF::NcDouble() , gs ) ).putVar(
				      { 0 } , { f_max_glob } , val.data() );
  ( group.addVar( "LagBFunction_Convexified" , netCDF::NcByte() , gs ) ).putVar(
				      { 0 } , { f_max_glob } , cvx.data() );

  for( Index i = 0 ; i < f_max_glob ; ++i ) {
   if( ! g_pool[ i ].sol )
    continue;

   auto gi = group.addGroup( "LagBFunction_Sol_" + std::to_string( i ) );
   g_pool[ i ].sol->serialize( gi );
   }
  }

 if( ! zLC.empty() ) {
  netCDF::NcDim cn = group.addDim( "LagBFunction_ImpCoeffNum" , zLC.size() );
  
  auto ncCI = group.addVar( "LagBFunction_ImpCoeffInd" , netCDF::NcInt() ,
			    cn );

  auto ncCV = group.addVar( "LagBFunction_ImpCoeffVal" , netCDF::NcDouble() ,
			    cn );
 
  for( Index i = 0 ; i < zLC.size() ; ++i ) {
   ncCI.putVar( { i } , zLC[ i ].first );
   ncCV.putVar( { i } , zLC[ i ].second );
   }
  }
 }  // end( LagBFunctionState::serialize )

/*--------------------------------------------------------------------------*/
/*---------------------- End File LagBFunction.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
