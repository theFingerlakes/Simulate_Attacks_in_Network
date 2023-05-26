/*
ENSC 894 Communication Networks
Simulation script for black hole attack
The code is adapted from src/examples/routing/manet-routing-compare.cc
The modifications are indicated by "894"
*/
#include <fstream>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;
using namespace dsr;

NS_LOG_COMPONENT_DEFINE ("blackhole-aodv");
/*
ENSC 894 Communication Networks
define the number of mobile nodes and gray hole nodes
*/
int nWifis;
int nmalicious;
/* 
ENSC 894 Communication Networks
define the prefix of the output files name
*/
std::string FILENAME_PREFIX;
/*
ENSC 894 Communication Networks
define the 5 kinds of scenarios where the number of mobile nodes range from 20 to 60 with step 10
*/ 
class RoutingExperiment
{
public:
  RoutingExperiment ();
  void Run (int nSinks, double txp, std::string CSVfileName);
  std::string CommandSetup (int argc, char **argv);

private:
  Ptr<Socket> SetupPacketReceive (Ipv4Address addr, Ptr<Node> node);
  void ReceivePacket (Ptr<Socket> socket);
  void CheckThroughput ();

  uint32_t port;
  uint32_t bytesTotal;
  uint32_t packetsReceived;

  std::string m_CSVfileName;
  int m_nSinks;
  std::string m_protocolName;
  double m_txp;
  bool m_traceMobility;
  uint32_t m_protocol;
};

RoutingExperiment::RoutingExperiment ()
  : port (9),
    bytesTotal (0),
    packetsReceived (0),
    m_CSVfileName (FILENAME_PREFIX + ".csv"),
    m_traceMobility (false),
    m_protocol (2) // AODV
{
}

static inline std::string
PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();

  if (InetSocketAddress::IsMatchingType (senderAddress))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (senderAddress);
      oss << " received one packet from " << addr.GetIpv4 ();
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
}

void
RoutingExperiment::ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address senderAddress;
  while ((packet = socket->RecvFrom (senderAddress)))
    {
      bytesTotal += packet->GetSize ();
      packetsReceived += 1;
      NS_LOG_UNCOND (PrintReceivedPacket (socket, packet, senderAddress));
    }
}

void
RoutingExperiment::CheckThroughput ()
{
  double kbs = (bytesTotal * 8.0) / 1000;
  bytesTotal = 0;

  std::ofstream out (m_CSVfileName.c_str (), std::ios::app);

  out << (Simulator::Now ()).GetSeconds () << ","
      << kbs << ","
      << packetsReceived << ","
      << m_nSinks << ","
      << m_protocolName << ","
      << m_txp << ""
      << std::endl;

  out.close ();
  packetsReceived = 0;
  Simulator::Schedule (Seconds (1.0), &RoutingExperiment::CheckThroughput, this);
}

Ptr<Socket>
RoutingExperiment::SetupPacketReceive (Ipv4Address addr, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, port);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&RoutingExperiment::ReceivePacket, this));

  return sink;
}

std::string
RoutingExperiment::CommandSetup (int argc, char **argv)
{
  CommandLine cmd (__FILE__);
  cmd.AddValue ("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
  cmd.AddValue ("traceMobility", "Enable mobility tracing", m_traceMobility);
  cmd.AddValue ("protocol", "1=OLSR;2=AODV;3=DSDV;4=DSR", m_protocol);
  cmd.Parse (argc, argv);
  return m_CSVfileName;
}

int
main (int argc, char *argv[])
{
  /*
  ENSC 894 Communication Networks
  set the number of mobile nodes in 5 kinds of scenarios
  */
  node_list[0] = 20;
  node_list[1] = 30;
  node_list[2] = 40;
  node_list[3] = 50;
  node_list[4] = 60;
	for(int i =0; i<5; i++){
	  for(int j=0;j<4; j++){
      /*
        ENSC 894 Communication Networks
        set parameters for this specific scenario
      */
		  nWifis=node_list[i];
		  nmalicious=j;
        /*
        ENSC 894 Communication Networks
        set the file prefix for this scenario
      */
		  FILENAME_PREFIX = "blackhole-" + std::to_string(nWifis) + "+" + std::to_string(nmalicious);
      std::cout <<"Enter" << FILENAME_PREFIX;
		  RoutingExperiment experiment;
                  std::string CSVfileName = experiment.CommandSetup (argc,argv);
                  //blank out the last output file and write the column headers
                  std::ofstream out (CSVfileName.c_str ());
                  out << "SimulationSecond," <<
                  "ReceiveRate," <<
                  "PacketsReceived," <<
                  "NumberOfSinks," <<
                  "RoutingProtocol," <<
                  "TransmissionPower" <<
                  std::endl;
                  out.close ();
                  /*
                    ENSC 894 Communication Networks
                    set nSinks to half of the number of mobile nodes
                  */
                  int nSinks = nWifis/2;
                  double txp = 7.5;

                experiment.Run (nSinks, txp, CSVfileName);
	  }
  }

}

void
RoutingExperiment::Run (int nSinks, double txp, std::string CSVfileName)
{
  Packet::EnablePrinting ();
  m_nSinks = nSinks;
  m_txp = txp;
  m_CSVfileName = CSVfileName;

  double TotalTime = 100.0;
  std::string rate ("2048bps");
  std::string phyMode ("DsssRate11Mbps");
  std::string tr_name (FILENAME_PREFIX + "manet-routing-compare");
  int nodeSpeed = 20; //in m/s
  int nodePause = 0; //in s
  m_protocolName = "protocol";

  Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue ("64"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (rate));

  //Set Non-unicastMode rate to unicast mode
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));

   /*
  ENSC 894 Communication Networks
  create nodes for mobile nodes and malicious nodes
  */
  NodeContainer adhocNodes;
  adhocNodes.Create(nWifis+nmalicious);
  NodeContainer not_malicious;
  for(int i = nmalicious; i < nWifis + nmalicious; i++){
    not_malicious.Add(adhocNodes.Get(i));
  }
  NodeContainer malicious;
  for(int i = 0; i < nmalicious; i++){
    malicious.Add(adhocNodes.Get(i));
  }

  // setting up wifi phy and channel using helpers
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy;
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

  wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));

  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer adhocDevices = wifi.Install (wifiPhy, wifiMac, adhocNodes);

  MobilityHelper mobilityAdhoc;
  int64_t streamIndex = 0; // used to get consistent mobility across scenarios

  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
  pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));

  Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
  streamIndex += taPositionAlloc->AssignStreams (streamIndex);

  std::stringstream ssSpeed;
  ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
  std::stringstream ssPause;
  ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
  mobilityAdhoc.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                  "Speed", StringValue (ssSpeed.str ()),
                                  "Pause", StringValue (ssPause.str ()),
                                  "PositionAllocator", PointerValue (taPositionAlloc));
  mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
  mobilityAdhoc.Install (not_malicious);
  streamIndex += mobilityAdhoc.AssignStreams (adhocNodes, streamIndex);
  NS_UNUSED (streamIndex); // From this point, streamIndex is unused
  
  	    
  /*
  ENSC 894 Communication Networks
  set position for malicious nodes according the total number of the malicious nodes
  */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
  if(nmalicious == 1){
        positionAlloc ->Add(Vector(150, 750, 0)); // node0
  }else if(nmalicious == 2){
        positionAlloc ->Add(Vector(150, 500, 0)); // node0
        positionAlloc ->Add(Vector(150, 1000, 0)); // node1
  }else if(nmalicious == 3){
        positionAlloc ->Add(Vector(150, 375, 0)); // node0
        positionAlloc ->Add(Vector(150, 750, 0)); // node1
        positionAlloc ->Add(Vector(150, 1125, 0)); // node2
  }
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(malicious);
  
  AodvHelper aodv;
  AodvHelper malicious_aodv;
  // Set up internet stack
  InternetStackHelper internet;
  internet.SetRoutingHelper (aodv);
  internet.Install (not_malicious);

   /*
  ENSC 894 Communication Networks
  install attack-enabled protocl with malicious nodes
  */
  malicious_aodv.Set("EnableBlackAttack",BooleanValue(true)); // putting *false* instead of *true* would disable the malicious behavior of the node
  internet.SetRoutingHelper (malicious_aodv);
  internet.Install (malicious);

  NS_LOG_INFO ("assigning ip address");

  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer adhocInterfaces;
  adhocInterfaces = addressAdhoc.Assign (adhocDevices);

  OnOffHelper onoff1 ("ns3::UdpSocketFactory",Address ());
  onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));

  /*
  ENSC 894 Communication Networks
  set up connection for mobile nodes; if there are 20 mobile nodes, node 0 is connected to node 10, node 1 is connected to 11...
  */
  for (int i = nmalicious; i < nSinks+nmalicious; i++)
    {
      Ptr<Socket> sink = SetupPacketReceive (adhocInterfaces.GetAddress (i), adhocNodes.Get (i));

      AddressValue remoteAddress (InetSocketAddress (adhocInterfaces.GetAddress (i), port));
      onoff1.SetAttribute ("Remote", remoteAddress);

      Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
      ApplicationContainer temp = onoff1.Install (adhocNodes.Get (i + nSinks));
      temp.Start (Seconds (var->GetValue (50.0,51.0)));
      temp.Stop (Seconds (TotalTime));
    }

  std::stringstream ss;
  ss << nWifis;
  std::string nodes = ss.str ();

  std::stringstream ss2;
  ss2 << nodeSpeed;
  std::string sNodeSpeed = ss2.str ();

  std::stringstream ss3;
  ss3 << nodePause;
  std::string sNodePause = ss3.str ();

  std::stringstream ss4;
  ss4 << rate;
  std::string sRate = ss4.str ();

  Ptr<FlowMonitor> flowmon;
  FlowMonitorHelper flowmonHelper;
  flowmon = flowmonHelper.InstallAll ();


  NS_LOG_INFO ("Run Simulation.");
  CheckThroughput ();

  Simulator::Stop (Seconds (TotalTime));
   /*
  ENSC 894 Communication Networks
  generate animation file
  */
  AnimationInterface anim (FILENAME_PREFIX + "animation.xml"); 
  for (uint32_t i = 0; i < not_malicious.GetN (); ++i)
    {
      anim.UpdateNodeDescription (not_malicious.Get (i), "N"); 
      anim.UpdateNodeColor (not_malicious.Get (i), 0, 255, 0); 
    }
  for (uint32_t i = 0; i < malicious.GetN (); ++i)
    {
      anim.UpdateNodeDescription (malicious.Get (i), "B"); 
      anim.UpdateNodeColor (malicious.Get (i), 255, 0, 0); 
    }
  /*
  ENSC 894 Communication Networks
  generate routing file
  */
  anim.EnableIpv4RouteTracking (FILENAME_PREFIX + "route.xml", Seconds (50), Seconds (100), Seconds (10)); //Optional

  Simulator::Run ();
  /*
  ENSC 894 Communication Networks
  generate flow monitor file
  */
  flowmon->SerializeToXmlFile ((tr_name + ".flowmon").c_str(), true, true);

  Simulator::Destroy ();
}

