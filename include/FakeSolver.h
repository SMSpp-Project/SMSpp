/*--------------------------------------------------------------------------*/
/*------------------------- File FakeSolver.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the FakeSolver class, which implements a "fake", "peeping
 * Tom" Solver whose role is not really that of solving any Block, but just
 * that of syphoning off and storing away all the Modification that the Block
 * produces.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __FakeSolver
 #define __FakeSolver /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Solver.h"

/*--------------------------------------------------------------------------*/
/*----------------------------- NAMESPACE ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup FakeSolver_CLASSES Classes in FakeSolver.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS FakeSolver -------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// "fake", "peeping Tom" Solver to store all the Modification of a Block
/** The concrete FakeSolver class derives from Solver and implements a
 * "fake", "peeping Tom" Solver whose role is not really that of solving any
 * Block, but just that of syphoning off and storing away all the 
 * Modification that the Block produces. As such, most of the Solver interface
 * for FakeSolver is meaningless.
 *
 * FakeSolver is added to the Solver factory in Solver.cpp. */

class FakeSolver : public Solver {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------- CONSTRUCTING AND DESTRUCTING FakeSolver -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing FakeSolver
 *  @{ */

 /// constructor: does nothing special
 FakeSolver( void ) : Solver() {}

/*--------------------------------------------------------------------------*/
 /// destructor: it has to release all the Modifications

 ~FakeSolver() override = default;

/** @} ---------------------------------------------------------------------*/
/*--------------------- METHODS FOR SOLVING THE MODEL ----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Solving the model encoded by the current Block
 *  @{ */

 /// does not even try to solve the model encoded in the Block
 /** FakeSolver does not even really try to solve the Block, so all this
  * method does is to return kError. */

 int compute( bool changedvars = true ) override { return( kError ); }

/** @} ---------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/

 bool has_var_solution() override { return( false ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void get_var_solution( Configuration * solc = nullptr ) override {}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// provide unfettered access to the list of Modification
 /** This method returns a non-const reference to the data structure holding
  * the list of Modification of the FakeSolver. By means of that reference the
  * user has unfettered access to that, and she's responsible for clearing it
  * once it has done with the Modification whatever she wants to.
  *
  * Because this is done externally, methods are provided to expose the
  * internal atomic flag that is supposed to protect the list. */

 Lst_sp_Mod & get_Modification_list() { return( v_mod ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// ensure that the list of Modification is locked, only return when it is

 void lock_Modification_list() {
  // try to acquire lock, spin on failure
  while( f_mod_lock.test_and_set( std::memory_order_acquire ) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// release the lock on the list of Modification

 void unlock_Modification_list() {
  f_mod_lock.clear( std::memory_order_release );  // release lock
 }

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

};   // end( class FakeSolver )

/** @} end( group( FakeSolver_CLASSES ) ) ----------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* FakeSolver.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File FakeSolver.h ---------------------------*/
/*--------------------------------------------------------------------------*/





