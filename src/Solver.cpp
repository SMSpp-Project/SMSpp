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
// define and initialize here the vector of int parameters names
const std::vector< std::string > Solver::int_pars_str = {
 "intMaxIter" , "intMaxThread" , "intEverykIt" , "intMaxSol" , "intLogVerb" };

// define and initialize here the vector of double parameters names
const std::vector< std::string > Solver::dbl_pars_str =
 { "dblMaxTime" ,  "dblEveryTTm" , "dblRelAcc" , "dblAbsAcc" , "dblUpCutOff" ,
   "dblLwCutOff" , "dblRAccSol" , "dblAAccSol" , "dblFAccSol" };

// define and initialize here the map for int parameters names
const std::map< std::string, Solver::idx_type > Solver::int_pars_map =
 { { "intMaxIter",   Solver::intMaxIter },
   { "intMaxThread", Solver::intMaxThread },
   { "intEverykIt",  Solver::intEverykIt },
   { "intMaxSol",    Solver::intMaxSol },
   { "intLogVerb",   Solver::intLogVerb }
  };

// define and initialize here the map for double parameters names
const std::map< std::string, Solver::idx_type > Solver::dbl_pars_map =
 { { "dblMaxTime",  Solver::dblMaxTime } ,
   { "dblEveryTTm", Solver::dblEveryTTm } ,
   { "dblRelAcc",   Solver::dblRelAcc } ,
   { "dblAbsAcc",   Solver::dblAbsAcc } ,
   { "dblUpCutOff", Solver::dblUpCutOff } ,
   { "dblLwCutOff", Solver::dblLwCutOff } ,
   { "dblRAccSol",  Solver::dblRAccSol } ,
   { "dblAAccSol",  Solver::dblAAccSol } ,
   { "dblFAccSol",  Solver::dblFAccSol }
  };

// define and initialize here the default int parameters
const std::vector< int > Solver::dflt_int_par =
 { Inf< int >() ,  // intMaxIter
   0 ,             // intEverykIt
   0 ,             // intMaxThread
   1 ,             // intMaxSol
   0               // intLogVerb
  };

// define and initialize here the default double parameters
const std::vector< double > Solver::dflt_dbl_par =
 { Inf< double >() ,            // dblMaxTime
   0 ,                          // dblEveryTTm
   1e-6 ,                       // dblRelAcc
   Inf< Solver::OFValue >() ,   // dblAbsAcc
   Inf< Solver::OFValue >() ,   // dblUpCutOff
   -Inf< Solver::OFValue >() ,  // dblLwCutOff
   Inf< Solver::OFValue >() ,   // dblRAccSol
   Inf< Solver::OFValue >() ,   // dblAAccSol
   0                            // dblFAccSol
  };

/*--------------------------------------------------------------------------*/
/*---------------------------- METHODS of Solver ---------------------------*/
/*--------------------------------------------------------------------------*/

void Solver::set_Block( Block * block )
{
 if( f_Block == block )  // registering to the same Block
  return;                // cowardly and silently return

 if( f_Block )           // was attached to some Block
  v_mod.clear();         // any pending Modification was about the old
                         // Block, so it is now irrelevant

 f_Block = block;        // this is the new Block now
 }

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
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/

Solver::SolverFactoryMap & Solver::f_factory( void ) {
 static SolverFactoryMap s_factory;
 return( s_factory );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File Solver.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
