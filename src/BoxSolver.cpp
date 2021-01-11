/*--------------------------------------------------------------------------*/
/*--------------------------- File BoxSolver.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the BoxSolver class, which implements a CDASolver for
 * problems (or relaxations thereof) with an extremely simple structure:
 * only bound (box) Constraint on the ColVariable and a separable Objective
 * (a FRealObjective with either a LinearFunction or a DQuadFunction inside).
 *
 * \version 0.10
 *
 * \date 09 - 01 - 2021
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *
 * Copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"

#include "BoxSolver.h"

#include "FRealObjective.h"

#include "DQuadFunction.h"

#include "LinearFunction.h"

#include "OneVarConstraint.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------- CONSTANTS --------------------------------*/
/*--------------------------------------------------------------------------*/

static constexpr auto INF = Inf< BoxSolver::OFValue >();

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register BoxSolver to the Solver factory

SMSpp_insert_in_factory_cpp_0( BoxSolver );

/*--------------------------------------------------------------------------*/
/*---------------------------- METHODS of Solver ---------------------------*/
/*--------------------------------------------------------------------------*/

int BoxSolver::compute( bool changedvars )
{
 if( ! f_Block )
  return( kError );

 if( f_sense < 0 )  // have to compute/update the sense
  check_sense( f_Block );

 if( f_state != kUnEval )  // done already
  return( f_state );

 f_max_val = f_min_val = 0;

 auto f = std::bind( &BoxSolver::process_variable , this ,
		     std::placeholders::_1 );

 // process static variables
 for( const auto & el : f_Block->get_static_variables() ) {
  if( un_any_const_static( el , f , un_any_type< ColVariable >() ) )
   continue;
  throw( std::invalid_argument(
		       "BoxSolver: static variable not a ColVariable" ) );
  }

 // process dynamic variables
 for( const auto & el : f_Block->get_dynamic_variables() ) {
  if( un_any_const_dynamic( el , f , un_any_type< ColVariable >() ) )
   continue;
  throw( std::invalid_argument(
		       "BoxSolver: dynamic variable not a ColVariable" ) );
  }

 // now see what value must be returned
 if( f_state != kInfeasible ) {
  if( f_sense )  // maximization
   if( f_max_val == INF )
    f_state = kUnbounded;
   else
    f_state = kOK;
  else           // minimization
   if( f_min_val == - INF )
    f_state = kUnbounded;
   else
    f_state = kOK;
  }

 return( f_state );

 }  // end( BoxSolver::compute )

/*--------------------------------------------------------------------------*/
/*------------- METHODS FOR ADDING / REMOVING / CHANGING DATA --------------*/
/*--------------------------------------------------------------------------*/

void BoxSolver::add_Modification( sp_Mod & mod )
{
 if( f_no_Mod || ( ! f_Block ) )
  return;

 // changing the sense
 if( std::dynamic_pointer_cast< const ObjectiveMod >( mod ) ) {
  f_sense = -1;  // check it has to be set
  return;
  }

 // changes in a Variable
 if( std::dynamic_pointer_cast< const VariableMod >( mod ) ) {
  reset();
  return;
  }

 // changes in a Constraint
 if( auto tmod = std::dynamic_pointer_cast< const ConstraintMod >( mod ) ) {
  // the Constraint is a box one
  if( dynamic_cast< OneVarConstraint * >( tmod->constraint() ) )
   reset();

  // else ignore it, since non-box Constraint themselves are ignored
  return;
  }

 // changes in a Function
 if( auto tmod = std::dynamic_pointer_cast< const FunctionMod >( mod ) ) {
  // the Function is inside an Objective
  if( dynamic_cast< Objective * >( tmod->function()->get_Observer() ) )
   reset();

  // else ignore it, the Function is in a Constraint that is ignored
  return;
  }

 // changes in the active Variables of a Function
 if( auto tmod = std::dynamic_pointer_cast< const FunctionModVars >( mod )
     ) {
  // the Function is inside an Objective
  if( dynamic_cast< Objective * >( tmod->function()->get_Observer() ) )
   reset();

  // else ignore it, the Function is in a Constraint that is ignored
  return;
  }

 // changes in the Block
 if( std::dynamic_pointer_cast< const BlockMod >( mod ) ) {
  reset();
  return;
  }

 // additions/deletions in the Block
 if( std::dynamic_pointer_cast< const BlockModAD >( mod ) )
  reset();

 // if anything else remains, ignore it: it's not a change that impacts
 // on the parts of the Block that BoxSolver looks at

 }  // end( BoxSolver::add_Modification )

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

void BoxSolver::check_sense( Block * blck )
{
 if( auto obj = blck->get_objective() ) {
  auto sense = obj->get_sense();
  if( f_sense < 0 )
   f_sense = sense;
  else
   if( f_sense != sense )
    throw( std::invalid_argument(
		     "BoxSolver: Block with non-uniform Objective sense" ) );
  }

 for( auto b :  blck->get_nested_Blocks() )
  check_sense( b );
 }
 
/*--------------------------------------------------------------------------*/

void BoxSolver::process_variable( ColVariable & var )
{
 using VarValue = ColVariable::VarValue;
 
 OFValue a = 0;  // quadratic term
 OFValue b = 0;  // linear term
 VarValue l;     // lower bound
 VarValue u;     // upper bound
 OneVarConstraint * cl = nullptr;  // constraint of active lower bound
 OneVarConstraint * cu = nullptr;  // constraint of active upper bound

 // initialize upper and lower bound
 if( var.is_fixed() )
  l = u = var.get_value();
 else {
  l = var.get_lb();
  u = var.get_ub();
  }

 // scan through all active stuff: compute in l and u the tightest lower
 // and upper bounds, in a and b the total (sum of) quadratic and linear
 // coefficients, and in cl, cu the pointer to the OneVarConstraint that
 // correspond to the "tight" cl / cu (if any)
 for( Block::Index i = 0 ; i < var.get_num_active() ; ++i ) {
  auto ai = var.get_active( i );

  // check that the ThinVarDepInterface belongs to the Block (or any of
  // its sub-Block, recursively)
  auto blck = ai->get_Block();
  while( blck && ( blck != f_Block ) )
   blck = blck->get_f_Block();

  if( ! blck )  // if not
   continue;    // ignore it: it must be defined in a Block enclosing the
                // one to which BoxSolver is registered, which means it is
                // not something that BoxSolver "sees"

  // Constraint
  if( auto ci = dynamic_cast< Constraint * >( ai ) ) {
   // a OneVarConstraint
   if( auto ovr = dynamic_cast< OneVarConstraint * >( ci ) ) {
    auto lhs = ovr->get_lhs();
    if( lhs >= l ) {
     l = lhs;
     cl = ovr;
     }
    auto rhs = ovr->get_rhs();
    if( rhs <= u ) {
     u = rhs;
     cu = ovr;
     }

    if( f_sol & 4 )       // if the dual solution is computed
     ovr->set_dual( 0 );  // it is 0 unless otherwise proven
    continue;
    }

   if( f_sol & 4 )        // if the dual solution is computed
    if( auto rc = dynamic_cast< RowConstraint * >( ai ) )
     rc->set_dual( 0 );  // it is 0 on all RowConstraint
     
   continue;              // any other Constraint is ignored
   }

  // Objective
  if( auto oi = dynamic_cast< Objective * >( ai ) ) {
   auto fro = dynamic_cast< FRealObjective * >( oi );
   if( ! fro )
    throw( std::invalid_argument( "BoxSolver: not a FRealObjective" ) );

   if( auto lf = dynamic_cast< LinearFunction * >( fro->get_function() ) ) {
    // WARNING: INEFFICIENT!!
    b += lf->get_coefficient( lf->is_active( & var ) );
    continue;
    }

   if( auto qf = dynamic_cast< DQuadFunction * >( fro->get_function() ) ) {
    auto p = qf->is_active( & var );    // WARNING: INEFFICIENT!!
    b += qf->get_linear_coefficient( p );
    a += qf->get_quadratic_coefficient( p );
    continue;
    }
   
   throw( std::invalid_argument(
	       "BoxSolver: invalid Function inside the FRealObjective" ) );
   }

  throw( std::invalid_argument( "BoxSolver: invalid ThinVarDepInterface" ) );
  
  }  // end( for( i ) ) 

 // now finally perform the minimization / maximization
 // the problem is
 //
 //    min / max { a x^2 + b x : l <= x <= u }
 //
 // note that the gradient of the objective is g( x ) = a x + b, while the
 // gradients of the constraints are +1 and -1. when a constraint is "tight"
 // (active and determining the optimal solution), its dual multiplier y >= 0
 // must therefore satisfy the KKY condition
 //
 //    y ( +/- 1 ) + ( a x + b ) = 0
 //
 // with the only delicate choice, as usual, being the sign

 // if the variable is integer, start with appropriately shrinking the
 // interval by making the extremes integer; note that if, say, l == u ==
 // a fractional number, this immediately makes the interval empty, as it
 // should be
 if( var.is_integer() ) {
  l = std::ceil( l );
  u = std::floor( u );
  }

 // now check unfeasibility: if l > u then the whole problem is unfeasible,
 // and it is so for both senses
 if( l > u ) {
  f_state = kInfeasible;
  return;
  }

 if( ( a == 0 ) && ( b == 0 ) ) {  // the easy-easy-case: constant obj
  if( f_sol & 1 )  // in case you want an optimal value
                   // any finite value is fine, take it "close to 0"
   var.set_value( std::min( u , std::max( l , VarValue( 0 ) ) ) );
  return;        // this ColVariable changes nothing; note that the dual
                 // solution, if required, is already set to 0, which is OK
  }

 if( a == 0 ) {  // linear but nontrivial: b != 0
  if( f_sense == 1 ) {               // maximization
   if( b > 0 ) {                     // with b > 0
    // the original problem
    if( u == INF )                   // if the upper bound is +INF
     f_max_val = INF;                // max is unbounded above
    else {                           // if the upper bound is finite
     if( f_max_val < INF ) {         // problem not unbounded already
      f_max_val += b * u;            // add the contribution
      if( f_sol & 1 )
       var.set_value( u );           // primal solution
      if( ( f_sol & 2 ) && cu )
       cu->set_dual( b );            // dual solution
      }
     }
    // the opposite problem
    if( l == - INF )                 // if the lower bound is -INF
     f_min_val = - INF;              // min is unbounded below
    else                             // if the lower bound is finite
     if( f_min_val > - INF )         // problem not unbounded already
      f_min_val += b * l;            // add the contribution
    }
   else {                            // [maximization] with b < 0
    // the original problem
    if( l == - INF )                 // if the lower bound is -INF
     f_max_val = INF;                // max is unbounded above
    else {                           // if the lower bound is finite
     if( f_max_val < INF ) {         // problem not unbounded already
      f_max_val += b * l;            // add the contribution
      if( f_sol & 1 )
       var.set_value( l );           // primal solution
      if( ( f_sol & 2 ) && cl )
       cl->set_dual( b );            // dual solution
      }
     }
    // the opposite problem
    if( u == INF )                   // if the upper bound is INF
     f_min_val = - INF;              // min is unbounded below
    else                             // if the upper bound is finite
     if( f_min_val > - INF )         // problem is unbounded already
      f_min_val += b * u;            // add the contribution
    }
   }
  else {                             // minimization
   if( b > 0 ) {                     // with b > 0
    // the original problem
    if( l == - INF )                 // if the lower bound is -INF
     f_min_val = - INF;              // min is unbounded below
    else {                           // if the lower bound is finite
     if( f_min_val > - INF ) {       // problem not unbounded already
      f_min_val += b * l;            // add the contribution
      if( f_sol & 1 )
       var.set_value( l );           // primal solution
      if( ( f_sol & 2 ) && cu )
       cu->set_dual( b );            // dual solution
      }
     }
    // the opposite problem
    if( u == INF )                   // if the upper bound is INF
     f_max_val = INF;                // max is unbounded below
    else                             // if the lower bound is finite
     if( f_max_val < INF )           // problem not unbounded already
      f_max_val += b * u;            // add the contribution
    }
   else {                            // [minimization] with b < 0
    // the original problem
    if( u == INF )                   // if the upper bound is INF
     f_min_val = - INF;              // min is unbounded below
    else {                           // if the upper bound is finite
     if( f_min_val > - INF ) {       // problem not unbounded already
      f_min_val += b * u;            // add the contribution
      if( f_sol & 1 )
       var.set_value( u );           // primal solution
      if( ( f_sol & 2 ) && cl )
       cl->set_dual( b );            // dual solution
      }
     }
    // the opposite problem
    if( l == - INF )                 // if the lower bound is - INF
     f_max_val = INF;                // max is unbounded above
    else                             // if the lower bound is finite
     if( f_max_val < INF )           // problem is unbounded already
      f_max_val += b * l;            // add the contribution
    }
   }

  return;  // the nontrivial linear case has been dealt with
  }

 // deal with the quadratic case: a != 0
 // the unique stationary point (max or min depending on the sign of a)
 // of a x^2 + b is at x = - b / ( 2 * a );
 OFValue x = - b / ( 2 * a );
 auto q = [ & ]( OFValue y ) -> OFValue { return( ( a * y + b ) * y ); };
 
 if( f_sense == 1 ) {               // maximization
  if( a < 0 ) {                     // with a < 0
   OFValue vxmax;

   // the original problem
   if( var.is_integer() ) {         // on an integer variable
    OFValue xm = std::max( l , std::floor( x ) );
    OFValue vxm = q( xm );
    OFValue xp = std::min( u , std::ceil( x ) );
    OFValue vxp = q( xp );
    if( vxm > vxp ) { x = xm; vxmax = vxm; }
    else            { x = xp; vxmax = vxp; }
    // note: no dual solution since it's integer
    }
   else {                           // on a continuous variable
    if( x > u ) {
     x = u;
     if( ( f_sol & 2 ) && cu )
      cu->set_dual( 2 * a * x + b );  // dual solution
     }
    else
     if( x < l ) {
      x = l;
      if( ( f_sol & 2 ) && cl )
       cl->set_dual( 2 * a * x + b );  // dual solution
      }

    vxmax = q( x );
    }

   if( f_sol & 1 )
    var.set_value( x );
   f_max_val += vxmax;

   // the opposite problem
   if( ( l == - INF ) || ( u == INF ) )
    f_min_val = - INF;
   else
    f_min_val += std::min( q( l ) , q( u ) );
   }
  else {                            // [maximization] with a > 0
   // the original problem
   // note: no dual solution since it's convex maximization
   if( ( l == - INF ) || ( u == INF ) )
    f_max_val = INF;
   else {
    OFValue vl = q( l );
    OFValue vu = q( u );
    f_max_val += std::max( vl , vu );
    if( f_sol & 1 )
     var.set_value( vl > vu ? l : u );
    }

   // the opposite problem
   if( var.is_integer() ) {         // on an integer variable
    OFValue xm = std::max( l , std::floor( x ) );
    OFValue xp = std::min( u , std::ceil( x ) );
    f_min_val += std::min( q( xm ) , q( xp ) );
    }
   else {                           // on a continuous variable
    x = std::min( u , std::max( l , x ) );
    f_min_val += q( x );
    }
   }

  return;  // the quadratic maximization case has been dealt with
  }

 // deal with the quadratic minimization case
 if( a > 0 ) {                     // with a > 0
  OFValue vxmin;

  // the original problem
  if( var.is_integer() ) {         // on an integer variable
   OFValue xm = std::max( l , std::floor( x ) );
   OFValue vxm = q( xm );
   OFValue xp = std::min( u , std::ceil( x ) );
   OFValue vxp = q( xp );
   if( vxm < vxp ) { x = xm; vxmin = vxm; }
   else            { x = xp; vxmin = vxp; }
   // note: no dual solution since it's integer
   }
  else {                           // on a continuous variable
   if( x > u ) {
    x = u;
    if( ( f_sol & 2 ) && cu )
     cu->set_dual( - 2 * a * x - b );  // dual solution
    }
   else
    if( x < l ) {
     x = l;
     if( ( f_sol & 2 ) && cl )
      cl->set_dual( - 2 * a * x - b );  // dual solution
     }

   vxmin = q( x );
   }

  if( f_sol & 1 )
   var.set_value( x );
  f_min_val += vxmin;

  // the opposite problem
  if( ( l == - INF ) || ( u == INF ) )
   f_max_val = INF;
  else
   f_max_val += std::max( q( l ) , q( u ) );
  }
 else {                            // [minimization] with a < 0
  // the original problem
  // note: no dual solution since it's concave minimization
  if( ( l == - INF ) || ( u == INF ) )
   f_min_val = - INF;
  else {
   OFValue vl = q( l );
   OFValue vu = q( u );
   f_min_val += std::min( vl , vu );
   if( f_sol & 1 )
    var.set_value( vl < vu ? l : u );
   }

  // the opposite problem
  if( var.is_integer() ) {         // on an integer variable
   OFValue xm = std::max( l , std::floor( x ) );
   OFValue xp = std::min( u , std::ceil( x ) );
   f_max_val += std::max( q( xm ) , q( xp ) );
   }
  else {                           // on a continuous variable
   x = std::min( u , std::max( l , x ) );
   f_max_val += q( x );
   }
  }
 }  // end( BoxSolver::process_variable )

/*--------------------------------------------------------------------------*/
/*----------------------- End File BoxSolver.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
