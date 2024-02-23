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

#ifndef NDN_WQ_CHECKPOINT_REDUCER_H
#define NDN_WQ_CHECKPOINT_REDUCER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/random-variable-stream.h"
#include "ndn-app.hpp"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include <deque>


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
class WqCheckpointReducer : public App {
public:
  static TypeId
  GetTypeId(void);

  WqCheckpointReducer();

  // inherited from NdnApp
  virtual void
  OnInterest(shared_ptr<const Interest> interest);
  // From App
  virtual void
  OnData(shared_ptr<const Data> contentObject);

public:
  void FindNeighbours();
  void DiscoverDownstreams();
  void QueryPit();
  void CheckNeiConnect();
  void ReplyRejoinInterest(std::string pathId);
  void ProcessRejoinInterest();
  void RejoinTreeNeighbour(std::string requestNeiName);
  void AddRejoinDownNei(std::string neiName);
  void SendOutInterest(std::string newInterest);
  void ReplyData(std::string replyContent, std::string replayName);
  void AddComputeGroup(std::string newId);
  void SaveSeqInterestName(std::string seqId, std::string requestSeqName);
  void ProcessDataBySeq(std::string startProcessId);
  void ProcessTaskNeis(std::string neiString);
  void AddLostNeiId(std::string lostNodeName);
  void NewJoinAssignId(std::string joinNode);
  void LinkBroken(std::string upNode, std::string downNode);
  void AddSeqData(std::string seqnum, std::string seqdata);
  void CheckFailSeq();
  void NotifyPathIdChange();
  void LeaveJobTree();
  void ClearHistorySaveData(std::string seqList);
  void ForwardClearDataSignal(std::string clearMessage);
  void ProcessNormalInterest(shared_ptr<const Interest> taskInterest);
  void RejoinTreeDueToUpNeiFail(std::string preChooseLink);
  void ReportFailure(std::string downNei, std::string seqNum);
  void CreateJobNeiList();


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
  Time m_interestLifeTime; ///< \brief LifeTime for interest packet


  std::string m_taskInterestName;
  Name m_pendingInterestName;
  std::map<std::string, std::string> m_buildTreeInterestMap;
  std::string m_askPitPrefix;
  std::string m_askFibPrefix;
  int m_askPitNum=0;
  int m_gotPitNum=0;
  std::string m_selectUpstream;
  std::vector<std::string> m_oneHopNeighbours;
  int m_sendDisNeiNum=0;
  int m_gotDisNeiNum=0;
  std::map<std::string, std::string> m_checkNeibMap;
  std::string m_disNeiPrefix;
  std::string m_disDownStream1;
  std::string m_disDownStream2;
  int m_sendDisDownNeiNum =0;
  int m_gotDisDownNeiNum =0;
  std::vector<std::string> m_pendingTreeTagList;
  std::map<std::string, std::string> m_pendTreeJobMap;
  std::string m_treeTag = "000";
  std::string m_currentTreeFlag = "000";
  std::map<std::string, std::string> m_jobRefMap; //(tree-id, job-nei-names in string)
  std::string m_jobRefNei = "0";
  std::map<std::string, std::string> m_disDownNodeMap; //(one-hop-nei-name, reply-content)
  std::vector<std::string> m_nodeList4Task;
  std::string m_assignTask;
  std::string m_reduceFunc;
  std::vector<std::string> m_upDisNodes;
  std::map<std::string, std::string> m_fibResult;
  std::map<std::string, std::string> m_neiReachable;
  std::string m_selectNodeName = "";
  std::string m_selectNodeFace;
  bool jobNeiChangeFlag = false;
  std::string m_askRejoinNeiName;
  std::map<std::string, int> m_seqDataSendNum;
  std::map<std::string, int> m_seqDataGotNum;
  std::map<std::string, std::string> m_allReceiveSeqData;
  std::string m_doubtSeq = "";
  std::map<std::string, std::string> m_computeGroup;
  std::map<std::string, std::string> m_seqInterestName;
  std::vector<std::string> m_groupIds;
  int m_countdata = 0;
  std::vector<std::string> m_countSeq;
  std::vector<std::string> m_processOkSeq;
  std::string m_lostNei = "";
  std::map<std::string, std::string> m_receiveNodeandData;
  std::map<std::string, std::string> m_nodePathId; //(nodeName, id)
  std::string m_myPathID;
  std::string m_joinNeiPathId;
  std::map<std::string, std::string> m_neiLocalId; //(nodeName, id)
  std::map<std::string, std::string> m_lostNeiIdRecords; //(nodeName, id)
  bool excute1Fail = false;
  bool excute2Fail = false;
  bool reJoinAsk = false;
  bool m_detectLinkFailure = false;
  std::map<std::string, std::string> m_detectFailureSeqData;
  std::map<std::string, std::string> m_possibleRejoinNeis; //(one-hop-nei-name, reply-content)
  int m_sendRejoinNum =0;
  int m_gotRejoinNum =0;
  std::string m_sendRejoinNode;
  std::string m_prePathID;
  std::string m_forwardDoubtCheck= "";
  std::string m_forwardSeqProcess= "";
  bool m_sendDoubtNode = false;
  shared_ptr<const Interest> m_normalInterest;
  std::string m_pathIdInterest = "";
  uint64_t m_countPathIdReply = 0;
  std::map<std::string, std::string> m_processedSeqData;
  bool m_upNodeFail = false;
  std::string m_interestOfUpfail= "";
  std::map<std::string, std::string> m_reportFailNeiList;
  std::string m_interestAsRecoverReducer= "";
  std::string m_preUpNodeName = "";
  bool m_cpFailure=false;
};


} // namespace ndn
} // namespace ns3

#endif
