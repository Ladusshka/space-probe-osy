# The Sentinel: Parallel Message Solver

Implement a set of classes that can quickly receive, decode, analyze, and solve problems sent by an alien probe. The solution must process messages from multiple sources concurrently, use the supplied interfaces for message reconstruction, and return exactly one result for every complete problem.

## The Scenario

In Arthur C. Clarke's short story *The Sentinel*, humanity discovers an alien probe protected by an unknown barrier. This assignment imagines a similar probe that grants access only to a civilization capable of receiving, analyzing, and returning solutions to its challenges. Your task is to implement the software that handles those challenges efficiently, using multiple threads to parallelize the work.

Some parts of the problem can be implemented directly. Others rely on the supplied library and interfaces, whose behavior is described below and in the accompanying headers.

## Message Format

The probe sends each problem as one message with a unique 22-bit ID. A message is split into 64-bit fragments. Fragments can arrive out of order, interleaved with fragments from other messages, and through multiple sources.

| Bits | Field | Description |
| --- | --- | --- |
| 63–42 | `msgID` | 22-bit message identifier |
| 41–37 | `fragCnt` | Number of fragments minus one |
| 36–0 | `payload` | 37-bit fragment payload |

For example, a message split into 10 fragments has `fragCnt = 9`. The payload's internal structure is unknown and does not need to be interpreted by your implementation.

Before reconstructing a message, group its fragments by `msgID` and verify that they are consistent: all fragments must have the same `fragCnt`, and the received count must match the expected count. The supplied `CMsgSerializer` performs the actual reconstruction. It may produce no valid bitfield, one bitfield, or several possible bitfields for a single message.

## The Boolean Problem

Each reconstructed bitfield describes Boolean values, processed from the least significant bit (LSB) to the most significant bit (MSB): `1` means `true`, and `0` means `false`. Insert the operators AND, OR, and XOR to form a prefix Boolean expression that evaluates to `true`.

The answer for a bitfield is the number of distinct valid expressions. If a message can be reconstructed into multiple bitfields, solve each one and use the largest result.

For example, the 3-bit value `0x06` is `0b110`, so its values are `false, true, true`. There are 18 possible prefix expressions; the 10 marked with `*` evaluate to `true`:

```text
AND false AND true true       AND false OR true true        AND false XOR true true
* OR  false AND true true     * OR  false OR true true       OR  false XOR true true
* XOR false AND true true     * XOR false OR true true       XOR false XOR true true

  AND AND false true true     * AND OR false true true       * AND XOR false true true
* OR  AND false true true     * OR  OR false true true       * OR  XOR false true true
* XOR AND false true true       XOR OR false true true         XOR XOR false true true
```

The result for this bitfield is **10**.

## Supplied Components

### `CBigInt`

`CBigInt` represents non-negative integers up to 768 bits. It supports addition, multiplication, comparison, and conversion to a decimal string. It is sufficient for this assignment, and reimplementing it is not the goal. Its interface is declared in `common.h`.

### `countExpressions`

The supplied `countExpressions` function counts the true-valued expressions for a bitfield. It is available for mandatory and optional tests, but is intentionally non-functional in bonus tests. To pass the bonus tests, implement the counting algorithm yourself; the supplied implementation is also not ideal for that test's performance requirements.

The function takes a bitfield and its length **in bits**.

### `CMsgSerializer`

Use `CMsgSerializer` to reconstruct messages from their fragments. It is an abstract interface; create instances with `createMsgSerializer()`.

Each serializer has a fixed problem capacity. Call `addProblem` to register each message; registration does not start the computation. Use `hasFreeCapacity()` to check whether another problem can be added. Prefer filling each serializer to capacity, leaving only the final batch partially filled. Serializer instances consume a shared, limited problem budget, so avoid creating unnecessary instances.

Call `solve()` from exactly `totalThreads()` threads. The call blocks until all required threads have arrived, then those threads perform the registered work. For each problem:

- The `found` callback is called for every successfully reconstructed bitfield. It may be called multiple times when reconstruction has multiple valid results, and it may be called from different threads.
- The `finished` callback is called exactly once, after processing that problem. It is called whether or not reconstruction succeeded.

`addProblem` is not thread-safe; calls that register problems on the same serializer must be externally synchronized.

The bundled library also provides `createMsgSerializerDbg(capacity, threads)` for local testing. This debug factory is not available in the Progtest environment.

### `CReceiver` and `CTransmitter`

`CReceiver` and `CTransmitter` are abstract interfaces. The test environment creates their concrete instances and registers them with your class.

Call `CReceiver::recv` in a loop. A `true` return value provides another fragment; `false` means that receiver has no more fragments.

After processing a message, report it through a transmitter:

- `send(id, count)` reports a successfully solved problem. Every complete, successfully solved message must be sent exactly once. Any registered transmitter may be used.
- `incomplete(id)` reports a message that cannot be completed, such as one with missing, excess, inconsistent, or unreconstructable fragments.

Transmitter instances are not thread-safe or reentrant. Do not call `send` or `incomplete` concurrently on the same instance. Serialize access to each transmitter, or use distinct transmitters for concurrent sends.

## Required Class: `CSentinelHacker`

Implement `CSentinelHacker` and, at minimum, the following methods:

| Method | Contract |
| --- | --- |
| `seqSolve(fragments, res)` | Sequentially validate and reconstruct one message, count its Boolean expressions, and store the result in `res`. Return `false` if the fragments cannot form a valid message; otherwise return `true`. This method is tested first, and must work. Using `CMsgSerializer` and `countExpressions` makes the non-bonus implementation straightforward. |
| `addReceiver(recv)` | Register a receiver before `start()`; do not start its thread here. |
| `addTransmitter(tran)` | Register a transmitter before `start()`; do not start its thread here. |
| `addFragment(fragment)` | Thread-safe entry point for a single fragment. The test environment may call it concurrently from multiple threads, only between `start()` and `stop()`. |
| `addFragment(fragments)` | Also provide an overload taking `std::span<uint64_t>` that accepts multiple fragments. |
| `start(thrCnt)` | Start the receiver, worker, and transmitter threads and return without waiting for them to finish. `thrCnt` is the number of worker threads. |
| `stop()` | Finish receiving, complete or report all pending messages, drain outstanding work and transmissions, join every thread created by `start()`, then return to the caller. |

The `std::span<uint64_t>` overload is an explicit requirement of this assignment. You may add helper classes, methods, and state, but do not modify the supplied declarations or implementations of `CReceiver` or `CTransmitter`.

## Threading and Shutdown

The main thread registers receivers and transmitters, calls `start()`, performs other work, and eventually calls `stop()`. Each receiver should have one reader thread. Create the requested number of worker threads, plus transmitter-service threads as needed. The test environment may also create and stop its own threads.

After `start()` returns, your threads should continue receiving fragments, grouping them into messages, reconstructing and solving problems, and sending results. `stop()` is a graceful shutdown, not an immediate cancellation: wait for all receivers to finish, finish outstanding computations, report incomplete messages, deliver queued results, and join all threads. The environment guarantees that asynchronous calls to `addFragment` have stopped when `stop()` is called.

Do not terminate the process from `stop()` with `exit`, `pthread_exit`, or a similar function. `stop()` must return normally after your threads have stopped.

## Performance Requirements

If implementing the Boolean counting algorithm yourself:

- An algorithm with time complexity `O(n^3)`, where `n` is the bitfield length, is sufficient.
- Keep the implementation efficient; this is expected to be the dominant part of runtime.
- Mandatory and optional tests use messages with at most 12 fragments and bitfields of at most 200 bits. The test environment calibrates runtime and scales problem sizes accordingly, so avoid an implementation that is orders of magnitude slower than the reference.
- Bonus tests also evaluate sequential performance and parallel speedup. They do not use the runtime calibration described above, and will reject implementations that are substantially slower than the reference or do not benefit adequately from multiple worker threads.

## Repository Files

- `common.h` defines the shared interfaces and `CBigInt`.
- `progtest_solver.h` declares `CMsgSerializer`, its factories, and `countExpressions`.
- `sample_tester.h` and `sample_tester.cpp` provide local test support.
- `solution.cpp` contains the `CSentinelHacker` implementation.
- `Makefile` builds the local test program.
