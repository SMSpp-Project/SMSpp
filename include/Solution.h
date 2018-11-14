/*--------------------------------------------------------------------------*/
/*---------------------------- File Solution.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the Solution class. The Solution class represents a
 * solution of a Block. A solution of a Block can be composed, for example,
 * by the values of the static and dynamic Variable and dual variables of
 * the Constraint of a Block.
 *
 * \version 0.10
 *
 * \date 11 - 05 - 2018
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Department of Applied Mathematics \n
 *         State University of Campinas, Brazil \n
 *
 * Copyright &copy by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __Solution
 #define __Solution  /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"

#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------ Solution-RELATED TYPES --------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Solution_TYPES Solution-related types
 *  @{ */

 class Block;            // forward definition of Block
 class Solution;         // forward definition of Solution

 typedef Solution * p_Solution;
 ///< a pointer to Solution

 typedef std::vector<p_Solution> Vec_Solution;
 ///< a vector of pointers to Solution

/** @}  end( group( Solution_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Solution_CLASSES Classes in Solution.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS Solution -------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a solution of a Block
/** The Solution class represents a solution of a Block. A solution of a
 * Block can be composed, for example, by the values of the static and
 * dynamic Variable and dual variables of the Constraint of a Block. This
 * Solution be used to store and retrieve those values. A Solution must
 * implement the method
 *
 *    void read( Block * const block )
 *
 * which takes a (pointer to a) Block, reads the values of the current
 * solution of this Block and stores them. Once the values of a solution of a
 * Block have been stored, they can be retrieved by the method
 *
 *    void write( Block * const block )
 *
 * which takes a (pointer to a) Block and writes the value of the solution
 * currently stored in this Solution into the Block.
 *
 * A Solution is normally constructed by the Block whose solution must be
 * stored. When it is constructed, a Solution object can be "configured" to
 * take only a specific part of the Block solution status (see the
 * Configuration parameter in Block::get_Solution()); say, only the primal
 * or the dual values, only the values of a specific set of Variable, ...
 * This configuration is "permanent": once a Solution is object created, it
 * will only store that particular set of information. Trying to read a
 * Solution from a Block that does not have the required information (say,
 * because dual information is required which is stored in some Constraint,
 * but these have not been constructed yet) is an error ans should result in
 * an exception being thrown.
 *
 * Of course, it is a fortiori an error (resulting in an exception being
 * thrown) to read or write a Solution out of the wrong Block. This does not
 * only mean "the wrong type of Block", but basically "the very same Block
 * that has created the solution object", or at least one that is "identical"
 * to it (say, a copy Block constructed as an R3 Block), or at the very very
 * least that is "compatible" (meaning it has the same size in the relevant
 * sets of Variable / Constraint). Solution are not meant to be exchanged
 * between different Block, even of the same type.
 *
 * Solution also provides support for producing weighted sum of solutions of
 * a given Block, which in particular allows to produce convex combinations
 * of them (convexity being an all-important property, and convex relaxations
 * being at the heart of countless many optimization techniques). This is
 * somehow delicate because some Solution, in particular discrete ones, may
 * not "be happy" with being arbitrarily scaled and/or summed, and thus
 * requires some care, see the comments to scale() and sum(). */

class Solution {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*@} -----------------------------------------------------------------------*/
/*----------------- CONSTRUCTING AND DESTRUCTING Solution ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing Solution
 *  @{ */

 Solution( void ) { }  ///< constructor of Solution, it has nothing to do

/*--------------------------------------------------------------------------*/

 Solution( const Solution & ) = delete;  ///< inhibit copy constructor

 /// inhibit assignment operator
 Solution & operator=( const Solution& ) = delete;

/*--------------------------------------------------------------------------*/

 virtual ~Solution() { }  ///< destructor: it is virtual, and empty

/*@} -----------------------------------------------------------------------*/
/*-------------- METHODS DESCRIBING THE BEHAVIOR OF A Solution -------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a Solution
 *  @{ */

 /// read the solution from the given Block
 /** This method reads the solution of the given Block and stores it in this
  * Solution. A Solution object can be "configured" to take only a specific
  * part of the Block solution status: it is an error if block does not have
  * the required part (and, a fortiori, if block is not the right Block). */

 virtual void read( const Block * const block ) = 0;

/*--------------------------------------------------------------------------*/
 /// write the solution in the given Block
 /** This method writes the solution currently stored in this Solution in the
  * given Block. A Solution object can be "configured" to take only a specific
  * part of the Block solution status: it is an error if block does not have
  * the required part (and, a fortiori, if block is not the right Block). */

 virtual void write( Block * const block ) = 0;

/*--------------------------------------------------------------------------*/
 /// returns a scaled version of this Solution
 /** This method constructs and returns a scaled version of this Solution,
  * where each of the solution information is scaled by the given double
  * value. A Solution object can be "configured" to take only a specific
  * part of the Block solution status: the scaled version of a Solution
  * object obviously "shares the same configuration" as the original object.
  *
  * Scaling by a double is a "somewhat dangerous" operation: it is quite
  * natural if all solution information is "double", which is what is most
  * likely to happen most of the time, but not in all cases. For instance,
  * some Block may correspond to combinatorial problems whose solutions are
  * essentially combinatorial objects (paths/cuts on a graph ...), for which
  * "scaling" makes little sense. Yet, these problems are typically also
  * represented in terms of subspaces of \R^n, and therefore one might
  * expect scaling to be possible. However, it is clear that scaling may
  * destroy some of the properties that solutions have: for instance, a
  * path in a graph can be represented by means of a predecessor function,
  * but a scaled path can not -- in the sense that it needs at least another
  * information, the scaling factor, or to be transformed into a different
  * format, such as the amount of "flow" going on each arc of the graph.
  *
  * This implies that there might be different Solution objects relative to
  * a given Block; say, the "original ones" corresponding to combinatorial
  * solutions (a path, represented by a predecessor function) and the "scaled
  * ones) produced by scaling and/or summing (see sum()) below (say, a double
  * for each arc of the graph). Thus, scale() might return a Solution object
  * that, while being appropriate for the original Block, may in fact be "of
  * a different type" than the originating one". The requirement is that the
  * newly created Solution must be a "general" one, in the sense that it
  * makes sense (obviously) to scale it, and also to *sum* it with other
  * Solution objects, see sum(). */

 virtual Solution * scale( double factor ) const = 0;

/*--------------------------------------------------------------------------*/
 /// adds a scaled version of the given Solution to this Solution
 /** This method adds a scaled version of the given Solution (see scale()) to
  * this Solution. A Solution object can be "configured" to take only a
  * specific part of the Block solution status: this means that even of the
  * Solution object pointed by solution has "more information" than the
  * current one, only the relevant part will be extracted and summed to that
  * of the current one, so that "the original configuration is preserved"
  * even after this operation. It is an error if solution does not have the
  * required part (and, a fortiori, if it is not the right Solution),
  * resulting in an exception being thrown.
  *
  * As discussed in scale(), scaling by a double is a "somewhat dangerous"
  * operation that may not necessarily make sense for all kinds of solution
  * information, in particular discrete ones. The same potentially holds for
  * sums (many combinatorial structures are closed under sum but not all are),
  * and a fortiori for "sum with a scaled object". Thus, some Solution may not
  * be able to properly implement this operation without fundamentally alter
  * their own internal representation, which is not supposed to happen. Thus,
  * 
  *    IT IS NOT NECESSARILY SAFE TO CALL sum() ON A Solution JUST
  *    PRODUCED BY Block::get_Solution()
  *
  * although in general it should always be possible to "configure the
  * Solution", by using the corresponding Configuration object to instruct
  * the Block to produce the kind of Solution object for which it is safe.
  * Furthermore,
  *
  *    IT IS SAFE TO CALL sum() ON A SOLUTION CONSTRUCTED BY scale()
  *
  * That is, scale() has to report a "general" Solution, one for which it
  * makes sense *both* scale it and sum it with other Solution objects.
  * Note that the idea is that is must be always possible to use "less
  * general" solution objects as the solution parameter in sum(): the
  * recipient (current) Solution object must be "general" for sum() to be
  * possible, but the summed one need not be. Of course, all this must be
  * entirely handled by the (different variants of) Solution.
  *
  * It should be remarked that there could be "intermediate" types of
  * Solution objects between the "less general" and the "more general" ones.
  * For instance, some discrete structures are closed under sum, or even
  * scaled sum where the scalar has appropriate properties (say, it's an
  * integer). Thus, it may not be efficient to require scale() to return the
  * "more general" Solution. Yet, handling these special cases should always
  * be possible by requiring the Block to produce "the right kind of Solution
  * object" by means of its Configuration. */

 virtual void sum( const Solution * solution , double multiplier ) = 0;

/*--------------------------------------------------------------------------*/
/// returns a clone of this Solution
/** This method creates and returns a Solution of the same type of this
 * Solution. If the parameter empty is true, then the returned Solution is
 * "empty", i.e., the solution information is not passed over, otherwise the
 * new Solution is a complete copy of the current one. A Solution object can
 * be "configured" to take only a specific part of the Block solution status:
 * the cloned object obviously "shares the same configuration" as the
 * original object.
 *
 * Note that clone() and scale( 1 ) return in principle the same Solution.
 * However, scale( 1 ) must return a "general solution" (see comments in
 * scale() and sum()), whereas clone() can return exactly the same type of
 * solution as the current one, i.e., a "less general" one if this is. */

 virtual Solution * clone( bool empty = false ) const = 0;

/*@} -----------------------------------------------------------------------*/
/*----------- METHODS FOR LOADING, PRINTING & SAVING THE Solution ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for printing the Solution
 */

 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected virtual method print(). This way operator<<() is
  * defined for each Solution, but its behavior can be customized by derived
  * classes. */

 friend std::ostream& operator<<( std::ostream& out , const Solution &s ) {
  s.print( out );
  return( out );
  }

/*@}------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

  protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED TYPES ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
    @{ */

 /// print information about the Solution on an ostream
 /** Protected method intended to print information about the Solution; it is
  * virtual so that derived classes can print their specific information in
  * the format they choose. */

 virtual void print( std::ostream &output ) const {
  output << "Solution [" << this << "]";
  }

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

// private:

/*--------------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

 };  // end( class( Solution ) )

/*@}  end( group( Solution_CLASSES ) ) -------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* Solution.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Solution.h ---------------------------*/
/*--------------------------------------------------------------------------*/
