/*--------------------------------------------------------------------------*/
/*-------------------------- File SMSTerminate.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file with smspp_terminate(), the handler of std::terminate() that
 * the tools and the testers install by std::set_terminate() to print the
 * exception that terminates them.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ DEFINITIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __SMSTerminate
 #define __SMSTerminate
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

// standard C++ libraries (alphabetical order)
#include <cstdlib>
#include <exception>
#include <iostream>
#include <typeinfo>

// boost libraries
#include <boost/core/demangle.hpp>

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/
/// namespace for the Structured Modeling System++ (SMS++)

namespace SMSpp_di_unipi_it
{
/*--------------------------------------------------------------------------*/
/*---------------------------- smspp_terminate -----------------------------*/
/*--------------------------------------------------------------------------*/
/// handler of std::terminate() that prints the exception, if any, and aborts
/** Handler of std::terminate(), to be installed by
 *
 *     std::set_terminate( smspp_terminate );
 *
 * It prints the type, demangled, and the message of the exception that is
 * being handled, and then calls std::abort(). std::terminate() is also
 * called with no exception, e.g., by the destructor of a std::thread that
 * can still be joined: then std::current_exception() is null, and
 * rethrowing it is undefined behaviour, so the handler only says so and
 * aborts. */

inline void smspp_terminate( void )
{
 const auto ep = std::current_exception();
 if( ! ep ) {
  std::cerr << "std::terminate() called with no exception in SMS++"
            << std::endl;
  std::abort();
  }
 std::cerr << "Uncaught exception in executing SMS++:\n";
 try {
  std::rethrow_exception( ep );
  }
 catch( const std::exception & e ) {
  std::cerr << "\tException type: "
            << boost::core::demangle( typeid( e ).name() ) << "\n";
  std::cerr << "\tException message: " << e.what() << "\n";
  }
 catch( ... ) {
  std::cerr << "\tUnknown exception" << std::endl;
  }
 std::abort();
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* SMSTerminate.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SMSTerminate.h -------------------------*/
/*--------------------------------------------------------------------------*/
