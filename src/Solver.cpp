/*--------------------------------------------------------------------------*/
/*---------------------------- File Solver.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the Solver class. It also registers FakeSolver in the
 * Solver factory.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
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
#include "Objective.h"
#include "Solver.h"

#include "FakeSolver.h"

#include "UpdateSolver.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/
/* Used in reset_event_handler(). For some unfathomable reason need be
 * defined as a function rather than as a Lambda. */

int do_nothing( void ) { return( Solver::eContinue ); };

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register FakeSolver to the Solver factory

SMSpp_insert_in_factory_cpp_0( FakeSolver );

/*--------------------------------------------------------------------------*/
// register UpdateSolver to the Solver factory

SMSpp_insert_in_factory_cpp_0( UpdateSolver );

/*--------------------------------------------------------------------------*/
/*---------------------------- METHODS of Solver ---------------------------*/
/*--------------------------------------------------------------------------*/

void Solver::set_Block( Block * block )
{
 if( f_Block == block )  // registering to the same Block
  return;                // cowardly and silently return

 // refuse to be attached to a Block that the caller has just told us
 // to ignore: this catches the obvious misuse where the very root of
 // the tree to be solved is included in the exclusion list, leaving
 // the Solver with nothing to do. Note that descendants of an
 // excluded ancestor are still allowed, since the exclusion semantics
 // is "prune that subtree from the model"; only the root of the
 // attach matters here
 if( block && f_excluded.count( block ) > 0 )
  throw( std::logic_error(
       "Solver::set_Block: the Block being attached is in the "
       "excluded set; the Solver would have nothing to do" ) );

 if( f_Block )           // was attached to some Block
  v_mod.clear();         // any pending Modification was about the old
                         // Block, so it is now irrelevant

 f_Block = block;        // this is the new Block now
 }

/*--------------------------------------------------------------------------*/

void Solver::set_excluded_blocks(
                          const std::unordered_set< Block * > * ignored )
{
 f_excluded.clear();
 if( ignored && ! ignored->empty() )
  f_excluded = *ignored;  // store the user-supplied minimal set verbatim;
                          // is_excluded() walks the father chain to test
                          // whether a Block is a descendant of any excluded
                          // ancestor, so we do NOT eagerly enumerate every
                          // descendant here
 }

/*--------------------------------------------------------------------------*/

bool Solver::is_excluded( Block * b ) const
{
 if( f_excluded.empty() || ! b )
  return( false );
 // walk b -> b->get_f_Block() -> ... until either some ancestor is in the
 // excluded set (return true) or the root is reached (return false). Note
 // that the f_Block boundary itself is not relevant here: a Solver is
 // attached to a specific Block, and the excluded set is a subset of the
 // descendants of that Block; walking past it returns false naturally
 for( Block * cur = b ; cur ; cur = cur->get_f_Block() )
  if( f_excluded.count( cur ) > 0 )
   return( true );
 return( false );
 }

/*--------------------------------------------------------------------------*/

void Solver::set_par( idx_type par , std::string && value )
{
 if( par == strLogFileName ) {
  if( f_log_file.is_open() )
   f_log_file.close();
  f_log_file.open( value , std::fstream::out | std::fstream::trunc );
  set_log( & f_log_file );
  return;
  }
 ThinComputeInterface::set_par( par , std::move( value ) );
 }  // end( Solver::set_par( std::string && ) )

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS FOR EVENTS HANDLING -----------------------*/
/*--------------------------------------------------------------------------*/

void Solver::reset_event_handler( int type , EventID id )
{
 if( type >= max_event_number() )
  throw( std::invalid_argument( "unsupported event type " +
                                 std::to_string( type ) ) );

 if( id >= v_events[ type ].size() )
  throw( std::invalid_argument( "incorrect event id " + std::to_string( id )
				+ " for type " + std::to_string( type ) ) );

 if( id == v_events[ type ].size() - 1 ) {
  // if the event is the last of its type, shorten the vector; moreover, if
  // any pf the previous events is a do_nothing, keep shortening
  do
   v_events[ type ].pop_back();
  while( ( ! v_events[ type ].empty() ) &&
         ( *( v_events[ type ].back().target < int( * )() > ( ) ) ==
         do_nothing ) );
  }
 else
  // the event is not the last of its type: replace it with a do_nothing to
  // avoid messing up with the id-s, which are positions in the vector
  v_events[ type ][ id ] = do_nothing;

 }  // end( Solver::reset_event_handler )

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/

Solver::OFValue Solver::get_var_value( void ) {
 return( f_Block ? f_Block->get_objective_sense() == Objective::eMin ?
	           get_ub() : get_lb()
                 : Objective::eMin );
 }

/*--------------------------------------------------------------------------*/

Solution * Solver::get_Solution( Configuration * solc )
{
 if( this->has_var_solution() ) {
  f_Block->lock( f_id );
  this->get_var_solution( solc );
  }
 else
  if( this->has_var_direction() ) {
   f_Block->lock( f_id );
   this->get_var_direction( solc );
   }
  else
   return( nullptr );

 auto sol = f_Block->get_Solution( solc , false );
 f_Block->unlock( f_id );
 return( sol );
 }

/*--------------------------------------------------------------------------*/

const std::string & Solver::get_str_par( idx_type par ) const {
 switch( par ) {
  case( strLogFileName ):   return( LogFileName );
  default:                  return( ThinComputeInterface::get_str_par( par ) );
  }
 }  // end( Solver::get_str_par )

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/

Solver::SolverFactoryMap & Solver::f_factory( void ) {
 static SolverFactoryMap s_factory;
 return( s_factory );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File Solver.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
