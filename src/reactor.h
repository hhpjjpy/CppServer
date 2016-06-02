#pragma once
#ifndef REACTOR_H
#define REACTOR_H
#include <map>
#include <vector>
#include <unistd.h>
namespace hhp {
	namespace reactor{
		static const unsigned int  g_max_err_msg_len = 256;
		enum EVENT
		{
			ON_READ = 0x01,  //读事件
			ON_WRITE = 0x04, //写事件
			ON_REEOR = 0x08, //出错
			ON_MOD  =  0x10, //边缘触发
		};

		class CEvevtHandler
		{
		public:
			CEvevtHandler() :
			m_iFd(-1)
			{  };
			virtual ~CEvevtHandler() {
				this->Close();
			}
			//可读
			virtual int OnRead() = 0;
			//可写
			virtual int OnWrite() = 0;
			//出错
			virtual int onErr();

			 
			int GetFd() { return m_iFd; }

			int Close() {
				if (m_iFd < 0) {
					::close(m_iFd);
				}
			}

		protected:
			int m_iFd;

		};

		class CEventChannel 
		{
		public:
			CEventChannel(int fd, int Event,CEvevtHandler *handler):
				m_iFd(fd),
				m_iEvent(Event),
				m_iRevent(0),
				m_oEventHeadler(handler)
			{

			}
			void setRevevt(int Revent) { m_iRevent = Revent; }
			void HandleEvent() {
				if (m_iRevent&ON_READ) {
					m_oEventHeadler->OnRead();
				}
				if (m_iRevent&ON_WRITE) {
					m_oEventHeadler->OnWrite();
				}
				if (m_iEvent&ON_REEOR) {
					m_oEventHeadler->onErr();
				}
			}
		public:
			int m_iFd;
			int m_iEvent;
			int m_iRevent;
			CEvevtHandler* m_oEventHeadler;
		};

		class CReactor
		{
		public:
			CReactor():
			m_bQuit(false)
			{   }

			virtual ~CReactor() {

			}

			//处理事件
			int EventHandle(int iTimeOut){
				this->WaitEvent(iTimeOut);
				for (auto it = m_vecActiveEvents.begin(); it != m_vecActiveEvents.end(); it++) {
					it->HandleEvent();
				}
			}

			void RunLoop() {
				while (!m_bQuit) {
					this->EventHandle(-1);
				}
			}

			void QuitLoop() {
				m_bQuit = true;
			}

			virtual int RegistIn(CEventChannel&& oChennel) = 0;

			virtual int RegistOut(CEventChannel&& oChennel) = 0;

		protected:
			virtual int WaitEvent(int iTimeOut) = 0;

		protected:
			std::map<int, CEventChannel> m_mapEvents;
			std::vector<CEventChannel> m_vecActiveEvents;
			bool m_bQuit;

		};


	}
}


#endif // !REACTOR_H
