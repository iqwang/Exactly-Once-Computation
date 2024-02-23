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

// #include "/usr/include/python3.8/Python.h"
#include "ndn-wq-checkpoint-reducer.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"
#include <memory>

#include <deque>



#include <iostream>
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/command-line.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include <fstream>



NS_LOG_COMPONENT_DEFINE("ndn.WqCheckpointReducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(WqCheckpointReducer);

TypeId
WqCheckpointReducer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::WqCheckpointReducer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<WqCheckpointReducer>()
      .AddAttribute("Prefix", "Prefix, for which producer has the data", StringValue("/"),
                    MakeNameAccessor(&WqCheckpointReducer::m_prefix), MakeNameChecker())
      .AddAttribute("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
					StringValue("/"), MakeNameAccessor(&WqCheckpointReducer::m_postfix), MakeNameChecker())
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets", UintegerValue(1024),
                    MakeUintegerAccessor(&WqCheckpointReducer::m_virtualPayloadSize),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                    TimeValue(Seconds(0)), MakeTimeAccessor(&WqCheckpointReducer::m_freshness),
                    MakeTimeChecker())
      .AddAttribute("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
					UintegerValue(0), MakeUintegerAccessor(&WqCheckpointReducer::m_signature),
					MakeUintegerChecker<uint32_t>())
      .AddAttribute("LifeTime", "LifeTime for interest packet", StringValue("200s"),
                    MakeTimeAccessor(&WqCheckpointReducer::m_interestLifeTime), MakeTimeChecker())
      .AddAttribute("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&WqCheckpointReducer::m_keyLocator), MakeNameChecker()); 
  return tid;
}

WqCheckpointReducer::WqCheckpointReducer()
{
  m_rand = CreateObject<UniformRandomVariable>();
  NS_LOG_FUNCTION_NOARGS();
}

// inherited from Application base class.
void
WqCheckpointReducer::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
}

void
WqCheckpointReducer::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}


void
WqCheckpointReducer::FindNeighbours()
{
  //std::cout << "find neighbours: " << m_pendingInterestName.toUri() << std::endl;
  m_disNeiPrefix = "/nei-";
  for(uint8_t i=0; i < m_allNodeName.size(); i++)
  {
    if(m_allNodeName[i] != m_prefix.toUri())
    {
      m_checkNeibMap.insert(std::pair<std::string, std::string>(m_allNodeName[i],"0"));
      // written as: /nei-/user-
      std::string tempDisNei = m_disNeiPrefix + m_allNodeName[i];
      shared_ptr<Name> disNeiName = make_shared<Name>(tempDisNei);
      //originalInterest->appendSequenceNumber(m_rand->GetValue(0, std::numeric_limits<uint16_t>::max()));
      shared_ptr<Interest> disNeiInterest = make_shared<Interest>();
      disNeiInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
      disNeiInterest->setName(*disNeiName);
      disNeiInterest->setInterestLifetime(ndn::time::seconds(1));
      m_transmittedInterests(disNeiInterest, this, m_face);
      m_appLink->onReceiveInterest(*disNeiInterest);
      m_sendDisNeiNum +=1;
    }
  };
};

void
WqCheckpointReducer::DiscoverDownstreams()
{
  m_disDownStream1 = "/discoverTS";
  m_disDownStream2 = "/TE-";
  //std::cout << m_prefix.toUri() << "continue discover: " << m_upDisNodes.size() << std::endl;
  
  if(m_oneHopNeighbours.size() == 0) 
  {
    FindNeighbours();
  }
  else
  {
    //std::cout << m_prefix.toUri() << "continue discover: " << m_upDisNodes.size() << std::endl;
    for(uint8_t t=0; t<m_oneHopNeighbours.size(); t++)
    {
      if(m_oneHopNeighbours[t] != m_treeTag && m_oneHopNeighbours[t] != m_selectNodeName )
      {
        m_disDownNodeMap.insert(std::pair<std::string, std::string>(m_oneHopNeighbours[t],"0"));
        std::string tempDisTree = m_oneHopNeighbours[t] + m_prefix.toUri() + m_disDownStream1 + m_treeTag + m_disDownStream2;
        //std::string tempDisTree = m_oneHopNeighbours[t];
        std::cout << m_prefix.toUri() << " send dis tree: " << tempDisTree << std::endl;
        shared_ptr<Name> disTreeName = make_shared<Name>(tempDisTree);
        disTreeName->appendSequenceNumber(m_rand->GetValue(0, std::numeric_limits<uint16_t>::max()));
        shared_ptr<Interest> disTreeInterest = make_shared<Interest>();
        disTreeInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
        disTreeInterest->setName(*disTreeName);
        disTreeInterest->setInterestLifetime(ndn::time::seconds(1));
        m_transmittedInterests(disTreeInterest, this, m_face);
        m_appLink->onReceiveInterest(*disTreeInterest);
        m_sendDisDownNeiNum++;
      }
      else if (m_oneHopNeighbours.size() ==1 && m_oneHopNeighbours[0] == m_selectNodeName)
      {
        // reply nope to upstream because no downstream
        std::string replyNackName = m_selectUpstream;
        Name replyNackDataName(replyNackName);
        auto replyNackData = make_shared<Data>();
        replyNackData->setName(replyNackDataName);
        replyNackData->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
        std::string rawData("nope");
        const uint8_t* p = reinterpret_cast<const uint8_t*>(rawData.c_str());
        auto buffer = make_shared< ::ndn::Buffer>(p, rawData.size());
        replyNackData->setContent(buffer);
          
        SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));
        if (m_keyLocator.size() > 0) {
          signatureInfo.setKeyLocator(m_keyLocator);
        }
        replyNackData->setSignatureInfo(signatureInfo);
        ::ndn::EncodingEstimator estimator;
        ::ndn::EncodingBuffer encoder(estimator.appendVarNumber(m_signature), 0);
        encoder.appendVarNumber(m_signature);
        replyNackData->setSignatureValue(encoder.getBuffer());
        NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << replyNackData->getName());

        // to create real wire encoding
        replyNackData->wireEncode();
        m_transmittedDatas(replyNackData, this, m_face);
        m_appLink->onReceiveData(*replyNackData);
        std::cout << m_prefix.toUri() << " reply (no downstream): " << rawData << " for: " << replyNackName << std::endl;
      }
    }
  }
    
};

void
WqCheckpointReducer::QueryPit()
{
  //std::cout << m_prefix.toUri() <<" map size " <<m_buildTreeInterestMap.size() << std::endl;
  for(std::map<std::string, std::string>::iterator it=m_buildTreeInterestMap.begin(); it!=m_buildTreeInterestMap.end(); ++it)
  {
    m_askPitPrefix = "/p-";
    std::string tempAskPit = m_askPitPrefix + it->first;
    //std::cout << m_prefix.toUri() <<"tempAskPit: "<< tempAskPit <<std::endl;
    shared_ptr<Name> askPitName = make_shared<Name>(tempAskPit);
    //subBtInterest->appendSequenceNumber(m_rand->GetValue(0, std::numeric_limits<uint16_t>::max()));
    shared_ptr<Interest> askPitInterest = make_shared<Interest>();
    askPitInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
    askPitInterest->setName(*askPitName);
    askPitInterest->setInterestLifetime(ndn::time::seconds(1));
    m_transmittedInterests(askPitInterest, this, m_face);
    m_appLink->onReceiveInterest(*askPitInterest);
    m_askPitNum++;
  }  
};

void
WqCheckpointReducer::SendOutInterest(std::string newInterest)
{
  shared_ptr<Name> sendName = make_shared<Name>(newInterest);
  if ((newInterest.substr(0,2) != "/p") & (newInterest.substr(0,2) != "/f")) {
    sendName->appendSequenceNumber(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  }
  // sendName->appendSequenceNumber(m_rand->GetValue(0, std::numeric_limits<uint16_t>::max()));
  shared_ptr<Interest> sendInterest = make_shared<Interest>();
  sendInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  sendInterest->setName(*sendName);
  sendInterest->setInterestLifetime(ndn::time::seconds(1));
  m_transmittedInterests(sendInterest, this, m_face);
  m_appLink->onReceiveInterest(*sendInterest);
  // std::cout <<m_prefix.toUri() << " Send-Interest: " << newInterest << std::endl;
};

void
WqCheckpointReducer::ReplyData(std::string replyContent, std::string replayName)
{
  Name dataName(replayName);
  auto taskData = make_shared<Data>();
  taskData->setName(dataName);
  taskData->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
  const uint8_t* p = reinterpret_cast<const uint8_t*>(replyContent.c_str());
  auto buffer = make_shared< ::ndn::Buffer>(p, replyContent.size());
  taskData->setContent(buffer);

  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));
  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }
  taskData->setSignatureInfo(signatureInfo);
  ::ndn::EncodingEstimator estimator;
  ::ndn::EncodingBuffer encoder(estimator.appendVarNumber(m_signature), 0);
  encoder.appendVarNumber(m_signature);
  taskData->setSignatureValue(encoder.getBuffer());
  NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << taskData->getName());

  // to create real wire encoding
  taskData->wireEncode();
  m_transmittedDatas(taskData, this, m_face);
  m_appLink->onReceiveData(*taskData);
  std::cout <<m_prefix.toUri() << " reply-data: " << replyContent << std::endl;
};

void
WqCheckpointReducer::AddRejoinDownNei(std::string neiName)
{
  std::string addNewNei = m_prefix.toUri() + neiName + m_currentTreeFlag;
  // std::cout << m_prefix.toUri() <<" add new nei " << addNewNei <<std::endl;
  m_neiReachable.insert(std::pair<std::string, std::string>(addNewNei, "true"));
  m_jobRefMap.insert(std::pair<std::string, std::string>(m_currentTreeFlag, neiName));
  // for (auto& x: m_neiReachable) {
  //   std::cout << m_prefix.toUri() << " link= " << x.first << "  === " << x.second << std::endl;
  // };
};

void
WqCheckpointReducer::NewJoinAssignId(std::string joinNode)
{
  if(m_lostNeiIdRecords.size() == 0)
  {
    // std::cout << m_prefix.toUri() << " ---- 111 = " << std::endl;
    // for(auto& x: m_nodePathId) {
    //   std::cout << m_prefix.toUri() << " nodeName= " << x.first << " pathID= "<< x.second << std::endl;
    // };

    std::map<std::string, std::string>::iterator checkNei = m_neiLocalId.find(joinNode);
    if(checkNei == m_neiLocalId.end())
    {
      if(m_neiLocalId.size() == 0) {
        m_neiLocalId.insert(std::pair<std::string, std::string>(joinNode, "0"));
        m_joinNeiPathId = m_myPathID + "-" + "0";
      }
      else {
        --checkNei;
        std::string existMaxId = checkNei->second;
        std::string assignId = std::to_string(std::stoi(existMaxId) + 1);
        m_neiLocalId.insert(std::pair<std::string, std::string>(joinNode, assignId));
        m_joinNeiPathId = m_myPathID + "-" + assignId;
      }
      m_nodePathId.insert(std::pair<std::string, std::string>(joinNode, m_joinNeiPathId));
      // std::cout  << " m_joinNeiPathId = " << m_joinNeiPathId << std::endl;
    }
    else
    {
      // std::cout  << " m_joinNeiPathId = " << m_joinNeiPathId << std::endl;
      std::map<std::string, std::string>::iterator checkId = m_nodePathId.find(joinNode);
      if(checkId == m_nodePathId.end())
      {
        m_joinNeiPathId = m_myPathID + "-" + checkNei->second;
        m_nodePathId.insert(std::pair<std::string, std::string>(joinNode, m_joinNeiPathId));
      }
      else {
        m_joinNeiPathId = checkId->second;
      };  
    };
    std::cout << m_prefix.toUri() <<" add new-join nei " << joinNode << " pathId= " << m_joinNeiPathId  << std::endl;
  }
  else
  {
    std::map<std::string, std::string>::iterator searchNode = m_lostNeiIdRecords.find(joinNode);
    if(searchNode == m_lostNeiIdRecords.end())
    {
      std::string reuseId = m_lostNeiIdRecords.begin()->second;
      m_neiLocalId.insert(std::pair<std::string, std::string>(joinNode, reuseId));
      m_joinNeiPathId = m_myPathID + "-" + reuseId;
      m_nodePathId.insert(std::pair<std::string, std::string>(joinNode, m_joinNeiPathId));
    }
    else
    {
      std::string preId = searchNode->second;
      m_neiLocalId.insert(std::pair<std::string, std::string>(joinNode, preId));
      m_joinNeiPathId = m_myPathID + "-" + preId;
      m_nodePathId.insert(std::pair<std::string, std::string>(joinNode, m_joinNeiPathId));
    };
    std::map<std::string, std::string>::iterator d = m_lostNeiIdRecords.begin();
    m_lostNeiIdRecords.erase(d->first);
    std::cout << m_prefix.toUri() <<" add new-join nei (reuse-LostID) " << joinNode << " pathId= " << m_joinNeiPathId  << std::endl;
  }
};

void
WqCheckpointReducer::ReplyRejoinInterest(std::string pathId)
{
  std::string rejoinAnswer = "";
  if(pathId == "none") {
    rejoinAnswer = "No-route";
  }
  else {
    rejoinAnswer = "Join-Success-/pathId(" + pathId + ")";
    AddRejoinDownNei(m_askRejoinNeiName);
  };
  Name dataName(m_pendingInterestName);
  auto taskData = make_shared<Data>();
  taskData->setName(dataName);
  taskData->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
  const uint8_t* p = reinterpret_cast<const uint8_t*>(rejoinAnswer.c_str());
  auto buffer = make_shared< ::ndn::Buffer>(p, rejoinAnswer.size());
  taskData->setContent(buffer);

  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));
  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }
  taskData->setSignatureInfo(signatureInfo);
  ::ndn::EncodingEstimator estimator;
  ::ndn::EncodingBuffer encoder(estimator.appendVarNumber(m_signature), 0);
  encoder.appendVarNumber(m_signature);
  taskData->setSignatureValue(encoder.getBuffer());
  NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << taskData->getName());

  // to create real wire encoding
  taskData->wireEncode();
  m_transmittedDatas(taskData, this, m_face);
  m_appLink->onReceiveData(*taskData);
  std::cout <<m_prefix.toUri() << " return: " << rejoinAnswer << std::endl;
  m_joinNeiPathId = "";
  m_askRejoinNeiName = "";
};

void
WqCheckpointReducer::RejoinTreeNeighbour(std::string requestNeiName)
{
  if(m_oneHopNeighbours.size() == 0) 
  {
    FindNeighbours();
  }
  else if (m_oneHopNeighbours.size() == 1 && m_oneHopNeighbours[0] == requestNeiName) 
  {
    std::cout << m_prefix.toUri() << "no route to join current tree" << std::endl;
    ReplyRejoinInterest("none");
  }
  else
  {
    if ( std::find(m_oneHopNeighbours.begin(), m_oneHopNeighbours.end(), m_currentTreeFlag) != m_oneHopNeighbours.end() ) 
    {
      std::cout << m_prefix.toUri() << " directly connect to sink node" << std::endl;
    }
    else 
    {
      for(uint8_t t=0; t<m_oneHopNeighbours.size(); t++)
      {
        if (m_oneHopNeighbours[t]!=requestNeiName) 
        {
          m_disDownNodeMap.insert(std::pair<std::string, std::string>(m_oneHopNeighbours[t],"0"));
          std::string tempDisTree = m_oneHopNeighbours[t] + "/rejoin-" + m_prefix.toUri() + "/TS" + m_currentTreeFlag + "/TE-";
          std::cout << m_prefix.toUri() << " send rejoin-discover tree: " << tempDisTree << std::endl;
          SendOutInterest(tempDisTree);
          m_sendDisDownNeiNum++;
        }
      };
    };
  }
};

void
WqCheckpointReducer::RejoinTreeDueToUpNeiFail (std::string preChooseLink)
{
  //to make sure the node has other possible up-neis
  if(m_neiReachable.size() > 1)
  {
    std::map<std::string, std::string>::iterator findLink;
    for(findLink=m_neiReachable.begin(); findLink != m_neiReachable.end(); ++findLink) 
    {
      if(findLink->first != preChooseLink) {
        uint64_t t = findLink->first.find_last_of("/");
        std::string treeId= findLink->first.substr(t);
        if(treeId == m_currentTreeFlag) 
        {
          std::cout<< m_prefix.toUri() << " potential nei to current user: " << findLink->first << std::endl;
          uint64_t s1 = findLink->first.find("-");
          uint64_t s2 = findLink->first.find_last_of("/");
          std::string upNeiName = findLink->first.substr(s1+1, s2-s1-1);

          std::string changeNeiInterest = upNeiName + "/rejoin-" + m_prefix.toUri() + "/TS" + m_currentTreeFlag + "/TE-";
          std::cout<< m_prefix.toUri() << " changeUpNeiInterest: " << changeNeiInterest << std::endl;
          SendOutInterest(changeNeiInterest);
          reJoinAsk = true;
          m_sendRejoinNum++;
          m_possibleRejoinNeis.insert(std::pair<std::string, std::string>(upNeiName,"0"));

          if(m_sendRejoinNum == 0) 
          {
            std::cout<< m_prefix.toUri() << " has no up-nei to rejoin" << std::endl;
            for(uint64_t i=0; i<m_nodeList4Task.size(); i++) {
              std::string tellDownNei = m_nodeList4Task[i] + "/Upfail-" + "/TS" + m_currentTreeFlag + "/TE-";
              SendOutInterest(tellDownNei);
              reJoinAsk = true;
            };
          };
        };
      };
    };
  }
  else {
    std::cout<< m_prefix.toUri() << " has NO other routes... " << std::endl;
  }; 
  
};

void
WqCheckpointReducer::ProcessRejoinInterest()
{
  std::cout << m_prefix.toUri() <<" get rejoin-tree Interest: " << m_pendingInterestName.toUri() <<std::endl;
  uint64_t u1 = m_pendingInterestName.toUri().find("TS");
  uint64_t u2 = m_pendingInterestName.toUri().find("TE");
  uint64_t u3 = m_pendingInterestName.toUri().find("rejoin");
  std::string treeId = m_pendingInterestName.toUri().substr(u1+2, u2-u1-3);
  m_askRejoinNeiName = m_pendingInterestName.toUri().substr(u3+7, u1-u3-8);
  // std::cout << m_prefix.toUri() <<" receive tree Id= " << treeId <<std::endl;
  std::map<std::string, std::string>::iterator t = m_jobRefMap.find(treeId);
  if(t != m_jobRefMap.end()) 
  {
    //downstream job neis=0, meaning this node leave job tree before, it needs to re-connect before rely to rejoin-node
    if(t->second == "0")
    {
      t->second = m_askRejoinNeiName;
      AddRejoinDownNei(m_askRejoinNeiName);
      std::string reconnect_upstream = m_selectNodeName + "/backTree-" + m_prefix.toUri() + "/TS" + m_currentTreeFlag + "/TE-";
      std::cout << m_prefix.toUri() << " re-connect to upstream: " << reconnect_upstream << std::endl;
      SendOutInterest(reconnect_upstream);
    }
    else {
      uint64_t check = t->second.find(m_askRejoinNeiName);
      if(check == std::string::npos)
      {
        t->second += m_askRejoinNeiName;
        jobNeiChangeFlag = true;
        AddRejoinDownNei(m_askRejoinNeiName);
        std::cout << m_prefix.toUri() << " update job neis: " << t->second << std::endl;
      }
      NewJoinAssignId(m_askRejoinNeiName);
      ReplyRejoinInterest(m_joinNeiPathId);
    };
  }
  else
  {
    m_currentTreeFlag = treeId;
    //initiate new round to ask one-hop neighbour
    if (m_oneHopNeighbours.size() != 0)
    {
      //send interests to one-hop-nei to rejoin tree
      RejoinTreeNeighbour(m_askRejoinNeiName);
    }
    else 
    {
      FindNeighbours();
    }
  }
};

void
WqCheckpointReducer::CheckNeiConnect()
{
  std::map<std::string, std::string>::iterator i;
  std::cout << "------------------rrrrrrrrrrrrr" << std::endl;
  for(i=m_neiReachable.begin(); i != m_neiReachable.end(); ++i) 
  {
    if(i->second == "false") 
    {
      uint64_t f1 = i->first.find_first_of("-");
      uint64_t f2 = i->first.find_last_of("/");
      std::string disconnectNei = i->first.substr(f1+1, f2-f1-1);
      std::string neiOnTreeId = i->first.substr(f2);
      std::cout << m_prefix.toUri() << " disconnect nei= " << disconnectNei << " current treeId= " << neiOnTreeId << " and Seq= " << m_doubtSeq << std::endl;
      std::map<std::string, std::string>::iterator t = m_jobRefMap.find(neiOnTreeId);
      if (t != m_jobRefMap.end()) 
      {
        std::string::size_type dn = t->second.find(disconnectNei);
        std::cout << m_prefix.toUri() << " before delete disconnect-nei = " << t->second << " find= " << dn << std::endl;
        if (dn != std::string::npos) 
        {
          t->second.erase(dn, disconnectNei.length());
          std::cout << m_prefix.toUri() << " after delete disconnect-nei = " << t->second << std::endl;
        }
        jobNeiChangeFlag = true;
        std::string disconnectNode = disconnectNei.substr(0, disconnectNei.length()-1);
        if (disconnectNode == m_lostNei)
        {
          std::cout << m_prefix.toUri() << " disconnect = lost: " << disconnectNode << std::endl;
        };
      };
    }; 
    std::cout << m_prefix.toUri() + " nei-reachable: " << i -> first <<  " == " << i -> second << std::endl;
  };
};

void
WqCheckpointReducer::AddComputeGroup(std::string newId)
{
  std::string groupID;
  //to easily check, m_groupIds only store compute-group-id
  if (m_groupIds.size() == 0) 
  {
    int endGroup = stoi(newId) + 4;
    groupID = newId + "-" + std::to_string(endGroup);
    m_groupIds.push_back(groupID);
    m_computeGroup.insert(std::pair<std::string, std::string>(groupID, ""));
  }
  else 
  {
    std::string existingId = m_groupIds.back();
    uint64_t i = existingId.find("-");
    int endGroup = stoi(existingId.substr(i+1));
    // std::cout << m_prefix.toUri() << " endGroup: " << endGroup << std::endl;
    int currentNum = stoi(newId);
    if ( currentNum > endGroup)
    {
      endGroup = currentNum + 4;
      groupID = newId + "-" + std::to_string(endGroup);
      m_groupIds.push_back(groupID);
      m_computeGroup.insert(std::pair<std::string, std::string>(groupID, ""));
    };
  };
  // for (auto& x: m_computeGroup) {
  //   std::cout << m_prefix.toUri() << " groupID: " << x.first << std::endl;
  // };
};

void
WqCheckpointReducer::SaveSeqInterestName(std::string seqId, std::string requestSeqName)
{
  std::map<std::string, std::string>::iterator checkid = m_seqInterestName.find(seqId);
  if(checkid == m_seqInterestName.end())
  {
    m_seqInterestName.insert(std::pair<std::string, std::string>(seqId, requestSeqName));
  };
  // for (auto& x: m_seqInterestName) {
  //   std::cout << m_prefix.toUri() << " seqID: " << x.first << " and name= " << x.second << std::endl;
  // };
};

void
WqCheckpointReducer::ProcessDataBySeq(std::string startProcessId)
{
  //check if already process and return seq-data
  if ( std::find(m_processOkSeq.begin(), m_processOkSeq.end(), startProcessId) == m_processOkSeq.end() )
  {
    std::map<std::string, int>::iterator compareSeq = m_seqDataSendNum.find(startProcessId);
    if (compareSeq != m_seqDataSendNum.end()) 
    {
      if (m_seqDataSendNum.at(startProcessId) == m_seqDataGotNum.at(startProcessId))
      {
        std::string replyInterest = m_seqInterestName[startProcessId];
        std::string alldata = m_allReceiveSeqData[startProcessId];
        // std::cout << m_prefix.toUri() << " InterstName= " << replyInterest << " && data= " << alldata << std::endl;

        std::vector<int> commaVec;
        std::vector<std::string> neiData;
        uint64_t pos = alldata.find(","); 
        if (pos == std::string::npos) {
          neiData.push_back(alldata);
        }
        else {
          while (pos != std::string::npos){
            commaVec.push_back(pos);
            pos = alldata.find(",", pos+1);
          }
        };

        if (commaVec.size() != 0) {
          neiData.push_back(alldata.substr(0, commaVec[0]));
          for(uint64_t k=0; k<commaVec.size(); k++) {
            neiData.push_back(alldata.substr(commaVec[k]+1, commaVec[k+1]-commaVec[k]-1));
          }
        };

        int tempSumData = 0;
        for (uint8_t i=0; i < neiData.size(); i++) 
        {
          uint64_t l1 = neiData[i].find_last_of("-");
          std::string mapperData = neiData[i].substr(l1+1);
          tempSumData += std::stoi(mapperData);
          // std::cout << m_prefix.toUri() << " --- HERE --- " << " neiData= " << neiData[i] << std::endl;
        };
        std::string rawData = std::to_string(tempSumData / neiData.size());
        uint64_t findS = startProcessId.find("Seq");
        std::string rxSeq = startProcessId.substr(findS);

        rawData = rxSeq + "-" + rawData;
        // std::cout<< m_prefix.toUri() << " Reply to Who=== " << replyInterest << std::endl;
        ReplyData(rawData, replyInterest);
        std::map<std::string, std::string>::iterator i = m_processedSeqData.find(rxSeq);
        if (i == m_processedSeqData.end()) {
          m_processedSeqData.insert(std::pair<std::string, std::string>(rxSeq, rawData));
        };
        std::ofstream recording;
        recording.open("computeStateRecord.txt", std::ios_base::app);
        recording << Simulator::Now().GetSeconds() << '\t' << m_prefix.toUri() << '\t' << m_processedSeqData.size() << std::endl;
        recording.close();

        m_processOkSeq.push_back(startProcessId);
        m_seqDataGotNum.erase(startProcessId);
        m_seqDataSendNum.erase(startProcessId);
      }
      else {
        std::cout << m_prefix.toUri() << " !!! " << startProcessId << " Send != Received " << std::endl;
      };
    }
    else {
      std::cout << m_prefix.toUri() << " SeqNum not existing " << std::endl;
    };
  }
};

void 
WqCheckpointReducer::ProcessTaskNeis(std::string neiString)
{
  std::deque<int> slashQ;
  std::deque<int> lineQ;
  for(uint64_t k=0; k<neiString.size(); k++)
  { 
    if(neiString[k] == '/')
    {
      slashQ.push_back(k);
    }
    if(neiString[k] == '-')
    {
      lineQ.push_back(k);
    }
  };
  for(uint64_t k1=0; k1<slashQ.size(); k1++)
  {
    std::string tempNodeName =  neiString.substr(slashQ[k1],lineQ[k1]-slashQ[k1]+1);
    // std::cout << m_prefix.toUri() << " 1. node name: " << tempNodeName << std::endl;
    if(std::find(m_nodeList4Task.begin(), m_nodeList4Task.end(), tempNodeName) == m_nodeList4Task.end()) {
      m_nodeList4Task.push_back(tempNodeName);
    };
  };
};

void 
WqCheckpointReducer::CreateJobNeiList()
{
  std::map<std::string, std::string>::iterator it_job;
  it_job = m_jobRefMap.find(m_currentTreeFlag);
  if(it_job == m_jobRefMap.end()) {
    m_jobRefMap.insert(std::pair<std::string, std::string>(m_currentTreeFlag, m_jobRefNei));
  }
  else {
    it_job->second = m_jobRefNei;
  }
  
  for(uint64_t j=0; j<m_nodeList4Task.size(); j++) {
    std::string addNewNei = m_prefix.toUri() + m_nodeList4Task[j] + m_currentTreeFlag;
    // std::cout << m_prefix.toUri() <<" add new nei " << addNewNei <<std::endl;
    it_job = m_neiReachable.find(addNewNei);
    if(it_job == m_neiReachable.end()) {
      m_neiReachable.insert(std::pair<std::string, std::string>(addNewNei, "true"));
    }
    else {
      it_job->second = "true";
    };
  };
};

void 
WqCheckpointReducer::AddLostNeiId(std::string lostNodeName)
{
  std::map<std::string, std::string>::iterator checkLocalId = m_neiLocalId.find(lostNodeName);
  if(checkLocalId != m_neiLocalId.end())
  {
    std::string nodeLocalId = checkLocalId->second;
    m_lostNeiIdRecords.insert(std::pair<std::string, std::string>(lostNodeName, nodeLocalId));
  };
  m_neiLocalId.erase(lostNodeName);
  m_nodePathId.erase(lostNodeName);
  // for(auto& x: m_neiLocalId) {
  //   std::cout << m_prefix.toUri() << " nodeName= " << x.first << " localID= "<< x.second << std::endl;
  // };
};

void 
WqCheckpointReducer::LinkBroken(std::string upNode, std::string downNode)
{
  if(upNode == "/2-" && downNode == "/4-")
  {
    std::string changeLink = "/2-/4-/0-";
    std::map<std::string, std::string>::iterator l = m_neiReachable.find(changeLink);
    if (l != m_neiReachable.end()) {
      l->second = "false";
    };
  }
  else if(upNode == "/4-" && downNode == "/7-")
  {
    std::string changeLink = "/7-/4-/0-";
    std::map<std::string, std::string>::iterator l = m_neiReachable.find(changeLink);
    if (l != m_neiReachable.end()) {
      l->second = "false";
    };
  }
  else if(upNode == "/5-" && downNode == "/m3-")
  {
    std::string changeLink = "/5-/m3-/0-";
    std::map<std::string, std::string>::iterator l = m_neiReachable.find(changeLink);
    if (l != m_neiReachable.end()) {
      l->second = "false";
    };
  }
  else if(upNode == "/1-" && downNode == "/m5-")
  {
    std::string changeLink = "/1-/m5-/0-";
    std::map<std::string, std::string>::iterator l = m_neiReachable.find(changeLink);
    if (l != m_neiReachable.end()) {
      l->second = "false";
    };
  }
  else if(upNode == "/28-" && downNode == "/55-")
  {
    std::string changeLink = "/28-/55-/0-";
    std::map<std::string, std::string>::iterator l = m_neiReachable.find(changeLink);
    if (l != m_neiReachable.end()) {
      l->second = "false";
    };
  }
  else if(upNode == "/17-" && downNode == "/48-")
  {
    std::string changeLink = "/17-/48-/0-";
    std::map<std::string, std::string>::iterator l = m_neiReachable.find(changeLink);
    if (l != m_neiReachable.end()) {
      l->second = "false";
    };
  }
  else if(upNode == "/26-" && downNode == "/80-")
  {
    std::string changeLink = "/26-/80-/0-";
    std::map<std::string, std::string>::iterator l = m_neiReachable.find(changeLink);
    if (l != m_neiReachable.end()) {
      l->second = "false";
    };
  }
  else if(upNode == "/24-" && downNode == "/95-")
  {
    std::string changeLink = "/24-/95-/0-";
    std::map<std::string, std::string>::iterator l = m_neiReachable.find(changeLink);
    if (l != m_neiReachable.end()) {
      l->second = "false";
    };
  }
  else if(upNode == "/14-" && downNode == "/32-")
  {
    std::string changeLink = "/14-/32-/0-";
    std::map<std::string, std::string>::iterator l = m_neiReachable.find(changeLink);
    if (l != m_neiReachable.end()) {
      l->second = "false";
    };
  }
};

void
WqCheckpointReducer::AddSeqData(std::string seqnum, std::string seqdata)
{
  if (m_detectLinkFailure == true) 
  {
    std::map<std::string, std::string>::iterator d = m_detectFailureSeqData.find(seqnum);
    if (d == m_detectFailureSeqData.end()) {
      m_detectFailureSeqData.insert(std::pair<std::string, std::string>(seqnum, seqdata));
    }
    else {
      std::cout << m_prefix.toUri() << "Insert Duplicated SEQ_num to FailureSeqData list" << std::endl;
    };
    m_detectLinkFailure = false;
    for (auto& x: m_detectFailureSeqData) {
      std::cout << m_prefix.toUri() << "failure SEQ" << x.first << ": " << x.second << '\n';
    }
  };
};

void
WqCheckpointReducer::CheckFailSeq()
{
  std::map<std::string, std::string>::iterator checkFail;
  std::string seqlist = "";
  for(checkFail = m_detectFailureSeqData.begin(); checkFail != m_detectFailureSeqData.end(); checkFail++) 
  {
    if(m_detectFailureSeqData.size() == 1) {
      seqlist = checkFail->first;
    }
    else {
      seqlist = seqlist + checkFail->first + "/";
    };
  };
  m_sendDoubtNode = true;
  std::string failSeqInterest = "/0-/doubt-T"+ m_currentTreeFlag + seqlist + "-/id(" + m_prePathID + ")-";
  std::cout << m_prefix.toUri() + " !!! checkFailSeq " << failSeqInterest << std::endl;
  SendOutInterest(failSeqInterest);
};

void
WqCheckpointReducer::NotifyPathIdChange()
{
  for(uint64_t j=0; j<m_nodeList4Task.size(); j++)
  {
    std::map<std::string, std::string>::iterator local = m_neiLocalId.find(m_nodeList4Task[j]);
    if(local != m_neiLocalId.end()) 
    {
      std::map<std::string, std::string>::iterator update = m_nodePathId.find(m_nodeList4Task[j]);
      if(update != m_nodePathId.end()) {
        update->second = m_myPathID + "-" + local->second;
        // std::cout << m_prefix.toUri() << " update: " << m_nodeList4Task[j] << " with NEW-id= " << update->second << std::endl;
        std::string updatePathId = m_nodeList4Task[j] + "/TS" + m_currentTreeFlag + "/TE-" + "/updateId("
                          + update->second + ")-";
        SendOutInterest(updatePathId);
        std::cout << m_prefix.toUri() << " send UPDATE pathID: " << updatePathId << std::endl;
      }
      else {
        std::cout << m_prefix.toUri() << " CANNOT find PreviousPathId of: " << m_nodeList4Task[j] << std::endl;
      };
    }
    else {
      std::cout << m_prefix.toUri() << " CANNOT find LocalId of: " << m_nodeList4Task[j] << std::endl;
    };
  }
};

void
WqCheckpointReducer::LeaveJobTree()
{
  std::string tellLeave = m_selectNodeName + "/leave-/TS" + m_currentTreeFlag + "/TE-(" + m_prefix.toUri() + ")-";
  std::cout << m_prefix.toUri() << " Notify Leave-Tree:  " << m_selectNodeName << std::endl;
  SendOutInterest(tellLeave);
};

void 
WqCheckpointReducer::ReportFailure(std::string downNei, std::string seqNum)
{
  std::string lostDownNei = "/0-/downfail(" + downNei + seqNum + ")-" + m_prefix.toUri();
  std::cout << m_prefix.toUri() + " report-fail " << lostDownNei << std::endl;
  SendOutInterest(lostDownNei);
  std::map<std::string, std::string>::iterator downId = m_reportFailNeiList.find(downNei);
  if(downId != m_reportFailNeiList.end()) {
    downId->second = "true";
  }
  else{
    std::cout << m_prefix.toUri() + " CANNOT find fail downNei in the list " << std::endl;
  };
};

void 
WqCheckpointReducer::ClearHistorySaveData(std::string seqList)
{
  std::vector<int> slashPos;
  std::vector<std::string> seqContent;
  uint64_t pos = seqList.find("/"); 
  if (pos == std::string::npos) {
    seqContent.push_back(seqList);
  }
  else {
    while (pos != std::string::npos){
      slashPos.push_back(pos);
      pos = seqList.find("/", pos+1);
    }
  };
  if (slashPos.size() != 0) {
    seqContent.push_back(seqList.substr(0, slashPos[0]));
    for(uint64_t k=0; k<slashPos.size(); k++) {
      seqContent.push_back(seqList.substr(slashPos[k]+1, slashPos[k+1]-slashPos[k]-1));
    }
    // for (auto x: seqContent) {
    //   std::cout << "each seq: " << x << std::endl;
    // };
  };

  for(uint64_t n=0; n<m_nodeList4Task.size(); n++)
  {
    std::string dataRecord = m_receiveNodeandData[m_nodeList4Task[n]];
    for(uint64_t q=0; q<seqContent.size(); q++)
    {
      uint64_t s1 = dataRecord.find(seqContent[q]);
      if(s1 != std::string::npos){
        uint64_t s2 = dataRecord.find_first_of(";");
        dataRecord = dataRecord.substr(s2+1);
        // std::cout << m_nodeList4Task[n] << " process " << seqContent[q] << " find-seq: " << dataRecord << std::endl;
      };
    };
  };

  // std::cout << m_prefix.toUri() << "Before------------- " << m_processedSeqData.size() << std::endl;
  for(uint64_t q=0; q<seqContent.size(); q++)
  {
    // std::string idtag = m_currentTreeFlag + "-" + seqContent[q];
    // m_allReceiveSeqData.erase(idtag);
    // std::cout << m_prefix.toUri() << " clear-Seq: " << seqContent[q] << " and its-Data= " << m_processedSeqData[seqContent[q]] << std::endl;
    m_processedSeqData.erase(seqContent[q]);
  };
  std::ofstream recording;
  recording.open("computeStateRecord.txt", std::ios_base::app);
  recording << Simulator::Now().GetSeconds() << '\t' << m_prefix.toUri() << '\t' << m_processedSeqData.size() << std::endl;
  recording.close();

  // Time writeTime = Simulator::Now();
  // std::ofstream recording;
  // recording.open("computeStateRecord.txt", std::ios_base::app);
  // recording << Simulator::Now().ToDouble(Time::S) << '\t' << m_prefix.toUri() << '\t' << m_allReceiveSeqData.size() << std::endl;
  // recording.close();
};

void 
WqCheckpointReducer::ForwardClearDataSignal(std::string clearMessage)
{
  for(uint64_t n=0; n<m_nodeList4Task.size(); n++)
  {
    std::string notifyClear = m_nodeList4Task[n] + clearMessage;
    SendOutInterest(notifyClear);
  };
};

void 
WqCheckpointReducer::ProcessNormalInterest(shared_ptr<const Interest> taskInterest)
{
  m_pendingInterestName = taskInterest->getName();
  // m_taskInterestName = m_pendingInterestName.toUri();
  uint64_t s1 = m_pendingInterestName.toUri().find("(");
  uint64_t s2 = m_pendingInterestName.toUri().find(")");
  std::string seqNum = m_pendingInterestName.toUri().substr(s1+1, s2-s1-1);
  std::string n = seqNum.substr(3);
  AddComputeGroup(n);
  m_doubtSeq = seqNum;
  uint64_t u1 = m_pendingInterestName.toUri().find("TS");
  uint64_t u2 = m_pendingInterestName.toUri().find("TE");
  std::string userId = m_pendingInterestName.toUri().substr(u1+2, u2-u1-3);
  m_assignTask = m_pendingInterestName.toUri().substr(u1-1, s2-u1+2);
  // std::cout << m_prefix.toUri() << " userID: " << userId << std::endl;

  std::string treeIdSeq = userId + "-" + seqNum;
  SaveSeqInterestName(treeIdSeq, m_pendingInterestName.toUri());
  std::map<std::string, int>::iterator checkSeq = m_seqDataSendNum.find(treeIdSeq);
  if(checkSeq == m_seqDataSendNum.end()) {
    m_seqDataSendNum.insert(std::pair<std::string, int>(treeIdSeq, 0));
  }
  int sendTaskNum = 0;
  std::map<std::string, std::string>::iterator fUser = m_jobRefMap.find(userId);

  if(fUser != m_jobRefMap.end())
  {
    // std::cout << m_prefix.toUri() << " TASK-neis: " << fUser->second << std::endl;

    // Notify upstream neis and clear local id records cause no downstream neis connected
    if(fUser->second == "0") 
    {
      LeaveJobTree();
      m_jobRefMap.erase(userId);
      m_neiLocalId.clear();
      m_nodePathId.clear();
      m_lostNeiIdRecords.clear();
    }
    else 
    {
      //the flag means down-nei-nodes change, job nodes list needs update
      if(jobNeiChangeFlag == true) {
        m_nodeList4Task.clear();
        sendTaskNum = 0;
      };
      if(m_nodeList4Task.size() == 0)
      {
        std::string taskRefNodes = fUser->second;
        ProcessTaskNeis(taskRefNodes);
        jobNeiChangeFlag = false;
      };

      for(uint64_t s=0; s<m_nodeList4Task.size(); s++)
      {
        //check link status before send task interest
        std::string checkNeiLink= m_prefix.toUri() + m_nodeList4Task[s] + m_treeTag;
        std::map<std::string, std::string>::iterator linkIter = m_neiReachable.find(checkNeiLink);
        if(linkIter != m_neiReachable.end()) 
        {
          if(linkIter->second == "true") 
          {
            std::map<std::string, std::string>::iterator checkNode = m_receiveNodeandData.find(m_nodeList4Task[s]);
            if (checkNode == m_receiveNodeandData.end())
            {
              m_receiveNodeandData.insert(std::pair<std::string, std::string>(m_nodeList4Task[s], ""));
            };

            std::string creatTask = m_nodeList4Task[s] + m_assignTask + "-";
            std::cout << m_prefix.toUri() << " creat: " << creatTask << std::endl; 
            shared_ptr<Name> mapTaskName = make_shared<Name>(creatTask);
            mapTaskName->appendSequenceNumber(m_rand->GetValue(0, std::numeric_limits<uint16_t>::max()));
            shared_ptr<Interest> mapTaskInterest = make_shared<Interest>();
            mapTaskInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
            mapTaskInterest->setName(*mapTaskName);
            mapTaskInterest->setInterestLifetime(ndn::time::seconds(20));
            //std::cout << "111 to sensor " << ": " << *subInterest << std::endl;
            m_transmittedInterests(mapTaskInterest, this, m_face);
            m_appLink->onReceiveInterest(*mapTaskInterest);
            sendTaskNum++;
            creatTask.clear();
          }
          else if((linkIter->second == "false")) 
          {
            std::map<std::string, std::string>::iterator downId = m_reportFailNeiList.find(m_nodeList4Task[s]);
            if(downId == m_reportFailNeiList.end()) {
              m_reportFailNeiList.insert(std::pair<std::string, std::string>(m_nodeList4Task[s],"false"));
            };
            downId = m_reportFailNeiList.find(m_nodeList4Task[s]);
            if(downId->second == "false") {
              std::cout << m_prefix.toUri() << " has nei link false = " << m_nodeList4Task[s] << std::endl;
              m_cpFailure = true;
              // ReportFailure(m_nodeList4Task[s], seqNum);
            }; 
          }
        }
      };
      m_seqDataSendNum.at(treeIdSeq) = sendTaskNum;
      sendTaskNum = 0;
      // for (auto& x: m_seqDataSendNum) {
      //   std::cout << m_prefix.toUri() << "send SEQ" << x.first << ": " << x.second << '\n';
      // };
    };      
  }
  else {
    //no job reference for current user
    std::cout << m_prefix.toUri() << "Return data to user: deploy failed, start discovery process"<<std::endl;
  };

};

void
WqCheckpointReducer::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside
  NS_LOG_FUNCTION(this << interest);

  if (!m_active)
  { return; }

  m_pendingInterestName = interest->getName();
  // std::cout << m_prefix.toUri() << " receive interest: " << m_pendingInterestName.toUri() << std::endl;
  uint64_t p = m_pendingInterestName.toUri().find("/discover");
  uint64_t r = m_pendingInterestName.toUri().find("/rejoin");
  uint64_t d = m_pendingInterestName.toUri().find("/doubt");
  uint64_t pro = m_pendingInterestName.toUri().find("/process");
  uint64_t c = m_pendingInterestName.toUri().find("Cancel");
  uint64_t leave = m_pendingInterestName.toUri().find("leave");
  uint64_t clear = m_pendingInterestName.toUri().find("clear");
  uint64_t backTree = m_pendingInterestName.toUri().find("backTree");
  uint64_t upNeiFail = m_pendingInterestName.toUri().find("Upfail");
  uint64_t cp = m_pendingInterestName.toUri().find("cp");
  uint64_t recover = m_pendingInterestName.toUri().find("recover");
  uint64_t newUpNei = m_pendingInterestName.toUri().find("newUp");
  uint64_t first_job = m_pendingInterestName.toUri().find("child");
  uint64_t rollback = m_pendingInterestName.toUri().find("rollback");
  
  //get current userId
  uint64_t t1 = m_pendingInterestName.toUri().find("TS");
  if(t1 != std::string::npos)
  {
    uint64_t t2 = m_pendingInterestName.toUri().find("TE");
    m_treeTag = m_pendingInterestName.toUri().substr(t1+2, t2-t1-3);
    // std::cout << "Reducer " << m_prefix.toUri() << " rececive tree tag: "<< m_treeTag <<std::endl;
  }
  
  // Interest for build tree
  if(p != std::string::npos) 
  {
    //check if discovery procedure already done for current userId
    std::map<std::string, std::string>::iterator userIdIter = m_jobRefMap.find(m_treeTag);
    if(userIdIter != m_jobRefMap.end())
    {
      //discovery done
      std::cout <<"Report discovery done"<<std::endl;
    }
    else 
    {
      if (m_currentTreeFlag == "000") 
      { 
        m_currentTreeFlag = m_treeTag;
      }

      if (m_currentTreeFlag == m_treeTag) 
      {
        //check if get upstream node from fib
        std::map<std::string, std::string>::iterator fibIter = m_fibResult.find(m_treeTag);
        //not found
        if(fibIter == m_fibResult.end())
        {
          m_fibResult.insert(std::pair<std::string, std::string>(m_treeTag,"0"));
          m_buildTreeInterestMap.insert(std::pair<std::string, std::string>(m_pendingInterestName.toUri(),"0"));
          //send Interest to ask FIB nexthop face
          m_askFibPrefix = "/f-";
          std::string tempAskFib = m_askFibPrefix + m_treeTag;
          // std::cout << "Reducer " << m_prefix.toUri() << " tempAskFib: "<< tempAskFib <<std::endl;
          shared_ptr<Name> askFibName = make_shared<Name>(tempAskFib);
          shared_ptr<Interest> askFibInterest = make_shared<Interest>();
          askFibInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
          askFibInterest->setName(*askFibName);
          askFibInterest->setInterestLifetime(ndn::time::seconds(1));
          m_transmittedInterests(askFibInterest, this, m_face);
          m_appLink->onReceiveInterest(*askFibInterest);
          //std::cout<< "go for fib round" <<std::endl;
        }
        //get fib info already
        else
        {
          m_askPitPrefix = "/p-";
          m_buildTreeInterestMap.insert(std::pair<std::string, std::string>(m_pendingInterestName.toUri(),"0"));
          std::string tempAskPit = m_askPitPrefix + m_pendingInterestName.toUri();
          //std::cout << m_prefix.toUri() <<"tempAskPit: "<< tempAskPit <<std::endl;
          shared_ptr<Name> askPitName = make_shared<Name>(tempAskPit);
          //subBtInterest->appendSequenceNumber(m_rand->GetValue(0, std::numeric_limits<uint16_t>::max()));
          shared_ptr<Interest> askPitInterest = make_shared<Interest>();
          askPitInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
          askPitInterest->setName(*askPitName);
          askPitInterest->setInterestLifetime(ndn::time::seconds(1));
          m_transmittedInterests(askPitInterest, this, m_face);
          m_appLink->onReceiveInterest(*askPitInterest);
          m_askPitNum++;
        }
      }
      else
      {
        //other tree is processing, current tree build should wait
        m_pendingTreeTagList.push_back(m_treeTag);
        //store pending interest and waiting userID
        m_pendTreeJobMap.insert(std::pair<std::string, std::string>(m_pendingInterestName.toUri(),m_treeTag));
      }
    }      
  }  
  // rejoin tree Interest due to link failure
  else if (r != std::string::npos)
  {
    // std::cout << "Reducer " << m_prefix.toUri() << " rececive rejoin-tree tag: "<< m_treeTag <<std::endl;
    m_currentTreeFlag = m_treeTag;
    ProcessRejoinInterest();
  }
  //notify from neis to cancel previous join request
  else if(c != std::string::npos)
  {
    std::cout << m_prefix.toUri() << " rececive: "<<  m_pendingInterestName.toUri() <<std::endl;
    uint64_t j1 = m_pendingInterestName.toUri().find("(");
    uint64_t j2 = m_pendingInterestName.toUri().find(")");
    std::string cancelNei = m_pendingInterestName.toUri().substr(j1+1, j2-j1-1);
    std::string cancelLink = m_prefix.toUri() + cancelNei + m_currentTreeFlag;
    // std::cout << m_prefix.toUri() <<" cancel nei " << cancelLink <<std::endl;
    m_neiReachable.erase(cancelLink);
    jobNeiChangeFlag=true;
    m_neiLocalId.erase(cancelNei);
    m_nodePathId.erase(cancelNei);
    ReplyData("ok", m_pendingInterestName.toUri());
  }
  // user ask if seq-data has been processed
  else if (d != std::string::npos)
  {
    std::cout << m_prefix.toUri() <<" get check-seq Interest: " << m_pendingInterestName.toUri() <<std::endl;
    uint64_t s1 = interest->getName().toUri().find("Seq");
    uint64_t s2 = interest->getName().toUri().find("/id");
    uint64_t p1 = interest->getName().toUri().find("(");
    uint64_t p2 = interest->getName().toUri().find(")");
    uint64_t t = interest->getName().toUri().find("T");
    std::string treeNum = interest->getName().toUri().substr(t+1, s1-t-1);
    std::string doubtSeq = interest->getName().toUri().substr(s1, s2-s1-1);
    std::string doubtNodeId = interest->getName().toUri().substr(p1+1, p2-p1-1);
    // std::cout << m_prefix.toUri() << " receive doubt seq= " << doubtSeq << " neiID= " << doubtNodeId << " treeId= " << treeNum << std::endl;

    std::map<std::string, std::string>::iterator findId;
    bool check = false;
    for(findId=m_nodePathId.begin(); findId!=m_nodePathId.end(); findId++)
    {
      if(findId->second == doubtNodeId)
      {
        check = true;
        std::string checkResult = "";
        std::string doubtNodeName = findId->first;
        // std::cout << m_prefix.toUri() << " find doubt node= " << doubtNodeName << std::endl;
        std::map<std::string, std::string>::iterator allSeq = m_receiveNodeandData.find(doubtNodeName);
        if(allSeq != m_receiveNodeandData.end())
        {
          std::string seqdata = allSeq->second;
          //use m_lostNei may help to re-assign local ID to other new-join nodes
          m_lostNei = doubtNodeName;
          // std::cout << m_prefix.toUri() << " retrive doubt seqdata= " << seqdata << std::endl;

          if((doubtSeq.length()>8) && (doubtSeq.find("/"))) 
          {
            std::cout << m_prefix.toUri() << " multiple  SEQ " << std::endl;
            std::vector<std::string> seqRecord;
            std::deque<int> slashList;
            for(uint64_t k=0; k<doubtSeq.size(); k++)
            { 
              if(doubtSeq[k] == '/') {
                slashList.push_back(k);
              };
            };
            std::string seq = doubtSeq.substr(0, slashList[0]);
            seqRecord.push_back(seq);
            for(uint64_t n=0; n<slashList.size()-1; n++)
            {
              seq =  doubtSeq.substr(slashList[n]+1, slashList[n+1]-slashList[n]-1);
              // std::cout << m_prefix.toUri() << " seq= " << seq << std::endl;
              seqRecord.push_back(seq);
            };

            std::vector<std::string> seqCheckResult;
            for(uint64_t q=0; q<seqRecord.size(); q++) 
            {
              // std::cout << m_prefix.toUri() << "seq-Record: " << seqRecord[q] << std::endl;
              uint64_t f = seqdata.find(seqRecord[q]);
              if(f == std::string::npos) {
                seqCheckResult.push_back("No");
              }
              else{
                seqCheckResult.push_back("Yes");
              };
            };

            int count = 0;      
            if(std::find(seqCheckResult.begin(), seqCheckResult.end(), "Yes") != seqCheckResult.end()) {
              checkResult = "Not-receive==";
              for(uint64_t l=0; l<seqCheckResult.size(); l++) 
              {
                if(seqCheckResult[l] == "No") {
                  checkResult = checkResult + "/" + seqRecord[l];
                }
                else if(seqCheckResult[l] == "Yes") {
                  count++;
                };

                if(count == seqCheckResult.size()) {
                  checkResult = "Already-receive";
                };
              };
            }
            else {
              checkResult = "Not-receive";
            }; 
          }
          else 
          {
            uint64_t findDoubt = seqdata.find(doubtSeq);
            if(findDoubt == std::string::npos)
            {
              checkResult = "Not-receive";
            }
            else{
              checkResult = "Already-receive";
              // std::cout << m_prefix.toUri() << " already RECEIVE doubt-seqdata " << std::endl;
            };
          };
          //update jobNeis as received pre-nei disconnect-doubt check
          std::map<std::string, std::string>::iterator checkTree = m_jobRefMap.find(treeNum);
          if (checkTree != m_jobRefMap.end()) 
          {
            std::string::size_type dn = checkTree->second.find(m_lostNei);
            std::cout << m_prefix.toUri() << " before delete disconnect-nei = " << checkTree->second << " find= " << dn << std::endl;
            if (dn != std::string::npos) 
            {
              checkTree->second.erase(dn, m_lostNei.length());
              std::cout << m_prefix.toUri() << " after delete disconnect-nei = " << checkTree->second << std::endl;
            }
            jobNeiChangeFlag = true;
            if(checkTree->second != "0") {
              // std::cout << m_prefix.toUri() <<"33333333333 here= " <<std::endl;
              ProcessTaskNeis(checkTree->second);
            }
            else {
              m_nodeList4Task.clear();
              LeaveJobTree();
              // std::cout << m_prefix.toUri() << " has noDownstreamNei so Notify->" << m_selectNodeName << std::endl;
            };
          };
        }
        else {
          checkResult = "Not-receive";
          std::cout << m_prefix.toUri() << " has NO Nei= "<< doubtNodeName << std::endl;
        };
        ReplyData(checkResult, interest->getName().toUri());
      };
    };
    //path-based ID is not found at current node, need continue forward the Interest
    if(check == false) 
    {
      m_forwardDoubtCheck = interest->getName().toUri();
      uint64_t h1 = interest->getName().toUri().find("hop");
      uint64_t h2 = interest->getName().toUri().find_last_of("-");
      std::string hopNum = interest->getName().toUri().substr(h1+3, h2-h1-3);
      int control = std::stoi(hopNum) + 1;
      // int control = 4;
      std::string myhopNum = std::to_string(control);
      std::string subId = doubtNodeId;
      for(int j=1; j<control; j++)
      {
        uint64_t c = subId.find_first_of("-");
        subId = subId.substr(c+1);
        // std::cout <<  "j= " << j << " /// sub_id " << subId << std::endl;
      };
      std::string neiId = subId.substr(0,1);
      bool search = false;
      std::map<std::string, std::string>::iterator searchid;
      for(searchid=m_neiLocalId.begin(); searchid!=m_neiLocalId.end(); searchid++)
      {
        if(searchid->second == neiId)
        {
          search = true;
          std::string forwardNeiName = searchid->first;
          // std::cout << " hop Num= " << hopNum << " myHop= " << myhopNum << std::endl;
          std::string forwardCheck = interest->getName().toUri().substr(d, h1);
          forwardCheck = forwardNeiName + forwardCheck + myhopNum + "-";
          std::cout << m_prefix.toUri() << " forwardCheck Interest: " << forwardCheck << std::endl;
          SendOutInterest(forwardCheck);
        }
      };
      if(search == false)
      {
        std::cout <<  m_prefix.toUri() << " NO neiId= " << neiId << std::endl;
      };
    };
  }
  // user asks to process data ignoring disconnect downneis
  else if (pro != std::string::npos)
  {
    std::cout << m_prefix.toUri() <<" get process-request: " << m_pendingInterestName.toUri() <<std::endl;
    uint64_t s1 = m_pendingInterestName.toUri().find("Seq");
    uint64_t s2 = m_pendingInterestName.toUri().find("/except");
    uint64_t p1 = m_pendingInterestName.toUri().find("(");
    uint64_t p2 = m_pendingInterestName.toUri().find(")");
    std::string proSeq = m_pendingInterestName.toUri().substr(s1, s2-s1-1);
    std::string ignoreNodeId = m_pendingInterestName.toUri().substr(p1+1, p2-p1-1);
    // std::cout << " process-seq: " << proSeq << " ignoreNodeId= " << ignoreNodeId <<std::endl;
    std::map<std::string, std::string>::iterator findNode;
    bool check = false;
    std::string ignoreNode;
    for(findNode=m_nodePathId.begin(); findNode!=m_nodePathId.end(); findNode++)
    {
      if(findNode->second == ignoreNodeId) {
        check = true;
        ignoreNode = findNode->first;
      }
    };

    if(check == true) 
    {
      if((proSeq.length()>8) && (proSeq.find("/"))) 
      {
        std::vector<std::string> seqRecord;
        std::deque<int> slashList;
        for(uint64_t k=0; k<proSeq.size(); k++)
        { 
          if(proSeq[k] == '/') {
            slashList.push_back(k);
          };
        };
        std::string seq = proSeq.substr(0, slashList[0]);
        seqRecord.push_back(seq);
        for(uint64_t n=0; n<slashList.size()-1; n++)
        {
          seq =  proSeq.substr(slashList[n]+1, slashList[n+1]-slashList[n]-1);
          // std::cout << m_prefix.toUri() << " seq= " << seq << std::endl;
          seqRecord.push_back(seq);
        };

        if (m_lostNei == ignoreNode) {
          AddLostNeiId(ignoreNode);
          m_lostNei = "";
        };

        int seqcount = 0;
        int seqfail = 0;
        std::string seqNoExist = "";
        for(uint64_t q=0; q<seqRecord.size(); q++)
        {
          std::string eachSeq = m_treeTag + "-" + seqRecord[q];
          std::map<std::string, int>::iterator checkSeq = m_seqDataSendNum.find(eachSeq);
          if(checkSeq != m_seqDataSendNum.end()) 
          {
            int n = m_seqDataSendNum[eachSeq];
            std::map<std::string, int>::iterator gotSeq = m_seqDataGotNum.find(eachSeq);
            if(gotSeq != m_seqDataGotNum.end()) 
            {
              int m = m_seqDataGotNum[eachSeq];
              if (n-1 == m) {
                m_seqDataSendNum[eachSeq] = m;
                ProcessDataBySeq(eachSeq);
                std::cout << m_prefix.toUri() << " Process and return by ignore Disconnect Nei: " << ignoreNode << std::endl;
              }
              else {
                //ignore current lost_nei, but n-1 still not m, maybe because have other lost neis
                m_seqDataSendNum[eachSeq] = n-1;
              };
            }
            else if ((gotSeq == m_seqDataGotNum.end()) && (n==1)) {
              seqcount++;
            };
          }
          else {
            seqfail++;
            seqNoExist += seqRecord[q];
          };
        };

        if(seqcount == seqRecord.size()) {
          std::string replyPro = "No data to process";
          ReplyData(replyPro, m_pendingInterestName.toUri());
        };
        if(seqfail != 0) {
          std::string replyPro = "Seq error= " + seqNoExist;
          ReplyData(replyPro, m_pendingInterestName.toUri());
        };
      }
      else
      {
        proSeq = m_treeTag + "-" + proSeq;
        std::map<std::string, int>::iterator checkSeq = m_seqDataSendNum.find(proSeq);
        if(checkSeq != m_seqDataSendNum.end()) 
        {
          int n = m_seqDataSendNum[proSeq];
          std::map<std::string, int>::iterator gotSeq = m_seqDataGotNum.find(proSeq);
          if (gotSeq != m_seqDataGotNum.end()) {
            int n1 = m_seqDataGotNum[proSeq];
            if (n-1 == n1)
            {
              if (m_lostNei == ignoreNode)
              {
                m_seqDataSendNum[proSeq] = n1;
                ProcessDataBySeq(proSeq);
                std::cout << m_prefix.toUri() << " Process and return by ignore Disconnect Nei: " << ignoreNode << std::endl;
                AddLostNeiId(ignoreNode);
                m_lostNei = "";
              }
              else
              {
                std::string replyPro = "Seq error";
                ReplyData(replyPro, m_pendingInterestName.toUri());
              };
              // std::cout << " Ingore node = " << ignoreNode << " lostnei = " << m_lostNei <<std::endl;
            }
            else 
            {
              std::string replyPro = "Seq error";
              ReplyData(replyPro, m_pendingInterestName.toUri());
            };
          }
          else
          {
            // std::cout << m_prefix.toUri() <<" No SeqData Exist " <<std::endl;
            ReplyData("No SeqData", m_pendingInterestName.toUri());
          };
        }
        else {
          std::string replyPro = "Seq error";
          ReplyData(replyPro, m_pendingInterestName.toUri());
        };
      };
    }
    else 
    {
      uint64_t h1 = interest->getName().toUri().find("hop");
      uint64_t h2 = interest->getName().toUri().find_last_of("-");
      std::string hopNum = interest->getName().toUri().substr(h1+3, h2-h1-3);
      int control = std::stoi(hopNum) + 1;
      std::string myhopNum = std::to_string(control);
      std::string subId = ignoreNodeId;
      for(int j=1; j<control; j++)
      {
        uint64_t c = subId.find_first_of("-");
        subId = subId.substr(c+1);
        // std::cout <<  "j= " << j << " /// sub_id " << subId << std::endl;
      };
      std::string neiId = subId.substr(0,1);
      bool search = false;
      std::map<std::string, std::string>::iterator searchid;
      for(searchid=m_neiLocalId.begin(); searchid!=m_neiLocalId.end(); searchid++)
      {
        if(searchid->second == neiId)
        {
          m_forwardSeqProcess = interest->getName().toUri();
          search = true;
          std::string forwardNeiName = searchid->first;
          // std::cout << " hop Num= " << hopNum << " myHop= " << myhopNum << std::endl;
          std::string forwardProcess = interest->getName().toUri().substr(pro, h1);
          forwardProcess = forwardNeiName + forwardProcess + myhopNum + "-";
          std::cout << m_prefix.toUri() << " forwardIgnoreProcess Interest: " << forwardProcess << std::endl;
          SendOutInterest(forwardProcess);
        }
      };
      if(search == false)
      {
        std::cout <<  m_prefix.toUri() << " NO neiId= " << neiId << std::endl;
      };

    };
  }
  // receive leave tree notification from downstream neis
  else if (leave != std::string::npos)
  {
    uint64_t s1 = m_pendingInterestName.toUri().find("(");
    uint64_t s2 = m_pendingInterestName.toUri().find(")");
    std::string leaveNode = m_pendingInterestName.toUri().substr(s1+1, s2-s1-1);
    uint64_t a1 = m_pendingInterestName.toUri().find("TS");
    uint64_t a2 = m_pendingInterestName.toUri().find("TE");
    std::string treeId = m_pendingInterestName.toUri().substr(a1+2, a2-a1-3);
    std::cout << m_prefix.toUri() << " Update Nei-list as receive Leave-Tree from Node: " << leaveNode << std::endl;
    std::map<std::string, std::string>::iterator jobneis = m_jobRefMap.find(treeId);
    if(jobneis != m_jobRefMap.end())
    {
      std::string::size_type dn = jobneis->second.find(leaveNode);
      // std::cout << m_prefix.toUri() << " before delete disconnect-nei = " << jobneis->second << " find= " << dn << std::endl;
      if (dn != std::string::npos) {
        jobneis->second.erase(dn, leaveNode.length());
        // std::cout << m_prefix.toUri() << " after delete disconnect-nei = " << jobneis->second << std::endl;
      };
    };
    jobNeiChangeFlag = true;
  }
  // user notify to clear history process-ok data
  else if (clear != std::string::npos)
  {
    std::cout << m_prefix.toUri() << " receive clear History-Seq " << std::endl;
    uint64_t s1 = m_pendingInterestName.toUri().find("Seq");
    uint64_t l = m_pendingInterestName.toUri().find_last_of("/");
    std::string seqs = m_pendingInterestName.toUri().substr(s1, l-s1-2);
    // std::cout << m_prefix.toUri() << " clear Seq= " << seqs << std::endl;
    ClearHistorySaveData(seqs);
    ReplyData("Clear-Done", m_pendingInterestName.toUri());
    std::string clearInfo = m_pendingInterestName.toUri().substr(clear-1, l-clear) + "-";
    ForwardClearDataSignal(clearInfo);
  }
  // receive back-tree request from previous downstream neighbours
  else if (backTree != std::string::npos)
  {
    std::cout << m_prefix.toUri() << " receive back-Tree request, currentTreeFlag= " << m_currentTreeFlag << std::endl;
    if(m_jobRefMap.size() == 0) 
    {
      uint64_t a1 = m_pendingInterestName.toUri().find("TS");
      uint64_t a2 = m_pendingInterestName.toUri().find("TE");
      std::string treeId = m_pendingInterestName.toUri().substr(a1+2, a2-a1-3);
      m_askRejoinNeiName = interest->getName().toUri().substr(backTree+9, a1-backTree-10);
      if(m_currentTreeFlag == treeId)
      {
        std::string addNeis = "0" + m_askRejoinNeiName;
        m_jobRefMap.insert(std::pair<std::string, std::string>(m_currentTreeFlag, addNeis));
        std::string reconnect_upstream = m_selectNodeName + "/backTree-" + m_prefix.toUri() + "/TS" + m_currentTreeFlag + "/TE-";
        SendOutInterest(reconnect_upstream);
      };
    }
    else {
      ProcessRejoinInterest();
    };
  }
  // Up-nei has link failure
  else if(upNeiFail != std::string::npos)
  {
    std::cout << m_prefix.toUri() << " receive  " << m_pendingInterestName.toUri() << std::endl;
    m_interestOfUpfail = m_pendingInterestName.toUri();
    uint64_t a1 = m_pendingInterestName.toUri().find("TS");
    uint64_t a2 = m_pendingInterestName.toUri().find("TE");
    std::string treeId = m_pendingInterestName.toUri().substr(a1+2, a2-a1-3);
    std::string cancelUpLink = m_prefix.toUri() + m_selectNodeName + treeId;
    std::cout << m_prefix.toUri() << " pre-link=  " << cancelUpLink << std::endl;
    m_sendRejoinNode = m_prefix.toUri();
    m_upNodeFail=true;
    RejoinTreeDueToUpNeiFail(cancelUpLink);
  }
  // to checkpoint
  else if (cp != std::string::npos)
  {
    uint64_t j = m_pendingInterestName.toUri().find("Com");
    if(j != std::string::npos) {
      std::cout << m_prefix.toUri() << " receive Checkpoint-ComputeNodeInfo " << std::endl;
      std::string replyContent = m_prefix.toUri() + "&Reducer";
      ReplyData(replyContent, m_pendingInterestName.toUri());
    }
    else {
      std::cout << m_prefix.toUri() << " receive Checkpoint-msg " << std::endl;
      std::string replyContent = "";
      if(m_cpFailure){
        replyContent = m_prefix.toUri() + "&Fail";
      }
      else{
        replyContent = m_prefix.toUri() + "&OK";
      };
      ReplyData(replyContent, m_pendingInterestName.toUri());
    }
  }
  // to act as a recover reducer by sink
  else if (recover != std::string::npos)
  {
    m_currentTreeFlag = m_treeTag;
    // std::cout << m_prefix.toUri() <<" m_currentTreeFlag " << m_currentTreeFlag <<std::endl;
    std::cout << m_prefix.toUri() << " ---- receive Recover-reducer-task " << std::endl;
    uint64_t c1 = m_pendingInterestName.toUri().find("<");
    uint64_t c2 = m_pendingInterestName.toUri().find(">");
    m_jobRefNei = m_pendingInterestName.toUri().substr(c1+1, c2-c1-1);
    // std::cout << m_prefix.toUri() <<" childs: " << m_jobRefNei <<std::endl;
    ProcessTaskNeis(m_jobRefNei);
    CreateJobNeiList();
    ReplyData(" OK-As-Recover-Reducer", m_pendingInterestName.toUri());
  }
  // rollback notification, need clear previous records to restart
  else if (rollback != std::string::npos)
  {
    m_seqDataSendNum.clear();
    m_seqDataGotNum.clear();
    m_allReceiveSeqData.clear();
    m_seqInterestName.clear();
    m_groupIds.clear();
    m_computeGroup.clear();
    m_processOkSeq.clear();
    m_countdata=0;
    m_countSeq.clear();
    m_receiveNodeandData.clear();
    ReplyData("rollback-OK", m_pendingInterestName.toUri());
  }
  // receive msg to change upstrem nei
  else if(newUpNei != std::string::npos)
  {
    uint64_t u1 = interest->getName().toUri().find("(");
    uint64_t u2 = interest->getName().toUri().find(")");
    std::string changeUp = interest->getName().toUri().substr(u1+1, u2-u1-1);
    std::cout << m_prefix.toUri() << " change Upstreame-Nei to: " << changeUp << std::endl;
    m_preUpNodeName = m_selectNodeName;
    m_selectNodeName = changeUp;
    std::string removeLink = m_prefix.toUri() + m_preUpNodeName + m_currentTreeFlag;
    std::string changeLink = m_prefix.toUri() + m_selectNodeName + m_currentTreeFlag;
    // std::cout << m_prefix.toUri() << " Previous-link:" << removeLink << std::endl;
    // std::cout << m_prefix.toUri() << " Change-link:" << changeLink << std::endl;
    std::map<std::string, std::string>::iterator find1, find2;
    find1 = m_neiReachable.find(removeLink);
    find2 = m_neiReachable.find(changeLink);
    if(find1 != m_neiReachable.end()) 
    {
      m_neiReachable.erase(find1);
    }
    else {
      std::cout << m_prefix.toUri() << " Previous-Upstream link Not-exist" << std::endl;
    };
    if(find2 != m_neiReachable.end()) 
    {
      find2->second = "true";
    }
    else {
      m_neiReachable.insert(std::pair<std::string, std::string>(changeLink, "true"));
    };
    ReplyData("OK", interest->getName().toUri());
  }
  // first-normal-Interest, has child-info to be parsed
  else if(first_job != std::string::npos) {
    m_currentTreeFlag = m_treeTag;
    std::cout << m_prefix.toUri() <<" get normal Interest: " << m_pendingInterestName.toUri() <<std::endl;
    uint64_t c1 = m_pendingInterestName.toUri().find("<");
    uint64_t c2 = m_pendingInterestName.toUri().find(">");
    m_jobRefNei = m_pendingInterestName.toUri().substr(c1+1, c2-c1-1);
    std::cout << m_prefix.toUri() <<" childs: " << m_jobRefNei <<std::endl;
    ProcessTaskNeis(m_jobRefNei);
    CreateJobNeiList();
    ProcessNormalInterest(interest);
  }
  //normal Interest
  else 
  { 
    //for checkpoint every seq-20
    Simulator::Schedule(Seconds(160), &WqCheckpointReducer::LinkBroken, this, "/14-", "/32-");
    Simulator::Schedule(Seconds(43), &WqCheckpointReducer::LinkBroken, this, "/17-", "/48-");
    Simulator::Schedule(Seconds(85), &WqCheckpointReducer::LinkBroken, this, "/28-", "/55-");
    Simulator::Schedule(Seconds(120), &WqCheckpointReducer::LinkBroken, this, "/26-", "/80-");
    Simulator::Schedule(Seconds(13), &WqCheckpointReducer::LinkBroken, this, "/24-", "/95-");
    //for checkpoint every seq5
    // Simulator::Schedule(Seconds(13), &WqCheckpointReducer::LinkBroken, this, "/14-", "/32-");
    // Simulator::Schedule(Seconds(37), &WqCheckpointReducer::LinkBroken, this, "/17-", "/48-");
    // Simulator::Schedule(Seconds(52), &WqCheckpointReducer::LinkBroken, this, "/28-", "/55-");
    // Simulator::Schedule(Seconds(67), &WqCheckpointReducer::LinkBroken, this, "/26-", "/80-");
    // Simulator::Schedule(Seconds(88), &WqCheckpointReducer::LinkBroken, this, "/24-", "/95-");
    //ECE-topo-2
    // Simulator::Schedule(Seconds(32), &WqCheckpointReducer::LinkBroken, this, "/5-", "/m3-");
    // Simulator::Schedule(Seconds(63), &WqCheckpointReducer::LinkBroken, this, "/1-", "/m5-");
    std::cout << m_prefix.toUri() <<" get normal Interest: " << m_pendingInterestName.toUri() <<std::endl;
    m_normalInterest = interest;
    ProcessNormalInterest(m_normalInterest);
    // std::string requestPit = "/p-" + m_pendingInterestName.toUri();
    // SendOutInterest(requestPit);
  }
};

void
WqCheckpointReducer::OnData(shared_ptr<const Data> data)
{

	if (!m_active)
    return;

	App::OnData(data); // tracing inside

  //parse data content
    auto *tmpContent = ((uint8_t*)data->getContent().value());
    std::string receivedData;
    for(uint8_t i=0; i < data->getContent().value_size(); i++) {
      receivedData.push_back((char)tmpContent[i]);
    };
    
    std::string gotData = data->getName().toUri();
    uint64_t findDis = gotData.find("/discover");
    uint64_t findDis2 = gotData.find("/func");
    uint64_t findRejoin = gotData.find("rejoin");
    uint64_t findCancel = gotData.find("Cancel");
    uint64_t u = gotData.find("update");
    uint64_t doubt = gotData.find("doubt");
    uint64_t process = gotData.find("process");
    uint64_t resend = gotData.find("resend");
    uint64_t leave = gotData.find("leave");
    uint64_t reconnect = gotData.find("backTree");
    uint64_t upNeiFail = gotData.find("Upfail");
    uint64_t downNeiFail = gotData.find("downfail");
    uint64_t newUp = gotData.find("newUp");

    if(findDis != std::string::npos && gotData[1] != 'p')
    {
      m_gotDisDownNeiNum++;
      std::cout << m_prefix.toUri() <<" get Data: " << receivedData << " from " << data->getName().toUri() << std::endl;
      std::string uriData = data->getName().toUri();
      uint64_t fu = uriData.find_first_of("-");
      std::string receDownNode = uriData.substr(0,fu+1);
      //std::cout << m_prefix.toUri() <<" get Data from " << receDownNode << " content is "<< receivedData << std::endl;
      //std::cout << m_prefix.toUri() << m_gotDisDownNeiNum << " and " << m_sendDisDownNeiNum << std::endl;
      std::map<std::string, std::string>::iterator disDownIt;
      for(disDownIt =m_disDownNodeMap.begin(); disDownIt!=m_disDownNodeMap.end(); ++disDownIt)
      {
        //std::cout << m_prefix.toUri() << "iterator first " << disDownIt->first << std::endl;
        if(disDownIt->first == receDownNode)
        {
          disDownIt->second = receivedData;
        }
      };
      if(m_gotDisDownNeiNum == m_sendDisDownNeiNum)
      {
        for(disDownIt =m_disDownNodeMap.begin(); disDownIt!=m_disDownNodeMap.end(); ++disDownIt)
        {
          if(disDownIt->second == "yes")
          {
            std::string downNei = m_prefix.toUri() + disDownIt->first + m_treeTag;
            m_neiReachable.insert(std::pair<std::string, std::string>(downNei, "true"));
            m_jobRefNei += disDownIt->first;
            std::cout << m_prefix.toUri() <<" jobRef nei " << m_jobRefNei << std::endl;
          }
        };
        //reply for upstream node
        Name replyYesDataName(m_selectUpstream);
        auto replyYesData = make_shared<Data>();
        replyYesData->setName(replyYesDataName);
        replyYesData->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
        std::string rawData;

        SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));
        if (m_keyLocator.size() > 0) {
          signatureInfo.setKeyLocator(m_keyLocator);
        }
        replyYesData->setSignatureInfo(signatureInfo);
        ::ndn::EncodingEstimator estimator;
        ::ndn::EncodingBuffer encoder(estimator.appendVarNumber(m_signature), 0);
        encoder.appendVarNumber(m_signature);
        replyYesData->setSignatureValue(encoder.getBuffer());
        NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << replyYesData->getName());
        
        if(m_jobRefNei != "0")
        {
          // store the downstrem nodes for current task
          m_jobRefMap.insert(std::pair<std::string, std::string>(m_currentTreeFlag,m_jobRefNei));
          
          rawData ="yes";
          const uint8_t* p = reinterpret_cast<const uint8_t*>(rawData.c_str());
          auto buffer = make_shared< ::ndn::Buffer>(p, rawData.size());
          replyYesData->setContent(buffer);
          replyYesData->wireEncode();
          m_transmittedDatas(replyYesData, this, m_face);
          m_appLink->onReceiveData(*replyYesData);
          std::cout << m_prefix.toUri() << " reply " << rawData << " for " << m_selectUpstream << std::endl;
        }
        else 
        {
          rawData = "nope";
          const uint8_t* p = reinterpret_cast<const uint8_t*>(rawData.c_str());
          auto buffer = make_shared< ::ndn::Buffer>(p, rawData.size());
          replyYesData->setContent(buffer);
          replyYesData->wireEncode();
          m_transmittedDatas(replyYesData, this, m_face);
          m_appLink->onReceiveData(*replyYesData);
          std::cout << m_prefix.toUri() << " reply NOPE for discover Interest: " << rawData << std::endl;
        }

        m_gotDisDownNeiNum = m_sendDisDownNeiNum =0;
        // m_currentTreeFlag = "";
        m_treeTag = "none";
        m_disDownNodeMap.clear();
      }
    }
    //data for rejoin tree request
    else if((findRejoin != std::string::npos) && (gotData[1] != 'f')) 
    {
      std::cout << m_prefix.toUri() <<" get Data: " << receivedData << " from " << data->getName().toUri() << std::endl;
      std::string uriData = data->getName().toUri();
      uint64_t fu = uriData.find_first_of("-");
      std::string rejoinNeiNode = uriData.substr(0,fu+1);

      // std::cout << m_prefix.toUri() <<" m_sendRejoinNode= " << m_sendRejoinNode << std::endl;

      if(m_prefix.toUri() == m_sendRejoinNode)
      {
        m_gotRejoinNum++;
        std::map<std::string, std::string>::iterator rejoinIter;
        for(rejoinIter =m_possibleRejoinNeis.begin(); rejoinIter!=m_possibleRejoinNeis.end(); ++rejoinIter)
        {
          // std::cout << m_prefix.toUri() << "iterator first " << rejoinIter->first << std::endl;
          if(rejoinIter->first == rejoinNeiNode)
          {
            rejoinIter->second = receivedData;
          }
        };

        if(m_gotRejoinNum == m_sendRejoinNum)
        {
          // for (auto& x: m_possibleRejoinNeis) {
          //   std::cout << m_prefix.toUri() << " key= " << x.first << " value= " << x.second << '\n';
          // };
          std::vector<std::string> possibleRejoinNeis;
          for(rejoinIter =m_possibleRejoinNeis.begin(); rejoinIter!=m_possibleRejoinNeis.end(); ++rejoinIter)
          {
            uint64_t j = rejoinIter->second.find("Join-Success");
            if(j != std::string::npos)
            {
              std::cout << m_prefix.toUri() <<" rejoin-Success link= " << rejoinIter->first << std::endl;
              possibleRejoinNeis.push_back(rejoinIter->first);
            }
            else
            {
              std::cout << m_prefix.toUri() <<" rejoin-Fail " << std::endl;
            };
          };

          if(possibleRejoinNeis.size() != 0) 
          {
            //if multiple neis available to rejoin current tree, choose only one and save others for future use
            std::string rejoinLink = m_prefix.toUri() + possibleRejoinNeis[0] + m_currentTreeFlag;
            std::cout << m_prefix.toUri() <<" FIND rejoin link= " << rejoinLink << std::endl;
            std::map<std::string, std::string>::iterator findLink;
            findLink = m_neiReachable.find(rejoinLink);
            if(findLink != m_neiReachable.end()) 
            {
              findLink->second = "true";
            }
            else {
              m_neiReachable.insert(std::pair<std::string, std::string>(rejoinLink, "true"));
            };
            m_selectNodeName = possibleRejoinNeis[0];
            uint64_t p1 = m_possibleRejoinNeis[m_selectNodeName].find("(");
            uint64_t p2 = m_possibleRejoinNeis[m_selectNodeName].find(")");
            m_prePathID = m_myPathID;
            m_myPathID = m_possibleRejoinNeis[m_selectNodeName].substr(p1+1, p2-p1-1);
            std::cout << m_prefix.toUri() <<" ----- REJOIN Id----  " << m_myPathID << std::endl;

            std::string upNeiFace = "/f-" + m_selectNodeName + "/rejoin-";
            SendOutInterest(upNeiFace);

            NotifyPathIdChange();
            if(m_detectFailureSeqData.size() != 0) {
              CheckFailSeq();
            };

            //reply to previous upstream nei
            if(m_upNodeFail) {
              ReplyData("Change-OK", m_interestOfUpfail);
              m_interestOfUpfail="";
              m_upNodeFail=false;
            };

            if (possibleRejoinNeis.size() > 1) 
            {
              for (uint64_t i=1; i<possibleRejoinNeis.size(); i++) 
              {
                std::string secondRejoinLink = m_prefix.toUri() + possibleRejoinNeis[i] + m_currentTreeFlag;
                findLink = m_neiReachable.find(secondRejoinLink);
                if(findLink != m_neiReachable.end()) 
                {
                  findLink->second = "false";
                }
                else {
                  m_neiReachable.insert(std::pair<std::string, std::string>(rejoinLink, "false"));
                };

                //except the select_up_nei, reply to other join-success neighbours to ignore
                std::string cancelReply = possibleRejoinNeis[i] + "/CancelJoin(" + m_prefix.toUri() + ")-";
                SendOutInterest(cancelReply);
                std::cout << m_prefix.toUri() << " notify-Nei " << cancelReply << std::endl;
              };
            };
          }
          else {
            std::cout << m_prefix.toUri() <<" re-join tree fail, need a new rejoin round" << std::endl;
          };
          m_gotRejoinNum = m_sendRejoinNum = 0;
          m_possibleRejoinNeis.clear();
        };
      }
      else {
        m_gotDisDownNeiNum++;
        std::map<std::string, std::string>::iterator disDownIt;
        for(disDownIt =m_disDownNodeMap.begin(); disDownIt!=m_disDownNodeMap.end(); ++disDownIt)
        {
          //std::cout << m_prefix.toUri() << "iterator first " << disDownIt->first << std::endl;
          if(disDownIt->first == rejoinNeiNode)
          {
            disDownIt->second = receivedData;
          }
        };
        if(m_gotDisDownNeiNum == m_sendDisDownNeiNum)
        {
          std::vector<std::string> possibleRejoinNeis;
          for(disDownIt =m_disDownNodeMap.begin(); disDownIt!=m_disDownNodeMap.end(); ++disDownIt)
          {
            uint64_t j = disDownIt->second.find("Join-Success");
            if(j != std::string::npos)
            {
              std::cout << m_prefix.toUri() <<" rejoin-Success link= " << disDownIt->first << std::endl;
              possibleRejoinNeis.push_back(disDownIt->first);
            }
            else
            {
              std::cout << m_prefix.toUri() <<" rejoin-Fail link= " << disDownIt->first << std::endl;
            };
          };
          //if multiple neis available to rejoin current tree, choose only one and save others for future use
          std::string rejoinLink = m_prefix.toUri() + possibleRejoinNeis[0] + m_currentTreeFlag;
          std::cout << m_prefix.toUri() <<" add rejoin link= " << rejoinLink << std::endl;
          std::map<std::string, std::string>::iterator findLink;
          findLink = m_neiReachable.find(rejoinLink);
          if(findLink != m_neiReachable.end()) 
          {
            findLink->second = "true";
          }
          else {
            m_neiReachable.insert(std::pair<std::string, std::string>(rejoinLink, "true"));
          };
          m_selectNodeName = possibleRejoinNeis[0];
          uint64_t p1 = m_disDownNodeMap[m_selectNodeName].find("(");
          uint64_t p2 = m_disDownNodeMap[m_selectNodeName].find(")");
          m_myPathID = m_disDownNodeMap[m_selectNodeName].substr(p1+1, p2-p1-1);
          
          if(m_neiLocalId.size() == 0) {
            m_neiLocalId.insert(std::pair<std::string, std::string>(m_askRejoinNeiName, "0"));
            std::string askRejoinLink = m_prefix.toUri() + m_askRejoinNeiName+ m_currentTreeFlag;
            std::map<std::string, std::string>::iterator check;
            check = m_neiReachable.find(askRejoinLink);
            if(check != m_neiReachable.end()) {
              m_neiReachable[askRejoinLink] = "true";
            }
            else {
              m_neiReachable.insert(std::pair<std::string, std::string>(askRejoinLink, "true"));
            };
            std::string pId = m_myPathID + "-" + "0";
            ReplyRejoinInterest(pId);
          }
          else {
            NewJoinAssignId(m_askRejoinNeiName);
            ReplyRejoinInterest(m_joinNeiPathId);
          };

          if (possibleRejoinNeis.size() > 1) 
          {
            for (uint64_t i=1; i<possibleRejoinNeis.size(); i++) 
            {
              std::string secondRejoinLink = m_prefix.toUri() + possibleRejoinNeis[i] + m_currentTreeFlag;
              findLink = m_neiReachable.find(secondRejoinLink);
              if(findLink != m_neiReachable.end()) 
              {
                findLink->second = "false";
              }
              else {
                m_neiReachable.insert(std::pair<std::string, std::string>(rejoinLink, "false"));
              };

              //except the select_up_nei, reply to other join-success neighbours to ignore
              std::string cancelReply = possibleRejoinNeis[i] + "/CancelJoin(" + m_prefix.toUri() + ")-";
              SendOutInterest(cancelReply);
              std::cout << m_prefix.toUri() << " notify-Nei " << cancelReply << std::endl;
            };
          };
          m_gotDisDownNeiNum = m_sendDisDownNeiNum = 0;
          m_disDownNodeMap.clear();
        };
      };
    }
    //confirm of cancel-join request
    else if(findCancel != std::string::npos)
    {
      std::cout << m_prefix.toUri() << " got Cancel-ACK: " << receivedData << std::endl;
    }
    //ACK of resend seq-data
    else if(resend != std::string::npos)
    {
      std::cout << m_prefix.toUri() << " got Resend-ACK: " << receivedData << std::endl;
    }
    //reply from downstream neis about doubt-seq check
    else if(doubt != std::string::npos && m_sendDoubtNode == false)
    {
      ReplyData(receivedData, m_forwardDoubtCheck);
      m_forwardDoubtCheck = "";
    }
    //reply from user about doubt-seq check
    else if(doubt != std::string::npos && m_sendDoubtNode == true)
    {
      std::cout << m_prefix.toUri() << " got SEQ-Doubt reply: " << receivedData << std::endl;
      if(receivedData == "Not-receive") 
      {
        uint64_t s1 = gotData.find_first_of("Seq");
        uint64_t s2 = gotData.find("/id");
        std::string resendSeq = gotData.substr(s1, s2-s1-1);
        std::cout << m_prefix.toUri() << " seq= " << resendSeq << std::endl;
        std::vector<std::string> seqRecord;
        std::deque<int> slashList;
        for(uint64_t k=0; k<resendSeq.size(); k++)
        { 
          if(resendSeq[k] == '/') {
            slashList.push_back(k);
          };
        };
        std::string seq = resendSeq.substr(0, slashList[0]);
        seqRecord.push_back(seq);
        for(uint64_t n=0; n<slashList.size()-1; n++)
        {
          seq =  resendSeq.substr(slashList[n]+1, slashList[n+1]-slashList[n]-1);
          seqRecord.push_back(seq);
        };

        for(uint64_t i=0; i<seqRecord.size(); i++)
        {
          // std::cout << m_prefix.toUri() << " seq= " << seqRecord[i] << std::endl;
          std::string seqId = m_treeTag + "-" + seqRecord[i];
          std::map<std::string, int>::iterator compareSeq = m_seqDataSendNum.find(seqId);
          if (compareSeq != m_seqDataSendNum.end()) 
          {
            if (m_seqDataSendNum.at(seqId) == m_seqDataGotNum.at(seqId))
            {
              std::string alldata = m_allReceiveSeqData[seqId];
              // std::cout << m_prefix.toUri() << " && data= " << alldata << std::endl;

              std::vector<int> commaVec;
              std::vector<std::string> neiData;
              uint64_t pos = alldata.find(","); 
              if (pos == std::string::npos) {
                neiData.push_back(alldata);
              }
              else {
                while (pos != std::string::npos){
                  commaVec.push_back(pos);
                  pos = alldata.find(",", pos+1);
                }
              };
              if (commaVec.size() != 0) {
                neiData.push_back(alldata.substr(0, commaVec[0]));
                for(uint64_t k=0; k<commaVec.size(); k++) {
                  neiData.push_back(alldata.substr(commaVec[k]+1, commaVec[k+1]-commaVec[k]-1));
                }
              };
              int tempSumData = 0;
              for (uint8_t i=0; i < neiData.size(); i++) 
              {
                uint64_t l1 = neiData[i].find_last_of("-");
                std::string mapperData = neiData[i].substr(l1+1);
                tempSumData += std::stoi(mapperData);
                // std::cout << m_prefix.toUri() << " --- HERE --- " << " neiData= " << neiData[i] << std::endl;
              };
              std::string dataStr = std::to_string(tempSumData / neiData.size());
              std::string re = "/resend-/";
              std::string resendSeqInterest = "/0-" + re + seqRecord[i] + "-" + dataStr + "-";
              std::cout << m_prefix.toUri() + "resend Seq-Data: " << resendSeqInterest << std::endl;
              SendOutInterest(resendSeqInterest);
            }
            else {
              std::cout << m_prefix.toUri() << " !!! " << seqRecord[i] << " Send != Received " << std::endl;
            };
          }
          else {
            std::cout << m_prefix.toUri() << " Seq Not Exist in sendList " << std::endl;
          };

        };
      };
    }
    //reply from process data by ignore lost-seq
    else if(process != std::string::npos)
    {
      uint64_t notRx = receivedData.find("No SeqData");
      if(notRx != std::string::npos) 
      {
        // no pending data come from downstream neis, this node could start process seq-data if any exist
        std::cout << m_prefix.toUri() <<  "receive Reply for " << m_forwardSeqProcess << std::endl;
        uint64_t s1 = m_forwardSeqProcess.find("Seq");
        uint64_t s2 = m_forwardSeqProcess.find("/except");
        std::string proSeq = m_forwardSeqProcess.substr(s1, s2-s1-1);
        proSeq = m_currentTreeFlag + "-" + proSeq;
        std::map<std::string, int>::iterator checkSeq = m_seqDataGotNum.find(proSeq);
        // std::cout << m_prefix.toUri() <<  "-----" << proSeq << std::endl;
        std::string alldata = m_allReceiveSeqData[proSeq];

        if(checkSeq != m_seqDataGotNum.end()) 
        {
          int n = m_seqDataSendNum[proSeq];
          int n1 = m_seqDataGotNum[proSeq];
          // std::cout << m_prefix.toUri() <<  " nnnn= " << n << " n1= " << n1 << std::endl;
          if((n-1) == n1)
          {
            m_seqDataSendNum[proSeq] = n1;
            ProcessDataBySeq(proSeq);
          };
          ReplyData("Start Process Data", m_forwardSeqProcess);
        }
        else{
          ReplyData("No SeqData", m_forwardSeqProcess);
        };
        m_forwardSeqProcess = "";
      }
      else {
        std::cout << m_prefix.toUri() << " receive seq-data from downstream " << receivedData << std::endl;
      };
    }
    // reply from path-based ID
    else if(u != std::string::npos)
    {
      m_countPathIdReply++;
      std::cout << m_prefix.toUri() << " got ACK: " << receivedData << std::endl;
      if(m_countPathIdReply == m_nodeList4Task.size()) {
        ReplyData("PathID OK", m_pathIdInterest);
        m_countPathIdReply = 0;
        m_pathIdInterest = "";
      };
    }
    // reply for leave-tree-interest
    else if(leave != std::string::npos)
    {
      std::cout << m_prefix.toUri() << " got ACK: " << receivedData << std::endl;
    }
    // reply for reconnect-upstream-interest
    else if(reconnect != std::string::npos)
    {
      std::cout << m_prefix.toUri() << " got: " << receivedData << std::endl;
      NewJoinAssignId(m_askRejoinNeiName);
      ReplyRejoinInterest(m_joinNeiPathId);
    }
    // reply for Report-DownNei-Fail
    else if(downNeiFail != std::string::npos)
    {
      // std::cout << m_prefix.toUri() << " got sink-reply: " << receivedData << std::endl;
      if(receivedData == "OK") 
      {
        uint64_t i1 = gotData.find("(");
        uint64_t i2 = gotData.find("Seq");
        std::string lostNode = gotData.substr(i1+1, i2-i1-1);
        std::map<std::string, std::string>::iterator downId = m_reportFailNeiList.find(lostNode);
        if(downId != m_reportFailNeiList.end()) {
          m_reportFailNeiList.erase(downId);
        };

        //update links saved in m_neiReachable list
        std::string removeLink = m_prefix.toUri() + lostNode + m_currentTreeFlag;
        std::map<std::string, std::string>::iterator f = m_neiReachable.find(removeLink);
        if (f != m_neiReachable.end()) 
        {
          m_neiReachable.erase(f);
        };

        //delete lost-downNei from jobRef
        std::map<std::string, std::string>::iterator checkTree = m_jobRefMap.find(m_currentTreeFlag);
        // std::cout << m_prefix.toUri() <<" ========== lostNode: " << lostNode << " treeId: " << checkTree->second <<std::endl;
        if (checkTree != m_jobRefMap.end()) 
        {
          std::string::size_type dn = checkTree->second.find(lostNode);
          // std::cout << m_prefix.toUri() << " before delete disconnect-nei = " << checkTree->second << " find= " << dn << std::endl;
          if (dn != std::string::npos) 
          {
            checkTree->second.erase(dn, lostNode.length());
            std::cout << m_prefix.toUri() << " Update-neiList after ReportFailure: " << checkTree->second << std::endl;
          }
          m_jobRefNei = checkTree->second;
          jobNeiChangeFlag = true;
          if(checkTree->second != "0") {
            ProcessTaskNeis(checkTree->second);
          }
          else {
            m_nodeList4Task.clear();
            LeaveJobTree();
            // std::cout << m_prefix.toUri() << " has noDownstreamNei so Notify->" << m_selectNodeName << std::endl;
          };
        }; 
      }
      else {
        std::cout << m_prefix.toUri() << " got reply for ReportFailure: " << receivedData << std::endl;
      };
    }
    // reply for Change Recover Upstream
    else if(newUp != std::string::npos)
    {
      std::cout << m_prefix.toUri() << " got: " << receivedData << std::endl;
      if(receivedData == "OK") 
      {
        ReplyData("Recover-Success", m_interestAsRecoverReducer);

        uint64_t m = gotData.find_first_of("-");
        std::string addNode = gotData.substr(0, m+1);

        std::map<std::string, std::string>::iterator checkTree = m_jobRefMap.find(m_currentTreeFlag);
        if (checkTree != m_jobRefMap.end()) 
        {
          checkTree->second += addNode;
          std::cout << m_prefix.toUri() <<" ========== addNode: " << addNode << " treeId: " << checkTree->second <<std::endl;
        };
        m_jobRefNei = checkTree->second;
        jobNeiChangeFlag = true;
        ProcessTaskNeis(checkTree->second);
        AddRejoinDownNei(addNode);

        m_interestAsRecoverReducer = "";
      };
    }
    else if(upNeiFail != std::string::npos)
    {
      if(receivedData == "Change-OK")
      {
        std::cout << m_prefix.toUri() << " receive child-node Change-Tree-Path-OK " << std::endl;
        uint64_t n = gotData.find_first_of("-");
        std::string preNei = gotData.substr(0, n+1);
        uint64_t a1 = m_pendingInterestName.toUri().find("TS");
        uint64_t a2 = m_pendingInterestName.toUri().find("TE");
        std::string treeId = m_pendingInterestName.toUri().substr(a1+2, a2-a1-3);
        std::map<std::string, std::string>::iterator jobneis = m_jobRefMap.find(treeId);
        if(jobneis != m_jobRefMap.end())
        {
          std::string::size_type dn = jobneis->second.find(preNei);
          // std::cout << m_prefix.toUri() << " before delete disconnect-nei = " << jobneis->second << " find= " << dn << std::endl;
          if (dn != std::string::npos) {
            jobneis->second.erase(dn, preNei.length());
            // std::cout << m_prefix.toUri() << " after delete disconnect-nei = " << jobneis->second << std::endl;
          };
        };
        jobNeiChangeFlag = true;
      }
      else {
        std::cout << m_prefix.toUri() << " got child-node reply= " << receivedData << std::endl;
      };
    }
    //data for normal Interest
    else if(findDis2 != std::string::npos && gotData[1] != 'p')
    {    
      uint64_t u1 = gotData.find("TS");
      uint64_t u2 = gotData.find("TE");
      std::string receiveTreeId = gotData.substr(u1+2, u2-u1-3); 
      if(receivedData == "Ignore")
      {
        // for (auto& x: m_seqDataSendNum) {
        //   std::cout << m_prefix.toUri() << "send SEQ" << x.first << ": " << x.second << '\n';
        // };
        uint64_t d1 = gotData.find("Seq");
        uint64_t d2 = gotData.find(")");
        std::string ignoreSeq = gotData.substr(d1, d2-d1);
        std::string SeqId = receiveTreeId + "-" + ignoreSeq;
        m_seqDataSendNum[SeqId] = m_seqDataSendNum[SeqId] - 1;
        // std::cout << m_prefix.toUri() << "get IGNORE == " <<  SeqId << " NUM= " << m_seqDataSendNum[SeqId] << std::endl;
      }
      else 
      {
        // std::cout << m_prefix.toUri() << "   //////////// receive == " << receivedData << std::endl;
        uint64_t s1 = receivedData.find("(");
        uint64_t s = receivedData.find("-Seq");
        uint64_t f = receivedData.find("-");
        std::string receiveSeqNum;
        if(s == std::string::npos) {
          receiveSeqNum = receivedData.substr(0,f);
        }
        else {
          receiveSeqNum = receivedData.substr(s+1, s1-s-1);
        };
        std::string receiveNeiName = gotData.substr(0, u1-1);
        std::string treeIdSeq = receiveTreeId + "-" + receiveSeqNum;

        std::map<std::string, std::string>::iterator checkNode = m_receiveNodeandData.find(receiveNeiName);
        if (checkNode != m_receiveNodeandData.end())
        {
          if(checkNode->second == "")
          {
            checkNode->second = receivedData;
          }
          else
          {
            checkNode->second = checkNode->second + ";" +receivedData;
          };
        }
        else {
          std::cout << m_prefix.toUri() << " receive wrong downstream data from " <<  receiveNeiName << std::endl;
        };
        // for (auto& x: m_receiveNodeandData) {
        //   std::cout << m_prefix.toUri() << " nodeID: " << x.first << " data= "<< x.second << std::endl;
        // };

        if ( std::find(m_countSeq.begin(), m_countSeq.end(), receiveSeqNum) == m_countSeq.end() )
        {
          m_countSeq.push_back(receiveSeqNum);
          m_countdata += 1;
          // std::cout<< m_prefix.toUri() << "////// count= " <<  m_countdata << std::endl;
        };

        uint64_t s2 = receiveSeqNum.find("q");
        std::string numonly = receiveSeqNum.substr(s2+1);
        std::string insertId;
        for(uint64_t j = 0; j < m_groupIds.size(); j++)
        {
          uint64_t k = m_groupIds[j].find("-");
          std::string smallId = m_groupIds[j].substr(0,k);
          std::string bigId = m_groupIds[j].substr(k+1);
          // std::cout<< m_prefix.toUri() << "groupIds: " << smallId << " and " << bigId << " rxSeqnum= " << receiveSeqNum << std::endl;
          if ((stoi(numonly) >= stoi(smallId)) & (stoi(numonly) <= stoi(bigId))) 
          {
            std::cout<< m_prefix.toUri() << "seq in Group: " << m_groupIds[j] << std::endl;
            insertId = m_groupIds[j];
            if(m_computeGroup[insertId] == "") {
              m_computeGroup[insertId] = treeIdSeq + ";" ;
            }
            else {
              uint64_t checkid = m_computeGroup[insertId].find(treeIdSeq);
              if (checkid == std::string::npos) {
                m_computeGroup[insertId] = m_computeGroup[insertId] + treeIdSeq + ";" ;
              };
            };
          };
        };
        // std::cout<< m_prefix.toUri() << "compute-group = " << m_computeGroup[insertId] << std::endl;
        // for (auto& x: m_computeGroup) {
        //   std::cout << m_prefix.toUri() << " groupID: " << x.first << " groupSeq= "<< x.second << std::endl;
        // };

        std::map<std::string, int>::iterator checkSeq = m_seqDataGotNum.find(treeIdSeq);
        receivedData = receiveNeiName + receivedData;
        if(checkSeq == m_seqDataGotNum.end()) {
          m_seqDataGotNum.insert(std::pair<std::string, int>(treeIdSeq, 1));
          m_allReceiveSeqData.insert(std::pair<std::string, std::string>(treeIdSeq, receivedData));
        }
        else {
          m_seqDataGotNum.at(treeIdSeq) += 1;
          m_allReceiveSeqData.at(treeIdSeq) += "," + receivedData;
        };
        // for (auto& x: m_allReceiveSeqData) {
        //   std::cout << m_prefix.toUri() << " treeSeqID: " << x.first << " data= "<< x.second << std::endl;
        // };
        
        // std::cout << m_prefix.toUri() << " rxSeq numOnly= " << numonly << std::endl;
        if (stoi(numonly) < m_countdata)
        {
          if ( std::find(m_processOkSeq.begin(), m_processOkSeq.end(), treeIdSeq) == m_processOkSeq.end() )
          {
            ProcessDataBySeq(treeIdSeq);
            // std::cout << m_prefix.toUri() << " receive before Seq " << receiveSeqNum << std::endl; 
          }; 
        };

        if (m_countdata >= 5) 
        {
          uint64_t aa = m_countdata % 5;
          if (aa == 0) {
            int computeIndex = m_countdata / 5 - 1;
            std::cout<< m_prefix.toUri() << "Call-Process-Data m_countdata= " << m_countdata << " RxId= " <<  receiveSeqNum << std::endl;
            std::string getId = m_groupIds[computeIndex];
            // std::cout << m_prefix.toUri() << " getId ====== " << getId << std::endl;
            std::string idString = m_computeGroup[getId];
            std::cout << m_prefix.toUri() << " id ====== " << idString << std::endl;
            std::vector<int> splitVec;
            std::vector<std::string> idList;
            uint64_t pos = idString.find(";"); 
            if (pos == std::string::npos) {
              idList.push_back(idString);
            }
            else {
              while (pos != std::string::npos){
                splitVec.push_back(pos);
                pos = idString.find(";", pos+1);
              }
            };
            if (splitVec.size() != 0) {
              idList.push_back(idString.substr(0, splitVec[0]));
              for(uint64_t k=0; k<splitVec.size()-1; k++) {
                idList.push_back(idString.substr(splitVec[k]+1, splitVec[k+1]-splitVec[k]-1));
              }
            };
            for(uint64_t l = 0; l < idList.size(); l++) 
            {
              std::string startProcessId = idList[l];
              ProcessDataBySeq(startProcessId);
            }; 
          };  
        };
      };
    }
    else 
    {
      switch(gotData[1])
      {
        case 'p':
        {
          uint64_t discoverFace = gotData.find("discover");
          if (discoverFace != std::string::npos) 
          {
            uint64_t tp = gotData.find_first_of("-");
            std::string tempInName = gotData.substr(tp+1);
            std::map<std::string, std::string>::iterator it = m_buildTreeInterestMap.find(tempInName);
            //std::cout<<"tempInName " << tempInName << std::endl;
            if(it != m_buildTreeInterestMap.end())
            {
              m_buildTreeInterestMap[tempInName] = receivedData;
              m_gotPitNum++;
            }
            
            if(m_askPitNum == m_gotPitNum)
            {
              std::map<std::string, std::string>::iterator fibIt = m_fibResult.find(m_treeTag);
              std::string fibContent = fibIt->second; 
              for(std::map<std::string, std::string>::iterator it=m_buildTreeInterestMap.begin(); it!=m_buildTreeInterestMap.end(); ++it)
              {
                // std::cout << m_prefix.toUri() << " 111 " << it->first << std::endl;
                // std::cout << m_prefix.toUri() << " 222 " << it->second << std::endl;
                if (it->second != fibContent)
                {
                  //add this node to check-nei-table as potential nei if select-nei link is broken
                  uint64_t p1 = it->first.find_first_of("-");
                  uint64_t p2 = it->first.find("/discover");
                  std::string potentialNei = it->first.substr(p1+1, p2-p1-1);
                  std::string taskNei =  m_prefix.toUri() + potentialNei + m_treeTag;
                  // std::cout<<m_prefix.toUri() << " potential Nei: " << taskNei <<std::endl;
                  m_neiReachable.insert(std::pair<std::string, std::string>(taskNei, "false"));

                  // std::cout << m_prefix.toUri() << " 222 " << it->first << std::endl;
                  std::string replyNackName = it->first;
                  Name replyNackDataName(replyNackName);
                  auto replyNackData = make_shared<Data>();
                  replyNackData->setName(replyNackDataName);
                  replyNackData->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
                  std::string rawData("nope");
                  const uint8_t* p = reinterpret_cast<const uint8_t*>(rawData.c_str());
                  auto buffer = make_shared< ::ndn::Buffer>(p, rawData.size());
                  replyNackData->setContent(buffer);
              
                  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));
                  if (m_keyLocator.size() > 0) {
                    signatureInfo.setKeyLocator(m_keyLocator);
                  }
                  replyNackData->setSignatureInfo(signatureInfo);
                  ::ndn::EncodingEstimator estimator;
                  ::ndn::EncodingBuffer encoder(estimator.appendVarNumber(m_signature), 0);
                  encoder.appendVarNumber(m_signature);
                  replyNackData->setSignatureValue(encoder.getBuffer());
                  NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << replyNackData->getName());

                  // to create real wire encoding
                  replyNackData->wireEncode();
                  m_transmittedDatas(replyNackData, this, m_face);
                  m_appLink->onReceiveData(*replyNackData);
                  std::cout << m_prefix.toUri() << "R reply: " << rawData << " for: " << replyNackName << std::endl;
                }
                else 
                {
                  // map size = 1 && the face = FIBface, got the selected upstream, not reply immediately, continue explore downstreams
                  m_selectUpstream = it->first;
                  // std::cout<< "selected: " << m_selectUpstream <<std::endl;
                  m_selectNodeFace = it->second;
          
                  //upstream node name
                  uint64_t p1 = m_selectUpstream.find_first_of("-");
                  uint64_t p2 = m_selectUpstream.find("/discover");
                  m_selectNodeName = m_selectUpstream.substr(p1+1, p2-p1-1);
                  // std::cout<<m_prefix.toUri() << " selected: " << m_selectNodeName <<std::endl;

                  //add selecet-upstream to check-nei-table
                  std::string taskNei =  m_prefix.toUri() + m_selectNodeName + m_treeTag;
                  std::cout<<m_prefix.toUri() << " taskNei: " << taskNei <<std::endl;
                  m_neiReachable.insert(std::pair<std::string, std::string>(taskNei, "true"));
                  
                  //initiate new round to ask one-hop neighbour
                  if (m_oneHopNeighbours.size() != 0)
                  {
                    //send interests to one-hop to discover tree
                    DiscoverDownstreams(); 
                  }
                  else 
                  {
                    FindNeighbours();
                  }
                  
                }
              }
            }
            m_buildTreeInterestMap.clear();
          }
          else {
            // std::cout << m_prefix.toUri() <<" select upNei Face= " << receivedData << std::endl; 
            if (receivedData == m_selectNodeFace)
            {
              ProcessNormalInterest(m_normalInterest);
            }
            else {
              ReplyData("Ignore", m_normalInterest->getName().toUri());
            };
          };
          break;
        }
        
        case 'f':
        {
          uint64_t rejoinFace = gotData.find("rejoin");
          if (rejoinFace == std::string::npos) {
            //std::cout <<m_prefix.toUri() << "fib data: " << receivedData <<std::endl;
            std::map<std::string, std::string>::iterator fibIter1 = m_fibResult.find(m_treeTag);
            if(fibIter1 != m_fibResult.end())
            {
              fibIter1->second = receivedData;
              QueryPit();
            };
          }
          else {
            //get FaceId of rejoin up-nei 
            m_selectNodeFace = receivedData;
            
          };
          break;
        }
        
        case 'n':
        {
          uint64_t d1 = gotData.find("-");
          std::string neiName = gotData.substr(d1+1, gotData.size()-d1-1);
          std::map<std::string, std::string>::iterator it = m_checkNeibMap.find(neiName);
          if(it != m_checkNeibMap.end())
          {
            //const uint8_t* p = reinterpret_cast<const uint8_t*>(rawData.c_str());
            //const uint8_t* tempVal = reinterpret_cast<const uint8_t*>(receivedData.c_str());
            m_checkNeibMap[neiName] = receivedData;
            m_gotDisNeiNum += 1;
          }
          //std::cout << "neighour name: " << neiName << std::endl;
          if(m_sendDisNeiNum == m_gotDisNeiNum) 
          { 
            //std::cout << "R receive all disNeiInterests" <<std::endl;
            for(it=m_checkNeibMap.begin(); it != m_checkNeibMap.end(); ++it)
            {
              if(it->second == "1")
              { 
                m_oneHopNeighbours.push_back(it->first);
                //std::cout << "it -> first: " << it->first <<std::endl;
              }
            }
            //std::cout << "next-hop neighbour number: " << m_oneHopNeighbours.size() <<std::endl;
            DiscoverDownstreams();
            m_checkNeibMap.clear();
          }
          break;
        }
      };
    }

};

} // namespace ndn
} // namespace ns3
