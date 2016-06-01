#pragma once
#ifndef CPPSTUB_H
#define CPPSTUB_H
#include <arpa/inet.h>
#include <cstdlib>

#include "hhp_connector.h"
#include "serialize.h"
#include "Log.h"

namespace app{
	namespace stub {
		template < uint32_t _iCmd, typename IN, typename OUT >
		struct Protocol
		{
			static const uint32_t iCmd = _iCmd;
			typedef IN in;
			typedef OUT out;
		};

		struct  ReqHead {
			int   dwPkgLen;
			int   dwCmd;
			int   dwSockfd;
			//int   dwCallIP;
			//int   dwCallPort;
			//long  dwSeqno;
			long  lReqTimeStamp;
			long  lRecvTimeStamp;

			ReqHead() :
			dwPkgLen(0),dwCmd(0),dwSockfd(-1),lReqTimeStamp(0),lRecvTimeStamp(0)
			{

			}

			template<class AR>
			AR & serialize(AR & ar)
			{
				ar & dwPkgLen;
				ar & dwCmd;
				ar & sockfd;
				//ar & dwSeqno;
				//ar & dwCallIP;
				//ar & dwCallPort;
				ar & lReqTimeStampl;
				ar & lRecvTimeStamp;
				return ar;
			}

		};

		template<typename T>
		class hhpStub {
		public:
			hhpStub(unsigned int uIP, unsigned int uPort, int uTimeOutMs = 100):
			m_Connector(uIP,uPort, uTimeOutMs)
			{
				INITLOG("hhpStub", "./log");
			}
			hhpStub(const char* sIP, unsigned int uPort, int uTimeOutMs = 100) :
				m_Connector(::inet_addr(sIP), uPort, uTimeOutMs)
			{
				INITLOG("hhpStub", "./log");
			}
			int Invoke(const typename T::in & oReq, typename T::out & oResp) {
				struct  ReqHead cHead;
				cHead.dwCmd = T::iCmd;
				cHead.lReqTimeStamp = hhp::net::NowMs();

				hhp::serialize::Buffer sBuff;
				cBuf.setversion(0);

				sBuff << cHead;
				sBuff << oReq;
				sBuff.setpkglen();
				
				std::string sMsg;
				sMsg.assign(sBuff.getbuf(), sBuff.length());

				std::string recvMsg;
				if (sMsg.size() != m_Connector.Send(sMsg)) {
					LOG(INFO) << " send msg error " << m_Connector.GetErrMsg() << std::endl;
					return -1;
				}
				else {
					int iPkgLen;
					std::string strLen;
					int iRead = m_Connector.Recv(strLen,sizeof(iPkgLen));
					iPkgLen = ::atoi(strLen);
					if (iRead != (int)sizeof(iPkgLen) || iPkgLen < 0 || iPkgLen>10 * 1024 * 1204) {
						LOG(INFO) << " recv pkglen error " << m_Connector.GetRrrMsg() << std::endl;
						return -2;
					}
					std::string recvMsg;
					int iLeft = iPkgLen - sizeof(iPkgLen);
					iRead = m_Connector.Recv(recvMsg, iLeft);

					if (iRead != iLeft) {
						LOG(INFO) << " recv Msg error " << m_Connector.GetRrrMsg() << std::endl;
						return -3;
					}

					hhp::serialize::Parser oParser(recvMsg.c_str(),recvMsg.size());
					oParser.setversion(0);

					oParser >> oResp;

					return 0;
				}

			}

		private:
			hhp::net::Connector m_Connector;
		};

	}
}




#endif // !CPPSTUB_H
