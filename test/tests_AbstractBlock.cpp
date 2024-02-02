/** @file
 * Unit tests for Block and AbstractBlock.
 *
 * \author Niccolo' Iardella \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Niccolo' Iardella, Donato Meoli
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractBlock.h"
#include "FRowConstraint.h"
#include "ColVariable.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

void TearDown( AbstractBlock * block )
{
 block->reset_static_constraints();
 assert( block->get_static_constraints().empty() );
 assert( block->get_s_const_name().empty() );

 block->reset_static_variables();
 assert( block->get_static_variables().empty() );
 assert( block->get_s_var_name().empty() );

 block->reset_dynamic_constraints();
 assert( block->get_dynamic_constraints().empty() );
 assert( block->get_d_const_name().empty() );

 block->reset_dynamic_variables();
 assert( block->get_dynamic_variables().empty() );
 assert( block->get_d_var_name().empty() );

 delete block;
}

/*--------------------------------------------------------------------------*/

void runAllTests()
{
 // test Sets_UpperBound
 double ub = 1.0;
 AbstractBlock * block = new AbstractBlock(); // SetUp
 assert( block->get_valid_upper_bound() == Inf< double >() );
 block->set_valid_upper_bound( ub , true );
 assert( block->get_valid_upper_bound( true ) == ub );
 TearDown( block ); // TearDown

 // test Sets_LowerBound
 double lb = -1.0;
 block = new AbstractBlock(); // SetUp
 assert( block->get_valid_lower_bound() == -Inf< double >() );
 block->set_valid_lower_bound( lb , true );
 assert( block->get_valid_lower_bound( true ) == lb );
 TearDown( block ); // TearDown

 // test Adds_StaticConstraints_One
 block = new AbstractBlock(); // SetUp
 auto c = new FRowConstraint();
 block->add_static_constraint( *c );
 assert( block->get_static_constraint< FRowConstraint >( 0 ) == c );
 assert(
  block->get_static_constraint< FRowConstraint >( 0 )->get_Block() == block );
 TearDown( block ); // TearDown

 // test Adds_StaticConstraints_Vector
 block = new AbstractBlock(); // SetUp
 c = new FRowConstraint();
 block->add_static_constraint( *c );
 assert( block->get_static_constraint< FRowConstraint >( 0 ) == c );
 assert(
  block->get_static_constraint< FRowConstraint >( 0 )->get_Block() == block );
 TearDown( block ); // TearDown

 // test Adds_StaticConstraints_MultiArray
 block = new AbstractBlock(); // SetUp
 auto v_c = new std::vector< FRowConstraint >( 5 );
 block->add_static_constraint( *v_c );
 assert( block->get_static_constraint_v< FRowConstraint >( 0 ) == v_c );
 for( const auto & i : *v_c )
  assert( i.get_Block() == block );
 Constraint::clear( *v_c );
 TearDown( block ); // TearDown

 // test Adds_StaticConstraints_MultiArray
 block = new AbstractBlock(); // SetUp
 auto m_c = new boost::multi_array< FRowConstraint , 2 >;
 m_c->resize( boost::extents[ 2 ][ 2 ] );
 block->add_static_constraint( *m_c );
 assert( ( block->get_static_constraint< FRowConstraint , 2 >( 0 ) ) == m_c );
 for( auto i = m_c->data() ; i < ( m_c->data() + m_c->num_elements() ) ; ++i )
  assert( i->get_Block() == block );
 Constraint::clear( *m_c );
 TearDown( block ); // TearDown

 // test Adds_StaticVariables_One
 block = new AbstractBlock(); // SetUp
 auto v = new ColVariable();
 block->add_static_variable( *v );
 assert( block->get_static_variable< ColVariable >( 0 ) == v );
 assert( block->get_static_variable< ColVariable >( 0 )->get_Block() == block );
 TearDown( block ); // TearDown

 // test Adds_StaticVariables_Vector
 block = new AbstractBlock(); // SetUp
 auto v_v = new std::vector< ColVariable >( 5 );
 block->add_static_variable( *v_v );
 assert( block->get_static_variable_v< ColVariable >( 0 ) == v_v );
 for( const auto & i : *v_v )
  assert( i.get_Block() == block );
 assert( ColVariable::is_feasible( *v_v ) );
 TearDown( block ); // TearDown

 // test Adds_StaticVariables_MultiArray
 block = new AbstractBlock(); // SetUp
 auto m_v = new boost::multi_array< ColVariable , 2 >;
 m_v->resize( boost::extents[ 2 ][ 2 ] );
 block->add_static_variable( *m_v );
 assert( ( block->get_static_variable< ColVariable , 2 >( 0 ) ) == m_v );
 for( auto i = m_v->data() ; i < ( m_v->data() + m_v->num_elements() ) ; ++i )
  assert( i->get_Block() == block );
 assert( ColVariable::is_feasible( *m_v ) );
 TearDown( block ); // TearDown

 // test Adds_DynamicConstraints_List
 block = new AbstractBlock(); // SetUp
 auto l_c = new std::list< FRowConstraint >( 5 );
 block->add_dynamic_constraint( *l_c );
 assert( block->get_dynamic_constraint< FRowConstraint >( 0 ) == l_c );
 for( const auto & i : *l_c )
  assert( i.get_Block() == block );
 Constraint::clear( *l_c );
 TearDown( block ); // TearDown

 // test Adds_DynamicConstraints_Vector
 block = new AbstractBlock(); // SetUp
 auto v_l_c = new std::vector< std::list< FRowConstraint > >( 5 );
 for( auto & i : *v_l_c )
  i.resize( 3 );
 block->add_dynamic_constraint( *v_l_c );
 assert( block->get_dynamic_constraint_v< FRowConstraint >( 0 ) == v_l_c );
 for( auto & i : *v_l_c )
  for( auto & j : i )
   assert( j.get_Block() == block );
 Constraint::clear( *v_l_c );
 TearDown( block ); // TearDown

 // test Adds_DynamicConstraints_MultiArray
 block = new AbstractBlock(); // SetUp
 auto m_l_c = new boost::multi_array< std::list< FRowConstraint > , 2 >;
 m_l_c->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = m_l_c->data() ;
      i < ( m_l_c->data() + m_l_c->num_elements() ) ; ++i )
  i->resize( 3 );
 block->add_dynamic_constraint( *m_l_c );
 assert(
  ( block->get_dynamic_constraint< FRowConstraint , 2 >( 0 ) ) == m_l_c );
 for( auto i = m_l_c->data() ;
      i < ( m_l_c->data() + m_l_c->num_elements() ) ; ++i )
  for( auto & j : *i )
   assert( j.get_Block() == block );
 Constraint::clear( *m_l_c );
 TearDown( block ); // TearDown

 // test Adds_DynamicVariables_List
 block = new AbstractBlock(); // SetUp
 auto l_v = new std::list< ColVariable >( 5 );
 block->add_dynamic_variable( *l_v );
 assert( block->get_dynamic_variable< ColVariable >( 0 ) == l_v );
 for( const auto & i : *l_v )
  assert( i.get_Block() == block );
 assert( ColVariable::is_feasible( *l_v ) );
 TearDown( block ); // TearDown

 // test Adds_DynamicVariables_Vector
 block = new AbstractBlock(); // SetUp
 auto v_l_v = new std::vector< std::list< ColVariable > >( 5 );
 for( auto & i : *v_l_v )
  i.resize( 3 );
 block->add_dynamic_variable( *v_l_v );
 assert( block->get_dynamic_variable_v< ColVariable >( 0 ) == v_l_v );
 for( auto & i : *v_l_v )
  for( auto & j : i )
   assert( j.get_Block() == block );
 assert( ColVariable::is_feasible( *v_l_v ) );
 TearDown( block ); // TearDown

 // test Adds_DynamicVariables_MultiArray
 block = new AbstractBlock(); // SetUp
 auto m_l_v = new boost::multi_array< std::list< ColVariable > , 2 >;
 m_l_v->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = m_l_v->data() ;
      i < ( m_l_v->data() + m_l_v->num_elements() ) ; ++i )
  i->resize( 3 );
 block->add_dynamic_variable( *m_l_v );
 assert( ( block->get_dynamic_variable< ColVariable , 2 >( 0 ) ) == m_l_v );
 for( auto i = m_l_v->data() ;
      i < ( m_l_v->data() + m_l_v->num_elements() ) ; ++i )
  for( auto & j : *i )
   assert( j.get_Block() == block );
 assert( ColVariable::is_feasible( *m_l_v ) );
 TearDown( block ); // TearDown
}

/*--------------------------------------------------------------------------*/

int main() {
 runAllTests();
 return( 0 );
}

/*--------------------------------------------------------------------------*/
/*--------------------- End File tests_AbstractBlock.cpp --------------------*/
/*--------------------------------------------------------------------------*/
