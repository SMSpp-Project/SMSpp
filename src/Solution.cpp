/*--------------------------------------------------------------------------*/
/*--------------------------- File Solution.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the Solution class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Solution.h"

#include "ColVariable.h"
#include "FRowConstraint.h"
#include "OneVarConstraint.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register Solution to the Solution factory
SMSpp_insert_in_factory_cpp_0( Solution );

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*---------------------------- METHODS of Solution -------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------- LOCAL FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

/// tells which elements of type C a Modification says have been removed
/** Writes in cell the address of the cell of the group of dynamic elements
 * of type C they were removed from and in positions the positions they had
 * in it, an empty positions meaning the whole cell. Returns false if the
 * Modification is not one that removes elements of type C saying which. */

template< class C >
bool removed_of( const Modification & mod , const void * & cell ,
		 Block::Subset & positions )
{
 if( const auto tmod =
     dynamic_cast< const BlockModRmvRngd< C > * >( & mod ) ) {
  cell = static_cast< const void * >( & tmod->whc() );
  const auto & rng = tmod->range();
  positions.clear();
  for( Block::Index i = rng.first ; i < rng.second ; ++i )
   positions.push_back( i );
  return( true );
  }

 if( const auto tmod =
     dynamic_cast< const BlockModRmvSbst< C > * >( & mod ) ) {
  cell = static_cast< const void * >( & tmod->whc() );
  positions = tmod->subset();
  return( true );
  }

 return( false );
 }

/*--------------------------------------------------------------------------*/
/// as removed_of(), for the dynamic Constraint of the core

bool removed_rows( const Modification & mod , const void * & cell ,
		   Block::Subset & positions )
{
 return( removed_of< FRowConstraint >( mod , cell , positions ) ||
	 removed_of< BoxConstraint >( mod , cell , positions ) ||
	 removed_of< LB0Constraint >( mod , cell , positions ) ||
	 removed_of< UB0Constraint >( mod , cell , positions ) ||
	 removed_of< LBConstraint >( mod , cell , positions ) ||
	 removed_of< UBConstraint >( mod , cell , positions ) ||
	 removed_of< NNConstraint >( mod , cell , positions ) ||
	 removed_of< NPConstraint >( mod , cell , positions ) ||
	 removed_of< ZOConstraint >( mod , cell , positions ) );
 }

}  // end( unnamed namespace )

/*--------------------------------------------------------------------------*/
/*--------------------------- METHODS OF Solution --------------------------*/
/*--------------------------------------------------------------------------*/

Solution::adapt_result Solution::adapt( const Block * const block ,
					const Modification & mod ,
					std::vector< double > & dropped )
{
 // a GroupModification, one sub-Modification at a time
 if( const auto gmod = dynamic_cast< const GroupModification * >( & mod ) ) {
  adapt_result res = kUnchanged;
  for( const auto & sub : gmod->sub_Modifications() ) {
   std::vector< double > sub_dropped;
   const auto r = adapt( block , *sub , sub_dropped );
   if( r == kInvalid )
    return( kInvalid );
   if( r == kAdapted ) {
    res = kAdapted;
    dropped.insert( dropped.end() , sub_dropped.begin() ,
		    sub_dropped.end() );
    }
   }
  return( res );
  }

 // after a NModification nothing of the Solution can be trusted
 if( dynamic_cast< const NModification * >( & mod ) )
  return( kInvalid );

 const auto c = mod.changes();
 const auto holds = adapts();

 // a physical Modification that this Solution reads
 if( Modification::is_physical( c ) ) {
  if( ! Modification::is_physical( holds ) )
   return( kUnchanged );
  return( drop_physical_values( block , & mod , dropped ) ? kAdapted
	                                                    : kInvalid );
  }

 // removed dynamic Variable or Constraint that this Solution holds values of
 const void * cell = nullptr;
 Block::Subset positions;
 if( ( ( holds & Modification::eModVarSet ) &&
       removed_of< ColVariable >( mod , cell , positions ) ) ||
     ( ( holds & Modification::eModCnsSet ) &&
       removed_rows( mod , cell , positions ) ) )
  return( drop_dynamic_values( block , cell , positions , dropped ) ?
	  kAdapted : kInvalid );

 return( kUnchanged );

 }  // end( Solution::adapt )

/*--------------------------------------------------------------------------*/

Solution * Solution::deserialize( const std::string & filename )
{
 int idx = 0;
 std::string fn = get_filename_prefix() + filename;
 if( fn.back() == ']' ) {
  auto pos = fn.find_last_of( '[' );
  if( pos != std::string::npos ) {
   try {
    idx = std::stoi( fn.substr( pos + 1 ) );
    fn.erase( pos );
    }
   catch( ... ) { idx = 0; }
   }
  }
 try {
  netCDF::NcFile f( fn.c_str() , netCDF::NcFile::read );
  return( Solution::deserialize( f , idx ) );
  }
 catch( netCDF::exceptions::NcException & e ) {
  std::cerr << "netCDF error " << e.what() << " in Solution::deserialize"
	    << std::endl;
  }
 catch( std::exception & e ) {
  std::cerr << "error " << e.what() << " in Solution::deserialize"
	    << std::endl;
  }
 catch( ... ) {
  std::cerr << "unknown error in Solution::deserialize" << std::endl;
  }

 return( nullptr );

 }  // end( Solution::deserialize( const std::string ) )

/*--------------------------------------------------------------------------*/

Solution * Solution::deserialize( const netCDF::NcFile & f , int idx )
{
 try {
  auto gtype = f.getAtt( "SMS++_file_type" );
  if( gtype.isNull() )
   return( nullptr );

  int type;
  gtype.getValues( & type );

  if( type != eSolutionFile )
   return( nullptr );

  auto cg = f.getGroup( "Solution_" + std::to_string( idx ) );

  return( new_Solution( cg ) );
  }
 catch( netCDF::exceptions::NcException & e ) {
  std::cerr << "netCDF error " << e.what() << " in Solution::deserialize"
	    << std::endl;
  }
 catch( std::exception & e ) {
  std::cerr << "error " << e.what() << " in Solution::deserialize"
	    << std::endl;
  }
 catch( ... ) {
  std::cerr << "unknown error in Solution::deserialize" << std::endl;
  }

 return( nullptr );

 }  // end( Solution::deserialize( netCDF::NcFile ) )

/*--------------------------------------------------------------------------*/

Solution * Solution::new_Solution( const netCDF::NcGroup & group )
{
 try {
  if( group.isNull() )
   return( nullptr );

  std::string tmp;
  auto gtype = group.getAtt( "type" );
  if( gtype.isNull() ) {
   auto gfile = group.getAtt( "filename" );
   if( gfile.isNull() )
    return( nullptr );

   gfile.getValues( tmp );

   return( deserialize( tmp ) );
   }

  gtype.getValues( tmp );
  auto result = new_Solution( tmp );
  result->deserialize( group );

  // whether it holds a direction is written by Solution::serialize(), hence
  // it is read here rather than in each :Solution [see is_direction()]
  auto gdir = group.getAtt( "direction" );
  if( ! gdir.isNull() ) {
   int dir = 0;
   gdir.getValues( & dir );
   result->is_direction( dir != 0 );
   }

  return( result );
  }
 catch( netCDF::exceptions::NcException & e ) {
  std::cerr << "netCDF error " << e.what() << " in Solution::new_Solution"
	    << std::endl;
  }
 catch( std::exception & e ) {
  std::cerr << "error " << e.what() << " in Solution::new_Solution"
	    << std::endl;
  }
 catch( ... ) {
  std::cerr << "unknown error in Solution::new_Solution" << std::endl;
  }

 return( nullptr );

 }  // end( Solution::new_Solution( netCDF::NcGroup ) )

/*--------------------------------------------------------------------------*/

std::string & solution_filename_prefix( void )
{
 static std::string prefix;
 return( prefix );
 }

void Solution::set_filename_prefix( std::string && prefix )
{
 solution_filename_prefix() = std::move( prefix );
 }

const std::string & Solution::get_filename_prefix( void )
{
 return( solution_filename_prefix() );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- End File Solution.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
