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
/*-------------------------------- FUNCTIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

/// calls f() on a group, returning false if it is not one of ColVariable

template< class F >
bool on_group( const std::unique_ptr< BaseGroup > & group , F f )
{
 if( ( ! group ) || ( group->get_element_type() != typeid( ColVariable ) ) )
  return( false );
 f( std::as_const( *group ) );
 return( true );
 }

}  // end( unnamed namespace )

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void ColVariableSolution::deserialize( const netCDF::NcGroup & group )
{
 // the inverse of serialize(): what is not there is read as nothing, so a
 // Solution that holds no Variable of one kind deserializes into an empty
 // one rather than into an error
 static_variable_values.clear();
 dynamic_variable_values.clear();
 nested_solutions.clear();

 ::deserialize< double >( group , "StaticValues" , "StaticValuesStart" ,
			  static_variable_values );

 std::vector< std::vector< double > > cells;
 if( ::deserialize< double >( group , "DynamicValues" ,
			      "DynamicValuesStart" , cells ) ) {
  auto ncVar = group.getVar( "DynamicCellsStart" );
  if( ncVar.isNull() )
   throw( std::invalid_argument( "ColVariableSolution::deserialize: "
				 "DynamicValues without DynamicCellsStart" ) );

  std::vector< int > group_start( ncVar.getDim( 0 ).getSize() );
  ncVar.getVar( group_start.data() );

  dynamic_variable_values.resize( group_start.size() );
  for( std::size_t g = 0 ; g < group_start.size() ; ++g ) {
   const auto begin = std::size_t( group_start[ g ] );
   const auto end = ( g + 1 < group_start.size() )
                    ? std::size_t( group_start[ g + 1 ] ) : cells.size();
   if( ( begin > end ) || ( end > cells.size() ) )
    throw( std::invalid_argument( "ColVariableSolution::deserialize: "
				  "wrong indices in DynamicCellsStart" ) );
   dynamic_variable_values[ g ].assign( cells.begin() + begin ,
					cells.begin() + end );
   }
  }

 // the Solution of the nested Blocks, each in a group of its own
 std::size_t n = 0;
 while( ! group.getGroup( "NestedSolution_" + std::to_string( n ) ).isNull() )
  ++n;

 nested_solutions.resize( n );
 for( std::size_t i = 0 ; i < n ; ++i )
  nested_solutions[ i ].deserialize(
		   group.getGroup( "NestedSolution_" + std::to_string( i ) ) );
 }

/*--------------------------------------------------------------------------*/

ColVariableSolution::~ColVariableSolution() {
 delete_vectors();
 }

/*--------------------------------------------------------------------------*/

void ColVariableSolution::delete_vectors() {
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
    ( *nested_solution_it ).initialize( *nested_block_it , read );
}

/*--------------------------------------------------------------------------*/

void ColVariableSolution::initialize_static_variable_values
( const Block * const block , bool read ) {

 const auto & groups = block->get_static_variable_groups();
 static_variable_values.resize( groups.size() );

 for( Vec_Group::size_type i = 0 ; i < static_variable_values.size() ; ++i ) {
  auto & values = static_variable_values[ i ];
  if( ! on_group( groups[ i ] ,
                 [ & values , read ]( const BaseGroup & group ) {
       values.assign( group.get_num_elements() , 0 );
       if( read ) {
	auto value = values.data();
	group.for_each_as< ColVariable >( [ & value ]( ColVariable & var ) {
	  *(value++) = var.get_value(); } );
	}
       } ) )
   throw( std::logic_error( std::string( "ColVariableSolution::initialize: "
					 "invalid variable group: " ) +
			    ( groups[ i ] ? groups[ i ]->get_element_type().name()
                           : "an empty group" ) ) );
  }
 }

/*--------------------------------------------------------------------------*/

void ColVariableSolution::initialize_dynamic_variable_values
( const Block * const block , bool read ) {

 const auto & groups = block->get_dynamic_variable_groups();
 dynamic_variable_values.resize( groups.size() );

 for( Vec_Group::size_type i = 0 ; i < dynamic_variable_values.size() ; ++i ) {
  auto & values = dynamic_variable_values[ i ];
  if( ! on_group( groups[ i ] ,
                 [ & values , read ]( const BaseGroup & group ) {
       values.assign( group.get_num_cells() , {} );
       if( read )
	group.for_each_cell_as< ColVariable >(
	 [ & values ]( BaseGroup::Index c , auto & cell ) {
	  values[ c ].resize( cell.size() );
	  auto value = values[ c ].data();
	  for( auto & item : cell )
	   *(value++) = group_element( item ).get_value();
	  } );
       } ) )
   throw( std::logic_error( std::string( "ColVariableSolution::"
					 "initialize_dynamic_variable_values: "
					 "invalid variable group: " ) +
			    ( groups[ i ] ? groups[ i ]->get_element_type().name()
                           : "an empty group" ) ) );
  }
 }

/*--------------------------------------------------------------------------*/

void ColVariableSolution::apply_static( const Block * const block ,
                                        bool read ) {

 const auto & groups = block->get_static_variable_groups();

  if( groups.size() != static_variable_values.size() )
    throw( std::logic_error
     ( std::string
        ( "ColVariableSolution::apply_static: "
          "number of static Variable groups of this ColVariableSolution (" ) +
       std::to_string( static_variable_values.size() ) +
       std::string( ") is different from that of the Block (" ) +
       std::to_string( groups.size() ) + std::string( ")" ) ) );

 for( Vec_Group::size_type i = 0 ; i < static_variable_values.size() ; ++i ) {
  auto & values = static_variable_values[ i ];
  bool conforming = true;
  if( ! on_group( groups[ i ] ,
                 [ & ]( const BaseGroup & group ) {
       if( group.get_num_elements() != values.size() ) {
	conforming = false;
	return;
	}
       auto value = values.data();
       if( read )
	group.for_each_as< ColVariable >( [ & value ]( ColVariable & var ) {
	  *(value++) = var.get_value(); } );
       else
	group.for_each_as< ColVariable >( [ & value ]( ColVariable & var ) {
	  var.set_value( *(value++) ); } );
       } ) )
   throw( std::logic_error( "ColVariableSolution::apply_static: "
			    "invalid types" ) );
  if( ! conforming )
   throw( std::logic_error( "ColVariableSolution::apply_static: the size of "
			    "static Variable group " + std::to_string( i ) +
			    " is different from that of the Block" ) );
  }
 }

/*--------------------------------------------------------------------------*/

void ColVariableSolution::apply_dynamic( const Block * const block ,
                                         bool read ) {

 const auto & groups = block->get_dynamic_variable_groups();

  if( groups.size() != dynamic_variable_values.size() )
    throw( std::logic_error
     ( std::string
        ( "ColVariableSolution::apply_dynamic: "
          "number of dynamic Variable groups of this ColVariableSolution (" ) +
       std::to_string( dynamic_variable_values.size() ) +
       std::string( ") is different from that of the Block (" ) +
       std::to_string( groups.size() ) + std::string( ")" ) ) );

 for( Vec_Group::size_type i = 0 ; i < dynamic_variable_values.size() ; ++i ) {
  auto & values = dynamic_variable_values[ i ];
  bool conforming = true;
  if( ! on_group( groups[ i ] ,
                 [ & ]( const BaseGroup & group ) {
       if( group.get_num_cells() != values.size() ) {
	conforming = false;
	return;
	}
       group.for_each_cell_as< ColVariable >(
	[ & values , read ]( BaseGroup::Index c , auto & cell ) {
	 auto & cell_values = values[ c ];
	 if( read ) {
	  // the values of a cell may outlive the Variables they were read
	  // from, hence the vector only grows
	  if( cell_values.size() < cell.size() )
	   cell_values.resize( cell.size() );
	  auto value = cell_values.data();
	  for( auto & item : cell )
	   *(value++) = group_element( item ).get_value();
	  }
	 else {
	  // Variables beyond the stored values get their default value
	  auto value = cell_values.begin();
	  for( auto & item : cell )
	   if( value != cell_values.end() )
	    group_element( item ).set_value( *(value++) );
	   else
	    group_element( item ).set_to_default_value();
	  }
	 } );
       } ) )
   throw( std::logic_error( "ColVariableSolution::apply_dynamic: "
			    "invalid types" ) );
  if( ! conforming )
   throw( std::logic_error( "ColVariableSolution::apply_dynamic: the number "
			    "of cells of dynamic Variable group " +
			    std::to_string( i ) +
			    " is different from that of the Block" ) );
  }
 }

/*--------------------------------------------------------------------------*/

bool ColVariableSolution::drop_dynamic_values
( const Block * const block , const void * cell ,
  const Block::Subset & positions , std::vector< double > & dropped )
{
 // look for the cell among the groups of dynamic Variable of this Block

 const auto & groups = block->get_dynamic_variable_groups();

 if( groups.size() == dynamic_variable_values.size() )
  for( Block::Index i = 0 ; i < groups.size() ; ++i ) {
   auto & values = dynamic_variable_values[ i ];
   bool found = false;

   on_group( groups[ i ] , [ & ]( const BaseGroup & group ) {
     group.for_each_cell_as< ColVariable >(
      [ & ]( BaseGroup::Index c , auto & cll ) {
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

 }  // end( ColVariableSolution::drop_dynamic_values )

/*--------------------------------------------------------------------------*/

void ColVariableSolution::read( const Block * const block ) {

  // what the Variable hold, a solution or a direction, is what this
  // Solution holds from now on
  f_direction = block->is_direction();

  if( static_variable_values.size() != block->get_static_variable_groups().size() ||
      dynamic_variable_values.size() != block->get_dynamic_variable_groups().size() ||
      nested_solutions.size() != block->get_nested_Blocks().size() ) {
    // This ColVariableSolution does not have the same structure as
    // that of the Variables of the Block: initialize it and read the
    // solution from the Block.
    this->initialize( block , true );
    return;
  }

  try {
    apply_static( block , true );
    apply_dynamic( block , true );
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
    ( *sub_solution_iterator++ ).read( sub_block );
  }
}

/*--------------------------------------------------------------------------*/

void ColVariableSolution::write( Block * const block ) {

  apply_static( block , false );
  apply_dynamic( block , false );

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
    ( *sub_solution_iterator++ ).write( sub_block );
  }

}

/*--------------------------------------------------------------------------*/

void ColVariableSolution::serialize( netCDF::NcGroup & group ) const
{
 // always call the method of the base class first
 Solution::serialize( group );

 /* The values of the static Variable are a matrix with rows of different
  * length, one row per group, and go in the two netCDF variables such a
  * matrix is written in [see serialize() in SMSTypedefs.h]: StaticValues
  * holds them all in a row, StaticValuesStart says where each group begins.
  *
  * Those of the dynamic Variable have one level more, since a group of them
  * is a grid of cells and each cell holds a collection of its own size: the
  * cells of all the groups are written as one matrix with rows of different
  * length, DynamicValues and DynamicValuesStart, and DynamicCellsStart says
  * which of those cells the groups begin at. */

 if( ! static_variable_values.empty() )
  ::serialize< double >( group , "StaticValues" , netCDF::NcDouble() ,
			 "StaticValuesStart" , static_variable_values );

 if( ! dynamic_variable_values.empty() ) {
  std::vector< std::vector< double > > cells;
  std::vector< int > group_start;
  group_start.reserve( dynamic_variable_values.size() );

  for( const auto & grp : dynamic_variable_values ) {
   group_start.push_back( int( cells.size() ) );
   for( const auto & cell : grp )
    cells.push_back( cell );
   }

  ::serialize< double >( group , "DynamicValues" , netCDF::NcDouble() ,
			 "DynamicValuesStart" , cells );

  auto ncDim = group.addDim( "NumberDynamicGroups" , group_start.size() );
  auto ncVar = group.addVar( "DynamicCellsStart" , netCDF::NcInt() , ncDim );
  ncVar.putVar( group_start.data() );
  }

 // the Solution of each nested Block goes in a group of its own, numbered
 // as the nested Block it belongs to
 if( ! nested_solutions.empty() ) {
  group.addDim( "NumberNestedSolutions" , nested_solutions.size() );
  for( std::size_t i = 0 ; i < nested_solutions.size() ; ++i ) {
   auto sg = group.addGroup( "NestedSolution_" + std::to_string( i ) );
   nested_solutions[ i ].serialize( sg );
   }
  }
 }

/*--------------------------------------------------------------------------*/

void ColVariableSolution::sum( const Solution * solution, double multiplier ) {

  auto other_solution = dynamic_cast< const ColVariableSolution * >( solution );

  if( ! other_solution )
    throw( std::invalid_argument
     ( "ColVariableSolution::sum: "
       "given Solution must be a ColVariableSolution" ) );

  // the sum is a direction only if every Solution in it is one
  f_direction = f_direction && other_solution->f_direction;

  if( empty() ) {
   scale( other_solution , multiplier );
   return;
   }

  if( ( static_variable_values.size() !=
	other_solution->static_variable_values.size() ) ||
      ( dynamic_variable_values.size() !=
	other_solution->dynamic_variable_values.size() ) )

    throw( std::logic_error
     ( std::string( "ColVariableSolution::sum: "
                    "number of variable groups of this Solution (" ) +
       std::to_string( static_variable_values.size() ) + "+" +
       std::to_string( dynamic_variable_values.size() ) +
       std::string( ") is different from the "
                    "number of variable groups (" ) +
       std::to_string( other_solution->static_variable_values.size() ) + "+" +
       std::to_string( other_solution->dynamic_variable_values.size() ) +
       std::string( ") of the given Solution" ) ) );

  // Sum the values of the static Variables

 for( Vec_Group::size_type i = 0 ; i < static_variable_values.size() ; ++i ) {
  auto & values = static_variable_values[ i ];
  const auto & other_values = other_solution->static_variable_values[ i ];
  if( values.size() != other_values.size() )
   throw( std::logic_error( "ColVariableSolution::sum: static variable group "
			    + std::to_string( i ) + " has " +
			    std::to_string( values.size() ) + " values here "
			    "and " + std::to_string( other_values.size() ) +
			    " in the given Solution" ) );
  for( std::size_t k = 0 ; k < values.size() ; ++k )
   values[ k ] += multiplier * other_values[ k ];
  }

  // Sum the values of the dynamic Variables: a value missing on either side
  // counts as zero

 for( Vec_Group::size_type i = 0 ; i < dynamic_variable_values.size() ; ++i ) {
  auto & cells = dynamic_variable_values[ i ];
  const auto & other_cells = other_solution->dynamic_variable_values[ i ];
  if( cells.size() != other_cells.size() )
   throw( std::logic_error( "ColVariableSolution::sum: dynamic variable group "
			    + std::to_string( i ) + " has " +
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
    ( *i1 ).sum( &( *i2 ) , multiplier );
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

 f_direction = solution->f_direction;  // scaling a direction gives one

 static_variable_values = solution->static_variable_values;
 for( auto & values : static_variable_values )
  for( auto & value : values )
   value *= factor;

 dynamic_variable_values = solution->dynamic_variable_values;
 for( auto & cells : dynamic_variable_values )
  for( auto & values : cells )
   for( auto & value : values )
    value *= factor;

 this->nested_solutions.resize( solution->nested_solutions.size() );

 // Scale the solutions of the nested Blocks

 auto i1 = this->nested_solutions.begin();
 auto i2 = solution->nested_solutions.begin();

 for( ; i1 != this->nested_solutions.end() ; ++i1 , ++i2 )
  ( *i1 ).scale( &( *i2 ) , factor );
 }

/*--------------------------------------------------------------------------*/
/*----------------- End File ColVariableSolution.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
