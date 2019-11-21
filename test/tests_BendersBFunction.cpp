/*--------------------------------------------------------------------------*/
/*-------------------- File tests_BendersBFunction.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Main for testing BendersBFunction
 *
 * \version 0.10
 *
 * \date 21 - 11 - 2019
 *
 * \author Rafael Durbano Lobato \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Rafael Durbano Lobato
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractBlock.h"
#include "CPXMILPSolver.h"
#include "CWLAbstractBlockBuilder.h"

#include "cwl-mcf/cwl-mcf.h"

#include <iostream>
#include <filesystem>

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;
using namespace SMSpp_di_unipi_it::tests;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

double solve( std::string file_name ) {
 auto block = build_CWL_block( file_name , true );
 auto solver = new CPXMILPSolver();
 block->register_Solver( solver );
 auto status = solver->compute( true );
 if( status != ThinComputeInterface::kOK )
  std::cout << "Problem not solved for instance " << file_name << std::endl;
 auto objective = static_cast< FRealObjective * >( block->get_objective() );
 auto solution_value = objective->get_function()->get_value();
 return solution_value;
}

void compare( std::string data_dir_path ) {

 for( const auto & file : std::filesystem::directory_iterator( data_dir_path ) ) {
  auto file_name = file.path();
  auto solution_value = solve( file_name );
  auto cwl_mcf_value = cwl_mcf( file_name );
  auto diff = std::abs( solution_value - cwl_mcf_value );
  auto max_diff = std::max( 1.0e-6 , 1.0e-6 *
                            std::min( abs( solution_value ),
                                      abs( cwl_mcf_value ) ) );

  if( diff > max_diff )
   std::cout << "Solution value difference for instance " <<
    file_name << ": "  << diff << std::endl;
 }
}

/*--------------------------------------------------------------------------*/

int main( int argc, char ** argv ) {

 if( argc < 2 ) {
  std::cerr << "The path to the directory containing the instance files " <<
   "must be provided as argument." << std::endl;
  std::cerr << "Usage: " << argv[ 0 ] << " PATH" << std::endl;
  return 1;
 }

 std::string path = argv[ 1 ];

 compare( path );

 return 0;
}

/*--------------------------------------------------------------------------*/
/*------------------ End File tests_BendersBFunction.cpp -------------------*/
/*--------------------------------------------------------------------------*/
