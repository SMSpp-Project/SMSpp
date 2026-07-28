/*--------------------------------------------------------------------------*/
/*------------------------ File GlobalInformation.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the classes CollectionBase, Collection and
 * GlobalInformation, which together provide the search-global, thread-safe
 * "shared memory" that an enumerative (Branch-and-Bound) algorithm hands to
 * the ChangeSolver [see ChangeSolver.h] driving each node.
 *
 * A ChangeSolver only ever sees the state local to the node it is currently
 * solving (reconstructed by apply()-ing Changes along a path of the tree);
 * anything that must instead be visible to - and contributed by - *every*
 * node, such as the current incumbent, a pool of globally valid cuts, or a
 * shared column pool, cannot live in the Change/apply() protocol without
 * turning every node into an explicit message to every other. Rather than
 * hard-wiring these into ChangeSolver, GlobalInformation exposes them as an
 * open, typed key-value store: any part of the code (the master enumerative
 * Solver, a ChangeSolver, an external observer) can, by agreement on a
 * string key and a type T, create, read and write a piece of global data
 * without GlobalInformation itself having to know what it is.
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

#include <memory>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

/*--------------------------------------------------------------------------*/
/*-------------------------- NAMESPACE & USING -----------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

    /*--------------------------------------------------------------------------*/
    /*------------------------------- CLASSES ----------------------------------*/
    /*--------------------------------------------------------------------------*/

    /*--------------------------------------------------------------------------*/
    /*----------------------------- CLASS CollectionBase ------------------------*/
    /*--------------------------------------------------------------------------*/
    /*--------------------------- GENERAL NOTES --------------------------------*/
    /*--------------------------------------------------------------------------*/
    /// type-erased base for Collection< T >
    /** CollectionBase exists for the sole purpose of letting GlobalInformation
     * hold, in a single homogeneous map, Collection< T > objects of different
     * (and a priori unrelated) T: every concrete Collection< T > is stored as
     * a std::shared_ptr< CollectionBase > and recovered through a
     * dynamic_pointer_cast to the T the caller expects [see
     * GlobalInformation::get_from_Universe()]. It carries no data and no
     * behaviour of its own. */

    class CollectionBase
    {
    public:
        virtual ~CollectionBase() = default;
    };

    /*--------------------------------------------------------------------------*/
    /*------------------------------- CLASS Collection ---------------------------*/
    /*--------------------------------------------------------------------------*/
    /*--------------------------- GENERAL NOTES --------------------------------*/
    /*--------------------------------------------------------------------------*/
    /// a thread-safe, named repository of values of a single type T
    /** Collection< T > is a std::unordered_map< std::string, T > guarded by a
     * std::shared_mutex, i.e., many concurrent readers or a single writer at
     * any time: this is the concurrency pattern expected of a piece of
     * search-global information accessed by many nodes of the enumeration
     * tree running in parallel, which are read far more often than written
     * (e.g., every node checks the incumbent, comparatively few improve it).
     *
     * The class exists as a stand-alone template - rather than being folded
     * directly into GlobalInformation - so that each named piece of global
     * information (the incumbent, a cut pool, a column pool...) can have its
     * own T and its own map, without forcing a single, one-size-fits-all
     * value type on every use.
     *
     * The read_with() / write_with() member templates are the preferred way
     * to operate on a value found by key: they run the supplied functor
     * under the appropriate lock without copying T in and out, which matters
     * whenever T is not cheap to copy (e.g., a whole cut pool). read() /
     * write() remain for the common case where T is small enough that a
     * copy is not a concern.
     *
     * for_each() gives read-only access to the whole Collection under a
     * single read lock; since std::shared_mutex is not reentrant, none of
     * the other methods of this same Collection may be called from inside
     * the functor passed to for_each(), read_with() or write_with(), or the
     * call will deadlock. */

    template <typename T>
    class Collection : public CollectionBase
    {

    public:
        /*--------------------------------------------------------------------------*/
        /*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
        /*--------------------------------------------------------------------------*/

        Collection() = default;

        ~Collection() override = default;

        /*--------------------------------------------------------------------------*/
        /*---------------------- METHODS FOR READING / WRITING ---------------------*/
        /*--------------------------------------------------------------------------*/

        /// read the value stored under \p key into \p out
        /** Returns false, leaving \p out untouched, if \p key is not present. */

        bool read(const std::string &key, T &out) const
        {
            std::shared_lock lock(f_mutex);
            auto it = f_data.find(key);
            if (it == f_data.end())
                return false;
            out = it->second;
            return true;
        }
        /// insert or overwrite the value stored under \p key

        void write(const std::string &key, T value)
        {
            std::unique_lock lock(f_mutex);
            f_data[key] = std::move(value);
        }

        /// apply a read-only functor to the value under \p key, under a read lock
        /** Returns false if \p key is not present, in which case \p func is not
         * invoked. */

        template <typename Func>
        bool read_with(const std::string &key, Func &&func) const
        {
            std::shared_lock lock(f_mutex);
            auto it = f_data.find(key);
            if (it == f_data.end())
                return false;
            func(it->second);
            return true;
        }

        /// apply a mutating functor to the value under \p key, under a write lock
        /** Returns false if \p key is not present, in which case \p func is not
         * invoked. */

        template <typename Func>
        bool write_with(const std::string &key, Func &&func)
        {
            std::unique_lock lock(f_mutex);
            auto it = f_data.find(key);
            if (it == f_data.end())
                return false;
            func(it->second);
            return true;
        }
        /// remove the value stored under \p key, if any; returns whether it was there

        bool erase(const std::string &key)
        {
            std::unique_lock lock(f_mutex);
            return f_data.erase(key) > 0;
        }

        /// tells whether \p key is currently present

        bool contains(const std::string &key) const
        {
            std::shared_lock lock(f_mutex);
            return f_data.find(key) != f_data.end();
        }

        /// the current number of (key, value) pairs

        size_t size() const
        {
            std::shared_lock lock(f_mutex);
            return f_data.size();
        }

        /// a snapshot of the current keys
        /** A copy, not a live view: exposing iterators over f_data without the
         * lock would let the caller race with concurrent writers. */

        std::vector<std::string> keys() const
        {
            std::shared_lock lock(f_mutex);
            std::vector<std::string> result;
            result.reserve(f_data.size());
            for (const auto &pair : f_data)
                result.push_back(pair.first);
            return result;
        }
        /// apply a read-only functor to every (key, value) pair, under one read lock
        /** WARNING: do not call any other method of this same Collection from
         * inside \p func, or the (non-reentrant) shared_mutex will deadlock. */

        template <typename Func>
        void for_each(Func &&func) const
        {
            std::shared_lock lock(f_mutex);
            for (const auto &pair : f_data)
                func(pair.first, pair.second);
        }

        /*--------------------------------------------------------------------------*/

    private:
        mutable std::shared_mutex f_mutex;
        std::unordered_map<std::string, T> f_data;

    }; // end( class( Collection ) )

    /*--------------------------------------------------------------------------*/
    /*--------------------------- CLASS GlobalInformation -------------------------*/
    /*--------------------------------------------------------------------------*/
    /*--------------------------- GENERAL NOTES --------------------------------*/
    /*--------------------------------------------------------------------------*/
    /// search-global "universe" of typed, named Collection< T >
    /** GlobalInformation is what the enumerative Solver hands each
     * ChangeSolver through ChangeSolver::set_global_information() [see
     * ChangeSolver.h]: a registry, keyed by name, of Collection< T > objects
     * of arbitrary (heterogeneous) type T. Whoever first needs a given piece
     * of global information declares it with add_to_Universe< T >( name ),
     * and from then on any holder of the GlobalInformation - typically every
     * ChangeSolver active on the tree - can get_from_Universe< T >( name )
     * the very same Collection< T > and read or write it concurrently with
     * the others, relying on the thread-safety Collection< T > itself
     * provides.
     *
     * GlobalInformation does not own the Solver, nor is it owned by any one
     * of them: a pointer to it is only *lent*, and multiple ChangeSolver
     * instances - possibly running in different threads on different nodes
     * of the enumeration tree - are meant to share the same instance for as
     * long as the search lasts. Individual Collection< T > are returned as
     * std::shared_ptr, so a Collection remains valid for whoever is still
     * using it even if remove_from_Universe() drops it from the registry
     * concurrently.
     *
     * The map from name to Collection is itself guarded by a
     * std::shared_mutex, distinct from (and unrelated to) the mutex inside
     * each individual Collection< T >: locking f_mutex only protects the
     * *registry* (adding, looking up, or removing a named Collection), never
     * the *contents* of a Collection, which is each Collection's own
     * responsibility. */

    class GlobalInformation
    {

    public:
        /*--------------------------------------------------------------------------*/
        /*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
        /*--------------------------------------------------------------------------*/

        GlobalInformation() = default;

        virtual ~GlobalInformation() = default;

        /*--------------------------------------------------------------------------*/
        /*------------- METHODS FOR ADDING / REMOVING COLLECTIONS ------------------*/
        /*--------------------------------------------------------------------------*/

        /// create a new, empty Collection< T > under the given \p name
        /** Throws std::runtime_error if a Collection already exists under
         * \p name, regardless of its type: names are a single flat namespace
         * shared by all T. */

        template <typename T>
        void add_to_Universe(const std::string &name)
        {
            std::unique_lock lock(f_mutex);

            auto res = f_Universe.emplace(
                name,
                std::make_shared<Collection<T>>());

            if (!res.second)
                throw std::runtime_error(
                    "Collection \"" + name + "\" already exists.");
        }

        /// retrieve the Collection< T > registered under \p name
        /** Returns nullptr if no Collection exists under \p name, or if one
         * exists but was registered with a T incompatible with the one
         * requested here (the dynamic_pointer_cast fails). The returned
         * shared_ptr keeps the Collection alive for as long as the caller
         * holds it, even past a concurrent remove_from_Universe(). */

        template <typename T>
        std::shared_ptr<Collection<T>> get_from_Universe(const std::string &name)
        {
            std::shared_lock lock(f_mutex);

            auto it = f_Universe.find(name);
            if (it == f_Universe.end())
                return nullptr;

            return std::dynamic_pointer_cast<Collection<T>>(it->second);
        }
        /// const version of get_from_Universe()

        template <typename T>
        std::shared_ptr<const Collection<T>> get_from_Universe(
            const std::string &name) const
        {
            std::shared_lock lock(f_mutex);

            auto it = f_Universe.find(name);
            if (it == f_Universe.end())
                return nullptr;

            return std::dynamic_pointer_cast<const Collection<T>>(it->second);
        }

        /// tells whether some Collection (of any type) is registered under \p name
        bool exists(const std::string &name) const
        {
            std::shared_lock lock(f_mutex);
            return f_Universe.find(name) != f_Universe.end();
        }
        /// remove the Collection registered under \p name, if any
        /** Does not invalidate shared_ptr already held by other users of that
         * Collection [see class comment]; it only makes \p name available
         * again for a future add_to_Universe(). */

        void remove_from_Universe(const std::string &name)
        {
            std::unique_lock lock(f_mutex);
            f_Universe.erase(name);
        }
        /*--------------------------------------------------------------------------*/

    private:
        mutable std::shared_mutex f_mutex;

        std::unordered_map<std::string, std::shared_ptr<CollectionBase>>
            f_Universe;

    }; // end( class( GlobalInformation ) )

} // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif /* GlobalInformation.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File GlobalInformation.h ----------------------*/
/*--------------------------------------------------------------------------*/