#include <Server.hpp>

using namespace std;

namespace Transport {
	void Server::listen(uint16_t port) {
			// Retrieve the device list
			char listBuf[PCAP_ERRBUF_SIZE];
			if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, listBuf) == -1) {
				return emit(Event::Error("Error in pcap_findalldevs: " + string(listBuf)));
			}

			// Print the list
			int inum = 0;
			pcap_if_t *d;
			for (d = alldevs; d; d = d->next) {
				threads.push_back(new thread([=]() {
					try {
						// Open the adapter
						pcap_t *adhandle;
						char errbuf[PCAP_ERRBUF_SIZE];
						if ((adhandle = pcap_open(d->name,	// name of the device
							65536,							// portion of the packet to capture.
							PCAP_OPENFLAG_PROMISCUOUS,		// promiscuous mode
							1000,							// read timeout
							NULL,							// remote authentication
							errbuf							// error buffer
							)) == NULL) {
							throw Event::Error("Unable to open the adapter. %s is not supported by WinPcap");
						}

						// Check the link layer. We support only Ethernet for simplicity.
						if (pcap_datalink(adhandle) != DLT_EN10MB) {
							throw Event::Error("This program works only on Ethernet networks.");
						}

						u_int netmask;
						if (d->addresses != NULL)
							netmask = ((struct sockaddr_in *)(d->addresses->netmask))->sin_addr.S_un.S_addr;
						else
							netmask = 0xffffff;


						string filter = "ether proto \\arp or ip proto \\icmp or \\tcp";
						// Compile the filter
						struct bpf_program fcode;
						if (pcap_compile(adhandle, &fcode, filter.c_str(), 1, netmask) < 0) {
							throw Event::Error("Unable to compile the packet filter. Check the syntax.");
						}

						// Set the filter
						if (pcap_setfilter(adhandle, &fcode) < 0) {
							throw Event::Error("Error setting the filter.");
						}

						auto localIP = *(ip_address*)(d->addresses->addr->sa_data);
						
						ostringstream buffer;
						buffer << "Listening on " << localIP << ":" << port << "..." << endl; // d->description
						cout << buffer.str();

						ip_address allIP{ 0, 0, 0, 0 };
						bool acceptAll = (localIP == allIP);

						// Start the capture
						struct pcap_pkthdr* header;
						const uint8_t* pkt_data;
						int res;
						while ((res = pcap_next_ex(adhandle, &header, &pkt_data)) >= 0) {
							if (res == 0)
								continue;

							// Retrieve the Ethernet header
							auto eh = (ethernet_header*)pkt_data;
							uint32_t eth_len = sizeof(ethernet_header);
							ntoh(eh);

							switch (eh->type) {
								case ARP: {
									// Retrieve the ARP header
									auto ah = (arp_header*)(pkt_data + eth_len);
									if(ah->htype == 1)
										cout << "ARP" << endl;
								}
								break;

								case IPV4: {
									// Retrieve the IP header
									auto ih = (ip_header*)(pkt_data + eth_len);
									uint32_t ip_len = ih->hlen * 4;

									if (ih->daddr == localIP || acceptAll) {
										switch (ih->proto) {
											case ICMP: { // ICMP
												auto ich = (icmp_header*)(pkt_data + eth_len + ip_len);
												ntoh(ich);

												ostringstream buffer;
												buffer << "ICMP: " << ich->code << endl;
												cout << buffer.str();

												// Print data
												printHex((uint8_t*)ich, sizeof(icmp_header));
											}
											break;

											case TCP: {// TCP
												// Retrieve the TCP header
												auto th = (tcp_header*)(pkt_data + eth_len + ip_len);
												ntoh(th);

												if (port == NULL || th->dport == port) {
													// Retrieve payload
													uint32_t tcp_len = th->offset * 4;
													uint32_t payload_len = (header->caplen - eth_len - ip_len - tcp_len);
													auto payload = (uint8_t*)(pkt_data + eth_len + ip_len + tcp_len);

													// Emit a connect event
													emit(Event::Connect(Request(*eh, *ih, *th, header->ts.tv_sec, header->caplen)));

													// Print payload
													printHex(payload, payload_len);
												}
											}
											break;
										}
									}
								}
								break;
							}
						}
					} catch (const Event::Error& err) {
						emit(err);
					}
				}));
			}
	}

	Server::~Server() {
		for (auto thr : threads) {
			thr->join();
			delete thr;
		}

		pcap_freealldevs(alldevs);
	}

	// Print payload data as hex
	void Server::printHex(uint8_t* data, uint32_t length) {
		char line[17];
		ostringstream buffer;
		for (uint32_t i = 0; i < length; i++) {
			uint8_t c = data[i];

			//Print the hex value for every character , with a space
			ostringstream asHex;
			asHex << setw(2) << setfill('0') << hex << (uint32_t)c;
			buffer << " " << asHex.str();

			//Add the character to data line
			uint8_t a = (c >= 32 && c <= 128) ? (uint8_t)c : '.';

			line[i % 16] = a;

			//if last character of a line , then print the line - 16 characters in 1 line
			if ((i != 0 && (i + 1) % 16 == 0) || i == length - 1) {
				line[i % 16 + 1] = '\0';

				//print a big gap of 10 characters between hex and characters
				buffer << "          ";

				//Print additional spaces for last lines which might be less than 16 characters in length
				for (int j = strlen(line); j < 16; j++) {
					buffer << "   ";
				}

				buffer << line << " " << endl;
			}
		}

		cout << buffer.str();
	}
}
