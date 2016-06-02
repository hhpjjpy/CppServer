#pragma once
#ifndef EPOLL_REACTOR_H
#define EPOLL_REACTOR_H
#include <sys/epoll.h>
#include <errno.h>
#include <string>
#include <vector>
#include "reactor.h"

namespace hhp {
	namespace reactor {
    #define EPOLLWAITNUM 20

		class CEpollReactor : public CReactor {
		public:
			CEpollReactor();
			~CEpollReactor();
			int Init();
			virtual int RegistIn(CEventChannel&& channel);
			virtual int RegistOut(CEventChannel&& channel);
		private:
			virtual int WaitEvent(int iTimeOut);
			std::vector<epoll_event> m_vecEpollEvents;
			int m_iEpollFd;
		};
	}
}
#endif
