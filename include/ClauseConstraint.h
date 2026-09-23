/*--------------------------------------------------------------------------*/
/*------------------------ File ClauseConstraint.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the ClauseConstraint class, derived from Constraint, which
 * is a clause of the propositional logic, i.e., the disjunction of a number
 * of literals, each being a BooleanVariable either as it is or negated, and
 * for the ClauseConstraintMod class, which describes its Modification.
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

#ifndef __ClauseConstraint
 #define __ClauseConstraint
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <utility>
#include <vector>

#include "BooleanVariable.h"
#include "Constraint.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup ClauseConstraint_CLASSES Classes in ClauseConstraint.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS ClauseConstraint --------------------------*/
/*--------------------------------------------------------------------------*/
/// a clause of the propositional logic, i.e., a disjunction of literals
/** The ClauseConstraint class, derived from Constraint, represents a clause
 * of the propositional logic, i.e., the disjunction
 * \f[
 *   \bigvee_{i \in P} x_i \;\vee\; \bigvee_{i \in N} \neg x_i
 * \f]
 * of a number of literals, each literal being a BooleanVariable [see
 * BooleanVariable.h] x_i either as it is (i in P) or negated (i in N). The
 * clause is satisfied if at least one of its literals is true, hence the
 * empty clause is never satisfied.
 *
 * A literal is a Literal, i.e., a pair whose first element is the pointer
 * to the BooleanVariable and whose second element is true if the literal is
 * the negation of the BooleanVariable. The same BooleanVariable can appear
 * in at most one literal of the clause (a clause holding both x and not x
 * is always satisfied and should not be written): an attempt to put it
 * twice throws an exception. The literals are kept in the order in which
 * they are given, which is the order of the "active" Variable of the
 * ClauseConstraint [see ThinVarDepInterface].
 *
 * Since the ClauseConstraint is the Constraint of the satisfiability
 * problems, which are solved by SAT solvers rather than by :MILPSolver,
 * it has no dual value; a Block that uses it is expected to offer, if a
 * :MILPSolver is to solve it, a reformulation of itself where the clause is
 * the linear constraint
 * \f[
 *   \sum_{i \in P} x_i + \sum_{i \in N} ( 1 - x_i ) \geq 1
 * \f]
 * on binary ColVariable [see Block::get_R3_Block()]. */

class ClauseConstraint : public Constraint
{
/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public types
 *  @{ */

 /// a literal: a BooleanVariable, and true if it appears negated
 using Literal = std::pair< BooleanVariable * , bool >;

 using v_Literal = std::vector< Literal >;  ///< a vector of Literal

 using c_v_Literal = const v_Literal;       ///< a const vector of Literal

/** @} ---------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

 /// constructor, taking the Block it belongs to and the literals
 /** Constructor of ClauseConstraint. It takes the pointer to the Block to
  * which the ClauseConstraint belongs and the literals of the clause, both
  * with a default (nullptr and none) so that this can be used as the void
  * constructor; no Modification is issued. */

 explicit ClauseConstraint( Block * my_block = nullptr ,
			    v_Literal && literals = {} )
  : Constraint( my_block ) {
  set_literals( std::move( literals ) , eNoMod );
  }

/*--------------------------------------------------------------------------*/
 /// destructor: removes the ClauseConstraint from its BooleanVariable

 ~ClauseConstraint() override;

/*--------------------------------------------------------------------------*/
 /// forgets the literals without updating the BooleanVariable
 /** Empties the set of literals without removing the ClauseConstraint from
  * the active stuff of its BooleanVariable, which is what one wants when
  * the BooleanVariable and the ClauseConstraint are about to be destroyed
  * together [see ThinVarDepInterface::clear()]. */

 void clear( void ) override { v_literals.clear(); }

/** @} ---------------------------------------------------------------------*/
/*---------------- METHODS FOR MODIFYING THE ClauseConstraint --------------*/
/*--------------------------------------------------------------------------*/
/** @name Modifying the ClauseConstraint
 *  @{ */

 /// sets the literals of the clause, replacing the current ones
 /** Replaces the whole set of literals of the clause with the given one.
  * The parameter issueMod decides if and how the ClauseConstraintMod (of
  * type eLiteralsChanged) is issued, as described in Observer::make_par().
  * An exception is thrown if a BooleanVariable appears twice. */

 void set_literals( v_Literal && literals , ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// adds literals to the clause, after the current ones
 /** Adds the given literals at the end of the clause, issuing (according
  * to issueMod) a ClauseConstraintMod of type eLiteralsAdded. An exception
  * is thrown if a BooleanVariable appears twice, among the new literals or
  * between them and the old ones, in which case nothing changes. */

 void add_literals( v_Literal && literals , ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// removes the i-th literal from the clause
 /** Removes the i-th literal of the clause, i.e., the i-th "active"
  * BooleanVariable, issuing (according to issueMod) a ClauseConstraintMod
  * of type eLiteralsRemoved. */

 void remove_variable( Index i , ModParam issueMod = eModBlck ) override;

/** @} ---------------------------------------------------------------------*/
/*--------------- METHODS FOR READING THE ClauseConstraint -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the ClauseConstraint
 *  @{ */

 /// returns the literals of the clause

 [[nodiscard]] c_v_Literal & get_literals( void ) const {
  return( v_literals );
  }

/*--------------------------------------------------------------------------*/
 /// returns the value of a literal out of that of its BooleanVariable

 static bool literal_value( const Literal & literal ) {
  return( literal.first->get_value() != literal.second );
  }

/*--------------------------------------------------------------------------*/
 /// evaluates the clause at the current values of its BooleanVariable
 /** Counts the true literals of the clause, which feasible() and
  * get_num_true_literals() then report; it always returns kOK. */

 int compute( bool changedvars = true ) override;

/*--------------------------------------------------------------------------*/
 /// returns true if the clause was satisfied at the last compute()
 /** Returns true if at least one literal was true at the last call to
  * compute(), or if the ClauseConstraint is relaxed. */

 [[nodiscard]] bool feasible( void ) const override {
  return( is_relaxed() || ( f_num_true > 0 ) );
  }

/*--------------------------------------------------------------------------*/
 /// returns the number of true literals at the last compute()

 [[nodiscard]] Index get_num_true_literals( void ) const {
  return( f_num_true );
  }

/** @} ---------------------------------------------------------------------*/
/*------------- METHODS FOR HANDLING THE "ACTIVE" Variable -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the set of "active" Variable
 *
 * The "active" Variable of a ClauseConstraint are the BooleanVariable of
 * its literals, in the order of the literals.
 *  @{ */

 [[nodiscard]] Index get_num_active_var( void ) const override {
  return( v_literals.size() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 Index is_active( const Variable * var ) const override {
  for( Index i = 0 ; i < v_literals.size() ; ++i )
   if( v_literals[ i ].first == var )
    return( i );
  return( Inf< Index >() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] Variable * get_active_var( Index i ) const override {
  return( v_literals[ i ].first );
  }

/*--------------------------------------------------------------------------*/
 /// virtualized concrete iterator over the BooleanVariable of the literals

 class v_iterator : public ThinVarDepInterface::v_iterator
 {
  public:

  explicit v_iterator( v_Literal::iterator itr ) : itr_( itr ) {}

  v_iterator * clone( void ) final { return( new v_iterator( itr_ ) ); }

  void operator++( void ) final { ++itr_; }
  reference operator*( void ) const final { return( *( itr_->first ) ); }
  pointer operator->( void ) const final { return( itr_->first ); }

  bool operator==( const ThinVarDepInterface::v_iterator & rhs )
   const final {
   auto tmp = dynamic_cast< const ClauseConstraint::v_iterator * >( & rhs );
   return( tmp ? itr_ == tmp->itr_ : false );
   }

  bool operator!=( const ThinVarDepInterface::v_iterator & rhs )
   const final {
   return( ! ( *this == rhs ) );
   }

  private:

  v_Literal::iterator itr_;
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// virtualized concrete const_iterator over the literals' BooleanVariable

 class v_const_iterator : public ThinVarDepInterface::v_const_iterator
 {
  public:

  explicit v_const_iterator( v_Literal::const_iterator itr ) : itr_( itr ) {}

  v_const_iterator * clone( void ) final {
   return( new v_const_iterator( itr_ ) );
   }

  void operator++( void ) final { ++itr_; }
  reference operator*( void ) const final { return( *( itr_->first ) ); }
  pointer operator->( void ) const final { return( itr_->first ); }

  bool operator==( const ThinVarDepInterface::v_const_iterator & rhs )
   const final {
   auto tmp = dynamic_cast< const ClauseConstraint::v_const_iterator * >(
								     & rhs );
   return( tmp ? itr_ == tmp->itr_ : false );
   }

  bool operator!=( const ThinVarDepInterface::v_const_iterator & rhs )
   const final {
   return( ! ( *this == rhs ) );
   }

  private:

  v_Literal::const_iterator itr_;
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 ThinVarDepInterface::v_iterator * v_begin( void ) override {
  return( new v_iterator( v_literals.begin() ) );
  }

 [[nodiscard]] ThinVarDepInterface::v_const_iterator * v_begin( void )
  const override {
  return( new v_const_iterator( v_literals.begin() ) );
  }

 ThinVarDepInterface::v_iterator * v_end( void ) override {
  return( new v_iterator( v_literals.end() ) );
  }

 [[nodiscard]] ThinVarDepInterface::v_const_iterator * v_end( void )
  const override {
  return( new v_const_iterator( v_literals.end() ) );
  }

/** @} ---------------------------------------------------------------------*/
/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

 /// print the ClauseConstraint

 void print( std::ostream & output ) const override;

/*--------------------------------------------------------------------------*/
 /// throws if a BooleanVariable is null or appears twice in literals, or in
 /// literals and the current literals together

 void check_literals( c_v_Literal & literals , bool with_current ) const;

/*--------------------------------------------------------------------------*/
 /// issues a ClauseConstraintMod of the given type, according to issueMod

 void issue( int type , ModParam issueMod );

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS -----------------------------*/
/*--------------------------------------------------------------------------*/

 v_Literal v_literals;  ///< the literals of the clause

 Index f_num_true = 0;  ///< number of true literals at the last compute()

/*--------------------------------------------------------------------------*/

};  // end( class( ClauseConstraint ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS ClauseConstraintMod -------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe the Modification specific to a ClauseConstraint
/** Derived class from ConstraintMod to describe the Modification specific
 * to a ClauseConstraint, i.e., changing its literals. */

class ClauseConstraintMod : public ConstraintMod
{
/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the types of ClauseConstraintMod, "extending" cons_mod_type
 enum clause_mod_type {
  eLiteralsChanged = eConstModLastParam ,
  ///< the whole set of literals has been replaced
  eLiteralsAdded ,
  ///< some literals have been added at the end of the clause
  eLiteralsRemoved ,
  ///< some literals have been removed from the clause
  eClauseConstModLastParam
  ///< first allowed parameter value for derived classes
  };

/*--------------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/

 /// constructor: takes the ClauseConstraint and the type of Modification

 explicit ClauseConstraintMod( ClauseConstraint * cnst ,
			       int type = eLiteralsChanged , bool cB = true )
  : ConstraintMod( cnst , type , cB ) {}

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~ClauseConstraintMod() override = default;  ///< destructor: does nothing

/*--------------------------------------------------------------------------*/
/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

 /// print the ClauseConstraintMod

 void print( std::ostream & output ) const override {
  output << "ClauseConstraintMod[" << ( concerns_Block() ? "t" : "f" )
	 << "] on ClauseConstraint [" << f_constraint << "]: ";
  switch( f_type ) {
   case( eLiteralsChanged ): output << "literals changed"; break;
   case( eLiteralsAdded ):   output << "literals added"; break;
   case( eLiteralsRemoved ): output << "literals removed"; break;
   case( eRelaxConst ):      output << "relaxing"; break;
   case( eEnforceConst ):    output << "enforcing"; break;
   default:                  output << "type " << f_type;
   }
  output << std::endl;
  }

/*--------------------------------------------------------------------------*/

};  // end( class( ClauseConstraintMod ) )

/** @} end( group( ClauseConstraint_CLASSES ) ) ----------------------------*/
/*--------------------------------------------------------------------------*/

}  /* namespace SMSpp_di_unipi_it */

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* ClauseConstraint.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File ClauseConstraint.h ------------------------*/
/*--------------------------------------------------------------------------*/
