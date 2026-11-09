// The classes in this header are used in the example test. You are free to
// modify these classes, add more test cases, and add more test sets.
// These classes do not exist in the progtest's testing environment.
#ifndef sample_tester_h_0982345234598123452345
#define sample_tester_h_0982345234598123452345

#include <functional>
#include <deque>
#include "common.h"

class CExampleReceiver : public CReceiver
{
  public:
                                       CExampleReceiver                        ( std::initializer_list<uint64_t>       data );
    bool                               recv                                    ( uint64_t                            & fragment ) override;
  private:
    std::deque<uint64_t>               m_Data;
};

class CExampleTransmitter : public CTransmitter
{
  public:
                                       CExampleTransmitter                     () = default;
    void                               send                                    ( uint32_t                              id,
                                                                                 const CBigInt                       & cnt ) override;
    void                               incomplete                              ( uint32_t                              id ) override;


    size_t                             totalSent                               () const;
    size_t                             totalIncomplete                         () const;
  private:
    size_t                             m_Sent                                  = 0;
    size_t                             m_Incomplete                            = 0;
};

struct CSampleData
{
  uint32_t                             m_ID;
  const char                         * m_Result;
  std::initializer_list<uint64_t>      m_Fragments;
};

void                                   fragmentSender                          ( std::function<void(uint64_t)>         target,
                                                                                 std::initializer_list<uint64_t>       data );

extern std::initializer_list<CSampleData> g_TestSets;

#endif /* sample_tester_h_0982345234598123452345 */
