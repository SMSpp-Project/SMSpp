/*--------------------------------------------------------------------------*/
/*---------------------------- File Observer.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the Observer class, an abstract base class implementing
 * the concept of an observer which can be notified about Modifications.
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

#ifndef __Observer
 #define __Observer  /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Modification.h"

#include <set>

#include <limits>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{
/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Observer_CLASSES Classes in Observer.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS Observer -------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// an observer that is interested in Modification
/** The Observer class implements an observer as in the design pattern of the
 * same name. This Observer is only interested in Modification and has the
 * (pure virtual, which is why this is an abstract class) add_Modification()
 * method whereby the subjects can report the Modification.
 *
 * In SMS++, Modification has to reach all the interested Solver. If the
 * Modification is directly issued by, say, a Variable, Constraint or
 * Objective, this can be done directly via the Block (which is an Observer),
 * to which both the Modification-spewing object and the interested Solver
 * are attached (directly, or to any ancestor). However, the reason for a
 * separate Observer class is that some objects (e.g., Function) can issue
 * Modification, but they may not be directly attached to a Block. Rather,
 * they can may "live inside another object"; a Function, for instance, may
 * be used by some Constraint or Objective to "implement themselves". In
 * this case, the object that "hosts" a Modification-spewing one has to be
 * an Observer in order for the Modification to be received, and ultimately
 * routed to the appropriate Block. A possible example of this behaviour is
 * that of "Function of Function" objects (SumFunction, CompositeFunction,
 * ...), which may be easily implemented provided that that they themselves
 * are made Observer.
 *
 * The Observer class also supports the following notions:
 *
 * - Modification can be sent to "different channels"; besides the "default
 *   channel", shipping them immediately to the Solver, new channels can
 *   be dynamically opened and closed that allow to bunch set of "logically
 *   related Modification" together in order to make it easier for the Solver
 *   to react to them.
 *
 * - The Observer has a way for telling the observed object whether "nobody
 *   is listening", and therefore there is no need to issue the Modification
 *   at all.
 *
 * - The Observer provides a few convenience methods for formatting in an
 *   uniform way the parameter of calls to methods that produce Modification
 *   which specifies if, how and where the Modification has to be issued. */

class Observer {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the "name" of a Modification "channel"
 using ChnlName = unsigned short int;

 using c_ChnlName = const ChnlName;  ///< a const ChnlName

/*--------------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

 /// constructor of Observer, it has nothing to do

 Observer( void ) = default;

/*--------------------------------------------------------------------------*/
 ///< copy constructor, it has nothing to do either

 Observer( const Observer & ) = default;

/*--------------------------------------------------------------------------*/
 ///< destructor: it is virtual, and empty

 virtual ~Observer() = default;

/** @} ---------------------------------------------------------------------*/
/*------------- METHODS DESCRIBING THE BEHAVIOR OF AN Observer -------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of an Observer
 *  @{ */

 /// returns the Block to which this Observer belongs, or is
 /** Any Observer either is a Block, or belongs to one. This method has to
  * return a pointer to such Block. */

 [[nodiscard]] virtual Block * get_Block( void ) const = 0;

/*--------------------------------------------------------------------------*/
 /// returns true if there is "anybody listening to Modification"
 /** Returns true if there is "anybody listening to Modification". In case of
  * a Block, this typically means that there is some Solver "listening to
  * this Block", which means either registered to this Bock or registered to
  * any ancestor (father, father of father, ...) of this Block. Other
  * Observer may either just redirect the method to the Block they belong to,
  * or force a different response in case they are "interested" to the
  * Modification themselves because they need to "intercept them and change
  * them along the way". */

 [[nodiscard]] virtual bool anyone_there( void ) const = 0;

/*--------------------------------------------------------------------------*/
 /// notify this Observer about a Modification
 /** This method notifies this Observer about a Modification.
  *
  * mod is a smart pointer to an object of class (derived from)  Modification
  * (std::shared_ptr< Modification >, sp_Mod in Modification.h), that has been
  * created by some object (Block, Constraint, Variable, Function, ...) and it
  * is passed to this Observer. Ultimately the Modification will have to reach
  * a Block for being dispatched to all Solver attached to it, as well as to
  * all Solver attached to its ancestors (the father, the father of the
  * father, ...). Some Modification-spewing objects (typically, Function) may
  * not be directly observed by a Block but by something else (typically,
  * FRowConstraint and FRealObjective), which however ultimately belongs to
  * a Block; if the Observer is not a Block, it is its responsibility that
  * the Modification object is ultimately dispatched to the Block. Indeed,
  * :Block may also use "abstract" Modification to update their "physical
  * representation" to keep it in synch with their "abstract representation"
  * [see Modification::concerns_Block()]. Furthermore, note that a non-Block
  * Observer may need to "translate" the Modification object into one or more
  * different Modification objects, according to the effect that the original
  * Modification (say, to a Function) has on the Observer (say, a
  * FRowConstraint).
  *
  * Since what is passed to this method is a shared pointer, the Modification
  * object is automatically destroyed after that all Solver have deleted it
  * (or, if a non-Block Observer translates it into a new set of
  * Modification, immediately after that). Note that the preferred way of
  * calling this method is
  *
  *     < Observer >.add_Modification(
  *                   std::make_shared< DerivedModification >( < params > ) );
  *
  * where < params > are the parameters of the constructor of
  * DerivedModification that should be called. Using std::make_shared<> is
  * slightly faster then doing "new DerivedModification( < params > )" and
  * then converting the returned ordinary pointer to a smart one, and calling
  * it within the call to add_Modification() may avoid one copy of the
  * shared pointer to be made only to be immediately destroyed. Not that this
  * matter much, because copies of the smart pointer (say, in the Solver
  * attached to the Block to which < Observer > belongs, with Block ==
  * < Observer > a definite possibility, and its ancestors, if any) will have
  * been made by then. Hence, even if a local copy were done, destroying
  * it would only decrease the counter by 1, effectively deleting it if and
  * only if there are no interested Solver, which is the intended semantic.
  *
  * Also, note that the Solver (or, possibly, the Observer itself if it has
  * to "translate" it) will have to downcast the Modification object to an
  * actual DerivedModification in order to gather the information about what
  * has been modified. These being smart pointers, this cannot be achieved
  * with the ordinary dynamic_cast<>; rather,
  *
  *     auto dmod = std::dynamic_pointer_cast< DerivedModification >( mod );
  *
  * has to be used. Note that one can check that the dynamic cast worked as
  * for an ordinary pointer, i.e., with
  *
  *     dmod != nullptr       or        ! dmod
  *
  * (thanks to shared_ptr operator bool). The alternative is to first extract
  * the original Modification * from mod using get() and then use the "normal"
  * dynamic_cast, as in
  *
  *     auto dmod = dynamic_cast< const DerivedModification * >( mod.get() );
  *
  * (note that the "const" above is not strictly necessary, but in general an
  * Observer is not supposed to change a Modification, and in fact basically
  * all methods of Modification are const anyway).
  *
  * It may be helpful to Solver that sets of "logically related Modification"
  * be dispatched together. For this reason, Observer supports the notion that
  * set of Modification can be bunched together into a GroupModification
  * object. This is done by defining different "channels" where Modification
  * can be sent, which are opened with open_channel(). The parameter chnl
  * specifies to which of the currently "open" GroupModification objects mod
  * has to be appended, with the following format:
  *
  * - If chnl is the name of a currently open channel, which in particular
  *   implies that it is != 0, then mod is added to the corresponding
  *   GroupModification object (possibly "nested inside it", see
  *   open_channel() for details).
  *
  * - If chnl is not the name of a currently open channel, (say, it is 0 as
  *   in the default), then mod is "sent to the default channel". Unless this
  *   is changed with set_default_channel(), this means that mod is not added
  *   to any GroupModification, and just immediately sent along to the
  *   interested Solver / Block.
  *
  * It is important to note that
  *
  *     CHANNEL NAMES ARE UNIQUE WITHIN ALL SMS++, AND THEREFORE EACH CHANNEL
  *     IS DEFINED IN A SPECIFIC Block. YET, SUB-Block OF THAT Block CAN STILL
  *     SEND Modification TO THAT VERY CHANNEL: THE Modification WILL TRAVEL
  *     UP IN THE Block TREE (MEANWHILE BEING SENT TO ALL ATTACHED Solver)
  *     UNENCUMBERED UP UNTIL IT REACHES THE Block WHERE THE CHANNEL IS
  *     DEFINED, AND ONLY THEN IT IS BUNCHED INTO THE GroupModification.
  *
  * As a consequence
  *
  *     IT IS AN ERROR TO SEND A Modification TO A CHANNEL THAT IS NOT
  *     DEFINED IN THE Block OR SOME ANCESTOR OF THE Block.
  *
  * This mechanism is implemented into Block::add_Modification(), and it is
  * not assumed to be re-implemented in different ways by :Block or other
  * :Observer. This is important in that it allows to enforce a useful
  * property:
  *
  *     THE EXACT :Block IN WHICH A :Modification HAPPEN THAT IS BEING
  *     SENT TO SOME NON-0 CHANNEL WILL NEVER "SEE" THE GroupModification,
  *     BUT RATHER THE INDIVIDUAL :Modification THAT WILL EVENTUALLY BE
  *     PACKED INTO IT. THIS IS TRUE EVEN IF THE CHANNEL HAS BEEN OPENED
  *     IN THIS VERY :Block, BUT IT ONLY APPLIES TO THE :Block ITSELF AND
  *     NOT TO THE Solver REGISTERED TO IT.
  *
  * This property is *only* true for the very specific :Block, and it does
  * not hold true for its ancestor Block (which also receive the
  * Modification) unless the channel has been defined somewhere above the
  * Block. In other words, regardless to where the channel is defined, the
  * very Block in which the Modification originates will "see" it
  * immediately when it is issued. This is important for the handling of
  * "abstract" Modification, since it is very useful that that :Block (and
  * that :Block only) is guaranteed to "see the Modification immediately".
  * This ensures that the data structures in the :Block are exactly the same
  * as the ones at the moment in which the Modification has been created,
  * which would *not* necessarily be true if the Modification was packed
  * into a GroupModification. Indeed, this could in principle happen "for a
  * long time" before the GroupModification is closed and dispatched, and
  * in the meantime "complex changes" may occur to the :Block that may make
  * it more complex to handle the corresponding changes to the "physical
  * representation". See the comments to Block::add_Modification() for more
  * details.
  *
  * However, note that, conversely, if the channel has been defined "way
  * above" the :Block in which the Modification is issued, then all the Block
  * (and attached Solver) between the originating Block and the one having
  * defined the channel will "see" it directly, rather than the
  * GroupModification in which it is ultimately packed. This means that if
  * it is important that the Modification is bunched together with other
  * ones before the other Block / Solver get to see it, the channel must be
  * defined "as close as possible", typically in the very Block in which it
  * is issued (which, to re-iterate, will however see it "naked" even if the
  * channel is defined there). */

 virtual void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) = 0;

/*--------------------------------------------------------------------------*/
 /// maximum channel name

 static constexpr ChnlName max_channel_name =
  std::numeric_limits< ChnlName >::max() / 4;

/*--------------------------------------------------------------------------*/
 /// "open" a channel
 /** This method allows to "bunch together" a set of "logically related
  * Modification". For maximal flexibility, the Modification can be grouped
  * in an arbitrarily complex tree-nested way, as dictated by the \p chnl
  * parameter.
  *
  * If open_channel() is invoked with chnl == 0 (the default), then a "new
  * channel is opened". This means that a GroupModification object is
  * (created, unless it is provided, and it is) assigned a new unique name,
  * which is returned. Then, being chnl the returned value, a call to
  * add_Modification( mod , chnl ) adds mod at the end of the STL container
  * of the GroupModification, rather than dispatching it to the Solver and
  * the ancestor Block. The latter operation is only done when
  * close_channel( chnl ) is called. Note that this means that
  * add_Modification() does see mod "naked", but the Modification is not
  * further forwarded to the Solver and the ancestor Block. This only holds
  * true if the channel has been defined in the very Block; otherwise, the
  * Modification will travel up (and be dispatched to the Solver) "naked"
  * until it reaches the Block where the channel has been defined.
  *
  * If, instead, open_channel() is invoked with chnl != 0, a new
  * GroupModification is (created, unless it is provided, and it is) nested
  * into the existing GroupModification associated with the given channel.
  * That is, the GroupModification object is appended at the end of the STL
  * container in the "outer" GroupModification associated to the channel.
  * Then, a call to add_Modification( mod , chnl ) adds mod to the "inner"
  * GroupModification. This lasts until the method is called again, on which
  * case an inner-inner-GroupModification is started (...), or
  * close_channel( chnl ) is called, in which case the last nested
  * GroupModification is closed and "control is returned" to the one
  * immediately above. If this is a "root" GroupModification (created with
  * chnl == 0), the whole GroupModification is closed and finally dispatched.
  * In this case, the return value of the method is the same as the input
  * value of \p chnl, as the channel name does not change.
  *
  * Calling the method with chnl not being either 0 or the name of an open
  * channel is an error and should throw exception.
  *
  * If the parameter gmpmod is != nullptr, then the pointed object is taken
  * as the GroupModification that is "opened". This means that the object
  * becomes "property" of the Observer; the raw pointer is later packaged in
  * a smart pointer (when the channel is closed), so it is crucial that no
  * copies of the raw pointer are retained. This is done in order to allow
  * the caller to provide objects of *derived classes* from
  * GroupModification; these may contain other data that is useful to the
  * Solver / Block to process the GroupModification, and even just being of
  * a specific :GroupModification class may help. If gmpmod is == nullptr,
  * an object of the base GroupModification class is automatically
  * constructed by the method. */

 virtual ChnlName open_channel( ChnlName chnl = 0 ,
				GroupModification * gmpmod = nullptr ) = 0;

/*--------------------------------------------------------------------------*/
 /// push back to the previous level of a channel
 /** This method allows to "finalize" the set of "logically related
  * Modification" contained in the GroupModification currently associated
  * with the channel chnl. What this means, however, depends on whether that
  * GroupModification is in "root mode" (currently adding to the outermost
  * GroupModification of the channel), as opposed to being inside another
  * GroupModification (...).
  *
  * In the former case the GroupModification is finally shipped to the
  * interested Solver and Block. This also closes the channel, i.e., chnl is
  * no longer the name of an open channel until it is produced again by a
  * call to open_channel().
  *
  * In the latter case, the effect depends on \p force. If \p force == false
  * (the default), then what is "finalized" is only the "inner"
  * GroupModification, in the sense that addition of Modification is resumed
  * for the "father" GroupModification of that object, which is still
  * associated to the same channel that is not closed, and the
  * GroupModification still remains inside the Block waiting for further
  * Modification to be added (possibly GroupModification if open_channel()
  * is called again with that channel name). This means that a channel is
  * closed only by calling close_channel( chnl , false ) as many times as
  * open_channel( chnl ) has been called (counting only the still active
  * levels, i.e., those that have not been closed already).
  *
  * If \p force == true, instead, then the effect is the same as if the
  * GroupModification were in "root mode": the "outermost" GroupModification
  * associated to the channel (irrespectively to how many levels of other
  * GroupModification are in between them) is finally shipped to the
  * interested Solver and Block, and this closes the channel. This allows to
  * immediately finalise a GroupModification, but it creates an imbalance
  * between the number of calls to open_channel() and close_channel() for
  * that channel. Since closing an un-opened channel is an error, calling
  * close_channel( chnl , true ) can only be done if one is 100% sure that
  * no other calls to close_channel() are going to be issued. As calls to
  * open_channel() and close_channel() are very likely to be always issued
  * in pairs, \p force == true is a "dangerous" choice that must be used
  * with care.
  *
  * It is always an error to try to close the default channel (0). */

 virtual void close_channel( ChnlName chnl , bool force = false ) = 0;

/*--------------------------------------------------------------------------*/
 /// set the "default" channel
 /** This method allows to "silently redirect" any Modification that is added
  * with add_Modification( mod , 0 ) to the specified open channel. This
  * mechanism is provided in case the Modification is generated by a
  * "complex reaction", i.e., in a method called by a method called by a
  * method ... called by the method that performs the changes whose
  * corresponding Modification should be bunched together (and where, most
  * likely, the GroupModification is opened). The point is that to send a
  * particular Modification to a particular channel it is necessary
  * to specify the channel name in add_Modification(), but it may not always
  * be possible to control what name is used in a method called by a method
  * called by a method ... Thus, this mechanism allows to (temporarily)
  * "silently hijack" the "standard channel" 0. Of course, some care has to
  * be exercised while using this.
  * 
  * If chnl is the name of a currently "open" GroupModification object, which
  * in particular implies that it is != 0, then all subsequent Modification
  * sent to "channel 0" are redirected to that GroupModification. This ends
  * if set_default_channel( 0 ) is called, or if the channel to which 0 has
  * been redirected is closed. Calling the method with chnl not the name of
  * an open channel or zero is an error and should throw exception. */

 virtual void set_default_channel( ChnlName chnl = 0 ) = 0;

/*--------------------------------------------------------------------------*/
 /// convenience method for "open a channel if the need arises"
 /** Small convenience method that takes a "composite" parameter, as
  * produced e.g. by make_par(), encoding both a ModParam and a channel, and
  * an expected number of "related" Modification to be issued with that
  * parameter, and returns another "composite" parameter corresponding to
  * having opened a channel to pack them together "only if it makes sense
  * to". This means that if either the ModParam indicates that no
  * Modification will be issued anyway, or the Modification are less than
  * two, then the same "composite" parameter value as the input one is
  * returned and no channel is opened. Otherwise, a channel is opened
  * (which may mean nested, if the input "composite" parameter already
  * specifies a non-0 channel name) and the "composite" parameter
  * corresponding to the same ModParam value and the new cannel is
  * returned. */

 ModParam open_if_needed( ModParam issueMod , unsigned int num ) {
  if( ( num <= 1 ) || ( ! issue_mod( issueMod ) ) )
   return( issueMod );

  return( make_par( par2mod( issueMod ) ,
		    open_channel( par2chnl( issueMod ) ) ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 /// convenience method for "close a channel if the need arises"
 /** Small convenience method that takes a "composite" parameter as
  * produced by open_if_needed() and closes the new channel if it had been
  * opened in the first place (otherwise it does nothing). */

 void close_if_needed( ModParam issueMod , unsigned int num ) {
  if( ( num <= 1 ) || ( ! issue_mod( issueMod ) ) )
   return;

  close_channel( par2chnl( issueMod ) );
  }

/*--------------------------------------------------------------------------*/
 /// method to "pack" all info about issuing Modification in one parameter
 /** When a method is called that may produce a Modification, it is necessary
  * to specify some information about if, how and where the Modification has
  * to be issued. In particular, one has to decide if the Modification has to
  * be issued at all, and its concerns_Block() value, according to the format
  * set by the amododification_type enum [see Modification.h]:
  *
  * - eDryRun   the change that the method is supposed to perform, which would
  *             result in a Modification would be issued, must *not* be done;
  *             as a consequence, no Modification should be issued. Allowing
  *             to call a method and actually not doing the change that the
  *             method should do is useful in particular for methods that
  *             change both the "abstract" and the "physical" representation
  *             (say, of a Block), but for which it may be useful to switch
  *             off the changes in one of the two, say because one is sure
  *             that the changes have already been done (for instance, because
  *             one is reacting to one AModification implying that the
  *             "abstract" one has changed already).
  *
  * - eNoMod    the Modification is *not* issued: this should not be done
  *             unless for some reason it is guaranteed that neither the
  *             Block nor any Solver will ever need this information;
  *
  * - eNoBlck   the Modification is issued, but *only* if there is "anyone
  *             listening"; furthermore, the concerns_Block() value is set
  *             to false, meaning that the Block receiving this Modification
  *             can safely ignore it on knowledge that the corresponding
  *             change in the Block has already happened;
  *
  * - eModBlck  the Modification is issued whether or not there is "anyone
  *             listening", and the concerns_Block() value is set to true;
  *             this is the default, "most conservative" setting.
  *
  * Furthermore, it is necessary to specify to which channel the Modification
  * is sent. This information can be "packed" into one single parameter, which
  * is what this method does: iM is the parameter containing one of the above
  * three values, and chnl the "name" of the channel. It is guaranteed that
  * make_par( iM , 0 ) == iM, so this method is only needed when sending to
  * a non-default channel. */

 static ModParam make_par( ModParam iM , ChnlName chnl ) {
  return( iM + 4 * chnl );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 /// method extracting the channel information
 /** Given a "composite" parameter as produced by make_par(), this method
  * returns the channel information alone. */

 static ChnlName par2chnl( ModParam issueMod ) { return( issueMod / 4 ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 /// method extracting the ModParam information
 /** Given a "composite" parameter as produced by make_par(), this method
  * returns the ModParam information alone. */

 static ModParam par2mod( ModParam issueMod ) { return( issueMod & 3 ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 /// method extracting the concerns_Block information
 /** Given a "composite" parameter as produced by make_par(), this method
  * returns the boolean to become the concerns_Block. */

 static bool par2concern( ModParam issueMod ) {
  return( par2mod( issueMod ) == eModBlck );
  }

/*--------------------------------------------------------------------------*/
 /// method for checking if a(n abstract) Modification has to be issued
 /** Given a "composite" parameter as produced by make_par(), this method
  * returns true if the parameter implies that a Modification must be issued.
  * It is intended that the Modification is an "abstract" one, because the
  * value eModBlck only makes sense for them. This also depends on the value
  * reported by anyone_there(), which is why the method is not static. */

 [[nodiscard]] bool issue_mod( ModParam issueMod ) const {
  return( ( par2mod( issueMod ) == eModBlck ) ||
	  ( ( par2mod( issueMod ) == eNoBlck ) && anyone_there() ) );
  }

/*--------------------------------------------------------------------------*/
 /// method for checking if a (physical) Modification has to be issued
 /** Given a "composite" parameter as produced by make_par(), this method
  * returns true if the parameter implies that a Modification must be issued.
  * This is intended only for "physical" Modification, in that it ignores the
  * value eModBlck and treats it as if it were eNoBlck. This also depends on
  * the value reported by anyone_there(), which is why the method is not
  * static. */

 [[nodiscard]] bool issue_pmod( ModParam issueMod ) const {
  return( par2mod( issueMod ) && anyone_there() );
  }

/*--------------------------------------------------------------------------*/
 /// static method for ensuring a parameter is not eModBlck
 /** Given a "composite" parameter as produced by make_par(), this static
  * method "downgrades" the "mod" part of the parameter to eNoBlck if it is
  * eModBlck keeping the same channel, and does nothing otherwise.
  * This method should be called when the eModBlck value should never be
  * allowed, such as when one is certain that the "abstract" representation
  * has been modified already. This happens for instance in a method of a
  * :Block that modifies the "physical" and the "abstract" representation at
  * the same time, and therefore where there is no possible reason why the
  * "abstract" Modification should be checked by the Block. */

 static void not_ModBlock( ModParam & issueMod ) {
  if( par2mod( issueMod ) == eModBlck )
   issueMod = make_par( eNoBlck , par2chnl( issueMod ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 /// static method for ensuring a parameter is not eModBlck
 /** Given a "composite" parameter as produced by make_par(), this static
  * method "downgrades" the "mod" part of the parameter to eNoBlck if it is
  * eModBlck keeping the same channel, and does nothing otherwise.
  * This is the same as not_ModBlock() and it has the same use, but it
  * returns the modified (or nor) value rather than changing the input
  * variable. */

 static ModParam un_ModBlock( ModParam issueMod ) {
  return( par2mod( issueMod ) == eModBlck ?
	  make_par( eNoBlck , par2chnl( issueMod ) ) : issueMod );
  }

/*--------------------------------------------------------------------------*/
 /// method for checking if the actual change has to be made
 /** Given a "composite" parameter as produced by make_par(), this method
  * returns true if the parameter implies that the actual change has to be
  * made (if not, clearly no Modification must be issued). Allowing to call
  * a method and actually not doing the change that the method should do is
  * useful in particular for methods that change both the "abstract" and the
  * "physical" representation (say, of a Block), but for which it may be
  * useful to switch off the changes in one of the two, say because one is
  * sure that the changes have already been done (for instance, because one
  * is reacting to one AModification implying that the "abstract" one has
  * changed already). */

 static bool not_dry_run( ModParam issueMod ) {
  return( par2mod( issueMod ) );
  }

/** @} ---------------------------------------------------------------------*/
/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for channel names
 *
 * Protected methods to acquire and release new channel names.
 * @{ */

 static ChnlName new_channel_name( void ) {
  ChnlName retval;

  // try to acquire lock, spin on failure
  while( f_ch_lock.test_and_set( std::memory_order_acquire ) )
   ;

  if( v_free_chnl.empty() ) {
   if( f_next_chnl >= max_channel_name ) {
    f_ch_lock.clear( std::memory_order_release );
    throw( std::logic_error( "Observer: max number of channels exhausted" ) );
    }
   retval = f_next_chnl++;
   }
  else {
   auto rit = v_free_chnl.rend();
   retval = *rit;
   v_free_chnl.erase( rit.base() );
   }
  
  f_ch_lock.clear( std::memory_order_release );  // release lock

  return( retval );
  }

/*--------------------------------------------------------------------------*/
 /// release an existing channel name to be available again

 static void release_channel_name( ChnlName chnl ) {
  // try to acquire lock, spin on failure
  while( f_ch_lock.test_and_set( std::memory_order_acquire ) )
   ;

  if( chnl >= f_next_chnl ) {
   f_ch_lock.clear( std::memory_order_release );
   throw( std::invalid_argument( "Observer: wrong channel name" ) );
   }
  
  if( chnl == f_next_chnl - 1 ) {
   --f_next_chnl;
   #ifndef NDEBUG
    if( ! f_next_chnl ) {
     f_ch_lock.clear( std::memory_order_release );
     throw( std::logic_error( "Observer: error in channels management" ) );
     }
   #endif
   if( v_free_chnl.empty() ) {
    f_ch_lock.clear( std::memory_order_release );
    return;
    }
   for( auto rit = v_free_chnl.rbegin() ;
	( rit != v_free_chnl.rend() ) && ( *rit == f_next_chnl - 1 ) ; ) {
    rit = decltype( rit )( v_free_chnl.erase( std::next( rit ).base() ) );
    --f_next_chnl;
    #ifndef NDEBUG
     if( ! f_next_chnl ) {
      f_ch_lock.clear( std::memory_order_release );
      throw( std::logic_error( "Observer: error in channels management" ) );
      }
    #endif
    }
   }
  else
   v_free_chnl.insert( chnl );
  
  f_ch_lock.clear( std::memory_order_release );  // release lock
  }

/** @} ---------------------------------------------------------------------*/
/*---------------------------- PROTECTED FIELDS  ---------------------------*/
/*--------------------------------------------------------------------------*/

 static std::atomic_flag f_ch_lock;  ///< active lock for channel names

 static ChnlName f_next_chnl;        ///< next free chanel

 static std::set< ChnlName > v_free_chnl;  ///< set of freed channels

/** @} ---------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

 };  // end( class( Observer ) )

/*--------------------------------------------------------------------------*/

/** @}  end( group( Observer_CLASSES ) ) -----------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* Observer.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Observer.h ---------------------------*/
/*--------------------------------------------------------------------------*/
