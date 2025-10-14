/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;


NS_LOG_COMPONENT_DEFINE("Lab1Part1");

struct arguments {
    int nClients;
    int nPackets;
};

int main(int argc, char* argv[])
{
    // Default logging levels
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable("Lab1Part1", LOG_LEVEL_INFO);

    arguments args = {1, 1};

    CommandLine cmd(__FILE__);
    cmd.Usage("Usage: first [--nClients] [--nPackets] \n");
    cmd.AddValue("nClients", "Number of echo clients", args.nClients);
    cmd.AddValue("nPackets", "Number of packets per client", args.nPackets);
    cmd.Parse(argc, argv);

    if (args.nClients > 5) args.nClients = 5;
    if (args.nPackets > 5) args.nPackets = 5;

    if (args.nClients < 1) args.nClients = 1;
    if (args.nPackets < 1) args.nPackets = 1;

    Time::SetResolution(Time::NS);

    NodeContainer nodes;
    nodes.Create(args.nClients + 1); // Create server + clients

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    Ipv4InterfaceContainer interfaces;

    for (int i = 1; i <= args.nClients; i++) {
        NodeContainer pair;
        pair.Add(nodes.Get(0)); // servidor
        pair.Add(nodes.Get(i)); // cliente

        // Set subnets 10.1.i.0/24 for each client i
        std::string base = "10.1." + std::to_string(i) + ".0";

        NetDeviceContainer linkDevices = pointToPoint.Install(pair);
        devices.Add(linkDevices);

        NS_LOG_INFO("Assigning IP addresses in subnet " << base << "/24 to link between server and client " << i);
        address.SetBase(base.c_str(), "255.255.255.0");
        Ipv4InterfaceContainer linkInterfaces = address.Assign(linkDevices);
        interfaces.Add(linkInterfaces);
    }

    // Enable global routing to handle multiple subnets
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    UdpEchoServerHelper echoServer(15); // Changed port to 15

    ApplicationContainer serverApps = echoServer.Install(nodes.Get(0));
    serverApps.Start(Seconds(1));
    serverApps.Stop(Seconds(20));

    // All clients connect to server at first subnet
    UdpEchoClientHelper echoClient(interfaces.GetAddress(0), 15);
    NS_LOG_INFO("All clients will connect to server at " << interfaces.GetAddress(0) << " on port 15");
    echoClient.SetAttribute("MaxPackets", UintegerValue(args.nPackets));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    // Create random number generator for start times
    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();
    x->SetAttribute("Min", DoubleValue(2.0));
    x->SetAttribute("Max", DoubleValue(7.0));

    for (int i = 1; i <= args.nClients; i++) {
        ApplicationContainer clientApps = echoClient.Install(nodes.Get(i));
        double startTime = x->GetValue();
        clientApps.Start(Seconds(startTime)); // Random start time between 2-7 seconds
        clientApps.Stop(Seconds(20));
    }

    AsciiTraceHelper ascii;
    pointToPoint.EnableAsciiAll(ascii.CreateFileStream("lab-1-part1.tr"));
    pointToPoint.EnablePcapAll("lab-1-part1");
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
