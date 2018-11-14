/*--------------------------------------------------------------------------*/
/*--------------------------- File Variable.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the Variable class, a quite general base class of all
 * the possible type of variables that a Block can support. Very few
 * assumptions are made about what form the variable actually has, this
 * being demanded to derived classes.
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

#ifndef __Variable
 #define __Variable /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <list>
#include <vector>
#include <boost/multi_array.hpp>
#include "Modification.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*----------------------- Variable-RELATED TYPES ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Variable_TYPES Variable-related types.
 *  @{ */

 class ThinVarDepInterface;  // forward definition of ThinVarDepInterface
 class Variable;             // forward definition of Variable
 class Block;                // forward definition of Block

 typedef Variable *p_Var;
 ///< a pointer to Variable (Variable *)

 typedef std::vector<p_Var> Vec_p_Var;
 ///< a (1-D) vector of pointer to Variable

 typedef const Vec_p_Var c_Vec_p_Var;
 ///< a (1-D) const vector of pointer to Variable

 template <size_t K>
 using KD_Vec_p_Var = boost::multi_array<p_Var , K>;
 ///< Vec_p_Var<K> is a K-D vector of pointer to Variable

 template <size_t K>
 using KD_c_Vec_p_Var = const boost::multi_array<p_Var , K>;
 ///< c_Vec_p_Var<K> is a const K-D vector of pointer to Variable

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 typedef std::list<p_Var> List_p_Var;
 ///< a list of pointer to Variable (Variable *)

 typedef List_p_Var::iterator List_p_Var_it;
 ///< iterator for a List_p_Var

 typedef const List_p_Var c_List_p_Var;
  ///< a const list of pointers to Variable

 typedef c_List_p_Var::const_iterator c_List_p_Var_it;
 ///< iterator for a c_List_p_Var

 typedef std::vector<List_p_Var> Vec_List_p_Var;
 ///< a 1-D array of lists of pointer to Variable

 typedef const Vec_List_p_Var c_Vec_List_p_Var;
 ///< a const 1-D array of lists of Variable *

 template<size_t K>
 using KD_Vec_List_p_Var = boost::multi_array<List_p_Var , K>;
 ///< Vec_List_p_Var<K> is a K-D vector of lists of Variable *

 template<size_t K>
 using KD_c_Vec_List_p_Var = const boost::multi_array<List_p_Var , K>;
 ///< c_Vec_List_p_Var<K> is a const K-D vector of lists of Variable *


/** @}  end( group( Variable_TYPES ) )*/
/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Variable_CLASSES Classes in Variable.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS Variable -------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// base class of all possible type of variables that a Block can support
/** The class Variable is the base class of all the possible type of
 * variables that the Block can support. This base class only supports a
 * few fundamental facts:
 *
 * - a Variable belongs to one Block;
 *
 * - a Variable influences a set of different objects, gathered below the
 *   abstract class ThinVarDepInterface (Constraint, Objective, Function,
 *   ...); note that the base class makes no provisions about how this set
 *   is stored in order to leave more freedom to derived classes to implement
 *   it in specialized ways;
 *
 * - a Variable can be (temporarily?) fixed, i.e., not really a Variable.
 *
 * Note that obviously a Variable has a value, but the methods for reading it
 * must necessarily depend on the type of value it can take, and therefore
 * must be demanded to derived classes. */

class Variable {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */

 /// type for the indices of "stuff" the Variable is active in
 typedef unsigned int Index;

/*--------------------------------------------------------------------------*/
 /// type for the "type" of the Variable
 typedef unsigned char var_type;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// enum for the different "states" in which the Variable can be
 /** The enum var_state is used to store information about the "state" of
  * a Variable. The base Variable class only supports the notion that a
  * Variable can either be fixed to its current value (i.e., actually a
  * constant), whatever that means for the specific concrete :Variable class,
  * or free to take whatever value (i.e., actually a variable). However,
  * there is still "quite some free space" in this field, so derived classes
  * can extend the set of possible states, for instance to indicate that the
  * Variable can vary, but with some limitations (say, only the integer, or
  * binary, value, ...). */

 enum var_state {
  kFixed = 0 ,   ///< the Variable is fixed to its current value

  kFree = 1      ///< the Variable is free to change its current value
  };

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *
 * Since Variable and the "stuff" (ThinVarDepInterface) they are active in
 * are "doubly" linked, destroying them has the problem of ensuring that the
 * pointers in each object's list are still "live" when the object is
 * destroyed. To simplify this, the underlying assumption is that
 *
 *     *Variable are constructed before the "stuff" they are active
 *      in and destructed after them*
 *
 * which makes it possible for Variable to ignore their list when they are
 * destroyed, possibly saving some pointless work, whereby the list are to
 * be updated by "stuff" destroying themselves right before than the Variable
 * itself is destroyed. However, this incurs the risk that a Variable may
 * leave a "stuff" it was active in in an inconsistent state, in particular
 * for dynamic Variable that can be destroyed "in the middle" of the lifetime
 * of a Block. In these cases, it is necessary to explicitly remove the
 * Variable from the "stuff" it is active in before destroying it; this has
 * to be done "outside" the Variable, because it will not do that.
 *
 * The other case that needs to be considered is that of Variable and "stuff"
 * (possibly std::vector<> etc. of them) being fields of a :Block, so that
 * they are automatically destroyed in the corresponding destructor. As the
 * order in which this happens is usually not very clear, one should strive
 * to have the fields explicitly cleared in the destructor, so that the
 * order in which Variable and "stuff" are destroyed is fixed. This is
 * simple if Variable and "stuff" are in STL containers (std::vector<> etc.)
 * since then one can call the clear() method of the vector to have them
 * destroyed; however, this does not work for individual objects. This is
 * (one of) the rationale for ThinVarDepInterface::clear(): it allows for
 * "stuff" to be explicitly cleared, *without* bothering to warn the
 * Variable of it. This should always be done for all "stuff" before
 * destroying any of their "active" Variable. If the Variable are to be
 * destroyed just next, this it is preferable to clear()-ing the STD
 * container they live in (if any) because the latter invokes the destructor,
 * which removes the "stuff" from the active Variable, while clear()-ing the
 * individual "stuff" does not do it, saving pointless work.
 *  @{ */

///< constructor of Variable: takes a pointer to the Block and the state
/**< Constructor of Variable. It accepts a pointer to the Block to which the
 * Variable belongs, and the state of the variable. Everything has a default
 * (nullptr and kFree, respectively) so that this can be used as the void
 * constructor. If nullptr is passed, then set_Block() [see below] will have
 * to be used later to initialize it. Note that while the enum var_type is
 * provided to encode the possible states of the Variable (kFixed or kFree),
 * the parameter of the constructor is a generic var_type in order to allow
 * derived classes to further "extend" the set of possible types. */

 Variable( Block * my_block = nullptr , const var_type state = kFree )
  : f_Block( my_block ) , f_state( state ) { }

/*--------------------------------------------------------------------------*/
 /// copy constructor
 /** Copy constructor: The father Block of this Variable is defined to be
  * the father Block of the Variable that is being copied. Notice, however,
  * that this does not make this Variable property of that Block. */

 Variable( const Variable & v ) {
  f_state = v.f_state;
  f_Block = v.f_Block;
  }

/*--------------------------------------------------------------------------*/
 /// destructor: it is virtual, and empty
 /** The destructor of the base Variable class has nothing to do. However, it
  * is important to remark that the destructor of any derived class is *not*
  * assumed to scan through the list of "stuff" this Variable is active in and
  * remove the Variable from then. The idea is that
  *
  *     *Variable are constructed before the "stuff" they are active
  *      in and destructed after them*
  *
  * and hence, by the time that a Variable is destructed, the "stuff" it is
  * active in have been destructed anyway; hence it is impossible (and
  * useless) to remove from them. To avoid useless updating of data structures
  * that may be about to be destructed anyway, the destructor of :Variable is
  * *not* supposed to check the list of "stuff" it is active in, and just
  * destroy the Variable (and the list with it).
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

 virtual ~Variable() { }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the pointer to the Block to which the Variable belongs
 /** Method to set the pointer to the Block to which the Variable belongs.
  * If the pointer is not provided in the constructor, it should be called
  * before any other method of the class. */

 void set_Block( Block *fblock ) { f_Block = fblock; }

/*--------------------------------------------------------------------------*/
 /// sets the value of this Variable to its default value
 /** Each Variable has a default value. This is used, for example, for
  * dealing with the dynamic variables. Dynamic variables are considered to
  * "be there even they are not there". That is, it is assumed that all the
  * dynamic variables not explicitly generated are implicitly present in the
  * Block set at their default value. */

 virtual void set_to_default_value( void ) = 0;

/*--------------------------------------------------------------------------*/
 /// change the state of the Variable
 /** Method to change the state of the Variable to a new value. For the base
  * Variable class this means either "fix" it (state = kFixed) or "un-fix" it
  * (state = kFree), but derived classes can add other meanings to the value
  * of this field.
  *
  * The parameter issueMod decides if and how the VariableMod is issued, as
  * described in Observer::make_par(). */

 virtual void set_state( const var_type state ,
			 c_ModParam issueMod = eModBlck );

/*@} -----------------------------------------------------------------------*/
/*------------- METHODS FOR READING THE DATA OF THE Variable ---------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the Variable
 *  @{ */

 /// returns the pointer to the Block to which the Variable belongs
 Block *get_Block( void ) const { return( f_Block ); }

/*@} -----------------------------------------------------------------------*/
/*------------- METHODS DESCRIBING THE BEHAVIOR OF A Variable --------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a Variable
 *  @{ */

 /// returns the state of the Variable
 /** Method to get the current state of the Variable. For the base Variable
  * class this means either "fix" it (state = kFixed) or "un-fix" it (state =
  * kFree), but derived classes can add other meanings to the value of this
  * field. */

 var_type get_state( void ) const { return( f_state ); }

/*@} -----------------------------------------------------------------------*/
/*--------- METHODS FOR HANDLING "STUFF" THE Variable IS ACTIVE IN ---------*/
/*--------------------------------------------------------------------------*/
/** @Name Methods for handling the set of "stuff" the Variable is active in
 *  @{ */

 virtual void add_active( ThinVarDepInterface *stuff ) = 0;

 ///< add a pointer to the list of "active" stuff
 /**< Pure virtual method that adds a pointer to a new "active" something (a
  * ThinVarDepInterface) to the corresponding set. It is pure virtual to
  * allow derived classes complete freedom in the way they implement such
  * list. Note that this method is not supposed to "tell" the
  * ThinVarDepInterface that the Variable is active in there, because the
  * ThinVarDepInterface is supposed to know it (it is likely the
  * ThinVarDepInterface itself who calls the method). In other words, no
  * Modification is issued when this operation is performed. */

/*--------------------------------------------------------------------------*/

 virtual void remove_active( ThinVarDepInterface *stuff ) = 0;

 ///< removes a pointer from the list of "active" stuff
 /**< Pure virtual method that removes the pointer of the given stuff (a
  * ThinVarDepInterface) from the set of "active" constraints. It is pure
  * virtual to allow derived classes complete freedom in the way they
  * implement such list. Note that this method is not supposed to
  * "tell" the ThinVarDepInterface that the Variable is no longer active in
  * there, because the ThinVarDepInterface is supposed to know it (it is
  * likely the ThinVarDepInterface itself who calls the method). In other
  * words, no Modification is issued when this operation is performed. */

/*--------------------------------------------------------------------------*/
 /// get the number of stuff in which this Variable is "active"
 /** Pure virtual method to get the number of "stuff" (ThinVarDepInterface)
  * in which this Variable is "active. It is pure virtual to allow derived
  * classes complete freedom in the way they implement the list of "active"
  * stuff. */

 virtual Index get_num_active( void ) const = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// tells if the given Variable is "active" in this ThinVarDepInterface
 /** Pure virtual method that returns:
  *
  * - if the given ThinVarDepInterface depends on this Variable, the index i
  *   in 0, ..., get_num_active() such that the given ThinVarDepInterface is
  *   the i-th in which the Variable is "active";
  *
  * - otherwise, any number >= get_num_active() (say, Inf<Index>()).
  *
  * The base Variable class makes no provisions about how this is done in
  * order to leave more freedom to derived classes to implement it in
  * specialized ways. */

 virtual Index is_active( ThinVarDepInterface * stuff ) const = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get the i-th ThinVarDepInterface in which this Variable is "active"
 /** Pure virtual method to get the i-th ThinVarDepInterface in which this
  * Variable is "active", where i is between 0 and get_num_active_const() - 1.
  * It is pure virtual to allow derived classes complete freedom in the way
  * they implement the list of "active" stuff. */

 virtual ThinVarDepInterface * get_active( const Index i ) const = 0;

/*@} -----------------------------------------------------------------------*/
/*----------- METHODS FOR LOADING, PRINTING & SAVING THE Variable ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for printing the Variable
 *
 * There is no extracting method (operator>>()) to load a constraint out of
 * a stream because a Variable is not supposed to build itself: rather, the
 * Block it belongs to has to construct it.
 * @{ */

 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected virtual method print(). This way the
  * operator<<() is defined for each Variable, but its behavior can be
  * customized by derived classes. */

 friend std::ostream& operator<< ( std::ostream& out , const Variable& v ) {
  v.print( out );
  return( out );
  }

/*@}------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
 *  @{ */

 /// print information about the Variable on an ostream
 /** Protected method intended to print information about the Variable; it
  * is virtual so that derived classes can print their specific information
  * in the format they choose. */

 virtual void print( std::ostream &output ) const {
  output << "Variable [" << this << "] of Block [" << f_Block << "] with "
         << get_num_active() << " active stuff" << std::endl;
  }

/*@} -----------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 Block *f_Block;    ///< pointer to the Block to which the Variable belongs

 var_type f_state;  ///< current state of the Variable

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

 };  // end( class( Variable ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS VariableMod ------------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modifications specific to a Variable
/** Derived class from AModification to describe modifications to a Variable,
 * i.e., changing its state. */

class VariableMod : public AModification {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: takes the new state of the Variable and a pointer to it

 VariableMod( Variable::var_type state , Variable *var ,
	      const bool cB = true )
  : AModification( cB ) , f_state( state ) , f_variable( var ) { }


/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~VariableMod() { }  ///< destructor: does nothing

/*--------------------- PUBLIC FIELDS OF THE CLASS ------------------------*/

 Variable::var_type f_state;  ///< new state of the Variable

 Variable *f_variable;        ///< Variable where the modification occurs

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the VariableMod

 virtual inline void print( std::ostream &output ) const {
  output << "VariableMod[";
  if( concerns_Block() )
   output << "t]:";
  else
   output << "f]:";  
  if( f_state == Variable::kFixed )
   output << "fixing";
  else
   output << "un-fixing";

  output << " Variable [" << f_variable << "]" << std::endl;
  }

 };  // end( class( VariableMod ) )

/*@}  end( group( Variable_CLASSES ) ) */
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* Variable.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Variable.h ----------------------------*/
/*--------------------------------------------------------------------------*/
