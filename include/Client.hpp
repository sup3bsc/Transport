#pragma once

#include <EventEmitter.hpp>
#include <Packet.hpp>
#include <Events.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <pcap.h>

namespace Transport {
	class Client : public EventEmitter<Client> {
		protected:
			ethernet_header constructEH();
			ip_header constructIH();
			tcp_header constructTH();
		public:
			Client();
			void send(void* data = nullptr, const uint32_t size = 0);
			~Client();

			ethernet_header eh;
			ip_header ih;
			tcp_header th;
		private:
			pcap_if_t *alldevs;
			std::vector<std::thread*> threads;
	};
}