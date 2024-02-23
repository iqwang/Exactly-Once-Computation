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

#include "ndn-wq-mr-user.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include <time.h>
#include <stdio.h>
#include <fstream>
#include "helper/ndn-fib-helper.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.WqMrUser");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(WqMrUser);

TypeId
WqMrUser::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::WqMrUser")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<WqMrUser>()

      .AddAttribute("Frequency", "Frequency of interest packets", StringValue("1.0"),
                    MakeDoubleAccessor(&WqMrUser::m_frequency), MakeDoubleChecker<double>())

      .AddAttribute("Randomize",
                    "Type of send time randomization: none (default), uniform, exponential",
                    StringValue("none"),
                    MakeStringAccessor(&WqMrUser::SetRandomize, &WqMrUser::GetRandomize),
                    MakeStringChecker())

      .AddAttribute("MaxSeq", "Maximum sequence number to request",
                    IntegerValue(std::numeric_limits<uint32_t>::max()),
                    MakeIntegerAccessor(&WqMrUser::m_seqMax), MakeIntegerChecker<uint32_t>())
      .AddAttribute("StartSeq", "Initial sequence number", IntegerValue(0),
                    MakeIntegerAccessor(&WqMrUser::m_seq), MakeIntegerChecker<int32_t>())

      .AddAttribute("LifeTime", "LifeTime for interest packet", StringValue("200s"),
                    MakeTimeAccessor(&WqMrUser::m_interestLifeTime), MakeTimeChecker())

      .AddAttribute("RetxTimer",
                    "Timeout defining how frequent retransmission timeouts should be checked",
                    StringValue("50ms"),
                    MakeTimeAccessor(&WqMrUser::GetRetxTimer, &WqMrUser::SetRetxTimer),
                    MakeTimeChecker())

      .AddTraceSource("LastRetransmittedInterestDataDelay",
                      "Delay between last retransmitted Interest and received Data",
                      MakeTraceSourceAccessor(&WqMrUser::m_lastRetransmittedInterestDataDelay),
                      "ns3::ndn::Consumer::LastRetransmittedInterestDataDelayCallback")

      .AddTraceSource("FirstInterestDataDelay",
                      "Delay between first transmitted Interest and received Data",
                      MakeTraceSourceAccessor(&WqMrUser::m_firstInterestDataDelay),
                      "ns3::ndn::Consumer::FirstInterestDataDelayCallback")
          
      .AddAttribute("Prefix", "Prefix, for which producer has the data", StringValue("/"),
                    MakeNameAccessor(&WqMrUser::m_prefix), MakeNameChecker())
      .AddAttribute("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
					StringValue("/"), MakeNameAccessor(&WqMrUser::m_postfix), MakeNameChecker())
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets", UintegerValue(1024),
                    MakeUintegerAccessor(&WqMrUser::m_virtualPayloadSize),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                    TimeValue(Seconds(0)), MakeTimeAccessor(&WqMrUser::m_freshness),
                    MakeTimeChecker())
      .AddAttribute("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
					UintegerValue(0), MakeUintegerAccessor(&WqMrUser::m_signature),
					MakeUintegerChecker<uint32_t>())
      .AddAttribute("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&WqMrUser::m_keyLocator), MakeNameChecker()); 

  return tid;
}

WqMrUser::WqMrUser()
  : m_rand(CreateObject<UniformRandomVariable>())
  , m_firstTime(true)
  , m_seq(0)
  , m_seqMax(0) 
{
  NS_LOG_FUNCTION_NOARGS();
  m_rtt = CreateObject<RttMeanDeviation>();

  // m_seqMax = std::numeric_limits<uint32_t>::max();

  //m_interestName = "/mr--/map.k-/T/H/L/.v-v/10--/reduce.a+b";
  // m_interestName = "/mr--/map(k,v)->(k+1,v/2)-/reduce.a+b";
  //m_interestName = "/mr--/map.k";
  
  //m_interestName = "/r";
  //m_taskContent = "/map.k-k+1.v-v/10--/reduce.a+b";
  // m_taskContent = "/map(k,v)->(k+1,v/2)-/reduce(a,b)->(a+b)";
  m_taskContent = "/func1";
  m_sendJobNeis.clear();
  m_buildTaskNeighbour = false;
  m_disNeiPrefix = "/nei-";
  m_ownPrefix = "/0-";
  m_disDownStream1 = "/discoverTS";
  m_disDownStream2 = "/TE-";
  m_disDownStream3 = "/TS";
  m_seqStart = "/(Seq";
  m_seqEnd = ")-";
}

void
WqMrUser::SetRetxTimer(Time retxTimer)
{
  m_retxTimer = retxTimer;
  if (m_retxEvent.IsRunning()) {
    // m_retxEvent.Cancel (); // cancel any scheduled cleanup events
    Simulator::Remove(m_retxEvent); // slower, but better for memory
  }

  // schedule even with new timeout
  m_retxEvent = Simulator::Schedule(m_retxTimer, &WqMrUser::CheckRetxTimeout, this);
}

Time
WqMrUser::GetRetxTimer() const
{
  return m_retxTimer;
}

void
WqMrUser::CheckRetxTimeout()
{
  Time now = Simulator::Now();

  Time rto = m_rtt->RetransmitTimeout();
  // NS_LOG_DEBUG ("Current RTO: " << rto.ToDouble (Time::S) << "s");

  while (!m_seqTimeouts.empty()) {
    SeqTimeoutsContainer::index<i_timestamp>::type::iterator entry =
      m_seqTimeouts.get<i_timestamp>().begin();
    if (entry->time + rto <= now) // timeout expired?
    {
      uint32_t seqNo = entry->seq;
      m_seqTimeouts.get<i_timestamp>().erase(entry);
      OnTimeout(seqNo);
    }
    else
      break; // nothing else to do. All later packets need not be retransmitted
  }

  m_retxEvent = Simulator::Schedule(m_retxTimer, &WqMrUser::CheckRetxTimeout, this);
}

void
WqMrUser::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);

  SendPacket();
}

void
WqMrUser::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  // cancel periodic packet generation
  Simulator::Cancel(m_sendEvent);

  // cleanup base stuff
  App::StopApplication();
}

void
WqMrUser::SendPacket()
{
	if (!m_active)
	return;
  
	NS_LOG_FUNCTION_NOARGS();
  
  
  // time_t now = std::time(0);
  // struct tm tstruct;
  // tstruct = *localtime(&now);
  // char output[200];
  // strftime(output, sizeof(output), "%Y-%M-%D.%X", &tstruct);
  // std::ofstream recording;
  // recording.open("userTimeRecord.txt", std::ios_base::app);
  // recording << "##############" << std::endl<< "start: " << output << std::endl;
  // recording.close();
  
  std::cout << "Job Neighbour Num: " << m_sendJobNeis.size() <<std::endl;
  std::cout << "One-hop Neighbour Num: " << m_oneHopNeighbours.size() <<std::endl;
   
  if(m_sendJobNeis.size() != 0) 
  {
    //send MapReduce Task
    std::cout << "User Finally!!!" <<std::endl;
    if(m_seqNum == 0) 
    {
      AssignNodeIdByPath();
    }
    else {
      AssignJobs();
    };
  } 
  else if (m_oneHopNeighbours.size() != 0)
  {
    //start build task-tree
    for(uint64_t t=0; t<m_oneHopNeighbours.size(); t++)
    {
      m_downNeiMap.insert(std::pair<std::string, std::string>(m_oneHopNeighbours[t],"0"));
      std::string tempDisTree = m_oneHopNeighbours[t] + m_ownPrefix + m_disDownStream1 + m_ownPrefix + m_disDownStream2;
      //std::string tempDisTree = "/m1-";
      std::cout << "dis tree: " << tempDisTree << std::endl;
      shared_ptr<Name> disTreeName = make_shared<Name>(tempDisTree);
      disTreeName->appendSequenceNumber(m_rand->GetValue(0, std::numeric_limits<uint16_t>::max()));
      shared_ptr<Interest> disTreeInterest = make_shared<Interest>();
      disTreeInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
      disTreeInterest->setName(*disTreeName);
      disTreeInterest->setInterestLifetime(time::milliseconds(m_interestLifeTime.GetMilliSeconds()));
      m_transmittedInterests(disTreeInterest, this, m_face);
      m_appLink->onReceiveInterest(*disTreeInterest);
      m_sendDisDownNeiNum++;
      //ScheduleNextPacket();
    }  
  }
  else
  {
    //start discovery neighbours
    m_sendDisNeiNum = 0;
    for(uint8_t i=0; i < m_allNodeName.size(); i++)
    {
      // std::cout << " m_allNode " << m_allNodeName[i] << std::endl;
      if(m_allNodeName[i] != m_ownPrefix)
      {
        m_checkNeibMap.insert(std::pair<std::string, std::string>(m_allNodeName[i],"0"));
        // written as: /nei-/user-
        std::string tempDisNei = m_disNeiPrefix + m_allNodeName[i];
        shared_ptr<Name> disNeiName = make_shared<Name>(tempDisNei);
        //originalInterest->appendSequenceNumber(m_rand->GetValue(0, std::numeric_limits<uint16_t>::max()));
        shared_ptr<Interest> disNeiInterest = make_shared<Interest>();
        disNeiInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
        disNeiInterest->setName(*disNeiName);
        disNeiInterest->setInterestLifetime(time::milliseconds(m_interestLifeTime.GetMilliSeconds()));
        std::cout << " User disNeiInterest " << disNeiInterest->getName().toUri() << std::endl;
        m_transmittedInterests(disNeiInterest, this, m_face);
        m_appLink->onReceiveInterest(*disNeiInterest);
        m_sendDisNeiNum +=1;
      }
    };
  };
}

void
WqMrUser::ScheduleNextPacket()
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";

  if (m_firstTime) {
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &WqMrUser::SendPacket, this);
    m_firstTime = false;
  }
  else if (!m_sendEvent.IsRunning())
    m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
                                                      : Seconds(m_random->GetValue()),
                                      &WqMrUser::SendPacket, this);
  // printLater.join();
}

void
WqMrUser::SetRandomize(const std::string& value)
{
  if (value == "uniform") {
    m_random = CreateObject<UniformRandomVariable>();
    m_random->SetAttribute("Min", DoubleValue(0.0));
    m_random->SetAttribute("Max", DoubleValue(2 * 1.0 / m_frequency));
  }
  else if (value == "exponential") {
    m_random = CreateObject<ExponentialRandomVariable>();
    m_random->SetAttribute("Mean", DoubleValue(1.0 / m_frequency));
    m_random->SetAttribute("Bound", DoubleValue(50 * 1.0 / m_frequency));
  }
  else
    m_random = 0;

  m_randomType = value;
}

std::string
WqMrUser::GetRandomize() const
{
  return m_randomType;
}

void
WqMrUser::AssignNodeIdByPath()
{
  //assign a path-based ID to each job_Nei 
  int i = 0;
  for(uint64_t j=0; j<m_sendJobNeis.size(); j++)
  {
    std::string assignId = std::to_string(i);
    // std::cout << m_prefix.toUri() << " assign node_path_ID = " << assignId << std::endl;
    std::map<std::string, std::string>::iterator checkId = m_nodePathId.find(assignId);
    if(checkId == m_nodePathId.end()) {
      m_nodePathId.insert(std::pair<std::string, std::string>(m_sendJobNeis[j], assignId));
    }
    else {
      std::cout << m_prefix.toUri() << " node_path_ID already existing " << std::endl;
    };
    i++;
  };
  // for(auto& x: m_nodePathId) {
  //   std::cout << m_prefix.toUri() << " nodeName= " << x.first << " pathID= "<< x.second << std::endl;
  // };

  //tell assined ID to each job_nei
  for(uint64_t j=0; j<m_sendJobNeis.size(); j++)
  {
    std::map<std::string, std::string>::iterator checkNei = m_nodePathId.find(m_sendJobNeis[j]);
    if(checkNei != m_nodePathId.end()) {
      std::string pathId = checkNei->second;
      std::string tellPathId = m_sendJobNeis[j] + m_disDownStream3 + m_ownPrefix + m_disDownStream2 + "/pathID("
                          + pathId + ")-";
      std::cout << m_prefix.toUri() << " tellPathId = " << tellPathId << std::endl;
      shared_ptr<Name> taskName = make_shared<Name>(tellPathId);
      taskName->appendSequenceNumber(m_rand->GetValue(0, std::numeric_limits<uint16_t>::max()));
      shared_ptr<Interest> taskInterest = make_shared<Interest>();
      taskInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
      taskInterest->setName(*taskName);
      taskInterest->setInterestLifetime(time::milliseconds(m_interestLifeTime.GetMilliSeconds()));
      m_transmittedInterests(taskInterest, this, m_face);
      m_appLink->onReceiveInterest(*taskInterest);
    }
    else {
      std::cout << m_prefix.toUri() << " node has no path-based ID " << std::endl;
    };

  }
}

void
WqMrUser::AssignJobs()
{
  m_seqNum += 1;
  std::string seqStr = std::to_string(m_seqNum);
  std::string seqFlag = "Seq" + seqStr;
  std::map<std::string, int>::iterator checkJobSeq = m_assignJobSeq.find(seqFlag);
  if (checkJobSeq == m_assignJobSeq.end()) {
    m_assignJobSeq.insert(std::pair<std::string, int>(seqFlag, 0));
  }
  // std::cout << "User assign Seq length " <<  m_assignJobSeq.size() << std::endl;
  int i = 0;
  for(uint64_t j=0; j<m_sendJobNeis.size(); j++)
  {
    std::string taskString = m_sendJobNeis[j] + m_disDownStream3 + m_ownPrefix + m_disDownStream2 + m_taskContent 
                              + "-" + m_seqStart + seqStr + m_seqEnd;
    std::cout << "Assign task: " << taskString << std::endl;
    shared_ptr<Name> taskName = make_shared<Name>(taskString);
    taskName->appendSequenceNumber(m_rand->GetValue(0, std::numeric_limits<uint16_t>::max()));
    shared_ptr<Interest> taskInterest = make_shared<Interest>();
    taskInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
    taskInterest->setName(*taskName);
    taskInterest->setInterestLifetime(time::milliseconds(m_interestLifeTime.GetMilliSeconds()));
    m_transmittedInterests(taskInterest, this, m_face);
    m_appLink->onReceiveInterest(*taskInterest);
    i++;
    ScheduleNextPacket();
  }
  m_assignJobSeq[seqFlag] = i;
  // std::cout << "222222 User assign Seq= " << seqFlag << " num=" << m_assignJobSeq[seqFlag] << std::endl;

}

void
WqMrUser::ReplyData(std::string replyContent, std::string replyName)
{
  Name dataName(replyName);
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
  std::cout <<m_prefix.toUri() << " reply: " << replyContent << std::endl;
}

void
WqMrUser::SendOutInterest(std::string newInterest)
{
  shared_ptr<Name> sendName = make_shared<Name>(newInterest);
  sendName->appendSequenceNumber(m_rand->GetValue(0, std::numeric_limits<uint16_t>::max()));
  shared_ptr<Interest> sendInterest = make_shared<Interest>();
  sendInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  sendInterest->setName(*sendName);
  sendInterest->setInterestLifetime(ndn::time::seconds(1));
  m_transmittedInterests(sendInterest, this, m_face);
  m_appLink->onReceiveInterest(*sendInterest);
}

void
WqMrUser::CheckSeqAtReducer(std::string reducerName, std::string checkSeq)
{
  std::string askReducerInterest = reducerName + "-/" + checkSeq;
  std::cout << " User checkReducerInterest= " << askReducerInterest << std::endl;
  SendOutInterest(askReducerInterest);
}

void
WqMrUser::ResentDataCheck(std::string resentSeq)
{
  std::map<std::string, int>::iterator checkJobSeq = m_receiveJobSeq.find(resentSeq);
  if (checkJobSeq == m_receiveJobSeq.end()) {
    m_receiveJobSeq.insert(std::pair<std::string, int>(resentSeq, 1));
  }
  else {
    m_receiveJobSeq[resentSeq] += 1;
  };
  // std::cout << "AssignNum= " << m_assignJobSeq.at(resentSeq) << " ReceiveNum= " <<  m_receiveJobSeq.at(resentSeq) << std::endl;
  std::map<std::string, int>::iterator compareSeq = m_assignJobSeq.find(resentSeq);
  if (compareSeq != m_assignJobSeq.end()) {
    if (m_assignJobSeq.at(resentSeq) == m_receiveJobSeq.at(resentSeq)) {
      m_seqOkList.push_back(resentSeq);
    };
  };
}

void
WqMrUser::OnData(shared_ptr<const Data> data)
{
  //parse data content
    auto *tmpContent = ((uint8_t*)data->getContent().value());
    std::string receivedData;
    for(uint8_t i=0; i < data->getContent().value_size(); i++) {
      receivedData.push_back((char)tmpContent[i]);
    };
    // std::cout << "User Receive Data: " << receivedData << " from " << data->getName().toUri() << std::endl;
    std::string gotData = data->getName().toUri();
    uint64_t checkD= gotData.find("/discover");
    uint64_t d = gotData.find("doubt");
    uint64_t p = gotData.find("pathID");
    uint64_t process = gotData.find("process");
    uint64_t clear = gotData.find("clear");

    // std::map<std::string, std::string>::iterator i;
    // for(i=m_checkNeibMap.begin(); i != m_checkNeibMap.end(); ++i) {
    //   std::cout << "check nei: " << i -> first << std::endl;
    // }
    // std::cout << "OnData check nei size: " << m_checkNeibMap.size() << std::endl;
    
    if(gotData[1] == 'n')
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
        
        if(m_sendDisNeiNum == m_gotDisNeiNum) 
        { 
          std::cout << "receive all disNeiInterests" <<std::endl;
          for(it=m_checkNeibMap.begin(); it != m_checkNeibMap.end(); ++it)
          {
            if(it->second == "1")
            { 
              m_oneHopNeighbours.push_back(it->first);
              //std::cout << "it -> first: " << it->first <<std::endl;
            }
          }
          std::cout << "next-hop neighbour number: " << m_oneHopNeighbours.size() <<std::endl;
          m_checkNeibMap.clear();
          m_gotDisNeiNum = 0;
          SendPacket();
        }
    }
    else if(checkD != std::string::npos)
    {
      uint64_t temp1 = gotData.find("-");
      std::string temp2 = gotData.substr(0,temp1+1);
      std::map<std::string, std::string>::iterator downIt = m_downNeiMap.find(temp2);
      if(downIt != m_downNeiMap.end())
      {
        m_downNeiMap[temp2] = receivedData;
        m_gotDisDownNeiNum++;
      }
      
      if(m_gotDisDownNeiNum == m_sendDisDownNeiNum)
      {
        for(downIt=m_downNeiMap.begin(); downIt != m_downNeiMap.end(); ++downIt)
        {
          if(downIt->second == "yes")
          {
            //m_sendJobNeis += downIt->first;
            m_sendJobNeis.push_back(downIt->first);
          }
        }
        std::cout << "job Ref Neighbours: " << m_sendJobNeis.size() << std::endl;
        SendPacket();
        m_downNeiMap.clear();
        m_gotDisDownNeiNum = m_sendDisDownNeiNum =0;
      }
    }
    // reducer reply for seq-check
    else if (d != std::string::npos)
    {
      std::cout << "User Receive reducer-check-reply: " << receivedData << std::endl;
      if (receivedData == "Not-receive")
      {
        uint64_t p1 = gotData.find("(");
        uint64_t p2 = gotData.find(")");
        std::string pathId = gotData.substr(p1+1, p2-p1-1);
        uint64_t t = gotData.find("T");
        uint64_t s = gotData.find("/id");
        std::string reSeq = gotData.substr(t, s-t-1);
        // std::cout << "reply-to-node: " << replyToNode << std::endl;
        std::map<std::string, std::string>::iterator r = m_doubtCheckInterest.find(pathId);
        if (r != m_doubtCheckInterest.end()) 
        {
          std::string rawReply = "Not-receive";
          ReplyData(rawReply, m_doubtCheckInterest.at(pathId));
        };

        std::string toReducer = gotData.substr(0, d-2);
        std::string notifyReducerInterest = toReducer + "-/process-" + reSeq + "-/except-(" + pathId + ")-" + "hop1-";
        std::cout << " User notifyReducerInterest= " << notifyReducerInterest << std::endl;
        SendOutInterest(notifyReducerInterest);
      }
      else if (receivedData == "Already-receive")
      {
        std::cout << "Doubt-seq-data already processed by reducer" << std::endl;
      };
    }
    //reply for assign path-based ID
    else if (p != std::string::npos)
    {
      std::cout << "User got ACK: " << receivedData << std::endl;
      //start to assign jobs after path id assignment is done
      AssignJobs();
    }
    //reply for process data without disconnect nei
    else if (process != std::string::npos)
    {
      std::cout << "User got Reply: " << receivedData << std::endl;
    }
    //reply for clear history data
    else if (clear != std::string::npos)
    {
      std::cout << "User got Reply: " << receivedData << std::endl;
    }
    // normal data
    else
    {
      std::cout << "User Receive Data: " << receivedData << std::endl;
      uint64_t s1 = receivedData.find("Seq");
      uint64_t s2 = receivedData.find("-");
      std::string gotSeq = receivedData.substr(s1, s2-s1);
      std::string gotResult = receivedData.substr(s2+1);
      // std::cout << "User Receive Seq= " << gotSeq << std::endl;

      std::map<std::string, int>::iterator checkJobSeq = m_receiveJobSeq.find(gotSeq);
      if (checkJobSeq == m_receiveJobSeq.end()) {
        m_receiveJobSeq.insert(std::pair<std::string, int>(gotSeq, 1));
        std::map<std::string, std::string>::iterator check2 = m_receiveSeqData.find(gotSeq);
        if(check2 == m_receiveSeqData.end())
        {
          m_receiveSeqData.insert(std::pair<std::string, std::string>(gotSeq, gotResult));
        }
        else
        {
          m_receiveSeqData.at(gotSeq) = m_receiveSeqData.at(gotSeq) + ";" + gotResult;
        };
        // std::cout << "AllData-SameSeq = " << m_receiveSeqData.at(gotSeq) << std::endl;
      }
      else {
        checkJobSeq->second += 1;
        m_receiveSeqData.at(gotSeq) = m_receiveSeqData.at(gotSeq) + ";" + gotResult;
        // std::cout << "AllData-SameSeq = " << m_receiveSeqData.at(gotSeq) << std::endl;
      };

      std::map<std::string, int>::iterator compareSeq = m_assignJobSeq.find(gotSeq);
      if (compareSeq != m_assignJobSeq.end()) 
      {
        if (m_assignJobSeq.at(gotSeq) == m_receiveJobSeq.at(gotSeq)) 
        {
          // std::cout << "User Receive==Sent Seq= " << gotSeq << " and num=" << m_receiveJobSeq.at(gotSeq)  << std::endl;
          for (auto x: m_seqOkList) {
            std::cout << "User Receive==Sent Seq List: " << x << std::endl;
          };
          // for (auto x: m_receiveSeqData) {
          //   std::cout << "User AllReceiveSeq= " << x.first << " Data= " << x.second << std::endl;
          // };
          m_seqOkList.push_back(gotSeq);
          std::ofstream recording;
          recording.open("computeStateRecord.txt", std::ios_base::app);
          recording << Simulator::Now().GetSeconds() << '\t' << m_prefix.toUri() << '\t' << m_seqOkList.size() << std::endl;
          recording.close();

          m_countOkSeq++;
          if(m_countOkSeq == 2000) {
            // for (auto x: m_seqOkList) {
            //   std::cout << "User Receive==Sent Seq List: " << x << std::endl;
            // };
            std::string notifySeqs = "";
            for(uint64_t h=0; h<10; h++) {
              notifySeqs = notifySeqs + m_seqOkList[h] + "/";
              m_askClearSeqs.push_back(m_seqOkList[h]);
            };
            for(uint64_t n=0; n<m_oneHopNeighbours.size(); n++)
            {
              std::string notifyClearSeq = m_oneHopNeighbours[n] + "/clear-" + notifySeqs + "-";
              std::cout << "User notify to clear-Seqs " << notifyClearSeq << std::endl;
              SendOutInterest(notifyClearSeq);
            };
            m_seqOkList.erase(m_seqOkList.begin(), m_seqOkList.begin()+20);
            m_countOkSeq=0;

            std::ofstream recording;
            recording.open("computeStateRecord.txt", std::ios_base::app);
            recording << Simulator::Now().GetSeconds() << '\t' << m_prefix.toUri() << '\t' << m_seqOkList.size() << std::endl;
            recording.close();
          };
        }
      };
      
    }
}

void
WqMrUser::OnInterest(shared_ptr<const Interest> interest)
{

  App::OnInterest(interest); // tracing inside
  NS_LOG_FUNCTION(this << interest);
  if (!m_active) {return;}
    
  uint64_t d = interest->getName().toUri().find("/doubt");
  uint64_t r = interest->getName().toUri().find("/resend");
  uint64_t leave = interest->getName().toUri().find("leave");
  uint64_t b = interest->getName().toUri().find("backTree");

  // node check fail seq&data
  if(d != std::string::npos)
  {
    std::cout << " !!! user !!! got Interest: " << interest->getName().toUri() <<std::endl;
    uint64_t s1 = interest->getName().toUri().find("Seq");
    uint64_t s2 = interest->getName().toUri().find("/id");
    uint64_t p1 = interest->getName().toUri().find("(");
    uint64_t p2 = interest->getName().toUri().find(")");
    std::string doubtSeq = interest->getName().toUri().substr(s1, s2-s1-1);
    std::string d_nodePathId = interest->getName().toUri().substr(p1+1, p2-p1-1);
    std::string directNeiId = d_nodePathId.substr(0,1);
    // std::cout << " User receive doubt seq= " << doubtSeq << " neiID= " << directNeiId << std::endl;
    std::map<std::string, std::string>::iterator findId;
    for(findId=m_nodePathId.begin(); findId!=m_nodePathId.end(); findId++)
    {
      if(findId->second == directNeiId)
      {
        std::string directNeiName = findId->first;
        // std::cout << " path nei name= " << directNeiName << std::endl;
        std::string checkInterest = interest->getName().toUri().substr(d, p2-d+2);
        checkInterest = directNeiName + checkInterest + "hop1-";
        SendOutInterest(checkInterest);
        std::cout << " forward pathID interest: " << checkInterest << std::endl;
      };
    };
   
    std::map<std::string, std::string>::iterator n1 = m_doubtCheckInterest.find(d_nodePathId);
    if (n1 == m_doubtCheckInterest.end()) 
    {
      m_doubtCheckInterest.insert(std::pair<std::string, std::string>(d_nodePathId, interest->getName().toUri()));
    };
  }
  //got resend-data from previous disconnectd node
  else if(r != std::string::npos)
  {
    std::cout << " !!! user got Resend-Seq: " << interest->getName().toUri() <<std::endl;
    uint64_t s1 = interest->getName().toUri().find("Seq");
    uint64_t s2 = interest->getName().toUri().find_last_of("/");
    std::string reSeqData = interest->getName().toUri().substr(s1, s2-s1-1);
    uint64_t s3 = reSeqData.find("-");
    std::string reseq = reSeqData.substr(0,s3);
    std::string redata = reSeqData.substr(s3+1);
    // std::cout << " seq= " << reseq << " data= " << redata <<std::endl;
    std::map<std::string, std::string>::iterator checkSeq = m_receiveSeqData.find(reseq);
    if (checkSeq == m_receiveSeqData.end()) {
      m_receiveSeqData.insert(std::pair<std::string, std::string>(reseq, redata));
    }
    else {
      m_receiveSeqData.at(reseq) = m_receiveSeqData.at(reseq) + ";" + redata;
      // std::cout << "receiveSeqData = " << m_receiveSeqData.at(reseq) << std::endl;
    };
    ResentDataCheck(reseq);
    std::string reAck = "reSendOK";
    ReplyData(reAck, interest->getName().toUri());
  }
  else if(leave != std::string::npos)
  {
    std::cout << " User got Leave-Tree_Mes: " << interest->getName().toUri() <<std::endl;
    uint64_t l1 = interest->getName().toUri().find("(");
    uint64_t l2 = interest->getName().toUri().find(")");
    std::string leaveNode = interest->getName().toUri().substr(l1+1, l2-l1-1);
    int leavePos = 0;
    for(int i=0; i<m_sendJobNeis.size(); i++)
    {
      if(m_sendJobNeis[i] == leaveNode) {
        leavePos = i;
      };
    };
    m_sendJobNeis.erase(m_sendJobNeis.begin()+leavePos);
    ReplyData("Leave-Tree-Done", interest->getName().toUri());
    // for(int i=0; i<m_sendJobNeis.size(); i++)
    // {
    //   std::cout << " job-nei=" << m_sendJobNeis[i] <<std::endl;
    // };
  }
  else if(b != std::string::npos)
  {
    std::cout << " User got rejoin-request: " << interest->getName().toUri() <<std::endl;
    uint64_t r = interest->getName().toUri().find("TS");
    std::string rejoinNode = interest->getName().toUri().substr(b+9, r-b-10);
    // std::cout << " rejoin-node= " << rejoinNode <<std::endl;
    m_sendJobNeis.push_back(rejoinNode);
    ReplyData("Rejoin-Ok", interest->getName().toUri());
  };
}

void
WqMrUser::OnTimeout(uint32_t sequenceNumber)
{
  NS_LOG_FUNCTION(sequenceNumber);
  // std::cout << Simulator::Now () << ", TO: " << sequenceNumber << ", current RTO: " <<
  // m_rtt->RetransmitTimeout ().ToDouble (Time::S) << "s\n";

  m_rtt->IncreaseMultiplier(); // Double the next RTO
  m_rtt->SentSeq(SequenceNumber32(sequenceNumber),
                 1); // make sure to disable RTT calculation for this sample
  m_retxSeqs.insert(sequenceNumber);
  ScheduleNextPacket();
}

} // namespace ndn
} // namespace ns3
