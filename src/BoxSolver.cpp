/*--------------------------------------------------------------------------*/
/*--------------------------- File BoxSolver.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the BoxSolver class, which implements a CDASolver for
 * problems (or relaxations thereof) with an extremely simple structure:
 * only bound (box) Constraint on the ColVariable and a separable Objective
 * (a FRealObjective with either a LinearFunction or a DQuadFunction inside).
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

#include "Block.h"

#include "BoxSolver.h"

#include "FRealObjective.h"

#include "FRowConstraint.h"

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
/*--------------------------- METHODS of BoxSolver -------------------------*/
/*--------------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/

void BoxSolver::set_Block( Block * block )
{
 CDASolver::set_Block( block );
 f_sense = -1;
 f_desc.clear();
 resetf();

 if( ! f_Block )
  return;

 // compute the set of all involved (sub-)Block
 f_desc.push_back( f_Block );
 for( Block::Index i = 0 ; i < f_desc.size() ; ++i ) {
  if( auto obj = f_desc[ i ]->get_objective() )
   if( ! dynamic_cast< FRealObjective * >( obj ) )
    throw( std::invalid_argument(
			  "BoxSolver::set_Block: unsupported Objective" ) );
  for( auto el : f_desc[ i ]->get_nested_Blocks() )
   f_desc.push_back( el );
  }

 f_desc.shrink_to_fit();
 }

/*--------------------------------------------------------------------------*/
/*--------------------- METHODS FOR SOLVING THE MODEL ----------------------*/
/*--------------------------------------------------------------------------*/

int BoxSolver::compute( bool changedvars )
{
 lock();  // lock the mutex: this is done again inside MILPSolver::compute,

 if( ! f_Block ) {    // no Block
  f_state = kError;   // that's an error
  goto Return_state;  // all done
  }

 if( f_sense < 0 )     // have to compute/update the sense
  check_sense( f_Block );

 if( f_sense < 0 )     // the sense is (still) undefined
  throw( std::invalid_argument(
	                  "BoxSolver::compute: undefined Objective sense" ) );

 if( f_state != kUnEval )  // all compute()-d already
  goto Return_state;       // all done

 f_max_val = f_min_val = 0;

 if( f_altobj ) {  // do it for an alternative Objective - - - - - - - - - - -
  if( ! f_feas ) {
   // first phase: if feasibility is not guaranteed already, look at all
   // [Col]Variable, check they are feasible and if required give them any
   // feasible value

   auto f = std::bind( & BoxSolver::process_variable_bnds , this ,
		       std::placeholders::_1 );

   for( auto bk : f_desc ) {
    // process static variables
    for( const auto & el : bk->get_static_variables() ) {
     if( un_any_const_static( el , f , un_any_type< ColVariable >() ) ) {
      if( f_state != kUnEval )  // infeasible
       goto endgame;
      continue;
      }
     throw( std::invalid_argument(
		       "BoxSolver: static variable not a ColVariable" ) );
     }

    // process dynamic variables
    for( const auto & el : bk->get_dynamic_variables() ) {
     if( un_any_const_dynamic( el , f , un_any_type< ColVariable >() ) ) {
      if( f_state != kUnEval )  // infeasible
       goto endgame;
      continue;
      }
     throw( std::invalid_argument(
		       "BoxSolver: dynamic variable not a ColVariable" ) );
     }
    }  // end( for( all involved Block ) )
   }  // end( ! f_feas )

  // second phase: look at only the ColVariable active in the alternative
  // Objective Function and do the actual optimization

  VarValue l , u;
  if( auto lf = dynamic_cast< LinearFunction * >( f_altobj ) ) {
   f_max_val = f_min_val = lf->get_constant_term();
   
   if( f_sol & 2 ) {
    for( auto el : lf->get_v_var() ) {
     if( el.second == 0 )
      continue;

     OneVarConstraint * cl = nullptr;
     OneVarConstraint * cu = nullptr;
     get_var_data( *(el.first) , l , u , cl , cu );
     sol_variable( *(el.first) , l , u , el.second , cl , cu );
     }
    }
   else {
    for( auto el : lf->get_v_var() ) {
     if( el.second == 0 )
      continue;

     get_var_data( *(el.first) , l , u );
     sol_variable( *(el.first) , l , u , el.second );
     }
    }
   }
  else
   if( auto qf = dynamic_cast< DQuadFunction * >( f_altobj ) ) {
    f_max_val = f_min_val = qf->get_constant_term();

    if( f_sol & 2 ) {
     for( auto el : qf->get_v_var() ) {
      if( ( std::get< 1 >( el ) == 0 ) && ( std::get< 2 >( el ) == 0 ) )
       continue;

      OneVarConstraint * cl = nullptr;
      OneVarConstraint * cu = nullptr;
      get_var_data( *std::get< 0 >( el ) , l , u , cl , cu );

      if( std::get< 2 >( el ) == 0 )
       sol_variable( *std::get< 0 >( el ) , l , u ,
		     std::get< 1 >( el ) , cl , cu );
      else
       sol_variable( *std::get< 0 >( el ) , l , u ,
		     std::get< 2 >( el ) , std::get< 1 >( el ) , cl , cu );
       }
      }
     else {
      for( auto el : qf->get_v_var() ) {
       if( ( std::get< 1 >( el ) == 0 ) && ( std::get< 2 >( el ) == 0 ) )
	continue;

       get_var_data( *std::get< 0 >( el ) , l , u );

      if( std::get< 2 >( el ) == 0 )
       sol_variable( *std::get< 0 >( el ) , l , u , std::get< 1 >( el ) );
      else
       sol_variable( *std::get< 0 >( el ) , l , u ,
		     std::get< 2 >( el ) , std::get< 1 >( el ) );
      }
     }
    }
   else
    throw( std::invalid_argument(
	   "BoxSolver: alternative Objective Function type not supported" ) );
  }
 else {            // do it for the standard Objective - - - - - - - - - - - -
  auto f = std::bind( & BoxSolver::process_variable , this ,
		      std::placeholders::_1 );

  for( auto bk : f_desc ) {
   // deal with the constant in the Objective
   if( auto obj = static_cast< RealObjective * >( bk->get_objective() ) ) {
    double ct = obj->get_constant_term();
    f_max_val += ct;
    f_min_val += ct;
    }

   // process static variables
   for( const auto & el : bk->get_static_variables() ) {
    if( un_any_const_static( el , f , un_any_type< ColVariable >() ) ) {
     if( f_state != kUnEval )  // infeasible
      goto endgame;
     continue;
     }
    throw( std::invalid_argument(
		       "BoxSolver: static variable not a ColVariable" ) );
    }

   // process dynamic variables
   for( const auto & el : bk->get_dynamic_variables() ) {
    if( un_any_const_dynamic( el , f , un_any_type< ColVariable >() ) ) {
     if( f_state != kUnEval )  // infeasible
      goto endgame;
     continue;
     }
    throw( std::invalid_argument(
		       "BoxSolver: dynamic variable not a ColVariable" ) );
    }
   }  // end( for( all involved Block ) )
  }  // end( else( do it for the standard Objective )- - - - - - - - - - - - -

 endgame:
 // now see what value must be returned
 if( f_state != kInfeasible ) {
  f_feas = true;
  if( f_sense )  // maximization
   if( f_max_val == INF )
    f_state = kUnbounded;
   else
    f_state = kOK;
  else           // minimization
   if( f_min_val == -INF )
    f_state = kUnbounded;
   else
    f_state = kOK;

  // depending on the value of f_sol, the primal and/or reduced cost part
  // of the solution is already computed
  f_sol_comp = f_sol & 3;
  }
 else {
  f_feas = false;
  f_sol_comp = 0;  // no solution available
  }

 Return_state:
 unlock();  // unlock the mutex
 return( f_state );

 }  // end( BoxSolver::compute )

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/

void BoxSolver::get_var_solution( Configuration *solc )
{
 if( ! has_var_solution() )
  throw( std::logic_error( "BoxSolver: Variable solution not available" ) );

 if( f_sol_comp & 1 )  // the solution is there already
  return;

 if( f_altobj ) {  // do it for an alternative Objective - - - - - - - - - - -
  if( ! f_feas ) {
   // first phase: if a feasible solution is not already there, look at all
   // [Col]Variable and give them any feasible value
   auto f = std::bind( & BoxSolver::process_variable_bnds , this ,
		       std::placeholders::_1 );

   for( auto bk : f_desc ) {
    // process static variables
    for( const auto & el : bk->get_static_variables() )
     un_any_const_static( el , f , un_any_type< ColVariable >() );

    // process dynamic variables
    for( const auto & el : bk->get_dynamic_variables() )
     un_any_const_dynamic( el , f , un_any_type< ColVariable >() );

    }  // end( for( all involved Block ) )

   f_feas = true;  // now this is done
   }

  // second phase: look at only the ColVariable active in the alternative
  // Objective Function and do the actual optimization

  if( auto lf = dynamic_cast< LinearFunction * >( f_altobj ) ) {
   for( auto el : lf->get_v_var() ) {
    if( el.second == 0 )
     continue;
    
    if( ! is_mine( *(el.first) ) )
     continue;
 
    VarValue l , u;
    get_var_data( *(el.first) , l , u );
    process_var_sol( *(el.first) , l , u , el.second );
    }
   }
  else
   if( auto qf = dynamic_cast< DQuadFunction * >( f_altobj ) ) {
    for( auto el : qf->get_v_var() ) {
     if( ( std::get< 1 >( el ) == 0 ) && ( std::get< 2 >( el ) == 0 ) )
      continue;

     if( ! is_mine( *std::get< 0 >( el ) ) )
      continue;

     VarValue l , u;
     get_var_data( *std::get< 0 >( el ) , l , u );

     if( std::get< 2 >( el ) == 0 )
      process_var_sol( *std::get< 0 >( el ) , l , u , std::get< 1 >( el ) );
     else
      process_var_sol( *std::get< 0 >( el ) , l , u ,
		       std::get< 2 >( el ) , std::get< 1 >( el ) );
     }
    }
   else
    throw( std::invalid_argument(
		"BoxSolver: alternative Objective Function not supported" ) );
  }
 else {            // do it for the standard Objective - - - - - - - - - - - -
  auto f = std::bind( & BoxSolver::process_variable_sol , this ,
		      std::placeholders::_1 );

  for( auto bk : f_desc ) {
   // process static variables
   for( const auto & el : bk->get_static_variables() )
    un_any_const_static( el , f , un_any_type< ColVariable >() );

   // process dynamic variables
   for( const auto & el : bk->get_dynamic_variables() )
    un_any_const_dynamic( el , f , un_any_type< ColVariable >() );

   }  // end( for( all involved Block ) )

  f_feas = true;  // a feasible solution is there
  
  }  // end( else( do it for the standard Objective )- - - - - - - - - - - - -

 f_sol_comp |= 1;  // the primal solution is now there

 }  // end( BoxSolver::get_var_solution )

/*--------------------------------------------------------------------------*/

void BoxSolver::get_dual_solution( Configuration *solc )
{
 if( ! has_dual_solution() )
  throw( std::logic_error( "BoxSolver: dual solution not available" ) );

 // produce reduced costs - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( ( f_sol & 2 ) && ( ! ( f_sol_comp & 2 ) ) ) {
  const auto f = []( OneVarConstraint & c ) { c.set_dual( 0 ); };

  if( f_altobj ) {  // do it for an alternative Objective - - - - - - - - - -
   // first phase: zero-out all reduced costs

   for( auto bk : f_desc ) {
    // process static constraints
    for( const auto & el : bk->get_static_constraints() ) {
     if( un_any_const_static( el , f , un_any_type< BoxConstraint >() ) )
      continue;
     if( un_any_const_static( el , f , un_any_type< LB0Constraint >() ) )
      continue;
     if( un_any_const_static( el , f , un_any_type< UB0Constraint >() ) )
      continue;
     if( un_any_const_static( el , f , un_any_type< LBConstraint >() ) )
      continue;
     if( un_any_const_static( el , f , un_any_type< UBConstraint >() ) )
      continue;
     if( un_any_const_static( el , f , un_any_type< NNConstraint >() ) )
      continue;
     if( un_any_const_static( el , f , un_any_type< NPConstraint >() ) )
      continue;
     un_any_const_static( el , f , un_any_type< ZOConstraint >() );
     }

    // process dynamic constraints
    for( const auto & el : bk->get_dynamic_constraints() ) {
     if( un_any_const_dynamic( el , f , un_any_type< BoxConstraint >() ) )
      continue;
     if( un_any_const_dynamic( el , f , un_any_type< LB0Constraint >() ) )
      continue;
     if( un_any_const_dynamic( el , f , un_any_type< UB0Constraint >() ) )
      continue;
     if( un_any_const_dynamic( el , f , un_any_type< LBConstraint >() ) )
      continue;
     if( un_any_const_dynamic( el , f , un_any_type< UBConstraint >() ) )
      continue;
     if( un_any_const_dynamic( el , f , un_any_type< NNConstraint >() ) )
      continue;
     if( un_any_const_dynamic( el , f , un_any_type< NPConstraint >() ) )
      continue;
     un_any_const_dynamic( el , f , un_any_type< ZOConstraint >() );
     }
    }  // end( for( all involved Block ) )

   // second phase: look at only the ColVariable active in the alternative
   // Objective Function and do the actual reduced cost computation
   VarValue l , u;
   if( auto lf = dynamic_cast< LinearFunction * >( f_altobj ) ) {
    for( auto el : lf->get_v_var() ) {
     if( el.second == 0 )
      continue;

     if( ! is_mine( *(el.first) ) )
      continue;
 
     OneVarConstraint * cl = nullptr;
     OneVarConstraint * cu = nullptr;
     get_var_data( *(el.first) , l , u , cl , cu );
     process_var_dual( *(el.first) , l , u , el.second , cl , cu );
     }
    }
   else
    if( auto qf = dynamic_cast< DQuadFunction * >( f_altobj ) ) {
     for( auto el : qf->get_v_var() ) {
      if( ( std::get< 1 >( el ) == 0 ) && ( std::get< 2 >( el ) == 0 ) )
       continue;

      if( ! is_mine( *std::get< 0 >( el ) ) )
       continue;

      OneVarConstraint * cl = nullptr;
      OneVarConstraint * cu = nullptr;
      get_var_data( *std::get< 0 >( el ) , l , u , cl , cu );

      if( std::get< 2 >( el ) == 0 )
       process_var_dual( *std::get< 0 >( el ) , l , u ,
			 std::get< 1 >( el ) , cl , cu );
      else
       process_var_dual( *std::get< 0 >( el ) , l , u ,
			 std::get< 2 >( el ) , std::get< 1 >( el ) ,
			 cl , cu );
      }
     }
    else
     throw( std::invalid_argument(
		"BoxSolver: alternative Objective Functionnot supported" ) );
   }
  else {            // do it for the standard Objective- - - - - - - - - - - -
   auto f = std::bind( & BoxSolver::process_variable_dual , this ,
		       std::placeholders::_1 );

   for( auto bk : f_desc ) {
    // process static variables
    for( const auto & el : bk->get_static_variables() )
     un_any_const_static( el , f , un_any_type< ColVariable >() );

    // process dynamic variables
    for( const auto & el : bk->get_dynamic_variables() )
     un_any_const_dynamic( el , f , un_any_type< ColVariable >() );

    }  // end( for( all involved Block ) )
   }  // end( else( do it for the standard Objective )- - - - - - - - - - - -

  f_sol_comp |= 2;
  }

 // produce other dual values - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( ( f_sol & 4 ) && ( ! ( f_sol_comp & 4 ) ) ) {
  const auto f = []( FRowConstraint & c ) { c.set_dual( 0 ); };

  for( auto bk : f_desc ) {
  // process static constraints
   for( const auto & el : bk->get_static_constraints() )
    un_any_const_static( el , f , un_any_type< FRowConstraint >() );

   // process dynamic constraints
   for( const auto & el : bk->get_dynamic_constraints() )
    un_any_const_dynamic( el , f , un_any_type< FRowConstraint >() );

   }  // end( for( all involved Block ) )

  f_sol_comp |= 4;
  } 
 }  // end( BoxSolver::get_dual_solution )

/*--------------------------------------------------------------------------*/

void BoxSolver::get_var_direction( Configuration * dirc )
{
 // note: unlike get_var_solution(), there is no mechanism so that the second
 //       consecutive call to get_var_direction() does nothing because the
 //       direction is already there. this looks rather unlikely to happen
 //       anyway

 if( ! has_var_direction() )
  throw( std::logic_error( "BoxSolver: Variable direction not available" ) );

 if( f_altobj ) {  // do it for an alternative Objective - - - - - - - - - - -
  // first phase: look at all [Col]Variable and give them value 0
  const auto f = []( ColVariable & var ) { var.set_value( 0 ); };

  for( auto bk : f_desc ) {
   // process static variables
   for( const auto & el : bk->get_static_variables() )
    un_any_const_static( el , f , un_any_type< ColVariable >() );

   // process dynamic variables
   for( const auto & el : bk->get_dynamic_variables() )
    un_any_const_dynamic( el , f , un_any_type< ColVariable >() );

   }  // end( for( all involved Block ) )

  // second phase: look at only the ColVariable active in the alternative
  // Objective Function and do the actual optimization

  if( auto lf = dynamic_cast< LinearFunction * >( f_altobj ) ) {
   for( auto el : lf->get_v_var() ) {
    if( el.second == 0 )
     continue;
    
    if( ! is_mine( *(el.first) ) )
     continue;

    VarValue l , u;
    get_var_data( *(el.first) , l , u );
    process_var_dir_l( *(el.first) , l , u , el.second );
    }
   }
  else
   if( auto qf = dynamic_cast< DQuadFunction * >( f_altobj ) ) {
    for( auto el : qf->get_v_var() ) {
     if( ( std::get< 1 >( el ) == 0 ) && ( std::get< 2 >( el ) == 0 ) )
      continue;

     if( ! is_mine( *std::get< 0 >( el ) ) )
      continue;

     VarValue l , u;
     get_var_data( *std::get< 0 >( el ) , l , u );

     if( std::get< 2 >( el ) == 0 )
      process_var_dir_l( *std::get< 0 >( el ) , l , u ,
			 std::get< 1 >( el ) );
     else
      process_var_dir_q( *std::get< 0 >( el ) , l , u ,
			 std::get< 2 >( el ) );
     }
    }
   else
    throw( std::invalid_argument(
		"BoxSolver: alternative Objective Function not supported" ) );
  }
 else {            // do it for the standard Objective - - - - - - - - - - - -
  auto f = std::bind( & BoxSolver::process_variable_dir , this ,
		      std::placeholders::_1 );

  for( auto bk : f_desc ) {
   // process static variables
   for( const auto & el : bk->get_static_variables() )
    un_any_const_static( el , f , un_any_type< ColVariable >() );

   // process dynamic variables
   for( const auto & el : bk->get_dynamic_variables() )
    un_any_const_dynamic( el , f , un_any_type< ColVariable >() );

   }  // end( for( all involved Block ) )
  }  // end( else( do it for the standard Objective )- - - - - - - - - - - - -

 f_sol_comp &= ~1;  // the primal solution is no longer there
 f_feas = false;    // no feasible solution available

 }  // end( BoxSolver::get_var_direction )

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
  resetf();  // this may change feasibility
  return;
  }

 // changes in a Constraint
 if( auto tmod = std::dynamic_pointer_cast< const ConstraintMod >( mod ) ) {
  // the Constraint is a box one
  if( dynamic_cast< OneVarConstraint * >( tmod->constraint() ) )
   resetf();  // this may change feasibility

  // else ignore it, since non-box Constraint themselves are ignored
  return;
  }

 // changes in a Function
 if( auto tmod = std::dynamic_pointer_cast< const FunctionMod >( mod ) ) {
  // the Function is inside an Objective
  if( dynamic_cast< Objective * >( tmod->function()->get_Observer() ) )
   reset();  // but it will not change feasibility

  // else ignore it, the Function is in a Constraint that is ignored
  return;
  }

 // changes in the active Variables of a Function
 if( auto tmod = std::dynamic_pointer_cast< const FunctionModVars >( mod )
     ) {
  // the Function is inside an Objective
  if( dynamic_cast< Objective * >( tmod->function()->get_Observer() ) )
   reset();  // but it will not change feasibility

  // else ignore it, the Function is in a Constraint that is ignored
  return;
  }

 // changes in the Block (changing the Objective)
 if( std::dynamic_pointer_cast< const BlockMod >( mod ) ) {
  reset();  // but it will not change feasibility
  return;
  }

 // additions/deletions in the Block; note that adding OneVarConstraint
 // may change feasibility so we err on the safe side, a few more if()s
 // would be enough to check it
 if( std::dynamic_pointer_cast< const BlockModAD >( mod ) )
  resetf();

 // if anything else remains, ignore it: it's not a change that impacts
 // on the parts of the Block that BoxSolver looks at

 }  // end( BoxSolver::add_Modification )

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

void BoxSolver::check_sense( Block * blck )
{
 auto sense = blck->get_objective_sense();
 if( sense != Objective::eUndef ) {
  sense = ( sense == Objective::eMax ? 1 : 0 );
  if( f_sense < 0 )
   f_sense = sense;
  else
   if( f_sense != sense )
    throw( std::invalid_argument(
		     "BoxSolver: Block with non-uniform Objective sense" ) );
  }

 for( auto b : blck->get_nested_Blocks() )
  check_sense( b );
 }

/*--------------------------------------------------------------------------*/

void BoxSolver::get_var_data( ColVariable & var ,
			      VarValue & l , VarValue & u )
{
 // initialize upper and lower bound
 if( var.is_fixed() )
  l = u = var.get_value();
 else {
  l = var.get_lb();
  u = var.get_ub();
  }

 // scan through all active stuff: compute in l and u the tightest lower
 // and upper bounds, and in cl, cu the pointer to the OneVarConstraint that
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

  // a OneVarConstraint
  if( auto ovr = dynamic_cast< OneVarConstraint * >( ai ) ) {
   if( auto lhs = ovr->get_lhs() ; lhs > l )
    l = lhs;
   if( auto rhs = ovr->get_rhs() ; rhs < u )
    u = rhs;

   if( f_sol & 2 )        // the dual solution is required
    ovr->set_dual( 0 );   // it is 0 unless otherwise proven
   }

  // anything else (different Constraint and Objective) is ignored

  }  // end( for( i ) )

 // if the variable is integer, appropriately shrink the interval by making
 // the extremes integer; note that if, say, l == u == a fractional number,
 // this immediately makes the interval empty, as it should be
 if( var.is_integer() ) {
  l = std::ceil( l );
  u = std::floor( u );
  }
 }  // end( BoxSolver::get_var_data )

/*--------------------------------------------------------------------------*/

void BoxSolver::get_var_data( ColVariable & var ,
			      VarValue & l , VarValue & u ,
			      OFValue & a , OFValue & b )
{
 // this is only called when f_sol & 2 == false
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
    if( auto lhs = ovr->get_lhs() ; lhs > l )
     l = lhs;
    if( auto rhs = ovr->get_rhs() ; rhs < u )
     u = rhs;
    }

   continue;              // any other Constraint is ignored
   }

  // Objective
  if( auto oi = dynamic_cast< Objective * >( ai ) ) {
   auto fro = static_cast< FRealObjective * >( oi );

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

 // if the variable is integer, appropriately shrink the interval by making
 // the extremes integer; note that if, say, l == u == a fractional number,
 // this immediately makes the interval empty, as it should be
 if( var.is_integer() ) {
  l = std::ceil( l );
  u = std::floor( u );
  }
 }  // end( BoxSolver::get_var_data( a, b ) )

/*--------------------------------------------------------------------------*/

void BoxSolver::get_var_data( ColVariable & var ,
			      VarValue & l , VarValue & u ,
			      OneVarConstraint * & cl ,
			      OneVarConstraint * & cu )
{
 // this is only called when f_sol & 2 == true
 // initialize upper and lower bound
 if( var.is_fixed() )
  l = u = var.get_value();
 else {
  l = var.get_lb();
  u = var.get_ub();
  }

 // scan through all active stuff: compute in l and u the tightest lower
 // and upper bounds, and in cl, cu the pointer to the OneVarConstraint that
 // correspond to the "tight" l / u (if any)
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

  // a OneVarConstraint
  if( auto ovr = dynamic_cast< OneVarConstraint * >( ai ) ) {
   auto lhs = ovr->get_lhs();
   if( lhs > l ) {
    l = lhs;
    cl = ovr;
    }
   auto rhs = ovr->get_rhs();
   if( rhs < u ) {
    u = rhs;
    cu = ovr;
    }

   ovr->set_dual( 0 );  // it is 0 unless otherwise proven
   }

  // anything else (different Constraint and Objective) is ignored
  
  }  // end( for( i ) )

 // if the variable is integer, appropriately shrink the interval by making
 // the extremes integer; note that if, say, l == u == a fractional number,
 // this immediately makes the interval empty, as it should be
 if( var.is_integer() ) {
  l = std::ceil( l );
  u = std::floor( u );
  }
 }  // end( BoxSolver::get_var_data( cl , cu ) )

/*--------------------------------------------------------------------------*/

void BoxSolver::get_var_data( ColVariable & var ,
			      VarValue & l , VarValue & u ,
			      OFValue & a , OFValue & b ,
			      OneVarConstraint * & cl ,
			      OneVarConstraint * & cu )
{
 // this is only called when f_sol & 2 == true
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
    if( lhs > l ) {
     l = lhs;
     cl = ovr;
     }
    auto rhs = ovr->get_rhs();
    if( rhs < u ) {
     u = rhs;
     cu = ovr;
     }

    ovr->set_dual( 0 );   // it is 0 unless otherwise proven
    }

   continue;              // any other Constraint is ignored
   }

  // Objective
  if( auto oi = dynamic_cast< Objective * >( ai ) ) {
   auto fro = static_cast< FRealObjective * >( oi );

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

 // if the variable is integer, appropriately shrink the interval by making
 // the extremes integer; note that if, say, l == u == a fractional number,
 // this immediately makes the interval empty, as it should be
 if( var.is_integer() ) {
  l = std::ceil( l );
  u = std::floor( u );
  }
 }  // end( BoxSolver::get_var_data( a, b , cl , cu ) )

/*--------------------------------------------------------------------------*/

void BoxSolver::sol_variable( ColVariable & var , VarValue l ,
			      VarValue u ,  OFValue b )
{
 // this is only called when f_sol & 2 == false
 // the problem is
 //
 //    min / max { b x : l <= x <= u }
 //
 // even if the problem is unbounded, a feasible solution is provided

 if( f_sense == 1 ) {               // maximization
  if( b > 0 ) {                     // with b > 0
   // the original problem
   if( u == INF ) {                 // if the upper bound is +INF
    f_max_val = INF;                // max is unbounded above
    if( f_sol & 1 )
     var.set_value( std::max( l , double( 0 ) ) );
    }
   else {                           // if the upper bound is finite
    if( f_max_val < INF )           // problem not unbounded already
     f_max_val += b * u;            // add the contribution
    if( f_sol & 1 )
     var.set_value( u );            // primal solution
    }
   // the opposite problem
   if( l == -INF )                  // if the lower bound is -INF
    f_min_val = -INF;               // min is unbounded below
   else                             // if the lower bound is finite
    if( f_min_val > -INF )          // problem not unbounded already
     f_min_val += b * l;            // add the contribution
   }
  else {                            // [maximization] with b < 0
   // the original problem
   if( l == -INF ) {                // if the lower bound is -INF
    f_max_val = INF;                // max is unbounded above
    if( f_sol & 1 )
     var.set_value( std::min( u , double( 0 ) ) );
    }
   else {                           // if the lower bound is finite
    if( f_max_val < INF )           // problem not unbounded already
     f_max_val += b * l;            // add the contribution
    if( f_sol & 1 )
     var.set_value( l );            // primal solution
    }
   // the opposite problem
   if( u == INF )                   // if the upper bound is INF
    f_min_val = -INF;               // min is unbounded below
   else                             // if the upper bound is finite
    if( f_min_val > -INF )          // problem is unbounded already
     f_min_val += b * u;            // add the contribution
   }
  }
 else {                             // minimization
  if( b > 0 ) {                     // with b > 0
   // the original problem
   if( l == -INF ) {                // if the lower bound is -INF
    f_min_val = -INF;               // min is unbounded below
    if( f_sol & 1 )
     var.set_value( std::min( u , double( 0 ) ) );
    }
   else {                           // if the lower bound is finite
    if( f_min_val > -INF )          // problem not unbounded already
     f_min_val += b * l;            // add the contribution
    if( f_sol & 1 )
     var.set_value( l );            // primal solution
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
   if( u == INF ) {                 // if the upper bound is INF
    f_min_val = -INF;               // min is unbounded below
    if( f_sol & 1 )
     var.set_value( std::max( l , double( 0 ) ) );
    }
   else {                           // if the upper bound is finite
    if( f_min_val > -INF )          // problem not unbounded already
     f_min_val += b * u;            // add the contribution
    if( f_sol & 1 )
     var.set_value( u );            // primal solution
    }
   // the opposite problem
   if( l == -INF )                  // if the lower bound is -INF
    f_max_val = INF;                // max is unbounded above
   else                             // if the lower bound is finite
    if( f_max_val < INF )           // problem is unbounded already
     f_max_val += b * l;            // add the contribution
   }
  }
 }  // end( BoxSolver::sol_variable( b ) )

/*--------------------------------------------------------------------------*/

void BoxSolver::sol_variable( ColVariable & var , VarValue l ,
			      VarValue u , OFValue a , OFValue b )
{
 // this is only called when f_sol & 2 == false and a != 0
 // the problem is
 //
 //    min / max { a x^2 + b x : l <= x <= u }
 //
 // even if the problem is unbounded, a feasible solution is provided
 //
 // the unique stationary point (max or min depending on the sign of a)
 // of a x^2 + b is at x = - b / ( 2 * a );
 auto q = [ & ]( OFValue y ) -> OFValue { return( ( a * y + b ) * y ); };
 OFValue x = - b / ( 2 * a );
 
 if( f_sense == 1 ) {               // maximization
  if( a < 0 ) {                     // with a < 0
   OFValue vxmax;

   // the original problem
   if( var.is_integer() ) {         // on an integer variable
    x = std::max( l , std::min( u , x ) );
    OFValue xm = std::floor( x );
    OFValue vxm = q( xm );
    OFValue xp = std::ceil( x );
    OFValue vxp = q( xp );
    if( vxm > vxp ) { x = xm; vxmax = vxm; }
    else            { x = xp; vxmax = vxp; }
    }
   else {                           // on a continuous variable
    if( x > u )
     x = u;
    else
     if( x < l )
      x = l;

    vxmax = q( x );
    }

   if( f_sol & 1 )
    var.set_value( x );
   f_max_val += vxmax;

   // the opposite problem
   if( ( l == -INF ) || ( u == INF ) )
    f_min_val = -INF;
   else
    f_min_val += std::min( q( l ) , q( u ) );
   }
  else {                            // [maximization] with a > 0
   // the original problem
   if( ( l == -INF ) || ( u == INF ) ) {
    f_max_val = INF;
    if( f_sol & 1 )
     var.set_value( std::max( l , std::min( u , double( 0 ) ) ) );
    }
   else {
    OFValue vl = q( l );
    OFValue vu = q( u );
    f_max_val += std::max( vl , vu );
    if( f_sol & 1 )
     var.set_value( vl > vu ? l : u );
    }

   // the opposite problem
   x = std::min( u , std::max( l , x ) );

   if( var.is_integer() )           // on an integer variable
    f_min_val += std::min( q( std::floor( x ) ) , q( std::ceil( x ) ) );
   else                             // on a continuous variable
    f_min_val += q( x );
   }

  return;  // the quadratic maximization case has been dealt with
  }

 // deal with the quadratic minimization case
 if( a > 0 ) {                     // with a > 0
  OFValue vxmin;

  // the original problem
  if( var.is_integer() ) {         // on an integer variable
   x = std::max( l , std::min( u , x ) );
   OFValue xm = std::floor( x );
   OFValue vxm = q( xm );
   OFValue xp = std::ceil( x );
   OFValue vxp = q( xp );
   if( vxm < vxp ) { x = xm; vxmin = vxm; }
   else            { x = xp; vxmin = vxp; }
   }
  else {                           // on a continuous variable
   if( x > u )
    x = u;
   else
    if( x < l )
     x = l;

   vxmin = q( x );
   }

  if( f_sol & 1 )
   var.set_value( x );
  f_min_val += vxmin;

  // the opposite problem
  if( ( l == -INF ) || ( u == INF ) )
   f_max_val = INF;
  else
   f_max_val += std::max( q( l ) , q( u ) );
  }
 else {                            // [minimization] with a < 0
  // the original problem
  if( ( l == -INF ) || ( u == INF ) ) {
   f_min_val = -INF;
   if( f_sol & 1 )
    var.set_value( std::max( l , std::min( u , double( 0 ) ) ) );
   }
  else {
   OFValue vl = q( l );
   OFValue vu = q( u );
   f_min_val += std::min( vl , vu );
   if( f_sol & 1 )
    var.set_value( vl < vu ? l : u );
   }

  // the opposite problem
  x = std::min( u , std::max( l , x ) );

  if( var.is_integer() )           // on an integer variable
   f_max_val += std::max( q( std::floor( x ) ) , q( std::ceil( x ) ) );
  else                             // on a continuous variable
   f_max_val += q( x );
  }
 }  // end( BoxSolver::sol_variable( a , b ) )

/*--------------------------------------------------------------------------*/

void BoxSolver::sol_variable( ColVariable & var , VarValue l ,
			      VarValue u ,  OFValue b ,
			      OneVarConstraint * cl , OneVarConstraint * cu )
{
 // this is only called when f_sol & 2 == true
 // the problem is
 //
 //    min / max { b x : l <= x <= u }

 if( f_sense == 1 ) {               // maximization
  if( b > 0 ) {                     // with b > 0
   // the original problem
   if( u == INF ) {                 // if the upper bound is +INF
    f_max_val = INF;                // max is unbounded above
    if( f_sol & 1 )
     var.set_value( std::max( l , double( 0 ) ) );
    }
   else {                           // if the upper bound is finite
    if( f_max_val < INF )           // problem not unbounded already
     f_max_val += b * u;            // add the contribution
    if( f_sol & 1 )
     var.set_value( u );            // primal solution
    if( cu )
     cu->set_dual( b );             // dual solution
    }
   // the opposite problem
   if( l == -INF )                  // if the lower bound is -INF
    f_min_val = -INF;               // min is unbounded below
   else                             // if the lower bound is finite
    if( f_min_val > -INF )          // problem not unbounded already
     f_min_val += b * l;            // add the contribution
   }
  else {                            // [maximization] with b < 0
   // the original problem
   if( l == -INF ) {                // if the lower bound is -INF
    f_max_val = INF;                // max is unbounded above
    if( f_sol & 1 )
     var.set_value( std::min( u , double( 0 ) ) );
    }
   else {                           // if the lower bound is finite
    if( f_max_val < INF )           // problem not unbounded already
     f_max_val += b * l;            // add the contribution
    if( f_sol & 1 )
     var.set_value( l );            // primal solution
    if( cl )
     cl->set_dual( b );             // dual solution
    }
   // the opposite problem
   if( u == INF )                   // if the upper bound is INF
    f_min_val = -INF;               // min is unbounded below
   else                             // if the upper bound is finite
    if( f_min_val > -INF )          // problem is unbounded already
     f_min_val += b * u;            // add the contribution
   }
  }
 else {                             // minimization
  if( b > 0 ) {                     // with b > 0
   // the original problem
   if( l == -INF ) {                // if the lower bound is -INF
    f_min_val = -INF;               // min is unbounded below
    if( f_sol & 1 )
     var.set_value( std::min( u , double( 0 ) ) );
    }
   else {                           // if the lower bound is finite
    if( f_min_val > -INF )          // problem not unbounded already
     f_min_val += b * l;            // add the contribution
    if( f_sol & 1 )
     var.set_value( l );            // primal solution
    if( cu )
     cu->set_dual( b );             // dual solution
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
   if( u == INF ) {                 // if the upper bound is INF
    f_min_val = -INF;               // min is unbounded below
    if( f_sol & 1 )
     var.set_value( std::max( l , double( 0 ) ) );
    }
   else {                           // if the upper bound is finite
    if( f_min_val > -INF )          // problem not unbounded already
     f_min_val += b * u;            // add the contribution
    if( f_sol & 1 )
     var.set_value( u );            // primal solution
    if( cl )
     cl->set_dual( b );             // dual solution
    }
   // the opposite problem
   if( l == -INF )                  // if the lower bound is -INF
    f_max_val = INF;                // max is unbounded above
   else                             // if the lower bound is finite
    if( f_max_val < INF )           // problem is unbounded already
     f_max_val += b * l;            // add the contribution
   }
  }
 }  // end( BoxSolver::sol_variable( b , cl , cu ) )

/*--------------------------------------------------------------------------*/

void BoxSolver::sol_variable( ColVariable & var , VarValue l ,
			      VarValue u , OFValue a , OFValue b ,
			      OneVarConstraint * cl , OneVarConstraint * cu )
{
 // this is only called when f_sol & 2 == true and a != 0
 // the problem is
 //
 //    min / max { a x^2 + b x : l <= x <= u }
 //
 // note that the gradient of the objective is g( x ) = a x + b, while the
 // gradients of the constraints are +1 and -1. when a constraint is "tight"
 // (active and determining the optimal solution), its dual multiplier y >= 0
 // must therefore satisfy the KKT condition
 //
 //    y ( +/- 1 ) + ( a x + b ) = 0
 //
 // with the only delicate choice, as usual, being the sign

 // the unique stationary point (max or min depending on the sign of a)
 // of a x^2 + b is at x = - b / ( 2 * a );
 auto q = [ & ]( OFValue y ) -> OFValue { return( ( a * y + b ) * y ); };
 OFValue x = - b / ( 2 * a );
 
 if( f_sense == 1 ) {               // maximization
  if( a < 0 ) {                     // with a < 0
   OFValue vxmax;

   // the original problem
   if( var.is_integer() ) {         // on an integer variable
    x = std::max( l , std::min( u , x ) );
    OFValue xm = std::floor( x );
    OFValue vxm = q( xm );
    OFValue xp = std::ceil( x );
    OFValue vxp = q( xp );
    if( vxm > vxp ) { x = xm; vxmax = vxm; }
    else            { x = xp; vxmax = vxp; }
    // note: no dual solution since it's integer
    }
   else {                           // on a continuous variable
    if( x > u ) {
     x = u;
     if( cu )
      cu->set_dual( 2 * a * x + b );  // dual solution
     }
    else
     if( x < l ) {
      x = l;
      if( cl )
       cl->set_dual( 2 * a * x + b );  // dual solution
      }

    vxmax = q( x );
    }

   if( f_sol & 1 )
    var.set_value( x );
   f_max_val += vxmax;

   // the opposite problem
   if( ( l == -INF ) || ( u == INF ) )
    f_min_val = -INF;
   else
    f_min_val += std::min( q( l ) , q( u ) );
   }
  else {                            // [maximization] with a > 0
   // the original problem
   // note: no dual solution since it's convex maximization
   if( ( l == -INF ) || ( u == INF ) ) {
    f_max_val = INF;
    if( f_sol & 1 )
     var.set_value( std::max( l , std::min( u , double( 0 ) ) ) );
    }
   else {
    OFValue vl = q( l );
    OFValue vu = q( u );
    f_max_val += std::max( vl , vu );
    if( f_sol & 1 )
     var.set_value( vl > vu ? l : u );
    }

   // the opposite problem
   x = std::min( u , std::max( l , x ) );

   if( var.is_integer() )           // on an integer variable
    f_min_val += std::min( q( std::floor( x ) ) , q( std::ceil( x ) ) );
   else                             // on a continuous variable
    f_min_val += q( x );
   }

  return;  // the quadratic maximization case has been dealt with
  }

 // deal with the quadratic minimization case
 if( a > 0 ) {                     // with a > 0
  OFValue vxmin;

  // the original problem
  if( var.is_integer() ) {         // on an integer variable
   x = std::max( l , std::min( u , x ) );
   OFValue xm = std::floor( x );
   OFValue vxm = q( xm );
   OFValue xp = std::ceil( x );
   OFValue vxp = q( xp );
   if( vxm < vxp ) { x = xm; vxmin = vxm; }
   else            { x = xp; vxmin = vxp; }
   // note: no dual solution since it's integer
   }
  else {                           // on a continuous variable
   if( x > u ) {
    x = u;
    if( cu )
     cu->set_dual( - 2 * a * x - b );  // dual solution
    }
   else
    if( x < l ) {
     x = l;
     if( cl )
      cl->set_dual( - 2 * a * x - b );  // dual solution
     }

   vxmin = q( x );
   }

  if( f_sol & 1 )
   var.set_value( x );
  f_min_val += vxmin;

  // the opposite problem
  if( ( l == -INF ) || ( u == INF ) )
   f_max_val = INF;
  else
   f_max_val += std::max( q( l ) , q( u ) );
  }
 else {                            // [minimization] with a < 0
  // the original problem
  // note: no dual solution since it's concave minimization
  if( ( l == -INF ) || ( u == INF ) ) {
   f_min_val = -INF;
   if( f_sol & 1 )
    var.set_value( std::max( l , std::min( u , double( 0 ) ) ) );
   }
  else {
   OFValue vl = q( l );
   OFValue vu = q( u );
   f_min_val += std::min( vl , vu );
   if( f_sol & 1 )
    var.set_value( vl < vu ? l : u );
   }

  // the opposite problem
  x = std::min( u , std::max( l , x ) );

  if( var.is_integer() )           // on an integer variable
   f_max_val += std::max( q( std::floor( x ) ) , q( std::ceil( x ) ) );
  else                             // on a continuous variable
   f_max_val += q( x );
  }
 }  // end( BoxSolver::sol_variable( a , b , cl , cu ) )

/*--------------------------------------------------------------------------*/

void BoxSolver::process_variable_bnds( ColVariable & var )
{
 if( ! is_mine( var ) )
  return;

 VarValue l , u;  // lower and upper bound
 get_var_data( var , l , u );

 // now check unfeasibility: if l > u then the whole problem is unfeasible,
 // and it is so for both senses
 if( l > u ) {
  f_state = kInfeasible;
  return;
  }

 // now give the ColVariable a value, if required
 // any finite value is fine, take it "close to 0"
 // the dual solution, if required, is already set to 0, which is OK
 if( f_sol & 1 )
  var.set_value( std::min( u , std::max( l , VarValue( 0 ) ) ) );

 }  // end( BoxSolver::process_variable_bnds )

/*--------------------------------------------------------------------------*/

void BoxSolver::process_variable( ColVariable & var )
{
 OFValue a = 0;   // quadratic term
 OFValue b = 0;   // linear term
 VarValue l , u;  // lower and upper bound

 if( f_sol & 2 ) {  // meanwhile compute the dual solution- - - - - - - - - -
  OneVarConstraint * cl = nullptr;
  OneVarConstraint * cu = nullptr;

  // get all the data about the ColVariable
  get_var_data( var , l , u , a , b , cl , cu );

  // variables not belonging to f_Block are fixed
  if( ! is_mine( var ) ) {
   auto x = var.get_value();
   auto dv = a * ( a * x + b );
   f_max_val += dv;
   f_min_val += dv;
   return;
   }

  // now check unfeasibility: if l > u then the whole problem is unfeasible,
  // and it is so for both senses
  if( l > u ) {
   f_state = kInfeasible;
   return;
   }

  // now perform the minimization / maximization

  if( a == 0 )     // linear case
   if( b == 0 ) {  // linear and trivial: b == 0
    // any finite value is optimal, take it "close to 0"
    // the dual solution is already set to 0, which is OK
    if( f_sol & 1 )
     var.set_value( std::min( u , std::max( l , VarValue( 0 ) ) ) );
    }
   else            //  linear but nontrivial: b != 0
    sol_variable( var , l , u , b , cl , cu );
  else             // the quadratic case
   sol_variable( var , l , u , a , b , cl , cu );
  }
 else {             // the dual solution is not required- - - - - - - - - - -
  // get all the data about the ColVariable
  get_var_data( var , l , u , a , b );

  // variables not belonging to f_Block are fixed
  if( ! is_mine( var ) ) {
   auto x = var.get_value();
   auto dv = a * ( a * x + b );
   f_max_val += dv;
   f_min_val += dv;
   return;
   }

  // now check unfeasibility: if l > u then the whole problem is unfeasible,
  // and it is so for both senses
  if( l > u ) {
   f_state = kInfeasible;
   return;
   }

  // now perform the minimization / maximization

  if( a == 0 )     // linear case
   if( b == 0 ) {  // linear and trivial: b == 0
    // any finite value is optimal, take it "close to 0"
    if( f_sol & 1 )
     var.set_value( std::min( u , std::max( l , VarValue( 0 ) ) ) );
    }
   else            // linear but nontrivial: b != 0
    sol_variable( var , l , u , b );
  else             // quadratic case
   sol_variable( var , l , u , a , b );

  }  // end( else( no dual solution )
 }  // end( BoxSolver::process_variable )

/*--------------------------------------------------------------------------*/

void BoxSolver::process_variable_sol( ColVariable & var )
{
 if( ! is_mine( var ) )  // variables not belonging to f_Block are fixed
  return;

 OFValue a = 0;   // quadratic term
 OFValue b = 0;   // linear term
 VarValue l , u;  // lower and upper bound

 // get all the data about the ColVariable
 get_var_data( var , l , u , a , b );

 if( ( a == 0 ) && ( b == 0 ) ) {  // the easy-easy-case: constant obj
  // any finite value is fine, take it "close to 0"
  var.set_value( std::min( u , std::max( l , VarValue( 0 ) ) ) );
  return;
  }

 if( a == 0 )
  process_var_sol( var , l , u , b );
 else
  process_var_sol( var , l , u , a , b );

 }  // end( BoxSolver::process_variable_sol )

/*--------------------------------------------------------------------------*/

void BoxSolver::process_var_sol( ColVariable & var , VarValue l ,
				 VarValue u , OFValue b )
{
 if( f_sense == 1 ) {               // maximization
  if( b > 0 ) {                     // with b > 0
   if( u == INF )                   // if the upper bound is +INF
    throw( std::invalid_argument(
		"BoxSolver::get_var_solution: unexpected unboundedness" ) );
   var.set_value( u );              // primal solution
   }
  else {                            // [maximization] with b < 0
   if( l == -INF )                  // if the lower bound is -INF
    throw( std::invalid_argument(
		"BoxSolver::get_var_solution: unexpected unboundedness" ) );
   var.set_value( l );              // primal solution
   }
  return;
  }

 // minimization
 if( b > 0 ) {                     // with b > 0
  if( l == -INF )                  // if the lower bound is -INF
   throw( std::invalid_argument(
		"BoxSolver::get_var_solution: unexpected unboundedness" ) );
  var.set_value( l );              // primal solution
  }
 else {                            // [minimization] with b < 0
  if( u == INF )                   // if the upper bound is INF
   throw( std::invalid_argument(
		"BoxSolver::get_var_solution: unexpected unboundedness" ) );
  var.set_value( u );              // primal solution
  }
 }  // end( BoxSolver::process_var_sol( b ) )

/*--------------------------------------------------------------------------*/

void BoxSolver::process_var_sol( ColVariable & var , VarValue l , VarValue u ,
				 OFValue a , OFValue b )
{
 // the unique stationary point (max or min depending on the sign of a)
 // of a x^2 + b is at x = - b / ( 2 * a );
 auto q = [ & ]( OFValue y ) -> OFValue { return( ( a * y + b ) * y ); };
 
 if( f_sense == 1 ) {               // maximization
  if( a < 0 ) {                     // with a < 0
   OFValue x = std::max( l , std::min( u , - b / ( 2 * a ) ) );
 
   if( var.is_integer() ) {         // on an integer variable
    OFValue xm = std::floor( x );
    OFValue xp = std::ceil( x );
    if( q( xm ) > q( xp ) )
     x = xm;
    else
     x = xp;
    }

   var.set_value( x );
   }
  else {                            // [maximization] with a > 0
   if( ( l == -INF ) || ( u == INF ) )
    throw( std::invalid_argument(
		"BoxSolver::get_var_solution: unexpected unboundedness" ) );

   var.set_value( q( l ) > q( u ) ? l : u );
   }

  return;  // the quadratic maximization case has been dealt with
  }

 // deal with the quadratic minimization case
 if( a > 0 ) {                     // with a > 0
  OFValue x = std::max( l , std::min( u , - b / ( 2 * a ) ) );
 
  if( var.is_integer() ) {         // on an integer variable
   OFValue xm = std::floor( x );
   OFValue xp = std::ceil( x );
   if( q( xm ) < q( xp ) )
    x = xm;
   else
    x = xp;
   }

  var.set_value( x );
  }
 else {                            // [minimization] with a < 0
  if( ( l == -INF ) || ( u == INF ) )
   throw( std::invalid_argument(
		"BoxSolver::get_var_solution: unexpected unboundedness" ) );
  var.set_value( q( l ) < q( u ) ? l : u );
  }
 }  // end( BoxSolver::process_var_sol( a , b ) )

/*--------------------------------------------------------------------------*/

void BoxSolver::process_variable_dir( ColVariable & var )
{
 if( ! is_mine( var ) )  // variables not belonging to f_Block are fixed
  return;

 OFValue a = 0;   // quadratic term
 OFValue b = 0;   // linear term
 VarValue l , u;  // lower and upper bound

 // get all the data about the ColVariable
 get_var_data( var , l , u , a , b );

 if( a == 0 )
  if( b == 0 )
   var.set_value( 0 );
  else
   process_var_dir_l( var , l , u , b );
 else
  process_var_dir_q( var , l , u , a );

 }  // end( BoxSolver::process_variable_dir )

/*--------------------------------------------------------------------------*/

void BoxSolver::process_var_dir_l( ColVariable & var , VarValue l ,
				   VarValue u , OFValue b )
{
 if( f_sense == 1 )                 // maximization
  if( b > 0 )                       // with b > 0 
   var.set_value( u == INF ? 1 : 0 ); 
  else                              // with b < 0
   var.set_value( l == -INF ? -1 : 0 );
 else                              // minimization
  if( b > 0 )                      // with b > 0
   var.set_value( l == -INF ? -1 : 0 );
  else                              // with b < 0
   var.set_value( u == INF ? 1 : 0 ); 

 }  // end( BoxSolver::process_var_dir_l )

/*--------------------------------------------------------------------------*/

void BoxSolver::process_var_dir_q( ColVariable & var , VarValue l ,
				   VarValue u , OFValue a  )
{
 if( ( ( f_sense == 1 ) && ( a < 0 ) ) ||
     ( ( f_sense == 0 ) && ( a > 0 ) ) )
  var.set_value( 0 );
 else
  var.set_value( u == INF ? 1 : ( l == -INF ? -1 : 0 ) );

 }  // end( BoxSolver::process_var_dir_q )

/*--------------------------------------------------------------------------*/

void BoxSolver::process_variable_dual( ColVariable & var )
{
 if( var.is_integer() )  // integer variables have no dual
  return;

 if( ! is_mine( var ) )  // variables not belonging to f_Block are fixed
  return;

 OFValue a = 0;   // quadratic term
 OFValue b = 0;   // linear term
 VarValue l , u;  // lower and upper bound
 OneVarConstraint * cl = nullptr;  // constraint of active lower bound
 OneVarConstraint * cu = nullptr;  // constraint of active upper bound

 // get all the data about the ColVariable
 get_var_data( var , l , u , a , b , cl , cu );

 if( ( a == 0 ) && ( b == 0 ) )    // the easy-easy-case: constant obj
  return;        // this ColVariable changes nothing; note that the dual
                 // solution is already set to 0, which is OK
 if( a == 0 )
  process_var_dual( var , l , u , b , cl , cu );
 else
  process_var_dual( var , l , u , a , b , cl , cu );

 }  // end( BoxSolver::process_variable_dual )

/*--------------------------------------------------------------------------*/

void BoxSolver::process_var_dual( ColVariable & var ,
				  VarValue l , VarValue u , OFValue b ,
				  OneVarConstraint * cl ,
				  OneVarConstraint * cu )
{
 if( f_sense == 1 ) {               // maximization
  if( b > 0 ) {                     // with b > 0
   if( u == INF )                   // if the upper bound is +INF
    throw( std::invalid_argument(
	       "BoxSolver::get_dual_solution: unexpected unboundedness" ) );
   if( cu )
    cu->set_dual( b );              // dual solution
   }
  else {                            // [maximization] with b < 0
   if( l == -INF )                  // if the lower bound is -INF
    throw( std::invalid_argument(
	       "BoxSolver::get_dual_solution: unexpected unboundedness" ) );
   if( cl )
    cl->set_dual( b );              // dual solution
   }
  }

 // minimization
 if( b > 0 ) {                     // with b > 0
  if( l == -INF )                  // if the lower bound is -INF
   throw( std::invalid_argument(
	       "BoxSolver::get_dual_solution: unexpected unboundedness" ) );
  if( cu )
   cu->set_dual( b );              // dual solution
  }
 else {                            // [minimization] with b < 0
  if( u == INF )                   // if the upper bound is INF
   throw( std::invalid_argument(
	       "BoxSolver::get_dual_solution: unexpected unboundedness" ) );
  if( cl )
   cl->set_dual( b );              // dual solution
  }
 }  // end( BoxSolver::process_var_dual( b ) )

/*--------------------------------------------------------------------------*/

void BoxSolver::process_var_dual( ColVariable & var ,
				  VarValue l , VarValue u ,
				  OFValue a , OFValue b ,
				  OneVarConstraint * cl ,
				  OneVarConstraint * cu  )
{
 // the unique stationary point (max or min depending on the sign of a)
 // of a x^2 + b is at x = - b / ( 2 * a );
 OFValue x = - b / ( 2 * a );
 auto q = [ & ]( OFValue y ) -> OFValue { return( ( a * y + b ) * y ); };
 
 if( f_sense == 1 ) {               // maximization
  if( a < 0 ) {                     // with a < 0
    if( x > u ) {
     x = u;
     if( cu )
      cu->set_dual( 2 * a * x + b );  // dual solution
     }
    else
     if( x < l ) {
      x = l;
      if( cl )
       cl->set_dual( 2 * a * x + b );  // dual solution
      }
   }

  // oherwise no dual solution since it's convex maximization
  return;  // the quadratic maximization case has been dealt with
  }

 // deal with the quadratic minimization case
 if( a > 0 ) {                        // with a > 0
  if( x > u ) {
   x = u;
   if( cu )
    cu->set_dual( - 2 * a * x - b );  // dual solution
   }
  else
   if( x < l ) {
    x = l;
    if( cl )
     cl->set_dual( - 2 * a * x - b ); // dual solution
    }
  }

 // else no dual solution since it's concave minimization

 }  // end( BoxSolver::process_var_dual( a , b ) )

/*--------------------------------------------------------------------------*/
/*----------------------- End File BoxSolver.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
