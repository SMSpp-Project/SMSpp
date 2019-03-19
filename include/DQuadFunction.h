/*--------------------------------------------------------------------------*/
/*------------------------- File DQuadFunction.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *concrete* class DQuadFunction, which
 * implements C15Function with a diagonal quadratic function.
 *
 * \version 0.20
 *
 * \date 04 - 03 - 2019
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __DQuadFunction
 #define __DQuadFunction  /* self-identification: #endif at the end
                            * of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "C15Function.h"
#include "ColVariable.h"
#include "Observer.h"
#include <math.h>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup DQuadFunction_CLASSES Classes in DQuadFunction.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS DQuadFunction ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a diagonal quadratic Function
/**< The class DQuadFunction implements C15Function with a diagonal
 * quadratic function of the form
 * \[
 * f(x) = c + sum{i in I} ( a_i * x_i * x_i + b_i * x_i )
 * \]
 * where the scalar c is the constant term of the Function, and a_i
 * and b_i are the coefficients of the Variable x_i in the quadratic
 * and linear terms, respectively, for all i in I.
 *
 * This Function issues the following modifications:
 *
 * - When Variables are added, a DQuadFunctionModSbst
 *   (C05FunctionModVarsSbst of type AddVar) is issued; pointers to
 *   the Variables that were added are provided in the Modification.
 *
 * - When Variables are removed, a either a DQuadFunctionModSbst
 *   (C05FunctionModVarsSbst of type RemoveVar) or a
 *   DQuadFunctionModRngd (C05FunctionModVarsRngd of type RemoveVar)
 *   is issued. Pointers to the Variables that were removed are
 *   provided in the Modification.
 *
 * - When the coefficients of some Variables change, either a
 *   DQuadFunctionModSbst (C05FunctionModVarsSbst of type
 *   SomeEntriesChange) or a DQuadFunctionModRngd
 *   (C05FunctionModVarsRngd of type SomeEntriesChange) is issued. The
 *   entries of a linearization affected by this modification are
 *   precisely the ones associated with the Variables whose
 *   coefficients have changed.
 *
 * - When the constant term changes, a FunctionMod is issued with the shift
 *   equals to the difference between the new and old constant term values.
 */

class DQuadFunction : public C15Function {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------- PUBLIC TYPES OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 typedef double Coefficient;
 ///< type of the coefficients of the function

 typedef std::vector< std::pair<Coefficient, Coefficient> > v_coeff_coeff;
 ///< a vector of pairs of Coefficients

 typedef const v_coeff_coeff c_v_coeff_coeff;
 ///< a const vector of pairs of Coefficients

 typedef v_coeff_coeff::iterator v_coeff_coeff_it;
 ///< iterator in v_coeff_coeff

 typedef v_coeff_coeff::const_iterator c_v_coeff_coeff_it;
 ///< const iterator in v_coeff_coeff

 typedef std::tuple<ColVariable *, Coefficient, Coefficient>
 var_coeff_coeff_triple;
 /**< Triple (ColVariable *, Coefficient, Coefficient) to store a
  * pointer to a Variable and the coefficients of the linear and
  * diagonal quadratic terms associated with that Variable. */

 typedef std::vector<var_coeff_coeff_triple> v_var_coeff_coeff_triple;
 ///< a vector of var_coeff_coeff_triple

 typedef const var_coeff_coeff_triple c_var_coeff_coeff_triple;
 ///< a const var_coeff_coeff_triple

 typedef const v_var_coeff_coeff_triple v_c_var_coeff_coeff_triple;
 ///< a vector of const var_coeff_coeff_triple

/*--------------------------------------------------------------------------*/
 /// virtualized concrete iterator
 /** A concrete class deriving from ThinVarDepInterface::v_iterator and
  * implementing the concrete iterator for sifting through the "active"
  * Variable of a DQuadFunction. */

 class v_iterator : public ThinVarDepInterface::v_iterator
 {
  public:

  v_iterator( v_var_coeff_coeff_triple::iterator itr ) : itr_( itr ) { }

  virtual void operator++( void ) override final { (itr_)++; }
  virtual reference operator*( void ) const override final {
   return( *( std::get<0>( (*itr_) ) ) );
   }
  virtual pointer operator->( void ) const override final {
   return( std::get<0>( (*itr_) ) );
   }
  virtual bool operator==( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast<const DQuadFunction::v_iterator *>( & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const DQuadFunction::v_iterator *>( & rhs );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }
  virtual bool operator!=( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast<const DQuadFunction::v_iterator *>( & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const DQuadFunction::v_iterator *>( & rhs );
    return( tmp ? itr_ != tmp->itr_ : false );
   #endif
   }

  private:

  v_var_coeff_coeff_triple::iterator itr_;
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// virtualized concrete const_iterator
 /** A concrete class deriving from ThinVarDepInterface::v_const_iterator and
  * implementing the concrete iterator for sifting through the "active"
  * Variable of a DQuadFunction. */

 class v_const_iterator : public ThinVarDepInterface::v_const_iterator
 {
  public:

  v_const_iterator( v_c_var_coeff_coeff_triple::const_iterator itr )
   : itr_( itr ) { }

  virtual void operator++( void ) override final { (itr_)++; }
  virtual reference operator*( void ) const override final {
   return( *( std::get<0>( (*itr_) ) ) );
   }
  virtual pointer operator->( void ) const override final {
   return( std::get<0> ( (*itr_) ) );
   }
  virtual bool operator==( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast<const DQuadFunction::v_const_iterator *>( & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const DQuadFunction::v_const_iterator *>( & rhs );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }
  virtual bool operator!=( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast<const DQuadFunction::v_const_iterator *>( & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const DQuadFunction::v_const_iterator *>( & rhs );
    return( tmp ? itr_ != tmp->itr_ : false );
   #endif
   }

  private:

  v_var_coeff_coeff_triple::const_iterator itr_;
  };

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and Destructor
 *  @{ */
 /** constructor of DQuadFunction, taking the the coefficients of the
  * linear and diagonal quadratic terms. It accepts a pointer to a
  * vector of triples < pointer to ColVariable , real coefficient ,
  * real coefficient > the first coefficient being the coefficient of
  * the variable in the linear term and the second being the
  * coefficient of the variable in the diagonal quadratic term of the
  * function (and a bool telling if this vector is already ordered by
  * ColVariable "name = pointer" or not, in which case they are ordered).
  *
  * Note that if this vector of triples is passed, it becomes property
  * of the DQuadFunction, which therefore has the responsibility to
  * delete it.
  *
  * All inputs have a default (nullptr, false respectively), so that
  * this can be used as the void constructor. */

 DQuadFunction(v_var_coeff_coeff_triple && v_var = {} ,
	       const FunctionValue ct = 0 ,
	       const bool ordered = false)
  : C15Function() , v_triples( std::move( v_var ) ) ,
    f_value( Inf<FunctionValue>() ) , f_constant_term( ct )
 {
  if( ! ordered )
   std::sort( v_triples.begin() , v_triples.end() ,
	      []( const auto & p1, const auto & p2 ) {
		return( std::get<0>( p1 ) < std::get<0>( p2 ) );
	      }
	      );
  }

/*--------------------------------------------------------------------------*/
 /// destructor: it does nothing (explicitly)

 virtual ~DQuadFunction() {}

/*--------------------------------------------------------------------------*/
 /// clear method: clears the v_triples field
 /** Method to "clear" the DQuadFunction: it clear() the vector v_triples.
  * This destroys the list of "active" Variable without unregistering from
  * them. Not that the LinearFunction would have, but the Observer may.
  * By not having any Variable, the Observer can no longer do that. */

 virtual void clear( void ) override {
  v_triples.clear();
  f_Observer = nullptr;
  }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the whole (empty) set of parameters in one blow
 /** Although a DQuadFunction formally has a lot of parameters, in fact it
  * "listens to no-one"; hence, the implementation of set_ComputeConfig() is
  * quite a trivial one. */

 virtual void set_ComputeConfig( ComputeConfig *scfg = nullptr )
  override final { }

/*@} -----------------------------------------------------------------------*/
/*----------- METHODS FOR READING THE DATA OF THE DQuadFunction ------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the DQuadFunction
    @{ */

 /// returns the vector of triples (ColVariable *, Coefficient, Coefficient)

 v_c_var_coeff_coeff_triple & get_v_var( void ) const {
  return( v_triples );
  }

/*@} -----------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE DQuadFunction -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the DQuadFunction
 *  @{ */

 virtual int compute( bool changedvars = true ) override;

/*--------------------------------------------------------------------------*/

 virtual FunctionValue get_value( void ) const override { return( f_value ); }

/*--------------------------------------------------------------------------*/

 virtual bool is_convex( void ) const override;

/*--------------------------------------------------------------------------*/

 virtual bool is_concave( void ) const override;

/*--------------------------------------------------------------------------*/

 virtual bool is_lower_semicontinuous( void ) const override final {
  return( true ); }

/*--------------------------------------------------------------------------*/

 virtual bool is_upper_semicontinuous( void ) const override final {
  return( true ); }

/*--------------------------------------------------------------------------*/

 virtual bool is_linear( void ) const override;

/*--------------------------------------------------------------------------*/

 virtual void compute_hessian_approximation( void ) override { };

/*--------------------------------------------------------------------------*/

 virtual void get_hessian_approximation( DenseHessian &hessian ) const
  override;

/*--------------------------------------------------------------------------*/

 virtual void get_hessian_approximation( SparseHessian &hessian )
   const override;

/*--------------------------------------------------------------------------*/

 virtual bool is_continuously_differentiable( void ) const override final {
  return( true );
  }

/*--------------------------------------------------------------------------*/

 virtual bool is_twice_continuously_differentiable( void )
   const override final { return true; }

/*--------------------------------------------------------------------------*/

/// Returns the linearization coefficient of the i-th active Variable
 virtual double get_linearization_coefficient( c_Index i ) const;

/*--------------------------------------------------------------------------*/

 /// get an arbitrary subset of linearization coefficients
 /** Like get_linearization_coefficients( FunctionValue *g , name ,
  * ... ) but without the name, because a DQuadFunction only have one
  * linearization. */

 void get_linearization_coefficients( FunctionValue * g ,
				      c_Vec_Index * const indices ,
				      c_Index start , c_Index end ) const;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get a range of linearization coefficients
 /** Like get_linearization_coefficients( FunctionValue * g , name ,
  * ... ) but without the name, because a DQuadFunction only have one
  * linearization, and without the arbitrary subset. */

 void get_linearization_coefficients( FunctionValue * g ,
				      c_Index start , c_Index end ) const;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual void get_linearization_coefficients( FunctionValue * g ,
   const LinearizationName name =
                              std::numeric_limits<LinearizationName>::max() ,
   c_Vec_Index * const indices = nullptr , c_Index start = 0 ,
   c_Index end = std::numeric_limits<Index>::max() ) override final;

/*--------------------------------------------------------------------------*/

 virtual void get_linearization_coefficients( SparseVector &g ,
   const LinearizationName name =
                              std::numeric_limits<LinearizationName>::max() ,
   c_Vec_Index * const indices = nullptr , c_Index start = 0 ,
   c_Index end = std::numeric_limits<Index>::max() ) override final;

/*--------------------------------------------------------------------------*/
 /** There is only one linearization in a DQuadFunction. The value of the
  * linearization constant for this diagonal quadratic function is
  * given by c - x'Ax. */

 virtual double get_linearization_constant( const LinearizationName name =
			      std::numeric_limits<Index>::max() ) override {
  double quadratic_term = 0.0;

  for( const auto &triple : v_triples ) {
   auto variable_value = std::get<0>( triple )->get_value();
   auto quadratic_coefficient_value = std::get<2>( triple );
   quadratic_term += variable_value * quadratic_coefficient_value *
    quadratic_coefficient_value;
   }

  return( this->f_constant_term - quadratic_term );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 ///< Returns the value of the constant term of this DQuadFunction.

 FunctionValue get_constant_term( void ) const { return( f_constant_term ); }

/*--------------------------------------------------------------------------*/
 /// returns the Coefficient in the linear term of the i-th Variable
 /** This method returns the Coefficient, in the linear term, of the
  * i-th Variable of this diagonal quadratic function. The index i
  * must be between 0 and get_num_active_var() - 1.
  *
  * @param i Index of the Variable whose coefficient is desired.
  */

 virtual Coefficient get_linear_coefficient( c_Index i ) const {
  return( std::get<1>( *( v_triples.begin() + i ) ) );
  }

/*--------------------------------------------------------------------------*/
 /// returns the Coefficient in the quadratic term of the i-th Variable
 /** This method returns the Coefficient, in the quadratic term, of
  * the i-th Variable of this diagonal quadratic function. The index i
  * must be between 0 and get_num_active_var() - 1.
  *
  * @param i Index of the Variable whose coefficient is desired.
  */

 virtual Coefficient get_quadratic_coefficient( c_Index i ) const {
  return( std::get<2>( *( v_triples.begin() + i ) ) );
  }

/*@} -----------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the DQuadFunction
 *  @{ */

 ///< get the whole (empty) set of parameters in one blow
 /** Although a DQuadFunction formally has a lot of parameters, in fact it
  * "listens to no-one"; hence, the implementation of get_ComputeConfig() is
  * quite a trivial one. */

 virtual ComputeConfig * get_ComputeConfig( bool all = false ,
		       ComputeConfig * ocfg = nullptr ) const override final
 {
  return( nullptr );
  }

/*@} -----------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE DQuadFunction --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" Variable in the
 * DQuadFunction; this is the actual concrete implementation exploiting the
 * vector v_triples of triples.
 * @{ */

 virtual Index get_num_active_var( void ) const override final {
  return( v_triples.size() );
  }

/*--------------------------------------------------------------------------*/

 virtual Index is_active( const Variable * const var )
  const override final {

  auto idx = std::lower_bound
    ( v_triples.begin() , v_triples.end() , std::make_tuple( var , 0 , 0 ) ,
      []( const auto & p1, const auto & p2 )
      { return std::get<0>( p1 ) < std::get<0>( p2 ); } );

  if( idx < v_triples.end() )
   return( std::distance( idx , v_triples.begin() ) );
  else
   return( std::numeric_limits<Index>::infinity() );
 }

/*--------------------------------------------------------------------------*/

 virtual void map_active( c_Vec_p_Var & vars , Vec_Index & map ,
			  const bool ordered = false ) const override final;

/*--------------------------------------------------------------------------*/

 virtual Variable *get_active_var( const Index i ) const override {
  return( std::get<0>( *( v_triples.begin() + i ) ) );
  }

/*--------------------------------------------------------------------------*/

 virtual v_iterator * v_begin( void ) override final {
  return( new DQuadFunction::v_iterator( v_triples.begin() ) );
  }

/*--------------------------------------------------------------------------*/

 virtual v_const_iterator * v_begin( void ) const override final {
  return( new DQuadFunction::v_const_iterator( v_triples.begin() ) );
  }

/*--------------------------------------------------------------------------*/

 virtual v_iterator * v_end( void ) override final {
  return( new DQuadFunction::v_iterator( v_triples.end() ) );
  }

/*--------------------------------------------------------------------------*/

 virtual v_const_iterator * v_end( void ) const override final {
  return( new DQuadFunction::v_const_iterator( v_triples.end() ) );
  }

/*@} -----------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE DQuadFunction -------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the DQuadFunction
 *  @{ */

 /// add a set of new Variable to the DQuadFunction
 /**< Method that receives a pointer to a vector of triples
  * < ColVariable * , Coefficient , Coefficient > and adds these triples
  * to the list of triples already in the DQuadFunction. The first
  * coefficient is that in the linear term and the second one is that
  * in the quadratic term. If any variable in vars is already an
  * active variable in the DQuadFunction, an exception is thrown. As
  * the the && tells, vars is "consumed" by the method and its
  * resources become property of the DQuadFunction object (which may
  * dispatch them to the Modification that it may issue).
  *
  * The parameter ordered tells if vars is already ordered by ColVariable
  * "name = pointer" or not, otherwise it may get ordered inside the
  * method (which is why it is not const).
  *
  * The parameter issueMod decides if and how the C05FunctionModVars is
  * issued, as described in Observer::make_par(). Note that a diagonal
  * quadratic function is additive, and therefore strongly quasi-additive. */

 void add_variables( v_var_coeff_coeff_triple && vars ,
		     const bool ordered = false ,
		     c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// add one single new Variable to the DQuadFunction
 /** Like add_variables(), but just only one Variable. The linear_coeff is
  * the coefficient of the Variable in the linear term and quadratic_coeff
  * is the coefficient of the Variable in the quadratic term. */

 void add_variable( ColVariable * const var , const Coefficient linear_coeff ,
		    const Coefficient quadratic_coeff ,
		    c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// modify a single existing quadratic term
 /** Method that modifies both the linear and the quadratic coefficients for
  * a specific Variable *. If var is not an active variable in the
  * DQuadFunction, exception is thrown.
  *
  * The parameter issueMod decides if and how the C05FunctionModRngd is
  * issued, as described in Observer::make_par(). */

 void modify_term( ColVariable * const var , const Coefficient linear_coeff ,
		   const Coefficient quadratic_coeff ,
		   c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify a single existing coefficient
 /** Method that modifies only the linear coefficient for a specific
  * Variable, leaving the quadratic one unchanged; if var is not an active
  * variable in the LinearFunction, exception is thrown. 
  *
  * The parameter issueMod decides if and how the C05FunctionModLin is issued,
  * as described in Observer::make_par(). */

 void modify_linear_coefficient( ColVariable * const var ,
				 const Coefficient coeff ,
				 c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify a set of existing quadratic terms
 /** Method that receives a vector of triples < ColVariable * ,
  * Coefficient , Coefficient > so that the ColVariables are already
  * in the DQuadFunction, and modify their coefficients
  * accordingly. If any ColVariable in vars is not an active variable
  * in the DQuadFunction, exception is thrown. As the the && tells,
  * vars is "consumed" by the method and its resources become property
  * of the DQuadFunction object (which may dispatch them to the
  * Modification that it may issue).
  *
  * The parameter ordered tells if vars is already ordered by ColVariable
  * "name = pointer" or not, otherwise it may get ordered inside the
  * method (which is why it is not const).
  *
  * The parameter issueMod decides if and how the DQuadFunctionMod is
  * issued, as described in Observer::make_par(). */

 void modify_terms( v_var_coeff_coeff_triple && vars ,
			   const bool ordered = false ,
			   c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify a set of existing coefficients
 /** Like modify_coefficients( v_var_coeff_coeff_triple * ), but takes
  * in input a set of index of the Variable whose coefficients need be
  * modified, together with (an iterator into) the vector of new
  * coefficient values. The coefficients come as pairs, the first
  * element being the coefficient of the Variable in the linear term
  * and the second element being the coefficient in the quadratic
  * term. Useful if one knows the indices already, so that they need
  * not be searched for. The set of indices must be ordered in
  * increasing sense. */

 virtual void modify_coefficients( c_v_coeff_coeff_it NCoef ,
				   c_Vec_Index &nms ,
				   c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify a range of coefficients
 /** Modify the coefficients of all the Variable that are in position
  * from start (included) to min( stop , get_num_active_var() )
  * (excluded) in this DQuadFunction. NCoef is a (const) iterator into
  * a vector of coefficients that must clearly be at least as long
  * (from NCoef to the end) as min( stop , get_num_active_var() ) -
  * start. The coefficients come as pairs, the first element being the
  * coefficient of the Variable in the linear term and the second
  * element being the coefficient in the quadratic term. Useful if one
  * knows the indices already, so that they need not be searched
  * for. */

 void modify_coefficients( c_v_coeff_coeff_it NCoef ,
			   c_Index strt = 0 , Index stop = Inf<Index>() ,
			   c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify a range of coefficients
 /** Modify the coefficients of all the Variable comprised between
  * strt (included) and stop (excluded). Setting strt == nullptr means
  * "the first Variable", and setting stop == nullptr means "(one
  * after) the last Variable". If no-nullptr arguments are provided,
  * they *must* be "names" of Variable currently active in this
  * DQuadFunction. NCoef is a (const) iterator into a vector of pairs
  * of coefficients that must clearly be at least as long (from NCoef
  * to the end) as there are coefficients between the one of strt and
  * the one of stop (these included). The first element of the pair is
  * the coefficient of the Variable in the linear term and the second
  * element is the coefficient in the quadratic term. */

 void modify_coefficients( c_v_coeff_coeff_it NCoef ,
			   const Variable * strt = nullptr ,
			   const Variable * stop = nullptr ,
			   c_ModParam issueMod = eModBlck )
 {
  c_Index istrt = strt ? is_active( strt ) : 0;
  if( istrt >= get_num_active_var() )
   throw( std::invalid_argument( "strt is not an active Variable" ) );

  Index istop;
  if( stop ) {
   istop = is_active( stop );
   if( istop >= get_num_active_var() )
    throw( std::invalid_argument( "stop is not an active Variable" ) );
   }
  else
   istop = get_num_active_var();

  modify_coefficients( NCoef , istrt , istop , issueMod );
  }

/*--------------------------------------------------------------------------*/
 /// remove the given Variable from the DQuadFunction
 /** Remove the given Variable from the DQuadFunction. This is
  * *mathematically* equivalent to setting the coefficients associated
  * with this Variable to zero. */

 virtual void remove_variable( Variable * var ,
			       c_ModParam issueMod = eModBlck )
  override final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// remove the i-th Variable
 /** Like remove_variable( Variable * ), but takes in input the index of
  * the Variable to be removed rather than its pointer. Useful if one knows
  * the index already, so that it need not be searched for. */

 void remove_variable( c_Index i , c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove a range of Variable
 /** Remove all the Variable that are in position from start (included) to
  * min( stop , get_num_active_var() ) (excluded) in this DQuadFunction. */

 void remove_variables( c_Index strt = 0 , Index stop = Inf<Index>() ,
			c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove a range of Variable
 /** Remove all the Variable comprised between strt (included) and stop
  * (excluded). Setting strt == nullptr means "the first Variable", and
  * setting stop == nullptr means "(one after) the last Variable". If
  * no-nullptr arguments are provided, they *must* be "names" of Variable
  * currently active in this DQuadFunction. */

 void remove_variables( const Variable * const strt = nullptr ,
			const Variable * const stop = nullptr ,
			c_ModParam issueMod = eModBlck )
 {
  c_Index istrt = strt ? is_active( strt ) : 0;
  if( istrt >= get_num_active_var() )
   throw( std::invalid_argument( "strt is not an active Variable" ) );

  Index istop;
  if( stop ) {
   istop = is_active( stop );
   if( istrt >= get_num_active_var() )
    throw( std::invalid_argument( "stop is not an active Variable" ) );
   }
  else
   istop = get_num_active_var();

  remove_variables( istrt , istop , issueMod );
  }

/*--------------------------------------------------------------------------*/

 virtual void remove_variables( Vec_p_Var && vars ,
				const bool ordered = false ,
				c_ModParam issueMod = eModBlck )
  override final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// remove a set of Variable by index
 /** Like remove_variables( Vec_p_Var * ), but takes in input a set of index
  * of the Variable to be removed rather than their pointers. Useful if one
  * knows the indices already, so that they need not be searched for. The set
  * of indices must be ordered in increasing sense. */

 virtual void remove_variables( c_Vec_Index & nms ,
				c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 ///< sets the value of the constant term of this function.
 /** Method that sets the new value to the constant term of this
  * diagonal quadratic Function to constant_term.
  *
  * The parameter issueMod decides if and how the DQuadFunctionMod is
  * issued, as described in Observer::make_par(). */

 void set_constant_term( const FunctionValue constant_term ,
			 c_ModParam issueMod = eModBlck );

/*@} -----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

 /// printing the DQuadFunction
 void print( std::ostream &output ) const override {
  output << "DQuadFunction [" << this << "] observed by ["
	 << &f_Observer << "] with " << get_num_active_var()
	 << " active variables;";
  output << "current value = " << get_value();
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 v_var_coeff_coeff_triple v_triples;
 /**< vector of triples (ColVariable *, Coefficient, Coefficient)
  * characterizing the linear and the diagonal quadratic terms of the
  * function; the vector is kept sorted in an ascending order based on
  * the pointers of the ColVariables. The first coefficient is that in
  * the linear term and the second one is that in the quadratic
  * term. */

 FunctionValue f_value;  ///< the value of the function

 FunctionValue f_constant_term = 0;
 ///< the value of the constant term of this function

/*@}------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/

 void issue_add_variables_modification( v_var_coeff_coeff_triple & triples ,
					c_ModParam issueMod );

/*--------------------------------------------------------------------------*/

 };  // end( class( DQuadFunction ) )

/*@}  end( group( DQuadFunction_CLASSES ) ) */
/*--------------------------------------------------------------------------*/

 }  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* DQuadFunction.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File DQuadFunction.h ---------------------------*/
/*--------------------------------------------------------------------------*/
