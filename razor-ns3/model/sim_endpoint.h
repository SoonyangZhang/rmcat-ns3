/*
 * sim_endpoint.h
 *
 *  Created on: 2018.4.25
 *      Author: zsy
 * I must claim I just import the razor project on ns3 platform
 * The original razor project hosted on github(https://github.com/yuanrongxi/razor)
 */

#ifndef SIM_ENDPOINT_H
#define SIM_ENDPOINT_H
#include "ns3/socket.h"
#include "ns3/application.h"
#include "razor_header.h"
#include "sim_constants.h"
#include"sim_sender.h"
#include "sim_receiver.h"
namespace ns3
{
enum{
	sim_connect_notify = 1000,
	sim_network_timout,
	sim_disconnect_notify,
	sim_start_play_notify,
	sim_stop_play_notify,
	net_interrupt_notify,
	net_recover_notify,
};
enum
{
	session_idle = 0x00,
	session_connecting,
	session_connected,
	session_disconnected,
};
class SimEndpoint:public Application
{
public:
	SimEndpoint();
	virtual ~SimEndpoint();
	void InitialSetup(Ipv4Address dest_ip,uint16_t dest_port);
	void Bind(uint16_t port);
	void Disconnect();
	void RecvPacket(Ptr<Socket> socket);
	uint32_t Geyuid() {return m_uid;}
	void Setuid(uint32_t uid){m_uid=uid;}
	void Setcid(uint32_t cid){m_cid=cid;}
	void SetConnectionCallback(Callback<void,uint32_t>connectionSucceeded,
			Callback<void,uint32_t>connectionFailed);
	void SendPing(void);
	void Send(Ptr<Packet> packet);
private:
	void Connect();
    virtual void StartApplication ();
    virtual void StopApplication ();
    void do_connect();
    void ProcessConnect(sim_header_t sheader,sim_connect_t con);
    void ProcessConnectAck(sim_header_t sheader,sim_connect_ack_t ack);
    void ProcessPing(sim_header_t sheader,sim_ping_t ping);
    void ProcessPong(sim_header_t sheader,sim_pong_t pong);
    void ProcessDisconnect(sim_header_t sheader,sim_disconnect_t discon);
    void ProcessDisconnectAck(sim_header_t sheader,sim_disconnect_ack_t disconack);
    void CalculateRtt(uint32_t keep_rtt);
public:
    void RateInit(uint32_t bitrate,uint32_t framerate){
    	m_bitRate=bitrate;
    	m_frameRate=framerate;
    }
    void ChangeRate(uint32_t bitrate);
    void SenderInit();
    void ReceiverInit();
    void GenerateFakeData();
    void SetPacketSize(uint32_t size);
private:
    Ipv4Address m_peerIP;
    uint16_t m_peerPort;
    bool m_initiator;
    uint32_t m_uid;
    uint32_t m_cid;
    uint16_t m_port;
    uint8_t m_state;
    uint32_t m_rtt;
    uint32_t m_rtt_var;
    uint8_t m_conCount;
    uint8_t m_conRetries;
    Time m_conTimeout;
    Time m_heartbeat;
    EventId m_reConEvent;
    EventId m_pingEvent;
    Ptr<Socket> m_socket;
    bool m_firstRtt;

    uint32_t m_bitRate;
    uint32_t m_frameRate;
    EventId m_generateDataEvent;
    uint32_t m_packetSize;//default 1000byte
    int64_t m_lastGenerateDataTime;
    //Time m_generateDataTime;//MilliSeconds(m_frameRate);
private:
    Callback<void,uint32_t> m_connSucceeded;
    Callback<void,uint32_t> m_connFailed;

    SimSender *m_sender;
    SimReceiver *m_receiver;
};
}
#endif /* SIM_ENDPOINT_H */
