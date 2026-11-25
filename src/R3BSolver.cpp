/*--------------------------------------------------------------------------*/
/*-------------------------- File R3BSolver.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the R3BSolver class, which solves a Block through one of
 * its R3 Block. It also registers R3BSolver in the Solver factory.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
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

#include "R3BSolver.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*-------------------------------- TYPES -----------------------------------*/
/*--------------------------------------------------------------------------*/

using p_VConf = SimpleConfiguration< std::vector< Configuration * > > *;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register R3BSolver to the Solver factory

SMSpp_insert_in_factory_cpp_0( R3BSolver );

/*--------------------------------------------------------------------------*/
/*------------------------- METHODS of R3BSolver ---------------------------*/
/*--------------------------------------------------------------------------*/

R3BSolver::~R3BSolver()
{
 delete_R3B();
 delete f_R3Cfg;
 }

/*--------------------------------------------------------------------------*/

void R3BSolver::set_Block( Block * block )
{
 if( f_Block == block )  // registering to the same Block
  return;                // cowardly and silently return

 delete_R3B();           // the R3 Block of the old Block (if any) goes
 Solver::set_Block( block );
 make_R3B();             // that of the new one comes (if all is there)
 }

/*--------------------------------------------------------------------------*/

void R3BSolver::set_ComputeConfig( const ComputeConfig * scfg )
{
 Solver::set_ComputeConfig( scfg );

 p_VConf ncfg = nullptr;
 if( scfg && scfg->f_extra_Configuration ) {
  ncfg = dynamic_cast< p_VConf >( scfg->f_extra_Configuration );
  if( ( ! ncfg ) || ncfg->f_value.empty() || ( ncfg->f_value.size() > 3 ) )
   throw( std::invalid_argument( "R3BSolver::set_ComputeConfig: the extra "
				 "Configuration is not a vector of the r3bc, "
				 "the BlockConfig and the BlockSolverConfig"
				 ) );
  if( ( ncfg->f_value.size() > 1 ) && ncfg->f_value[ 1 ] &&
      ( ! dynamic_cast< BlockConfig * >( ncfg->f_value[ 1 ] ) ) )
   throw( std::invalid_argument( "R3BSolver::set_ComputeConfig: the 2nd "
				 "extra Configuration is not a BlockConfig" ) );
  if( ( ncfg->f_value.size() < 3 ) ||
      ( ! dynamic_cast< BlockSolverConfig * >( ncfg->f_value[ 2 ] ) ) )
   throw( std::invalid_argument( "R3BSolver::set_ComputeConfig: the 3rd "
				 "extra Configuration is not a "
				 "BlockSolverConfig" ) );
  }

 // a differential ComputeConfig with no extra Configuration keeps it
 if( scfg && scfg->diff() && ( ! ncfg ) )
  return;

 delete_R3B();
 delete f_R3Cfg;
 f_R3Cfg = ncfg ? ncfg->clone() : nullptr;
 make_R3B();
 }

/*--------------------------------------------------------------------------*/

ComputeConfig * R3BSolver::get_ComputeConfig( bool all ,
					      ComputeConfig * ocfg ) const
{
 auto ccfg = Solver::get_ComputeConfig( all , ocfg );
 if( ! f_R3Cfg )
  return( ccfg );

 if( ! ccfg ) {  // all parameters at their default, but the extra is there
  ccfg = new ComputeConfig;
  ccfg->set_diff( ! all );
  }

 delete ccfg->f_extra_Configuration;
 ccfg->f_extra_Configuration = f_R3Cfg->clone();
 return( ccfg );
 }

/*--------------------------------------------------------------------------*/

int R3BSolver::compute( bool changedvars )
{
 if( ! f_inner )
  return( kError );

 return( f_inner->compute( changedvars ) );
 }

/*--------------------------------------------------------------------------*/

Solver::OFValue R3BSolver::get_lb( void )
{
 // the lower bound of the R3 Block is a dual one if minimizing, a primal
 // one if maximizing
 if( f_inner && ( ( f_kind == kReformulation ) ||
		  ( ( f_kind == kRelaxation ) == is_min() ) ) )
  return( f_inner->get_lb() );

 return( - Inf< OFValue >() );
 }

/*--------------------------------------------------------------------------*/

Solver::OFValue R3BSolver::get_ub( void )
{
 // the upper bound of the R3 Block is a primal one if minimizing, a dual
 // one if maximizing
 if( f_inner && ( ( f_kind == kReformulation ) ||
		  ( ( f_kind == kRestriction ) == is_min() ) ) )
  return( f_inner->get_ub() );

 return( Inf< OFValue >() );
 }

/*--------------------------------------------------------------------------*/

Solver::OFValue R3BSolver::get_var_value( void )
{
 if( ! f_inner )
  return( is_min() ? Inf< OFValue >() : - Inf< OFValue >() );

 return( f_inner->get_var_value() );
 }

/*--------------------------------------------------------------------------*/

void R3BSolver::get_var_solution( Configuration * solc )
{
 if( ( ! f_inner ) || ( ! f_Block ) )
  return;

 f_inner->get_var_solution();  // the whole solution, in the R3 Block
 f_Block->map_back_solution( f_R3B , get_R3B_Configuration() , solc );
 }

/*--------------------------------------------------------------------------*/

void R3BSolver::add_Modification( sp_Mod & mod )
{
 if( f_no_Mod || ( ! f_Block ) || ( ! f_R3B ) )
  return;

 bool owned = f_R3B->is_owned_by( f_id );
 if( ( ! owned ) && ( ! f_R3B->lock( f_id ) ) )
  throw( std::logic_error(
		   "R3BSolver::add_Modification: can't lock the R3 Block" ) );

 // unlock the R3 Block also if mapping throws
 try {
  f_Block->map_forward_Modification( f_R3B , mod.get() ,
				     get_R3B_Configuration() );
  }
 catch( ... ) {
  if( ! owned )
   f_R3B->unlock( f_id );
  throw;
  }

 if( ! owned )
  f_R3B->unlock( f_id );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/

void R3BSolver::make_R3B( void )
{
 if( f_R3B || ( ! f_Block ) || ( ! f_R3Cfg ) )
  return;

 const auto & V = f_R3Cfg->f_value;

 f_R3B = f_Block->get_R3_Block( V[ 0 ] );
 if( ! f_R3B )
  throw( std::invalid_argument( "R3BSolver::make_R3B: " +
				f_Block->classname() + " makes no such R3 "
				"Block" ) );

 try {
  // apply() moves the Configuration of the BlockConfig into the Block,
  // hence a copy is applied
  if( auto bc = dynamic_cast< BlockConfig * >( V[ 1 ] ) ) {
   auto cbc = bc->clone();
   cbc->apply( f_R3B );
   delete cbc;
   }

  // the sub-Block of the R3 Block must all be there before any Solver is
  // registered to it, and some may only appear when its abstract Variable
  // are generated
  f_R3B->generate_abstract_variables();

  // a copy of the BlockSolverConfig is apply()-ed, and then clear()-ed so
  // that apply()-ing it again deletes the Solver it has registered
  f_R3BSC = static_cast< BlockSolverConfig * >( V[ 2 ] )->clone();
  f_R3BSC->apply( f_R3B );
  f_R3BSC->clear();

  if( f_R3B->get_registered_solvers().empty() )
   throw( std::invalid_argument( "R3BSolver::make_R3B: the "
				 "BlockSolverConfig registers no Solver to "
				 "the R3 Block" ) );
  }
 catch( ... ) {
  delete_R3B();
  throw;
  }

 f_inner = f_R3B->get_registered_solvers().front();
 }

/*--------------------------------------------------------------------------*/

void R3BSolver::delete_R3B( void )
{
 f_inner = nullptr;

 if( f_R3BSC ) {
  if( f_R3B )
   f_R3BSC->apply( f_R3B );
  delete f_R3BSC;
  f_R3BSC = nullptr;
  }

 delete f_R3B;
 f_R3B = nullptr;
 }

/*--------------------------------------------------------------------------*/
/*----------------------- End File R3BSolver.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
