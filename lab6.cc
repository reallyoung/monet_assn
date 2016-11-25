#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"

NS_LOG_COMPONENT_DEFINE("labExample");

using namespace ns3;

void ReceivePacket(Ptr<Socket> socket) {
	static int num =1;
  NS_LOG_UNCOND("Received one packet!");
   Ptr<Packet> packet = socket->Recv ();
   SocketIpTosTag tosTag;
   uint8_t data[packet->GetSize()];
   packet->CopyData(data,1);
   NS_LOG_UNCOND("Total recived packet for all node: "<< num++);
   NS_LOG_UNCOND("this node recived : "<<(uint32_t)data[0]<<" packets");

}

static void GenerateTraffic(Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval) {
	static uint8_t pn=pktCount;
	uint8_t data[pktSize];	
	data[0]=(uint8_t)(pn-pktCount+1);
//	NS_LOG_UNCOND("pn:"<< (uint32_t)data[0]);
  if(pktCount > 0) { 
		Ptr<Packet> p = Create<Packet>(data,pktSize);
      socket->Send(p);
      Simulator::Schedule(pktInterval, &GenerateTraffic, socket, pktSize,pktCount-1, pktInterval);
    }
  else
      socket->Close();
}

int main(int argc, char *argv[]) {
  std::string phyMode("DsssRate1Mbps");
  int nodeNumber = 2;
  uint32_t packetNumber = 40;
  
  CommandLine cmd;
  cmd.AddValue("nodeNumber", "Number of nodes", nodeNumber);
  cmd.AddValue("packetNumber", "Number of packet", packetNumber);
  cmd.Parse(argc,argv);  

  Time interPacketInterval = Seconds(1.0);

  NodeContainer c;

  c.Create(nodeNumber);
  NS_LOG_UNCOND("Create Nodes!");

  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default();
  wifiPhy.Set("RxGain", DoubleValue(0)); 
  wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::FixedRssLossModel","Rss", DoubleValue(-80));
  wifiPhy.SetChannel(wifiChannel.Create());

  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode",StringValue(phyMode), "ControlMode",StringValue(phyMode));
  
  wifiMac.SetType("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, c);
  NS_LOG_UNCOND("Create devices!");
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
for(int k=0 ;k<nodeNumber;k++)
{
  positionAlloc->Add(Vector(k*5.0, k*5.0, 0.0));
}
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(c);

  InternetStackHelper internet;
  internet.Install(c);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign(devices);

  TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
  Ptr<Socket> mysocket[nodeNumber];
  InetSocketAddress local=InetSocketAddress(i.GetAddress(0), 80);
  InetSocketAddress remote=InetSocketAddress(i.GetAddress(1), 80);
  for(int nd=0;nd<nodeNumber;nd+=2)
{
	//reciver
  mysocket[nd] = Socket::CreateSocket(c.Get(nd), tid);
  local = InetSocketAddress(i.GetAddress(nd), 80);
  mysocket[nd]->Bind(local);
  mysocket[nd]->SetRecvCallback(MakeCallback(&ReceivePacket));
  if(nd+1>=nodeNumber)
	  break;
	//source
  mysocket[nd+1] = Socket::CreateSocket(c.Get(nd+1),tid);
  remote = InetSocketAddress(i.GetAddress(nd+1),80);
  mysocket[nd+1]->Bind(remote);
  mysocket[nd+1]->SetAllowBroadcast(true);
  mysocket[nd+1]->Connect(local);
  NS_LOG_UNCOND("Make Connection with "<<nd<<"&"<<nd+1);
}

  wifiPhy.EnablePcap("labExample", devices);
 // Simulator::ScheduleWithContext(source->GetNode()->GetId(), Seconds(1.0), &GenerateTraffic, 
  //                               source, 1000, packetNumber, interPacketInterval);
for(int nd=0;nd<nodeNumber;nd+=2)
{
	if(nd+1>=nodeNumber)
		break;
	Simulator::ScheduleWithContext(mysocket[nd+1]->GetNode()->GetId(), Seconds(1.0), &GenerateTraffic, mysocket[nd+1], 1000, packetNumber, interPacketInterval);
NS_LOG_UNCOND("Setup Sending node"<<nd+1);
}
  AnimationInterface anim("Wireless-animation.xml");
  for (int ii=0;ii<nodeNumber;ii++)
  { 
	  anim.SetConstantPosition(c.Get(ii), (double)ii*5.0, (double)ii*5.0);	
  }
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

