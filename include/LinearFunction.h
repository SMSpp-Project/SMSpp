/*--------------------------------------------------------------------------*/
/*------------------------ File LinearFunction.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *concrete* class LinearFunction, which
 * implements C15Function with a simple linear function.
 *
 * \version 0.11
 *
 * \date 08 - 02 - 2019
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
 * Copyright &copy by Antonio Frangioni, Rafael Durbano Lobato
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

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup LFun_CLASSES Classes in LinearFunction.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS LinearFunction ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a linear Function
/**< The class LinearFunction implements C15Function with a
 * simple linear function of the form
 * \[
 *  f(x) = c + sum_{i \in I} a_i * x_i
 * \]
 * where the scalar c is the constant term of the Function, and a_i is the
 * coefficient of the Variable x_i, for i in I.
 *
 * This Function issues the following modifications:
 *
 * - When Variables are added, a FunctionModVars of type AddVar is issued;
 *   pointers to the Variables that were added are provided in the
 *   Modification.
 *
 * - When Variables are removed, a FunctionModVars of type RemoveVar is
 *   issued. Pointers to the Variables that were removed are provided in the
 *   Modification.
 *
 * - When the coefficients of some Variables change, a C05FunctionModVars of
 *   type LinearizationEntriesChange isissued. The entries of a linearization
 *   affected by this modification are precisely the ones associated with the
 *   Variables whose coefficients have changed.
 *
 * - When the constant term changes, a FunctionMod is issued with the shift
 *   equals to the difference between the new and old constant term values.
 */

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

 typedef double Coefficient;
 ///< type of the coefficients of the linear function

 typedef std::vector<Coefficient> v_coeff;
 ///< a vector of coeff_pair

 typedef const v_coeff c_v_coeff;
 ///< a const vector of coeff_pair

 typedef v_coeff::iterator v_coeff_it;
 ///< iterator in v_coeff

 typedef v_coeff::const_iterator c_v_coeff_it;
 ///< const iterator in v_coeff

 typedef std::pair<ColVariable *, Coefficient> coeff_pair;
 ///< element of a Linear Coefficient matrix: (ColVariable *, Coefficient)

 typedef std::vector<coeff_pair> v_coeff_pair;
 ///< a vector of coeff_pair

 typedef const coeff_pair c_coeff_pair;
 ///< a const coeff_pair

 typedef const v_coeff_pair v_c_coeff_pair;
 ///< a const vector of coeff_pair

/*--------------------------------------------------------------------------*/
 /// virtualized concrete iterator
 /** A concrete class deriving from ThinVarDepInterface::v_iterator and
  * implementing the concrete iterator for sifting through the "active"
  * Variable of a LinearFunction. */

 class v_iterator : public ThinVarDepInterface::v_iterator
 {
  public:

  v_iterator( v_coeff_pair::iterator itr ) : itr_( itr ) { }

  virtual void operator++( void ) override final { (itr_)++; }
  virtual reference operator*( void ) const override final {
   return( *((*itr_).first) );
   }
  virtual pointer operator->( void ) const override final {
   return( (*itr_).first );
   }
  virtual bool operator==( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast<const LinearFunction::v_iterator *>( & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const LinearFunction::v_iterator *>( & rhs );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }
  virtual bool operator!=( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast<const LinearFunction::v_iterator *>( & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const LinearFunction::v_iterator *>( & rhs );
    return( tmp ? itr_ != tmp->itr_ : false );
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

  v_const_iterator( v_c_coeff_pair::const_iterator itr ) : itr_( itr ) { }

  virtual void operator++( void ) override final { (itr_)++; }
  virtual reference operator*( void ) const override final {
   return( *((*itr_).first) );
   }
  virtual pointer operator->( void ) const override final {
   return( (*itr_).first );
   }
  virtual bool operator==( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast<const LinearFunction::v_const_iterator *>( & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const LinearFunction::v_const_iterator *>( & rhs
								       );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }
  virtual bool operator!=( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast<const LinearFunction::v_const_iterator *>( & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const LinearFunction::v_const_iterator *>( & rhs
								       );
    return( tmp ? itr_ != tmp->itr_ : false );
   #endif
   }

  private:

  v_coeff_pair::const_iterator itr_;
  };

/*@}------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and Destructor
 *  @{ */

 /// constructor of LinearFunction, taking the pairs
 /** Constructor of LinearFunction. It accepts a vector of pairs
  * < pointer to ColVariable , Coefficient > representing the linear
  * expression of the function, the value of the constant term of the
  * function, and a bool telling if the given vector is already ordered by
  * ColVariable "name = pointer" or not, in which case it is ordered. As the
  * the && tells, vars is "consumed" by the constructor and its resources
  * become property of the LinearFunction object.
  *
  * All inputs have a default ({}, 0, and true, respectively) so that this
  * can be used as the void constructor. */

 LinearFunction( v_coeff_pair && vars = {} , const FunctionValue ct = 0,
                 const bool ordered = false )
  :  C15Function() , v_pairs( std::move( vars ) ) ,
     f_value( Inf<FunctionValue>() ) , f_constant_term( ct )
 {
  if( ! ordered )
   std::sort( v_pairs.begin() , v_pairs.end() ,
	      []( const auto & p1, const auto & p2 ) {
	       return( p1.first < p2.first );
	       }
	      );
  }

/*--------------------------------------------------------------------------*/
 /// destructor: it (apparently) does nothing

 virtual ~LinearFunction() { }

/*--------------------------------------------------------------------------*/
 
 virtual void clear( void ) override {
  v_pairs.clear();
  f_Observer = nullptr;
  }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the pointer to the Observer of this Function
 /** If a non-nullptr Observer is set that also is a ThinVarDepInterface, then
  * the Observer (rather than the LinearFunction itself) will be registered
  * in the Variable of the LinearFunction. If a null Observer is set
  * pointer is provided (either in the constructor or here), or the Function
  * is "unregistered" from the current observe by calling the method with
  * nullptr (default), then the Function is left "free floating", which means
  * that no Modification is ever produced and no Block/Solver ever gets
  * informed of any change occurring in the Function. */

 virtual void register_Observer( Observer * const observer = nullptr )
  override;

/*--------------------------------------------------------------------------*/
 /// set the whole (empty) set of parameters in one blow
 /** Although a LinearFunction formally has a lot of parameters, in fact it
  * "listens to no-one"; hence, the implementation of set_ComputeConfig() is
  * quite a trivial one. */

 virtual void set_ComputeConfig( ComputeConfig *scfg = nullptr )
  override final { }

/*@} -----------------------------------------------------------------------*/
/*---------- METHODS FOR READING THE DATA OF THE LinearFunction ------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the LinearFunction
    @{ */

 /// returns the vector of pairs (ColVariable *, Coefficient)

 v_c_coeff_pair & get_v_var( void ) const { return( v_pairs ); }

/*--------------------------------------------------------------------------*/
 /// returns the Coefficient of the i-th Variable of this LinearFunction
 /** This method returns the Coefficient of the i-th Variable of this
  * LinearFunction. The index i must be between 0 and get_num_active_var()
  * - 1.
  *
  * @param i Index of the Variable whose coefficient is desired. */

 Coefficient get_coefficient( const int i ) const {
  return( ( v_pairs.begin() + i )->second );
  }

/*@} -----------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE LinearFunction ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the LinearFunction
 *  @{ */

 virtual int compute( bool changedvars = true ) override;

/*--------------------------------------------------------------------------*/

 virtual FunctionValue get_value( void ) const override { return( f_value ); }

/*--------------------------------------------------------------------------*/

 virtual FunctionValue get_Lipschitz_constant() override;

/*--------------------------------------------------------------------------*/

 virtual bool is_convex( void ) const override final { return( true ); }

/*--------------------------------------------------------------------------*/

 virtual bool is_concave( void ) const override final { return( true ); }

/*--------------------------------------------------------------------------*/

 virtual void compute_hessian_approximation( void ) override final { };

/*--------------------------------------------------------------------------*/

 virtual void get_hessian_approximation( DenseHessian &hessian )
  const override final;

/*--------------------------------------------------------------------------*/

 virtual void get_hessian_approximation( SparseHessian &hessian )
   const override final;

/*--------------------------------------------------------------------------*/

 virtual bool is_continuously_differentiable( void ) const override final {
  return( true );
  }

/*--------------------------------------------------------------------------*/

 virtual bool is_twice_continuously_differentiable( void )
  const override final { return( true ); }

/*--------------------------------------------------------------------------*/
 /// get an arbitrary subset of linearization coefficients
 /** Like get_linearization_coefficients( FunctionValue *g , name , ... )
  * but without the name, because a LinearFunction only have one
  * linearization. */

 void get_linearization_coefficients( FunctionValue * g ,
				      c_Vec_Index * const indices ,
				      c_Index start , c_Index end ) const;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get a range of linearization coefficients
 /** Like get_linearization_coefficients( FunctionValue * g , name , ... )
  * but without the name, because a LinearFunction only have one
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
 /** There is only one linearization in a LinearFunction, its value being
  * the opposite of its constant term. */

 virtual double get_linearization_constant( const LinearizationName name =
   std::numeric_limits<Index>::max() ) const override final {
 return( f_constant_term );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 ///< Returns the value of the constant term of this LinearFunction.

 FunctionValue get_constant_term( void ) const { return( f_constant_term ); }

/*@} -----------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the LinearFunction
 *  @{ */

 ///< get the whole (empty) set of parameters in one blow
 /** Although a LinearFunction formally has a lot of parameters, in fact it
  * "listens to no-one"; hence, the implementation of get_ComputeConfig() is
  * quite a trivial one. */

 virtual ComputeConfig * get_ComputeConfig( bool all = false ,
		       ComputeConfig * ocfg = nullptr ) const override final
 {
  return( nullptr );
  }

/*@} -----------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE LinearFunction -------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" Variable in the
 * LinearFunction; this is the actual concrete implementation exploiting the
 * vector v_pairs of pairs.
 * @{ */

 virtual Index get_num_active_var( void ) const override final {
  return( v_pairs.size() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual Index is_active( const Variable * const var )
  const override final {
  auto idx = std::lower_bound( v_pairs.begin() , v_pairs.end() ,
                               std::make_pair( var , 0 ) ,
                               []( const auto & p1, const auto & p2 )
                                 { return p1.first < p2.first; } );
  if( idx < v_pairs.end() )
   return( std::distance( idx , v_pairs.begin() ) );
  else
   return( std::numeric_limits<Index>::infinity() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual void map_active( c_Vec_p_Var & vars , Vec_Index & map ,
			  const bool ordered = false ) const override final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual Variable *get_active_var( const Index i ) const override final {
  return( ( v_pairs.begin() + i )->first );
  }

/*--------------------------------------------------------------------------*/

 virtual v_iterator * v_begin( void ) override final
 {
  return( new LinearFunction::v_iterator( v_pairs.begin() ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual v_const_iterator * v_begin( void )
  const override final
 {
  return( new LinearFunction::v_const_iterator( v_pairs.begin() ) );
  }

/*--------------------------------------------------------------------------*/

 virtual v_iterator * v_end( void ) override final
 {
  return( new LinearFunction::v_iterator( v_pairs.end() ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual v_const_iterator * v_end( void )
  const override final
 {
  return( new LinearFunction::v_const_iterator( v_pairs.end() ) );
  }

/*--------------------------------------------------------------------------*/
 /// remove the given Variable from the LinearFunction
 /** Remove the given Variable from the LinearFunction. This is equivalent to
  * setting the corresponding coefficient to zero. */

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
  * min( stop , get_num_active_var() ) (excluded) in this LinearFunction. */

 void remove_variables( c_Index strt = 0 , Index stop = Inf<Index>() ,
			c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove a range of Variable
 /** Remove all the Variable comprised between strt (included) and stop
  * (excluded). Setting strt == nullptr means "the first Variable", and
  * setting stop == nullptr means "(one after) the last Variable". If
  * no-nullptr arguments are provided, they *must* be "names" of Variable
  * currently active in this LinearFunction. */

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

/*@} -----------------------------------------------------------------------*/
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
  * The parameter issueMod decides if and how the LinearFunctionMod is issued,
  * as described in Observer::make_par(). */

 void add_variables( v_coeff_pair && vars , const bool ordered = false ,
		     c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// add one single new Variable to the LinearFunction
 /** Like add_variables(), but just only one Variable. */

 void add_variable( ColVariable * const var , const Coefficient coeff ,
		    c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// modify a single existing coefficient
 /**  Method that receives a new coefficient for a specific Variable and
  * modifies it. If var is not an active variable in the LinearFunction,
  * exception is thrown. 
  *
  * The parameter issueMod decides if and how the LinearFunctionMod is issued,
  * as described in Observer::make_par(). */

 void modify_coefficient( ColVariable * const var , const Coefficient coeff ,
			  c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify a set of existing coefficients
 /** Method that receives a vector of pairs < ColVariable * , Coefficient >
  * so that the ColVariables are already in the LinearFunction, and modify
  * their coefficients accordingly. If any ColVariable in vars is not an
  * active variable in the LinearFunction, exception is thrown. As the the
  * && tells, vars is "consumed" by the method and its resources become
  * property of the LinearFunction object (which may dispatch them to the
  * Modification that it may issue).
  *
  * The parameter ordered tells if vars is already ordered by ColVariable
  * "name = pointer" or not, otherwise it may get ordered inside the
  * method (which is why it is not const).
  *
  * The parameter issueMod decides if and how the LinearFunctionMod is issued,
  * as described in Observer::make_par(). */

 void modify_coefficients( v_coeff_pair && vars , const bool ordered = false ,
			   c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify a set of existing coefficients
 /** Like modify_coefficients( v_coeff_pair * ), but takes in input a set of
  * index of the Variable whose coefficients need be modified, together with
  * (an iterator into) the vector of new coefficient values. Useful if one
  * knows the indices already, so that they need not be searched for. The
  * set of indices must be ordered in increasing sense. */

 virtual void modify_coefficients( c_v_coeff_it NCoef , c_Vec_Index &nms ,
				   c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify a range of coefficients
 /** Modify the coefficients of all the Variable that are in position from
  * start (included) to min( stop , get_num_active_var() ) (excluded) in this
  * LinearFunction. NCoef is a (const) iterator into a vector of coefficients
  * that must clearly be at least as long (from NCoef to the end) as
  * min( stop , get_num_active_var() ) - start. Useful if one knows the
  * indices already, so that they need not be searched for. */

 void modify_coefficients( c_v_coeff_it NCoef ,
			   c_Index strt = 0 , Index stop = Inf<Index>() ,
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
  * of strt and the one of stop (these included). */

 void modify_coefficients( c_v_coeff_it NCoef ,
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
 ///< sets the value of the constant term of this function.
 /** Method that sets the new value to the constant term of this linear
  * (actually, affine) Function to constant_term.
  *
  * The parameter issueMod decides if and how the LinearFunctionMod is issued,
  * as described in Observer::make_par(). */

 void set_constant_term( const FunctionValue constant_term ,
			 c_ModParam issueMod = eModBlck );

/*@} -----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

 /// printing the LinearFunction
 void print( std::ostream &output ) const override {
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

/*@}------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/

 void issue_add_variables_modification( v_coeff_pair & pairs ,
					c_ModParam issueMod );

/*--------------------------------------------------------------------------*/

 };  // end( class( LinearFunction ) )

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS LinearFunctionModRngd -------------------------*/
/*--------------------------------------------------------------------------*/
/// derived from C05FunctionModVarsRngd for changes in a range of coefficients
/** Derived class from C05FunctionModVarsRngd to describe changes specific to
 * a LinearFunction, i.e., those of a range of coefficients.
 *
 * The change of some of the coefficients in a linear function perfectly
 * coincides with what the type of modification of LinearFunctionModRngd
 * "SomeEntriesChange" postulates. Indeed, there is no real reason for
 * defining this class, as it is identical to LinearFunctionModRngd, save
 * for the fact that some Block / Solver may want to be sure that the
 * Modification is actually coming out of a LinearFunction. */

 class LinearFunctionModRngd : public C05FunctionModVarsRngd
 {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*-------------------- CONSTRUCTOR AND DESTRUCTOR --------------------------*/

 /// constructor: identical to that of C05FunctionModVarsRngd

 LinearFunctionModRngd( Function * const f , const int mod ,
			Variable * const strt = nullptr ,
			Variable * const stop = nullptr ,
			const FunctionValue shift = 0 , const bool cB = true )
  : C05FunctionModVarsRngd( f , mod , strt , stop , shift , cB ) { }

 virtual ~LinearFunctionModRngd() { }  ///< destructor, does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the LinearFunctionModRngd

 virtual inline void print( std::ostream &output ) const {
  output << "LinearFunctionModRngd[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function[" << f_function << " ]: ";
  switch( f_type ) {
   case( AddVar ): output << "add variables"; break;
   case( RemoveVar ): output << "delete variables"; break;
   case( SomeEntriesChange ): output << "modify coefficients";
   }
  output << "[ " << f_strt << ", " << f_stop << "]" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( LinearFunctionModRngd ) )

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS LinearFunctionModSbst -------------------------*/
/*--------------------------------------------------------------------------*/
/// derived from C05FunctionModVarsSbst for changes in subset of coefficients
/** Derived class from C05FunctionModVarsSbst to describe changes specific to
 * a LinearFunction, i.e., those of a subset of coefficients.
 *
 * The change of some of the coefficients in a linear function perfectly
 * coincides with what the type of modification of C05FunctionModVarsSbst
 * "SomeEntriesChange" postulates. Indeed, there is no real reason for
 * defining this class, as it is identical to C05FunctionModVarsSbst, save
 * for the fact that some Block / Solver may want to be sure that the
 * Modification is actually coming out of a LinearFunction. */

 class LinearFunctionModSbst : public C05FunctionModVarsSbst
 {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*-------------------- CONSTRUCTOR AND DESTRUCTOR --------------------------*/

 /// constructor: identical to that of C05FunctionModVarsSbst

 LinearFunctionModSbst( Function * const f , const int mod ,
			std::vector<Variable *> && vars ,
			FunctionValue shift = 0 , const bool cB = true )
  : C05FunctionModVarsSbst( f , mod , std::move( vars ) , shift , cB ) { }

 virtual ~LinearFunctionModSbst() { }  ///< destructor, does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the LinearFunctionModSbst

 virtual inline void print( std::ostream &output ) const {
  output << "LinearFunctionModSbst[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function[" << f_function << " ]: ";
  switch( f_type ) {
   case( AddVar ): output << "add variables"; break;
   case( RemoveVar ): output << "delete variables"; break;
   case( SomeEntriesChange ): output << "modify coefficients";
   }
  output << "(# " << v_vars.size() << ")" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( LinearFunctionModSbst ) )

/*@}  end( group( LFun_CLASSES ) ) */
/*--------------------------------------------------------------------------*/

 }  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* LinearFunction.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File LinearFunction.h --------------------------*/
/*--------------------------------------------------------------------------*/
