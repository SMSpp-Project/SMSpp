/*--------------------------------------------------------------------------*/
/*------------------------ File LinearFunction.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *concrete* class LinearFunction, which implements
 * C15Function with a simple linear function.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
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
namespace SMSpp_di_unipi_it
{
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
 * \f[
 *  f(x) = c + sum_{ i = 1, ..., n } a_i * x_i
 * \f]
 * where the scalar c is the constant term of the Function, and a_i is the
 * fixed real coefficient of the ColVariable x_i (since the return value
 * need be a real and the a_i are reals, so must be the x_i).
 *
 * This Function issues the following :Modification:
 *
 * - A C05FunctionModVarsAddd when Variable are added; note that, being this
 *   a linear Function, the addition is strongly quasi-additive.
 *
 * - A C05FunctionModVarsRngd or C05FunctionModVarsSbst when Variable are
 *   removed (depending if they were in a range or not); note that, being
 *   this a linear Function, the removal is strongly quasi-additive.
 *
 * - a C05FunctionModLinRngd or a C05FunctionModLinSbst when the coefficients
 *   of some Variable change (depending if they were in a range or not); the
 *   shift is NaN, no check on the sign of the Variables / coefficient change
 *   is performed (yet).
 *
 * - a FunctionMod when the constant term changes, with the shift equal to
 *   the difference between the new and old constant term values.
 *
 * TODO: when "a lot" of the coefficients change, rahter issue a
 *       C05FunctionMod of type AllEntriesChanged, because it might be
 *       faster to just re-read all the coefficients than to skip the
 *       few non-changed ones. */

class LinearFunction : public C15Function
{
/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------- PUBLIC TYPES OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */

 /// type of the coefficients of the linear function = FunctionValue
 using Coefficient = FunctionValue;

 using v_coeff = std::vector< Coefficient >;  ///< a vector of Coefficients

 using v_coeff_it = v_coeff::iterator;  ///< iterator in v_coeff

 using c_v_coeff_it = v_coeff::const_iterator;  ///< const iterator in v_coeff

 /// one term of the sum: ( ColVariable * , Coefficient )
 using coeff_pair = std::pair< ColVariable * , Coefficient >;

 using v_coeff_pair = std::vector< coeff_pair >;  ///< a vector of coeff_pair

 using v_c_coeff_pair = const v_coeff_pair;  ///< a const vector of coeff_pair

/*--------------------------------------------------------------------------*/
 /// virtualized concrete iterator
 /** A concrete class deriving from ThinVarDepInterface::v_iterator and
  * implementing the concrete iterator for sifting through the "active"
  * Variable of a LinearFunction. */

 class v_iterator : public ThinVarDepInterface::v_iterator
 {
  public:

  explicit v_iterator( v_coeff_pair::iterator & itr ) : itr_( itr ) {}

  explicit v_iterator( v_coeff_pair::iterator && itr )
   : itr_( std::move( itr ) ) {}

  v_iterator * clone( void ) final { return( new v_iterator( itr_ ) ); }

  void operator++( void ) override final { ++( itr_ ); }
  reference operator*( void ) const final { return( *( *itr_ ).first ); }
  pointer operator->( void ) const final { return( ( *itr_ ).first ); }

  bool operator==( const ThinVarDepInterface::v_iterator & rhs )
   const final {
   #ifdef NDEBUG
    auto tmp = static_cast< const LinearFunction::v_iterator * >( & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const LinearFunction::v_iterator * >( &rhs );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }

  bool operator!=( const ThinVarDepInterface::v_iterator & rhs )
   const final {
   #ifdef NDEBUG
    auto tmp = static_cast< const LinearFunction::v_iterator * >( & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const LinearFunction::v_iterator * >( &rhs );
    return( tmp ? itr_ != tmp->itr_ : true );
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

 class v_const_iterator : public ThinVarDepInterface::v_const_iterator
 {
  public:

  explicit v_const_iterator( v_c_coeff_pair::const_iterator & itr ) :
   itr_( itr ) {}

  explicit v_const_iterator( v_c_coeff_pair::const_iterator && itr ) :
   itr_( std::move( itr ) ) {}

  v_const_iterator * clone( void ) final {
   return( new v_const_iterator( itr_ ) );
   }

  void operator++( void ) final { ++( itr_ ); }
  reference operator*( void ) const final { return( *( *itr_ ).first ); }
  pointer operator->( void ) const final { return( ( *itr_ ).first ); }

  bool operator==( const ThinVarDepInterface::v_const_iterator & rhs )
   const final {
   #ifdef NDEBUG
    auto tmp = static_cast< const LinearFunction::v_const_iterator * >( & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const LinearFunction::v_const_iterator * >( &rhs );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }

  bool operator!=( const ThinVarDepInterface::v_const_iterator & rhs )
   const final {
   #ifdef NDEBUG
    auto tmp = static_cast< const LinearFunction::v_const_iterator * >( & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const LinearFunction::v_const_iterator * >( &rhs );
    return( tmp ? itr_ != tmp->itr_ : true );
   #endif
   }

  private:

  v_coeff_pair::const_iterator itr_;
 };

/** @} ---------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and Destructor
 *  @{ */

 /// constructor of LinearFunction, taking the data describing it
 /** Constructor of LinearFunction. It accepts:
  *
  * @param vars, a && to a vector of pairs < ColVariable * , Coefficient >
  *        representing the linear expression of the function; as the &&
  *        tells, vars is "consumed" by the constructor and its resources
  *        become property of the LinearFunction object.
  *
  * @param ct, a FunctionValue providing the value of the constant term of the
  *        function (which is affine rather than, strictly speaking, linear);
  *
  * @param observer, a pointer to the Observer of this LinearFunction.
  *
  * Important note:
  *
  *     THE ORDER OF THE vars VECTOR WILL DICTATE THE ORDER OF THE "ACTIVE"
  *     [Col]Variable OF THE LinearFunction
  *
  * That is, get_active_var( 0 ) == vars[ 0 ].first,
  * get_active_var( 1 ) == vars[ 1 ].first, ...
  *
  * All inputs have a default ({}, 0, and nullptr, respectively) so that this
  * can be used as the void constructor. */

 explicit LinearFunction( v_coeff_pair && vars = {}, FunctionValue ct = 0,
                          Observer * const observer = nullptr )
  : C15Function( observer ), v_pairs( std::move( vars ) ),
    f_value( Inf< FunctionValue >() ), f_constant_term( ct ) {}

/*--------------------------------------------------------------------------*/
 /// destructor: it does nothing (explicitly)

 ~LinearFunction() override = default;

/*--------------------------------------------------------------------------*/
 /// clear method: clears the v_pairs field
 /** Method to "clear" the LinearFunction: it clear() the vector v_pairs.
  * This destroys the list of "active" Variable without unregistering from
  * them. Not that the LinearFunction would have, but the Observer may.
  * By not having any Variable, the Observer can no longer do that. */

 void clear( void ) override { v_pairs.clear(); }

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the whole (empty) set of parameters in one blow
 /** Although a LinearFunction formally has a lot of parameters, in fact it
  * "listens to no-one"; hence, the implementation of set_ComputeConfig() is
  * quite a trivial one. */

 void set_ComputeConfig( ComputeConfig * scfg ) final {}

/** @} ---------------------------------------------------------------------*/
/*---------- METHODS FOR READING THE DATA OF THE LinearFunction ------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the LinearFunction
    @{ */

 /// returns the vector of pairs (ColVariable *, Coefficient)

 [[nodiscard]] v_c_coeff_pair & get_v_var( void ) const { return( v_pairs ); }

/*--------------------------------------------------------------------------*/
 /// returns the Coefficient of the i-th Variable in this LinearFunction
 /** This method returns the Coefficient of the i-th Variable of this
  * LinearFunction. The index i must be between 0 and get_num_active_var()
  * - 1.
  *
  * @param i Index of the Variable whose coefficient is desired. */

 [[nodiscard]] Coefficient get_coefficient( Index i ) const {
  return( ( v_pairs.begin() + i )->second );
  }

/** @} ---------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE LinearFunction ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the LinearFunction
 *  @{ */

 int compute( bool changedvars ) final;

/*--------------------------------------------------------------------------*/
 /// returns the value of the LinearFunction

 [[nodiscard]] FunctionValue get_value( void ) const override {
  return( f_value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the LinearFunction is exact, hence lower_estimate == value

 [[nodiscard]] FunctionValue get_lower_estimate( void ) const override {
  return( f_value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the LinearFunction is exact, hence upper_estimate == value

 [[nodiscard]] FunctionValue get_upper_estimate( void ) const override {
  return( f_value );
  }

/*--------------------------------------------------------------------------*/

 FunctionValue get_Lipschitz_constant( void ) override;

/*--------------------------------------------------------------------------*/

 [[nodiscard]] bool is_convex( void ) const final { return( true ); }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] bool is_concave( void ) const final { return( true ); }

/*--------------------------------------------------------------------------*/

 void compute_hessian_approximation( void ) final {};

/*--------------------------------------------------------------------------*/

 void get_hessian_approximation( DenseHessian & hessian ) const final;

/*--------------------------------------------------------------------------*/

 void get_hessian_approximation( SparseHessian & hessian ) const final;

/*--------------------------------------------------------------------------*/

 [[nodiscard]] bool is_continuously_differentiable( void ) const final {
  return( true );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] bool is_twice_continuously_differentiable( void ) const final {
  return( true );
  }

/*--------------------------------------------------------------------------*/

 void get_linearization_coefficients( FunctionValue * g ,
                                      Range range = INFRange ,
                                      Index name = Inf< Index >() ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void get_linearization_coefficients( SparseVector & g ,
                                      Range range = INFRange ,
                                      Index name = Inf< Index >() ) override;

/*--------------------------------------------------------------------------*/

 void get_linearization_coefficients( FunctionValue * g , c_Subset & subset ,
                                      bool ordered = false ,
                                      Index name = Inf< Index >() ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void get_linearization_coefficients( SparseVector & g , c_Subset & subset ,
                                      bool ordered = false ,
                                      Index name = Inf< Index >() ) override;

/*--------------------------------------------------------------------------*/
 /// returns the linearization constant of the only linearization
 /** There is only one linearization in a LinearFunction. The linearization
  * constant is equal to the constant term of the LinearFunction. */

 FunctionValue get_linearization_constant( Index name = Inf< Index >() )
  override { return( f_constant_term ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the value of the constant term of this LinearFunction.

 [[nodiscard]] FunctionValue get_constant_term( void ) const override {
  return( f_constant_term );
  }

/** @} ---------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the LinearFunction
 *  @{ */

 ///< get the whole (empty) set of parameters in one blow
 /** Although a LinearFunction formally has a lot of parameters, in fact it
  * "listens to no-one"; hence, the implementation of get_ComputeConfig() is
  * quite a trivial one. */

 ComputeConfig * get_ComputeConfig( bool all , ComputeConfig * ocfg )
  const override { return( nullptr ); }

/** @} ---------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE LinearFunction -------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" Variable in the
 * LinearFunction; this is the actual concrete implementation exploiting the
 * vector v_pairs of pairs.
 * @{ */

 [[nodiscard]] Index get_num_active_var( void ) const override {
  return( v_pairs.size() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 Index is_active( const Variable * var ) const override {
  auto idx = std::find_if( v_pairs.begin() , v_pairs.end() ,
                           [ & var ]( const auto & p ) -> bool {
                            return( p.first == var );
                            } );
  return( idx != v_pairs.end() ? std::distance( v_pairs.begin() , idx )
                               : Inf< Index >() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void map_active( c_Vec_p_Var & vars , Subset & map , bool ordered )
  const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 Subset map_index( const std::vector< Variable * > & vars , c_Subset & nms )
  const override {
  if( vars.size() != nms.size() )
   throw( std::invalid_argument( "vars and nms sizes do not match" ) );

  Subset map( nms.size() );
  if( map.empty() )
   return( map );

  auto nmsit = nms.begin();
  auto varsit = vars.begin();
  auto mapit = map.begin();

  // for all Variable in the set
  while( varsit != vars.end() ) {
   auto var = *(varsit++);  // next variable
   auto oi = *(nmsit++);    // its original index
   // if var has not been deleted (and, possibly, re-added), its index
   // must be <= oi: search backward from oi to find it
   auto avoi = v_pairs[ oi ].first;
   Index i = oi;
   while( var != avoi ) {
    if( ! i )
     break;
    avoi = v_pairs[ --i ].first;
    }
   if( var == avoi )  // the Variable was found
    *(mapit++) = i;   // this is its index
   else {             // the Variable was not found
    // restart the search from the last variable to oi (excluded), for the
    // case where var has been deleted and re-added, and therefore its
    // index can now be arbitrary (but it is more likely to be "close to
    // the end" than "at the beginning")
    for( i = v_pairs.size() , avoi = v_pairs[ --i ].first ;
	 ( var != avoi ) && ( i > oi ) ; )
     avoi = v_pairs[ --i ].first;
    *(mapit++) = ( var == avoi ) ? i : Inf< Index >();
    }
   }  // end( for all Variable )

  return( map );

  }  // end( map_index( Subset )

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 Subset map_index( const std::vector< Variable * > & vars , Range rng )
  const override {
  if( vars.size() != rng.second - rng.first )
   throw( std::invalid_argument( "vars and rng sizes do not match" ) );

  Subset map( vars.size() );
  if( map.empty() )
   return( map );

  auto varsit = vars.begin();
  auto mapit = map.begin();

  // for all Variable in the set
  for( auto oi = rng.first ; oi < rng.second ; ++oi ) {
   auto var = *(varsit++);  // next variable
   // if var has not been deleted (and, possibly, re-added), its index
   // must be <= oi: search backward from oi to find it
   auto avoi = v_pairs[ oi ].first;
   Index i = oi;
   while( var != avoi ) {
    if( ! i )
     break;
    avoi = v_pairs[ --i ].first;
    }
   if( var == avoi )  // the Variable was found
    *(mapit++) = i;   // this is its index
   else {             // the Variable was not found
    // restart the search from the last variable to oi (excluded), for the
    // case where var has been deleted and re-added, and therefore its
    // index can now be arbitrary (but it is more likely to be "close to
    // the end" than "at the beginning")
    for( i = v_pairs.size() , avoi = v_pairs[ --i ].first ;
	 ( var != avoi ) && ( i > oi ) ; )
     avoi = v_pairs[ --i ].first;
    *(mapit++) = ( var == avoi ) ? i : Inf< Index >();
    }
   }  // end( for all Variable )

  return( map );

  }  // end( map_index( Range )

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] Variable * get_active_var( Index i ) const override {
  return( v_pairs[ i ].first );
  }

/*--------------------------------------------------------------------------*/

 v_iterator * v_begin( void ) override {
  return( new LinearFunction::v_iterator( v_pairs.begin() ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] v_const_iterator * v_begin( void ) const override {
  return( new LinearFunction::v_const_iterator( v_pairs.begin() ) );
 }

/*--------------------------------------------------------------------------*/

 v_iterator * v_end( void ) override {
  return( new LinearFunction::v_iterator( v_pairs.end() ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] v_const_iterator * v_end( void ) const override {
  return( new LinearFunction::v_const_iterator( v_pairs.end() ) );
  }

/** @} ---------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE LinearFunction ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the LinearFunction
 *  @{ */

 /// add a set of new Variable to the LinearFunction
 /**< Method that receives a pointer to a vector of pairs < ColVariable * ,
  * Coefficient > and adds them to these already in the LinearFunction.
  * Note that
  *
  *     IT IS EXPECTED THAT NONE OF THE NEW ColVariable IS ALREADY "ACTIVE"
  *     IN THE LinearFunction, BUT NO CHECK IS DONE TO ENSURE THIS
  *
  * Indeed, the check is costly, and the LinearFunction does not really have
  * a functional issue with repeated ColVariable. The real issue rather comes
  * whenever the LinearFunction is used within a Constraint or Objective that
  * need to register istelf among the "active" Variable of the LinearFunction;
  * this process is not structured to work with multiple copies of the same
  * "active" Variable. Thus, a LinearFunction used within such an object
  * should not have repeated Variable, but if this is an issue then the
  * check will have to be performed elsewhere, it is not done here.
  *
  * As the && tells, vars is "consumed" by the method and its resources
  * become property of the LinearFunction object.
  *
  * The parameter issueMod decides if and how the C05FunctionModVarsAddd is
  * issued, as described in Observer::make_par(). Note that a linear function
  * is additive, and therefore strongly quasi-additive, which is why a
  * C05FunctionModVarsAddd (rather than a FunctionModVarsAddd) is issued. */

 void add_variables( v_coeff_pair && vars , ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// add one single new Variable to the LinearFunction
 /** Like add_variables(), but just only one Variable. coeff is the
  * coefficient of the Variable in the linear function.
  *
  * The parameter issueMod decides if and how the C05FunctionModVarsAddd is
  * issued, as described in Observer::make_par(). Note that a linear function
  * is additive, and therefore strongly quasi-additive. */

 void add_variable( ColVariable * var , Coefficient coeff ,
                    ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// modify a single existing coefficient
 /** Method that modifies the coefficient for the i-th "active" Variable; if
  * i is not a valid index, exception is thrown. 
  *
  * The parameter issueMod decides if and how the C05FunctionModLin is issued,
  * as described in Observer::make_par(). */

 void modify_coefficient( Index i , Coefficient coeff ,
                          ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// modify a set of existing coefficients
 /** Method that receives the vector of coefficient values and the set of
  * indices of the ColVariable whose coefficients need be modified, and sets
  * the coefficient of ColVariable nms[ i ] to NCoef[ i ]. As the && tells,
  * both NCoef and nms become property of the LinearFunction object (possibly
  * to be immediately dispatched to the issued C05FunctionModLinSbst). The
  * ordered parameter tells if subset is ordered by increasing Index, which
  * is actually *not* helpful for LinearFunction but it may be for 
  * Block/Solver having to deal with the FunctionModVarsSbst.
  *
  * The parameter issueMod decides if and how the C05FunctionModLinSbst is
  * issued, as described in Observer::make_par(). */

 void modify_coefficients( Vec_FunctionValue && NCoef , Subset && nms ,
                           bool ordered = false ,
                           ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify a range of coefficients
 /** Modify the coefficients of all the Variable that are in position i,
  * where range.first <= i < min( range.second , get_num_active_var() ),
  * giving them value NCoef[ i - range.first ]. As the && tells, NCoef
  * becomes property of the LinearFunction object (possibly to be
  * immediately dispatched to the issued C05FunctionModLinRngd).
  *
  * The parameter issueMod decides if and how the C05FunctionModLinRngd is
  * issued, as described in Observer::make_par(). */

 void modify_coefficients( Vec_FunctionValue && NCoef ,
                           Range range = INFRange ,
                           ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove the i-th "active" Variable from the LinearFunction
 /** Remove the i-th "active" Variable from the LinearFunction. This is
  * *mathematically* equivalent to setting the corresponding coefficient to
  * zero, but it is considered a "stronger" operation (it is possible to have
  * an active Variable with zero coefficient). If there is no Variable with
  * the given index, an exception is thrown.
  *
  * The parameter issueMod decides if and how the C05FunctionModVarsRngd is
  * issued, as described in Observer::make_par(). Note that a linear function
  * is additive, and therefore strongly quasi-additive, which is why a
  * C05FunctionModVarsRngd is issued as opposed to a FunctionModVarsRngd
  * one. */

 void remove_variable( Index i , ModParam issueMod = eModBlck ) override;

/*--------------------------------------------------------------------------*/
 /// remove a range of "active" Variable
 /** Remove all the "active" Variable in the given Range, i.e., all those
  * with index i s.t. range.first <= i < min( range.second ,
  * get_num_active_var() ), from this LinearFunction.
  *
  * The parameter issueMod decides if and how the C05FunctionModVarsRngd is
  * issued, as described in Observer::make_par(). Note that a linear function
  * is additive, and therefore strongly quasi-additive, which is why a
  * C05FunctionModVarsRngd is issued as opposed to a FunctionModVarsRngd
  * one. */

 void remove_variables( Range range , ModParam issueMod = eModBlck ) override;

/*--------------------------------------------------------------------------*/
 /// remove the given subset of Variable
 /** Remove all the Variable in the given set of indices. As the && tells,
  * nms becomes property of the LinearFunction object (possibly to be
  * immediately dispatched to the issued C05FunctionModVarSbst). a special
  * setting is if
  *
  *     nms.empty() == true, IN WHICH CASE ALL Variable ARE ELIMINATED
  *
  * The parameter ordered tells if nms is ordered by increasing index. This
  * is useful for efficently deleting them; indeed, if ordered == false the
  * vector is sorted inside. The parameter is provided to signal the lucky
  * case in which this operation can be avoided since nms "naturally" comes
  * out ordered.
  *
  * The parameter issueMod decides if and how the C05FunctionModVarSbst is
  * issued, as described in Observer::make_par(). Note that a linear function
  * is additive, and therefore strongly quasi-additive, which is why a
  * C05FunctionModVarSbst is issued as opposed to a FunctionModVarSbst one. */

 void remove_variables( Subset && nms , bool ordered = false ,
                        ModParam issueMod = eModBlck ) override;

/*--------------------------------------------------------------------------*/
 ///< sets the value of the constant term of this function.
 /** Method that sets the new value to the constant term of this linear
  * (actually, affine) Function to constant_term.
  *
  * The parameter issueMod decides if and how the C05FunctionMod (with
  * type() == NothingChanged and therefore clearly which().empty, and with
  * shift() == constant_term - < old constant term >) is issued, as
  * described in Observer::make_par(). */

 void set_constant_term( FunctionValue constant_term,
                         ModParam issueMod = eModBlck );

/** @} ---------------------------------------------------------------------*/
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
  * LinearFunction. */

 FunctionValue f_value;  ///< the value of the Function

 FunctionValue f_constant_term;
 ///< the value of the constant term of this Function

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

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
