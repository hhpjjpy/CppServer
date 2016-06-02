#include "epoll_reactor.h"
#include "../include/Log.h"



int ConverToEpollEvent(int ev)
{
	int iEpollEvent = 0;
	if (ev & hhp::reactor::ON_READ) {
		iEpollEvent |= EPOLLIN;
	}
	if (ev & hhp::reactor::ON_WRITE) {
		iEpollEvent |= EPOLLOUT;
	}
	if (ev & hhp::reactor::ON_MOD) {
		iEpollEvent |= EPOLLET;
	}
	return iEpollEvent;
}

int ConverToREvent(int epollev)
{
	int iEvent = 0;
	if (epollev & (EPOLLIN | EPOLLPRI | EPOLLHUP)) {
		iEvent |= hhp::reactor::ON_READ;
	}
	if (epollev & EPOLLOUT) {
		iEvent |= hhp::reactor::ON_WRITE;
	}
	if (epollev |= EPOLLERR) {
		iEvent |= hhp::reactor::ON_REEOR;
	}
	return iEvent;
}


hhp::reactor::CEpollReactor::CEpollReactor() :
	m_vecEpollEvents(EPOLLWAITNUM),
	m_iEpollFd(-1)
{
}

hhp::reactor::CEpollReactor::~CEpollReactor()
{
	if (m_iEpollFd > 0) {
		::close(m_iEpollFd);
	}
}

int hhp::reactor::CEpollReactor::Init()
{
	if ((m_iEpollFd = ::epoll_create(100)) < 0) {
		LOG(INFO) << "epoll_create ERROR" <<std::endl;
		return -1;
	}
	return 0;
}

int hhp::reactor::CEpollReactor::RegistIn(CEventChannel && channel)
{
	struct epoll_event oEpollEvent;
	oEpollEvent.data.fd = channel.m_iFd;
	oEpollEvent.events = ConverToEpollEvent(channel.m_iEvent);
	
	int op = -1;
	auto it = m_mapEvents.find(channel.m_iFd);
	if (it != m_mapEvents.end()) {
		op = EPOLL_CTL_MOD;
		oEpollEvent.events |= ConverToEpollEvent((it->second).m_iEvent);
		(it->second).m_iEvent = ConverToREvent(oEpollEvent.events);
	}
	else {
		m_mapEvents[channel.m_iFd] = channel;
		op = EPOLL_CTL_ADD;
	}

	if (::epoll_ctl(m_iEpollFd, op, channel.m_iFd, &oEpollEvent) != 0) {
		LOG(INFO) << " epoll_ctl error " << std::endl;
		return -1;
	}

	return 0;
}

int hhp::reactor::CEpollReactor::RegistOut(CEventChannel && channel)
{
	auto it = m_mapEvents.find(channel.m_iFd);
	if (it == m_mapEvents.end()) {
		return 0;
	}
	struct  epoll_event oEpollEvent;
	oEpollEvent.data.fd = channel.m_iFd;
	oEpollEvent.events = ConverToEpollEvent(channel.m_iEvent);
	
	if (::epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, channel.m_iFd, &oEpollEvent)<0) {
		LOG(INFO) << " epoll del error " << std::endl;
		return -1;
	}
	
	m_mapEvents.erase(channel.m_iFd);

	return 0;
}

int hhp::reactor::CEpollReactor::WaitEvent(int iTimeOut)
{
	int iReadyNum = ::epoll_wait(m_iEpollFd, m_vecEpollEvents.data(), m_vecEpollEvents.size(),iTimeOut);
	if (iReadyNum < 0) {
		LOG(INFO) << " epoll wait error " << std::endl;
		return -1;
	}
	for (int i = 0; i < iReadyNum;i++){
		auto mapIt = m_mapEvents.find(m_vecEpollEvents[i].data.fd);
		if (mapIt == m_mapEvents.end()) {
			LOG(INFO) << " fd : " << m_vecEpollEvents[i].data.fd<<" have error not find in map"<<std::endl;
			continue;
		}
		
		CEventChannel oChannel = mapIt->second;
		oChannel.setRevevt(ConverToREvent(m_vecEpollEvents[i].events));
		m_vecActiveEvents.push_back(std::move(oChannel));
	}


	return 0;
}


