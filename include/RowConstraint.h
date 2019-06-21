/*--------------------------------------------------------------------------*/
/*------------------------ File RowConstraint.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *abstract* RowConstraint class, derived from
 * Constraint, which is intended as the base class for all the Constraints
 * that are have a "row form", that is
 *
 *   LHS <= ( some function from Variables to reals ) <= RHS
 *
 * where LHS and RHS are two extended reals (hopefully at least one of which
 * is finite and LHS <= RHS, but this is not enforced in the class).
 *
 * \version 0.20
 *
 * \date 16 - 08 - 2018
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
 * Copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __RowConstraint
 #define __RowConstraint
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Constraint.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup RowConstraint_CLASSES Classes in RowConstraint.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS RowConstraint ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a Constraint that is a "single row"
/** The class RowConstraint, derived from Constraint, is intended as the base
 * class for all the Constraints that are have a "row form", that is,
 *
 *   LHS <= ( some function from Variable to reals ) <= RHS
 *
 * where LHS and RHS are two extended reals (hopefully at least one of which
 * is finite and LHS <= RHS, but this is not enforced in the class). A type
 * "RHSValue" is defined, which is bound by default to doubles, to hold the
 * type of the LHS and RHS. Changing this type here is possible, but this
 * changes it to the whole SMS++ hierarchy, so this does not look too
 * reasonable; if one really needs a different return value than double she
 * can rather re-define a similar class to this.
 *
 * The above form encodes all possible kind of equality, inequality and ranged
 * constraints. In this base class, *no assumption is done upon the form of
 * the function*: typical examples are linear functions, quadratic functions
 * etc., but this is dealt with in derived classes.
 *
 * For many classes of problems this kind of Constraint naturally have a
 * "dual" information attached [see CDASolver.h], which typically has the
 * form of a *Lagrangian multiplier*: a single real value (in this case,
 * again taken to be of type RHSValue). Support to this case is therefore
 * offered by this class. This may be redundant in some cases, but if this
 * really is a problem then a similar derived class from Constraint lacking
 * the dual value can be easily defined.
 *
 * On top of this the basic ConstraintMod, other modifications are possible
 * for this kind of Constraint, namely
 *
 * - changing the LHS/RHS.
 *
 * Yet, note that the base class does not explicitly store LHS and RHS values
 * in order to allow more flexibility for derived classes to do that as they
 * see better fit (for instance, not storing them at all if they are fixed).
 * Thus, the methods for setting and changing these values (which are the
 * ones to throw these Modification) are pure virtual.
 *  */

class RowConstraint : public Constraint {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
 /** @name Public Types
  *  @{ */

 typedef double RHSValue;  ///< type of the LHS/RHS of the RowConstraint
			   /**< type of the LHS/RHS of the RowConstraint,
			    * and therefore also of the attached dual
			    * information (Lagrangian multiplier). */

 typedef const RHSValue c_RHSValue;  ///< a const VarValue

/**@} ----------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
    @{ */

 /// constructor of RowConstraint, taking the Block, LHS and RHS
 /** Constructor of RowConstraint. Takes the pointer to the Block to which
  * the RowConstraint belongs, to be passed to the constructor of
  * Constrain (default nullptr, so that this can be used as the void
  * constructor). */

 RowConstraint( Block *my_block = nullptr ): Constraint( my_block ) ,
  d_value( 0 ) { }

/*--------------------------------------------------------------------------*/

 virtual ~RowConstraint() { }  ///< destructor: it is virtual, and empty

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the RHS of this to RowConstraint
 /** Set the RHS of this to RowConstraint to rhs_value. Since the base class
  * does not directly handles these values, this method is pure virtual.
  *
  * The parameter issueMod decides if and how any Modification is issued, as
  * described in Observer::make_par(). */

 virtual void set_rhs( c_RHSValue rhs_value ,
		       c_ModParam issueMod = eModBlck ) = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set the LHS of this to RowConstraint
 /** Set the LHS of this to RowConstraint to lhs_value. Since the base class
  * does not directly handles these values, this method is pure virtual.
  *
  * The parameter issueMod decides if and how any Modification is issued, as
  * described in Observer::make_par(). */

 virtual void set_lhs( c_RHSValue lhs_value ,
		       c_ModParam issueMod = eModBlck ) = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set both the LHS and the RHS of this to RowConstraint
 /** Set the both the LHS and the RHS of this to RowConstraint to the same
  * value, both_value. This is useful for equality constraints. Since the base
  * class does not directly handles these values, this method is pure virtual.
  *
  * The parameter issueMod decides if and how any Modification is issued, as
  * described in Observer::make_par(). */

 virtual void set_both( c_RHSValue both_value ,
			c_ModParam issueMod = eModBlck) = 0;

/*--------------------------------------------------------------------------*/
 /// method to set the dual value of the RowConstraint
 /** method to set the dual value (i.e. the Langrangian multiplier) of the
  * RowConstraint; typically, a CDASolver [see CDASolver.h] attached to the
  * Block to which this RowConstraint belongs will do it. For more ease of
  * mind, this method is virtual. */

 virtual void set_dual( c_RHSValue new_value = 0 ) { d_value = new_value; }

/**@} ----------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE DATA OF THE RowConstraint -----------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the RowConstraint
    @{ */

 /// pure virtual method to get the RHS of the RowConstraint
 virtual RHSValue get_rhs( void ) const = 0;

 /// pure virtual  method to get the LHS of the RowConstraint
 virtual RHSValue get_lhs( void ) const = 0;

/**@} ---------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF A RowConstraint -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a RowConstraint
    @{ */

 /// compute the value of variable part of the RowConstraint

 virtual int compute( bool changedvars = true ) override = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get the value of variable part of the RowConstraint
 /** Method to get the value of variable part of the RowConstraint. It is
  * virtual, so that derived classes can store it as they best see fit. */

 virtual RHSValue value( void ) const = 0;

/*--------------------------------------------------------------------------*/

 virtual bool feasible( void ) const override {
  bool feas = true;
  c_RHSValue val = value();
  c_RHSValue lhs = get_lhs();
  c_RHSValue rhs = get_rhs();
  
  if( lhs > - Inf<RHSValue>() )
   feas &= ( val >= lhs );

  if( rhs < Inf<RHSValue>() )
   feas &= ( val <= rhs );

  return( feas );
  }

/*--------------------------------------------------------------------------*/
 /// returns the absolute violation of the RowConstraint
 /** The method computes and returns the absolute violation of the 
  * RowConstraint corresponding to its current value [see value()]. The
  * value is positive if one of the two bounds is violated (of course not
  * both can be) and it is the amount of violation; otherwise. An infinite
  * bound (lhs == - Inf<RHSValue>() or rhs == Inf<RHSValue>()) corresponds to
  * a - Inf<RHSValue>() violation whatever the value (even if it is the same
  * infinity). 
  *
  * This method is provided because checking feasibility of a RowConstraint
  * should reasonably require numerical tolerances, which are not there in
  * the feasible() method (because in general constraints could be checked
  * "exactly", say if only integer arithmetic is involved). This is somehow
  * against the grain of ThinComputeInterface: there could be parameters
  * for doing this. However, this would complicate the interface, and
  * could mean that each RowConstraint need to carry its own values of
  * the parameters (while they are generally constant across many constraints
  * of "the same type" in a model); via this method checking can be done
  * outside of the RowConstraint, which for the time being we condiser the
  * better alternative. */

 virtual RHSValue abs_viol( void ) const {
  RHSValue viol = -Inf<RHSValue>();
  c_RHSValue val = value();
  c_RHSValue lhs = get_lhs();
  c_RHSValue rhs = get_rhs();

  if( ( lhs > - Inf<RHSValue>() ) && ( val < Inf<RHSValue>() ) )
   viol = ( val <= -Inf<RHSValue>() ? Inf<RHSValue>() : lhs - val );

  if( ( rhs < Inf<RHSValue>() ) && ( val > -Inf<RHSValue>() ) )
   viol = std::max( viol , ( val >= Inf<RHSValue>() ? Inf<RHSValue>() :
			                              val - rhs ) );
  return( viol );
  }

/*--------------------------------------------------------------------------*/
 /// returns the relative violation of the RowConstraint
 /** The method computes and returns the relative violation of the 
  * RowConstraint corresponding to its current value [see value()]. This is
  * the same value as abs_viol() returns, except each violation is divided
  * for the absolute value of the corresponding bound. If one bound is zero
  * then its violation is divided by the absolute value of the other bound,
  * and if both are zero then the absolute violation is returned (the
  * scaling factor is 1). Only finite bound are considered.
  *
  * See abs_viol() for the rationale of providing such a method. */

 virtual RHSValue rel_viol( void ) const {
  c_RHSValue rhs = get_rhs();
  c_RHSValue val = value();

  // if the value is +INF, then if the RHS is < +INF then the constraint is
  // infinitely violated, otherwise is infinitely slackened
  if( val >= Inf<RHSValue>() )
   return( rhs < Inf<RHSValue>() ? Inf<RHSValue>() : - Inf<RHSValue>() );

  c_RHSValue lhs = get_lhs();
  // if the value is -INF, then if the LHS is > -INF then the constraint is
  // infinitely violated, otherwise is infinitely slackened
  if( val <= -Inf<RHSValue>() )
   return( lhs > -Inf<RHSValue>() ? Inf<RHSValue>() : - Inf<RHSValue>() );

  // the value is finite
  if( lhs <= - Inf<RHSValue>() )
   if( rhs >= Inf<RHSValue>() )
    return( -Inf<RHSValue>() );
   else
    return( rhs == 0 ? val : ( val - rhs ) / std::abs( rhs ) );
  else
   if( rhs >= Inf<RHSValue>() )
    return( lhs == 0 ? - val : ( lhs - val ) / std::abs( lhs ) );

  // both LHS and RHS are finite
  if( lhs == 0 )
   if( rhs == 0 )
    return( std::abs( val ) );
   else
    return( std::max( - val , val - rhs ) / std::abs( rhs ) );
  else
   if( rhs == 0 )
    return( std::max( lhs - val , val ) / std::abs( lhs ) );
   else
    return( std::max( ( lhs - val ) / std::abs( lhs ) ,
		      ( val - rhs ) / std::abs( rhs ) ) );
  }

/*--------------------------------------------------------------------------*/
 /// get the dual value (the Langrangian multiplier) of the RowConstraint

 RHSValue get_dual( void ) const { return ( d_value ); }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
 *  @{ */

 /// print information about the RowConstraint on an ostream
 virtual void print( std::ostream &output ) const override {
  output << "RowConstraint [" << this << "] of Block [" << f_Block
	 << "] with " << get_num_active_var() << " active variables, ";
  if( feasible() )
   output << "feasible";
  else
   output << "unfeasible";

  output << " (value = " << value() << ")" << std::endl;
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 RHSValue d_value;  ///< dual value (Langrangian multiplier)

/*--------------------------------------------------------------------------*/

 };  // end( class( RowConstraint ) )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS RowConstraintMod --------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modifications specific to a RowConstraint
/** Derived class from ConstraintMod to describe Modification specific to a
 * RowConstraint, i.e., change its LHS / RHS.
 *
 * Defining a class is a bit weird because the only thing the class does is
 * to define am enum for the new value of the type of Modification in the
 * ConstraintMod. However, throwing a Modification of a different class (but
 * derived from ConstraintMod) may make it easier for the solver to handle
 * it (at the very least, it directly knows it comes from a RowConstraint
 * without having to check it). */

class RowConstraintMod : public ConstraintMod {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/
 /// public enum for the types of ConstraintMod thrown
 /** Public enum "extending" cons_mod_type with the new types of Modification
  * thrown by a RowConstraint. */

 enum RowC_mod_type {
  eChgLHS = eConstModLastParam ,    ///< change the LHS
  eChgRHS ,                         ///< change the RHS
  eChgBTS ,                         ///< change both the RHS and the LHS
  eRowConstModLastParam  ///< first allowed value for derived classes
                         /**< Convenience value for easily allow derived
			  * classes to further extend the set of types of
			  * Modification. */
  };

/*---------------------- CONSTRUCTOR & DESTRUCTOR --------------------------*/

 /// constructor: just calls that of ConstraintMod

 RowConstraintMod( RowConstraint *cnst , int mod = eChgLHS ,
		   const bool cB = true )
  : ConstraintMod( cnst , mod , cB ) { }

 virtual ~RowConstraintMod() { }  ///< destructor: does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the RowConstraintMod

 virtual inline void print( std::ostream &output ) const override {
  output << "RowConstraintMod[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";  
  output << "]: changing ";
  switch( f_type ) {
   case( eChgLHS ): output << "LHS"; break;
   case( eChgRHS ): output << "RHS"; break;
   default:         output << "both";
   }

  output << " of RowConstraint [" << f_constraint << "]" << std::endl;
  }

/*--------------------------------------------------------------------------*/

  };  // end( class( RowConstraintMod ) )

/*--------------------------------------------------------------------------*/

/** @} end( group( RowConstraint_CLASSES ) ) -------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* RowConstraint.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File RowConstraint.h --------------------------*/
/*--------------------------------------------------------------------------*/
