/*--------------------------------------------------------------------------*/
/*------------------------ File BooleanVariable.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the BooleanVariable class, derived from Variable, which
 * is the Variable of the propositional logic, i.e., one that is either true
 * or false.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __BooleanVariable
 #define __BooleanVariable
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>

#include "SMSTypedefs.h"
#include "Variable.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup BooleanVariable_CLASSES Classes in BooleanVariable.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS BooleanVariable ---------------------------*/
/*--------------------------------------------------------------------------*/
/// a Variable that is either true or false
/** The BooleanVariable class, derived from Variable, is the Variable of the
 * propositional logic: its value is either true or false, and it is the
 * natural Variable of the satisfiability problems (SAT, MaxSAT), where it
 * appears in the clauses [see ClauseConstraint.h] either as it is or
 * negated.
 *
 * It is purposely not a ColVariable with binary type: a ColVariable is a
 * real value restricted to {0, 1}, which is what a :MILPSolver works with,
 * while a BooleanVariable is what a SAT solver works with, and a Block that
 * uses BooleanVariable is expected to offer, if a :MILPSolver is to solve
 * it, a reformulation of itself with binary ColVariable [see
 * Block::get_R3_Block()].
 *
 * Besides being fixed [see Variable::is_fixed()], a BooleanVariable has no
 * "type", since its only possible values are already the two it can have.
 * As any Variable, it keeps the set of the "active stuff" where it appears,
 * here as an ordered set of pointers to ThinVarDepInterface. */

class BooleanVariable : public Variable
{
/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

 /// constructor of BooleanVariable, taking the Block it belongs to
 /** Constructor of BooleanVariable. It takes the pointer to the Block to
  * which the BooleanVariable belongs, nullptr by default so that this can
  * be used as the void constructor, and sets the value to its default,
  * i.e., false. */

 explicit BooleanVariable( Block * my_block = nullptr )
  : Variable( my_block ) , f_value( false ) {}

/*--------------------------------------------------------------------------*/
 /// copy constructor: the value is copied, the vector of active stuff not

 BooleanVariable( const BooleanVariable & v )
  : Variable( v ) , f_value( v.f_value ) {}

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: it does not update the stuff where the Variable is active
 /** As for any :Variable [see ColVariable::~ColVariable()], the destructor
  * does not scan the active stuff to remove the BooleanVariable from it:
  * whoever destroys a BooleanVariable that is still active somewhere has
  * to take care of that first. */

 ~BooleanVariable() override = default;

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the value of the BooleanVariable (typically done by a Solver)
 /** Method to set the value of the BooleanVariable, typically called by a
  * Solver attached to the Block. As for ColVariable::set_value(), changing
  * the value of a Variable is not a change in the data of the problem, and
  * therefore this method does not issue any Modification. */

 virtual void set_value( bool new_value = false ) { f_value = new_value; }

/*--------------------------------------------------------------------------*/
 /// sets the value of this BooleanVariable to its default value (false)

 void set_to_default_value( void ) override { set_value( false ); }

/** @} ---------------------------------------------------------------------*/
/*---------- METHODS DESCRIBING THE BEHAVIOR OF A BooleanVariable ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a BooleanVariable
 *  @{ */

 /// returns the value of the BooleanVariable

 [[nodiscard]] bool get_value( void ) const { return( f_value ); }

/** @} ---------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING ACTIVE "STUFF" ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" stuff
 *  @{ */

 [[nodiscard]] Index get_num_active( void ) const override {
  return( v_active.size() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the position of the given stuff, Inf< Index >() if not active

 Index is_active( const ThinVarDepInterface * stuff ) const override {
  auto it = std::lower_bound( v_active.begin() , v_active.end() , stuff );
  if( ( it == v_active.end() ) || ( *it != stuff ) )
   return( Inf< Index >() );
  return( std::distance( v_active.begin() , it ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] ThinVarDepInterface * get_active( Index i ) const override {
  return( v_active[ i ] );
  }

/*--------------------------------------------------------------------------*/
 /// returns (a reference to) the ordered vector of pointers to active stuff

 [[nodiscard]] const std::vector< ThinVarDepInterface * > & active_stuff(
								      void ) const {
  return( v_active );
  }

/*--------------------------------------------------------------------------*/
 /// adds a pointer to the vector of active stuff, keeping it sorted

 void add_active( ThinVarDepInterface * stuff ) override {
  v_active.insert( std::upper_bound( v_active.begin() , v_active.end() ,
				     stuff ) , stuff );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removes the given pointer from the vector of active stuff

 void remove_active( ThinVarDepInterface * stuff ) override {
  auto it = std::lower_bound( v_active.begin() , v_active.end() , stuff );
  if( ( it == v_active.end() ) || ( *it != stuff ) )
   throw( std::invalid_argument(
	  "BooleanVariable::remove_active: called on non-active stuff" ) );
  v_active.erase( it );
  }

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

 /// print the BooleanVariable

 void print( std::ostream & output ) const override {
  output << "BooleanVariable [" << this << "] of Block [" << get_Block()
         << "] with " << get_num_active() << " active stuff, value = "
	 << ( f_value ? "true" : "false" ) << std::endl;
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS -----------------------------*/
/*--------------------------------------------------------------------------*/

 bool f_value;  ///< value of the BooleanVariable

 std::vector< ThinVarDepInterface * > v_active;  ///< set of active stuff

/*--------------------------------------------------------------------------*/

};  // end( class( BooleanVariable ) )

/** @} end( group( BooleanVariable_CLASSES ) ) -----------------------------*/
/*--------------------------------------------------------------------------*/

}  /* namespace SMSpp_di_unipi_it */

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* BooleanVariable.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File BooleanVariable.h -------------------------*/
/*--------------------------------------------------------------------------*/
