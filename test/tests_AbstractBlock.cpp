/** @file
 * Unit tests for Block and AbstractBlock.
 *
 * \author Niccolo' Iardella \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Niccolo' Iardella
 */

#include <gtest/gtest.h>

#include "AbstractBlock.h"
#include "FRowConstraint.h"
#include "ColVariable.h"

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------ TEST FIXTURE ------------------------------*/
/*--------------------------------------------------------------------------*/
class AbstractBlockTest : public ::testing::Test {

 protected:

 AbstractBlock * block{};

 AbstractBlockTest() = default;

 ~AbstractBlockTest() override = default;

 void SetUp() override {
  block = new AbstractBlock();
 }

 void TearDown() override {
  block->reset_static_constraints();
  ASSERT_TRUE( block->get_static_constraints().empty() );
  ASSERT_TRUE( block->get_s_const_name().empty() );

  block->reset_static_variables();
  ASSERT_TRUE( block->get_static_variables().empty() );
  ASSERT_TRUE( block->get_s_var_name().empty() );

  block->reset_dynamic_constraints();
  ASSERT_TRUE( block->get_dynamic_constraints().empty() );
  ASSERT_TRUE( block->get_d_const_name().empty() );

  block->reset_dynamic_variables();
  ASSERT_TRUE( block->get_dynamic_variables().empty() );
  ASSERT_TRUE( block->get_d_var_name().empty() );

  EXPECT_NO_THROW( delete block );
 }

};

/*--------------------------------------------------------------------------*/
/*------------------------------- TEST CASES -------------------------------*/
/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Sets_UpperBound ) {
 ASSERT_EQ( block->get_valid_upper_bound(), Inf< double >() );

 double ub = 1.0;
 block->set_valid_upper_bound( ub, true );
 ASSERT_EQ( block->get_valid_upper_bound( true ), ub );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Sets_LowerBound ) {
 ASSERT_EQ( block->get_valid_lower_bound(), -Inf< double >() );

 double lb = -1.0;
 block->set_valid_lower_bound( lb, true );
 ASSERT_EQ( block->get_valid_lower_bound( true ), lb );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_StaticConstraints_One ) {
 auto c = new FRowConstraint();

 block->add_static_constraint( *c );

 ASSERT_EQ( block->get_static_constraint< FRowConstraint >( 0 ), c );
 ASSERT_EQ( block->get_static_constraint< FRowConstraint >( 0 )->get_Block(), block );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_StaticConstraints_Vector ) {
 auto c = new std::vector< FRowConstraint >( 5 );

 block->add_static_constraint( *c );

 ASSERT_EQ( block->get_static_constraint_v< FRowConstraint >( 0 ), c );
 for( const auto & i : *c ) {
  ASSERT_EQ( i.get_Block(), block );
 }

 Constraint::clear( *c );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_StaticConstraints_Multiarray ) {
 auto c = new boost::multi_array< FRowConstraint, 2 >;
 c->resize( boost::extents[ 2 ][ 2 ] );

 block->add_static_constraint( *c );

 ASSERT_EQ( ( block->get_static_constraint< FRowConstraint, 2 >( 0 ) ), c );
 for( auto i = c->data(); i < ( c->data() + c->num_elements() ); ++i ) {
  ASSERT_EQ( i->get_Block(), block );
 }

 Constraint::clear( *c );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_StaticVariables_One ) {
 auto v = new ColVariable();

 block->add_static_variable( *v );

 ASSERT_EQ( block->get_static_variable< ColVariable >( 0 ), v );
 ASSERT_EQ( block->get_static_variable< ColVariable >( 0 )->get_Block(), block );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_StaticVariables_Vector ) {
 auto v = new std::vector< ColVariable >( 5 );

 block->add_static_variable( *v );

 ASSERT_EQ( block->get_static_variable_v< ColVariable >( 0 ), v );
 for( const auto & i : *v ) {
  ASSERT_EQ( i.get_Block(), block );
 }

 ASSERT_TRUE( ColVariable::is_feasible( *v ) );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_StaticVariables_Multiarray ) {
 auto v = new boost::multi_array< ColVariable, 2 >;
 v->resize( boost::extents[ 2 ][ 2 ] );

 block->add_static_variable( *v );

 ASSERT_EQ( ( block->get_static_variable< ColVariable, 2 >( 0 ) ), v );
 for( auto i = v->data(); i < ( v->data() + v->num_elements() ); ++i ) {
  ASSERT_EQ( i->get_Block(), block );
 }

 ASSERT_TRUE( ColVariable::is_feasible( *v ) );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicConstraints_List ) {
 auto c = new std::list< FRowConstraint >( 5 );

 block->add_dynamic_constraint( *c );

 ASSERT_EQ( block->get_dynamic_constraint< FRowConstraint >( 0 ), c );

 for( const auto & i : *c ) {
  ASSERT_EQ( i.get_Block(), block );
 }

 Constraint::clear( *c );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicConstraints_Vector ) {
 auto c = new std::vector< std::list< FRowConstraint > >( 5 );
 for( auto & i : *c ) {
  i.resize( 3 );
 }

 block->add_dynamic_constraint( *c );

 ASSERT_EQ( block->get_dynamic_constraint_v< FRowConstraint >( 0 ), c );
 for( auto & i : *c ) {
  for( auto & j: i ) {
   ASSERT_EQ( j.get_Block(), block );
  }
 }

 Constraint::clear( *c );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicConstraints_Multiarray ) {
 auto c = new boost::multi_array< std::list< FRowConstraint >, 2 >;
 c->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = c->data(); i < ( c->data() + c->num_elements() ); ++i ) {
  i->resize( 3 );
 }

 block->add_dynamic_constraint( *c );

 ASSERT_EQ( ( block->get_dynamic_constraint< FRowConstraint, 2 >( 0 ) ), c );
 for( auto i = c->data(); i < ( c->data() + c->num_elements() ); ++i ) {
  for( auto & j: *i ) {
   ASSERT_EQ( j.get_Block(), block );
  }
 }

 Constraint::clear( *c );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicVariables_List ) {
 auto v = new std::list< ColVariable >( 5 );

 block->add_dynamic_variable( *v );

 ASSERT_EQ( block->get_dynamic_variable< ColVariable >( 0 ), v );
 for( const auto & i : *v ) {
  ASSERT_EQ( i.get_Block(), block );
 }

 ASSERT_TRUE( ColVariable::is_feasible( *v ) );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicVariables_Vector ) {
 auto v = new std::vector< std::list< ColVariable > >( 5 );
 for( auto & i : *v ) {
  i.resize( 3 );
 }

 block->add_dynamic_variable( *v );

 ASSERT_EQ( block->get_dynamic_variable_v< ColVariable >( 0 ), v );
 for( auto & i : *v ) {
  for( auto & j: i ) {
   ASSERT_EQ( j.get_Block(), block );
  }
 }

 ASSERT_TRUE( ColVariable::is_feasible( *v ) );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicVariables_Multiarray ) {
 auto v = new boost::multi_array< std::list< ColVariable >, 2 >;
 v->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = v->data(); i < ( v->data() + v->num_elements() ); ++i ) {
  i->resize( 3 );
 }

 block->add_dynamic_variable( *v );

 ASSERT_EQ( ( block->get_dynamic_variable< ColVariable, 2 >( 0 ) ), v );
 for( auto i = v->data(); i < ( v->data() + v->num_elements() ); ++i ) {
  for( auto & j: *i ) {
   ASSERT_EQ( j.get_Block(), block );
  }
 }

 ASSERT_TRUE( ColVariable::is_feasible( *v ) );
}

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/
int main( int argc, char ** argv ) {
 ::testing::InitGoogleTest( &argc, argv );
 return( RUN_ALL_TESTS() );
}
