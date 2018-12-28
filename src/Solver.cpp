/*--------------------------------------------------------------------------*/
/*---------------------------- File Solver.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the Solver class. It also registers FakeSolver in the
 * Solver factory.
 *
 * \version 0.20
 *
 * \date 24 - 07 - 2018
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"
#include "Solver.h"

#include "FakeSolver.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register the various FakeSolver to the Solver factory

SMSpp_insert_in_factory_cpp_0( FakeSolver );

/*--------------------------------------------------------------------------*/
// define and initialize here the vector of int parameters names
const std::vector< std::string > Solver::int_pars_str =
                               { "intMaxIter" , "intMaxSol" , "intLogVerb" };

// define and initialize here the vector of double parameters names
const std::vector< std::string > Solver::dbl_pars_str =
		     { "dblMaxTime"  , "dblRelAcc"   ,  "dblAbsAcc"   ,
		       "dblUpCutOff" , "dblLwCutOff" ,  "dblRAccSol"  ,
		       "dblAAccSol"  , "dblFAccSol"
		      };

// define and initialize here the map for double parameters names
const std::map< std::string , Solver::idx_type > Solver::dbl_pars_map =
                   { { "dblMaxTime"  , Solver::dblMaxTime  } ,
		     { "dblRelAcc"   , Solver::dblRelAcc   } ,
		     { "dblAbsAcc"   , Solver::dblAbsAcc   } ,
		     { "dblUpCutOff" , Solver::dblUpCutOff } ,
		     { "dblLwCutOff" , Solver::dblLwCutOff } ,
		     { "dblRAccSol"  , Solver::dblRAccSol  } ,
		     { "dblAAccSol"  , Solver::dblAAccSol  } ,
		     { "dblFAccSol"  , Solver::dblFAccSol  }
                    };

// define and initialize here the default int parameters
const std::vector<int> Solver::dflt_int_par =
                       { Inf<int>() ,  // intMaxIter
			 1 ,           // intMaxSol
			 0             // intLogVerb
                         };

// define and initialize here the default double parameters
const std::vector<double> Solver::dflt_dbl_par =
                       { Inf<double>() ,             // dblMaxTime
			 1e-6 ,                      // dblRelAcc
			 Inf<Solver::OFValue>() ,    // dblAbsAcc
			 Inf<Solver::OFValue>() ,    // dblUpCutOff
			 - Inf<Solver::OFValue>() ,  // dblLwCutOff
			 Inf<Solver::OFValue>() ,    // dblRAccSol
			 Inf<Solver::OFValue>() ,    // dblAAccSol
			 0                           // dblFAccSol
                         };

/*--------------------------------------------------------------------------*/
/*---------------------------- METHODS of Solver ---------------------------*/
/*--------------------------------------------------------------------------*/

void Solver::set_Block( Block *block )
{
 if( f_Block == block )  // registering to the same Block
  return;                // cowardly and silently return

 if( f_Block )           // was attached to some Block
  v_mod.clear();         // any pending Modification was about the old
                         // Block, so it is now irrelevant

 f_Block = block;        // this is the new Block now

 if( block )             // if it really is a Block (i.e., not nullptr)
  f_Block->register_Solver( this );  // register to it
 }

/*--------------------------------------------------------------------------*/

Solver::SolverFactoryMap & Solver::f_factory( void )
{
 static SolverFactoryMap s_factory;
 return( s_factory );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File Solver.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
