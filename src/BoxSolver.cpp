/*--------------------------------------------------------------------------*/
/*--------------------------- File BoxSolver.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the BoxSolver class, which implements a CDASolver for
 * problems (or relaxations of problems) with an extremely simple structure:
 * only bound (box) Constraint on the ColVariable and a separable Objective
 * (a FRealObjective with either a LinearFunction or a DQuadFunction inside).
 *
 * \version 0.10
 *
 * \date 05 - 01 - 2021
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

 if( f_verse < 0 )  // have to compute/update the verse
  check_verse( f_Block );

 if( f_max_val >= f_min_val )  // done already
  return( kOK );

 f_max_val = f_min_val = 0;

 // process static variables
 for( const auto & el : f_Block->get_static_variables() ) {
  // Singles
  if( un_any_thing_0( ColVariable , el , process_variable( var ) ) ) {
   if( ( f_max_val == Inf< OFValue >() ) &&
       ( f_min_val == - Inf< OFValue >() ) )
    break;
   else
    continue;
   }
  // Vectors
  if( un_any_thing_1( ColVariable , el ,
		      {
		       for( auto & v : var ) {
			process_variable( v );
			if( ( f_max_val == Inf< OFValue >() ) &&
			    ( f_min_val == - Inf< OFValue >() ) )
			 break;
		        }
		       } ) ) {
   if( ( f_max_val == Inf< OFValue >() ) &&
       ( f_min_val == - Inf< OFValue >() ) )
    break;
   else
    continue;
   }
  // Multiarrays
  if( un_any_thing_K( ColVariable , el ,
		      {
		       for( auto vit = var.data() ;
			    vit != var.data() + var.num_elements() ;
			    ++vit ) {
			process_variable( *vit );
			if( ( f_max_val == Inf< OFValue >() ) &&
			    ( f_min_val == - Inf< OFValue >() ) )
			 break;
		        }
		       } ) ) {
   if( ( f_max_val == Inf< OFValue >() ) &&
       ( f_min_val == - Inf< OFValue >() ) )
    break;
   else
    continue;
   }
  throw( std::invalid_argument(
		       "BoxSolver: static variable not a ColVariable" ) );
  }

 // process dynamic variables
 for( const auto & el : f_Block->get_dynamic_variables() ) {
  // Singles
  if( un_any_thing_0( std::list< ColVariable > , el ,
		      {
		       for( auto & v : var ) {
			process_variable( v );
			if( ( f_max_val == Inf< OFValue >() ) &&
			    ( f_min_val == - Inf< OFValue >() ) )
			 break;
		        }
		       } ) ) {
   if( ( f_max_val == Inf< OFValue >() ) &&
       ( f_min_val == - Inf< OFValue >() ) )
    break;
   else
    continue;
   }
  // Vectors
  if( un_any_thing_1( std::list< ColVariable > , el ,
		      {
		       for( auto & vl : var ) {
			for( auto & v : vl ) {
			 process_variable( v );
			 if( ( f_max_val == Inf< OFValue >() ) &&
			     ( f_min_val == - Inf< OFValue >() ) )
			  break;
			 }
			if( ( f_max_val == Inf< OFValue >() ) &&
			    ( f_min_val == - Inf< OFValue >() ) )
			 break;
		        }
		       } ) ) {
   if( ( f_max_val == Inf< OFValue >() ) &&
       ( f_min_val == - Inf< OFValue >() ) )
    break;
   else
    continue;
   }
  // Multiarrays
  if( un_any_thing_K( std::list< ColVariable > , el ,
		      {
		       for( auto vit = var.data() ;
			    vit != var.data() + var.num_elements() ;
			    ++vit ) {
			for( auto & v : *vit ) {
			 process_variable( v );
			 if( ( f_max_val == Inf< OFValue >() ) &&
			     ( f_min_val == - Inf< OFValue >() ) )
			  break;
			 }
			if( ( f_max_val == Inf< OFValue >() ) &&
			    ( f_min_val == - Inf< OFValue >() ) )
			 break;
		        }
		       } ) ) {
   if( ( f_max_val == Inf< OFValue >() ) &&
       ( f_min_val == - Inf< OFValue >() ) )
    break;
   else
    continue;
   }
  throw( std::invalid_argument(
		       "BoxSolver: dynamic variable not a ColVariable" ) );
  }

 
 }  // end( BoxSolver::compute )

/*--------------------------------------------------------------------------*/
/*------------- METHODS FOR ADDING / REMOVING / CHANGING DATA --------------*/
/*--------------------------------------------------------------------------*/

void BoxSolver::add_Modification( sp_Mod & mod )
{
 if( f_no_Mod || ( ! f_Block ) )
  return;

 // changing the verse
 if( std::dynamic_pointer_cast< const ObjectiveMod >( mod ) ) {
  f_verse = -1;  // check it has to be set
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
  if( dynamic_cast< Objective * >( tmod->function->get_observer() ) )
   reset();

  // else ignore it, the Function is in a Constraint that is ignored
  return;
  }

 // changes in the active Variables of a Function
 if( auto tmod = std::dynamic_pointer_cast< const FunctionModVars >( mod )
     ) {
  // the Function is inside an Objective
  if( dynamic_cast< Objective * >( tmod->function->get_observer() ) )
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

void BoxSolver::check_verse( Block * blck )
{
 if( auto obj = blck->get_objective() ) {
  auto verse = obj->get_sense();
  if( f_verse < 0 )
   f_verse = verse;
  else
   if( f_verse != verse )
    throw( std::invalid_argument(
		     "BoxSolver: Block with non-uniform Objective sense" ) );
  }

 for( b : v_Block )
  check_verse( b );
 }
 
/*--------------------------------------------------------------------------*/

void BoxSolver::process_variable( ColVariable & var )
{
 using VarValue = ColVariable::VarValue
 
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

 // scan through all active stuff
 for( Index i = 0 ; i < var.get_num_active() ; ++i ) {
  auto ai = var.get_active( i );

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
    }
   // any other Constraint is ignored
   continue;
   }

  // Objective
  if( auto oi = dynamic_cast< Objective * >( ai ) ) {
   auto fro = dynamic_cast< FRealObjective * >( oi );
   if( ! fro )
    throw( std::invalid_argument( "BoxSolver:: not a FRealObjective" ) );

   if( auto lf = dynamic_cast< LinearFunction * >( fro->get_function() ) ) {
    // WARNING: INEFFICIENT!!
    b += lf->get_coefficient( lf->is_active( & var ) );
    continue;
    }

   if( auto qf = dynamic_cast< DQuadFunction * >( fro->get_function() ) ) {
    // WARNING: INEFFICIENT!!
    auto p = qf->is_active( & var );
    b += qf->get_linear_coefficient( p );
    a += qf->get_quadratic_coefficient( p );
    continue;
    }
   
   throw( std::invalid_argument(
	       "BoxSolver:: invalid Function inside the FRealObjective" ) );
   }
  }  // end( for( i ) ) 

 // now finally perform the minimization / maximization
 // the problem is
 //
 //    min / max { a x^2 + b x : l <= x <= u }
 //
 //

 }  // end( BoxSolver::process_variable )




/*--------------------------------------------------------------------------*/
/*----------------------- End File BoxSolver.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
