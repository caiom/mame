// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    crsshairudp.cpp

    Exposes crosshair position via UDP.

***************************************************************************/

#include "emu.h"
#include "crsshairudp.h"

#include "config.h"
#include "emuopts.h"
#include "fileio.h"
#include "render.h"
#include "rendutil.h"
#include "screen.h"

#include "xmlfile.h"
#include <iostream>


//**************************************************************************
//  CROSSHAIR MANAGER
//**************************************************************************

//-------------------------------------------------
//  crosshair_manager - constructor
//-------------------------------------------------

crosshair_udp::crosshair_udp(running_machine &machine): m_machine(machine)
{
    m_running = true;
    std::cout << "Constructor " << std::endl;
	/* request a callback upon exiting */
	machine.add_notifier(MACHINE_NOTIFY_EXIT, machine_notify_delegate(&crosshair_udp::exit, this));
    struct sockaddr_in server_address;

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return;
    }

    std::cout << "Init " << std::endl;

    m_server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_server_socket == INVALID_SOCKET) {
        std::cerr << "Cannot create socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    std::cout << "Socket " << std::endl;

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(3374);

    if (bind(m_server_socket, (SOCKADDR*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(m_server_socket);
        WSACleanup();
        return;
    }

    std::cout << "bind " << std::endl;

    int timeout_ms = 1000; // Timeout in milliseconds
    if (setsockopt(m_server_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms)) == SOCKET_ERROR) {
        std::cerr << "Setsockopt failed: " << WSAGetLastError() << std::endl;
        closesocket(m_server_socket);
        WSACleanup();
        return;
    }

    std::cout << "timeout " << std::endl;

    m_recv_thread = std::make_unique<std::thread>(&crosshair_udp::receive_clients, this, m_server_socket);


    std::cout << "thread " << std::endl;

	/* register the vblank callback */
	screen_device *first_screen = screen_device_enumerator(machine.root_device()).first();
	if (first_screen)
    {
		first_screen->register_vblank_callback(vblank_state_delegate(&crosshair_udp::animate, this));
        m_screen_width = first_screen->width();
        m_screen_height = first_screen->height();
    }

    std::cout << "screen " << std::endl;
}

void crosshair_udp::receive_clients(int server_socket) 
{
    std::cout << "receive_clients " << std::endl;
    struct sockaddr_in client_address;
    int client_address_size = sizeof(client_address);

    while (true) {
        char buffer[1024] = {0};
        std::cout << "waiting message " << std::endl;
        int bytes_received = recvfrom(server_socket, buffer, sizeof(buffer), 0,
                                      (struct sockaddr *)&client_address, &client_address_size);


        std::cout << "message received " << std::endl;
        if (bytes_received > 0 && buffer[0] == 12 && buffer[1] == 37) {
            std::lock_guard<std::mutex> lock(m_clients_mutex);
            std::cout << "updating clients " << std::endl;
            update_client(client_address);
        }

        if (!m_running)
            break;
    }
}

void crosshair_udp::update_client(const struct sockaddr_in &client_address) {
    for (auto &client : m_clients_list) {
        if (client.address.sin_addr.s_addr == client_address.sin_addr.s_addr &&
            client.address.sin_port == client_address.sin_port) {
            client.timestamp = std::chrono::steady_clock::now();
            return;
        }
    }

    m_clients_list.push_back({client_address, std::chrono::steady_clock::now()});
    std::cout << "Client added: " << inet_ntoa(client_address.sin_addr) << ":"
              << ntohs(client_address.sin_port) << std::endl;
}

bool crosshair_udp::is_expired(const ClientInfo &client) {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - client.timestamp).count();
    return duration > EXPIRATION_TIME_MS;
}

void crosshair_udp::update_position(int player)
{
    std::cout << "updating position " << std::endl;
	// read all the lightgun values
	bool gotx = false, goty = false;
	for (auto const &port : m_machine.ioport().ports())
		for (ioport_field const &field : port.second->fields())
			if (field.player() == player && field.crosshair_axis() != CROSSHAIR_AXIS_NONE && field.enabled())
			{
				// handle X axis
				if (field.crosshair_axis() == CROSSHAIR_AXIS_X)
				{
					m_x = field.crosshair_read();
					printf("X: %f\n", m_x);
					gotx = true;
					if (field.crosshair_altaxis() != 0)
					{
						m_y = field.crosshair_altaxis();
						goty = true;
					}
				}

				// handle Y axis
				else
				{
					printf("Y: %f\n", m_y);
					m_y = field.crosshair_read();
					goty = true;
					if (field.crosshair_altaxis() != 0)
					{
						m_x = field.crosshair_altaxis();
						gotx = true;
					}
				}

				// if we got both, stop
				if (gotx && goty)
					return;
			}
}

/*-------------------------------------------------
exit - free memory allocated for
the crosshairs
-------------------------------------------------*/

void crosshair_udp::exit()
{
    std::cout << "exiting " << std::endl;
    m_running = false;
    closesocket(m_server_socket);
    WSACleanup();
    m_recv_thread->join();
}


/*-------------------------------------------------
    animate - animates the crosshair once a frame
-------------------------------------------------*/

void crosshair_udp::animate(screen_device &device, bool vblank_state)
{
    int screen_width = device.visible_area().width();
    int screen_height = device.visible_area().height();
    std::cout << "animate " << std::endl;
    // Update position of player 0
	update_position(0);

    // Send position to every client
    socklen_t client_address_size = sizeof(struct sockaddr_in);

    char buffer[18] = {34};
    memcpy(&(buffer[2]), (void*)&screen_width, sizeof(screen_width));
    memcpy(&(buffer[6]), (void*)&screen_height, sizeof(screen_height));
    memcpy(&(buffer[10]), (void*)&m_x, sizeof(m_x));
    memcpy(&(buffer[14]), (void*)&m_y, sizeof(m_y));

    std::cout << "buffer criated " << std::endl;

    // std::vector<ClientInfo> clients_copy;
    {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        m_clients_list.erase(std::remove_if(m_clients_list.begin(), m_clients_list.end(), &crosshair_udp::is_expired), m_clients_list.end());
        // clients_copy = m_clients_list;

        for (const auto &client : m_clients_list)
            sendto(m_server_socket, buffer, 18, 0, (struct sockaddr *)&client.address, client_address_size);
    }

    std::cout << "sent buffer " << std::endl;
}
