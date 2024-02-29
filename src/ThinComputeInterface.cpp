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

void ThinComputeInterface::set_ComputeConfig( ComputeConfig * scfg )
{
 if( ( ! scfg ) || ( ! scfg->f_diff ) ) {  // "factory reset"
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

 for( const auto & pair : scfg->int_pars ) {
  auto idx = int_par_str2idx( pair.first );
  if( idx == Inf< idx_type >() )
   throw( std::invalid_argument( "int parameter " + pair.first + " unknown" ) );
  set_par( idx , pair.second );
  }

 for( const auto & pair : scfg->dbl_pars ) {
  auto idx = dbl_par_str2idx( pair.first );
  if( idx == Inf< idx_type >() )
   throw( std::invalid_argument( "double parameter " + pair.first +
				 " unknown" ) );
  set_par( idx , pair.second );
  }

 for( const auto & pair : scfg->str_pars ) {
  auto idx = str_par_str2idx( pair.first );
  if( idx == Inf< idx_type >() )
   throw( std::invalid_argument( "string parameter " + pair.first +
				 " unknown" ) );
  set_par( idx , pair.second );
  }

 for( const auto & pair : scfg->vint_pars ) {
  auto idx = vint_par_str2idx( pair.first );
  if( idx == Inf< idx_type >() )
   throw( std::invalid_argument( "vector-of-int parameter " + pair.first +
				 " unknown" ) );
  set_par( idx , pair.second );
  }

 for( const auto & pair : scfg->vdbl_pars ) {
  auto idx = vdbl_par_str2idx( pair.first );
  if( idx == Inf< idx_type >() )
   throw( std::invalid_argument( "vector-of-double parameter " + pair.first +
				 " unknown" ) );
  set_par( idx , pair.second );
  }

 for( const auto & pair : scfg->vstr_pars ) {
  auto idx = vstr_par_str2idx( pair.first );
  if( idx == Inf< idx_type >() )
   throw( std::invalid_argument( "vector-of-string parameter " + pair.first +
				 " unknown" ) );
  set_par( idx , pair.second );
  }
 }  // end( ThinComputeInterface::set_ComputeConfig )

/*--------------------------------------------------------------------------*/

ComputeConfig * ThinComputeInterface::get_ComputeConfig(
				       bool all, ComputeConfig * ocfg ) const
{
 ComputeConfig * ccfg = ocfg ? ocfg : new ComputeConfig;
 ccfg->f_diff = ! all;
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

void ThinComputeInterface::serialize_State( netCDF::NcGroup & group ,
                                const std::string & sub_group_name ) const {
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

 }  // end( ThinComputeInterface::serialize_State )

/*--------------------------------------------------------------------------*/
/*------------------------ METHODS of ComputeConfig ------------------------*/
/*--------------------------------------------------------------------------*/

void ComputeConfig::deserialize( const netCDF::NcGroup & group )
{
 // call the method of the base class, which does not much
 Configuration::deserialize( group );

 // f_diff field- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcGroupAtt diff = group.getAtt( "diff" );
 if( diff.isNull() )
  f_diff = false;
 else {
  int diffint;
  diff.getValues( &diffint );
  f_diff = diffint > 0;
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
 group.putAtt( "diff", netCDF::NcInt(), int( f_diff ) );

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

  ( group.addVar( "v_str_par_vals" , netCDF::NcString() , tot ) ).putVar(
							       tmp.data() ); 
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
 if( f_diff ) output << "[diff]";
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

 input >> f_diff;
 if( advance( input , sre ) )
  return;

 unsigned int k;
 input >> k;
 checkfail( input , sre );

 int_pars.resize( k );
 for( unsigned int i = 0 ; i < k ; ++i ) {
  input >> eatcomments >> int_pars[ i ].first
        >> eatcomments >> int_pars[ i ].second;
  checkfail( input , sre );
  }

 if( advance( input ) )
  return;

 input >> k;
 checkfail( input , sre );

 dbl_pars.resize( k );
 for( unsigned int i = 0 ; i < k ; ++i ) {
  input >> eatcomments >> dbl_pars[ i ].first
        >> eatcomments >> dbl_pars[ i ].second;
  checkfail( input , sre );
  }

 if( advance( input ) )
  return;

 input >> k;
 checkfail( input , sre );

 str_pars.resize( k );
 for( unsigned int i = 0 ; i < k ; ++i ) {
  input >> eatcomments >> str_pars[ i ].first
        >> eatcomments >> str_pars[ i ].second;
  checkfail( input , sre );
  }

 if( advance( input ) )
  return;

 input >> k;
 checkfail( input , sre );

 vint_pars.resize( k );
 for( unsigned int i = 0 ; i < k ; ++i ) {
  input >> eatcomments >> vint_pars[ i ].first
        >> eatcomments >> vint_pars[ i ].second;
  checkfail( input , sre );
  }

 if( advance( input ) )
  return;

 input >> k;
 checkfail( input , sre );

 vdbl_pars.resize( k );
 for( unsigned int i = 0 ; i < k ; ++i ) {
  input >> eatcomments >> vdbl_pars[ i ].first
        >> eatcomments >> vdbl_pars[ i ].second;
  checkfail( input , sre );
  }

 if( advance( input ) )
  return;

 input >> k;
 checkfail( input , sre );

 vstr_pars.resize( k );
 for( unsigned int i = 0 ; i < k ; ++i ) {
  input >> eatcomments >> vstr_pars[ i ].first
        >> eatcomments >> vstr_pars[ i ].second;
  }

 if( advance( input ) )
  return;

 f_extra_Configuration = Configuration::deserialize( input );

 }  // end( ComputeConfig::load )

/*--------------------------------------------------------------------------*/
/*------------------ End File ThinComputeInterface.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
