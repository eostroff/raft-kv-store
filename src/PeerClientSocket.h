#ifndef __PEERCLIENTSOCKET_H__
#define __PEERCLIENTSOCKET_H__

#include <string>

#include "Socket.h"


class PeerClientSocket: public Socket {
public:
	PeerClientSocket() {}
	~PeerClientSocket() {}

	int Init(std::string ip, int port);
};


#endif // end of #ifndef __PEERCLIENTSOCKET_H__