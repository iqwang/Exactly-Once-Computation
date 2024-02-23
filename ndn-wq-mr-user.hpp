/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#ifndef NDN_WQ_MR_USER_H
#define NDN_WQ_MR_USER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/random-variable-stream.h"
#include "ndn-app.hpp"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include <deque>
#include "ns3/ndnSIM/utils/ndn-rtt-estimator.hpp"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief Ndn application for sending out Interest packets at a "constant" rate (Poisson process)
 */
class WqMrUser : public App {
public:
  static TypeId
  GetTypeId();

  /**
   * \brief Default constructor
   * Sets up randomizer function and packet sequence number
   */
  WqMrUser();
  // virtual ~WqMrUser();
  
  virtual void
  OnData(shared_ptr<const Data> contentObject);
 
  virtual void
  OnInterest(shared_ptr<const Interest> interest);

  /**
   * @brief Timeout event
   * @param sequenceNumber time outed sequence number
   */
  virtual void
  OnTimeout(uint32_t sequenceNumber);

  virtual void
  StartApplication();

  virtual void
  StopApplication();

  /**
   * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN
   * protocol
   */
  virtual void
  ScheduleNextPacket();

private:
  void
  SendPacket();


  /**
   * @brief Set type of frequency randomization
   * @param value Either 'none', 'uniform', or 'exponential'
   */
  void
  SetRandomize(const std::string& value);

  /**
   * @brief Get type of frequency randomization
   * @returns either 'none', 'uniform', or 'exponential'
   */
  std::string
  GetRandomize() const;

   /**
   * \brief Checks if the packet need to be retransmitted becuase of retransmission timer expiration
   */
  void
  CheckRetxTimeout();

  /**
   * \brief Modifies the frequency of checking the retransmission timeouts
   * \param retxTimer Timeout defining how frequent retransmission timeouts should be checked
   */
  void
  SetRetxTimer(Time retxTimer);

  /**
   * \brief Returns the frequency of checking the retransmission timeouts
   * \return Timeout defining how frequent retransmission timeouts should be checked
   */
  Time
  GetRetxTimer() const;

public:
  void ReplyData(std::string replyContent, std::string replyName);
  void SendOutInterest(std::string newInterest);
  void CheckSeqAtReducer(std::string reducerName, std::string checkSeq);
  void AssignNodeIdByPath();
  void ResentDataCheck(std::string resentSeq);
  void AssignJobs();

protected:
  double m_frequency; // Frequency of interest packets (in hertz)
  bool m_firstTime;
  Ptr<RandomVariableStream> m_random;
  std::string m_randomType;
  Ptr<UniformRandomVariable> m_rand; ///< @brief nonce generator
  uint32_t m_seq;      ///< @brief currently requested sequence number
  uint32_t m_seqMax;   ///< @brief maximum number of sequence number
  EventId m_sendEvent; ///< @brief EventId of pending "send packet" event
  Time m_retxTimer;    ///< @brief Currently estimated retransmission timer
  EventId m_retxEvent; ///< @brief Event to check whether or not retransmission should be performed

  Ptr<RttEstimator> m_rtt; ///< @brief RTT estimator

  Time m_offTime;          ///< \brief Time interval between packets
  Name m_interestName;     ///< \brief NDN Name of the Interest (use Name)
  Time m_interestLifeTime; ///< \brief LifeTime for interest packet

private:
  Name m_prefix;
  Name m_postfix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;
  uint32_t m_signature;
  Name m_keyLocator;

  std::string m_taskContent;
  std::string m_disNeiPrefix;
  std::vector<std::string> m_oneHopNeighbours;
  std::vector<std::string> m_sendJobNeis;
  bool m_buildTaskNeighbour;
  std::map<std::string, std::string> m_checkNeibMap;
  int m_sendDisNeiNum=0;
  int m_gotDisNeiNum=0;
  std::string m_ownPrefix;
  std::string m_disDownStream1;
  std::string m_disDownStream2;
  std::string m_disDownStream3;
  int m_sendDisDownNeiNum=0;
  int m_gotDisDownNeiNum=0;
  std::map<std::string, std::string> m_downNeiMap;
  std::string m_userTimeRecord;
  int m_seqNum = 0;
  std::string m_seqStart;
  std::string m_seqEnd;
  std::map<std::string, int> m_assignJobSeq;
  std::map<std::string, int> m_receiveJobSeq;
  std::map<std::string, std::string> m_receiveSeqData;
  std::vector<std::string> m_seqOkList;
  std::map<std::string, std::string> m_doubtCheckInterest;
  std::map<std::string, std::string> m_nodePathId;
  int m_countOkSeq = 0;
  std::vector<std::string> m_askClearSeqs;

    /// @cond include_hidden
  /**
   * \struct This struct contains sequence numbers of packets to be retransmitted
   */
  struct RetxSeqsContainer : public std::set<uint32_t> {
  };

  RetxSeqsContainer m_retxSeqs; ///< \brief ordered set of sequence numbers to be retransmitted

  /**
   * \struct This struct contains a pair of packet sequence number and its timeout
   */
  struct SeqTimeout {
    SeqTimeout(uint32_t _seq, Time _time)
      : seq(_seq)
      , time(_time)
    {
    }

    uint32_t seq;
    Time time;
  };
  /// @endcond

  /// @cond include_hidden
  class i_seq {
  };
  class i_timestamp {
  };
  /// @endcond

  /// @cond include_hidden
  /**
   * \struct This struct contains a multi-index for the set of SeqTimeout structs
   */
  struct SeqTimeoutsContainer
    : public boost::multi_index::
        multi_index_container<SeqTimeout,
                              boost::multi_index::
                                indexed_by<boost::multi_index::
                                             ordered_unique<boost::multi_index::tag<i_seq>,
                                                            boost::multi_index::
                                                              member<SeqTimeout, uint32_t,
                                                                     &SeqTimeout::seq>>,
                                           boost::multi_index::
                                             ordered_non_unique<boost::multi_index::
                                                                  tag<i_timestamp>,
                                                                boost::multi_index::
                                                                  member<SeqTimeout, Time,
                                                                         &SeqTimeout::time>>>> {
  };

  SeqTimeoutsContainer m_seqTimeouts; ///< \brief multi-index for the set of SeqTimeout structs

  SeqTimeoutsContainer m_seqLastDelay;
  SeqTimeoutsContainer m_seqFullDelay;
  std::map<uint32_t, uint32_t> m_seqRetxCounts;

  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */, int32_t /*hop count*/>
    m_lastRetransmittedInterestDataDelay;
  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */,
                 uint32_t /*retx count*/, int32_t /*hop count*/> m_firstInterestDataDelay;

  /// @endcond
  
};

} // namespace ndn
} // namespace ns3

#endif
