/*--------------------------------------------------------------------------*/
/*--------------------------- File SMSFactory.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file with the macros that insert a class in the factory of its
 * base class (SMSpp_insert_in_factory_h and SMSpp_insert_in_factory_cpp_*),
 * and the few helpers they use.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ DEFINITIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __SMSFactory
 #define __SMSFactory
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

// standard C++ libraries (alphabetical order)
#include <algorithm>
#include <cctype>
#include <map>
#include <string>

// boost libraries
#include "boost/function.hpp"
#include "boost/functional/factory.hpp"
#include "boost/functional/forward_adapter.hpp"

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/
/// namespace for the Structured Modeling System++ (SMS++)

namespace SMSpp_di_unipi_it
{
/*--------------------------------------------------------------------------*/
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
 * in the private part of < class name > declaration in the .h, and in the
 * .cpp of < class name > (if any, in *exactly one* .cpp otherwise). The "_k"
 * versions of the _cpp macro refer to the fact that the constructor of the
 * base class has k parameters. The further "_t" versions need be used if the
 * class is template, since then a template specialization is needed and this
 * requires adding "template<>" at proper places.
 *
 * IMPORTANT NOTE: while the macros properly define the code in the object,
 * it has to be actually included in the final executable. If a component
 * using this feature is not *explicitly* referenced anywhere in all the
 * linked objects, the linker may decide to exclude it from the final
 * executable. This is particularly common when the unit containing the object
 * is contained in a dynamic library. In this case you may end up seeing an
 * error of the kind
 *
 *     terminate called after throwing an instance of 'std::invalid_argument'
 *      what():  XXXX not present in YYYY factory
 *
 * If this happens, it is because the code inserting it in the factory has not
 * been included due to having been aggressively optimised out by the linker.
 * In this case, one must either forcibly include an object of the specific
 * type in the final main (which requires also #include-ing the corresponding
 * headers, and it is therefore best avoided), or use options of the linker
 * that disable this optimization; at the time of writing this is
 * "--no-as-needed" for g++ (to be passed as "-Wl,--no-as-needed" if the
 * linker is invoked via the compiler), and "-all_load" for clang++. Note that
 * the behaviour depends on the default settings of the linker and therefore
 * it may change even for the same compiler toolchain on different OS
 * distributions. Yet, activating said options may have unwanted side effects
 * that one may want to do without. This is why the further macro
 *
 *   SMSpp_ensure_load( < class name > )
 *
 * is provided. By adding it at the beginning of a .cpp, it does some dirty
 * but hopefully not terribly costly things that ensure that < class name >
 * is added to the corresponding factory. However, note that for this to
 * work the corresponding headed must be explicitly included; it is not
 * possible to automatically include files using a macro due to the fact that
 * the C preprocessor only does one pass, and while there are some very dirty
 * hacks around that we prefer to steer well clear of those. Anyway this
 * mechanism should only be used as last resort since it has a (hopefully,
 * little but) nonzero cost at runtime.
 * @{ */

/** The macro defines a very small, "fake" class _init. Its only meaning is to
 * define a static member _initializer that is initialized in whatever object
 * the other macro SMSpp_insert_in_factory_cpp() is put as soon as the program
 * starts; when the constructor is called, it will register the class in the
 * f_factory of the appropriate base class, as well as calling the
 * static_initialization() method of the class (useful for any other static
 * initialization, such as registering methods in the methods factory).
 *
 * It also defines the private_name() method and the _private_name() static
 * method which is the implementation of private_name() (but it is static,
 * and therefore can be called in initialization statements, whereas
 * private_name() is virtual and therefore it can not). */

#define SMSpp_insert_in_factory_h                                            \
 static class _init { public:  _init(); } _initializer;                      \
 const std::string & private_name() const override;                          \
 static const std::string & _private_name()

namespace SMSpp_type_traits
{
/* The name of template classes may have commas, which prevent them to be
 * directly passed as arguments to the SMSpp_insert_in_factory_cpp_*_t
 * macros, as the comma is then interpreted by the tokenizer as separating
 * different arguments of the macro. To allow passing such names as
 * arguments to these macros, some extra template shenanigans are needed.
 *
 * The get_type struct is used within those macros to obtain the type U of a
 * (template) class whose name may have been enclosed in parentheses. The
 * type T is not relevant, as it is used only to obtain a well-formed type
 * name which contains parentheses. To obtain the right type of any class
 * "ClassName" within these macros, one can do the following:
 *
 *     SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type
 *
 * where ClassName is the parameter of the macro. For instance, the class
 *
 *     SimpleConfiguration< std::pair< int , int > >
 *
 * can be inserted in the factory as:
 *
 *     SMSpp_insert_in_factory_cpp_0_t( ( SimpleConfiguration<
 *                                        std::pair< int , int > > ) );
 *
 * There, ClassName would be
 *
 *     ( SimpleConfiguration< std::pair< int , int > > )
 *
 * Note that *not* using the "(" and ")" would result in the macro
 * invocation to fail. */

template< typename T >
struct t;
template< typename T , class U >
struct t< T( U ) > { using type = U; };
}

/*--------------------------------------------------------------------------*/
/// reformats the string in the way required by the class factory
/** This functions takes a std::string && str and returns the string that has
 * been stripped of the whitespaces and of all enclosing pair of parentheses
 * so that it can be inserted in the factory. This can be useful to check if
 * an object out of the factory is of a particular class as in
 *
 *     if( object.classname() == SMSpp_classname_normalise( name_string ) )
 *
 * in the case name_string is not guaranteed to be free of whitespaces and
 * parentheses. */

inline std::string && SMSpp_classname_normalise( std::string && str ) {
 str.erase( std::remove_if( str.begin() , str.end() , ::isspace ) ,
            str.end() );
 while( str.front() == '(' ) {
  str.pop_back();
  str.erase( 0 , 1 );
  }
 return( std::move( str ) );
 }

/*--------------------------------------------------------------------------*/
/** The macros SMSpp_insert_in_factory_cpp_* do five things for the class
 * \p ClassName for which they are invoked:
 *
 * 1) the actual implementation of the ClassType::_init::_init( void )
 *    constructor, within which:
 *
 *    1.1) the class is inserted into the corresponding factory;
 *
 *    1.2) the static_initialization() method of the class is invoked;
 *
 * 2) the actual declaration of the ClassType::_initializer static object;
 *
 * 3) the actual implementation of the static ClassType::_private_name();
 *
 * 4) the actual implementation of virtual ClassType::private_name(), itself
 *    using ClassType::_private_name().
 *
 * The approach can be applied to any class deriving from a base class that
 * has the following (possibly, pure virtual) methods (that should be visible
 * to each of its derived classes, and therefore typically protected):
 *
 * - private_name(), defined e.g. as
 *
 *      virtual const std::string & private_name( void ) const = 0;
 *
 * - f_factory(), defined e.g. as
 *
 *      static XXXFactoryMap & f_factory( void );
 *
 *   where
 *
 *       using XXXFactory = boost::function< XXX *( XXX * ) >;
 *       using XXXFactoryMap = std::map< std::string , XXXFactory >;
 *
 *    and XXX is the base class
 *
 * - static_initialization(), defined e.g. as
 *
 *       static void static_initialization( void ) {}
 *
 *   (note that this need not be virtual because it will be called with the
 *   explicit classname::static_initialization() format and classname being
 *   that of the derived class, which requires care if XXX or some derived
 *   class from which classname further derives does something, as then the
 *   parent class static_initialization() will have to be explicitly called
 *   in the derived one)
 *
 * The \p ClassName parameter of the macros must be the type of a class
 * satisfying the above constraints, possibly enclosed in any number of
 * parentheses "( ... )"; that is, for ClassName == MyBlock, the following
 *
 *     SMSpp_insert_in_factory_cpp_0( MyBlock )
 *
 *     SMSpp_insert_in_factory_cpp_0( ( MyBlock ) )
 *
 *     SMSpp_insert_in_factory_cpp_0( ( ( MyBlock ) ) )
 *
 * are equivalent, save that
 *
 *     IN SOME CASES ONE PAIR OF PARENTHESES IS STRICTLY REQUIRED
 *
 * as detailed in the following.
 *
 *     IMPORTANT NOTE 1: THE STRING RETURNED BY [_]private_name(), WHICH IS
 *     THE ONE THAT IS USED TO INDEX THE FACTORY, IS STRIPPED OF ANY
 *     WHITESPACE AND ENCLOSING PARENTHESES. For instance, a template class
 *     like MyBlock< std::pair< int , int > > gets name
 *     "MyBlock< std::pair< int , int > >" (which is syntactically wrong due to
 *     the closing ">>" instead of "> >", but after all it is only a string).
 *     This makes it possible to read it from a std::stream, where
 *     whitespaces are separators. If MyBlock derives from Block, it is then
 *     possible to create an object of class MyBlock with
 *
 *     new_Block( "MyBlock< std::pair< int , int > >" )
 *
 *     Note that the implementation of Block::my_Block() automatically strips
 *     all the whitespaces from the input string, which means that
 *
 *     new_Block( " MyBlock< std::pair< int , int > > " )
 *
 *     (and any similar version) works as well; however, this depends on
 *     my_Block() preprocessing its input, the string in the factory is
 *     the first one.
 *
 *     IMPORTANT NOTE 2: IF THE CLASS NAME CONTAINS ANY COMMAS, THE CLASS
 *     CLASS PASSED TO THE MACRO *MUST* BE ENCLOSED IN PARENTHESES. That
 *     is, class names whose "natural" form contain a comma (typically,
 *     some but not all of the template ones) will necessarily have to be
 *     enclosed in parentheses. For instance, the template class
 *
 *     MyBlock< std::pair< int , int > >
 *
 * must be registered in the factory by
 *
 *   SMSpp_insert_in_factory_cpp_1_t( ( MyBlock< std::pair< int , int > > ) );
 *
 * i.e., with at least one (possibly more, but there is no use in that) pair
 * of extra parentheses around the name of the class. For classes that do not
 * have commas in their names, the use of parentheses is optional. For
 * instance, the classes MyBlock and MyTBlock< int > could be registered in the
 * factory by
 *
 *    SMSpp_insert_in_factory_cpp_1_t( MyBlock );
 *
 *    SMSpp_insert_in_factory_cpp_1_t( MyTBlock< int > );
 *
 * or, equivalently,
 *
 *     SMSpp_insert_in_factory_cpp_1_t( ( MyBlock ) );
 *
 *     SMSpp_insert_in_factory_cpp_1_t( ( MyTBlock< int > ) );
 *
 * Note the use of the "stringification" operator "#" when converting the
 * macro parameter ClassName to its string representation.
 *
 * Note that a previous version of these macros required an ugly two-step
 * mechanism to define the object inserted in the factory in the "_1" version.
 * This was due to the fact that boost (up to and incl. 1.67) did not do
 * "perfect forwarding": factory<>() required lvalue arguments, but bind
 * provided rvalues. This flaw has apparently been fixed in later versions of
 * boost, and this implementation works with boost 1.74.0.
 *
 * The alert reader may similarly wonder why the funny "{}" after
 * "_initializer" in the _t versions, but not in the standard ones. This is
 * because for a template class (say, A, say template over the int) with a
 * static member (say, int a), something like
 *
 *   template<>
 *   int A<5>::a;
 *
 * is *not* a definition of the object for the specialization A<5>, as it
 * would be for a normal int, but rather a "specialization declaration" that
 * says "don't instantiate a from the primary template, because there is a
 * specialized definition somewhere else". In order to force it to be a
 * definition the object has to be explicitly initialized, as in
 *
 *   template<>
 *   int A<5>::a{};
 *
 * cue the funny "{}".
 *
 * As the ClassName may not be a type (as it may contain parentheses), we use
 * the trait get_type<> in order to extract the type of the class (ClassType
 * above) from ClassName.
 *
 * Finally, we remark that stripping the class name from whitespaces and
 * parentheses is done during the initialization of the static const
 * std::string my_name variable of _private_name() by using a lambda that is
 * defined and immediately called on #ClassName. This operation hence happens
 * *at runtime*, although only once the first time that _private_name() is
 * called (which is immediately as the class is registered in the factory).
 * This is slightly inefficient since the stripping should rather happen at
 * compile time; as C++-20 arrives most of std::algorithms will be
 * constexpr-able and therefore this will hopefully be possible. */

#define SMSpp_pp_cat_0( x , y ) x ## y
#define SMSpp_pp_cat( x , y ) SMSpp_pp_cat_0( x , y )

// using ellipsis to take all preprocessor-tokens
#define SMSpp_define_force_load( ... )                                       \
extern "C" void SMSpp_pp_cat( SMSpp_force_load_ , __VA_ARGS__ )( void );     \
extern "C" void SMSpp_pp_cat( SMSpp_force_load_ , __VA_ARGS__ )( void ) {}

// using ellipsis to take all preprocessor-tokens
#define SMSpp_insert_in_factory_cpp_0( ... )                                 \
 SMSpp_define_force_load( __VA_ARGS__ )                                      \
                                                                             \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name( void ) {  \
  static const std::string _name( SMSpp_classname_normalise(                 \
                                            std::string( #__VA_ARGS__ ) ) ); \
  return( _name );                                                           \
  }                                                                          \
                                                                             \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::private_name( void )     \
 const {                                                                     \
  return(                                                                    \
       SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ); \
  }                                                                          \
                                                                             \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init::_init( void ) {   \
  f_factory()[                                                               \
   SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ] =    \
    boost::factory< SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type * >(); \
  SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::static_initialization();\
  }                                                                          \
                                                                             \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init                    \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_initializer

// using ellipsis to take all preprocessor-tokens
#define SMSpp_insert_in_factory_cpp_1( ... )                                 \
 SMSpp_define_force_load( __VA_ARGS__ )                                      \
                                                                             \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name( void ) {  \
  static const std::string _name( SMSpp_classname_normalise(                 \
                                            std::string( #__VA_ARGS__ ) ) ); \
  return( _name );                                                           \
  }                                                                          \
                                                                             \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::private_name( void )     \
 const {                                                                     \
  return(                                                                    \
       SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ); \
  }                                                                          \
                                                                             \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init::_init( void ) {   \
  f_factory()[                                                               \
   SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ] =    \
    boost::factory< SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type * >(); \
  SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::static_initialization();\
  }                                                                          \
                                                                             \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init                    \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_initializer

// using ellipsis to take all preprocessor-tokens
#define SMSpp_insert_in_factory_cpp_0_t( ... )                               \
 template<>                                                                  \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name( void ) {  \
  static const std::string _name( SMSpp_classname_normalise(                 \
                                            std::string( #__VA_ARGS__ ) ) ); \
  return( _name );                                                           \
  }                                                                          \
                                                                             \
 template<>                                                                  \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::private_name( void )     \
 const {                                                                     \
  return(                                                                    \
       SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ); \
  }                                                                          \
                                                                             \
 template<>                                                                  \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init::_init( void ) {   \
  f_factory()[                                                               \
   SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ] =    \
    boost::factory< SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type * >(); \
  SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::static_initialization();\
  }                                                                          \
                                                                             \
 template<>                                                                  \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init                    \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_initializer{}

// using ellipsis to take all preprocessor-tokens
#define SMSpp_insert_in_factory_cpp_1_t( ... )                               \
 template<>                                                                  \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name( void ) {  \
  static const std::string _name( SMSpp_classname_normalise(                 \
                                            std::string( #__VA_ARGS__ ) ) ); \
  return( _name );                                                           \
  }                                                                          \
                                                                             \
 template<>                                                                  \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::private_name( void )     \
 const {                                                                     \
  return(                                                                    \
       SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ); \
  }                                                                          \
                                                                             \
 template<>                                                                  \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init::_init( void ) {   \
  f_factory()[                                                               \
   SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ] =    \
    boost::factory< SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type * >(); \
  SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::static_initialization();\
  }                                                                          \
                                                                             \
 template<>                                                                  \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init                    \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_initializer{}

/** @} ---------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* SMSFactory.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SMSFactory.h --------------------------*/
/*--------------------------------------------------------------------------*/
