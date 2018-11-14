/*--------------------------------------------------------------------------*/
/*------------------------- File FakeSolver.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the FakeSolver class, which implements a "fake", "peeping
 * Tom" Solver whose role is not really that of solving any Block, but just
 * that of syphoning off and storing away all the Modification that the Block
 * produces.
 *
 * \version 0.10
 *
 * \date 01 - 10 - 2018
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __FakeSolver
 #define __FakeSolver
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Solver.h"

/*--------------------------------------------------------------------------*/
/*----------------------------- NAMESPACE ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

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

/*@} -----------------------------------------------------------------------*/
/*---------------- CONSTRUCTING AND DESTRUCTING FakeSolver -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing FakeSolver
 *  @{ */

 /// constructor: does nothing special
 FakeSolver( void ) : Solver() {}

/*--------------------------------------------------------------------------*/
 /// destructor: it has to release all the Modifications

 virtual ~FakeSolver() { }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the (pointer to the) Block that the FakeSolver has to solve
 /** Method to set the (pointer to the) Block that the FakeSolver has to solve.
  * If there were any other block attached to Block this FakeSolver, any
  * information about solutions to the previous Block the FakeSolver had
  * computed is lost for good. This is why the method is virtual: derived
  * classes may need to do more to reach to such an abrupt change.
  *
  * Passing block == nullptr signals to the FakeSolver to discard every
  * information related to the solution process of the previous Block (if
  * any), and sit down quietly in a corner waiting for new orders. */

 virtual void set_Block( Block *block ) override
 {
  if( f_Block ) {  // there was another Block attached before
   f_Block->unregister_Solver( this );  // this Solver looks elsewhere now
   v_mod.clear();  /* any outstanding Modification was for the old Block,
		    * hence it is irrelevant now. */
   }

  Solver::set_Block( block );  // attach to the new Block
  }

/*@} -----------------------------------------------------------------------*/
/*--------------------- METHODS FOR SOLVING THE MODEL ----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Solving the model encoded by the current Block
 *  @{ */

 /// does not even try to solve the model encoded in the Block
 /** FakeSolver does not even really try to solve the Block, so all this
  * method does is to return kError. */

 virtual int compute( bool changedvars = true ) override
 {
  return( kError );
  }

/*@} -----------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/

 virtual bool has_var_solution( void ) override { return( false ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual void get_var_solution( void ) override { }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 /// provide unfettered access to the list of Modificatiomn
 /** This method returns a non-const reference to the data structure holding
  * the list of Modification of the FakeSolver. By means of that reference the
  * user has unfettered access to that, and she's responsible for clearing it
  * once it has done with the Modification whatever she wants to. */

 Lst_sp_Mod & get_Modification_list( void )
 {
  return( v_mod );
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

/*@}  end( group( FakeSolver_CLASSES ) ) -----------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* FakeSolver.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File FakeSolver.h ---------------------------*/
/*--------------------------------------------------------------------------*/





