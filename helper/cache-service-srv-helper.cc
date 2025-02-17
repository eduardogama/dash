#include "cache-service-srv-helper.h"

#include "ns3/inet-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/string.h"
#include "ns3/names.h"


namespace ns3 {

    CacheServiceHelper::CacheServiceHelper(std::string protocol, Address address)
    {
        m_factory.SetTypeId ("ns3::CacheService");
        m_factory.Set ("Protocol", StringValue (protocol));
        m_factory.Set ("Remote", AddressValue (address));
    }

    CacheServiceHelper::CacheServiceHelper (std::string protocol, Address server_address, Address connection_address)
    {
        m_factory.SetTypeId ("ns3::CacheService");
        m_factory.Set ("Protocol", StringValue (protocol));
        m_factory.Set ("Local", AddressValue (server_address));
        m_factory.Set ("Remote", AddressValue (connection_address));
    }

    void CacheServiceHelper::SetAttribute (std::string name, const AttributeValue &value)
    {
        m_factory.Set (name, value);
    }

    ApplicationContainer CacheServiceHelper::Install (Ptr<Node> node) const
    {
        return ApplicationContainer (InstallPriv (node));
    }

    ApplicationContainer CacheServiceHelper::Install (std::string nodeName) const
    {
        Ptr<Node> node = Names::Find<Node> (nodeName);
        return ApplicationContainer (InstallPriv (node));
    }

    ApplicationContainer CacheServiceHelper::Install (NodeContainer c) const
    {
        ApplicationContainer apps;
        for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
        {
            apps.Add (InstallPriv (*i));
        }

        return apps;
    }

    Ptr<Application> CacheServiceHelper::InstallPriv (Ptr<Node> node) const
    {
        Ptr<Application> app = m_factory.Create<Application> ();
        node->AddApplication (app);

        return app;
    }

} //namespace ns3
