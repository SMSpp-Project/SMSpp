/*--------------------------------------------------------------------------*/
/*----------------------- File ColVariableSolution.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the ColVariableSolution class. A ColVariableSolution
 * represents a solution of a Block whose Variables are all
 * ColVariables. Usually, this ColVariableSolution is constructed by the
 * Block whose solution it represents. A ColVariableSolution stores the
 * values of the static and dynamic Variables of a Block as well as the
 * ColVariableSolutions of the nested Blocks.
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

#ifndef __ColVariableSolution
#define __ColVariableSolution
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

 class ColVariable;         ///< forward definition of ColVariable
 class ColVariableSolution; ///< forward definition of ColVariableSolution

/*--------------------------------------------------------------------------*/
/*------------------ ColVariableSolution-RELATED TYPES ---------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup ColVariableSolution_TYPES ColVariableSolution-related types
 *  @{ */

 typedef std::vector< ColVariableSolution > Vec_ColVariableSolution;
 ///< a vector of ColVariableSolution

 typedef const std::vector< ColVariableSolution > c_Vec_ColVariableSolution;
 ///< a const vector of ColVariableSolution

/** @}  end( group( ColVariableSolution_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup ColVariableSolution_CLASSES Classes in ColVariableSolution.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS ColVariableSolution -------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a solution (values of the Variables) of a Block
/** The ColVariableSolution class represents a solution of a Block whose
 * Variables are all ColVariables. It is able to store the values of the
 * static and dynamic Variables of a Block. The main characteristic of
 * ColVariableSolution is that of entirely relying on the "abstract
 * representation" of the Block to work, which means that it can in
 * principle be used with any Block, and in particular with those that only
 * have the "abstract representation", like AbstractBlock.
 *
 * Conversely, this may not necessarily play well with Block that rely on
 * a "physical representation" of the solution information, since the
 * information saved here may not be enough to fully reconstruct it
 * (although it well may). */

class ColVariableSolution : public Solution {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

 /// constructor of ColVariableSolution, does nothing

 ColVariableSolution() : Solution() { }

/*--------------------------------------------------------------------------*/
 /// inhibit copy constructor, so that it cannot be used

 ColVariableSolution( const ColVariableSolution & ) : Solution() {
  throw( std::invalid_argument( "Trying to copy ColVariableSolution" ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual void deserialize( const netCDF::NcGroup & group ) override final;

/*--------------------------------------------------------------------------*/

 virtual ~ColVariableSolution();  ///< destructor

/** @} ---------------------------------------------------------------------*/
/*-------- METHODS DESCRIBING THE BEHAVIOR OF A ColVariableSolution --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a ColVariableSolution
 *  @{ */

 /// read the ColVariableSolution from the given Block
 /** This method reads the solution of the given Block and stores it in this
  * ColVariableSolution. For this method to be used, it is required that:
  *
  * 1) the abstract representation of the Variables of the Block has been
  *    generated; and
  *
  * 2) this ColVariableSolution has been initialized to represent a solution
  *    of the given Block. This should normally mean that this
  *    ColVariableSolution was obtained from a call to the method
  *    get_Solution() of the Block associated with this ColVariableSolution.
  */

 void read( const Block * const block ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// write the ColVariableSolution in the given Block
 /** This method writes the solution currently stored in this
  * ColVariableSolution on the given Block. For this method to be used, it
  * is required that:
  *
  * 1) the abstract representation of the Variables of the Block has been
  *    generated; and
  *
  * 2) this ColVariableSolution has been initialized to represent a solution
  *    of the given Block. This should normally mean that this
  *    ColVariableSolution was obtained from a call to the method
  *    get_Solution() of the Block associated with this ColVariableSolution.
  */

 void write( Block * const block ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// serialize a ColVariableSolution into a netCDF::NcGroup
 /** Serialize a ColVariableSolution into a netCDF::NcGroup, with the
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
 /// returns a scaled version of this ColVariableSolution
 /** This method constructs and returns a scaled version of this
  * ColVariableSolution. The new ColVariableSolution will have the
  * same structure of this ColVariableSolution. This means that the newly
  * created ColVariableSolution will be equal to this ColVariableSolution
  * except for the value of the Variables. For each Variable whose value "v"
  * is stored in this Solution, the newly created ColVariableSolution will
  * store the value "factor * v". */

 ColVariableSolution * scale( double factor ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// stores a scaled version of the given ColVariableSolution
 /** This method stores a scaled version of the ColVariableSolution provided as
  * argument into this Solution. This Solution is completely destroyed and
  * its structure is reconstructed to be the same as that of the given
  * Solution. This Solution will then be equal to the given Solution except
  * for the value of the Variables. For each Variable whose value "v" is
  * stored in the Solution provided as argument, this Solution will store the
  * value "factor * v". */

 virtual void scale( const ColVariableSolution * const solution ,
		     const double factor );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// adds a multiple of the given Solution to this Solution
 /** This method adds a multiple of the values of the Variables stored in the
  * Solution provided as argument to the values stored in this
  * ColVariableSolution.  The Solution provided as argument must have the same
  * structure as this ColVariableSolution, unless this ColVariableSolution is
  * empty, in which case this ColVariableSolution gets the same structure as
  * that of \p solution. If the value associated with a Variable is "v" in
  * this ColVariableSolution and "v2" in \p solution, then it will become "v +
  * multiplier * v2" in this ColVariableSolution. */

 void sum( const Solution * solution , double multiplier ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 ColVariableSolution * clone( bool empty = false ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the values of the *static* Variables
 /** Method for reading the values of the *static* Variables of the Block
  * associated with this Solution. It returns a vector of boost::any, each
  * element of which is supposed to contain only one among:
  *
  * - a pointer to a single double;
  *
  * - a pointer to a std::vector of double;
  *
  * - a pointer to a boost::multi_array< double , K >;
  *
  * This vector of boost::any is structured in the same way the vector
  * v_s_Variable of static Variables is structured in the associated
  * Block. This means that the i-th element of this vector is associated
  * with the i-th element of v_s_Variable. If the i-th element of
  * v_s_Variable is
  *
  * - a pointer to a single ColVariable, then the i-th element of this
  *   vector is a pointer to a single double which is the value of the
  *   ColVariable;
  *
  * - a pointer to a std::vector of any class derived from ColVariable or a
  *   pointer to a std::vector of pointers to ColVariable, then the i-th
  *   element of this vector is a pointer to a std::vector of double which
  *   are the values of those ColVariables;
  *
  * - a pointer to a boost::multi_array< V , K > or a pointer to a
  *   boost::multi_array< V * , K >, where V is any class derived from
  *   ColVariable, then the i-th element of this vector is a pointer to a
  *   boost::multi_array< double , K >, which stores the values of those
  *   ColVariables. */

 c_Vec_any & get_static_variable_values( void ) const {
  return( static_variable_values );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the values of the *dynamic* Variables
 /** Method for reading the values of the *dynamic* Variables of the Block
  * associated with this Solution. It returns a vector of boost::any, each
  * element of which is supposed to contain only one among:
  *
  * - a pointer to a std::vector< double >;
  *
  * - a pointer to a std::vector< std::vector< double > >;
  *
  * - a pointer to a boost::multi_array< std::vector< double > , K >;
  *
  * This vector of boost::any is structured in the same way the vector
  * v_d_Variable of dynamic Variables is structured in the associated
  * Block. This means that the i-th element of this vector is associated
  * with the i-th element of v_d_Variable. If the i-th element of
  * v_d_Variable is
  *
  * - a pointer to a std::list of any class derived from ColVariable or a
  *   pointer to a std::list of pointers to ColVariable, then the i-th
  *   element of this vector is a pointer to a std::vector of double which
  *   are the values of those ColVariables;
  *
  * - a pointer to a std::vector of std::list< V > or a pointer to a
  *   std::vector of std::list< V * >, where V is any class derived from
  *   ColVariable, then the i-th element of this vector is a pointer to a
  *   std::vector of std::vector< double > which are the values of those
  *   ColVariables;
  *
  * - a pointer to a boost::multi_array< std::list< V > , K > or a pointer to a
  *   boost::multi_array< std::list< V * > , K >, where V is any class derived
  *   from ColVariable, then the i-th element of this vector is a pointer to
  *   a boost::multi_array< std::vector< double > , K >, which stores the values
  *   of those ColVariables. */

 c_Vec_any & get_dynamic_variable_values( void ) const {
  return( dynamic_variable_values );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the vector of inner sub-Solutions of this ColVariableSolution
 /** Method for reading the vector of inner sub-Solutions of this
  * Solution. */

 c_Vec_ColVariableSolution & get_nested_solutions( void ) const {
  return( nested_solutions );
  }

/** @} ---------------------------------------------------------------------*/
/*----- METHODS FOR LOADING, PRINTING & SAVING THE ColVariableSolution -----*/
/*--------------------------------------------------------------------------*/
/** @name Methods for printing the ColVariableSolution
 *  @{ */

 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected virtual method print(). This way the operator<<()
  * is defined for each ColVariableSolution, but its behavior can be
  * customized by derived classes. */

 friend std::ostream& operator<< ( std::ostream& out ,
				   const ColVariableSolution &o ) {
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

 template< class F1 , class F2 >
 void apply_static( const Block * const block , F1 f1 , F2 f2 );

/*--------------------------------------------------------------------------*/

 template< class F1 , class F2 >
 void apply_dynamic( const Block * const block , F1 f1 , F2 f2 );

/*--------------------------------------------------------------------------*/

 /// initialize the solution from the given Block
 /** This method initializes this ColVariableSolution in order for it
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

 /// delete all vectors created for this Solution
 /** This method deletes every object currently "stored" in the vectors
  * static_variable_values and dynamic_variable_values. Moreover, these two
  * vectors and the vector nested_solutions of nested Solutions are resized
  * to 0. */

 void delete_vectors();

/*--------------------------------------------------------------------------*/

 /// returns true if and only if this ColVariableSolution is empty
 /** A ColVariableSolution is empty if and only if its structure is empty.
  * That is, it is empty if and only if the vectors returned by
  * get_static_variable_values(), get_dynamic_variable_values() and
  * get_nested_solutions() are all empty.
  *
  * @return true if and only if this ColVariableSolution is empty. */

 bool empty() const {
  return static_variable_values.empty() && dynamic_variable_values.empty() &&
   nested_solutions.empty();
  }

/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
 *  @{ */

 /// print information about the ColVariableSolution on an ostream
 /** Protected method intended to print information about the
  * ColVariableSolution; it is virtual so that derived classes can
  * print their specific information in the format they choose. */

 void print( std::ostream &output ) const override {
  output << "ColVariableSolution [" << this << "]";
  }

/** @} ---------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

 Vec_any static_variable_values; ///< the values of the static Variables
 /**< vector of pointers to [multi/single dimensional arrays of]
  * [pointers to] [classes derived from] Variable */

 Vec_any dynamic_variable_values; ///< the values of the dynamic Variables
 /**< vector of pointers to [multi/single dimensional arrays of]
  * [pointers to] [classes derived from] Variable */

 Vec_ColVariableSolution nested_solutions;
 ///< vector of ColVariableSolutions of the nested Blocks

/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( ColVariableSolution ) )

/** @} end( group( ColVariableSolution_CLASSES ) ) -------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* ColVariableSolution.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File ColVariableSolution.h ---------------------*/
/*--------------------------------------------------------------------------*/
