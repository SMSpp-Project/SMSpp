/*--------------------------------------------------------------------------*/
/*-------------------------- File ColRowSolution.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the ColRowSolution class. A ColRowSolution represents a
 * primal-dual solution of a Block whose Variable are all ColVariable and
 * whose Constraint are all RowConstraint. Usually, this ColRowSolution is
 * constructed by the Block whose solution it represents. A ColRowSolution has
 * a ColVariableSolution and a RowConstraintSolution and therefore it stores
 *
 * - the values of the static and dynamic Variable of a Block as well as the
 *   ColVariableSolution of the nested Block; and
 *
 * - the dual values of the static and dynamic RowConstraint of a Block as
 *   well as the RowConstraintSolution of the nested Block.
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ColRowSolution
#define __ColRowSolution
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ColVariableSolution.h"
#include "RowConstraintSolution.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup ColRowSolution_CLASSES Classes in ColRowSolution.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS ColRowSolution ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a solution (a ColVariableSolution and a RowConstraintSolution) of a Block
/** The ColRowSolution class represents a primal and a dual solution of a
 * Block whose Variable are all ColVariable and whose Constraint are all
 * RowConstraint. It is able to store values of the static and dynamic
 * ColVariable as well as the dual values of the static and dynamic
 * RowConstraint of a Block. The main characteristic of ColRowSolution is that
 * of entirely relying on the "abstract representation" of the Block to work,
 * which means that it can in principle be used with any Block, and in
 * particular with those that only have the "abstract representation", like
 * AbstractBlock.
 *
 * Conversely, this may not necessarily play well with Block that rely on
 * a "physical representation" of the solution information, since the
 * information saved here may not be enough to fully reconstruct it
 * (although it well may). */

class ColRowSolution : public Solution {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

  /// constructor of ColRowSolution, does nothing

 ColRowSolution() : Solution() { }

/*--------------------------------------------------------------------------*/

 ColRowSolution( const ColRowSolution & ) : Solution() {
  throw( std::invalid_argument( "Trying to copy ColRowSolution" ) );
 }
 ///< copy constructor, so that it cannot be used
 /**< inhibit copy constructor */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 // TODO This method is not implemented yet.
 virtual void deserialize( const netCDF::NcGroup & group ) override final;

/*--------------------------------------------------------------------------*/

 virtual ~ColRowSolution() {}  ///< destructor

/** @} ---------------------------------------------------------------------*/
/*---------- METHODS DESCRIBING THE BEHAVIOR OF A ColRowSolution -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a ColRowSolution
 *  @{ */

 /// read the ColRowSolution from the given Block
 /** This method reads the primal and dual solution of the given Block and
  * stores it in this ColRowSolution. For this method to be used, it is
  * required that:
  *
  * 1) the abstract representations of the ColVariable and RowConstraint of
  *    the Block have been generated; and
  *
  * 2) this ColRowSolution has been initialized to represent a primal and dual
  *    solution of the given Block. This should normally mean that this
  *    ColRowSolution was obtained from a call to the method get_Solution() of
  *    the Block associated with this ColRowSolution.
  */

 virtual void read( const Block * const block ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// write the ColRowSolution in the given Block
 /** This method writes the solution currently stored in this ColRowSolution
  * into the given Block. For this method to be used, it is required that:
  *
  * 1) the abstract representations of the ColVariable and RowConstraint of
  *    the Block have been generated; and
  *
  * 2) this ColRowSolution has been initialized to represent a solution of the
  *    given Block. This should normally mean that this ColRowSolution was
  *    obtained from a call to the method get_Solution() of the Block
  *    associated with this ColRowSolution.
  */

 virtual void write( Block * const block ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// serialize a ColRowSolution into a netCDF::NcGroup
 /** Serialize a ColRowSolution into a netCDF::NcGroup, with the following
  * format.
  *
  * TODO This method is not implemented yet.
  */

 virtual void serialize( netCDF::NcGroup & group ) const override final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns a scaled version of this ColRowSolution
 /** This method constructs and returns a scaled version of this
  * ColRowSolution. The new ColRowSolution will have the same structure of
  * this ColRowSolution. This means that the newly created ColRowSolution will
  * be equal to this ColRowSolution except for the values of the Variable and
  * the dual values of the RowConstraint. For each Variable whose value "v" is
  * stored in this Solution, the newly created ColRowSolution will store the
  * value "factor * v". Likewise, for each RowConstraint whose dual value "v"
  * is stored in this ColRowSolution, the newly created ColRowSolution will
  * store the value "factor * v". */

 virtual ColRowSolution * scale( double factor ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// stores a scaled version of the given ColRowSolution
 /** This method stores a scaled version of the ColRowSolution provided as
  * argument (\p solution) into this ColRowSolution. This ColRowSolution is
  * completely destroyed and its structure is reconstructed to be the same as
  * that of the given \p solution. This ColRowSolution will then be equal to
  * the given \p solution except for the values of the Variable and the dual
  * values of the RowConstraint. For each Variable whose value "v" is stored
  * in \p solution, this ColRowSolution will store the value "factor *
  * v". Likewise, for each RowConstraint whose dual value "v" is stored in \p
  * solution, this ColRowSolution will store the value "factor * v". */

 virtual void scale( const ColRowSolution * const solution ,
                     const double factor );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// adds a multiple of the given \p solution to this ColRowSolution
 /** This method adds a multiple of the primal and dual values stored in \p
  * solution to the values stored in this ColRowSolution. The Solution
  * provided as argument must have the same structure as this ColRowSolution,
  * unless this ColRowSolution is empty, in which case this ColRowSolution
  * gets the same structure as that of \p solution.  If the value associated
  * with a Variable is "v" in this ColRowSolution and "v2" in \p solution,
  * then it will become "v + multiplier * v2" in this
  * ColRowSolution. Analogously, if the dual value associated with a
  * RowConstraint is "v" in this ColRowSolution and "v2" in \p solution, then
  * it will become "v + multiplier * v2" in this ColRowSolution. */

 virtual void sum( const Solution * solution , double multiplier ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual ColRowSolution * clone( bool empty = false ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 /// returns a reference to the ColVariableSolution
 const ColVariableSolution & get_variable_solution( void ) const {
  return( f_variable_solution );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 /// returns a reference to the RowConstraintSolution
 const RowConstraintSolution & get_constraint_solution( void ) const {
  return( f_constraint_solution );
  }

/** @} ---------------------------------------------------------------------*/
/*------- METHODS FOR LOADING, PRINTING & SAVING THE ColRowSolution --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for printing the ColRowSolution
 *  @{ */

 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected virtual method print(). This way the operator<<()
  * is defined for each ColRowSolution, but its behavior can be customized by
  * derived classes. */

 friend std::ostream& operator<< ( std::ostream& out ,
                                   const ColRowSolution &o ) {
  o.print( out );
  return( out );
  }

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

protected:

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/

/** @name Protected methods for printing and serializing
    @{ */

 /// print information about the ColRowSolution on an ostream
 /** Protected method intended to print information about the ColRowSolution;
  * it is virtual so that derived classes can print their specific information
  * in the format they choose. */

 virtual void print( std::ostream &output ) const override {
  output << "ColRowSolution [" << this << "]";
  }

/** @} ---------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

 ColVariableSolution f_variable_solution;
 ///< the Solution associated with the ColVariable of the Block

 RowConstraintSolution f_constraint_solution;
 ///< the Solution associated with the RowConstraint of the Block

/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

};  // end( class( ColRowSolution ) )

/** @} end( group( ColRowSolution_CLASSES ) ) ------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* ColRowSolution.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File ColRowSolution.h ---------------------------*/
/*--------------------------------------------------------------------------*/
