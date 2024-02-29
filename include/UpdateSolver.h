/*--------------------------------------------------------------------------*/
/*------------------------- File UpdateSolver.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the UpdateSolver class, which implements a Solver whose
 * role is to immediately map_forward any Modification received to an R3
 * Block of the attached Block, or map_back it to the original Block if it is
 * attached to the R3 Block.
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

#ifndef __UpdateSolver
 #define __UpdateSolver
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
/** @defgroup UpdateSolver_CLASSES Classes in UpdateSolver.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS UpdateSolver -----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// Solver to immediately "forward" Modification to a R3 Block
/** The concrete UpdateSolver class derives from Solver and implements a
 * Solver whose role is to immediately map_forward any Modification received
 * to an R3 Block of the attached Block, or map_back it to the original Block
 * if it is attached to the R3 Block.
 *
 * As such, most of the Solver interface for UpdateSolver is meaningless.
 *
 * UpdateSolver is added to the Solver factory in Solver.cpp. */

class UpdateSolver : public Solver {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*---------------- CONSTRUCTING AND DESTRUCTING UpdateSolver -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing UpdateSolver
 *  @{ */

 /// empty constructor
 UpdateSolver( void ) : Solver() , f_R3B( nullptr ) , f_R3C( nullptr ) ,
  f_options( 0 ) , f_iPM( eNoBlck ) , f_iAM( eModBlck ) { }

/*--------------------------------------------------------------------------*/
 /// constructor: stores the values of the parameters
 /** Constructor. Takes and stores all the relevant parameters:
  *
  * @param R3B is (a pointer to) the "other" to which the Modification have
  *        to be forwarded. If \p forward (see below) is true, then the
  *        Block that UpdateSolver is attached to (f_Block) is the original
  *        Block and R3B is the R3 Block produced by f_Block, hence the 
  *        Modification has to be map_forward from f_Block to R3B. Otherwise,
  *        R3B is the original Block and f_Block is the R3 Block, hence the 
  *        Modification has to be map_back from f_Block to R3B
  *
  * @param r3bc is (a pointer to) the Configuration used to produce the
  *        R3 Block (be it f_Block or R3B)
  *
  * @param options allows to configure how UpdateSolver deals with
  *        Modification, coded bit-wise:
  *
  *        - bit 0: if 0 the Modification are map_forward from f_Block to
  *                 R3B, if 1 the Modification are map_back from R3B to
  *                 f_Block
 *
  *        - bit 1: if 0 Modification are mapped (either _forward or _back),
  *                 if 1 Modification are just passed to the R3B unchanged
  *
  *        - bit 2: if 0 all Modification are mapped, if 1 only Modification
  *                 whose block() is f_Block (i.e., not its inner Block) are
  *                 mapped
  *
  *        - bit 3: if 0 all Modification are mapped, if 1 only Modification
  *                 whose block() is *not* f_Block (i.e., its inner Block
  *                 but not f_Block itself) are mapped
  *
  *        - bit 4:  if 0 all Modification are mapped, if 1 only Modification
  *                  with concerns_Block() == true are mapped
  *
  *        - bit 5:  if 0 all Modification are mapped, if 1 only Modification
  *                  with concerns_Block() == false are mapped
  *
  * @param issuePMod is the value of the issuePMod parameter to be passed to
  *        map_[forward/back]_Modification()
  *
  * @param issueAMod is the value of the issueAMod parameter to be passed to
  *        map_[forward/back]_Modification()
  */

 UpdateSolver( Block *R3B , Configuration *r3bc = nullptr , int options = 0 ,
	       ModParam issuePMod = eNoBlck , ModParam issueAMod = eModBlck )
  : Solver() , f_R3B( R3B ) , f_R3C( r3bc ) , f_options( options ) ,
    f_iPM( issuePMod ) , f_iAM( issueAMod ) {
  if( ! f_R3B )
   throw( std::invalid_argument( "UpdateSolver::UpdateSolver: null R3B" ) );
  }

/*--------------------------------------------------------------------------*/
 /// destructor: it really does nothing since v_mod is empty

 virtual ~UpdateSolver() { }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 void set_R3B( Block * R3B ) { f_R3B = R3B; }

 void set_R3C( Configuration * r3bc = nullptr ) { f_R3C = r3bc; }

 void set_options( int options = 0 ) { f_options = options; }

 void set_issuePMod( ModParam issuePMod = eNoBlck ) { f_iPM = issuePMod; }

 void set_issueAMod( ModParam issueAMod = eNoBlck ) { f_iAM = issueAMod; }

/**@} ----------------------------------------------------------------------*/
/*--------------------- METHODS FOR SOLVING THE MODEL ----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Solving the model encoded by the current Block
 *  @{ */

 /// does not even try to solve the model encoded in the Block
 /** UpdateSolver does not even really try to solve the Block, so all this
  * method does is to return kError. */

 int compute( bool changedvars = true ) override { return( kError ); }

/**@} ----------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/

 bool has_var_solution( void ) override { return( false ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void get_var_solution( Configuration *solc = nullptr ) override {}

/**@} ----------------------------------------------------------------------*/
/*------------- METHODS FOR ADDING / REMOVING / CHANGING DATA --------------*/
/*--------------------------------------------------------------------------*/
/** @name Changing the data of the model
 *  @{ */

 /// add a new Modification to the list
 /** This method is the core of the operation: rather than storing the
  * Modification to v_mod, it immediately invokes either
  * map_forward_Modification() or map_back_Modification() to "forward" it to
  * the R3 Block (or original Block if the modified one was the R3 one). */

 void add_Modification( sp_Mod & mod ) override final
 {
  if( f_no_Mod || ( ! f_Block )  || ( ! f_R3B ) )
   return;

  // check the various cases where the Modification has to be ignored
  auto blck = mod->get_Block();
  if( ( f_options & 4 ) && ( blck != f_Block ) )
   return;

  if( ( f_options & 8 ) && ( blck == f_Block ) )
   return;

  if( ( f_options & 16 ) && ( ! mod->concerns_Block() ) )
   return;

  if( ( f_options & 32 ) && mod->concerns_Block() )
   return;

  // if the Modification just has to be forwarded (instead of mapped), do it
  if( f_options & 2 ) {
   f_R3B->add_Modification( mod );
   return;
   }

  // first, lock the R3B
  bool owned = f_R3B->is_owned_by( f_id );
  if( ( ! owned ) && ( ! f_R3B->lock( f_id ) ) )
   throw( std::logic_error(
		    "UpdateSolver::add_Modification can't lock the R3B" ) );

  // now map the Modification
  if( f_options & 1 )
   f_R3B->map_back_Modification( f_Block , mod.get() ,
				 f_R3C , f_iPM , f_iAM );
  else
   f_Block->map_forward_Modification( f_R3B , mod.get() ,
				      f_R3C , f_iPM , f_iAM );
  // finally, unlock the R3B
  if( ! owned )
   f_R3B->unlock( f_id );
  
  }  // end( add_Modification )

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------------------- PRIVATE FIELDS -------------------------------*/
/*--------------------------------------------------------------------------*/

 Block * f_R3B;  ///< the R3 Block to "forward" the Modification to

 Configuration * f_R3C;  ///< the R3 Configuration of the R3 Block

 int f_options;          ///< how Modification are dealt with, coded bit-wise

 ModParam f_iPM;         ///< the value of the issuePMod parmeter

 ModParam f_iAM;         ///< the value of the issueAMod parmeter

/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };   // end( class UpdateSolver )

/** @} end( group( UpdateSolver_CLASSES ) ) ----------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* UpdateSolver.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File UpdateSolver.h -------------------------*/
/*--------------------------------------------------------------------------*/





