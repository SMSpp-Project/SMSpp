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
  EXPECT_NO_THROW( delete block );
 }

};

/*--------------------------------------------------------------------------*/
/*------------------------------- TEST CASES -------------------------------*/
/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, SetsAndGetsBounds ) {
 ASSERT_EQ( block->get_valid_upper_bound(), Inf< double >() );
 ASSERT_EQ( block->get_valid_lower_bound(), -Inf< double >() );

 double ub = 1.0;
 double lb = -1.0;
 block->set_valid_upper_bound( ub, true );
 block->set_valid_lower_bound( lb, true );
 ASSERT_EQ( block->get_valid_upper_bound( true ), ub );
 ASSERT_EQ( block->get_valid_lower_bound( true ), lb );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, AddsStaticConstraints ) {
 std::string name;
 int n = 0;

 name = "Single constraint";
 auto c = new FRowConstraint();
 block->add_static_constraint( *c, name );
 ASSERT_EQ( name.compare( block->get_s_const_name()[ n ] ), 0 );
 ASSERT_EQ( block->get_static_constraint< FRowConstraint >( n ), c );
 ASSERT_EQ( block->get_static_constraint< FRowConstraint >( n )->get_Block(), block );
 n++;

 name = "A vector of constraints";
 auto v = new std::vector< FRowConstraint >( 5 );
 block->add_static_constraint( *v, name );
 ASSERT_EQ( name.compare( block->get_s_const_name()[ n ] ), 0 );
 ASSERT_EQ( block->get_static_constraint_v< FRowConstraint >( n ), v );

 for( const auto & i : *v ) {
  ASSERT_EQ( i.get_Block(), block );
 }
 n++;

 name = "A vector of pointers to constraints";
 auto v_p = new std::vector< FRowConstraint * >( 5 );
 for( auto & i : *v_p ) {
  i = new FRowConstraint();
 }
 block->add_static_constraint( *v_p, name );
 ASSERT_EQ( name.compare( block->get_s_const_name()[ n ] ), 0 );
 ASSERT_EQ( block->get_static_constraint_v< FRowConstraint * >( n ), v_p );

 for( const auto & i : *v_p ) {
  ASSERT_EQ( i->get_Block(), block );
 }
 n++;

 name = "A 2D boost::multi_array of constraints";
 auto ma = new boost::multi_array< FRowConstraint, 2 >;
 ma->resize( boost::extents[ 2 ][ 2 ] );
 block->add_static_constraint( *ma, name );
 ASSERT_EQ( name.compare( block->get_s_const_name()[ n ] ), 0 );
 auto new_ma = block->get_static_constraint< FRowConstraint, 2 >( n );
 ASSERT_EQ( new_ma, ma );

 for( auto i = ma->data(); i < ( ma->data() + ma->num_elements() ); ++i ) {
  ASSERT_EQ( i->get_Block(), block );
 }
 n++;

 name = "A 2D boost::multi_array of pointers to constraints";
 auto ma_p = new boost::multi_array< FRowConstraint *, 2 >;
 ma_p->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = ma_p->data(); i < ( ma_p->data() + ma_p->num_elements() ); ++i ) {
  *i = new FRowConstraint();
 }
 block->add_static_constraint( *ma_p, name );
 ASSERT_EQ( name.compare( block->get_s_const_name()[ n ] ), 0 );
 auto new_ma_p = block->get_static_constraint< FRowConstraint *, 2 >( n );
 ASSERT_EQ( new_ma_p, ma_p );

 for( auto i = ma_p->data(); i < ( ma_p->data() + ma_p->num_elements() ); ++i ) {
  ASSERT_EQ( ( *i )->get_Block(), block );
 }

 block->reset_static_constraints();
 ASSERT_TRUE( block->get_static_constraints().empty() );
 ASSERT_TRUE( block->get_s_const_name().empty() );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, AddsStaticVariables ) {
 std::string name;
 int n = 0;

 name = "Single variable";
 auto c = new ColVariable();
 block->add_static_variable( *c, name );
 ASSERT_EQ( name.compare( block->get_s_var_name()[ n ] ), 0 );
 ASSERT_EQ( block->get_static_variable< ColVariable >( n ), c );
 ASSERT_EQ( block->get_static_variable< ColVariable >( n )->get_Block(), block );
 n++;

 name = "A vector of variables";
 auto v = new std::vector< ColVariable >( 5 );
 block->add_static_variable( *v, name );
 ASSERT_EQ( name.compare( block->get_s_var_name()[ n ] ), 0 );
 ASSERT_EQ( block->get_static_variable_v< ColVariable >( n ), v );

 for( const auto & i : *v ) {
  ASSERT_EQ( i.get_Block(), block );
 }
 n++;

 name = "A vector of pointers to variables";
 auto v_p = new std::vector< ColVariable * >( 5 );
 for( auto & i : *v_p ) {
  i = new ColVariable();
 }
 block->add_static_variable( *v_p, name );
 ASSERT_EQ( name.compare( block->get_s_var_name()[ n ] ), 0 );
 ASSERT_EQ( block->get_static_variable_v< ColVariable * >( n ), v_p );

 for( const auto & i : *v_p ) {
  ASSERT_EQ( i->get_Block(), block );
 }
 n++;

 name = "A 2D boost::multi_array of variables";
 auto ma = new boost::multi_array< ColVariable, 2 >;
 ma->resize( boost::extents[ 2 ][ 2 ] );
 block->add_static_variable( *ma, name );
 ASSERT_EQ( name.compare( block->get_s_var_name()[ n ] ), 0 );
 auto new_ma = block->get_static_variable< ColVariable, 2 >( n );
 ASSERT_EQ( new_ma, ma );

 for( auto i = ma->data(); i < ( ma->data() + ma->num_elements() ); ++i ) {
  ASSERT_EQ( i->get_Block(), block );
 }
 n++;

 name = "A 2D boost::multi_array of pointers to variables";
 auto ma_p = new boost::multi_array< ColVariable *, 2 >;
 ma_p->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = ma_p->data(); i < ( ma_p->data() + ma_p->num_elements() ); ++i ) {
  *i = new ColVariable();
 }
 block->add_static_variable( *ma_p, name );
 ASSERT_EQ( name.compare( block->get_s_var_name()[ n ] ), 0 );
 auto new_ma_p = block->get_static_variable< ColVariable *, 2 >( n );
 ASSERT_EQ( new_ma_p, ma_p );

 for( auto i = ma_p->data(); i < ( ma_p->data() + ma_p->num_elements() ); ++i ) {
  ASSERT_EQ( ( *i )->get_Block(), block );
 }

 block->reset_static_variables();
 ASSERT_TRUE( block->get_static_variables().empty() );
 ASSERT_TRUE( block->get_s_var_name().empty() );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, AddsDynamicConstraints ) {
 std::string name;
 int n = 0;

 name = "A list of constraints";
 auto l = new std::list< FRowConstraint >( 5 );
 block->add_dynamic_constraint( *l, name );
 ASSERT_EQ( name.compare( block->get_d_const_name()[ n ] ), 0 );
 ASSERT_EQ( block->get_dynamic_constraint< FRowConstraint >( n ), l );

 for( const auto & i : *l ) {
  ASSERT_EQ( i.get_Block(), block );
 }
 n++;

 name = "A list of pointers to constraints";
 auto l_p = new std::list< FRowConstraint * >( 5 );
 for( auto & i : *l_p ) {
  i = new FRowConstraint();
 }
 block->add_dynamic_constraint( *l_p, name );
 ASSERT_EQ( name.compare( block->get_d_const_name()[ n ] ), 0 );
 ASSERT_EQ( block->get_dynamic_constraint< FRowConstraint * >( n ), l_p );

 for( const auto & i : *l_p ) {
  ASSERT_EQ( i->get_Block(), block );
 }
 n++;

 name = "A vector of lists of constraints";
 auto v_l = new std::vector< std::list< FRowConstraint>>( 5 );
 for( auto & i : *v_l ) {
  i.resize( 3 );
 }
 block->add_dynamic_constraint( *v_l, name );
 ASSERT_EQ( name.compare( block->get_d_const_name()[ n ] ), 0 );
 ASSERT_EQ( block->get_dynamic_constraint_v< FRowConstraint >( n ), v_l );

 for( auto & i : *v_l ) {
  for( auto & j: i ) {
   ASSERT_EQ( j.get_Block(), block );
  }
 }
 n++;

 name = "A vector of lists of pointers to constraints";
 auto v_l_p = new std::vector< std::list< FRowConstraint *>>( 5 );
 for( auto & i : *v_l_p ) {
  i.resize( 3 );
  for( auto & j: i ) {
   j = new FRowConstraint();
  }
 }
 block->add_dynamic_constraint( *v_l_p, name );
 ASSERT_EQ( name.compare( block->get_d_const_name()[ n ] ), 0 );
 ASSERT_EQ( block->get_dynamic_constraint_v< FRowConstraint * >( n ), v_l_p );

 for( auto & i : *v_l_p ) {
  for( auto & j: i ) {
   ASSERT_EQ( j->get_Block(), block );
  }
 }
 n++;

 name = "A 2D boost::multi_array of lists of constraints";
 auto ma_l = new boost::multi_array< std::list< FRowConstraint >, 2 >;
 ma_l->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = ma_l->data(); i < ( ma_l->data() + ma_l->num_elements() ); ++i ) {
  i->resize( 3 );
 }
 block->add_dynamic_constraint( *ma_l, name );
 ASSERT_EQ( name.compare( block->get_d_const_name()[ n ] ), 0 );
 auto new_ma_l = block->get_dynamic_constraint< FRowConstraint, 2 >( n );
 ASSERT_EQ( new_ma_l, ma_l );

 for( auto i = ma_l->data(); i < ( ma_l->data() + ma_l->num_elements() ); ++i ) {
  for( auto & j: *i ) {
   ASSERT_EQ( j.get_Block(), block );
  }
 }
 n++;

 name = "A 2D boost::multi_array of lists of pointers to constraints";
 auto ma_l_p = new boost::multi_array< std::list< FRowConstraint * >, 2 >;
 ma_l_p->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = ma_l_p->data(); i < ( ma_l_p->data() + ma_l_p->num_elements() ); ++i ) {
  i->resize( 3 );
  for( auto & j: *i ) {
   j = new FRowConstraint();
  }
 }
 block->add_dynamic_constraint( *ma_l_p, name );
 ASSERT_EQ( name.compare( block->get_d_const_name()[ n ] ), 0 );
 auto new_ma_l_p = block->get_dynamic_constraint< FRowConstraint *, 2 >( n );
 ASSERT_EQ( new_ma_l_p, ma_l_p );

 for( auto i = ma_l_p->data(); i < ( ma_l_p->data() + ma_l_p->num_elements() ); ++i ) {
  for( auto & j: *i ) {
   ASSERT_EQ( j->get_Block(), block );
  }
 }

 block->reset_dynamic_constraints();
 ASSERT_TRUE( block->get_dynamic_constraints().empty() );
 ASSERT_TRUE( block->get_d_const_name().empty() );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, AddsDynamicVariables ) {
 std::string name;
 int n = 0;

 name = "A list of variables";
 auto l = new std::list< ColVariable >( 5 );
 block->add_dynamic_variable( *l, name );
 ASSERT_EQ( name.compare( block->get_d_var_name()[ n ] ), 0 );
 ASSERT_EQ( block->get_dynamic_variable< ColVariable >( n ), l );

 for( const auto & i : *l ) {
  ASSERT_EQ( i.get_Block(), block );
 }
 n++;

 name = "A list of pointers to variables";
 auto l_p = new std::list< ColVariable * >( 5 );
 for( auto & i : *l_p ) {
  i = new ColVariable();
 }
 block->add_dynamic_variable( *l_p, name );
 ASSERT_EQ( name.compare( block->get_d_var_name()[ n ] ), 0 );
 ASSERT_EQ( block->get_dynamic_variable< ColVariable * >( n ), l_p );

 for( const auto & i : *l_p ) {
  ASSERT_EQ( i->get_Block(), block );
 }
 n++;

 name = "A vector of lists of variables";
 auto v_l = new std::vector< std::list< ColVariable>>( 5 );
 for( auto & i : *v_l ) {
  i.resize( 3 );
 }
 block->add_dynamic_variable( *v_l, name );
 ASSERT_EQ( name.compare( block->get_d_var_name()[ n ] ), 0 );
 ASSERT_EQ( block->get_dynamic_variable_v< ColVariable >( n ), v_l );

 for( auto & i : *v_l ) {
  for( auto & j: i ) {
   ASSERT_EQ( j.get_Block(), block );
  }
 }
 n++;

 name = "A vector of lists of pointers to variables";
 auto v_l_p = new std::vector< std::list< ColVariable *>>( 5 );
 for( auto & i : *v_l_p ) {
  i.resize( 3 );
  for( auto & j: i ) {
   j = new ColVariable();
  }
 }
 block->add_dynamic_variable( *v_l_p, name );
 ASSERT_EQ( name.compare( block->get_d_var_name()[ n ] ), 0 );
 ASSERT_EQ( block->get_dynamic_variable_v< ColVariable * >( n ), v_l_p );

 for( auto & i : *v_l_p ) {
  for( auto & j: i ) {
   ASSERT_EQ( j->get_Block(), block );
  }
 }
 n++;

 name = "A 2D boost::multi_array of lists of variables";
 auto ma_l = new boost::multi_array< std::list< ColVariable >, 2 >;
 ma_l->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = ma_l->data(); i < ( ma_l->data() + ma_l->num_elements() ); ++i ) {
  i->resize( 3 );
 }
 block->add_dynamic_variable( *ma_l, name );
 ASSERT_EQ( name.compare( block->get_d_var_name()[ n ] ), 0 );
 auto new_ma_l = block->get_dynamic_variable< ColVariable, 2 >( n );
 ASSERT_EQ( new_ma_l, ma_l );

 for( auto i = ma_l->data(); i < ( ma_l->data() + ma_l->num_elements() ); ++i ) {
  for( auto & j: *i ) {
   ASSERT_EQ( j.get_Block(), block );
  }
 }
 n++;

 name = "A 2D boost::multi_array of lists of pointers to variables";
 auto ma_l_p = new boost::multi_array< std::list< ColVariable * >, 2 >;
 ma_l_p->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = ma_l_p->data(); i < ( ma_l_p->data() + ma_l_p->num_elements() ); ++i ) {
  i->resize( 3 );
  for( auto & j: *i ) {
   j = new ColVariable();
  }
 }
 block->add_dynamic_variable( *ma_l_p, name );
 ASSERT_EQ( name.compare( block->get_d_var_name()[ n ] ), 0 );
 auto new_ma_l_p = block->get_dynamic_variable< ColVariable *, 2 >( n );
 ASSERT_EQ( new_ma_l_p, ma_l_p );

 for( auto i = ma_l_p->data(); i < ( ma_l_p->data() + ma_l_p->num_elements() ); ++i ) {
  for( auto & j: *i ) {
   ASSERT_EQ( j->get_Block(), block );
  }
 }

 block->reset_dynamic_variables();
 ASSERT_TRUE( block->get_dynamic_variables().empty() );
 ASSERT_TRUE( block->get_d_var_name().empty() );
}

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/
int main( int argc, char ** argv ) {
 ::testing::InitGoogleTest( &argc, argv );
 return RUN_ALL_TESTS();
}
