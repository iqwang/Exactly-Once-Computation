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

// #include "/usr/include/python2.7/Python.h"
#include "ndn-wq-checkpoint-mapper.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>
#include <deque>
//////////////////

#include<unistd.h>
#include<stdio.h>
#include<errno.h>
#include<stdlib.h>
#include <fstream>

/////////////////





NS_LOG_COMPONENT_DEFINE("ndn.WqCheckpointMapper");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(WqCheckpointMapper);

TypeId
WqCheckpointMapper::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::WqCheckpointMapper")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<WqCheckpointMapper>()
      .AddAttribute("Prefix", "Prefix, for which producer has the data", StringValue("/"),
                    MakeNameAccessor(&WqCheckpointMapper::m_prefix), MakeNameChecker())
      .AddAttribute("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                    StringValue("/"), MakeNameAccessor(&WqCheckpointMapper::m_postfix), MakeNameChecker())
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets", UintegerValue(1024),
                    MakeUintegerAccessor(&WqCheckpointMapper::m_virtualPayloadSize),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                    TimeValue(Seconds(0)), MakeTimeAccessor(&WqCheckpointMapper::m_freshness),
                    MakeTimeChecker())
      .AddAttribute("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                    UintegerValue(0), MakeUintegerAccessor(&WqCheckpointMapper::m_signature),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
                    "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&WqCheckpointMapper::m_keyLocator), MakeNameChecker());
  return tid;
}

WqCheckpointMapper::WqCheckpointMapper()
{
  m_rand = CreateObject<UniformRandomVariable>();
  NS_LOG_FUNCTION_NOARGS();
}

// inherited from Application base class.
void
WqCheckpointMapper::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
}

void
WqCheckpointMapper::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}

void 
WqCheckpointMapper::SendInterest(std::string sendName)
{
  shared_ptr<Name> mapTaskName = make_shared<Name>(sendName);
  if ((sendName.substr(0,2) != "/p") & (sendName.substr(0,2) != "/f")) {
    mapTaskName->appendSequenceNumber(m_rand->GetValue(0, std::numeric_limits<uint16_t>::max()));
  }
  shared_ptr<Interest> mapTaskInterest = make_shared<Interest>();
  mapTaskInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  mapTaskInterest->setName(*mapTaskName);
  mapTaskInterest->setInterestLifetime(ndn::time::seconds(1));
  m_transmittedInterests(mapTaskInterest, this, m_face);
  m_appLink->onReceiveInterest(*mapTaskInterest);
};

void
WqCheckpointMapper::CheckNeiConnect()
{
  std::cout << "------------------mmmmmmmmmmmmm" << std::endl;
  std::map<std::string, std::string>::iterator i;
  for(i=m_neiReachable.begin(); i != m_neiReachable.end(); ++i) {
    std::cout << m_prefix.toUri() + " nei-reachable: " << i -> first <<  " == " << i -> second << std::endl;
  }
  // std::cout << "OnData check nei size: " << m_checkNeibMap.size() << std::endl;
};

void 
WqCheckpointMapper::LinkBroken(std::string upNode, std::string downNode)
{
  if(upNode == "/6-" && downNode == "/m3-")
  {
    std::string changeLink = "/m3-/6-/0-";
    std::map<std::string, std::string>::iterator l = m_neiReachable.find(changeLink);
    if (l != m_neiReachable.end()) {
      l->second = "false";
      CheckNeiConnect();  // maybe not using this function anymore?
    };
  }
  else if(upNode == "/2-" && downNode == "/m3-")
  {
    std::string changeLink = "/m3-/2-/0-";
    std::map<std::string, std::string>::iterator l = m_neiReachable.find(changeLink);
    if (l != m_neiReachable.end()) {
      l->second = "false";
      CheckNeiConnect();
    };
  }
  else if(upNode == "/1-" && downNode == "/m6-")
  {
    std::string changeLink = "/m6-/1-/0-";
    std::map<std::string, std::string>::iterator l = m_neiReachable.find(changeLink);
    if (l != m_neiReachable.end()) {
      l->second = "false";
      CheckNeiConnect();
    };
  }
  else if(upNode == "/1-" && downNode == "/82-")
  {
    std::string changeLink = "/82-/1-/0-";
    std::map<std::string, std::string>::iterator l = m_neiReachable.find(changeLink);
    if (l != m_neiReachable.end()) {
      l->second = "false";
      CheckNeiConnect();
    };
  }
  else if(upNode == "/26-" && downNode == "/35-")
  {
    std::string changeLink = "/35-/26-/0-";
    std::map<std::string, std::string>::iterator l = m_neiReachable.find(changeLink);
    if (l != m_neiReachable.end()) {
      l->second = "false";
      CheckNeiConnect();
    };
  }
  else if(upNode == "/22-" && downNode == "/98-")
  {
    std::string changeLink = "/98-/22-/0-";
    std::map<std::string, std::string>::iterator l = m_neiReachable.find(changeLink);
    if (l != m_neiReachable.end()) {
      l->second = "false";
      CheckNeiConnect();
    };
  };
};

void
WqCheckpointMapper::AddSeqData(std::string seqnum, int seqdata)
{
  if (m_detectLinkFailure == true) 
  {
    std::map<std::string, int>::iterator d = m_detectFailureSeqData.find(seqnum);
    if (d == m_detectFailureSeqData.end()) {
      m_detectFailureSeqData.insert(std::pair<std::string, int>(seqnum, seqdata));
    }
    else {
      std::cout << m_prefix.toUri() << "Insert Duplicated SEQ_num to FailureSeqData list" << std::endl;
    };
    m_detectLinkFailure = false;
    for (auto& x: m_detectFailureSeqData) {
      std::cout << m_prefix.toUri() << "failure SEQ" << x.first << ": " << x.second << '\n';
    }
  }
  else {
    std::map<std::string, int>::iterator i = m_sentSeqData.find(seqnum);
    if (i == m_sentSeqData.end()) 
    {
      m_sentSeqData.insert(std::pair<std::string, int>(seqnum, seqdata));

      // Time writeTime = Simulator::Now().ToDouble(Time::S);
      std::ofstream recording;
      recording.open("computeStateRecord.txt", std::ios_base::app);
      recording << Simulator::Now().GetSeconds() << '\t' << m_prefix.toUri() << '\t' << m_sentSeqData.size() << std::endl;
      recording.close();
    }
    else {
      // std::cout << m_prefix.toUri() << "Insert Duplicated SEQ_num" << std::endl;
      m_sentSeqData.erase(i);
      m_sentSeqData.insert(std::pair<std::string, int>(seqnum, seqdata));
    };

    // for (auto& x: m_sentSeqData) {
    //   std::cout << m_prefix.toUri() << "send SEQ" << x.first << ": " << x.second << '\n';
    // };
    // for (auto& x: m_sentSeqToNei) {
    //   std::cout << m_prefix.toUri() << "SEQ---Nei" << x.first << ": " << x.second << '\n';
    // };
  }
};

void
WqCheckpointMapper::AddSeqUpNei(std::string seqnum, std::string neiname)
{
  std::map<std::string, std::string>::iterator i = m_sentSeqToNei.find(seqnum);
  neiname += ";";
  if (i == m_sentSeqToNei.end()) 
  {
    m_sentSeqToNei.insert(std::pair<std::string, std::string>(seqnum, neiname));
  }
  else {
    i->second += neiname;
  };
};

void
WqCheckpointMapper::CheckFailSeq()
{
  std::map<std::string, int>::iterator checkFail;
  std::string seqlist = "";
  for(checkFail = m_detectFailureSeqData.begin(); checkFail != m_detectFailureSeqData.end(); checkFail++) 
  {
    // seqlist = seqlist + checkFail->first + "/";
    seqlist = checkFail->first;
  }
  std::string failSeqInterest = "/0-/doubt-T"+ m_currentTreeTag + seqlist + "-/id(" + m_prePathID + ")-";
  std::cout << m_prefix.toUri() + " !!! checkFailSeq " << failSeqInterest << std::endl;
  SendInterest(failSeqInterest);
};

void
WqCheckpointMapper::ReplyData(std::string replyContent, shared_ptr<const Interest> interest)
{
  Name dataName(interest->getName());
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

  // to create real wire encoding
  taskData->wireEncode();
  m_transmittedDatas(taskData, this, m_face);
  m_appLink->onReceiveData(*taskData);
  std::cout <<m_prefix.toUri() << " reply-data: " << replyContent << std::endl;
};

void
WqCheckpointMapper::ProcessNormalInterest(shared_ptr<const Interest> interest)
{
  std::string interestName = interest->getName().toUri();
  uint64_t u1 = interestName.find("(");
  uint64_t u2 = interestName.find(")");
  std::string seqNum = interestName.substr(u1+1, u2-u1-1);

  int rawNum = std::rand() % 100 + 10;
  std::string rawData = std::to_string(rawNum);
  rawData = seqNum + "-" + rawData;
  // add seqNum&data pair to list in case for re-sending
  std::map<std::string, std::string>::iterator i = m_sentSeqToNei.find(seqNum);
  if (i == m_sentSeqToNei.end()) 
  {
    ReplyData(rawData, interest);
    rawData.clear();
  };
  AddSeqData(seqNum, rawNum);
};

void 
WqCheckpointMapper::ClearHistorySaveData(std::string seqList)
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

  for(uint64_t m=0; m<seqContent.size(); m++)
  {
    m_sentSeqData.erase(seqContent[m]);
    m_sentSeqToNei.erase(seqContent[m]);
  };
  // for (auto& x: m_sentSeqData) {
  //   std::cout << m_prefix.toUri() << " Seq: " << x.first << " data= "<< x.second << std::endl;
  // };
  // for (auto& x: m_sentSeqToNei) {
  //   std::cout << m_prefix.toUri() << " Seq: " << x.first << " Nei= "<< x.second << std::endl;
  // };

  std::ofstream recording;
  recording.open("computeStateRecord.txt", std::ios_base::app);
  // Time writeTime = Simulator::Now().GetSeconds();
  // Simulator::Now().ToDouble(Time::S)
  recording << Simulator::Now().GetSeconds() << '\t' << m_prefix.toUri() << '\t' << m_sentSeqData.size() << std::endl;
  recording.close();
};

void
WqCheckpointMapper::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside

  NS_LOG_FUNCTION(this << interest);

  if (!m_active) {
    return;
  }
    
    m_pendingInterestName = interest->getName();
    //std::cout <<"interest name is : " << m_pendingInterestName.toUri() << std::endl;
    uint64_t sp = m_pendingInterestName.toUri().find("/discover");
    uint64_t sp1 = m_pendingInterestName.toUri().find("rejoin");
    uint64_t d = m_pendingInterestName.toUri().find("doubt");
    uint64_t fClear = m_pendingInterestName.toUri().find("clear");
    uint64_t cp = m_pendingInterestName.toUri().find("cp");
    uint64_t changeUp = m_pendingInterestName.toUri().find("newUp");
  
    //interest for discover tree
    if (sp != std::string::npos) 
    {
      //std::cout <<m_prefix.toUri() <<"000 m_askPitNum: "<< m_askPitNum <<std::endl;
      std::cout <<m_prefix.toUri() << " got disTree Interest: "<< m_pendingInterestName.toUri()<< std::endl;
      m_askPitPrefix = "/p-";
      m_disTreeInterestMap.insert(std::pair<std::string, std::string>(m_pendingInterestName.toUri(),"0"));
      
      uint64_t t1 = m_pendingInterestName.toUri().find("TS");
      uint64_t t2 = m_pendingInterestName.toUri().find("TE");
      m_currentTreeTag = m_pendingInterestName.toUri().substr(t1+2, t2-t1-3);
      //std::cout <<"tree tag: "<< m_currentTreeTag <<std::endl;
  
      std::string tempAskPit = m_askPitPrefix + m_pendingInterestName.toUri();
      SendInterest(tempAskPit);
      //std::cout <<"tempAskPit: "<< tempAskPit <<std::endl;
      m_askPitNum++;
      //std::cout <<m_prefix.toUri() <<"111 m_askPitNum: "<< m_askPitNum <<std::endl;
    }
    else if (sp1 != std::string::npos)
    {
      std::string rawData = "nope";
      Name dataName(m_pendingInterestName);
      auto taskData = make_shared<Data>();
      taskData->setName(dataName);
      taskData->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
      const uint8_t* p = reinterpret_cast<const uint8_t*>(rawData.c_str());
      auto buffer = make_shared< ::ndn::Buffer>(p, rawData.size());
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

      // to create real wire encoding
      taskData->wireEncode();
      m_transmittedDatas(taskData, this, m_face);
      m_appLink->onReceiveData(*taskData);
      std::cout <<m_prefix.toUri() << " return: " << rawData << std::endl;
    }
    else if (d != std::string::npos)
    {
      std::cout << m_prefix.toUri() <<" receive Interest: " << m_pendingInterestName.toUri() <<std::endl;
    }
    //notify to clear history data
    else if (fClear != std::string::npos)
    {
      std::cout << m_prefix.toUri() << " receive clear History-Seq " << std::endl;
      uint64_t s1 = m_pendingInterestName.toUri().find("Seq");
      uint64_t l = m_pendingInterestName.toUri().find_last_of("/");
      std::string seqs = m_pendingInterestName.toUri().substr(s1, l-s1-2);
      // std::cout << m_prefix.toUri() << " clear Seq= " << seqs << std::endl;
      ClearHistorySaveData(seqs);
      ReplyData("Clear-Done", interest);
    }
    //to checkpoint current state
    else if (cp != std::string::npos)
    {
      uint64_t j = m_pendingInterestName.toUri().find("Com");
      if(j != std::string::npos) {
        std::cout << m_prefix.toUri() << " receive Checkpoint-ComputeNodeInfo " << std::endl;
        std::string replyContent = m_prefix.toUri() + "&Mapper";
        ReplyData(replyContent, interest);
      }
      else {
       std::cout << m_prefix.toUri() << " receive Checkpoint-Msg " << std::endl;
       ReplyData("OK", interest);
      }
    }
    //change Upstreame-Nei
    else if (changeUp != std::string::npos)
    {
      std::cout << m_prefix.toUri() << " change Upstreame-Nei " << std::endl;
      uint64_t u1 = interest->getName().toUri().find("(");
      uint64_t u2 = interest->getName().toUri().find(")");
      std::string changeUp = interest->getName().toUri().substr(u1+1, u2-u1-1);
      // std::cout << m_prefix.toUri() << " change Upstreame-Nei to: " << changeUp << std::endl;
      m_preUpNeiName = m_selectNodeName;
      m_selectNodeName = changeUp;
      std::string removeLink = m_prefix.toUri() + m_preUpNeiName + m_currentTreeTag;
      std::string changeLink = m_prefix.toUri() + m_selectNodeName + m_currentTreeTag;
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

      ReplyData("OK", interest);
    }
    // normal interst
    else 
    {
      // Simulator::Schedule(Seconds(12), &WqMapper::LinkBroken, this, "/8-", "/m3-");
      // Simulator::Schedule(Seconds(62), &WqMapper::LinkBroken, this, "/9-", "/m4-");
      // Simulator::Schedule(Seconds(82), &WqMapper::LinkBroken, this, "/7-", "/m2-");
      // Simulator::Schedule(Seconds(32), &WqMapper::LinkBroken, this, "/2-", "/m3-");
      // Simulator::Schedule(Seconds(62), &WqMapper::LinkBroken, this, "/3-", "/m4-");
      // Simulator::Schedule(Seconds(82), &WqMapper::LinkBroken, this, "/1-", "/m2-");
      // Simulator::Schedule(Seconds(6), &WqCheckpointMapper::LinkBroken, this, "/6-", "/m3-");
      // Simulator::Schedule(Seconds(82), &WqMapper::LinkBroken, this, "/1-", "/m6-");
      // Simulator::Schedule(Seconds(9), &WqMapper::LinkBroken, this, "/1-", "/82-");
      // Simulator::Schedule(Seconds(29), &WqMapper::LinkBroken, this, "/26-", "/35-");
      // Simulator::Schedule(Seconds(69), &WqMapper::LinkBroken, this, "/22-", "/98-");
      // m_sendEvent = Simulator::Schedule(Seconds(2), &WqMapper::LinkBroken, this);
      // Simulator::Remove(m_sendEvent);
      std::cout << m_prefix.toUri() <<" get normal Interest: " << m_pendingInterestName.toUri() <<std::endl;
      m_normalInterest = interest;
      // std::string requestPit = "/p-" + m_pendingInterestName.toUri();
      // SendInterest(requestPit);
      ProcessNormalInterest(m_normalInterest);
    }
}

void
WqCheckpointMapper::OnData(shared_ptr<const Data> data)
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
    // std::cout << m_prefix.toUri() <<" get Data: " << receivedData << std::endl;
    std::string gotDataName = data->getName().toUri();
    uint64_t r = gotDataName.find("rejoin");
    uint64_t d = gotDataName.find("doubt");
    uint64_t rs = gotDataName.find("resend");
    

    //data for rejoin-tree Interest
    if((r != std::string::npos) & (gotDataName[1] != 'f')) 
    {
      std::cout << m_prefix.toUri() <<" get Data: " << receivedData << std::endl;
      std::string uriData = data->getName().toUri();
      uint64_t fu = uriData.find_first_of("-");
      std::string rejoinNeiNode = uriData.substr(0,fu+1);
      m_gotRejoinNum++;
      std::map<std::string, std::string>::iterator rejoinIter;
      for(rejoinIter =m_possibleRejoinNeis.begin(); rejoinIter!=m_possibleRejoinNeis.end(); ++rejoinIter)
      {
        //std::cout << m_prefix.toUri() << "iterator first " << disDownIt->first << std::endl;
        if(rejoinIter->first == rejoinNeiNode)
        {
          rejoinIter->second = receivedData;
        }
      };

      if(m_gotRejoinNum == m_sendRejoinNum)
      {
        reJoinAsk = false;
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
          std::string rejoinLink = m_prefix.toUri() + possibleRejoinNeis[0] + m_currentTreeTag;
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

          m_prePathID = m_myPathID;
          m_preUpNeiName = m_selectNodeName;
          m_selectNodeName = possibleRejoinNeis[0];
          uint64_t p1 = m_possibleRejoinNeis[m_selectNodeName].find("(");
          uint64_t p2 = m_possibleRejoinNeis[m_selectNodeName].find(")");
          m_myPathID = m_possibleRejoinNeis[m_selectNodeName].substr(p1+1, p2-p1-1);
          std::cout << m_prefix.toUri() <<" ----- REJOIN Id----  " << m_myPathID << std::endl;

          if (possibleRejoinNeis.size() > 1) 
          {
            for (uint64_t i=1; i<possibleRejoinNeis.size(); i++) 
            {
              std::string secondRejoinLink = m_prefix.toUri() + possibleRejoinNeis[i] + m_currentTreeTag;
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
              SendInterest(cancelReply);
              std::cout << m_prefix.toUri() << " notify-Nei " << cancelReply << std::endl;
            };
          };

          std::string upNeiFace = "/f-" + m_selectNodeName + "/rejoin-";
          SendInterest(upNeiFace);
          if (m_detectFailureSeqData.size() != 0) {
            CheckFailSeq();
          };
        }
        else {
          std::cout << m_prefix.toUri() <<" re-join tree fail, need a new rejoin round" << std::endl;
        };
        m_gotRejoinNum = m_sendRejoinNum = 0;
        m_possibleRejoinNeis.clear();
      };
    }
    // data for seq-check Interest
    else if(d != std::string::npos)
    {
      if(receivedData == "Not-receive") 
      {
        std::cout << m_prefix.toUri() <<" get SEQ-reply " << receivedData << std::endl;
        //resend previous seq-data
        uint64_t s1 = gotDataName.find("Seq");
        uint64_t s2 = gotDataName.find("/id");
        std::string resendSeq = gotDataName.substr(s1, s2-s1-1);
        std::string dataStr = "";
        std::map<std::string, int>::iterator i = m_detectFailureSeqData.find(resendSeq);
        if (i != m_detectFailureSeqData.end()) 
        {
          dataStr = std::to_string(i->second);  
        }
        else
        {
          std::map<std::string, int>::iterator j = m_sentSeqData.find(resendSeq);
          if (j != m_sentSeqData.end()) {
            dataStr = std::to_string(j->second);
          }
        };
        std::string re = "/resend-/";
        std::string resendSeqInterest = "/0-" + re + resendSeq + "-" + dataStr + "-";
        std::cout << m_prefix.toUri() + "resend Seq-Data: " << resendSeqInterest << std::endl;
        SendInterest(resendSeqInterest);
      }
      else {
        std::cout << m_prefix.toUri() <<" get SEQ-reply " << receivedData << std::endl;
      };
    }
    //ACK for resend seq-data
    else if(rs != std::string::npos)
    {
      std::cout << m_prefix.toUri() <<" get resend-ACK = " << receivedData << std::endl;
    }
    else 
    {
      switch(gotDataName[1])
      {
        case 'p':
        {
          uint64_t discoverFace = gotDataName.find("discover");
          if (discoverFace != std::string::npos)
          {
            //std::cout << m_prefix.toUri() <<" get pit Data: " << receivedData << std::endl;
            uint64_t tp = gotDataName.find_first_of("-");
            std::string tempInName = gotDataName.substr(tp+1);
            std::map<std::string, std::string>::iterator it = m_disTreeInterestMap.find(tempInName);
            //std::cout<<"tempInName " << tempInName << std::endl;
            if(it != m_disTreeInterestMap.end())
            {
              m_disTreeInterestMap[tempInName] = receivedData;
              m_gotPitNum++;
              //std::cout <<m_prefix.toUri() <<" m_gotPitNum: "<< m_gotPitNum <<std::endl;
            };
            if(m_gotPitNum == m_askPitNum)
            {
              //send Interest to ask FIB nexthop face
              m_askFibPrefix = "/f-";
              //m_askFibPrefix = "/r1-";
              std::string tempAskFib = m_askFibPrefix + m_currentTreeTag;
              SendInterest(tempAskFib);
              //std::cout <<m_prefix.toUri() <<" tempAskFib: "<< tempAskFib <<std::endl;
              //std::cout<< "go for fib round" <<std::endl;
              m_askPitNum = 0;
              m_gotPitNum = 0;
            };
          }
          else
          {
            // only process Interest from selected Up-nei Face, ignore same Interest from other neis
            // std::cout << m_prefix.toUri() <<" current Interest from Face= " << receivedData << std::endl;
            if (receivedData == m_selectNodeFace)
            {
              ProcessNormalInterest(m_normalInterest);
            }
          };
          break;
        };
        case 'f':
        {
          // std::cout << m_prefix.toUri() <<" get fib Data: " << receivedData << std::endl;
          uint64_t rejoinFace = gotDataName.find("rejoin");
          if (rejoinFace == std::string::npos) 
          {
            for(std::map<std::string, std::string>::iterator it=m_disTreeInterestMap.begin(); it!=m_disTreeInterestMap.end(); ++it)
            {
              std::cout << m_prefix.toUri() <<" reply InterestName= " << it->first << std::endl;
              //add this node to check-nei-table as potential nei if select-nei link is broken
              uint64_t p1 = it->first.find_first_of("-");
              uint64_t p2 = it->first.find("/discover");
              std::string upstreamNei = it->first.substr(p1+1, p2-p1-1);
              std::string taskNei =  m_prefix.toUri() + upstreamNei + m_currentTreeTag;

              std::string replyDisName = it->first;
              Name replyDisDataName(replyDisName);
              auto replyDisData = make_shared<Data>();
              replyDisData->setName(replyDisName);
              replyDisData->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
            
              SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));
              if (m_keyLocator.size() > 0) {
                signatureInfo.setKeyLocator(m_keyLocator);
              }
              replyDisData->setSignatureInfo(signatureInfo);
              ::ndn::EncodingEstimator estimator;
              ::ndn::EncodingBuffer encoder(estimator.appendVarNumber(m_signature), 0);
              encoder.appendVarNumber(m_signature);
              replyDisData->setSignatureValue(encoder.getBuffer());
      
              if (it->second != receivedData)
              {
                //current not-selected upstream nei, save as potential neis if current select nei is disconnect
                m_neiReachable.insert(std::pair<std::string, std::string>(taskNei, "false"));
                // std::cout<<m_prefix.toUri() << " potential Nei: " << taskNei <<std::endl;

                std::string rawData("nope");
                const uint8_t* p = reinterpret_cast<const uint8_t*>(rawData.c_str());
                auto buffer = make_shared< ::ndn::Buffer>(p, rawData.size());
                replyDisData->setContent(buffer);
                // to create real wire encoding
                replyDisData->wireEncode();
                m_transmittedDatas(replyDisData, this, m_face);
                m_appLink->onReceiveData(*replyDisData);
                std::cout << m_prefix.toUri() << " reply Nack for disTree: " << rawData << std::endl;
              }
              // map size = 1 && the face = FIBface, got the selected upstream, not reply immediately, continue explore downstreams
              else 
              {
                //current selected upstream nei
                m_selectNodeName = upstreamNei;
                // std::cout<<m_prefix.toUri() << " select Nei: " << taskNei <<std::endl;
                m_selectNodeFace = it->second;
                std::string rawData;
                // if(m_prefix.toUri() == "/m5-"){
                //   //to simulate node join after the job tree has running for a while
                //   m_neiReachable.insert(std::pair<std::string, std::string>(taskNei, "false"));
                //   rawData = "nope";
                // }
                // else {
                //   m_neiReachable.insert(std::pair<std::string, std::string>(taskNei, "true"));
                //   rawData = "yes";
                // };

                m_neiReachable.insert(std::pair<std::string, std::string>(taskNei, "true"));
                rawData = "yes";
                const uint8_t* p = reinterpret_cast<const uint8_t*>(rawData.c_str());
                auto buffer = make_shared< ::ndn::Buffer>(p, rawData.size());
                replyDisData->setContent(buffer);
                
                // to create real wire encoding
                replyDisData->wireEncode();
                m_transmittedDatas(replyDisData, this, m_face);
                m_appLink->onReceiveData(*replyDisData);
                std::cout << m_prefix.toUri() << " reply Yes for disTree: " << rawData << std::endl;
              }
            }
            m_disTreeInterestMap.clear();
          }
          else 
          {
            //get FaceId of rejoin up-nei 
            m_selectNodeFace = receivedData;
            // std::cout << m_prefix.toUri() <<" select upNei Face= " << receivedData << std::endl; 
          };
          break;
        };
      };
    }
}


} // namespace ndn
} // namespace ns3
