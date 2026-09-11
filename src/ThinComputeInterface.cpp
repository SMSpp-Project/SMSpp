/*--------------------------------------------------------------------------*/
/*-------------------- File ThinComputeInterface.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the ThinComputeInterface and of the ComputeConfig
 * classes.
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

#include "SMSTypedefs.h"

#include "ThinComputeInterface.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*-------------------------------- FUNCTIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

static bool advance( std::istream & input )
{
 input >> eatcomments;
 return( input.eof() );
 }

/*--------------------------------------------------------------------------*/

static bool advance( std::istream & input , const std::string & msg )
{
 if( input.fail() )
  throw( std::invalid_argument( msg ) );
 return( advance( input ) );
 }

/*--------------------------------------------------------------------------*/

static void checkfail( std::istream & input , const std::string & msg )
{
 if( input.fail() )
  throw( std::invalid_argument( msg ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register ComputeConfig to the Configuration factory
SMSpp_insert_in_factory_cpp_0( ComputeConfig );

/*--------------------------------------------------------------------------*/
/*--------------------- METHODS of ThinComputeInterface --------------------*/
/*--------------------------------------------------------------------------*/

void ThinComputeInterface::set_ComputeConfig( const ComputeConfig * scfg )
{
 if( ( ! scfg ) || ( ! scfg->diff() ) ) {  // "factory reset"
  for( int i = 0 ; i < get_num_int_par() ; ++i )
   set_par( i , get_dflt_int_par( i ) );

  for( int i = 0 ; i < get_num_dbl_par() ; ++i )
   set_par( i , get_dflt_dbl_par( i ) );

  for( int i = 0 ; i < get_num_str_par() ; ++i )
   set_par( i , get_dflt_str_par( i ) );

  for( int i = 0 ; i < get_num_vint_par() ; ++i )
   set_par( i , get_dflt_vint_par( i ) );

  for( int i = 0 ; i < get_num_vdbl_par() ; ++i )
   set_par( i , get_dflt_vdbl_par( i ) );

  for( int i = 0 ; i < get_num_vstr_par() ; ++i )
   set_par( i , get_dflt_vstr_par( i ) );
  }

 if( ! scfg )  // no ComputeConfig
  return;      // all done

 for( const auto & pair : scfg->int_pars )
  if( ( ! set_par( pair.first , pair.second ) ) && ( ! scfg->relax() ) )
   throw( std::invalid_argument( "Invalid int parameter name " +
				 pair.first ) );

 for( const auto & pair : scfg->dbl_pars )
  if( ( ! set_par( pair.first , pair.second ) ) && ( ! scfg->relax() ) )
   throw( std::invalid_argument( "Invalid double parameter name " +
				 pair.first ) );

 for( const auto & pair : scfg->str_pars )
  if( ( ! set_par( pair.first , pair.second ) ) && ( ! scfg->relax() ) )
   throw( std::invalid_argument( "Invalid string parameter name " +
				 pair.first ) );

 for( const auto & pair : scfg->vint_pars )
  if( ( ! set_par( pair.first , pair.second ) ) && ( ! scfg->relax() ) )
   throw( std::invalid_argument( "Invalid vector-of-int parameter name " +
				 pair.first ) );

 for( const auto & pair : scfg->vdbl_pars )
  if( ( ! set_par( pair.first , pair.second ) ) && ( ! scfg->relax() ) )
   throw( std::invalid_argument( "Invalid vector-of-double parameter name "
				 + pair.first ) );

 for( const auto & pair : scfg->vstr_pars )
  if( ( ! set_par( pair.first , pair.second ) ) && ( ! scfg->relax() ) )
  throw( std::invalid_argument( "Invalid vector-of-string parameter name "
				+ pair.first ) );

 }  // end( ThinComputeInterface::set_ComputeConfig )

/*--------------------------------------------------------------------------*/

ComputeConfig * ThinComputeInterface::get_ComputeConfig( bool all ,
						ComputeConfig * ocfg ) const
{
 ComputeConfig * ccfg = ocfg ? ocfg : new ComputeConfig;
 ccfg->set_diff( ! all );
 if( all ) {
  ccfg->int_pars.resize( get_num_int_par() );
  for( int i = 0; i < get_num_int_par(); ++i ) {
   ccfg->int_pars[ i ].first = int_par_idx2str( i );
   ccfg->int_pars[ i ].second = get_int_par( i );
   }

  ccfg->dbl_pars.resize( get_num_dbl_par() );
  for( int i = 0; i < get_num_dbl_par(); ++i ) {
   ccfg->dbl_pars[ i ].first = dbl_par_idx2str( i );
   ccfg->dbl_pars[ i ].second = get_dbl_par( i );
   }

  ccfg->str_pars.resize( get_num_str_par() );
  for( int i = 0; i < get_num_str_par(); ++i ) {
   ccfg->str_pars[ i ].first = str_par_idx2str( i );
   ccfg->str_pars[ i ].second = get_str_par( i );
   }
  
  ccfg->vint_pars.resize( get_num_vint_par() );
  for( int i = 0; i < get_num_vint_par(); ++i ) {
   ccfg->vint_pars[ i ].first = vint_par_idx2str( i );
   ccfg->vint_pars[ i ].second = get_vint_par( i );
   }

  ccfg->vdbl_pars.resize( get_num_vdbl_par() );
  for( int i = 0; i < get_num_vdbl_par(); ++i ) {
   ccfg->vdbl_pars[ i ].first = vdbl_par_idx2str( i );
   ccfg->vdbl_pars[ i ].second = get_vdbl_par( i );
   }

  ccfg->vstr_pars.resize( get_num_vstr_par() );
  for( int i = 0; i < get_num_vstr_par(); ++i ) {
   ccfg->vstr_pars[ i ].first = vstr_par_idx2str( i );
   ccfg->vstr_pars[ i ].second = get_vstr_par( i );
   }
  }
 else {
  for( int i = 0 ; i < get_num_int_par() ; ++i )
   if( get_int_par( i ) != get_dflt_int_par( i ) )
    ccfg->int_pars.emplace_back( int_par_idx2str( i ) , get_int_par( i ) );

  for( int i = 0 ; i < get_num_dbl_par() ; ++i )
   if( get_dbl_par( i ) != get_dflt_dbl_par( i ) )
    ccfg->dbl_pars.emplace_back( dbl_par_idx2str( i ) , get_dbl_par( i ) );

  for( int i = 0 ; i < get_num_str_par() ; ++i )
   if( ! ( get_str_par( i ) == get_dflt_str_par( i ) ) )
    ccfg->str_pars.emplace_back( str_par_idx2str( i ) , get_str_par( i ) );

  for( int i = 0 ; i < get_num_vint_par() ; ++i )
   if( ! ( get_vint_par( i ) == get_dflt_vint_par( i ) ) )
    ccfg->vint_pars.emplace_back( vint_par_idx2str( i ) , get_vint_par( i ) );

  for( int i = 0 ; i < get_num_vdbl_par() ; ++i )
   if( ! ( get_vdbl_par( i ) == get_dflt_vdbl_par( i ) ) )
    ccfg->vdbl_pars.emplace_back( vdbl_par_idx2str( i ) , get_vdbl_par( i ) );

  for( int i = 0 ; i < get_num_vstr_par() ; ++i )
   if( ! ( get_vstr_par( i ) == get_dflt_vstr_par( i ) ) )
    ccfg->vstr_pars.emplace_back( vstr_par_idx2str( i ) , get_vstr_par( i ) );
  }

 if( ccfg->empty() ) {
  delete ccfg;
  ccfg = nullptr;
  }

 return( ccfg );

 }  // end( ThinComputeInterface::get_ComputeConfig )

/*--------------------------------------------------------------------------*/

void ThinComputeInterface::print_parameters( std::ostream & os ) const
{
 // the name of a parameter, or its index when the class does not name it
 auto nm = [ this ]( const std::string & ( ThinComputeInterface::*idx2str )
                     ( idx_type ) const , idx_type i ) -> std::string {
  try { return( ( this->*idx2str )( i ) ); }
  catch( ... ) { return( "[ " + std::to_string( i ) + " ]" ); }
  };

 // a vector printed as its elements between brackets
 auto vec = []( std::ostream & o , const auto & v ) -> std::ostream & {
  o << "[";
  for( const auto & el : v )
   o << " " << el;
  return( o << " ]" );
  };

 for( idx_type i = 0 ; i < get_num_int_par() ; ++i ) {
  os << nm( &ThinComputeInterface::int_par_idx2str , i ) << " = ";
  try { os << get_int_par( i ) << " (" << get_dflt_int_par( i ) << ")"; }
  catch( ... ) { os << "?"; }
  os << std::endl;
  }

 for( idx_type i = 0 ; i < get_num_dbl_par() ; ++i ) {
  os << nm( &ThinComputeInterface::dbl_par_idx2str , i ) << " = ";
  try { os << get_dbl_par( i ) << " (" << get_dflt_dbl_par( i ) << ")"; }
  catch( ... ) { os << "?"; }
  os << std::endl;
  }

 for( idx_type i = 0 ; i < get_num_str_par() ; ++i ) {
  os << nm( &ThinComputeInterface::str_par_idx2str , i ) << " = ";
  try { os << "\"" << get_str_par( i ) << "\" (\""
             << get_dflt_str_par( i ) << "\")"; }
  catch( ... ) { os << "?"; }
  os << std::endl;
  }

 for( idx_type i = 0 ; i < get_num_vint_par() ; ++i ) {
  os << nm( &ThinComputeInterface::vint_par_idx2str , i ) << " = ";
  try { vec( os , get_vint_par( i ) ); }
  catch( ... ) { os << "?"; }
  os << std::endl;
  }

 for( idx_type i = 0 ; i < get_num_vdbl_par() ; ++i ) {
  os << nm( &ThinComputeInterface::vdbl_par_idx2str , i ) << " = ";
  try { vec( os , get_vdbl_par( i ) ); }
  catch( ... ) { os << "?"; }
  os << std::endl;
  }

 for( idx_type i = 0 ; i < get_num_vstr_par() ; ++i ) {
  os << nm( &ThinComputeInterface::vstr_par_idx2str , i ) << " = ";
  try { vec( os , get_vstr_par( i ) ); }
  catch( ... ) { os << "?"; }
  os << std::endl;
  }
 }  // end( ThinComputeInterface::print_parameters )

/*--------------------------------------------------------------------------*/

void ThinComputeInterface::serialize_State( netCDF::NcGroup & group ,
                                const std::string & sub_group_name ) const
{
 auto state = get_State();
 if( ! state )
  return;
 if( sub_group_name.empty() )
  state->serialize( group );
 else {
  auto sub_group = group.getGroup( sub_group_name );
  if( sub_group.isNull() )
   sub_group = group.addGroup( sub_group_name );
  if( sub_group.isNull() )
   throw( std::logic_error( "ThinComputeInterface::serialize_State: "
                            "cannot create group " + sub_group_name ) );
  state->serialize( sub_group );
  }
 delete state;

 }  // end( ThinComputeInterface::serialize_State( NcGroup , std::string )

/*--------------------------------------------------------------------------*/
/*------------------------ METHODS of ComputeConfig ------------------------*/
/*--------------------------------------------------------------------------*/

void ComputeConfig::deserialize( const netCDF::NcGroup & group )
{
 // call the method of the base class, which does not much
 Configuration::deserialize( group );

 // f_diff and f_relax fields - - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcGroupAtt diff = group.getAtt( "diff" );
 if( diff.isNull() )
  f_diff = f_relax = false;
 else {
  int diffint;
  diff.getValues( &diffint );
  f_diff = ( diffint & 1 );
  f_relax = ( diffint & 2 );
  }

 // int parameters- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcDim nip = group.getDim( "int_par_num" );

 if( size_t num = nip.isNull() ? 0 : nip.getSize() ) {
  netCDF::NcVar names = group.getVar( "int_par_names" );
  if( names.isNull() )
   throw( std::invalid_argument( "missing int_par_names in netCDF group" ) );

  netCDF::NcVar vals = group.getVar( "int_par_vals" );
  if( vals.isNull() )
   throw( std::invalid_argument( "missing int_par_vals in netCDF group" ) );

  int_pars.resize( num );
  for( size_t i = 0; i < num ; ++i ) {
   std::vector< size_t > idx = { i };
   names.getVar( idx , &( int_pars[ i ].first ) );
   vals.getVar( idx , &( int_pars[ i ].second ) );
   }
  }

 // double parameters - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcDim ndp = group.getDim( "dbl_par_num" );

 if( size_t num = ndp.isNull() ? 0 : ndp.getSize() ) {
  netCDF::NcVar names = group.getVar( "dbl_par_names" );
  if( names.isNull() )
   throw( std::invalid_argument( "missing dbl_par_names in netCDF group" ) );

  netCDF::NcVar vals = group.getVar( "dbl_par_vals" );
  if( vals.isNull() )
   throw( std::invalid_argument( "missing dbl_par_vals in netCDF group" ) );

  dbl_pars.resize( num );
  for( size_t i = 0 ; i < num ; ++i ) {
   std::vector< size_t > idx = { i };
   names.getVar( idx , &( dbl_pars[ i ].first ) );
   vals.getVar( idx , &( dbl_pars[ i ].second ) );
   }
  }

 // string parameters - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcDim nsp = group.getDim( "str_par_num" );
 
 if( size_t num = nsp.isNull() ? 0 : nsp.getSize() ) {
  netCDF::NcVar names = group.getVar( "str_par_names" );
  if( names.isNull() )
   throw( std::invalid_argument( "missing str_par_names in netCDF group" ) );

  netCDF::NcVar vals = group.getVar( "str_par_vals" );
  if( vals.isNull() )
   throw( std::invalid_argument( "missing str_par_vals in netCDF group" ) );

  str_pars.resize( num );
  for( size_t i = 0 ; i < num ; ++i ) {
   std::vector< size_t > idx = { i };
   names.getVar( idx , &( str_pars[ i ].first ) );
   vals.getVar( idx , &( str_pars[ i ].second ) );
   }
  }

 // vector-of-int parameters- - - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcDim nvip = group.getDim( "v_int_par_num" );

 if( size_t num = nvip.isNull() ? 0 : nvip.getSize() ) {
  netCDF::NcVar names = group.getVar( "v_int_par_names" );
  if( names.isNull() )
   throw( std::invalid_argument( "missing v_int_par_names in netCDF group" )
	  );

  std::vector< std::vector< int > > tmp;
  ::deserialize( group , "v_int_par_vals" , "v_int_par_start" , tmp , false );

  vint_pars.resize( num );
  for( size_t i = 0 ; i < num ; ++i ) {
   names.getVar( { i } , &( vint_pars[ i ].first ) );
   vint_pars[ i ].second = std::move( tmp[ i ] );
   }
  }

 // vector-of-float parameters- - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcDim nvdp = group.getDim( "v_dbl_par_num" );

 if( size_t num = nvdp.isNull() ? 0 : nvdp.getSize() ) {
  netCDF::NcVar names = group.getVar( "v_dbl_par_names" );
  if( names.isNull() )
   throw( std::invalid_argument( "missing v_dbl_par_names in netCDF group" )
	  );

  std::vector< std::vector< double > > tmp;
  ::deserialize( group , "v_dbl_par_vals" , "v_dbl_par_start" , tmp , false );

  vdbl_pars.resize( num );
  for( size_t i = 0 ; i < num ; ++i ) {
   names.getVar( { i } , &( vdbl_pars[ i ].first ) );
   vdbl_pars[ i ].second = std::move( tmp[ i ] );
   }
  }

 // vector-of-string parameters - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcDim nvsp = group.getDim( "v_str_par_num" );

 if( size_t num = nvsp.isNull() ? 0 : nvsp.getSize() ) {
  netCDF::NcVar names = group.getVar( "v_str_par_names" );
  if( names.isNull() )
   throw( std::invalid_argument( "missing v_str_par_names in netCDF group" )
	  );

  std::vector< std::vector< std::string > > tmp;
  ::deserialize( group , "v_str_par_vals" , "v_str_par_start" , tmp , false );

  vstr_pars.resize( num );
  for( size_t i = 0 ; i < num ; ++i ) {
   names.getVar( { i } , &( vstr_pars[ i ].first ) );
   vstr_pars[ i ].second = std::move( tmp[ i ] );
   }
  }

 // "extra" Configuration - - - - - - - - - - - - - - - - - - - - - - - - - -
 auto ec = group.getGroup( "extra" );
 f_extra_Configuration = new_Configuration( ec );

 }  // end( ComputeConfig::deserialize( netCDF::NcGroup ) )

/*--------------------------------------------------------------------------*/

void ComputeConfig::serialize( netCDF::NcGroup & group ) const
{
 // call the method of the base class, which writes the "type" attribute
 Configuration::serialize( group );

 // f_diff field- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 group.putAtt( "diff", netCDF::NcInt(),
	       int( ( f_diff ? 1 : 0 ) + ( f_relax ? 2 : 0 ) ) );

 // int parameters- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( ! int_pars.empty() ) {
  auto num = group.addDim( "int_par_num" , int_pars.size() );
  auto nmes = group.addVar( "int_par_names" , netCDF::NcString() , num );
  auto vals = group.addVar( "int_par_vals" , netCDF::NcInt() , num );
  for( size_t i = 0 ; i < int_pars.size() ; ++i ) {
   std::vector< size_t > idx = { i };
   nmes.putVar( idx, int_pars[ i ].first );
   vals.putVar( idx, int_pars[ i ].second );
   }
  }

 // double parameters - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( ! dbl_pars.empty() ) {
  auto num = group.addDim( "dbl_par_num" , dbl_pars.size() );
  auto nmes = group.addVar( "dbl_par_names" , netCDF::NcString() , num );
  auto vals = group.addVar( "dbl_par_vals" ,netCDF::NcDouble() , num );
  for( size_t i = 0 ; i < dbl_pars.size() ; ++i ) {
   std::vector< size_t > idx = { i };
   nmes.putVar( idx , dbl_pars[ i ].first );
   vals.putVar( idx , dbl_pars[ i ].second );
   }
  }

 // string parameters - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( ! str_pars.empty() ) {
  auto num = group.addDim( "str_par_num" , str_pars.size() );
  auto nmes = group.addVar( "str_par_names" , netCDF::NcString() , num );
  auto vals = group.addVar( "str_par_vals" , netCDF::NcString() , num );
  for( size_t i = 0 ; i < str_pars.size() ; ++i ) {
   std::vector< size_t > idx = { i };
   nmes.putVar( idx , str_pars[ i ].first );
   vals.putVar( idx , str_pars[ i ].second );
   }
  }

 // vector-of-int parameters- - - - - - - - - - - - - - - - - - - - - - - - -
 if( ! vint_pars.empty() ) {
  auto num = group.addDim( "v_int_par_num" , vint_pars.size() );
  auto nmes = group.addVar( "v_int_par_names" , netCDF::NcString() , num );

  auto cnt = 0;
  std::vector< int > strt( vint_pars.size() );

  for( size_t i = 0 ; i < vint_pars.size() ; ++i ) {
   nmes.putVar( { i } , vint_pars[ i ].first );
   strt[ i ] = cnt;
   cnt += vint_pars[ i ].second.size();
   }

  auto tot = group.addDim( "v_int_par_tot" , cnt );
  ( group.addVar( "v_int_par_start", netCDF::NcInt() , num ) ).putVar(
							      strt.data() );
  std::vector< int > tmp( cnt );
  auto tit = tmp.begin();
  for( auto & el : vint_pars )
   tit = std::copy( el.second.begin() , el.second.end() , tit );

  ( group.addVar( "v_int_par_vals" , netCDF::NcInt() , tot ) ).putVar(
							       tmp.data() ); 
  }

 // vector-of-float parameters- - - - - - - - - - - - - - - - - - - - - - - -
 if( ! vdbl_pars.empty() ) {
  auto num = group.addDim( "v_dbl_par_num" , vdbl_pars.size() );
  auto nmes = group.addVar( "v_dbl_par_names" , netCDF::NcString() , num );

  auto cnt = 0;
  std::vector< int > strt( vdbl_pars.size() );

  for( size_t i = 0 ; i < vdbl_pars.size() ; ++i ) {
   nmes.putVar( { i } , vdbl_pars[ i ].first );
   strt[ i ] = cnt;
   cnt += vdbl_pars[ i ].second.size();
   }

  auto tot = group.addDim( "v_dbl_par_tot" , cnt );
  ( group.addVar( "v_dbl_par_start", netCDF::NcInt() , num ) ).putVar(
							      strt.data() );
  std::vector< double > tmp( cnt );
  auto tit = tmp.begin();
  for( auto & el : vdbl_pars )
   tit = std::copy( el.second.begin() , el.second.end() , tit );

  ( group.addVar( "v_dbl_par_vals" , netCDF::NcDouble() , tot ) ).putVar(
							       tmp.data() ); 
  }

 // vector-of-string parameters - - - - - - - - - - - - - - - - - - - - - - -
 if( ! vstr_pars.empty() ) {
  auto num = group.addDim( "v_str_par_num" , vstr_pars.size() );
  auto nmes = group.addVar( "v_str_par_names" , netCDF::NcString() , num );

  auto cnt = 0;
  std::vector< int > strt( vstr_pars.size() );

  for( size_t i = 0 ; i < vstr_pars.size() ; ++i ) {
   nmes.putVar( { i } , vstr_pars[ i ].first );
   strt[ i ] = cnt;
   cnt += vstr_pars[ i ].second.size();
   }

  auto tot = group.addDim( "v_str_par_tot" , cnt );
  ( group.addVar( "v_str_par_start", netCDF::NcInt() , num ) ).putVar(
							      strt.data() );
  std::vector< std::string > tmp( cnt );
  auto tit = tmp.begin();
  for( auto & el : vstr_pars )
   tit = std::copy( el.second.begin() , el.second.end() , tit );

  // a netCDF NcString variable is backed by variable-length strings, so
  // putVar() expects an array of C-strings (char **): passing the std::string
  // objects directly (tmp.data()) would make the netCDF/HDF5 layer read their
  // internal representation as char * and crash. Build the array of C-string
  // pointers explicitly.
  std::vector< const char * > tmp_cstr( tmp.size() );
  for( size_t i = 0 ; i < tmp.size() ; ++i )
   tmp_cstr[ i ] = tmp[ i ].c_str();

  ( group.addVar( "v_str_par_vals" , netCDF::NcString() , tot ) ).putVar(
							       tmp_cstr.data() );
  }

 // "extra" Configuration - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( f_extra_Configuration ) {
  auto eg = group.addGroup( "extra" );
  f_extra_Configuration->serialize( eg );
  }
 }  // end( ComputeConfig::serialize( netCDF::NcGroup ) )

/*--------------------------------------------------------------------------*/

void ComputeConfig::reset_par( const std::string & name , char type )
{
 switch( type ) {
  case( 'i' ): {
   auto it = std::find_if( int_pars.begin() , int_pars.end() ,
			   [ & name ]( auto & el ) {
			    return( name == el.first );
			    } );
   if( it != int_pars.end() ) {
    *it = std::move( int_pars.back() );
    int_pars.pop_back();
    }
   return;
   }
  case( 'd' ): {
   auto it = std::find_if( dbl_pars.begin() , dbl_pars.end() ,
			   [ & name ]( auto & el ) {
			    return( name == el.first );
			    } );
   if( it != dbl_pars.end() ) {
    *it = std::move( dbl_pars.back() );
    dbl_pars.pop_back();
    }
   return;
   }
  case( 's' ): {
   auto it = std::find_if( str_pars.begin() , str_pars.end() ,
			   [ & name ]( auto & el ) {
			    return( name == el.first );
			    } );
   if( it != str_pars.end() ) {
    *it = std::move( str_pars.back() );
    str_pars.pop_back();
    }
   return;
   }
  case( 'I' ): {
   auto it = std::find_if( vint_pars.begin() , vint_pars.end() ,
			   [ & name ]( auto & el ) {
			    return( name == el.first );
			    } );
   if( it != vint_pars.end() ) {
    *it = std::move( vint_pars.back() );
    vint_pars.pop_back();
    }
   return;
   }
  case( 'D' ): {
   auto it = std::find_if( vdbl_pars.begin() , vdbl_pars.end() ,
			   [ & name ]( auto & el ) {
			    return( name == el.first );
			    } );
   if( it != vdbl_pars.end() ) {
    *it = std::move( vdbl_pars.back() );
    vdbl_pars.pop_back();
    }
   return;
   }
  case( 'S' ): {
   auto it = std::find_if( vstr_pars.begin() , vstr_pars.end() ,
			   [ & name ]( auto & el ) {
			    return( name == el.first );
			    } );
   if( it != vstr_pars.end() ) {
    *it = std::move( vstr_pars.back() );
    vstr_pars.pop_back();
    }
   return;
   }
  default:
   throw( std::invalid_argument( "reset_par: invalid parameter type" ) );
  }
 }  // end( ComputeConfig::reset_par )

/*--------------------------------------------------------------------------*/

void ComputeConfig::print( std::ostream & output ) const
{
 output << "ComputeConfig";
 if( f_diff )
  if( f_relax )
   output << "[diff,relax]";
  else
   output << "[diff]";
 else
  if( f_relax )
   output << "[relax]";
 output << ": " << std::endl;
 for( auto & pair : int_pars )
  output << pair.first << " = " << pair.second << std::endl;
 for( auto & pair : dbl_pars )
  output << pair.first << " = " << pair.second << std::endl;
 for( auto & pair : str_pars )
  output << pair.first << " = " << pair.second << std::endl;
 for( auto & pair : vint_pars )
  output << pair.first << " = " << pair.second << std::endl;
 for( auto & pair : vdbl_pars )
  output << pair.first << " = " << pair.second << std::endl;
 for( auto & pair : vstr_pars )
  output << pair.first << " = " << pair.second << std::endl;
 if( f_extra_Configuration )
  output << "xtra Config:" << *f_extra_Configuration;
 }

/*--------------------------------------------------------------------------*/

void ComputeConfig::load( std::istream & input )
{
 if( f_extra_Configuration ) {
  delete f_extra_Configuration;
  f_extra_Configuration = nullptr;
  }

 clear();
 f_diff = true;

 static const std::string sre( "ComputeConfig::load: stream read error" );
 if( advance( input , sre ) )
  return;

 unsigned int k;
 input >> k;
 f_diff = k & 1;
 f_relax = k & 2;
 if( advance( input , sre ) )
  return;

 // helper: read `count` (name, value) pairs into `pairs`; on failure
 // throw a message that pinpoints the parameter block, the declared
 // count, the index of the failing entry and the last name successfully
 // parsed - so the user sees *which* count was wrong without having to
 // recount the file by hand
 auto read_pair_block = [ & input ]( const char * block ,
                                     unsigned int count ,
                                     auto & pairs ,
                                     const std::string & last_name ) {
  pairs.resize( count );
  std::string prev_name = last_name;
  for( unsigned int i = 0 ; i < count ; ++i ) {
   input >> eatcomments >> pairs[ i ].first
         >> eatcomments >> pairs[ i ].second;
   if( input.fail() )
    throw( std::logic_error(
         "ComputeConfig::load: " + std::string( block ) +
         ": declared " + std::to_string( count ) + " entries but "
         "failed to read entry #" + std::to_string( i ) +
         " (last successfully parsed name: '" + prev_name + "'); "
         "check that the count matches the number of name/value pairs "
         "in the file" ) );
   prev_name = pairs[ i ].first;
   }
  };

 // helper: read the count of the next parameter block; if the next
 // token in the stream is not an integer, this typically means that the
 // *previous* block declared fewer entries than were actually written
 // in the file, and the remainder of that block spilled over into the
 // count position - so blame the previous block in the error message
 auto read_count = [ & input ]( const char * block ,
                                const char * previous_block ,
                                unsigned int & out ) {
  input >> eatcomments;
  if( ! ( input >> out ) ) {
   input.clear();
   std::string stray;
   input >> stray;
   throw( std::logic_error(
        "ComputeConfig::load: expected the count of " +
        std::string( block ) + " but found '" + stray + "'; the "
        "most likely cause is an off-by-one in the count of " +
        std::string( previous_block ) + " (declared fewer entries "
        "than were actually written, so the leftover ones spilled "
        "into this block's count position)" ) );
   }
  };

 read_count( "int_pars" , "<header>" , k );
 read_pair_block( "int_pars" , k , int_pars , "<none>" );

 if( advance( input ) )
  return;

 read_count( "dbl_pars" , "int_pars" , k );
 read_pair_block( "dbl_pars" , k , dbl_pars , "<none>" );

 if( advance( input ) )
  return;

 read_count( "str_pars" , "dbl_pars" , k );
 read_pair_block( "str_pars" , k , str_pars , "<none>" );

 if( advance( input ) )
  return;

 read_count( "vint_pars" , "str_pars" , k );
 read_pair_block( "vint_pars" , k , vint_pars , "<none>" );

 if( advance( input ) )
  return;

 read_count( "vdbl_pars" , "vint_pars" , k );
 read_pair_block( "vdbl_pars" , k , vdbl_pars , "<none>" );

 if( advance( input ) )
  return;

 read_count( "vstr_pars" , "vdbl_pars" , k );
 read_pair_block( "vstr_pars" , k , vstr_pars , "<none>" );

 if( advance( input ) )
  return;

 f_extra_Configuration = Configuration::deserialize( input );

 }  // end( ComputeConfig::load )

/*--------------------------------------------------------------------------*/

void ComputeConfig::merge_overrides( std::istream & input )
{
 // body format is identical to ComputeConfig::load — same diff/relax
 // header, same six count+pairs blocks, same trailing extra slot — but
 // each name/value pair is fed through set_par so that entries already
 // present in this ComputeConfig (loaded from the included base file)
 // are replaced and new entries are appended, instead of the whole
 // vectors being resized and overwritten.

 static const std::string sre(
                  "ComputeConfig::merge_overrides: stream read error" );
 if( advance( input , sre ) )
  return;

 unsigned int k;
 input >> k;
 f_diff = k & 1;
 f_relax = k & 2;
 if( advance( input , sre ) )
  return;

 input >> k;
 checkfail( input , sre );
 for( unsigned int i = 0 ; i < k ; ++i ) {
  std::string name;
  int value;
  input >> eatcomments >> name >> eatcomments >> value;
  checkfail( input , sre );
  set_par( std::move( name ) , value );
  }

 if( advance( input ) )
  return;

 input >> k;
 checkfail( input , sre );
 for( unsigned int i = 0 ; i < k ; ++i ) {
  std::string name;
  double value;
  input >> eatcomments >> name >> eatcomments >> value;
  checkfail( input , sre );
  set_par( std::move( name ) , value );
  }

 if( advance( input ) )
  return;

 input >> k;
 checkfail( input , sre );
 for( unsigned int i = 0 ; i < k ; ++i ) {
  std::string name , value;
  input >> eatcomments >> name >> eatcomments >> value;
  checkfail( input , sre );
  set_par( std::move( name ) , std::move( value ) );
  }

 if( advance( input ) )
  return;

 input >> k;
 checkfail( input , sre );
 for( unsigned int i = 0 ; i < k ; ++i ) {
  std::string name;
  std::vector< int > value;
  input >> eatcomments >> name >> eatcomments >> value;
  checkfail( input , sre );
  set_par( std::move( name ) , std::move( value ) );
  }

 if( advance( input ) )
  return;

 input >> k;
 checkfail( input , sre );
 for( unsigned int i = 0 ; i < k ; ++i ) {
  std::string name;
  std::vector< double > value;
  input >> eatcomments >> name >> eatcomments >> value;
  checkfail( input , sre );
  set_par( std::move( name ) , std::move( value ) );
  }

 if( advance( input ) )
  return;

 input >> k;
 checkfail( input , sre );
 for( unsigned int i = 0 ; i < k ; ++i ) {
  std::string name;
  std::vector< std::string > value;
  input >> eatcomments >> name >> eatcomments >> value;
  checkfail( input , sre );
  set_par( std::move( name ) , std::move( value ) );
  }

 // extra Configuration slot is optional in an override block: if the
 // stream is exhausted (or the next non-whitespace token belongs to the
 // enclosing container), the base's extra is preserved. Otherwise the
 // override's extra wholesale replaces it.
 if( advance( input ) )
  return;

 if( f_extra_Configuration ) {
  delete f_extra_Configuration;
  f_extra_Configuration = nullptr;
  }
 f_extra_Configuration = Configuration::deserialize( input );

 }  // end( ComputeConfig::merge_overrides )

/*--------------------------------------------------------------------------*/
/*---------------------------- METHODS of State ----------------------------*/
/*--------------------------------------------------------------------------*/

State * State::deserialize( const std::string & filename )
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
  return( State::deserialize( f , idx ) );
  }
 catch( netCDF::exceptions::NcException & e ) {
  std::cerr << "netCDF error " << e.what() << " in State::deserialize"
	    << std::endl;
  }
 catch( std::exception & e ) {
  std::cerr << "error " << e.what() << " in State::deserialize" << std::endl;
  }
 catch( ... ) {
  std::cerr << "unknown error in State::deserialize" << std::endl;
  }

 return( nullptr );

 }  // end( State::deserialize( const std::string ) )

/*--------------------------------------------------------------------------*/

State * State::deserialize( const netCDF::NcFile & f , int idx )
{
 try {
  auto gtype = f.getAtt( "SMS++_file_type" );
  if( gtype.isNull() )
   return( nullptr );

  int type;
  gtype.getValues( & type );

  if( type != eStateFile )
   return( nullptr );

  auto cg = f.getGroup( "State_" + std::to_string( idx ) );

  return( new_State( cg ) );
  }
 catch( netCDF::exceptions::NcException & e ) {
  std::cerr << "netCDF error " << e.what() << " in State::deserialize"
	    << std::endl;
  }
 catch( std::exception & e ) {
  std::cerr << "error " << e.what() << " in State::deserialize" << std::endl;
  }
 catch( ... ) {
  std::cerr << "unknown error in State::deserialize" << std::endl;
  }

 return( nullptr );

 }  // end( State::deserialize( netCDF::NcFile ) )

/*--------------------------------------------------------------------------*/

State * State::new_State( const netCDF::NcGroup & group )
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
  auto result = new_State( tmp );
  result->deserialize( group );
  return( result );
  }
 catch( netCDF::exceptions::NcException & e ) {
  std::cerr << "netCDF error " << e.what() << " in State::new_State"
	    << std::endl;
  }
 catch( std::exception & e ) {
  std::cerr << "error " << e.what() << " in State::new_State"
	    << std::endl;
  }
 catch( ... ) {
  std::cerr << "unknown error in State::new_State" << std::endl;
  }

 return( nullptr );

 }  // end( State::new_State( netCDF::NcGroup ) )

/*--------------------------------------------------------------------------*/

std::string & state_filename_prefix()
{
 static std::string prefix;
 return( prefix );
 }

void State::set_filename_prefix( std::string && prefix )
{
 state_filename_prefix() = std::move( prefix );
 }

const std::string & State::get_filename_prefix()
{
 return( state_filename_prefix() );
 }

/*--------------------------------------------------------------------------*/
/*------------------ End File ThinComputeInterface.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
