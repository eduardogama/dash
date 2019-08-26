/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 TEI of Western Macedonia, Greece
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Eduardo S. Gama <eduardogama72@gmail.com>
 */

#include <string>
#include <fstream>
#include <iostream>
#include <ctime>

#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/dash-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"

#include "ns3/mobility-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"

// #include "ns3/netanim-module.h"
#include "ns3/flow-monitor-helper.h"


using namespace ns3;
using namespace std;

extern vector<vector<int> > readNxNMatrix (std::string adj_mat_file_name);
extern vector<vector<double> > readCordinatesFile (std::string node_coordinates_file_name);
extern void printCoordinateArray (const char* description, vector<vector<double> > coord_array);
extern void printMatrix (const char* description, vector<vector<int> > array);

extern vector<std::string> split(const std::string& str, const std::string& delim);
extern void store(uint32_t &tNodes, uint32_t &layer, uint32_t &users, std::string &str);
extern void calcAvg(uint32_t &tNodes, uint32_t &layer, uint32_t &users);


uint32_t tRate;
double tStalls;
double tStalls_time;


NS_LOG_COMPONENT_DEFINE("DashExample");

int main(int argc, char *argv[]) {

    tStalls      = 0;
    tStalls_time = 0;
    tRate        = 0;

    bool tracing         = false;
    uint32_t maxBytes    = 100;
    uint32_t n_nodes     = 5;
    uint32_t users       = 1;
    uint32_t layer       = 0;
    double target_dt     = 35.0;
    double stopTime      = 100.0;
    std::string protocol = "ns3::DashClient";
    std::string window   = "10s";

    // MpdFileHandler *mpd_instance = MpdFileHandler::getInstance();

    // LogComponentEnable("MpegPlayer", LOG_LEVEL_ALL);
    // LogComponentEnable ("DashServer", LOG_LEVEL_ALL);
    // LogComponentEnable ("DashClient", LOG_LEVEL_ALL);
    LogComponentEnable ("DashExample", LOG_LEVEL_ALL);

    //
    // Allow the user to override any of the defaults at
    // run-time, via command-line arguments
    //
    CommandLine cmd;
    cmd.AddValue("tracing", "Flag to enable/disable tracing", tracing);
    cmd.AddValue("maxBytes", "Total number of bytes for application to send",
      maxBytes);
    cmd.AddValue("users", "The number of concurrent videos", users);
    cmd.AddValue("nodes", "The number of nodes.", n_nodes);
    cmd.AddValue("targetDt",
        "The target time difference between receiving and playing a frame.", target_dt);
    cmd.AddValue("layer",
        "The layer where the nodes support the cloud provisioning resquest segments clients", layer);
    cmd.AddValue("stopTime",
        "The time when the clients will stop requesting segments", stopTime);
    // cmd.AddValue("linkRate",
    //   "The bitrate of the link connecting the clients to the server (e.g. 500kbps)",
    //   linkRate);
    // cmd.AddValue("delay",
    //   "The delay of the link connecting the clients to the server (e.g. 5ms)",
    //   delay);
    cmd.AddValue("protocol",
      "The adaptation protocol. It can be 'ns3::DashClient' or 'ns3::OsmpClient (for now).",
      protocol);
    cmd.AddValue("window",
      "The window for measuring the average throughput (Time).", window);
    cmd.Parse(argc, argv);

    std::string adj_mat_file_name ("src/dash/examples/adjacency_matrix_5_nodes.txt");
    // std::string node_coordinates_file_name ("examples/matrix-topology/node_coordinates.txt");

    // ---------- Read Adjacency Matrix ----------------------------------------
    vector<vector<int> > Adj_Matrix;
    Adj_Matrix = readNxNMatrix (adj_mat_file_name);

    printMatrix ("Initial network", Adj_Matrix);
    // ---------- End of Read Adjacency Matrix ---------------------------------

    // ---------- Network Setup ------------------------------------------------

    //
    // Explicitly create the nodes required by the topology (shown above).
    //
    NS_LOG_INFO("Create nodes.");

    NodeContainer nodes;   // Declare nodes objects
    nodes.Create (n_nodes);

    NS_LOG_INFO("Create channels.");

    // Explicitly create the point-to-point link required by the topology (shown above).
    PointToPointHelper p2p;

    NS_LOG_INFO ("Create Links Between Nodes.");

    uint32_t linkCount = 0;

    std::vector<NetDeviceContainer> devices;

    for (size_t i = 0; i < Adj_Matrix.size (); i++) {
        for (size_t j = 0; j < Adj_Matrix[i].size (); j++) {
            if (Adj_Matrix[i][j] != 0) {
                NodeContainer n_links = NodeContainer (nodes.Get (i), nodes.Get (j));

                std::string l = std::to_string(Adj_Matrix[i][j]) + std::string("Mbps");
                p2p.SetDeviceAttribute("DataRate", StringValue(l));
                // p2p.SetChannelAttribute("Delay", StringValue("50ms"));


                NetDeviceContainer n_devs = p2p.Install (n_links);

                devices.push_back(n_devs);
                linkCount++;
                NS_LOG_INFO ("matrix element [" << i << "][" << j << "] is 1");
            // } else {
                // NS_LOG_INFO ("matrix element [" << i << "][" << j << "] is 0");
            }
        }
    }
    NS_LOG_INFO ("Number of links in the adjacency matrix is: " << linkCount);
    NS_LOG_INFO ("Number of all nodes is: " << nodes.GetN());


    // ---------- Network WiFi Users Setup -------------------------------------

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(users);

    NodeContainer wifiApNode = nodes.Get(4);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");
    mac.SetType ("ns3::StaWifiMac",
                 "Ssid", SsidValue (ssid),
                 "ActiveProbing", BooleanValue (false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue (ssid));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install(phy, mac, wifiApNode);

    MobilityHelper mobility;
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (0.0),
                                   "MinY", DoubleValue (0.0),
                                   "DeltaX", DoubleValue (5.0),
                                   "DeltaY", DoubleValue (5.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));

    // mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
    //                             "Mode", StringValue ("Time"),
    //                             "Time", StringValue ("1s"),
    //                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
    //                             "Bounds", RectangleValue (Rectangle (-100, 100, -100, 100)));
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");


    mobility.Install(wifiStaNodes);
    mobility.Install(wifiApNode);
    mobility.Install(nodes);


    // ---------- End Network WiFi Users Setup ---------------------------------

    // ---------- Internet Stack Setup -------------------------------------------
    NS_LOG_INFO ("Install Internet Stack to Nodes.");

    InternetStackHelper internet;
    internet.Install(NodeContainer::GetGlobal());

    NS_LOG_INFO ("Assign Addresses to Nodes.");

    Ipv4AddressHelper ipv4_n;
    ipv4_n.SetBase("10.1.1.0", "255.255.255.0");

    for (size_t i = 0; i < devices.size(); i++) {
        ipv4_n.Assign(devices[i]);
        ipv4_n.NewNetwork();
    }

    for (size_t i = 0; i < n_nodes; i++) {
        Ptr<Node> n = nodes.Get(i);
        Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();
        std::cout << "Node " << i << " --> " << ipv4->GetAddress(1,0) << '\n';
    }

    ipv4_n.Assign(staDevices);
    ipv4_n.Assign(apDevices);

    for (size_t i = 0; i < users; i++) {
        Ptr<Node> n = wifiStaNodes.Get(i);
        Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();
        std::cout << "Node User WiFi " << i << " --> " << ipv4->GetAddress(1,0) << '\n';
    }


    // ---------- Applications Setup -------------------------------------------
    NS_LOG_INFO("Create Applications.");

    std::vector<std::string> protocols;
    std::stringstream ss(protocol); // ns3::DashClient
    std::string proto;
    uint32_t protoNum = 0; // The number of protocols (algorithms)

    while (std::getline(ss, proto, ',') && protoNum++ < users) {
        protocols.push_back(proto);
    }

    uint16_t port = 80;  // well-known echo port number
    // uint16_t port_client = 1000;

    DashServerHelper server("ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer serverApp = server.Install(nodes.Get(0));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(stopTime + 5.0));

    DashServerHelper fogServer("ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer fogServerApp = fogServer.Install(nodes.Get(layer));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(stopTime + 5.0));


    std::vector<DashClientHelper> clients;
    std::vector<ApplicationContainer> clientApps;

    std::vector<Ipv4Address> ipv4_interfaces;
    for (size_t i = 0; i < n_nodes; i++) {
        Ptr<Node> fog_n = nodes.Get(i);
        Ptr<Ipv4> fog_ipv4 = fog_n->GetObject<Ipv4> ();
        Ipv4InterfaceAddress fog_ipv4_int_addr = fog_ipv4->GetAddress (1, 0);

        ipv4_interfaces.push_back(fog_ipv4_int_addr.GetLocal());
        std::cout << "Node " << i << " --> " << fog_ipv4_int_addr.GetLocal() << '\n';
    }

    // ---------- Network Seed Setup ------------------------------------------------
    SeedManager::SetRun(time(0));
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
    // ---------- End Network Seed Setup --------------------------------------------

    for (uint32_t user = 0; user < users; user++) {

        DashClientHelper client("ns3::TcpSocketFactory",
            InetSocketAddress(ipv4_interfaces[0], port),
            InetSocketAddress(ipv4_interfaces[layer], port),
            protocols[user % protoNum]);

        //client.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
        client.SetAttribute("VideoId", UintegerValue(user + 1)); // VideoId should be positive
        client.SetAttribute("TargetDt", TimeValue(Seconds(target_dt)));
        client.SetAttribute("window", TimeValue(Time(window)));

        double rdm = uv->GetValue();
        std::cout << "Random Variable=" << rdm << '\n';

        ApplicationContainer clientApp = client.Install( wifiStaNodes.Get(user) );
        clientApp.Start(Seconds(0.25 + rdm));
        clientApp.Stop(Seconds(stopTime));

        clients.push_back(client);
        clientApps.push_back(clientApp);
    }

    NS_LOG_INFO ("Initialize Global Routing.");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(stopTime));

    // Ipv4GlobalRoutingHelper g;
    // Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("dynamic-global-routing.routes", std::ios::out);
    // g.PrintRoutingTableAllAt (Seconds (1), routingStream);

    // Set up tracing if enabled
    if (tracing) {
        AsciiTraceHelper ascii;
        p2p.EnableAsciiAll(ascii.CreateFileStream("dash-send.tr"));
        p2p.EnablePcapAll("dash-send", false);
    }

    // Now, do the actual simulation.
    NS_LOG_INFO("Run Simulation.");

    // AnimationInterface anim ("animation.xml");


    // Flow Monitor -----------------------------------------------------------
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    monitor->Start (Seconds (0));
    monitor->Stop (Seconds (stopTime));

    // Ptr<FlowMonitor> flowMonitor;
    // FlowMonitorHelper flowHelper;
    // flowMonitor = flowHelper.InstallAll();
    //
    // Simulator::Stop (Seconds(stop_time));
    // Simulator::Run ();



    Simulator::Run();

    // Print per flow statistics
    monitor->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

        // if ((t.sourceAddress == Ipv4Address("10.1.1.1") && t.destinationAddress == Ipv4Address("10.1.1.25"))
        //     || (t.sourceAddress == Ipv4Address("10.1.1.11") && t.destinationAddress == Ipv4Address("10.1.1.15"))
        //     || (t.sourceAddress == Ipv4Address("10.1.1.21") && t.destinationAddress == Ipv4Address("10.1.1.5")))
        // {
            NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
            NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
            NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
            NS_LOG_UNCOND("Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps");
        // }
    }

    std::string flowfile = std::string("dash-1-zone-") + std::to_string(n_nodes) + std::string("-") + std::to_string(users) + std::string("-") + std::to_string(layer) + std::string(".xml");
  	monitor->SerializeToXmlFile (flowfile, true, true);

    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    // flowMonitor->SerializeToXmlFile("dash-1-zone.flowmon", true, true);


    for (uint16_t k = 0; k < users; k++) {
        Ptr<DashClient> app = DynamicCast<DashClient>(clientApps[k].Get(0));
        std::cout << protocols[k % protoNum] << "-Node: " << k;
        std::string str = app->GetStats();

        store(n_nodes, layer, users, str);
    }
    calcAvg(n_nodes, layer, users);

    return 0;
}

// ---------- Function Definitions -------------------------------------------

void calcAvg(uint32_t &tNodes, uint32_t &layer, uint32_t &users) {
    fstream file;

    ostringstream arq;   // stream used for the conversion

    arq << "PB_total_" << tNodes << "_" << users << "_" << layer << ".txt";

    file.open(arq.str().c_str(),fstream::out | fstream::app);


    locale mylocale("");
    file.imbue( mylocale );

    file << tStalls/(users) << " " << tStalls_time/(users) << " " << tRate/(users) << endl;

    file.close();
}

void store(uint32_t &tNodes, uint32_t &layer, uint32_t &users, std::string &str) {
    fstream file;

    ostringstream arq;   // stream used for the conversion
    arq << "PB_events_" << tNodes << "_" << users << "_" << layer << ".txt";

    file.open(arq.str().c_str(),fstream::out | fstream::app);

    locale mylocale("");
    file.imbue( mylocale );

    //PB per user
    file << str << endl;
    file.close();

    vector<string> tokens = split(str, " ");

    tStalls      += std::stoi(tokens[0]);
    tStalls_time += std::stof(tokens[1]);
    tRate        += std::stoi(tokens[2]);
}

vector<std::string> split(const std::string& str, const std::string& delim) {
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do {
        pos = str.find(delim, prev);
        if (pos == string::npos) pos = str.length();
        string token = str.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());

    return tokens;
}

vector<vector<int> > readNxNMatrix (std::string adj_mat_file_name) {
    ifstream adj_mat_file;
    adj_mat_file.open (adj_mat_file_name.c_str (), ios::in);
    if (adj_mat_file.fail ()) {
        NS_FATAL_ERROR ("File " << adj_mat_file_name.c_str () << " not found");
    }
    vector<vector<int> > array;
    int i = 0;
    int n_nodes = 0;

    while (!adj_mat_file.eof ()) {
        string line;
        getline (adj_mat_file, line);
        if (line == "") {
            NS_LOG_WARN ("WARNING: Ignoring blank row in the array: " << i);
            break;
        }

        istringstream iss (line);
        int element;
        vector<int> row;
        int j = 0;

        while (iss >> element) {
            row.push_back (element);
            j++;
        }

        if (i == 0) {
            n_nodes = j;
        }

        if (j != n_nodes ) {
            NS_LOG_ERROR ("ERROR: Number of elements in line " << i << ": " << j << " not equal to number of elements in line 0: " << n_nodes);
            NS_FATAL_ERROR ("ERROR: The number of rows is not equal to the number of columns! in the adjacency matrix");
        } else {
            array.push_back (row);
        }
        i++;
    }

    if (i != n_nodes) {
        NS_LOG_ERROR ("There are " << i << " rows and " << n_nodes << " columns.");
        NS_FATAL_ERROR ("ERROR: The number of rows is not equal to the number of columns! in the adjacency matrix");
    }

    adj_mat_file.close ();
    return array;
}

vector<vector<double> > readCordinatesFile (std::string node_coordinates_file_name) {
    ifstream node_coordinates_file;
    node_coordinates_file.open (node_coordinates_file_name.c_str (), ios::in);
    if (node_coordinates_file.fail ()) {
        NS_FATAL_ERROR ("File " << node_coordinates_file_name.c_str () << " not found");
    }
    vector<vector<double> > coord_array;
    int m = 0;

    while (!node_coordinates_file.eof ()) {
        string line;
        getline (node_coordinates_file, line);

        if (line == "") {
            NS_LOG_WARN ("WARNING: Ignoring blank row: " << m);
            break;
        }

        istringstream iss (line);
        double coordinate;
        vector<double> row;
        int n = 0;
        while (iss >> coordinate) {
            row.push_back (coordinate);
            n++;
        }

        if (n != 2) {
            NS_LOG_ERROR ("ERROR: Number of elements at line#" << m << " is "  << n << " which is not equal to 2 for node coordinates file");
            exit (1);
        } else {
            coord_array.push_back (row);
        }
        m++;
    }
    node_coordinates_file.close ();
    return coord_array;
}

void printMatrix (const char* description, vector<vector<int> > array) {
    cout << "**** Start " << description << "********" << endl;
    for (size_t m = 0; m < array.size (); m++) {
        for (size_t n = 0; n < array[m].size (); n++) {
            cout << array[m][n] << ' ';
        }
        cout << endl;
    }
    cout << "**** End " << description << "********" << endl;
}

void printCoordinateArray (const char* description, vector<vector<double> > coord_array) {
    cout << "**** Start " << description << "********" << endl;
    for (size_t m = 0; m < coord_array.size (); m++) {
        for (size_t n = 0; n < coord_array[m].size (); n++) {
            cout << coord_array[m][n] << ' ';
        }
        cout << endl;
    }
    cout << "**** End " << description << "********" << endl;
}

// ---------- End of Function Definitions ------------------------------------
