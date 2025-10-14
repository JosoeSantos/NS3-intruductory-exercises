/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

//
// 10.1.1.0 10.1.3.0
// n0 -------------- n1 n2 n3 n4 -------------- n5
// point-to-point   |  |  |  |  point-to-point
//                 ============
// LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Lab1Part2");

int
main(int argc, char* argv[])
{
    bool verbose = true;
    uint32_t nCsma = 3;
    uint32_t nPackets = 1;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue("nPackets", "Number of packets to send", nPackets);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);

    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    nCsma = nCsma < 1 ? 1 : nCsma;
    nPackets = nPackets < 1 ? 1 : nPackets;



    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    NodeContainer csmaNodes;
    csmaNodes.Add(p2pNodes.Get(1));
    csmaNodes.Create(nCsma);

    NodeContainer endNodes;
    endNodes.Create(1);
    endNodes.Add(csmaNodes.Get(nCsma));

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install(csmaNodes);

    PointToPointHelper endPointToPoint;
    endPointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    endPointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer endDevices;
    endDevices = endPointToPoint.Install(endNodes);

    InternetStackHelper stack;
    stack.Install(p2pNodes.Get(0));
    stack.Install(csmaNodes);
    stack.Install(endNodes.Get(0));

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(p2pDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces;
    csmaInterfaces = address.Assign(csmaDevices);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer endInterfaces;
    endInterfaces = address.Assign(endDevices);
    NS_LOG_INFO("Assigning IP addresses in subnet 10.1.3.0");
    NS_LOG_INFO("Server IP address: " << endInterfaces.GetAddress(1));
    NS_LOG_INFO("Client IP address: " << p2pInterfaces.GetAddress(0));


    UdpEchoServerHelper echoServer(9);

    ApplicationContainer serverApps = echoServer.Install(endNodes.Get(0));
    serverApps.Start(Seconds(1));
    serverApps.Stop(Seconds(2*nPackets + ceil(nPackets/2.0)));

    // TODO: Alterar aqui para que cada cliente envie nPackets
    // TODO: Apontar para o node correto do servidor
    UdpEchoClientHelper echoClient(endInterfaces.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(nPackets));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(p2pNodes.Get(0));
    clientApps.Start(Seconds(2));
    clientApps.Stop(Seconds(2*nPackets + ceil(nPackets/2.0)));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    pointToPoint.EnablePcapAll("Lab1Part2");
    csma.EnablePcap("Lab1Part2", csmaDevices.Get(1), true);

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
