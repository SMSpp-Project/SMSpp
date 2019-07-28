/*--------------------------------------------------------------------------*/
/*------------------------ File LinearFunction.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *concrete* class LinearFunction, which implements
 * C15Function with a simple linear function.
 *
 * \version 0.20
 *
 * \date 20 - 03 - 2019
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
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __LinearFunction
#define __LinearFunction
/* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "C15Function.h"
#include "ColVariable.h"
#include "Observer.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup LinearFunction_CLASSES Classes in LinearFunction.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS LinearFunction ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a linear Function
/**< The class LinearFunction implements C15Function with a simple linear
 * function of the form
 * \[
 *  f(x) = c + sum_{i \in I} a_i * x_i
 * \]
 * where the scalar c is the constant term of the Function, and a_i is the
 * fixed real coefficient of the Variable x_i, for i in I.
 *
 * This Function issues the following modifications:
 *
 * - When Variables are added or removed, a C05FunctionModVars of appropriate
 *   type (AddVar or RemoveVar) is issued; pointers to the Variables that
 *   were added/removed are provided in the Modification. Note that, being
 *   this a linear Function, the addition/removal is strongly quasi-additive.
 *
 * - When the coefficients of some Variables change, a C05FunctionModLin is
 *   issued; the Modification provides both the set of Variables whose
 *   coefficients have changed and the change in the coefficients. The
 *   shift is NaN, no check on the sign of the Variables / coefficient
 *   change is performed (yet).
 *
 * - When the constant term changes, a FunctionMod is issued with the shift
 *   equals to the difference between the new and old constant term values.
 *
 * TODO: when "a lot" of the coefficients change, rahter issue a
 *       C05FunctionMod of type AllEntriesChanged, because it might be
 *       faster to just re-read all the coefficients than to skip the
 *       few non-changed ones. */

class LinearFunction : public C15Function {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------- PUBLIC TYPES OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */

 typedef FunctionValue Coefficient;
 ///< type of the coefficients of the linear function = FunctionValue

 typedef std::vector< Coefficient > v_coeff;
 ///< a vector of Coefficients

 typedef v_coeff::iterator v_coeff_it;
 ///< iterator in v_coeff

 typedef v_coeff::const_iterator c_v_coeff_it;
 ///< const iterator in v_coeff

 typedef std::pair< ColVariable *, Coefficient > coeff_pair;
 ///< element of a Linear Coefficient matrix: (ColVariable *, Coefficient)

 typedef std::vector< coeff_pair > v_coeff_pair;
 ///< a vector of coeff_pair

 typedef const v_coeff_pair v_c_coeff_pair;
 ///< a const vector of coeff_pair

/*--------------------------------------------------------------------------*/
 /// virtualized concrete iterator
 /** A concrete class deriving from ThinVarDepInterface::v_iterator and
  * implementing the concrete iterator for sifting through the "active"
  * Variable of a LinearFunction. */

 class v_iterator : public ThinVarDepInterface::v_iterator {
  public:

  explicit v_iterator( v_coeff_pair::iterator itr ) : itr_( itr ) {}

  v_iterator * clone() override {
   return new v_iterator( itr_ );
  }

  void operator++() final { itr_++; }

  reference operator*() const final {
   return *( *itr_ ).first;
  }

  pointer operator->() const final {
   return ( *itr_ ).first;
  }

  bool operator==( const ThinVarDepInterface::v_iterator & rhs ) const final {
#ifdef NDEBUG
   auto tmp = static_cast<const LinearFunction::v_iterator *>( & rhs );
   return itr_ == tmp->itr_ ;
#else
   auto tmp = dynamic_cast<const LinearFunction::v_iterator *>( &rhs );
   return tmp ? itr_ == tmp->itr_ : false;
#endif
  }

  bool operator!=( const ThinVarDepInterface::v_iterator & rhs ) const final {
#ifdef NDEBUG
   auto tmp = static_cast<const LinearFunction::v_iterator *>( & rhs );
   return itr_ != tmp->itr_ ;
#else
   auto tmp = dynamic_cast<const LinearFunction::v_iterator *>( &rhs );
   return tmp ? itr_ != tmp->itr_ : true;
#endif
  }

  private:

  v_coeff_pair::iterator itr_;
 };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// virtualized concrete const_iterator
 /** A concrete class deriving from ThinVarDepInterface::v_const_iterator and
  * implementing the concrete iterator for sifting through the "active"
  * Variable of a LinearFunction. */

 class v_const_iterator : public ThinVarDepInterface::v_const_iterator {
  public:

  explicit v_const_iterator( v_c_coeff_pair::const_iterator itr ) :
   itr_( itr ) {}

  v_const_iterator * clone() override {
   return new v_const_iterator( itr_ );
  }

  void operator++() final { itr_++; }

  reference operator*() const final {
   return *( *itr_ ).first;
  }

  pointer operator->() const final {
   return ( *itr_ ).first;
  }

  bool operator==( const ThinVarDepInterface::v_const_iterator & rhs )
  const final {
#ifdef NDEBUG
   auto tmp = static_cast<const LinearFunction::v_const_iterator *>( & rhs );
   return itr_ == tmp->itr_;
#else
   auto tmp = dynamic_cast<const LinearFunction::v_const_iterator *>( &rhs );
   return tmp ? itr_ == tmp->itr_ : false;
#endif
  }

  bool operator!=( const ThinVarDepInterface::v_const_iterator & rhs )
  const final {
#ifdef NDEBUG
   auto tmp = static_cast<const LinearFunction::v_const_iterator *>( & rhs );
   return itr_ != tmp->itr_;
#else
   auto tmp = dynamic_cast<const LinearFunction::v_const_iterator *>( &rhs );
   return tmp ? itr_ != tmp->itr_ : true;
#endif
  }

  private:

  v_coeff_pair::const_iterator itr_;
 };

/**@} ----------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and Destructor
 *  @{ */

 /// constructor of LinearFunction, taking the data describing it
 /** Constructor of LinearFunction. It accepts:
  *
  * @param vars, a && to a vector of pairs < pointer to ColVariable ,
  *        Coefficient > representing the linear expression of the function;
  *        as the the && tells, vars is "consumed" by the constructor and its
  *        resources become property of the LinearFunction object.
  *
  * @param ct, a FunctionValue providing the value of the constant term of the
  *        function (which is affine rather than, strictly speaking, linear);
  *
  * @param ordered, a boolean indicating whether or not vars is already
  *        ordered in increasing pointer ColVariable "name = pointer" (if not
  *        it is ordered);
  *
  * @param observer, a pointer to the Observer of this LinearFunction.
  *
  * All inputs have a default ({}, 0, true, and nullptr, respectively) so
  * that this can be used as the void constructor. */

 explicit LinearFunction( v_coeff_pair && vars = {},
                 const FunctionValue ct = 0,
                 const bool ordered = false,
                 Observer * const observer = nullptr )
  : C15Function( observer ),
    v_pairs( std::move( vars ) ),
    f_value( Inf< FunctionValue >() ),
    f_constant_term( ct ) {
  if( !ordered )
   std::sort( v_pairs.begin(), v_pairs.end(),
              []( const auto & p1, const auto & p2 ) {
               return p1.first < p2.first;
              }
   );
 }

/*--------------------------------------------------------------------------*/
 /// destructor: it does nothing (explicitly)

 ~LinearFunction() override = default;

/*--------------------------------------------------------------------------*/
 /// clear method: clears the v_pairs field
 /** Method to "clear" the LinearFunction: it clear() the vector v_pairs.
  * This destroys the list of "active" Variable without unregistering from
  * them. Not that the LinearFunction would have, but the Observer may.
  * By not having any Variable, the Observer can no longer do that. */

 void clear() override { v_pairs.clear(); }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the whole (empty) set of parameters in one blow
 /** Although a LinearFunction formally has a lot of parameters, in fact it
  * "listens to no-one"; hence, the implementation of set_ComputeConfig() is
  * quite a trivial one. */

 void set_ComputeConfig( ComputeConfig *scfg ) final { }

/**@} ----------------------------------------------------------------------*/
/*---------- METHODS FOR READING THE DATA OF THE LinearFunction ------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the LinearFunction
    @{ */

 /// returns the vector of pairs (ColVariable *, Coefficient)

 v_c_coeff_pair & get_v_var() const { return v_pairs; }

/*--------------------------------------------------------------------------*/
 /// returns the Coefficient of the i-th Variable of this LinearFunction
 /** This method returns the Coefficient of the i-th Variable of this
  * LinearFunction. The index i must be between 0 and get_num_active_var()
  * - 1.
  *
  * @param i Index of the Variable whose coefficient is desired. */

 Coefficient get_coefficient( const int i ) const {
  return ( v_pairs.begin() + i )->second;
 }

 /*--------------------------------------------------------------------------*/
 /// returns the Coefficient of the Variable var of this LinearFunction
 /** Like get_coefficient( int ), but works directly with the Variable
  * instead of its index. */

 inline Coefficient get_coefficient( const ColVariable * const var ) const {

  auto idx = std::lower_bound( v_pairs.begin(),
                               v_pairs.end(),
                               std::make_pair( var, 0 ),
                               []( const auto & p1, const auto & p2 ) {
                                return p1.first < p2.first;
                               } );
  if( idx->first != var )
   throw ( std::invalid_argument( "stop is not an active Variable" ) );

  return idx->second;
 }

/**@} ----------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE LinearFunction ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the LinearFunction
 *  @{ */

 int compute( bool changedvars ) final;

/*--------------------------------------------------------------------------*/
 /// returns the value of the LinearFunction

 FunctionValue get_value() const final {
  return f_value;
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the LinearFunction is exact, hence lower_estimate == value

 FunctionValue get_lower_estimate() const final {
  return f_value;
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the LinearFunction is exact, hence upper_estimate == value

 FunctionValue get_upper_estimate() const final {
  return f_value;
 }

/*--------------------------------------------------------------------------*/

 FunctionValue get_Lipschitz_constant() override;

/*--------------------------------------------------------------------------*/

 bool is_convex() const final { return true; }

/*--------------------------------------------------------------------------*/

 bool is_concave() const final { return true; }

/*--------------------------------------------------------------------------*/

 void compute_hessian_approximation() final {};

/*--------------------------------------------------------------------------*/

 void get_hessian_approximation( DenseHessian & hessian ) const final;

/*--------------------------------------------------------------------------*/

 void get_hessian_approximation( SparseHessian & hessian ) const final;

/*--------------------------------------------------------------------------*/

 bool is_continuously_differentiable() const final { return true; }

/*--------------------------------------------------------------------------*/

 bool is_twice_continuously_differentiable() const final { return true; }

/*--------------------------------------------------------------------------*/
 /// get an arbitrary subset of linearization coefficients
 /** Like get_linearization_coefficients( FunctionValue *g , name , ... )
  * but without the name, because a LinearFunction only have one
  * linearization. */

 void get_linearization_coefficients( FunctionValue * g,
                                      c_Vec_Index & indices,
                                      c_Index start,
                                      c_Index end ) const;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get a range of linearization coefficients
 /** Like get_linearization_coefficients( FunctionValue * g , name , ... )
  * but without the name, because a LinearFunction only have one
  * linearization, and without the arbitrary subset. */

 void get_linearization_coefficients( FunctionValue * g,
                                      c_Index start,
                                      c_Index end ) const;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void get_linearization_coefficients( FunctionValue * g,
                                      LinearizationName name = Inf<LinearizationName>() ,
                                      c_Vec_Index & indices = {},
                                      c_Index start = 0 ,
                                      c_Index end = Inf<Index>() ) final;

/*--------------------------------------------------------------------------*/

 void get_linearization_coefficients( SparseVector & g,
                                      LinearizationName name = Inf<LinearizationName>(),
                                      c_Vec_Index & indices  = {},
                                      c_Index start = 0,
                                      c_Index end = Inf<Index>() ) final;

/*--------------------------------------------------------------------------*/
/** There is only one linearization in a LinearFunction. The linearization
 * constant is equal to the constant term of the LinearFunction. */

 Function::FunctionValue
 get_linearization_constant(
     const LinearizationName name = Inf<LinearizationName>() ) final {
  return f_constant_term;
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 ///< Returns the value of the constant term of this LinearFunction.

 FunctionValue get_constant_term() const { return f_constant_term; }

/**@} ----------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the LinearFunction
 *  @{ */

 ///< get the whole (empty) set of parameters in one blow
 /** Although a LinearFunction formally has a lot of parameters, in fact it
  * "listens to no-one"; hence, the implementation of get_ComputeConfig() is
  * quite a trivial one. */

 ComputeConfig * get_ComputeConfig( bool all,
                                    ComputeConfig * ocfg ) const final {
  return nullptr;
 }

/**@} ----------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE LinearFunction -------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" Variable in the
 * LinearFunction; this is the actual concrete implementation exploiting the
 * vector v_pairs of pairs.
 * @{ */

 Index get_num_active_var() const final {
  return v_pairs.size();
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 Index is_active( const Variable * const var ) const final {
  auto idx = std::lower_bound( v_pairs.begin(),
                               v_pairs.end(),
                               std::make_pair( var, 0 ),
                               []( const auto & p1, const auto & p2 ) {
                                return p1.first < p2.first;
                               } );
  if( idx < v_pairs.end() )
   return std::distance( v_pairs.begin(), idx );
  else
   return Inf< Index >();
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void map_active( c_Vec_p_Var & vars,
                  Vec_Index & map,
                  bool ordered ) const final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 Variable * get_active_var( const Index i ) const final {
  return ( v_pairs.begin() + i )->first;
 }

/*--------------------------------------------------------------------------*/

 v_iterator * v_begin() final {
  return new LinearFunction::v_iterator( v_pairs.begin() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 v_const_iterator * v_begin() const final {
  return new LinearFunction::v_const_iterator( v_pairs.begin() );
 }

/*--------------------------------------------------------------------------*/

 v_iterator * v_end() final {
  return new LinearFunction::v_iterator( v_pairs.end() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 v_const_iterator * v_end() const final {
  return new LinearFunction::v_const_iterator( v_pairs.end() );
 }

/**@} ----------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE LinearFunction ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the LinearFunction
 *  @{ */

 /// add a set of new Variable to the LinearFunction
 /**< Method that receives a pointer to a vector of pairs < ColVariable * ,
  * Coefficient > and adds them to these already in the LinearFunction. If
  * any variable in vars is already an active variable in the LinearFunction,
  * exception is thrown. As the the && tells, vars is "consumed" by the
  * method and its resources become property of the LinearFunction object
  * (which may dispatch them to the Modification that it may issue).
  *
  * The parameter ordered tells if vars is already ordered by ColVariable
  * "name = pointer" or not, otherwise it may get ordered inside the
  * method (which is why it is not const).
  *
  * The parameter issueMod decides if and how the C05FunctionModVars is
  * issued, as described in Observer::make_par(). Note that a linear function
  * is additive, and therefore strongly quasi-additive. */

 void add_variables( v_coeff_pair && vars,
                     bool ordered = false,
                     c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// add one single new Variable to the LinearFunction
 /** Like add_variables(), but just only one Variable. coeff is the
  * coefficient of the Variable in the linear function.
 *
  * The parameter issueMod decides if and how the C05FunctionModVars is
  * issued, as described in Observer::make_par(). Note that a linear function
  * is additive, and therefore strongly quasi-additive. */

 void add_variable( ColVariable * var,
                    Coefficient coeff,
                    c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// modify a single existing coefficient
 /** Method that modifies the coefficient for a specific Variable; if var is
  * not an active variable in the LinearFunction, exception is thrown. 
  *
  * The parameter issueMod decides if and how the C05FunctionModLin is issued,
  * as described in Observer::make_par(). */

 void modify_coefficient( ColVariable * var,
                          Coefficient coeff,
                          c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify a set of existing coefficients
 /** Method that receives a vector of pairs < ColVariable * , Coefficient >
  * so that the ColVariables are already in the LinearFunction, and modify
  * their coefficients accordingly. If any ColVariable in vars is not an
  * active variable in the LinearFunction, exception is thrown.  The
  * parameter ordered tells if vars is already ordered by ColVariable
  * "name = pointer" or not, otherwise it gets ordered inside the method
  * (which is why it is not const).
  *
  * The parameter issueMod decides if and how the C05FunctionModLin is issued,
  * as described in Observer::make_par(). */

 void modify_coefficients( v_coeff_pair & vars,
                           bool ordered = false,
                           c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify a set of existing coefficients
 /** Like modify_coefficients( v_coeff_pair * ), but takes in input a set of
  * index of the Variable whose coefficients need be modified, together with
  * (an iterator into) the vector of new coefficient values. Useful if one
  * knows the indices already, so that they need not be searched for. The
  * parameter ordered tells if nms is already ordered in increasing sense
  * (which corresponds to the fact that the ColVariable are ordered by
  * "name = pointer").
  *
  * The parameter issueMod decides if and how the C05FunctionModLin is issued,
  * as described in Observer::make_par(). */

 virtual void modify_coefficients( c_v_coeff_it NCoef,
                                   c_Vec_Index & nms,
                                   bool ordered,
                                   c_ModParam issueMod );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify a range of coefficients
 /** Modify the coefficients of all the Variable that are in position from
  * start (included) to min( stop , get_num_active_var() ) (excluded) in this
  * LinearFunction. NCoef is a (const) iterator into a vector of coefficients
  * that must clearly be at least as long (from NCoef to the end) as
  * min( stop , get_num_active_var() ) - start. Useful if one knows the
  * indices already, so that they need not be searched for.
  *
  * The parameter issueMod decides if and how the C05FunctionModLin is issued,
  * as described in Observer::make_par(). */

 void modify_coefficients( c_v_coeff_it NCoef,
                           c_Index strt = 0,
                           Index stop = Inf< Index >(),
                           c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify a range of coefficients
 /** Modify the coefficients of all the Variable comprised between strt
  * (included) and stop (excluded). Setting strt == nullptr means "the first
  * Variable", and setting stop == nullptr means "(one after) the last
  * Variable". If no-nullptr arguments are provided, they *must* be "names"
  * of Variable currently active in this LinearFunction. NCoef is a (const)
  * iterator into a vector of coefficients that must clearly be at least as
  * long (from NCoef to the end) as there are coefficients between the one
  * of strt and the one of stop (these included).
  *
  * The parameter issueMod decides if and how the C05FunctionModLin is issued,
  * as described in Observer::make_par(). */

 void modify_coefficients( c_v_coeff_it NCoef,
                           const Variable * strt = nullptr,
                           const Variable * stop = nullptr,
                           c_ModParam issueMod = eModBlck ) {
  c_Index istrt = strt ? is_active( strt ) : 0;
  if( istrt >= get_num_active_var() )
   throw ( std::invalid_argument( "strt is not an active Variable" ) );

  Index istop;
  if( stop ) {
   istop = is_active( stop );
   if( istop >= get_num_active_var() )
    throw ( std::invalid_argument( "stop is not an active Variable" ) );
  } else
   istop = get_num_active_var();

  modify_coefficients( NCoef, istrt, istop, issueMod );
 }

/*--------------------------------------------------------------------------*/
 /// remove the given Variable from the LinearFunction
 /** Remove the given Variable from the LinearFunction. This is
  * *mathematically* equivalent to setting the corresponding coefficient to
  * zero, but it is considered a "stronger" operation (it is possible to have
  * an active Variable with zero coefficient). If the Variable is not active
  * in the LinearFunction, exception is thrown.
  *
  * Note that the pointer must necessarily be to a ColVariable for it to
  * be active in a LinearFunction, but this method overrides that of
  * ThinVarDepInterface which, by necessity, has a Variable * type.
  *
  * The parameter issueMod decides if and how the C05FunctionModVars is
  * issued, as described in Observer::make_par(). Note that a linear function
  * is additive, and therefore strongly quasi-additive. */

 void remove_variable( Variable * var, c_ModParam issueMod ) final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// remove the i-th Variable
 /** Like remove_variable( Variable * ), but takes in input the index of
  * the Variable to be removed rather than its pointer. Useful if one knows
  * the index already, so that it need not be searched for.
  *
  * The parameter issueMod decides if and how the C05FunctionModVars is
  * issued, as described in Observer::make_par(). Note that a linear function
  * is additive, and therefore strongly quasi-additive. */

 void remove_variable( c_Index i, c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove a range of Variable
 /** Remove all the Variable that are in position from start (included) to
  * min( stop , get_num_active_var() ) (excluded) in this LinearFunction.
  *
  * The parameter issueMod decides if and how the C05FunctionModVars is
  * issued, as described in Observer::make_par(). Note that a linear function
  * is additive, and therefore strongly quasi-additive. */

 void remove_variables( c_Index strt = 0,
                        Index stop = Inf< Index >(),
                        c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove a range of Variable
 /** Remove all the Variable comprised between strt (included) and stop
  * (excluded). Setting strt == nullptr means "the first Variable", and
  * setting stop == nullptr means "(one after) the last Variable". If
  * no-nullptr arguments are provided, they *must* be "names" of Variable
  * currently active in this LinearFunction.
  *
  * The parameter issueMod decides if and how the C05FunctionModVars is
  * issued, as described in Observer::make_par(). Note that a linear function
  * is additive, and therefore strongly quasi-additive. */

 void remove_variables( const Variable * const strt = nullptr,
                        const Variable * const stop = nullptr,
                        c_ModParam issueMod = eModBlck ) {
  c_Index istrt = strt ? is_active( strt ) : 0;
  if( istrt >= get_num_active_var() )
   throw ( std::invalid_argument( "strt is not an active Variable" ) );

  Index istop;
  if( stop ) {
   istop = is_active( stop );
   if( istrt >= get_num_active_var() )
    throw ( std::invalid_argument( "stop is not an active Variable" ) );
  } else
   istop = get_num_active_var();

  remove_variables( istrt, istop, issueMod );
 }

/*--------------------------------------------------------------------------*/
 /// remove the given set of Variable
 /** Remove all the Variable in the given vector of pointers vars. If any
  * Variable in vars is not an active Variable in the LinearFunction,
  * exception is thrown. The parameter ordered tells if vars is already
  * ordered by Variable "name = pointer" or not, otherwise it gets ordered
  * inside the method (which is why it is not const).
  *
  * Note that vars is a std::vector< Variable * > rather than a
  * std::vector< ColVariable * >, although of course all the pointers have
  * to be to a ColVariable. This is because the vector can then be passed
  * right away to the C05FunctionModLin, that expects one; indeed, as the
  * && tells, the vector (as that of new coefficients) becomes "property"
  * of the LinearFunction, that dispatches it to the Modification. The
  * point is that since C05FunctionModLin is defined in C05Function, it is
  * not restricted to the case where Variable is a ColVariable, although it
  * should be "like" one (a Variable representing a single real value) for
  * the current form of linearizations to work. Although a
  * std::vector< ColVariable * > and a std::vector< Variable * > should be
  * physically indistinguishable, there is no sound way to cheaply pass the
  * former as the latter in C++; having the input as a
  * std::vector< Variable * > circumvents the problems (and each pointer
  * could be static_cast-ed to a ColVariable * as soon as it is confirmed
  * that the Variable is active in the LinearFunction, should this be
  * necessary).
  *
  * The parameter issueMod decides if and how the C05FunctionModVars is
  * issued, as described in Observer::make_par(). Note that a linear function
  * is additive, and therefore strongly quasi-additive. */

 void remove_variables( Vec_p_Var && vars,
                        bool ordered,
                        c_ModParam issueMod ) final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// remove a set of Variable by index
 /** Like remove_variables( Vec_p_Var * ), but takes in input a set of index
  * of the Variable to be removed rather than their pointers. Useful if one
  * knows the indices already, so that they need not be searched for. The
  * parameter ordered tells if nms is already ordered in increasing sense
  * (by index, but this also implies by Variable "name = pointer") if not
  * otherwise it gets ordered inside the method (which is why it is not
  * const). Note that nms is *not* &&, hence it is not "taken" by the
  * LinearFunction.
  *
  * The parameter issueMod decides if and how the C05FunctionModVars is
  * issued, as described in Observer::make_par(). Note that a linear function
  * is additive, and therefore strongly quasi-additive. */

 virtual void remove_variables( Vec_Index & nms,
                                bool ordered,
                                c_ModParam issueMod );

/*--------------------------------------------------------------------------*/
 ///< sets the value of the constant term of this function.
 /** Method that sets the new value to the constant term of this linear
  * (actually, affine) Function to constant_term.
  *
  * The parameter issueMod decides if and how the LinearFunctionMod is issued,
  * as described in Observer::make_par(). */

 void set_constant_term( FunctionValue constant_term,
                         c_ModParam issueMod );

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

 /// printing the LinearFunction
 void print( std::ostream & output ) const override {
  output << "LinearFunction [" << this << "] observed by ["
         << &f_Observer << "] with " << get_num_active_var()
         << " active variables;";
  output << "current value = " << get_value();
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 v_coeff_pair v_pairs;
 ///< vector of pairs < ColVariable * , Coefficient > for the Function
 /**< vector of pairs < ColVariable * , Coefficient > characterizing the
  * LinearFunction; the vector is kept sorted in an ascending order of the
  * pointers of the ColVariables. */

 FunctionValue f_value;  ///< the value of the Function

 FunctionValue f_constant_term;
 ///< the value of the constant term of this Function

/**@} ----------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/

 void issue_add_variables_modification( v_coeff_pair & pairs,
                                        c_ModParam issueMod );

/*--------------------------------------------------------------------------*/

};  // end( class( LinearFunction ) )

/** @} end( group( LinearFunction_CLASSES ) ) ------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* LinearFunction.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File LinearFunction.h --------------------------*/
/*--------------------------------------------------------------------------*/
