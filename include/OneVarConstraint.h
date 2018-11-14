/*--------------------------------------------------------------------------*/
/*----------------------- File OneVarConstraint.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the OneVarConstraint class, derived from RowConstraint,
 * which is a class that defines a RowConstraint where the row contains a
 * *single* ColVariable; hence,
 *
 *            some function from Variables to reals
 *
 * can basically only be the (real) value of that single ColVariable, and the
 * Constraint itself can basically only constrain that value to live into
 * some (possibly, degenerate) real interval.
 *
 * From OneVarConstraint a few classes are derived that deal with different
 * versions of the interval:
 *
 * - BoxConstraint has user-defined LHS and RHS;
 *
 * - LBConstraint has user-defined LHS and RHS = +Infty (Lower Bound);
 *
 * - UBConstraint has LHS = -Infty and user-defined RHS (Upper Bound);
 *
 * - NNConstraint has LHS = 0 and RHS = +Infty (Non-Negativity);
 *
 * - NPConstraint has LHS = -Infty and RHS = 0 (Non-Positivity);
 *
 * - ZOConstraint has LHS = 0 and RHS = 1 (Zero-One).
 *
 * Besides saving a bit of space, the classes with fixed LHS and/or RHS
 * guarantee that it can never be changed, which might be useful.
 *
 *
 * \version 0.10
 *
 * \date 16 - 08 - 2018
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __OneVarConstraint
 #define __OneVarConstraint
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ColVariable.h"
#include "RowConstraint.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup OneVarConstraint_CLASSES Classes in OneVarConstraint.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS OneVarConstraint -------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a RowConstraint with a single Variable
/** The class OneVarConstraint, derived from RowConstraint, defines a row
 * constraint where the row contains a single ColVariable; hence,
 *
 *            some function from Variables to reals
 *
 * can basically only be the value of that single ColVariable, and the
 * Constraint itself can basically only constrain that value to live into
 * some (possibly, degenerate) interval. Yet, OneVarConstraint still does
 * not specify how the interval is managed (like RowConstraint) to leave
 * freedom to further derived classes to do this in different ways.
 *
 * It would, of course, be possible to have a constraint of the form
 *
 *   LHS <= f( x ) <= RHS
 *
 * where f(x) is a univariate function (x is a single ColVariable). However,
 * for algebraic functions this should often be equivalently stated as the
 * constraint that x belongs to some set of intervals, i.e., as a bunch of
 * OneVarConstraint. Support for the fact that f( x ) can live in a set of
 * different intervals may be added to RowConstraint (and hence to
 * OneVarConstraint one day), but that day is not today. */

class OneVarConstraint : public RowConstraint {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------- PUBLIC TYPES OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */

 /// virtualized concrete iterator
 /** A concrete class deriving from ThinVarDepInterface::v_iterator and
  * implementing the concrete iterator for "sifting through" (!!) the (only
  * one) "active" ColVariable of a OneVarConstraint. */

 class v_iterator : public ThinVarDepInterface::v_iterator
 {
  public:

  v_iterator( ColVariable * ptr ) : ptr_( ptr ) { }

  virtual void operator++( void ) override final { (ptr_)++; }
  virtual reference operator*( void ) const override final {
   return( *ptr_ );
   }
  virtual pointer operator->( void ) const override final {
   return( ptr_ );
   }
  virtual bool operator==( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   const OneVarConstraint::v_iterator * tmp =
   #ifdef NDEBUG
                  static_cast<const OneVarConstraint::v_iterator *>( & rhs );
   return( ptr_ == tmp->ptr_ );
   #else
                  dynamic_cast<const OneVarConstraint::v_iterator *>( & rhs );
   return( tmp ? ptr_ == tmp->ptr_ : false );
   #endif
   }
  virtual bool operator!=( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   const OneVarConstraint::v_iterator * tmp =
   #ifdef NDEBUG
                  static_cast<const OneVarConstraint::v_iterator *>( & rhs );
   return( ptr_ != tmp->ptr_ );
   #else
                  dynamic_cast<const OneVarConstraint::v_iterator *>( & rhs );
   return( tmp ? ptr_ != tmp->ptr_ : false );
   #endif
   }

  private:

  ColVariable * ptr_;
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// virtualized concrete const_iterator
 /** A concrete class deriving from ThinVarDepInterface::v_const_iterator and
  * implementing the concrete iterator for "sifting through" (!!) the (only
  * one) "active" ColVariable of a OneVarConstraint. */

 class v_const_iterator : public ThinVarDepInterface::v_const_iterator
 {
  public:

  v_const_iterator( ColVariable * ptr ) : ptr_( ptr ) { }

  virtual void operator++( void ) override final { (ptr_)++; }
  virtual reference operator*( void ) const override final {
   return( *ptr_ );
   }
  virtual pointer operator->( void ) const override final {
   return( ptr_ );
   }
  virtual bool operator==( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   const OneVarConstraint::v_const_iterator * tmp =
   #ifdef NDEBUG
            static_cast<const OneVarConstraint::v_const_iterator *>( & rhs );
   return( ptr_ == tmp->ptr_ );
   #else
            dynamic_cast<const OneVarConstraint::v_const_iterator *>( & rhs );
   return( tmp ? ptr_ == tmp->ptr_ : false );
   #endif
   }
  virtual bool operator!=( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   const OneVarConstraint::v_const_iterator * tmp =
   #ifdef NDEBUG
            static_cast<const OneVarConstraint::v_const_iterator *>( & rhs );
   return( ptr_ != tmp->ptr_ );
   #else
            dynamic_cast<const OneVarConstraint::v_const_iterator *>( & rhs );
   return( tmp ? ptr_ != tmp->ptr_ : false );
   #endif
   }

  private:

  ColVariable * ptr_;
  };

/*@}------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
    @{ */

 /// constructor of OneVarConstraint: takes a Block * and a ColVariable *
 /** Constructor of OneVarConstraint. It receives a pointer to the Block to
  * which the OneVarConstraint belongs and a pointer to the ColVariable that
  * defines the constraint. Everything has a default (nullptr and nullptr,
  * respectively) so that this can be used as the void constructor. */

 OneVarConstraint( Block *my_block = nullptr ,
		   ColVariable * const variable = nullptr )
  : RowConstraint( my_block ) , f_variable( nullptr ) {
  this->set_variable( variable , eNoMod );
  }

/*--------------------------------------------------------------------------*/
 /// destructor: unregister the OneVarConstraint from the ColVariable
 virtual ~OneVarConstraint() { set_variable( nullptr , eNoMod ); }

/*--------------------------------------------------------------------------*/
 /// "guts of destructor", not unregistering from the ColVariable
 virtual void clear( void ) override { f_variable = nullptr; }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the pointer to the ColVariable in this OneVarConstraint
 /**< Method to set the pointer to the ColVariable that defines this
  * OneVarConstraint. A call to set_variable() (with default == nullptr
  * argument) removes any ColVariable from the OneVarConstraint, leaving it
  * "empty".
  *
  * The parameter issueMod decides if and how the corresponding
  * OneVarConstraintMod is issued, as described in Observer::make_par().
  *
  * Usually, the OneVarConstraintMod should be issued. One case where this is
  * not necessary is when, say, the method is called with nullptr argument
  * while destroying a (dynamic) ColVariable, in which case the affected
  * ColVariable don't really need to know that it is no longer active on this
  * OneVarConstraint (as it is to be destroyed anyway). See the comments to
  * remove_variable(). */

 virtual void set_variable( ColVariable * const variable = nullptr ,
			    c_ModParam issueMod = eModBlck );

/*@} -----------------------------------------------------------------------*/
/*---------- METHODS DESCRIBING THE BEHAVIOR OF A OneVarConstraint ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a OneVarConstraint
    @{ */

 /// compute the value of variable part of the OneVarConstraint
 /** The method has to compute the value of variable part of the
  * OneVarConstraint ... i.e., do nothing, as it is just the value of the
  * given ColVariable. */

 virtual int compute( bool changedvars = true ) override { return( kOK ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get the value of variable part of the OneVarConstraint
 /** Method to get the value of variable part of the OneVarConstraint, which
  * is just the value of the value of the given ColVariable. */

 virtual RHSValue value( void ) const override {
  return( f_variable ? f_variable->get_value() : 0 );
  }

/*@} -----------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE OneVarConstraint -----*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" Variable in the
 * OneVarConstraint, which is not very hard ...
 *  @{ */

 Index get_num_active_var( void ) const override
 {
  return( f_variable ? 1 : 0 );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual Index is_active( const Variable * const variable ) const override
 {
  return( f_variable == variable ?0 : Inf<Index>() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual Variable * get_active_var( const Index i ) const override
 {
  return( i == 0 ? f_variable : nullptr );
  }

/*--------------------------------------------------------------------------*/

 virtual v_iterator * v_begin( void ) override {
  return( new OneVarConstraint::v_iterator( f_variable ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual v_const_iterator * v_begin( void ) const override {
  return( new OneVarConstraint::v_const_iterator( f_variable ) );
  }

/*--------------------------------------------------------------------------*/

 virtual v_iterator * v_end( void ) override {
  return( new OneVarConstraint::v_iterator( f_variable + 1 ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual v_const_iterator * v_end( void ) const override {
  return( new OneVarConstraint::v_const_iterator( f_variable + 1 ) );
  }

/*--------------------------------------------------------------------------*/

 virtual void remove_variable( Variable * variable ,
			       c_ModParam issueMod = eModBlck ) override
 {
  if( variable == f_variable )
   set_variable( nullptr , issueMod );
  else
   throw( std::invalid_argument( "Variable not present in the Constraint" ) );
  }

/*--------------------------------------------------------------------------*/

 virtual void remove_variables( std::vector<Variable *> && vars ,
                                const bool ordered = false ,
                                c_ModParam issueMod = eModBlck ) override
 {
  if( ! vars.size() )  // nothing to do
   return;             // cowardly (and silently) return

  if( ( vars.size() != 1 ) || ( vars[ 0 ] != f_variable ) )
   throw( std::invalid_argument( "Variable not present in the Constraint" ) );

  set_variable( nullptr , issueMod );
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

 /// print information about the OneVarConstraint on an ostream
 virtual void print( std::ostream &output ) const override {
  output << "OneVarConstraint [" << this << "] of Block [" << f_Block
	 << "] with ColVariable [" << f_variable << "], ";
  if( feasible() )
   output << "feasible";
  else
   output << "unfeasible";

  output << " (value = " << value() << ")" << std::endl;
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 ColVariable * f_variable;
 ///< pointer to the ColVariable that defines this Constraint

/*--------------------------------------------------------------------------*/

 };  // end( class( OneVarConstraint ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS BoxConstraint ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// an OneVarConstraint with generic LHS and RHS
/** The class BoxConstraint deriveds from OneVarConstraint and allows the user
 * to arbitrarily define the LHS and the RHS. */

class BoxConstraint : public OneVarConstraint {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*@}------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
    @{ */

 /// constructor of BoxConstraint: takes Block *, ColVariable *, LHS and RHS
 /** Constructor of BoxConstraint. It receives a pointer to the Block to
  * which the BoxConstraint belongs, a pointer to the ColVariable that
  * defines the constraint, the LHS and the RHS. Everything has a default
  * (nullptr, nullptr, 0, and Inf<RHSValue>, respectively) so that this can
  * be used as the void constructor. */

 BoxConstraint( Block *my_block = nullptr ,
		ColVariable * const variable = nullptr ,
		c_RHSValue lhs = 0 ,
		c_RHSValue rhs = std::numeric_limits<RHSValue>::infinity() )
  : OneVarConstraint( my_block , variable ) ,
    f_lhs( lhs ) , f_rhs( rhs ) {}

/*--------------------------------------------------------------------------*/
 /// destructor: does nothing special
 virtual ~BoxConstraint() { }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the RHS of this to BoxConstraint
 /** Set the RHS of this to BoxConstraint to rhs_value. Note that this issues
  * a OneVarConstraintMod but with "type" RowConstraintMod::eChgRHS; thus,
  * this can be dealt with as a "regular" RowConstraintMod, but a Solver
  * aware of this specific kind of RowConstraint can react in specialized
  * ways. */

 virtual void set_rhs( c_RHSValue rhs_value ,
		       c_ModParam issueMod = eModBlck ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set the LHS of this to BoxConstraint
 /** Set the LHS of this to BoxConstraint to lhs_value. Note that this issues
  * a OneVarConstraintMod but with "type" RowConstraintMod::eChgLHS; thus,
  * this can be dealt with as a "regular" RowConstraintMod, but a Solver
  * aware of this specific kind of RowConstraint can react in specialized
  * ways. */

 virtual void set_lhs( c_RHSValue lhs_value ,
		       c_ModParam issueMod = eModBlck ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set both the LHS and the RHS of this to BoxConstraint
 /** Set the both the LHS and the RHS of this to BoxConstraint to the same
  * value, both_value. This is useful for equality constraints. Note that
  * this throws a OneVarConstraintMod but with "type"
  * RowConstraintMod::eChgBTS; thus, this can be dealt with as a
  * "regular" RowConstraintMod, but a Solver aware of this specific
  * kind of RowConstraint can react in specialized ways. */

 virtual void set_both( c_RHSValue both_value ,
			c_ModParam issueMod = eModBlck ) override;

/*@} -----------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE DATA OF THE BoxConstraint -----------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the BoxConstraint
    @{ */

 /// method to get the RHS of the BoxConstraint
 virtual RHSValue get_rhs( void ) const override { return( f_rhs ); }

 /// method to get the LHS of the BoxConstraint
 virtual RHSValue get_lhs( void ) const override { return( f_lhs ); }

/*@}------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
 *  @{ */

 /// print information about the OneVarConstraint on an ostream
 virtual void print( std::ostream &output ) const override {
  output << "BoxConstraint [" << this << "] of Block [" << f_Block
	 << "] with ColVariable [" << f_variable << "], LHS = "
	 << f_lhs << ", RHS = " << f_rhs <<  ", value = " << value()
	 << std::endl;
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 RHSValue f_lhs;        ///< LHS of the BoxConstraint
 RHSValue f_rhs;        ///< RHS of the BoxConstraint

/*--------------------------------------------------------------------------*/

 };  // end( class( BoxConstraint ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS LB0Constraint ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// an OneVarConstraint with LHS = 0 and generic RHS
/** The class LB0Constraint deriveds from OneVarConstraint and allows the user
 * to arbitrarily define the RHS, while the LHS is fixed to 0. */

class LB0Constraint : public OneVarConstraint {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*@}------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
    @{ */

 /// constructor of LB0Constraint: takes Block *, ColVariable *, and RHS
 /** Constructor of LB0Constraint. It receives a pointer to the Block to
  * which the LB0Constraint belongs, a pointer to the ColVariable that
  * defines the constraint, and the RHS. Everything has a default (nullptr,
  * nullptr, and Inf<RHSValue>, respectively) so that this can be used as
  * the void constructor. */

 LB0Constraint( Block *my_block = nullptr ,
		ColVariable * const variable = nullptr ,
		c_RHSValue rhs = std::numeric_limits<RHSValue>::infinity() )
  : OneVarConstraint( my_block , variable ) , f_rhs( rhs ) {}

/*--------------------------------------------------------------------------*/
 /// destructor: does nothing special
 virtual ~LB0Constraint() { }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the RHS of this to LB0Constraint
 /** Set the RHS of this to LB0Constraint to rhs_value. Note that this
  * throws a OneVarConstraintMod but with "type" RowConstraintMod::eChgRHS;
  * thus, this can be dealt with as a "regular" RowConstraintMod, but a
  * Solver aware of this specific kind of RowConstraint can react in
  * specialized ways. */

 virtual void set_rhs( c_RHSValue rhs_value ,
		       c_ModParam issueMod = eModBlck ) override final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set the LHS of this to LB0Constraint: this throws exception

 virtual void set_lhs( c_RHSValue lhs_value ,
		       c_ModParam issueMod = eModBlck ) override final {
  if( lhs_value != 0 )
   throw( std::invalid_argument( "cannot change LHS in a LB0Constraint" ) );
   }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set both the LHS and the RHS of this to LB0Constraint

 virtual void set_both( c_RHSValue both_value ,
			c_ModParam issueMod = eModBlck  ) override final {
  if( both_value == 0 )
   set_rhs( both_value , issueMod );
  else
   throw( std::invalid_argument( "cannot change LHS in a LB0Constraint" ) );
  }

/*@} -----------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE DATA OF THE LB0Constraint -----------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the LB0Constraint
    @{ */

 /// method to get the RHS of the LB0Constraint
 virtual RHSValue get_rhs( void ) const override final { return( f_rhs ); }

 /// method to get the LHS of the LB0Constraint
 virtual RHSValue get_lhs( void ) const override final { return( 0 ); }

/*@}------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
 *  @{ */

 /// print information about the LB0Constraint on an ostream
 virtual void print( std::ostream &output ) const override final {
  output << "LB0Constraint [" << this << "] of Block [" << f_Block
	 << "] with ColVariable [" << f_variable << "], RHS = "
	 << f_rhs <<  ", value = " << value() << std::endl;
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 RHSValue f_rhs;        ///< RHS of the LB0Constraint

/*--------------------------------------------------------------------------*/

 };  // end( class( LB0Constraint ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS UB0Constraint ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// an OneVarConstraint with generic LHS and RHS = 0 
/** The class UB0Constraint deriveds from OneVarConstraint and allows the user
 * to arbitrarily define the LHS, while the RHS is fixed to 0. */

class UB0Constraint : public OneVarConstraint {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*@}------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
    @{ */

 /// constructor of UB0Constraint: takes Block *, ColVariable *, and LHS
 /** Constructor of UB0Constraint. It receives a pointer to the Block to
  * which the UB0Constraint belongs, a pointer to the ColVariable that
  * defines the constraint, and the LHS. Everything has a default (nullptr,
  * nullptr, and -Inf<RHSValue>, respectively) so that this can be used as
  * the void constructor. */

 UB0Constraint( Block *my_block = nullptr ,
		ColVariable * const variable = nullptr ,
		c_RHSValue lhs = -std::numeric_limits<RHSValue>::infinity() )
  : OneVarConstraint( my_block , variable ) , f_lhs( lhs ) {}

/*--------------------------------------------------------------------------*/
 /// destructor: does nothing special
 virtual ~UB0Constraint() { }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the RHS of this to UB0Constraint: this throws exception

 virtual void set_rhs( c_RHSValue rhs_value ,
		       c_ModParam issueMod = eModBlck ) override final {
  if( rhs_value != 0 )
   throw( std::invalid_argument( "cannot change RHS in a UB0Constraint" ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set the LHS of this to UB0Constraint
 /** Set the LHS of this to UB0Constraint to rhs_value. Note that this
  * throws a OneVarConstraintMod but with "type" RowConstraintMod::eChgLHS;
  * thus, this can be dealt with as a "regular" RowConstraintMod, but a
  * Solver aware of this specific kind of RowConstraint can react in
  * specialized ways. */

 virtual void set_lhs( c_RHSValue lhs_value ,
		       c_ModParam issueMod = eModBlck ) override final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set both the LHS and the RHS of this to UB0Constraint

 virtual void set_both( c_RHSValue both_value ,
			c_ModParam issueMod = eModBlck ) override final {
  if( both_value == 0 )
   set_lhs( both_value , issueMod );
  else
   throw( std::invalid_argument( "cannot change RHS in a UB0Constraint" ) );
  }

/*@} -----------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE DATA OF THE UB0Constraint -----------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the UB0Constraint
    @{ */

 /// method to get the RHS of the UB0Constraint
 virtual RHSValue get_rhs( void ) const override { return( 0 ); }

 /// method to get the LHS of the UB0Constraint
 virtual RHSValue get_lhs( void ) const override { return( f_lhs ); }

/*@}------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
 *  @{ */

 /// print information about the OneVarConstraint on an ostream
 virtual void print( std::ostream &output ) const override {
  output << "UB0Constraint [" << this << "] of Block [" << f_Block
	 << "] with ColVariable [" << f_variable << "], LHS = "
	 << f_lhs <<  ", value = " << value() << std::endl;
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 RHSValue f_lhs;        ///< LHS of the UB0Constraint

/*--------------------------------------------------------------------------*/

 };  // end( class( UB0Constraint ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS LBConstraint -----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// an OneVarConstraint with generic LHS and +Infty RHS
/** The class UBConstraint deriveds from OneVarConstraint and allows the user
 * to arbitrarily define the LHS, while the RHS is fixed to +Infty. */

class LBConstraint : public OneVarConstraint {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*@}------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
    @{ */

 /// constructor of LBConstraint: takes Block *, ColVariable *, and LHS
 /** Constructor of LBConstraint. It receives a pointer to the Block to
  * which the BoxConstraint belongs, a pointer to the ColVariable that
  * defines the constraint and the LHS. Everything has a default (nullptr,
  * nullptr, and 0, respectively) so that this can be used as the void
  * constructor. */

 LBConstraint( Block *my_block = nullptr ,
	       ColVariable * const variable = nullptr , c_RHSValue lhs = 0 )
  : OneVarConstraint( my_block , variable ) , f_lhs( lhs ) { }

/*--------------------------------------------------------------------------*/
 /// destructor: does nothing special
 virtual ~LBConstraint() { }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the RHS of this to LBConstraint: this throws exception

 virtual void set_rhs( c_RHSValue rhs_value ,
		       c_ModParam issueMod = eModBlck ) override {
  if( rhs_value < std::numeric_limits<RHSValue>::infinity() )
   throw( std::invalid_argument( "cannot change RHS in a LBConstraint" ) );
   }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set the LHS of this to LBConstraint
 /** Set the LHS of this to LBConstraint to lhs_value. Note that this
  * throws a OneVarConstraintMod but with "type" RowConstraintMod::eChgLHS;
  * thus, this can be dealt with as a "regular" RowConstraintMod, but a
  * Solver aware of this specific kind of RowConstraint can react in
  * specialized ways. */

 virtual void set_lhs( c_RHSValue lhs_value ,
		       c_ModParam issueMod = eModBlck ) override final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set both the LHS and the RHS of this to LBConstraint

 virtual void set_both( c_RHSValue both_value ,
			c_ModParam issueMod = eModBlck ) override final {
  if( both_value >= std::numeric_limits<RHSValue>::infinity() )
   set_lhs( both_value , issueMod );
  else
   throw( std::invalid_argument( "cannot change RHS in a LBConstraint" ) );
  }

/*@} -----------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE DATA OF THE LBConstraint ------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the LBConstraint
    @{ */

 /// method to get the RHS of the LBConstraint
 virtual RHSValue get_rhs( void ) const override final {
  return( std::numeric_limits<RHSValue>::infinity() );
  }

 /// method to get the LHS of the LBConstraint
 virtual RHSValue get_lhs( void ) const override final { return( f_lhs ); }

/*@} -----------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF A LBConstraint ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a LBConstraint
    @{ */

 virtual bool feasible( void ) const override final {
  return( ( f_lhs <= - Inf<double>() ) ||
	  ( f_variable->get_value() >= f_lhs ) );
  }

/*--------------------------------------------------------------------------*/

 virtual RHSValue abs_viol( void ) const override final {
  if( f_lhs <= - Inf<double>() )
   return( - Inf<double>() );
  
  c_RHSValue val = f_variable->get_value();
  return( val <= -Inf<double>() ? Inf<double>() : f_lhs - val );
  }

/*--------------------------------------------------------------------------*/

 virtual RHSValue rel_viol( void ) const override final {
  if( f_lhs <= -Inf<double>() )
   return( -Inf<double>() );

  c_RHSValue val = f_variable->get_value();
  if( val >= Inf<double>() )
   return( -Inf<double>() );

  if( val <= -Inf<double>() )
   return( Inf<double>() );

  return( f_lhs == 0 ? f_lhs - val : ( f_lhs - val ) / std::abs( f_lhs ) );
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

 /// print information about the LBConstraint on an ostream
 virtual void print( std::ostream &output ) const override {
  output << "LBConstraint [" << this << "] of Block [" << f_Block
	 << "] with ColVariable [" << f_variable << "], LHS = "
	 << f_lhs << ", value = " << value() << std::endl;
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 RHSValue f_lhs;        ///< LHS of the LBConstraint

/*--------------------------------------------------------------------------*/

 };  // end( class( LBConstraint ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS UBConstraint -----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// an OneVarConstraint with generic RHS and -Infty LHS
/** The class UBConstraint deriveds from OneVarConstraint and allows the user
 * to arbitrarily define the RHS, while the LHS is fixed to -Infty. */

class UBConstraint : public OneVarConstraint {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*@}------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
    @{ */

 /// constructor of UBConstraint: takes Block *, ColVariable *, and RHS
 /** Constructor of UBConstraint. It receives a pointer to the Block to
  * which the BoxConstraint belongs, a pointer to the ColVariable that
  * defines the constraint and the RHS. Everything has a default (nullptr,
  * nullptr, and 0, respectively) so that this can be used as the void
  * constructor. */

 UBConstraint( Block *my_block = nullptr ,
	       ColVariable * const variable = nullptr , c_RHSValue rhs = 0 )
  : OneVarConstraint( my_block , variable ) , f_rhs( rhs ) { }

/*--------------------------------------------------------------------------*/
 /// destructor: does nothing special
 virtual ~UBConstraint() { }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the RHS of this to UBConstraint
 /** Set the RHS of this to UBConstraint to lhs_value. Note that this
  * throws a OneVarConstraintMod but with "type" RowConstraintMod::eChgRHS;
  * thus, this can be dealt with as a "regular" RowConstraintMod, but a
  * Solver aware of this specific kind of RowConstraint can react in
  * specialized ways. */

 virtual void set_rhs( c_RHSValue rhs_value ,
		       c_ModParam issueMod = eModBlck ) override final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set the LHS of this to UBConstraint: this throws exception

 virtual void set_lhs( c_RHSValue lhs_value ,
		       c_ModParam issueMod = eModBlck ) override final {
  if( lhs_value > - std::numeric_limits<RHSValue>::infinity() )
   throw( std::invalid_argument( "cannot change LHS in a UBConstraint" ) );
   }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set both the LHS and the RHS of this to LBConstraint

 virtual void set_both( c_RHSValue both_value ,
			c_ModParam issueMod = eModBlck ) override final {
  if( both_value <= - std::numeric_limits<RHSValue>::infinity() )
   set_rhs( both_value , issueMod );
  else
   throw( std::invalid_argument( "cannot change LHS in a UBConstraint" ) );
  }

/*@} -----------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE DATA OF THE UBConstraint ------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the UBConstraint
    @{ */

 /// method to get the RHS of the UBConstraint
 virtual RHSValue get_rhs( void ) const override final { return( f_rhs ); }

 /// method to get the LHS of the LBConstraint
 virtual RHSValue get_lhs( void ) const override final {
  return( - std::numeric_limits<RHSValue>::infinity() );
  }

/*@} -----------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF A UBConstraint ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a UBConstraint
    @{ */

 virtual bool feasible( void ) const override final {
  return( ( f_rhs >= Inf<double>() ) ||
	  ( f_variable->get_value() <= f_rhs ) );
  }

/*--------------------------------------------------------------------------*/

 virtual RHSValue abs_viol( void ) const override final {
  if( f_rhs >= Inf<double>() )
   return( - Inf<double>() );
  
  c_RHSValue val = f_variable->get_value();
  return( val >= Inf<double>() ? Inf<double>() : val - f_rhs );
  }

/*--------------------------------------------------------------------------*/

 virtual RHSValue rel_viol( void ) const override final {
  if( f_rhs >= Inf<double>() )
   return( -Inf<double>() );

  c_RHSValue val = f_variable->get_value();
  if( val <= -Inf<double>() )
   return( -Inf<double>() );

  if( val <= -Inf<double>() )
   return( Inf<double>() );

  return( f_rhs == 0 ? val - f_rhs : ( val - f_rhs ) / std::abs( f_rhs ) );
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

 /// print information about the UBConstraint on an ostream
 virtual void print( std::ostream &output ) const override {
  output << "UBConstraint [" << this << "] of Block [" << f_Block
	 << "] with ColVariable [" << f_variable << "], RHS = "
	 << f_rhs << ", value = " << value() << std::endl;
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 RHSValue f_rhs;        ///< RHS of the UBConstraint

/*--------------------------------------------------------------------------*/

 };  // end( class( UBConstraint ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS NNConstraint -----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// an OneVarConstraint with LHS = 0 and RHS = +Infty
/** The class UBConstraint deriveds from OneVarConstraint represent a box
 * constraint with fixed LHS = 0 and RHS = +Infty (a Non-Negativity
 * constraint). */

class NNConstraint : public OneVarConstraint {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*@}------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
    @{ */

 /// constructor of NNConstraint: takes Block * an d ColVariable *
 /** Constructor of NNConstraint. It receives a pointer to the Block to
  * which the BoxConstraint belongs and a pointer to the ColVariable that
  * defines the constraint. Everything has a default (nullptr and nullptr,
  * respectively) so that this can be used as the void constructor. */

 NNConstraint( Block *my_block = nullptr ,
	       ColVariable * const variable = nullptr )
  : OneVarConstraint( my_block , variable ) { }

/*--------------------------------------------------------------------------*/
 /// destructor: does nothing special
 virtual ~NNConstraint() { }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the RHS of this to NNConstraint: this throws exception

 virtual void set_rhs( c_RHSValue rhs_value ,
		       c_ModParam issueMod = eModBlck ) override final {
  if( rhs_value < std::numeric_limits<RHSValue>::infinity() )
   throw( std::invalid_argument( "cannot change RHS in a NNConstraint" ) );
   }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set the LHS of this to NNConstraint: this throws exception

 virtual void set_lhs( c_RHSValue lhs_value ,
		       c_ModParam issueMod = eModBlck ) override final {
  if( lhs_value != 0 )
   throw( std::invalid_argument( "cannot change LHS in a NNConstraint" ) );
   }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set both the LHS and the RHS of this to NNConstraint: throws exception

 virtual void set_both( c_RHSValue both_value ,
			c_ModParam issueMod = eModBlck ) override final {
  throw( std::invalid_argument( "LHS == RHS impossible in a NNConstraint" ) );
  }

/*@} -----------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE DATA OF THE NNConstraint ------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the NNConstraint
    @{ */

 /// method to get the RHS of the NNConstraint
 virtual RHSValue get_rhs( void ) const override final {
  return( std::numeric_limits<RHSValue>::infinity() );
  }

 /// method to get the LHS of the NNConstraint
 virtual RHSValue get_lhs( void ) const override final { return( 0 ); }

/*@} -----------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF A NNConstraint ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a NNConstraint
    @{ */

 virtual bool feasible( void ) const override final {
  return( f_variable->get_value() >= 0 );
  }

/*--------------------------------------------------------------------------*/

 virtual RHSValue abs_viol( void ) const override final {
  c_RHSValue val = f_variable->get_value();
  return( val <= -Inf<double>() ? Inf<double>() : - val );
  }

/*--------------------------------------------------------------------------*/

 virtual RHSValue rel_viol( void ) const override final {
  return( NNConstraint::abs_viol() );
  }

/*@}------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

 /// print information about the NNConstraint on an ostream
 virtual void print( std::ostream &output ) const override {
  output << "NNConstraint [" << this << "] of Block [" << f_Block
	 << "] with ColVariable [" << f_variable << "]" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( NNConstraint ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS NPConstraint -----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// an OneVarConstraint with LHS = -Infty and RHS = 0
/** The class UBConstraint deriveds from OneVarConstraint represent a box
 * constraint with fixed LHS = -Infty and RHS = 0 (a Non-Positivity
 * constraint). */

class NPConstraint : public OneVarConstraint {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*@}------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
    @{ */

 /// constructor of NPConstraint: takes Block * an d ColVariable *
 /** Constructor of NPConstraint. It receives a pointer to the Block to
  * which the BoxConstraint belongs and a pointer to the ColVariable that
  * defines the constraint. Everything has a default (nullptr and nullptr,
  * respectively) so that this can be used as the void constructor. */

 NPConstraint( Block *my_block = nullptr ,
	       ColVariable * const variable = nullptr )
  : OneVarConstraint( my_block , variable ) { }

/*--------------------------------------------------------------------------*/
 /// destructor: does nothing special
 virtual ~NPConstraint() { }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the RHS of this to NPConstraint: this throws exception

 virtual void set_rhs( c_RHSValue rhs_value ,
		       c_ModParam issueMod = eModBlck ) override final {
  if( rhs_value != 0 )
   throw( std::invalid_argument( "cannot change RHS in a NPConstraint" ) );
   }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set the LHS of this to NNConstraint: this throws exception

 virtual void set_lhs( c_RHSValue lhs_value ,
		       c_ModParam issueMod = eModBlck ) override final {
  if( lhs_value > - std::numeric_limits<RHSValue>::infinity() )
   throw( std::invalid_argument( "cannot change LHS in a NPConstraint" ) );
   }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set both the LHS and the RHS of this to NPConstraint: throws exception

 virtual void set_both( c_RHSValue both_value ,
			c_ModParam issueMod = eModBlck ) override final {
  throw( std::invalid_argument( "LHS == RHS impossible in a NPConstraint" ) );
  }

/*@} -----------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE DATA OF THE NPConstraint ------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the NPConstraint
    @{ */

 /// method to get the RHS of the NNConstraint
 virtual RHSValue get_rhs( void ) const override final { return( 0 ); }

 /// method to get the LHS of the NNConstraint
 virtual RHSValue get_lhs( void ) const override final {
  return( - std::numeric_limits<RHSValue>::infinity( ) );
  }

/*@} -----------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF A NPConstraint ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a NPConstraint
    @{ */

 virtual bool feasible( void ) const override final {
  return( f_variable->get_value() <= 0 );
  }

/*--------------------------------------------------------------------------*/

 virtual RHSValue abs_viol( void ) const override final {
  c_RHSValue val = f_variable->get_value();
  return( val >= Inf<double>() ? Inf<double>() : val );
  }

/*--------------------------------------------------------------------------*/

 virtual RHSValue rel_viol( void ) const override final {
  return( NPConstraint::abs_viol() );
  }

/*@}------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

 /// print information about the NPConstraint on an ostream
 virtual void print( std::ostream &output ) const override {
  output << "NPConstraint [" << this << "] of Block [" << f_Block
	 << "] with ColVariable [" << f_variable << "]" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( NPConstraint ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS ZOConstraint -----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// an OneVarConstraint with LHS = 0 and RHS = 1
/** The class UBConstraint deriveds from OneVarConstraint represent a box
 * constraint with fixed LHS = 0 and RHS = 1 (a Zero-One constraint). */

class ZOConstraint : public OneVarConstraint {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*@}------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
    @{ */

 /// constructor of ZOConstraint: takes Block * an d ColVariable *
 /** Constructor of ZOConstraint. It receives a pointer to the Block to
  * which the BoxConstraint belongs and a pointer to the ColVariable that
  * defines the constraint. Everything has a default (nullptr and nullptr,
  * respectively) so that this can be used as the void constructor. */

 ZOConstraint( Block *my_block = nullptr ,
	       ColVariable * const variable = nullptr )
  : OneVarConstraint( my_block , variable ) { }

/*--------------------------------------------------------------------------*/
 /// destructor: does nothing special
 virtual ~ZOConstraint() { }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the RHS of this to ZOConstraint: this throws exception

 virtual void set_rhs( c_RHSValue rhs_value ,
		       c_ModParam issueMod = eModBlck ) override {
  if( rhs_value != 1 )
   throw( std::invalid_argument( "cannot change RHS in a ZOConstraint" ) );
   }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set the LHS of this to ZOConstraint: this throws exception

 virtual void set_lhs( c_RHSValue lhs_value ,
		       c_ModParam issueMod = eModBlck ) override {
  if( lhs_value != 0 )
   throw( std::invalid_argument( "cannot change LHS in a ZOConstraint" ) );
   }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set both the LHS and the RHS of this to ZOConstraint: throws exception

 virtual void set_both( c_RHSValue both_value ,
			c_ModParam issueMod = eModBlck ) override final {
  throw( std::invalid_argument( "LHS == RHS impossible in a ZOConstraint" ) );
  }

/*@} -----------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE DATA OF THE NNConstraint ------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the ZOConstraint
    @{ */

 /// method to get the RHS of the ZOConstraint
 virtual RHSValue get_rhs( void ) const override final { return( 1 ); }

 /// method to get the LHS of the ZOConstraint
 virtual RHSValue get_lhs( void ) const override final { return( 0 ); }

/*@} -----------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF A ZOConstraint ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a ZOConstraint
    @{ */

 virtual bool feasible( void ) const override final {
  c_RHSValue val = f_variable->get_value();
  return( ( val >= 0 ) && ( val <= 1 ) );
  }

/*--------------------------------------------------------------------------*/

 virtual RHSValue abs_viol( void ) const override final {
  c_RHSValue val = f_variable->get_value();
  return( std::max( - val , val - 1 ) );
  }

/*--------------------------------------------------------------------------*/

 virtual RHSValue rel_viol( void ) const override final {
  return( ZOConstraint::abs_viol() );
  }

/*@}------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

 /// print information about the ZOConstraint on an ostream
 virtual void print( std::ostream &output ) const override {
  output << "ZOConstraint [" << this << "] of Block [" << f_Block
	 << "] with ColVariable [" << f_variable << "]" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( ZOConstraint ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS OneVarConstraintMod -------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modifications specific to a OneVarConstraint
/** Derived class from RowConstraintMod to describe Modification specific to
 * a OneVarConstraint, i.e., changing the Variable.
 *
 * Defining a class is a bit weird because the only thing the class does is
 * to define am enum for the new value of the type of Modification in the
 * RowConstraintMod. However, throwing a Modification of a different class
 * (but derived from RowConstraintMod) may make it easier for the solver to
 * handle it (at the very least, it directly knows it comes from a
 * OneVarConstraint without having to check it). */

class OneVarConstraintMod : public RowConstraintMod
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/
 /// public enum for the types of OneVarConstraintMod thrown
 /** Public enum "extending" RowC_mod_type with the new types of Modification
  * thrown by OneVarConstraint. */

 enum OVC_cons_mod_type {
  eVariableChanged = eRowConstModLastParam ,
  ///< the Function underlying this OneVarConstraint changed whole

  eOVCConstModLastParam
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of types of Modification. */
  };

/*---------------------- CONSTRUCTOR & DESTRUCTOR --------------------------*/

 /// constructor: just calls that of RowConstraintMod

 OneVarConstraintMod( OneVarConstraint *cnst , int mod = eVariableChanged ,
		      const bool cB = true )
  : RowConstraintMod( cnst , mod , cB ) { }

 virtual ~OneVarConstraintMod() { }  ///< destructor: does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the OneVarConstraintMod

 virtual inline void print( std::ostream &output ) const {
  output << "OneVarConstraintMod on OneVarConstraint["
	 << f_constraint << "]: changing the Variable" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( OneVarConstraintMod ) )

/*--------------------------------------------------------------------------*/

/*@}  end( group( OneVarConstraint_CLASSES ) ) -----------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* OneVarConstraint.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File OneVarConstraint.h ------------------------*/
/*--------------------------------------------------------------------------*/
