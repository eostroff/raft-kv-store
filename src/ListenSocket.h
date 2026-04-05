#ifndef __LISTENSOCKET_H__
#define __LISTENSOCKET_H__

#include <memory>

#include "Socket.h"

class ListenSocket: public Socket {
public:
	ListenSocket() {}
	~ListenSocket() {}

	ListenSocket(int fd, bool nagle_on = NAGLE_ON);

	bool Init(int port);
	std::unique_ptr<ListenSocket> Accept();
};


#endif // end of #ifndef __LISTENSOCKET_H__