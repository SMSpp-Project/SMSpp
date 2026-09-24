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

#include "FRowConstraint.h"
#include "OneVarConstraint.h"
#include "RowConstraintSolution.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*-------------------------------- FUNCTIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

/// calls f() on each element of a group of RowConstraint, typed on its class
/** The elements of the group are all of one of the concrete :RowConstraint
 * of the core, which on_group() has checked: the type is matched exactly,
 * and the loop runs on it. */

template< class F >
void for_each_row( const BaseGroup & group , F f )
{
 auto type = group.get_element_type();
 if( type == typeid( FRowConstraint ) )
  group.for_each_as< FRowConstraint >( f );
 else if( type == typeid( BoxConstraint ) )
  group.for_each_as< BoxConstraint >( f );
 else if( type == typeid( LB0Constraint ) )
  group.for_each_as< LB0Constraint >( f );
 else if( type == typeid( UB0Constraint ) )
  group.for_each_as< UB0Constraint >( f );
 else if( type == typeid( LBConstraint ) )
  group.for_each_as< LBConstraint >( f );
 else if( type == typeid( UBConstraint ) )
  group.for_each_as< UBConstraint >( f );
 else if( type == typeid( NNConstraint ) )
  group.for_each_as< NNConstraint >( f );
 else if( type == typeid( NPConstraint ) )
  group.for_each_as< NPConstraint >( f );
 else if( type == typeid( ZOConstraint ) )
  group.for_each_as< ZOConstraint >( f );
 }

/*--------------------------------------------------------------------------*/
/// calls f( c , cell ) on each cell of a group of RowConstraint
/** The counterpart of for_each_row() for the groups whose cells are
 * collections. */

template< class F >
void for_each_row_cell( const BaseGroup & group , F f )
{
 auto type = group.get_element_type();
 if( type == typeid( FRowConstraint ) )
  group.for_each_cell_as< FRowConstraint >( f );
 else if( type == typeid( BoxConstraint ) )
  group.for_each_cell_as< BoxConstraint >( f );
 else if( type == typeid( LB0Constraint ) )
  group.for_each_cell_as< LB0Constraint >( f );
 else if( type == typeid( UB0Constraint ) )
  group.for_each_cell_as< UB0Constraint >( f );
 else if( type == typeid( LBConstraint ) )
  group.for_each_cell_as< LBConstraint >( f );
 else if( type == typeid( UBConstraint ) )
  group.for_each_cell_as< UBConstraint >( f );
 else if( type == typeid( NNConstraint ) )
  group.for_each_cell_as< NNConstraint >( f );
 else if( type == typeid( NPConstraint ) )
  group.for_each_cell_as< NPConstraint >( f );
 else if( type == typeid( ZOConstraint ) )
  group.for_each_cell_as< ZOConstraint >( f );
 }

/*--------------------------------------------------------------------------*/
/// calls f() on a group, returning false if it is not one for_each_row() reads

template< class F >
bool on_group( const std::unique_ptr< BaseGroup > & group , F f )
{
 if( ! group )
  return( false );
 auto type = group->get_element_type();
 if( ( type != typeid( FRowConstraint ) ) &&
     ( type != typeid( BoxConstraint ) ) &&
     ( type != typeid( LB0Constraint ) ) &&
     ( type != typeid( UB0Constraint ) ) &&
     ( type != typeid( LBConstraint ) ) &&
     ( type != typeid( UBConstraint ) ) &&
     ( type != typeid( NNConstraint ) ) &&
     ( type != typeid( NPConstraint ) ) &&
     ( type != typeid( ZOConstraint ) ) )
  return( false );
 f( std::as_const( *group ) );
 return( true );
 }

}  // end( unnamed namespace )

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register RowConstraintSolution to the Solution factory

SMSpp_insert_in_factory_cpp_0( RowConstraintSolution );

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void RowConstraintSolution::deserialize( const netCDF::NcGroup & group ) {
 // the inverse of serialize(): what is not there is read as nothing, so a
 // Solution that holds no Constraint of one kind deserializes into an empty
 // one rather than into an error
 static_constraint_dual_values.clear();
 dynamic_constraint_dual_values.clear();
 nested_solutions.clear();

 ::deserialize< double >( group , "StaticDuals" , "StaticDualsStart" ,
			  static_constraint_dual_values );

 std::vector< std::vector< double > > cells;
 if( ::deserialize< double >( group , "DynamicDuals" , "DynamicDualsStart" ,
			      cells ) ) {
  auto ncVar = group.getVar( "DynamicCellsStart" );
  if( ncVar.isNull() )
   throw( std::invalid_argument( "RowConstraintSolution::deserialize: "
				 "DynamicDuals without DynamicCellsStart" ) );

  std::vector< int > group_start( ncVar.getDim( 0 ).getSize() );
  ncVar.getVar( group_start.data() );

  dynamic_constraint_dual_values.resize( group_start.size() );
  for( std::size_t g = 0 ; g < group_start.size() ; ++g ) {
   const auto begin = std::size_t( group_start[ g ] );
   const auto end = ( g + 1 < group_start.size() )
                    ? std::size_t( group_start[ g + 1 ] ) : cells.size();
   if( ( begin > end ) || ( end > cells.size() ) )
    throw( std::invalid_argument( "RowConstraintSolution::deserialize: "
				  "wrong indices in DynamicCellsStart" ) );
   dynamic_constraint_dual_values[ g ].assign( cells.begin() + begin ,
					       cells.begin() + end );
   }
  }

 std::size_t n = 0;
 while( ! group.getGroup( "NestedSolution_" + std::to_string( n ) ).isNull() )
  ++n;

 nested_solutions.resize( n );
 for( std::size_t i = 0 ; i < n ; ++i )
  nested_solutions[ i ].deserialize(
		   group.getGroup( "NestedSolution_" + std::to_string( i ) ) );
}

/*--------------------------------------------------------------------------*/

RowConstraintSolution::~RowConstraintSolution() {
 delete_vectors();
}

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::delete_vectors() {
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

void RowConstraintSolution::initialize_static_constraint_dual_values
( const Block * const block , bool read ) {

 const auto & groups = block->get_static_constraint_groups();
 static_constraint_dual_values.resize( groups.size() );

 for( Vec_Group::size_type i = 0 ; i < static_constraint_dual_values.size() ;
      ++i ) {
  auto & values = static_constraint_dual_values[ i ];
  if( ! on_group( groups[ i ] ,
                 [ & values , read ]( const BaseGroup & group ) {
       values.assign( group.get_num_elements() , 0 );
       if( read ) {
	auto value = values.data();
	for_each_row( group , [ & value ]( auto & con ) {
	  *(value++) = con.get_dual(); } );
	}
       } ) )
   throw( std::logic_error
    ( "RowConstraintSolution::initialize_static_constraint_dual_values: "
      "invalid constraint group: " +
      std::string( ( groups[ i ] ? groups[ i ]->get_element_type().name()
                           : "an empty group" ) ) ) );
  }
}

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::initialize_dynamic_constraint_dual_values
( const Block * const block , bool read ) {

 const auto & groups = block->get_dynamic_constraint_groups();
 dynamic_constraint_dual_values.resize( groups.size() );

 for( Vec_Group::size_type i = 0 ; i < dynamic_constraint_dual_values.size() ;
      ++i ) {
  auto & values = dynamic_constraint_dual_values[ i ];
  if( ! on_group( groups[ i ] ,
                 [ & values , read ]( const BaseGroup & group ) {
       values.assign( group.get_num_cells() , {} );
       if( read )
	for_each_row_cell( group ,
			   [ & values ]( BaseGroup::Index c , auto & cell ) {
	  values[ c ].resize( cell.size() );
	  auto value = values[ c ].data();
	  for( auto & item : cell )
	   *(value++) = group_element( item ).get_dual();
	  } );
       } ) )
   throw( std::logic_error(
    "RowConstraintSolution::initialize_dynamic_constraint_dual_values: "
    "invalid constraint group: " +
    std::string( ( groups[ i ] ? groups[ i ]->get_element_type().name()
                           : "an empty group" ) ) ) );
  }
}

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::apply_static( const Block * const block ,
                                          const bool read ) {

 const auto & groups = block->get_static_constraint_groups();

 if( groups.size() != static_constraint_dual_values.size() )
  throw( std::logic_error
   ( "RowConstraintSolution::apply_static: number of static "
     "Constraint groups of this RowConstraintSolution ("
     + std::to_string( static_constraint_dual_values.size() )
     + ") is different from that of the Block ("
     + std::to_string( groups.size() ) + ")" ) );

 for( Vec_Group::size_type i = 0 ; i < static_constraint_dual_values.size() ;
      ++i ) {
  auto & values = static_constraint_dual_values[ i ];
  bool conforming = true;
  if( ! on_group( groups[ i ] ,
                 [ & ]( const BaseGroup & group ) {
       if( group.get_num_elements() != values.size() ) {
	conforming = false;
	return;
	}
       auto value = values.data();
       if( read )
	for_each_row( group , [ & value ]( auto & con ) {
	  *(value++) = con.get_dual(); } );
       else
	for_each_row( group , [ & value ]( auto & con ) {
	  con.set_dual( *(value++) ); } );
       } ) )
   throw( std::logic_error( "RowConstraintSolution::apply_static: "
                            "invalid types" ) );
  if( ! conforming )
   throw( std::logic_error( "RowConstraintSolution::apply_static: the size "
			    "of static Constraint group " +
			    std::to_string( i ) +
			    " is different from that of the Block" ) );
  }
}

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::apply_dynamic
( const Block * const block , const bool read ,
  const RowConstraint::RHSValue default_dual_value ) {

 const auto & groups = block->get_dynamic_constraint_groups();

 if( groups.size() != dynamic_constraint_dual_values.size() )
  throw( std::logic_error
   ( "RowConstraintSolution::apply_dynamic(): number of dynamic Constraint "
     "groups of this RowConstraintSolution (" +
     std::to_string( dynamic_constraint_dual_values.size() ) +
     ") is different from that of the Block (" +
     std::to_string( groups.size() ) + ")" ) );

 for( Vec_Group::size_type i = 0 ; i < dynamic_constraint_dual_values.size() ;
      ++i ) {
  auto & values = dynamic_constraint_dual_values[ i ];
  bool conforming = true;
  if( ! on_group( groups[ i ] ,
                 [ & ]( const BaseGroup & group ) {
       if( group.get_num_cells() != values.size() ) {
	conforming = false;
	return;
	}
       for_each_row_cell( group ,
			  [ & values , read , default_dual_value ]
			  ( BaseGroup::Index c , auto & cell ) {
	auto & cell_values = values[ c ];
	if( read ) {
	 // the values of a cell may outlive the Constraint they were read
	 // from, hence the vector only grows
	 if( cell_values.size() < cell.size() )
	  cell_values.resize( cell.size() );
	 auto value = cell_values.data();
	 for( auto & item : cell )
	  *(value++) = group_element( item ).get_dual();
	 }
	else {
	 // Constraint beyond the stored values get the default dual value
	 auto value = cell_values.begin();
	 for( auto & item : cell )
	  group_element( item ).set_dual( value != cell_values.end() ?
					  *(value++) : default_dual_value );
	 }
	} );
       } ) )
   throw( std::logic_error(
    "RowConstraintSolution::apply_dynamic: invalid types" ) );
  if( ! conforming )
   throw( std::logic_error( "RowConstraintSolution::apply_dynamic: the "
			    "number of cells of dynamic Constraint group " +
			    std::to_string( i ) +
			    " is different from that of the Block" ) );
  }
}

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::read( const Block * const block ) {

 // what the Block holds, a solution or a direction, is what this Solution
 // holds from now on
 f_direction = block->is_direction();

 if( ( static_constraint_dual_values.size() !=
       block->get_static_constraint_groups().size() ) ||
     ( dynamic_constraint_dual_values.size() !=
       block->get_dynamic_constraint_groups().size() ) ||
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

bool RowConstraintSolution::drop_dynamic_values
( const Block * const block , const void * cell ,
  const Block::Subset & positions , std::vector< double > & dropped )
{
 // look for the cell among the groups of dynamic Constraint of this Block

 const auto & groups = block->get_dynamic_constraint_groups();

 if( groups.size() == dynamic_constraint_dual_values.size() )
  for( Block::Index i = 0 ; i < groups.size() ; ++i ) {
   auto & values = dynamic_constraint_dual_values[ i ];
   bool found = false;

   on_group( groups[ i ] , [ & ]( const BaseGroup & group ) {
     for_each_row_cell( group , [ & ]( BaseGroup::Index c , auto & cll ) {
       if( found || ( static_cast< const void * >( & cll ) != cell ) )
	return;
       found = true;
       if( c >= values.size() )  // nothing is held for this cell
	return;
       auto & cell_values = values[ c ];

       if( positions.empty() ) {  // the whole cell is gone
	dropped.assign( cell_values.begin() , cell_values.end() );
	cell_values.clear();
	return;
	}

       dropped.assign( positions.size() , 0 );
       for( Block::Index k = 0 ; k < positions.size() ; ++k )
	if( positions[ k ] < cell_values.size() )
	 dropped[ k ] = cell_values[ positions[ k ] ];

       // erase from the back, so that the positions keep their meaning
       auto sorted = positions;
       std::sort( sorted.begin() , sorted.end() , std::greater<>() );
       for( auto p : sorted )
	if( p < cell_values.size() )
	 cell_values.erase( cell_values.begin() + p );
       } );
     } );

   if( found )
    return( true );
   }

 // it is not in this Block: look in the nested ones

 const auto & sub_blocks = block->get_nested_Blocks();

 if( sub_blocks.size() != nested_solutions.size() )
  return( false );

 for( Block::Index i = 0 ; i < sub_blocks.size() ; ++i )
  if( nested_solutions[ i ].drop_dynamic_values( sub_blocks[ i ] , cell ,
						 positions , dropped ) )
   return( true );

 return( false );

 }  // end( RowConstraintSolution::drop_dynamic_values )

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::write( Block * const block ) {

 RowConstraint::RHSValue default_dual_value = 0;

 apply_static( block , false );
 apply_dynamic( block , false , default_dual_value );

 // Write the solutions of the nested Blocks

 auto & sub_blocks = block->get_nested_Blocks();

 if( sub_blocks.size() != nested_solutions.size() )
  throw( std::logic_error( "RowConstraintSolution::write(): "
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

 /* The dual values of the static Constraint are a matrix with rows of
  * different length, one row per group [see serialize() in SMSTypedefs.h];
  * those of the dynamic ones have one level more, the cells of all the
  * groups going in one such matrix and DynamicCellsStart saying which of
  * those cells each group begins at. */

 if( ! static_constraint_dual_values.empty() )
  ::serialize< double >( group , "StaticDuals" , netCDF::NcDouble() ,
			 "StaticDualsStart" , static_constraint_dual_values );

 if( ! dynamic_constraint_dual_values.empty() ) {
  std::vector< std::vector< double > > cells;
  std::vector< int > group_start;
  group_start.reserve( dynamic_constraint_dual_values.size() );

  for( const auto & grp : dynamic_constraint_dual_values ) {
   group_start.push_back( int( cells.size() ) );
   for( const auto & cell : grp )
    cells.push_back( cell );
   }

  ::serialize< double >( group , "DynamicDuals" , netCDF::NcDouble() ,
			 "DynamicDualsStart" , cells );

  auto ncDim = group.addDim( "NumberDynamicGroups" , group_start.size() );
  auto ncVar = group.addVar( "DynamicCellsStart" , netCDF::NcInt() , ncDim );
  ncVar.putVar( group_start.data() );
  }

 if( ! nested_solutions.empty() ) {
  group.addDim( "NumberNestedSolutions" , nested_solutions.size() );
  for( std::size_t i = 0 ; i < nested_solutions.size() ; ++i ) {
   auto sg = group.addGroup( "NestedSolution_" + std::to_string( i ) );
   nested_solutions[ i ].serialize( sg );
   }
  }
 }

/*--------------------------------------------------------------------------*/

void RowConstraintSolution::sum( const Solution * solution,
                                 double multiplier ) {

 auto other_solution =
  dynamic_cast< const RowConstraintSolution * >( solution );

 if( ! other_solution )
  throw( std::invalid_argument( "RowConstraintSolution::sum: given Solution "
                                "must be a RowConstraintSolution" ) );

 // the sum is a direction only if every Solution in it is one
 f_direction = f_direction && other_solution->f_direction;

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

 if( this->dynamic_constraint_dual_values.size() !=
     other_solution->dynamic_constraint_dual_values.size() )

  throw( std::logic_error
   ( "RowConstraintSolution::sum() "
     "number of dynamic constraint groups of this Solution (" +
     std::to_string( this->dynamic_constraint_dual_values.size() ) +
     ") is different from the number of dynamic constraint groups (" +
     std::to_string( other_solution->dynamic_constraint_dual_values.size() ) +
     ") of the given Solution" ) );

 // Sum the values of the static Constraints

 for( Vec_Group::size_type i = 0 ; i < static_constraint_dual_values.size() ;
      ++i ) {
  auto & values = static_constraint_dual_values[ i ];
  const auto & other_values = other_solution->static_constraint_dual_values[ i ];
  if( values.size() != other_values.size() )
   throw( std::logic_error( "RowConstraintSolution::sum: static constraint "
			    "group " + std::to_string( i ) + " has " +
			    std::to_string( values.size() ) + " values here "
			    "and " + std::to_string( other_values.size() ) +
			    " in the given Solution" ) );
  for( std::size_t k = 0 ; k < values.size() ; ++k )
   values[ k ] += multiplier * other_values[ k ];
  }

 // Sum the values of the dynamic Constraints: a value missing on either side
 // counts as zero

 for( Vec_Group::size_type i = 0 ; i < dynamic_constraint_dual_values.size() ;
      ++i ) {
  auto & cells = dynamic_constraint_dual_values[ i ];
  const auto & other_cells =
   other_solution->dynamic_constraint_dual_values[ i ];
  if( cells.size() != other_cells.size() )
   throw( std::logic_error( "RowConstraintSolution::sum: dynamic constraint "
			    "group " + std::to_string( i ) + " has " +
			    std::to_string( cells.size() ) + " cells here "
			    "and " + std::to_string( other_cells.size() ) +
			    " in the given Solution" ) );
  for( std::size_t c = 0 ; c < cells.size() ; ++c ) {
   if( cells[ c ].size() < other_cells[ c ].size() )
    cells[ c ].resize( other_cells[ c ].size() , 0 );
   for( std::size_t k = 0 ; k < other_cells[ c ].size() ; ++k )
    cells[ c ][ k ] += multiplier * other_cells[ c ][ k ];
   }
  }

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

 f_direction = solution->f_direction;  // scaling a direction gives one


 this->delete_vectors();

 static_constraint_dual_values = solution->static_constraint_dual_values;
 for( auto & values : static_constraint_dual_values )
  for( auto & value : values )
   value *= factor;

 dynamic_constraint_dual_values = solution->dynamic_constraint_dual_values;
 for( auto & cells : dynamic_constraint_dual_values )
  for( auto & values : cells )
   for( auto & value : values )
    value *= factor;

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
