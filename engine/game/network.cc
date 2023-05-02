#include "config.h"
#include "network.h"

namespace Game
{
bool InitializeENet()
{
	if (enet_initialize() != 0)
	{
		printf("\n[ERROR] failed to initialize ENet.\n");
		return false;
	}
	atexit(enet_deinitialize);
	return true;
}

// --- data ---

Data::Data()
{
	sender = nullptr;
}

Data::Data(ENetPeer* _sender, enet_uint8* dataToCopy, size_t dataSize)
{
	sender = _sender;
	data = std::make_shared<std::vector<enet_uint8>>();
	data->reserve(dataSize);
	for (size_t i = 0; i < dataSize; i++)
		data->push_back(dataToCopy[i]);
}

Data::Data(const Data& other)
{
	sender = other.sender;
	data = other.data;
}

Data& Data::operator=(const Data& other)
{
	sender = other.sender;
	data = other.data;
	return *this;
}

Data::~Data() {}


// --- host base class ---

Host::Host(HostType _type) :
	type(_type),
	host(nullptr)
{}

Host::~Host()
{
	enet_host_destroy(host);
}

void Host::Update()
{
	ENetEvent event;
	while (enet_host_service(host, &event, 0) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
			OnConnect(event.peer);
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			dataStack.push_back(Data(event.peer, event.packet->data, event.packet->dataLength));
			enet_packet_destroy(event.packet);
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			OnDisconnect(event.peer);
			break;
		}
	}
}

bool Host::PopDataStack(Data& outData)
{
	if (dataStack.size() == 0)
		return false;

	outData = dataStack.back();
	dataStack.pop_back();
	return true;
}

void Host::SendData(void* data, size_t byteSize, ENetPeer* peer, ENetPacketFlag packetFlag)
{
	if (peer == nullptr)
	{
		printf("\n[ERROR] tried to send data to nullptr peer.\n");
		return;
	}

	ENetPacket* packet = enet_packet_create(data, byteSize, packetFlag);
	enet_peer_send(peer, 0, packet);
	enet_host_flush(host);
}


// --- server ---

Server::Server() :
	Host(HostType::Server),
	onClientConnect(nullptr),
	onClientDisconnect(nullptr)
{}

Server::~Server()
{}

bool Server::Initialize(const char* serverIP, enet_uint16 port, ConnectionEvent _onClientConnect, ConnectionEvent _onClientDisconnect)
{
	onClientConnect = _onClientConnect;
	onClientDisconnect = _onClientDisconnect;

	ENetAddress address;
	enet_address_set_host(&address, serverIP);
	address.port = port;

	host = enet_host_create(&address, 32, 2, 0, 0);

	if (host == nullptr)
	{
		printf("\n[ERROR] failed to create ENet server host.\n");
		return false;
	}

	printf("\n[INFO] server created.\n");
	return true;
}

void Server::BroadcastData(void* data, size_t byteSize, ENetPacketFlag packetFlag, ENetPeer* exlude)
{
	ENetPacket* packet = enet_packet_create(data, byteSize, packetFlag);

	if (exlude == nullptr)
	{
		enet_host_broadcast(host, 0, packet);
	}
	else
	{
		for (auto& peer : connectedPeers)
		{
			if(peer != exlude)
				enet_peer_send(peer, 0, packet);
		}
	}

	enet_host_flush(host);
}

void Server::OnConnect(ENetPeer* peer)
{
	if (connectedPeers.count(peer) == 0)
	{
		connectedPeers.insert(peer);
		onClientConnect(peer);
	}
}

void Server::OnDisconnect(ENetPeer* peer)
{
	onClientDisconnect(peer);
	connectedPeers.erase(peer);
}


// --- client ---

Client::Client() :
	Host(HostType::Client),
	onServerConnect(nullptr),
	onServerDisconnect(nullptr),
	server(nullptr)
{}

Client::~Client()
{}

bool Client::Initialize(ConnectionEvent _onServerConnect, ConnectionEvent _onServerDisconnect)
{
	onServerConnect = _onServerConnect;
	onServerDisconnect = _onServerDisconnect;

	host = enet_host_create(nullptr, 1, 2, 0, 0);

	if (host == nullptr)
	{
		printf("\n[ERROR] failed to create ENet client host.\n");
		return false;
	}

	printf("\n[INFO] client created.\n");
	return true;
}

bool Client::RequestConnectionToServer(const char* serverIP, enet_uint16 port)
{
	ENetAddress address;
	ENetEvent event;

	enet_address_set_host(&address, serverIP);
	address.port = port;

	server = enet_host_connect(host, &address, 2, 0);

	if (server == nullptr)
	{
		printf("\n[ERROR] failed to initiate ENet connection.\n");
		return false;
	}

	return true;
}

void Client::OnConnect(ENetPeer* peer)
{
	onServerConnect(server);
}

void Client::OnDisconnect(ENetPeer* peer)
{
	onServerDisconnect(server);
	server = nullptr;
}
}