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
 std::string name = "Test string";
 auto c = new FRowConstraint();

 block->add_static_constraint( *c, name );

 ASSERT_EQ( name.compare( block->get_s_const_name()[ 0 ] ), 0 );
 ASSERT_EQ( block->get_static_constraint< FRowConstraint >( 0 ), c );
 ASSERT_EQ( block->get_static_constraint< FRowConstraint >( 0 )->get_Block(), block );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_StaticConstraints_Vector ) {
 std::string name = "Test string";
 auto c = new std::vector< FRowConstraint >( 5 );

 block->add_static_constraint( *c, name );

 ASSERT_EQ( name.compare( block->get_s_const_name()[ 0 ] ), 0 );
 ASSERT_EQ( block->get_static_constraint_v< FRowConstraint >( 0 ), c );
 for( const auto & i : *c ) {
  ASSERT_EQ( i.get_Block(), block );
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_StaticConstraints_Vector_p ) {
 std::string name = "Test string";
 auto c = new std::vector< FRowConstraint * >( 5 );
 for( auto & i : *c ) {
  i = new FRowConstraint();
 }

 block->add_static_constraint( *c, name );

 ASSERT_EQ( name.compare( block->get_s_const_name()[ 0 ] ), 0 );
 ASSERT_EQ( block->get_static_constraint_v< FRowConstraint * >( 0 ), c );
 for( const auto & i : *c ) {
  ASSERT_EQ( i->get_Block(), block );
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_StaticConstraints_Multiarray ) {
 std::string name = "Test string";
 auto c = new boost::multi_array< FRowConstraint, 2 >;
 c->resize( boost::extents[ 2 ][ 2 ] );

 block->add_static_constraint( *c, name );

 ASSERT_EQ( name.compare( block->get_s_const_name()[ 0 ] ), 0 );
 ASSERT_EQ( ( block->get_static_constraint< FRowConstraint, 2 >( 0 ) ), c );
 for( auto i = c->data(); i < ( c->data() + c->num_elements() ); ++i ) {
  ASSERT_EQ( i->get_Block(), block );
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_StaticConstraints_Multiarray_p ) {
 std::string name = "Test string";
 auto c = new boost::multi_array< FRowConstraint *, 2 >;
 c->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = c->data(); i < ( c->data() + c->num_elements() ); ++i ) {
  *i = new FRowConstraint();
 }

 block->add_static_constraint( *c, name );

 ASSERT_EQ( name.compare( block->get_s_const_name()[ 0 ] ), 0 );
 ASSERT_EQ( ( block->get_static_constraint< FRowConstraint *, 2 >( 0 ) ), c );
 for( auto i = c->data(); i < ( c->data() + c->num_elements() ); ++i ) {
  ASSERT_EQ( ( *i )->get_Block(), block );
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_StaticVariables_One ) {
 std::string name = "Test string";
 auto v = new ColVariable();

 block->add_static_variable( *v, name );

 ASSERT_EQ( name.compare( block->get_s_var_name()[ 0 ] ), 0 );
 ASSERT_EQ( block->get_static_variable< ColVariable >( 0 ), v );
 ASSERT_EQ( block->get_static_variable< ColVariable >( 0 )->get_Block(), block );
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_StaticVariables_Vector ) {
 std::string name = "Test string";
 auto v = new std::vector< ColVariable >( 5 );

 block->add_static_variable( *v, name );

 ASSERT_EQ( name.compare( block->get_s_var_name()[ 0 ] ), 0 );
 ASSERT_EQ( block->get_static_variable_v< ColVariable >( 0 ), v );
 for( const auto & i : *v ) {
  ASSERT_EQ( i.get_Block(), block );
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_StaticVariables_Vector_p ) {
 std::string name = "Test string";
 auto v = new std::vector< ColVariable * >( 5 );
 for( auto & i : *v ) {
  i = new ColVariable();
 }

 block->add_static_variable( *v, name );

 ASSERT_EQ( name.compare( block->get_s_var_name()[ 0 ] ), 0 );
 ASSERT_EQ( block->get_static_variable_v< ColVariable * >( 0 ), v );
 for( const auto & i : *v ) {
  ASSERT_EQ( i->get_Block(), block );
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_StaticVariables_Multiarray ) {
 std::string name = "Test string";
 auto v = new boost::multi_array< ColVariable, 2 >;
 v->resize( boost::extents[ 2 ][ 2 ] );

 block->add_static_variable( *v, name );

 ASSERT_EQ( name.compare( block->get_s_var_name()[ 0 ] ), 0 );
 ASSERT_EQ( ( block->get_static_variable< ColVariable, 2 >( 0 ) ), v );
 for( auto i = v->data(); i < ( v->data() + v->num_elements() ); ++i ) {
  ASSERT_EQ( i->get_Block(), block );
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_StaticVariables_Multiarray_p ) {
 std::string name = "Test string";
 auto v = new boost::multi_array< ColVariable *, 2 >;
 v->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = v->data(); i < ( v->data() + v->num_elements() ); ++i ) {
  *i = new ColVariable();
 }

 block->add_static_variable( *v, name );

 ASSERT_EQ( name.compare( block->get_s_var_name()[ 0 ] ), 0 );
 ASSERT_EQ( ( block->get_static_variable< ColVariable *, 2 >( 0 ) ), v );
 for( auto i = v->data(); i < ( v->data() + v->num_elements() ); ++i ) {
  ASSERT_EQ( ( *i )->get_Block(), block );
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicConstraints_List ) {
 std::string name = "Test string";
 auto c = new std::list< FRowConstraint >( 5 );

 block->add_dynamic_constraint( *c, name );

 ASSERT_EQ( name.compare( block->get_d_const_name()[ 0 ] ), 0 );
 ASSERT_EQ( block->get_dynamic_constraint< FRowConstraint >( 0 ), c );

 for( const auto & i : *c ) {
  ASSERT_EQ( i.get_Block(), block );
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicConstraints_List_p ) {
 std::string name = "Test string";
 auto c = new std::list< FRowConstraint * >( 5 );
 for( auto & i : *c ) {
  i = new FRowConstraint();
 }

 block->add_dynamic_constraint( *c, name );

 ASSERT_EQ( name.compare( block->get_d_const_name()[ 0 ] ), 0 );
 ASSERT_EQ( block->get_dynamic_constraint< FRowConstraint * >( 0 ), c );
 for( const auto & i : *c ) {
  ASSERT_EQ( i->get_Block(), block );
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicConstraints_Vector ) {
 std::string name = "Test string";
 auto c = new std::vector< std::list< FRowConstraint>>( 5 );
 for( auto & i : *c ) {
  i.resize( 3 );
 }

 block->add_dynamic_constraint( *c, name );

 ASSERT_EQ( name.compare( block->get_d_const_name()[ 0 ] ), 0 );
 ASSERT_EQ( block->get_dynamic_constraint_v< FRowConstraint >( 0 ), c );
 for( auto & i : *c ) {
  for( auto & j: i ) {
   ASSERT_EQ( j.get_Block(), block );
  }
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicConstraints_Vector_p ) {
 std::string name = "Test string";
 auto c = new std::vector< std::list< FRowConstraint *>>( 5 );
 for( auto & i : *c ) {
  i.resize( 3 );
  for( auto & j: i ) {
   j = new FRowConstraint();
  }
 }

 block->add_dynamic_constraint( *c, name );

 ASSERT_EQ( name.compare( block->get_d_const_name()[ 0 ] ), 0 );
 ASSERT_EQ( block->get_dynamic_constraint_v< FRowConstraint * >( 0 ), c );
 for( auto & i : *c ) {
  for( auto & j: i ) {
   ASSERT_EQ( j->get_Block(), block );
  }
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicConstraints_Multiarray ) {
 std::string name = "Test string";
 auto c = new boost::multi_array< std::list< FRowConstraint >, 2 >;
 c->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = c->data(); i < ( c->data() + c->num_elements() ); ++i ) {
  i->resize( 3 );
 }

 block->add_dynamic_constraint( *c, name );

 ASSERT_EQ( name.compare( block->get_d_const_name()[ 0 ] ), 0 );
 ASSERT_EQ( ( block->get_dynamic_constraint< FRowConstraint, 2 >( 0 ) ), c );
 for( auto i = c->data(); i < ( c->data() + c->num_elements() ); ++i ) {
  for( auto & j: *i ) {
   ASSERT_EQ( j.get_Block(), block );
  }
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicConstraints_Multiarray_p ) {
 std::string name = "Test string";
 auto c = new boost::multi_array< std::list< FRowConstraint * >, 2 >;
 c->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = c->data(); i < ( c->data() + c->num_elements() ); ++i ) {
  i->resize( 3 );
  for( auto & j: *i ) {
   j = new FRowConstraint();
  }
 }

 block->add_dynamic_constraint( *c, name );

 ASSERT_EQ( name.compare( block->get_d_const_name()[ 0 ] ), 0 );
 ASSERT_EQ( ( block->get_dynamic_constraint< FRowConstraint *, 2 >( 0 ) ), c );
 for( auto i = c->data(); i < ( c->data() + c->num_elements() ); ++i ) {
  for( auto & j: *i ) {
   ASSERT_EQ( j->get_Block(), block );
  }
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicVariables_List ) {
 std::string name = "Test string";
 auto v = new std::list< ColVariable >( 5 );

 block->add_dynamic_variable( *v, name );

 ASSERT_EQ( name.compare( block->get_d_var_name()[ 0 ] ), 0 );
 ASSERT_EQ( block->get_dynamic_variable< ColVariable >( 0 ), v );
 for( const auto & i : *v ) {
  ASSERT_EQ( i.get_Block(), block );
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicVariables_List_p ) {
 std::string name = "Test string";
 auto v = new std::list< ColVariable * >( 5 );
 for( auto & i : *v ) {
  i = new ColVariable();
 }

 block->add_dynamic_variable( *v, name );

 ASSERT_EQ( name.compare( block->get_d_var_name()[ 0 ] ), 0 );
 ASSERT_EQ( block->get_dynamic_variable< ColVariable * >( 0 ), v );
 for( const auto & i : *v ) {
  ASSERT_EQ( i->get_Block(), block );
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicVariables_Vector ) {
 std::string name = "Test string";
 auto v = new std::vector< std::list< ColVariable>>( 5 );
 for( auto & i : *v ) {
  i.resize( 3 );
 }

 block->add_dynamic_variable( *v, name );

 ASSERT_EQ( name.compare( block->get_d_var_name()[ 0 ] ), 0 );
 ASSERT_EQ( block->get_dynamic_variable_v< ColVariable >( 0 ), v );
 for( auto & i : *v ) {
  for( auto & j: i ) {
   ASSERT_EQ( j.get_Block(), block );
  }
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicVariables_Vector_p ) {
 std::string name = "Test string";
 auto v = new std::vector< std::list< ColVariable *>>( 5 );
 for( auto & i : *v ) {
  i.resize( 3 );
  for( auto & j: i ) {
   j = new ColVariable();
  }
 }

 block->add_dynamic_variable( *v, name );

 ASSERT_EQ( name.compare( block->get_d_var_name()[ 0 ] ), 0 );
 ASSERT_EQ( block->get_dynamic_variable_v< ColVariable * >( 0 ), v );
 for( auto & i : *v ) {
  for( auto & j: i ) {
   ASSERT_EQ( j->get_Block(), block );
  }
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicVariables_Multiarray ) {
 std::string name = "Test string";
 auto v = new boost::multi_array< std::list< ColVariable >, 2 >;
 v->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = v->data(); i < ( v->data() + v->num_elements() ); ++i ) {
  i->resize( 3 );
 }

 block->add_dynamic_variable( *v, name );

 ASSERT_EQ( name.compare( block->get_d_var_name()[ 0 ] ), 0 );
 ASSERT_EQ( ( block->get_dynamic_variable< ColVariable, 2 >( 0 ) ), v );
 for( auto i = v->data(); i < ( v->data() + v->num_elements() ); ++i ) {
  for( auto & j: *i ) {
   ASSERT_EQ( j.get_Block(), block );
  }
 }
}

/*--------------------------------------------------------------------------*/

TEST_F( AbstractBlockTest, Adds_DynamicVariables_Multiarray_p ) {
 std::string name = "Test string";
 auto v = new boost::multi_array< std::list< ColVariable * >, 2 >;
 v->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = v->data(); i < ( v->data() + v->num_elements() ); ++i ) {
  i->resize( 3 );
  for( auto & j: *i ) {
   j = new ColVariable();
  }
 }

 block->add_dynamic_variable( *v, name );

 ASSERT_EQ( name.compare( block->get_d_var_name()[ 0 ] ), 0 );
 ASSERT_EQ( (block->get_dynamic_variable< ColVariable *, 2 >( 0 )), v );
 for( auto i = v->data(); i < ( v->data() + v->num_elements() ); ++i ) {
  for( auto & j: *i ) {
   ASSERT_EQ( j->get_Block(), block );
  }
 }
}

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/
int main( int argc, char ** argv ) {
 ::testing::InitGoogleTest( &argc, argv );
 return RUN_ALL_TESTS();
}
