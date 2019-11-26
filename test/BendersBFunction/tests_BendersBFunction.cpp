/*--------------------------------------------------------------------------*/
/*-------------------- File tests_BendersBFunction.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Main for testing BendersBFunction
 *
 * \version 0.10
 *
 * \date 26 - 11 - 2019
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
#include "BundleSolver.h"
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

double solve_with_bundle( std::string file_name , bool continuous_relaxation ) {
 auto inner_block_solver = new CPXMILPSolver();
 auto block = build_CWL_block_with_Benders_decomposition
   ( file_name , continuous_relaxation , inner_block_solver );

 {
  std::ifstream BundleParFile( "BundlePar.txt" );
  if( ! BundleParFile.is_open() ) {
   cerr << "Error: cannot open file BundlePar.txt" << endl;
   return( 1 );
   }

  BlockSolverConfig * bsc = new BlockSolverConfig;
  BundleParFile >> *( bsc );
  BundleParFile.close();

  block->set_SolverConfig( bsc );
  delete bsc;
 }

 auto solver = block->get_registered_solvers().front();
 auto status = solver->compute( true );
 if( status != ThinComputeInterface::kOK )
  std::cout << "Problem not solved for instance " << file_name << std::endl;
 auto solution_value = solver->get_var_value();
 std::cout << solution_value << std::endl;
 delete block;
 return solution_value;
}

double solve( std::string file_name , bool continuous_relaxation ) {
 auto block = build_CWL_block( file_name , continuous_relaxation );
 auto solver = new CPXMILPSolver();
 block->register_Solver( solver );
 auto status = solver->compute( true );
 if( status != ThinComputeInterface::kOK )
  std::cout << "Problem not solved for instance " << file_name << std::endl;
 auto solution_value = solver->get_var_value();
 delete block;
 return solution_value;
}

void compare( std::string data_dir_path ) {

 bool continuous_relaxation = true;

 for( const auto & file : std::filesystem::directory_iterator( data_dir_path ) ) {
  auto file_name = file.path();
  auto solution_value = solve( file_name , continuous_relaxation );
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
