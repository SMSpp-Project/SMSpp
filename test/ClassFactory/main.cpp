/*--------------------------------------------------------------------------*/
/*---------------------- File tests_class_factory.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the tests for the class factories.
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Rafael Durbano Lobato
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractBlock.h"

#include "BendersBFunction.h"

#include "BendersBlock.h"

#include "FakeSolver.h"

#include "LagBFunction.h"

#include "PolyhedralFunctionBlock.h"

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

void test_Block( const std::string & classname ) {
 try {
  auto c = Block::new_Block( classname );
  std::cout << "success";
  delete c;
  }
 catch( std::invalid_argument ) {
  std::cout << "failure";
  }
 catch( ... ) {
  std::cout << std::endl << "unexpected exception" << std::endl;
  exit( 1 );
  }
 std::cout << " for \"" << classname << "\"" << std::endl;
 }

/*--------------------------------------------------------------------------*/

template< class B >
void test_Block( const std::string & classname ) {
 try {
  auto c = Block::new_Block( classname );
  if( dynamic_cast< B * >( c ) )
   std::cout << "success";
  else
   std::cout << "wrong type in factory";
  delete c;
  }
 catch( std::invalid_argument ) {
  std::cout << "failure";
  }
 catch( ... ) {
  std::cout << std::endl << "unexpected exception" << std::endl;
  exit( 1 );
  }
 std::cout << " for \"" << classname << "\"" << std::endl;
 }

/*--------------------------------------------------------------------------*/

void test_Block( void ) {

 // SMS++ Block

 test_Block( " AbstractBlock " );
 test_Block< BendersBFunction >( " BendersBFunction " );
 test_Block< BendersBlock >( " BendersBlock " );
 test_Block( " LagBFunction " );
 test_Block< PolyhedralFunctionBlock > ( " PolyhedralFunctionBlock " );

 // DummyBlock

 test_Block( " DummyBlock " );
 test_Block< DummyBlock2 >( " DummyBlock2 " );

 test_Block( " DummyBlockT<> " );
 test_Block( " DummyBlockT< double > " );
 test_Block( " DummyBlockT< char > " );
 test_Block( " DummyBlockT< int > " );
 test_Block( " DummyBlockT< std::pair< double , int > > " );
 test_Block( " DummyBlockT< std::pair< double , int > > " );
 test_Block< DummyBlockT< std::list< std::pair< int , double > > > > (
		          " DummyBlockT< std::list< std::pair< int , double > > > " );

 test_Block( " DummyBlockT< void , 1 > " );
 test_Block( " DummyBlockT< double , 1 > " );
 test_Block( " DummyBlockT< char , 1 > " );
 test_Block( " DummyBlockT< int , 1 > " );
 test_Block( " DummyBlockT< std::pair< double , int > , 1 > " );
 test_Block< DummyBlockT< std::list< std::pair< int , double > > , 1 > >(
			 " DummyBlockT< std::list< std::pair< int , double > > , 1 > " );
 }

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

void test_Configuration( const std::string & classname ) {
 try {
  auto c = Configuration::new_Configuration( classname );
  std::cout << "success";
  delete c;
  }
 catch( std::invalid_argument ) {
  std::cout << "failure";
  }
 catch( ... ) {
  std::cout << std::endl << "unexpected exception" << std::endl;
  exit( 1 );
  }
 std::cout << " for \"" << classname << "\"" << std::endl;
 }

/*--------------------------------------------------------------------------*/

void test_Configuration( void ) {

 // ComputeConfig

 test_Configuration( " ComputeConfig " );

 // SimpleConfiguration

 test_Configuration( " SimpleConfiguration< int > " );
 test_Configuration( " SimpleConfiguration< double > " );
 test_Configuration( " SimpleConfiguration< std::pair< int , int > > " );
 test_Configuration( " SimpleConfiguration< std::pair< double , double > > " );
 test_Configuration( " SimpleConfiguration< std::pair< int , double > > " );
 test_Configuration( " SimpleConfiguration< std::pair< double , int > > " );
 test_Configuration( " SimpleConfiguration< std::vector< int > > " );
 test_Configuration( " SimpleConfiguration< std::vector< double > > " );
 test_Configuration( " SimpleConfiguration< std::pair< Configuration * , Configuration * > > " );
 test_Configuration( " SimpleConfiguration< std::vector< Configuration * > > " );

 // DummyConfiguration

 test_Configuration( " DummyConfiguration<> " );
 test_Configuration( " DummyConfiguration< int , double > " );
 test_Configuration( " DummyConfiguration< double , std::pair< int , double > > " );
 test_Configuration( " DummyConfiguration< int , std::list< std::pair< int , double > > > " );
 test_Configuration( " DummyConfiguration< int , DummyConfiguration< int , std::list< std::pair< int , double > > > > " );
}

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

void test_Solver( const std::string & classname ) {
 try {
  auto c = Solver::new_Solver( classname );
  std::cout << "success";
  delete c;
  }
 catch( std::invalid_argument ) {
  std::cout << "failure";
  }
 catch( ... ) {
  std::cout << std::endl << "unexpected exception" << std::endl;
  exit( 1 );
  }
 std::cout << " for \"" << classname << "\"" << std::endl;
 }

/*--------------------------------------------------------------------------*/

void test_Solver( void ) {
 test_Solver( " FakeSolver " );
 test_Solver( " UpdateSolver " );

 // DummySolver

 test_Solver( " DummySolver<> " );
 test_Solver( " DummySolver< int , double > " );
 test_Solver( " DummySolver< double , std::pair< int , double > > " );
 test_Solver( " DummySolver< int , std::list< std::pair< int , double > > > " );
 test_Solver( " DummySolver< int , DummySolver< int , std::list< std::pair< int , double > > > > " );
}

/*--------------------------------------------------------------------------*/

int main() {
 test_Block();
 test_Configuration();
 test_Solver();
 return( 0 );
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_class_factory.cpp --------------------*/
/*--------------------------------------------------------------------------*/
