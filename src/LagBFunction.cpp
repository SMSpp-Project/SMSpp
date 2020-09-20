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
/*------------------------------ LOCAL TYPES -------------------------------*/
/*--------------------------------------------------------------------------*/

using p_LF = LinearFunction *;

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
 auto i = *(Bit++);
 auto git = g.begin() + (i++);

 for( ; Bit != B.end() ; ++i ) {
  auto h = *(Bit++);
  while( i < h )
   *(git++) = std::move( g[ i++ ] );
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

LagBFunction::LagBFunction( Block* innerblock , Observer * const observer )
 : C05Function() , obj( nullptr ) , IsConvex( true ) , f_max_glob( 0 ) ,
   LastSolution( 0 ) , VarSol( true ) , f_yb( -INF ) , f_play_dumb( false ) ,
   f_dirty_Lc( false ) , LPMaxSz( 0 ) , RAccLin( 0 ) , AAccLin( 0 ) ,
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
 // delete all the Lagrangian terms (and the ColVariable with them)
 clear_lp();

 // delete the auxiliary data structure for computing the Lagrangian costs
 CostMatrix.clear();

 // delete the global pool (do not issue any Modification because clear()
 // is only meant to be called right before destroying the Function, any
 // Solver attached to it should have been done with long ago
 for( Index i = 0 ; i < f_max_glob ; ++i )
  delete g_pool[ i ].first;
 g_pool.clear();
 f_max_glob = 0;
 f_yb = -INF;  // since b is empty, there are no nonzeros
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::set_inner_block( Block * innerblock )
{
 // consistency checks
 if( ! innerblock )
  throw( std::invalid_argument( "empty inner Block not allowed" ) );

 const auto frobj = innerblock->get_objective< FRealObjective >();
 if( ! frobj )
  throw( std::invalid_argument( "inner Block Objective not a FRealObjective"
				) );

 IsConvex = ( frobj->get_sense() == Objective::eMax );

 obj = dynamic_cast< p_LF >( frobj->get_function() );
 if( ! obj )
  throw( std::invalid_argument( "inner Block Objective not a LinearFunction"
				) );
 if( v_Block.empty() )
  v_Block.resize( 1 );
 else
  if( v_Block.front() ) {
   v_Block.front()->set_f_Block( nullptr );
   delete v_Block.front();
   }

 // set the inner Block
 v_Block.front() = innerblock;
 innerblock->set_f_Block( this );

  // construct CostMatrix whose size is that of active variables in (obj_B)
 const auto & rp = obj->get_v_var();
 CostMatrix.resize( rp.size() );

 // save in CostMatrix[ i ].first is the original coefficient on the i-th
 // ColVariable in (obj_B)
 for( Index i = 0 ; i < rp.size() ; ++i )
  CostMatrix[ i ].first = rp[ i ].second;

 f_dirty_Lc = true;  // Lagrangian costs have to be updated

 }  // end( LagBFunction::set_inner_block )

/*--------------------------------------------------------------------------*/

void LagBFunction::set_dual_pairs( v_dual_pair && dp )
{
 clear_lp(); // ensure we are starting from a "tabula rasa"

 // construct the auxiliary structure CostMatrix which is used to update the
 // Lagrangian cost vector
 //
 // CostMatrix is a vector of pairs < c_j , A_j > indexed like the primal
 // variable x_j = obj->get_active_var( j ). c_j is the original cost (which
 // would no longer be available in obj and therefore need to be stored
 // somewhere, and A_j is a LinearFuction::v_coeff_pair: a vector of pairs
 // < y_i , a_{ij} > (pointer to ColVariable, real coefficient) describing
 // linear function y A_j required to compute the Lagrangian cost c_j - y A_j

 add_columns( dp );

 // save the dual pairs in the LagPairs data structure

 LagPairs = std::move( dp );

 f_yb = -INF;                       // b == 0
 for( auto const & lp : LagPairs )  // ... unless otherwise proven
  if( static_cast< p_LF >( lp.second )->get_constant_term() ) {
   f_yb = NaN; break;               // if so, yb has to be computed
   }

 f_dirty_Lc = true;                 // Lagrangian costs have to be updated

 }  // end( LagBFunction::set_dual_pairs )

/*--------------------------------------------------------------------------*/

void LagBFunction::set_default_inner_BlockConfig( void )
{
 if( auto inner_block = get_inner_block() ) {
  auto config = new OCRBlockConfig( inner_block );
  config->clear();
  config->apply( inner_block );
  }
 }

/*--------------------------------------------------------------------------*/

void LagBFunction::set_default_inner_BlockSolverConfig( void )
{
 if( auto inner_block = get_inner_block() ) {
  auto solver_config = new RBlockSolverConfig( inner_block );
  solver_config->clear();
  solver_config->apply( inner_block );
  }
 }

/*--------------------------------------------------------------------------*/

void LagBFunction::set_ComputeConfig( ComputeConfig * scfg )
{
 ThinComputeInterface::set_ComputeConfig( scfg );

 auto inner_block = get_inner_block();
 if( ! inner_block )
  return;

 if( ! scfg ) {  // scfg is nullptr
  set_default_inner_Block_configuration();
  return;
  }

 if( ! scfg->f_extra_Configuration ) {
  // scfg->f_extra_Configuration is nullptr
  if( ! scfg->f_diff )
   set_default_inner_Block_configuration();
  return;
  }

 // if the extra_Configuration is a std::pair of Configuration *
 if( auto config = dynamic_cast< SConf_p_p * >( scfg->f_extra_Configuration
						) ) {
  // set the BlockConfig of the inner Block
  if( config->f_value.first ) {  // if it was provided in extra_Configuration
   if( auto block_config = dynamic_cast< BlockConfig * >(
						  config->f_value.first ) )
    block_config->apply( inner_block );
   else
    throw( std::invalid_argument( "LagBFunction::set_ComputeConfig: scfg "
				  "extra_Configuration.first must be a "
				  "BlockConfig *" ) );
   }
  else                           // it was not provided in extra_Configuration
   if( ! scfg->f_diff )          // and scfg is in set mode
    set_default_inner_BlockConfig();  // reset to default

  // set the BlockSolverConfig of the inner Block
  if( config->f_value.second ) {  // if it was provided in extra_Configuration
   if( auto block_config = dynamic_cast< BlockSolverConfig * >(
						   config->f_value.second ) )
    block_config->apply( inner_block );
   else
    throw( std::invalid_argument( "LagBFunction::set_ComputeConfig: scfg "
				  "extra:Configuration.second must be a "
				  "BlockSolverConfig *" ) );
   }
  else                           // it was not provided in extra_Configuration
   if( ! scfg->f_diff )          // and scfg is in set mode
    set_default_inner_BlockSolverConfig();  // reset to default
  }
 else
  throw( std::invalid_argument( "LagBFunction::set_ComputeConfig: "
                                "invalid extra Configuration" ) );

 }  // end( LagBFunction::set_ComputeConfig )

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
   if( ( LastSolution < Inf<Index>() ) && ( LastSolution >= g_pool.size() ) )
    // LastSolution is undefined: ensure it remains so even if
    LastSolution = value;         // the global pool grows
   // note: if the global pool shrinks and LastSolution is one of the deleted
   //       ones it is >= value, which automatically means "undefined"
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
 }  // end( LagBFunction::set_par( int ) )

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
 throw( std::logic_error( "LagBFunction::deserialize not implemented yet" ) );

 guts_of_destructor();  // cleanup whatever is there now
 f_dirty_Lc = true;     // Lagrangian costs have to be updated
 f_yb = INF;            // have to check if b == 0 or not

 // now the inner Block - - - - - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcGroup sb = group.getGroup( "B" );
 if( sb.isNull() )
  throw( std::invalid_argument( "no inner Block provided" ) );

 v_Block.push_back( new_Block( sb , this ) );

 // now the Lagrangian term < y , g(x) >- - - - - - - - - - - - - - - - - - -
 //!! not implemented yet

 // call the method of Block- - - - - - - - - - - - - - - - - - - - - - - - -
 // inside this the NBModification, the "nuclear option",  is issued

 Block::deserialize( group );

 }  // end( LagBFunction::deserialize )

/*--------------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::add_dual_pairs( v_dual_pair && lp , ModParam issueMod )
{
 if( lp.empty() )  // adding nothing
  return;          // cowardly (and silently) return

 // update CostMatrix- - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 add_columns( lp );

 // if b == 0, check if this remains true- - - - - - - - - - - - - - - - - - -
 if( f_yb == -INF )
  for( auto const & el : lp )
   if( static_cast< p_LF >( el.second )->get_constant_term() ) {
    f_yb = NaN; break;  // if not, yb has to be computed
    }

 f_dirty_Lc = true;  // Lagrangian costs have to be updated

 // merge the list of dual Lagrangian pairs  - - - - - - - - - - - - - - - - -
 // be sure to use std::make_move_iterator() to have the contents of lp moved
 // into LagPairs rather than copied

 Index k = LagPairs.size();
 LagPairs.insert( LagPairs.end() , std::make_move_iterator( lp.begin() ) ,
		                   std::make_move_iterator( lp.end() ) );

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

 // update CostMatrix- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // get the list of pairs < x_j , a_{ij} > in g_i(x)
 LinearFunction::v_c_coeff_pair & rp =
  static_cast< p_LF >( LagPairs[ i ].second )->get_v_var();

 // for each pair < x_j , a_{ij} > in g_i(x)
 for( const auto & monomial : rp ) {

  // find the position of x_j in (obj_B)
  Index j = obj->is_active( monomial.first );
  #ifndef NDEBUG
   if( j >= obj->get_num_active_var() )
    throw( std::logic_error( "inconsistency between obj and LagPairs" ) );
  #endif

  // find the position of the term < y_i , a_{ij} > in CostMatrix[ j ]
  auto it = std::lower_bound( CostMatrix[ j ].second.begin() ,
			      CostMatrix[ j ].second.end() ,
			      std::make_pair( LagPairs[ i ].first , 0 ) ,
			      []( const LinearFunction::coeff_pair & a ,
				  const LinearFunction::coeff_pair & b )
	   	 		{ return( a.first < b.first ); } );
  #ifndef NDEBUG
   if( it == CostMatrix[ j ].second.end() )
    throw( std::logic_error( "inconsistency between CostMatrix and LagPairs"
			     ) );
  #endif

  CostMatrix[ j ].second.erase( it );   // now erase it

  }  // end( for( rp ) )

 // if b != 0 but we are eliminating a nonzero, it may have become 0 - - - - -
 if( ( f_yb > -INF ) && ( static_cast< p_LF >(
			     LagPairs[ i ].second )->get_constant_term() ) ) {
  f_yb = INF;  // if so, signal to check if b == 0 or not
  }

 f_dirty_Lc = true;  // Lagrangian costs have to be updated

 // now actually eliminate the row from LagPairs - - - - - - - - - - - - - - -
 auto itv = LagPairs.begin() + i;
 auto var = itv->first;
 LagPairs.erase( itv );       // erase it

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // a Lagrangian function is strongly quasi-additive: shift() == 0
 f_Observer->add_Modification( std::make_shared<C05FunctionModVarsRngd>(
					this , Vec_p_Var( { var } ) ,
			                Range( i , i + 1 ) , 0 ,
				        Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LagBFunction::remove_variable )

/*--------------------------------------------------------------------------*/

void LagBFunction::remove_variables( Range range , ModParam issueMod )
{
 range.second = std::min( range.second , c_Index( LagPairs.size() ) );
 if( range.second <= range.first )  // actually nothing to remove
  return;                           // cowardly (and silently) return

 f_dirty_Lc = true;  // Lagrangian costs have to be updated anyway

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
  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   f_Observer->add_Modification( std::make_shared<C05FunctionModVarsSbst>(
				 this , std::move( vars ) , Subset() , true ,
				 0 , Observer::par2concern( issueMod ) ) ,
				 Observer::par2chnl( issueMod ) );
   }
  else          // no-one is listening
   clear_lp();  // just do it

  f_yb = -INF;  // b is empty, hence there are no nonzeros
  return;
  }

 // this is not a complete reset
 const auto strtit = LagPairs.begin() + range.first;
 const auto stopit = LagPairs.begin() + range.second;

 // update CostMatrix- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // for each row i to remove
 for( auto lpit = strtit ; lpit != stopit ; ++lpit ) {
  // get the list of pairs < x_j , a_{ij} > in g_i(x)
  LinearFunction::v_c_coeff_pair & rp =
   static_cast< p_LF >( lpit->second )->get_v_var();

  // for each pair < x_j , a_{ij} > in g_i(x)
  for( const auto & monomial : rp ) {

   // find the position of x_j in (obj_B)
   Index j = obj->is_active( monomial.first );
   #ifndef NDEBUG
    if( j >= obj->get_num_active_var() )
     throw( std::logic_error( "inconsistency between obj and LagPairs" ) );
   #endif

   // find the position of the term < y_i , a_{ij} > in CostMatrix[ j ]
   auto it = std::lower_bound( CostMatrix[ j ].second.begin() ,
			       CostMatrix[ j ].second.end() ,
			       std::make_pair( lpit->first , 0 ) ,
			       []( const LinearFunction::coeff_pair & a ,
				   const LinearFunction::coeff_pair & b )
	   	 		 { return( a.first < b.first ); } );
   #ifndef NDEBUG
    if( it == CostMatrix[ j ].second.end() )
     throw( std::logic_error( "inconsistency between CostMatrix and LagPairs"
			      ) );
   #endif

   }  // end( for( rp ) )
  }  // end( for( rows to eliminate )

 // if b != 0 but we are eliminating nonzeros, it may have become 0- - - - - -
 if( f_yb > -INF )
  for( auto lpit = strtit ; lpit != stopit ; ++lpit )
   if( static_cast< p_LF >( lpit->second )->get_constant_term() ) {
    f_yb = INF; break;  // if so, signal to check if b == 0 or not
    }

 // now actually eliminate the rows from LagPairs- - - - - - - - - - - - - - -
 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vars( range.second - range.first );
  auto vpit = vars.begin();
  for( auto tmpit = strtit ; tmpit < stopit ; )
   *(vpit++) = (tmpit++)->first;

  LagPairs.erase( strtit , stopit );

  // a Lagrangian function is strongly quasi-additive: shift() == 0
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
 f_dirty_Lc = true;  // Lagrangian costs have to be updated anyway

 if( nms.empty() ) {  // removing all Variable
  if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
   // an Observer is there: copy the names of deleted Variable (all of them)
   Vec_p_Var vars( LagPairs.size() );

   for( Index i = 0 ; i < LagPairs.size() ; ++i )
    vars[ i ] = LagPairs[ i ].first;

   clear_lp();  // then clear the LagBFunction

  // now issue the Modification: note that the subset is empty
  // a LagBFunction is strongly quasi-additive
  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   f_Observer->add_Modification( std::make_shared<C05FunctionModVarsSbst>(
				 this , std::move( vars ) , Subset() , true ,
				 0 , Observer::par2concern( issueMod ) ) ,
				 Observer::par2chnl( issueMod ) );
   }
  else          // no-one is listening
   clear_lp();  // just do it

  f_yb = -INF;  // b is empty, hence there are no nonzeros
  return;
  }

 // this is not a complete reset
 if( ! ordered )
  std::sort( nms.begin() , nms.end() );

 if( nms.back() >= LagPairs.size() )
  throw( std::invalid_argument( "LagBFunction::remove_variables: wrong index"
				) );
 // update CostMatrix- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 for( Index i : nms ) {  // for each row i to remove
  // get the list of pairs < x_j , a_{ij} > in g_i(x)
  LinearFunction::v_c_coeff_pair & rp =
   static_cast< p_LF >( LagPairs[ i ].second )->get_v_var();

  // for each pair < x_j , a_{ij} > in g_i(x)
  for( const auto & monomial : rp ) {

   // find the position of x_j in (obj_B)
   Index j = obj->is_active( monomial.first );
   #ifndef NDEBUG
    if( j >= obj->get_num_active_var() )
     throw( std::logic_error( "inconsistency between obj and LagPairs" ) );
   #endif

   // find the position of the term < y_i , a_{ij} > in CostMatrix[ j ]
   auto it = std::lower_bound( CostMatrix[ j ].second.begin() ,
			       CostMatrix[ j ].second.end() ,
			       std::make_pair( LagPairs[ i ].first , 0 ) ,
			       []( const LinearFunction::coeff_pair & a ,
				   const LinearFunction::coeff_pair & b )
	   	 		 { return( a.first < b.first ); } );
   #ifndef NDEBUG
    if( it == CostMatrix[ j ].second.end() )
     throw( std::logic_error( "inconsistency between CostMatrix and LagPairs"
			      ) );
   #endif

   }  // end( for( rp ) )
  }  // end( for( rows to eliminate )

 // if b != 0 but we are eliminating nonzeros, it may have become 0- - - - - -
 if( f_yb > -INF )
  for( Index i : nms )
   if( static_cast< p_LF >( LagPairs[ i ].second )->get_constant_term() ) {
    f_yb = INF; break;  // if so, signal to check if b == 0 or not
    }

 // now actually eliminate the rows from LagPairs- - - - - - - - - - - - - - -
 auto it = nms.begin();
 auto vi = *it;    // first element to be eliminated
 auto curr = LagPairs.begin() + vi;   // position where to move stuff

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
    *(curr++) = std::move( LagPairs[ vi ] );  // move in the current position

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
 throw( std::logic_error( "LagBFunction::serialize not implemented yet" ) );

 // call the method of Block- - - - - - - - - - - - - - - - - - - - - - - - -

 Block::serialize( group );

 // now the Lagrangian term < y , g(x) >- - - - - - - - - - - - - - - - - - -
 //!! not implemented yet

 // now the inner Block - - - - - - - - - - - - - - - - - - - - - - - - - - -

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
  v_Block.front()->unlock( this );  // unlock it

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

 if( newlin ) {                  // the Solver has the desired stuff
  VarSol = diagonal;             // set the type of the Solution
  LastSolution = g_pool.size();  // signal it has to be read in the Block
  }

 return( newlin );

 }  // end( LagBFunction::has_linearization )

/*--------------------------------------------------------------------------*/

bool LagBFunction::compute_new_linearization( const bool diagonal )
{
 // true if another linearization of the related type exists
 bool newlin = diagonal
  ? v_Block.front()->get_registered_solvers().front()->new_var_solution()
  : v_Block.front()->get_registered_solvers().front()->new_var_direction();

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
  throw( std::logic_error( "invalid linearization name" ) );

 // throw exception if the solution does not exist or has been already stored
 if( LastSolution < Inf<Index>() )
  throw( std::logic_error( "the linearization is unvailable" ) );

 // get the current Solution from the Solver - - - - - - - - - - - - - - - - -

 delete g_pool[ name ].first;  // delete the Solution already there (if any)

 // get a "fully loaded" Solution out of the inner Block, using the default
 // f_solution_Configuration in the BlockConfig of the inner Block
 g_pool[ name ].first = v_Block.front()->get_Solution( nullptr , false );

 g_pool[ name ].second = VarSol;  // record the Solution type
 LastSolution = name;             // record that the Solution has been stored

 if( name >= f_max_glob )         // update f_max_glob
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

 g_pool[ name ].first = convex_combination;  // store the Solution
 g_pool[ name ].second = type;               // store the type

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

 delete g_pool[ name ].first;     // delete the Solution
 g_pool[ name ].first = nullptr;  // mark that the position is empty

 if( name == LastSolution )    // if this was the Solution in the inner Block
  LastSolution = g_pool.size();  // it is no longer valid

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
  if( LastSolution < Inf<Index>() )  // LastSolution was in the global pool
   LastSolution = g_pool.size();     // it is no longer valid

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

  if( i == LastSolution )    // if this was the Solution in the inner Block
   LastSolution = g_pool.size();  // it is no longer valid

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
 if( v_Block.empty() )  // there is no inner Block
  return( kError );     // that's clearly an error

 // no Solver is attached to the inner Block
 if( v_Block.front()->get_registered_solvers().empty() )
  return( kError );     // that's clearly an error

 // if required, check if b == 0 or not- - - - - - - - - - - - - - - - - - - -
 if( f_yb == INF ) {
  f_yb = -INF;  // b == 0 until otherwise proven
  for( auto const & lp : LagPairs )
   if( static_cast< p_LF >( lp.second )->get_constant_term() ) {
    f_yb = NaN; break;  // if b has nonzeros, yb need be recomputed
    }
  }

 // check what needs be updated- - - - - - - - - - - - - - - - - - - - - - - -
 if( changedvars ) {  // if the Lagrangian variables have changed
  f_dirty_Lc = true;  // Lagrangian costs c^y = c + yA need be recomputed
  if( ( ! std::isnan( f_yb ) ) && ( f_yb > -INF ) )
                      // unless b is known to be all-0
   f_yb = NaN;        // force to recompute the linear term yb
  }

 // if necessary, recompute the Lagrangian costs c^y = c + yA- - - - - - - - -
 if( f_dirty_Lc ) {
  // array of new Lagrangian costs c^y = c + yA
  Vec_FunctionValue NCoef( CostMatrix.size() );

  // compute the Lagrangian costs
  for( Index i = 0 ; i < CostMatrix.size() ; ++i ) {
   NCoef[ i ] = CostMatrix[ i ].first;
   for( const auto & el : CostMatrix[ i ].second )
    NCoef[ i ] += el.first->get_value() * el.second;
   }

  // try to lock the inner Block: if this does not work
  bool owned = v_Block.front()->is_owned_by( this );
  if( ( ! owned ) && ( ! v_Block.front()->lock( this ) ) )
   return( kError );     // that's clearly an error

  f_play_dumb = true;                // ignore any ensuing Modification

  // modify the coefficients in the LinearFunction
  obj->modify_coefficients( std::move( NCoef ) ,
			    Range( 0 , CostMatrix.size() ) );

  f_play_dumb = false;               // back to normal operations
  if( ! owned )
   v_Block.front()->unlock( this );  // unlock it

  f_dirty_Lc = false;                // Lagrangian costs are current
  }

 // if necessary, recompute the linear term- - - - - - - - - - - - - - - - - -
 if( std::isnan( f_yb ) ) {
  f_yb = 0;
  for( const auto & lp : LagPairs )
   f_yb += static_cast< p_LF >( lp.second )->get_constant_term() *
           lp.first->get_value();
  }
 
 // if some parameters have been changed, set BlockSolverConfig- - - - - - - -
 if( svcc ) {
  svcc->apply( v_Block.front() );
  delete svcc;
  svcc = nullptr;
  }

 // finally, compute() the inner Block - - - - - - - - - - - - - - - - - - - -
 // it is assumed that the inner Block (B) does not have Variable defined in
 // other Blocks: then, the re-optimization of (B) can be performed starting
 // from the old solution, i.e., compute( false ) can be called; this means
 // that in fact the Solver may not have to do anything because the inner
 // Block may not have changed (say, only b has), but this is left to the
 // Solver to properly check to avoid doing useless work

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

 auto alpha = obj->get_constant_term();

 const auto & rp = obj->get_v_var();
 #ifndef NDEBUG
  if( rp.size() != CostMatrix.size() )
   throw( std::logic_error( "CostMatrix inconsistent with obj" ) );
 #endif
 for( Index i = 0 ; i < rp.size() ; ++i )
  alpha += rp[ i ].first->get_value() * CostMatrix[ i ].first;

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

 }  // end( LagBFunction::get_MatDesc )

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
   return( C05Function::get_dflt_int_par( par ) );
  }
 }  // end( LagBFunction::get_int_par )

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
   return( C05Function::get_dflt_dbl_par( par ) );
  }
 }  // end( LagBFunction::get_dbl_par )

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
    throw( std::invalid_argument( "map_active: some Variable is not active"
				  ) );
   }
 else {
  auto it = map.begin();
  for( auto var : vars ) {
   Index i = LagBFunction::is_active( var );
   if( i >= LagPairs.size() )
    throw( std::invalid_argument( "map_active: some Variable is not active"
				  ) );
   *(it++) = i;
   }
  }
 }  // end( LagBFunction::map_active )

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

void LagBFunction::add_columns( v_dual_pair & v_LagPairsair )
{
 // given a new vector of pairs < y_i , g_i(x) >, that were not a part of
 // LagPairs already, update CostMatrix, which provides the information used
 // to compute the Lagrangian costs. the new g_i(x) may contain some Variable
 // x_j that is not in the Objective of the inner Block already, in which
 // case this is added (and CostMatrix grows by one row)

 LinearFunction::v_coeff_pair toadd;  // new ColVariable to add to obj

 for( const auto & lp : v_LagPairsair ) {
  // for each dual pair < y_i , g_i(x) >
  auto gi = dynamic_cast< p_LF >( lp.second );
  if( ! gi )
   throw( std::invalid_argument( "Lagrangian term not a LinearFunction" ) );

  const auto & rp = gi->get_v_var();

  for( const auto & monomial : rp ) {
   // for each Variable x_j in g_i(x), add the pair < y_i , a_{ij} > to
   // CostMatrix[ j ] (if it exists, otherwise create it)

   // construct the pair < y_i , a_{ij} > to be added to CostMatrix[ j ]
   const auto y_pair = std::make_pair( lp.first , monomial.second );

   // find the position of x_j in (obj_B)
   auto j = obj->is_active( monomial.first );

   if( j >= obj->get_num_active_var() ) {
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
     j = Inf<Index>();
     }
    else
     j = obj->get_num_active_var() + std::distance( toadd.begin() , it );
    }

   if( j < Inf<Index>() ) {
    // x_j was there already in CostMatrix, although possibly not in obj
    // find the place of < y_i , a_{ij} > in A_j
    auto it = std::lower_bound( CostMatrix[ j ].second.begin() ,
				CostMatrix[ j ].second.end() ,
				std::make_pair( lp.first , 0 ) ,
				[]( const auto & a , const auto & b )
	   	 		  { return( a.first < b.first ); } );
    // add < y_i , a_{ij} > to A_j
    CostMatrix[ j ].second.insert( it , y_pair );
    }
   }  // end( for( each monomial in g_i(x) ) )
  }  // end( for( each Lagrangian pair < y_i , g_i(x) > ) )- - - - - - - - - -

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
  const auto & rp =
     static_cast< const LinearFunction * >( l_pair.second )->get_v_var();

  for( const auto & monomial : rp ) {
   // construct the pair < y_i , a_{ij} > to be added to CostMatrix[ j ]
   const auto y_pair = std::make_pair( l_pair.first , monomial.second );

   // find the position of x_j in (obj_B)
   Index j = obj->is_active( monomial.first );

   if( j >= obj->get_num_active_var() ) {
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
     j = Inf<Index>();
     }
    else  // was already in toadd, hence in CostMatrix
     j = obj->get_num_active_var() + std::distance( toadd.begin() , it );
    }

   if( j < Inf<Index>() ) {
    // x_j was there already in CostMatrix, although possibly not in obj
    // find the place of < y_i , a_{ij} > in A_j
    auto it = std::lower_bound( CostMatrix[ j ].second.begin() ,
				 CostMatrix[ j ].second.end() ,
				 std::make_pair( l_pair.first , 0 ) ,
				 []( const LinearFunction::coeff_pair & a ,
				     const LinearFunction::coeff_pair & b )
	   	 		   { return( a.first < b.first ); } );

    if( it == CostMatrix[ j ].second.end() ) {
     // there is no term < y_i , a_{ij} > into A_j currently
     if( monomial.second )  // a_{ij} != 0: add < y_i , a_{ij} > to A_j
      CostMatrix[ j ].second.insert( it , y_pair );
     // else it was not there before and it is not created
     }
    else  // there is a term < y_i , a_{ij} > into A_j currently
     if( monomial.second )                  // a_{ij} != 0
      it->second = monomial.second;         // modify a_{ij}
     else {                                 // a_{ij} == 0
      CostMatrix[ j ].second.erase( it );   // remove the term
      if( CostMatrix[ j ].second.empty() )  // if A_j becomes empty 
       nms.push_back( j );                  // mark it for deletion
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
 }  // end( LagBFunction::update_columns )

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

void LagBFunction::guts_of_destructor( void )
{
 // clear() all the LagBFunction - - - - - - - - - - - - - - - - - - - - - - -

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
    set_original_costs( Subset() );
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
     f_yb = NaN;  // since b_i has changed, the linear term has

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
