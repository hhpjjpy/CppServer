#pragma once
#ifndef HHP_CONNECTOR_H
#define HHP_CONNECTOR_H
#include <string>
#include <stack>
#include <vector>
#include <sys/time.h>

namespace hhp {
	namespace net {
		inline long NowMs()
		{
			struct timeval  secMillisec;
			gettimeofday(&secMillisec, NULL);
			return (long)((secMillisec.tv_sec * 1000) + secMillisec.tv_usec / 1000);
		}

		class Connector{
			public:
				Connector(unsigned int uIP, unsigned int uPort,unsigned int uTimeOutMs=100);
				~Connector();
				int Send(const std::string& buf);
				int Recv(std::string& msg,int pkglen);
				const std::string& GetErrMsg() { return m_sErrMsg; }

				enum
				{
					RET_NO_DATA = 0,
					RET_ERROR = -1,
					RET_TIME_OUT = -2,
					RET_PEER_CLOSED = -3,
				};

		    private:
				bool ReConnect();
				void CloseFD();
				int RecvSome(int iMaxBytes, std::string & sOut, int iTimeOutMs = 100);

				unsigned int m_uIP;
				unsigned int m_uPort;
				int m_iTimeOutMs;
				int m_isockfd;
				std::string m_sErrMsg;
		};
	}
}



#endif // !HHP_CONNECTOR_H
