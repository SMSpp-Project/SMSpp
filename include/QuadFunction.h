/*--------------------------------------------------------------------------*/
/*------------------------- File QuadFunction.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *concrete* class QuadFunction, which implements
 * C15Function with a non-diagonal quadratic function.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Wim van Ackooij \n
 *         EDF Lab Paris-Saclay \n
 *
 *
 * \copyright &copy; by Antonio Frangioni, Wim van Ackooij
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __QuadFunction
 #define __QuadFunction
                     /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "C15Function.h"
#include "DQuadFunction.h"

#include "ColVariable.h"

#include "Observer.h"

#include <cmath>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS QuadFunction ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a quadratic Function
/**< The class QuadFunction implements C15Function with a non-diagonal
 * quadratic function of the form
 * \f[
 * f(x) = c + sum{i = 1, ..., n} ( a_i * x_i^2 + b_i * x_i ) + 
 *            sum_{j = 1, ..., n, i < j} a_{ij}x_i x_j
 * \f]
 * where the scalar c is the constant term of the Function, and a_i and b_i
 * are the coefficients of the Variable x_i in the quadratic and linear
 * terms, respectively. Moreover, a_{ij} are the off diagonal elements ; 
 * We can ofcourse assume that the quadratic matrix is symmetric, since 
 * we can always write 
 *    sum_{j = 1, ..., n, i < j} a_{ij}x_i x_j =
 *       sum_{j = 1, ..., n, i < j} a_{ij}/2 x_i x_j +
 *       sum_{j = 1, ..., n, i > j} a_{ij}/2 x_i x_j
 *
 * Globally the function is understood as the non-diagonal terms + DQuadFunction
 * 
 * This Function issues the following :Modification:
 *
 *
 * 
 * 
 */
 
class QuadFunction : public DQuadFunction {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------- PUBLIC TYPES OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 /// one term of the sum: < Index , Index , Coefficient >
 /** Triple < Index , Index , Coefficient > that describes
  * the term of the sum corresponding to a_{ij}x_i x_j . 
  * 
  * Internally the collection of off_diagonal terms are not stored as a 
  * random collection of triplets, but rather the (lower half) of a SparseMatrix
  * It will be assumed throughout that a ColVariable corresponding to Index 
  * actually exists in DQuadFunction
  * This will not be explicitly checked, in principle
  * */
 using off_diag_term = std::tuple< Index , Index , Coefficient >;

 using c_off_diag_term = const off_diag_term;  ///< a const off_diag_term
 
 /// a vector of off_diag_term
 using v_off_diag_term = std::vector< off_diag_term >;

 /// a const vector of off_diag_term
 using v_c_off_diag_term = const v_off_diag_term;

 using Qmat = Eigen::SparseMatrix< Coefficient >; 

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and Destructor
 *  @{ */

 /// constructor of QuadFunction, taking the data describing it
 /** Constructor of QuadFunction. It accepts:
  *
  * @param v_var, a && to a vector of triples < ColVariable * , Coefficient ,
  *        Coefficient >, with the first coefficient being that of the linear
  *        term and the second being that of the quadratic term of the
  *        function corresponding to the given ColVariable; as the &&
  *        tells, vars is "consumed" by the constructor and its resources
  *        become property of the LinearFunction object.
  * 
  * @param v_nd_var a && to a vector of triples < Index , Index ,
  *        Coefficient >, providing the off diagonal terms of the 
  *        quadratic function. Here the indexes refer to variables 
  *        in v_var and as a result we must have Index i < v_var.size(). 
  *
  * @param ct, a FunctionValue providing the value of the constant term of the
  *        function;
  *
  * @param observer, a pointer to the Observer of this QuadFunction.
  *
  * All inputs have a default ({}, 0, and nullptr, respectively) so that this
  * can be used as the void constructor.
  *
  * Important note:
  *
  *     THE ORDER OF THE vars VECTOR WILL DICTATE THE ORDER OF THE "ACTIVE"
  *     [Col]Variable OF THE QuadFunction
  *
  * That is, get_active_var( 0 ) == std::get< 0 >( vars[ 0 ] ),
  * get_active_var( 1 ) == std::get< 0 >( vars[ 1 ] ), ... */

 explicit QuadFunction( v_coeff_triple && v_var = {} ,
			v_off_diag_term && v_nd_var = {} ,
			const FunctionValue ct = 0 ,
			Observer * const observer = nullptr )
  : DQuadFunction( std::move( v_var ) , ct , observer ) ,
  my_convexity( Unknown )
 { 
  /** Copy the all the off diagonal terms in an Eigen::SparseMatrix.
   *  For this we must copy the information to appropriate Eigen triplets. */
  mat_nd.resize( DQuadFunction::get_num_active_var() ,
		 DQuadFunction::get_num_active_var() );
  std::vector< Eigen::Triplet< Coefficient > > vv_nd;
  vv_nd.reserve( v_nd_var.size() );
  for( boost::multi_array_types::size_type i = 0 ;
       i < v_nd_var.size() ; ++i ) {
   #ifndef NDEBUG
    // std::cout << "mat_nd is : " << mat_nd.rows() << " x "
    //           << mat_nd.cols() << "\n";
    if( ( std::min( std::get< 0 >( v_nd_var[ i ] ) ,
		    std::get< 1 >( v_nd_var[ i ]) ) < 0 ) ||
	( std::min( std::get< 0 > ( v_nd_var[ i ] ) ,
		    std::get< 1 >( v_nd_var[ i ] ) ) >= mat_nd.rows() ) )
     throw( std::invalid_argument(
			  "Invalid tuplet provided : out of bounds : " +
			  std::to_string( std::get< 0 >( v_nd_var[ i ] ) ) +
			  std::to_string( std::get< 1 >( v_nd_var[ i ] ) ) +
			  " for term " + std::to_string( i ) ) ); 
   #endif
   Eigen::Triplet< Coefficient > term( std::get< 0 >( v_nd_var[ i ] ) ,
				       std::get< 1 >( v_nd_var[ i ] ) ,
				       std::get< 2 >( v_nd_var[ i ] ) );
   vv_nd.push_back( term ); 
   }
  mat_nd.setFromTriplets( vv_nd.begin(), vv_nd.end() );
  #ifndef NDEBUG
   for( Index k = 0 ; k < mat_nd.outerSize() ; ++k )
    for( Qmat::InnerIterator it( mat_nd , k ) ; it ; ++it ) {
     if( it.col() > it.row() )
      throw( std::invalid_argument(
	"QuadFunction: non-diagonal terms must be lower diagonal" ) );
            
     if( it.col() == it.row() )
      throw( std::invalid_argument(
 	  "QuadFunction: non-diagonal terms cannot contain diagonal elements"
				   ) ); 
     }
  #endif
  }

/*--------------------------------------------------------------------------*/
 /// destructor: it does nothing (explicitly)

 ~QuadFunction() override = default;

/*--------------------------------------------------------------------------*/
 /// clear method: clears the mat_nd field
 /** Method to "clear" the QuadFunction: it clear() the matrix mat_nd.
  * This destroys the list of "active" Variable without unregistering from
  * them. Not that the LinearFunction would have, but the Observer may.
  * By not having any Variable, the Observer can no longer do that. */

 void clear( void ) override { 
    // Clear the matrix and clear all memory
     DQuadFunction::clear();

    mat_nd.resize(0,0); 
    mat_nd.data().squeeze();
 }

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/


/** @} ---------------------------------------------------------------------*/
/*----------- METHODS FOR READING THE DATA OF THE QuadFunction ------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the QuadFunction
    @{ */

 /// returns the vector of triples (Index, Index, Coefficient)

 void get_v_nd_var( v_off_diag_term & v_nd_triples ) const { 
    v_nd_triples.resize( mat_nd.nonZeros() ); 
    int k_nd_t = 0;
    for (int k=0; k < mat_nd.outerSize(); ++k){
      for (Qmat::InnerIterator it(mat_nd,k); it; ++it){
        v_nd_triples[k_nd_t] = std::make_tuple( it.row(), it.col(), it.value() );
        ++k_nd_t;
      } 
    }
 }
/*--------------------------------------------------------------------------*/

 /// returns directly the Eigen::SparseMatrix

 Qmat get_matrix( void ) const { return mat_nd; }
 
/*--------------------------------------------------------------------------*/

 /// returns the term of the i-th and j-th Variable 
 /**
  * This approach makes the matrix symmetric, assuming it is stored "sparsely"
  *  Both indexes must be between 0 and get_num_active_var() - 1.
  *  @param i Index of the first Variable 
  *  @param j Index of the second Variable 
  * */
 Coefficient get_quadratic_coefficient( Index i, Index j ) const {
    #ifndef NDEBUG
      if ( ( std::min(i,j) < 0 ) || ( std::max(i,j) >= DQuadFunction::get_num_active_var() ) ){
        throw( std::invalid_argument( "QuadFunction::get_quadratic_coefficient: invalid "
                                  "index: " + std::to_string( i ) + " , " + std::to_string( j ) ) );
      }
    #endif
    if ( i < j ){
      return mat_nd.coeff( j , i);
    }
    else if ( i == j){
      return DQuadFunction::get_quadratic_coefficient( i );
    }
    else{
      return mat_nd.coeff( i , j);
    }
  }

/** @} ---------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE QuadFunction -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the QuadFunction
 *  @{ */

 int compute( bool changedvars ) override;

/*--------------------------------------------------------------------------*/
 /// returns the value of the QuadFunction

 FunctionValue get_value( void ) override { return( f_value ); }

/*--------------------------------------------------------------------------*/

 bool is_convex( void ) override;

/*--------------------------------------------------------------------------*/

 bool is_concave( void ) override;

/*--------------------------------------------------------------------------*/

 bool is_linear( void ) override;

/*--------------------------------------------------------------------------*/

 void compute_hessian_approximation( void ) override { };

/*--------------------------------------------------------------------------*/

 void get_hessian_approximation( DenseHessian & hessian ) const override;

/*--------------------------------------------------------------------------*/

 void get_hessian_approximation( SparseHessian & hessian ) const override;

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
 /// returns the linearization constant of the current point
 /** Since QuadFunction currently does not support the global pool, the
  * method only works for returning the value of the linearization constant
  * at the current point x, which is given by c - x^T A x. */

 FunctionValue get_linearization_constant( Index name = Inf< Index >() )
  override final {
  if( name < Inf< Index >() )
   throw( std::invalid_argument(
			"global pool not supported yet by QuadFunction" ) );

  FunctionValue quadratic_term = 0.0;

  for( const auto & triple : DQuadFunction::v_triples ) {
   auto variable_value = std::get< 0 >( triple )->get_value();
   auto quadratic_coefficient_value = std::get< 2 >( triple );
   quadratic_term += variable_value * variable_value *
                     quadratic_coefficient_value;
   }

  // Cache the variable values prior to use, in case get_value() is costly
  std::vector< double > vals( DQuadFunction::get_num_active_var() );
  for( boost::multi_array_types::size_type i = 0 ; i < vals.size() ; ++i )
   vals[ i ] = std::get< 0 >( DQuadFunction::v_triples[ i ] )->get_value();

  for( int k = 0 ; k < mat_nd.outerSize() ; ++k )
   for( Qmat::InnerIterator it( mat_nd , k ) ; it ; ++it ) {
    auto variable_value1 = vals[ it.row() ];
    auto variable_value2 = vals[ it.col() ];
    quadratic_term += variable_value1 * variable_value2 * it.value();
    }

  return( this->f_constant_term - quadratic_term );
  }

/** @} ---------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/

/** @} ---------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE QuadFunction --------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//  MAPS TODO ??
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/


/** @} ---------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE QuadFunction -------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the QuadFunction
 *  @{ */

 /// add a set of new Variable to the QuadFunction
 /**< Method that receives a pointer to a vector of triples < ColVariable * ,
  * Coefficient , Coefficient > and adds these triples to the list of
  * triples already in the QuadFunction. The first coefficient is that in
  * the linear term and the second one is that in the quadratic term. Note
  * that
  *
  *     IT IS EXPECTED THAT NONE OF THE NEW ColVariable IS ALREADY "ACTIVE"
  *     IN THE QuadFunction, BUT NO CHECK IS DONE TO ENSURE THIS
  *
  * Indeed, the check is costly, and the QuadFunction does not really have
  * a functional issue with repeated ColVariable. The real issue rather comes
  * whenever the QuadFunction is used within a Constraint or Objective that
  * need to register itself among the "active" Variable of the QuadFunction;
  * this process is not structured to work with multiple copies of the same
  * "active" Variable. Thus, a QuadFunction used within such an object
  * should not have repeated Variable, but if this is an issue then the
  * check will have to be performed elsewhere, it is not done here.
  *
  * As the && tells, vars is "consumed" by the method and its resources
  * become property of the QuadFunction object.
  * 
  * We also add the non-diagonal terms, if any, this can be empty ;
  * The added variables come with ``absolute indexes", in particular cross terms with
  * existing variables can be specified
  * 
  *
  * The parameter issueMod decides if and how the C05FunctionModVars is
  * issued, as described in Observer::make_par(). Note that a diagonal
  * quadratic function is additive, and therefore strongly quasi-additive. */

 void add_variables( v_coeff_triple && vars , v_off_diag_term && v_nd_var , 
                     ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 
 /// add just a single non-diagonal term to the QuadFunction
 /** We have the two Variables and the coefficient
  *  This approach will first have to check if indeed both var1 and var2 exist 
  *  if not, the missing variables will be added and the non-diagonal term expanded
  *  in the rare case when quad_coeff = 0.0 nothing is done at all
  * 
  */
 void add_nd_term ( ColVariable * var1 , ColVariable * var2 ,
                    Coefficient quad_coeff , ModParam issueMod = eModBlck );
 
/*--------------------------------------------------------------------------*/

 /// modify a single existing non-diagonal quadratic term
 /** for
  * the i-th and j-th "active" Variables; 
  * if neither i nor j are valid indexes, exception is
  * thrown. 
  *
  * The parameter issueMod decides if and how the C05FunctionModRngd is
  * issued, as described in Observer::make_par(). */

 void modify_term( Index i , Index j ,
                   Coefficient quad_nd_coeff , ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// modify a set of existing quadratic terms : TODO

/*--------------------------------------------------------------------------*/
 /// remove the i-th "active" Variable from the QuadFunction
 /** Remove the i-th "active" Variable from the QuadFunction. This is
  * *mathematically* equivalent to setting both its linear and quadratic
  * coefficients to zero, but it is considered a "stronger" operation (it is
  * possible to have an active Variable with both zero coefficients). If there
  * is no Variable with the given index, an exception is thrown.
  *
  * The parameter issueMod decides if and how the C05FunctionModVarsRngd is
  * issued, as described in Observer::make_par(). Note that a diagonal
  * quadratic function is additive, and therefore strongly quasi-additive,
  * which is why a C05FunctionModVarsRngd is issued as opposed to a
  * FunctionModVarsRngd one. 
  * 
  * In our case this will delete all non-diagonal terms involving this index i;
  * 
  * */

 void remove_variable( Index i , ModParam issueMod = eModBlck )
  override final;

/*--------------------------------------------------------------------------*/
 /// remove a range of "active" Variable
 /** Remove all the "active" Variable in the given Range, i.e., all those
  * with index i s.t. range.first <= i < min( range.second ,
  * get_num_active_var() ), from this QuadFunction.
  *
  * The parameter issueMod decides if and how the C05FunctionModVarsRngd is
  * issued, as described in Observer::make_par(). Note that a linear function
  * is additive, and therefore strongly quasi-additive, which is why a
  * C05FunctionModVarsRngd is issued as opposed to a FunctionModVarsRngd
  * one. 
  * 
  * In our case this will delete all non-diagonal terms involving this index Range;
  *
  * */

 void remove_variables( Range range , ModParam issueMod = eModBlck )
  override final;
  

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

 /// printing the QuadFunction
 void print( std::ostream & output ) override {
  output << "QuadFunction [" << this << "] observed by ["
         << &f_Observer << "] with " << get_num_active_var()
         << " active variables;";
  output << "current value = " << get_value();
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 // It makes sense to store the Child version of the function value too
 FunctionValue f_value;  ///< the value of the function

 Qmat mat_nd; ///< Matrix with the off diagonal terms

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

  /**
  * The following are internal flags to remember what kind of function we are dealing
  * with ; The flag NotConvex means that the function is neither convex nor concave
  * and that this is known with certainty ; This evidently means that the Hessian is
  * indefinite ; Unkown is we do not know yet or some modification has ruined our 
  * former knowledge 
  *
  */
  enum fctType { Convex, Concave, NotConvex, Unknown};
  fctType my_convexity;

  /**
   * 
   * Identify the convexity type of this function ;
   * 
  */
  void identify_convexity();

/*--------------------------------------------------------------------------*/

 };  // end( class( QuadFunction ) )

/*--------------------------------------------------------------------------*/
/*------------------- Class QuadFunctionModVarsAddd ----------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe "nicely" adding Variable of a QuadFunction
/** Derived class from DQuadFunctionModVarsAddd to describe adding "active"
 * ColVariables to a QuadFunction. The only difference with the base
 * DQuadFunctionModVarsAddd is that the QuadFunctionModVarsAddd stores the
 * values of the off-diagonal coefficients involving the new
 * ColVariables in the QuadFunction at the time when they are added. */

class QuadFunctionModVarsAddd : public DQuadFunctionModVarsAddd
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: that of DQuadFunctionModVarsAddd plus the coefficients
 /** Constructor of QuadFunctionModVarsAddd: takes the same arguments as
  * that of DQuadFunctionModVarsAddd plus a
  * std::vector( std::tuple< Index , Index , Coefficient > ) with the 
  * off-diagonal terms involving the newly added ColVariables.
  */

 QuadFunctionModVarsAddd( C05Function * f ,
         QuadFunction::v_off_diag_term && od_terms ,
			   DQuadFunction::v_coeff_pair && coeff ,
			   Vec_p_Var && vars , Index first ,
			   FunctionValue shift = NaNshift , bool cB = true )
  : DQuadFunctionModVarsAddd( f , std::move( coeff ) , std::move( vars ) , 
                              first , shift , cB ) ,
    offdiag_terms( std::move( od_terms ) ) {}

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: does nothing (explicitly)
 
 ~QuadFunctionModVarsAddd() override = default;

/*-------------------------------- GETTERS ----.----------------------------*/
 /// returns (a const reference to) the vector of off diagonal terms
 
 QuadFunction::v_c_off_diag_term & od_terms( void ) const 
  { return( offdiag_terms ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the QuadFunctionModVarsAddd

 void print( std::ostream & output ) const override {
  output << "QuadFunctionModVarsAddd[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function [" << f_function
         << " ]: strongly quasi-additively (";

  if( std::isnan( f_shift ) )
   output << "+-(?)";
  else
   if( f_shift >= Inf< FunctionValue >() )
    output << "+(?)";
   else
    if( f_shift <= - Inf< FunctionValue >() )
     output << "-(?)";
    else
     output << f_shift;

  output << ") adding variables [ " << f_first << " , "
         << f_first + v_vars.size() << " ]" << std::endl;
  }

/*--------------------------- PROTECTED FIELDS -----------------------------*/

 QuadFunction::v_off_diag_term offdiag_terms;
 ///< the off diagonal terms involving the newly added ColVariables
  
/*--------------------------------------------------------------------------*/

 };  // end( class( QuadFunctionModVarsAddd ) )

/*--------------------------------------------------------------------------*/
/*---------------------- Class QuadFunctionModSbst -------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe changes to a QuadFunction involving a subset of Variable
/** Derived class from C05FunctionModSbst to describe modification to a range 
 * of "active" ColVariable of a QuadFunction. The only difference with the base
 * C05FunctionModSbst is that the QuadFunctionModSbst stores the difference
 * between the old and the new values of off-diagonal coefficients involving
 * modified ColVariable in the QuadFunction. 
 *
 * NOTE: at the moment we assume that a Modification corresponding to a 
 * change in the off-diagonal part of the quadratic matrix involves a single
 * coefficient. Thus, in the subset field we will find only the two 
 * variables affected by this modification (i.e. if we are modifying the term
 * x_i*x_j, in the Subset field we will find the tuple (i,j)). */

class QuadFunctionModSbst : public C05FunctionModSbst
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------- PUBLIC TYPES OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 /// type of the coefficients of quadratic terms = FunctionValue
 using Coefficient = FunctionValue;

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: that of C05FunctionModSbst plus the coefficient
 /** Constructor of QuadFunctionModSbst: takes the same arguments as
  * that of C05FunctionModSbst plus a single Coefficient with the delta
  * value for the off diagonal term identified by the Subset. Moreover,
  * we also store a boolean value "is_added" to understand if the term have been
  * added to the quadratic matrix or simply modified. */ 

 QuadFunctionModSbst( C05Function * f , int type,
			   Coefficient && od_term , bool added ,
			   Vec_p_Var && vars , Subset && subset , bool ordered = false ,
			   Subset && which = {} , FunctionValue shift = NaNshift , 
         bool cB = true )
  : C05FunctionModSbst( f , type , std::move( vars ) , std::move( subset ) , 
      ordered , std::move( which ) , shift , cB ) , 
    d_coeff( std::move( od_term ) ) { is_added = added; }

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: does nothing (explicitly)
 
 ~QuadFunctionModSbst() override = default;

/*-------------------------------- GETTERS ----.----------------------------*/
 /// returns (a const reference to) the coefficient
 
 Coefficient delta( void ) const { return( d_coeff ); }

 /// returns the type of Modification
 
 bool Mod_type( void ) const { return( is_added ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the QuadFunctionModSbst

 void print( std::ostream & output ) const override {
  output << "QuadFunctionModSbst[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function [" << f_function << " ]: in ";
  if( ( f_type == AllEntriesChanged ) ||
      ( f_type == AllLinearizationChanged ) ) {
   if( v_which.empty() )
    output << "all";
   else
    output << v_which.size();
   output << "linearizations";
   if( f_type == AllEntriesChanged )
    output << " the g have changed";
   else
    output << "both \alpha and g have changed";
   }
  else
   output << "[ incorrect type() ]";
  if( f_shift ) {
   output << ", f-values changed";
   if( std::isnan( f_shift ) )
    output << "(+-)";
   else
    if( f_shift >= INFshift )
     output << "(+)";
    else
     if( f_shift <= - INFshift )
      output << "(-)";
     else
      output << " by " << f_shift;
   }
  output << std::endl;
  }

/*--------------------------- PROTECTED FIELDS -----------------------------*/

 Coefficient d_coeff;   /**< the delta coefficient (quadratic) given to the
			 * term identified by the subset of ColVariable */
 bool is_added;         /**< boolean value stating if the term have been
			 * added for the first time to the quadratic matrix */
  
/*--------------------------------------------------------------------------*/

 };  // end( class( QuadFunctionModSbst ) ) 

/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* QuadFunction.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File QuadFunction.h ---------------------------*/
/*--------------------------------------------------------------------------*/
