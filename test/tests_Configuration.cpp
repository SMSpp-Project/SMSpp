/*--------------------------------------------------------------------------*/
/*---------------------- File tests_Configuration.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Unit tests for the Configuration of the core: SimpleConfiguration,
 * ComputeConfig, BlockConfig and OCRBlockConfig, BlockSolverConfig.
 *
 * A Configuration is read from text, written and read back in netCDF and
 * cloned, and each of these has to give back what was put in; the text
 * format is also taken through the ways it has to redirect to a file ("*"
 * followed by a filename, with the executable-wide prefix and the "[idx]"
 * position in a netCDF file) and through the cascade override "*file +" of
 * a ComputeConfig, with and without the slot of its extra Configuration. The
 * Configuration of a Block are got from a Block and applied to another, in
 * both modes, and cleared; the BlockSolverConfig register their Solver, and
 * the cleared ones remove all and only these.
 *
 * A second set of tests, whose files go in a directory of their own that
 * is removed at the end, compares what is read with what was written field
 * by field: the pairs of numbers and the nested SimpleConfiguration, the
 * meta-configuration with its "*file" and "*file +" entries, whose extra
 * slot may be left out before the next key, the ComputeConfig applied to a
 * ThinComputeInterface, the ten slots of a BlockConfig and what its print()
 * writes, the :BlockConfig with the handlers of the Objective, of the
 * Constraint and of the sub-Block, and the RBlockSolverConfig.
 *
 * The checks that are made with expect() rather than assert() are those
 * whose failure leaves the test able to go on: each failed one is printed,
 * and main() returns non-zero if any has failed, so that a run shows all of
 * them at once.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractBlock.h"
#include "BlockSolverConfig.h"
#include "FakeSolver.h"
#include "RBlockConfig.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using PairConf = SimpleConfiguration< std::pair< Configuration * ,
						 Configuration * > >;

using MapConf = SimpleConfiguration< std::map< std::string ,
					       Configuration * > >;

using VecConf = SimpleConfiguration< std::vector< Configuration * > >;

using SC_int = SimpleConfiguration< int >;
using SC_dbl = SimpleConfiguration< double >;
using SC_map =
 SimpleConfiguration< std::map< std::string , Configuration * > >;

/*--------------------------------------------------------------------------*/
/*-------------------------- STATIC MEMBERS --------------------------------*/
/*--------------------------------------------------------------------------*/
// the core does not register SimpleConfiguration< std::string >, whoever
// uses it does [see SimpleConfiguration]

SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration< std::string > );

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

static int n_failed = 0;  ///< the number of expect() that have failed

/// reports a failed check without stopping the test

static void expect( bool ok , const std::string & what )
{
 if( ok )
  return;
 ++n_failed;
 std::cout << "FAILED: " << what << std::endl;
 }

/*--------------------------------------------------------------------------*/
/// writes the text in the file with the given name

static void write_file( const std::string & name , const std::string & text )
{
 std::ofstream f( name );
 f << text;
 }

/*--------------------------------------------------------------------------*/
/// the netCDF file every round trip goes through

static const std::string nc_file = "tests_Configuration.nc4";

/* Writes the Configuration as the only one of an eConfigFile and reads it
 * back with Configuration::deserialize( filename ); the caller owns what
 * comes back, which is nullptr if the reading fails. */

/* The serialize() that takes a filename is called through the base class,
 * since the :Configuration that define their own serialize() hide it. */

static void write_nc( const Configuration & c , const std::string & fn ,
		      int type = eConfigFile , bool replace = true )
{
 c.serialize( fn , type , replace );
 }

/*--------------------------------------------------------------------------*/

static Configuration * nc_round_trip( const Configuration & c )
{
 write_nc( c , nc_file );
 return( Configuration::deserialize( nc_file ) );
 }

/*--------------------------------------------------------------------------*/
/// reads a Configuration out of the text with Configuration::deserialize()

static Configuration * from_text( const std::string & text )
{
 std::istringstream in( text );
 return( Configuration::deserialize( in ) );
 }

/*--------------------------------------------------------------------------*/
/* Loads the Configuration out of the text with the operator>>() of
 * Configuration, which is called through the base class: for a
 * SimpleConfiguration< T > the operator>>() that SMSTypedefs.h has for any
 * container C< T > is a better match, and it does not compile. */

static void load_into( Configuration & c , const std::string & text )
{
 std::istringstream in( text );
 in >> c;
 }

/*--------------------------------------------------------------------------*/
/// what the Configuration prints

static std::string printed( const Configuration & c )
{
 std::ostringstream out;
 out << c;
 return( out.str() );
 }

/*--------------------------------------------------------------------------*/
/// the value of a Configuration that has to be a SimpleConfiguration< T >

template< class T >
static const T & value_of( const Configuration * c )
{
 auto sc = dynamic_cast< const SimpleConfiguration< T > * >( c );
 assert( sc );
 return( sc->f_value );
 }

/*--------------------------------------------------------------------------*/
/// true if the call throws an exception of type E

template< class E , class F >
static bool throws( F f )
{
 try {
  f();
  }
 catch( E & ) {
  return( true );
  }
 catch( ... ) {
  return( false );
  }
 return( false );
 }

/*--------------------------------------------------------------------------*/
/// true if the two ComputeConfig hold the same parameters and flags

static bool same( const ComputeConfig & a , const ComputeConfig & b )
{
 return( ( a.diff() == b.diff() ) && ( a.relax() == b.relax() ) &&
	 ( a.int_pars == b.int_pars ) && ( a.dbl_pars == b.dbl_pars ) &&
	 ( a.str_pars == b.str_pars ) && ( a.vint_pars == b.vint_pars ) &&
	 ( a.vdbl_pars == b.vdbl_pars ) && ( a.vstr_pars == b.vstr_pars ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------- TESTS OF SimpleConfiguration -------------------------*/
/*--------------------------------------------------------------------------*/
/* A SimpleConfiguration< int > is read past comments, prints its value in a
 * form that reads back to the same one, goes through netCDF and clone(),
 * and a value that is not an int makes load() throw. */

static void test_simple_int( void )
{
 SimpleConfiguration< int > c;
 load_into( c , "# a comment\n  -42 # and another one\n" );
 assert( c.f_value == -42 );

 SimpleConfiguration< int > r;
 load_into( r , printed( c ) );
 assert( r.f_value == -42 );

 auto n = nc_round_trip( c );
 assert( n && ( value_of< int >( n ) == -42 ) );
 assert( n->classname() == "SimpleConfiguration<int>" );
 delete n;

 auto k = c.clone();
 assert( ( k != & c ) && ( k->f_value == -42 ) );
 delete k;

 SimpleConfiguration< int > bad;
 assert( throws< std::invalid_argument >( [ & ]() {
    load_into( bad , "notanumber" ); } ) );

 // the name of the class first, then the value
 auto t = from_text( "SimpleConfiguration<int> 7" );
 assert( t && ( value_of< int >( t ) == 7 ) );
 delete t;

 std::cout << "SimpleConfiguration< int >: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A SimpleConfiguration< double > keeps values that are exact in binary
 * through text, netCDF and clone(). */

static void test_simple_double( void )
{
 for( double v : { 3.25 , -0.5 , 0.0 , 1e+300 } ) {
  SimpleConfiguration< double > c( v );

  SimpleConfiguration< double > r;
  load_into( r , printed( c ) );
  assert( r.f_value == v );

  auto n = nc_round_trip( c );
  assert( n && ( value_of< double >( n ) == v ) );
  delete n;

  auto k = c.clone();
  assert( k->f_value == v );
  delete k;
  }

 std::cout << "SimpleConfiguration< double >: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A SimpleConfiguration< std::string >, registered in this file, reads one
 * word, prints it, and goes through netCDF and clone(). */

static void test_simple_string( void )
{
 SimpleConfiguration< std::string > c;
 load_into( c , "  # comment\n hello_world  trailing" );
 assert( c.f_value == "hello_world" );
 assert( printed( c ) == "hello_world" );

 auto n = nc_round_trip( c );
 assert( n && ( value_of< std::string >( n ) == "hello_world" ) );
 delete n;

 // the empty string is a value too, in netCDF
 SimpleConfiguration< std::string > e( std::string( "" ) );
 n = nc_round_trip( e );
 assert( n && value_of< std::string >( n ).empty() );
 delete n;

 auto k = c.clone();
 assert( k->f_value == "hello_world" );
 delete k;

 std::cout << "SimpleConfiguration< std::string >: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A SimpleConfiguration< std::vector< int > > reads the dense, the sparse
 * and the empty format, and a nonempty and an empty one go through netCDF
 * and clone(). */

static void test_simple_vector_int( void )
{
 using V = std::vector< int >;
 SimpleConfiguration< V > c;

 load_into( c , "3 # three of them\n 1 -2 3" );
 assert( ( c.f_value == V{ 1 , -2 , 3 } ) );

 // five elements, two not default: 7 in position 1 and 9 in position 3
 load_into( c , "-5 2  1 7  3 9" );
 assert( ( c.f_value == V{ 0 , 7 , 0 , 9 , 0 } ) );

 auto n = nc_round_trip( c );
 assert( n && ( value_of< V >( n ) == V{ 0 , 7 , 0 , 9 , 0 } ) );
 delete n;

 auto k = c.clone();
 assert( ( k->f_value == V{ 0 , 7 , 0 , 9 , 0 } ) );
 delete k;

 load_into( c , "0" );
 assert( c.f_value.empty() );

 n = nc_round_trip( c );
 assert( n && value_of< V >( n ).empty() );
 delete n;

 std::cout << "SimpleConfiguration< std::vector< int > >: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The same for a SimpleConfiguration< std::vector< double > >, whose
 * sparse format may have all its elements at the default. */

static void test_simple_vector_double( void )
{
 using V = std::vector< double >;
 SimpleConfiguration< V > c;

 load_into( c , "2 0.25 -1.5" );
 assert( ( c.f_value == V{ 0.25 , -1.5 } ) );

 auto n = nc_round_trip( c );
 assert( n && ( value_of< V >( n ) == V{ 0.25 , -1.5 } ) );
 delete n;

 load_into( c , "-3 0" );
 assert( ( c.f_value == V{ 0 , 0 , 0 } ) );

 auto k = c.clone();
 assert( ( k->f_value == V{ 0 , 0 , 0 } ) );
 delete k;

 std::cout << "SimpleConfiguration< std::vector< double > >: OK"
	   << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A pair of Configuration * is read with the "*" for nullptr, is cloned
 * deeply, clear()-s both halves, and goes through netCDF with a nullptr in
 * it. */

static void test_simple_pair_of_configurations( void )
{
 auto c = from_text( "SimpleConfiguration<std::pair<Configuration*,"
		     "Configuration*>>\n"
		     "  SimpleConfiguration<int> 3   # the first\n"
		     "  *                            # no second\n" );
 auto p = dynamic_cast< PairConf * >( c );
 assert( p );
 assert( p->f_value.first && ( ! p->f_value.second ) );
 assert( value_of< int >( p->f_value.first ) == 3 );

 auto k = p->clone();
 assert( k->f_value.first && ( k->f_value.first != p->f_value.first ) );
 assert( value_of< int >( k->f_value.first ) == 3 );
 assert( ! k->f_value.second );
 delete k;

 auto n = nc_round_trip( *p );
 auto np = dynamic_cast< PairConf * >( n );
 assert( np );
 assert( np->f_value.first && ( value_of< int >( np->f_value.first ) == 3 ) );
 assert( ! np->f_value.second );
 delete n;

 // a pair with two ComputeConfig: clear() goes to both
 PairConf two;
 auto cc1 = new ComputeConfig;
 cc1->set_par( "a" , 1 );
 auto cc2 = new ComputeConfig;
 cc2->set_par( "b" , 2.0 );
 two.f_value = { cc1 , cc2 };
 two.clear();
 assert( cc1->int_pars.empty() && cc2->dbl_pars.empty() );

 delete c;
 std::cout << "SimpleConfiguration< std::pair< Configuration * , "
	   << "Configuration * > >: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A map from strings to Configuration * is read with its count, with a
 * nullptr among the values, is cloned deeply and goes through netCDF; the
 * empty one is read out of a zero count. */

static void test_simple_map( void )
{
 auto c = from_text( "SimpleConfiguration<std::map<std::string,"
		     "Configuration*>>\n"
		     "3\n"
		     "alpha SimpleConfiguration<int> 1\n"
		     "beta  *\n"
		     "gamma SimpleConfiguration<double> 2.5\n" );
 auto m = dynamic_cast< MapConf * >( c );
 assert( m );
 assert( m->f_value.size() == 3 );
 assert( value_of< int >( m->f_value.at( "alpha" ) ) == 1 );
 assert( ! m->f_value.at( "beta" ) );
 assert( value_of< double >( m->f_value.at( "gamma" ) ) == 2.5 );

 auto k = m->clone();
 assert( k->f_value.size() == 3 );
 assert( k->f_value.at( "alpha" ) != m->f_value.at( "alpha" ) );
 assert( value_of< int >( k->f_value.at( "alpha" ) ) == 1 );
 delete k;

 // what is written in netCDF is what is read back, into a fresh one
 auto n = nc_round_trip( *m );
 auto nm = dynamic_cast< MapConf * >( n );
 expect( nm , "netCDF round trip of SimpleConfiguration< std::map< "
	 "std::string , Configuration * > > gives back a map" );
 if( nm ) {
  expect( nm->f_value.size() == 3 ,
	  "netCDF round trip of SimpleConfiguration< std::map< std::string "
	  ", Configuration * > >: 3 keys written, " +
	  std::to_string( nm->f_value.size() ) + " read back" );
  auto it = nm->f_value.find( "alpha" );
  expect( ( it != nm->f_value.end() ) && it->second &&
	  ( value_of< int >( it->second ) == 1 ) ,
	  "netCDF round trip of the map: \"alpha\" -> 1 read back" );
  it = nm->f_value.find( "beta" );
  expect( ( it != nm->f_value.end() ) && ( ! it->second ) ,
	  "netCDF round trip of the map: \"beta\" -> nullptr read back" );
  it = nm->f_value.find( "gamma" );
  expect( ( it != nm->f_value.end() ) && it->second &&
	  ( value_of< double >( it->second ) == 2.5 ) ,
	  "netCDF round trip of the map: \"gamma\" -> 2.5 read back" );
  }
 delete n;

 auto e = from_text( "SimpleConfiguration<std::map<std::string,"
		     "Configuration*>> 0" );
 assert( e && dynamic_cast< MapConf * >( e )->f_value.empty() );
 delete e;

 delete c;
 std::cout << "SimpleConfiguration< std::map< std::string , "
	   << "Configuration * > >: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A vector of Configuration * goes through netCDF, which needs the "type"
 * attribute every serialized Configuration has to start with [see
 * Configuration::serialize( netCDF::NcGroup )]. */

static void test_simple_vector_of_configurations( void )
{
 VecConf v;
 v.f_value = { new SimpleConfiguration< int >( 4 ) ,
	       new SimpleConfiguration< double >( 0.5 ) };

 auto k = v.clone();
 assert( ( k->f_value.size() == 2 ) && ( k->f_value[ 0 ] != v.f_value[ 0 ] ) );
 assert( value_of< int >( k->f_value[ 0 ] ) == 4 );
 delete k;

 auto n = nc_round_trip( v );
 auto nv = dynamic_cast< VecConf * >( n );
 expect( nv , "netCDF round trip of SimpleConfiguration< std::vector< "
	 "Configuration * > > gives back a vector (the serialized group has "
	 "no \"type\" attribute)" );
 if( nv ) {
  expect( ( nv->f_value.size() == 2 ) && nv->f_value[ 1 ] &&
	  ( value_of< int >( nv->f_value[ 0 ] ) == 4 ) &&
	  ( value_of< double >( nv->f_value[ 1 ] ) == 0.5 ) ,
	  "netCDF round trip of the vector of Configuration *: { 4 , 0.5 } "
	  "read back" );
  }
 delete n;

 // a nullptr in it is read back as nullptr
 VecConf w;
 w.f_value = { nullptr , new SimpleConfiguration< int >( 1 ) };
 n = nc_round_trip( w );
 nv = dynamic_cast< VecConf * >( n );
 expect( nv && ( nv->f_value.size() == 2 ) && ( ! nv->f_value[ 0 ] ) &&
	 nv->f_value[ 1 ] && ( value_of< int >( nv->f_value[ 1 ] ) == 1 ) ,
	 "netCDF round trip of the vector { nullptr , 1 } of Configuration *" );
 delete n;

 std::cout << "SimpleConfiguration< std::vector< Configuration * > >: done"
	   << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*------------------- TESTS OF THE TEXT AND FILE FORMATS -------------------*/
/*--------------------------------------------------------------------------*/
/* Configuration::deserialize( std::istream ): the empty stream and a "*"
 * alone give nullptr, a name not in the factory throws, "*filename" loads
 * a text or a netCDF file, and a missing file gives nullptr. */

static void test_deserialize_stream( void )
{
 assert( ! from_text( "" ) );
 assert( ! from_text( "  # only a comment\n" ) );
 assert( ! from_text( "* # nothing" ) );
 assert( ! from_text( "* # nothing\n" ) );

 // a "*" that ends the stream is followed by the empty string as well
 bool alone = false;
 try {
  alone = ! from_text( "*" );
  }
 catch( std::exception & e ) {
  std::cout << "\"*\" at the end of the stream: " << e.what() << std::endl;
  }
 expect( alone , "Configuration::deserialize( std::istream ) of a \"*\" "
	 "that ends the stream gives nullptr" );

 assert( throws< std::invalid_argument >( []() {
    from_text( "NoSuchConfiguration 1" ); } ) );

 write_file( "tests_Configuration_int.txt" , "SimpleConfiguration<int> 11\n" );
 auto t = from_text( "*tests_Configuration_int.txt" );
 assert( t && ( value_of< int >( t ) == 11 ) );
 delete t;

 SimpleConfiguration< int > ni( 12 );
 write_nc( ni , "tests_Configuration_int.nc4" );
 t = from_text( "*tests_Configuration_int.nc4" );
 assert( t && ( value_of< int >( t ) == 12 ) );
 delete t;

 assert( ! from_text( "*tests_Configuration_missing.txt" ) );

 // a Configuration is read at the point of the stream it starts at, and the
 // stream is left right after it
 std::istringstream two( "SimpleConfiguration<int> 1 SimpleConfiguration<int> 2" );
 auto c1 = Configuration::deserialize( two );
 auto c2 = Configuration::deserialize( two );
 assert( c1 && c2 && ( value_of< int >( c1 ) == 1 ) &&
	 ( value_of< int >( c2 ) == 2 ) );
 assert( ! Configuration::deserialize( two ) );
 delete c1;
 delete c2;

 std::cout << "Configuration::deserialize( std::istream ): OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The "[idx]" at the end of a netCDF filename selects the Configuration of
 * an eConfigFile, the one appended by serialize( ... , false ) being the
 * second; a position that is not in the file gives nullptr. */

static void test_netCDF_positions( void )
{
 const std::string fn = "tests_Configuration_multi.nc4";
 SimpleConfiguration< int > first( 1 );
 SimpleConfiguration< double > second( 2.5 );
 write_nc( first , fn );
 write_nc( second , fn , eConfigFile , false );

 auto c0 = Configuration::deserialize( fn );
 auto c1 = Configuration::deserialize( fn + "[1]" );
 auto c00 = Configuration::deserialize( fn + "[0]" );
 assert( c0 && ( value_of< int >( c0 ) == 1 ) );
 assert( c00 && ( value_of< int >( c00 ) == 1 ) );
 assert( c1 && ( value_of< double >( c1 ) == 2.5 ) );
 delete c0;
 delete c1;
 delete c00;

 assert( ! Configuration::deserialize( fn + "[2]" ) );

 // replace == true starts the file anew
 write_nc( second , fn );
 c0 = Configuration::deserialize( fn );
 assert( c0 && ( value_of< double >( c0 ) == 2.5 ) );
 delete c0;
 assert( ! Configuration::deserialize( fn + "[1]" ) );

 // a file type that is not one of a Configuration is refused
 assert( throws< std::invalid_argument >( [ & ]() {
    write_nc( first , fn , eBlockFile ); } ) );

 std::cout << "positions in a netCDF file: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The filename prefix is put before every relative filename and not before
 * an absolute one. */

static void test_filename_prefix( void )
{
 write_file( "tests_Configuration_int.txt" , "SimpleConfiguration<int> 11\n" );
 const std::string abs = std::filesystem::absolute(
				     "tests_Configuration_int.txt" ).string();

 Configuration::set_filename_prefix( "./" );
 auto t = Configuration::deserialize( "tests_Configuration_int.txt" );
 assert( t && ( value_of< int >( t ) == 11 ) );
 delete t;

 Configuration::set_filename_prefix( "/no/such/directory/" );
 assert( ! Configuration::deserialize( "tests_Configuration_int.txt" ) );
 t = Configuration::deserialize( abs );
 assert( t && ( value_of< int >( t ) == 11 ) );
 delete t;

 Configuration::set_filename_prefix( "" );
 assert( Configuration::get_filename_prefix().empty() );

 std::cout << "filename prefix: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*------------------------- TESTS OF ComputeConfig -------------------------*/
/*--------------------------------------------------------------------------*/
/// a ComputeConfig with every kind of parameter and an extra Configuration

static const char * full_ComputeConfig =
 "ComputeConfig 3             # diff and relax\n"
 "2  intA 5  intB -3          # int\n"
 "1  dblA 0.5                 # double\n"
 "1  strA hello               # string\n"
 "2  vintA 3 1 2 3  vintB 0   # vector of int, one of them empty\n"
 "1  vdblA 2 0.25 0.75        # vector of double\n"
 "1  vstrA 2 one two          # vector of string\n"
 "SimpleConfiguration<int> 7  # extra\n";

/*--------------------------------------------------------------------------*/
/* The full format is read in all its parts, and the ComputeConfig goes
 * through netCDF and clone() with all of them, the extra Configuration
 * included. */

static void test_ComputeConfig_load( void )
{
 auto c = from_text( full_ComputeConfig );
 auto cc = dynamic_cast< ComputeConfig * >( c );
 assert( cc );
 assert( cc->diff() && cc->relax() );
 assert( ( cc->int_pars == decltype( cc->int_pars ){ { "intA" , 5 } ,
						      { "intB" , -3 } } ) );
 assert( ( cc->dbl_pars == decltype( cc->dbl_pars ){ { "dblA" , 0.5 } } ) );
 assert( ( cc->str_pars == decltype( cc->str_pars ){ { "strA" , "hello" } } ) );
 assert( ( cc->vint_pars == decltype( cc->vint_pars ){
	    { "vintA" , { 1 , 2 , 3 } } , { "vintB" , {} } } ) );
 assert( ( cc->vdbl_pars == decltype( cc->vdbl_pars ){
	    { "vdblA" , { 0.25 , 0.75 } } } ) );
 assert( ( cc->vstr_pars == decltype( cc->vstr_pars ){
	    { "vstrA" , { "one" , "two" } } } ) );
 assert( value_of< int >( cc->f_extra_Configuration ) == 7 );
 assert( ! cc->empty() );

 auto n = nc_round_trip( *cc );
 auto nc = dynamic_cast< ComputeConfig * >( n );
 expect( nc , "netCDF round trip of a ComputeConfig with every kind of "
	 "parameter gives back a ComputeConfig" );
 if( nc ) {
  expect( same( *nc , *cc ) , "netCDF round trip of a ComputeConfig with "
	  "every kind of parameter gives back the same parameters" );
  assert( nc->f_extra_Configuration &&
	  ( value_of< int >( nc->f_extra_Configuration ) == 7 ) );
  }
 delete n;

 // one kind of parameter at a time, to tell which of them do not make it
 for( char kind : { 'i' , 'd' , 's' , 'I' , 'D' , 'S' } ) {
  ComputeConfig one( *cc );
  for( char other : { 'i' , 'd' , 's' , 'I' , 'D' , 'S' } )
   if( other != kind )
    switch( other ) {
     case( 'i' ): one.int_pars.clear(); break;
     case( 'd' ): one.dbl_pars.clear(); break;
     case( 's' ): one.str_pars.clear(); break;
     case( 'I' ): one.vint_pars.clear(); break;
     case( 'D' ): one.vdbl_pars.clear(); break;
     default: one.vstr_pars.clear();
     }
  n = nc_round_trip( one );
  nc = dynamic_cast< ComputeConfig * >( n );
  std::string what = std::string( "netCDF round trip of a ComputeConfig "
				  "with only the parameters of kind '" ) +
                     kind + "'";
  expect( nc , what + " gives back a ComputeConfig" );
  if( nc ) {
   std::string got;
   for( auto & p : nc->int_pars ) got += " \"" + p.first + "\"";
   for( auto & p : nc->dbl_pars ) got += " \"" + p.first + "\"";
   for( auto & p : nc->str_pars ) got += " \"" + p.first + "\" = \"" +
				      p.second + "\"";
   for( auto & p : nc->vint_pars ) got += " \"" + p.first + "\"";
   for( auto & p : nc->vdbl_pars ) got += " \"" + p.first + "\"";
   expect( same( *nc , one ) , what + " gives back the same parameters "
	   "(names read back:" + got + " )" );
   }
  delete n;
  }

 auto k = cc->clone();
 assert( same( *k , *cc ) );
 assert( k->f_extra_Configuration &&
	 ( k->f_extra_Configuration != cc->f_extra_Configuration ) );
 k->set_par( "intA" , 6 );
 assert( cc->int_pars[ 0 ].second == 5 );
 delete k;

 // an empty ComputeConfig goes through netCDF as well
 ComputeConfig e;
 e.set_diff( false );
 n = nc_round_trip( e );
 nc = dynamic_cast< ComputeConfig * >( n );
 assert( nc && nc->empty() && ( ! nc->diff() ) && ( ! nc->relax() ) );
 delete n;

 delete c;
 std::cout << "ComputeConfig load, netCDF and clone: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The flags are bit 0 (diff) and bit 1 (relax); a stream that ends between
 * two sections leaves the rest empty, and one that ends before the flags
 * leaves f_diff == false [see ComputeConfig::load()]; a count larger than
 * the entries that follow throws. */

static void test_ComputeConfig_flags( void )
{
 const std::pair< const char * , std::pair< bool , bool > > cases[] = {
  { "ComputeConfig 0" , { false , false } } ,
  { "ComputeConfig 1" , { true , false } } ,
  { "ComputeConfig 2" , { false , true } } ,
  { "ComputeConfig 3" , { true , true } } };
 for( auto & [ text , flags ] : cases ) {
  auto c = dynamic_cast< ComputeConfig * >( from_text( text ) );
  assert( c && ( c->diff() == flags.first ) &&
	  ( c->relax() == flags.second ) && c->empty() );
  delete c;
  }

 // the stream ends after the int parameters
 auto c = dynamic_cast< ComputeConfig * >(
			     from_text( "ComputeConfig 0  1 intA 4\n" ) );
 assert( c && ( c->int_pars.size() == 1 ) && c->dbl_pars.empty() &&
	 c->vstr_pars.empty() && ( ! c->f_extra_Configuration ) );
 delete c;

 // the stream ends before the flags
 c = dynamic_cast< ComputeConfig * >( from_text( "ComputeConfig" ) );
 assert( c && c->empty() );
 expect( ! c->diff() , "ComputeConfig::load() of a stream that ends before "
	 "the flags leaves f_diff == false, as the comment of load() says "
	 "(\"if the attribute is not there, f_diff == false is assumed\")" );
 delete c;

 // two int parameters are announced, one is there
 assert( throws< std::logic_error >( []() {
    from_text( "ComputeConfig 0  2 intA 1  0" ); } ) );

 // a count that is not a number
 assert( throws< std::logic_error >( []() {
    from_text( "ComputeConfig 0  1 intA 1  x" ); } ) );

 std::cout << "ComputeConfig flags and partial streams: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* set_par() adds a parameter that is not there, tells whether the value
 * has changed, reset_par() removes one and ignores a name that is not
 * there, clear() empties everything and clears the extra Configuration. */

static void test_ComputeConfig_set_par( void )
{
 ComputeConfig cc;
 assert( cc.diff() && ( ! cc.relax() ) && cc.empty() );

 assert( cc.set_par( "a" , 1 ) );
 assert( ! cc.set_par( "a" , 1 ) );
 assert( cc.set_par( "a" , 2 ) );
 assert( ( cc.int_pars.size() == 1 ) && ( cc.int_pars[ 0 ].second == 2 ) );

 assert( cc.set_par( "d" , 0.5 ) );
 assert( ! cc.set_par( "d" , 0.5 ) );
 assert( cc.set_par( "s" , std::string( "x" ) ) );
 assert( ! cc.set_par( "s" , std::string( "x" ) ) );
 assert( cc.set_par( "vi" , std::vector< int >{ 1 , 2 } ) );
 assert( ! cc.set_par( "vi" , std::vector< int >{ 1 , 2 } ) );
 assert( cc.set_par( "vd" , std::vector< double >{} ) );
 assert( cc.set_par( "vs" , std::vector< std::string >{ "p" } ) );

 // one entry of a vector that is there, past its end: it is extended
 cc.set_par( "vi" , 4u , 9 );
 assert( ( cc.vint_pars[ 0 ].second == std::vector< int >{ 1 , 2 , 0 , 0 ,
							   9 } ) );
 cc.set_par( "vd" , 0u , 1.5 );
 assert( ( cc.vdbl_pars[ 0 ].second == std::vector< double >{ 1.5 } ) );

 cc.reset_par( "a" , 'i' );
 assert( cc.int_pars.empty() );
 cc.reset_par( "nosuchname" , 'd' );
 assert( cc.dbl_pars.size() == 1 );
 cc.reset_par( "vs" , 'S' );
 assert( cc.vstr_pars.empty() );
 assert( throws< std::invalid_argument >( [ & ]() {
    cc.reset_par( "d" , 'x' ); } ) );

 // clear() empties the lists, clears the extra Configuration and leaves
 // f_diff == false
 auto extra = new ComputeConfig;
 extra->set_par( "inner" , 1 );
 cc.f_extra_Configuration = extra;
 cc.set_relax( true );
 cc.clear();
 assert( ( ! cc.diff() ) && ( ! cc.relax() ) );
 assert( cc.dbl_pars.empty() && cc.str_pars.empty() &&
	 cc.vint_pars.empty() && cc.vdbl_pars.empty() );
 assert( ( cc.f_extra_Configuration == extra ) && extra->empty() );
 assert( ! cc.empty() );  // the extra Configuration is still there

 std::cout << "ComputeConfig set_par, reset_par, clear: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* set_par( name , pos , value ) on a name that is not there adds the vector
 * parameter, extended up to pos [see ComputeConfig::set_par()]. */

static void test_ComputeConfig_set_par_entry_of_new_name( void )
{
 std::cout << "ComputeConfig set_par( name , pos , value ) on a name not "
	   << "yet there: checking (a crash right after this line is this "
	   << "check failing)" << std::endl;

 ComputeConfig cc;
 cc.set_par( "vi" , 2u , 5 );
 expect( ( cc.vint_pars.size() == 1 ) &&
	 ( cc.vint_pars[ 0 ].first == "vi" ) &&
	 ( cc.vint_pars[ 0 ].second == std::vector< int >{ 0 , 0 , 5 } ) ,
	 "set_par( \"vi\" , 2 , 5 ) on an empty ComputeConfig gives vi = "
	 "{ 0 , 0 , 5 }" );

 std::cout << "ComputeConfig set_par( name , pos , value ) on a name not "
	   << "yet there: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/// the base of the cascade overrides

static const char * base_ComputeConfig =
 "ComputeConfig 1\n"
 "2  intA 1  intB 2\n"
 "1  dblA 0.5\n"
 "0\n"
 "1  vintA 2 1 1\n"
 "0\n"
 "0\n"
 "SimpleConfiguration<int> 7\n";

/// the body of an override of the base, without the extra slot

static const char * override_body =
 " 0                 # the flags are those of the override\n"
 " 1  intB 20        # replaced\n"
 " 1  dblB 1.5       # added\n"
 " 0\n"
 " 1  vintA 1 9      # replaced\n"
 " 0\n"
 " 0\n";

/*--------------------------------------------------------------------------*/
/* "*file +" loads the file and merges the body that follows onto it: the
 * parameters in the body replace or are added to those of the file, the
 * others are kept. At the end of the stream the extra slot can be left
 * out, and the extra Configuration of the file is kept; the slot, if it is
 * there, replaces it [see ComputeConfig::merge_overrides()]. */

static void test_ComputeConfig_override( void )
{
 write_file( "tests_Configuration_base.txt" , base_ComputeConfig );

 auto c = from_text( std::string( "*tests_Configuration_base.txt +\n" ) +
		     override_body );
 auto cc = dynamic_cast< ComputeConfig * >( c );
 assert( cc );
 assert( ! cc->diff() );
 assert( ( cc->int_pars == decltype( cc->int_pars ){ { "intA" , 1 } ,
						      { "intB" , 20 } } ) );
 assert( ( cc->dbl_pars == decltype( cc->dbl_pars ){ { "dblA" , 0.5 } ,
						      { "dblB" , 1.5 } } ) );
 assert( ( cc->vint_pars == decltype( cc->vint_pars ){ { "vintA" , { 9 } } } ) );
 // the stream has ended where the slot would be: the extra is kept
 assert( cc->f_extra_Configuration &&
	 ( value_of< int >( cc->f_extra_Configuration ) == 7 ) );
 delete c;

 // the slot is there and says nullptr
 c = from_text( std::string( "*tests_Configuration_base.txt +\n" ) +
		override_body + " * # [none]\n" );
 cc = dynamic_cast< ComputeConfig * >( c );
 assert( cc && ( cc->int_pars.size() == 2 ) );
 assert( ! cc->f_extra_Configuration );
 delete c;

 // the slot is there and says something else
 c = from_text( std::string( "*tests_Configuration_base.txt +\n" ) +
		override_body + " SimpleConfiguration<double> 0.25\n" );
 cc = dynamic_cast< ComputeConfig * >( c );
 assert( cc && cc->f_extra_Configuration &&
	 ( value_of< double >( cc->f_extra_Configuration ) == 0.25 ) );
 delete c;

 // a Configuration that has no named parameters cannot be overridden
 write_file( "tests_Configuration_int.txt" , "SimpleConfiguration<int> 11\n" );
 assert( throws< std::invalid_argument >( []() {
    from_text( "*tests_Configuration_int.txt + 4" ); } ) );

 // and nothing can be overridden if the file gives nothing
 assert( throws< std::invalid_argument >( []() {
    from_text( "*tests_Configuration_missing.txt + 0" ); } ) );

 // whitespace and comments may come between the filename and the "+"
 c = from_text( std::string( "*tests_Configuration_base.txt # base\n"
			     "  + # and now the override\n" ) + override_body );
 cc = dynamic_cast< ComputeConfig * >( c );
 assert( cc && ( cc->int_pars.size() == 2 ) &&
	 ( cc->int_pars[ 1 ].second == 20 ) );
 delete c;

 std::cout << "ComputeConfig cascade override: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* An override body reads exactly as many items as a full ComputeConfig
 * [see Configuration::merge_overrides()], the extra slot included: in a
 * container, the slot "*" has to be written for the next Configuration to
 * be read as the next one; without it, the next Configuration is read as
 * the extra Configuration of the override. */

static void test_ComputeConfig_override_in_a_container( void )
{
 write_file( "tests_Configuration_base.txt" , base_ComputeConfig );
 const std::string head = "SimpleConfiguration<std::pair<Configuration*,"
                          "Configuration*>>\n"
                          "*tests_Configuration_base.txt +\n";

 // with the slot
 auto c = from_text( head + override_body + " * # [none]\n"
		     "ComputeConfig 1  1 intC 3\n" );
 auto p = dynamic_cast< PairConf * >( c );
 assert( p );
 auto first = dynamic_cast< ComputeConfig * >( p->f_value.first );
 auto second = dynamic_cast< ComputeConfig * >( p->f_value.second );
 assert( first && ( first->int_pars.size() == 2 ) &&
	 ( ! first->f_extra_Configuration ) );
 assert( second && second->diff() && ( second->int_pars.size() == 1 ) &&
	 ( second->int_pars[ 0 ].first == "intC" ) );
 delete c;

 // without the slot, the second ComputeConfig becomes the extra of the first
 c = from_text( head + override_body + "ComputeConfig 1  1 intC 3\n" );
 p = dynamic_cast< PairConf * >( c );
 assert( p );
 first = dynamic_cast< ComputeConfig * >( p->f_value.first );
 assert( first );
 auto swallowed = dynamic_cast< ComputeConfig * >(
					       first->f_extra_Configuration );
 assert( swallowed && ( swallowed->int_pars.size() == 1 ) &&
	 ( swallowed->int_pars[ 0 ].first == "intC" ) );
 assert( ! p->f_value.second );
 delete c;

 std::cout << "ComputeConfig cascade override in a container: OK"
	   << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*------------------------ TESTS OF BlockConfig ----------------------------*/
/*--------------------------------------------------------------------------*/
/* The text format: the flag, the version, then the ten slots in order; the
 * old format without the version and a version that is not the current one
 * throw, a stream that ends early leaves the rest nullptr. */

static void test_BlockConfig_load( void )
{
 auto c = from_text( "BlockConfig 1 2\n"
		     "*                            # structure\n"
		     "*  *  *  *                   # constraints and variables\n"
		     "SimpleConfiguration<int> 1   # objective\n"
		     "*  *  *                      # feasible, optimal, solution\n"
		     "SimpleConfiguration<int> 2   # extra\n" );
 auto bc = dynamic_cast< BlockConfig * >( c );
 assert( bc && bc->is_diff() );
 assert( ! bc->f_structure_Configuration );
 assert( ! bc->f_static_constraints_Configuration );
 assert( ! bc->f_dynamic_variables_Configuration );
 assert( value_of< int >( bc->f_objective_Configuration ) == 1 );
 assert( ! bc->f_solution_Configuration );
 assert( value_of< int >( bc->f_extra_Configuration ) == 2 );
 assert( ! bc->empty() );
 delete c;

 c = from_text( "BlockConfig 0 2" );
 bc = dynamic_cast< BlockConfig * >( c );
 assert( bc && ( ! bc->is_diff() ) && bc->empty() );
 delete c;

 assert( throws< std::invalid_argument >( []() {
    from_text( "BlockConfig 0  *  *  *" ); } ) );
 assert( throws< std::invalid_argument >( []() {
    from_text( "BlockConfig 0 1" ); } ) );
 assert( throws< std::invalid_argument >( []() {
    from_text( "BlockConfig 0 3" ); } ) );

 std::cout << "BlockConfig load: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/// a BlockConfig with an objective and an extra Configuration

static BlockConfig * make_BlockConfig( bool diff , int objective ,
				       int extra )
{
 auto bc = new BlockConfig( diff );
 bc->f_objective_Configuration = new SimpleConfiguration< int >( objective );
 bc->f_extra_Configuration = new SimpleConfiguration< int >( extra );
 return( bc );
 }

/*--------------------------------------------------------------------------*/
/* get() clones the BlockConfig of a Block, apply() moves it into another
 * Block in setting mode (the previous one goes) and in differential mode
 * (the slots that are nullptr keep what is there), clear() empties it. */

static void test_BlockConfig_get_apply( void )
{
 AbstractBlock A , B;
 A.set_BlockConfig( make_BlockConfig( false , 1 , 2 ) );

 BlockConfig g;
 g.get( & A );
 assert( ! g.is_diff() );
 assert( g.f_objective_Configuration &&
	 ( g.f_objective_Configuration !=
	   A.get_BlockConfig()->f_objective_Configuration ) );
 assert( value_of< int >( g.f_objective_Configuration ) == 1 );
 assert( value_of< int >( g.f_extra_Configuration ) == 2 );

 // the constructor from a Block does the same
 BlockConfig g2( & A );
 assert( value_of< int >( g2.f_extra_Configuration ) == 2 );

 // apply() empties the BlockConfig and gives its content to the Block
 g.apply( & B );
 assert( g.empty() );
 auto bbc = B.get_BlockConfig();
 assert( bbc && ( value_of< int >( bbc->f_objective_Configuration ) == 1 ) &&
	 ( value_of< int >( bbc->f_extra_Configuration ) == 2 ) );

 // differential: only the extra changes
 BlockConfig d( true );
 d.f_extra_Configuration = new SimpleConfiguration< int >( 9 );
 d.apply( & B );
 bbc = B.get_BlockConfig();
 assert( value_of< int >( bbc->f_objective_Configuration ) == 1 );
 assert( value_of< int >( bbc->f_extra_Configuration ) == 9 );

 // setting: what is not there goes
 BlockConfig s( false );
 s.f_extra_Configuration = new SimpleConfiguration< int >( 5 );
 s.apply( & B );
 bbc = B.get_BlockConfig();
 assert( ! bbc->f_objective_Configuration );
 assert( value_of< int >( bbc->f_extra_Configuration ) == 5 );

 // a Block with no BlockConfig gives an empty one; nullptr is no Block
 AbstractBlock C;
 BlockConfig e( true );
 e.f_extra_Configuration = new SimpleConfiguration< int >( 3 );
 e.get( & C );
 assert( e.empty() );
 e.apply( nullptr );

 // clear() empties it and sets the setting mode
 g2.clear();
 assert( g2.empty() && ( ! g2.is_diff() ) );

 // a plain Block has no structure to choose [see Block::set_structure()]
 BlockConfig st( false );
 st.f_structure_Configuration = new SimpleConfiguration< int >( 1 );
 assert( throws< std::invalid_argument >( [ & ]() { st.apply( & C ); } ) );

 std::cout << "BlockConfig get, apply, clear: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A BlockConfig goes through netCDF and clone(), as an eConfigFile and as
 * the "BlockConfig" group of the "Prob_0" group of an eProbFile [see
 * smspp_netCDF_file_type], which is where Configuration::deserialize()
 * looks for it. */

static void test_BlockConfig_netCDF( void )
{
 std::unique_ptr< BlockConfig > bc( make_BlockConfig( true , 4 , 6 ) );
 bc->f_is_optimal_Configuration = new SimpleConfiguration< double >( 0.5 );

 auto n = nc_round_trip( *bc );
 auto nb = dynamic_cast< BlockConfig * >( n );
 assert( nb && nb->is_diff() );
 assert( value_of< int >( nb->f_objective_Configuration ) == 4 );
 assert( value_of< int >( nb->f_extra_Configuration ) == 6 );
 assert( value_of< double >( nb->f_is_optimal_Configuration ) == 0.5 );
 assert( ! nb->f_structure_Configuration );
 delete n;

 auto k = bc->clone();
 assert( k->is_diff() &&
	 ( value_of< int >( k->f_extra_Configuration ) == 6 ) &&
	 ( k->f_extra_Configuration != bc->f_extra_Configuration ) );
 delete k;

 const std::string fn = "tests_Configuration_prob.nc4";
 write_nc( *bc , fn , eProbFile );
 {
  netCDF::NcFile f( fn , netCDF::NcFile::read );
  expect( ! f.getGroup( "Prob_0" ).isNull() , "BlockConfig::serialize( " +
	  fn + " , eProbFile ) writes the group \"Prob_0\"" );
  }
 n = Configuration::deserialize( fn );
 expect( dynamic_cast< BlockConfig * >( n ) , "a BlockConfig written with "
	 "serialize( filename , eProbFile ) is read back by Configuration::"
	 "deserialize( filename )" );
 delete n;

 std::cout << "BlockConfig netCDF and clone: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The move constructor takes every sub-Configuration, the structure one
 * included, and leaves the moved-from BlockConfig empty. */

static void test_BlockConfig_move( void )
{
 BlockConfig a( false );
 auto strc = new SimpleConfiguration< int >( 8 );
 auto extra = new SimpleConfiguration< int >( 9 );
 a.f_structure_Configuration = strc;
 a.f_extra_Configuration = extra;

 BlockConfig b( std::move( a ) );
 const bool taken = ( b.f_structure_Configuration == strc );
 expect( taken , "the move constructor of BlockConfig moves "
	 "f_structure_Configuration" );
 expect( ! a.f_structure_Configuration , "the move constructor of "
	 "BlockConfig leaves f_structure_Configuration of the moved-from one "
	 "nullptr" );
 assert( ( b.f_extra_Configuration == extra ) && ( ! a.f_extra_Configuration ) );

 // whatever happened, each pointer is deleted once
 if( ! taken )
  b.f_structure_Configuration = nullptr;
 a.f_structure_Configuration = nullptr;
 delete b.f_structure_Configuration;
 b.f_structure_Configuration = nullptr;
 if( ! taken )
  delete strc;

 std::cout << "BlockConfig move constructor: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* An OCRBlockConfig got from a Block records the BlockConfig of its
 * sub-Block, by index or by name [see OCRBlockConfig::get_right_BlockConfig()],
 * applies them to the sub-Block of another Block, keeps its structure
 * through clear(), and refuses a sub-Block that is not there. */

static void test_OCRBlockConfig( void )
{
 // two Block with two sub-Block each, the second one named
 AbstractBlock P , Q;
 for( auto father : { & P , & Q } ) {
  father->add_nested_Block( new AbstractBlock( father ) );
  auto named = new AbstractBlock( father );
  named->set_name( "second" );
  father->add_nested_Block( named );
  }
 P.get_nested_Block( 0 )->set_BlockConfig( make_BlockConfig( false , 1 , 11 ) );
 P.get_nested_Block( 1 )->set_BlockConfig( make_BlockConfig( false , 2 , 22 ) );

 // got out of the whole Block, it finds the BlockConfig of the sub-Block
 OCRBlockConfig got( & P );
 expect( got.num_sub_BlockConfig() == 2 , "OCRBlockConfig( Block * ) of a "
	 "Block whose 2 sub-Block have a BlockConfig records 2 sub-"
	 "BlockConfig, it records " +
	 std::to_string( got.num_sub_BlockConfig() ) );
 if( got.num_sub_BlockConfig() == 2 ) {
  expect( got.get_sub_Block_id( 0 ) == "0" , "the unnamed sub-Block is "
	  "recorded by its index" );
  expect( got.get_sub_Block_id( 1 ) == "second" , "the named sub-Block is "
	  "recorded by its name" );
  }

 auto right = OCRBlockConfig::get_right_BlockConfig( P.get_nested_Block( 0 ) );
 expect( right && right->f_extra_Configuration &&
	 ( value_of< int >( right->f_extra_Configuration ) == 11 ) ,
	 "OCRBlockConfig::get_right_BlockConfig() of a Block whose BlockConfig "
	 "has an extra Configuration gives a BlockConfig with it" );
 delete right;

 // it is the simplest *BlockConfig that holds it, and there is none for a
 // Block with no BlockConfig; for the whole Block it is an RBlockConfig
 right = OCRBlockConfig::get_right_BlockConfig( P.get_nested_Block( 0 ) );
 expect( right && ( right->classname() == "BlockConfig" ) ,
	 "OCRBlockConfig::get_right_BlockConfig() of a Block whose BlockConfig "
	 "has no sub-BlockConfig is a BlockConfig" );
 delete right;
 AbstractBlock bare;
 assert( ! OCRBlockConfig::get_right_BlockConfig( & bare ) );
 right = OCRBlockConfig::get_right_BlockConfig( & P );
 expect( right && ( right->classname() == "RBlockConfig" ) ,
	 "OCRBlockConfig::get_right_BlockConfig() of a Block whose sub-Block "
	 "have a BlockConfig is an RBlockConfig" );
 delete right;

 // what is got out of one Block is applied to the other one
 if( got.num_sub_BlockConfig() == 2 ) {
  got.apply( & Q );
  auto g0 = Q.get_nested_Block( 0 )->get_BlockConfig();
  auto g1 = Q.get_nested_Block( 1 )->get_BlockConfig();
  assert( g0 && ( value_of< int >( g0->f_extra_Configuration ) == 11 ) );
  assert( g1 && ( value_of< int >( g1->f_extra_Configuration ) == 22 ) );
  Q.get_nested_Block( 0 )->set_BlockConfig();
  Q.get_nested_Block( 1 )->set_BlockConfig();
  }

 // got with the sub-Block already there, by name, it gets their BlockConfig
 OCRBlockConfig again( false );
 again.add_sub_BlockConfig( new BlockConfig , "second" );
 again.get( & P );
 assert( ( again.num_sub_BlockConfig() == 1 ) &&
	 ( value_of< int >(
		  again.get_sub_BlockConfig( 0 )->f_extra_Configuration ) == 22 ) );

 // one by index and one by name
 OCRBlockConfig oc( false );
 oc.add_sub_BlockConfig( make_BlockConfig( false , 1 , 11 ) , Block::Index( 0 ) );
 oc.add_sub_BlockConfig( make_BlockConfig( false , 2 , 22 ) , "second" );
 assert( oc.num_sub_BlockConfig() == 2 );
 assert( oc.get_sub_Block_id( 0 ) == "0" );
 assert( oc.get_sub_Block_id( 1 ) == "second" );
 assert( throws< std::invalid_argument >( [ & ]() {
    static_cast< void >( oc.get_sub_BlockConfig( 2 ) ); } ) );
 assert( throws< std::invalid_argument >( [ & ]() {
    static_cast< void >( oc.get_sub_Block_id( 2 ) ); } ) );

 auto k = oc.clone();
 oc.apply( & Q );
 auto q0 = Q.get_nested_Block( 0 )->get_BlockConfig();
 auto q1 = Q.get_nested_Block( 1 )->get_BlockConfig();
 assert( q0 && ( value_of< int >( q0->f_extra_Configuration ) == 11 ) );
 assert( q1 && ( value_of< int >( q1->f_objective_Configuration ) == 2 ) );

 // cleared, it keeps its sub-BlockConfig, empty, and in setting mode they
 // empty the BlockConfig of the sub-Block
 k->clear();
 assert( k->num_sub_BlockConfig() == 2 );
 assert( k->get_sub_BlockConfig( 0 )->empty() );
 k->apply( & Q );
 q0 = Q.get_nested_Block( 0 )->get_BlockConfig();
 assert( ( ! q0 ) || q0->empty() );
 delete k;

 // a sub-Block that is not there
 OCRBlockConfig bad;
 bad.add_sub_BlockConfig( new BlockConfig( false ) , Block::Index( 5 ) );
 assert( throws< std::logic_error >( [ & ]() { bad.apply( & Q ); } ) );

 std::cout << "OCRBlockConfig get, apply, clear: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*--------------------- TESTS OF BlockSolverConfig -------------------------*/
/*--------------------------------------------------------------------------*/
/// a BlockSolverConfig read out of the text

static BlockSolverConfig * bsc_from_text( const std::string & text )
{
 auto c = from_text( text );
 auto bsc = dynamic_cast< BlockSolverConfig * >( c );
 assert( bsc );
 return( bsc );
 }

/*--------------------------------------------------------------------------*/
/* The text format: the mode, the names with "*" for the empty one, the
 * ComputeConfig with "*" for nullptr; a mode that is not one of the three
 * and a ComputeConfig slot that holds something else throw; the index
 * accessors throw out of range. */

static void test_BlockSolverConfig_load( void )
{
 auto bsc = bsc_from_text( "BlockSolverConfig 1\n"
			   "2  FakeSolver  *\n"
			   "2  *  ComputeConfig 1  1 intMaxIter 10  0 0 0 0 0 *\n" );
 assert( bsc->is_diff() == BlockSolverConfig::eDiffMode );
 assert( bsc->num_ComputeConfig() == 2 );
 assert( bsc->get_SolverName( 0 ) == "FakeSolver" );
 assert( bsc->get_SolverName( 1 ).empty() );
 assert( ! bsc->get_SolverConfig( 0 ) );
 assert( bsc->get_SolverConfig( 1 ) &&
	 ( bsc->get_SolverConfig( 1 )->int_pars.size() == 1 ) );
 assert( ! bsc->empty() );

 for( Index i : { Index( 2 ) , Index( 100 ) } ) {
  assert( throws< std::invalid_argument >( [ & ]() {
     static_cast< void >( bsc->get_SolverName( i ) ); } ) );
  assert( throws< std::invalid_argument >( [ & ]() {
     static_cast< void >( bsc->get_SolverConfig( i ) ); } ) );
  assert( throws< std::invalid_argument >( [ & ]() {
     bsc->set_Solver_name( i , "FakeSolver" ); } ) );
  assert( throws< std::invalid_argument >( [ & ]() {
     bsc->set_ComputeConfig( i ); } ) );
  assert( throws< std::invalid_argument >( [ & ]() {
     bsc->remove_ComputeConfig( i ); } ) );
  }

 // remove the first pair, then the other one: an empty one is left
 bsc->remove_ComputeConfig( 0 );
 assert( ( bsc->num_ComputeConfig() == 1 ) && bsc->get_SolverConfig( 0 ) );
 bsc->remove_ComputeConfig( 0 );
 assert( bsc->empty() && ( bsc->num_ComputeConfig() == 0 ) );
 delete bsc;

 // fewer ComputeConfig than names: the missing ones are nullptr
 bsc = bsc_from_text( "BlockSolverConfig 2  2 FakeSolver FakeSolver  0" );
 assert( ( bsc->is_diff() == BlockSolverConfig::eAddMode ) &&
	 ( bsc->num_ComputeConfig() == 2 ) && ( ! bsc->get_SolverConfig( 1 ) ) );
 delete bsc;

 // an empty one
 bsc = bsc_from_text( "BlockSolverConfig 0 0" );
 assert( bsc->empty() && ( bsc->is_diff() == BlockSolverConfig::eSetMode ) );
 delete bsc;

 assert( throws< std::invalid_argument >( []() {
    from_text( "BlockSolverConfig 3 0" ); } ) );
 assert( throws< std::invalid_argument >( []() {
    from_text( "BlockSolverConfig 0  1 FakeSolver  1 "
	       "SimpleConfiguration<int> 3" ); } ) );

 std::cout << "BlockSolverConfig load: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* apply() registers the Solver and records them, a ComputeConfig with a
 * parameter the Solver does not know throws unless it is relax()-ed, the
 * cleared BlockSolverConfig removes all and only its own Solver, once, and
 * an empty or never applied one does nothing. */

static void test_BlockSolverConfig_apply( void )
{
 AbstractBlock block;

 // somebody else's Solver, which nobody but its owner has to remove
 auto own = new FakeSolver;
 block.register_Solver( own );

 auto bsc = bsc_from_text( "BlockSolverConfig 2\n"
			   "2  FakeSolver  FakeSolver\n"
			   "2  ComputeConfig 1  1 intMaxIter 10  0 0 0 0 0 *\n"
			   "   ComputeConfig 3  1 noSuchPar 1  0 0 0 0 0 *\n" );
 bsc->apply( & block );
 auto & solvers = block.get_registered_solvers();
 assert( solvers.size() == 3 );
 assert( solvers.front() == own );
 auto & mine = bsc->get_Solvers( & block );
 assert( mine.size() == 2 );
 for( auto s : mine ) {
  assert( dynamic_cast< FakeSolver * >( s ) );
  assert( std::find( solvers.begin() , solvers.end() , s ) != solvers.end() );
  }

 // the clone has registered nothing
 auto k = bsc->clone();
 assert( ( k->num_ComputeConfig() == 2 ) && k->get_Solvers( & block ).empty() );
 assert( k->get_SolverConfig( 0 ) != bsc->get_SolverConfig( 0 ) );
 delete k;

 // get(): the names in setting mode, the empty names in differential mode
 BlockSolverConfig gs( & block , BlockSolverConfig::eSetMode );
 assert( gs.num_ComputeConfig() == 3 );
 for( auto & name : gs.get_SolverNames() )
  assert( name == "FakeSolver" );
 BlockSolverConfig gd( & block , BlockSolverConfig::eDiffMode );
 assert( gd.num_ComputeConfig() == 3 );
 for( auto & name : gd.get_SolverNames() )
  assert( name.empty() );
 BlockSolverConfig gc( & block , BlockSolverConfig::eSetMode , true );
 assert( gc.empty() );

 // cleared, it removes its two and leaves the other one
 bsc->clear();
 assert( bsc->empty() && ( bsc->is_diff() == BlockSolverConfig::eSetMode ) );
 bsc->apply( & block );
 assert( ( solvers.size() == 1 ) && ( solvers.front() == own ) );
 assert( bsc->get_Solvers( & block ).empty() );

 // a second time it does nothing
 bsc->apply( & block );
 assert( ( solvers.size() == 1 ) && ( solvers.front() == own ) );
 delete bsc;

 // an empty one, in any mode, does nothing
 for( int mode : { BlockSolverConfig::eSetMode , BlockSolverConfig::eDiffMode ,
		   BlockSolverConfig::eAddMode } ) {
  BlockSolverConfig empty( mode );
  empty.apply( & block );
  assert( ( solvers.size() == 1 ) && ( solvers.front() == own ) );
  }

 // a parameter the Solver does not know, the ComputeConfig not relax()-ed
 bsc = bsc_from_text( "BlockSolverConfig 2  1 FakeSolver\n"
		      "1  ComputeConfig 1  1 noSuchPar 1  0 0 0 0 0 *\n" );
 assert( throws< std::invalid_argument >( [ & ]() { bsc->apply( & block ); } ) );
 assert( ( solvers.size() == 1 ) && ( solvers.front() == own ) );
 delete bsc;

 // a Solver that is not in the factory
 bsc = bsc_from_text( "BlockSolverConfig 2  1 NoSuchSolver  0" );
 assert( throws< std::invalid_argument >( [ & ]() { bsc->apply( & block ); } ) );
 assert( solvers.size() == 1 );
 delete bsc;

 // setting mode on a Block with a Solver of somebody else replaces it, and
 // the new one is recorded
 bsc = bsc_from_text( "BlockSolverConfig 0  1 FakeSolver  0" );
 own = nullptr;  // deleted by the replacement
 bsc->apply( & block );
 assert( ( solvers.size() == 1 ) &&
	 ( bsc->get_Solvers( & block ).size() == 1 ) &&
	 ( bsc->get_Solvers( & block ).front() == solvers.front() ) );
 bsc->clear();
 bsc->apply( & block );
 assert( solvers.empty() );
 delete bsc;

 // apply() to no Block does nothing
 BlockSolverConfig none( BlockSolverConfig::eAddMode );
 none.add_ComputeConfig( "FakeSolver" );
 none.apply( nullptr );
 assert( none.get_Solvers( nullptr ).empty() );

 std::cout << "BlockSolverConfig apply and clear: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A BlockSolverConfig goes through netCDF as an eConfigFile, and as the
 * "BlockSolver" group of the "Prob_0" group of an eProbFile, where its
 * deserialize( netCDF::NcFile ) looks for it [see smspp_netCDF_file_type
 * and BlockSolverConfig::deserialize( netCDF::NcFile , unsigned int )]. */

static void test_BlockSolverConfig_netCDF( void )
{
 BlockSolverConfig bsc( BlockSolverConfig::eAddMode );
 auto cc = new ComputeConfig;
 cc->set_par( "intMaxIter" , 3 );
 bsc.add_ComputeConfig( "FakeSolver" , cc );
 bsc.add_ComputeConfig( "UpdateSolver" );

 auto n = nc_round_trip( bsc );
 auto nb = dynamic_cast< BlockSolverConfig * >( n );
 assert( nb && ( nb->is_diff() == BlockSolverConfig::eAddMode ) );
 assert( ( nb->get_SolverNames() ==
	   std::vector< std::string >{ "FakeSolver" , "UpdateSolver" } ) );
 assert( nb->get_SolverConfig( 0 ) );
 expect( same( *nb->get_SolverConfig( 0 ) , *cc ) , "netCDF round trip of "
	 "a BlockSolverConfig gives back the same parameters of its "
	 "ComputeConfig" );
 assert( ! nb->get_SolverConfig( 1 ) );
 delete n;

 // the empty one
 BlockSolverConfig e( BlockSolverConfig::eSetMode );
 n = nc_round_trip( e );
 nb = dynamic_cast< BlockSolverConfig * >( n );
 assert( nb && nb->empty() && ( nb->is_diff() == BlockSolverConfig::eSetMode ) );
 delete n;

 // an eProbFile written by hand as SMSTypedefs.h describes it
 const std::string fn = "tests_Configuration_prob.nc4";
 {
  netCDF::NcFile f( fn , netCDF::NcFile::replace );
  f.putAtt( "SMS++_file_type" , netCDF::NcInt() , int( eProbFile ) );
  auto g = f.addGroup( "Prob_0" ).addGroup( "BlockSolver" );
  bsc.serialize( g );
  }
 {
  netCDF::NcFile f( fn , netCDF::NcFile::read );
  auto r = BlockSolverConfig::deserialize( f , 0 );
  expect( r , "BlockSolverConfig::deserialize( eProbFile , 0 ) reads the "
	  "group \"BlockSolver\" of the group \"Prob_0\"" );
  delete r;
  }

 // and one written by serialize( filename , eProbFile )
 write_nc( bsc , fn , eProbFile );
 {
  netCDF::NcFile f( fn , netCDF::NcFile::read );
  expect( ! f.getGroup( "Prob_0" ).isNull() ,
	  "BlockSolverConfig::serialize( filename , eProbFile ) writes the "
	  "group \"Prob_0\"" );
  auto r = BlockSolverConfig::deserialize( f , 0 );
  assert( r && ( r->num_ComputeConfig() == 2 ) );
  delete r;
  }

 // Configuration::deserialize() finds it at the position -1, that of the
 // BlockSolver of "Prob_0" [see Configuration::deserialize( netCDF::NcFile ,
 // int )], and its BlockConfig, which is not there, at the position 0
 n = Configuration::deserialize( fn + "[-1]" );
 expect( dynamic_cast< BlockSolverConfig * >( n ) , "Configuration::"
	 "deserialize( filename[-1] ) reads the BlockSolver of \"Prob_0\"" );
 delete n;
 assert( ! Configuration::deserialize( fn + "[0]" ) );

 std::cout << "BlockSolverConfig netCDF: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- FIELD BY FIELD -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the directory of the files the tests write, removed at the end

static const std::filesystem::path & dir( void )
{
 static const std::filesystem::path d =
  std::filesystem::temp_directory_path() /
  ( "smspp_Configuration_test_" + std::to_string(
       std::chrono::steady_clock::now().time_since_epoch().count() ) );
 return( d );
 }

/*--------------------------------------------------------------------------*/
/// the absolute name of a file in dir()

static std::string file( const std::string & name )
{
 return( ( dir() / name ).string() );
 }

/*--------------------------------------------------------------------------*/
/// writes \p text in the file of dir() with the given name, returns its name

static std::string write_tmp_file( const std::string & name ,
			       const std::string & text )
{
 std::ofstream f( file( name ) );
 f << text;
 return( file( name ) );
 }

/*--------------------------------------------------------------------------*/
/// reads a Configuration out of \p text, as a .txt file would give it
/** The text is given the newline a .txt file ends with. */

static Configuration * from_txt( const std::string & text )
{
 std::istringstream in( text + "\n" );
 return( Configuration::deserialize( in ) );
 }

/*--------------------------------------------------------------------------*/
/// reads a Configuration of type T out of \p text, nullptr if another type

template< class T >
static T * text_as( const std::string & text )
{
 auto c = from_txt( text );
 auto t = dynamic_cast< T * >( c );
 if( ! t )
  delete c;
 return( t );
 }

/*--------------------------------------------------------------------------*/
/// writes \p c to a netCDF file of Configuration and reads it back
/** The call goes through a reference to the base class, since the :Block
 * Configuration override the serialize() of an open file, which hides the
 * one taking a filename. */

static Configuration * nc_back( const Configuration & c )
{
 const auto fn = file( "round_trip.nc4" );
 c.serialize( fn , eConfigFile );
 auto r = Configuration::deserialize( fn );
 std::filesystem::remove( fn );
 return( r );
 }

/*--------------------------------------------------------------------------*/
/// the netCDF round trip of \p c, nullptr if it is not a T

template< class T >
static T * nc_as( const Configuration & c )
{
 auto r = nc_back( c );
 auto t = dynamic_cast< T * >( r );
 if( ! t )
  delete r;
 return( t );
 }

/*--------------------------------------------------------------------------*/
/// true if the two Configuration are equal, for the types used here

static bool same( const Configuration * a , const Configuration * b );

/*--------------------------------------------------------------------------*/
/// true if the two ComputeConfig hold the same parameters

static bool same_cc( const ComputeConfig & a , const ComputeConfig & b )
{
 return( ( a.diff() == b.diff() ) && ( a.relax() == b.relax() ) &&
	 ( a.int_pars == b.int_pars ) && ( a.dbl_pars == b.dbl_pars ) &&
	 ( a.str_pars == b.str_pars ) && ( a.vint_pars == b.vint_pars ) &&
	 ( a.vdbl_pars == b.vdbl_pars ) && ( a.vstr_pars == b.vstr_pars ) &&
	 same( a.f_extra_Configuration , b.f_extra_Configuration ) );
 }

/*--------------------------------------------------------------------------*/
/// true if the ten sub-Configuration of the two BlockConfig are the same

static bool same_bc( const BlockConfig & a , const BlockConfig & b )
{
 return( ( a.is_diff() == b.is_diff() ) &&
	 same( a.f_structure_Configuration , b.f_structure_Configuration ) &&
	 same( a.f_static_constraints_Configuration ,
	       b.f_static_constraints_Configuration ) &&
	 same( a.f_dynamic_constraints_Configuration ,
	       b.f_dynamic_constraints_Configuration ) &&
	 same( a.f_static_variables_Configuration ,
	       b.f_static_variables_Configuration ) &&
	 same( a.f_dynamic_variables_Configuration ,
	       b.f_dynamic_variables_Configuration ) &&
	 same( a.f_objective_Configuration , b.f_objective_Configuration ) &&
	 same( a.f_is_feasible_Configuration , b.f_is_feasible_Configuration ) &&
	 same( a.f_is_optimal_Configuration , b.f_is_optimal_Configuration ) &&
	 same( a.f_solution_Configuration , b.f_solution_Configuration ) &&
	 same( a.f_extra_Configuration , b.f_extra_Configuration ) );
 }

/*--------------------------------------------------------------------------*/
/// true if the two Configuration are equal, for the types used here

static bool same( const Configuration * a , const Configuration * b )
{
 if( ( ! a ) || ( ! b ) )
  return( a == b );
 if( a->classname() != b->classname() )
  return( false );
 if( auto x = dynamic_cast< const SC_int * >( a ) )
  return( x->f_value == static_cast< const SC_int * >( b )->f_value );
 if( auto x = dynamic_cast< const SC_dbl * >( a ) )
  return( x->f_value == static_cast< const SC_dbl * >( b )->f_value );
 if( auto x = dynamic_cast< const ComputeConfig * >( a ) )
  return( same_cc( *x , *static_cast< const ComputeConfig * >( b ) ) );
 if( auto x = dynamic_cast< const BlockConfig * >( a ) ) {
  auto y = static_cast< const BlockConfig * >( b );
  if( ! same_bc( *x , *y ) )
   return( false );
  if( auto o = dynamic_cast< const BlockConfigHandlers::OHandler * >( x ) )
   if( ! same( o->get_Config_Objective() ,
	       dynamic_cast< const BlockConfigHandlers::OHandler * >( y )
	       ->get_Config_Objective() ) )
    return( false );
  if( auto c = dynamic_cast< const BlockConfigHandlers::CHandler * >( x ) ) {
   auto d = dynamic_cast< const BlockConfigHandlers::CHandler * >( y );
   if( c->num_ComputeConfig_Constraint() != d->num_ComputeConfig_Constraint() )
    return( false );
   for( Block::Index i = 0 ; i < c->num_ComputeConfig_Constraint() ; ++i )
    if( ( c->get_Constraint_id( i ) != d->get_Constraint_id( i ) ) ||
	( ! same( c->get_ComputeConfig_Constraint( i ) ,
		  d->get_ComputeConfig_Constraint( i ) ) ) )
     return( false );
   }
  if( auto r = dynamic_cast< const BlockConfigHandlers::RHandler * >( x ) ) {
   auto s = dynamic_cast< const BlockConfigHandlers::RHandler * >( y );
   if( r->num_sub_BlockConfig() != s->num_sub_BlockConfig() )
    return( false );
   for( Block::Index i = 0 ; i < r->num_sub_BlockConfig() ; ++i )
    if( ( r->get_sub_Block_id( i ) != s->get_sub_Block_id( i ) ) ||
	( ! same( r->get_sub_BlockConfig( i ) , s->get_sub_BlockConfig( i ) ) ) )
     return( false );
   }
  return( true );
  }
 if( auto x = dynamic_cast< const BlockSolverConfig * >( a ) ) {
  auto y = static_cast< const BlockSolverConfig * >( b );
  if( ( x->is_diff() != y->is_diff() ) ||
      ( x->get_SolverNames() != y->get_SolverNames() ) ||
      ( x->num_ComputeConfig() != y->num_ComputeConfig() ) )
   return( false );
  for( Block::Index i = 0 ; i < x->num_ComputeConfig() ; ++i )
   if( ! same( x->get_SolverConfig( i ) , y->get_SolverConfig( i ) ) )
    return( false );
  if( auto r = dynamic_cast< const RBlockSolverConfig * >( x ) ) {
   auto s = static_cast< const RBlockSolverConfig * >( y );
   if( r->num_BlockSolverConfig() != s->num_BlockSolverConfig() )
    return( false );
   for( Block::Index i = 0 ; i < r->num_BlockSolverConfig() ; ++i )
    if( ( r->get_sub_Block_id( i ) != s->get_sub_Block_id( i ) ) ||
	( ! same( r->get_BlockSolverConfig( i ) ,
		  s->get_BlockSolverConfig( i ) ) ) )
     return( false );
   }
  return( true );
  }
 assert( false );  // a type the tests do not use
 return( false );
 }

/*--------------------------------------------------------------------------*/
/// a ThinComputeInterface with two int and one double parameters
/** The parameters are "intA" (default 10), "intB" (default 20) and "dblC"
 * (default 0.5): enough to see what set_ComputeConfig() does with a
 * differential ComputeConfig and with one that is not. */

class Params : public ThinComputeInterface
{
 public:

 Params( void ) : f_int{ 10 , 20 } , f_dbl( 0.5 ) {}

 int compute( bool changedvars = true ) override { return( kOK ); }

 using ThinComputeInterface::set_par;
 using ThinComputeInterface::get_int_par;
 using ThinComputeInterface::get_dbl_par;

 [[nodiscard]] idx_type get_num_int_par( void ) const override {
  return( 2 );
  }
 [[nodiscard]] idx_type get_num_dbl_par( void ) const override {
  return( 1 );
  }
 [[nodiscard]] int get_dflt_int_par( idx_type par ) const override {
  return( par == 0 ? 10 : 20 );
  }
 [[nodiscard]] double get_dflt_dbl_par( idx_type par ) const override {
  return( 0.5 );
  }
 [[nodiscard]] idx_type int_par_str2idx( const std::string & name )
  const override {
  return( name == "intA" ? 0 : name == "intB" ? 1 : Inf< idx_type >() );
  }
 [[nodiscard]] idx_type dbl_par_str2idx( const std::string & name )
  const override {
  return( name == "dblC" ? 0 : Inf< idx_type >() );
  }
 [[nodiscard]] const std::string & int_par_idx2str( idx_type idx )
  const override {
  static const std::string n[] = { "intA" , "intB" };
  return( n[ idx ] );
  }
 [[nodiscard]] const std::string & dbl_par_idx2str( idx_type idx )
  const override {
  static const std::string n = "dblC";
  return( n );
  }
 void set_par( idx_type par , int value ) override { f_int[ par ] = value; }
 void set_par( idx_type par , double value ) override { f_dbl = value; }
 [[nodiscard]] int get_int_par( idx_type par ) const override {
  return( f_int[ par ] );
  }
 [[nodiscard]] double get_dbl_par( idx_type par ) const override {
  return( f_dbl );
  }

 int f_int[ 2 ];
 double f_dbl;
 };

/*--------------------------------------------------------------------------*/
/* The name given to the factory, with or without blanks, and the names and
 * values it refuses. */

static void test_factory_names( void )
{
 // the factory ignores the blanks in the name
 auto c = Configuration::new_Configuration( " Simple Configuration < int > " );
 assert( dynamic_cast< SC_int * >( c ) );
 delete c;

 // a name nobody registered is refused, and so is a value that is not there
 assert( throws< std::invalid_argument >( [](){
    delete from_txt( "SimpleConfiguration<float> 1" ); } ) );
 assert( throws< std::invalid_argument >( [](){
    delete from_txt( "SimpleConfiguration<int> abc" ); } ) );

 std::cout << "names in the factory: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* SimpleConfiguration of the pairs of numbers and of the vector of pairs,
 * in text and in netCDF. */

static void test_pairs_and_vectors( void )
{
 {
  using T = SimpleConfiguration< std::pair< int , int > >;
  auto c = text_as< T >( "SimpleConfiguration<std::pair<int,int>> 3 -4" );
  assert( c && ( c->f_value == std::make_pair( 3 , -4 ) ) );
  auto n = nc_as< T >( *c );
  assert( n && ( n->f_value == c->f_value ) );
  delete n; delete c;
  }
 {
  using T = SimpleConfiguration< std::pair< double , double > >;
  auto c = text_as< T >(
		     "SimpleConfiguration<std::pair<double,double>> 0.5 0" );
  assert( c && ( c->f_value == std::make_pair( 0.5 , 0.0 ) ) );
  auto n = nc_as< T >( *c );
  assert( n && ( n->f_value == c->f_value ) );
  delete n; delete c;
  }
 {
  using T = SimpleConfiguration< std::pair< int , double > >;
  auto c = text_as< T >( "SimpleConfiguration<std::pair<int,double>> 1 2.5" );
  assert( c && ( c->f_value == std::make_pair( 1 , 2.5 ) ) );
  auto n = nc_as< T >( *c );
  assert( n && ( n->f_value == c->f_value ) );
  delete n; delete c;
  }
 {
  using T = SimpleConfiguration< std::pair< double , int > >;
  auto c = text_as< T >( "SimpleConfiguration<std::pair<double,int>> 2.5 1" );
  assert( c && ( c->f_value == std::make_pair( 2.5 , 1 ) ) );
  auto n = nc_as< T >( *c );
  assert( n && ( n->f_value == c->f_value ) );
  delete n; delete c;
  }

 using VI = SimpleConfiguration< std::vector< int > >;
 using VP = SimpleConfiguration< std::vector< std::pair< int , int > > >;

 {
  auto c = text_as< VP >(
	   "SimpleConfiguration<std::vector<std::pair<int,int>>> 2 1 2 3 4" );
  assert( c && ( c->f_value == std::vector< std::pair< int , int > >(
					     { { 1 , 2 } , { 3 , 4 } } ) ) );
  auto n = nc_as< VP >( *c );
  assert( n && ( n->f_value == c->f_value ) );
  delete n; delete c;
  }

 // printing a pair or a vector is for a human reader, and does not throw
 {
  std::ostringstream out;
  SimpleConfiguration< std::pair< int , int > > p( { 1 , 2 } );
  VI v( std::vector< int >( { 1 , 2 } ) );
  out << p << v;
  assert( ! out.str().empty() );
  }

 std::cout << "pairs and vectors: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* SimpleConfiguration holding other Configuration: the pairs whose second
 * element is a Configuration, the vector of them and the vector of pairs;
 * the '*' giving no Configuration, the clone() being deep. */

static void test_nested( void )
{
 {
  using T = SimpleConfiguration< std::pair< int , Configuration * > >;
  auto c = text_as< T >( "SimpleConfiguration<std::pair<int,Configuration*>> "
			 "5 SimpleConfiguration<double> 2.5" );
  assert( c && ( c->f_value.first == 5 ) );
  assert( value_of< double >( c->f_value.second ) == 2.5 );

  auto n = nc_as< T >( *c );
  assert( n && ( n->f_value.first == 5 ) );
  assert( same( n->f_value.second , c->f_value.second ) );

  auto k = static_cast< T * >( c->clone() );
  assert( k->f_value.second != c->f_value.second );
  assert( same( k->f_value.second , c->f_value.second ) );
  delete k; delete n; delete c;

  // '*' followed by nothing, or by a blank, is no Configuration
  c = text_as< T >( "SimpleConfiguration<std::pair<int,Configuration*>> 7 *" );
  assert( c && ( c->f_value.first == 7 ) && ( ! c->f_value.second ) );
  n = nc_as< T >( *c );
  assert( n && ( n->f_value.first == 7 ) && ( ! n->f_value.second ) );
  delete n; delete c;
  }
 {
  using T = SimpleConfiguration< std::pair< double , Configuration * > >;
  auto c = text_as< T >( "SimpleConfiguration<std::pair<double,"
			 "Configuration*>> 0 SimpleConfiguration<int> 3" );
  assert( c && ( c->f_value.first == 0 ) );
  assert( value_of< int >( c->f_value.second ) == 3 );
  auto n = nc_as< T >( *c );
  assert( n && ( n->f_value.first == 0 ) );
  assert( same( n->f_value.second , c->f_value.second ) );
  delete n; delete c;
  }
 {
  using T = SimpleConfiguration< std::pair< std::string , Configuration * > >;
  auto c = text_as< T >( "SimpleConfiguration<std::pair<std::string,"
			 "Configuration*>> name SimpleConfiguration<int> 3" );
  assert( c && ( c->f_value.first == "name" ) );
  assert( value_of< int >( c->f_value.second ) == 3 );
  auto n = nc_as< T >( *c );
  assert( n && ( n->f_value.first == "name" ) );
  assert( same( n->f_value.second , c->f_value.second ) );
  delete n; delete c;
  }
 {
  // a pair of pairs: a Configuration nested two levels down
  using T = SimpleConfiguration< std::pair< Configuration * ,
					    Configuration * > >;
  auto c = text_as< T >(
   "SimpleConfiguration<std::pair<Configuration*,Configuration*>>\n"
   " SimpleConfiguration<std::pair<int,Configuration*>> 4\n"
   "  SimpleConfiguration<double> -1\n"
   " SimpleConfiguration<int> 2\n" );
  assert( c );
  auto in = dynamic_cast< SimpleConfiguration< std::pair< int ,
		       Configuration * > > * >( c->f_value.first );
  assert( in && ( in->f_value.first == 4 ) );
  assert( value_of< double >( in->f_value.second ) == -1 );
  assert( value_of< int >( c->f_value.second ) == 2 );

  auto n = nc_as< T >( *c );
  assert( n );
  auto nin = dynamic_cast< SimpleConfiguration< std::pair< int ,
			Configuration * > > * >( n->f_value.first );
  assert( nin && ( nin->f_value.first == 4 ) );
  assert( same( nin->f_value.second , in->f_value.second ) );
  assert( same( n->f_value.second , c->f_value.second ) );
  delete n; delete c;
  }
 {
  using T = SimpleConfiguration< std::vector< Configuration * > >;
  auto c = text_as< T >( "SimpleConfiguration<std::vector<Configuration*>> 3 "
			 "SimpleConfiguration<int> 1 * "
			 "SimpleConfiguration<double> 2" );
  assert( c && ( c->f_value.size() == 3 ) );
  assert( value_of< int >( c->f_value[ 0 ] ) == 1 );
  assert( ! c->f_value[ 1 ] );
  assert( value_of< double >( c->f_value[ 2 ] ) == 2 );

  auto k = static_cast< T * >( c->clone() );
  assert( ( k->f_value.size() == 3 ) && ( ! k->f_value[ 1 ] ) );
  assert( ( k->f_value[ 0 ] != c->f_value[ 0 ] ) &&
	  same( k->f_value[ 0 ] , c->f_value[ 0 ] ) );
  delete k;

  auto e = text_as< T >(
		   "SimpleConfiguration<std::vector<Configuration*>> 0" );
  assert( e && e->f_value.empty() );
  delete e;

  // read out of text, the '*' element comes back as nullptr from netCDF
  auto m = nc_as< T >( *c );
  assert( m && ( m->f_value.size() == 3 ) && ( ! m->f_value[ 1 ] ) );
  delete m;
  delete c;
  }
 {
  using T = SimpleConfiguration< std::vector< std::pair< int ,
							 Configuration * > > >;
  auto c = text_as< T >(
	 "SimpleConfiguration<std::vector<std::pair<int,Configuration*>>> 2 "
	 "1 SimpleConfiguration<int> 10 2 *" );
  assert( c && ( c->f_value.size() == 2 ) );
  assert( ( c->f_value[ 0 ].first == 1 ) &&
	  ( value_of< int >( c->f_value[ 0 ].second ) == 10 ) );
  assert( ( c->f_value[ 1 ].first == 2 ) && ( ! c->f_value[ 1 ].second ) );

  auto n = nc_as< T >( *c );
  assert( n && ( n->f_value.size() == 2 ) );
  assert( ( n->f_value[ 0 ].first == 1 ) &&
	  same( n->f_value[ 0 ].second , c->f_value[ 0 ].second ) );
  assert( ( n->f_value[ 1 ].first == 2 ) && ( ! n->f_value[ 1 ].second ) );
  delete n; delete c;

  auto e = text_as< T >(
	 "SimpleConfiguration<std::vector<std::pair<int,Configuration*>>> 0" );
  assert( e && e->f_value.empty() );
  auto ne = nc_as< T >( *e );
  assert( ne && ne->f_value.empty() );
  delete ne; delete e;
  }

 std::cout << "nested Configuration: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The meta-configuration mapping a classname to a Configuration, i.e., the
 * SimpleConfiguration< std::map< std::string , Configuration * > > that
 * dispatches a Configuration to each :Block of a tree by classname. */

static void test_meta_configuration( void )
{
 auto c = text_as< SC_map >(
	   "SimpleConfiguration<std::map<std::string,Configuration*>>\n"
	   "2  # two classes\n"
	   "A SimpleConfiguration<int> 1  # the first\n"
	   "B *                           # the second has none\n" );
 assert( c && ( c->f_value.size() == 2 ) );
 assert( value_of< int >( c->f_value.at( "A" ) ) == 1 );
 assert( c->f_value.count( "B" ) && ( ! c->f_value.at( "B" ) ) );

 // printing is for a human reader, and does not throw
 std::ostringstream out;
 out << *c;
 assert( out.str().find( "A" ) != std::string::npos );

 auto e = text_as< SC_map >(
		  "SimpleConfiguration<std::map<std::string,Configuration*>> 0" );
 assert( e && e->f_value.empty() );
 auto ne = nc_as< SC_map >( *e );
 assert( ne && ne->f_value.empty() );
 delete ne; delete e;
 delete c;

 std::cout << "meta-configuration: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A MetaBlockSolverConfig configures the Block it is apply()-ed to with its
 * own fields and the descendants, not the Block itself, with the map, by
 * classname() and "*" for the others; cleared, it removes all it has
 * registered; the map goes through clone() and netCDF, a missing map is
 * none, and an entry that is not a BlockSolverConfig throws. */

static void test_MetaBlockSolverConfig( void )
{
 AbstractBlock root;
 auto child = new AbstractBlock( & root );
 root.add_nested_Block( child );
 auto grand = new AbstractBlock( child );
 child->add_nested_Block( grand );

 const std::string map_head =
                "SimpleConfiguration<std::map<std::string,Configuration*>> ";
 auto c = from_text( "MetaBlockSolverConfig 2  1 FakeSolver  0\n" + map_head +
		     "1  AbstractBlock BlockSolverConfig 2  2 FakeSolver "
		     "FakeSolver  0\n" );
 auto m = dynamic_cast< MetaBlockSolverConfig * >( c );
 expect( m && m->get_map() && ( m->get_map()->f_value.size() == 1 ) ,
	 "MetaBlockSolverConfig: the map is read" );
 if( ! m ) {
  delete c;
  return;
  }

 m->apply( & root );
 expect( ( root.get_registered_solvers().size() == 1 ) &&
	 ( child->get_registered_solvers().size() == 2 ) &&
	 ( grand->get_registered_solvers().size() == 2 ) ,
	 "MetaBlockSolverConfig: own fields on the Block, map on the rest" );

 auto k = m->clone();
 expect( k->get_map() && ( k->get_map() != m->get_map() ) &&
	 ( k->get_map()->f_value.size() == 1 ) &&
	 ( k->get_map()->f_value.begin()->second !=
	   m->get_map()->f_value.begin()->second ) ,
	 "MetaBlockSolverConfig: clone() copies the map" );
 delete k;

 auto n = nc_as< MetaBlockSolverConfig >( *m );
 expect( n && n->get_map() && n->get_map()->f_value.count( "AbstractBlock" ) &&
	 dynamic_cast< BlockSolverConfig * >(
		     n->get_map()->f_value.at( "AbstractBlock" ) ) ,
	 "MetaBlockSolverConfig: the map goes through netCDF" );
 delete n;

 m->clear();
 m->apply( & root );
 expect( root.get_registered_solvers().empty() &&
	 child->get_registered_solvers().empty() &&
	 grand->get_registered_solvers().empty() ,
	 "MetaBlockSolverConfig: cleared, it removes all it registered" );
 delete m;

 // "*" for the classname() that are not in the map
 c = from_text( "MetaBlockSolverConfig 2  0  0\n" + map_head +
		"2  NoSuchBlock BlockSolverConfig 2  2 FakeSolver FakeSolver"
		"  0"
		"   * BlockSolverConfig 2  1 FakeSolver  0\n" );
 m = dynamic_cast< MetaBlockSolverConfig * >( c );
 expect( m , "MetaBlockSolverConfig: read with a \"*\" entry" );
 if( m ) {
  m->apply( & root );
  expect( root.get_registered_solvers().empty() &&
	  ( child->get_registered_solvers().size() == 1 ) &&
	  ( grand->get_registered_solvers().size() == 1 ) ,
	  "MetaBlockSolverConfig: \"*\" for the others" );
  m->clear();
  m->apply( & root );
  expect( child->get_registered_solvers().empty() &&
	  grand->get_registered_solvers().empty() ,
	  "MetaBlockSolverConfig: \"*\" entries are removed by the cleared" );
  }
 delete c;

 // no map, and "*" for no map
 for( std::string tail : { "" , " *" } ) {
  c = from_text( "MetaBlockSolverConfig 2  1 FakeSolver  0" + tail );
  m = dynamic_cast< MetaBlockSolverConfig * >( c );
  expect( m && ( ! m->get_map() ) , "MetaBlockSolverConfig: no map" );
  delete c;
  }

 // an entry that is not a BlockSolverConfig
 expect( throws< std::invalid_argument >( [ & ]() {
          delete from_text( "MetaBlockSolverConfig 2  0  0\n" + map_head +
			    "1  AbstractBlock SimpleConfiguration<int> 1\n" );
	  } ) , "MetaBlockSolverConfig: an entry of the wrong type throws" );

 std::cout << "MetaBlockSolverConfig: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A MetaBlockConfig configures the Block it is apply()-ed to with its own
 * fields and the descendants with the map, through copies, so that the map
 * is unchanged and can be apply()-ed again. */

static void test_MetaBlockConfig( void )
{
 const std::string slots = " *  *  *  *  *  *  *  *  * ";
 const std::string map_head =
                "SimpleConfiguration<std::map<std::string,Configuration*>> ";
 auto c = from_text( "MetaBlockConfig 1 2" + slots +
		     "SimpleConfiguration<int> 3\n" + map_head +
		     "1  AbstractBlock BlockConfig 1 2" + slots +
		     "SimpleConfiguration<int> 7\n" );
 auto m = dynamic_cast< MetaBlockConfig * >( c );
 expect( m && m->get_map() && ( m->get_map()->f_value.size() == 1 ) ,
	 "MetaBlockConfig: the map is read" );
 if( ! m ) {
  delete c;
  return;
  }

 auto extra_of = []( Block * b ) {
  auto bc = b->get_BlockConfig();
  auto sc = bc ? dynamic_cast< SimpleConfiguration< int > * >(
				     bc->f_extra_Configuration ) : nullptr;
  return( sc ? sc->f_value : -1 );
  };

 for( int round = 0 ; round < 2 ; ++round ) {
  AbstractBlock root;
  auto child = new AbstractBlock( & root );
  root.add_nested_Block( child );
  auto grand = new AbstractBlock( child );
  child->add_nested_Block( grand );

  auto k = m->clone();
  k->apply( & root );
  delete k;
  expect( ( extra_of( & root ) == 3 ) && ( extra_of( child ) == 7 ) &&
	  ( extra_of( grand ) == 7 ) ,
	  "MetaBlockConfig: own fields on the Block, map on descendants" );
  }

 auto n = nc_as< MetaBlockConfig >( *m );
 expect( n && n->get_map() && n->get_map()->f_value.count( "AbstractBlock" ) ,
	 "MetaBlockConfig: the map goes through netCDF" );
 delete n;
 delete m;

 expect( throws< std::invalid_argument >( [ & ]() {
          delete from_text( "MetaBlockConfig 1 2" + slots + "*\n" + map_head +
			    "1  AbstractBlock SimpleConfiguration<int> 1\n" );
	  } ) , "MetaBlockConfig: an entry of the wrong type throws" );

 std::cout << "MetaBlockConfig: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The "*file" of the text format inside a meta-configuration: a .txt file,
 * the Configuration in a given position of a netCDF file, and '*'. */

static void test_includes( void )
{
 write_tmp_file( "int.txt" ,
		 "# an included file\nSimpleConfiguration<int> 9\n" );

 // a netCDF file of two Configuration, the position in brackets
 const auto nc = file( "two.nc4" );
 SC_int first( 1 );
 SC_dbl second( 2.5 );
 static_cast< Configuration & >( first ).serialize( nc , eConfigFile );
 static_cast< Configuration & >( second ).serialize( nc , eConfigFile ,
						     false );
 // the includes inside a meta-configuration
 auto m = text_as< SC_map >(
	   "SimpleConfiguration<std::map<std::string,Configuration*>> 3 "
	   "A *" + file( "int.txt" ) + " B *" + nc + "[1] C *" );
 assert( m && ( m->f_value.size() == 3 ) );
 assert( value_of< int >( m->f_value.at( "A" ) ) == 9 );
 assert( value_of< double >( m->f_value.at( "B" ) ) == 2.5 );
 assert( ! m->f_value.at( "C" ) );
 delete m;

 std::cout << "includes: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* ComputeConfig: the deep copy, the printing and the extra Configuration
 * read out of another file. */

static const char * full_cc =
 "ComputeConfig\n"
 "1                  # differential\n"
 "2 intA 1 intB -2   # int\n"
 "1 dblC 0.5         # double\n"
 "1 strD hello       # string\n"
 "2 vintE 3 1 2 3    # vector of int\n"
 "  vintF 0          # an empty one\n"
 "1 vdblG 2 0.5 1.5  # vector of double\n"
 "1 vstrH 2 a b      # vector of string\n"
 "SimpleConfiguration<int> 7  # the extra Configuration\n";

static void check_full_cc( const ComputeConfig & c )
{
 assert( c.diff() && ( ! c.relax() ) );
 using IP = std::vector< std::pair< std::string , int > >;
 using DP = std::vector< std::pair< std::string , double > >;
 using SP = std::vector< std::pair< std::string , std::string > >;
 using VIP = std::vector< std::pair< std::string , std::vector< int > > >;
 using VDP = std::vector< std::pair< std::string , std::vector< double > > >;
 using VSP = std::vector< std::pair< std::string ,
				     std::vector< std::string > > >;
 assert( ( c.int_pars == IP( { { "intA" , 1 } , { "intB" , -2 } } ) ) );
 assert( ( c.dbl_pars == DP( { { "dblC" , 0.5 } } ) ) );
 assert( ( c.str_pars == SP( { { "strD" , "hello" } } ) ) );
 assert( ( c.vint_pars == VIP( { { "vintE" , { 1 , 2 , 3 } } ,
				 { "vintF" , {} } } ) ) );
 assert( ( c.vdbl_pars == VDP( { { "vdblG" , { 0.5 , 1.5 } } } ) ) );
 assert( ( c.vstr_pars == VSP( { { "vstrH" , { "a" , "b" } } } ) ) );
 assert( value_of< int >( c.f_extra_Configuration ) == 7 );
 }

static void test_ComputeConfig_copy( void )
{
 auto c = text_as< ComputeConfig >( full_cc );
 assert( c );
 check_full_cc( *c );

 // the copy is deep
 ComputeConfig copy( *c );
 check_full_cc( copy );
 assert( copy.f_extra_Configuration != c->f_extra_Configuration );
 auto k = c->clone();
 assert( same( k , c ) );
 delete k;

 // printing is for a human reader
 std::ostringstream out;
 out << *c;
 assert( out.str().find( "intA = 1" ) != std::string::npos );

 // the extra Configuration read out of another file
 write_tmp_file( "extra.txt" , "SimpleConfiguration<double> 4.5" );
 auto z = text_as< ComputeConfig >( "ComputeConfig 0 0 0 0 0 0 0 *" +
				    file( "extra.txt" ) );
 assert( z && ( value_of< double >( z->f_extra_Configuration ) == 4.5 ) );
 delete z;

 delete c;
 std::cout << "ComputeConfig copy, print, extra from a file: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* What applying a ComputeConfig does: a differential one changes only the
 * parameters it gives, one that is not first puts all of them back to
 * their default, nullptr puts all of them back to their default; a name
 * nobody knows is an error, unless the ComputeConfig is relaxed. */

static void test_ComputeConfig_apply( void )
{
 Params p;

 auto set = text_as< ComputeConfig >( "ComputeConfig 0 "
				      "2 intA 1 intB 2 1 dblC 3" );
 p.set_ComputeConfig( set );
 assert( ( p.f_int[ 0 ] == 1 ) && ( p.f_int[ 1 ] == 2 ) && ( p.f_dbl == 3 ) );

 auto diff = text_as< ComputeConfig >( "ComputeConfig 1 1 intB 7" );
 p.set_ComputeConfig( diff );
 assert( ( p.f_int[ 0 ] == 1 ) && ( p.f_int[ 1 ] == 7 ) && ( p.f_dbl == 3 ) );

 // a differential ComputeConfig with nothing in it changes nothing
 auto none = text_as< ComputeConfig >( "ComputeConfig 1 0 0 0 0 0 0 *" );
 p.set_ComputeConfig( none );
 assert( ( p.f_int[ 0 ] == 1 ) && ( p.f_int[ 1 ] == 7 ) && ( p.f_dbl == 3 ) );

 auto reset = text_as< ComputeConfig >( "ComputeConfig 0 1 intB 8" );
 p.set_ComputeConfig( reset );
 assert( ( p.f_int[ 0 ] == 10 ) && ( p.f_int[ 1 ] == 8 ) &&
	 ( p.f_dbl == 0.5 ) );

 p.set_ComputeConfig( set );
 p.set_ComputeConfig( nullptr );
 assert( ( p.f_int[ 0 ] == 10 ) && ( p.f_int[ 1 ] == 20 ) &&
	 ( p.f_dbl == 0.5 ) );

 // what the ThinComputeInterface gives back is what it was given
 p.set_ComputeConfig( set );
 auto got = p.get_ComputeConfig( true );
 assert( ( ! got->diff() ) && ( got->int_pars.size() == 2 ) &&
	 ( got->dbl_pars.size() == 1 ) );
 Params q;
 q.set_ComputeConfig( got );
 assert( ( q.f_int[ 0 ] == 1 ) && ( q.f_int[ 1 ] == 2 ) && ( q.f_dbl == 3 ) );
 delete got;

 // a name nobody knows
 auto unknown = text_as< ComputeConfig >( "ComputeConfig 1 1 intZ 1" );
 assert( throws< std::invalid_argument >( [ & ](){
    p.set_ComputeConfig( unknown ); } ) );
 auto relaxed = text_as< ComputeConfig >( "ComputeConfig 3 2 intZ 1 intA 5" );
 p.set_ComputeConfig( relaxed );
 assert( p.f_int[ 0 ] == 5 );

 delete relaxed; delete unknown; delete reset; delete none; delete diff;
 delete set;
 std::cout << "applying a ComputeConfig: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The differential override "*file.txt +" of the text format inside a
 * meta-configuration, whose next key is not the name of a Configuration:
 * the extra slot of the body can be given or left out. */

static void test_override_in_a_map( void )
{
 write_tmp_file( "base.txt" , "ComputeConfig 0\n"
		 "2 intA 1 intB 2\n"
		 "1 dblC 0.5\n"
		 "0 0 0 0\n"
		 "SimpleConfiguration<int> 3\n" );

 // a full body, extra slot included, inside a meta-configuration
 auto m = text_as< SC_map >(
	   "SimpleConfiguration<std::map<std::string,Configuration*>> 2 "
	   "A *" + file( "base.txt" ) + " + 1 1 intB 6 0 0 0 0 0 "
	   "SimpleConfiguration<int> 5 "
	   "B SimpleConfiguration<int> 4" );
 assert( m && ( m->f_value.size() == 2 ) );
 auto a = dynamic_cast< ComputeConfig * >( m->f_value.at( "A" ) );
 assert( a && ( a->int_pars.size() == 2 ) &&
	 ( a->int_pars[ 1 ].second == 6 ) );
 assert( value_of< int >( a->f_extra_Configuration ) == 5 );
 assert( value_of< int >( m->f_value.at( "B" ) ) == 4 );
 delete m;

 // the extra slot of the body is optional also inside a
 // meta-configuration: the next key of the map is not the name of a
 // Configuration, and is left to the map
 m = text_as< SC_map >(
	   "SimpleConfiguration<std::map<std::string,Configuration*>> 2 "
	   "A *" + file( "base.txt" ) + " + 1 1 intB 6 0 0 0 0 0 "
	   "B SimpleConfiguration<int> 4" );
 assert( m && ( m->f_value.size() == 2 ) );
 assert( value_of< int >( m->f_value.at( "B" ) ) == 4 );
 delete m;

 std::cout << "override in a meta-configuration: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* BlockConfig: the ten slots compared field by field after the netCDF round
 * trip, the copy, the move and print(), which writes what load() reads. */

static const char * full_bc =
 "BlockConfig\n"
 "1 2                           # differential, version 2\n"
 "*                             # structure\n"
 "SimpleConfiguration<int> 1    # static constraints\n"
 "*                             # dynamic constraints\n"
 "SimpleConfiguration<double> 2.5 # static variables\n"
 "*                             # dynamic variables\n"
 "SimpleConfiguration<int> 3    # objective\n"
 "*                             # is_feasible\n"
 "*                             # is_optimal\n"
 "*                             # solution\n"
 "SimpleConfiguration<int> 9    # extra\n";

static void test_BlockConfig_fields( void )
{
 auto c = text_as< BlockConfig >( full_bc );
 assert( c && c->is_diff() && ( ! c->empty() ) );
 assert( ! c->f_structure_Configuration );
 assert( value_of< int >( c->f_static_constraints_Configuration ) == 1 );
 assert( ! c->f_dynamic_constraints_Configuration );
 assert( value_of< double >( c->f_static_variables_Configuration ) == 2.5 );
 assert( ! c->f_dynamic_variables_Configuration );
 assert( value_of< int >( c->f_objective_Configuration ) == 3 );
 assert( ! c->f_is_feasible_Configuration );
 assert( ! c->f_is_optimal_Configuration );
 assert( ! c->f_solution_Configuration );
 assert( value_of< int >( c->f_extra_Configuration ) == 9 );

 auto n = nc_as< BlockConfig >( *c );
 assert( n && same( n , c ) );
 delete n;

 BlockConfig copy( *c );
 assert( same( & copy , c ) );
 assert( copy.f_extra_Configuration != c->f_extra_Configuration );

 // print() writes the format that load() reads, which reads back the
 // same BlockConfig
 std::ostringstream out;
 out << c->classname() << " " << *c;
 auto p = text_as< BlockConfig >( out.str() );
 assert( p && same( p , c ) );
 delete p;

 // the move constructor takes all the ten Configuration, the structure
 // one included, and leaves none in the moved-from BlockConfig
 BlockConfig moved( std::move( copy ) );
 assert( same( & moved , c ) );
 assert( ! copy.f_structure_Configuration );

 // a slot read out of another file
 auto e = text_as< BlockConfig >( "BlockConfig 1 2 * *" + file( "int.txt" ) );
 assert( e && ( value_of< int >( e->f_static_constraints_Configuration ) ==
		9 ) );
 delete e;

 // the structure slot is the first one
 e = text_as< BlockConfig >( "BlockConfig 1 2 SimpleConfiguration<int> 4" );
 assert( e && ( value_of< int >( e->f_structure_Configuration ) == 4 ) );
 auto ns = nc_as< BlockConfig >( *e );
 assert( ns && same( ns , e ) );
 delete ns; delete e;

 delete c;
 std::cout << "BlockConfig field by field: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A differential BlockConfig with nothing in it changes nothing of the
 * BlockConfig of a Block, and what is read back from the Block is what is
 * there. */

static void test_BlockConfig_apply_nothing( void )
{
 AbstractBlock block;

 auto full = text_as< BlockConfig >( "BlockConfig 0 2 * "
				     "SimpleConfiguration<int> 3 * * * * * * * "
				     "SimpleConfiguration<int> 2" );
 full->apply( & block );

 // a differential one with nothing in it changes nothing
 auto none = text_as< BlockConfig >( "BlockConfig 1 2" );
 none->apply( & block );
 auto bc = block.get_BlockConfig();
 assert( value_of< int >( bc->f_static_constraints_Configuration ) == 3 );
 assert( value_of< int >( bc->f_extra_Configuration ) == 2 );

 // and what is read back from the Block is what is there
 BlockConfig got( & block );
 assert( same( & got , bc ) );

 delete none; delete full;
 std::cout << "applying an empty differential BlockConfig: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The :BlockConfig with the handlers of the Objective (O), of the
 * Constraint (C) and of the sub-Block (R), alone and together. */

static const std::string empty_slots = " * * * * * * * * * * ";

static void test_handlers( void )
{
 // the Objective
 {
  auto c = text_as< OBlockConfig >( "OBlockConfig 1 2" + empty_slots +
				    "ComputeConfig 1 1 intA 3 0 0 0 0 0 *" );
  assert( c && c->get_Config_Objective() );
  assert( c->get_Config_Objective()->int_pars[ 0 ].second == 3 );
  auto n = nc_as< OBlockConfig >( *c );
  assert( n && ( n->get_Config_Objective() ) &&
	  ( n->get_Config_Objective()->int_pars[ 0 ].second == 3 ) );
  delete n;

  auto k = c->clone();
  assert( same( k , c ) );
  delete k;

  // nothing after the BlockConfig: no ComputeConfig for the Objective
  auto e = text_as< OBlockConfig >( "OBlockConfig 1 2" + empty_slots );
  assert( e && ( ! e->get_Config_Objective() ) );
  auto ne = nc_as< OBlockConfig >( *e );
  assert( ne && ( ! ne->get_Config_Objective() ) );
  delete ne; delete e;

  // a '*' is no ComputeConfig for the Objective
  e = text_as< OBlockConfig >( "OBlockConfig 1 2" + empty_slots + "*" );
  assert( e && ( ! e->get_Config_Objective() ) );
  delete e;
  delete c;
  }

 // the Constraint
 {
  auto c = text_as< CBlockConfig >( "CBlockConfig 1 2" + empty_slots +
				    "2 rows 0 ComputeConfig 1 1 intA 1 0 0 0 0 0 *"
				    "  rows 3 ComputeConfig 1 1 intA 2 0 0 0 0 0 *" );
  assert( c && ( c->num_ComputeConfig_Constraint() == 2 ) );
  assert( ( c->get_Constraint_id( 0 ) ==
	    std::pair< std::string , Block::Index >( "rows" , 0 ) ) );
  assert( ( c->get_Constraint_id( 1 ) ==
	    std::pair< std::string , Block::Index >( "rows" , 3 ) ) );
  assert( c->get_ComputeConfig_Constraint( 1 )->int_pars[ 0 ].second == 2 );

  auto n = nc_as< CBlockConfig >( *c );
  assert( n && ( n->num_ComputeConfig_Constraint() == 2 ) );
  for( Block::Index i = 0 ; i < 2 ; ++i ) {
   assert( n->get_Constraint_id( i ).second ==
	   c->get_Constraint_id( i ).second );
   assert( n->get_ComputeConfig_Constraint( i )->int_pars[ 0 ].second ==
	   c->get_ComputeConfig_Constraint( i )->int_pars[ 0 ].second );
   }
  // the netCDF format keeps the name of the group of each Constraint
  for( Block::Index i = 0 ; i < 2 ; ++i )
   assert( n->get_Constraint_id( i ) == c->get_Constraint_id( i ) );
  delete n;

  // the numeric identification of a group of Constraint
  auto d = text_as< CBlockConfig >( "CBlockConfig 1 2" + empty_slots +
				    "1 0 1 ComputeConfig 1 0 0 0 0 0 0 *" );
  assert( d && ( d->get_Constraint_id( 0 ) ==
		 std::pair< std::string , Block::Index >( "0" , 1 ) ) );
  delete d;

  auto e = text_as< CBlockConfig >( "CBlockConfig 1 2" + empty_slots + "0" );
  assert( e && ( ! e->num_ComputeConfig_Constraint() ) );
  auto ne = nc_as< CBlockConfig >( *e );
  assert( ne && ( ! ne->num_ComputeConfig_Constraint() ) );
  delete ne; delete e;

  // a '*' is no ComputeConfig for a Constraint
  e = text_as< CBlockConfig >( "CBlockConfig 1 2" + empty_slots +
			       "1 rows 0 *" );
  assert( e && ( e->num_ComputeConfig_Constraint() == 1 ) &&
	  ( ! e->get_ComputeConfig_Constraint( 0 ) ) );
  delete e;
  delete c;
  }

 // the sub-Block, by name and by position, one of them with none
 {
  auto c = text_as< RBlockConfig >( "RBlockConfig 1 2" + empty_slots +
				    "-2 sub1 BlockConfig 1 2 * "
				    "SimpleConfiguration<int> 4 * * * * * * * *"
				    " sub2 *" );
  assert( c && ( c->num_sub_BlockConfig() == 2 ) );
  assert( c->get_sub_Block_id( 0 ) == "sub1" );
  assert( c->get_sub_Block_id( 1 ) == "sub2" );
  assert( value_of< int >( c->get_sub_BlockConfig( 0 )
			   ->f_static_constraints_Configuration ) == 4 );
  assert( ! c->get_sub_BlockConfig( 1 ) );

  // the netCDF round trip of one sub-Block
  {
   auto one = text_as< RBlockConfig >( "RBlockConfig 1 2" + empty_slots +
				       "-1 sub1 BlockConfig 1 2 * "
				       "SimpleConfiguration<int> 4 * * * * * * * *" );
   assert( one && ( one->num_sub_BlockConfig() == 1 ) );
   auto n = nc_as< RBlockConfig >( *one );
   assert( n && ( n->num_sub_BlockConfig() == 1 ) );
   assert( same( n->get_sub_BlockConfig( 0 ) ,
		 one->get_sub_BlockConfig( 0 ) ) );
   delete n;
   delete one;
   }
  // the netCDF format keeps the ids of the sub-Block
  {
   auto n = nc_as< RBlockConfig >( *c );
   assert( n && ( n->num_sub_BlockConfig() == 2 ) );
   assert( same( n->get_sub_BlockConfig( 0 ) ,
		 c->get_sub_BlockConfig( 0 ) ) );
   assert( ! n->get_sub_BlockConfig( 1 ) );
   assert( n->get_sub_Block_id( 0 ) == "sub1" );
   assert( n->get_sub_Block_id( 1 ) == "sub2" );
   delete n;
   }

  auto p = text_as< RBlockConfig >( "RBlockConfig 0 2" + empty_slots +
				    "1 BlockConfig 0 2" + empty_slots );
  assert( p && ( p->num_sub_BlockConfig() == 1 ) );
  assert( p->get_sub_Block_id( 0 ) == "0" );
  assert( p->get_sub_BlockConfig( 0 ) &&
	  p->get_sub_BlockConfig( 0 )->empty() );
  delete p;

  auto e = text_as< RBlockConfig >( "RBlockConfig 1 2" + empty_slots + "0" );
  assert( e && ( ! e->num_sub_BlockConfig() ) );
  auto ne = nc_as< RBlockConfig >( *e );
  assert( ne && ( ! ne->num_sub_BlockConfig() ) );
  delete ne; delete e;
  delete c;
  }

 // all three together, in the order O, C, R
 {
  auto c = text_as< OCRBlockConfig >( "OCRBlockConfig 1 2" + empty_slots +
				      "ComputeConfig 1 1 intA 3 0 0 0 0 0 * "
				      "1 0 0 ComputeConfig 1 0 0 0 0 0 0 * "
				      "1 BlockConfig 1 2" + empty_slots );
  assert( c && c->get_Config_Objective() &&
	  ( c->num_ComputeConfig_Constraint() == 1 ) &&
	  ( c->num_sub_BlockConfig() == 1 ) );
  auto n = nc_as< OCRBlockConfig >( *c );
  assert( n && n->get_Config_Objective() &&
	  ( n->num_ComputeConfig_Constraint() == 1 ) &&
	  ( n->num_sub_BlockConfig() == 1 ) );
  assert( n->get_Config_Objective()->int_pars[ 0 ].second == 3 );
  delete n; delete c;
  }

 // the other combinations are there, and read what they are given
 for( const std::string & name : { "OCBlockConfig" , "ORBlockConfig" ,
				   "CRBlockConfig" } ) {
  // the handlers are the letters before "BlockConfig"
  const auto h = name.substr( 0 , 2 );
  std::string text = name + " 1 2" + empty_slots;
  if( h.find( 'O' ) != std::string::npos )
   text += "ComputeConfig 1 1 intA 3 0 0 0 0 0 * ";
  if( h.find( 'C' ) != std::string::npos )
   text += "1 0 0 ComputeConfig 1 0 0 0 0 0 0 * ";
  if( h.find( 'R' ) != std::string::npos )
   text += "1 BlockConfig 1 2" + empty_slots;
  auto c = from_txt( text );
  assert( c && ( c->classname() == name ) );
  auto n = nc_back( *c );
  assert( n && ( n->classname() == name ) );
  delete n; delete c;
  }

 std::cout << "handlers of a BlockConfig: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* BlockSolverConfig and RBlockSolverConfig: the names with no Solver, more
 * ComputeConfig than names, the eProbFile written by serialize( NcFile ),
 * the sub-Block by name and by position, and '*' for no BlockSolverConfig
 * of a sub-Block. */

static void test_BlockSolverConfig_fields( void )
{
 auto c = text_as< BlockSolverConfig >(
	  "BlockSolverConfig\n"
	  "1                   # differential\n"
	  "2 BoxSolver *       # two Solver, the second with no name\n"
	  "2                   # two ComputeConfig\n"
	  "ComputeConfig 1 1 intPDSol 3 0 0 0 0 0 *\n"
	  "*\n" );
 assert( c && ( c->is_diff() == BlockSolverConfig::eDiffMode ) );
 assert( ( c->get_SolverNames() ==
	   std::vector< std::string >( { "BoxSolver" , "" } ) ) );
 assert( c->get_SolverConfig( 0 ) && ( ! c->get_SolverConfig( 1 ) ) );
 assert( c->get_SolverConfig( 0 )->int_pars[ 0 ].second == 3 );

 auto n = nc_as< BlockSolverConfig >( *c );
 assert( n && ( n->is_diff() == c->is_diff() ) );
 assert( n->get_SolverNames() == c->get_SolverNames() );
 assert( n->get_SolverConfig( 0 ) && ( ! n->get_SolverConfig( 1 ) ) );
 assert( n->get_SolverConfig( 0 )->int_pars[ 0 ].second == 3 );
 delete n;

 auto k = c->clone();
 assert( same( k , c ) );
 delete k;

 // the eProbFile the BlockSolverConfig writes is the one it reads
 {
  const auto fn = file( "bsc.nc4" );
  {
   netCDF::NcFile f( fn , netCDF::NcFile::replace );
   f.putAtt( "SMS++_file_type" , netCDF::NcInt() , eProbFile );
   c->serialize( f , eProbFile );
   }
  netCDF::NcFile f( fn , netCDF::NcFile::read );
  auto r = BlockSolverConfig::deserialize( f , 0 );
  assert( r && ( r->get_SolverNames() == c->get_SolverNames() ) );
  delete r;
  }

 // more ComputeConfig than names
 auto f = text_as< BlockSolverConfig >( "BlockSolverConfig 0 1 A 2 * "
				   "ComputeConfig 0 0 0 0 0 0 0 *" );
 assert( f && ( f->num_ComputeConfig() == 2 ) );
 assert( ( f->get_SolverNames() ==
	   std::vector< std::string >( { "A" , "" } ) ) );
 assert( ( ! f->get_SolverConfig( 0 ) ) && f->get_SolverConfig( 1 ) );
 delete f;

 // the sub-Block, by name
 auto r = text_as< RBlockSolverConfig >(
	  "RBlockSolverConfig 0 1 BoxSolver 1 ComputeConfig 0 0 0 0 0 0 0 *\n"
	  "-2 sub1 BlockSolverConfig 1 1 BoxSolver 0\n"
	  "   sub2 RBlockSolverConfig 2 0 0\n" );
 assert( r && ( r->num_BlockSolverConfig() == 2 ) );
 assert( r->get_sub_Block_id( 0 ) == "sub1" );
 assert( r->get_sub_Block_id( 1 ) == "sub2" );
 assert( r->get_BlockSolverConfig( 0 )->get_SolverName( 0 ) == "BoxSolver" );
 assert( dynamic_cast< RBlockSolverConfig * >( r->get_BlockSolverConfig( 1 ) )
	 );
 assert( r->get_BlockSolverConfig( 1 )->is_diff() ==
	 BlockSolverConfig::eAddMode );

 // the netCDF round trip of one sub-Block
 {
  auto one = text_as< RBlockSolverConfig >(
	  "RBlockSolverConfig 0 1 BoxSolver 1 ComputeConfig 0 0 0 0 0 0 0 *\n"
	  "-1 sub1 BlockSolverConfig 1 1 BoxSolver 0\n" );
  assert( one && ( one->num_BlockSolverConfig() == 1 ) );
  auto nr = nc_as< RBlockSolverConfig >( *one );
  assert( nr && ( nr->num_BlockSolverConfig() == 1 ) );
  assert( nr->get_SolverNames() == one->get_SolverNames() );
  assert( same( nr->get_BlockSolverConfig( 0 ) ,
		one->get_BlockSolverConfig( 0 ) ) );
  delete nr;
  delete one;
  }
 // the netCDF format keeps the ids of the sub-Block
 {
  auto nr = nc_as< RBlockSolverConfig >( *r );
  assert( nr && ( nr->num_BlockSolverConfig() == 2 ) );
  assert( same( nr->get_BlockSolverConfig( 0 ) ,
		r->get_BlockSolverConfig( 0 ) ) );
  assert( same( nr->get_BlockSolverConfig( 1 ) ,
		r->get_BlockSolverConfig( 1 ) ) );
  assert( nr->get_sub_Block_id( 0 ) == "sub1" );
  assert( nr->get_sub_Block_id( 1 ) == "sub2" );
  delete nr;
  }

 // by position
 auto p = text_as< RBlockSolverConfig >(
	  "RBlockSolverConfig 0 0 1 BlockSolverConfig 0 0" );
 assert( p && ( p->num_BlockSolverConfig() == 1 ) &&
	 ( p->get_sub_Block_id( 0 ) == "0" ) );
 delete p;

 // no sub-Block at all
 p = text_as< RBlockSolverConfig >( "RBlockSolverConfig 0 0" );
 assert( p && ( ! p->num_BlockSolverConfig() ) && p->empty() );
 auto np = nc_as< RBlockSolverConfig >( *p );
 assert( np && ( ! np->num_BlockSolverConfig() ) );
 delete np; delete p;

 // a '*' is no BlockSolverConfig for a sub-Block
 p = text_as< RBlockSolverConfig >( "RBlockSolverConfig 0 0 1 *" );
 assert( p && ( p->num_BlockSolverConfig() == 1 ) &&
	 ( ! p->get_BlockSolverConfig( 0 ) ) );
 delete p;

 delete r; delete c;
 std::cout << "BlockSolverConfig and RBlockSolverConfig field by field: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*----------------------------------- MAIN ---------------------------------*/
/*--------------------------------------------------------------------------*/

int main( void )
{
 test_simple_int();
 test_simple_double();
 test_simple_string();
 test_simple_vector_int();
 test_simple_vector_double();
 test_simple_pair_of_configurations();
 test_simple_map();
 test_simple_vector_of_configurations();

 test_deserialize_stream();
 test_netCDF_positions();
 test_filename_prefix();

 test_ComputeConfig_load();
 test_ComputeConfig_flags();
 test_ComputeConfig_set_par();
 test_ComputeConfig_override();
 test_ComputeConfig_override_in_a_container();

 test_BlockConfig_load();
 test_BlockConfig_get_apply();
 test_BlockConfig_netCDF();
 test_BlockConfig_move();

 test_BlockSolverConfig_load();
 test_BlockSolverConfig_apply();
 test_BlockSolverConfig_netCDF();

 std::filesystem::create_directories( dir() );
 test_factory_names();
 test_pairs_and_vectors();
 test_nested();
 test_meta_configuration();
 test_MetaBlockSolverConfig();
 test_MetaBlockConfig();
 test_includes();
 test_ComputeConfig_copy();
 test_ComputeConfig_apply();
 test_override_in_a_map();
 test_BlockConfig_fields();
 test_BlockConfig_apply_nothing();
 test_handlers();
 test_BlockSolverConfig_fields();
 std::filesystem::remove_all( dir() );

 // last, since what they find broken may not leave the test standing
 test_OCRBlockConfig();
 test_ComputeConfig_set_par_entry_of_new_name();

 if( n_failed ) {
  std::cout << n_failed << " checks FAILED" << std::endl;
  return( 1 );
  }

 std::cout << "All tests passed!!" << std::endl;

 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File tests_Configuration.cpp ------------------*/
/*--------------------------------------------------------------------------*/
