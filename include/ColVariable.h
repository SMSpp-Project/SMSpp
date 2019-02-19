/*--------------------------------------------------------------------------*/
/*-------------------------- File ColVariable.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the ColVariable class, derived from Variable, which is
 * intended as the base class for all the Variable that are single real
 * values, possibly restricted to some subset (e.g., the integers).
 *
 * \version 0.10
 *
 * \date 03 - 09 - 2018
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
 * \author Kostas Tavlaridis-Gyparakis \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ColVariable
 #define __ColVariable
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Variable.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup ColVariable_CLASSES Classes in ColVariable.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS ColVariable -----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a Variable that holds a single real value, possibly restricted to a subset
/** The ColVariable class, derived from Variable, is intended as the base
 * class for all the Variable that are single real values, possibly
 * restricted to some subset (e.g., the integers). In a linear program this
 * would correspond to a column in the coefficient matrix, whence the name.
 *
 * This class extends Variable to support the following further facts:
 *
 *  - A ColVariable has a value, which is a real number. More specifically, a
 *    type "VarValue" is defined, which is bound by default to doubles, to
 *    hold the type of the variable. Changing this type here is possible, but
 *    this changes it to the whole SMS++ hierarchy, so this does not look too
 *    reasonable; if one really needs a different return value than double
 *    can rather re-define a similar class to this.
 *
 *  - Other than being fiked, a ColVariable can be restricted to live into
 *    some "interesting" subsets of the reals, such as integers and binary
 *    values.
 *
 * - A trivial implementation of the set of "active stuff" for this
 *   ColVariable as an ordered set of pointers to ThinVarDepInterface.
 */

class ColVariable : public Variable {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
 /** @name Public Types
     @{ */

 /// Definition of the possible type of ColVariable
 enum col_var_type {
  kBinary = 0 ,   ///< can only have values 0 and 1 (but can have both)
  kInteger    ,   ///< can only have integer values
  kContinuous ,   ///< can have any real value
  ColVarLastType  ///< first allowed parameter value for derived classes
                  /**< Convenience value for easily allow derived classes
		   * to extend the set of types of real subsets */
  };

/*--------------------------------------------------------------------------*/

 typedef double VarValue;  ///< type of the value of the ColVariable
                           /**< type of the value of the ColVariable,
			    * and therefore also of the attached dual
			    * information (reduced cost). */

 typedef const VarValue c_VarValue;  ///< a const VarValue

/*@} -----------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

 /// constructor of ColVariable, taking the Block and the type
 /** Constructor of ColVariable. It takes the pointer to the Block to which
  * the ColVariable belongs and the "type" of the ColVariable. Everything
  * has a default (nullptr and kContinuous, respectively), so that this can
  * be used as the void constructor. Note that while the enum col_var_type is
  * provided to encode the possible "types" of the ColVariable, the
  * parameter of the constructor is a generic var_type in order to allow
  * further derived classes to further "extend" the set of possible types.
  *
  * The constructor sets the value of the ColVariable to its default. */

 ColVariable( Block *my_block = nullptr , const var_type type = kContinuous )
  : Variable( my_block ) {
  f_state |= type * 2;
  set_to_default_value();
  }

/*--------------------------------------------------------------------------*/
 /// copy constructor: the vector of active stuff will be empty

 ColVariable( const ColVariable & v ) : Variable( v ) {
  f_value = v.f_value;
  }

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor, virtual and empty
 /** Destructor of ColVariable. According to the guidelines set for the
  * destructor of any :Variable, it does *not* check the list of "stuff" this
  * ColVariable, but just destrois it (and the list with it).
  *
  * This would of course leave any "stuff" in which the Variable is active in
  * an inconsistent state, so care has to be exercised to ensure this does not
  * happen by, if necessary, scan the list and do the removal *before*
  * destroying the Variable. However, this also ties in with the
  * ThinVarDepInterface::clear() [see ThinVarDepInterface.h] method in that
  * the combination of that method and this assumption on the destructor of
  * Variable allows Variable and "stuff" (Constraint, Objective, Function)
  * to be destroyed without having to pointlessly update the data structures
  * linking them just before destruction. */

 virtual ~ColVariable() {}

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the value of the Variable (typically done by a Solver)
 /** Method to set the value of the Variable. Typically, a Solver [see
  * Solver.h] attached to the Block to which this ColVariable belongs will do
  * it. For more ease of mind, this method is virtual.
  *
  * Note that this is *not* a Modification-spewing method, and therefore it
  * does *not* have all the standard Modification-governing parameters.
  * Changing the value of [Col]Variable is arguably not a change in the data
  * of the problem, althouhg it could be a change of the data for a sub-Block
  * that does not directly own the Variable but for which the Variable is
  * active in some Constraint / Objective. Yet, this occurrence is not
  * reported by a Modification, and other mechanisms must be put in place to
  * (avoid) deal(ing) with it; see the discussion in ThinComputeInterface. */

 virtual void set_value( c_VarValue new_value ) { f_value = new_value; }

/*--------------------------------------------------------------------------*/
 /// sets the "type" of the ColVariable
 /** Sets the "type" of the ColVariable. This is encoded in the protected
  * field f_state that the base Variable classe uses to store the "state",
  * i.e., whether or not the [Col]Variable is fixed, so this mathod takes
  * great care to not mess up with the LSB of the field where that information
  * is stored.
  *
  * The parameter issueMod decides if and how the VariableMod is issued, as
  * described in Observer::make_par(). */

 virtual void set_type( const var_type type ,
			c_ModParam issueMod = eModBlck );

/*@} -----------------------------------------------------------------------*/
/*------------ METHODS DESCRIBING THE BEHAVIOR OF A ColVariable ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a ColVariable
 *  @{ */

 /// method to get the value of the ColVariable (a real, i.e., a VarValue)
 VarValue get_value( void ) const { return( f_value ); }

/*--------------------------------------------------------------------------*/

 /// method to get the type of the ColVariable
 /** Returns the "type" of the ColVariable, encoded accordingly to the enum
  * col_var_type. */

 var_type get_type( void ) const { return( f_state / 2 ); }

/*@} -----------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING ACTIVE "STUFF" ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" stuff
 *  @{ */

 virtual Index get_num_active( void ) const override {
  return( v_active.size() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual Index is_active( ThinVarDepInterface * stuff ) const override {
  auto idx = std::lower_bound( v_active.begin() , v_active.end() , stuff );

  if( idx < v_active.end() )
   return( std::distance( idx , v_active.begin() ) );
  else
   return( std::numeric_limits<Index>::infinity() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual ThinVarDepInterface * get_active( const Index i ) const override {
  return( v_active[ i ] );
  }

/*--------------------------------------------------------------------------*/
 /// returns (a reference to) the vector of pointers to active stuff
 /** Method that returns (a reference to) the vector of pointers to active
  * stuff, which is ordered in increasing sense (using as key the "name"
  * of the ThinVarDepInterface, i.e., the pointer itself). */

 const std::vector<ThinVarDepInterface *> & active_stuff( void ) const {
  return( v_active );
  }

/*--------------------------------------------------------------------------*/
 /// adds a pointer to the vector of (pointers to) active stuff
 /**< Method that adds a pointer to a new active "stuff" in the proper
  * vector, keeping it sorted. */

 void add_active( ThinVarDepInterface *stuff ) override {
   // find proper position in ascending order
  auto it = std::upper_bound( v_active.begin() , v_active.end() , stuff );

  v_active.insert( it , stuff );  // insert before it
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removes the given pointer from the vector of active stuff
 /**< Method that removes a pointer to an active "stuff" from the proper
  * vector, keeping it sorted; the input is the pointer to be removed. */

 void remove_active( ThinVarDepInterface * stuff ) override {
  // find proper position in ascending order
  auto it = std::find( v_active.begin() , v_active.end() , stuff );

  v_active.erase( it );  // now remove it
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removes the active stuff at the given position in the vector
 /** Method that removes a pointer to an active ThinVarDepInterface from the
  * proper vector, keeping it sorted: the input is the iterator */

 void remove_active( std::vector<ThinVarDepInterface *>::iterator it ) {
  v_active.erase( it );  // just remove it
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// sets the value of this ColVariable to its default value (0)
 /** The default value of a ColVariable is 0. If ColVariable with a different
  * default value is needed then one of the following approaches could be
  * used:
  *
  * - add a (static) field to this class that stores the default value for 
  *   a ColVariable;
  *
  * - create a class that derives from ColVariable and overrides this method.
  */

 virtual void set_to_default_value() override {
  this->set_value( VarValue( 0 ) );
  }

/*@}------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
 *  @{ */

 /// print the ColVariable

 virtual void print( std::ostream &output ) const override {
  output << "ColVariable [" << this << "] of Block [" << f_Block
	 << "] with " << get_num_active()
	 << " active stuff, value = " << f_value << std::endl;
  }

/*@}------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS -----------------------------*/
/*--------------------------------------------------------------------------*/

  VarValue f_value;     ///< value of the variable

  std::vector<ThinVarDepInterface *> v_active;  ///< set of active stiff

/*@}------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 };  // end( class( ColVariable ) )

/*@}  end( group( ColVariable_CLASSES ) ) ----------------------------------*/
/*--------------------------------------------------------------------------*/

 }  /* namespace SMSpp_di_unipi_it */

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* ColVariable.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File ColVariable.h ---------------------------*/
/*--------------------------------------------------------------------------*/
