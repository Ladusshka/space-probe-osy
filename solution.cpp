#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <climits>
#include <cfloat>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <deque>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <compare>
#include <condition_variable>
#include <span>
#include <optional>
#include "progtest_solver.h"
#include "sample_tester.h"
#endif /* __PROGTEST__ */

class CSentinelHacker
{
  public:
    static bool                        seqSolve                                ( const std::vector<uint64_t>         & fragments,
                                                                                 CBigInt                             & res );
    void                               addTransmitter                          ( ATransmitter                          x );
    void                               addReceiver                             ( AReceiver                             x );
    void                               addFragment                             ( uint64_t                              x );
    void                               start                                   ( unsigned                              thrCount );
    void                               stop                                    ();

    void RecieverWorker (AReceiver reciever){
      uint64_t fragment;
      while ( reciever -> recv(fragment)){
        this->addFragment(fragment);
      }
    }

    void SolveWorker() {
    while (true) {
        AMsgSerializer serToSolve = nullptr;

        {
            std::unique_lock<std::mutex> lock(m_WorkMutex);
            m_WorkCV.wait(lock, [&] {
                return (m_ReadySerializer != nullptr && m_SolveThreadsNeeded > 0)
                       || !m_UnBuiltMessages.empty()
                       || m_Stop;
            });

            if (m_ReadySerializer != nullptr && m_SolveThreadsNeeded > 0) {
                m_SolveThreadsNeeded--;
                serToSolve = m_ReadySerializer;
                if (m_SolveThreadsNeeded == 0) m_ReadySerializer = nullptr;
                lock.unlock();
                serToSolve->solve();
                continue;
            }

            if (!m_UnBuiltMessages.empty()) {
                auto msg = std::move(m_UnBuiltMessages.front());
                m_UnBuiltMessages.pop();

                if (!m_CurSerializer) m_CurSerializer = createMsgSerializer();

                uint32_t msgId = msg.first;
                auto resultPtr = std::make_shared<CBigInt>(0);
                auto foundPtr = std::make_shared<bool>(false);

                m_CurSerializer->addProblem(std::move(msg.second),
                    [resultPtr, foundPtr](uint32_t id, const uint8_t* data, size_t bits) {
                        CBigInt val = countExpressions(data, bits);
                        if (!*foundPtr || *resultPtr < val)
                            *resultPtr = val;
                        *foundPtr = true;
                    },
                    [this, msgId, resultPtr, foundPtr](uint32_t) {
                        std::unique_lock<std::mutex> tlock(m_TransmitMessagesMutex);
                        m_TransmitMessages.push({msgId, *resultPtr, !*foundPtr});
                        m_TransmitMessagesCV.notify_one();
                    });

                if (!m_CurSerializer->hasFreeCapacity()) {
                    m_SolveThreadsNeeded = m_CurSerializer->totalThreads() - 1;
                    m_ReadySerializer = m_CurSerializer;
                    serToSolve = m_CurSerializer;
                    m_CurSerializer = nullptr;
                    if (m_SolveThreadsNeeded == 0) m_ReadySerializer = nullptr;
                    m_WorkCV.notify_all();
                    lock.unlock();
                    serToSolve->solve();
                    continue;
                }
                continue;
            }

            if (m_Stop && !m_CurSerializer && !m_ReadySerializer) break;

            if (m_Stop && m_CurSerializer) {
                m_SolveThreadsNeeded = m_CurSerializer->totalThreads() - 1;
                m_ReadySerializer = m_CurSerializer;
                serToSolve = m_CurSerializer;
                m_CurSerializer = nullptr;
                if (m_SolveThreadsNeeded == 0) m_ReadySerializer = nullptr;
                m_WorkCV.notify_all();
                lock.unlock();
                serToSolve->solve();
                m_WorkCV.notify_all();
                continue;
            }
        }
    }
    m_WorkCV.notify_all();
}

    void TransmitterWorker (ATransmitter transmitter){
      while (true) {
        Message task;
        {
          std::unique_lock<std::mutex> lock(m_TransmitMessagesMutex);
          m_TransmitMessagesCV.wait(lock, [&] {
            return !m_TransmitMessages.empty() || m_TransmitDone;
          });
          if (m_TransmitMessages.empty() && m_TransmitDone) break;
          task = std::move(m_TransmitMessages.front());
          m_TransmitMessages.pop();
        }
        if (task.incomplete)
          transmitter->incomplete(task.msgID);
        else
          transmitter->send(task.msgID, task.result);
      }
    }

    private:

      struct Message{
        uint32_t msgID;
        CBigInt result;
        bool incomplete;
      };

      std::vector<std::thread> m_Threads;
      unsigned m_ThrCount = 0;
      std::atomic<bool> m_Stop{false};
      bool m_TransmitDone = false;

      std::vector<AReceiver> m_recievers;
      std::vector<ATransmitter> m_transmitters;

      std::mutex m_WorkMutex;
      std::condition_variable m_WorkCV;
      std::map<uint32_t, std::vector<uint64_t>> m_Fragments;
      std::mutex m_FragmentsMutex;
      std::queue<std::pair<uint32_t, std::vector<uint64_t>>> m_UnBuiltMessages;
      AMsgSerializer m_CurSerializer;
      AMsgSerializer m_ReadySerializer;
      unsigned m_SolveThreadsNeeded = 0;

      std::mutex m_TransmitMessagesMutex;
      std::queue<Message> m_TransmitMessages;
      std::condition_variable m_TransmitMessagesCV;
};


void CSentinelHacker::start(unsigned thrCount){
    this->m_ThrCount = thrCount;
    m_Stop = false;

    for (auto & reciever : m_recievers){
      m_Threads.emplace_back(&CSentinelHacker::RecieverWorker, this, reciever);
    }

    for (unsigned i = 0; i < this->m_ThrCount; i++){
      m_Threads.emplace_back(&CSentinelHacker::SolveWorker, this);
    }

    for (auto transmitter : m_transmitters){
      m_Threads.emplace_back(&CSentinelHacker::TransmitterWorker, this, transmitter);
    }
}

void CSentinelHacker::addFragment(uint64_t x){
  uint32_t msgId = x >> SHIFT_MSG_ID;
  uint8_t fragCnt = (x >> SHIFT_FRAGMENT_CNT) & MASK_FRAGMENT_CNT;
  
  std::unique_lock<std::mutex> lock(m_FragmentsMutex);
  m_Fragments[msgId].push_back(x);
  if( m_Fragments[msgId].size() == (size_t) fragCnt + 1){
    {
      std::unique_lock<std::mutex> wLock(m_WorkMutex);
      m_UnBuiltMessages.push({msgId, std::move(m_Fragments[msgId])});
    }
    m_Fragments.erase(msgId);
    m_WorkCV.notify_one();
  }
}

bool CSentinelHacker::seqSolve(const std::vector<uint64_t> & fragments, CBigInt & res){
    AMsgSerializer serializer = createMsgSerializer();
    if (fragments.empty()) return false;

    uint8_t frgCnt = (fragments[0] >> SHIFT_FRAGMENT_CNT) & MASK_FRAGMENT_CNT;
    for (const auto & fragment : fragments){
        if (((fragment >> SHIFT_FRAGMENT_CNT) & MASK_FRAGMENT_CNT) != frgCnt) return false;
    }

    bool found = false;
    CBigInt maxRes(0);
    serializer->addProblem(fragments,
        [&found, &maxRes](uint32_t msgID, const uint8_t *frags, size_t size) {
            CBigInt val = countExpressions(frags, size);
            if (!found || maxRes < val)
                maxRes = val;
            found = true;
        },
        [&](uint32_t) {});

    serializer->solve();
    if (!found) return false;
    res = maxRes;
    return true;
}

void CSentinelHacker::addReceiver(AReceiver x) {
  m_recievers.push_back(x);
}

void CSentinelHacker::addTransmitter(ATransmitter x){
  m_transmitters.push_back(x);
}

void CSentinelHacker::stop() {
    for (size_t i = 0; i < m_recievers.size(); i++) {
        m_Threads[i].join();
    }

    {
        std::unique_lock<std::mutex> fLock(m_FragmentsMutex);
        std::unique_lock<std::mutex> tLock(m_TransmitMessagesMutex);
        for (auto & [msgId, frags] : m_Fragments) {
            m_TransmitMessages.push({msgId, CBigInt(0), true});
        }
        m_Fragments.clear();
        m_TransmitMessagesCV.notify_all();
    }

    {
        std::unique_lock<std::mutex> lock(m_WorkMutex);
        m_Stop = true;
    }
    m_WorkCV.notify_all();

    for (size_t i = m_recievers.size(); i < m_recievers.size() + m_ThrCount; i++) {
        m_Threads[i].join();
    }

    {
        std::unique_lock<std::mutex> lock(m_TransmitMessagesMutex);
        m_TransmitDone = true;
    }
    m_TransmitMessagesCV.notify_all();

    for (size_t i = m_recievers.size() + m_ThrCount; i < m_Threads.size(); i++) {
        m_Threads[i].join();
    }

    m_Threads.clear();
    m_recievers.clear();
    m_transmitters.clear();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __PROGTEST__
int                                    main                                    ()
{
  using namespace std::placeholders;

  msgSerializerLimits ( 100, 1, 1, 1, 1 ); // friendly CMsgSerializer instances in sequential tests
  for ( const auto & x : g_TestSets )
  {
    CBigInt res;
    assert ( CSentinelHacker::seqSolve ( x . m_Fragments, res ) );
    assert ( CBigInt ( x . m_Result ) == res );
  }
  msgSerializerLimits ( 4, 3, 5, 2, 2 ); // more exciting boundaries for real runs

  CSentinelHacker test;
  auto            trans = std::make_shared<CExampleTransmitter> ();
  AReceiver       recv  = std::make_shared<CExampleReceiver> ( std::initializer_list<uint64_t> { 0x508e000072ba, 0x508a000004a1, 0x788a0000058c, 0x246700000092 } );

  test . addTransmitter ( trans );
  test . addReceiver ( recv );
  test . start ( 3 );

  static std::initializer_list<uint64_t> t1Data = { 0x247300061fa2, 0x246d00003977, 0x5c8e000029aa, 0x5c890000009b };
  std::thread t1 ( fragmentSender, std::bind ( &CSentinelHacker::addFragment, &test, _1 ), t1Data );

  static std::initializer_list<uint64_t> t2Data = { 0x788d000036c6, 0x788e00002ab0, 0x508a0000036c, 0x246b00000e2b };
  std::thread t2 ( fragmentSender, bind ( &CSentinelHacker::addFragment, &test, _1 ), t2Data );
  fragmentSender ( std::bind ( &CSentinelHacker::addFragment, &test, _1 ), std::initializer_list<uint64_t> { 0x508d0000007f, 0x5c8b00000aab, 0x788e00007d7d, 0x508d00002f0b, 0x7893000e6648, 0x5c8f00009f2d } );
  t1 . join ();
  t2 . join ();
  test . stop ();
  assert ( trans -> totalSent () == 2 );
  assert ( trans -> totalIncomplete () == 2 );
  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
