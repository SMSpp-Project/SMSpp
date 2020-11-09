/** @file
 * Unit tests for ThinComputerInterface. Since it's an interface,
 * is it tested through its closest implementations.
 *
 * \author Niccolò Iardella \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni, Niccolò Iardella
 */

#include <gtest/gtest.h>

#include "C05Function.h"

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------- TEST CASES -------------------------------*/
/*--------------------------------------------------------------------------*/

TEST( ThinComputerInterfaceTest, GetsParameters) {
 C05Function * fun;



}
/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/
int main( int argc, char ** argv ) {
 ::testing::InitGoogleTest( &argc, argv );
 return RUN_ALL_TESTS();
}
