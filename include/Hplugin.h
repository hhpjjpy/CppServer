#ifndef HPLUGIN_H
#define HPLUGIN_H

struct  ReqHead {
	int   dwSockfd;
	int   dwPkgLen;
	long  dwSeqno;
	long  lReqTimeStamp;
	long  lRecvTimeStamp;
};

template < uint32_t _iCmd, typename IN, typename OUT = NilType >
struct Protocol
{
	static const uint32_t iCmd = _iCmd;
	typedef IN in;
	typedef OUT out;
};

#define HHP_DEFINE(CMD,PROTOCOL,_CLASS,MOTHOED) \
extern "C" { \
		hhp::plugin::CPlugin * make_##CMD(){return new _CLASS(); }\
		void destory_##CMD(hhp::plugin::CPlugin* p ){if(p!=NULL) delete p; }\
		int  servant_##CMD(hhp::plugin::CPligin* p ,const char * sReq,int reqLen,char * sResq,int MaxLen){ \
			int iRet = 0;\
			PROTOCOL::in  req;\
			PROTOCOL::out resp;\
			boss::serialize::Parser cParser(szReq,reqLen);\
			cParser.setversion(0);\
			try { cParser>>req; }\
			catch ( ... ) {iRet =  0xEFFFFFFF;}\
		if(iRet == 0) {\
			iRet = ((_CLASS*)(p))->MOTHOED(req,resp);\
			if(iRet<0){  } \
			boss::serialize::Buffer cBuf;\
			cBuf<<resp;\
			memcpy(szResp,cBuf.getbuf(),cBuf.length());}\  
			return iRet;  }\			
}


namespace hhp {
	namespace plugin {
		class CPlugin;

		typedef CPlugin * maker_t();
		typedef void destroy_t(CPlugin *);
		typedef int servant_t(CPlugin *, const char *, int, char *, int);

		class CPlugin{
			//一些全局配置可在此初始化处理
		};
	}
}


#endif // !HPLUGIN_H

