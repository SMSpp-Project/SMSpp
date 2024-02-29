/*--------------------------------------------------------------------------*/
/*--------------------------- File Objective.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the Objective class, a quite general base class of all the
 * possible type of objective functions that a Block can support. Very few
 * assumptions are made about what form the function actually has, this being
 * demanded to derived classes. Since Objective can be costly to compute
 * (think multiple integrals to compute expectations or min-max functions
 * requiring the solution of a hard optimization problem), the class
 * implements the ThinComputeInterface paradigm. Also, since an Objective
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

#ifndef __Objective
 #define __Objective  /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ThinComputeInterface.h"

#include "ThinVarDepInterface.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{
/*--------------------------------------------------------------------------*/
/*---------------------- Objective-RELATED TYPES ---------------------------*/
/*--------------------------------------------------------------------------*/

class Block;       // forward definition
class Variable;    // forward definition

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Objective_CLASSES Classes in Objective.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS Objective ------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// base class of all possible objective functions that a Block can support
/** The class Objective is the base class of all the possible type of
 * objective that the Block can support. This base class only supports a few
 * fundamental facts:
 *
 * - an Objective belong to one Block;
 *
 * - an Objective is influenced by a set of "active" Variables, i.e.,
 *   these which contribute to it setting its vaule: hence, the class
 *   implements the ThinVarDepInterface paradigm (i.e., derives from
 *   ThinVarDepInterface) that class makes no provisions about how this set
 *   is stored in order to leave more freedom to derived classes to implement
 *   it in specialized ways;
 *
 * - an Objective has a "sense", say, it can either be minimized or maximized;
 *
 * - an Objective has to be computed, and this can be costly (think multiple
 *   integrals for expectation estimates or min-max functions requiring the
 *   solution of a hard optimization problem), which is why the class
 *   implements the ThinComputeInterface paradigm (i.e., derives from
 *   ThinComputeInterface).
 *
 * Note that obviously an Objective has a value, which might expected to be a
 * single real value. However, this is not defined in this base class (e.g.,
 * in order to support multi-objective problems); therefore, the methods for
 * reading it must be demanded to derived classes.
 *
 * A fundamental design decision in SMS++ is that THE "NAME" OF AN Objective
 * IS ITS MEMORY ADDRESS. This means that MOVING AN Objective IS NOT POSSIBLE:
 * copying an Objective to a different memory location makes a distinct
 * Objective. */

class Objective : public ThinComputeInterface , public ThinVarDepInterface
{
/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */

 /// define the possible type of optimization (min, max)
 /** Public enum for describing the different "senses" that a (real-valued)
  * Objective can have: eMin = minimization (lower values are better), and
  * eMax = maximization (higher values are better). The catch is that this
  * only works for Objective whose return value has a complete ordering,
  * which is the case e.g. for the all-important real-valued ones but not
  * for, say, multi-objective ones. Yet, single-objective real-valued
  * optimization is so important as to make it worth to define this in the
  * base Objective class. Also, Objective supports ways (see set_sense()
  * and get_sense()) to easily "extend" the set of supported senses. Finally,
  * the eUndef value is provided to indicate that the sense is not (yet)
  * defined. */
 enum of_type {
  eMin   = -1 ,  ///< minimize the objective (real-valued function)
  eUndef =  0 ,  ///< undefined 
  eMax   =  1    ///< maximize the objective (real-valued function)
  };

/** @} ---------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

 /// constructor of Objective, taking the Block it belongs to
 /** Constructor of Objective. It accepts a pointer to the Block to which the
  * Objective belongs, defaulting to nullptr so that this can be used as the
  * void constructor. If nullptr is passed, then set_Block() [see below] will
  * have to be used later to initialize it. */

 explicit Objective( Block * my_block = nullptr ) : ThinComputeInterface() ,
                                                    ThinVarDepInterface() {
  set_Block( my_block );
  f_sense = eMin;
  }

/*--------------------------------------------------------------------------*/
 /// copy constructor: it cannot be used, but it is not deleted
 /** The copy constructor of Objective is not currently implemented, but it
  * cannot be deleted because it is required to resize() empty vectors of
  * :Objective. */

 Objective( const Objective & )
  : ThinComputeInterface() , ThinVarDepInterface() {
  throw( std::logic_error( "copy constructor of Objective invoked" ) );
  }

/*--------------------------------------------------------------------------*/
 /// destructor of Objective: it is virtual, and empty
 ~Objective() override = default;

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the pointer to the Block to which the Objective belongs
 /** Method to set the pointer to the Block to which the Objective belongs.
  * If the pointer is not provided in the constructor, it should be called
  * before any other method of the class. */

 void set_Block( Block * fblock ) { f_Block = fblock; }

/*--------------------------------------------------------------------------*/
 ///< change the "sense" (min/max) of the problem
 /** Method to change the sense of the problem: it is virtual because
  * derived classes may want to do more (e.g. because they have different
  * types of problems). Indeed, note that while the enum of_type is provided
  * to encode the "classical" values (max/min), the field f_sense is of type
  * "int", and therefore so is the parameter of this method, in order to
  * allow derived classes to "extend" the set of possible types.
  *
  * The parameter issueMod decides if and how the ObjectiveMod is issued, as
  * described in Observer::make_par(). */

 virtual void set_sense( int new_sense , c_ModParam issueMod = eModBlck );

/** @} ---------------------------------------------------------------------*/
/*------------- METHODS FOR READING THE DATA OF THE Objective --------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the Objective
 *  @{ */

 /// returns the pointer of the Block to which the Objective belongs

 Block * get_Block( void ) const override { return( f_Block ); }

/*--------------------------------------------------------------------------*/
 /// returns the sense (min/max) of the Objective
 /** Returns the sense (min/max) of the Objective. Note that, while the enum
  * of_type is provided to encode the "classical" values (max/min), the field
  * f_sense is of type "int", and therefore so is the return value of this
  * method, in order to allow derived classes to "extend" the set of possible
  * types. */

 [[nodiscard]] virtual int get_sense( void ) const { return( f_sense ); }

/** @} ---------------------------------------------------------------------*/
/*------------- METHODS DESCRIBING THE BEHAVIOR OF AN Objective ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a Objective
 *
 *  Most of this interface derives directly from ThinComputeInterface and it
 *  is not changed in the base Constraint class; derived classes will have to
 *  do the actual implementation.
 *  @{ */

 /// compute (evaluate) the Objective
 /** Pure virtual method: it has to compute whatever expression/function the
  * Objective depends onto and store the results into some protected field.
  * Evaluating an Objective can be a lengthy task, involving e.g. expensive
  * function computations or numerical integrals, which is why Objective are
  * not computed by default, and need to be computed explicitly with this
  * method. This is also why this implements the compute() method of
  * ThinComputeInterface; see the comments on the base class, in particular
  * about the rules concerning what can change during a call to this method,
  * and between two calls depending on the value of the changedvars parameter.
  *
  * Note, however, that this method does not return the *value* of the
  * Objective, because the base class makes no provisions about what type
  * this actually is. */

 virtual int compute( bool changedvars = true ) override = 0;

/** @} ---------------------------------------------------------------------*/
/*---------- METHODS FOR LOADING, PRINTING & SAVING THE Objective ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for printing the Objective
 *
 * There is no extracting method (operator>>()) to load an Objective out of a
 * stream because an Objective is not supposed to build itself: rather, the
 * Block it belongs to has to construct it.
 *
 * @{ */

 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected virtual method print(). This way the
  * operator<<() is defined for each Objective, but its behavior can be
  * customized by derived classes. */

 friend std::ostream & operator<<( std::ostream & out, const Objective & o ) {
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

 /// print information about the Objective on an ostream
 /** Protected method intended to print information about the Objective; it
  * is virtual so that derived classes can print their specific information
  * in the format they choose. The level of the verbosity of the printed
  * information can be controlled looking at the appropriate information in
  * the Block to which this Objective belongs [see Block.h]. */

 virtual void print( std::ostream & output ) const {
  output << "Objective [" << this << "] of Block [" << f_Block
         << "] with " << get_num_active_var() << " active variables, to "
         << std::endl;
  if( f_sense == eMin )
   output << "minimize" << std::endl;
  else
   output << "maximize" << std::endl;
 }

/** @} ---------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 Block * f_Block{};  ///< reference to the Block where the Objective is defined

 int f_sense{};  ///< type of the problem
 /**< type of the problem. Note that while the enum of_type is
  * provided to encode the "classical" values (max/min), the
  * field f_sense is of type "int" in order to allow derived
  * classes to "extend" the set of possible types. */

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

 };  // end( class( Objective ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS RealObjective ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// an Objective that is a function between Variable and the reals
/** The class RealObjective quickly specializes the more general Objective to
 * the case where the objective function is a function between Variable (of
 * any possinle type) and the reals.
 * More specifically, a type "OFValue" is defined, which is bound by default
 * to doubles, to hold the type of the objective function return value.
 * Changing this type here is possible but it changes it to the whole SMS++
 * hierarchy, so this does not look too reasonable; if one really needs a
 * different return value than double can rather re-define a similar class to
 * this. */

class RealObjective : public Objective
{
/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */

 using OFValue = double;  ///< type of the returned value

/** @} ---------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
 /** @name Constructor and destructor
     @{ */

 /// constructor of RealObjective: takes a pointer to its Block
 /** Constructor of RealObjective: it does nothing but throw that of
  * the base Objective. */

 explicit RealObjective( Block * blck = nullptr ) : Objective( blck ) {}

/*--------------------------------------------------------------------------*/

 ~RealObjective() override = default;  ///< destructor, virtual and empty

/*--------------------------------------------------------------------------*/
/*------------ METHODS DESCRIBING THE BEHAVIOR OF A RealObjective ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a RealObjective
 *  @{ */

 /// returns the (real) value of the Objective
 /** Pure virtual method that returns the (real) value of the Objective given
  * the values of the Variables it depends onto. More specifically, the
  * method has to return the value that the Objective *had* at the last time
  * in which compute() has been called. It is an error to call this method if
  * compute() has never been called, although implementations are allowed not
  * to check this and return any random response: is the users'
  * responsibility to ensure that compute() has been called at the proper
  * time. */

 [[nodiscard]] virtual OFValue value( void ) const = 0;

/*--------------------------------------------------------------------------*/
 /// returns the "constant term" of the Objective
 /** Virtual method that returns "the constant term" of the Objective. Each
  * Objective can be seen to have a "constant term", i.e., to have the form
  *
  *    f( x ) = f_0 + h( x )
  *
  * with f_0 constant and h( x ) depending on x. Of course for a general
  * Objective one can choose f_0 arbitrarily by changing h(), but for many
  * Objective the "constant term" is clearly defined. The interest of the
  * constant term is that changing it does not really change the underlying
  * optimization problem, save for correspondingly changing its optimal
  * value; this is therefore a very mild way of changing the problem that
  * is good to know about.
  * This method provides access to "the constant term" f_0. Since this may
  * not really make sense for all Objective, a default implementation is
  * provided that just returns 0, a non-invasive constant term. Note that,
  * unlike get_value(), it is not required to call compute() prior to calling
  * this method as the returned value is not supposed to change. */

 [[nodiscard]] virtual OFValue get_constant_term( void ) const {
  return( 0 );
  }

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

 /// print the RealObjective
 void print( std::ostream & output ) const override {
  output << "RealObjective [" << this << "] of Block ["
         << f_Block << "] with " << get_num_active_var()
         << " active variables, to ";
  if( f_sense == eMin )
   output << "minimize;";
  else
   output << "maximize;";
  output << "current value = " << value();
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( RealObjective ) )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS ObjectiveMod ---------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modifications specific to a Objective
/** Derived class from AModification to describe modifications to an
 * Objective, i.e., changing the sense of the problem. */

class ObjectiveMod : public AModification
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/

 /// Definition of the possible type of Modification
 enum of_mod_type {
  eSetMin ,        ///< minimize the objective function
  eSetMax ,        ///< maximize the objective function
  eOFModLastParam  ///< first allowed parameter value for derived classes
  /**< convenience value for easily allow derived classes
   * to extend the set of types of modifications */
  };

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// Constructor: takes a Objective * and the type of Modification
 /** Cconstructor: takes a pointer to the affected Objective and the type of
  * the Modification. Note that while the enum of_mod_type is provided to
  * encode the possible values of modification, the field f_type is of type
  * "int", and therefore so is the parameter of the constructor, in order to
  * allow derived classes to "extend" the set of possible types of changes. */

 explicit ObjectiveMod( Objective * of , int mod = eSetMin ,
                        bool cB = true )
  : AModification( cB ) , f_of( of ) , f_type( mod ) {}

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~ObjectiveMod() override = default;  ///< destructor: does nothing

/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// returns the Block to which the Objective belongs

 [[nodiscard]] Block * get_Block() const override {
  return( f_of->get_Block() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to (the pointer to) the affected Objective

 [[nodiscard]] Objective * of( void ) const { return( f_of ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to type of Modificatiom

 [[nodiscard]] int type( void ) const { return( f_type ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the ObjectiveMod

 void print( std::ostream & output ) const override {
  output << "ObjectiveMod[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] of Objective [" << &f_of << " ] : changing sense to ";
  if( f_type == eSetMin )
   output << "minimization" << std::endl;
  else
   output << "maximization" << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Objective * f_of;   ///< pointer to the Objective where the change occurs

 int f_type;         ///< type of Modification

/*--------------------------------------------------------------------------*/

 };  // end( class( ObjectiveMod ) )

/** @} end( group( Objective_CLASSES ) ) */
/*--------------------------------------------------------------------------*/
/*---------------------- Objective-RELATED TYPES ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Objective_TYPES Objective-related types.
    @{ */

/*--------------------------------------------------------------------------*/

typedef Objective * p_ObjF;  ///< a pointer to Objective

/** @} end( group( Objective_TYPES ) ) -------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* Objective.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Objective.h ---------------------------*/
/*--------------------------------------------------------------------------*/
