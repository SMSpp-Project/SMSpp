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
 // the two halves live in two groups of their own, so that each of them is
 // written exactly as it is when it travels alone
 auto vg = group.getGroup( "VariableSolution" );
 if( ! vg.isNull() )
  f_variable_solution.deserialize( vg );

 auto cg = group.getGroup( "ConstraintSolution" );
 if( ! cg.isNull() )
  f_constraint_solution.deserialize( cg );
}

/*--------------------------------------------------------------------------*/

void ColRowSolution::read( const Block * const block ) {
 f_direction = block->is_direction();
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

 auto vg = group.addGroup( "VariableSolution" );
 f_variable_solution.serialize( vg );

 auto cg = group.addGroup( "ConstraintSolution" );
 f_constraint_solution.serialize( cg );
 }

/*--------------------------------------------------------------------------*/

void ColRowSolution::sum( const Solution * solution, double multiplier ) {

 auto other_solution = dynamic_cast< const ColRowSolution * >( solution );

 if( ! other_solution )
  throw( std::invalid_argument( "ColRowSolution::sum: given Solution "
                                "must be a ColRowSolution" ) );

 // the sum is a direction only if every Solution in it is one
 f_direction = f_direction && other_solution->f_direction;

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
 f_direction = solution->f_direction;  // scaling a direction gives one
 f_variable_solution.scale( & solution->get_variable_solution() , factor );
 f_constraint_solution.scale( & solution->get_constraint_solution() , factor );
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File ColRowSolution.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
