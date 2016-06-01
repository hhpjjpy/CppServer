#include <iostream>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>

#include "hhp_connector.h"

hhp::net::Connector::Connector(unsigned int uIP, unsigned int uPort, unsigned int uTimeOutMs):
	m_uIP(uIP),
	m_uPort(uPort),
	m_iTimeOutMs(uTimeOutMs),
	m_isockfd(-1)
{
}

hhp::net::Connector::~Connector()
{
	this->CloseFD();
}

int hhp::net::Connector::Send(const std::string & buf)
{
	if (m_isockfd < 0 && !this->ReConnect()) {
		return RET_ERROR;
	}
	if (buf.size() == 0) {
		m_sErrMsg.clear();
		m_sErrMsg.append(" send buf is null");
		return RET_NO_DATA;
	}

	int leftSize = buf.size();
	int iWritten = 0;
	const char * p = buf.data();
	while (leftSize > 0) {
		if ((iWritten = write(m_isockfd, p, leftSize)) <= 0) {
			if (errno == EINTR) {
				iWritten = 0;
			}
			else {
				this->CloseFD();
				m_sErrMsg.clear();
				m_sErrMsg.append("send error ").append(strerror(errno));
				return RET_ERROR;
			}
			leftSize -= iWritten;
			p += iWritten;
		}
	}

	return buf.size();
}

int hhp::net::Connector::Recv(std::string & msg,int paklen)
{
	msg.clear();
	int iLeft = paklen;
	int iTimeOutMs = m_iTimeOutMs;
	if (iTimeOutMs < 0) {
		iTimeOutMs = 100;
	}
	long lStartMs = NowMs();
	while (iLeft > 0&&iTimeOutMs>0)
	{
		std::string sTemp;
		int ret = this->RecvSome(iLeft,sTemp,iTimeOutMs);
		iTimeOutMs -= NowMs() - lStartMs;
		if(ret <= 0){
			return ret;
		}
		msg.append(sTemp);
		iLeft -= sTemp.size();
	}

	return msg.size();
}

bool hhp::net::Connector::ReConnect()
{ 
	this->CloseFD();
	if (m_uIP == 0 && m_uPort < 0) {
		m_sErrMsg.clear();
		m_sErrMsg.append("Connect is error , IP port not vaild ");
		return false;
	}
	int iFd = -1;
	m_isockfd = iFd;

	struct sockaddr_in stAddr;
	memset(&stAddr, 0, sizeof(stAddr));
	stAddr.sin_family = AF_INET;
	stAddr.sin_addr.s_addr = m_uIP;
	stAddr.sin_port = htons((unsigned short)m_uPort);

	iFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (iFd < 0)
	{
		m_sErrMsg.clear();
		m_sErrMsg.append("connect() socket error2:").append(strerror(errno));
		return false;
	}

	int fdFlag = 0;
	int ret = -1;

	//linger
	struct linger stLinger;
	stLinger.l_onoff = 1;
	stLinger.l_linger = 0;
	ret = setsockopt(iFd, SOL_SOCKET, SO_LINGER, (char *)&stLinger, sizeof(stLinger));
	if (ret != 0) {
		m_sErrMsg.clear();
		m_sErrMsg.append("set sockopt linger ERROR ");
		this->CloseFD();
		return false;
	}

	if (m_iTimeOutMs > 0)
	{
		fdFlag = fcntl(iFd, F_GETFL, 0);
		ret = fcntl(iFd, F_SETFL, fdFlag | O_NONBLOCK);
		if (ret < 0)
		{
			m_sErrMsg.clear();
			m_sErrMsg.append("connect()fcntl error3:").append(strerror(errno));
			this->CloseFD();
			return false;
		}

		ret = ::connect(iFd, (sockaddr*)&stAddr, sizeof(stAddr));
		if (ret < 0) {
			if (errno != EINPROGRESS) {
				m_sErrMsg.clear();
				return false;
			}
		}
		if (ret == 0) {
			m_isockfd = iFd;
			return true;
		}

		fd_set rset, wset;
		FD_ZERO(&rset);
		FD_SET(iFd,&rset);
		wset = rset;
		struct timeval tval;
		tval.tv_sec = m_iTimeOutMs / 1000;
		tval.tv_usec = (m_iTimeOutMs % 1000) * 1000;
		int n = 0;
		if ( (n = ::select(iFd + 1, &rset, &wset, NULL, &tval)) == 0 ) {
			close(iFd);
			errno = ETIMEDOUT;
			return false;
		}
		if (FD_ISSET(iFd, &rset) || FD_ISSET(iFd, &wset)) {
			int error;
			socklen_t len = sizeof(error);
			if (::getsockopt(iFd, SOL_SOCKET, SO_ERROR, &error, &len)<0) {
				m_sErrMsg.clear();
				m_sErrMsg.append("FDISSET ERROR ");
				return true;
			}
			else {
				m_isockfd = iFd;
				return true;
			}
		}
		else {
			return false;
		}

	}
	else {
	    ret =  ::connect(iFd,(sockaddr*)&stAddr,sizeof(stAddr));
		if (ret < 0) {
			m_sErrMsg.clear();
			m_sErrMsg.append("connect error4 ").append(strerror(errno));
			return false;
		}

		m_isockfd = iFd;
		return true;
	}

}

void hhp::net::Connector::CloseFD()
{
	if (m_isockfd > 0)  ::close(m_isockfd);
}

int hhp::net::Connector::RecvSome(int iMaxBytes, std::string & sOut, int iTimeOutMs)
{
	sOut.clear();
	if (m_isockfd < 0 || iMaxBytes < 0) {
		return RET_ERROR;
	}
	if (iTimeOutMs > 0) {
		
		struct pollfd rfd[1];
		rfd[0].fd = m_isockfd;
		rfd[0].events = POLLIN;
		
		int ret = poll(rfd, 1, iTimeOutMs);
		if (ret < 0) {
			this->CloseFD();
			m_sErrMsg.clear();
			m_sErrMsg.append(" recv Poll Error1 ; ").append(strerror(errno));
			return RET_ERROR;
		}
		else if (ret == 0) {
			this->CloseFD();
			m_sErrMsg.clear();
			m_sErrMsg.append(" recv error TIMEOUT ");
			return RET_TIME_OUT;
		}
	}

	char szBuf[20 * 1024];
	int recvSize = iMaxBytes>(int)sizeof(szBuf) ? (int)sizeof(szBuf) : iMaxBytes;
	
	int iRead = read(m_isockfd, szBuf, recvSize);
	if (iRead < 0) {
		if (errno == EINTR)
		{
			m_sErrMsg.clear();
			m_sErrMsg.append(" recv no data error  ").append(strerror(errno));
			return RET_NO_DATA;
		}
		else {
			this->CloseFD();
			m_sErrMsg.clear();
			m_sErrMsg.append("recv some error ").append(strerror(errno));
			return RET_ERROR;
		}
	}
	
	sOut.assign(szBuf, iRead);


	return sOut.size();
}
