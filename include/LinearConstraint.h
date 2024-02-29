/*--------------------------------------------------------------------------*/
/*----------------------- File LinearConstraint.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the LinearConstraint class, an header-only convenience
 * class which derives from FRowConstraint and implements the concept that
 * the Function of the FRowConstraint is a LinearFunction, "redirecting" the
 * relevant part of the LinearFunction interface.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __LinearConstraint
#define __LinearConstraint
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

//#include "Block.h"
#include "FRowConstraint.h"
#include "LinearFunction.h"
//#include "Variable.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS LinearConstraint --------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a Constraint that is a "single row" and defined by a LinearFunction
/** The class LinearConstraint, derived from FRowConstraint, implements the
 * concept of "Linear Constraint", that is, forces the Function within the
 * FRowConstraint to be a LinearFunction and "redirects" the relevant part of
 * the LinearFunction interface. */

class LinearConstraint : public FRowConstraint {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------- PUBLIC TYPES OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types: "import" those of LinearFunction
    @{ */

 using Coefficient = LinearFunction::Coefficient;

 using v_coeff = LinearFunction::v_coeff;

 using v_coeff_it = LinearFunction::v_coeff_it;

 using c_v_coeff_it = LinearFunction::c_v_coeff_it;

 using coeff_pair = LinearFunction::coeff_pair;

 using v_coeff_pair = LinearFunction::v_coeff_pair;

 using v_c_coeff_pair = LinearFunction::v_c_coeff_pair;

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
    @{ */

 /// constructor of LinearConstraint
 /** Constructor of LinearConstraint. It takes all the arguments of the
  * constructor of FRowConstraint plus those for the LinearFunction that is
  * constructed internally (save the Observer, that is the LinearConstraint).
  */

 LinearConstraint( Block * block = nullptr ,
		   RHSValue lhs = 0 , RHSValue rhs = 0 ,
		   v_coeff_pair && vars = {} ,
		   LinearFunction::FunctionValue ct = 0 )
  : FRowConstraint( block , lhs , rhs ,
		    new LinearFunction( std::move( vars ) , ct ) ) {}

/*--------------------------------------------------------------------------*/
 /// destructor: deletes the Function and un-registers with the Variable
 /** Apparently does nothing, but does everything by automatically calling
  * the FRowConstraint destructor. */

 virtual ~LinearConstraint() {}

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// (do not) set the pointer to the Function in this LinearConstraint
 /**< Method to set the pointer to the Function that defines this
  * LinearConstraint: this is not possible, so exception is thrown. */

 void set_function( Function * const function = nullptr ,
		    ModParam issueMod = eModBlck , bool deleteold = true ) {
  throw( std::logic_error( "call to LinearConstraint::set_function" ) );
  }

/** @} ---------------------------------------------------------------------*/
/*---------- METHODS FOR READING THE DATA OF THE LinearConstraint ----------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the LinearConstraint
 *  @{ */

 /// returns the Coefficient of the i-th Variable in this LinearConstraint

 Coefficient get_coefficient( Index i ) const {
  return( static_cast< LinearFunction * >( f_function )->get_coefficient( i )
	  );
  }

/*--------------------------------------------------------------------------*/
 ///< method to get a pointer to the LinearFunction of the LinearConstraint

 LinearFunction * get_linear_function( void ) const {
  return( static_cast< LinearFunction * >( f_function ) );
  }

/** @} ---------------------------------------------------------------------*/
/*------------- METHODS FOR MODIFYING THE LinearConstraint -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the LinearConstraint
 *  @{ */

 /// add a set of new Variable to the LinearConstraint

 void add_variables( v_coeff_pair && vars , ModParam issueMod = eModBlck ) {
  static_cast< LinearFunction * >( f_function )->add_variables(
					      std::move( vars ) , issueMod );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// add one single new Variable to the LinearFunction

 void add_variable( ColVariable * var , Coefficient coeff ,
                    ModParam issueMod = eModBlck ) {
  static_cast< LinearFunction * >( f_function )->add_variable( var , coeff ,
							       issueMod );
  }

/*--------------------------------------------------------------------------*/
 /// modify a single existing coefficient

 void modify_coefficient( Index i , Coefficient coeff ,
                          ModParam issueMod = eModBlck ) {
  static_cast< LinearFunction * >( f_function )->modify_coefficient( i ,
							   coeff , issueMod );
  }

/*--------------------------------------------------------------------------*/
 /// modify a set of existing coefficients

 void modify_coefficients( LinearFunction::Vec_FunctionValue && NCoef ,
                           Subset && nms ,
			   bool ordered = false ,
			   ModParam issueMod = eModBlck ) {
  static_cast< LinearFunction * >( f_function )->modify_coefficients(
		std::move( NCoef ) , std::move( nms ) , ordered , issueMod );
  }
  

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify a range of coefficients

 void modify_coefficients( LinearFunction::Vec_FunctionValue && NCoef ,
			   Range range = std::make_pair( 0 , Inf< Index >() ) ,
                           ModParam issueMod = eModBlck )  {
  static_cast< LinearFunction * >( f_function )->modify_coefficients(
		                      std::move( NCoef ) , range , issueMod );
  }

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
 *  @{ */

 /// print information about the LinearConstraint on an ostream

 void print( std::ostream &output ) const override {
  output << "LinearConstraint [" << this << "] of Block [" << f_Block
	 << "] with LinearFunction [" << f_function << "] with "
	 << ( f_function ? f_function->get_num_active_var() : 0 )
	 << " active variables, ";
  if( feasible() )
   output << "feasible";
  else
   output << "unfeasible";

  output << " (value = " << value() << ")" << std::endl;
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

 };  // end( class( LinearConstraint ) )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* LinearConstraint.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File LinearConstraint.h ------------------------*/
/*--------------------------------------------------------------------------*/
