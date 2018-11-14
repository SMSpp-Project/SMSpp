/*--------------------------------------------------------------------------*/
/*------------------------- File DQuadFunction.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *concrete* class DQuadFunction, which
 * implements C15Function with a diagonal quadratic function.
 *
 * \version 0.10
 *
 * \date 29 - 11 - 2017
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
 * Copyright &copy by Antonio Frangioni, Rafael Durbano Lobato, Kostas
 * Tavlaridis-Gyparakis
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
#include "Block.h"
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
/** @defgroup DQuadFun_CLASSES Classes in DQuadFunction.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS DQuadFunction ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a diagonal quadratic Function
/**< The class DQuadFunction implements C15Function with a diagonal
 * quadratic function of the form
 *
 * f(x) = c + sum{i in I} ( a_i * x_i * x_i + b_i * x_i )
 *
 * where the scalar c is the constant term of the Function, and a_i
 * and b_i are the coefficients of the Variable x_i in the quadratic
 * and linear terms, respectively, for all i in I.
 *
 * This Function throws the following modifications:
 *
 * - When Variables are added, a FunctionModificationVariables
 *   modification of type AddVar is thrown. Pointers to the Variables
 *   that were added are provided in the Modification.
 *
 * - When Variables are removed, a FunctionModificationVariables
 *   modification of type RemoveVar is thrown. Pointers to the
 *   Variables that were removed are provided in the Modification.
 *
 * - When the coefficients of some Variables change, a
 *   C05FunctionModificationVariables modification of type
 *   SubgradientEntriesChange is thrown. The entries of a subgradient
 *   affected by this modification are precisely the ones associated
 *   with the Variables whose coefficients have changed. Therefore,
 *   the list of entries of the subgradient affected by this
 *   modification also informs the Variables whose coefficients have
 *   changed.
 *
 * - When the constant term changes, a FunctionModification is thrown
 *   with the shift caused by this modification. The shift is given by
 *   the difference between the new and old constant term values.
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

 typedef std::tuple<ColVariable *, Coefficient, Coefficient>
 var_coeff_coeff_triple;
 /**< Triple (ColVariable *, Coefficient, Coefficient) to store a
  * pointer to a Variable and the coefficients of the linear and
  * diagonal quadratic terms associated with that Variable. */

 typedef std::vector<var_coeff_coeff_triple> v_var_coeff_coeff_triple;
 ///< a vector of var_coeff_coeff_triple

 typedef const var_coeff_coeff_triple c_var_coeff_coeff_triple;
 ///< a const var_coeff_coeff_triple

 typedef std::vector<var_coeff_coeff_triple> v_c_var_coeff_coeff_triple;
 ///< a vector of const var_coeff_coeff_triple

 typedef std::vector< std::pair<Coefficient, Coefficient> > v_coeff_coeff;
 ///< a vector of pairs of Coefficients

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
  * coefficient of the variable in the diagonial quadratic term of the
  * function (and a bool telling if this vector is already ordered by
  * ColVariable "name = pointer" or not, in which case they are
  * ordered).
  *
  * Note that if this vector of triples is passed, it becomes property
  * of the DQuadFunction, which therefore has the responsibility to
  * delete it.
  *
  * All inputs have a default (nullptr, false respectively), so that
  * this can be used as the void constructor. */
  DQuadFunction(v_var_coeff_coeff_triple *v_var = nullptr ,
                const bool ordered = false)
    : C15Function () {

    v_variables = v_var;
    if( v_variables && ( ! ordered ) )
      std::sort( v_variables->begin() , v_variables->end() );

    f_value = 0;
    Lipschitz_constant = Inf<double>();
  }

/*--------------------------------------------------------------------------*/
  /// destructor: deletes the vector of variables and coefficients

  virtual ~DQuadFunction() {
    delete v_variables;
  }

/*@} -----------------------------------------------------------------------*/
/*----------- METHODS FOR READING THE DATA OF THE DQuadFunction ------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the DQuadFunction
    @{ */

 /// returns the vector of triples (ColVariable *, Coefficient, Coefficient)
 const v_c_var_coeff_coeff_triple * get_v_var( void ) const {
   return( v_variables );
 }

/*@} -----------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE DQuadFunction -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the DQuadFunction
 *  @{ */

 virtual void evaluate( void ) override;

/*--------------------------------------------------------------------------*/

 virtual bool is_active( const Variable * const f_variable ) const override {
   return std::binary_search( v_variables->begin() , v_variables->end(),
                              std::make_tuple(f_variable, 0, 0),
                              []( const auto & t1, const auto & t2 )
                              { return get<0>(t1) < get<0>(t2); } );
 }

/*--------------------------------------------------------------------------*/

 virtual int get_num_active_var( void ) const override {
   return( v_variables->size() );
 }

/*--------------------------------------------------------------------------*/

 virtual Variable *get_active_var( const int i ) const override {
   return std::get<0>( * ( v_variables->begin() + i ) );
 }

/*--------------------------------------------------------------------------*/

 /// Returns the Coefficient in the linear term of the i-th Variable
 /** This method returns the Coefficient, in the linear term, of the
  * i-th Variable of this diagonal quadratic function. The index i
  * must be between 0 and get_num_active_var() - 1.
  *
  * @param i Index of the Variable whose coefficient is desired.
  */
 virtual Coefficient get_linear_coefficient( const int i ) const {
   return std::get<1>( * ( v_variables->begin() + i ) );
 }

/*--------------------------------------------------------------------------*/

  /// Returns the Coefficient in the quadratic term of the i-th Variable
 /** This method returns the Coefficient, in the quadratic term, of
  * the i-th Variable of this diagonal quadratic function. The index i
  * must be between 0 and get_num_active_var() - 1.
  *
  * @param i Index of the Variable whose coefficient is desired.
  */
 virtual Coefficient get_quadratic_coefficient( const int i ) const {
   return std::get<2>( * ( v_variables->begin() + i ) );
 }

/*--------------------------------------------------------------------------*/

 virtual void set_lower_target( FunctionValue lower_target ) override {}

/*--------------------------------------------------------------------------*/

 virtual void set_upper_target( FunctionValue upper_target ) override {}

/*--------------------------------------------------------------------------*/

 virtual void set_epsilon( FunctionValue epsilon ) override {}

/*--------------------------------------------------------------------------*/

 virtual FunctionValue get_value( void ) const override { return( f_value ); }

/*--------------------------------------------------------------------------*/

 virtual FunctionValue get_lower_estimate( void ) const override {
   return get_value();
 }

/*--------------------------------------------------------------------------*/

 virtual FunctionValue get_upper_estimate( void ) const override {
   return get_value();
 }

/*--------------------------------------------------------------------------*/

 virtual double get_Lipschitz_constant() override;

/*--------------------------------------------------------------------------*/

 virtual bool is_convex( void ) const override;

/*--------------------------------------------------------------------------*/

 virtual bool is_concave( void ) const override;

/*--------------------------------------------------------------------------*/

 virtual bool is_lower_semicontinuous( void )
   const override final { return true; }

/*--------------------------------------------------------------------------*/

 virtual bool is_upper_semicontinuous( void )
   const override final { return true; }

/*--------------------------------------------------------------------------*/

 virtual bool is_linear( void ) const override;

/*--------------------------------------------------------------------------*/

 virtual void compute_hessian_approximation( void ) override { };

/*--------------------------------------------------------------------------*/

 virtual void get_hessian_approximation( DenseHessian &hessian ) const override;

/*--------------------------------------------------------------------------*/

 virtual void get_hessian_approximation( SparseHessian &hessian )
   const override;

/*--------------------------------------------------------------------------*/

 virtual bool is_continuously_differentiable( void ) const override final {
   return true;
 }

/*--------------------------------------------------------------------------*/

 virtual bool is_twice_continuously_differentiable( void )
   const override final { return true; }

/*--------------------------------------------------------------------------*/

 virtual bool compute_new_subgradient( void ) override;

/*--------------------------------------------------------------------------*/

 virtual void get_subgradient( DenseSubgradient &subgradient ) const override;

/*--------------------------------------------------------------------------*/

 virtual void set_maximum_number_of_subgradients( const int n ) override { }

/*--------------------------------------------------------------------------*/

 virtual bool compute_new_subgradient_approximation( void ) override {
   return compute_new_subgradient();
 }

/*--------------------------------------------------------------------------*/

 virtual double get_subgradient_approximation( DenseSubgradient &subgradient )
   const {
   get_subgradient( subgradient );
   return 0.0;
 }

/*--------------------------------------------------------------------------*/

/*@} -----------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE DQuadFunction -------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the DQuadFunction
 *  @{ */

 void add_variables( v_var_coeff_coeff_triple *v_var ,
                     const bool ordered = true );

 /**< Method that receives a pointer to a vector of triples
  * (ColVariable *, double, double) and adds these triples to the list
  * of triples already in the DQuadFunction that form the linear and
  * the diagonal quadratic parts of the function. If any variable in
  * v_var is already an active variable in the DQuadFunction, an
  * exception is thrown. ordered tells if v_var is already ordered by
  * ColVariable "name = pointer" or not.
  *
  * Important note: the vector v_var becomes property of the
  * DQuadFunction, which then takes care of deleting it. This may
  * mean that the vector is actually passed to the appropriate
  * DQuadFunctionModification, and it is destroyed when it is. */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void add_variable( ColVariable * const variable ,
                    const Coefficient linear_coefficient ,
                    const Coefficient quadratic_coefficient );

 /**< Method that receives a pointer to a ColVariable and adds it to
  * this DQuadFunction with the given coefficients. The first
  * coefficient is the coefficient of the given Variable in the linear
  * term and the second is the coefficient of the given Variable in
  * the diagonal quadratic term of this DQuadFunction. If this
  * variable is already an active variable in this DQuadFunction, an
  * exception is thrown. */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void delete_variables( std::vector<Variable *> *v_var ,
                        const bool ordered = false ,
                        const bool throwModification = true ) override;

 /**< Method that receives a vector of pointers to Variables and
  * deletes them from the list of triples already in the DQuadFunction
  * that form the linear and the diagonal parts of the function. If
  * any variable in v_var is not an active variable in this
  * DQuadFunction, an exception is thrown. ordered tells if v_var is
  * already ordered by Variable "name = pointer" or
  * not. throwModification tells whether a modification should be
  * thrown.
  *
  * Important note: the vector v_var becomes property of the
  * DQuadFunction, which then takes care of deleting it. This may
  * mean that the vector is actually passed to the appropriate
  * DQuadFunctionModification, and it is destroyed when it is. */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual void delete_variable( Variable * variable ,
                               const bool throwModification = true ) override;

 /**< Method that receives a pointer to a Variable and deletes it from
  * this DQuadFunction. If this Variable is not an active Variable in
  * this DQuadFunction, an exception is thrown. throwModification
  * tells whether a modification should be thrown. */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void modify_coefficients( v_var_coeff_coeff_triple *v_var ,
                           const bool ordered = true );

 /**< Method that receives a vector of triples (ColVariable *, double,
  * double) so that the ColVariables are already part of the
  * DQuadFunction, and modify their coefficients (in the linear and
  * diagonal quadratic parts of the function) accordingly. If any
  * variable in v_var is not an active variable in this DQuadFunction,
  * an exception is thrown. ordered tells if v_var is already ordered
  * by ColVariable "name = pointer" or not.
  *
  * Important note: the vector v_var becomes property of the
  * DQuadFunction, which then takes care of deleting it. This may
  * mean that the vector is actually passed to the appropriate
  * DQuadFunctionModification, and it is destroyed when it is. */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void set_constant_term( const FunctionValue constant_term );
 ///< Sets the value of the constant term of this function.

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 FunctionValue get_constant_term( void ) const;
 ///< Returns the value of the constant term of this function.

/*@} -----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

 /// resets this Function
 /** It resets this Function. This means that the subgradient that has
  * posssibly been computed is discarded and the method
  * compute_new_subgradient() should return true in the next call. */
 virtual void reset() {
   subgradient_computed = false;
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 /// printing the DQuadFunction

 void print( std::ostream &output ) const {
  output << "DQuadFunction [" << this << "] of Block ["
	 << f_Block << "] with " << get_num_active_var()
	 << " active variables;";
  output << "current value = " << get_value();
  }

/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 v_var_coeff_coeff_triple * v_variables;
 /**< vector of triples (ColVariable *, Coefficient, Coefficient)
  * characterizing the linear and the diagonal quadratic terms of the
  * function; the vector is kept sorted in an ascending order based on
  * the pointers of the ColVariables. */

 FunctionValue f_value;  ///< the value of the function

 FunctionValue constant_term = 0;
 ///< the value of the constant term of this function

 double Lipschitz_constant; //< the Lipschitz constant of the function

 bool subgradient_computed = false;
 /**< indicates the number of subgradients computed after the function
  * is evaluated. */

/*@}------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

  void throw_add_variables_modification( v_var_coeff_coeff_triple *v_var );

  void throw_delete_variable_modification( std::vector<Variable *> *v_var );

  void throw_modify_coefficients_modification
  ( v_var_coeff_coeff_triple * v_var );

  void throw_constant_term_modification( FunctionValue old_constant_term,
                                         FunctionValue new_constant_term );

/*--------------------------------------------------------------------------*/

 };  // end( class( DQuadFunction ) )

/*@}  end( group( DQuadFun_CLASSES ) ) */
/*--------------------------------------------------------------------------*/

 }  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* DQuadFunction.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File DQuadFunction.h ---------------------------*/
/*--------------------------------------------------------------------------*/
