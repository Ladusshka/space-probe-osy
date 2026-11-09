// The classes in this header define the common interface between your implementation and
// the testing environment. Exactly the same implementation is present in the Progtest's
// testing environment. You are not supposed to modify any declaration in this file,
// any change is likely to break the compilation.
#ifndef common_h_22034590234652093456179823592
#define common_h_22034590234652093456179823592

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <compare>


constexpr uint32_t                     SHIFT_MSG_ID                            = 42;
constexpr uint32_t                     SHIFT_FRAGMENT_CNT                      = 37;
constexpr uint64_t                     MASK_FRAGMENT_CNT                       = 0x1f;
//=============================================================================================================================================================
/**
 * CBigInt is a simple implementation of a big-number class. The class handles positive integers of
 * size 768 bits maximum. Supported operations are initialization, addition, multiplication, comparison,
 * and a conversion-to-string.
 *
 * @note This interface is present in the progtest testing environment.
 * @note CBigInt class and the overloaded operators are available in all tests, even in the bonus tests.
 *
 */
class CBigInt
{
  public:
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Initialize an object with a 64 bit unsigned integer (zero extended in the upper bits).
     * @param[in] val        value to set
     */
                                       CBigInt                                 ( uint64_t                              val = 0);
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Initialize an object with a decimal integer.
     * @param[in] val        value to set
     * @exception std::invalid_argument if the input string is not a valid decimal integer.
     * @note overflow is silently ignored
     */
                                       CBigInt                                 ( std::string_view                      val );
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Replace the contents with a 64 bit unsigned integer (zero extended in the upper bits).
     * @param[in] val        value to set
     * @return a reference to *this
     */
    CBigInt                          & operator =                              ( uint64_t                              val );
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Convert the value into a string (decimal notation)
     * @return a string representation of *this
     */
    std::string                        toString                                () const;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Test whether the value is zero, or not
     * @return zero -> true, nonzero -> false
     */
    bool                               isZero                                  () const;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Compare two big integers
     * @param[in] x          the value to compare to
     * @return one of: less/equal/greater
     */
    std::strong_ordering               operator <=>                            ( const CBigInt                       & x ) const;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    bool                               operator ==                             ( const CBigInt                       & x ) const = default;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Add a CBigInt to *this
     * @param[in] x          the value to add
     * @return a reference to *this
     * @note overflow is ignored
     */
    CBigInt                          & operator +=                             ( const CBigInt                       & x );
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Multiply *this wint another CBigInt
     * @param[in] x          the multiplier
     * @return a reference to *this
     * @note overflow is ignored
     */
    CBigInt                          & operator *=                             ( const CBigInt                       & x );
  private:
    static constexpr uint32_t          BIGINT_BITS                             = 768;
    static constexpr uint32_t          BIGINT_INTS                             = BIGINT_BITS >> 5;
    uint32_t                           m_Data[ BIGINT_INTS ];
    void                               mulAdd                                  ( uint32_t                              st,
                                                                                 const uint32_t                        v[],
                                                                                 uint64_t                              mul );
    uint32_t                           divMod                                  ( uint32_t                              x );
};
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * A convenience wrapper - sum two big integers
 */
inline CBigInt                         operator +                              ( CBigInt                               a,
                                                                                 const CBigInt                       & b )
{
  return std::move ( a += b );
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * A convenience wrapper - multiply two big integers
 */
inline CBigInt                         operator *                              ( CBigInt                               a,
                                                                                 const CBigInt                       & b )
{
  return std::move ( a *= b );
}
//=============================================================================================================================================================
class CReceiver : public std::enable_shared_from_this<CReceiver>
{
  public:
    virtual                            ~CReceiver                              () = default;
    virtual bool                       recv                                    ( uint64_t                            & data ) = 0;
};
using AReceiver                        = std::shared_ptr<CReceiver>;
//=============================================================================================================================================================
class CTransmitter : public std::enable_shared_from_this<CTransmitter>
{
  public:
    virtual                            ~CTransmitter                           () = default;
    virtual void                       send                                    ( uint32_t                              id,
                                                                                 const CBigInt                       & cnt) = 0;
    virtual void                       incomplete                              ( uint32_t                              id ) = 0;
};
using ATransmitter                     = std::shared_ptr<CTransmitter>;
//=============================================================================================================================================================
#endif /* common_h__22034590234652093456179823592 */
