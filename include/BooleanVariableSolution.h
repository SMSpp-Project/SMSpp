/*--------------------------------------------------------------------------*/
/*--------------------- File BooleanVariableSolution.h ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the BooleanVariableSolution class. A
 * BooleanVariableSolution represents a solution of a Block whose Variables
 * are all BooleanVariable [see BooleanVariable.h], storing the values of
 * the static and dynamic Variables of the Block as well as the
 * BooleanVariableSolution of the nested Blocks. It is the counterpart for
 * BooleanVariable of ColVariableSolution, whose structure it follows.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato, Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __BooleanVariableSolution
 #define __BooleanVariableSolution
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Solution.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

 class BooleanVariable;         ///< forward definition of BooleanVariable
 class BooleanVariableSolution; ///< forward definition of BooleanVariableSolution

/*--------------------------------------------------------------------------*/
/*------------------ BooleanVariableSolution-RELATED TYPES ---------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup BooleanVariableSolution_TYPES BooleanVariableSolution-related types
 *  @{ */

 typedef std::vector< BooleanVariableSolution > Vec_BooleanVariableSolution;
 ///< a vector of BooleanVariableSolution

 typedef const std::vector< BooleanVariableSolution > c_Vec_BooleanVariableSolution;
 ///< a const vector of BooleanVariableSolution

/** @}  end( group( BooleanVariableSolution_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup BooleanVariableSolution_CLASSES Classes in BooleanVariableSolution.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS BooleanVariableSolution -------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a solution (values of the Variables) of a Block of BooleanVariable
/** The BooleanVariableSolution class represents a solution of a Block whose
 * Variables are all BooleanVariable. As ColVariableSolution, it relies only
 * on the "abstract representation" of the Block, and so it can be used with
 * any Block whose Variables are BooleanVariable.
 *
 * The value of each BooleanVariable is stored as a double, 1 for true and 0
 * for false, so that scale() and sum() have the same meaning as for a
 * ColVariableSolution: a convex combination of solutions gives, for each
 * BooleanVariable, the weighted frequency with which it is true, which is
 * what a method working on a relaxation (say, the convexified primal
 * solution of a Lagrangian dual) needs. write() goes back to the Boolean
 * values by rounding, i.e., a BooleanVariable is set to true if and only if
 * its stored value is at least 1/2. */

class BooleanVariableSolution : public Solution {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

 /// constructor of BooleanVariableSolution, does nothing

 BooleanVariableSolution() : Solution() { }

/*--------------------------------------------------------------------------*/
 /// inhibit copy constructor, so that it cannot be used

 BooleanVariableSolution( const BooleanVariableSolution & ) : Solution() {
  throw( std::invalid_argument( "Trying to copy BooleanVariableSolution" ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual void deserialize( const netCDF::NcGroup & group ) override final;

/*--------------------------------------------------------------------------*/

 virtual ~BooleanVariableSolution();  ///< destructor

/** @} ---------------------------------------------------------------------*/
/*-------- METHODS DESCRIBING THE BEHAVIOR OF A BooleanVariableSolution --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a BooleanVariableSolution
 *  @{ */

 /// read the BooleanVariableSolution from the given Block
 /** This method reads the solution of the given Block and stores it in this
  * BooleanVariableSolution. For this method to be used, it is required that:
  *
  * 1) the abstract representation of the Variables of the Block has been
  *    generated; and
  *
  * 2) this BooleanVariableSolution has been initialized to represent a solution
  *    of the given Block. This should normally mean that this
  *    BooleanVariableSolution was obtained from a call to the method
  *    get_Solution() of the Block associated with this BooleanVariableSolution.
  */

 void read( const Block * const block ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// write the BooleanVariableSolution in the given Block
 /** This method writes the solution currently stored in this
  * BooleanVariableSolution on the given Block. For this method to be used, it
  * is required that:
  *
  * 1) the abstract representation of the Variables of the Block has been
  *    generated; and
  *
  * 2) this BooleanVariableSolution has been initialized to represent a solution
  *    of the given Block. This should normally mean that this
  *    BooleanVariableSolution was obtained from a call to the method
  *    get_Solution() of the Block associated with this BooleanVariableSolution.
  */

 void write( Block * const block ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// serialize a BooleanVariableSolution into a netCDF::NcGroup
 /** Serialize a BooleanVariableSolution into a netCDF::NcGroup, with the
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

 void serialize( netCDF::NcGroup & group ) const override final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns a scaled version of this BooleanVariableSolution
 /** This method constructs and returns a scaled version of this
  * BooleanVariableSolution. The new BooleanVariableSolution will have the
  * same structure of this BooleanVariableSolution. This means that the newly
  * created BooleanVariableSolution will be equal to this BooleanVariableSolution
  * except for the value of the Variables. For each Variable whose value "v"
  * is stored in this Solution, the newly created BooleanVariableSolution will
  * store the value "factor * v". */

 BooleanVariableSolution * scale( double factor ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// stores a scaled version of the given BooleanVariableSolution
 /** This method stores a scaled version of the BooleanVariableSolution provided as
  * argument into this Solution. This Solution is completely destroyed and
  * its structure is reconstructed to be the same as that of the given
  * Solution. This Solution will then be equal to the given Solution except
  * for the value of the Variables. For each Variable whose value "v" is
  * stored in the Solution provided as argument, this Solution will store the
  * value "factor * v". */

 virtual void scale( const BooleanVariableSolution * const solution ,
		     const double factor );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// adds a multiple of the given Solution to this Solution
 /** This method adds a multiple of the values of the Variables stored in the
  * Solution provided as argument to the values stored in this
  * BooleanVariableSolution.  The Solution provided as argument must have the same
  * structure as this BooleanVariableSolution, unless this BooleanVariableSolution is
  * empty, in which case this BooleanVariableSolution gets the same structure as
  * that of \p solution. If the value associated with a Variable is "v" in
  * this BooleanVariableSolution and "v2" in \p solution, then it will become "v +
  * multiplier * v2" in this BooleanVariableSolution. */

 void sum( const Solution * solution , double multiplier ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 BooleanVariableSolution * clone( bool empty = false ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the values of the *static* Variables
 /** Method for reading the values of the *static* Variables of the Block
  * associated with this Solution. The i-th entry of the returned vector
  * corresponds to the i-th group of static Variables of the Block, and holds
  * the values of its BooleanVariable in storage order: for a
  * boost::multi_array this is the order of its data(), and for a group whose
  * cells are std::vector it is cell by cell. The group may hold the
  * BooleanVariable or pointers to them. */

 const std::vector< std::vector< double > > &
 get_static_variable_values( void ) const {
  return( static_variable_values );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the values of the *dynamic* Variables
 /** Method for reading the values of the *dynamic* Variables of the Block
  * associated with this Solution. The i-th entry of the returned vector
  * corresponds to the i-th group of dynamic Variables of the Block, and has
  * one std::vector of double per cell of its grid, in storage order, holding
  * the values of the BooleanVariable of the std::list of that cell in list
  * order. The vector of a cell may be longer than the list, if the list was
  * longer when the values were read. */

 const std::vector< std::vector< std::vector< double > > > &
 get_dynamic_variable_values( void ) const {
  return( dynamic_variable_values );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the vector of inner sub-Solutions of this BooleanVariableSolution
 /** Method for reading the vector of inner sub-Solutions of this
  * Solution. */

 c_Vec_BooleanVariableSolution & get_nested_solutions( void ) const {
  return( nested_solutions );
  }

/** @} ---------------------------------------------------------------------*/
/*----- METHODS FOR LOADING, PRINTING & SAVING THE BooleanVariableSolution -----*/
/*--------------------------------------------------------------------------*/
/** @name Methods for printing the BooleanVariableSolution
 *  @{ */

 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected virtual method print(). This way the operator<<()
  * is defined for each BooleanVariableSolution, but its behavior can be
  * customized by derived classes. */

 friend std::ostream& operator<< ( std::ostream& out ,
				   const BooleanVariableSolution &o ) {
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

 /// reads (if read) or writes the values of the static Variables of block

 void apply_static( const Block * const block , bool read );

/*--------------------------------------------------------------------------*/
 /// reads (if read) or writes the values of the dynamic Variables of block

 void apply_dynamic( const Block * const block , bool read );

/*--------------------------------------------------------------------------*/

 /// initialize the solution from the given Block
 /** This method initializes this BooleanVariableSolution in order for it
  * to be ready to store a solution of that Block. If "read" is true,
  * the current solution of the Block is also read and stored into
  * this Solution. */

 void initialize( const Block * const block , bool read );

/*--------------------------------------------------------------------------*/

 void initialize_static_variable_values( const Block * const block ,
					 bool read );

/*--------------------------------------------------------------------------*/

 void initialize_dynamic_variable_values( const Block * const block ,
					  bool read );

/*--------------------------------------------------------------------------*/

 /// empties the structure of this Solution
 /** This method resizes to 0 the vectors static_variable_values,
  * dynamic_variable_values and nested_solutions. */

 void delete_vectors();

/*--------------------------------------------------------------------------*/

 /// returns true if and only if this BooleanVariableSolution is empty
 /** A BooleanVariableSolution is empty if and only if its structure is empty.
  * That is, it is empty if and only if the vectors returned by
  * get_static_variable_values(), get_dynamic_variable_values() and
  * get_nested_solutions() are all empty.
  *
  * @return true if and only if this BooleanVariableSolution is empty. */

 bool empty() const {
  return static_variable_values.empty() && dynamic_variable_values.empty() &&
   nested_solutions.empty();
  }

/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
 *  @{ */

 /// print information about the BooleanVariableSolution on an ostream
 /** Protected method intended to print information about the
  * BooleanVariableSolution; it is virtual so that derived classes can
  * print their specific information in the format they choose. */

 void print( std::ostream &output ) const override {
  output << "BooleanVariableSolution [" << this << "]";
  }

/** @} ---------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

 std::vector< std::vector< double > > static_variable_values;
 ///< the values of the static Variables, one vector per group

 std::vector< std::vector< std::vector< double > > > dynamic_variable_values;
 ///< the values of the dynamic Variables, one vector per cell per group

 Vec_BooleanVariableSolution nested_solutions;
 ///< vector of BooleanVariableSolutions of the nested Blocks

/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( BooleanVariableSolution ) )

/** @} end( group( BooleanVariableSolution_CLASSES ) ) -------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* BooleanVariableSolution.h included */

/*--------------------------------------------------------------------------*/
/*------------------- End File BooleanVariableSolution.h -------------------*/
/*--------------------------------------------------------------------------*/
