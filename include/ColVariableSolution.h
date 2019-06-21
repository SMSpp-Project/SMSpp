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
 * Copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
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

 typedef std::vector<ColVariableSolution> Vec_ColVariableSolution;
 ///< a vector of ColVariableSolution

 typedef const std::vector<ColVariableSolution> c_Vec_ColVariableSolution;
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
 * static and dynamic Variables of a Block.
 */

class ColVariableSolution : public Solution { // Stores the values of the Variables of a Block

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

  /// constructor of ColVariableSolution
  /** Constructor of ColVariableSolution. */

  ColVariableSolution( ) : Solution() { }

/*--------------------------------------------------------------------------*/

  ColVariableSolution(const ColVariableSolution & ) : Solution() {
    throw std::invalid_argument( "Trying to copy ColVariableSolution" );
  }
  ///< copy constructor, so that it cannot be used
  /**< inhibit copy constructor */

/*--------------------------------------------------------------------------*/

  virtual ~ColVariableSolution();  ///< destructor

/**@} ----------------------------------------------------------------------*/
/*------------- METHODS DESCRIBING THE BEHAVIOR OF AN OBSERVER -------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a ColVariableSolution
 *  @{ */

  /// read the solution from the given Block
  /** This method reads the solution of the given Block and stores it in this
   * ColVariableSolution. For this method to be used, it is required that:
   *
   * 1) the abstract representation of the Variables of the Block have been
   * generated; and
   *
   * 2) this Solution has been initialized to represent a solution of the
   * given Block. This should normally mean that this Solution was obtained
   * from a call to the method get_solution() of the Block associated with
   * this Solution.
   */

  virtual void read( const Block * const block ) override;

  /// write the solution in the given Block
  /** This method writes the solution currently stored in this
   * ColVariableSolution on the given Block. For this method to be used, it
   * is required that:
   *
   * 1) the abstract representation of the Variables of the Block have been
   * generated; and
   *
   * 2) this Solution has been initialized to represent a solution of the
   * given Block. This should normally mean that this Solution was obtained
   * from a call to the method get_solution() of the Block associated with
   * this Solution.
   */

  virtual void write( Block * const block ) override;

  /// returns a scaled version of this Solution
  /** This method constructs and returns a scaled version of this
   * Solution. The newly created Solution will have the same structure of
   * this Solution. This means that the newly created Solution will be equal
   * to this Solution except for the value of the Variables. For each
   * Variable whose value "v" is stored in this Solution, the newly created
   * Solution will store the value "factor * v". */

  virtual ColVariableSolution * scale( double factor ) const override;

  /// stores a scaled version of the given Solution
  /** This method stores a scaled version of the Solution provided as
   * argument into this Solution. This Solution is completely destroyed and
   * its structure is reconstructed to be the same as that of the given
   * Solution. This Solution will then be equal to the given Solution except
   * for the value of the Variables. For each Variable whose value "v" is
   * stored in the Solution provided as argument, this Solution will store the
   * value "factor * v". */

  virtual void scale( const ColVariableSolution * const solution ,
                      const double factor );

  /// adds a multiple of the given Solution to this Solution
  /** This method adds a multiple of the values of the Variables stored in
   * the Solution provided as argument to the values stored in this
   * Solution. The Solution provided as argument must have the same structure
   * as this Solution. If the value associated with a Variable is "v" in this
   * Solution and "v2" in the Solution provided as argument, then it will
   * become "v + multiplier * v2" in this Solution. */

  virtual void sum( const Solution * solution , double multiplier ) override;

  virtual ColVariableSolution * clone( bool empty = false ) const override;

  /// returns the values of the *static* Variables
  /** Method for reading the values of the *static* Variables of the Block
   * associated with this Solution. It returns a vector of boost::any, each
   * element of which is supposed to contain only one among:
   *
   * - a pointer to a single double;
   *
   * - a pointer to a std::vector of double;
   *
   * - a pointer to a boost::multi_array<double , K>;
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
   * - a pointer to a boost::multi_array<V , K> or a pointer to a
   *   boost::multi_array<V *, K>, where V is any class derived from
   *   ColVariable, then the i-th element of this vector is a pointer to a
   *   boost::multi_array<double , K>, which stores the values of those
   *   ColVariables. */

  c_Vec_any & get_static_variable_values( void ) const {
    return ( static_variable_values );
  }

  /// returns the values of the *dynamic* Variables
  /** Method for reading the values of the *dynamic* Variables of the Block
   * associated with this Solution. It returns a vector of boost::any, each
   * element of which is supposed to contain only one among:
   *
   * - a pointer to a std::vector<double>;
   *
   * - a pointer to a std::vector< std::vector<double> >;
   *
   * - a pointer to a boost::multi_array<std::vector<double> , K>;
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
   * - a pointer to a std::vector of std::list<V> or a pointer to a
   *   std::vector of std::list<V *>, where V is any class derived from
   *   ColVariable, then the i-th element of this vector is a pointer to a
   *   std::vector of std::vector<double> which are the values of those
   *   ColVariables;
   *
   * - a pointer to a boost::multi_array<std::list<V> , K> or a pointer to a
   *   boost::multi_array<std::list<V *>, K>, where V is any class derived
   *   from ColVariable, then the i-th element of this vector is a pointer to
   *   a boost::multi_array<std::vector<double> , K>, which stores the values
   *   of those ColVariables. */

  c_Vec_any & get_dynamic_variable_values( void ) const {
    return ( dynamic_variable_values );
  }

 /// returns the vector of inner sub-Solutions of this ColVariableSolution
 /** Method for reading the vector of inner sub-Solutions of this
  * Solution. */

  c_Vec_ColVariableSolution & get_nested_solutions( void ) const {
    return nested_solutions;
  }

/**@} ----------------------------------------------------------------------*/
/*----- METHODS FOR LOADING, PRINTING & SAVING THE ColVariableSolution -----*/
/*--------------------------------------------------------------------------*/
/** @name Methods for printing the ColVariableSolution
 *  @{ */

  /// friend operator<<(), dispatching to virtual protected print()
  /** Not really a method, but a friend operator<<() that just
   * dispatches the ostream to the protected virtual method
   * print(). This way the operator<<() is defined for each
   * ColVariableSolution, but its behavior can be customized by derived
   * classes. */

  friend std::ostream& operator<< ( std::ostream& out ,
                                    const ColVariableSolution &o ) {
    o.print( out );
    return( out );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

  protected:

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/

  template<class F1 , class F2>
  void apply_static( const Block * const block , F1 f1 , F2 f2 );

  template<class F1 , class F2>
  void apply_dynamic( const Block * const block , F1 f1 , F2 f2 );

  /// initialize the solution from the given Block
  /** This method initializes this ColVariableSolution in order for it
   * to be ready to store a solution of that Block. If "read" is true,
   * the current solution of the Block is also read and stored into
   * this Solution. */

  void initialize( const Block * const block , bool read );

  void initialize_static_variable_values( const Block * const block ,
                                          bool read );

  void initialize_dynamic_variable_values( const Block * const block ,
                                           bool read );

  /// delete all vectors created for this Solution
  /** This method deletes every object currently "stored" in the
   * vectors static_variable_values and
   * dynamic_variable_values. Moreover, these two vectors and the
   * vector nested_solutions of nested Solutions are resized to 0. */

  void delete_vectors();

/** @name Protected methods for printing and serializing
    @{ */

  /// print information about the ColVariableSolution on an ostream
  /** Protected method intended to print information about the
   * ColVariableSolution; it is virtual so that derived classes can
   * print their specific information in the format they choose. */

  virtual void print( std::ostream &output ) const override {
    output << "ColVariableSolution [" << this << "]";
  }

/**@} ----------------------------------------------------------------------*/
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
