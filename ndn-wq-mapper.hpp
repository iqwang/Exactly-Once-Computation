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

#ifndef NDN_WQ_MAPPER_H
#define NDN_WQ_MAPPER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-app.hpp"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include <deque>
#include "ns3/random-variable-stream.h"


namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A simple Interest-sink applia simple Interest-sink application
 *
 * A simple Interest-sink applia simple Interest-sink application,
 * which replying every incoming Interest with Data packet with a specified
 * size and name same as in Interest.cation, which replying every incoming Interest
 * with Data packet with a specified size and name same as in Interest.
 */
class WqMapper : public App {
public:
  static TypeId
  GetTypeId(void);

  WqMapper();

  // inherited from NdnApp
  virtual void
  OnInterest(shared_ptr<const Interest> interest);
  
  virtual void
  OnData(shared_ptr<const Data> contentObject);

public:
  void CheckNeiConnect();
  void LinkBroken(std::string upNode, std::string downNode);
  void SendInterest(std::string sendName);
  void AddSeqData(std::string seqnum, int seqdata);
  void AddSeqUpNei(std::string seqnum, std::string neiname);
  void CheckFailSeq();
  void ReplyData(std::string replyContent, shared_ptr<const Interest> interest);
  void ProcessNormalInterest(shared_ptr<const Interest> interest);
  void RegularCheckLink();
  void ClearHistorySaveData(std::string seqList);


protected:
  // inherited from Application base class.
  virtual void
  StartApplication(); // Called at time specified by Start

  virtual void
  StopApplication(); // Called at time specified by Stop

private:
  Ptr<UniformRandomVariable> m_rand;
  Name m_prefix;
  Name m_postfix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;
  uint32_t m_signature;
  Name m_keyLocator;
  
  Name m_interestName;
  Name m_pendingInterestName;
  std::string m_askPitPrefix;
  std::map<std::string, std::string> m_disTreeInterestMap;
  std::string m_currentTreeTag = "000";
  int m_askPitNum =0;
  int m_gotPitNum =0;
  std::string m_askFibPrefix;
  std::map<std::string, std::string> m_neiReachable;
  std::string m_selectNodeName;
  std::string m_selectNodeFace;
  bool reJoinAsk = false;
  std::string m_rejoinUpNeiName = "";
  EventId m_sendEvent;
  bool excuteR2M3Fail = false;
  bool excuteR2M6Fail = false;
  bool excuteR1M7Fail = false;
  std::map<std::string, int> m_sentSeqData;
  std::map<std::string, std::string> m_sentSeqToNei;
  std::map<std::string, int> m_detectFailureSeqData;
  bool m_detectLinkFailure = false;
  shared_ptr<const Interest> m_normalInterest;
  std::string m_preUpNeiName;
  std::string m_myPathID;
  std::string m_prePathID;
  std::map<std::string, std::string> m_possibleRejoinNeis; //(one-hop-nei-name, reply-content)
  int m_sendRejoinNum =0;
  int m_gotRejoinNum =0;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_MRSENSORTEST_H
