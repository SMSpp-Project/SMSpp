/*--------------------------------------------------------------------------*/
/*------------------------ File SMSTypedefs.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file defining a bunch of data types that are useful in mutiple
 * SMS++ classes, and therefore that it would be annoying to define as
 * public data types of some specific class. The file also provides:
 *
 * - some macros for easily using factories
 *
 * - some methods and macros for easily applying some operations to a
 *   boost::any in a way that is as much independent as possible to the shape
 *   of the content (individual/std::vector/boost::multi_array of [std::list]
 *   of [classes derived from] Variable/Constraint);
 *
 * - handles printing (in the sense of operator<<()) of boost::multi_array<>,
 *   std::list<> and std::vector<>.
 *
 * \version 0.11
 *
 * \date 15 - 07 - 2018
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Department of Applied Mathematics \n
 *         State University of Campinas, Brazil \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni, Rafael Durbano Lobato, Kostas
 * Tavlaridis-Gyparakis
 */
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __SMSTypedefs
 #define __SMSTypedefs

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <iostream>
#include <list>
#include <algorithm>
#include <memory>
#include <vector>
#include <unordered_map>
#include <map>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <list>
#include <tuple>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <type_traits>

#include <boost/any.hpp>
#include <boost/bind.hpp>
#include "boost/function.hpp"
#include "boost/functional/factory.hpp"
#include "boost/functional/forward_adapter.hpp"
#include <boost/multi_array.hpp>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*---------------------------- GENERAL TYPES -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup SMS_types General types useful in SMS++
 *
 * A few useful typedefs for types not directly tied to any of the major
 * classes of SMS++.
 * @{ */

 typedef std::vector<boost::any> Vec_any;
 ///< a vector of boost::any, i.e., almost anything

 typedef const std::vector<boost::any> c_Vec_any;
 ///< a const vector of boost::any, i.e., almost anything

 typedef Vec_any::iterator Vec_any_it;
 ///< iterator for a Vec_any

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 typedef std::vector<std::string> Vec_string;
 ///< a vector of strings (std::string)

 typedef Vec_string::iterator Vec_string_it;
 ///< iterator for a Vec_string

 typedef const std::vector<std::string> c_Vec_string;
 ///< a const vector of strings (std::string)

 typedef const c_Vec_string::const_iterator c_Vec_string_it;
 ///< iterator for a c_Vec_string

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 /// Inf<T> = infinity value for T
 template <typename T>
 class Inf {
 public:
  Inf() {}
  operator T() { return( std::numeric_limits<T>::infinity() ); }
  };

/*--------------------------------------------------------------------------*/
 /// public enum for types of SMS++ netCDF files
 /** Public enum for describing the different kinds of netCDF files that can
  * be read and produced by SMS++ objects (notably, Block and Configuration).
  * The value eLastFileParam is provided if some :Block or :Configuration
  * needs to read/write files with a specific structure. */

 enum smspp_netCDF_file_type {
  eProbFile ,      ///< a "complete" file with both Block and Configuration
  eBlockFile ,     ///< a file of Block
  eConfigFile ,    ///< a file of Configuration
  eLastFileParam   ///< first value available to define new file types
  };

/*@} -----------------------------------------------------------------------*/
/*------------------------- UTILITIES FOR FACTORIES ------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup macro_for_factories Macros to simplify insertion in factories
 *
 * The five macros
 *
 *   SMSpp_insert_in_factory_h
 *
 *   SMSpp_insert_in_factory_cpp_0( < class name > )
 *
 *   SMSpp_insert_in_factory_cpp_1( < class name > )
 *
 *   SMSpp_insert_in_factory_cpp_0_t( < class name > )
 *
 *   SMSpp_insert_in_factory_cpp_1_t( < class name > )
 *
 * can be used to quickly ensure that < class name > is inserted in the
 * corresponding factory. As the name says, they have to be put respectively
 * in the private part of < class name > declaration in the .h, and in
 * the .cpp of < class name > (if any, *exactly one* .cpp otherwise). The
 * "_k" versions of the _cpp macro refer to the fact that the constructor of
 * the base class has k parameters. The further "_t" versions need be used if
 * the class is template, since then a template specialization is needed and
 * this requires adding "template<>" at proper places. */
/*--------------------------------------------------------------------------*/

/* The macro defines a very small, "fake" class _init. Its only meaning is to
 * define a static member _initializer that is initialized in whatever object
 * the other macro SMSpp_insert_in_factory_cpp() is put as soon as the program
 * starts; when the constructor is called, it will register the class in the
 * f_factory of the appropriate base class.
 *
 * It also defines the private_name() method. */
 
#define SMSpp_insert_in_factory_h \
 static class _init { \
 public: \
  _init( void ); \
 } _initializer; \
  \
 virtual const std::string & private_name( void ) const override

/*--------------------------------------------------------------------------*/
/* These macros define three things:
 *
 * 1) the actual implementation of the ClassName::_init::_init( void )
 *    constructor;
 *
 * 2) the actual declaration of the ClassName::_initializer static object;
 *
 *
 * 3) the actual implementation of ClassName::private_name().
 *
 * Note the use of the "Stringification" operator "#" when converting the
 * macro parameter ClassName to its string representation.
 *
 * Note: the approach requires that there is only one "f_factory" field,
 * coming from the base class, visible to each of its derived classes.
 *
 * The alert reader may wonder why the ugly two-step mechanism to define the
 * object inserted in the factory in the "_1" version. This is due to the
 * fact that boost (up to and incl. 1.67) does not do 'perfect forwarding':
 * factory<>() requires lvalue arguments, but bind provides rvalues. */

#define SMSpp_insert_in_factory_cpp_0( ClassName ) \
 const std::string & ClassName::private_name( void ) const { \
  static const std::string my_name( #ClassName ); \
  return( my_name ); \
  } \
    \
 ClassName::_init::_init( void ) {				\
  f_factory()[ #ClassName ] = boost::factory<ClassName*>();	\
  } \
    \
 ClassName::_init ClassName::_initializer

#define SMSpp_insert_in_factory_cpp_1( ClassName )	\
 const std::string & ClassName::private_name( void ) const { \
  static const std::string my_name( #ClassName ); \
  return( my_name ); \
  } \
    \
 ClassName::_init::_init( void ) { \
  auto f = boost::factory<ClassName*>(); \
  auto f2 = boost::forward_adapter<decltype(f)>( f ); \
  f_factory()[ #ClassName ] = boost::bind<ClassName*>(f2,_1);	\
  } \
    \
 ClassName::_init ClassName::_initializer

#define SMSpp_insert_in_factory_cpp_0_t( ClassName ) \
 template<> \
 const std::string & ClassName::private_name( void ) const {	\
  static const std::string my_name( #ClassName ); \
  return( my_name ); \
  } \
    \
 template<> ClassName::_init::_init( void ) { \
  f_factory()[ #ClassName ] = boost::factory<ClassName*>();	\
  } \
    \
 template<> ClassName::_init ClassName::_initializer

#define SMSpp_insert_in_factory_cpp_1_t( ClassName ) \
 template<> \
 const std::string & ClassName::private_name( void ) const { \
  static const std::string my_name( #ClassName ); \
  return( my_name ); \
  } \
    \
 template<> ClassName::_init::_init( void ) { \
  auto f = boost::factory<ClassName*>(); \
  auto f2 = boost::forward_adapter<decltype(f)>( f ); \
  f_factory()[ #ClassName ] = boost::bind<ClassName*>(f2,_1);	\
  } \
    \
 template<> ClassName::_init ClassName::_initializer

/*@} -----------------------------------------------------------------------*/
/*------------------- HANDLE boost::any SPECIALIZATIONS --------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup boost_any_stuff Handling boost::any specializations for SMS++
 *
 *  Two separate approaches are provided to automate the task of applying some
 *  fixed operations to a boost::any in a way that is as much independent as
 *  possible to the shape of the content, which can typically be:
 *
 *  - a single pointer to an object of some type (Constraint, Variable or
 *    some of their derived classes);
 *
 *  - a pointer to a std::vector of objects of some type (Constraint,
 *    Variable or some of their derived classes);
 *
 *  - a pointer to a std::vector of pointers to objects of some type (...);
 *
 *  - a pointer to a boost::multi_array<K> of objects of some type (...);
 *
 *  - a pointer to a boost::multi_array<K> of pointers to objects of some
 *    type (...);
 *
 *  - a pointer to a single std::list of objects of some type (...);
 *
 *  - a pointer to a single std::list of pointers to objects of some type
 *    (...);
 *
 *  - a pointer to a std::vector of std::list of objects of some type (...);
 *
 *  - a pointer to a std::vector of std::list of pointers to objects of some
 *    type (...);
 *
 *  - a pointer to a boost::multi_array<K> of std::list of objects of some
 *    type (...);
 *
 *  - a pointer to a boost::multi_array<K> of std::list of pointers to
 *    objects of some type (...).
 *
 * This is provided through the four template functions
 *
 *   bool un_any_static( boost::any & any , F f , un_any_type<T> )
 *
 *   bool un_any_static_ptr( boost::any & any , F f , un_any_type<T> )
 *
 *   bool un_any_dyanamic( boost::any & any , F f , un_any_type<T> )
 *
 *   bool un_any_dyanamic_ptr( boost::any & any , F f , un_any_type<T> )
 *
 * and the four macros (which, however, behave as a bool-returning function)
 *
 *   #define un_any_thing( thing_type , my_thing , f )
 *
 *   #define un_any_thing_0( thing_type , my_thing , f )
 *
 *   #define un_any_thing_1( thing_type , my_thing , f )
 *
 *   #define un_any_thing_K( thing_type , my_thing , f )
 *
 * The difference between the two is that the functions take a "f" that is
 * a ( T & ) --> void function and it is applied to all *elements* in the
 * boost any (it could also be a ( T ) --> void function but this would
 * mean copying the object and noone wants that, right?), whereas the macros
 * take a "f" that is a *piece of code* that is applied to the *container*
 * of the elements, i.e., either a thing_type, or a std::vector<thing_type>,
 * or a boost::multi_array<thing_type>. This requires the piece of code to
 * be "type polymorphic" (it has to work in all three cases), which is
 * nontrivial and (to the best of our knowledge) cannot be obtained with
 * templates at all, whence the not-very-C++ approach of using macros.
 *  @{ */

/*--------------------------------------------------------------------------*/

template<unsigned short K> struct un_any_int {};
///< empty type, template over the integers, for recursive template sheningans

template<class T> struct un_any_type {};
///< empty type, template over a type, for template functions sheningans
/**< empty type for allowing to declare the expected inner type in
 * un_any_*() and un_any_*_ptr() */

/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_static( boost::any & any , F f , un_any_type<T> )
 *
 * is intended to take a boost:any that contains either:
 *
 * - a pointer (reference) to a T;
 *
 * - a pointer (reference) to a std::vector<T>;
 *
 * - a pointer (reference) to a  boost::multi_array<T , K> for "all" K;
 *
 * and apply the function "f" to all the objects of type T it contains. "f"
 * must be a ( T & ) --> void function (it could also be a ( T ) --> void
 * function but this would mean copying the object and noone wants that,
 * right?); a lambda would work perfectly there.
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any" did indeed contain one of the sought-for types, in
 * which case "f" have been applied to all its elements, and false if "any"
 * contained something else, and therefore "f" has not been applied to
 * anything. */

template<typename T , class F>
bool un_any_static( boost::any & any , F f , un_any_type<T> ) {
 if( any.type() == typeid( T * ) ) {
  auto & el = * boost::any_cast< T * >( any ); f( el ); return( true );
  }
 else
 if( any.type() == typeid( std::vector<T> * ) ) {
  auto & var = * boost::any_cast< std::vector<T> * >( any );
  for( auto & el : var )
   f( el );
  return( true );
  }
 else return( un_any_static( any , f , un_any_type<T>() , un_any_int < 2 >() ) );

 }

template<typename T , class F>
bool un_any_static( boost::any , F , un_any_type<T> , un_any_int<9> )
{
 return( false );
 }

template<typename T , class F , unsigned short K>
bool un_any_static( boost::any & any , F f , un_any_type<T> ,
		                             un_any_int<K> ) {
 if( any.type() == typeid( boost::multi_array< T , K > * ) ) {
  auto & var = * boost::any_cast< boost::multi_array<T , K> * >( any );
  T* p = var.data();
  for( boost::multi_array_types::size_type i = var.num_elements() ; i-- ; )
   f( *(p++) );
  return( true );
  }
 else return( un_any_static( any , f , un_any_type<T>() ,
			               un_any_int<K + 1>() ) );
 }

/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_static_2( boost::any & any1 , boost::any & any2 ,
 *                         F f , un_any_type<T> , un_any_type<U> )
 *
 * is intended to take two boost:any "any1" and "any2" so that they
 * contain respectively:
 *
 * - a pointer (reference) to a T and a pointer (reference) to a U;
 *
 * - a pointer (reference) to a std::vector<T> and a pointer (reference) to a
 *   std::vector<U>;
 *
 * - a pointer (reference) to a boost::multi_array<T , K> and a
 *   pointer (reference) to a boost::multi_array<U , K>, for "all" K;
 *
 * and apply the function "f" to all corresponding pairs of objects of type T
 * and U they contain. "f" must be a ( T & , U & ) --> void function (it could
 * also be a ( T , U ) --> void function but this would mean copying the object
 * and noone wants that, right?); a lambda would work perfectly there.
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any1" and "any2" did indeed contain one of the sought-for
 * pairs of types, in which case "f" have been applied to all its elements, and
 * false if "any1" or "any2" contained something else, and therefore "f" has
 * not been applied to anything.
 *
 * If "any1" and "any2" are std::vectors, then "f" will be applied to the i-th
 * elements of "any1" and "any2" for every position i that is present in both
 * vectors. If "any1" and "any2" are boost::multi_arrays, then the data of each
 * one is extracted as an array and then "f" is applied to the elements in the
 * i-th position of these arrays if and only if position i is present in both
 * arrays.
 *
 * Notice that in debug mode, the std::vectors are required to have the same
 * size and the boost:multi_arrays are required to have the same number of
 * dimensions and shape.
 */

template<typename T , typename U , class F>
bool un_any_static_2( const boost::any & any1 , const boost::any & any2 , F f ,
                      un_any_type<T> , un_any_type<U> ) {

  if( any1.type() == typeid( T * ) ) {
    auto & el1 = * boost::any_cast< T * >( any1 );

#ifdef DEBUG
    if( any2.type() != typeid( U * ) )
      throw std::invalid_argument( "un_any_static_2: "
                                   "type of second argument should be U *");
#endif

    auto & el2 = * boost::any_cast< U * >( any2 );
    f( el1 , el2 );
    return( true );
  }
  else {
    if( any1.type() == typeid( std::vector<T> * ) ) {
      auto & var1 = * boost::any_cast< std::vector<T> * >( any1 );

#ifdef DEBUG
    if( any2.type() != typeid( std::vector<U> * ) )
      throw std::invalid_argument( "un_any_static_2: "
                                   "type of second argument should be "
                                   "std::vector<U> *" );
#endif

      auto & var2 = * boost::any_cast< std::vector<U> * >( any2 );

#ifdef DEBUG
      if( var1.size() != var2.size() )
        throw std::logic_error( "un_any_static_2: "
                                "vectors must have the same size");
#endif

      auto i1 = var1.begin();
      auto i2 = var2.begin();

      for( ; i1 != var1.end() && i2 != var2.end() ; ++i1 , ++i2 )
        f( *i1 , *i2 );

      return( true );
    }
    else {
      return( un_any_static_2( any1 , any2 , f , un_any_type<T>() ,
                               un_any_type<U>() , un_any_int < 2 >() ) );
    }
  }
}

template<typename T , typename U , class F>
bool un_any_static_2( const boost::any , const boost::any , F , un_any_type<T> ,
                      un_any_type<U> , un_any_int<9> ) {
  return( false );
}

template<typename T , typename U , class F , unsigned short K>
bool un_any_static_2( const boost::any & any1 , const boost::any & any2 , F f ,
                      un_any_type<T> , un_any_type<U> , un_any_int<K> ) {
  if( any1.type() == typeid( boost::multi_array< T , K > * ) ) {
    auto & var1 = * boost::any_cast< boost::multi_array<T , K> * >( any1 );

#ifdef DEBUG
    if( any2.type() != typeid( boost::multi_array<U , K> * ) )
      throw std::invalid_argument( "un_any_static_2: "
                                   "type of second argument should be "
                                   "boost::multi_array<U , K> *");
#endif

    auto & var2 = * boost::any_cast< boost::multi_array<U , K> * >( any2 );

#ifdef DEBUG
    if( var1.num_dimensions() != var2.num_dimensions() ||
        ! std::equal( var1.shape() , var1.shape() + var1.num_dimensions() ,
                      var2.shape() ) )
      throw std::logic_error( "un_any_static_2: "
                              "multi_arrays must have the same shape");
#endif

    T* p1 = var1.data();
    U* p2 = var2.data();

    for( boost::multi_array_types::size_type i =
           std::min( var1.num_elements() , var2.num_elements() ) ; i-- ; )
      f( *(p1++) , *(p2++) );
    return( true );
  }
  else return( un_any_static_2( any1 , any2 , f , un_any_type<T>() ,
                              un_any_type<U>() , un_any_int<K + 1>() ) );
}

/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_static_2_create( const boost::any & any1 , boost::any & any 2 ,
 *                                un_any_type<T> , un_any_type<U> , F f ,
 *                                bool apply_f )
 *
 * is intended to take two boost:any "any1" and "any2" so that if "any1"
 * contains
 *
 * - a pointer (reference) to a T, then a U is created and a pointer to this
 *   newly created object is stored in "any2";
 *
 * - a pointer (reference) to a std::vector<T>, then a std::vector<U> is
 *   created having the same size as the vector pointed by "any1" and the
 *   pointer to this just created object is stored in "any2";
 *
 * - a pointer (reference) to a boost::multi_array<T , K>, then a
 *   boost::multi_array<U , K> is created having the same shape as the
 *   boost::multi_array pointed by "any1" and the pointer to this newly created
 *   object is stored in "any2", for "all" K.
 *
 * The function can work with any K, but a maximum K has to be fixed at compile
 * time; currently the maximum K is 8, but it may be easily extended to go
 * higher if needed.
 *
 * If the function "f" is present and "apply_f" is true, then the function
 * "f" is applied to all corresponding pairs of objects of types T and U that
 * any1 and any2 contain. "f" must be a ( T & , U & ) --> void function (it
 * could also be a ( T , U ) --> void function but this would mean copying
 * the object and noone wants that, right?); a lambda would work perfectly
 * there.
 *
 * Returns true if "any1" did indeed contain one of the sought-for types.
 */

template<typename T , typename U , class F>
bool un_any_static_2_create( const boost::any & any1 , boost::any & any2 ,
                             un_any_type<T> , un_any_type<U> ,
                             F f , bool apply_f = true ) {

  if( any1.type() == typeid( T * ) ) {
    any2 = new U();

    if( apply_f ) {
      auto & var1 = * boost::any_cast< T * >( any1 );
      auto & var2 = * boost::any_cast< U * >( any2 );

      f( var1 , var2 );
    }

    return( true );
  }
  else {
    if( any1.type() == typeid( std::vector<T> * ) ) {
      auto & var1 = * boost::any_cast< std::vector<T> * >( any1 );
      any2 = new std::vector<U>(var1.size());

      if( apply_f ) {
        auto & var2 = * boost::any_cast< std::vector<U> * >( any2 );
        auto i1 = var1.begin();
        auto i2 = var2.begin();
        for( ; i1 != var1.end() ; ++i1 , ++i2 )
          f( *i1 , *i2 );
      }

      return( true );
    }
    else {
      return( un_any_static_2_create( any1 , any2 , un_any_type<T>() ,
                                      un_any_type<U>() , un_any_int < 2 >() ,
                                      f , apply_f ) );
    }
  }
}

template<typename T , typename U , class F>
bool un_any_static_2_create( const boost::any & , boost::any & ,
                             un_any_type<T> , un_any_type<U> , un_any_int<9> ,
                             F f , bool apply_f = true ) {
  return( false );
}

template<typename T , typename U , class F , unsigned short K>
bool un_any_static_2_create( const boost::any & any1 , boost::any & any2 ,
                             un_any_type<T> , un_any_type<U> , un_any_int<K> ,
                             F f , bool apply_f = true ) {

  if( any1.type() == typeid( boost::multi_array< T , K > * ) ) {
    auto & var1 = * boost::any_cast< boost::multi_array<T , K> * >( any1 );
    auto first = var1.shape();
    std::vector<int> shape( first , first + var1.num_dimensions() );
    any2 = new boost::multi_array<U , K>( shape );

    if( apply_f ) {
      auto & var2 = * boost::any_cast< boost::multi_array<U , K> * >( any2 );

      T* p1 = var1.data();
      U* p2 = var2.data();

      for( boost::multi_array_types::size_type i =
             std::min( var1.num_elements() , var2.num_elements() ) ; i-- ; )
        f( *(p1++) , *(p2++) );
    }

    return( true );
  }
  else return( un_any_static_2_create( any1 , any2 , un_any_type<T>() ,
                                       un_any_type<U>() ,
                                       un_any_int<K + 1>() , f, apply_f ) );
}

template<typename T , typename U>
bool un_any_static_2_create( const boost::any & any1 , boost::any & any2 ,
                             un_any_type<T> , un_any_type<U> ) {
  return un_any_static_2_create( any1 , any2 ,
                                 un_any_type<T>() , un_any_type<U>() ,
                                 [](T& t,U& u){} , false );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/** The template function
 *
 *   bool un_any_static_ptr( boost::any & any , F f , un_any_type<T> )
 *
 * is intended to take a boost:any that contains either:
 *
 * - a pointer (reference) to a T * (pointer to T);
 *
 * - a pointer (reference) to a std::vector<T *> (...);
 *
 * - a pointer (reference) to a  boost::multi_array<T * , K> for "all" K;
 *
 * and apply the function "f" to all the objects of type T it contains. "f"
 * must be a ( T & ) --> void function (it could also be a ( T ) --> void
 * function but this would mean copying the object and noone wants that,
 * right?): note that it is *not* a ( T * ) --> void function, i.e., it is
 * passed the (reference to the) dereferenced object rather than the pointer.
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any" did indeed contain one of the sought-for types, in
 * which case "f" have been applied to all its elements, and false if "any"
 * contained something else, and therefore "f" has not been applied to
 * anything. */

template<typename T , class F>
bool un_any_static_ptr( boost::any & any , F f , un_any_type<T> ) {
 if( any.type() == typeid( T ** ) ) {
  auto & el = * boost::any_cast< T ** >( any ); f( *el ); return( true );
  }
 else
 if( any.type() == typeid( std::vector<T *> * ) ) {
  auto & var = * boost::any_cast< std::vector<T *> * >( any );
  for( auto & el : var )
   f( *el );
  return( true );
  }
 else return( un_any_static_ptr( any , f , un_any_type<T>() ,
				           un_any_int<2>() ) );
 }

template<typename T , class F>
bool un_any_static_ptr( boost::any , F , un_any_type<T> , un_any_int<9> )
{
 return( false );
 }

template<typename T , class F , unsigned short K>
bool un_any_static_ptr( boost::any & any , F f , un_any_type<T> ,
		                                 un_any_int<K> ) {
 if( any.type() == typeid( boost::multi_array< T * , K > * ) ) {
  auto & var = * boost::any_cast< boost::multi_array<T * , K> * >( any );
  typedef T * TP;
  TP *p = var.data();
  for( boost::multi_array_types::size_type i = var.num_elements() ; i-- ;
       ++p )
   f( *(*p) );
  return( true );
  }
 else return( un_any_static_ptr( any , f , un_any_type<T>() ,
			                   un_any_int<K + 1>() ) );
 }

/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_const_static( const boost::any & any , F f , un_any_type<T> )
 *
 * is intended to take a const boost:any that contains either:
 *
 * - a pointer (reference) to a T;
 *
 * - a pointer (reference) to a std::vector<T>;
 *
 * - a pointer (reference) to a  boost::multi_array<T , K> for "all" K;
 *
 * and apply the function "f" to all the objects of type T it contains. "f"
 * must be a ( T & ) --> void function (it could also be a ( T ) --> void
 * function but this would mean copying the object and noone wants that,
 * right?); a lambda would work perfectly there.
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any" did indeed contain one of the sought-for types, in
 * which case "f" have been applied to all its elements, and false if "any"
 * contained something else, and therefore "f" has not been applied to
 * anything. */

template<typename T , class F>
bool un_any_const_static( const boost::any & any , F f , un_any_type<T> ) {
 if( any.type() == typeid( T * ) ) {
  auto & el = * boost::any_cast< T * >( any ); f( el ); return( true );
  }
 else
 if( any.type() == typeid( std::vector<T> * ) ) {
  auto & var = * boost::any_cast< std::vector<T> * >( any );
  for( auto & el : var )
   f( el );
  return( true );
  }
 else return( un_any_const_static( any , f , un_any_type<T>() , un_any_int < 2 >() ) );

 }

template<typename T , class F>
bool un_any_const_static( const boost::any , F , un_any_type<T> , un_any_int<9> )
{
 return( false );
 }

template<typename T , class F , unsigned short K>
bool un_any_const_static( const boost::any & any , F f , un_any_type<T> ,
		                             un_any_int<K> ) {
 if( any.type() == typeid( boost::multi_array< T , K > * ) ) {
  auto & var = * boost::any_cast< boost::multi_array<T , K> * >( any );
  T* p = var.data();
  for( boost::multi_array_types::size_type i = var.num_elements() ; i-- ; )
   f( *(p++) );
  return( true );
  }
 else return( un_any_const_static( any , f , un_any_type<T>() ,
			               un_any_int<K + 1>() ) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/** The template function
 *
 *   bool un_any_const_static_ptr( const boost::any & any , F f , un_any_type<T> )
 *
 * is intended to take a const boost:any that contains either:
 *
 * - a pointer (reference) to a T * (pointer to T);
 *
 * - a pointer (reference) to a std::vector<T *> (...);
 *
 * - a pointer (reference) to a  boost::multi_array<T * , K> for "all" K;
 *
 * and apply the function "f" to all the objects of type T it contains. "f"
 * must be a ( T & ) --> void function (it could also be a ( T ) --> void
 * function but this would mean copying the object and noone wants that,
 * right?): note that it is *not* a ( T * ) --> void function, i.e., it is
 * passed the (reference to the) dereferenced object rather than the pointer.
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any" did indeed contain one of the sought-for types, in
 * which case "f" have been applied to all its elements, and false if "any"
 * contained something else, and therefore "f" has not been applied to
 * anything. */

template<typename T , class F>
bool un_any_const_static_ptr( const boost::any & any , F f , un_any_type<T> ) {
 if( any.type() == typeid( T ** ) ) {
  auto & el = * boost::any_cast< T ** >( any ); f( *el ); return( true );
  }
 else
 if( any.type() == typeid( std::vector<T *> * ) ) {
  auto & var = * boost::any_cast< std::vector<T *> * >( any );
  for( auto & el : var )
   f( *el );
  return( true );
  }
 else return( un_any_const_static_ptr( any , f , un_any_type<T>() ,
				           un_any_int<2>() ) );
 }

template<typename T , class F>
bool un_any_const_static_ptr( const boost::any , F , un_any_type<T> , un_any_int<9> )
{
 return( false );
 }

template<typename T , class F , unsigned short K>
bool un_any_const_static_ptr( const boost::any & any , F f , un_any_type<T> ,
		                                 un_any_int<K> ) {
 if( any.type() == typeid( boost::multi_array< T * , K > * ) ) {
  auto & var = * boost::any_cast< boost::multi_array<T * , K> * >( any );
  typedef T * TP;
  TP *p = var.data();
  for( boost::multi_array_types::size_type i = var.num_elements() ; i-- ;
       ++p )
   f( *(*p) );
  return( true );
  }
 else return( un_any_const_static_ptr( any , f , un_any_type<T>() ,
			                   un_any_int<K + 1>() ) );
 }

/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_dynamic( boost::any & any , F f , un_any_type<T> )
 *
 * is intended to take a boost:any that contains either:
 *
 * - a pointer (reference) to a std::list<T>;
 *
 * - a pointer (reference) to a std::vector<std::list<T> >;
 *
 * - a pointer (reference) to a  boost::multi_array<std::list<T> , K> for
 *   "all" K;
 *
 * and apply the function "f" to all the objects of type T it contains. Note
 * that "f" is applied to the *individual objects*, *not* to the *lists* of
 * object: in fact, "f" must be a ( T & ) --> void function (it could also
 * be a ( T ) --> void function but this would mean copying the object and
 * noone wants that, right?)
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any" did indeed contain one of the sought-for types, in
 * which case "f" have been applied to all its elements, and false if "any"
 * contained something else, and therefore "f" has not been applied to
 * anything. */

template<typename T , class F>
bool un_any_dynamic( boost::any & any , F f , un_any_type<T> ) {
 if( any.type() == typeid( std::list<T> * ) ) {
  auto & el = * boost::any_cast< std::list<T> * >( any );
  for( auto & ell : el )
   f( ell );
  return( true );
  }
 else
 if( any.type() == typeid( std::vector<std::list<T> > * ) ) {
  auto & var = * boost::any_cast< std::vector<std::list<T> > * >( any );
  for( auto & el : var )
   for( auto & ell : el )
    f( ell );
  return( true );
  }
 else return( un_any_dynamic( any , f , un_any_type<T>() ,
			                un_any_int<2>() ) );
 }

template<typename T , class F>
bool un_any_dynamic( boost::any , F , un_any_type<T> , un_any_int<9> )
{
 return( false );
 }

template<typename T , class F , unsigned short K>
bool un_any_dynamic( boost::any & any , F f , un_any_type<T> ,
		                              un_any_int<K> ) {
 if( any.type() == typeid( boost::multi_array< std::list<T> , K > * ) ) {
  auto & var =
       * boost::any_cast< boost::multi_array<std::list<T> , K> * >( any );
  std::list<T> *p = var.data();
  for( boost::multi_array_types::size_type i = var.num_elements() ; i-- ;
       ++p )
   for( auto & ell : *p )
    f( ell );
  return( true );
  }
 else return( un_any_dynamic( any , f , un_any_type<T>() ,
			                un_any_int<K + 1>() ) );
 }

/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_dynamic_2( boost::any & any1 , boost::any & any2 ,
 *                          F f , un_any_type<T> , un_any_type<U> )
 *
 * is intended to take two boost:any "any1" and "any2" so that they
 * contain respectively:
 *
 * - a pointer (reference) to a std::list<T> and a pointer (reference) to a U;
 *
 * - a pointer (reference) to a std::vector<std::list<T>> and a pointer
 *   (reference) to a std::vector<U>;
 *
 * - a pointer (reference) to a boost::multi_array<std::list<T> , K> and a
 *   pointer (reference) to a boost::multi_array<U , K>, for "all" K;
 *
 * and apply the function "f" to the objects they point to. "f" must be a
 * ( std::list<T> & , U & ) --> void function (it could also be a

 * ( std::list<T> , U ) --> void function but this would mean copying the
 * object and noone wants that, right?); a lambda would work perfectly there.
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any1" and "any2" did indeed contain one of the sought-for
 * pair of types, in which case "f" have been applied to all its elements, and
 * false if "any1" or "any2" contained something else, and therefore "f" has
 * not been applied to anything.
 *
 * Notice that in debug mode, the std::vectors are required to have the same
 * size and the boost:multi_arrays are required to have the same number of
 * dimensions and shape.
 */

template<typename T , typename U , class F>
bool un_any_dynamic_2( const boost::any & any1 , const boost::any & any2 ,
                       F f , un_any_type<T> , un_any_type<U> ) {

  if( any1.type() == typeid( std::list<T> * ) ) {
    auto & el1 = * boost::any_cast< std::list<T> * >( any1 );

#ifdef DEBUG
    if( any2.type() != typeid( U * ) )
      throw std::invalid_argument
        ( std::string("un_any_dynamic_2: "
                      "type of second argument should be ") +
          typeid( U * ).name() +
          std::string("(aka U *) but it is ") +
          any2.type().name() );
#endif

    auto & el2 = * boost::any_cast< U * >( any2 );
    f( el1, el2 );
    return( true );
  }
  else
    if( any1.type() == typeid( std::vector< std::list<T> > * ) ) {
      auto & var1 = * boost::any_cast< std::vector<std::list<T> > * >( any1 );

#ifdef DEBUG
      if( any2.type() != typeid( std::vector< U > * ) )
        throw std::invalid_argument
          ( std::string("un_any_dynamic_2: "
                        "type of second argument should be ") +
            typeid( std::vector<U> * ).name() +
            std::string("(aka std::vector<U> *) but it is ") +
            any2.type().name() );
#endif

      auto & var2 = * boost::any_cast< std::vector<U> * >( any2 );

#ifdef DEBUG
      if( var1.size() != var2.size() )
        throw std::invalid_argument( "un_any_dynamic_2: "
                                     "vectors have different sizes");
#endif

      auto i1 = var1.begin();
      auto i2 = var2.begin();

      for( ; i1 != var1.end() && i2 != var2.end() ; ++i1 , ++i2 )
        f( *i1 , *i2 );

      return( true );
    }
    else return( un_any_dynamic_2( any1 , any2 , f , un_any_type<T>() ,
                                   un_any_type<U>() , un_any_int<2>() ) );
}

template<typename T , typename U , class F>
bool un_any_dynamic_2( const boost::any , const boost::any , F ,
                       un_any_type<T> , un_any_type<U> , un_any_int<9> ) {
  return( false );
}

template<typename T , typename U , class F , unsigned short K>
bool un_any_dynamic_2( const boost::any & any1 , const boost::any & any2 , F f ,
                       un_any_type<T> , un_any_type<U> , un_any_int<K> ) {
  if( any1.type() == typeid( boost::multi_array< std::list<T> , K > * ) ) {
    auto & var1 =
      * boost::any_cast< boost::multi_array<std::list<T> , K> * >( any1 );

#ifdef DEBUG
      if( any2.type() != typeid( boost::multi_array< U , K > * ) )
          ( std::string("un_any_dynamic_2: "
                        "type of second argument should be ") +
            typeid( boost::multi_array< U , K > * ).name() +
            std::string("(aka boost::multi_array< U , K > *) "
                        "but it is ") + any2.type().name() );
#endif

      auto & var2 =
        * boost::any_cast< boost::multi_array< U , K > * >( any2 );

#ifdef DEBUG
    if( var1.num_dimensions() != var2.num_dimensions() ||
        ! std::equal( var1.shape() , var1.shape() + var1.num_dimensions() ,
                      var2.shape() ) )
      throw std::logic_error( "un_any_dynamic_2: "
                              "multi_arrays must have the same shape");
#endif

    std::list<T> *p1 = var1.data();
    U *p2 = var2.data();

    for( boost::multi_array_types::size_type i =
           std::min( var1.num_elements() , var2.num_elements() ) ; --i ; )
      f( *(p1++), *(p2++) );

    return( true );
  }
  else return( un_any_dynamic_2( any1 , any2 , f , un_any_type<T>() ,
                                 un_any_type<U>() , un_any_int<K + 1>() ) );
}


/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_dynamic_2_create( const boost::any & any1 ,
 *                                 boost::any & any 2 ,
 *                                 un_any_type<T> , un_any_type<U> ,
 *                                 F f , bool apply_f )
 *
 * is intended to take two boost:any "any1" and "any2" so that if "any1"
 * contains
 *
 * - a pointer (reference) to a std::list<T>, then a U is created and a pointer
 *   to this newly created object is stored in "any2";
 *
 * - a pointer (reference) to a std::vector<std::list<T>>, then a
 *   std::vector<U> is created having the same size as the vector pointed by
 *   "any1" and the pointer to this just created object is stored in "any2";
 *
 * - a pointer (reference) to a boost::multi_array<std::list<T> , K>, then a
 *   boost::multi_array<U , K> is created having the same shape as the
 *   boost::multi_array pointed by "any1" and the pointer to this newly created
 *   object is stored in "any2", for "all" K.
 *
 * The function can work with any K, but a maximum K has to be fixed at compile
 * time; currently the maximum K is 8, but it may be easily extended to go
 * higher if needed.
 *
 * If the function "f" is present and "apply_f" is true, then the
 * function "f" is applied to all corresponding pairs of objects of
 * types std::list<T> and U that any1 and any2 contain. "f" must be a
 * ( std::list<T> & , U & ) --> void function (it could also be a
 * ( std::list<T> , U ) --> void function but this would mean copying
 * the object and noone wants that, right?); a lambda would work
 * perfectly there.
 *
 * Returns true if "any1" did indeed contain one of the sought-for types.
 */

template<typename T , typename U , class F>
bool un_any_dynamic_2_create( const boost::any & any1 , boost::any & any2 ,
                              un_any_type<T> , un_any_type<U> ,
                              F f , bool apply_f ) {

  if( any1.type() == typeid( std::list<T> * ) ) {
    any2 = new U();

    if( apply_f ) {
      auto & var1 = * boost::any_cast< std::list<T> * >( any1 );
      auto & var2 = * boost::any_cast< U * >( any2 );
      f( var1, var2 );
    }

    return( true );
  }
  else
    if( any1.type() == typeid( std::vector< std::list<T> > * ) ) {
      auto & var1 = * boost::any_cast< std::vector< std::list<T> > * >( any1 );
      any2 = new std::vector< U >(var1.size());

      if( apply_f ) {
        auto & var2 = * boost::any_cast< std::vector< U > * >( any2 );

        auto i1 = var1.begin();
        auto i2 = var2.begin();

        for( ; i1 != var1.end() && i2 != var2.end() ; ++i1 , ++i2 )
          f( *i1 , *i2 );
      }

      return( true );
    }
    else
      return( un_any_dynamic_2_create( any1 , any2 , un_any_type<T>() ,
                                       un_any_type<U>() , un_any_int<2>() ,
                                       f , apply_f ) );
}

template<typename T , typename U , class F>
bool un_any_dynamic_2_create( const boost::any & , boost::any & ,
                              un_any_type<T> , un_any_type<U> ,
                              un_any_int<9> , F f , bool apply_f ) {
  return( false );
}

template<typename T , typename U , class F , unsigned short K>
bool un_any_dynamic_2_create( const boost::any & any1 , boost::any & any2 ,
                              un_any_type<T> , un_any_type<U> ,
                              un_any_int<K> , F f , bool apply_f ) {
  if( any1.type() == typeid( boost::multi_array< std::list<T> , K > * ) ) {
    auto & var1 =
      * boost::any_cast< boost::multi_array< std::list<T> , K> * >( any1 );
    auto first = var1.shape();
    std::vector<int> shape( first , first + var1.num_dimensions() );
    any2 = new boost::multi_array< U , K >( shape );

    if( apply_f ) {
      auto & var2 = * boost::any_cast< boost::multi_array< U , K> * >( any2 );

      std::list<T> *p1 = var1.data();
      U *p2 = var2.data();

      for( boost::multi_array_types::size_type i =
             std::min( var1.num_elements() , var2.num_elements() ) ; --i ; )
        f( *(p1++), *(p2++) );
    }


    return( true );
  }
  else return( un_any_dynamic_2_create( any1 , any2 , un_any_type<T>() ,
                                        un_any_type<U>() ,
                                        un_any_int<K + 1>() , f , apply_f ) );
}

template<typename T , typename U>
bool un_any_dynamic_2_create( const boost::any & any1 , boost::any & any2 ,
                              un_any_type<T> , un_any_type<U> ) {
  return un_any_dynamic_2_create( any1 , any2 ,
                                  un_any_type<T>() , un_any_type<U>() ,
                                  [](T& t,U& u){} , false );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/** The template function
 *
 *   bool un_any_dynamic_ptr( boost::any & any , F f , un_any_type<T> )
 *
 * is intended to take a boost:any that contains either:
 *
 * - a pointer (reference) to a std::list<T *> (pointers to T);
 *
 * - a pointer (reference) to a std::vector<std::list<T *> > (...);
 *
 * - a pointer (reference) to a  boost::multi_array<std::list<T *> , K> for
 *   "all" K;
 *
 * and apply the function "f" to all the objects of type T it contains. Note
 * that "f" is applied to the *individual objects*, *not* to the *lists* of
 * object: in fact, "f" must be a ( T & ) --> void function. Be careful that
 * "f" is *not* a ( T * ) --> void function, i.e., it is passed the
 * (reference to the) dereferenced object rather than the pointer (it could
 * also be a ( T ) --> void function but this would mean copying the object
 * and noone wants that, right?)
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any" did indeed contain one of the sought-for types, in
 * which case "f" have been applied to all its elements, and false if "any"
 * contained something else, and therefore "f" has not been applied to
 * anything. */

template<typename T , class F>
bool un_any_dynamic_ptr( boost::any & any , F f , un_any_type<T> ) {
 if( any.type() == typeid( std::list<T *> * ) ) {
  auto & el = * boost::any_cast< std::list<T *> * >( any );
  for( auto & ell : el )
   f( *ell );
  return( true );
  }
 else
 if( any.type() == typeid( std::vector<std::list<T *> > * ) ) {
  auto & var = * boost::any_cast< std::vector<std::list<T *> > * >( any );
  for( auto & el : var )
   for( auto & ell : el )
    f( *ell );
  return( true );
  }
 else return( un_any_dynamic_ptr( any , f , un_any_type<T>() ,
				            un_any_int<2>() ) );
 }

template<typename T , class F>
bool un_any_dynamic_ptr( boost::any , F , un_any_type<T> , un_any_int<9> )
{
 return( false );
 }

template<typename T , class F , unsigned short K>
bool un_any_dynamic_ptr( boost::any & any , F f , un_any_type<T> ,
		                                  un_any_int<K> ) {
 if( any.type() == typeid( boost::multi_array<std::list<T *> , K > * ) ) {
  auto & var =
     * boost::any_cast< boost::multi_array<std::list<T *> , K> * >( any );
  std::list<T *> *p = var.data();
  for( boost::multi_array_types::size_type i = var.num_elements() ; i-- ;
       ++p )
   for( auto & ell : *p )
    f( *ell );
  return( true );
  }
 else return( un_any_dynamic_ptr( any , f , un_any_type<T>() ,
			                    un_any_int<K + 1>() ) );
 }

/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_const_dynamic( const boost::any & any , F f , un_any_type<T> )
 *
 * is intended to take a const boost:any that contains either:
 *
 * - a pointer (reference) to a std::list<T>;
 *
 * - a pointer (reference) to a std::vector<std::list<T> >;
 *
 * - a pointer (reference) to a  boost::multi_array<std::list<T> , K> for
 *   "all" K;
 *
 * and apply the function "f" to all the objects of type T it contains. Note
 * that "f" is applied to the *individual objects*, *not* to the *lists* of
 * object: in fact, "f" must be a ( T & ) --> void function (it could also
 * be a ( T ) --> void function but this would mean copying the object and
 * noone wants that, right?)
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any" did indeed contain one of the sought-for types, in
 * which case "f" have been applied to all its elements, and false if "any"
 * contained something else, and therefore "f" has not been applied to
 * anything. */

template<typename T , class F>
bool un_any_const_dynamic( const boost::any & any , F f , un_any_type<T> ) {
 if( any.type() == typeid( std::list<T> * ) ) {
  auto & el = * boost::any_cast< std::list<T> * >( any );
  for( auto & ell : el )
   f( ell );
  return( true );
  }
 else
 if( any.type() == typeid( std::vector<std::list<T> > * ) ) {
  auto & var = * boost::any_cast< std::vector<std::list<T> > * >( any );
  for( auto & el : var )
   for( auto & ell : el )
    f( ell );
  return( true );
  }
 else return( un_any_const_dynamic( any , f , un_any_type<T>() ,
			                un_any_int<2>() ) );
 }

template<typename T , class F>
bool un_any_const_dynamic( const boost::any , F , un_any_type<T> , un_any_int<9> )
{
 return( false );
 }

template<typename T , class F , unsigned short K>
bool un_any_const_dynamic( const boost::any & any , F f , un_any_type<T> ,
		                              un_any_int<K> ) {
 if( any.type() == typeid( boost::multi_array< std::list<T> , K > * ) ) {
  auto & var =
       * boost::any_cast< boost::multi_array<std::list<T> , K> * >( any );
  std::list<T> *p = var.data();
  for( boost::multi_array_types::size_type i = var.num_elements() ; i-- ;
       ++p )
   for( auto & ell : *p )
    f( ell );
  return( true );
  }
 else return( un_any_const_dynamic( any , f , un_any_type<T>() ,
			                un_any_int<K + 1>() ) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/** The template function
 *
 *   bool un_any_dynamic_ptr( const boost::any & any , F f , un_any_type<T> )
 *
 * is intended to take a const boost:any that contains either:
 *
 * - a pointer (reference) to a std::list<T *> (pointers to T);
 *
 * - a pointer (reference) to a std::vector<std::list<T *> > (...);
 *
 * - a pointer (reference) to a  boost::multi_array<std::list<T *> , K> for
 *   "all" K;
 *
 * and apply the function "f" to all the objects of type T it contains. Note
 * that "f" is applied to the *individual objects*, *not* to the *lists* of
 * object: in fact, "f" must be a ( T & ) --> void function. Be careful that
 * "f" is *not* a ( T * ) --> void function, i.e., it is passed the
 * (reference to the) dereferenced object rather than the pointer (it could
 * also be a ( T ) --> void function but this would mean copying the object
 * and noone wants that, right?)
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any" did indeed contain one of the sought-for types, in
 * which case "f" have been applied to all its elements, and false if "any"
 * contained something else, and therefore "f" has not been applied to
 * anything. */

template<typename T , class F>
bool un_any_const_dynamic_ptr( const boost::any & any , F f , un_any_type<T> ) {
 if( any.type() == typeid( std::list<T *> * ) ) {
  auto & el = * boost::any_cast< std::list<T *> * >( any );
  for( auto & ell : el )
   f( *ell );
  return( true );
  }
 else
 if( any.type() == typeid( std::vector<std::list<T *> > * ) ) {
  auto & var = * boost::any_cast< std::vector<std::list<T *> > * >( any );
  for( auto & el : var )
   for( auto & ell : el )
    f( *ell );
  return( true );
  }
 else return( un_any_const_dynamic_ptr( any , f , un_any_type<T>() ,
				            un_any_int<2>() ) );
 }

template<typename T , class F>
bool un_any_const_dynamic_ptr( const boost::any , F , un_any_type<T> , un_any_int<9> )
{
 return( false );
 }

template<typename T , class F , unsigned short K>
bool un_any_const_dynamic_ptr( const boost::any & any , F f , un_any_type<T> ,
		                                  un_any_int<K> ) {
 if( any.type() == typeid( boost::multi_array<std::list<T *> , K > * ) ) {
  auto & var =
     * boost::any_cast< boost::multi_array<std::list<T *> , K> * >( any );
  std::list<T *> *p = var.data();
  for( boost::multi_array_types::size_type i = var.num_elements() ; i-- ;
       ++p )
   for( auto & ell : *p )
    f( *ell );
  return( true );
  }
 else return( un_any_const_dynamic_ptr( any , f , un_any_type<T>() ,
			                    un_any_int<K + 1>() ) );
 }

/*--------------------------------------------------------------------------*/
/** The four macro
 *
 *   #define un_any_thing( thing_type , my_thing , f )
 *
 *   #define un_any_thing_0( thing_type , my_thing , f )
 *
 *   #define un_any_thing_1( thing_type , my_thing , f )
 *
 *   #define un_any_thing_K( thing_type , my_thing , f )
 *
 * takes the boost::any "my_thing", that is assumed to only take values in
 * the correct types for a "thing" described by "thing_type". This means
 * that "thing_type" is expected to be:
 *
 * - an object of class Variable or of any class derived from Variable;
 *
 * - an object of class Constraint or of any class derived from Constraint;
 *
 * - a pointer to an object of class Variable or of any class derived from
 *   Variable;
 *
 * - a pointer to object of class Constraint or of any class derived from
 *   Constraint;
 *
 * and that "my_thing" has to be:
 *
 * - a pointer to a "thing_type";
 *
 * - a pointer to a std::vector of "thing_type";
 *
 * - a pointer to a boost::multi_array<K> of "thing_type";
 *
 * - a pointer to a std::list of "thing_type";
 *
 * - a pointer to a std::vector of std::list of "thing_type";
 *
 * - a pointer to a boost::multi_array<K> of std::list of "thing_type";
 *
 * for "all" K, and apply the type-independent block of code "f" to the
 * corresponding variable "var" of the type (among the above) that the
 * boost::any turns out to be. Note that "var" is, therefore *not always
 * of the same type*: it can be an object, an array, a multi-array, a
 * list, an array of lists, or a multi-array of lists (technically, "var"
 * is a reference to any of these). Hence, "f" is not a function with a
 * well-specified input type that is applied to all *elements of the
 * container*, but rather a *piece of code* that is applied to the
 * *container itself*. This requires the piece of code to be "type
 * polymorphic" (it has to work in all three cases), which is nontrivial
 * and (to the best of our knowledge) cannot be obtained with templates at
 * all, whence the not-very-C++ approach of using macros.
 *
 * Because this may be impossible to do, there are four macros:
 *
 *  - un_any_thing() applies the same "f" to all types of containers;
 *
 *  - un_any_thing_0() only applies "f" if "my_thing" is a single
 *    "thing_type";
 *
 *  - un_any_thing_1() only applies "f" if "my_thing" is a
 *    std::vector<"thing_type">;
 *
 *  - un_any_thing_K() only applies "f" if "my_thing" is a
 *    boost::multi_array<"thing_type" , K>.
 *
 * This is why, although these are macros, they have been structured to
 * "behave like functions", in the sense that they are an expression
 * returning a bool (this is obtained by the magic of defining a lambda
 * returning a bool and immediately evaluating it on "my_thing"). The
 * "function" returns true if "my_thing" did indeed contain one of the
 * sought-for types, in which case "f" has been executed with the
 * corresponding "var" of the right type, and false if "my_thing" contained
 * something else, and therefore "f" has not been executed at all anything.
 *
 * Note that, unlike in the un_any_*_*() functions, there is no distinction
 * between the static (single "thing_type" elements) and dynamic (lists of
 * "thing_type" elements), because one can (and perhaps must) separately
 * call
 *
 *   un_any_thing( basic_type , ... );
 *
 * and
 *
 *   un_any_thing( std:list<basic_type> , ... );
 *
 * The pesky part in these macros (in particular, in un_any_thing_K() and
 * therefore in un_any_thing()) is that they have to work with "all" K, but
 * a maximum K has to be fixed at compile time; currently the maximum K is
 * 8, but it may be easily extended to go higher if needed. */

#define un_any_thing( thing_type , my_thing , f ) \
 [&]( boost::any & _any ) -> bool { \
  if( un_any_thing_0( thing_type , _any , f ) ) return( true ); \
  else if( un_any_thing_1( thing_type , _any , f ) ) return( true ); \
  return( un_any_thing_K( thing_type , _any , f ) ); \
  }( my_thing )

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define un_any_thing_0( thing_type , my_thing , f )	\
 [&]( boost::any & _any ) -> bool { \
  if( _any.type() == typeid( thing_type * ) ) { auto & var = \
   * boost::any_cast< thing_type * >( _any ); f; return( true ); } \
  return( false ); \
  }( my_thing )

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define un_any_thing_1( thing_type , my_thing , f )	\
 [&]( boost::any & _any ) -> bool { \
  if( _any.type() == typeid( std::vector< thing_type > * ) ) { \
   auto & var = * boost::any_cast< std::vector< thing_type > * >( _any ); \
   f; return( true ); } \
  return( false ); \
  }( my_thing )

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define un_any_thing_K( thing_type , my_thing , f )	\
 [&]( boost::any & _any ) -> bool { \
  if( _any.type() == typeid( boost::multi_array< thing_type , 2 > * ) ) { \
   auto & var = \
   * boost::any_cast< boost::multi_array< thing_type , 2 > * >( _any ); f; \
   return( true ); } \
  if( _any.type() == typeid( boost::multi_array< thing_type , 3 > * ) ) { \
   auto & var = \
   * boost::any_cast< boost::multi_array< thing_type , 3 > * >( _any ); f; \
   return( true ); } \
  if( _any.type() == typeid( boost::multi_array< thing_type , 4 > * ) ) { \
   auto & var = \
   * boost::any_cast< boost::multi_array< thing_type , 4 > * >( _any ); f; \
   return( true ); } \
  if( _any.type() == typeid( boost::multi_array< thing_type , 5 > * ) ) { \
   auto & var = \
   * boost::any_cast< boost::multi_array< thing_type , 5 > * >( _any ); f; \
   return( true ); } \
  if( _any.type() == typeid( boost::multi_array< thing_type , 6 > * ) ) { \
   auto & var = \
   * boost::any_cast< boost::multi_array< thing_type , 6 > * >( _any ); f; \
   return( true ); } \
  if( _any.type() == typeid( boost::multi_array< thing_type , 7 > * ) ) { \
   auto & var = \
   * boost::any_cast< boost::multi_array< thing_type , 7 > * >( _any ); f; \
   return( true ); } \
  if( _any.type() == typeid( boost::multi_array< thing_type , 8 > * ) ) { \
   auto & var = \
   * boost::any_cast< boost::multi_array< thing_type , 8 > * >( _any ); f; \
   return( true ); } \
  return( false ); \
  }( my_thing )

/*@} -----------------------------------------------------------------------*/
/*----------------- PRINTING list, array and multi_array -------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup print_multi_arrays Printing lists, pairs, arrays and multi_arrays
 *
 * A few versions of operator<< for printing lists, pairs, arrays and
 * boost::multi_arrays.
 *  @{ */

template <class T1 , class T2>
std::ostream &operator<<( std::ostream &os , const std::pair<T1 , T2> &p )
{
 os << "( " << p.first << ", " << p.second << " )";
 return os;
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template <typename T , unsigned long K>
std::ostream &operator<<( std::ostream &os ,
			  const boost::multi_array<T , K> &A )
{
 const T* p = A.data();
 for( boost::multi_array_types::size_type i = A.num_elements() ; i-- ; ++p )
 {
  os << "[ ";
  for( boost::multi_array_types::size_type k = 0 ; k < K ; ) {
   os << ( p - A.origin()  ) / A.strides()[ k ] % A.shape()[ k ]
         +  A.index_bases()[ k ];
   if( ++k < K )
    os << ", ";
    }
  os << " ] = " << *p << std::endl;
  }

 return os;
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template <typename T , unsigned long K>
std::ostream &operator<<( std::ostream &os ,
			  const boost::multi_array<T *, K> &A )
{
 typedef T * TP;
 const TP * p = A.data();
 for( boost::multi_array_types::size_type i = A.num_elements() ; i-- ; ++p )
 {
  os << "[ ";
  for( boost::multi_array_types::size_type k = 0 ; k < K ; ) {
   os << ( p - A.origin()  ) / A.strides()[ k ] % A.shape()[ k ]
         +  A.index_bases()[ k ];
   if( ++k < K )
    os << ", ";
    }
  os << " ] = " << **p << std::endl;
  }

 return os;
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template<typename T>
std::ostream &operator<<( std::ostream &os , const std::vector<T> &l )
{
 for( unsigned int i = 0 ; i < l.size() ; ++i )
  os << "[ " << i << " ] = " << l[ i ] << std::endl;

 return os;
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template<typename T>
std::ostream &operator<<( std::ostream &os , const std::vector<T *> &l )
{
 for( unsigned int i = 0 ; i < l.size() ; ++i )
  os << "[ " << i << " ] = " << *l[ i ] << std::endl;

 return os;
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template<typename T>
std::ostream &operator<<( std::ostream &os , const std::list<T> &l )
{
 auto it = l.begin();
 for( unsigned int i = 0 ; i < l.size() ; ++i , ++it ) {
  os << i << " ) = " << *it << std::endl;
  }

 return os;
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template<typename T>
std::ostream &operator<<( std::ostream &os , const std::list<T *> &l )
{
 auto it = l.begin();
 for( unsigned int i = 0 ; i < l.size() ; ++i , ++it ) {
  os << i << " ) = " << **it << std::endl;
  }

 return os;
 }

/*@} -----------------------------------------------------------------------*/
/*----------------- LOADING things while skipping comments -----------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup eatcomments simple operator which eats up comments in a istream
 *
 *  @{ */

inline std::istream & eatcomments( std::istream& is )
{
 for(;;) {
  is >> std::ws;  // skip whitespaces
  if( is.peek() == is.widen( '#' ) )
   // a comment: skip the rest of line and move to next
   is.ignore( std::numeric_limits<std::streamsize>::max() , is.widen( '\n' ) );
  else
   break;
  }

 return( is );
 }

/*@} -----------------------------------------------------------------------*/
/*------------------ LOADING list, array and multi_array -------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup load_multi_arrays Loading lists and arrays
 *
 * A few versions of operator>> for loading pairs, lists and arrays. For lists
 * and arrays the format is always
 *
 * number of elements k
 * for i = 1 to k
 * - element of the list
 *
 * while for pairs the format is just
 *
 *  first element of the pair
 *  second element of the pair
 *
 * It is assumed that each element can be read with >> itself, which rules
 * out pointers. Elements are separated by whitespaces and comments (see
 * eatcomments above).
 *  @{ */

template <class T1 , class T2>
std::istream &operator>>( std::istream &is , std::pair<T1 , T2> &p )
{
 is >> eatcomments >> p.first;
 is >> eatcomments >> p.second;
 return is;
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template<typename T>
std::istream &operator>>( std::istream &is , std::vector<T> &l )
{
 unsigned int k;
 is >> eatcomments >> k;
 l.resize( k );
 for( auto & li : l  )
  is >> eatcomments >> li;

 return is;
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template<typename T>
std::istream &operator>>( std::istream &is , std::list<T> &l )
{
 unsigned int k;
 is >> eatcomments >> k;
 l.resize( k );
 for( auto & li : l  )
  is >> eatcomments >> li;

 return is;
 }

/*@} -----------------------------------------------------------------------*/

} // end( namespace SMS_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* SMSTypedefs.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File SMSTypedefs.h ---------------------------*/
/*--------------------------------------------------------------------------*/
