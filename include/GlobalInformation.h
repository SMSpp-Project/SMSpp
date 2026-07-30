/*--------------------------------------------------------------------------*/
/*----------------------- File GlobalInformation.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the classes CollectionBase, Collection (and its
 * specialization for std::atomic types) and GlobalInformation, which
 * together provide a thread-safe, typed, named "blackboard" for the
 * information that a set of cooperating Solver working on the same problem
 * needs to share. Nothing in it is specific to any one algorithm: an
 * enumerative search sharing the incumbent and a pool of globally valid
 * cuts, a set of parallel heuristics exchanging the best solution found so
 * far, or an asynchronous decomposition method are all equally valid users.
 * By agreement on a name and a type, any part of the code can create, read
 * and write a piece of global data without GlobalInformation itself having
 * to know what it is.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Filippo Magi \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni, Donato Meoli, Filippo Magi
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __GlobalInformation
 #define __GlobalInformation
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

/*--------------------------------------------------------------------------*/
/*-------------------------- NAMESPACE & USING -----------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup GlobalInformation_CLASSES Classes in GlobalInformation.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS CollectionBase ----------------------------*/
/*--------------------------------------------------------------------------*/
/// type-erased base for Collection< T >
/** CollectionBase exists for the sole purpose of letting GlobalInformation
 * hold, in a single homogeneous map, Collection< T > objects of different
 * (and a priori unrelated) T: every concrete Collection< T > is stored as a
 * std::shared_ptr< CollectionBase > and recovered through a
 * dynamic_pointer_cast to the T the caller expects [see
 * GlobalInformation::get_from_Universe()]. It carries no data and no
 * behaviour of its own. */

class CollectionBase
{
 public:

 virtual ~CollectionBase() = default;

 };  // end( class( CollectionBase ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS Collection ------------------------------*/
/*--------------------------------------------------------------------------*/
/// a thread-safe, named repository of values of a single type T
/** Collection< T > is a std::unordered_map< std::string , T > guarded by a
 * std::shared_mutex, i.e., many concurrent readers or a single writer at
 * any time: this is the concurrency pattern expected of a piece of global
 * information accessed by many cooperating Solver, possibly running in
 * different threads, which read it far more often than they write it (say,
 * every node of an enumeration tree checks the incumbent, comparatively few
 * improve it).
 *
 * The class exists as a stand-alone template, rather than being folded
 * directly into GlobalInformation, so that each named piece of global
 * information (an incumbent, a cut pool, a column pool, ...) can have its
 * own T and its own map, without forcing a single one-size-fits-all value
 * type on every use.
 *
 * Two forms of the template serve two different access patterns, and
 * declaring one or the other documents the intended use:
 *
 * - Collection< T > (this primary form) keeps plain T values, and every
 *   access goes through the Collection lock: the right choice for data
 *   accessed "once in a while", where the cost of the lock is irrelevant
 *   and T can be arbitrary (a whole pool, a std::vector, ...);
 *
 * - Collection< std::atomic< T > > (the specialization below) keeps atomic
 *   values meant to be read and written lock-free through a reference
 *   cached once [see operator[]()]: the right choice for "hot" scalars
 *   read at every step by everybody, the incumbent being the prototypical
 *   example.
 *
 * There deliberately is *no* erase(): entries are created once and never
 * removed. Since std::unordered_map is node-based, this means that
 * references and pointers to the mapped values are *never* invalidated
 * (rehashing only invalidates iterators), so a user can look a value up
 * once, cache the reference, and use it from then on; an erase() would
 * silently break that contract. Removal is also at odds with the
 * append-only discipline that makes sharing sound in the first place [see
 * GlobalInformation]. If a whole Collection is no longer needed it can be
 * dropped via GlobalInformation::remove_from_Universe(), which does not
 * disturb whoever is still holding it.
 *
 * The read_with() / write_with() member templates are the preferred way to
 * operate on a value found by key: they run the supplied functor under the
 * appropriate lock without copying T in and out, which matters whenever T
 * is not cheap to copy (e.g., a whole cut pool). read() / write() remain
 * for the common case where T is small enough that a copy is not a
 * concern.
 *
 * WARNING: std::shared_mutex is not reentrant, hence none of the methods
 * of this same Collection may be called from inside the functor passed to
 * read_with(), write_with() or for_each(), or the call will deadlock. */

template< typename T >
class Collection : public CollectionBase
{

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

 Collection() = default;           ///< constructor: does nothing

 ~Collection() override = default; ///< destructor: does nothing

/*--------------------------------------------------------------------------*/
/*------------------- METHODS FOR READING / WRITING ------------------------*/
/*--------------------------------------------------------------------------*/

 /// read the value stored under \p key into \p out
 /** Returns false, leaving \p out untouched, if \p key is not present. */

 bool read( const std::string & key , T & out ) const {
  std::shared_lock lock( f_mutex );
  auto it = f_data.find( key );
  if( it == f_data.end() )
   return( false );
  out = it->second;
  return( true );
  }

/*--------------------------------------------------------------------------*/
 /// insert or overwrite the value stored under \p key

 void write( const std::string & key , T value ) {
  std::unique_lock lock( f_mutex );
  f_data[ key ] = std::move( value );
  }

/*--------------------------------------------------------------------------*/
 /// apply a read-only functor to the value under \p key, under a read lock
 /** Returns false if \p key is not present, in which case \p func is not
  * invoked [see the WARNING in the class comment about reentrancy]. */

 template< typename Func >
 bool read_with( const std::string & key , Func && func ) const {
  std::shared_lock lock( f_mutex );
  auto it = f_data.find( key );
  if( it == f_data.end() )
   return( false );
  func( it->second );
  return( true );
  }

/*--------------------------------------------------------------------------*/
 /// apply a mutating functor to the value under \p key, under a write lock
 /** Returns false if \p key is not present, in which case \p func is not
  * invoked [see the WARNING in the class comment about reentrancy]. */

 template< typename Func >
 bool write_with( const std::string & key , Func && func ) {
  std::unique_lock lock( f_mutex );
  auto it = f_data.find( key );
  if( it == f_data.end() )
   return( false );
  func( it->second );
  return( true );
  }

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING STATE -------------------------*/
/*--------------------------------------------------------------------------*/

 /// tells whether \p key is currently present

 [[nodiscard]] bool contains( const std::string & key ) const {
  std::shared_lock lock( f_mutex );
  return( f_data.find( key ) != f_data.end() );
  }

/*--------------------------------------------------------------------------*/
 /// the current number of ( key , value ) pairs

 [[nodiscard]] size_t size() const {
  std::shared_lock lock( f_mutex );
  return( f_data.size() );
  }

/*--------------------------------------------------------------------------*/
 /// a snapshot of the current keys
 /** A copy, not a live view: exposing iterators over the underlying map
  * without the lock would let the caller race with concurrent writers. */

 [[nodiscard]] std::vector< std::string > keys() const {
  std::shared_lock lock( f_mutex );
  std::vector< std::string > result;
  result.reserve( f_data.size() );
  for( const auto & pair : f_data )
   result.push_back( pair.first );
  return( result );
  }

/*--------------------------------------------------------------------------*/
 /// apply a read-only functor to every ( key , value ) pair, one read lock
 /** The functor is invoked as func( key , value ) on every pair, all under
  * a single read lock [see the WARNING in the class comment about
  * reentrancy]. */

 template< typename Func >
 void for_each( Func && func ) const {
  std::shared_lock lock( f_mutex );
  for( const auto & pair : f_data )
   func( pair.first , pair.second );
  }

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

 mutable std::shared_mutex f_mutex;         ///< guards f_data

 std::unordered_map< std::string , T > f_data;  ///< the ( key , value ) map

 };  // end( class( Collection ) )

/*--------------------------------------------------------------------------*/
/*------------- CLASS Collection< std::atomic< T > > -----------------------*/
/*--------------------------------------------------------------------------*/
/// the Collection of "hot" values, read and written lock-free
/** Specialization of Collection for std::atomic< T > values. It exists
 * because the primary template does not compile with a std::atomic mapped
 * type (std::atomic is neither copyable nor movable, so reading out and
 * writing in by value is impossible), and it only does differently the
 * pieces that require it: read() loads, write() stores (creating the entry
 * if needed), and operator[]() hands out a *reference* to the atomic
 * itself.
 *
 * The reference is the whole point of this form: since entries are created
 * once and never removed [see the primary template about the absence of
 * erase()], the reference stays valid for the entire life of the
 * Collection, so the intended pattern is to look it up *once*, cache it,
 * and from then on load() / store() through it with whatever memory order
 * fits the use, entirely outside the Collection lock. For monotone
 * quantities - an incumbent that only ever improves - relaxed ordering
 * suffices: a stale read is never *wrong*, it is just information that has
 * not arrived yet.
 *
 * The internal mutex only guards the *structure* of the map (lookup and
 * entry creation), never the values, whose concurrent access is entirely
 * delegated to std::atomic. */

template< typename T >
class Collection< std::atomic< T > > : public CollectionBase
{

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

 Collection() = default;           ///< constructor: does nothing

 ~Collection() override = default; ///< destructor: does nothing

/*--------------------------------------------------------------------------*/
/*------------------- METHODS FOR READING / WRITING ------------------------*/
/*--------------------------------------------------------------------------*/

 /// read (load) the value stored under \p key into \p out
 /** Returns false, leaving \p out untouched, if \p key is not present. */

 bool read( const std::string & key , T & out ) const {
  std::shared_lock lock( f_mutex );
  auto it = f_data.find( key );
  if( it == f_data.end() )
   return( false );
  out = it->second.load();
  return( true );
  }

/*--------------------------------------------------------------------------*/
 /// write (store) \p value under \p key, creating the entry if needed
 /** Note that the write lock is only taken when the entry has to be
  * created; storing into an existing entry happens under the read lock,
  * concurrently with any number of other readers and writers, atomicity
  * being provided by the value itself. */

 void write( const std::string & key , T value ) {
  {
   std::shared_lock lock( f_mutex );
   auto it = f_data.find( key );
   if( it != f_data.end() ) {
    it->second.store( value );
    return;
    }
   }
  std::unique_lock lock( f_mutex );
  auto res = f_data.try_emplace( key , value );
  if( ! res.second )       // someone else created it in the meantime
   res.first->second.store( value );
  }

/*--------------------------------------------------------------------------*/
 /// reference to the atomic stored under \p key, created if not present
 /** Returns a reference to the std::atomic< T > stored under \p key,
  * value-initializing it (T{}) if it is not there yet. The reference is
  * *never* invalidated [see the class comment], so the intended use is to
  * call this once, cache the result, and load() / store() through it
  * lock-free from then on. */

 std::atomic< T > & operator[]( const std::string & key ) {
  {
   std::shared_lock lock( f_mutex );
   auto it = f_data.find( key );
   if( it != f_data.end() )
    return( it->second );
   }
  std::unique_lock lock( f_mutex );
  return( f_data.try_emplace( key , T{} ).first->second );
  }

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING STATE -------------------------*/
/*--------------------------------------------------------------------------*/

 /// tells whether \p key is currently present

 [[nodiscard]] bool contains( const std::string & key ) const {
  std::shared_lock lock( f_mutex );
  return( f_data.find( key ) != f_data.end() );
  }

/*--------------------------------------------------------------------------*/
 /// the current number of ( key , value ) pairs

 [[nodiscard]] size_t size() const {
  std::shared_lock lock( f_mutex );
  return( f_data.size() );
  }

/*--------------------------------------------------------------------------*/
 /// a snapshot of the current keys [see the primary template]

 [[nodiscard]] std::vector< std::string > keys() const {
  std::shared_lock lock( f_mutex );
  std::vector< std::string > result;
  result.reserve( f_data.size() );
  for( const auto & pair : f_data )
   result.push_back( pair.first );
  return( result );
  }

/*--------------------------------------------------------------------------*/
 /// apply a read-only functor to every ( key , atomic ) pair, one read lock
 /** The functor is invoked as func( key , atomic ) on every pair (read the
  * value with .load()), all under a single read lock [see the WARNING in
  * the primary template about reentrancy]. */

 template< typename Func >
 void for_each( Func && func ) const {
  std::shared_lock lock( f_mutex );
  for( const auto & pair : f_data )
   func( pair.first , pair.second );
  }

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

 mutable std::shared_mutex f_mutex;   ///< guards the structure of f_data

 std::unordered_map< std::string , std::atomic< T > > f_data;
                                      ///< the ( key , atomic value ) map

 };  // end( class( Collection< std::atomic< T > > ) )

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS GlobalInformation --------------------------*/
/*--------------------------------------------------------------------------*/
/// the "universe" of typed, named Collection shared by cooperating Solver
/** GlobalInformation is a data structure that centralizes the exchange of
 * information between algorithms working on the same problem: a registry,
 * keyed by name, of Collection< T > objects of arbitrary (heterogeneous)
 * type T. Whoever first needs a given piece of global information declares
 * it with add_to_Universe< T >( name ), and from then on any holder of the
 * GlobalInformation can get_from_Universe< T >( name ) the very same
 * Collection< T > and read or write it concurrently with the others,
 * relying on the thread-safety each Collection itself provides. Nothing
 * here is specific to any one algorithm: an enumerative search sharing the
 * incumbent and a pool of globally valid cuts, a set of parallel
 * heuristics exchanging the best solution found so far, or an asynchronous
 * decomposition method are all equally valid users, and can coexist on the
 * same instance as long as they agree on names and types.
 *
 * Nobody is ever *forced* to interact with a GlobalInformation: it is a
 * channel offered to whoever finds it useful, not a protocol. A Solver
 * handed one may ignore it entirely; a Solver never handed one must work
 * exactly as if the information did not exist.
 *
 * GlobalInformation does not own the Solver sharing it, nor is it owned by
 * any one of them: a pointer to it is only *lent* [see
 * ChangeSolver::set_global_information()], and multiple Solver, possibly
 * running in different threads, are meant to share the same instance for
 * as long as the cooperation lasts. Individual Collection< T > are
 * returned as std::shared_ptr, so a Collection remains valid for whoever
 * is still using it even if remove_from_Universe() drops it from the
 * registry concurrently.
 *
 * The registry itself is guarded by a std::shared_mutex, distinct from
 * (and unrelated to) the mutex inside each individual Collection< T >:
 * locking it only protects *finding* a named Collection, never its
 * *contents*, which is each Collection's own responsibility. The intended
 * discipline is therefore: get (and cache) the shared_ptr to the needed
 * Collection once, at the beginning, then only ever go through it; for the
 * "hot" values kept in a Collection< std::atomic< T > >, further cache the
 * reference to the individual atomic [see Collection< std::atomic< T >
 * >::operator[]()] and read it lock-free from then on.
 *
 * Two design rules make the sharing sound, and every user is expected to
 * respect them:
 *
 * - data is *append-only and monotone*: entries are added and improved,
 *   never removed or invalidated (the incumbent only gets better, a cut
 *   pool only grows). A reader working on a stale value is thus never
 *   *wrong*, only marginally less effective, which is what allows reading
 *   without synchronization and is also what would make an asynchronously
 *   replicated, distributed form of this structure straightforward;
 *
 * - values are either held directly by the Collection, or through
 *   *shared-ownership* handles: anything a Collection hands out must stay
 *   valid for whoever received it, so pools of polymorphic objects (say,
 *   the Change of a cut or column pool) are stored as std::shared_ptr,
 *   never as raw owning pointers, and this is what keeps the ownership
 *   well-defined when the same object is being consumed by several Solver
 *   at once.
 *
 * RESERVED NAMES AND KEYS. For unrelated algorithms to actually cooperate
 * they must agree on what things are called, so the names and keys of
 * general interest are reserved and documented here, forming a small
 * dictionary that also illustrates how the structure is meant to be used:
 *
 * - str_AtomicScalars ("AtomicScalars") names a
 *   Collection< std::atomic< double > > holding the hot lock-free scalar
 *   values of the whole cooperation;
 *
 * - str_Incumbent ("Incumbent") is the key, within str_AtomicScalars, of
 *   the objective function value of the best feasible solution found so
 *   far by *anybody*: not finite (say, + / - infinity according to the
 *   verse of the optimization, or NaN) when none has been found yet, and
 *   only ever improved by whoever writes it;
 *
 * - str_AtomicFlags ("AtomicFlags") names a
 *   Collection< std::atomic< bool > > holding the hot lock-free boolean
 *   flags of the whole cooperation;
 *
 * - str_LocalFixingAllowed ("LocalFixingAllowed") is the key, within
 *   str_AtomicFlags, telling whether incumbent-dependent local tightenings
 *   (say, a reduced-cost fixing folded into a branching Change) are
 *   currently sound: such a tightening is valid only for the incumbent in
 *   force when it is generated, which is fine within a single solve (the
 *   incumbent only improves) but not when state is retained across
 *   re-solves with different incumbents, and whoever drives the search
 *   clears the flag in that case; true when absent.
 *
 * Every other user defines and documents its own names and keys alongside
 * its own code (say, an enumerative Solver its cut pool), taking care not
 * to collide with the reserved ones: names are a single flat namespace
 * shared by all types. */

class GlobalInformation
{

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*------------------------- RESERVED NAMES / KEYS --------------------------*/
/*--------------------------------------------------------------------------*/

 /// name of the Collection< std::atomic< double > > of the hot scalars
 static constexpr const char * str_AtomicScalars = "AtomicScalars";

 /// key (in str_AtomicScalars) of the value of the best solution found
 static constexpr const char * str_Incumbent = "Incumbent";

 /// name of the Collection< std::atomic< bool > > of the hot flags
 static constexpr const char * str_AtomicFlags = "AtomicFlags";

 /// key (in str_AtomicFlags) of the local-tightenings-are-sound flag
 static constexpr const char * str_LocalFixingAllowed = "LocalFixingAllowed";

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

 GlobalInformation() = default;           ///< constructor: does nothing

 virtual ~GlobalInformation() = default;  ///< destructor: does nothing

/*--------------------------------------------------------------------------*/
/*------------- METHODS FOR ADDING / REMOVING COLLECTIONS ------------------*/
/*--------------------------------------------------------------------------*/

 /// create a new, empty Collection< T > under the given \p name
 /** Throws std::runtime_error if a Collection already exists under
  * \p name, regardless of its type: names are a single flat namespace
  * shared by all T. */

 template< typename T >
 void add_to_Universe( const std::string & name ) {
  std::unique_lock lock( f_mutex );

  auto res = f_Universe.emplace( name ,
                                 std::make_shared< Collection< T > >() );
  if( ! res.second )
   throw( std::runtime_error( "GlobalInformation::add_to_Universe: "
                              "Collection " + name + " already exists" ) );
  }

/*--------------------------------------------------------------------------*/
 /// retrieve the Collection< T > registered under \p name
 /** Returns nullptr if no Collection exists under \p name, or if one
  * exists but was registered with a T incompatible with the one requested
  * here (the dynamic_pointer_cast fails). The returned shared_ptr keeps
  * the Collection alive for as long as the caller holds it, even past a
  * concurrent remove_from_Universe(). */

 template< typename T >
 std::shared_ptr< Collection< T > > get_from_Universe(
					       const std::string & name ) {
  std::shared_lock lock( f_mutex );

  auto it = f_Universe.find( name );
  if( it == f_Universe.end() )
   return( nullptr );

  return( std::dynamic_pointer_cast< Collection< T > >( it->second ) );
  }

/*--------------------------------------------------------------------------*/
 /// const version of get_from_Universe()

 template< typename T >
 std::shared_ptr< const Collection< T > > get_from_Universe(
					 const std::string & name ) const {
  std::shared_lock lock( f_mutex );

  auto it = f_Universe.find( name );
  if( it == f_Universe.end() )
   return( nullptr );

  return( std::dynamic_pointer_cast< const Collection< T > >(
							    it->second ) );
  }

/*--------------------------------------------------------------------------*/
 /// tells whether some Collection (of any type) is registered under \p name

 [[nodiscard]] bool exists( const std::string & name ) const {
  std::shared_lock lock( f_mutex );
  return( f_Universe.find( name ) != f_Universe.end() );
  }

/*--------------------------------------------------------------------------*/
 /// remove the Collection registered under \p name, if any
 /** Does not invalidate the shared_ptr already held by other users of that
  * Collection [see the class comment]; it only makes \p name available
  * again for a future add_to_Universe(). */

 void remove_from_Universe( const std::string & name ) {
  std::unique_lock lock( f_mutex );
  f_Universe.erase( name );
  }

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

 mutable std::shared_mutex f_mutex;   ///< guards the registry f_Universe

 /// the registry of the named Collection
 std::unordered_map< std::string , std::shared_ptr< CollectionBase > >
  f_Universe;

 };  // end( class( GlobalInformation ) )

/** @} end( group( GlobalInformation_CLASSES ) ) ---------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* GlobalInformation.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File GlobalInformation.h -----------------------*/
/*--------------------------------------------------------------------------*/
