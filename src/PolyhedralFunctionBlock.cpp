/*--------------------------------------------------------------------------*/
/*------------------- File PolyhedralFunctionBlock.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the PolyhedralFunctionBlock class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "PolyhedralFunctionBlock.h"

#include "AbstractBlock.h"
#include "BlockSolverConfig.h"
#include "ColVariable.h"
#include "FRealObjective.h"
#include "LinearFunction.h"

#include <algorithm>
#include <cmath>
#include <list>
#include <unordered_set>

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register PolyhedralFunctionBlock to the Block factory

SMSpp_insert_in_factory_cpp_1( PolyhedralFunctionBlock );

/*--------------------------------------------------------------------------*/
/*------------------------- BIT MASKS FOR f_rep ----------------------------*/
/*--------------------------------------------------------------------------*/
/* The "representation" character f_rep is bit-encoded. The two lowest bits
 * (k_rep_type_mask) tell *which* representation has been requested:
 *
 *   bit 0 (k_rep_abstract): 0 == "natural representation",
 *                           1 == one of the "abstract" representations
 *   bit 1 (k_rep_dual):     only meaningful when bit 0 == 1, selects
 *                           between linearized primal (0) and linearized
 *                           dual (1)
 *
 * The remaining bits track which slice of the abstract representation has
 * been constructed already, so that the build can be done in steps. */

static constexpr char k_rep_abstract = 0x1;   // bit 0
static constexpr char k_rep_dual     = 0x2;   // bit 1
static constexpr char k_rep_type_mask = k_rep_abstract | k_rep_dual;

static constexpr char k_built_var    = 0x4;   // bit 2
static constexpr char k_built_cnst   = 0x8;   // bit 3
static constexpr char k_built_obj    = 0x10;  // bit 4

/* The SimpleConfiguration< int > passed to generate_abstract_variables()
 * uses the next two bits for scaling and the third one for disabling the
 * objective function. They are stored separately from f_rep because bits
 * 2 and above of f_rep are construction-state flags. */

static constexpr int k_cfg_scale_local  = 0x4;  // bit 2
static constexpr int k_cfg_scale_global = 0x8;  // bit 3
static constexpr int k_cfg_no_objective = 0x10; // bit 4

// A global rescaling updates every scale-dependent coefficient of the
// abstract representation. Keep a wide hysteresis band so that this
// remains an occasional batch operation.
static constexpr double k_global_rescale_ratio = 100.0;

/*--------------------------------------------------------------------------*/
/*----------------- METHODS of PolyhedralFunctionBlock ---------------------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::generate_abstract_variables(
						        Configuration * stvv )
{
 if( f_rep & k_built_var )  // done already
  return;                   // nothing else to do

 // figure out the requested representation; only the lowest two bits of
 // the int are read (cf. the bit-mask comments at the top of the file)
 int wsol = 0;
 auto tstvv = dynamic_cast< SimpleConfiguration< int > * >( stvv );

 if( ( ! tstvv ) && f_BlockConfig &&
     f_BlockConfig->f_static_variables_Configuration )
  tstvv = dynamic_cast< SimpleConfiguration< int > * >(
			    f_BlockConfig->f_static_variables_Configuration );
 if( tstvv )
  wsol = tstvv->f_value;

 // record only the representation bits, not the build-status ones
 f_rep |= ( wsol & k_rep_type_mask );
 f_scaling = ( ( wsol & k_cfg_scale_local ) ? 1 : 0 ) |
             ( ( wsol & k_cfg_scale_global ) ? 2 : 0 );
 f_no_objective = ( wsol & k_cfg_no_objective ) != 0;
 InitialiseScaling();

 if( is_linearized() ) {
  // The physical epigraph variable "v" is always exposed as the first
  // static ColVariable. With global scaling, cuts use a second internal
  // variable linked to it by generate_abstract_constraints().
  f_1st_stat_var = has_global_scaling() ? 2 : 1;
  if( has_global_scaling() )
   add_static_variable( f_scaled_v , "PolyF_scaled_v" , true );
  add_static_variable( f_v , "PolyF_v" , true );
  }
 else
  if( is_dual() ) {
   // linearized dual: gamma is a single non-negative ColVariable;
   // if no bound is set it is fixed to 0 so it contributes nothing to
   // either the normalization constraint or the objective
   f_gamma.is_positive( true , eNoMod );
   if( ! PF().is_bound_set() ) {
    f_gamma.set_value( 0 );
    f_gamma.is_fixed( true , eNoMod );
    }
   // gamma is added as the first static ColVariable, so that further
   // derived classes coming after cannot displace it
   f_1st_stat_var = 1;
   add_static_variable( f_gamma , "PolyF_gamma" , true );

   // theta_i: one non-negative ColVariable per row of PF()
   // (both diagonal and vertical, in the same order). It is a *dynamic*
   // list because the rows of PF() can be added/removed
   f_theta.clear();
   const Index nr = PF().get_A().size();
   for( Index i = 0 ; i < nr ; ++i ) {
    f_theta.emplace_back();
    f_theta.back().is_positive( true , eNoMod );
    }
   f_1st_dyn_var = 1;
   add_dynamic_variable( f_theta , "PolyF_theta" , true );
   }
 // else (natural): nothing to do here

 f_rep |= k_built_var;

 }  // end( PolyhedralFunctionBlock::generate_abstract_variables )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::generate_abstract_constraints(
						        Configuration * stcc )
{
 if( f_rep & k_built_cnst )  // done already
  return;                    // nothing else to do

 if( ! ( f_rep & k_built_var ) )  // variables not constructed
  throw( std::logic_error( "Variable must be generated before Constraint" ) );

 if( is_linearized() ) {
  // linearized primal: bounds on physical v + optional scale link + cuts
  if( has_global_scaling() ) {
   f_scale_cns.set_lhs( 0.0 , eNoMod );
   f_scale_cns.set_rhs( 0.0 , eNoMod );
   f_scale_cns.set_function( new LinearFunction(
                 { std::make_pair( & f_scaled_v , 1.0 ) ,
                   std::make_pair( & f_v , - f_global_scale ) } ) , eNoMod );

   // Add the link first: f_bcv is added in front immediately afterwards,
   // preserving the historical convention that the box is group 0.
   add_static_constraint( f_scale_cns , "PolyF_v_scale" , true );
   }

  // add the bounds on the physical epigraph variable
  f_bcv.set_variable( &f_v );
  f_bcv.set_rhs( PF().get_global_upper_bound() , eNoMod );
  f_bcv.set_lhs( PF().get_global_lower_bound() , eNoMod );

  // note: the bounds on v are added "in front"
  f_1st_stat_cnst = has_global_scaling() ? 2 : 1;
  add_static_constraint( f_bcv , "" , true );

  // add the linear constraints
  f_const.resize( PF().get_A().size() );
  auto cit = f_const.begin();
  for( Index i = 0 ; i < PF().get_A().size() ; )
   ConstructLPConstraint( i++ , *(cit++) );

  // note: the linear constraints are added "in front"
  f_1st_dyn_cnst = 1;
  add_dynamic_constraint( f_const , "" , true );
  }
 else
  if( is_dual() ) {
   // linearized dual: the single static normalization constraint
   //   sum_{i in B_D} local_scale_i theta_i + gamma = 1 / global_scale
   // (after set_lambda(lambda), lambda is appended with coefficient
   // -1 / global_scale to the LHS LinearFunction and the RHS is set to 0;
   // cf. set_lambda())
   LinearFunction::v_coeff_pair vp;
   vp.reserve( 1 + PF().get_A().size() );

   // gamma always appears with coefficient 1, even when it is fixed to 0
   vp.emplace_back( & f_gamma , 1.0 );

   // each non-vertical theta_i appears with its local row scale
   auto thit = f_theta.begin();
   for( Index i = 0 ; i < PF().get_A().size() ; ++i , ++thit )
    if( ! PF().is_row_vertical( i ) )
     vp.emplace_back( & *thit , RowScale( i ) );

   f_normcns.set_lhs( 1.0 / f_global_scale , eNoMod );
   f_normcns.set_rhs( 1.0 / f_global_scale , eNoMod );
   f_normcns.set_function( new LinearFunction( std::move( vp ) ) , eNoMod );

   // the normalization is added as the first static constraint
   f_1st_stat_cnst = 1;
   add_static_constraint( f_normcns , "PolyF_norm" , true );
   }
 // else (natural): nothing to do here

 f_rep |= k_built_cnst;

 }  // end( PolyhedralFunctionBlock::generate_abstract_constraints )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::generate_objective( Configuration * objc )
{
 if( f_rep & k_built_obj )  // done already
  return;                   // nothing else to do

 // the configuration specified that no objective is to be constructed
 if( f_no_objective ) {
  f_rep |= k_built_obj;
  return;
  }

 if( ! ( f_rep & k_built_var ) )  // variables not constructed
  throw( std::logic_error( "Variable must be generated before Objective" ) );

 f_res_obj = true;  // in any representation the objective is "reserved"

 auto obj = new FRealObjective();

 // For the natural / linearized-primal representations the
 // objective sense is the "natural" verse of PF() (min for convex,
 // max for concave). For the linearized-dual representation it is the
 // *opposite* verse: the dual LP is a max-problem when the primal is a
 // min (convex case), and a min-problem when the primal is a max
 // (concave case). With this choice, primal and dual problems have the
 // same numerical optimum, so the test harness can compare them
 // directly.
 const bool convex = PF().is_convex();
 const bool dual_min = ! convex;  // dual sense = opposite of primal
 obj->set_sense( ( is_dual() ? dual_min : convex )
                 ? FRealObjective::eMin : FRealObjective::eMax , eNoMod );

 if( is_linearized() )
  obj->set_function( new LinearFunction(
                     { std::make_pair( & f_v , 1.0 ) } ) );
 else
  if( is_dual() ) {
   // Build the dual objective. By LP duality (cf. ConstructLPConstraint
   // for the primal row form), in both convex and concave cases the dual
   // objective has POSITIVE b_i as coefficient on each theta_i and positive
   // bound (LB for convex, UB for concave) on gamma; only the sense flips:
   //   convex : maximize  +sum_i theta_i b_i + gamma * LB
   //   concave: minimize  +sum_i theta_i b_i + gamma * UB
   // (the sense is already set above)
   LinearFunction::v_coeff_pair vp;
   const Index nr = PF().get_A().size();
   vp.reserve( 1 + nr );

   // gamma * bound; if no bound is set, gamma is fixed to 0 and the bound
   // returned by get_global_bound() may be +/- INF: use 0 as the coefficient
   // in that case so that no INF * 0 ever appears
   const double bnd = PF().is_bound_set()
                      ? PF().get_global_bound()
                      : 0.0;
   vp.emplace_back( & f_gamma , f_global_scale * bnd );

   // + theta_i b_i for every row of PF() (diagonal AND vertical)
   auto thit = f_theta.begin();
   for( Index i = 0 ; i < nr ; ++i , ++thit )
    vp.emplace_back( & *thit , ScaledRowFactor( i ) * PF().get_b()[ i ] );

   obj->set_function( new LinearFunction( std::move( vp ) ) );
   }
  else
   obj->set_function( & PF() );  // natural representation

 set_objective( obj , eNoMod );

 f_rep |= k_built_obj;

 }  // end( PolyhedralFunctionBlock::generate_objective )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::set_lambda( ColVariable * lambda )
{
 // sanity checks
 if( ! is_dual() )
  throw( std::logic_error(
            "set_lambda() requires the dual representation" ) );
 if( ! ( f_rep & k_built_cnst ) )
  throw( std::logic_error(
       "set_lambda() must be called after generate_abstract_constraints()" ) );
 if( ! lambda )
  throw( std::invalid_argument( "set_lambda(): nullptr lambda" ) );

 f_lambda = lambda;

 // The normalization constraint built by generate_abstract_constraints
 // is the stand-alone scaled simplex
 //
 //     sum_{i in B_D} local_scale_i theta_i + gamma_local
 //       = 1 / global_scale
 //
 // where gamma_local is the per-PFB f_gamma. When this PFB is embedded
 // as one sub-Block of a larger dual master, the master owns a single
 // shared non-negative dual variable lambda (the multiplier of the
 // model-value equation of the lower model); the per-PFB row that
 // consistently states stationarity at v_k is then
 //
 //     sum_{i in B_D} local_scale_i theta_i^k + gamma_local^k
 //       - lambda / global_scale = 0
 //
 // and ties the per-component theta + gamma mass to the global lambda.
 // set_lambda() therefore appends lambda with coefficient
 // -1 / global_scale to
 // f_normcns and pulls LHS / RHS to 0; gamma_local is left untouched
 // (its coefficient stays +1 in the normalization row and its sign in
 // the Objective is the per-PFB lower bound).

 auto lf = static_cast< LinearFunction * >( f_normcns.get_function() );
 if( ! lf )
  throw( std::logic_error(
            "set_lambda(): normalization constraint not initialized" ) );

 // build a new LinearFunction with lambda appended (skipping the append
 // if lambda is already present, e.g. because of a re-invocation).
 LinearFunction::v_coeff_pair new_vp;
 const auto & old_vp = lf->get_v_var();
 new_vp.reserve( old_vp.size() + 1 );
 bool already_present = false;
 for( const auto & p : old_vp ) {
  if( p.first == lambda )
   already_present = true;
  new_vp.emplace_back( p );
  }
 if( ! already_present )
  new_vp.emplace_back( lambda , -1.0 / f_global_scale );

 f_normcns.set_function( new LinearFunction( std::move( new_vp ) ) , eNoMod );

 // lift the RHS = 1 / global_scale set by
 // generate_abstract_constraints to RHS = 0,
 // since lambda now plays the role of "shifted normalization mass"
 f_normcns.set_lhs( 0.0 , eNoMod );
 f_normcns.set_rhs( 0.0 , eNoMod );

 }  // end( set_lambda )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::set_conjugate_constraint(
                                  std::list< FRowConstraint > & constraints )
{
 // sanity checks
 if( ! is_dual() )
  throw( std::logic_error(
   "set_conjugate_constraint() requires the dual representation" ) );
 if( ! ( f_rep & k_built_var ) )
  throw( std::logic_error( "set_conjugate_constraint() must be called "
                           "after generate_abstract_variables()" ) );

 const Index nv = PF().get_num_active_var();
 if( constraints.size() != nv )
  throw( std::invalid_argument( "set_conjugate_constraint(): the size of "
                                "the provided constraint list does not "
                                "match get_num_active_var()" ) );

 // remember the list of coupling constraints so that the
 // add_Modification machinery can keep them in sync with PF()
 f_coupling = & constraints;

 const Index nr = PF().get_A().size();
 const auto & A = PF().get_A();

 // build, for each j, the list of ( theta_i , A[i][j] ) pairs that this
 // PolyhedralFunctionBlock contributes to the j-th coupling constraint
 std::vector< LinearFunction::v_coeff_pair > contribs( nv );

 auto thit = f_theta.begin();
 for( Index i = 0 ; i < nr ; ++i , ++thit )
  for( Index j = 0 ; j < nv ; ++j )
   if( A[ i ][ j ] != 0 )
    contribs[ j ].emplace_back( & *thit ,
                                ScaledRowFactor( i ) * A[ i ][ j ] );

 // attach the contributions to each external constraint. The
 // constraint may already have an empty LinearFunction (which the
 // caller created so that the constraint already has a valid Function
 // when this method runs) or it may have an existing one with some
 // variables in it (e.g. populated by another PolyhedralFunctionBlock
 // sharing the same coupling list). In either case we *replace* the
 // LinearFunction with a new one built from "old contents + new
 // contributions", since add_variables() with issueMod = eNoMod does
 // not update the parent FRowConstraint's active-Variable list and
 // therefore MILPSolver does not see the freshly-added Variables
 // (whereas set_function() does invalidate and rebuild the list).
 Index j = 0;
 for( auto cit = constraints.begin() ; cit != constraints.end() ;
      ++cit , ++j ) {
  if( contribs[ j ].empty() )
   continue;

  LinearFunction::v_coeff_pair merged;

  // preserve any pre-existing terms in the constraint
  if( auto old_lf = static_cast< LinearFunction * >( cit->get_function() ) ) {
   const auto & old_vp = old_lf->get_v_var();
   merged.reserve( old_vp.size() + contribs[ j ].size() );
   merged.insert( merged.end() , old_vp.begin() , old_vp.end() );
   }
  else
   merged.reserve( contribs[ j ].size() );

  // append our own contributions
  merged.insert( merged.end() ,
                 std::make_move_iterator( contribs[ j ].begin() ) ,
                 std::make_move_iterator( contribs[ j ].end() ) );

  // install the new LinearFunction in the constraint (this also takes
  // care of registering the active Variable through the constraint's
  // own machinery)
  cit->set_function( new LinearFunction( std::move( merged ) ) , eNoMod );
  }

 }  // end( set_conjugate_constraint )

/*--------------------------------------------------------------------------*/
/*------- Methods for reading the data of the PolyhedralFunctionBlock ------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

Function::FunctionValue
PolyhedralFunctionBlock::get_row_multiplier( Index i ) const
{
 if( i >= PF().get_A().size() )
  throw( std::out_of_range( "row index out of range" ) );

 if( is_linearized() ) {
  if( ! ( f_rep & k_built_cnst ) )
   throw( std::logic_error(
           "get_row_multiplier() requires generated abstract constraints" ) );
  auto cit = f_const.cbegin();
  std::advance( cit , i );
  return( - ScaledRowFactor( i ) * cit->get_dual() );
  }

 if( is_dual() ) {
  if( ! ( f_rep & k_built_var ) )
   throw( std::logic_error(
             "get_row_multiplier() requires generated abstract variables" ) );
  auto vit = f_theta.cbegin();
  std::advance( vit , i );
  return( ScaledRowFactor( i ) * vit->get_value() );
  }

 throw( std::logic_error(
            "get_row_multiplier() requires a linearized representation" ) );

 }  // end( PolyhedralFunctionBlock::get_row_multiplier )

/*--------------------------------------------------------------------------*/
/*------------------------- Methods for R3 Blocks --------------------------*/
/*--------------------------------------------------------------------------*/

Block * PolyhedralFunctionBlock::get_R3_Block( Configuration *r3bc ,
					       Block * base , Block * father )
{
 if( r3bc != nullptr )
  throw( std::invalid_argument( "non-nullptr R3B Configuration" ) );

 PolyhedralFunctionBlock *PFB;
 if( base ) {
  PFB = dynamic_cast< PolyhedralFunctionBlock * >( base );
  if( ! PFB )
   throw( std::invalid_argument( "base is not a PolyhedralFunctionBlock" ) );
  }
 else
  PFB = new PolyhedralFunctionBlock( father );

 PFB->PF().set_PolyhedralFunction( MultiVector( PF().get_A() ) ,
				      RealVector( PF().get_b() ) ,
				      PF().get_global_bound() ,
				      PF().is_convex() , eNoMod ,
				      PolyhedralFunction::BoolVector(
					PF().get_is_vert() ) );
 return( PFB );

 }  // end( MCFBlock::get_R3_Block )

/*--------------------------------------------------------------------------*/

bool PolyhedralFunctionBlock::map_forward_Modification(
			   Block * R3B , c_p_Mod mod , Configuration * r3bc ,
			   ModParam issuePMod , ModParam issueAMod )
{
 if( mod->concerns_Block() )  // an abstract Modification
  return( false );            // none of my business

 auto PFB = dynamic_cast< PolyhedralFunctionBlock * >( R3B );
 if( ! PFB )
  throw( std::invalid_argument( "R3B is not a PolyhedralFunctionBlock" ) );
 if( r3bc != nullptr )
  throw( std::invalid_argument( "non-nullptr R3B Configuration" ) );

 /* Note that issueAMod is completely ignored because we only perform
    physical Modification; the "translation" to "abstract" Modification, if
    ever, will be done by PFB->add_Modification() when it receives the
    physical one generated here. */

 /* Use a Lambda to define a "guts" of the method that can be called
    recursively without having to pass "local globals". Note the trick of
    defining the std::function object and "passing" it to the lambda,
    which allows recursive calls. Note the need to explicitly capture
    "this" to use fields/methods of the class. */

 std::function< bool( c_p_Mod , ModParam ) > guts_of_mfM;
 guts_of_mfM = [ this , & guts_of_mfM , & PFB ]( c_p_Mod mod ,
						 ModParam iPM ) {
  // process Modification- - - - - - - - - - - - - - - - - - - - - - - - - - -
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  /* This requires to patiently sift through the possible Modification types
     to find what this Modification exactly is, and call the appropriate
     method changing the "physical representation" of PFB. */

  // GroupModification - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const GroupModification * >( mod ) ) {
   auto niPM = make_par( par2mod( iPM ) ,
			 PFB->open_channel( par2chnl( iPM ) ) );
   bool ok = true;
   for( const auto & submod : tmod->sub_Modifications() )  // for each sub-Mod
    if( ! guts_of_mfM( submod.get() , niPM ) )             // make the call
     ok = false;

   PFB->close_channel( par2chnl( niPM ) );  // close it
   return( ok );
   }

  // PolyhedralFunctionModAddd - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const PolyhedralFunctionModAddd * >( mod ) ) {
   if( tmod->function() != & PF() )  // not my PolyhedralFunction
    return( false );                    // none of my business

   Index nr = PF().get_A().size();
   MultiVector nA( tmod->addedrows() );
   RealVector nb( tmod->addedrows() );
   PolyhedralFunction::BoolVector niV;  // empty unless some are vertical
   Index j = 0;
   for( Index i = nr - tmod->addedrows() ; i < nr ; ) {
    nA[ j ] = PF().get_A()[ i ];
    nb[ j ] = PF().get_b()[ i ];
    if( PF().is_row_vertical( i ) ) {
     if( niV.empty() )
      niV.assign( tmod->addedrows() , false );
     niV[ j ] = true;
     }
    ++j; ++i;
    }

   PFB->PF().add_rows( std::move( nA ) , nb , iPM , std::move( niV ) );
   return( true );
   }

  // PolyhedralFunctionModRngd - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const PolyhedralFunctionModRngd * >( mod ) ) {
   if( tmod->function() != & PF() )  // not my PolyhedralFunction
    return( false );                    // none of my business

   Index n = tmod->range().second - tmod->range().first;
   switch( tmod->PFtype() ) {
    case( PolyhedralFunctionMod::ModifyRows ):
     if( n == 1 )
      PFB->PF().modify_row(
		    tmod->range().first ,
		    RealVector( PF().get_A()[ tmod->range().first ] ) ,
		    PF().get_b()[ tmod->range().first ] , iPM ,
		    PF().is_row_vertical( tmod->range().first ) );
     else {
      MultiVector nA( n );
      RealVector nb( n );
      PolyhedralFunction::BoolVector niV;  // empty unless some are vertical
      Index j = 0;
      for( Index i = tmod->range().first ; i < tmod->range().second ; ) {
       nA[ j ] = PF().get_A()[ i ];
       nb[ j ] = PF().get_b()[ i ];
       if( PF().is_row_vertical( i ) ) {
	if( niV.empty() )
	 niV.assign( n , false );
	niV[ j ] = true;
	}
       ++j; ++i;
       }

      PFB->PF().modify_rows( std::move( nA ) , std::move( nb ) ,
				tmod->range() , iPM , std::move( niV ) );
      }
     break;
    case( PolyhedralFunctionMod::ModifyCnst ):
     if( n == 0 ) {
      PFB->PF().modify_bound(  PF().get_global_bound() , iPM );
      break;
      }
       
     if( n == 1 )
      PFB->PF().modify_constant( tmod->range().first ,
				    PF().get_b()[ tmod->range().first ] ,
				    iPM );
     else {
      RealVector nb( n );
      auto bit = nb.begin();
      for( Index i = tmod->range().first ; i < tmod->range().second ; )
       *(bit++) = PF().get_b()[ i++ ];

      PFB->PF().modify_constants( std::move( nb ) , tmod->range() , iPM );
      }
     break;
    case( PolyhedralFunctionMod::DeleteRows ):
     if( n == 1 )
      PFB->PF().delete_row( tmod->range().first , iPM );
     else
      PFB->PF().delete_rows( tmod->range() , iPM );
     break;
    default:
     throw( std::invalid_argument(
			      "unknown PolyhedralFunctionModRngd PFtype" ) );
    }
   return( true );
   }

  // PolyhedralFunctionModSbst - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const PolyhedralFunctionModSbst * >( mod ) ) {
   if( tmod->function() != & PF() )  // not my PolyhedralFunction
    return( false );                    // none of my business

   Index n = tmod->rows().size();
   switch( tmod->PFtype() ) {
    case( PolyhedralFunctionMod::ModifyRows ):
     if( n == 1 )
      PFB->PF().modify_row(
		    tmod->rows()[ 0 ] ,
		    RealVector( PF().get_A()[ tmod->rows()[ 0 ] ] ) ,
		    PF().get_b()[ tmod->rows()[ 0 ] ] , iPM ,
		    PF().is_row_vertical( tmod->rows()[ 0 ] ) );
     else {
      MultiVector nA( n );
      RealVector nb( n );
      PolyhedralFunction::BoolVector niV;
      Index j = 0;
      for( auto i : tmod->rows() ) {
       nA[ j ] = PF().get_A()[ i ];
       nb[ j ] = PF().get_b()[ i ];
       if( PF().is_row_vertical( i ) ) {
	if( niV.empty() )
	 niV.assign( n , false );
	niV[ j ] = true;
	}
       ++j;
       }

      PFB->PF().modify_rows( std::move( nA ) , std::move( nb ) ,
				Subset( tmod->rows() ) , true , iPM ,
				std::move( niV ) );
      }
     break;
    case( PolyhedralFunctionMod::ModifyCnst ):
     if( n == 1 )
      PFB->PF().modify_constant( tmod->rows()[ 0 ] ,
				    PF().get_b()[ tmod->rows()[ 0 ] ] ,
				    iPM );
     else {
      RealVector nb( n );
      auto bit = nb.begin();
      for( auto i : tmod->rows() )
       *(bit++) = PF().get_b()[ i ];
       
      PFB->PF().modify_constants( std::move( nb ) ,
				     Subset( tmod->rows() ) , true , iPM );
      }
     break;
    case( PolyhedralFunctionMod::DeleteRows ):
     if( n == 1 )
      PFB->PF().delete_row( tmod->rows()[ 0 ] , iPM );
     else
      PFB->PF().delete_rows( Subset( tmod->rows() ) , true , iPM );
     break;
    default:
     throw( std::invalid_argument(
			      "unknown PolyhedralFunctionModRngd PFtype" ) );
    }
   return( true );
   }

  // C05FunctionModVarsAddd- - - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const C05FunctionModVarsAddd * >( mod ) ) {
   if( tmod->function() != & PF() )  // not my PolyhedralFunction
    return( false );                    // none of my business

   return( true );  // pretend we have done it, which is impossible
                    // see comments for rationale
   }

  // C05FunctionModVarsRngd- - - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const C05FunctionModVarsRngd * >( mod ) ) {
   if( tmod->function() != & PF() )  // not my PolyhedralFunction
    return( false );                    // none of my business

   if( tmod->range().second == tmod->range().first + 1 )
    PFB->PF().remove_variable( tmod->range().first , iPM );
   else
    PFB->PF().remove_variables( tmod->range() , iPM );

   return( true );
   }

  // C05FunctionModVarsSbst- - - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const C05FunctionModVarsSbst * >( mod ) ) {
   if( tmod->function() != & PF() )  // not my PolyhedralFunction
    return( false );                    // none of my business

   PFB->PF().remove_variables( Subset( tmod->subset() ) , iPM );     
   return( true );
   }

  // PolyhedralFunctionMod - - - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const PolyhedralFunctionMod * >( mod ) ) {
   if( tmod->function() != & PF() )  // not my PolyhedralFunction
    return( false );                    // none of my business

   if( tmod->type() != C05FunctionMod::NothingChanged )
    throw( std::invalid_argument( "unexpected type() in C05FunctionMod" ) );

   PFB->PF().set_is_convex( PF().is_convex() , iPM );
     
   return( true );
   }

  // FunctionMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const FunctionMod * >( mod ) ) {
   // "nuclear Modification for Function": everything changed

   if( tmod->function() != & PF() )  // not my PolyhedralFunction
    return( false );                    // none of my business

   if( ! std::isnan( tmod->shift() ) )
    throw( std::invalid_argument( "unexpected shift() in FunctionMod" ) );

   PFB->PF().set_PolyhedralFunction( MultiVector( PF().get_A() ) ,
					RealVector( PF().get_b() ) ,
					PF().get_global_bound() ,
					PF().is_convex() , iPM ,
					PolyhedralFunction::BoolVector(
					  PF().get_is_vert() ) );
   return( true );
   }

  return( false );

  };  // end( guts_of_mfM )- - - - - - - - - - - - - - - - - - - - - - - - - -
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // finally, call the "guts of"- - - - - - - - - - - - - - - - - - - - - - - -
 return( guts_of_mfM( mod , issuePMod ) );

 }  // end( PolyhedralFunctionBlock::map_forward_Modification )

/*--------------------------------------------------------------------------*/

bool PolyhedralFunctionBlock::map_back_Modification(
			      Block *R3B , c_p_Mod mod , Configuration *r3bc ,
			      ModParam issuePMod , ModParam issueAMod )
{
 /* Fantastically dirty trick: because the two objects are copies, mapping
    back a Modification to this from R3B is the same as mapping forward a
    Modification from R3B to this. */

 auto PFB = dynamic_cast< PolyhedralFunctionBlock * >( R3B );
 if( ! PFB )
  throw( std::invalid_argument( "R3B is not a PolyhedralFunctionBlock" ) );

 return( PFB->map_forward_Modification( this , mod , r3bc , issuePMod ,
					issueAMod ) );

 }  // end( PolyhedralFunctionBlock::map_back_Modification )

/*--------------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------- METHODS FOR PRINTING & SAVING THE PolyhedralFunctionBlock --------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::print( std::ostream & output , char vlvl ) const
{
 output << std::endl << "PolyhedralFunctionBlock[";
 if( is_dual() )
  output << "d/";
 else
  if( is_linearized() )
   output << "l/";
  else
   output << "n/";
  /**
   * can't do anymore since is_convex no longer works on const functions
  if( PF().is_convex() ) {
   output << "cvx";
  }
  else{
   output << "cnc";
  }*/

 output << "] with PolyhedralFunction( " << PF().get_num_active_var()
	<< ", " << PF().get_A().size() << " )" << std::endl;

 if( vlvl ) {
  for( Index i = 0 ; i < PF().get_A().size()  ; ++i ) {
   output << "A[ " << i << " ] = [ ";
   for( Index j = 0 ; j < PF().get_num_active_var() ; ++j )
    output << PF().get_A()[ i ][ j ] << " ";
   output << "], b[ " << i << " ] = " << PF().get_b()[ i ] << std::endl;
   }

  /*!! can't do as get_global_*_bound() are not const
  if( PF().is_bound_set() ) {
   if( PF().is_convex() )
    output << "LB = " << PF().get_global_lower_bound();
   else
    output << "UB = " << PF().get_global_upper_bound();

   output << std::endl;
   }
   !!*/
  }

 AbstractBlock::print( output );

 }  // end( PolyhedralFunctionBlock::print )

/*--------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------------------- PRIVATE METHODS ------------------------------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::guts_of_destructor( void )
{
 // clear the Objective (if any)
 auto obj = static_cast< FRealObjective * >( get_objective() );
 if( obj )
  obj->clear();

 if( is_linearized() ) {  // linearized primal representation
  // first clear() all the constraints
  Constraint::clear( f_const );
  f_bcv.clear();
  f_scale_cns.clear();

  // then nothing, they will be deleted when f_const/f_bcv are

  // ensure that the LinearFunction inside the Objective is deleted
  if( obj )
   obj->set_function( nullptr , eNoMod , true );
  }
 else
  if( is_dual() ) {  // linearized dual representation
   // clear the normalization constraint (its LinearFunction is owned
   // by f_normcns and will be deleted with it)
   f_normcns.clear();

   // the f_theta dynamic Variable and the static f_gamma ColVariable
   // are owned by this PolyhedralFunctionBlock and will be destroyed
   // along with it; no explicit clear is needed (ColVariable has no
   // clear()), but f_theta gets emptied to release the storage now
   f_theta.clear();

   // ensure that the LinearFunction inside the Objective is deleted
   if( obj )
    obj->set_function( nullptr , eNoMod , true );
   }
  else {             // natural representation
   // ensure that the PolyhedralFunction inside the Objective is NOT
   // deleted (it is PF(), which lives on)
   if( obj )
    obj->set_function( nullptr , eNoMod , false );
   }

 // finally delete the Objective
 delete obj;

 }  // end( PolyhedralFunctionBlock::guts_of_destructor )

/*--------------------------------------------------------------------------*/

bool PolyhedralFunctionBlock::guts_of_add_Modification_PF(
				    const FunctionMod * mod , ChnlName chnl )
{
 // process a FunctionMod produced by the PolyhedralFunction- - - - - - - - -
 /* This requires to patiently sift through the possible Modification types
  * (but only those derived from FunctionMod) to find what this Modification
  * exactly is, and appropriately mirror the changes to the PolyhedralFunction
  * (which in this case counts as the "physical representation") into the
  * "abstract" one, i.e., performing the corresponding changes on the LP. */

 // C05FunctionModVarsAddd- - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod =  dynamic_cast< const C05FunctionModVarsAddd * >( mod ) ) {
  c_Index frst = tmod->first();
  c_Index nav = PF().get_num_active_var();

  // open a new GroupModification, not concerning PolyhedralFunctionBlock
  auto par = open_if_needed( make_par( eNoBlck , chnl ) , f_const.size() );

  Index i = 0;
  for( auto & ci : f_const ) {
   const auto scale = ScaledRowFactor( i );
   LinearFunction::v_coeff_pair vars( nav - frst );
   auto vit = vars.begin();
   auto Aiit = PF().get_A()[ i++ ].begin(); 
   for( Index j = frst ; j < nav ; ++j )
    *(vit++) = std::make_pair( static_cast< ColVariable * >(
					     PF().get_active_var( j ) ) ,
			       - scale * *(Aiit++) );
   static_cast< LinearFunction * >( ci.get_function() )->
                                   add_variables( std::move( vars ) , par );
   }

  close_if_needed( par , f_const.size() );
  return( false );
  }

 // C05FunctionModVarsRngd- - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const C05FunctionModVarsRngd * >( mod ) ) {
  // this is "remove Variables, ranged"
  auto rng = tmod->range();
  rng.first++;   // variables names in the constraints are +1 w.r.t. those
  rng.second++;  // of the PolyhedralFunction

  // open a new GroupModification, not concerning PolyhedralFunctionBlock
  auto par = open_if_needed( make_par( eNoBlck , chnl ) , f_const.size() );

  for( auto & ci : f_const )
   static_cast< LinearFunction * >( ci.get_function() )->
                                             remove_variables( rng , par );
  close_if_needed( par , f_const.size() );
  return( false );
  }

 // C05FunctionModVarsSbst- - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const C05FunctionModVarsSbst * >( mod ) ) {
  // this is "remove Variables, subset"
  Subset sbst( tmod->subset() );
  for( auto & si : sbst )  // variables names in the constraints are +1
   si++;                   // w.r.t. those of the PolyhedralFunction

  // open a new GroupModification, not concerning PolyhedralFunctionBlock
  auto par = open_if_needed( make_par( eNoBlck , chnl ) , f_const.size() );

  for( auto & ci : f_const )
   static_cast< LinearFunction * >( ci.get_function() )->
              remove_variables( std::move( Subset( sbst ) ) , true , par );

  close_if_needed( par , f_const.size() );
  return( false );
  }

 // PolyhedralFunctionModRngd - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const PolyhedralFunctionModRngd * >( mod ) ) {
  // this is "modify/delete a range of rows"
  Index strt = tmod->range().first;
  Index stop = tmod->range().second;

  if( strt == stop ) {  // special case: the lower/upper bound
   if( PF().is_convex() )  // convex ==> lower bound
    f_bcv.set_lhs( PF().get_global_bound() ,
                   make_par( eNoBlck , chnl ) );
   else                       // concave ==> upper bound
    f_bcv.set_rhs( PF().get_global_bound() ,
                   make_par( eNoBlck , chnl ) );
   return( false );
   }

  // open a new GroupModification, not concerning PolyhedralFunctionBlock
  // unless it's deleting or only one row and *not* also its constant
  Index nc = tmod->PFtype() == PolyhedralFunctionMod::DeleteRows ? 0 :
             ( tmod->PFtype() == PolyhedralFunctionMod::ModifyCnst ? 2 :
	       stop - strt );
  if( tmod->PFtype() == PolyhedralFunctionMod::DeleteRows ) {
   // delete rows
   DeleteRowScaling( tmod->range() );
   }
  else {
   RefreshRowMeasure( tmod->range() );
   if( RescaleGlobalIfNeeded( chnl ) )
    return( false );
   }

  auto par = open_if_needed( make_par( eNoBlck , chnl ) , nc );

  if( tmod->PFtype() == PolyhedralFunctionMod::DeleteRows ) {
   remove_dynamic_constraints( f_const , tmod->range() , par );
   RescaleGlobalIfNeeded( chnl );
   }
  else {

   auto cit = f_const.size() - strt < strt ?
	      std::prev( f_const.end() , f_const.size() - strt ) :
              std::next( f_const.begin() , strt );

   if( tmod->PFtype() == PolyhedralFunctionMod::ModifyRows ) {
    // modify rows & constants. Cover index 0 (the coefficient of v) so
    // that diagonal-vs-vertical type changes are reflected in the LP too
    Range rng = Range( 0 , PF().get_num_active_var() + 1 );
    const auto nv = PF().get_num_active_var();
    for( Index i = strt ; i < stop ; ) {
     const auto local_scale = RowScale( i );
     const auto scale = ScaledRowFactor( i );
     LinearFunction::Vec_FunctionValue Ai( nv + 1 );
     Ai[ 0 ] = PF().is_row_vertical( i ) ? 0.0 : local_scale;
     for( Index j = 0 ; j < nv ; ++j )
      Ai[ j + 1 ] = - scale * PF().get_A()[ i ][ j ];
     static_cast< LinearFunction * >( cit->get_function() )->
                          modify_coefficients( std::move( Ai ) , rng , par );
     if( PF().is_convex() )
      (cit++)->set_lhs( scale * PF().get_b()[ i++ ] , par );
     else
      (cit++)->set_rhs( scale * PF().get_b()[ i++ ] , par );
     }
    }
   else  // modify constants only
    if( PF().is_convex() )
     for( Index i = strt ; i < stop ; )
      (cit++)->set_lhs( ScaledRowFactor( i ) * PF().get_b()[ i++ ] ,
                        par );
    else
     for( Index i = strt ; i < stop ; )
      (cit++)->set_rhs( ScaledRowFactor( i ) * PF().get_b()[ i++ ] ,
                        par );
   }

  close_if_needed( par , nc );
  return( false );
  }

 // PolyhedralFunctionModSbst - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const PolyhedralFunctionModSbst * >( mod ) ) {
  // this is "modify/delete a subset of rows"
  // open a new GroupModification, not concerning PolyhedralFunctionBlock
  // unless it's deleting or only one row and *not* also its constant
  Index nc = tmod->PFtype() == PolyhedralFunctionMod::DeleteRows ? 0 :
             ( tmod->PFtype() == PolyhedralFunctionMod::ModifyCnst ? 2 :
	       tmod->rows().size() );
  if( tmod->PFtype() == PolyhedralFunctionMod::DeleteRows ) {
   // delete rows
   DeleteRowScaling( tmod->rows() );
   }
  else {
   RefreshRowMeasure( tmod->rows() );
   if( RescaleGlobalIfNeeded( chnl ) )
    return( false );
   }

  auto par = open_if_needed( make_par( eNoBlck , chnl ) , nc );

  Index prev = 0;
  auto cit = f_const.begin();
  auto rit = tmod->rows().begin();
  if( tmod->PFtype() == PolyhedralFunctionMod::DeleteRows ) {
   remove_dynamic_constraints( f_const , Subset( tmod->rows() ) , true ,
			       par );
   RescaleGlobalIfNeeded( chnl );
   }
  else
   if( tmod->PFtype() == PolyhedralFunctionMod::ModifyRows ) {
    // modify rows & constants. Cover index 0 (the coefficient of v) so
    // that diagonal-vs-vertical type changes are reflected in the LP too
    Range rng = Range( 0 , PF().get_num_active_var() + 1 );
    const auto nv = PF().get_num_active_var();
    for( ; rit != tmod->rows().end() ; ) {
     cit = std::next( cit , *rit - prev );
     const auto local_scale = RowScale( *rit );
     const auto scale = ScaledRowFactor( *rit );
     LinearFunction::Vec_FunctionValue Ai( nv + 1 );
     Ai[ 0 ] = PF().is_row_vertical( *rit ) ? 0.0 : local_scale;
     for( Index j = 0 ; j < nv ; ++j )
      Ai[ j + 1 ] = - scale * PF().get_A()[ *rit ][ j ];
     static_cast< LinearFunction * >( cit->get_function() )->
                          modify_coefficients( std::move( Ai ) , rng , par );
     if( PF().is_convex() )
      cit->set_lhs( scale * PF().get_b()[ *rit ] , par );
     else
      cit->set_rhs( scale * PF().get_b()[ *rit ] , par );
     prev = *(rit++);
     }
    }
   else  // modify constants only
    for( ; rit != tmod->rows().end() ; ) {
     cit = std::next( cit , *rit - prev );
     if( PF().is_convex() )
      cit->set_lhs( ScaledRowFactor( *rit ) * PF().get_b()[ *rit ] ,
                    par );
     else
      cit->set_rhs( ScaledRowFactor( *rit ) * PF().get_b()[ *rit ] ,
                    par );
     prev = *(rit++);
     }
 
  close_if_needed( par , nc );
  return( false );
  }

 // PolyhedralFunctionModAddd - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const PolyhedralFunctionModAddd * >( mod ) ) {
  // this is "add new rows"
  Index nr = PF().get_A().size();
  AppendRowScaling( nr - tmod->addedrows() );
  AppendRowMeasure( nr - tmod->addedrows() );
  RescaleGlobalIfNeeded( chnl );
  std::list< FRowConstraint > newc( tmod->addedrows() );
  auto cit = newc.begin();
  for( Index i = nr - tmod->addedrows() ; i < nr ; )
   ConstructLPConstraint( i++ , *(cit++) );
  add_dynamic_constraints( f_const , newc , make_par( eNoBlck , chnl ) );
  return( false );
  }

 // C05FunctionMod- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const C05FunctionMod * >( mod ) ) {
  // this is a change of the "verse" of the PolyhedralFunction
  if( tmod->type() != C05FunctionMod::NothingChanged )
   throw( std::logic_error( "wrong C05FunctionMod in PolyhedralFunction" ) );

  // open a new GroupModification, not concerning PolyhedralFunctionBlock
  auto par = open_if_needed( make_par( eNoBlck , chnl ) , 2 );
  Index i = 0;

  if( PF().is_convex() ) {
   // change the "verse" of the objective accordingly
   if( auto obj = get_objective() )
    obj->set_sense( Objective::eMin , par );

   // set upper/lower bound on v
   f_bcv.set_lhs( PF().get_global_lower_bound() , par );
   f_bcv.set_rhs( Inf< Function::FunctionValue >() , par );

   // properly set the lhs/rhs of the constraints
   for( auto & ci : f_const ) {
    ci.set_lhs( ScaledRowFactor( i ) * PF().get_b()[ i++ ] , par );
    ci.set_rhs( Inf< Function::FunctionValue >() , par );
    }
   }
  else {
   // change the "verse" of the objective accordingly
   if( auto obj = get_objective() )
    obj->set_sense( Objective::eMax , par );

   // properly set upper/lower bound on v
   f_bcv.set_lhs( -Inf< Function::FunctionValue >() , par );
   f_bcv.set_rhs( PF().get_global_upper_bound() , par );

   // properly set the lhs/rhs of the constraints
   for( auto & ci : f_const ) {
    ci.set_lhs( -Inf< Function::FunctionValue >() , par );
    ci.set_rhs( ScaledRowFactor( i ) * PF().get_b()[ i++ ] , par );
    }
   }

  close_if_needed( par , 2 );
  return( false );
  }

 // FunctionMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // if all else fails, this must be a "simple" FunctionMod, whose
 // meaning is "everything is changed", hence change everything

 assert( std::isnan( mod->shift() ) );

 // set upper/lower bound on v
 f_row_scale.clear();
 f_row_scale.reserve( PF().get_A().size() );
 f_row_measure.clear();
 f_row_measure.reserve( PF().get_A().size() );
 AppendRowScaling( 0 );
 AppendRowMeasure( 0 );
 UpdateGlobalScaleIfNeeded();
 f_bcv.set_lhs( PF().get_global_lower_bound() , eNoMod );
 f_bcv.set_rhs( PF().get_global_upper_bound() , eNoMod );
 RebuildLinearizedPrimal();
 return( true );

 }  // end( PolyhedralFunctionBlock::guts_of_add_Modification_PF )

/*--------------------------------------------------------------------------*/

bool PolyhedralFunctionBlock::guts_of_add_Modification_LR( c_p_Mod mod ,
							   ChnlName chnl )
{
 // process a Modification produced by the "linearized" representation - - - -
 /* This requires to patiently sift through the possible Modification types
  * find what this Modification exactly, is and appropriately mirror the
  * changes of the "abstract" representation into the PolyhedralFunction
  * (which in this case counts as the "physical" one). Note, however, that
  *
  *     SOME Modification OF THE LP ARE NOT SUPPORTED SINCE THEY WOULD
  *     LEAVE THE PolyhedralFunction IN AN INCONSISTENT STATE
  */

 // BlockModAdd< FRowConstraint > - - - - - - - - - - - - - - - - - - - - - -
 // adding a dynamic constraint
 if( auto tmod = dynamic_cast< const BlockModAdd< FRowConstraint > * >( mod )
     ) {
  if( & tmod->whc() != & f_const )   // if it's not about f_const
   return( false );                  // none of my business

  const auto & arr = tmod->added();

  if( arr.empty() )  // should not happen, but in case
   return( false );  // nothing to do
  MultiVector A( arr.size() );
  RealVector b( arr.size() );
  RealVector row_scale( arr.size() , 1.0 );
  // recover the per-row vertical flag from the coefficient of v (which is
  // positive for diagonal rows and 0 for vertical rows; see
  // ConstructLPConstraint and the GENERAL NOTES of PolyhedralFunction)
  PolyhedralFunction::BoolVector iV;

  Index i = 0;
  for( auto ci : arr ) {
   // now the though part: recover the linearization
   auto lf = dynamic_cast< LinearFunction * >( ci->get_function() );
   if( ! lf )
    throw( std::logic_error( "FRowConstraint with no LinearFunction" ) );

   const auto & coeff = lf->get_v_var();

   // note that the LinearFunction has exactly one active Variable more than
   // the PolyhedralFunction, the first one being "v"
   if( coeff.size() != PF().get_num_active_var() + 1 )
    throw( std::logic_error( "incorrect LinearFunction in FRowConstraint" ) );

   #ifndef NDEBUG
   // TODO: check that the Variables actually are the same
   #endif
   const bool is_vert = coeff[ 0 ].second == 0.0;
   if( has_local_scaling() ) {
    if( ! is_vert ) {
     if( coeff[ 0 ].second <= 0 )
      throw( std::logic_error( "non-positive local row scale" ) );
     row_scale[ i ] = coeff[ 0 ].second;
     }
    else {
     // Vertical rows have no v coefficient carrying the local factor.
     // Recover it from the already-scaled row. If R is the largest
     // unscaled coefficient and local_scale = 1 / sqrt( R ), then the
     // largest coefficient after removing the global scale is sqrt( R ).
     auto mx = std::abs( PF().is_convex() ? ci->get_lhs()
                                             : ci->get_rhs() ) /
               f_global_scale;
     for( Index j = 1 ; j < coeff.size() ; ++j )
      mx = std::max( mx , std::abs( coeff[ j ].second ) / f_global_scale );
     row_scale[ i ] = 1.0 / std::max( 1.0 , mx );
     }
    }

   const auto scale = f_global_scale * row_scale[ i ];

   // recover the unscaled constant and linearization
   b[ i ] = ( PF().is_convex() ? ci->get_lhs() : ci->get_rhs() ) /
            scale;
   A[ i ].resize( PF().get_num_active_var() );

   for( Index j = 1 ; j < coeff.size() ; ++j )
    A[ i ][ j - 1 ] = - coeff[ j ].second / scale;

   // A vertical row has coef of v == 0. Diagonal rows can have an
   // arbitrarily small positive coefficient when local scaling is active,
   // so zero must be tested exactly here.
   if( is_vert ) {
    if( iV.empty() )
     iV.assign( arr.size() , false );
    iV[ i ] = true;
    }

   ++i;
   }

  PF().add_rows( std::move( A ) , b , make_par( eNoBlck , chnl ) ,
		    std::move( iV ) );
  f_row_scale.insert( f_row_scale.end() , row_scale.begin() ,
                      row_scale.end() );
  AppendRowMeasure( PF().get_A().size() - row_scale.size() );
  RescaleGlobalIfNeeded( chnl );
  return( false );
  }

 // BlockModRmvRngd< FRowConstraint > - - - - - - - - - - - - - - - - - - - -
 // removing a range of dynamic Constraint = rows of PolyhedralFunction
 if( auto tmod = dynamic_cast< const BlockModRmvRngd< FRowConstraint > *
                               >( mod ) ) {
  if( & tmod->whc() != & f_const )   // if it's not about f_const
   return( false );                  // none of my business

  PF().delete_rows( tmod->range() , make_par( eNoBlck , chnl ) );
  DeleteRowScaling( tmod->range() );
  RescaleGlobalIfNeeded( chnl );
  return( false );
  }

 // BlockModRmvSbst< FRowConstraint > - - - - - - - - - - - - - - - - - - - -
 // removing a subset of dynamic Constraint = rows of PolyhedralFunction
 if( const auto tmod =
     dynamic_cast< BlockModRmvSbst< FRowConstraint > * const >( mod ) ) {
  if( & tmod->whc() != & f_const )   // if it's not about f_const
   return( false );                  // none of my business

  PF().delete_rows( Subset( tmod->subset() ) , true ,
		       make_par( eNoBlck , chnl ) );
  DeleteRowScaling( tmod->subset() );
  RescaleGlobalIfNeeded( chnl );
  return( false );
  }

 // ObjectiveMod- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const ObjectiveMod * >( mod ) )
  throw( std::logic_error(
		   "ObjectiveMod not allowed in PolyhedralFunctionBlock" ) );

 // RowConstraintMod- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( const auto tmod = dynamic_cast< RowConstraintMod * const >( mod ) ) {
  if( & f_scale_cns == tmod->constraint() )
   throw( std::logic_error(
             "cannot modify the internal v scaling constraint" ) );

  // first check if it's about the box constraint on v
  if( & f_bcv == tmod->constraint() ) {
   if( ( tmod->type() == RowConstraintMod::eChgBTS ) ||
       ( ( tmod->type() == RowConstraintMod::eChgRHS ) &&
	 PF().is_convex() ) ||
       ( ( tmod->type() == RowConstraintMod::eChgLHS ) &&
	 ( ! PF().is_convex() ) ) )
    throw( std::logic_error(
		    "wrong RowConstraintMod in PolyhedralFunctionBlock" ) );

   PF().modify_bound( PF().is_convex() ? f_bcv.get_lhs()
			                     : f_bcv.get_rhs() ,
			 make_par( eNoBlck , chnl ) );
   return( false );
   }

  // now check if it's about one linear constraint
  Index i = 0;
  auto ci = f_const.begin();
  for( ; ci != f_const.end() ; ++ci , ++i )
   if( & (*ci) == tmod->constraint() )
    break;

  if( ci == f_const.end() )  // that's not in the linearized representation
   return( false );          // none of my business

  if( ( tmod->type() == RowConstraintMod::eChgBTS ) ||
      ( ( tmod->type() == RowConstraintMod::eChgRHS ) &&
	PF().is_convex() ) ||
      ( ( tmod->type() == RowConstraintMod::eChgLHS ) &&
	( ! PF().is_convex() ) ) )
   throw( std::logic_error(
		    "wrong RowConstraintMod in PolyhedralFunctionBlock" ) );

  PF().modify_constant( i , ( PF().is_convex() ? ci->get_lhs()
		                                     : ci->get_rhs() ) /
                                ScaledRowFactor( i ) ,
			   make_par( eNoBlck , chnl ) );
  RefreshRowMeasure( i );
  RescaleGlobalIfNeeded( chnl );
  return( false );
  }

 // VariableMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const VariableMod * >( mod ) ) {
  if( ( tmod->variable() == & f_v ) ||
      ( tmod->variable() == & f_scaled_v ) )
   throw( std::logic_error(
		          "wrong VariableMod in PolyhedralFunctionBlock" ) );
  return( false );  // if it's not about v, none of my business
  }

 // C05FunctionModLinRngd - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const C05FunctionModLinRngd * >( mod ) ) {
  Index i = 0;
  auto ci = f_const.begin();
  for( ; ci != f_const.end() ; ++ci , ++i )
   if( ci->get_function() == tmod->function() )
    break;

  if( ci == f_const.end() )  // that's not in the linearized representation
   return( false );          // none of my business

  // note that the LinearFunction has exactly one active Variable more than
  // the PolyhedralFunction, the first one being "v", whence the "- 1"
  // for variable index translation. The coefficients of the x variables
  // are the opposite of the entries in A; hence, if the coefficients are
  // changed by adding tmod->delta(), the entries of A change by
  // subtracting tmod->delta(). Index 0 in the LinearFunction is v
  // itself: if its coefficient ends up 0, the row is
  // a *vertical* linearization of the PolyhedralFunction
  RealVector ai( PF().get_A()[ i ] );
  const auto scale = ScaledRowFactor( i );
  bool is_vert = PF().is_row_vertical( i );
  bool v_changed = false;
  for( Index j = 0 ; j < tmod->delta().size() ; ++j ) {
   const Index pos = tmod->range().first + j;
   if( pos == 0 )
    v_changed = true;       // delta on v's coefficient: type may change
   else
    ai[ pos - 1 ] -= tmod->delta()[ j ] / scale;
   }
  if( v_changed ) {
   // re-derive the row's vertical/diagonal status from the *new* v coef
   const auto & vp = static_cast< LinearFunction * >( ci->get_function() )
		     ->get_v_var();
   is_vert = vp[ 0 ].second == 0.0;
   if( has_local_scaling() && ( ! is_vert ) ) {
    if( vp[ 0 ].second <= 0 )
     throw( std::logic_error( "non-positive local row scale" ) );
    f_row_scale[ i ] = vp[ 0 ].second;
    const auto new_scale = ScaledRowFactor( i );
    for( Index j = 1 ; j < vp.size() ; ++j )
     ai[ j - 1 ] = - vp[ j ].second / new_scale;
    }
   }

  PF().modify_row( i , std::move( ai ) , PF().get_b()[ i ] ,
		      make_par( eNoBlck , chnl ) , is_vert );
  RefreshRowMeasure( i );
  RescaleGlobalIfNeeded( chnl );
  return( false );
  }

 // C05FunctionModLinSbst - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const C05FunctionModLinSbst * >( mod ) ) {
  Index i = 0;
  auto ci = f_const.begin();
  for( ; ci != f_const.end() ; ++ci , ++i )
   if( ci->get_function() == tmod->function() )
    break;

  if( ci == f_const.end() )  // that's not in the linearized representation
   return( false );          // none of my business

  // see comment for C05FunctionModLinRngd above for the index translation
  RealVector ai( PF().get_A()[ i ] );
  const auto scale = ScaledRowFactor( i );
  bool is_vert = PF().is_row_vertical( i );
  bool v_changed = false;
  for( Index j = 0 ; j < tmod->subset().size() ; ++j ) {
   const Index pos = tmod->subset()[ j ];
   if( pos == 0 )
    v_changed = true;
   else
    ai[ pos - 1 ] -= tmod->delta()[ j ] / scale;
   }
  if( v_changed ) {
   const auto & vp = static_cast< LinearFunction * >( ci->get_function() )
		     ->get_v_var();
   is_vert = vp[ 0 ].second == 0.0;
   if( has_local_scaling() && ( ! is_vert ) ) {
    if( vp[ 0 ].second <= 0 )
     throw( std::logic_error( "non-positive local row scale" ) );
    f_row_scale[ i ] = vp[ 0 ].second;
    const auto new_scale = ScaledRowFactor( i );
    for( Index j = 1 ; j < vp.size() ; ++j )
     ai[ j - 1 ] = - vp[ j ].second / new_scale;
    }
   }

  PF().modify_row( i , std::move( ai ) , PF().get_b()[ i ] ,
		      make_par( eNoBlck , chnl ) , is_vert );
  RefreshRowMeasure( i );
  RescaleGlobalIfNeeded( chnl );
  return( false );
  }

 // FunctionModVars - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // any addition/removal of Variables in the linearized representation is bad
 if( auto tmod = dynamic_cast< const FunctionModVars * >( mod ) ) {
  auto ci = f_const.begin();
  for( ; ci != f_const.end() ; ++ci )
   if( ci->get_function() == tmod->function() )
    break;

  if( ci != f_const.end() )  // it's in the linearized representation
   throw( std::logic_error(
	             "wrong FunctionModVars in PolyhedralFunctionBlock" ) );
 
  return( false );  // else, none of my business
  }

 // C05FunctionMod- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // that's changing the constant, not good either
 if( auto tmod = dynamic_cast< const C05FunctionMod * >( mod ) ) {
  auto ci = f_const.begin();
  for( ; ci != f_const.end() ; ++ci )
   if( ci->get_function() == tmod->function() )
    break;

  if( ci != f_const.end() )  // it's in the linearized representation
   throw( std::logic_error(
		       "wrong C05FunctionMod in PolyhedralFunctionBlock" ) );
 
  return( false );  // else, none of my business
  }
 return( false );
 }  // end( PolyhedralFunctionBlock::guts_of_add_Modification_LR )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------ MODIFICATION HANDLERS FOR DUAL REPRESENTATION ---------*/
/*--------------------------------------------------------------------------*/

bool PolyhedralFunctionBlock::guts_of_add_Modification_PF_dual(
                                    const FunctionMod * mod , ChnlName chnl )
{
 // process a FunctionMod produced by PF() in the *dual* representation,
 // mirroring the change into f_theta (the dynamic theta variables),
 // f_normcns (the static normalisation constraint), the FRealObjective
 // LinearFunction, and f_coupling (the external coupling constraints
 // registered via set_conjugate_constraint(), if any).
 //
 // Strategy:
 //  * for "cheap" Modifications that only touch the objective LinearFunction
 //    or gamma (ModifyCnst, modify bound, convex/concave flip), apply the
 //    change in place via modify_coefficient(s) / set_sense / VariableMod
 //    -- MILPSolver / CPXMILPSolver support those.
 //  * for Modifications that touch the constraint matrix of the dual LP
 //    (AddRows / DeleteRows / ModifyRows, since rows of PF() are
 //    *columns* of the dual LP), apply the corresponding incremental
 //    column-side updates: add_dynamic_variables / remove_dynamic_variables
 //    on f_theta plus the matching add_variables / remove_variables /
 //    modify_coefficients on obj_lf, on the normalisation constraint, and
 //    on every external coupling LinearFunction (the latter through the
 //    f_coupling pointer set up by set_conjugate_constraint()). These
 //    are propagated to CPXMILPSolver / GRBMILPSolver via their
 //    add_dynamic_variable / remove_dynamic_variable /
 //    constraint_function_modification / constraint_fvars_modification
 //    overrides (which call CPXaddcols / CPXdelcols / CPXchgcoeflist and
 //    the Gurobi equivalents).

 // shortcuts to the abstract structures of the dual representation
 auto frobj = static_cast< FRealObjective * >( get_objective() );
 auto obj_lf = static_cast< LinearFunction * >( frobj->get_function() );

 // C05FunctionModVarsAddd/Rngd/Sbst - - - - - - - - - - - - - - - - - - - -
 // x variables of PF() added/removed: this also requires the father
 // Block to add/remove its corresponding coupling constraints, which is
 // outside the responsibility of a single PolyhedralFunctionBlock. Until
 // a higher-level coordination mechanism is in place, refuse these mods
 if( dynamic_cast< const C05FunctionModVarsAddd * >( mod ) ||
     dynamic_cast< const C05FunctionModVarsRngd * >( mod ) ||
     dynamic_cast< const C05FunctionModVarsSbst * >( mod ) )
  throw( std::logic_error( "PolyhedralFunctionBlock: changing the active "
                           "Variable of the PolyhedralFunction is not yet "
			   "supported in "
                           "the dual representation" ) );

 // detect the "cheap" sub-cases that can be handled incrementally
 // (without touching the constraint matrix of the dual LP) before
 // falling through to the rebuild path.

 // ModifyCnst is cheap: only b_i changes -> only obj coefficients
 const auto rng_mod = dynamic_cast< const PolyhedralFunctionModRngd * >( mod );
 const auto sbst_mod = dynamic_cast< const PolyhedralFunctionModSbst * >( mod );

 // bound change is encoded as PolyhedralFunctionModRngd with empty range
 if( rng_mod && rng_mod->range().first == rng_mod->range().second ) {
  // recompute the new bound (possibly +/-INF if the bound is unset)
  const bool was_fixed = f_gamma.is_fixed();
  const bool now_set = PF().is_bound_set();
  const double nbnd = now_set ? ScaledBound( PF().get_global_bound() )
                              : 0.0;

  auto par = make_par( eNoBlck , chnl );
  if( was_fixed != ( ! now_set ) ) {
   // gamma's "fixed-to-0" status flips
   if( now_set )
    f_gamma.is_fixed( false , par );
   else {
    f_gamma.set_value( 0 );
    f_gamma.is_fixed( true , par );
    }
   }
  // update gamma's coefficient in the objective LinearFunction (gamma
  // is always at position 0 of obj_lf by construction)
  obj_lf->modify_coefficient( 0 , nbnd , par );
  return( false );
  }

 // ModifyCnst (range / subset): only b coefficients change in obj LF
 if( rng_mod && rng_mod->PFtype() == PolyhedralFunctionMod::ModifyCnst ) {
  RefreshRowMeasure( rng_mod->range() );
  if( RescaleGlobalIfNeeded( chnl ) )
   return( false );
  const Index strt = rng_mod->range().first;
  const Index stop = rng_mod->range().second;
  LinearFunction::Vec_FunctionValue nb( stop - strt );
  for( Index i = strt ; i < stop ; ++i )
   nb[ i - strt ] = ScaledRowFactor( i ) * PF().get_b()[ i ];
  obj_lf->modify_coefficients( std::move( nb ) ,
                               Range( strt + 1 , stop + 1 ) ,
                               make_par( eNoBlck , chnl ) );
  return( false );
  }
 if( sbst_mod && sbst_mod->PFtype() == PolyhedralFunctionMod::ModifyCnst ) {
  RefreshRowMeasure( sbst_mod->rows() );
  if( RescaleGlobalIfNeeded( chnl ) )
   return( false );
  const auto & rows = sbst_mod->rows();
  LinearFunction::Vec_FunctionValue nb( rows.size() );
  Subset nms( rows.size() );
  for( Index i = 0 ; i < rows.size() ; ++i ) {
   nb[ i ] = ScaledRowFactor( rows[ i ] ) *
             PF().get_b()[ rows[ i ] ];
   nms[ i ] = rows[ i ] + 1;
   }
  obj_lf->modify_coefficients( std::move( nb ) , std::move( nms ) , true ,
                               make_par( eNoBlck , chnl ) );
  return( false );
  }

 // C05FunctionMod (NothingChanged): convex/concave flip; just flip the
 // sense of the dual objective (dual sense = opposite of primal)
 if( auto tmod = dynamic_cast< const C05FunctionMod * >( mod ) ) {
  if( ! rng_mod && ! sbst_mod &&
      ! dynamic_cast< const PolyhedralFunctionModAddd * >( mod ) ) {
   if( tmod->type() != C05FunctionMod::NothingChanged )
    throw( std::logic_error(
                       "wrong C05FunctionMod in PolyhedralFunction" ) );

   frobj->set_sense( PF().is_convex() ? Objective::eMax : Objective::eMin ,
                     make_par( eNoBlck , chnl ) );
   return( false );
   }
  // else fall through: PolyhedralFunctionMod[Addd|Rngd|Sbst] derive from
  // C05FunctionMod and we want the rebuild path for them
  }

 // The remaining PolyhedralFunctionMod variants — AddRows, DeleteRows,
 // ModifyRows (in both range- and subset-flavoured forms) — change the
 // *constraint matrix* of the dual LP, since rows of PF() correspond
 // to *columns* of the dual LP. The incremental column-side updates
 // are supported by CPXMILPSolver (CPXaddcols / CPXdelcols /
 // CPXchgcoeflist) and GRBMILPSolver via their overrides of
 // add_dynamic_variable / remove_dynamic_variable /
 // constraint_function_modification / constraint_fvars_modification.

 // PolyhedralFunctionModAddd: append `nadd` rows at the end of PF()
 if( auto tmod = dynamic_cast< const PolyhedralFunctionModAddd * >( mod ) ) {
  const Index nadd = tmod->addedrows();
  if( nadd == 0 )
   return( false );

  const Index nr_total = PF().get_A().size();
  const Index nr_old = nr_total - nadd;
  AppendRowScaling( nr_old );
  AppendRowMeasure( nr_old );
  RescaleGlobalIfNeeded( chnl );

  // use the caller's channel directly (don't open a nested one);
  // eNoBlck means concerns_Block() = false on the self-generated structural
  // Modifications below (new dynamic theta ColVariables, augmented
  // LinearFunction coefficients on obj_lf / normcns_lf / coupling[j]);
  // PFB::add_Modification recognises these by !concerns_Block() and
  // routes them through the pass-up branch (PolyhedralFunctionBlock.h),
  // so they are forwarded to a father Solver without re-entering the
  // dispatcher and tripping the "unsupported Modification" guard at the
  // bottom of guts_of_add_Modification_LR_dual()
  auto par = make_par( eNoBlck , chnl );

  // 1) create the new theta ColVariables in a temporary list and splice
  //    them into f_theta via add_dynamic_variables(). splice keeps the
  //    node addresses we collected here valid afterwards
  std::list< ColVariable > newt( nadd );
  std::vector< ColVariable * > new_ptrs( nadd );
  {
   Index k = 0;
   for( auto & v : newt ) {
    v.is_positive( true , eNoMod );
    new_ptrs[ k++ ] = & v;
    }
   }
  add_dynamic_variables( f_theta , newt , par );

  // 2) append (theta_new , b_new) to the FRealObjective LinearFunction
  {
   LinearFunction::v_coeff_pair to_obj( nadd );
   for( Index i = 0 ; i < nadd ; ++i )
    to_obj[ i ] = std::make_pair(
                             new_ptrs[ i ] ,
                             ScaledRowFactor( nr_old + i ) *
                             PF().get_b()[ nr_old + i ] );
   obj_lf->add_variables( std::move( to_obj ) , par );
   }

  // 3) append (theta_new , 1) to the normalisation LinearFunction for
  //    every new diagonal row
  {
   auto nrm_lf = static_cast< LinearFunction * >(
                                            f_normcns.get_function() );
   LinearFunction::v_coeff_pair to_nrm;
   for( Index i = 0 ; i < nadd ; ++i )
    if( ! PF().is_row_vertical( nr_old + i ) )
     to_nrm.emplace_back( new_ptrs[ i ] , RowScale( nr_old + i ) );
   if( ! to_nrm.empty() )
    nrm_lf->add_variables( std::move( to_nrm ) , par );
   }

  // 4) for each external coupling FRowConstraint, append the non-zero
  //    (theta_new , a_{new,j}) contributions of the new rows
  if( f_coupling ) {
   Index j = 0;
   for( auto & c : *f_coupling ) {
    auto cf = static_cast< LinearFunction * >( c.get_function() );
    LinearFunction::v_coeff_pair to_cp;
    to_cp.reserve( nadd );
    for( Index i = 0 ; i < nadd ; ++i ) {
     const double a = ScaledRowFactor( nr_old + i ) *
                      PF().get_A()[ nr_old + i ][ j ];
     if( a != 0 )
      to_cp.emplace_back( new_ptrs[ i ] , a );
     }
    if( ! to_cp.empty() )
     cf->add_variables( std::move( to_cp ) , par );
    ++j;
    }
   }

  return( false );
  }

 // PolyhedralFunctionModRngd with DeleteRows / ModifyRows
 if( rng_mod ) {
  const Index strt = rng_mod->range().first;
  const Index stop = rng_mod->range().second;
  // see comment in the PolyhedralFunctionModAddd branch above: eModBlck
  // is required for the structural removes / modifies emitted below to
  // reach an attached Solver across a father Block boundary
  auto par = make_par( eNoBlck , chnl );
  auto nrm_lf = static_cast< LinearFunction * >( f_normcns.get_function() );

  if( rng_mod->PFtype() == PolyhedralFunctionMod::DeleteRows ) {
   DeleteRowScaling( Range( strt , stop ) );

   // collect pointers of the theta variables being removed
   std::vector< ColVariable * > rem_ptrs;
   rem_ptrs.reserve( stop - strt );
   auto thit = std::next( f_theta.begin() , strt );
   for( Index i = strt ; i < stop ; ++i , ++thit )
    rem_ptrs.push_back( & *thit );

   // 1) obj_lf: the theta variables sit at positions [strt+1, stop+1)
   //    (gamma is at position 0)
   obj_lf->remove_variables( Range( strt + 1 , stop + 1 ) , par );

   // 2) normcns_lf: lookup by pointer (only diagonal thetas are there)
   {
    Subset to_rm;
    to_rm.reserve( rem_ptrs.size() );
    for( auto p : rem_ptrs ) {
     const Index k = nrm_lf->is_active( p );
     if( k < nrm_lf->get_num_active_var() )
      to_rm.push_back( k );
     }
    if( ! to_rm.empty() ) {
     std::sort( to_rm.begin() , to_rm.end() );
     nrm_lf->remove_variables( std::move( to_rm ) , true , par );
     }
    }

   // 3) each coupling[j]: lookup by pointer (entries are present only
   //    for non-zero a_{i,j})
   if( f_coupling )
    for( auto & c : *f_coupling ) {
     auto cf = static_cast< LinearFunction * >( c.get_function() );
     Subset to_rm;
     to_rm.reserve( rem_ptrs.size() );
     for( auto p : rem_ptrs ) {
      const Index k = cf->is_active( p );
      if( k < cf->get_num_active_var() )
       to_rm.push_back( k );
      }
     if( ! to_rm.empty() ) {
      std::sort( to_rm.begin() , to_rm.end() );
      cf->remove_variables( std::move( to_rm ) , true , par );
      }
     }

   // 4) finally remove the theta variables themselves from the dynamic
   //    list (this delivers a BlockModRmv<ColVariable> to the Solver)
   remove_dynamic_variables( f_theta , Range( strt , stop ) , par );
   RescaleGlobalIfNeeded( chnl );

   return( false );
   }

  if( rng_mod->PFtype() == PolyhedralFunctionMod::ModifyRows ) {
   RefreshRowMeasure( Range( strt , stop ) );
   RescaleGlobalIfNeeded( chnl );

   const auto & A = PF().get_A();

   // 1) obj_lf: update b for rows [strt, stop) at positions [strt+1, stop+1)
   {
    LinearFunction::Vec_FunctionValue nb( stop - strt );
    for( Index i = strt ; i < stop ; ++i )
     nb[ i - strt ] = ScaledRowFactor( i ) * PF().get_b()[ i ];
    obj_lf->modify_coefficients( std::move( nb ) ,
                                 Range( strt + 1 , stop + 1 ) , par );
    }

   // 2) normcns_lf: handle diag/vert flips (theta_i may need to be
   //    added if it just became diagonal, or removed if it just became
   //    vertical; otherwise no change)
   {
    LinearFunction::v_coeff_pair to_add;
    Subset to_rm;
    auto thit = std::next( f_theta.begin() , strt );
    for( Index i = strt ; i < stop ; ++i , ++thit ) {
     ColVariable * p = & *thit;
     const Index k = nrm_lf->is_active( p );
     const bool in_nrm = ( k < nrm_lf->get_num_active_var() );
     const bool should_in = ! PF().is_row_vertical( i );
     if( in_nrm && ! should_in )
      to_rm.push_back( k );
     else if( should_in && ! in_nrm )
      to_add.emplace_back( p , RowScale( i ) );
     }
    if( ! to_rm.empty() ) {
     std::sort( to_rm.begin() , to_rm.end() );
     nrm_lf->remove_variables( std::move( to_rm ) , true , par );
     }
    if( ! to_add.empty() )
     nrm_lf->add_variables( std::move( to_add ) , par );
    }

   // 3) each coupling[j]: update a_{i,j} for rows i in [strt, stop)
   if( f_coupling ) {
    Index j = 0;
    for( auto & c : *f_coupling ) {
     auto cf = static_cast< LinearFunction * >( c.get_function() );
     Subset nms;
     LinearFunction::Vec_FunctionValue ncoef;
     auto thit = std::next( f_theta.begin() , strt );
     for( Index i = strt ; i < stop ; ++i , ++thit ) {
      ColVariable * p = & *thit;
      const Index k = cf->is_active( p );
      if( k < cf->get_num_active_var() ) {
       nms.push_back( k );
       ncoef.push_back( ScaledRowFactor( i ) * A[ i ][ j ] );
       }
      else if( A[ i ][ j ] != 0 )
       cf->add_variable( p , ScaledRowFactor( i ) * A[ i ][ j ] , par );
      }
     if( ! nms.empty() )
      cf->modify_coefficients( std::move( ncoef ) , std::move( nms ) ,
                               false , par );
     ++j;
     }
    }

   return( false );
   }

  throw( std::invalid_argument(
                       "unknown PolyhedralFunctionModRngd PFtype" ) );
  }

 // PolyhedralFunctionModSbst with DeleteRows / ModifyRows
 if( sbst_mod ) {
  const auto & rows = sbst_mod->rows();
  // see comment in the PolyhedralFunctionModAddd branch above: eModBlck
  // is required for the structural removes / modifies emitted below to
  // reach an attached Solver across a father Block boundary
  auto par = make_par( eNoBlck , chnl );
  auto nrm_lf = static_cast< LinearFunction * >( f_normcns.get_function() );

  if( sbst_mod->PFtype() == PolyhedralFunctionMod::DeleteRows ) {
   DeleteRowScaling( rows );
   }
  else if( sbst_mod->PFtype() == PolyhedralFunctionMod::ModifyRows ) {
   RefreshRowMeasure( rows );
   RescaleGlobalIfNeeded( chnl );
   }

  // PolyhedralFunctionModSbst always has rows sorted by Subst contract;
  // resolve them once into iterators / pointers into f_theta
  std::vector< ColVariable * > sub_ptrs( rows.size() );
  {
   Index prev = 0;
   auto thit = f_theta.begin();
   for( Index k = 0 ; k < rows.size() ; ++k ) {
    thit = std::next( thit , rows[ k ] - prev );
    sub_ptrs[ k ] = & *thit;
    prev = rows[ k ];
    }
   }

  if( sbst_mod->PFtype() == PolyhedralFunctionMod::DeleteRows ) {

   // 1) obj_lf: positions are rows[i]+1, already sorted
   {
    Subset to_rm( rows.size() );
    for( Index i = 0 ; i < rows.size() ; ++i )
     to_rm[ i ] = rows[ i ] + 1;
    obj_lf->remove_variables( std::move( to_rm ) , true , par );
    }

   // 2) normcns_lf: lookup by pointer
   {
    Subset to_rm;
    to_rm.reserve( sub_ptrs.size() );
    for( auto p : sub_ptrs ) {
     const Index k = nrm_lf->is_active( p );
     if( k < nrm_lf->get_num_active_var() )
      to_rm.push_back( k );
     }
    if( ! to_rm.empty() ) {
     std::sort( to_rm.begin() , to_rm.end() );
     nrm_lf->remove_variables( std::move( to_rm ) , true , par );
     }
    }

   // 3) each coupling[j]: lookup by pointer
   if( f_coupling )
    for( auto & c : *f_coupling ) {
     auto cf = static_cast< LinearFunction * >( c.get_function() );
     Subset to_rm;
     to_rm.reserve( sub_ptrs.size() );
     for( auto p : sub_ptrs ) {
      const Index k = cf->is_active( p );
      if( k < cf->get_num_active_var() )
       to_rm.push_back( k );
      }
     if( ! to_rm.empty() ) {
      std::sort( to_rm.begin() , to_rm.end() );
      cf->remove_variables( std::move( to_rm ) , true , par );
      }
     }

   // 4) finally remove from f_theta
   remove_dynamic_variables( f_theta , Subset( rows ) , true , par );
   RescaleGlobalIfNeeded( chnl );

   return( false );
   }

  if( sbst_mod->PFtype() == PolyhedralFunctionMod::ModifyRows ) {
   const auto & A = PF().get_A();

   // 1) obj_lf: positions rows[i]+1
   {
    LinearFunction::Vec_FunctionValue nb( rows.size() );
    Subset nms( rows.size() );
    for( Index i = 0 ; i < rows.size() ; ++i ) {
     nb[ i ] = ScaledRowFactor( rows[ i ] ) *
               PF().get_b()[ rows[ i ] ];
     nms[ i ] = rows[ i ] + 1;
     }
    obj_lf->modify_coefficients( std::move( nb ) , std::move( nms ) ,
                                 true , par );
    }

   // 2) normcns_lf: diag/vert flips per row in subset
   {
    LinearFunction::v_coeff_pair to_add;
    Subset to_rm;
    for( Index i = 0 ; i < rows.size() ; ++i ) {
     ColVariable * p = sub_ptrs[ i ];
     const Index k = nrm_lf->is_active( p );
     const bool in_nrm = ( k < nrm_lf->get_num_active_var() );
     const bool should_in = ! PF().is_row_vertical( rows[ i ] );
     if( in_nrm && ! should_in )
      to_rm.push_back( k );
     else if( should_in && ! in_nrm )
      to_add.emplace_back( p , RowScale( rows[ i ] ) );
     }
    if( ! to_rm.empty() ) {
     std::sort( to_rm.begin() , to_rm.end() );
     nrm_lf->remove_variables( std::move( to_rm ) , true , par );
     }
    if( ! to_add.empty() )
     nrm_lf->add_variables( std::move( to_add ) , par );
    }

   // 3) each coupling[j]: a_{rows[i], j} update per row in subset
   if( f_coupling ) {
    Index j = 0;
    for( auto & c : *f_coupling ) {
     auto cf = static_cast< LinearFunction * >( c.get_function() );
     Subset nms;
     LinearFunction::Vec_FunctionValue ncoef;
     for( Index i = 0 ; i < rows.size() ; ++i ) {
      ColVariable * p = sub_ptrs[ i ];
      const Index k = cf->is_active( p );
      if( k < cf->get_num_active_var() ) {
       nms.push_back( k );
       ncoef.push_back( ScaledRowFactor( rows[ i ] ) *
                        A[ rows[ i ] ][ j ] );
       }
      else if( A[ rows[ i ] ][ j ] != 0 )
       cf->add_variable( p , ScaledRowFactor( rows[ i ] ) *
                              A[ rows[ i ] ][ j ] , par );
      }
     if( ! nms.empty() )
      cf->modify_coefficients( std::move( ncoef ) , std::move( nms ) ,
                               false , par );
     ++j;
     }
    }

   return( false );
   }

  throw( std::invalid_argument(
                       "unknown PolyhedralFunctionModSbst PFtype" ) );
  }

 // Rebuild-and-NBModification fallback for any generic FunctionMod
 // with shift==NaN ("everything changed"). PolyhedralFunctionMod
 // [Addd|Rngd|Sbst] all derive from C05FunctionMod / FunctionMod and
 // are already handled by the incremental branches above; reaching
 // this point means the modification is the bare FunctionMod variant
 // with no incremental info.

 assert( std::isnan( mod->shift() ) );

 f_row_scale.clear();
 f_row_scale.reserve( PF().get_A().size() );
 f_row_measure.clear();
 f_row_measure.reserve( PF().get_A().size() );
 AppendRowScaling( 0 );
 AppendRowMeasure( 0 );
 UpdateGlobalScaleIfNeeded();
 RebuildLinearizedDual();
 return( true );

 }  // end( PolyhedralFunctionBlock::guts_of_add_Modification_PF_dual )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::guts_of_add_Modification_LR_dual( c_p_Mod mod ,
                                                                ChnlName chnl )
{
 // process a Modification produced by the *dual* abstract
 // representation. In contrast to the primal LR direction, the dual
 // abstract structures (theta, normalisation constraint, objective
 // LinearFunction, coupling constraints) are not expected to be
 // modified directly by the user: changes should always come through
 // PF(), which then flows into the dual abstract via
 // guts_of_add_Modification_PF_dual().
 //
 // However the LinearFunction::modify_coefficient(s)() / add_variable(s)()
 // / remove_variables() calls inside guts_of_add_Modification_PF_dual
 // *do* produce Modifications, which are observed by this Block and
 // routed back here through add_Modification(). We must therefore
 // recognise such "self-generated" Modifications and silently absorb
 // them. To do so we check whether the Modification refers to one of
 // our own dual structures, and if so, just let it pass through.

 // Self-generated Modifications on the dual abstract structures:
 // anything coming from a LinearFunction associated to f_normcns, the
 // FRealObjective, or one of the f_coupling constraints is "internal"
 // and should be passed up unchanged (we just return without rebuilding
 // PF()). The same goes for BlockModAdd< ColVariable > / Rmv on
 // f_theta.

 // BlockModAdd / Rmv on f_theta -> internal
 if( auto tmod = dynamic_cast< const BlockModAdd< ColVariable > * >( mod ) ) {
  if( & tmod->whc() == & f_theta )
   return;
  }
 if( dynamic_cast< const BlockModRmvRngd< ColVariable > * >( mod ) )
  return;
 if( dynamic_cast< const BlockModRmvSbst< ColVariable > * >( mod ) )
  return;

 // VariableMod on f_gamma (fix/unfix) -> internal
 if( auto tmod = dynamic_cast< const VariableMod * >( mod ) ) {
  if( tmod->variable() == & f_gamma )
   return;
  // any other VariableMod is not our business
  return;
  }

 // LinearFunctionModVarsAddd / Rngd / Sbst (and C05FunctionModLin*)
 // affecting obj_lf, nrm_lf or any coupling LF -> internal
 auto matches_internal_LF = [ this ]( const Function * f ) -> bool {
  if( auto frobj = static_cast< FRealObjective * >( get_objective() ) )
   if( f == frobj->get_function() )
    return( true );
  if( f == f_normcns.get_function() )
   return( true );
  if( f_coupling )
   for( auto & c : *f_coupling )
    if( f == c.get_function() )
     return( true );
  return( false );
  };

 if( auto tmod = dynamic_cast< const FunctionMod * >( mod ) )
  if( matches_internal_LF( tmod->function() ) )
   return;

 // RowConstraintMod on f_normcns (e.g. set_lhs/set_rhs from set_lambda)
 // or on any f_coupling constraint -> internal
 if( auto tmod = dynamic_cast< const RowConstraintMod * >( mod ) ) {
  if( tmod->constraint() == & f_normcns )
   return;
  if( f_coupling )
   for( auto & c : *f_coupling )
    if( tmod->constraint() == & c )
     return;
  // any other RowConstraintMod is not about our structures, ignore
  return;
  }

 // ObjectiveMod (e.g. set_sense from convex/concave flip) -> internal
 if( dynamic_cast< const ObjectiveMod * >( mod ) )
  return;

 // anything else from somewhere in the dual abstract representation:
 // not supported, throw
 throw( std::logic_error( "PolyhedralFunctionBlock: unsupported Modification "
                          "on dual abstract representation" ) );

 }  // end( PolyhedralFunctionBlock::guts_of_add_Modification_LR_dual )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::ConstructLPConstraint( Index i ,
						     FRowConstraint & ci )
{
 // if the PolyhedralFunction is convex, then the (diagonal) constraint is
 // b_i <= v - A_i x <= INF, otherwise it is -INF <= v - A_i x <= b_i.
 // For vertical rows the encoding is identical except that the coefficient
 // of v is zero, so the LP constraint becomes a "domain" constraint on x:
 //   convex  vertical:  b_i <= - A_i x <= INF   (i.e. A_i x + b_i <= 0)
 //   concave vertical:  -INF <= - A_i x <= b_i  (i.e. A_i x + b_i >= 0)
 const auto local_scale = RowScale( i );
 const auto scale = ScaledRowFactor( i );

 ci.set_lhs( PF().is_convex() ? scale * PF().get_b()[ i ]
	                         : -Inf< Function::FunctionValue >() ,
	     eNoMod );
 ci.set_rhs( PF().is_convex() ? Inf< Function::FunctionValue >()
	                         : scale * PF().get_b()[ i ] ,
	     eNoMod );

 const auto nv = PF().get_num_active_var();
 LinearFunction::v_coeff_pair vars( nv + 1 );
 auto vit = vars.begin();

 // v is the *first* Variable of the LinearFunction, since it is the only
 // one that "never moves"; as a consequence, x[ i ] is the (i+1)-th active
 // Variable in each constraint. Its coefficient is the local row scale for
 // diagonal rows and 0 for vertical rows.
 *(vit++) = std::make_pair( LinearizedV() ,
			    PF().is_row_vertical( i ) ? 0.0 : local_scale );

 auto Aiit = PF().get_A()[ i ].begin();
 for( Index j = 0 ; j < nv ; ++j )
  *(vit++) = std::make_pair( static_cast< ColVariable * >(
					      PF().get_active_var( j ) ) ,
			     - scale * *(Aiit++) );

 ci.set_function( new LinearFunction( std::move( vars ) ) , eNoMod );

 }  // end( PolyhedralFunctionBlock::ConstructLPConstraint )

/*--------------------------------------------------------------------------*/

Function::FunctionValue
PolyhedralFunctionBlock::ComputeRowMeasure( Index i ) const
{
 Function::FunctionValue mx = std::max( 1.0 ,
                                        std::abs( PF().get_b()[ i ] ) );
 for( const auto a : PF().get_A()[ i ] )
  mx = std::max( mx , std::abs( a ) );

 return( std::isfinite( mx ) ? mx : 1.0 );

 }  // end( PolyhedralFunctionBlock::ComputeRowMeasure )

/*--------------------------------------------------------------------------*/

Function::FunctionValue
PolyhedralFunctionBlock::ComputeRowScale( Index i ) const
{
 if( ! has_local_scaling() )
  return( 1.0 );

 return( 1.0 / std::sqrt( ComputeRowMeasure( i ) ) );

 }  // end( PolyhedralFunctionBlock::ComputeRowScale )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::RefreshRowMeasure( Index i )
{
 f_row_measure[ i ] = ComputeRowMeasure( i );

 }  // end( PolyhedralFunctionBlock::RefreshRowMeasure )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::RefreshRowMeasure( Range range )
{
 for( Index i = range.first ; i < range.second ; ++i )
  RefreshRowMeasure( i );

 }  // end( PolyhedralFunctionBlock::RefreshRowMeasure )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::RefreshRowMeasure( const Subset & rows )
{
 for( const auto i : rows )
  RefreshRowMeasure( i );

 }  // end( PolyhedralFunctionBlock::RefreshRowMeasure )

/*--------------------------------------------------------------------------*/

Function::FunctionValue PolyhedralFunctionBlock::RowScale( Index i ) const
{
 return( f_row_scale[ i ] );

 }  // end( PolyhedralFunctionBlock::RowScale )

/*--------------------------------------------------------------------------*/

Function::FunctionValue
PolyhedralFunctionBlock::ScaledRowFactor( Index i ) const
{
 return( f_global_scale * RowScale( i ) );

 }  // end( PolyhedralFunctionBlock::ScaledRowFactor )

/*--------------------------------------------------------------------------*/

Function::FunctionValue
PolyhedralFunctionBlock::ScaledBound( Function::FunctionValue value ) const
{
 return( f_global_scale * value );

 }  // end( PolyhedralFunctionBlock::ScaledBound )

/*--------------------------------------------------------------------------*/

ColVariable * PolyhedralFunctionBlock::LinearizedV( void )
{
 return( has_global_scaling() ? & f_scaled_v : & f_v );

 }  // end( PolyhedralFunctionBlock::LinearizedV )

/*--------------------------------------------------------------------------*/

Function::FunctionValue
PolyhedralFunctionBlock::ComputeGlobalMeasure( void ) const
{
 assert( f_row_measure.size() == PF().get_A().size() );

 Function::FunctionValue mx = 1.0;
 for( const auto measure : f_row_measure )
  mx = std::max( mx , measure );

 return( std::isfinite( mx ) ? mx : 1.0 );

 }  // end( PolyhedralFunctionBlock::ComputeGlobalMeasure )

/*--------------------------------------------------------------------------*/

bool PolyhedralFunctionBlock::UpdateGlobalScaleIfNeeded( bool force )
{
 if( ! has_global_scaling() )
  return( false );

 const auto measure = ComputeGlobalMeasure();
 if( ! force ) {
  const auto ratio = measure / f_global_reference;
  if( ( ratio >= 1.0 / k_global_rescale_ratio ) &&
      ( ratio <= k_global_rescale_ratio ) )
   return( false );
  }

 const auto new_scale = 1.0 / std::sqrt( measure );
 f_global_reference = measure;
 if( new_scale == f_global_scale )
  return( false );

 f_global_scale = new_scale;
 return( true );

 }  // end( PolyhedralFunctionBlock::UpdateGlobalScaleIfNeeded )

/*--------------------------------------------------------------------------*/

bool PolyhedralFunctionBlock::RescaleGlobalIfNeeded( ChnlName chnl )
{
 if( ! UpdateGlobalScaleIfNeeded() )
  return( false );

 if( is_linearized() && ( f_rep & k_built_cnst ) ) {
  UpdateLinearizedPrimalScale( chnl );
  return( true );
  }

 if( is_dual() && ( f_rep & k_built_cnst ) && ( f_rep & k_built_obj ) ) {
  UpdateLinearizedDualScale( chnl );
  return( true );
  }

 return( false );

 }  // end( PolyhedralFunctionBlock::RescaleGlobalIfNeeded )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::UpdateLinearizedPrimalScale( ChnlName chnl )
{
 // A global rescaling does not change the identity or sparsity pattern of
 // any cut. Keep every existing Constraint and LinearFunction in place and
 // only update the coefficients that depend on f_global_scale.
 auto par = make_par( eNoBlck , chnl );
 auto scale_lf = static_cast< LinearFunction * >(
                                             f_scale_cns.get_function() );
 scale_lf->modify_coefficient( 1 , - f_global_scale , par );

 const auto nv = PF().get_num_active_var();
 auto cit = f_const.begin();
 for( Index i = 0 ; cit != f_const.end() ; ++i , ++cit ) {
  const auto scale = ScaledRowFactor( i );
  LinearFunction::Vec_FunctionValue coeff( nv );
  for( Index j = 0 ; j < nv ; ++j )
   coeff[ j ] = - scale * PF().get_A()[ i ][ j ];
  static_cast< LinearFunction * >( cit->get_function() )->
          modify_coefficients( std::move( coeff ) , Range( 1 , nv + 1 ) ,
                               par );
  if( PF().is_convex() )
   cit->set_lhs( scale * PF().get_b()[ i ] , par );
  else
   cit->set_rhs( scale * PF().get_b()[ i ] , par );
  }

 }  // end( PolyhedralFunctionBlock::UpdateLinearizedPrimalScale )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::UpdateLinearizedDualScale( ChnlName chnl )
{
 // Preserve the theta Variables and every LinearFunction. A global
 // rescaling only changes numerical coefficients and, in the stand-alone
 // dual form, the normalization RHS.
 auto par = make_par( eNoBlck , chnl );
 auto frobj = static_cast< FRealObjective * >( get_objective() );
 auto obj_lf = static_cast< LinearFunction * >( frobj->get_function() );

 LinearFunction::Vec_FunctionValue obj_coeff( 1 + f_theta.size() );
 obj_coeff[ 0 ] = PF().is_bound_set()
                  ? ScaledBound( PF().get_global_bound() ) : 0.0;
 Index i = 0;
 for( const auto & theta : f_theta ) {
  obj_coeff[ i + 1 ] = ScaledRowFactor( i ) * PF().get_b()[ i ];
  ++i;
  }
 obj_lf->modify_coefficients( std::move( obj_coeff ) ,
                              Range( 0 , 1 + f_theta.size() ) , par );

 auto nrm_lf = static_cast< LinearFunction * >( f_normcns.get_function() );
 if( f_lambda ) {
  const auto k = nrm_lf->is_active( f_lambda );
  if( k >= nrm_lf->get_num_active_var() )
   throw( std::logic_error( "normalization lambda is not active" ) );
  nrm_lf->modify_coefficient( k , -1.0 / f_global_scale , par );
  }
 else {
  f_normcns.set_lhs( 1.0 / f_global_scale , par );
  f_normcns.set_rhs( 1.0 / f_global_scale , par );
  }

 if( f_coupling ) {
  Index j = 0;
  for( auto & c : *f_coupling ) {
   auto cf = static_cast< LinearFunction * >( c.get_function() );
   Subset nms;
   LinearFunction::Vec_FunctionValue coeff;
   i = 0;
   for( auto & theta : f_theta ) {
    const auto k = cf->is_active( & theta );
    if( k < cf->get_num_active_var() ) {
     nms.push_back( k );
     coeff.push_back( ScaledRowFactor( i ) * PF().get_A()[ i ][ j ] );
     }
    ++i;
    }
   if( ! nms.empty() )
    cf->modify_coefficients( std::move( coeff ) , std::move( nms ) , false ,
                             par );
   ++j;
   }
  }

 }  // end( PolyhedralFunctionBlock::UpdateLinearizedDualScale )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::RebuildLinearizedPrimal( void )
{
 if( has_global_scaling() )
  f_scale_cns.set_function( new LinearFunction(
                { std::make_pair( & f_scaled_v , 1.0 ) ,
                  std::make_pair( & f_v , - f_global_scale ) } ) , eNoMod );

 f_const.resize( PF().get_A().size() );
 auto cit = f_const.begin();
 for( Index i = 0 ; i < PF().get_A().size() ; ++i , ++cit ) {
  cit->set_Block( this );
  ConstructLPConstraint( i , *cit );
  }

 AbstractBlock::add_Modification( std::make_shared< NBModification >( this ) );

 }  // end( PolyhedralFunctionBlock::RebuildLinearizedPrimal )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::RebuildLinearizedDual( void )
{
 auto frobj = static_cast< FRealObjective * >( get_objective() );

 // Preserve entries owned by the parent Block, such as the lambda installed
 // by set_lambda(), while replacing every theta-dependent coefficient.
 std::unordered_set< const Variable * > old_theta;
 old_theta.reserve( f_theta.size() );
 for( const auto & t : f_theta )
  old_theta.insert( & t );

 std::vector< LinearFunction::v_coeff_pair > cpl_kept;
 if( f_coupling ) {
  cpl_kept.resize( f_coupling->size() );
  Index j = 0;
  for( auto & c : *f_coupling ) {
   auto cf = static_cast< LinearFunction * >( c.get_function() );
   if( cf )
    for( const auto & p : cf->get_v_var() )
     if( ! old_theta.count( p.first ) )
      cpl_kept[ j ].push_back( p );
   ++j;
   }
  }

 LinearFunction::v_coeff_pair nrm_kept;
 if( auto nrm_lf = static_cast< LinearFunction * >(
                                             f_normcns.get_function() ) )
  for( auto p : nrm_lf->get_v_var() )
   if( ! old_theta.count( p.first ) && ( p.first != & f_gamma ) ) {
    if( p.first == f_lambda )
     p.second = -1.0 / f_global_scale;
    nrm_kept.push_back( p );
    }

 const Index nr = PF().get_A().size();
 std::list< ColVariable > new_theta;
 std::vector< ColVariable * > new_ptrs;
 new_ptrs.reserve( nr );
 for( Index i = 0 ; i < nr ; ++i ) {
  new_theta.emplace_back();
  auto & v = new_theta.back();
  v.is_positive( true , eNoMod );
  v.set_Block( this );
  new_ptrs.push_back( & v );
  }

 if( PF().is_bound_set() ) {
  if( f_gamma.is_fixed() )
   f_gamma.is_fixed( false , eNoMod );
  }
 else {
  f_gamma.set_value( 0 );
  if( ! f_gamma.is_fixed() )
   f_gamma.is_fixed( true , eNoMod );
  }

 {
  LinearFunction::v_coeff_pair obj_vp;
  obj_vp.reserve( 1 + nr );
  const double bnd = PF().is_bound_set()
                     ? ScaledBound( PF().get_global_bound() )
                     : 0.0;
  obj_vp.emplace_back( & f_gamma , bnd );
  for( Index i = 0 ; i < nr ; ++i )
   obj_vp.emplace_back( new_ptrs[ i ] ,
                        ScaledRowFactor( i ) * PF().get_b()[ i ] );
  frobj->set_function( new LinearFunction( std::move( obj_vp ) ) , eNoMod );
  }

 {
  LinearFunction::v_coeff_pair nrm_vp;
  nrm_vp.reserve( 1 + nr + nrm_kept.size() );
  nrm_vp.emplace_back( & f_gamma , 1.0 );
  for( Index i = 0 ; i < nr ; ++i )
   if( ! PF().is_row_vertical( i ) )
    nrm_vp.emplace_back( new_ptrs[ i ] , RowScale( i ) );
  for( auto & p : nrm_kept )
   nrm_vp.push_back( p );
  f_normcns.set_function( new LinearFunction( std::move( nrm_vp ) ) ,
                          eNoMod );
  if( f_lambda ) {
   f_normcns.set_lhs( 0.0 , eNoMod );
   f_normcns.set_rhs( 0.0 , eNoMod );
   }
  else {
   f_normcns.set_lhs( 1.0 / f_global_scale , eNoMod );
   f_normcns.set_rhs( 1.0 / f_global_scale , eNoMod );
   }
  }

 if( f_coupling ) {
  const Index nv = PF().get_num_active_var();
  Index j = 0;
  auto cit = f_coupling->begin();
  for( ; cit != f_coupling->end() && j < nv ; ++cit , ++j ) {
   LinearFunction::v_coeff_pair vp = std::move( cpl_kept[ j ] );
   for( Index i = 0 ; i < nr ; ++i ) {
    const double a = ScaledRowFactor( i ) * PF().get_A()[ i ][ j ];
    if( a != 0 )
     vp.emplace_back( new_ptrs[ i ] , a );
    }
   cit->set_function( new LinearFunction( std::move( vp ) ) , eNoMod );
   }
  }

 f_theta.swap( new_theta );

 AbstractBlock::add_Modification( std::make_shared< NBModification >( this ) );

 }  // end( PolyhedralFunctionBlock::RebuildLinearizedDual )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::InitialiseScaling( void )
{
 f_global_scale = 1.0;
 f_global_reference = 1.0;

 f_row_scale.clear();
 f_row_scale.reserve( PF().get_A().size() );
 f_row_measure.clear();
 f_row_measure.reserve( PF().get_A().size() );
 AppendRowScaling( 0 );
 AppendRowMeasure( 0 );
 UpdateGlobalScaleIfNeeded( true );

 }  // end( PolyhedralFunctionBlock::InitialiseScaling )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::AppendRowScaling( Index first )
{
 for( Index i = first ; i < PF().get_A().size() ; ++i )
  f_row_scale.push_back( ComputeRowScale( i ) );

 }  // end( PolyhedralFunctionBlock::AppendRowScaling )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::AppendRowMeasure( Index first )
{
 for( Index i = first ; i < PF().get_A().size() ; ++i )
  f_row_measure.push_back( ComputeRowMeasure( i ) );

 }  // end( PolyhedralFunctionBlock::AppendRowMeasure )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::DeleteRowScaling( Range range )
{
 f_row_scale.erase( f_row_scale.begin() + range.first ,
                    f_row_scale.begin() + range.second );
 f_row_measure.erase( f_row_measure.begin() + range.first ,
                      f_row_measure.begin() + range.second );

 }  // end( PolyhedralFunctionBlock::DeleteRowScaling )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::DeleteRowScaling( const Subset & rows )
{
 for( auto rit = rows.rbegin() ; rit != rows.rend() ; ++rit )
  f_row_scale.erase( f_row_scale.begin() + *rit );
 for( auto rit = rows.rbegin() ; rit != rows.rend() ; ++rit )
  f_row_measure.erase( f_row_measure.begin() + *rit );

 }  // end( PolyhedralFunctionBlock::DeleteRowScaling )

/*--------------------------------------------------------------------------*/
/*----------------------- remove_redundant_rows() --------------------------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::remove_redundant_rows(
				 BlockSolverConfig * solver_config )
{
 // the four tolerances are read from the "extra" Configuration of this
 // Block's BlockConfig, a SimpleConfiguration< std::vector< double > > with
 // up to four entries [ parallel_abs, parallel_rel, optimization_abs,
 // optimization_rel ]; any missing entry ( or a missing Configuration )
 // defaults to 0
 double par_abs = 0 , par_rel = 0 , opt_abs = 0 , opt_rel = 0 ;
 if( f_BlockConfig )
  if( auto tc = dynamic_cast< SimpleConfiguration< std::vector< double > > * >(
				 f_BlockConfig->f_extra_Configuration ) ) {
   const auto & tv = tc->f_value;
   if( tv.size() > 0 ) par_abs = tv[ 0 ];
   if( tv.size() > 1 ) par_rel = tv[ 1 ];
   if( tv.size() > 2 ) opt_abs = tv[ 2 ];
   if( tv.size() > 3 ) opt_rel = tv[ 3 ];
   }

 // first eliminate the parallel ( dominated ) rows geometrically, then the
 // inactive ones via the LP below
 PF().remove_parallel_rows( par_abs , par_rel );

 auto & polyf = PF();
 const auto num_rows = polyf.get_nrows();
 if( num_rows <= 1 )
  return;

 const bool convex = polyf.is_convex();
 const double s = convex ? - 1.0 : 1.0;
 const auto nv = polyf.get_num_active_var();
 const auto & A = polyf.get_A();
 const auto & b = polyf.get_b();

 // The "active" x of the PolyhedralFunction are not Variable of this Block
 // (they live in the parent model), so the epigraph LP cannot be assembled
 // over them directly. Build a transient, self-contained LP: an AbstractBlock
 // holding fresh *free* x, with an inner PolyhedralFunctionBlock that SHARES
 // this' PolyhedralFunction (via the pointer constructor), so that its own
 // linearized representation (v, the v >= a_i x + b_i rows) is reused as the
 // LP. The active Variables are silently swapped to the free x while the
 // inner constraints are generated, and restored at the end. Row h is
 // redundant iff, with its own constraint relaxed, max s ( v - a_h x ) < s b_h
 // within tolerances; the removals are applied only at the end, so every test
 // sees the full cut set.

 // save the current active Variables, to be restored at the end
 PolyhedralFunction::VarVector saved_vars( nv );
 for( Index j = 0 ; j < nv ; ++j )
  saved_vars[ j ] = static_cast< ColVariable * >( polyf.get_active_var( j ) );

 auto AB = new AbstractBlock;

 auto x = new std::vector< ColVariable >( nv );  // fresh free x, owned by AB
 AB->add_static_variable( * x , "x" );

 PolyhedralFunction::VarVector new_vars( nv );
 for( Index j = 0 ; j < nv ; ++j )
  new_vars[ j ] = & ( * x )[ j ];

 // silently point the PolyhedralFunction at the free x (set_variables issues
 // no Modification); the inner Block builds its constraints on these
 polyf.set_variables( std::move( new_vars ) );

 auto inner = new PolyhedralFunctionBlock( AB , & polyf );
 AB->add_nested_Block( inner );

 SimpleConfiguration< int > lin_cfg( 1 );  // linearized primal representation
 inner->generate_abstract_variables( & lin_cfg );
 inner->generate_abstract_constraints();
 inner->generate_objective();

 // from now on the inner Block must NOT mirror the changes we make to its
 // Constraint / Objective back onto the shared PolyhedralFunction (which
 // would corrupt it): the Modification still reach the Solver, PF() is
 // left untouched
 inner->set_play_dumb();

 // reuse the inner's own objective ( min v convex, max v concave ) as the LP
 // objective, extending it into min/max ( v - a_h x ) by appending the free x
 // ( coefficient - a_hj, set per row h below ); v is its first Variable, so
 // the x sit at positions 1 .. nv
 auto of = static_cast< LinearFunction * >(
	    static_cast< FRealObjective * >( inner->get_objective()
					     )->get_function() );
 for( Index j = 0 ; j < nv ; ++j )
  of->add_variable( & ( * x )[ j ] , 0.0 );

 solver_config->apply( AB );
 auto solver = AB->get_registered_solvers().front();

 auto & inner_const = inner->f_const;

 Subset rows_to_remove;
 rows_to_remove.reserve( num_rows );

 for( Index h = 0 ; h < num_rows ; ++h ) {
  for( Index j = 0 ; j < nv ; ++j )       // aim the objective at row h
   of->modify_coefficient( j + 1 , - A[ h ][ j ] );

  auto ch = std::next( inner_const.begin() , h );
  const auto saved = convex ? ch->get_lhs() : ch->get_rhs();
  if( convex )                                       // relax row h
   ch->set_lhs( - Inf< Function::FunctionValue >() );
  else
   ch->set_rhs( Inf< Function::FunctionValue >() );

  if( ( solver->compute() == Solver::kOK ) && solver->has_var_solution() ) {
   solver->get_var_solution();
   // objective value max s ( v - a_h x ), read from the solution rather than
   // via get_var_value() ( which is unreliable across these warm-started
   // re-solves ); a non-finite value flags an unbounded relaxation, i.e. row
   // h is needed, so it is not removed
   Function::FunctionValue val = s * inner->f_v.get_value();
   for( Index j = 0 ; j < nv ; ++j )
    val -= s * A[ h ][ j ] * ( * x )[ j ].get_value();
   if( std::isfinite( val ) &&
       ( val < s * b[ h ] + opt_abs ) &&
       ( val < s * b[ h ] + opt_rel *
	 std::max( std::abs( val ) , std::abs( b[ h ] ) ) ) )
    rows_to_remove.push_back( h );
   }

  if( convex )                                       // restore row h
   ch->set_lhs( saved );
  else
   ch->set_rhs( saved );
  }

 // tear down: detach inner from AB ( so ~AbstractBlock does not delete it ),
 // destroy inner ( it shares polyf, hence does NOT destroy it ) while the free
 // x are still alive, then destroy AB ( which destroys the x )
 delete( solver );
 AB->access_nested_Blocks().clear();
 delete( inner );
 delete( AB );

 // restore the original active Variables, then apply the removals: the
 // Modification now reaches this Block ( the PolyhedralFunction's Observer )
 // and its parent, keeping the real model in sync
 polyf.set_variables( std::move( saved_vars ) );
 polyf.delete_rows( std::move( rows_to_remove ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------- End File PolyhedralFunctionBlock.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
