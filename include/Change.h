/*--------------------------------------------------------------------------*/
/*--------------------------- File Change.h --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the Change class, an *abstract* base class intended to 
 * provide objects to represent and apply a (set of) method(s) to change a 
 * Block. 
 * 
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Federica Di Pasquale \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Federica Di Pasquale
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __Change
 #define __Change
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "SMSTypedefs.h"

#include "Modification.h"

/*--------------------------------------------------------------------------*/
/*------------------------------ NAMESPACE ---------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Change_CLASSES Classes in Change.h
 *  @{ */

class Block;            // forward definition of Block

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS Change --------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// Base class for any possible change that can be applied to a Block
/** The class Change is an *abstract* base class intended to provide objects
 * to represent and apply a (set of) method(s) to change a Block.
 *
 * The main difference between Change objects and Modification objects is that
 * the latter are used to notify changes that have already occurred in a Block,
 * while the former are used to represent changes that may occur in a Block
 * also before they are actually applied. A method apply() is then provided to
 * apply all the changes represented by the Change object to a given Block. 
 * Therefore, Change objects need to contain all the necessary informations to 
 * apply the changes, i.e., all the new data of the Block.
 * 
 * Changes in a Block can be applied either to its Abstract Representation (AR) 
 * or to its Physical Representation (PR). Hence, derived classes of Change 
 * can be either "abstract" (and hence they can be applied to any Block) or 
 * "specific", i.e., for a specific derived class from Block. The base Change
 * class only provides the common interface, supporting the general virtual 
 * method apply( Block * ) that must be implemented by derived classes.
 * 
 * It is supposed that Change for specific derived classes from Block will use
 * the interface of the Block to directly change its PR. Therefore, the 
 * supported changes are at most the ones supported by the Block they refer 
 * to. However, it is possible that specific Change only support a smaller set 
 * of changes in order to be able to provide an "UndoChange". An "UndoChange" 
 * is a Change returned by the apply() method that can be used to restore the 
 * state of the Block prior to the application of the current Change. In fact, 
 * although this mechanism can be useful in some cases, it requires to possibly  
 * restrict the total number of supported changes, since creating an 
 * "UndoChange" of an arbitrary Change can be complicated and sometimes not 
 * supported. */

class Change {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

 Change( void ) = default;     ///< constructor: does nothing

 virtual ~Change() = default;  ///< destructor: does nothing

/*--------------------------------------------------------------------------*/
 /// construct a :Change of given type using the Change factory
 /** Use the Change factory to construct a :Change object of type specified
  * by classname (a std::string with the name of the class inside).
  * Note that the method is static because the factory is static, hence it 
  * has to be called as
  *
  *     Change * myChange = Change::new_Change( some_class );
  *
  * i.e., without any reference to any specific Change (and, therefore, it
  * can be used to construct the very first Change if needed).
  * 
  * For this to work, each :Change has to:
  *
  * - add the line
  *
  *     SMSpp_insert_in_factory_h;
  *
  *   to its definition (typically, in the private part in its .h file);
  *
  * - add the line
  *
  *     SMSpp_insert_in_factory_cpp_0( name_of_the_class );
  *
  *   to exactly *one* .cpp file, typically that :Change .cpp file. If the
  *   name of the class contains any parentheses, then one must enclose the
  *   name of the class in parentheses and instead add the line
  *
  *     SMSpp_insert_in_factory_cpp_0( ( name_of_the_class ) );
  *
  * Any whitespaces that the given \p classname may contain is ignored. So,
  * for example, to create an instance of the class MyChange< int > one could
  * pass "MyChange< int >" or "MyChange< int >" (even " M y C h a n g e < int >
  * would work).
  *
  * @param classname The name of the :Change class that must be constructed */

 static Change * new_Change( const std::string & classname ) {
  const std::string classname_( SMSpp_classname_normalise(
        std::string( classname ) ) );
  const auto it = Change::f_factory().find( classname_ );
  if( it == Change::f_factory().end() )
   throw( std::invalid_argument( classname + 
                                 " not present in Change factory" ) );
  return( ( it->second )() );
  }

 /*--------------------------------------------------------------------------*/
 /// de-serialize a :Change out of netCDF::NcGroup, returns it
 /** First-level, static de-serialization method: takes a netCDF::NcGroup
  * supposedly containing  (all the information describing) a :Change and
  * returns a pointer to a newly minted :Change object corresponding to
  * what is found in the file. The netCDF::NcGroup \p group must contain at
  * least the string attribute "type"; this is used it in the factory
  * to construct an "empty" :Change of that type, see new_Change(
  * std::string & ), and then the method deserialize( netCDF::NcGroup ) of
  * the newly minted :Change is invoked (with argument \p group) to finish
  * the work.
  *
  * Note that this method is static (see the previous versions for comments
  * about it) and returns a pointer to Change, hence it has to have a
  * different name from deserialize( netCDF::NcGroup ) (since the signature
  * is the same but for the return type).
  *
  * If anything goes wrong with the process, nullptr is returned. */

 static Change * new_Change( const netCDF::NcGroup & group ) {
  if( group.isNull() )
   return( nullptr );

  auto gtype = group.getAtt( "type" );
  if( gtype.isNull() )
   return( nullptr );

  std::string tmp;
  gtype.getValues( tmp );
  auto result = new_Change( tmp );
  try {
   result->deserialize( group );
   return( result );
   }
  catch( netCDF::exceptions::NcException & e ) {
   std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
   }
  catch( std::exception & e ) {
   std::cerr << "error " << e.what() << " in deserialize" << std::endl;
   }
  catch( ... ) {
   std::cerr << "unknown error in deserialize" << std::endl;
   }

  return( nullptr );
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Change out of a file
 /** Top-level de-serialization method: takes the \p filename of a file and
  * returns the complete :Change object contained in the file.
  * 
  * The method supports netCDF::NcFile and, since each file can contain
  * multiple Change inside, filename is used to encode the position in the 
  * file. If the \p filename ends with ']', then it is supposed to have the 
  * form "real filename[idx]": the "[idx] part is excised and used to compute
  * the int parameter of deserialize() (the position), with the remaining 
  * part being used for the string parameter (the filename);
  *
  * If anything goes wrong with the entire operation, nullptr is returned.
  *
  * Note that the method is static, hence it has to be called as
  *
  *     auto myChange = Change::deserialize( somefile );
  *
  * i.e., without any reference to any specific Change (and, therefore, it
  * can be used to construct the very first Change, if needed). */

 static Change * deserialize( const std::string & filename ) {
  
  try {
   // if filename does not contain "[idx]" at the end
   if( filename.back() != ']' ) {
    netCDF::NcFile f( filename , netCDF::NcFile::read );
    return( deserialize( f ) );
    }

   // otherwise find idx and return deserialize( , idx )
   auto pos = filename.find_last_of( '[' );
   std::string sidx( filename.begin() + pos + 1 , filename.end() - 1 );
   int idx = std::stoi( sidx );

   std::string fn( filename ); 
   fn.erase( pos );
   netCDF::NcFile f( fn , netCDF::NcFile::read );

   return( deserialize( f , idx ) );
   }
  catch( netCDF::exceptions::NcException & e ) {
   std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl; 
   }
  catch( std::exception & e ) {
   std::cerr << "error " << e.what() << " in deserialize" << std::endl; 
   }
  catch( ... ) {
   std::cerr << "unknown error in deserialize" << std::endl; 
   }

  return( nullptr ); 
  
  } // end( Change::deserialize( const std::string ) )

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Change out of an open netCDF SMS++ file
 /** Second-level de-serialization method: takes an open netCDF file and the
  * index of a Change into the file, and returns the correspinding complete 
  * :Change object.
  *
  * Note that the method is static, hence it is to be called as
  *
  *     Change *myChange = Change::deserialize( somefile );
  *
  * i.e., without any reference to any specific Change (and,
  * therefore, it can be used to construct the very first Change if needed).*/

 static Change * deserialize( const netCDF::NcFile & f , int idx = 0 ) {
  
  try {
   netCDF::NcGroup cg = f.getGroup( "Change_" + std::to_string( idx ) );
   return( new_Change( cg ) );
   }
  catch( netCDF::exceptions::NcException & e ) {
   std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
   }
  catch( std::exception & e ) {
   std::cerr << "error " << e.what() << " in deserialize" << std::endl;
   }
  catch( ... ) {
   std::cerr << "unknown error in deserialize" << std::endl;
   }

  return( nullptr );
  
  } // end( Change::deserialize( netCDF::NcFile ) )

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Change out of netCDF::NcGroup
 /** The method takes a netCDF::NcGroup supposedly containing all the
  * information required to de-serialize the :Change, and produces a "full"
  * Change object as a result. Most likely, the netCDF::NcGroup has been
  * produced by calling serialize() with a previously existing :Change (of
  * the very same type as this one), but individual :Change should openly
  * declare the format of their :Change so that possibly a netCDF::NcGroup
  * containing some pre-computed Change can be constructed from scratch
  * whenever this is useful.
  *
  * This method is pure virtual, as it clearly has to be implemented by
  * derived classes. */

 virtual void deserialize( const netCDF::NcGroup & group ) = 0;

/*--------------------------------------------------------------------------*/
 /// getting the classname of this Change
 /** Given a Change, this method returns a string with its class name;
  * unlike std::type_info.name(), there *are* guarantees, i.e., the name will
  * always be the same.
  *
  * The method works by dispatching the private virtual method private_name().
  * The latter is automatically implemented by the 
  * SMSpp_insert_in_factory_cpp_* macros [see SMSTypedefs.h], hence this
  * comes at no cost since these have to be called somewhere to ensure that
  * any :Change will be added to the factory. Actually, since 
  * Change::private_name() is pure virtual, this ensures that it is not
  * possible to forget to call the appropriate SMSpp_insert_in_factory_cpp_*
  * for any :Change because otherwise it is a pure virtual class
  * (unless the programmer purposely defines private_name() without calling
  * the macro, which seems rather pointless). */

 [[nodiscard]] const std::string & classname( void ) const {
  return( private_name() );
  }

/*--------------------------------------------------------------------------*/
 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected *pure* virtual method print(). This way the
  * operator<<() is defined for each Change, but its behavior must be
  * customized by derived classes (since the base class has nothing to
  * print). */

 friend std::ostream & operator<<( std::ostream & out , const Change & b ) {
  b.print( out );
  return( out );
  }

/*--------------------------------------------------------------------------*/
 /// serialize a Change to a netCDF file given the filename
 /** Method to serialize a Change to a file in netCDF-based given the 
  * filename. Note that any existing contect of the file is overwritten, and 
  * that the Change is saved as *the first one* in the newly created file.
  *
  * The base class implementation opens the netCDF file, and dispatches to
  * the netCDF file version of the method. Although the method is virtual, it
  * is not expected that derived classes will have a need to re-define it.  */

 virtual void serialize( const std::string & filename ) const {
  netCDF::NcFile f( filename, netCDF::NcFile::replace );
  serialize_f( f );
  }


/*--------------------------------------------------------------------------*/
 /// serialize a :Change to an open netCDF file
 /** The method takes a (supposedly, "full") Change object and serializes
  * it into the provided netCDF::NcFile, so that it can possibly be read by
  * deserialize() (of a :Change of the very same type as this one).
  *
  * The current Change is *appended* after any existing Change in the file
  * 
  * The method of the base class just creates and fills the "type" attribute
  * (with the right name, thanks to the classname() method) and the optional
  * "name" attribute. Yet
  *
  *     serialize() OF ANY :Change SHOULD CALL Change::serialize()
  *
  * While this currently does so little that one might well be tempted to
  * skip the call and just copy the three lines of code, enforcing this
  * standard is forward-looking since in this way any future revision of the
  * base Change class may add other mandatory/optional fields: as soon as
  * they are managed by the (revised) method of the base class, they would
  * then be automatically dealt with by the derived classes without them even
  * knowing it happened. */

 virtual void serialize_f( netCDF::NcFile & f ) const {
  auto cg = f.addGroup( "Change_" + std::to_string( f.getGroupCount() ) );
  serialize( cg );
  }

/*--------------------------------------------------------------------------*/
 /// serialize a Change to a netCDF NcGroup
 /** Method to serialize a Change to a netCDF NcGroup.
  *
  *      THIS IS THE METHOD TO BE IMPLEMENTED BY DERIVED CLASSES
  *
  * All the information required to de-serialize the Change need be saved in
  * the provided netCDF NcGroup, which is assumed to be "empty", starting 
  * with the "type" attribute that has to contain the classname() of the 
  * Change.
  *
  * The method of the base class just creates and fills the "type" attribute
  * (with the right name, thanks to the classname() method). Yet
  *
  *     serialize() OF ANY :Change SHOULD CALL Change::serialize()
  **/

 virtual void serialize( netCDF::NcGroup & group ) const {
  group.putAtt( "type" , classname() );
  }
 
/*--------------------------------------------------------------------------*/
 /// Apply a :Change to a :Block
 /** Apply a :Change to a :Block which is given as a parameter. As for any 
  * method that changes a Block, apply() has two parameters issueMod and 
  * issueAMod which control if and how the, respectively, "physical 
  * Modification" and "abstract Modification" corresponding to the Change 
  * have to be issue. 
  * 
  * If the second parameter doUndo == true, then the method returns an
  * "UndoChange", that is a :Change to restore the state of the Block prior 
  * to the application of the current :Change. 
  * 
  * The method is pure virtual hence it must be implemented by derived 
  * classes. */

 virtual Change * apply( Block * block , 
                         bool doUndo = false , 
                         ModParam issueMod = eNoBlck , 
                         ModParam issueAMod = eNoBlck ) = 0;

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED TYPES ------------------------------*/
/*--------------------------------------------------------------------------*/

 typedef boost::function< Change *( void ) > ChangeFactory;
 // type of the factory of Change

 typedef std::map< std::string , ChangeFactory > ChangeFactoryMap;
 // Type of the map between strings and the factory of Change

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for inserting and extracting
 *
 * The Change class provides two pairs of vaguely symmetric print() /
 * load() and serialize() / deserialize() methods to save information about
 * it on a std::stream / netCDF::NcGroup and retrieve it. For print() the
 * save information is *not* supposed to be enough to fully reconstruct the
 * original Change via load(), while this must be true for serialize().
 *   @{ */
/*--------------------------------------------------------------------------*/
 /// method for allowing any Change to print itself
 /** Method intended to provide support for Change to print themselves
  * out in human-readable form. The base Change class has preciously little
  * to print, but it still does a bit. */

 virtual void print( std::ostream & output ) const {
  output << "Change [" << this << "]" << std::endl;
  }

/** @} ---------------------------------------------------------------------*/
/** @name Protected methods for handling static fields
 *
 * These methods allow derived classes to partake into static initialization
 * procedures performed once and for all at the start of the program. These
 * are typically related with factories.
 * @{ */

 /// method incapsulating the Change factory
 /** This method returns the Change factory, which is a static object.
  * The rationale for using a method is that this is the "Construct On First
  * Use Idiom" that solves the "static initialization order problem". */

 static ChangeFactoryMap & f_factory( void ) {
  static ChangeFactoryMap c_factory;
  return( c_factory );
  }

/*--------------------------------------------------------------------------*/
 /// empty placeholder for class-specific static initialization
 /** The method static_initialization() is an empty placeholder which is made
  * available to derived classes that need to perform some class-specific
  * static initialization besides these of any :Change class, i.e., the
  * management of the factory. This method is invoked by the
  * SMSpp_insert_in_factory_cpp_* macros [see SMSTypedefs.h] during the
  * standard initialization procedures. If a derived class needs to perform
  * any static initialization it just have to do this into its version of
  * this method; if not it just has nothing to do, as the (empty) method of
  * the base class will be called.
  *
  * This mechanism has a potential drawback in that a redefined
  * static_initialization() may be called multiple times. Assume that a
  * derived class X redefines the method to perform something, and that a
  * further class Y is derived from X that has to do nothing, and that
  * therefore will not define Y::static_initialization(): them, within the
  * SMSpp_insert_in_factory_cpp_* of Y, X::static_initialization() will be
  * called again.
  *
  * If this is undesirable, X will have to explicitly instruct derived classes
  * to redefine their (empty) static_initialization(). Alternatively,
  * X::static_initialization() may contain mechanisms to ensure that it will
  * actually do things only the very first time it is called. One standard
  * trick is to do everything within the initialisation of a static local
  * variable of X::static_initialization(): this is guaranteed by the
  * compiler to happen only once, regardless of how many times the function
  * is called. Alternatively, an explicit static boolean could be used (this
  * may just be the same as what the compiler does during the initialization
  * of static variables without telling you). */

 static void static_initialization( void ) {}

/** @} ---------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

/** @} ---------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/
 // Definition of Change::private_name() (pure virtual)

 [[nodiscard]] virtual const std::string & private_name( void ) const = 0;

 };  // end( class( Change ) )

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif /* Change.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Change.h ----------------------------*/
/*--------------------------------------------------------------------------*/
