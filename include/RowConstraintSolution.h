/*--------------------------------------------------------------------------*/
/*---------------------- File RowConstraintSolution.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the RowConstraintSolution class. A RowConstraintSolution
 * represents a dual solution of a Block whose Constraint are all
 * RowConstraint. Usually, this RowConstraintSolution is constructed by the
 * Block whose solution it represents. A RowConstraintSolution stores the dual
 * values of the static and dynamic RowConstraint of a Block as well as the
 * RowConstraintSolutions of the nested Blocks.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __RowConstraintSolution
#define __RowConstraintSolution
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "RowConstraint.h"
#include "Solution.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

 class RowConstraintSolution; ///< forward definition of RowConstraintSolution

/*--------------------------------------------------------------------------*/
/*----------------- RowConstraintSolution-RELATED TYPES --------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup RowConstraintSolution_TYPES RowConstraintSolution-related types
 *  @{ */

 using Vec_RowConstraintSolution = std::vector< RowConstraintSolution >;
 ///< a vector of RowConstraintSolution

 using c_Vec_RowConstraintSolution = const std::vector< RowConstraintSolution >;
 ///< a const vector of RowConstraintSolution

/** @}  end( group( RowConstraintSolution_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup RowConstraintSolution_CLASSES Classes in RowConstraintSolution.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS RowConstraintSolution ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a solution (dual values of the RowConstraint) of a Block
/** The RowConstraintSolution class represents a dual solution of a Block
 * whose Constraint are all RowConstraint. It is able to store the dual values
 * of the static and dynamic RowConstraint of a Block. The main characteristic
 * of RowConstraintSolution is that of entirely relying on the "abstract
 * representation" of the Block to work, which means that it can in principle
 * be used with any Block, and in particular with those that only have the
 * "abstract representation", like AbstractBlock.
 *
 * Conversely, this may not necessarily play well with Block that rely on
 * a "physical representation" of the solution information, since the
 * information saved here may not be enough to fully reconstruct it
 * (although it well may). */

class RowConstraintSolution : public Solution {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

  /// constructor of RowConstraintSolution, does nothing

 RowConstraintSolution() : Solution() { }

/*--------------------------------------------------------------------------*/

 RowConstraintSolution( const RowConstraintSolution & ) : Solution() {
  throw( std::invalid_argument( "Trying to copy RowConstraintSolution" ) );
 }
 ///< copy constructor, so that it cannot be used
 /**< inhibit copy constructor */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual void deserialize( const netCDF::NcGroup & group ) override final;

/*--------------------------------------------------------------------------*/

 virtual ~RowConstraintSolution();  ///< destructor

/** @} ---------------------------------------------------------------------*/
/*------- METHODS DESCRIBING THE BEHAVIOR OF A RowConstraintSolution -------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a RowConstraintSolution
 *  @{ */

 /// read the RowConstraintSolution from the given Block
 /** This method reads the dual solution of the given Block and stores it in
  * this RowConstraintSolution. For this method to be used, it is required
  * that:
  *
  * 1) the abstract representation of the RowConstraint of the Block has been
  *    generated; and
  *
  * 2) this RowConstraintSolution has been initialized to represent a dual
  *    solution of the given Block. This should normally mean that this
  *    RowConstraintSolution was obtained from a call to the method
  *    get_Solution() of the Block associated with this RowConstraintSolution.
  */

 virtual void read( const Block * const block ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// write the RowConstraintSolution in the given Block
 /** This method writes the solution currently stored in this
  * RowConstraintSolution into the given Block. For this method to be used, it
  * is required that:
  *
  * 1) the abstract representation of the RowConstraint of the Block has been
  *    generated; and
  *
  * 2) this RowConstraintSolution has been initialized to represent a solution
  *    of the given Block. This should normally mean that this
  *    RowConstraintSolution was obtained from a call to the method
  *    get_Solution() of the Block associated with this RowConstraintSolution.
  */

 virtual void write( Block * const block ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// serialize a RowConstraintSolution into a netCDF::NcGroup
 /** Serialize a RowConstraintSolution into a netCDF::NcGroup, with the
  * following format:
  *
  * - The dimension "xxx" containing ... The dimension
  *   is optional, if it is not specified then the corresponding variable
  *   "yyy" is not read
  *
  *
  * - The variable "yyy", of type double and indexed over the
  *   dimension xxx. The variable is optional, if it is not specified
  *   then ...
  */

 virtual void serialize( netCDF::NcGroup & group ) const override final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns a scaled version of this RowConstraintSolution
 /** This method constructs and returns a scaled version of this
  * RowConstraintSolution. The new RowConstraintSolution will have the same
  * structure of this RowConstraintSolution. This means that the newly created
  * RowConstraintSolution will be equal to this RowConstraintSolution except
  * for the dual value of the RowConstraint. For each RowConstraint whose dual
  * value "v" is stored in this RowConstraintSolution, the newly created
  * RowConstraintSolution will store the value "factor * v". */

 virtual RowConstraintSolution * scale( double factor ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// stores a scaled version of the given RowConstraintSolution
 /** This method stores a scaled version of the RowConstraintSolution provided
  * as argument into this RowConstraintSolution. This RowConstraintSolution is
  * completely destroyed and its structure is reconstructed to be the same as
  * that of the given RowConstraintSolution. This RowConstraintSolution will
  * then be equal to the given RowConstraintSolution except for the dual
  * values of the RowConstraint. For each RowConstraint whose dual value "v"
  * is stored in the RowConstraintSolution provided as argument, this
  * RowConstraintSolution will store the value "factor * v". */

 virtual void scale( const RowConstraintSolution * const solution ,
                     const double factor );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// adds a multiple of the given Solution to this Solution
 /** This method adds a multiple of the dual values of the RowConstraint
  * stored in the Solution provided as argument to the values stored in this
  * Solution. The Solution provided as argument must have the same structure
  * as this Solution. If the dual value associated with a RowConstraint is "v"
  * in this Solution and "v2" in the Solution provided as argument, then it
  * will become "v + multiplier * v2" in this Solution. */

 virtual void sum( const Solution * solution , double multiplier ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual RowConstraintSolution * clone( bool empty = false ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the dual values of the *static* Constraint
 /** Method for reading the dual values of the *static* Constraint of the
  * Block associated with this Solution. It returns a vector of boost::any, in
  * which each element is supposed to contain only one among:
  *
  * - a pointer to a single double;
  *
  * - a pointer to a std::vector of double;
  *
  * - a pointer to a boost::multi_array< double , K >;
  *
  * This vector of boost::any is structured in the same way the vector
  * v_s_Constraint of static Constraint is structured in the associated
  * Block. This means that the i-th element of this vector is associated with
  * the i-th element of v_s_Constraint. If the i-th element of v_s_Constraint
  * is
  *
  * - a pointer to a single RowConstraint, then the i-th element of this
  *   vector is a pointer to a single double which is the dual value of that
  *   RowConstraint;
  *
  * - a pointer to a std::vector of any class derived from RowConstraint or a
  *   pointer to a std::vector of pointers to RowConstraint, then the i-th
  *   element of this vector is a pointer to a std::vector of double which are
  *   the dual values of those RowConstraint;
  *
  * - a pointer to a boost::multi_array< V , K > or a pointer to a
  *   boost::multi_array< V * , K >, where V is any class derived from
  *   RowConstraint, then the i-th element of this vector is a pointer to a
  *   boost::multi_array< double , K >, which stores the dual values of those
  *   RowConstraint. */

 c_Vec_any & get_static_constraint_dual_values( void ) const {
  return( static_constraint_dual_values );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the dual values of the *dynamic* Constraint
 /** Method for reading the dual values of the *dynamic* Constraint of the
  * Block associated with this Solution. It returns a vector of boost::any,
  * each element of which is supposed to contain only one among:
  *
  * - a pointer to a std::vector< double >;
  *
  * - a pointer to a std::vector< std::vector< double > >;
  *
  * - a pointer to a boost::multi_array< std::vector< double > , K >;
  *
  * This vector of boost::any is structured in the same way the vector
  * v_d_Constraint of dynamic Constraint is structured in the associated
  * Block. This means that the i-th element of this vector is associated with
  * the i-th element of v_d_Constraint. If the i-th element of v_d_Constraint
  * is
  *
  * - a pointer to a std::list of any class derived from RowConstraint or a
  *   pointer to a std::list of pointers to RowConstraint, then the i-th
  *   element of this vector is a pointer to a std::vector of double which
  *   are the dual values of those RowConstraint;
  *
  * - a pointer to a std::vector of std::list< V > or a pointer to a std::vector
  *   of std::list< V * >, where V is any class derived from RowConstraint, then
  *   the i-th element of this vector is a pointer to a std::vector of
  *   std::vector< double > which are the dual values of those RowConstraint;
  *
  * - a pointer to a boost::multi_array< std::list< V > , K > or a pointer to a
  *   boost::multi_array< std::list< V * > , K >, where V is any class derived
  *   from RowConstraint, then the i-th element of this vector is a pointer to a
  *   boost::multi_array< std::vector< double > , K >, which stores the dual
  *   values of those RowConstraint. */

 c_Vec_any & get_dynamic_constraint_dual_values( void ) const {
  return( dynamic_constraint_dual_values );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the vector of inner sub-Solutions of this RowConstraintSolution
 /** Method for reading the vector of inner sub-Solutions of this
  * Solution. */

 c_Vec_RowConstraintSolution & get_nested_solutions( void ) const {
  return( nested_solutions );
  }

/** @} ---------------------------------------------------------------------*/
/*---- METHODS FOR LOADING, PRINTING & SAVING THE RowConstraintSolution ----*/
/*--------------------------------------------------------------------------*/
/** @name Methods for printing the RowConstraintSolution
 *  @{ */

 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected virtual method print(). This way the
  * operator<<() is defined for each RowConstraintSolution, but its behavior
  * can be customized by derived classes. */

 friend std::ostream& operator<< ( std::ostream& out ,
                                   const RowConstraintSolution &o ) {
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

 void apply_static( const Block * const block , const bool read );

/*--------------------------------------------------------------------------*/

 void apply_dynamic
 ( const Block * const block , const bool read ,
   const RowConstraint::RHSValue default_dual_value = 0 );

/*--------------------------------------------------------------------------*/

 /// initialize the solution from the given Block
 /** This method initializes this RowConstraintSolution in order for it
  * to be ready to store a solution of that Block. If "read" is true,
  * the current solution of the Block is also read and stored into
  * this Solution. */

 void initialize( const Block * const block , bool read );

/*--------------------------------------------------------------------------*/

 void initialize_static_constraint_dual_values( const Block * const block ,
                                                bool read );

/*--------------------------------------------------------------------------*/

 void initialize_dynamic_constraint_dual_values( const Block * const block ,
                                                 bool read );

/*--------------------------------------------------------------------------*/

 /// delete all vectors created for this Solution
 /** This method deletes every object currently "stored" in the vectors
  * static_constraint_dual_values and
  * dynamic_constraint_dual_values. Moreover, these two vectors and the vector
  * nested_solutions of nested Solutions are resized to 0. */

 void delete_vectors();

/*--------------------------------------------------------------------------*/

 /// returns true if and only if this RowConstraintSolution is empty
 /** A RowConstraintSolution is empty if and only if its structure is empty.
  * That is, it is empty if and only if the vectors returned by
  * get_static_constraint_dual_values(), get_dynamic_constraint_dual_values()
  * and get_nested_solutions() are all empty.
  *
  * @return true if and only if this RowConstraintSolution is empty. */

 bool empty() const {
  return( ( nested_solutions.empty() ) &&
          ( static_constraint_dual_values.empty() ) &&
          ( dynamic_constraint_dual_values.empty() ) );
  }

/*--------------------------------------------------------------------------*/

/** @name Protected methods for printing and serializing
    @{ */

 /// print information about the RowConstraintSolution on an ostream
 /** Protected method intended to print information about the
  * RowConstraintSolution; it is virtual so that derived classes can
  * print their specific information in the format they choose. */

 virtual void print( std::ostream &output ) const override {
  output << "RowConstraintSolution [" << this << "]";
  }

/** @} ---------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

 Vec_any static_constraint_dual_values;
 ///< the dual values of the static Constraint
 /**< vector of pointers to [multi/single dimensional arrays of]
  * [pointers to] [classes derived from] Constraint */

 Vec_any dynamic_constraint_dual_values;
 ///< the dual values of the dynamic Constraint
 /**< vector of pointers to [multi/single dimensional arrays of]
  * [pointers to] [classes derived from] RowConstraint */

 Vec_RowConstraintSolution nested_solutions;
 ///< vector of RowConstraintSolutions of the nested Blocks

/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

};  // end( class( RowConstraintSolution ) )

/** @} end( group( RowConstraintSolution_CLASSES ) ) -----------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* RowConstraintSolution.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File RowConstraintSolution.h --------------------*/
/*--------------------------------------------------------------------------*/
