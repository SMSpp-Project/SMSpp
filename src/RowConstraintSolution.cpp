/*--------------------------------------------------------------------------*/
/*-------------------- File RowConstraintSolution.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the RowConstraintSolution class.
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
#include "RowConstraint.h"
#include "RowConstraintSolution.h"
#include "FRowConstraint.h"
#include "OneVarConstraint.h"

/*--------------------------------------------------------------------------*/
/*----------------- CLASSES DERIVED FROM RowConstraint ---------------------*/
/*--------------------------------------------------------------------------*/

/* The following is a list of all concrete classes derived from RowConstraint
 * that will be considered in a RowConstraintSolution. This
 * RowConstraintSolution can only be used with a Block that has constraints of
 * these types.
 */

#define RowConstraint_Derived_Classes \
  FRowConstraint , BoxConstraint, ZOConstraint, NPConstraint, NNConstraint, \
  UBConstraint, LBConstraint, UB0Constraint, LB0Constraint

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register RowConstraintSolution to the Solution factory

SMSpp_insert_in_factory_cpp_0( RowConstraintSolution );

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void RowConstraintSolution::deserialize( const netCDF::NcGroup & group ) {
 throw( std::logic_error( "RowConstraintSolution::deserialize not ready yet" ) );
}

/*--------------------------------------------------------------------------*/

RowConstraintSolution::~RowConstraintSolution() {
 delete_vectors();
}

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::delete_vectors() {

 for( Vec_any::size_type i = 0 ; i < static_constraint_dual_values.size() ;
      ++i )

  if( ! un_any_thing( double , static_constraint_dual_values[ i ] ,
                      [ & var ]() { delete &var; }() ) )

   throw( std::logic_error
    ( "RowConstraintSolution::~RowConstraintSolution() "
      "invalid static constraint group: " +
      std::string( static_constraint_dual_values[ i ].type().name() ) ) );

 for( Vec_any::size_type i = 0 ;
      i < dynamic_constraint_dual_values.size() ; ++i )

  if( ! un_any_thing( std::vector< double > ,
                      dynamic_constraint_dual_values[ i ] ,
                      [ & var ]() { delete &var; }() ) )

   throw( std::logic_error
    ( "RowConstraintSolution::~RowConstraintSolution() "
      "invalid dynamic constraint group: " +
      std::string( dynamic_constraint_dual_values[ i ].type().name() ) ) );

 static_constraint_dual_values.resize( 0 );
 dynamic_constraint_dual_values.resize( 0 );
 nested_solutions.resize( 0 );
}

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::initialize( const Block * const block , bool read ) {
 this->delete_vectors();
 this->initialize_static_constraint_dual_values( block , read );
 this->initialize_dynamic_constraint_dual_values( block , read );

 // Initialize the Solutions of the nested Blocks

 auto & nested_blocks = block->get_nested_Blocks();
 this->nested_solutions.resize( nested_blocks.size() );
 auto nested_solution_it = this->nested_solutions.begin();
 auto nested_block_it = nested_blocks.begin();
 for( ; nested_solution_it != this->nested_solutions.end() ;
      ++nested_solution_it , ++nested_block_it )
  ( * nested_solution_it ).initialize( *nested_block_it , read );
}

/*--------------------------------------------------------------------------*/

template< class T >
bool create_static
( const boost::any & constraint_group ,
  boost::any & static_constraint_dual_value , const bool read ) {

 return
  un_any_static_2_create
  ( constraint_group , static_constraint_dual_value ,
    un_any_type< T >() , un_any_type< double >() ,
    []( T & constraint , double & value ) {
     value = constraint.get_dual(); } , read )
  ||
  un_any_static_2_create
  ( constraint_group , static_constraint_dual_value ,
    un_any_type< T * >() , un_any_type< double >() ,
    []( T * constraint , double & value ) {
     value = constraint->get_dual(); } , read );
}

/*--------------------------------------------------------------------------*/

template< class T , class... Rest >
bool try_create_static( const boost::any & constraint_group ,
                        boost::any & static_constraint_dual_value ,
                        const bool read ) {

 bool created = create_static< T >( constraint_group ,
                                    static_constraint_dual_value ,
                                    read );
 if( created )
  return( true );
 else if constexpr( sizeof...(Rest) != 0 )
  return( try_create_static< Rest... >( constraint_group ,
                                        static_constraint_dual_value ,
                                        read ) );
 return( false );
}

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::initialize_static_constraint_dual_values
( const Block * const block , bool read ) {

 // Initialize static constraint values

 const auto & constraint_groups = block->get_static_constraints();
 static_constraint_dual_values.resize( constraint_groups.size() );

 for( Vec_any::size_type i = 0; i < static_constraint_dual_values.size();
      ++i ) {

  if( ! try_create_static< RowConstraint_Derived_Classes >
      ( constraint_groups[ i ] , static_constraint_dual_values[ i ] , read ) )
   throw( std::logic_error
    ( "RowConstraintSolution::initialize_static_constraint_dual_values: "
      "invalid constraint group: " +
      std::string( constraint_groups[ i ].type().name() ) ) );
 }
}

/*--------------------------------------------------------------------------*/

template< class T >
bool create_dynamic
( const boost::any & constraint_group ,
  boost::any & dynamic_constraint_dual_value , const bool read ) {

 return
  ( un_any_dynamic_2_create
    ( constraint_group , dynamic_constraint_dual_value ,
      un_any_type< T >() , un_any_type< std::vector< double > >() ,

      []( std::list< T > & list_constraints ,
          std::vector< double > & list_values ) {

       // Resize the vector of values so that it can accommodate
       // the values of all constraints
       if( list_values.size() < list_constraints.size() )
        list_values.resize( list_constraints.size() );

       auto i1 = list_constraints.begin();
       auto i2 = list_values.begin();

       for( ; i1 != list_constraints.end() &&
             i2 != list_values.end() ; ++i1 , ++i2 )
        *i2 = ( *i1 ).get_dual();
      } , read )

    ||

    un_any_dynamic_2_create
    ( constraint_group , dynamic_constraint_dual_value ,
      un_any_type< T * >() , un_any_type< std::vector< double > >() ,

      []( std::list< T * > & list_constraints ,
          std::vector< double > & list_values ) {

       // Resize the vector of values so that it can accommodate
       // the values of all constraints
       if( list_values.size() < list_constraints.size() )
        list_values.resize( list_constraints.size() );

       auto i1 = list_constraints.begin();
       auto i2 = list_values.begin();

       for( ; i1 != list_constraints.end() &&
             i2 != list_values.end() ; ++i1 , ++i2 )
        *i2 = ( *i1 )->get_dual();
      } , read ) );
}

/*--------------------------------------------------------------------------*/

template< class T , class... Rest >
bool try_create_dynamic( const boost::any & constraint_group ,
                         boost::any & dynamic_constraint_dual_value ,
                         const bool read ) {

 bool created = create_dynamic< T >( constraint_group ,
                                     dynamic_constraint_dual_value ,
                                     read );
 if( created )
  return( true );
 else if constexpr( sizeof...(Rest) != 0 )
  return( try_create_dynamic< Rest... >( constraint_group ,
                                         dynamic_constraint_dual_value ,
                                         read ) );
 return( false );
}

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::initialize_dynamic_constraint_dual_values
( const Block * const block , bool read ) {

 // Initialize dynamic constraint values

 const auto & constraint_groups = block->get_dynamic_constraints();
 dynamic_constraint_dual_values.resize( constraint_groups.size() );

 for( Vec_any::size_type i = 0; i < dynamic_constraint_dual_values.size();
      ++i ) {

  if( ! try_create_dynamic< RowConstraint_Derived_Classes >
      ( constraint_groups[ i ] , dynamic_constraint_dual_values[ i ] , read ) )
   throw( std::logic_error(
    "RowConstraintSolution::initialize_dynamic_constraint_dual_values: "
    "invalid constraint group: " +
    std::string( constraint_groups[ i ].type().name() ) ) );
 }
}

/*--------------------------------------------------------------------------*/

template< class T , class F1 , class F2 >
bool un_static( const boost::any & constraint_group ,
                boost::any & static_constraint_dual_value , F1 f1 , F2 f2 ) {
 return
  ( un_any_static_2( constraint_group , static_constraint_dual_value ,
                     f1 , un_any_type< T >() , un_any_type< double >() )
    ||
    un_any_static_2( constraint_group , static_constraint_dual_value ,
                     f2 , un_any_type< T * >() , un_any_type< double >() ) );
}

/*--------------------------------------------------------------------------*/

template< class T >
bool un_static( const boost::any & constraint_group ,
                boost::any & static_constraint_dual_value ,
                const bool read ) {
 if( read ) {
  auto f1 = []( T & constraint , double & value ) {
             value = constraint.get_dual(); };
  auto f2 = []( T * constraint , double & value ) {
             value = constraint->get_dual(); };
  return( un_static< T , decltype( f1 ) , decltype( f2 ) >
   ( constraint_group , static_constraint_dual_value , f1 , f2 ) );
 }
 else {
  auto f1 = []( T & constraint , double value ) {
             constraint.set_dual( value ); };
  auto f2 = []( T * constraint , double value ) {
             constraint->set_dual( value ); };
  return( un_static< T , decltype( f1 ) , decltype( f2 ) >
   ( constraint_group , static_constraint_dual_value , f1 , f2 ) );
 }
}

/*--------------------------------------------------------------------------*/


template< class T , class... Rest >
bool try_un_static( const boost::any & constraint_group ,
                    boost::any & static_constraint_dual_value ,
                    const bool read ) {

 bool created = un_static< T >( constraint_group ,
                                static_constraint_dual_value ,
                                read );
 if( created )
  return( true );
 else if constexpr( sizeof...(Rest) != 0 )
  return( try_un_static< Rest... >( constraint_group ,
                                    static_constraint_dual_value ,
                                    read ) );
 return( false );
}

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::apply_static( const Block * const block ,
                                          const bool read ) {

 auto & constraint_groups = block->get_static_constraints();

 if( constraint_groups.size() != static_constraint_dual_values.size() )
  throw( std::logic_error
   ( "RowConstraintSolution::apply_static: number of static "
     "Constraint groups of this RowConstraintSolution ("
     + std::to_string( static_constraint_dual_values.size() )
     + ") is different from that of the Block ("
     + std::to_string( constraint_groups.size() ) + ")" ) );

 for( Vec_any::size_type i = 0; i < static_constraint_dual_values.size();
      ++i ) {

  if( ! try_un_static< RowConstraint_Derived_Classes >
      ( constraint_groups[ i ] , static_constraint_dual_values[ i ] , read ) )
   throw( std::logic_error( "RowConstraintSolution::apply_static: "
                            "invalid types" ) );
 }
}

/*--------------------------------------------------------------------------*/

template< class T , class F1 , class F2 >
bool un_dynamic( const boost::any & constraint_group ,
                 boost::any & dynamic_constraint_dual_value ,
                 F1 f1 , F2 f2 ) {
 return
  ( un_any_dynamic_2( constraint_group , dynamic_constraint_dual_value , f1 ,
                      un_any_type< T >() ,
                      un_any_type< std::vector< double > >() )
    ||
    un_any_dynamic_2( constraint_group , dynamic_constraint_dual_value , f2 ,
                      un_any_type< T * >() ,
                      un_any_type< std::vector< double > >() )
    );
}

/*--------------------------------------------------------------------------*/

template< class T >
bool un_dynamic( const boost::any & constraint_group ,
                 boost::any & dynamic_constraint_dual_value ,
                 const bool read ,
                 const RowConstraint::RHSValue default_dual_value = 0 ) {

 if( read ) {
  auto f1 = []( std::list< T > & list_constraints ,
                std::vector< double > & list_values ) {

             // Resize the vector of values so that it can accommodate
             // the values of all constraints
             if( list_values.size() < list_constraints.size() )
              list_values.resize( list_constraints.size() );

             auto i1 = list_constraints.begin();
             auto i2 = list_values.begin();

             for( ; i1 != list_constraints.end() &&
                   i2 != list_values.end() ; ++i1 , ++i2 )
              *i2 = ( *i1 ).get_dual();
            };

  auto f2 = []( std::list< T * > & list_constraints ,
                std::vector< double > & list_values ) {

             // Resize the vector of values so that it can accommodate
             // the values of all constraints
             if( list_values.size() < list_constraints.size() )
              list_values.resize( list_constraints.size() );

             auto i1 = list_constraints.begin();
             auto i2 = list_values.begin();

             for( ; i1 != list_constraints.end() &&
                   i2 != list_values.end() ; ++i1 , ++i2 )
              *i2 = ( *i1 )->get_dual();
            };

  return( un_dynamic< T , decltype( f1 ) , decltype( f2 ) >
   ( constraint_group , dynamic_constraint_dual_value , f1 , f2 ) );
 }

 else {
  auto f1 = [ default_dual_value ]
   ( std::list< T > & list_constraints ,
     std::vector< double > & list_values ) {

             auto i1 = list_constraints.begin();
             auto i2 = list_values.begin();

             for( ; i1 != list_constraints.end() &&
                   i2 != list_values.end() ; ++i1 , ++i2 )
              ( *i1 ).set_dual( *i2 );

             // If the number of Constraints is greater than the number of
             // values in the Solution, set the dual value of the
             // remaining Constraints to their default value.
             for( ; i1 != list_constraints.end() ; ++i1 )
              ( *i1 ).set_dual( default_dual_value );
            };

  auto f2 = [ default_dual_value ]
   ( std::list< T * > & list_constraints ,
     std::vector< double > & list_values ) {

             auto i1 = list_constraints.begin();
             auto i2 = list_values.begin();

             for( ; i1 != list_constraints.end() &&
                   i2 != list_values.end() ; ++i1 , ++i2 )
              ( *i1 )->set_dual( *i2 );

             // If the number of Constraints is greater than the number of
             // values in the Solution, set the dual value of the
             // remaining Constraints to their default value.
             for( ; i1 != list_constraints.end() ; ++i1 )
              ( *i1 )->set_dual( default_dual_value );

            };

  return( un_dynamic< T , decltype( f1 ) , decltype( f2 ) >
   ( constraint_group , dynamic_constraint_dual_value , f1 , f2 ) );
 }
}

/*--------------------------------------------------------------------------*/

template< class T , class... Rest >
bool try_un_dynamic( const boost::any & constraint_group ,
                     boost::any & dynamic_constraint_dual_value ,
                     const bool read ,
                     const RowConstraint::RHSValue default_dual_value ) {

 bool created = un_dynamic< T >( constraint_group ,
                               dynamic_constraint_dual_value ,
                               read , default_dual_value );
 if( created )
  return( true );
 else if constexpr( sizeof...(Rest) != 0 )
  return( try_un_dynamic< Rest... >( constraint_group ,
                                     dynamic_constraint_dual_value ,
                                     read , default_dual_value ) );
 return( false );
}

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::apply_dynamic
( const Block * const block , const bool read ,
  const RowConstraint::RHSValue default_dual_value ) {

 auto & constraint_groups = block->get_dynamic_constraints();

 if( constraint_groups.size() != dynamic_constraint_dual_values.size() )
  throw( std::logic_error
   ( "RowConstraintSolution::apply_dynamic(): number of dynamic Constraint "
     "groups of this RowConstraintSolution (" +
     std::to_string( dynamic_constraint_dual_values.size() ) +
     ") is different from that of the Block (" +
     std::to_string( constraint_groups.size() ) + ")" ) );

 for( Vec_any::size_type i = 0; i < dynamic_constraint_dual_values.size();
      ++i ) {

  if( ! try_un_dynamic< RowConstraint_Derived_Classes >
      ( constraint_groups[ i ] , dynamic_constraint_dual_values[ i ] ,
        read , default_dual_value ) )
   throw( std::logic_error(
    "RowConstraintSolution::apply_dynamic: invalid types" ) );
 }
}

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::read( const Block * const block ) {

 if( ( static_constraint_dual_values.size() !=
       block->get_static_constraints().size() ) ||
     ( dynamic_constraint_dual_values.size() !=
       block->get_dynamic_constraints().size() ) ||
     ( nested_solutions.size() != block->get_nested_Blocks().size() ) ) {
  // This RowConstraintSolution does not have the same structure as
  // that of the Constraints of the Block: initialize it and read the
  // solution from the Block.
  this->initialize( block , true );
  return;
 }

 try {
  apply_static( block , true );
  apply_dynamic( block , true );
 }
 catch( std::exception & e ) {
  // The given Block and this RowConstraintSolution do not have the
  // same structure. Initialize this RowConstraintSolution so that it
  // is compatible with the given Block and read it.
  this->initialize( block , true );
  return;
 }

 // Read the solutions of the nested Blocks

 auto & sub_blocks = block->get_nested_Blocks();

 if( sub_blocks.size() != nested_solutions.size() )
  throw( std::logic_error( "RowConstraintSolution::read(): "
                           "number of nested Blocks (" +
                           std::to_string( sub_blocks.size() ) +
                           ") is different from the "
                           "number of nested Solutions (" +
                           std::to_string( nested_solutions.size() ) + ")" ) );

 auto sub_solution_iterator = nested_solutions.begin();
 for( auto & sub_block : sub_blocks ) {
  ( *sub_solution_iterator++ ).read( sub_block );
 }
}

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::write( Block * const block ) {

 RowConstraint::RHSValue default_dual_value = 0;

 apply_static( block , false );
 apply_dynamic( block , false , default_dual_value );

 // Write the solutions of the nested Blocks

 auto & sub_blocks = block->get_nested_Blocks();

 if( sub_blocks.size() != nested_solutions.size() )
  throw( std::logic_error( "RowConstraintSolution::read(): "
                           "number of nested Blocks (" +
                           std::to_string( sub_blocks.size() ) +
                           ") is different from the "
                           "number of nested Solutions (" +
                           std::to_string( nested_solutions.size() ) +
                           ")" ) );

 auto sub_solution_iterator = nested_solutions.begin();
 for( auto & sub_block : sub_blocks ) {
  ( *sub_solution_iterator++ ).write( sub_block );
 }
}

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::serialize( netCDF::NcGroup & group ) const
{
 // always call the method of the base class first
 Solution::serialize( group );

 throw( std::logic_error( " RowConstraintSolution::serialize not ready yet" ) );
 }

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::sum( const Solution * solution,
                                 double multiplier ) {

 auto other_solution =
  dynamic_cast< const RowConstraintSolution * >( solution );

 if( ! other_solution )
  throw( std::invalid_argument( "RowConstraintSolution::sum: given Solution "
                                "must be a RowConstraintSolution" ) );

 if( empty() ) {
  scale( other_solution , multiplier );
  return;
  }

 if( this->static_constraint_dual_values.size() !=
     other_solution->static_constraint_dual_values.size() )

  throw( std::logic_error
   ( "RowConstraintSolution::sum() "
     "number of constraint groups of this Solution (" +
     std::to_string( this->static_constraint_dual_values.size() ) +
     ") is different from the number of constraint groups (" +
     std::to_string( other_solution->static_constraint_dual_values.size() ) +
     ") of the given Solution" ) );

 // Sum the values of the static Constraints

 for( Vec_any::size_type i = 0; i < static_constraint_dual_values.size(); ++i )

  if( ! un_any_static_2
      ( this->static_constraint_dual_values[ i ] ,
        other_solution->static_constraint_dual_values[ i ] ,
        [ multiplier ]( double & this_value , double & other_value ) {
       this_value += multiplier * other_value;
      } ,
        un_any_type< double >() , un_any_type< double >() ) )

   throw( std::logic_error
    ( "RowConstraintSolution::sum: invalid or non-conforming "
      "static constraint group types: " +
      std::string( static_constraint_dual_values[ i ].type().name() )
      + " and " +
      other_solution->static_constraint_dual_values[ i ].type().name() ) );

 // Sum the values of the dynamic Constraints

 for( Vec_any::size_type i = 0; i < dynamic_constraint_dual_values.size(); ++i )

  if( ! un_any_static_2
      ( this->dynamic_constraint_dual_values[ i ] ,
        other_solution->dynamic_constraint_dual_values[ i ] ,
        [ multiplier ]( std::vector< double > & this_values ,
                        std::vector< double > & other_values ) {

       // Reserve space in case the other solution has more dynamic
       // Constraint values
       this_values.reserve( other_values.size() );

       auto i1 = this_values.begin();
       auto i2 = other_values.begin();

       for( ; i1 != this_values.end() &&
             i2 != other_values.end() ; ++i1 , ++i2 )
        *i1 += multiplier * ( *i2 );

       // Copy the values of the extra Constraints
       for( ; i2 != other_values.end() ; ++i2 )
        *i1 = multiplier * ( *i2 );
      } ,
        un_any_type< std::vector< double > >() ,
        un_any_type< std::vector< double > >() ) )

   throw( std::logic_error
    ( "RowConstraintSolution::sum(): invalid or non-conforming dynamic "
      "constraint group types: " +
      std::string( this->dynamic_constraint_dual_values[ i ].type().name() ) +
      " and " +
      other_solution->dynamic_constraint_dual_values[ i ].type().name() ) );

 // Sum the solutions of the nested Blocks

 if( this->nested_solutions.size() != other_solution->nested_solutions.size() )

  throw( std::logic_error( "RowConstraintSolution::sum(): "
                           "number of nested Solutions (" +
                           std::to_string( this->nested_solutions.size() ) +
                           ") of this Solution is different from the "
                           "number of nested Solutions (" +
                           std::to_string( other_solution->
                            nested_solutions.size() ) +
                           ") of the given Solution" ) );

 auto i1 = this->nested_solutions.begin();
 auto i2 = other_solution->nested_solutions.begin();

 for( ; i1 != this->nested_solutions.end() ; ++i1 , ++i2 )
  ( *i1 ).sum( &( *i2 ) , multiplier );
}

/*--------------------------------------------------------------------------*/

RowConstraintSolution * RowConstraintSolution::scale( double factor ) const {
 auto scaled_solution = new RowConstraintSolution();
 scaled_solution->scale( this , factor );
 return( scaled_solution );
}

/*--------------------------------------------------------------------------*/

RowConstraintSolution * RowConstraintSolution::clone( bool empty ) const {
 auto cloned_solution = new RowConstraintSolution();

 if( ! empty )
  cloned_solution->scale( this , 1.0 );

 return( cloned_solution );
}

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::scale( const RowConstraintSolution * const solution ,
                                   const double factor ) {

 this->delete_vectors();

 // Scale static constraint values

 this->static_constraint_dual_values.resize
  ( solution->static_constraint_dual_values.size() );

 for( Vec_any::size_type i = 0; i < static_constraint_dual_values.size(); ++i )

  if( ! un_any_static_2_create
      ( solution->static_constraint_dual_values[ i ] ,
        this->static_constraint_dual_values[ i ] ,
        un_any_type< double >() , un_any_type< double >() ,
        [ factor ]( double & given_solution_value , double & scaled_value ) {
         scaled_value = factor * given_solution_value;
        } ,
        true ) )

   throw( std::logic_error
    ( "RowConstraintSolution::sum(): invalid or "
      "non-conforming static constraint group types: " +
      std::string( solution->static_constraint_dual_values[ i ].type().name() )
      + " and " +
      std::string( this->static_constraint_dual_values[ i ].type().name() ) ) );

 // Scale dynamic constraint values

 this->dynamic_constraint_dual_values.resize
  ( solution->dynamic_constraint_dual_values.size() );

 for( Vec_any::size_type i = 0; i < dynamic_constraint_dual_values.size(); ++i )

  if( ! un_any_static_2_create
      ( solution->dynamic_constraint_dual_values[ i ] ,
        this->dynamic_constraint_dual_values[ i ] ,
        un_any_type< std::vector< double > >() ,
        un_any_type< std::vector< double > >() ,
        [ factor ]( std::vector< double > & given_solution_values ,
                    std::vector< double > & scaled_values ) {
         scaled_values.resize( 0 );
         scaled_values.reserve( given_solution_values.size() );
         for( auto & value : given_solution_values )
          scaled_values.push_back( factor * value );
        } ) )

   throw( std::logic_error
    ( "RowConstraintSolution::sum(): invalid or "
      "non-conforming dynamic constraint group types: " +
      std::string( solution->dynamic_constraint_dual_values[ i ].type().name() )
      + " and " +
      std::string( this->dynamic_constraint_dual_values[ i ].type().name() )
    ) );

 // Scale the solutions of the nested Blocks

 this->nested_solutions.resize( solution->nested_solutions.size() );

 auto i1 = this->nested_solutions.begin();
 auto i2 = solution->nested_solutions.begin();

 for( ; i1 != this->nested_solutions.end() ; ++i1 , ++i2 )
  ( *i1 ).scale( &( *i2 ) , factor );
}

/*--------------------------------------------------------------------------*/
/*---------------- End File RowConstraintSolution.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
