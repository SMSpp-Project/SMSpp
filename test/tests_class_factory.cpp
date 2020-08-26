/*--------------------------------------------------------------------------*/
/*---------------------- File tests_class_factory.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the tests for the class factories.
 *
 * \version 0.10
 *
 * \date 25 - 08 - 2020
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
#include "BendersBFunction.h"
#include "BendersBlock.h"
#include "FakeSolver.h"
#include "LagBFunction.h"
#include "PolyhedralFunctionBlock.h"
#include "UpdateSolver.h"

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
 void load( std::istream &input ) override {}; \
private: \
 SMSpp_insert_in_factory_h; \
}

create_Block_class( DummyBlock );
create_Block_class( DummyBlock2 );

SMSpp_insert_in_factory_cpp_0( ( ( ( ( DummyBlock ) ) ) ) );
SMSpp_insert_in_factory_cpp_1( ( ( ( ( ( DummyBlock2 ) ) ) ) ) );

/*--------------------------------------------------------------------------*/

template< class T = void , int i = 0 >
class DummyBlockT : public Block {
public:
 DummyBlockT( Block * ) {}
protected:
 void load( std::istream &input ) override {};
private:
 SMSpp_insert_in_factory_h;
};

SMSpp_insert_in_factory_cpp_0_t( ( DummyBlockT<> ) );
SMSpp_insert_in_factory_cpp_0_t( ( ( DummyBlockT< double > ) ) );
SMSpp_insert_in_factory_cpp_0_t( ( ( ( DummyBlockT< char > ) ) ) );
SMSpp_insert_in_factory_cpp_0_t( ( ( ( ( DummyBlockT< int > ) ) ) ) );
SMSpp_insert_in_factory_cpp_0_t
( ( ( ( ( DummyBlockT< std::pair< double , int > > ) ) ) ) );
SMSpp_insert_in_factory_cpp_0_t
( ( ( ( DummyBlockT< std::list< std::pair < int , double > > > ) ) ) );
SMSpp_insert_in_factory_cpp_1_t( ( DummyBlockT< void , 1 > ) );
SMSpp_insert_in_factory_cpp_1_t( ( ( DummyBlockT< double , 1 > ) ) );
SMSpp_insert_in_factory_cpp_1_t( ( ( ( DummyBlockT< char , 1 > ) ) ) );
SMSpp_insert_in_factory_cpp_1_t( ( ( ( ( DummyBlockT< int , 1 > ) ) ) ) );
SMSpp_insert_in_factory_cpp_1_t
( ( ( ( ( DummyBlockT< std::pair< double , int > , 1 > ) ) ) ) );
SMSpp_insert_in_factory_cpp_1_t
( ( ( ( DummyBlockT< std::list< std::pair < int , double > > , 1 > ) ) ) );


/*--------------------------------------------------------------------------*/

template<class T>
void test_new_Block( const std::string & classname ) {
 auto c = Block::new_Block( classname );
 assert( dynamic_cast<T *>( c ) );
}

/*--------------------------------------------------------------------------*/

void test_new_Block() {

 // SMS++ Block

 test_new_Block< AbstractBlock >( " AbstractBlock " );
 test_new_Block< BendersBFunction >( " BendersBFunction " );
 test_new_Block< BendersBlock >( " BendersBlock " );
 test_new_Block< LagBFunction >( " LagBFunction " );
 test_new_Block< PolyhedralFunctionBlock >( " PolyhedralFunctionBlock " );

 // DummyBlock

 test_new_Block< DummyBlock >( " DummyBlock " );
 test_new_Block< DummyBlock2 >( " DummyBlock2 " );

 test_new_Block< DummyBlockT<> >( " DummyBlockT<> " );
 test_new_Block< DummyBlockT< double > >( " DummyBlockT<double> " );
 test_new_Block< DummyBlockT< char > >( " DummyBlockT< char > " );
 test_new_Block< DummyBlockT< int > >( " DummyBlockT< int > " );
 test_new_Block< DummyBlockT< std::pair< double , int > > >
  ( " DummyBlockT< std::pair< double , int > > " );
 test_new_Block< DummyBlockT< std::list< std::pair < int , double > > > >
  ( " DummyBlockT< std::list< std::pair < int , double > > > " );

 test_new_Block< DummyBlockT< void , 1 > >( " DummyBlockT< void , 1 > " );
 test_new_Block< DummyBlockT< double , 1 > >( " DummyBlockT<double,1> " );
 test_new_Block< DummyBlockT< char , 1 > >( " DummyBlockT< char , 1 > " );
 test_new_Block< DummyBlockT< int , 1 > >( " DummyBlockT< int,1 > " );
 test_new_Block< DummyBlockT< std::pair< double , int > , 1 > >
  ( " DummyBlockT< std::pair< double , int > , 1 > " );
 test_new_Block< DummyBlockT< std::list< std::pair < int , double > > , 1 > >
  ( " DummyBlockT< std::list< std::pair < int , double > > , 1 > " );
}

/*--------------------------------------------------------------------------*/
/*---------------------------- Configuration -------------------------------*/
/*--------------------------------------------------------------------------*/

template<class T = void , class U = void>
class DummyConfiguration : public Configuration {
protected:
 Configuration * clone( void ) const override { return nullptr; }
 void load( std::istream &input ) override {};
private:
 SMSpp_insert_in_factory_h;
};

SMSpp_insert_in_factory_cpp_0_t( DummyConfiguration<> );
SMSpp_insert_in_factory_cpp_0_t( ( DummyConfiguration< int , double > ) );
SMSpp_insert_in_factory_cpp_0_t
( ( DummyConfiguration< double , std::pair< int , double > > ) );
SMSpp_insert_in_factory_cpp_0_t
( ( DummyConfiguration< int , std::list< std::pair< int , double > > > ) );
SMSpp_insert_in_factory_cpp_0_t
( ( DummyConfiguration< int , DummyConfiguration< int , std::list<
    std::pair< int , double > > > > ) );

/*--------------------------------------------------------------------------*/

template<class T>
void test_new_Configuration( const std::string & classname ) {
 auto c = Configuration::new_Configuration( classname );
 assert( dynamic_cast<T *>( c ) );
}

/*--------------------------------------------------------------------------*/

void test_new_Configuration() {

 // ComputeConfig

 test_new_Configuration< ComputeConfig >( " ComputeConfig " );

 // SimpleConfiguration

 test_new_Configuration< SimpleConfiguration< int > >
  ( " SimpleConfiguration < int > " );
 test_new_Configuration< SimpleConfiguration< int > >
  ( "SimpleConfiguration<int>" );
 test_new_Configuration< SimpleConfiguration< double > >
  ( " SimpleConfiguration < double > " );
 test_new_Configuration< SimpleConfiguration< std::pair< int , int > > >
  ( " SimpleConfiguration< std::pair< int , int > > " );
 test_new_Configuration< SimpleConfiguration< std::pair< double , double > > >
  ( " SimpleConfiguration< std::pair< double , double > > " );
 test_new_Configuration< SimpleConfiguration< std::pair< int , double > > >
  ( " SimpleConfiguration< std::pair< int , double > > " );
 test_new_Configuration< SimpleConfiguration< std::pair< double , int > > >
  ( " SimpleConfiguration< std::pair< double , int > > " );
 test_new_Configuration< SimpleConfiguration< std::vector< int > > >
  ( " SimpleConfiguration< std::vector< int > > " );
 test_new_Configuration< SimpleConfiguration< std::vector< double > > >
  ( " SimpleConfiguration< std::vector< double > > " );
 test_new_Configuration< SimpleConfiguration< std::list< int > > >
  ( " SimpleConfiguration< std::list< int > > " );
 test_new_Configuration< SimpleConfiguration< std::list< double > > >
  ( " SimpleConfiguration< std::list< double > > " );
 test_new_Configuration< SimpleConfiguration< std::pair< Configuration * ,
                                                         Configuration * > > >
  ( " SimpleConfiguration< std::pair< Configuration * , Configuration * > > " );
 test_new_Configuration< SimpleConfiguration< std::vector< Configuration * > > >
  ( " SimpleConfiguration< std::vector< Configuration * > > " );

 // DummyConfiguration

 test_new_Configuration< DummyConfiguration<> >( " DummyConfiguration<> " );
 test_new_Configuration< DummyConfiguration< int , double > >
  ( " DummyConfiguration< int , double > " );
 test_new_Configuration< DummyConfiguration< double ,
                                             std::pair< int , double > > >
  ( " DummyConfiguration< double , std::pair< int , double > > " );
 test_new_Configuration< DummyConfiguration< int , std::list<
  std::pair< int , double > > > >
  ( " DummyConfiguration< int , std::list< std::pair< int , double > > > " );
 test_new_Configuration< DummyConfiguration< int , DummyConfiguration<
  int , std::list< std::pair< int , double > > > > >
  ( " DummyConfiguration< int , DummyConfiguration< int , std::list< "
    " std::pair< int , double > > > > " );
}

/*--------------------------------------------------------------------------*/
/*-------------------------------- Solver ----------------------------------*/
/*--------------------------------------------------------------------------*/

template<class T = void , class U = void>
class DummySolver : public Solver {
public:
 int compute( bool ) override { return 0; };
 void get_var_solution( Configuration * ) override {};
private:
 SMSpp_insert_in_factory_h;
};

SMSpp_insert_in_factory_cpp_0_t( DummySolver<> );
SMSpp_insert_in_factory_cpp_0_t( ( DummySolver< int , double > ) );
SMSpp_insert_in_factory_cpp_0_t
( ( DummySolver< double , std::pair< int , double > > ) );
SMSpp_insert_in_factory_cpp_0_t
( ( DummySolver< int , std::list< std::pair< int , double > > > ) );
SMSpp_insert_in_factory_cpp_0_t
( ( DummySolver< int , DummySolver<int ,
    std::list<std::pair< int , double > > > > ) );

/*--------------------------------------------------------------------------*/

template<class T>
void test_new_Solver( const std::string & classname ) {
 auto c = Solver::new_Solver( classname );
 assert( dynamic_cast<T *>( c ) );
}

/*--------------------------------------------------------------------------*/

void test_new_Solver() {
 test_new_Solver< FakeSolver >( " FakeSolver " );
 test_new_Solver< UpdateSolver >( " UpdateSolver " );

 // DummySolver

 test_new_Solver< DummySolver<> >( " DummySolver<> " );
 test_new_Solver< DummySolver< int , double > >
  ( " DummySolver< int , double > " );
 test_new_Solver< DummySolver< double , std::pair< int , double > > >
  ( " DummySolver< double , std::pair< int , double > > " );
 test_new_Solver< DummySolver< int , std::list<
  std::pair< int , double > > > >
  ( " DummySolver< int , std::list< std::pair< int , double > > > " );
 test_new_Solver< DummySolver< int , DummySolver<
  int , std::list< std::pair< int , double > > > > >
  ( " DummySolver< int , DummySolver< int , std::list< "
    " std::pair< int , double > > > > " );
}

/*--------------------------------------------------------------------------*/

int main() {
 test_new_Block();
 test_new_Configuration();
 test_new_Solver();
 return 0;
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_class_factory.cpp --------------------*/
/*--------------------------------------------------------------------------*/
