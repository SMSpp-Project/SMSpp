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

#include <filesystem>
#include <memory>
#include <fstream>
#include <iostream>
#include <sstream>

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
