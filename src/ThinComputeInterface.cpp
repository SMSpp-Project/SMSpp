/*--------------------------------------------------------------------------*/
/*-------------------- File ThinComputeInterface.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the ThinComputeInterface and of the ComputeConfig
 * classes.
 *
 * \version 0.10
 *
 * \date 03 - 09 - 2018
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

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
  }

 if( ! scfg )  // no ComputeConfig
  return;      // all done

 for( auto pair : scfg->int_pars )
  set_par( int_par_str2idx( pair.first ) , pair.second );

 for( auto pair : scfg->dbl_pars )
  set_par( dbl_par_str2idx( pair.first ) , pair.second );

 for( auto pair : scfg->str_pars )
  set_par( str_par_str2idx( pair.first ) , pair.second );

 }  // end( ThinComputeInterface::set_ComputeConfig )

/*--------------------------------------------------------------------------*/

ComputeConfig * ThinComputeInterface::get_ComputeConfig( bool all ,
						ComputeConfig * ocfg ) const
{
 ComputeConfig * ccfg = ocfg ? ocfg : new ComputeConfig();
 ccfg->f_diff = ~all;
 if( all ) {
  ccfg->int_pars.resize( get_num_int_par() );
  for( int i = 0 ; i < get_num_int_par() ; ++i ) {
   ccfg->int_pars[ i ].first = int_par_idx2str( i );
   ccfg->int_pars[ i ].second = get_int_par( i );
   }

  ccfg->dbl_pars.resize( get_num_dbl_par() );
  for( int i = 0 ; i < get_num_dbl_par() ; ++i ) {
   ccfg->dbl_pars[ i ].first = dbl_par_idx2str( i );
   ccfg->dbl_pars[ i ].second = get_dbl_par( i );
   }

  ccfg->str_pars.resize( get_num_str_par() );
  for( int i = 0 ; i < get_num_str_par() ; ++i ) {
   ccfg->str_pars[ i ].first = str_par_idx2str( i );
   ccfg->str_pars[ i ].second = get_str_par( i );
   }
  }
 else {
  for( int i = 0 ; i < get_num_int_par() ; ++i )
   if( get_int_par( i ) != get_dflt_int_par( i ) )
    ccfg->int_pars.push_back( std::make_pair( int_par_idx2str( i ) ,
					      get_int_par( i ) ) );

  for( int i = 0 ; i < get_num_dbl_par() ; ++i )
   if( get_dbl_par( i ) != get_dflt_dbl_par( i ) )
    ccfg->dbl_pars.push_back( std::make_pair( dbl_par_idx2str( i ) ,
					      get_dbl_par( i ) ) );

  for( int i = 0 ; i < get_num_str_par() ; ++i )
   if( ! ( get_str_par( i ) == get_dflt_str_par( i ) ) )
    ccfg->str_pars.push_back( std::make_pair( str_par_idx2str( i ) ,
					      get_str_par( i ) ) );

  if( ( ! ocfg ) && ( ! ccfg->int_pars.size() ) &&
      ( ! ccfg->dbl_pars.size() ) && ( ! ccfg->str_pars.size() ) ) {
   delete ccfg;
   ccfg = nullptr;
   }   
  }

 return( ccfg );

 }  // end( ThinComputeInterface::get_ComputeConfig )

/*--------------------------------------------------------------------------*/
/*------------------------ METHODS of ComputeConfig ------------------------*/
/*--------------------------------------------------------------------------*/

void ComputeConfig::deserialize( netCDF::NcGroup & group )
{
 // call the method of the base class, which does not much
 Configuration::deserialize( group );

 // f_diff field- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcGroupAtt diff = group.getAtt( "diff" );
 if( diff.isNull() )
  throw( std::invalid_argument( "missing diff in netCDF group" ) );

 int diffint;
 diff.getValues( &diffint );
 f_diff = diffint > 0;

 // int parameters- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcDim nip = group.getDim( "num_int_par" );
 if( nip.isNull() )
  throw( std::invalid_argument( "missing num_int_par in netCDF group" ) );

 size_t num_int_par = nip.getSize();
 if( num_int_par ) {
  netCDF::NcVar int_par_names = group.getVar( "int_par_names" );
  if( int_par_names.isNull() )
   throw( std::invalid_argument( "missing int_par_names in netCDF group" ) );

  netCDF::NcVar int_par_vals = group.getVar( "int_par_vals" );
  if( int_par_vals.isNull() )
   throw( std::invalid_argument( "missing int_par_vals in netCDF group" ) );

  int_pars.resize( num_int_par );
  for( size_t i = 0 ; i < num_int_par ; ++i ) {
   std::vector<size_t> idx = { i };
   int_par_names.getVar( idx , &(int_pars[ i ].first) );
   int_par_vals.getVar( idx , &(int_pars[ i ].second) );
   }
  }

 // double parameters - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcDim ndp = group.getDim( "num_dbl_par" );
 if( ndp.isNull() )
  throw( std::invalid_argument( "missing num_dbl_par in netCDF group" ) );

 size_t num_dbl_par = ndp.getSize();
 if( num_dbl_par ) {
  netCDF::NcVar dbl_par_names = group.getVar( "dbl_par_names" );
  if( dbl_par_names.isNull() )
   throw( std::invalid_argument( "missing dbl_par_names in netCDF group" ) );

  netCDF::NcVar dbl_par_vals = group.getVar( "dbl_par_vals" );
  if( dbl_par_vals.isNull() )
   throw( std::invalid_argument( "missing dbl_par_vals in netCDF group" ) );

  dbl_pars.resize( num_dbl_par );
  for( size_t i = 0 ; i < num_dbl_par ; ++i ) {
   std::vector<size_t> idx = { i };
   dbl_par_names.getVar( idx , &(dbl_pars[ i ].first) );
   dbl_par_vals.getVar( idx , &(dbl_pars[ i ].second) );
   }
  }

 // string parameters - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcDim nsp = group.getDim( "num_str_par" );
 if( nsp.isNull() )
  throw( std::invalid_argument( "missing num_str_par in netCDF group" ) );

 size_t num_str_par = nsp.getSize();
 if( num_str_par ) {
  netCDF::NcVar str_par_names = group.getVar( "str_par_names" );
  if( str_par_names.isNull() )
   throw( std::invalid_argument( "missing str_par_names in netCDF group" ) );

  netCDF::NcVar str_par_vals = group.getVar( "str_par_vals" );
  if( str_par_vals.isNull() )
   throw( std::invalid_argument( "missing str_par_vals in netCDF group" ) );

  str_pars.resize( num_str_par );
  for( size_t i = 0 ; i < num_str_par ; ++i ) {
   std::vector<size_t> idx = { i };
   str_par_names.getVar( idx , &(str_pars[ i ].first) );
   str_par_vals.getVar( idx , &(str_pars[ i ].second) );
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
 group.putAtt( "diff" , netCDF::NcInt() , int( f_diff ) );

 // int parameters- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcDim num_int_par = group.addDim( "num_int_par" , int_pars.size() );
 if( int_pars.size() ) {
  netCDF::NcVar int_par_names = group.addVar( "int_par_names" ,
					      netCDF::NcString() ,
					      num_int_par );
  netCDF::NcVar int_par_vals = group.addVar( "int_par_names" ,
					     netCDF::NcInt() ,
					     num_int_par );
  for( size_t i = 0 ; i < int_pars.size() ; ++i ) {
   std::vector<size_t> idx = { i };
   int_par_names.putVar( idx , int_pars[ i ].first );
   int_par_vals.putVar( idx , int_pars[ i ].second );
   }
  }

 // double parameters - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcDim num_dbl_par = group.addDim( "num_dbl_par" , dbl_pars.size() );
 if( dbl_pars.size() ) {
  netCDF::NcVar dbl_par_names = group.addVar( "dbl_par_names" ,
					      netCDF::NcString() ,
					      num_dbl_par );
  netCDF::NcVar dbl_par_vals = group.addVar( "dbl_par_vals" ,
					     netCDF::NcDouble() ,
					     num_dbl_par );
  for( size_t i = 0 ; i < dbl_pars.size() ; ++i ) {
   std::vector<size_t> idx = { i };
   dbl_par_names.putVar( idx , dbl_pars[ i ].first );
   dbl_par_vals.putVar( idx , dbl_pars[ i ].second );
   }
  }

 // string parameters - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 netCDF::NcDim num_str_par = group.addDim( "num_str_par" , str_pars.size() );
 if( str_pars.size() ) {
  netCDF::NcVar str_par_names = group.addVar( "str_par_names" ,
					      netCDF::NcString() ,
					      num_str_par );
  netCDF::NcVar str_par_vals = group.addVar( "str_par_names" ,
					     netCDF::NcString() ,
					     num_str_par );
  for( size_t i = 0 ; i < str_pars.size() ; ++i ) {
   std::vector<size_t> idx = { i };
   str_par_names.putVar( idx , str_pars[ i ].first );
   str_par_vals.putVar( idx , str_pars[ i ].second );
   }
  }

 // "extra" Configuration - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( f_extra_Configuration ) {
  auto ec = group.addGroup( "extra" );
  f_extra_Configuration->serialize( ec );
  }
 }  // end( ComputeConfig::serialize( netCDF::NcGroup ) )

/*--------------------------------------------------------------------------*/

void ComputeConfig::print( std::ostream &output ) const {
 output << "ComputeConfig";
 if( f_diff ) output << "[diff]";
 output << ": " << std::endl;
 for( auto pair : int_pars )
  output << pair.first << " = " << pair.second << std::endl;
 for( auto pair : dbl_pars )
  output << pair.first << " = " << pair.second << std::endl;
 for( auto pair : str_pars )
  output << pair.first << " = " << pair.second << std::endl;
 output << *f_extra_Configuration;
 }

/*--------------------------------------------------------------------------*/

void ComputeConfig::load( std::istream &input )
{
 input >> eatcomments >> f_diff;

 int k;
 input >> eatcomments >> k;
 int_pars.resize( k );
 for( int i = 0 ; i < k ; ++i )
  input >> eatcomments >> int_pars[ i ].first
	>> eatcomments >> int_pars[ i ].second;

 input >> eatcomments >> k;
 dbl_pars.resize( k );
 for( int i = 0 ; i < k ; ++i )
  input >> eatcomments >> dbl_pars[ i ].first
	>> eatcomments >> dbl_pars[ i ].second;

 input >> eatcomments >> k;
 str_pars.resize( k );
 for( int i = 0 ; i < k ; ++i )
  input >> eatcomments >> str_pars[ i ].first
	>> eatcomments >> str_pars[ i ].second;

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) ) {
  input.get();
  f_extra_Configuration = nullptr;
  }
 else {
  std::string cname;
  input >> cname;
  f_extra_Configuration = Configuration::new_Configuration( cname );
  input >> *f_extra_Configuration;
  }
 }  // end( ComputeConfig::load )

/*--------------------------------------------------------------------------*/
/*------------------ End File ThinComputeInterface.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
