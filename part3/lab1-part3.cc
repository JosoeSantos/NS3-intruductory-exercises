/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/wifi-module.h"
#include "ns3/yans-wifi-helper.h"

// Default Network Topology
//
//   Wifi1 10.1.2.0                   Wifi2 10.1.3.0
//        AP1                              AP2
//   *    *    *                      *    *    *
//   |    |    |    10.1.1.0         |    |    |
//  n4   n5   n6   n0 ------------- n1   n7   n8   n9
//                  point-to-point

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Lab1Part3");

int
main(int argc, char* argv[])
{
    bool verbose = true;
    uint32_t nWifi = 3;
    uint32_t nPackets = 1;
    bool tracing = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nWifi", "Number of wifi STA devices per network", nWifi);
    cmd.AddValue("nPackets", "Number of packets to send", nPackets);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);

    cmd.Parse(argc, argv);

    // The underlying restriction of 9 is due to the grid position
    // allocator's configuration; the grid layout will exceed the
    // bounding box if more than 9 nodes per WiFi network are provided.
    if (nWifi > 9)
    {
        std::cout << "nWifi should be 9 or less; otherwise grid layout exceeds the bounding box"
                  << std::endl;
        return 1;
    }

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);



    // First WiFi network (left side)
    NodeContainer wifiStaNodes1;
    wifiStaNodes1.Create(nWifi);
    NodeContainer wifiApNode1 = p2pNodes.Get(0);

    // Second WiFi network (right side)
    NodeContainer wifiStaNodes2;
    wifiStaNodes2.Create(nWifi);
    NodeContainer wifiApNode2 = p2pNodes.Get(1);

    // First WiFi network setup
    YansWifiChannelHelper channel1 = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy1;
    phy1.SetChannel(channel1.Create());

    WifiMacHelper mac1;
    Ssid ssid1 = Ssid("wifi-network-1");
    WifiHelper wifi1;

    NetDeviceContainer staDevices1;
    mac1.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid1), "ActiveProbing", BooleanValue(false));
    staDevices1 = wifi1.Install(phy1, mac1, wifiStaNodes1);

    NetDeviceContainer apDevices1;
    mac1.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid1));
    apDevices1 = wifi1.Install(phy1, mac1, wifiApNode1);

    // Second WiFi network setup
    YansWifiChannelHelper channel2 = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy2;
    phy2.SetChannel(channel2.Create());

    WifiMacHelper mac2;
    Ssid ssid2 = Ssid("wifi-network-2");
    WifiHelper wifi2;

    NetDeviceContainer staDevices2;
    mac2.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid2), "ActiveProbing", BooleanValue(false));
    staDevices2 = wifi2.Install(phy2, mac2, wifiStaNodes2);

    NetDeviceContainer apDevices2;
    mac2.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid2));
    apDevices2 = wifi2.Install(phy2, mac2, wifiApNode2);

    MobilityHelper mobility;

    // Mobility for first WiFi network
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(5.0),
                                  "DeltaY",
                                  DoubleValue(10.0),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds",
                              RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.Install(wifiStaNodes1);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode1);

    // Mobility for second WiFi network (offset position)
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(60.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(5.0),
                                  "DeltaY",
                                  DoubleValue(10.0),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds",
                              RectangleValue(Rectangle(10, 110, -50, 50)));
    mobility.Install(wifiStaNodes2);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode2);

    InternetStackHelper stack;
    stack.Install(wifiApNode1);
    stack.Install(wifiStaNodes1);
    stack.Install(wifiApNode2);
    stack.Install(wifiStaNodes2);

    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(p2pDevices);



    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer wifi1Interfaces;
    wifi1Interfaces = address.Assign(staDevices1);
    wifi1Interfaces.Add(address.Assign(apDevices1));

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer wifi2Interfaces;
    wifi2Interfaces = address.Assign(staDevices2);
    wifi2Interfaces.Add(address.Assign(apDevices2));

    UdpEchoServerHelper echoServer(9);

    // Install server on the last WiFi network's sta node
    ApplicationContainer serverApps = echoServer.Install(wifiStaNodes2.Get(nWifi - 1));
    NS_LOG_INFO("Server address: " << wifi2Interfaces.GetAddress(nWifi - 1));
    serverApps.Start(Seconds(1));
    serverApps.Stop(Seconds(25));

    // Install client on the first WiFi network's first STA node
    UdpEchoClientHelper echoClient(wifi2Interfaces.GetAddress(nWifi - 1), 9); // AP is at index  nWifi - 1
    echoClient.SetAttribute("MaxPackets", UintegerValue(nPackets));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(wifiStaNodes1.Get(0));

    NS_LOG_INFO("Client address: " << wifi1Interfaces.GetAddress(0));
    clientApps.Start(Seconds(2));
    clientApps.Stop(Seconds(25));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(25));

    if (tracing)
    {
        phy1.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        phy2.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        pointToPoint.EnablePcapAll("third");
        phy1.EnablePcap("third", apDevices1.Get(0));
        phy2.EnablePcap("third", apDevices2.Get(0));
    }

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
