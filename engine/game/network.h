#pragma once
#include "enet/enet.h"
#include <vector>
#include <memory>
#include <unordered_set>
#include <functional>

namespace Game
{
bool InitializeENet();

struct Data
{
	ENetPeer* sender;
	std::shared_ptr<std::vector<enet_uint8>> data;

	Data();
	Data(ENetPeer* _sender, enet_uint8* dataToCopy, size_t dataSize);
	Data(const Data& other);
	Data& operator=(const Data& other);
	~Data();
};

enum class HostType
{
	Client,
	Server
};

class Host
{
protected:
	ENetHost* host;
	std::vector<Data> dataStack;

public:
	HostType type;

	Host(HostType _type);
	virtual ~Host();

	void Update();
	bool PopDataStack(Data& outData);
	void SendData(void* data, size_t byteSize, ENetPeer* peer, ENetPacketFlag packetFlag);

protected:
	virtual void OnConnect(ENetPeer* peer) = 0;
	virtual void OnDisconnect(ENetPeer* peer) = 0;
};

typedef std::function<void(ENetPeer*)> ConnectionEvent;

class Server : public Host
{
private:
	ConnectionEvent onClientConnect;
	ConnectionEvent onClientDisconnect;

public:
	std::unordered_set<ENetPeer*> connectedPeers;

	Server();
	virtual ~Server() override;

	bool Initialize(const char* serverIP, enet_uint16 port, ConnectionEvent _onClientConnect, ConnectionEvent _onClientDisconnect);
	void BroadcastData(void* data, size_t byteSize, ENetPacketFlag packetFlag, ENetPeer* exlude = nullptr);

private:
	virtual void OnConnect(ENetPeer* peer) override;
	virtual void OnDisconnect(ENetPeer* peer) override;
};

class Client : public Host
{
private:
	ConnectionEvent onServerConnect;
	ConnectionEvent onServerDisconnect;

public:
	ENetPeer* server;

	Client();
	virtual ~Client() override;

	bool Initialize(ConnectionEvent _onServerConnect, ConnectionEvent _onServerDisconnect);
	bool RequestConnectionToServer(const char* serverIP, enet_uint16 port);

private:
	virtual void OnConnect(ENetPeer* peer) override;
	virtual void OnDisconnect(ENetPeer* peer) override;
};
}