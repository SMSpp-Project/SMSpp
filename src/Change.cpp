/*--------------------------------------------------------------------------*/
/*--------------------------- File Change.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the classes defined in Change.h: everything is defined
 * in the header, only the factory registration of the concrete GroupChange
 * lives here.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Filippo Magi \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni, Filippo Magi, Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Change.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register GroupChange to the Change factory

SMSpp_insert_in_factory_cpp_0( GroupChange );

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Change.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
