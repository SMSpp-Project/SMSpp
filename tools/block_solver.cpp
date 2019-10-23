#include <iostream>
#include <getopt.h>

#include <Block.h>
#include <FRealObjective.h>

using namespace SMSpp_di_unipi_it;

std::string filename{};

void print_help() {
 // http://docopt.org
 std::cout << "Usage: block_solver <file>" << std::endl
           << std::endl
           << "-h, --help  Print this help." << std::endl;
}

void process_args( int argc, char ** argv ) {

 if( argc < 2 ) {
  print_help();
  exit( 1 );
 }

 const char * const short_opts = "h";
 const option long_opts[] = {
  { "help",  no_argument, nullptr, 'h' },
  { nullptr, no_argument, nullptr, 0 }
 };

 // Options
 while( true ) {
  const auto opt = getopt_long( argc, argv, short_opts, long_opts, nullptr );

  if( -1 == opt ) {
   break;
  }

  switch( opt ) {
   case 'h': // -h or --help
    print_help();
    exit( 0 );
   case '?': // Unrecognized option[
   default:
    print_help();
    exit( 1 );
  }
 }

 // Last argument
 if( optind < argc ) {
  filename = std::string( argv[ optind ] );
 } else {
  print_help();
  exit( 1 );
 }
}

int main( int argc, char ** argv ) {

 process_args( argc, argv );

 netCDF::NcFile f;
 try {
  f.open( filename, netCDF::NcFile::read );
 } catch( netCDF::exceptions::NcException & e ) {
  std::cerr << "Cannot open nc4 file " << filename << std::endl;
  exit( 1 );
 }

 netCDF::NcGroupAtt gtype = f.getAtt( "SMS++_file_type" );
 if( gtype.isNull() ) {
  std::cerr << filename << " is not an SMS++ nc4 file" << std::endl;
  exit( 1 );
 }

 int type;
 gtype.getValues( &type );

 if( type != eProbFile ) {
  std::cerr << filename << " is not an SMS++ problem file" << std::endl;
  exit( 1 );
 }

 std::multimap< std::string, netCDF::NcGroup > problems = f.getGroups();
 // for each problem descriptor:
 for( auto & p : problems ) {

  // Deserialize block
  auto gb = p.second.getGroup( "Block" );
  auto block = Block::new_Block( gb );

  // Generate abstract representation
  int tmp = 15;
  SimpleConfiguration< int > myconfig( tmp );

  block->generate_abstract_variables( &myconfig );
  block->generate_abstract_constraints( nullptr );
  block->generate_objective( nullptr );

  // Configure block
  auto bgc = p.second.getGroup( "BlockConfig" );
  auto b_config = static_cast<BlockConfig *>(BlockConfig::new_Configuration( bgc ));
  block->set_BlockConfig( b_config );

  // Configure solver
  auto bgs = p.second.getGroup( "BlockSolver" );
  auto b_solver = static_cast<BlockSolverConfig *>(BlockSolverConfig::new_Configuration( bgs ));
  block->set_SolverConfig( b_solver );

  // Solve
  auto solver = block->get_registered_solvers().front();
  auto status = solver->compute();
  auto ub = solver->get_ub();
  auto obj = dynamic_cast<FRealObjective *>(block->get_objective());
  auto obj_f = obj->get_function();
  auto obj_val = obj_f->get_value();

  std::cout << "Problem: " << p.first << std::endl;
  std::cout << "Status = " << status << std::endl;
  std::cout << "Upper bound = " << ub << std::endl;
  std::cout << "Function value =  " << obj_val << std::endl;
 }
 return 0;
}
