// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    crsshair.h

    Crosshair handling.

***************************************************************************/

#ifndef MAME_EMU_CRSSHAIRUDP_H
#define MAME_EMU_CRSSHAIRUDP_H

#pragma once

#include <chrono>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <mutex>
#include <vector>
#include <atomic>
#include <thread>


#undef interface


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define CROSSHAIR_SCREEN_NONE   ((screen_device *) 0)
#define CROSSHAIR_SCREEN_ALL    ((screen_device *) ~0)

const int EXPIRATION_TIME_MS = 5000; // 5 seconds

struct ClientInfo {
    struct sockaddr_in address;
    std::chrono::steady_clock::time_point timestamp;
};

// ======================> crosshair_udp

class crosshair_udp
{
public:
	// construction/destruction
	crosshair_udp(running_machine &machine);

	// getters
	running_machine &machine() const { return m_machine; }

private:
	void exit();
	void animate(screen_device &device, bool vblank_state);
    void update_client(const struct sockaddr_in &client_address);
    void receive_clients(int server_socket);
    static bool is_expired(const ClientInfo &client);
    void update_position(int player);

	// internal state
	running_machine &       m_machine;                  // reference to our machine
    int                     m_server_socket;

    std::vector<ClientInfo> m_clients_list;
    std::mutex              m_clients_mutex;

    //crosshair position
    float                   m_x;
    float                   m_y;

    // Screen size for reference
    int                     m_screen_width;
    int                     m_screen_height;

    // Are we running?
    std::atomic<bool>       m_running;

    //Receive thread
    std::unique_ptr<std::thread> m_recv_thread;
};

#endif  /* MAME_EMU_CRSSHAIRUDP_H */
