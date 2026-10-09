/*--------------------------------------------------------------------------*/
/*--------------------------- File TestAssert.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Makes assert() check in every build type of the tests, the Release one
 * (which defines NDEBUG) included, since the checks of a test are what the
 * test is made of.
 *
 * It has to be included AFTER every header of the library: some of them
 * declare different members according to NDEBUG, and they have to be read as
 * the library was compiled, or the test and the library would disagree on
 * the layout of the classes they share. The header has deliberately no
 * include guard, since <cassert> redefines assert() each time it is included.
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

#undef NDEBUG
#include <cassert>

/*--------------------------------------------------------------------------*/
/*------------------------- End File TestAssert.h --------------------------*/
/*--------------------------------------------------------------------------*/
