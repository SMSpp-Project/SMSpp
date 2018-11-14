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
 * Copyright &copy by Antonio Frangioni
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

 input >> eatcomments >> k;
 if( input.peek() == input.widen( '*' ) )
  f_extra_Configuration = nullptr;
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
