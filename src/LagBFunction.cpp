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
   ChkState( false ) , PushCostToOwner( true ) , f_max_glob( 0 ) ,
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
   delete g_pool[ i ].first;
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
   if( ! innerblock ) {          // was a cleanup
    guts_of_destructor( false ); // all done
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
  throw( std::invalid_argument( "inner Block Objective not a FRealObjective" ) );

 IsConvex = ( frobj->get_sense() == Objective::eMax );

 if( PushCostToOwner )
 {
  // Clear data structures (in case of reuse)
  v_Obj.clear();
  v_ObjIsQuad.clear();
  v_BlockBFS.clear();
  Block2Idx.clear();
  CostMatrix.clear();

  // BFS traversal of Block tree starting from innerblock
  std::queue< Block * > q;
  q.push( innerblock );

  while( ! q.empty() )
  {
   Block * curr = q.front(); q.pop();

   Index h = v_BlockBFS.size();
   v_BlockBFS.push_back( curr );
   Block2Idx[ curr ] = h;

   FRealObjective * obj = static_cast< FRealObjective * >( curr->get_objective() );
   v_Obj.push_back( obj );

   bool isQuad = dynamic_cast< const p_QF >( obj->get_function() ) != nullptr;
   v_ObjIsQuad.push_back( isQuad );

   // Init CostMatrix for each Block's objective
   if( ! isQuad )
   {
    const auto & rp = static_cast< p_LF >( obj->get_function() )->get_v_var();
    m_column cm( rp.size() );
    for( Index i = 0 ; i < rp.size() ; ++i )
     cm[ i ].first = rp[ i ].second;
    CostMatrix.emplace_back( std::move( cm ) );
   }
   else
   {
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
 else
 {
  // if we are not pushing the cost to the owner, build a single CostMatrix
  CostMatrix.clear();  // ensure empty

  m_column cm;
  auto * fn = frobj->get_function();

  if( auto * lf = dynamic_cast< p_LF >( fn ) )
  {
   const auto & rp = lf->get_v_var();
   cm.resize( rp.size() );
   for( Index i = 0 ; i < rp.size() ; ++i )
    cm[ i ].first = rp[ i ].second;
  }
  else if( auto * qf = dynamic_cast< p_QF >( fn ) )
  {
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
     if( ! scfg->f_diff )  // if not in differential mode
      set_default_inner_BlockSolverConfig();  // reset the BlockSolverConfig

    if( scpp->f_value.second ) {
     BC = dynamic_cast< BlockConfig * >( scpp->f_value.second );
     if( ! BC ) throw( std::invalid_argument(
     "LagBFunction::set_ComputeConfig: invalid extra_Configuration.second" ) );
     }
    else
     if( ! scfg->f_diff )  // if not in differential mode
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
    f_BSC = BSC->clone();              // keep a copy of the new one
    f_BSC->clear();                    // but clear it
    }
   }
  else {  // scfg->f_extra_Configuration is nullptr
   if( ! scfg->f_diff )  // if not in differential mode
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
      delete it->first;

    if( f_max_glob > Index( value ) ) {
     f_max_glob = value;
     update_f_max_glob();
     }    
    }
   g_pool.resize( value , gpool_el( nullptr , true ) );
   break;
  case( intInnrSlvr ):  // intInnrSlvr - - - - - - - - - - - - - - - - - - -
   if( InnrSlvr != Index( value ) ) {
    InnrSlvr = Index( value );
    // ensure there is a ComputeConfig in diff mode ready
    while( f_BSC->num_ComputeConfig() <= InnrSlvr ) {
     auto cc = new ComputeConfig;
     cc->f_diff = true;
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
     if( g_pool[ i ].first ) {
      delete g_pool[ i ].first;
      // any surely nonzero address
      g_pool[ i ].first = reinterpret_cast < Solution * >( this );
      }
    break;
    }
   if( ( value == 0 ) && ( NoSol == true ) ) {
    NoSol = false;
    // setting NoSol == false when it was true: has to cleanup all the
    // global pool since the information there is not reliable (no
    // Solution is a real Solution)
    for( Index i = 0 ; i < f_max_glob ; ++i )
     g_pool[ i ].first = nullptr;
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
  throw( std::invalid_argument( "LagBFunction::remove_variable: wrong index" ) );

 // update CostMatrix - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // determine which Block defines the i-th variable
 Block * defBlock = LagPairs[ i ].first->get_Block();

 // find the index of the Block in the BFS order
 auto it = Block2Idx.find( defBlock );
 if( it == Block2Idx.end() )
  throw( std::logic_error( "LagBFunction::remove_variable: Block not found in Block2Idx" ) );
 Index h = it->second;

 auto & CMh = CostMatrix[ h ];

 // find the position of the term < i , a_{ij} > in CMj
 auto itcm = std::lower_bound( CMh.begin() , CMh.end() , mon_pair( i , 0 ) ,
                               []( const auto & a , const auto & b ) {
                                return ( a.first < b.first ); } );

 if( ( itcm != CMh.end() ) && ( itcm->first == i ) ) {
  // decrease by 1 the names of all the < h , a_{hj} > with h > i
  for( auto nit = itcm + 1 ; nit != CMh.end() ; ++nit )
   --( nit->first );

  CMh.erase( itcm );  // finally erase it

  // NOTE: we do not remove the row from CostMatrix[ h ] even if it's now empty,
  // because the objective still exists and we want to keep the alignment
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
  // removing *all* variable
  if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
   // an Observer is there: copy the names of deleted Variable (all of them)
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
                             // Lagrangian costs should be the original costs, so
                             // they will have to be modified unless by chance
                             // they already are so

  f_yb = -INF;  // b is empty, hence there are no nonzeros
  return;
 }

 // this is not a complete reset
 // update CostMatrix - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // for each variable in the range to be removed
 for( Index i = range.first ; i < range.second ; ++i )
 {
  Block * defBlock = LagPairs[ i ].first->get_Block();

  auto it = Block2Idx.find( defBlock );
  if( it == Block2Idx.end() )
   throw( std::logic_error(
    "LagBFunction::remove_variables: Block not found in Block2Idx" ) );

  Index h = it->second;
  auto & CMh = CostMatrix[ h ];

  // find the position of the term < range.first , a_{*j} > in CMj
  auto iit = std::lower_bound( CMh.begin() , CMh.end() ,
                               mon_pair( i , 0 ) ,
                               []( const auto & a , const auto & b ) {
                                return ( a.first < b.first ); } );

  // find the position of the term < range.second , a_{*j} > in CMj
  auto eit = std::lower_bound( CMh.begin() , CMh.end() ,
                               mon_pair( range.second , 0 ) ,
                               []( const auto & a , const auto & b ) {
                                return ( a.first < b.first ); } );

  if( iit != eit ) {  // if any of these are found
   // decrease by range.second - range.first the names of all the
   // < h , a_{hj} > with h >= range.second
   for( auto nit = eit ; nit != CMh.end() ; ++nit )
    ( nit->first ) -= range.second - range.first;

   CMh.erase( iit , eit );   // finally, erase them all
   // NOTE: we do not remove the row from CostMatrix[ h ] even if it's now empty,
   // because the objective still exists and we want to keep the alignment
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

 // now actually eliminate the rows from LagPairs - - - - - - - - - - - - - - -
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
 if( nms.empty() ) {  // removing all Variable
  if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
   // an Observer is there: copy the names of deleted Variable (all of them)
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
                             // so they will have to be modified unless by chance
                             // they already are so
  f_yb = -INF;  // b is empty, hence there are no nonzeros
  return;
 }

 f_Lc = -1;     // the Lipschitz constant must be computed

 // this is not a complete reset
 if( ! ordered )
  std::sort( nms.begin() , nms.end() );

 if( nms.back() >= LagPairs.size() )
  throw( std::invalid_argument( "LagBFunction::remove_variables: wrong index" ) );

 // update CostMatrix - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // for each variable to remove, update its corresponding CostMatrix[ h ]
 for( Index j = 0 ; j < nms.size() ; ++j )
 {
  Index i = nms[ j ];  // index of variable to remove
  Block * defBlock = LagPairs[ i ].first->get_Block();

  auto it = Block2Idx.find( defBlock );
  if( it == Block2Idx.end() )
   throw( std::logic_error(
    "LagBFunction::remove_variables(Subset): Block not found in Block2Idx" ) );

  Index h = it->second;
  auto & CMh = CostMatrix[ h ];

  // find the position of the term < nms.front() , a_{*j} > in CMj
  auto iit = std::lower_bound( CMh.begin() , CMh.end() ,
                               mon_pair( nms.front() , 0 ) ,
                               []( const auto & a , const auto & b ) {
                                return ( a.first < b.first ); } );

  if( iit == CMh.end() )  // none of them is there
   continue;              // next

  // for each < h , a_{hj} > in CMj with h >= nms.front(); if h is in nms
  // erase the element (meaning, leave it there to be overwritten), if h
  // is not in nms then decrease it by the proper amount, which is the
  // number of elements in nms that are < h

  auto wit = iit;    // free position where to write
  Index count = 0;   // how many elements of nms have been overtaken already
  for( auto nit = nms.begin() ; ( nit != nms.end() ) &&
	                        ( iit != CMh.end() ) ; )
   if( iit->first == *nit ) {  ++iit; ++nit; ++count; }
   else
    if( iit->first > *nit ) { ++nit; ++count; }
    else { *wit = *iit; wit->first -= count; ++wit; }

  // decrease by nms.size() the names of all the < h , a_{hj} > with
  // h > nms.back()
  for( auto nit = iit ; nit != CMh.end() ; ++nit )
   ( nit->first ) -= nms.size();

  CMh.erase( wit , iit );  // now erase the deleted part
  // if this leaves the term empty and the term actually was of some variable
  // that still had to be added to obj, just don't do that: rather, erase
  // the row of CostMatrix and the corresponding one in v_tmpCP
  // [NOT APPLICABLE]: do not erase CostMatrix[ h ] entry; must remain aligned
 }

 // if b != 0 but we are eliminating nonzeros, it may have become 0 - - - - - -
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

 // now actually eliminate the rows from LagPairs - - - - - - - - - - - - - - -
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
 for( Index h = 0 ; h < CostMatrix.size() ; ++h )
 {
  const auto & CMh = CostMatrix[ h ];

  // construct the vector of original coefficients
  Vec_FunctionValue NC( CMh.size() );
  for( Index i = 0 ; i < CMh.size() ; ++i )
   NC[ i ] = CMh[ i ].first;

  // modify the objective (linear or quadratic)
  if( ! v_ObjIsQuad[ h ] )
   static_cast< p_LF >( v_Obj[ h ]->get_function() )
    ->modify_coefficients( std::move( NC ) );
  else
   static_cast< p_QF >( v_Obj[ h ]->get_function() )
    ->modify_linear_coefficients( std::move( NC ) );
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
 if( f_play_dumb && ( mod->get_Block() == v_Block.front() ) )
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
    g_pool[ i ].first = nullptr;
   f_max_glob = 0;
   }
  else {
   for( Index i = 0 ; i < f_max_glob ; ++i ) {
    if( g_pool[ i ].first ) {  // a Solution is there
     ++cnt;

     // write it in the Variable of the inner Block
     g_pool[ i ].first->write( v_Block.front() );
     LastSolution = i;  // and recall what's there

     // check it's still a feasible solution/direction
     bool feas = g_pool[ i ].second ? v_Block.front()->is_feasible()
                                    : v_Block.front()->is_unbounded();
     if( ! feas ) {              // if not
      delete g_pool[ i ].first;  // eliminate it
      g_pool[ i ].first = nullptr;
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
  // unpredictably
  // if all linearizations have been removed, then pass an empty Subset
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

 for( Index h = 0 ; h < v_Obj.size() ; ++h )
 {
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
  else if( auto * qf = dynamic_cast< p_QF >( f ) ) {
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
 for( Index h = 0 ; h < v_Obj.size() ; ++h )
 {
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
  else if( auto * qf = dynamic_cast< p_QF >( f ) ) {
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
 // if state is not a LagBFunctionState &, exception will be thrown
 const auto & s = dynamic_cast< const LagBFunctionState & >( state );

 // ensure g_pool is large enough
 if( s.f_max_glob > g_pool.size() )
  g_pool.resize( s.f_max_glob , std::make_pair( nullptr , true ) );

 // copy the important linearization information
 zLC = s.zLC;

 bool gpempty = ( f_max_glob > 0 );
 Subset Addd;

 // first void the current global pool
 if( NoSol ) {
  std::fill( g_pool.begin() , g_pool.end() ,
	     std::make_pair( nullptr , true ) );
  f_max_glob = 0;
  }
 else {
  for( auto & el : g_pool ) {
   delete el.first;
   el.first = nullptr;
   el.second = true;
   }

  // now add back all the Solution in the State (possibly after a check)
  auto gpit = g_pool.begin();

  if( ChkState )  // if Solutions are checked
   for( Index i = 0 ; i < s.g_pool.size() ; ++i ) {
    if( s.g_pool[ i ].first ) {
     // write the Solution to the inner Block
     s.g_pool[ i ].first->write( v_Block.front() );

     // if it's still a feasible solution/direction, copy it
     if( ( s.g_pool[ i ].second ? v_Block.front()->is_feasible()
	                        : v_Block.front()->is_unbounded() ) ) {
      gpit->first = s.g_pool[ i ].first->clone();  // clone() the Solution in
      gpit->second = s.g_pool[ i ].second;
      Addd.push_back( i );
      f_max_glob = i + 1;
      }
     }
    ++gpit;
    }
  else {        // it is trusted that Solution are correct
   for( Index i = 0 ; i < s.g_pool.size() ; ++i ) {
    if( s.g_pool[ i ].first ) {
     gpit->first = s.g_pool[ i ].first->clone();  // clone() the Solution in
     gpit->second = s.g_pool[ i ].second;
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
   this , C05FunctionMod::GlobalPoolRemoved , std::move( Subset() ) , 0 , 0 ) );

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
  g_pool.resize( s.f_max_glob , std::make_pair( nullptr , true ) );

 // move the important linearization information
 zLC = std::move( s.zLC );

 bool gpempty = ( f_max_glob > 0 );
 Subset Addd;

 // first void the current global pool
 if( NoSol ) {
  std::fill( g_pool.begin() , g_pool.end() ,
	     std::make_pair( nullptr , true ) );
  f_max_glob = 0;
  }
 else {
  for( auto & el : g_pool ) {
   delete el.first;
   el.first = nullptr;
   el.second = true;
   }

  // now add back all the Solution in the State (possibly after a check)
  auto gpit = g_pool.begin();

  if( ChkState )  // if Solutions are checked
   for( Index i = 0 ; i < s.g_pool.size() ; ++i ) {
    if( s.g_pool[ i ].first ) {
     // write the Solution to the inner Block
     s.g_pool[ i ].first->write( v_Block.front() );

     // if it's still a feasible solution/direction, copy it
     if( ( s.g_pool[ i ].second ? v_Block.front()->is_feasible()
	                        : v_Block.front()->is_unbounded() ) ) {
      gpit->first = s.g_pool[ i ].first;  // move the Solution in
      s.g_pool[ i ].first = nullptr;      // delete it from the State
      gpit->second = s.g_pool[ i ].second;
      Addd.push_back( i );
      f_max_glob = i + 1;
      }
     }
    ++gpit;
    }
  else {        // it is trusted that Solution are correct
   for( Index i = 0 ; i < s.g_pool.size() ; ++i ) {
    if( s.g_pool[ i ].first ) {
     gpit->first = s.g_pool[ i ].first;  // move the Solution in
     s.g_pool[ i ].first = nullptr;      // delete it from the State
     gpit->second = s.g_pool[ i ].second;
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
   this , C05FunctionMod::GlobalPoolRemoved , std::move( Subset() ) , 0 , 0 ) );

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
   typ[ i ] = g_pool[ i ].second ? 1 : 0;
 
    ( group.addVar( "LagBFunction_Type" , netCDF::NcByte() , gs ) ).putVar(
			          { 0 } , {  f_max_glob } , typ.data() );

  for( Index i = 0 ; i < f_max_glob ; ++i ) {
   if( ! g_pool[ i ].first )
    continue;

   auto gi = group.addGroup( "LagBFunction_Sol_" + std::to_string( i ) );
   g_pool[ i ].first->serialize( gi );
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
  g_pool[ name ].first = reinterpret_cast< Solution * >( this );
 else {
  delete g_pool[ name ].first;  // delete the Solution already there (if any)

  // get a "fully loaded" Solution out of the inner Block, using the default
  // f_solution_Configuration in the BlockConfig of the inner Block
  g_pool[ name ].first = v_Block.front()->get_Solution( nullptr , false );
  if( ! g_pool[ name ].first )
   throw( std::logic_error( "LagBFunction: no Solution provided by Block" ) );
  }

 g_pool[ name ].second = VarSol;  // record the Solution type
 LastSolution = name;             // record that the Solution has been stored

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
  g_pool[ name ].first = reinterpret_cast< Solution * >( this );
  g_pool[ name ].second = true;
  return;
  }

 #if CHECK_SOLUTIONS & 4
  std::cout << "LagBFunction " << this << ": computing "
	    << coefficients.size() << "-combination in "
	    << name << std::endl;
 #endif

 // get a scaled version of the first Solution
 auto first = coefficients[ 0 ].first;
 auto convex_combination = ( g_pool[ first ].first
        )->scale( coefficients[ 0 ].second );
 bool type = g_pool[ first ].second;  // diagonal unless already vertical

 // for all other Solutions in the pool
 for( Index i = 1 ; i < coefficients.size() ; ++i ) {
  auto pos = coefficients[ i ].first;
  if( pos == first )
   continue;
  auto mult = coefficients[ i ].second;

  #if CHECK_SOLUTIONS & 4
   std::cout << "pos = " << pos << ", mult = " << mult << ", sol = "
             << * g_pool[ pos ].first;
  #endif

  // add the new term to the convex combination
  convex_combination->sum( g_pool[ pos ].first , mult );

  // if the convex combination even contains a single direction
  if( ! g_pool[ pos ].second )
   type = false;  // then it is a direction
  }

 delete g_pool[ name ].first;  // delete the current Solution (if any)

 g_pool[ name ].first = convex_combination;  // store the Solution
 g_pool[ name ].second = type;               // store the type

 if( name == LastSolution )    // if this was the Solution in the inner Block
  LastSolution = g_pool.size();  // it is no longer valid

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
 if( ( name >= g_pool.size() ) || ( ! g_pool[ name ].first ) )
  throw( std::invalid_argument(
	 "LagBFunction::delete_linearization: invalid linearization name" ) );

 if( ! NoSol )                    // if the Solution is there
  delete g_pool[ name ].first;    // delete it
 g_pool[ name ].first = nullptr;  // mark that the position is empty

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
    g_pool[ i ].first = nullptr;
  else
   for( Index i = 0 ; i < f_max_glob ; ++i )
    if( g_pool[ i ].first ) {
     delete g_pool[ i ].first;
     g_pool[ i ].first = nullptr;
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
  if( ! g_pool[ i ].first )
   throw( std::invalid_argument(
       "LagBFunction::delete_linearizations: invalid linearization name" ) );

  if( i == LastSolution )    // if this was the Solution in the inner Block
   LastSolution = g_pool.size();  // it is no longer valid

  if( ! NoSol )               // if a Solution really is there
   delete g_pool[ i ].first;  // delete it
  g_pool[ i ].first = nullptr;
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

 if( ! v_tmpCP.empty() )
  tounlock = flush_v_tmpCP();

 // if necessary, recompute the Lagrangian costs c^y = c + yA- - - - - - - - -
 if( f_dirty_Lc ) {
  // temp array of y values, more cache friendly
  Vec_FunctionValue y( LagPairs.size() );
  for( Index i = 0 ; i < LagPairs.size() ; ++i )
   y[ i ] = LagPairs[ i ].first->get_value();

  // loop over all Blocks in BFS order
  for( Index h = 0 ; h < CostMatrix.size() ; ++h ) {
   const auto & cm = CostMatrix[ h ];

   // array of new Lagrangian costs c^y = c + yA
   Vec_FunctionValue NCoef( cm.size() );

   // compute the Lagrangian costs
   for( Index i = 0 ; i < cm.size() ; ++i ) {
    NCoef[ i ] = cm[ i ].first;
    for( const auto & el : cm[ i ].second )
     NCoef[ i ] += y[ el.first ] * el.second;
   }

   // if the Block has not been locked yet and it is not owned
   bool block_locked = false;
   Block * blk = v_BlockBFS[ h ];
   if( ( ! tounlock ) && ( ! blk->is_owned_by( f_id ) ) ) {
    if( ! blk->lock( f_id ) )  // try to lock it; failure
     return( kError );         // clearly is an error
    block_locked = true;       // it'll have to be unlocked
   }

   f_play_dumb = true;         // ignore any ensuing Modification

   // modify the coefficients in the Objective
   if( ! v_ObjIsQuad[ h ] )
    static_cast< p_LF >( v_Obj[ h ]->get_function() )
      ->modify_coefficients( std::move( NCoef ) , Range( 0 , cm.size() ) );
   else
    static_cast< p_QF >( v_Obj[ h ]->get_function() )
      ->modify_linear_coefficients( std::move( NCoef ) , Range( 0 , cm.size() ) );

   f_play_dumb = false;        // back to normal operations

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
  is->set_ComputeConfig( f_CC );
  f_CC->clear();
  f_CC_changed = false;
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

  if( ! g_pool[ name ].first )
   throw( std::logic_error(
   "LagBFunction::get_linearization_coefficients: invalid linearization name"
			   ) );

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
  *( g++ ) = LagPairs[ i ].second->get_value();
  }
 }  // end( LagBFunction::get_linearization_coefficients( * , range ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::get_linearization_coefficients( FunctionValue * g ,
			      c_Subset & subset , bool ordered , Index name )
{
 // the solution shall be written in the Variable of the Block - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( name == Inf< Index >() ) {  // the last computed linearization- - - - - - -

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

  if( ! g_pool[ name ].first )
   throw( std::logic_error(
   "LagBFunction::get_linearization_coefficients: invalid linearization name"
			   ) );

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
  *( g++ ) = LagPairs[ i ].second->get_value();
  }
 }  // end( LagBFunction::get_linearization_coefficients( * , subset ) )

/*--------------------------------------------------------------------------*/

Function::FunctionValue LagBFunction::get_linearization_constant( Index name )
{
 if( name == Inf< Index >() ) {  // the last computed linearization- - - - - - -

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

  if( ! g_pool[ name ].first )  // if no such linearization
   return( NaN );               // return NaN

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

 // compute the value of the original Objective function (excluding any Lagrangian terms)
 // this includes both the Objective of the root Block and those of all sub-Blocks recursively

 OFValue alpha = 0;

 // if PushCostToOwner == 0, only the root Block's Objective is modified,
 // so we can safely rely on get_objective_value() to collect the *original*
 // values of all sub-Block Objectives recursively
 if( ! PushCostToOwner )
  alpha += get_objective_value( get_inner_block() );

 // for the root Block (and for all other Objectives when PushCostToOwner == 1),
 // we need to compute the value "by hand" using the original coefficients
 // stored in CostMatrix, since their internal Objective may be modified

 for( Index h = 0 ; h < v_Obj.size() ; ++h ) {

  const auto * obj = v_Obj[ h ];
  const auto & cm = CostMatrix[ h ];

  if( ! v_ObjIsQuad[ h ] ) {
   alpha += obj->get_constant_term();
   const auto & rp = static_cast< p_LF >( obj->get_function() )->get_v_var();

   #ifndef NDEBUG
    if( rp.size() > cm.size() )
     throw( std::logic_error( "CostMatrix inconsistent with linear objective" ) );
   #endif

   for( Index i = 0 ; i < rp.size() ; ++i )
    alpha += rp[ i ].first->get_value() * cm[ i ].first;
  }
  else {
   alpha += obj->get_constant_term();
   const auto & rp = static_cast< p_QF >( obj->get_function() )->get_v_var();

   #ifndef NDEBUG
    if( rp.size() > cm.size() )
     throw( std::logic_error( "CostMatrix inconsistent with quadratic objective" ) );
   #endif

   for( Index i = 0 ; i < rp.size() ; ++i ) {
    auto val = std::get< 0 >( rp[ i ] )->get_value();
    if( val ) {
     alpha += cm[ i ].first * val;
     val *= val;
     alpha += std::get< 2 >( rp[ i ] ) * val;
    }
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
 const
{
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
 for( Index h = 0 ; h < v_Obj.size() ; ++h ) {
  if( ! v_ObjIsQuad[ h ] ) {  // the linear case
   auto * lf = static_cast< p_LF >( v_Obj[ h ]->get_function() );
   if( v_tmpCP[ h ].size() == 1 )
    lf->add_variable( v_tmpCP[ h ].front().first ,
		      v_tmpCP[ h ].front().second );
   else
    lf->add_variables( std::move( v_tmpCP[ h ] ) );
   }
  else {                      // the quadratic case
   auto * qf = static_cast< p_QF >( v_Obj[ h ]->get_function() );
   if( v_tmpCP[ h ].size() == 1 )
    qf->add_variable( v_tmpCP[ h ].front().first ,
		      v_tmpCP[ h ].front().second , 0 );
   else {
    v_coeff_triple vars( v_tmpCP[ h ].size() ,
			 coeff_triple( nullptr , 0 , 0 ) );
    for( Index i = 0 ; i < v_tmpCP[ h ].size() ; ++i ) {
     std::get< 0 >( vars[ i ] ) = v_tmpCP[ h ][ i ].first;
     std::get< 1 >( vars[ i ] ) = v_tmpCP[ h ][ i ].second;
     }
    qf->add_variables( std::move( vars ) );
    }
   }
  v_tmpCP[ h ].clear();             // done
  }
 v_tmpCP.clear();

 f_play_dumb = false;               // back to normal operations

 return( tounlock );

 }  // end( LagBFunction::flush_v_tmpCP )

/*--------------------------------------------------------------------------*/

void LagBFunction::add_to_CostMatrix( v_c_dual_pair & newdp )
{
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
     throw std::logic_error( "add_to_CostMatrix: variable block not found" );
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
   } else {
    auto * qf = static_cast< p_QF >( fn );
    nv = qf->get_num_active_var();
    j = qf->is_active( rpj.first );
   }

   if( j >= nv ) {
    // the variable x_j is not (yet) in obj, but it may be in v_tmpCP[ h ] already
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
    } else
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
 if( ( ! v_tmpCP.empty() ) && flush_v_tmpCP() )
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
    throw std::logic_error( "mod_CostMatrix: variable block not found" );
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
  } else {
   auto * qf = static_cast< p_QF >( fn );
   nv = qf->get_num_active_var();
   j = qf->is_active( rp[ h ].first );
  }

  if( j >= nv ) {
   // the variable x_j is not (yet) in obj, but it may be in v_tmpCP[k] already
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
   } else
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
 f_CC->f_diff = true;       // set it in "diff mode"
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
  // registered by it (hence, be sure it is in f_diff == true mode)
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

char LagBFunction::guts_of_guts_of_add_Modification( p_Mod mod , ChnlName chnl )
{
 // process Modification - - - - - - - - - - - - - - - - - - - - - - - - - - -
 /* This requires to patiently sift through the possible Modification types
    to find what this Modification exactly is and appropriately react. */

 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // C05FunctionModLin: the "linear part" of a Function has been changed
 // C05FunctionModLin can have a special treatment, and therefore need be
 // checked before FunctionMod (because C05FunctionModLin is a FunctionMod)
 // in case they come from:
 //
 // - the (LinearFunction or DQuadFunction inside the) Objective of the inner
 //   Block;
 //
 // - the LinearFunction that defines one of the Lagrangian terms
 //   < y_i , g_i( x ) = A_i x + b_i >
 //
 // There are two types of C05FunctionModLin, according to if the
 // coefficients of the LinearFunction that change are a Range or a Subset.
 // Hence, two almost identical pieces of code follow, one for each of them.
 //
 // IMPORTANT NOTE 1: Modification coming from obj can be "arbitrarily
 //                   delayed", since (say) they can be stored in a
 // GroupModification and only processed a lot later. In particular, the
 // indices in delta() may NO LONGER CORRESPOND TO THE CURRENT POSITION OF
 // THE ColVariable IN obj, SINCE "ACTIVE" Variable MAY HAVE BEEN ADDED OR
 // DELETED. However, if this happens it is signalled by a Modification to be
 // found *after* the current one. CostMatrix is kept parallel to obj as these
 // Modification happen, which means that the current status of CostMatrix is
 // exactly parallel to the status of obj at the time in which the
 // Modification was issued, which allows directly using range().
 //
 // IMPORTANT NOTE 2: conversely, Modification coming from Lagrangian terms
 //                   "immediately reach" the LagBFunction, since they do
 // not pass from any other Block before and therefore they cannot ever be
 // packed in a GroupModification and delayed (before getting here, this can
 // happen for Block further up the tree and for Solver) 
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( Index h = 0 ; h < CostMatrix.size() ; ++h ) {
  auto & CMh = CostMatrix[ h ];
  auto * CMh_f = v_Obj[ h ]->get_function();

  if( ! v_ObjIsQuad[ h ] )
   CMh_f = static_cast< p_LF >( v_Obj[ h ]->get_function() );
  else
   CMh_f = static_cast< p_QF >( v_Obj[ h ]->get_function() );

  // C05FunctionModLinRngd- - - - - - - - - - - - - - - - - - - - - - - - - - -
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  if( const auto tmod = dynamic_cast< const C05FunctionModLinRngd * >( mod ) ) {

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

    if( lf->get_Observer() == this ) {
     // ... defining a Lagrangian term < y_i , g_i( x ) > - - - - - - - - - - -
     // the corresponding entry of all the linearizations changes

     // search for the Lagrangian term which has changed
     auto it = std::find_if( LagPairs.begin() , LagPairs.end() ,
        [ lf ]( auto & p )
              { return( p.second == lf ); } );

     #ifndef NDEBUG
      if( it == LagPairs.end() )
       throw( std::logic_error( "Lagrangian term not found" ) );
     #endif

     c_Index i = std::distance( LagPairs.begin() , it );
     const auto & rc = lf->get_v_var();
     auto dit = tmod->delta().begin();

     // for all the coefficients a_{ij} in A_j that have changed
     for( Index h = tmod->range().first ; h < tmod->range().second ; ++h ) {
      auto j = CMh_f->is_active( rc[ h ].first ); // find x_j

      // find the place of < y_i , a_{ij} > in A_j (has to be there)
      auto ajit = std::lower_bound( CMh[ j ].second.begin() ,
                                    CMh[ j ].second.end() ,
                                    mon_pair( i , 0 ) ,
                                    []( const auto & a , const auto & b )
                                    { return( a.first < b.first ); } );

      #ifndef NDEBUG
       if( ajit == CMh[ j ].second.end() )
        throw( std::logic_error( "inconsistent CostMatrix" ) );
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

    }  // end( coming from( < y_i , g_i( x ) > ) )
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
  // same comments and IMPORTANT NOTESs as for C05FunctionModLinRngd, except of
  // course there is a Subset rather than a Range
  if( const auto tmod = dynamic_cast< const C05FunctionModLinSbst * >( mod ) ) {
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

    if( lf->get_Observer() == this ) {
     // ... defining a Lagrangian term < y_i , g_i( x ) > - - - - - - - - - - -
     // the corresponding entry of all the linearizations changes

     // search for the Lagrangian term which has changed
     auto it = std::find_if( LagPairs.begin() , LagPairs.end() ,
        [ lf ]( auto & p )
              { return( p.second == lf ); } );

     #ifndef NDEBUG
      if( it == LagPairs.end() )
       throw( std::logic_error( "Lagrangian term not found" ) );
     #endif

     c_Index i = std::distance( LagPairs.begin() , it );
     const auto & rc = lf->get_v_var();
     auto dit = tmod->delta().begin();

     // for all the coefficients a_{ij} in A_j that have changed
     for( auto h : tmod->subset() ) {
      auto j = CMh_f->is_active( rc[ h ].first ); // find x_j

      // find the place of < y_i , a_{ij} > in A_j (has to be there)
      auto ajit = std::lower_bound( CMh[ j ].second.begin() ,
                                    CMh[ j ].second.end() ,
                                    mon_pair( i , 0 ) ,
                                    []( const auto & a , const auto & b )
                                    { return( a.first < b.first ); } );

      #ifndef NDEBUG
       if( ajit == CMh[ j ].second.end() )
        throw( std::logic_error( "inconsistent CostMatrix" ) );
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

    }  // end( coming from( < y_i , g_i( x ) > ) )
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
  // - the (LinearFunction od DQuadFunction inside the) Objective of the inner
  //   Block, or any of its sub-Block (recursively); if it is obj, the only
  //   remaining FunctionMod is the C05FunctionMod with type() ==
  //   NothingChanged corresponding to the change of the constant term
  //
  // - the LinearFunction that defines a Lagrangian term < y_i , g_i( x ) >;
  //   also in this case, the only remaining FunctionMod is the C05FunctionMod
  //   with type() == NothingChanged corresponding to the change of the
  //   constant term
  //
  // - any Constraint in the inner Block, or any of its sub-Block (recursively)
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( const auto tmod = dynamic_cast< const FunctionMod * >( mod ) ) {
   auto f = tmod->function();  // the Function it comes from

   if( CMh_f && ( dynamic_cast< p_LF >( f ) == CMh_f ) ) {  // if it is obj - -
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

   if( CMh_f && ( dynamic_cast< p_QF >( f ) == CMh_f ) ) {  // if it is qobj- -
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

   }  // end( if( from qobj ) )

   if( dynamic_cast< Objective * >( f->get_Observer() ) ) {
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // if it is not obj, it may still be the Function inside the Objective of a
    // further sub-Block of the inner Block

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

   if( f->get_Observer() == this ) {  // a g_i( x ) - - - - - - - - - - - - - -
    // the next case is the one where f is one of the LinearFunction defining
    // the Lagrangian term < y_i , g_i( x ) >; these are easy to spot in that
    // are the only Function whose Observer is directly the LagBFunction.
    // again, the only remaining FunctionMod is the C05FunctionMod with
    // type() == NothingChanged corresponding to the change of the constant
    // term. that is, the constant term b_i of the LinearFunction g_i( x ) =
    // A_i x + b_i has changed to b'_i. hence, the i-th entry of all
    // linearizations changes by shift() == b'_i - b_i, which is the perfect
    // case for a C05FunctionModLinRngd with range() == ( i , i + 1 ) and
    // delta() == { shift() }

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
        [ f ]( auto & p )
             { return( p.second ==
         static_cast< p_LF >( f ) ); } );

     #ifndef NDEBUG
      if( it == LagPairs.end() )
       throw( std::logic_error( "Lagrangian term not found" ) );
     #endif

     Index i = std::distance( LagPairs.begin() , it );
     f_Observer->add_Modification( std::make_shared< C05FunctionModLinRngd >(
          this , Vec_FunctionValue( { tmod->shift() } ) ,
          Vec_p_Var( { it->first } ) ,
          Range( i , i + 1 ) , NaN , true ) ,
       chnl );
    }

    return( 0 );  // the case of changes in the Lagrangian term is over
   }

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
  // FunctionModVars) in case they come from:
  //
  // - the (LinearFunction or DQuadFunction inside the) Objective of the inner
  //   Block;
  //
  // - the LinearFunction that defines one of the Lagrangian terms
  //   < y_i , g_i( x ) >
  //
  // There are three types of C05FunctionModVars, according to if the
  // Variable are added or deleted, and in the latter case if what is
  // deleted is a Range or a Subset. Hence, three similar pieces of code
  // follow, two almost being identical.
  //
  // IMPORTANT NOTE: see IMPORTANT NOTE 1 and IMPORTANT NOTE 2 for the
  //                 C05FunctionModLin, which apply verbatim here as well
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // C05FunctionModVarsAddd - - - - - - - - - - - - - - - - - - - - - - - - - -
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // thid is: [Col]Variable are being added
  if( const auto tmod = dynamic_cast< const C05FunctionModVarsAddd * >( mod ) ) {
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
                                     this , C05FunctionMod::AlphaChanged , Subset() ,
                                     1 , NaN , true ) ,
                                    chnl );
     return( 0 );  // all done

    }  // end( coming from obj )

    if( lf->get_Observer() == this ) {  // coming from a g_i( x ) - - - - - - -
     // add the corresponding terms to CostMatrix

     // search for the Lagrangian term which has changed
     auto it = std::find_if( LagPairs.begin() , LagPairs.end() ,
        [ lf ]( auto & p )
              { return( p.second == lf ); } );

     #ifndef NDEBUG
      if( it == LagPairs.end() )
       throw( std::logic_error( "Lagrangian term not found" ) );
     #endif

     c_Index i = std::distance( LagPairs.begin() , it );
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

    }  // end( coming from( < y_i , g_i( x ) > ) )
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
                                      this , C05FunctionMod::AlphaChanged , Subset() ,
                                      1 , NaN , true ) ,
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
  if( const auto tmod = dynamic_cast< const C05FunctionModVarsRngd * >( mod ) ) {
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

    if( lf->get_Observer() == this ) {  // coming with a g_i( x ) - - - - - - -
     // remove the corresponding terms from CostMatrix

     // search for the Lagrangian term which has changed
     auto it = std::find_if( LagPairs.begin() , LagPairs.end() ,
        [ lf ]( auto & p )
              { return( p.second == lf ); } );

     #ifndef NDEBUG
      if( it == LagPairs.end() )
       throw( std::logic_error( "Lagrangian term not found" ) );
     #endif

     c_Index i = std::distance( LagPairs.begin() , it );
     c_Index nv = CMh_f->get_num_active_var();

     // for all the Variable that have been eliminated
     for( auto xj : tmod->vars() ) {
      auto j = CMh_f->is_active( xj );
      if( j >= nv ) {
       // the deleted variable is not in obj yet, but it may be in v_tmpCP
       // waiting to be added to obj
       Index h;
       if( v_Obj.size() == 1 )
        h = 0;
       else {
        auto itb = Block2Idx.find( xj->get_Block() );
        if( ( itb == Block2Idx.end() ) || ( itb->second >= v_Obj.size() ) )
         throw( std::logic_error( "deleted variable not found in Block2Idx" ) );
        h = itb->second;
       }

       auto tCPit = std::find_if( v_tmpCP[ h ].begin() , v_tmpCP[ h ].end() ,
         [ & xj ]( const auto & p ) { return( p.first == xj ); } );
       if( tCPit == v_tmpCP[ h ].end() )
        throw( std::logic_error( "deleted variable not found" ) );
       j = nv + std::distance( v_tmpCP[ h ].begin() , tCPit );
      }

      auto ajit = std::lower_bound( CMh[ j ].second.begin() ,
                                    CMh[ j ].second.end() ,
                                    mon_pair( i , 0 ) ,
                                    []( const auto & a , const auto & b )
                                    { return( a.first < b.first ); } );

      #ifndef NDEBUG
       if( ajit == CMh[ j ].second.end() )
        throw( std::logic_error( "a_{ij} term not found in CostMatrix" ) );
      #endif

      // remove < y_i , a_{ij} > from A_j
      CMh[ j ].second.erase( ajit );

      // if this leaves the term empty and the term actually was of some
      // variable that still had to be added to obj, just don't do that:
      // rather, erase the row of CostMatrix and the corresponding one in
      // v_tmpCP
      if( CMh[ j ].second.empty() && ( j >= nv ) ) {
       CMh.erase( CMh.begin() + j );
       v_tmpCP[ h ].erase( v_tmpCP[ h ].begin() + ( j - nv ) );
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

    }  // end( coming from( < y_i , g_i( x ) > ) )
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
  if( const auto tmod = dynamic_cast< const C05FunctionModVarsSbst * >( mod ) ) {
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

    if( lf->get_Observer() == this ) {  // coming with a g_i( x ) - - - - - - -
     // remove the corresponding terms from CostMatrix

     // search for the Lagrangian term which has changed
     auto it = std::find_if( LagPairs.begin() , LagPairs.end() ,
        [ lf ]( auto & p )
              { return( p.second == lf ); } );

     #ifndef NDEBUG
      if( it == LagPairs.end() )
       throw( std::logic_error( "Lagrangian term not found" ) );
     #endif

     c_Index i = std::distance( LagPairs.begin() , it );
     c_Index nv = CMh_f->get_num_active_var();

     // for all the Variable that have been eliminated
     for( auto xj : tmod->vars() ) {
      auto j = CMh_f->is_active( xj );
      if( j >= nv ) {
       // the deleted variable is not in obj yet, but it may be in v_tmpCP[h]
       Index h;
       if( v_Obj.size() == 1 )
        h = 0;
       else {
        auto itb = Block2Idx.find( xj->get_Block() );
        if( ( itb == Block2Idx.end() ) || ( itb->second >= v_Obj.size() ) )
         throw( std::logic_error( "deleted variable not found in Block2Idx" ) );
        h = itb->second;
       }

       auto tCPit = std::find_if( v_tmpCP[ h ].begin() , v_tmpCP[ h ].end() ,
        [ & xj ]( const auto & p ) { return( p.first == xj ); } );
       if( tCPit == v_tmpCP[ h ].end() )
        throw( std::logic_error( "deleted variable not found" ) );
       j = nv + std::distance( v_tmpCP[ h ].begin() , tCPit );
      }

      auto ajit = std::lower_bound( CMh[ j ].second.begin() ,
                                    CMh[ j ].second.end() ,
                                    std::make_pair( i , 0 ) ,
                                    []( const auto & a , const auto & b )
                                    { return( a.first < b.first ); } );

      #ifndef NDEBUG
       if( ajit == CMh[ j ].second.end() )
        throw( std::logic_error( "a_{ij} term not found in CostMatrix" ) );
      #endif

      // remove < y_i , a_{ij} > from A_j
      CMh[ j ].second.erase( ajit );

      // if this leaves the term empty and the term actually was of some
      // variable that still had to be added to obj, just don't do that:
      // rather, erase the row of CostMatrix and the corresponding one in
      // v_tmpCP
      if( CMh[ j ].second.empty() && ( j >= nv ) ) {
       CMh.erase( CMh.begin() + j );
       v_tmpCP[ h ].erase( v_tmpCP[ h ].begin() + ( j - nv ) );
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

    }  // end( coming from( < y_i , g_i( x ) > ) )
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
  // The Function can *not* be
  //
  // - the (LinearFunction or DQuadFunction inside the) Objective of the inner
  //   Block
  //
  // - a LinearFunction that define the Lagrangian term < y_i , g_i( x ) > for
  //
  // since these have been dealt with already. What remains is the Objective
  // of some sub-Block of the inner Block, or a Constraint in the inner Block
  // (or any of its sub-Block, recursively)
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( const auto tmod = dynamic_cast< const FunctionModVars * >( mod ) ) {
   auto f = tmod->function();  // the Function it comes from

   if( dynamic_cast< Objective * >( f->get_Observer() ) ) {
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // the Function inside the Objective of a sub-Block of the inner Block

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
   ( ( ! xj->is_positive() ) && xj->is_positive( tmod->old_state() ) ) )
       && ( ( xj->is_negative() == xj->is_negative( tmod->old_state() ) ) ||
   ( ( ! xj->is_negative() ) && xj->is_negative( tmod->old_state() ) ) )
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
 }
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
  for( Index j = rng.first ; j < rng.second ; ++j )
   CM[ j ].first = rc[ j ].second;
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
  for( Index j : sbst )
   CM[ j ].first = rc[ j ].second;
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

void LagBFunction::update_CostMatrix_ModVarsAddd( c_Vec_p_Var & vars ,
                                                  Index first )
{
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

  for( Index i = 0 ; i < f_max_glob ; ++i ) {
   int ti;
   nct.getVar( { i } , &ti );
   g_pool[ i ].second = ( ti != 0 );

   auto gi = group.getGroup( "LagBFunction_Sol_" + std::to_string( i ) );
   if( gi.isNull() )
    g_pool[ i ].first = nullptr;
   else
    g_pool[ i ].first = Solution::new_Solution( gi );
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
   typ[ i ] = g_pool[ i ].second ? 1 : 0;
 
  ( group.addVar( "LagBFunction_Type" , netCDF::NcByte() , gs ) ).putVar(
				      { 0 } , {  f_max_glob } , typ.data() );

  for( Index i = 0 ; i < f_max_glob ; ++i ) {
   if( ! g_pool[ i ].first )
    continue;

   auto gi = group.addGroup( "LagBFunction_Sol_" + std::to_string( i ) );
   g_pool[ i ].first->serialize( gi );
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
