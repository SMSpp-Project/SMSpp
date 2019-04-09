/*--------------------------------------------------------------------------*/
/*-------------------------- File ColVariable.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the ColVariable class, derived from Variable, which is
 * intended as the base class for all the Variable that are single real
 * values, possibly restricted to some subset (e.g., the integers).
 *
 * \version 0.30
 *
 * \date 21 - 03 - 2019
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
#include <cmath>

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
/** The ColVariable class, derived from Variable, is intended as the simple
 * version of "Variable whose value is a single real, possibly restricted to
 * some "interesting" subset (e.g., the integers)". In a Linear Program this
 * would correspond to a column in the coefficient matrix, whence the name.
 *
 * This class extends Variable to support the following further facts:
 *
 *  - A ColVariable has a value, which is a real number. More specifically, a
 *    type "VarValue" is defined, which is bound by default to doubles, to
 *    hold the type of the variable. Changing this type here is possible, but
 *    this changes it to the whole of SMS++ that uses ColVariable; if
 *    real-valued :Variable with different precisions than double are
 *    required, one should rather re-define a specific similar class to this.
 *
 *  - Other than being fiked, a ColVariable can be restricted to live into
 *    some "interesting subsets of the reals". This is mainly used to impose
 *    integrality restrictions: each subset has both an "integral" and a
 *    "continuous" variant. However, also sign constraints and "unitary"
 *    constraints (the absolute value of the value of the ColVariable must
 *    be not larger than 1) can be imposed, yielding 13 different subsets
 *    (formally 16, but 4 only allow the value 0) which comprise many of
 *    the bound/sign/integrality constraints found in practical models, most
 *    notably that of binary values (the value being either 0 or 1).
 *
 * - A trivial implementation of the set of "active stuff" for this
 *   ColVariable as an ordered set of pointers to ThinVarDepInterface.
 *
 * Important note: the "type" of the ColVariable is in fact a constraint on
 * its possible values; as such, it being a part of the ColVariable, rather
 * being specified by something that derives from Constraint, is a bit on the
 * slippery side. In particular, several (but not all) types imply finite
 * upper and/or lower bounds on the value. In some cases, such as in Linear
 * (Convex) Programs, these bounds may be active at an optimal solution and
 * have non-zero dual variables (often called the "reduced cost" of the
 * variable). However, there is no place in the ColVariable to store this
 * information. Yet, it is usually possible to reconstruct this the reduced
 * cost out of the rest of the dual solution if necessary. Also, very many
 * variables in practical models only have these very special (bound)
 * constraints imposed on them, which means that it is possible to save a
 * significant amount of memory by not having to implement them through an
 * explicit :Constraint object. */

class ColVariable : public Variable {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
 /** @name Public Types
  *  @{ */

 /// Definition of the possible type of ColVariable
 /** The enum col_var_type defines all possible "types" that the ColVariable
  * can have, i.e., which subset of the reals (possibly, the whole set) its
  * value is restricted to belong to. There are 16 possible types of
  * ColVariable, according to all possible combinations of 4 independent
  * conditions:
  *
  * - the ColVariable must be integer-valued;
  *
  * - the ColVariable must be non-negative;
  *
  * - the ColVariable must be non-positive;
  *
  * - the maximum absolute value ColVariable must be 1.
  *
  * These give the possibility to specify many "interesting subsets of the
  * reals", such as binary variables (those that can only attain either value
  * 0 or value 1). Very many variables in practical models fit in one of
  * these cases. In order to allow all possible combinations some "weird"
  * cases are also comprised, but likely the "type" of the ColVariable is
  * read/manipulated with the methods allowing to check/set the individual
  * properties rather than looking at these enums, so it does not matter
  * much. */

 enum col_var_type {
  kContinuous  =  0 ,  ///< any real value
  kInteger     =  1 ,  ///< any integer value
  kNonNegative =  2 ,  ///< any non-negative real value
  kNatural     =  3 ,  ///< any non-negative integer (natural)
  kNonPositive =  4 ,  ///< any non-positive real value
  kNegative    =  5 ,  ///< any non-positive integer value
  kZeroReal    =  6 ,  ///< any real value provided it is 0
  kZeroInteger =  7 ,  ///< any integer value provided it is 0
  kUnitary     =  8 ,  ///< any real value between -1 and 1
  kTernary     =  9 ,  ///< either -1, or 0, or 1
  kPosUnitary  = 10 ,  ///< any real value between 0 and 1
  kBinary      = 11 ,  ///< either 0 or 1
  kNegUnitary  = 12 ,  ///< any real value between -1 and 0
  kNegBinary   = 13 ,  ///< either -1 or 0
  kZeroRealU   = 14 ,  ///< any real value between -1 and 1 provided it is 0
  kZeroIntU    = 15 ,  ///< any int value between -1 and 1 provided it is 0
  ColVarLastType       ///< first allowed parameter value for derived classes
                       /**< Convenience value for easily allow derived classes
			* to extend the set of types of real subsets. */
  };

/*--------------------------------------------------------------------------*/

 typedef double VarValue;            ///< type of the value of the ColVariable

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
  f_state &= var_type( 1 );
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

 virtual void set_value( c_VarValue new_value = 0 ) { f_value = new_value; }

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

/*--------------------------------------------------------------------------*/
 /// method to set whether the ColVariable is integer-valued

 virtual void is_integer( const bool yn , c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to set whether the ColVariable is non-negative

 virtual void is_positive( const bool yn , c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to set whether the ColVariable is non-positive

 virtual void is_negative( const bool yn , c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to set whether the max absolute value of the ColVariable is 1

 virtual void is_unitary( const bool yn , c_ModParam issueMod = eModBlck );

/*@} -----------------------------------------------------------------------*/
/*------------ METHODS DESCRIBING THE BEHAVIOR OF A ColVariable ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a ColVariable
 *  @{ */

 /// method to get the value of the ColVariable (a real, i.e., a VarValue)
 VarValue get_value( void ) const { return( f_value ); }

/*--------------------------------------------------------------------------*/
 /// method to check feasibility of a ColVariable
 /** The current value of a ColVariable may or may not be "compatible" with
  * the current "type" of the ColVariable; this method returns true if it is.
  * The eps parameter gives the maximum deviation from the "ideal" value;
  * that is, a non-negative ColVariable is feasible if its value is >= - eps,
  * and so on. */

 bool is_feasible( const var_type eps = 0 ) const {
  if( is_integer() && ( std::abs( std::round( f_value ) - f_value ) > eps ) )
   return( false );

  if( is_positive() && ( f_value < - eps ) )
   return( false );
		       
  if( is_negative() && ( f_value > eps ) )
   return( false );

  if( is_unitary() && ( std::abs( f_value ) - 1 > eps ) )
   return( false );

  return( true );
  }

/*--------------------------------------------------------------------------*/
 /// method to get the type of the ColVariable
 /** Returns the "type" of the ColVariable, encoded accordingly to the enum
  * col_var_type. */

 var_type get_type( void ) const { return( f_state / 2 ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to tell whether the ColVariable is integer-valued

 bool is_integer( void ) const { return( f_state & var_type( 2 ) ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to tell whether the ColVariable is non-negative

 bool is_positive( void ) const { return( f_state & var_type( 4 ) ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to tell whether the ColVariable is non-positive

 bool is_negative( void ) const { return( f_state & var_type( 8 ) ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to tell whether the max absolute value of the ColVariable is 1

 bool is_unitary( void ) const { return( f_state & var_type( 16 ) ); }

/*--------------------------------------------------------------------------*/
 /// method to return the lower bound on the ColVariable implied by its type

 VarValue get_lb( void ) const
 {
  return( is_positive() ? VarValue( 0 ) :
	                  ( is_unitary() ? VarValue( -1 ) :
			      -std::numeric_limits<VarValue>::infinity() ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to return the upper bound on the ColVariable implied by its type

 VarValue get_ub( void ) const
 {
  return( is_negative() ? VarValue( 0 ) :
	                  ( is_unitary() ? VarValue( 1 ) :
			        std::numeric_limits<VarValue>::infinity() ) );
  }

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
