/*--------------------------------------------------------------------------*/
/*---------------------------- File Solution.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the Solution class. The Solution class represents a
 * solution of a Block. A solution of a Block can be composed, for example,
 * by the values of the static and dynamic Variable and dual variables of
 * the Constraint of a Block.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __Solution
 #define __Solution  /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"

#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------ Solution-RELATED TYPES --------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Solution_TYPES Solution-related types
 *  @{ */

 class Block;            // forward definition of Block
 class Solution;         // forward definition of Solution

 typedef Solution * p_Solution;
 ///< a pointer to Solution

 typedef std::vector< p_Solution > Vec_Solution;
 ///< a vector of pointers to Solution

/** @}  end( group( Solution_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS Solution -------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a solution of a Block
/** The Solution class represents a solution of a Block. A solution of a
 * Block can be composed, for example, by the values of the static and
 * dynamic Variable and dual variables of the Constraint of a Block. This
 * Solution be used to store and retrieve those values. A Solution must
 * implement the method
 *
 *    void read( Block * const block )
 *
 * which takes a (pointer to a) Block, reads the values of the current
 * solution of this Block and stores them. Once the values of a solution of a
 * Block have been stored, they can be retrieved by the method
 *
 *    void write( Block * const block )
 *
 * which takes a (pointer to a) Block and writes the value of the solution
 * currently stored in this Solution into the Block.
 *
 * A Solution is normally constructed by the Block whose solution must be
 * stored. When it is constructed, a Solution object can be "configured" to
 * take only a specific part of the Block solution status (see the
 * Configuration parameter in Block::get_Solution()); say, only the primal
 * or the dual values, only the values of a specific set of Variable, ...
 * This configuration is "permanent": once a Solution is object created, it
 * will only store that particular set of information. Trying to read a
 * Solution from a Block that does not have the required information (say,
 * because dual information is required which is stored in some Constraint,
 * but these have not been constructed yet) is an error ans should result in
 * an exception being thrown.
 *
 * Of course, it is a fortiori an error (resulting in an exception being
 * thrown) to read or write a Solution out of the wrong Block. This does not
 * only mean "the wrong type of Block", but basically "the very same Block
 * that has created the solution object", or at least one that is "identical"
 * to it (say, a copy Block constructed as an R3 Block), or at the very very
 * least that is "compatible" (meaning it has the same size in the relevant
 * sets of Variable / Constraint). Solution are not meant to be exchanged
 * between different Block, even of the same type; exception to this rule
 * should be explicitly mentioned by each specific :Solution, and should be
 * exploited with due care.
 *
 * Solution also provides support for producing weighted sum of solutions of
 * a given Block, which in particular allows to produce convex combinations
 * of them (convexity being an all-important property, and convex relaxations
 * being at the heart of countless many optimization techniques). This is
 * somehow delicate because some Solution, in particular discrete ones, may
 * not "be happy" with being arbitrarily scaled and/or summed, and thus
 * requires some care, see the comments to scale() and sum(). */

class Solution {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/** @} ---------------------------------------------------------------------*/
/*----------------- CONSTRUCTING AND DESTRUCTING Solution ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing Solution
 *  @{ */

 Solution( void ) { }  ///< constructor of Solution, it has nothing to do

/*--------------------------------------------------------------------------*/

 Solution( const Solution & ) = delete;  ///< inhibit copy constructor

 /// inhibit assignment operator
 Solution & operator=( const Solution & ) = delete;

/*--------------------------------------------------------------------------*/
 /// construct a :Solution of specific type using the Solution factory
 /** Use the Solution factory to construct a :Solution object of type
  * specified by \p classname (a std::string with the name of the class
  * inside). Note that the method is static because the factory is static,
  * hence it is to be called as
  *
  *   Solution *mySolution = Solution::new_Solution( some_class );
  *
  * i.e., without any reference to any specific Solution (and, therefore, it
  * can be used to construct the very first Solution if needed).
  * 
  * For this to work, each :Solution has to:
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
  *   to exactly *one* .cpp file, typically the .cpp file of the :Block that
  *   the :Solution refers to. If the name of the class contains any
  *   parentheses, then one must enclose the name of the class in parentheses
  *   and instead add the line
  *
  *     SMSpp_insert_in_factory_cpp_0( ( name_of_the_class ) );
  *
  * Any whitespaces that the given \p classname may contain is ignored. So,
  * for example, to create an instance of the class MySolution< int > one
  * could pass "MySolution< int >" or "MySolution< int >", or even
  * " M y S o l u t i o n < int > ".
  *
  * Note that :Solution objects are generally constructed by their :Block,
  * and therefore there is often no need to use this method. However, a
  * :Solution may have to be serialized (see serialize()), and then
  * deserialized without the help of its :Block. This is possible with the
  * new_Solution( netCDF::NcGroup ) method, which requires this one (and
  * therefore the factory) to work.
  *
  * @param classname The name of the :Solution class that must be
  *        constructed. */

 static Solution * new_Solution( const std::string & classname ) {
  const std::string classname_( SMSpp_classname_normalise(
					        std::string( classname ) ) );
  const auto it = Solution::f_factory().find( classname_ );
  if( it == Solution::f_factory().end() )
   throw( std::invalid_argument( classname +
				 " not present in Solution factory" ) );
  return( ( it->second )() );
  }

/*--------------------------------------------------------------------------*/
 /// set the executable-wide prefix for all Solution filenames
 /** The static method deserialize( std::string ) is provided for loading a
  * Solution from a netCDF file. That method is in turn used for "file
  * redirection"; a Solution netCDF file can contain one (or many) filenames
  * in which a parts of the description of that Solution (typically a
  * Solution inside a Solution inside a Solution ...) can be found, see
  * new_Solution( netCDF::NcGroup ). It could arguably be convenient to be
  * able to specify filenames relative to some given prefix, so as to be
  * able to freely move the description of a Solution (which can be a rather
  * large object, and therefore require many files) across the filesystem.
  * Solution provides a *static* member for this purpose, that can be
  * set with this (static) method. Note that
  *
  *     BEING THE MEMBER STATIC, THE PREFIX IS APPLIED TO ALL LOADING 
  *     OPERATIONS OF ANY Solution IN THE EXECUTABLE
  *
  * Use of this feature therefore requires care. */

 static void set_filename_prefix( std::string && prefix ) {
  f_prefix = prefix;
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Solution out of a file
 /** Top-level de-serialization method: takes the \p filename of a SMS++ 
  * netCDF file (possibly also encoding a position into it), and returns the
  * complete :Solution object whose description is the one found (at the
  * specified position) in the file. \p filename has to be a SMS++ Solution
  * file, i.e., to have int netCDF attribute "SMS++_file_type" of value
  * eSolutionFile (see SMSTypedefs.h).
  *
  * SMS++ Solution files exploit the capacity of netCDF::NcFile to support
  * the notion of having multiple Solution inside; thus, \p filename can be
  * used to encode the position (Solution) in the file:
  *
  * - if the \p filename ends with ']', then is supposed to have the form
  *   "real filename[idx]": the "[idx] part is excised and used to compute
  *   the int parameter for deserialize( const netCDF::NcFile & , int ) (the
  *   position), with the remaining part being used for the string parameter
  *   (the filename);
  *
  * - otherwise, the whole string is used as the string parameter (the
  *   filename).
  *
  * If anything goes wrong with the entire operation, nullptr is returned.
  *
  * Note that if a filename prefix has been defined (for all Solution)
  * by means of set_filename_prefix(), then \p filename has to be intended
  * as relative to that prefix (in the sense that the prefix is prefix to
  * \p filename).
  *
  * Note that the method is static, hence it is to be called as
  *
  *     auto mySol = Solution::deserialize( somefile );
  *
  * i.e., without any reference to any specific Solution. */

 static Solution * deserialize( const std::string & filename );

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Solution out of an open netCDF SMS++ file
 /** Second-level de-serialization method: takes the open SMS++ netCDF
  * Solution file \p f and the index \p idx of a Solution into the file and
  * returns the correspinding complete :Solution object, which is the one
  * extracted by the "Solution_< \p idx >" group, by calling 
  * new_Solution( netCDF::NcGroup & ); see the corresponding comments for the
  * format options (that is, file indirection). Anything going wrong with the
  * entire operation (the file is not there, the "SMS++_file_type" attribute
  * is not there, there is no required "Solution_< \p idx >" child group,
  * there is any fatal error during the process, ...) results in nullptr
  * being returned.
  *
  * Note that the method is static, hence it is to be called as
  *
  *     Configuration *myConfig = Solution::deserialize( somefile );
  *
  * i.e., without any reference to any specific Solution. */

 static Solution * deserialize( const netCDF::NcFile & f , int idx = 0 );

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Solution out of netCDF::NcGroup, returns it
/** Third-level de-serialization method: takes a netCDF::NcGroup supposedly
  * containing (all the information describing) a :Solution (either
  * "directly" or "indirectly") and returns a pointer to a newly minted
  * Solution object corresponding to what is found in the group.
  *
  * The method works with two different kinds of netCDF::NcGroup:
  *
  * - A "direct" group that contains at least the string attribute "type";
  *   this is used it in the factory to construct an "empty" :Solution of
  *   that type [see new_Solution( string )], and then the method
  *   deserialize( netCDF::NcGroup ) of the newly minted :Solution is
  *   invoked (with argument \p group) to finish the work.
  *
  * - An "indirect" group that just need to contain the single string
  *   attribute "filename"; in this case, the attribute is used as argument
  *   for a call to deserialize( const std::string & ) that will extract the
  *   :Solution by the corresponding netCDF file (be it a text or  one).
  *   Note that the filename string can also be used to encode the position
  *   in the file, see the comments in the method for details.
  *
  * In case \p group contains both "type" and "filename", the first takes the
  * precedence (direct groups have precedence over indirect ones).
  *
  * Note that this method is static (see the previous versions for comments
  * about it) and returns a pointer to Solution, hence it has to have a
  * different name from deserialize( netCDF::NcGroup ) (since the signature
  * is the same but for the return type).
  *
  * If anything goes wrong with the process, nullptr is returned. */

 static Solution * new_Solution( const netCDF::NcGroup & group );

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Solution out of netCDF::NcGroup
 /** Fourth and final level de-serialization method: takes a netCDF::NcGroup
  * supposedly containing all the information required to de-serialize the
  * :Solution, and produces a "full" Solution object as a result. The
  * netCDF::NcGroup \p group may have been produced by calling serialize()
  * with a previously existing :Solution (of the very same type as this one),
  * but individual :Solution should openly declare the format of their
  * :Solution so that possibly a netCDF::NcGroup containing some pre-computed
  * solution can be constructed from scratch whenever this is useful.
  *
  * This method is pure virtual, as it clearly has to be implemented by
  * derived classes. */

 virtual void deserialize( const netCDF::NcGroup & group ) = 0;

/*--------------------------------------------------------------------------*/

 virtual ~Solution() {}  ///< destructor: it is virtual, and empty

/** @} ---------------------------------------------------------------------*/
/*--------------- Methods for reading the data of the Solution -------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the Solution
 *  @{ */

 /// getting the classname of this Solution
 /** Given a Solution, this method returns a string with its class name;
  * unlike std::type_info.name(), there *are* guarantees, i.e., the name will
  * always be the same.
  *
  * The method works by dispatching the private virtual method private_name().
  * The latter is automatically implemented by the 
  * SMSpp_insert_in_factory_cpp_* macros [see SMSTypedefs.h], hence this
  * comes at no cost since these have to be called somewhere to ensure that
  * any :Solution will be added to the factory. Actually, since
  * Solution::private_name() is pure virtual, this ensures that it is not
  * possible to forget to call the appropriate SMSpp_insert_in_factory_cpp_*
  * for any :Solution because otherwise it is a pure virtual class (unless
  * the programmer purposely defines private_name() without calling the macro,
  * which seems rather pointless). */

 const std::string & classname( void ) const { return( private_name() ); }

/** @} ---------------------------------------------------------------------*/
/*-------------- METHODS DESCRIBING THE BEHAVIOR OF A Solution -------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a Solution
 *  @{ */

 /// read the solution from the given Block
 /** This method reads the solution of the given Block and stores it in this
  * Solution. A Solution object can be "configured" to take only a specific
  * part of the Block solution status: it is an error if block does not have
  * the required part (and, a fortiori, if block is not the right Block). */

 virtual void read( const Block * const block ) = 0;

/*--------------------------------------------------------------------------*/
 /// write the solution in the given Block
 /** This method writes the solution currently stored in this Solution in the
  * given Block. A Solution object can be "configured" to take only a specific
  * part of the Block solution status: it is an error if block does not have
  * the required part (and, a fortiori, if block is not the right Block). */

 virtual void write( Block * const block ) = 0;

/*--------------------------------------------------------------------------*/
 /// returns a scaled version of this Solution
 /** This method constructs and returns a scaled version of this Solution,
  * where each of the solution information is scaled by the given double
  * value. A Solution object can be "configured" to take only a specific
  * part of the Block solution status: the scaled version of a Solution
  * object obviously "shares the same configuration" as the original object.
  *
  * Scaling by a double is a "somewhat dangerous" operation: it is quite
  * natural if all solution information is "double", which is what is most
  * likely to happen most of the time, but not in all cases. For instance,
  * some Block may correspond to combinatorial problems whose solutions are
  * essentially combinatorial objects (paths/cuts on a graph ...), for which
  * "scaling" makes little sense. Yet, these problems are typically also
  * represented in terms of subspaces of \R^n, and therefore one might
  * expect scaling to be possible. However, it is clear that scaling may
  * destroy some of the properties that solutions have: for instance, a
  * path in a graph can be represented by means of a predecessor function,
  * but a scaled path can not -- in the sense that it needs at least another
  * information, the scaling factor, or to be transformed into a different
  * format, such as the amount of "flow" going on each arc of the graph.
  *
  * This implies that there might be different Solution objects relative to
  * a given Block; say, the "original ones" corresponding to combinatorial
  * solutions (a path, represented by a predecessor function) and the "scaled
  * ones) produced by scaling and/or summing (see sum()) below (say, a double
  * for each arc of the graph). Thus, scale() might return a Solution object
  * that, while being appropriate for the original Block, may in fact be "of
  * a different type" than the originating one". The requirement is that the
  * newly created Solution must be a "general" one, in the sense that it
  * makes sense (obviously) to scale it, and also to *sum* it with other
  * Solution objects, see sum(). */

 virtual Solution * scale( double factor ) const = 0;

/*--------------------------------------------------------------------------*/
 /// adds a scaled version of the given Solution to this Solution
 /** This method adds a scaled version of the given Solution (see scale()) to
  * this Solution. A Solution object can be "configured" to take only a
  * specific part of the Block solution status: this means that even if the
  * Solution object pointed by solution has "more information" than the
  * current one, only the relevant part will be extracted and summed to that
  * of the current one, so that "the original configuration is preserved"
  * even after this operation. It is an error if solution does not have the
  * required part (and, a fortiori, if it is not the right Solution),
  * resulting in an exception being thrown.
  *
  * As discussed in scale(), scaling by a double is a "somewhat dangerous"
  * operation that may not necessarily make sense for all kinds of solution
  * information, in particular discrete ones. The same potentially holds for
  * sums (many combinatorial structures are closed under sum but not all are),
  * and a fortiori for "sum with a scaled object". Thus, some Solution may not
  * be able to properly implement this operation without fundamentally alter
  * their own internal representation, which is not supposed to happen. Thus,
  * 
  *    IT IS NOT NECESSARILY SAFE TO CALL sum() ON A Solution JUST
  *    PRODUCED BY Block::get_Solution()
  *
  * although in general it should always be possible to "configure the
  * Solution", by using the corresponding Configuration object to instruct
  * the Block to produce the kind of Solution object for which it is safe.
  * Furthermore,
  *
  *    IT IS SAFE TO CALL sum() ON A SOLUTION CONSTRUCTED BY scale()
  *
  * That is, scale() has to report a "general" Solution, one for which it
  * makes sense *both* scale it and sum it with other Solution objects.
  * Note that the idea is that is must be always possible to use "less
  * general" solution objects as the solution parameter in sum(): the
  * recipient (current) Solution object must be "general" for sum() to be
  * possible, but the summed one need not be. Of course, all this must be
  * entirely handled by the (different variants of) Solution.
  *
  * It should be remarked that there could be "intermediate" types of
  * Solution objects between the "less general" and the "more general" ones.
  * For instance, some discrete structures are closed under sum, or even
  * scaled sum where the scalar has appropriate properties (say, it's an
  * integer). Thus, it may not be efficient to require scale() to return the
  * "more general" Solution. Yet, handling these special cases should always
  * be possible by requiring the Block to produce "the right kind of Solution
  * object" by means of its Configuration. */

 virtual void sum( const Solution * solution , double multiplier ) = 0;

/*--------------------------------------------------------------------------*/
/// returns a clone of this Solution
/** This method creates and returns a Solution of the same type of this
 * Solution. If the parameter empty is true, then the returned Solution is
 * "empty", i.e., the solution information is not passed over, otherwise the
 * new Solution is a complete copy of the current one. A Solution object can
 * be "configured" to take only a specific part of the Block solution status:
 * the cloned object obviously "shares the same configuration" as the
 * original object.
 *
 * Note that clone() and scale( 1 ) return in principle the same Solution.
 * However, scale( 1 ) must return a "general solution" (see comments in
 * scale() and sum()), whereas clone() can return exactly the same type of
 * solution as the current one, i.e., a "less general" one if this is. */

 virtual Solution * clone( bool empty = false ) const = 0;

/** @} ---------------------------------------------------------------------*/
/*--------------- METHODS FOR PRINTING & SAVING THE Solution ---------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for printing & saving the Solution
 *
 * The base Solution class provides the friend operator<<() dispatching to a
 * protected virtual method print( std::ostream& ); the idea is that
 * derived classes will implement the latter in order to provide output on
 * std::stream.
 *
 * The base Solution class also defines the interface for serializing and
 * de-serializing a :Solution onto netCDF files. This is done via the two
 * versions of serialize() taking a file name (char *) and a netCDF file.
 * The first dispatches on the second, and the latter ultimately to the
 * protected method taking a netCDF group, like their deserialize()
 * counterparts.
 *  @{ */

 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected virtual method print(). This way operator<<() is
  * defined for each Solution, but its behavior can be customized by derived
  * classes. */

 friend std::ostream& operator<<( std::ostream& out , const Solution &s ) {
  s.print( out );
  return( out );
  }

/*--------------------------------------------------------------------------*/
 /// serialize a Solution to a netCDF file given the filename
 /** Method to serialize a Solution to a file in SMS++ netCDF-based Solution
  * format, given the \p filename. If \p replace == true (the default) any
  * existing content of the file is overwritten and the Solution is saved as
  * *the first one* in the newly created file, while if  \p replace == false
  * the file is opened for appending and the Solution is saved after the last
  * one currently present (if any).
  *
  * The base class implementation opens the netCDF file, creates the required
  * attribute "SMS++_file_type" and assigns it the eSolutionFile type, and
  * dispatches to serialize( netCDF::NcFile & ). If anything goes wrong with
  * any step of the process, exception is thrown. Although the method is
  * virtual, it is not expected that derived classes will have a need to
  * re-define it. */

 virtual void serialize( const std::string & filename , bool replace = true )
  const {
  netCDF::NcFile f( filename , replace ? netCDF::NcFile::replace
		                       : netCDF::NcFile::write );

  f.putAtt( "SMS++_file_type" , netCDF::NcInt() , eSolutionFile );

  serialize( f );
  }

/*--------------------------------------------------------------------------*/
 /// serialize a Solution to an open netCDF file
 /** Method to serialize a Solution to an open netCDF file in SMS++ Solution
  * format. The current Solution is *appended* after any existing Solution
  * in the file.
  *
  * The base class implementation creates the new group and dispatches to
  * serialize( netCDF::NcGroup ), which is where the :Solution-dependent
  * serialization happens. If anything goes wrong with any step of the
  * process, exception is thrown. Although the method is virtual, it is not
  * expected that derived classes will have a need to re-define it. */

 virtual void serialize( netCDF::NcFile & f ) const
 {
  auto cg = f.addGroup( "Solution_" + std::to_string( f.getGroupCount() ) );
  serialize( cg );
  }

/*--------------------------------------------------------------------------*/
 /// serialize a :Solution into a netCDF::NcGroup
 /** The method takes a (supposedly, "full") Solution object and serializes
  * it into the provided netCDF::NcGroup, so that it can possibly be read by
  * deserialize() (of a :Solution of the very same type as this one).
  *
  *      THIS IS THE METHOD TO BE IMPLEMENTED BY DERIVED CLASSES
  *
  * The method of the base class just creates and fills the "type" attribute
  * (with the right name, thanks to the classname() method) and the optional
  * "name" attribute. Yet
  *
  *     serialize() OF ANY :Solution SHOULD CALL Solution::serialize()
  *
  * While this currently does so little that one might well be tempted to
  * skip the call and just copy the one line of code, enforcing this
  * standard is forward-looking since in this way any future revision of the
  * base Solution class may add other mandatory/optional fields: as soon as
  * they are managed by the (revised) method of the base class, they would
  * then be automatically dealt with by the derived classes without them
  * even knowing it happened. */

 virtual void serialize( netCDF::NcGroup & group ) const {
  group.putAtt( "type" , classname() );
  }

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

  protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED TYPES ------------------------------*/
/*--------------------------------------------------------------------------*/

 typedef boost::function< Solution *() > SolutionFactory;
 // type of the factory of Solution

 typedef std::map< std::string, SolutionFactory > SolutionFactoryMap;
 // type of the map between strings and the factory of Solution

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
    @{ */

 /// print information about the Solution on an ostream
 /** Protected method intended to print information about the Solution; it is
  * virtual so that derived classes can print their specific information in
  * the format they choose. */

 virtual void print( std::ostream &output ) const {
  output << "Solution [" << this << "]";
  }

/** @} ---------------------------------------------------------------------*/
/** @name Protected methods for handling static fields
 *
 * These methods allow derived classes to partake into static initialization
 * procedures performed once and for all at the start of the program. These
 * are typically related with factories.
 * @{ */

 /// method incapsulating the Solution factory
 /** This method returns the Solution factory, which is a static object.
  * The rationale for using a method is that this is the "Construct On First
  * Use Idiom" that solves the "static initialization order problem". */

 static SolutionFactoryMap & f_factory( void ) {
  static SolutionFactoryMap s_factory;
  return( s_factory );
  }

/*--------------------------------------------------------------------------*/
 /// empty placeholder for class-specific static initialization
 /** The method static_initialization() is an empty placeholder which is made
  * available to derived classes that need to perform some class-specific
  * static initialization besides these of any :Solution class, i.e., the
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

 static std::string f_prefix;  ///< the executable-wide filename prefix

/** @} ---------------------------------------------------------------------*/
/*-------------------------- PROTECTED FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/
 // Definition of Solution::private_name() (pure virtual)

 [[nodiscard]] virtual const std::string & private_name( void ) const = 0;

/*--------------------------------------------------------------------------*/

 };  // end( class( Solution ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* Solution.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Solution.h ---------------------------*/
/*--------------------------------------------------------------------------*/
