
#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/point-to-point-dumbbell.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("p1");

int
main (int argc, char *argv[])
{
  SeedManager::SetSeed (11223344);
 
 
  uint32_t maxBytes = 100000000;
  uint32_t queueSize;
  uint32_t segSize;
  uint32_t windowSize;
  uint16_t nFlows=1;
  std::string TCPFlavor;	
  uint32_t tcpType;

  CommandLine cmd;
  cmd.AddValue ("nFlows", "Are there 1 or 10 flows", nFlows);	
  cmd.AddValue ("queueSize", "Maximum Bytes allowed in queue", queueSize);
  cmd.AddValue ("segSize", "Maximum TCP segment size", segSize);
  cmd.AddValue ("windowSize", "Maximum receiver advertised window size", windowSize);
  cmd.AddValue ("tcpType", "TCP type", tcpType);//Tahoe or Reno
   
cmd.Parse (argc, argv);
Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (segSize)); 	
  Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (windowSize));

if (tcpType==0)
	TCPFlavor="Tahoe";
else if (tcpType==1)
	TCPFlavor="Reno";

std::string temp = "ns3::Tcp"+TCPFlavor;
 Config::SetDefault("ns3::TcpL4Protocol::SocketType",StringValue(temp));	
Config::SetDefault ("ns3::DropTailQueue::Mode", StringValue ("QUEUE_MODE_BYTES"));
Config::SetDefault ("ns3::DropTailQueue::MaxBytes", UintegerValue (queueSize));
Config::SetDefault ("ns3::TcpSocketBase::WindowScaling", BooleanValue(false));

 PointToPointHelper pointToPointRouter;
 pointToPointRouter.SetDeviceAttribute  ("DataRate", StringValue ("1Mbps"));
 pointToPointRouter.SetChannelAttribute ("Delay", StringValue ("20ms"));
 pointToPointRouter.SetQueue ("ns3::DropTailQueue","MaxBytes",UintegerValue(queueSize));
 PointToPointHelper pointToPointLeaf;
 pointToPointLeaf.SetDeviceAttribute    ("DataRate", StringValue ("5Mbps"));
 pointToPointLeaf.SetChannelAttribute   ("Delay", StringValue ("10ms"));
  
 PointToPointDumbbellHelper d (nFlows, pointToPointLeaf,
                                   nFlows, pointToPointLeaf,
                                   pointToPointRouter);
   
      // Install Stack
      InternetStackHelper stack;
      d.InstallStack (stack);
    
      // Assign IP Addresses
      d.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                             Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                             Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));
    

 	
 NS_LOG_INFO("Populate Routing Tables"); 
  Ipv4GlobalRoutingHelper::PopulateRoutingTables(); 	

  NS_LOG_INFO ("Create Applications.");


 

  uint16_t port = 9;  // well-known echo port number
std::vector<Ptr<PacketSink> > sinks;
std::vector<double> starttime; 
double min = 0.0;
  double max = 0.1;
 Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
x->SetAttribute ("Min", DoubleValue (min));
x->SetAttribute ("Max", DoubleValue (max));

x->SetAttribute("Stream", IntegerValue(6110));
 for (uint16_t i=0; i<nFlows; i++)
 {
  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (d.GetRightIpv4Address(i), port+i));

  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  source.SetAttribute ("SendSize", UintegerValue (segSize));	
  ApplicationContainer sourceApps;
  sourceApps.Add(source.Install (d.GetLeft (i)));

 
double val = x->GetValue();
//std::cout<<val<<std::endl;

starttime.push_back(val);
sourceApps.Start (Seconds (val));
sourceApps.Stop (Seconds (10.0));

  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port+i));
  //AddressValue addr(InetSocketAddress(d.GetRightIpv4Address(i), port+i));
  //sink.SetAttribute("local", addr);
  ApplicationContainer sinkApps;
  sinkApps.Add(sink.Install(d.GetRight(i)));
  sinkApps.Start (Seconds (val));
  sinkApps.Stop (Seconds (10.0));
sinks.push_back(DynamicCast<PacketSink> (sinkApps.Get (0)));
 }

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();

for (uint16_t j=0; j<nFlows; j++)
{

	std::cout<<"tcp,"<<tcpType; 
	std::cout<<",flow,"<<j<<",";
	std::cout<<"windowSize,"<<windowSize<<","<<"queueSize,"<<queueSize<<","<<"segSize,"<<segSize<<",";
	Ptr<PacketSink> sink1 = sinks.at(j);
	std::cout<<"goodput,"<<sink1->GetTotalRx()/(10.0-starttime.at(j))<<std::endl;
	
}


/**********************************/

  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");



}
