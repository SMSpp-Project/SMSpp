/*--------------------------------------------------------------------------*/
/*------------------------ File ColRowSolution.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the ColRowSolution class.
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ColRowSolution.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register ColRowSolution to the Solution factory

SMSpp_insert_in_factory_cpp_0( ColRowSolution );

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void ColRowSolution::deserialize( const netCDF::NcGroup & group ) {
 throw( std::logic_error( "ColRowSolution::deserialize not ready yet" ) );
}

/*--------------------------------------------------------------------------*/

void ColRowSolution::read( const Block * const block ) {
 f_variable_solution.read( block );
 f_constraint_solution.read( block );
}

/*--------------------------------------------------------------------------*/

void ColRowSolution::write( Block * const block ) {
 f_variable_solution.write( block );
 f_constraint_solution.write( block );
}

/*--------------------------------------------------------------------------*/

void ColRowSolution::serialize( netCDF::NcGroup & group ) const {
 // always call the method of the base class first
 Solution::serialize( group );

 throw( std::logic_error( " ColRowSolution::serialize not ready yet" ) );
 }

/*--------------------------------------------------------------------------*/

void ColRowSolution::sum( const Solution * solution, double multiplier ) {

 auto other_solution = dynamic_cast< const ColRowSolution * >( solution );

 if( ! other_solution )
  throw( std::invalid_argument( "ColRowSolution::sum: given Solution "
                                "must be a ColRowSolution" ) );

 f_variable_solution.sum( & other_solution->get_variable_solution() ,
                          multiplier );
 f_constraint_solution.sum( & other_solution->get_constraint_solution() ,
                            multiplier);
}

/*--------------------------------------------------------------------------*/

ColRowSolution * ColRowSolution::scale( double factor ) const {
 auto scaled_solution = new ColRowSolution();
 scaled_solution->scale( this , factor );
 return( scaled_solution );
}

/*--------------------------------------------------------------------------*/

ColRowSolution * ColRowSolution::clone( bool empty ) const {
 auto cloned_solution = new ColRowSolution();

 if( ! empty )
  cloned_solution->scale( this , 1.0 );

 return( cloned_solution );
}

/*--------------------------------------------------------------------------*/

void ColRowSolution::scale( const ColRowSolution * const solution ,
                            const double factor ) {
 f_variable_solution.scale( & solution->get_variable_solution() , factor );
 f_constraint_solution.scale( & solution->get_constraint_solution() , factor );
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File ColRowSolution.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
