/*--------------------------------------------------------------------------*/
/*-------------------------- File Constraint.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the Constraint class, a quite general base class of all
 * the possible type of constraints that a Block can support. Very few
 * assumptions are made about what form the constraint actually has, this
 * being demanded to derived classes. Since Constraint can be costly to
 * compute (think multiple integrals for probability constraints or min-max
 * functions requiring the solution of a hard optimization problem), the class
 * implements the ThinComputeInterface paradigm. Also, since a Constraint
 * depends on a set of "active" Variable, it implements the
 * ThinVarDepInterface paradigm.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __Constraint
 #define __Constraint /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <list>
#include <vector>
#include <array>

#include <boost/multi_array.hpp>

#include "Modification.h"
#include "ThinComputeInterface.h"
#include "ThinVarDepInterface.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

class Block;       // forward definition
class Variable;    // forward definition
class Constraint;  // forward definition

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Constraint_CLASSES Classes in Constraint.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS Constraint ------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// base class of all possible type of constraints that a Block can support
/** The class Constraint is the base class of all the possible type of
 * constraints that the Block can support. This base class only supports a
 * few fundamental facts:
 *
 * - a Constraint belong to one Block;
 *
 * - a Constraint is influenced by a set of "active" Variables, i.e., these
 *   which contribute to it being satisfied (or not): hence, the class
 *   implements the ThinVarDepInterface paradigm (i.e., derives from
 *   ThinVarDepInterface) that class makes no provisions about how this set
 *   is stored in order to leave more freedom to derived classes to implement
 *   it in specialized ways;
 *
 * - a Constraint can be (temporarily?) relaxed, i.e., not really constraining
 *   the set of solutions of the Block;
 *
 * - a Constraint can be either satisfied or not, depending on the values
 *   that its "active" Variables currently have;
 *
 * - a Constraint has to be computed before it can be checked, and this can
 *   be costly (think multiple integrals for probability constraints or
 *   min-max functions requiring the solution of a hard optimization
 *   problem), which is why the class implements the ThinComputeInterface
 *   paradigm (i.e., derives from ThinComputeInterface).
 *
 * A fundamental design decision in SMS++ is that THE "NAME" OF A Constraint
 * IS ITS MEMORY ADDRESS. This means that MOVING A Constraint IS NOT
 * POSSIBLE: copying a Constraint to a different memory location makes a
 * distinct Constraint. */

class Constraint : public ThinComputeInterface , public ThinVarDepInterface
{
/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and Destructor
 *  @{ */

 /// constructor of Constraint, taking the Block it belongs to
 /** Constructor of Constraint. It accepts a pointer to the Block to which
  * the Constraint belongs, defaulting to nullptr so that this can be used as
  * the void constructor. If nullptr is passed, then set_Block() [see below]
  * will have to be used later to initialize it. */

 explicit Constraint( Block * my_block = nullptr ) : ThinComputeInterface() ,
                                                     ThinVarDepInterface() ,
                                                     f_Block( my_block ) ,
                                                     f_is_relaxed( false ) {}

/*--------------------------------------------------------------------------*/
 /// copy constructor: it cannot be used, but it is not deleted
 /** The copy constructor of Constraint is not currently implemented, but it
  * cannot be deleted because it is required to resize() empty vectors of
  * :Constraint. */

 Constraint( const Constraint & ) : ThinComputeInterface() ,
                                    ThinVarDepInterface() {
  throw( std::logic_error( "copy constructor of Constraint invoked" ) );
  }

/*--------------------------------------------------------------------------*/
 /// destructor of Constraint: it is virtual, and empty

 ~Constraint() override = default;

/*--------------------------------------------------------------------------*/
 /// clear a std::vector of Constraint
 template< typename T >
 static std::enable_if_t< std::is_base_of_v< Constraint , T > , void >
 clear( std::vector< T > & constraints ) {
  for( auto & constraint : constraints )
   constraint.clear();
  constraints.clear();
 }

/*--------------------------------------------------------------------------*/
 /// clear a K-D boost::multi_array of Constraint
 template< typename T , std::size_t K >
 static std::enable_if_t< std::is_base_of_v< Constraint , T > , void >
 clear( boost::multi_array< T , K > & constraints ) {
  auto constraint = constraints.data();
  auto n = constraints.num_elements();
  for( decltype( n ) i = 0 ; i < n ; ++i , ++constraint )
   constraint->clear();
  std::array< int , K > shape = {}; // for int, {} will zero-initialize
  constraints.resize( shape );
 }

/*--------------------------------------------------------------------------*/
 /// clear a std::list of Constraint
 template< typename T >
 static std::enable_if_t< std::is_base_of_v< Constraint , T > , void >
 clear( std::list< T > & constraints ) {
  for( auto & constraint : constraints )
   constraint.clear();
  constraints.clear();
 }

/*--------------------------------------------------------------------------*/
 /// clear a std::vector of std::list of Constraint
 template< typename T >
 static std::enable_if_t< std::is_base_of_v< Constraint , T > , void >
 clear( std::vector< std::list< T > > & constraints ) {
  for( auto & l_constraints : constraints )
   Constraint::clear( l_constraints );
  constraints.clear();
 }

/*--------------------------------------------------------------------------*/
 /// clear a K-D boost::multi_array of std::list of Constraint
 template< typename T , std::size_t K >
 static std::enable_if_t< std::is_base_of_v< Constraint , T > , void >
 clear( boost::multi_array< std::list< T > , K > & constraints ) {
  auto l_constraints = constraints.data();
  auto n = constraints.num_elements();
  for( decltype( n ) i = 0 ; i < n ; ++i , ++l_constraints )
   Constraint::clear( *l_constraints );
  std::array< int , K > shape = {}; // for int, {} will zero-initialize
  constraints.resize( shape );
 }

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the pointer to the Block to which the Constraint belongs
 /** Method to set the pointer to the Block to which the Constraint belongs.
  * If the pointer is not provided in the constructor, it should be called
  * before any other method of the class. */

 void set_Block( Block * fblock ) { f_Block = fblock; }

/*--------------------------------------------------------------------------*/
 /// method to relax or enforce the Constraint
 /** Method to relax or enforce the Constraint. If relax_it == true then the
  * Constraint is (temporarily) relaxed, i.e., no longer binding the set of
  * values that its Variables can take. If relax_it == false, the Constraint
  * is enforced again. The method is virtual because derived classes (actually
  * having a constraint to relax/enforce) may have to do more.
  *
  * The parameter issueMod decides if and how the ConstraintMod is issued, as
  * described in Observer::make_par(). */

 virtual void relax( bool relax_it, c_ModParam issueMod = eModBlck );

/** @} ---------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE DATA OF THE Constraint --------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the Constraint
    @{ */

 /// returns the pointer to the Block to which the Constraint belongs
 [[nodiscard]] Block * get_Block( void ) const override {
  return( f_Block );
  }

/** @} ---------------------------------------------------------------------*/
/*------------ METHODS DESCRIBING THE BEHAVIOR OF THE Constraint -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the Constraint
 *
 *  Most of this interface derives directly from ThinComputeInterface and it
 *  is not changed in the base Constraint class; derived classes will have to
 *  do the actual implementation.
 *  @{ */

 /// compute (evaluate) the Constraint
 /** Pure virtual method: it has to compute whatever expression/function the
  * Constraint depends onto and store the results into some protected field,
  * so that feasible() [see below] can then check whether or not the
  * Constraint is feasible (without changing it, which allows feasible() to
  * be const). Evaluating a Constraint can be a lengthy task, involving e.g.
  * expensive function computations or numerical integrals, which is why
  * Constraints are not evaluated by default, and need to be evaluated
  * explicitly with this method. This is also why this implements the
  * compute() method of ThinComputeInterface; see the comments on the base
  * class, in particular about the rules concerning what can change during a
  * call to this method, and between two calls depending on the value of the
  * changedvars parameter. */

 int compute( bool changedvars = true ) override = 0;

/*--------------------------------------------------------------------------*/
 /// returns true if Constraint is feasible
 /** Pure virtual method: it has to return true if Constraint is feasible
  * given the values of the Variables it depends onto. More specifically, the
  * method has to return true if the Constraint *was* feasible at the last time
  * in which compute() [see above] has been called. It is an error to call
  * this method if compute() has never been called, although implementations
  * are allowed not to check this and return any random response: it is the
  * users' responsibility to ensure that compute() has been called at the
  * proper time. */

 [[nodiscard]] virtual bool feasible( void ) const = 0;

/*--------------------------------------------------------------------------*/
 /// returns true if the Constraint is relaxed, i.e., not really a Constraint

 [[nodiscard]] bool is_relaxed( void ) const { return( f_is_relaxed ); }

/** @} ---------------------------------------------------------------------*/
/*---------- METHODS FOR LOADING, PRINTING & SAVING THE Constraint ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for printing the Constraint
 *
 * There is no extracting method (operator>>()) to load a constraint out of
 * a stream because a Constraint is not supposed to build itself: rather, the
 * Block it belongs to has to construct it. Saving the in-memory
 * representation of a Constraint is supported via the boost::serialization
 * approach.
 * @{ */

 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected virtual method print(). This way the
  * operator<<() is defined for each Constraint, but its behavior can be
  * customized by derived classes. */

 friend std::ostream & operator<<( std::ostream & out ,
                                   const Constraint & c ) {
  c.print( out );
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
 *  @{ */

 /// print information about the Constraint on an ostream
 /**< Protected method intended to print information about the Constraint; it
  * is virtual so that derived classes can print their specific information
  * in the format they choose.
  * The level of the verbosity of the printed information can be controlled
  * looking at the appropriate information in the Block to which this
  * Constraint belongs [see Block.h]. */

 virtual void print( std::ostream & output ) const {
  output << "Constraint [" << this << "] of Block [" << f_Block
         << "] with " << get_num_active_var() << " active variables"
         << std::endl;
 }

/** @} ---------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 Block * f_Block;   ///< pointer to the Block which the Constraint belongs to

 bool f_is_relaxed; ///< true if the Constraint is relaxed

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

// private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

};  // end( class( Constraint ) )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS ConstraintMod -----------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modifications specific to a Constraint
/** Derived class from AModification to describe modifications to a
 * Constraint, i.e., relaxing and enforcing it. */

class ConstraintMod : public AModification {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/
 /// Definition of the possible type of Modification
 enum cons_mod_type {
  eRelaxConst,       ///< relax the Constraint
  eEnforceConst,     ///< enforce the Constraint
  eConstModLastParam ///< first allowed parameter value for derived classes
  /**< convenience value for easily allow derived classes
    * to extend the set of types of modifications */
 };

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: takes the type of Modification and a Constraint pointer
 /** Constructor: takes the type of the Modification and a pointer to the
  * affected Constraint. Note that while the enum cons_mod_type is provided
  * to encode the possible values of modification, the field f_type is of
  * type "int", and therefore so is the parameter of the constructor, in
  * order to allow derived classes to "extend" the set of possible types of
  * modifications. */

 explicit ConstraintMod( Constraint * cnst , int mod = eRelaxConst,
                         bool cB = true )
  : AModification( cB ) , f_constraint( cnst ) , f_type( mod ) {}

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~ConstraintMod() override = default;  ///< destructor: does nothing

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// returns the Block to which the Constraint belongs

 [[nodiscard]] Block * get_Block( void ) const override {
  return( f_constraint->get_Block() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to (the pointer to) the affected Constraint

 [[nodiscard]] Constraint * constraint( void ) const {
  return( f_constraint );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to the type of Modification

 [[nodiscard]] int type( void ) const { return( f_type ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the ConstraintMod

 void print( std::ostream & output ) const override {
  output << "ConstraintMod[";
  if( concerns_Block() )
   output << "t]:";
  else
   output << "f]:";
  if( f_type == eRelaxConst )
   output << "relaxing";
  else
   output << "enforcing";

  output << " Constraint [" << f_constraint << "] " << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Constraint * f_constraint;   ///< pointer to the modified Constraint

 int f_type;                  ///< type of modification

/*--------------------------------------------------------------------------*/

 };  // end( class( ConstraintMod ) )

/** @} end( group( Constraint_CLASSES ) ) ----------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* Constraint.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File Constraint.h ---------------------------*/
/*--------------------------------------------------------------------------*/
