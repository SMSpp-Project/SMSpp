/*--------------------------------------------------------------------------*/
/*--------------------- File ColVariableSolution.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the ColVariableSolution class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "SMSTypedefs.h"
#include "ColVariable.h"
#include "ColVariableSolution.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register ColVariableSolution to the Solution factory

SMSpp_insert_in_factory_cpp_0( ColVariableSolution );

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void ColVariableSolution::deserialize( const netCDF::NcGroup & group )
{
 throw( std::logic_error( " ColVariableSolution::deserialize not ready yet" )
	);
 }

/*--------------------------------------------------------------------------*/

ColVariableSolution::~ColVariableSolution() {
 delete_vectors();
 }

/*--------------------------------------------------------------------------*/

void ColVariableSolution::delete_vectors() {

  for( Vec_any::size_type i = 0 ; i < static_variable_values.size() ; ++i )

    if( ! un_any_thing( double , static_variable_values[i] ,
                        [&var]() { delete &var; }() ) )

      throw( std::logic_error
       ( std::string( "ColVariableSolution::~ColVariableSolution() "
                      "invalid static variable group: " ) +
         static_variable_values[ i ].type().name() ) );

  for( Vec_any::size_type i = 0 ; i < dynamic_variable_values.size() ; ++i )

    if( ! un_any_thing( std::vector< double > ,
                        dynamic_variable_values[i] ,
                        [&var]() { delete &var; }() ) )

      throw( std::logic_error
       ( std::string( "ColVariableSolution::~ColVariableSolution() "
                      "invalid dynamic variable group: " ) +
         dynamic_variable_values[ i ].type().name() ) );

  static_variable_values.resize( 0 );
  dynamic_variable_values.resize( 0 );
  nested_solutions.resize( 0 );
}

/*--------------------------------------------------------------------------*/

void ColVariableSolution::initialize( const Block * const block , bool read ) {
  this->delete_vectors();
  this->initialize_static_variable_values( block , read );
  this->initialize_dynamic_variable_values( block , read );

  // Initialize the Solutions of the nested Blocks

  auto & nested_blocks = block->get_nested_Blocks();
  this->nested_solutions.resize( nested_blocks.size() );
  auto nested_solution_it = this->nested_solutions.begin();
  auto nested_block_it = nested_blocks.begin();
  for( ; nested_solution_it != this->nested_solutions.end() ;
       ++nested_solution_it , ++nested_block_it )
    (*nested_solution_it).initialize( *nested_block_it , read );
}

/*--------------------------------------------------------------------------*/

void ColVariableSolution::initialize_static_variable_values
( const Block * const block , bool read ) {

  // Initialize static variable values

  const auto & variable_groups = block->get_static_variables();
  static_variable_values.resize( variable_groups.size() );

  for( Vec_any::size_type i = 0; i < static_variable_values.size(); ++i ) {

    if( ! (
           un_any_static_2_create
           ( variable_groups[i] , static_variable_values[i] ,
             un_any_type< ColVariable >() , un_any_type< double >() ,
             []( ColVariable & variable , double & value ) {
             value = variable.get_value(); } , read )
           ||
           un_any_static_2_create
           ( variable_groups[i] , static_variable_values[i] ,
             un_any_type< ColVariable * >() , un_any_type< double >() ,
             []( ColVariable * variable , double & value ) {
             value = variable->get_value(); } , read ) ) )

      throw( std::logic_error( std::string( "ColVariableSolution::initialize: "
                                            "invalid variable group: " ) +
                               variable_groups[ i ].type().name() ) );
  }
}

/*--------------------------------------------------------------------------*/

void ColVariableSolution::initialize_dynamic_variable_values
( const Block * const block , bool read ) {

  // Initialize dynamic variable values

  const auto & variable_groups = block->get_dynamic_variables();
  dynamic_variable_values.resize( variable_groups.size() );

  for( Vec_any::size_type i = 0; i < dynamic_variable_values.size(); ++i ) {

    if( ! (
           un_any_dynamic_2_create
           ( variable_groups[i] , dynamic_variable_values[i] ,
             un_any_type< ColVariable >() ,
             un_any_type< std::vector< double > >() ,

             []( std::list< ColVariable > & list_variables ,
                 std::vector< double > & list_values ) {

             // Resize the vector of values so that it can accommodate
             // the values of all variables
             if( list_values.size() < list_variables.size() )
               list_values.resize( list_variables.size() );

             auto i1 = list_variables.begin();
             auto i2 = list_values.begin();

             for( ; i1 != list_variables.end() &&
                    i2 != list_values.end() ; ++i1 , ++i2 )
               *i2 = (*i1).get_value();
           } , read )

           ||

           un_any_dynamic_2_create
           ( variable_groups[i] , dynamic_variable_values[i] ,
             un_any_type< ColVariable * >() ,
             un_any_type< std::vector< double > >() ,

             []( std::list< ColVariable * > & list_variables ,
                 std::vector< double > & list_values ) {

             // Resize the vector of values so that it can accommodate
             // the values of all variables
             if( list_values.size() < list_variables.size() )
               list_values.resize( list_variables.size() );

             auto i1 = list_variables.begin();
             auto i2 = list_values.begin();

             for( ; i1 != list_variables.end() &&
                    i2 != list_values.end() ; ++i1 , ++i2 )
               *i2 = (*i1)->get_value();
           } , read ) ) )

      throw( std::logic_error
       ( std::string( "ColVariableSolution::"
                      "initialize_dynamic_variable_values: "
                      "invalid variable group: " ) +
         variable_groups[ i ].type().name() ) );
  }
}

/*--------------------------------------------------------------------------*/

template< class F1 , class F2 >
void ColVariableSolution::apply_static( const Block * const block ,
                                        F1 f1 , F2 f2 ) {

  auto & variable_groups = block->get_static_variables();

  if( variable_groups.size() != static_variable_values.size() )
    throw( std::logic_error
     ( std::string
        ( "ColVariableSolution::apply_static: "
          "number of static Variable groups of this ColVariableSolution (" ) +
       std::to_string( static_variable_values.size() ) +
       std::string( ") is different from that of the Block (" ) +
       std::to_string( variable_groups.size() ) + std::string( ")" ) ) );

  for( Vec_any::size_type i = 0; i < static_variable_values.size(); ++i ) {

    if( ! (
           un_any_static_2( variable_groups[i] , static_variable_values[i] ,
                            f1 ,
                            un_any_type< ColVariable >() ,
                            un_any_type< double >() )
           ||
           un_any_static_2( variable_groups[i] ,
                            static_variable_values[i] ,
                            f2 ,
                            un_any_type< ColVariable * >() ,
                            un_any_type< double >() )
           ) )

      throw( std::logic_error( std::string(
       "ColVariableSolution::apply_static: invalid types" ) ) );
  }
}

/*--------------------------------------------------------------------------*/

template< class F1 , class F2 >
void ColVariableSolution::apply_dynamic( const Block * const block ,
                                         F1 f1 , F2 f2 ) {

  auto & variable_groups = block->get_dynamic_variables();

  if( variable_groups.size() != dynamic_variable_values.size() )
    throw( std::logic_error
     ( std::string
        ( "ColVariableSolution::apply_dynamic: "
          "number of dynamic Variable groups of this ColVariableSolution (" ) +
       std::to_string( dynamic_variable_values.size() ) +
       std::string( ") is different from that of the Block (" ) +
       std::to_string( variable_groups.size() ) + std::string( ")" ) ) );

  for( Vec_any::size_type i = 0; i < dynamic_variable_values.size(); ++i ) {

    if( ! (
           un_any_dynamic_2( variable_groups[i] , dynamic_variable_values[i] ,
                             f1 ,
                             un_any_type< ColVariable >() ,
                             un_any_type< std::vector< double > >() )
           ||
           un_any_dynamic_2( variable_groups[i] , dynamic_variable_values[i] ,
                             f2 ,
                             un_any_type< ColVariable * >() ,
                             un_any_type< std::vector< double > >() )
           ) )

      throw( std::logic_error
       ( std::string( "ColVariableSolution::apply_dynamic: "
                      "invalid types" ) ) );
  }
}

/*--------------------------------------------------------------------------*/

void ColVariableSolution::read( const Block * const block ) {

  if( static_variable_values.size() != block->get_static_variables().size() ||
      dynamic_variable_values.size() != block->get_dynamic_variables().size() ||
      nested_solutions.size() != block->get_nested_Blocks().size() ) {
    // This ColVariableSolution does not have the same structure as
    // that of the Variables of the Block: initialize it and read the
    // solution from the Block.
    this->initialize( block , true );
    return;
  }

  try {

    apply_static( block ,
                  []( ColVariable & variable , double & value ) {
                    value = variable.get_value(); } ,
                  []( ColVariable * variable , double & value ) {
                    value = variable->get_value(); } );

    apply_dynamic( block ,
                   []( std::list< ColVariable > & list_variables ,
                       std::vector< double > & list_values ) {

                     // Resize the vector of values so that it can accommodate
                     // the values of all variables
                     if( list_values.size() < list_variables.size() )
                       list_values.resize( list_variables.size() );

                     auto i1 = list_variables.begin();
                     auto i2 = list_values.begin();

                     for( ; i1 != list_variables.end() &&
                            i2 != list_values.end() ; ++i1 , ++i2 )
                       *i2 = (*i1).get_value();
                   } ,

                   []( std::list< ColVariable * > & list_variables ,
                       std::vector< double > & list_values ) {

                     // Resize the vector of values so that it can accommodate
                     // the values of all variables
                     if( list_values.size() < list_variables.size() )
                       list_values.resize( list_variables.size() );

                     auto i1 = list_variables.begin();
                     auto i2 = list_values.begin();

                     for( ; i1 != list_variables.end() &&
                            i2 != list_values.end() ; ++i1 , ++i2 )
                       *i2 = (*i1)->get_value();
                   }
                   );
  }
  catch( std::exception & e ) {
    // The given Block and this ColVariableSolution do not have the
    // same structure. Initialize this ColVariableSolution so that it
    // is compatible with the given Block and read it.
    this->initialize( block , true );
    return;
  }

  // Read the solutions of the nested Blocks

  auto & sub_blocks = block->get_nested_Blocks();

  if( sub_blocks.size() != nested_solutions.size() )
      throw( std::logic_error
       ( std::string( "ColVariableSolution::read() "
                      "number of nested Blocks (" ) +
         std::to_string( sub_blocks.size() ) +
         std::string( ") is different from the "
                      "number of nested Solutions (" ) +
         std::to_string( nested_solutions.size() ) +
         std::string( ")" ) ) );

  auto sub_solution_iterator = nested_solutions.begin();
  for( auto & sub_block : sub_blocks ) {
    (*sub_solution_iterator++).read( sub_block );
  }
}

/*--------------------------------------------------------------------------*/

void ColVariableSolution::write( Block * const block ) {

  apply_static( block ,
                []( ColVariable & variable , double value ) {
                  variable.set_value(value); } ,
                []( ColVariable * variable , double value ) {
                  variable->set_value(value); } );

  apply_dynamic( block ,
                 []( std::list< ColVariable > & list_variables ,
                     std::vector< double > & list_values ) {

                   auto i1 = list_variables.begin();
                   auto i2 = list_values.begin();

                   for( ; i1 != list_variables.end() &&
                          i2 != list_values.end() ; ++i1 , ++i2 )
                     (*i1).set_value(*i2);

                   // If the number of Variables is greater than the number of
                   // values in the Solution, set the value of the remaining
                   // Variables to their default value.
                   for( ; i1 != list_variables.end() ; ++i1 )
                     (*i1).set_to_default_value();

                 } ,

                 []( std::list< ColVariable * > & list_variables ,
                     std::vector< double > & list_values ) {

                   auto i1 = list_variables.begin();
                   auto i2 = list_values.begin();

                   for( ; i1 != list_variables.end() &&
                          i2 != list_values.end() ; ++i1 , ++i2 )
                     (*i1)->set_value(*i2);

                   // If the number of Variables is greater than the number of
                   // values in the Solution, set the value of the remaining
                   // Variables to their default value.
                   for( ; i1 != list_variables.end() ; ++i1 )
                     (*i1)->set_to_default_value();

                 } );

  // Write the solutions of the nested Blocks

  auto & sub_blocks = block->get_nested_Blocks();

  if( sub_blocks.size() != nested_solutions.size() )
   throw( std::logic_error
    ( std::string( "ColVariableSolution::write: number of nested Blocks (" ) +
      std::to_string( sub_blocks.size() ) +
      std::string( ") is different from the number of nested Solutions (" ) +
      std::to_string( nested_solutions.size() ) + std::string( ")" ) ) );

  auto sub_solution_iterator = nested_solutions.begin();
  for( auto & sub_block : sub_blocks ) {
    (*sub_solution_iterator++).write( sub_block );
  }

}

/*--------------------------------------------------------------------------*/

void ColVariableSolution::serialize( netCDF::NcGroup & group ) const
{
 // always call the method of the base class first
 Solution::serialize( group );

 throw( std::logic_error( "ColVariableSolution::serialize not ready yet" ) );
 }

/*--------------------------------------------------------------------------*/

void ColVariableSolution::sum( const Solution * solution, double multiplier ) {

  auto other_solution = dynamic_cast< const ColVariableSolution * >( solution );

  if( ! other_solution )
    throw( std::invalid_argument
     ( "ColVariableSolution::sum: "
       "given Solution must be a ColVariableSolution" ) );

  if( empty() ) {
   scale( other_solution , multiplier );
   return;
   }

  if( this->static_variable_values.size() !=
      other_solution->static_variable_values.size() )

    throw( std::logic_error
     ( std::string( "ColVariableSolution::sum: "
                    "number of variable groups of this Solution (" ) +
       std::to_string( this->static_variable_values.size() ) +
       std::string( ") is different from the "
                    "number of variable groups (" ) +
       std::to_string( other_solution->static_variable_values.size() ) +
       std::string( ") of the given Solution" ) ) );

  // Sum the values of the static Variables

  for( Vec_any::size_type i = 0; i < static_variable_values.size(); ++i )

    if( ! un_any_static_2
        ( this->static_variable_values[i] ,
          other_solution->static_variable_values[i] ,
          [multiplier]( double & this_value , double & other_value ) {
          this_value += multiplier * other_value;
        } ,
          un_any_type< double >() , un_any_type< double >() ) )

      throw( std::logic_error
       ( std::string( "ColVariableSolution::sum: invalid or non-conforming "
                      "static variable group types: " ) +
         static_variable_values[ i ].type().name() + std::string( " and " ) +
         other_solution->static_variable_values[ i ].type().name() ) );

  // Sum the values of the dynamic Variables

  for( Vec_any::size_type i = 0; i < dynamic_variable_values.size(); ++i )

    if( ! un_any_static_2
        ( this->dynamic_variable_values[i] ,
          other_solution->dynamic_variable_values[i] ,
          [multiplier]( std::vector< double > & this_values ,
                        std::vector< double > & other_values ) {

          // Reserve space in case the other solution has more dynamic
          // Variable values
          this_values.reserve( other_values.size() );

          auto i1 = this_values.begin();
          auto i2 = other_values.begin();

          for( ; i1 != this_values.end() &&
                 i2 != other_values.end() ; ++i1 , ++i2 )
            *i1 += multiplier * (*i2);

          // Copy the values of the extra Variables
          for( ; i2 != other_values.end() ; ++i2 )
            *i1 = multiplier * (*i2);
        } ,
          un_any_type< std::vector< double > >() ,
          un_any_type< std::vector< double > >() ) )

      throw( std::logic_error
       ( std::string( "ColVariableSolution::sum: invalid or non-conforming "
                      "dynamic variable group types: " ) +
         this->dynamic_variable_values[ i ].type().name() +
         std::string( " and " ) +
         other_solution->dynamic_variable_values[ i ].type().name() ) );

  // Sum the solutions of the nested Blocks

  if( this->nested_solutions.size() != other_solution->nested_solutions.size() )

    throw( std::logic_error
     ( std::string( "ColVariableSolution::sum: "
                    "number of nested Solutions (" ) +
       std::to_string( this->nested_solutions.size() ) +
       std::string( ") of this Solution is different from the "
                    "number of nested Solutions (" ) +
       std::to_string( other_solution->nested_solutions.size() ) +
       std::string( ") of the given Solution" ) ) );

  auto i1 = this->nested_solutions.begin();
  auto i2 = other_solution->nested_solutions.begin();

  for( ; i1 != this->nested_solutions.end() ; ++i1 , ++i2 )
    (*i1).sum( &(*i2) , multiplier );
}

/*--------------------------------------------------------------------------*/

ColVariableSolution * ColVariableSolution::scale( double factor ) const {
  auto scaled_solution = new ColVariableSolution();
  scaled_solution->scale( this , factor );
  return( scaled_solution );
}

/*--------------------------------------------------------------------------*/

ColVariableSolution * ColVariableSolution::clone( bool empty ) const {
  auto cloned_solution = new ColVariableSolution();

  if( ! empty )
    cloned_solution->scale( this , 1.0 );

  return( cloned_solution );
}

/*--------------------------------------------------------------------------*/

void ColVariableSolution::scale( const ColVariableSolution * const solution ,
                                 const double factor ) {

  this->delete_vectors();

  // Scale static variable values

  this->static_variable_values.resize
    ( solution->static_variable_values.size() );

  for( Vec_any::size_type i = 0; i < static_variable_values.size(); ++i )

    if( ! un_any_static_2_create
        ( solution->static_variable_values[i] ,
          this->static_variable_values[i] ,
          un_any_type< double >() , un_any_type< double >() ,
          [factor]( double & given_solution_value , double & scaled_value ) {
          scaled_value = factor * given_solution_value;
        } ,
          true ) )

      throw( std::logic_error
       ( std::string( "ColVariableSolution::scale: invalid or non-conforming "
                      "static variable group types: " ) +
         solution->static_variable_values[ i ].type().name() +
         std::string( " and " ) +
         this->static_variable_values[ i ].type().name() ) );

  // Scale dynamic variable values

  this->dynamic_variable_values.resize
    ( solution->dynamic_variable_values.size() );

  for( Vec_any::size_type i = 0; i < dynamic_variable_values.size(); ++i )

    if( ! un_any_static_2_create
        ( solution->dynamic_variable_values[i] ,
          this->dynamic_variable_values[i] ,
          un_any_type< std::vector< double > >() ,
          un_any_type< std::vector< double > >() ,
          [factor]( std::vector< double > & given_solution_values ,
                    std::vector< double > & scaled_values ) {
          scaled_values.resize(0);
          scaled_values.reserve(given_solution_values.size());
          for( auto & value : given_solution_values )
            scaled_values.push_back( factor * value );
        } ) )

      throw( std::logic_error
       ( std::string( "ColVariableSolution::scale: invalid or non-conforming "
                      "dynamic variable group types: " ) +
         solution->dynamic_variable_values[ i ].type().name() +
         std::string( " and " ) +
         this->dynamic_variable_values[ i ].type().name() ) );

  // Scale the solutions of the nested Blocks

  this->nested_solutions.resize( solution->nested_solutions.size() );

  auto i1 = this->nested_solutions.begin();
  auto i2 = solution->nested_solutions.begin();

  for( ; i1 != this->nested_solutions.end() ; ++i1 , ++i2 )
    (*i1).scale( &(*i2) , factor );
}

/*--------------------------------------------------------------------------*/
/*----------------- End File ColVariableSolution.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
