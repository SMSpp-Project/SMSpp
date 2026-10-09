/*--------------------------------------------------------------------------*/
/*---------------------- File tests_class_factory.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the tests for the class factories: each factory of the
 * core (Block, Configuration, Solver, Solution, State and Change) is asked
 * for every class the core registers and for classes registered here, with
 * a classname written in different ways, and the object it gives must be of
 * the class asked; a name nobody registered, or no name at all, must be
 * refused with std::invalid_argument.
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Rafael Durbano Lobato, Donato Meoli
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractBlock.h"

#include "BendersBFunction.h"

#include "BendersBlock.h"

#include "BlockSolverConfig.h"

#include "BoxSolver.h"

#include "Change.h"

#include "ColRowSolution.h"

#include "FakeSolver.h"

#include "LagBFunction.h"

#include "MasterProblemBlock.h"

#include "PolyhedralFunction.h"

#include "PolyhedralFunctionBlock.h"

#include "RBlockConfig.h"

#include "UpdateSolver.h"

#include <iostream>

#include "../TestAssert.h"  // last: see the comments in it

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*-------------------------------- Block -----------------------------------*/
/*--------------------------------------------------------------------------*/

#define create_Block_class( ClassName ) \
class ClassName : public Block { \
public: \
 ClassName( Block * ) {} \
protected: \
 void load( std::istream &input , char frmt ) override {}; \
private: \
 SMSpp_insert_in_factory_h; \
}

create_Block_class( DummyBlock );
create_Block_class( DummyBlock2 );

SMSpp_insert_in_factory_cpp_1( DummyBlock );
SMSpp_insert_in_factory_cpp_1( DummyBlock2 );

/*--------------------------------------------------------------------------*/

template< class T = void , int i = 0 >
class DummyBlockT : public Block {
 public:
  DummyBlockT( Block * ) {}
 protected:
  void load( std::istream &input , char frmt ) override {};
 private:
  SMSpp_insert_in_factory_h;
 };

SMSpp_insert_in_factory_cpp_1_t( DummyBlockT<> );
SMSpp_insert_in_factory_cpp_1_t( DummyBlockT< double > );
SMSpp_insert_in_factory_cpp_1_t( DummyBlockT< char > );
SMSpp_insert_in_factory_cpp_1_t( DummyBlockT< int > );
SMSpp_insert_in_factory_cpp_1_t( DummyBlockT< std::pair< double , int > > );
SMSpp_insert_in_factory_cpp_1_t
( DummyBlockT< std::list< std::pair < int , double > > > );
SMSpp_insert_in_factory_cpp_1_t( DummyBlockT< void , 1 > );
SMSpp_insert_in_factory_cpp_1_t( DummyBlockT< double , 1 > );
SMSpp_insert_in_factory_cpp_1_t( DummyBlockT< char , 1 > );
SMSpp_insert_in_factory_cpp_1_t( DummyBlockT< int , 1 > );
SMSpp_insert_in_factory_cpp_1_t( DummyBlockT< std::pair< double , int > , 1 > );
SMSpp_insert_in_factory_cpp_1_t
( DummyBlockT< std::list< std::pair < int , double > > , 1 > );

/*--------------------------------------------------------------------------*/
/*---------------------------- Configuration -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T = void , class U = void >
class DummyConfiguration : public Configuration {
 protected:
  Configuration * clone( void ) const override { return( nullptr ); }
  void load( std::istream &input ) override {};
 private:
  SMSpp_insert_in_factory_h;
 };

SMSpp_insert_in_factory_cpp_0_t( DummyConfiguration<> );
SMSpp_insert_in_factory_cpp_0_t( DummyConfiguration< int , double > );
SMSpp_insert_in_factory_cpp_0_t
( DummyConfiguration< double , std::pair< int , double > > );
SMSpp_insert_in_factory_cpp_0_t
( DummyConfiguration< int , std::list< std::pair< int , double > > > );
SMSpp_insert_in_factory_cpp_0_t
( DummyConfiguration< int , DummyConfiguration< int , std::list<
    std::pair< int , double > > > > );

/*--------------------------------------------------------------------------*/
/*-------------------------------- Solver ----------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T = void , class U = void >
class DummySolver : public Solver {
 public:
  int compute( bool ) override { return( 0 ); };
  void get_var_solution( Configuration * ) override {};
 private:
  SMSpp_insert_in_factory_h;
 };

SMSpp_insert_in_factory_cpp_0_t( DummySolver<> );
SMSpp_insert_in_factory_cpp_0_t( DummySolver< int , double > );
SMSpp_insert_in_factory_cpp_0_t
( DummySolver< double , std::pair< int , double > > );
SMSpp_insert_in_factory_cpp_0_t
( DummySolver< int , std::list< std::pair< int , double > > > );
SMSpp_insert_in_factory_cpp_0_t
( DummySolver< int , DummySolver< int ,
    std::list< std::pair< int , double > > > > );

/*--------------------------------------------------------------------------*/
/*----------------------------- THE CHECKS ---------------------------------*/
/*--------------------------------------------------------------------------*/

static int n_checks = 0;  ///< how many classnames have been checked

/*--------------------------------------------------------------------------*/
/// the factory \p make gives an object of class T for \p classname

template< class T , class Base , class Make >
static void check( Make make , const std::string & classname )
{
 Base * obj = make( classname );
 assert( obj );
 assert( dynamic_cast< T * >( obj ) );
 delete obj;
 ++n_checks;
 }

/*--------------------------------------------------------------------------*/
/// the factory \p make refuses \p classname with std::invalid_argument

template< class Make >
static void check_refused( Make make , const std::string & classname )
{
 bool refused = false;
 try {
  delete make( classname );
  }
 catch( std::invalid_argument & ) {
  refused = true;
  }
 assert( refused );
 ++n_checks;
 }

/*--------------------------------------------------------------------------*/

static void test_Block( void )
{
 auto make = []( const std::string & n ) { return( Block::new_Block( n ) ); };

 // the Block of the core
 check< AbstractBlock , Block >( make , "AbstractBlock" );
 check< BendersBFunction , Block >( make , "BendersBFunction" );
 check< BendersBlock , Block >( make , "BendersBlock" );
 check< LagBFunction , Block >( make , "LagBFunction" );
 check< MasterProblemBlock , Block >( make , "MasterProblemBlock" );
 check< PolyhedralFunctionBlock , Block >( make , "PolyhedralFunctionBlock" );

 // the classname is found whatever the blanks around it
 check< AbstractBlock , Block >( make , " AbstractBlock " );

 // the Block registered here, templates included, the arguments written
 // with and without the blanks between them
 check< DummyBlock , Block >( make , "DummyBlock" );
 check< DummyBlock2 , Block >( make , " DummyBlock2 " );
 check< DummyBlockT<> , Block >( make , "DummyBlockT<>" );
 check< DummyBlockT< double > , Block >( make , "DummyBlockT< double >" );
 check< DummyBlockT< double > , Block >( make , "DummyBlockT<double>" );
 check< DummyBlockT< char > , Block >( make , "DummyBlockT< char >" );
 check< DummyBlockT< int > , Block >( make , "DummyBlockT< int >" );
 check< DummyBlockT< std::pair< double , int > > , Block >( make ,
			      "DummyBlockT< std::pair< double , int > >" );
 check< DummyBlockT< std::pair< double , int > > , Block >( make ,
				       "DummyBlockT<std::pair<double,int>>" );
 check< DummyBlockT< std::list< std::pair< int , double > > > , Block >(
       make , "DummyBlockT< std::list< std::pair< int , double > > >" );
 check< DummyBlockT< void , 1 > , Block >( make , "DummyBlockT< void , 1 >" );
 check< DummyBlockT< double , 1 > , Block >( make ,
					      "DummyBlockT< double , 1 >" );
 check< DummyBlockT< char , 1 > , Block >( make , "DummyBlockT< char , 1 >" );
 check< DummyBlockT< int , 1 > , Block >( make , "DummyBlockT< int , 1 >" );
 check< DummyBlockT< std::pair< double , int > , 1 > , Block >( make ,
			  "DummyBlockT< std::pair< double , int > , 1 >" );
 check< DummyBlockT< std::list< std::pair< int , double > > , 1 > , Block >(
   make , "DummyBlockT< std::list< std::pair< int , double > > , 1 >" );

 // a name nobody registered, and no name
 check_refused( make , "NoSuchBlock" );
 check_refused( make , "DummyBlockT< float >" );
 check_refused( make , "" );
 }

/*--------------------------------------------------------------------------*/

static void test_Configuration( void )
{
 auto make = []( const std::string & n ) {
  return( Configuration::new_Configuration( n ) ); };

 // the Configuration of the core
 check< ComputeConfig , Configuration >( make , "ComputeConfig" );
 check< BlockConfig , Configuration >( make , "BlockConfig" );
 check< OBlockConfig , Configuration >( make , "OBlockConfig" );
 check< CBlockConfig , Configuration >( make , "CBlockConfig" );
 check< RBlockConfig , Configuration >( make , "RBlockConfig" );
 check< OCBlockConfig , Configuration >( make , "OCBlockConfig" );
 check< ORBlockConfig , Configuration >( make , "ORBlockConfig" );
 check< CRBlockConfig , Configuration >( make , "CRBlockConfig" );
 check< OCRBlockConfig , Configuration >( make , "OCRBlockConfig" );
 check< BlockSolverConfig , Configuration >( make , "BlockSolverConfig" );
 check< RBlockSolverConfig , Configuration >( make , "RBlockSolverConfig" );

 // the SimpleConfiguration the core registers, with and without blanks
 check< SimpleConfiguration< int > , Configuration >( make ,
					       "SimpleConfiguration< int >" );
 check< SimpleConfiguration< int > , Configuration >( make ,
						 "SimpleConfiguration<int>" );
 check< SimpleConfiguration< double > , Configuration >( make ,
					    "SimpleConfiguration< double >" );
 check< SimpleConfiguration< std::pair< int , int > > , Configuration >(
		   make , "SimpleConfiguration< std::pair< int , int > >" );
 check< SimpleConfiguration< std::pair< double , double > > , Configuration >(
	     make , "SimpleConfiguration< std::pair< double , double > >" );
 check< SimpleConfiguration< std::pair< int , double > > , Configuration >(
		make , "SimpleConfiguration< std::pair< int , double > >" );
 check< SimpleConfiguration< std::pair< double , int > > , Configuration >(
		make , "SimpleConfiguration< std::pair< double , int > >" );
 check< SimpleConfiguration< std::vector< int > > , Configuration >(
			make , "SimpleConfiguration< std::vector< int > >" );
 check< SimpleConfiguration< std::vector< int > > , Configuration >(
			     make , "SimpleConfiguration<std::vector<int>>" );
 check< SimpleConfiguration< std::vector< double > > , Configuration >(
		     make , "SimpleConfiguration< std::vector< double > >" );
 check< SimpleConfiguration< std::pair< Configuration * ,
					Configuration * > > , Configuration >(
   make , "SimpleConfiguration< std::pair< Configuration * , "
	  "Configuration * > >" );
 check< SimpleConfiguration< std::vector< Configuration * > > ,
	Configuration >( make ,
		   "SimpleConfiguration< std::vector< Configuration * > >" );
 check< SimpleConfiguration< std::vector< std::pair< int , int > > > ,
	Configuration >( make ,
	      "SimpleConfiguration< std::vector< std::pair< int , int > > >" );
 check< SimpleConfiguration< std::vector< std::pair< int ,
					    Configuration * > > > ,
	Configuration >( make , "SimpleConfiguration< std::vector< "
			 "std::pair< int , Configuration * > > >" );
 check< SimpleConfiguration< std::map< std::string , Configuration * > > ,
	Configuration >( make , "SimpleConfiguration< std::map< "
			 "std::string , Configuration * > >" );

 // the Configuration registered here
 check< DummyConfiguration<> , Configuration >( make ,
						 "DummyConfiguration<>" );
 check< DummyConfiguration< int , double > , Configuration >( make ,
				       "DummyConfiguration< int , double >" );
 check< DummyConfiguration< double , std::pair< int , double > > ,
	Configuration >( make ,
		  "DummyConfiguration< double , std::pair< int , double > >" );
 check< DummyConfiguration< int , std::list< std::pair< int , double > > > ,
	Configuration >( make ,
       "DummyConfiguration< int , std::list< std::pair< int , double > > >" );
 check< DummyConfiguration< int , DummyConfiguration< int ,
	std::list< std::pair< int , double > > > > , Configuration >( make ,
   "DummyConfiguration< int , DummyConfiguration< int , std::list< "
   "std::pair< int , double > > > >" );

 // a SimpleConfiguration of a type nobody registered, and no name
 check_refused( make , "SimpleConfiguration< float >" );
 check_refused( make , "NoSuchConfiguration" );
 check_refused( make , "" );
 }

/*--------------------------------------------------------------------------*/

static void test_Solver( void )
{
 auto make = []( const std::string & n ) { return( Solver::new_Solver( n ) ); };

 // the Solver of the core
 check< FakeSolver , Solver >( make , "FakeSolver" );
 check< UpdateSolver , Solver >( make , "UpdateSolver" );
 check< BoxSolver , Solver >( make , "BoxSolver" );

 // the Solver registered here
 check< DummySolver<> , Solver >( make , "DummySolver<>" );
 check< DummySolver< int , double > , Solver >( make ,
					      "DummySolver< int , double >" );
 check< DummySolver< double , std::pair< int , double > > , Solver >( make ,
			 "DummySolver< double , std::pair< int , double > >" );
 check< DummySolver< int , std::list< std::pair< int , double > > > ,
	Solver >( make ,
	      "DummySolver< int , std::list< std::pair< int , double > > >" );
 check< DummySolver< int , DummySolver< int ,
	std::list< std::pair< int , double > > > > , Solver >( make ,
   "DummySolver< int , DummySolver< int , std::list< std::pair< int , "
   "double > > > >" );

 check_refused( make , "NoSuchSolver" );
 check_refused( make , "" );
 }

/*--------------------------------------------------------------------------*/

static void test_Solution( void )
{
 auto make = []( const std::string & n ) {
  return( Solution::new_Solution( n ) ); };

 check< ColVariableSolution , Solution >( make , "ColVariableSolution" );
 check< RowConstraintSolution , Solution >( make , "RowConstraintSolution" );
 check< ColRowSolution , Solution >( make , "ColRowSolution" );

 check_refused( make , "NoSuchSolution" );
 check_refused( make , "" );
 }

/*--------------------------------------------------------------------------*/

static void test_State( void )
{
 auto make = []( const std::string & n ) { return( State::new_State( n ) ); };

 check< LagBFunctionState , State >( make , "LagBFunctionState" );
 check< PolyhedralFunctionState , State >( make , "PolyhedralFunctionState" );
 check< BendersBFunctionState , State >( make , "BendersBFunctionState" );

 check_refused( make , "NoSuchState" );
 check_refused( make , "" );
 }

/*--------------------------------------------------------------------------*/

static void test_Change( void )
{
 auto make = []( const std::string & n ) { return( Change::new_Change( n ) ); };

 check< GroupChange , Change >( make , "GroupChange" );

 check_refused( make , "NoSuchChange" );
 check_refused( make , "" );
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 test_Block();
 test_Configuration();
 test_Solver();
 test_Solution();
 test_State();
 test_Change();

 std::cout << "ClassFactory_test: all " << n_checks << " checks passed"
	   << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_class_factory.cpp --------------------*/
/*--------------------------------------------------------------------------*/
