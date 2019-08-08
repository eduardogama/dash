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
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/dash-module.h"


// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1 -------------- n2
//    point-to-point |  point-to-point
//

using namespace ns3;
using namespace std;

extern vector<vector<int> > readNxNMatrix (std::string adj_mat_file_name);
extern vector<vector<double> > readCordinatesFile (std::string node_coordinates_file_name);
extern void printCoordinateArray (const char* description, vector<vector<double> > coord_array);
extern void printMatrix (const char* description, vector<vector<int> > array);


NS_LOG_COMPONENT_DEFINE("dash-matrix-topology-1");

int main(int argc, char *argv[]) {

    bool tracing = false;
    uint32_t maxBytes = 100;
    uint32_t users = 1;
    double target_dt = 35.0;
    double stopTime = 100.0;
    std::string linkRate = "500Kbps";
    std::string delay = "178ms";
    std::string protocol = "ns3::FdashClient";
    std::string window = "10s";



    // MpdFileHandler *mpd_instance = MpdFileHandler::getInstance();

  /*  LogComponentEnable("MpegPlayer", LOG_LEVEL_ALL);*/
//    LogComponentEnable ("DashServer", LOG_LEVEL_ALL);
//    LogComponentEnable ("DashClient", LOG_LEVEL_ALL);
    LogComponentEnable ("dash-matrix-topology-1", LOG_LEVEL_ALL);

    //
    // Allow the user to override any of the defaults at
    // run-time, via command-line arguments
    //
    CommandLine cmd;
    cmd.AddValue("tracing", "Flag to enable/disable tracing", tracing);
    cmd.AddValue("maxBytes", "Total number of bytes for application to send",
      maxBytes);
    cmd.AddValue("users", "The number of concurrent videos", users);
    cmd.AddValue("targetDt",
      "The target time difference between receiving and playing a frame.",
      target_dt);
    cmd.AddValue("stopTime",
      "The time when the clients will stop requesting segments", stopTime);
    cmd.AddValue("linkRate",
      "The bitrate of the link connecting the clients to the server (e.g. 500kbps)",
      linkRate);
    cmd.AddValue("delay",
      "The delay of the link connecting the clients to the server (e.g. 5ms)",
      delay);
    cmd.AddValue("protocol",
      "The adaptation protocol. It can be 'ns3::DashClient' or 'ns3::OsmpClient (for now).",
      protocol);
    cmd.AddValue("window",
      "The window for measuring the average throughput (Time).", window);
    cmd.Parse(argc, argv);

    std::string adj_mat_file_name ("src/dash/examples/adjacency_matrix_15_nodes.txt");
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
    uint32_t n_nodes = 15;

    NodeContainer nodes;   // Declare nodes objects
    nodes.Create (n_nodes);

    NS_LOG_INFO("Create channels.");

    //
    // Explicitly create the point-to-point link required by the topology (shown above).
    //
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue(linkRate));
    // p2p.SetChannelAttribute("Delay", StringValue(delay));

    NS_LOG_INFO ("Create Links Between Nodes.");

    uint32_t linkCount = 0;

    std::vector<Ipv4InterfaceAddress> ipv4_interfaces;
    std::vector<NetDeviceContainer> devices;
    std::vector<int> links;

    for (size_t i = 0; i < Adj_Matrix.size (); i++) {
        for (size_t j = 0; j < Adj_Matrix[i].size (); j++) {
            if (Adj_Matrix[i][j] != 0) {
                NodeContainer n_links = NodeContainer (nodes.Get (i), nodes.Get (j));

                std::string l = std::to_string(Adj_Matrix[i][j]) + std::string("Mbps");
                p2p.SetDeviceAttribute("DataRate", StringValue(l));

                NetDeviceContainer n_devs = p2p.Install(n_links);

                devices.push_back(n_devs);
                links.push_back(Adj_Matrix[i][j]);
                linkCount++;
                NS_LOG_INFO ("matrix element [" << i << "][" << j << "] is 1");
            } else {
                // NS_LOG_INFO ("matrix element [" << i << "][" << j << "] is 0");
            }
        }
    }
    NS_LOG_INFO ("Number of links in the adjacency matrix is: " << linkCount);
    NS_LOG_INFO ("Number of all nodes is: " << nodes.GetN ());

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
        // std::cout << "->" << Address(n) << '\n';
        std::cout << "Node " << i << " --> " << ipv4->GetAddress(1,0) << '\n';
    }

    // ---------- Applications Setup ------------------------------------------------

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
    ApplicationContainer fogServerApp = fogServer.Install(nodes.Get(1));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(stopTime + 5.0));


    std::vector<DashClientHelper> clients;
    std::vector<ApplicationContainer> clientApps;

    Ptr<Node> n                        = nodes.Get(0);
    Ptr<Ipv4> ipv4                     = n->GetObject<Ipv4> ();
    Ipv4InterfaceAddress ipv4_int_addr = ipv4->GetAddress (1, 0);
    Ipv4Address ip_addr                = ipv4_int_addr.GetLocal();

    std::cout << "Connecting in Server ip=" << ipv4->GetAddress (1, 0) << '\n';
    std::cout << "Connecting in Server ip=" << ipv4_int_addr.GetLocal () << '\n';

    Ptr<Node> fog_n = nodes.Get(1);
    Ptr<Ipv4> fog_ipv4 = fog_n->GetObject<Ipv4> ();
    Ipv4InterfaceAddress fog_ipv4_int_addr = fog_ipv4->GetAddress (1, 0);
    Ipv4Address fog_ip_addr = fog_ipv4_int_addr.GetLocal();

    std::cout << "Connecting in Fog Server ip=" << fog_ipv4->GetAddress (1, 0)  << '\n';
    std::cout << "Connecting in Fog Server ip=" << fog_ipv4_int_addr.GetLocal() << '\n';

    for (size_t i = 10; i < 15; i++) {
        for (uint32_t user = 0; user < users; user++) {

            Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
            double rn = x->GetValue();

            DashClientHelper client("ns3::TcpSocketFactory",
                InetSocketAddress(ip_addr, port),
                InetSocketAddress(fog_ip_addr, port),
                protocols[user % protoNum]);

            //client.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
            client.SetAttribute("VideoId", UintegerValue(user + 1)); // VideoId should be positive
            client.SetAttribute("TargetDt", TimeValue(Seconds(target_dt)));
            client.SetAttribute("window", TimeValue(Time(window)));

            ApplicationContainer clientApp = client.Install(nodes.Get(2));
            clientApp.Start(Seconds(0.25 + rn));
            clientApp.Stop(Seconds(stopTime));

            clients.push_back(client);
            clientApps.push_back(clientApp);
        }
    }

    NS_LOG_INFO ("Initialize Global Routing.");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // Ipv4GlobalRoutingHelper g;
    // Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("dynamic-global-routing.routes", std::ios::out);
    // g.PrintRoutingTableAllAt (Seconds (1), routingStream);
    // g.PrintRoutingTableAllAt (Seconds (2.1), routingStream);

    //
    // Set up tracing if enabled
    //
    if (tracing) {
        AsciiTraceHelper ascii;
        p2p.EnableAsciiAll(ascii.CreateFileStream("dash-send.tr"));
        p2p.EnablePcapAll("dash-send", false);
    }

    //
    // Now, do the actual simulation.
    //
    NS_LOG_INFO("Run Simulation.");
    /*Simulator::Stop(Seconds(100.0));*/
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    for (size_t i = 10; i < 15; i++) {
        uint32_t k;
        for (k = 0; k < users; k++){
            Ptr<DashClient> app = DynamicCast<DashClient>(clientApps[k].Get(0));
            std::cout << protocols[k % protoNum] << "-Node: " << k;
            app->GetStats();
        }
    }
    return 0;
}

// ---------- Function Definitions -------------------------------------------

vector<vector<int> > readNxNMatrix (std::string adj_mat_file_name)
{
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

vector<vector<double> > readCordinatesFile (std::string node_coordinates_file_name)
{
  ifstream node_coordinates_file;
  node_coordinates_file.open (node_coordinates_file_name.c_str (), ios::in);
  if (node_coordinates_file.fail ())
    {
      NS_FATAL_ERROR ("File " << node_coordinates_file_name.c_str () << " not found");
    }
  vector<vector<double> > coord_array;
  int m = 0;

  while (!node_coordinates_file.eof ())
    {
      string line;
      getline (node_coordinates_file, line);

      if (line == "")
        {
          NS_LOG_WARN ("WARNING: Ignoring blank row: " << m);
          break;
        }

      istringstream iss (line);
      double coordinate;
      vector<double> row;
      int n = 0;
      while (iss >> coordinate)
        {
          row.push_back (coordinate);
          n++;
        }

      if (n != 2)
        {
          NS_LOG_ERROR ("ERROR: Number of elements at line#" << m << " is "  << n << " which is not equal to 2 for node coordinates file");
          exit (1);
        }

      else
        {
          coord_array.push_back (row);
        }
      m++;
    }
  node_coordinates_file.close ();
  return coord_array;

}

void printMatrix (const char* description, vector<vector<int> > array)
{
  cout << "**** Start " << description << "********" << endl;
  for (size_t m = 0; m < array.size (); m++)
    {
      for (size_t n = 0; n < array[m].size (); n++)
        {
          cout << array[m][n] << ' ';
        }
      cout << endl;
    }
  cout << "**** End " << description << "********" << endl;

}

void printCoordinateArray (const char* description, vector<vector<double> > coord_array)
{
  cout << "**** Start " << description << "********" << endl;
  for (size_t m = 0; m < coord_array.size (); m++)
    {
      for (size_t n = 0; n < coord_array[m].size (); n++)
        {
          cout << coord_array[m][n] << ' ';
        }
      cout << endl;
    }
  cout << "**** End " << description << "********" << endl;

}

// ---------- End of Function Definitions ------------------------------------
