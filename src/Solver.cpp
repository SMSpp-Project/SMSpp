/*--------------------------------------------------------------------------*/
/*---------------------------- File Solver.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the Solver class. It also registers FakeSolver in the
 * Solver factory.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
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

#include "Block.h"
#include "Objective.h"
#include "Solver.h"
#include "AbstractPath.h"

#include "FakeSolver.h"

#include "UpdateSolver.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/
/* Used in reset_event_handler(). For some unfathomable reason need be
 * defined as a function rather than as a Lambda. */

int do_nothing( void ) { return( Solver::eContinue ); };

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register FakeSolver to the Solver factory

SMSpp_insert_in_factory_cpp_0( FakeSolver );

/*--------------------------------------------------------------------------*/
// register UpdateSolver to the Solver factory

SMSpp_insert_in_factory_cpp_0( UpdateSolver );

/*--------------------------------------------------------------------------*/
/*---------------------------- METHODS of Solver ---------------------------*/
/*--------------------------------------------------------------------------*/

void Solver::set_Block( Block * block )
{
 if( f_Block == block )  // registering to the same Block
  return;                // cowardly and silently return

 if( f_Block )           // was attached to some Block
  v_mod.clear();         // any pending Modification was about the old
                         // Block, so it is now irrelevant

 f_Block = block;        // this is the new Block now
 }

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS FOR EVENTS HANDLING -----------------------*/
/*--------------------------------------------------------------------------*/

void Solver::reset_event_handler( int type , EventID id )
{
 if( type >= max_event_number() )
  throw( std::invalid_argument( "unsupported event type " +
                                 std::to_string( type ) ) );

 if( id >= v_events[ type ].size() )
  throw( std::invalid_argument( "incorrect event id " + std::to_string( id )
				+ " for type " + std::to_string( type ) ) );

 if( id == v_events[ type ].size() - 1 ) {
  // if the event is the last of its type, shorten the vector; moreover, if
  // any pf the previous events is a do_nothing, keep shortening
  do
   v_events[ type ].pop_back();
  while( ( ! v_events[ type ].empty() ) &&
         ( *( v_events[ type ].back().target < int( * )() > ( ) ) ==
         do_nothing ) );
  }
 else
  // the event is not the last of its type: replace it with a do_nothing to
  // avoid messing up with the id-s, which are positions in the vector
  v_events[ type ][ id ] = do_nothing;

 }  // end( Solver::reset_event_handler )

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/

Solver::OFValue Solver::get_var_value( void ) {
 return( f_Block ? f_Block->get_objective_sense() == Objective::eMin ?
	           get_ub() : get_lb()
                 : Objective::eMin );
 }


/*--------------------------------------------------------------------------*/
/*------------------------ METHODS FOR WARM STARTS -------------------------*/
/*--------------------------------------------------------------------------*/

std::vector< AbstractPath > Solver::deserialize( const std::string & filename )
{
 int idx = 0;
 std::string fn = filename;
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
  return( Solver::deserialize( f , idx ) );
  }
 catch( netCDF::exceptions::NcException & e ) {
  std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
  }
 catch( std::exception & e ) {
  std::cerr << "error " << e.what() << " in deserialize" << std::endl;
  }
 catch( ... ) {
  std::cerr << "unknown error in deserialize" << std::endl;
  }

 return( std::vector< AbstractPath >{} );

 }  // end( Solver::deserialize( const std::string ) )

/*--------------------------------------------------------------------------*/

std::vector< AbstractPath > Solver::deserialize( const netCDF::NcFile & f , int idx )
{
 try {
  auto gtype = f.getAtt( "SMS++_file_type" );
  if( gtype.isNull() )
   return( std::vector< AbstractPath >{} );

  int type;
  gtype.getValues( & type );

  if( type != eWarmStartFile ) //// CHECK
   return( std::vector< AbstractPath >{} );

  auto cg = f.getGroup( "WarmStart_" + std::to_string( idx ) );

  return( deserialize( cg ) );
  }
 catch( netCDF::exceptions::NcException & e ) {
  std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
  }
 catch( std::exception & e ) {
  std::cerr << "error " << e.what() << " in deserialize" << std::endl;
  }
 catch( ... ) {
  std::cerr << "unknown error in deserialize" << std::endl;
  }

 return( std::vector< AbstractPath >{} );

 }  // end( Solver::deserialize( netCDF::NcFile ) )

/*--------------------------------------------------------------------------*/

std::vector< AbstractPath > Solver::deserialize( const netCDF::NcGroup & group )
{
 try {
  if( group.isNull() )
   return( std::vector< AbstractPath >{} );

  // Retrieve number of Variable in the Group
  int nvars;
  auto gNVar = group.getDim( "NumberVariableGroups" );
  if( gNVar.isNull() )
   nvars = 1;
  else
   nvars = gNVar.getSize();

  // Initialize empty vector
  std::vector< AbstractPath > output_struct( nvars );

  for( int idx = 0; idx < nvars; ++idx ){
   auto vg = group.getGroup( "VariableGroup_" + std::to_string( idx ) );

   AbstractPath varpath;
   varpath.deserialize( vg );

   // Append it to the output struct
   output_struct[ idx ] = varpath;

   return( output_struct );
   }
  }
 catch( netCDF::exceptions::NcException & e ) {
  std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
  }
 catch( std::exception & e ) {
  std::cerr << "error " << e.what() << " in deserialize" << std::endl;
  }
 catch( ... ) {
  std::cerr << "unknown error in deserialize" << std::endl;
  }

 return( std::vector< AbstractPath >{} );

 }  // end( Solver::new_WarmStart( netCDF::NcGroup ) )

/*----------------------------------------------------------------------------









/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/

Solver::SolverFactoryMap & Solver::f_factory( void ) {
 static SolverFactoryMap s_factory;
 return( s_factory );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File Solver.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
