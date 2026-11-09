#ifndef progtest_solver_h_132487812734523145234
#define progtest_solver_h_132487812734523145234

#include <cstdint>
#include <functional>
#include "common.h"


//=============================================================================================================================================================
/**
 * fragment-to-message conversion class interface. The class is abstract, the actual implementation is not exposed.
 * The class processes a batch of problems. The batch has a fixed size, the size is set in initialization. You
 * may use the entire capacity (recommended), but may start the computation earlier (not recommended, you may run
 * of available conversions). The conversion may require more than one thread to actually start, the given number
 * of threads is expected to call the entry point (method solve). The coversion progress is reported by user-supplied
 * callbacks, see foundFn and finishedFn parameters to addProblem.
 */
class CMsgSerializer
{
  public:
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * A standar destructor.
     */
    virtual                            ~CMsgSerializer                         () noexcept = default;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Check whether the instance may accept another problem. If the capacity is exhausted (no further
     * problems may be added), the method returns false.
     * @return true=at least one problem may be added, false=capacity is fully used
     */
    virtual bool                       hasFreeCapacity                         () const = 0;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Add a new problem to the list of problems to solve. The call to this method only registers the problem, the computation does
     * not start immediately. A problem may be rejected if the instance does not have any free capacity.
     * @param [in] fragments           a list of fragments to assemble into a message. If the fragments do not
     *                                 have the same msgID, they do not have the same fragCnt, or there is not enough/too many fragments,
     *                                 the conversion is likely to fail.
     * @param [in] foundFn             a callback to call when the fragments are successfully converted into a message. The callback may be
     *                                 called more than once if there are two or more ways to assemble the fragments into a message.
     *                                 Conversely, the callback is not called at all if the fragments cannot be assembled into a valid message.
     *                                 The parameters to the callback are: msgId, the bitfield that forms the message, and the number of bits of the bitfield.
     * @param [in] finishedFn          a callback that is called when the processing of the fragments is finished. It is guaranteed to be
     *                                 called exactly once. A parameter to the callback is the number of messages successfully assembled
     *                                 from the fragments.
     * @return true=the problem was successfully registered, false=problem is rejected (no capacity)
     * @note the method is not thread-safe. External locking is required if the method is called simultaneously from two or more threads.
     * @note the callbacks are called later, when the computation starts (i.e., from within solve method). Since the computation may employ more
     *        than one thread, the callbacks may be called from different threads.
     */
    virtual bool                       addProblem                              ( std::vector<uint64_t>                 fragments,
                                                                                 std::function<void(uint32_t, const uint8_t[], size_t)> foundFn,
                                                                                 std::function<void(uint32_t)>         finishedFn ) = 0;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Get the number of threads required to start the computation. The computation starts when totalThreads() calls method solve().
     * @return the number of threads, at least 1.
     */
    virtual uint32_t                   totalThreads                            () const = 0;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Start the computation. The method must be called by exactly totalThreads() threads. The threads are blocked, until all required threads
     * called solve (). Once unblocked, the threads solve the registered problems and use the registered callbacks to pass the results.
     * @return true=ok, false=error(already solved)
     */
    virtual bool                       solve                                   () = 0;
};
using AMsgSerializer = std::shared_ptr<CMsgSerializer>;
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * Factory function - create a new instance of CMsgSolver. This function is available in this library as well as in the
 * testing environment.
 * @return CMsgSolver instance with capacity/#threads set somehow
 */
AMsgSerializer                         createMsgSerializer                     ();
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * Setup function - control the capacity and thread boundaries in the generated CMsgSerializer instances.
 * This function is NOT AVAILABLE in the testing environment, it exists just in this library.
 *
 * @param[in]   capacityTotal          generate CMsgSerializer instances, this is the limit on total capacity
 * @param[in]   capacityMin            lower bound on capacity for each generated instance
 * @param[in]   capacityMax            upper bound on capacity for each generated instance
 * @param[in]   thrMin                 lower bound on # of required threads
 * @param[in]   thrMax                 upper bound on # of required threads
 * @return true=new limits set, false=invalid limits (lower bound > upper bound)
 */
bool                                   msgSerializerLimits                     ( size_t                                capacityTotal,
                                                                                 size_t                                capacityMin,
                                                                                 size_t                                capacityMax,
                                                                                 uint32_t                              thrMin,
                                                                                 uint32_t                              thrMax );
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * Factory function - create a new instance of CMsgSolver. This function is NOT AVAILABLE in the testing environment, it exists
 * just in this library. The function creates a new CMsgSolver instance with the given capacity and number of threads.
 * The function is intended for testing and debugging only.
 *
 * @param[in] capacity                 instance capacity (# of problems)
 * @param[in] threads                  number of threads required to start solve ()
 * @return CMsgSolver instance with capacity/#threads set
 */
AMsgSerializer                         createMsgSerializerDbg                  ( size_t                                capacity,
                                                                                 uint32_t                              threads );
//=============================================================================================================================================================
/**
 * Given a bitfield representing boolean values, the function adds boolean operators &, | and ^ to
 * form a valid Boolean expression in a prefix notation. The result is the number of prefix Boolean
 * expressions that evaluate to true.
 *
 * @param [in] data   a pointer to the input bitfield (the processing starts with the least
 *                    significant bit of the byte data points to)
 * @param [in] bits   the number of bits in the input bitfield
 * @return the number of boolean expressions found
 * @note the function is rather slow - the time complexity is ~n^3.
 * @note the function is usable in mandatory and optional tests. The implementation in the Progtest environment
 *       does not work correctly in the bonus test.
 */
CBigInt                                countExpressions                        ( const uint8_t                         data[],
                                                                                 size_t                                bits );
//=============================================================================================================================================================
#endif /* progtest_solver_h_132487812734523145234 */
