
#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <list>
#include <map>
#include <stack>
#include <string.h>
#include <stdlib.h>

#include <ext/hash_map>
extern "C" {
#include<arpa/inet.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/param.h>
}


// int64网络字节序转换为主机字节序
inline uint64_t ntoh64(uint64_t val)
{
    uint64_t ret = val;
#if __BYTE_ORDER == __LITTLE_ENDIAN
    ret = (((uint64_t)ntohl((uint32_t)val)) << 32) | ntohl((uint32_t)(val >> 32));
#endif
    return ret;
}

#define hton64(h) ntoh64(h)


/*Buffer 内部维护的buffer的每次增长的步长*/
#ifdef  _NO_DUMP_NAME_
#define _SERIALIZE_(ar,t) ar & t;
#else
#define _SERIALIZE_(ar,t) do{ hhp::serialize::CDummy __o___(#t);ar & __o___ & t ;}while(0);
#endif


#define DUMP_OBJ(obj)    hhp::serialize::CSDumper().c_str(obj,#obj,hhp::serialize::FOMAT_JSON,false)
#define DUMP_OBJ_ML(obj) hhp::serialize::CSDumper().c_str(obj,#obj,hhp::serialize::FOMAT_JSON,true)

namespace hhp
{

    namespace serialize
    {

        static const int UINT64_HEX_BUF_LEN = 17;
        static const int BUF_BLK_SIZE       = 4096;

        class CDummy
        {
        public:
            CDummy(const char *szName): m_pName(szName) {}
            std::string GetName()
            {
                return m_pName;
            }
        private:
            const char *m_pName;
        };

        class Buffer
        {
        public:
            template<typename DATA>
            Buffer &operator &(DATA &d)
            {
                int32_t iLen = this->length();
                if (_version > 0)
                {
                    *this&(int32_t)0;
                }
                d.serialize(*this);
                if (_version > 0)
                {
                    int32_t iLenOfd = this->length() - iLen - sizeof(int32_t);
                    *(int *)(_buf + iLen)  = htonl(iLenOfd);
                }
                return *this;
            }

            Buffer &operator &(CDummy &d)
            {
                return *this;
            }

            inline Buffer &operator & (int32_t d)
            {
                check(sizeof(d));
                int32_t i = htonl(d);
                memcpy(_p, &i, sizeof(i));
                _p += sizeof(i);
                return *this;
            }

            inline Buffer &operator & (int16_t d)
            {
                check(sizeof(d));
                int16_t i = htons(d);
                memcpy(_p, &i, sizeof(i));
                _p += sizeof(i);
                return *this;
            }

            inline Buffer &operator & (uint16_t d)
            {
                check(sizeof(d));
                uint16_t i = htons(d);
                memcpy(_p, &i, sizeof(i));
                _p += sizeof(i);
                return *this;
            }

            inline Buffer &operator & (int8_t d)
            {
                check(sizeof(d));
                int8_t i = d;
                memcpy(_p, &i, sizeof(i));
                _p += sizeof(i);
                return *this;
            }

            inline Buffer &operator & (uint8_t d)
            {
                check(sizeof(d));
                uint8_t i = d;
                memcpy(_p, &i, sizeof(i));
                _p += sizeof(i);
                return *this;
            }

            inline Buffer &operator & (std::string &s)
            {
                check(sizeof(s.size()));
                (*this)&(uint32_t)s.size();
                check(s.size());
                memcpy(_p, s.c_str(), s.size());
                _p += s.size();
                return *this;
            }

            inline Buffer &operator & (const std::string &s)
            {
                check(sizeof(s.size()));
                (*this)&(uint32_t)s.size();
                check(s.size());
                memcpy(_p, s.c_str(), s.size());
                _p += s.size();
                return *this;
            }

            template<typename DATA>
            Buffer &operator & (std::vector<DATA> &vec)
            {
                uint32_t sz = vec.size();
                (*this)&sz;
                for ( uint32_t i = 0; i < sz; ++i )
                    (*this)&vec[i];
                return *this;
            }
            // added by jarry

            inline Buffer &operator & (uint32_t d)
            {
                (*this)&(int32_t)d;
                return (*this);
            }

            inline Buffer &operator & (uint64_t d)
            {
                if (!_int64_use_str)
                {
                    check(sizeof(d));
                    uint64_t i = hton64(d);
                    memcpy(_p, &i, sizeof(i));
                    _p += sizeof(i);
                }
                else
                {
                    check(hhp::serialize::UINT64_HEX_BUF_LEN);
                    char szllBuffer[hhp::serialize::UINT64_HEX_BUF_LEN + 1] = {0};

#if defined (__LP64__) || defined (__64BIT__) || defined (_LP64) || (__WORDSIZE == 64)
    #if defined(__APPLE__) && defined(__GNUC__) 
                    snprintf(szllBuffer, hhp::serialize::UINT64_HEX_BUF_LEN, "%016llx", d);
    #else
                    snprintf(szllBuffer, hhp::serialize::UINT64_HEX_BUF_LEN, "%016lx", d);
    #endif
#else
                    snprintf(szllBuffer, hhp::serialize::UINT64_HEX_BUF_LEN, "%016llx", d);
#endif

                    memcpy(_p, szllBuffer, hhp::serialize::UINT64_HEX_BUF_LEN);
                    _p += hhp::serialize::UINT64_HEX_BUF_LEN;
                }
                return *this;
            }

            inline Buffer &operator & (int64_t d)
            {
                (*this)&(uint64_t)d;
                return (*this);
            }

            template<typename DATA>
            Buffer &operator & (std::list<DATA> &ls)
            {
                typedef typename std::list<DATA>::iterator MyIterater;
                uint32_t sz = ls.size();
                (*this)&sz;
                for (MyIterater p = ls.begin(); p != ls.end(); ++p )
                {
                    (*this)&(*p);
                }
                return *this;
            }

            template<typename LEFT, typename RIGHT>
            Buffer &operator & (std::pair<LEFT, RIGHT> &p)
            {
                (*this) & (p.first);
                (*this) & (p.second);
                return *this;
            }

            template<typename KEY, typename VALUE>
            Buffer &operator & (std::map<KEY, VALUE> &mp)
            {
                typedef typename std::map<KEY, VALUE>::iterator MyIterater;
                uint32_t sz = mp.size();
                (*this)&sz;
                for (MyIterater p = mp.begin(); p != mp.end(); ++p )
                {
                    (*this)&(p->first);
                    (*this)&(p->second);
                }
                return *this;
            }

            template<typename KEY, typename VALUE>
            Buffer &operator & (std::multimap<KEY, VALUE> &mp)
            {
                typedef typename std::multimap<KEY, VALUE>::iterator MyIterater;
                uint32_t sz = mp.size();
                (*this)&sz;
                for (MyIterater p = mp.begin(); p != mp.end(); ++p )
                {
                    (*this)&(p->first);
                    (*this)&(p->second);
                }
                return *this;
            }

            template<typename KEY, typename VALUE>
            Buffer &operator & (__gnu_cxx::hash_map<KEY, VALUE> &mp)
            {
                typedef typename __gnu_cxx::hash_map<KEY, VALUE>::iterator MyIterater;
                uint32_t sz = mp.size();
                (*this)&sz;
                for (MyIterater p = mp.begin(); p != mp.end(); ++p )
                {
                    (*this)&(p->first);
                    (*this)&(p->second);
                }
                return *this;
            }
            //!add
            template<typename DATA>
            Buffer &operator <<(DATA &d)
            {
                return (*this)&d;
            }

            Buffer(): _capacity(BUF_BLK_SIZE), _version(0), _int64_use_str(true)
            {
                _buf = new char[_capacity];
                _p = _buf;
                //memset(_buf,0,BUF_BLK_SIZE);
            }

            ~Buffer()
            {
                if ( _buf )
                {
                    delete [] _buf;
                }
            }

            inline void reset()
            {
                _p = _buf;
            }

            inline int32_t length() const
            {
                return _p - _buf;
            }
            inline int32_t capacity() const
            {
                return _capacity;
            }
            inline  void check(int32_t sz)
            {
                if ( (length() + sz) > capacity() )
                    rise(sz - (capacity() - length()));
            }

            inline const char *getbuf()
            {
                return _buf;
            }


            void setpkglen()
            {
                *((int32_t *)_buf) = htonl(this->length());
            }

            void setversion(uint32_t ver)
            {
                _version = ver;
            }

            void setint64(bool useStr)
            {
                _int64_use_str = useStr;
            }

        private:
            void rise(int32_t n)
            {
                uint32_t len = 2 * capacity();
                if ( n > capacity())
                {
                    len = capacity() + n;//翻倍不够
                }
                char *old = _buf;
                int32_t pos = _p - old;
                _buf = new char[len];
                _capacity = len;
                _p = _buf + pos;
                //memset(_buf,0,BUF_BLK_SIZE * _alloc_time);
                memcpy(_buf, old, pos);
                delete [] old;
                //printf("%u\n",capacity());
            }

        private:
            char *_buf;
            char *_p;
            int32_t  _capacity;
            int32_t  _version;
            bool     _int64_use_str;
        };

        class Parser
        {
        public:
            template<typename DATA>
            Parser &operator &(DATA &d)
            {
                int32_t iLen = 0;
                int32_t iLeft = 0;
                if (_version > 0)
                {
                    *this &iLen;
                    iLeft = this->left();
                }
                d.serialize(*this);

                if (_version > 0)
                {
                    int32_t iSkip = iLen - iLeft + this->left();
                    if (iSkip > 0)
                    {
                        _p += iSkip;
                    }
                }
                return *this;
            }

            inline Parser &operator & (CDummy &d)
            {
                return *this;
            }

            inline Parser &operator & (int32_t &d)
            {
                check(sizeof(d));
                memcpy(&d, _p, sizeof(d));
                d = ntohl(d);
                _p += sizeof(d);
                return *this;
            }

            inline Parser &operator & (std::string &s)
            {
                check(sizeof(int32_t));
                int32_t len = 0;
                (*this)&len;
                check(len);
                s.assign(_p, len);
                _p += len;
                return *this;
            }

            // added by jarry

            inline Parser &operator & (uint32_t &d)
            {
                (*this)&(int32_t &)d;
                return *this;
            }

            inline Parser &operator & (uint64_t &d)
            {
                if (!_int64_use_str)
                {
                    check(sizeof(d));
                    memcpy(&d, _p, sizeof(d));
                    d = ntoh64(d);
                    _p += sizeof(d);
                }
                else
                {
                    check(hhp::serialize::UINT64_HEX_BUF_LEN);

                    char szllBuffer[hhp::serialize::UINT64_HEX_BUF_LEN + 1] = {0};
                    memcpy(szllBuffer, _p, hhp::serialize::UINT64_HEX_BUF_LEN);
                    _p += hhp::serialize::UINT64_HEX_BUF_LEN;

#if defined (__LP64__) || defined (__64BIT__) || defined (_LP64) || (__WORDSIZE == 64)
    #if defined(__APPLE__) && defined(__GNUC__)
                    sscanf(szllBuffer, "%016llx", &d);
    #else
                    sscanf(szllBuffer, "%016lx", &d);
    #endif
#else
                    sscanf(szllBuffer, "%016llx", &d);
#endif
                }
                return *this;
            }

            inline Parser &operator & (int64_t &d)
            {
                (*this)&(uint64_t &)d;
                return *this;
            }

            inline Parser &operator & (int16_t &d)
            {
                check(sizeof(d));
                memcpy(&d, _p, sizeof(d));
                d = ntohs(d);
                _p += sizeof(d);
                return *this;
            }

            inline Parser &operator & (uint16_t &d)
            {
                check(sizeof(d));
                memcpy(&d, _p, sizeof(d));
                d = ntohs(d);
                _p += sizeof(d);
                return *this;
            }

            inline Parser &operator & (int8_t &d)
            {
                check(sizeof(d));
                memcpy(&d, _p, sizeof(d));
                _p += sizeof(d);
                return *this;
            }

            inline Parser &operator & (uint8_t &d)
            {
                check(sizeof(d));
                memcpy(&d, _p, sizeof(d));
                _p += sizeof(d);
                return *this;
            }

            template<typename DATA>
            Parser &operator & (std::list<DATA> &ls)
            {
                ls.clear();
                uint32_t sz = 0;
                DATA t;
                (*this)&sz;
                for ( uint32_t i = 0; i < sz; ++i )
                {
                    (*this)&t;
                    ls.push_back(t);
                }
                return *this;
            }

            template<typename LEFT, typename RIGHT>
            Parser &operator & (std::pair<LEFT, RIGHT> &p)
            {
                p.clear();
                (*this) & (p.first);
                (*this) & (p.second);
                return *this;
            }

            template<typename KEY, typename VALUE>
            Parser &operator & (std::map<KEY, VALUE> &mp)
            {
                mp.clear();
                uint32_t sz = 0;
                (*this)&sz;
                for ( uint32_t i = 0; i < sz; ++i )
                {
                    KEY key;
                    VALUE value;
                    (*this)&key;
                    (*this)&value;
                    mp.insert(typename std::map<KEY, VALUE>::value_type(key, value));
                }
                return *this;
            }

            template<typename KEY, typename VALUE>
            Parser &operator & (std::multimap<KEY, VALUE> &mp)
            {
                mp.clear();
                uint32_t sz = 0;
                (*this)&sz;
                for ( uint32_t i = 0; i < sz; ++i )
                {
                    KEY key;
                    VALUE value;
                    (*this)&key;
                    (*this)&value;
                    mp.insert(typename std::multimap<KEY, VALUE>::value_type(key, value));
                }
                return *this;
            }
            template<typename KEY, typename VALUE>
            Parser &operator & (__gnu_cxx::hash_map<KEY, VALUE> &mp)
            {
                mp.clear();
                uint32_t sz = 0;
                (*this)&sz;
                for ( uint32_t i = 0; i < sz; ++i )
                {
                    KEY key;
                    VALUE value;
                    (*this)&key;
                    (*this)&value;
                    mp.insert(typename __gnu_cxx::hash_map<KEY, VALUE>::value_type(key, value));
                }
                return *this;
            }
            //!add
            template<typename DATA>
            Parser &operator & (std::vector<DATA> &vec)
            {
                vec.clear();
                uint32_t sz = 0;
                DATA t;
                (*this)&sz;
                for ( uint32_t i = 0; i < sz; ++i )
                {
                    (*this)&t;
                    vec.push_back(t);
                }
                return *this;
            }

            template<typename DATA>
            Parser &operator >>(DATA &d)
            {
                return (*this)&d;
            }

            Parser(const char *p, int32_t len): _buf(p), _p(p), _len(len), _version(0), _int64_use_str(true)
            {}

            void reset(const char *buf, int32_t len)
            {
                _buf = buf;
                _p = buf;
                _len = len;
            }

            inline int32_t left() const
            {
                return _len - (_p - _buf);
            }

            inline void check(int32_t sz)
            {
                if (this->left() < sz) throw std::string("reach the end of the buffer");
            }

            void setversion(uint32_t ver)
            {
                _version = ver;
            }

            void setint64(bool useStr)
            {
                _int64_use_str = useStr;
            }

        private:
            const char *_buf;
            const char *_p;
            int32_t   _len;
            uint32_t _version;
            bool _int64_use_str;
        };

        /**
         */
        enum eFmtType
        {
            /**
             * JSON格式输出
             */
            FOMAT_JSON = 1,
            /**
             * HDF格式输出
             */
            FOMAT_HDF  = 2,
            FOMAT_TAB  = 3,     //tab分割形式输出
        };

        static inline void ReplaceAll(std::string &sOrg, const char *szFrom, const char *szTo)
        {
            std::size_t position = 0;
            while ( ( position = sOrg.find(szFrom, position)) != std::string::npos )
            {
                sOrg.replace( position, strlen(szFrom), szTo);
                position += strlen(szTo);
            }
        }
        template<typename SS> class CDumper;
        typedef CDumper<std::ostream> COsDumper;

        template<typename SS>
        class CDumper
        {
        public:
            CDumper(SS &ss, const char *pTopName = "", eFmtType eType = FOMAT_JSON,
                    bool isEnableMultiLine = true, bool bNeedName = true ):
                m_ss(ss), m_sName(pTopName), m_eFormatType(eType),
                m_iIndent(0),    //层级关系
                m_bNeedName(bNeedName)
            {
                SetFmt(eType);
                SetMultiLine(isEnableMultiLine);
            }

            template<typename DATA>
            CDumper &operator << (DATA &d)
            {
                return (*this)&d;
            }

            //用于dump name
            CDumper &operator & (CDummy &oName)
            {
                if (m_bNeedName)
                {
                    // JSON not allow attributes name without ""
                    if (m_eFormatType == FOMAT_JSON)
                    {
                        m_sName = "\"";
                        m_sName += oName.GetName();
                        m_sName += "\"";
                    }
                    else
                    {
                        m_sName = oName.GetName();
                    }
                }
                return *this;
            }

            //基本类型的dump
#define DUMP(T)\
    CDumper & operator & (T d)\
    {\
        if(!m_sPostfix.empty()){\
            m_ss<<m_sPostfix;\
            m_sPostfix = "";\
        }\
        Indent();\
        m_ss<<m_sName<<m_sKvSplit<<d;\
        m_sPostfix =  m_sItemSplit; \
        m_sPostfix += m_sLineSplit; \
        return *this;\
    }
            DUMP(uint32_t);
            DUMP(int32_t);
            DUMP(uint64_t);
            DUMP(int64_t);
            DUMP(uint16_t);
            DUMP(int16_t);

            CDumper &operator & (uint8_t d)
            {
                return (*this) & (uint16_t)d;
            }

            CDumper &operator & (int8_t d)
            {
                return (*this) & (int16_t)d;
            }

            CDumper &operator & (std::string &d)
            {
                if (!m_sPostfix.empty())
                {
                    m_ss << m_sPostfix;
                    m_sPostfix = "";
                }
                Indent();
                std::string es(d);
                m_ss << m_sName;
                if (m_eFormatType == FOMAT_JSON)
                {
                    m_ss << m_sKvSplit;
                    ReplaceAll(es, "\"", "\\\"");
                    m_ss << "\"" << es << "\"";
                }
                else
                {
                    if (es.find("\n") != std::string::npos )
                    {
                        m_ss << " << EOM\n";
                        m_ss << es;
                        m_ss << "\nEOM\n";
                    }
                    else     //单行
                    {
                        m_ss << m_sKvSplit;
                        m_ss << es;
                    }
                }
                m_sPostfix =  m_sItemSplit;
                m_sPostfix += m_sLineSplit;
                return *this;
            }

            template<typename DATA>
            CDumper &operator & (DATA &d)
            {
                if ( !m_sPostfix.empty() )
                {
                    m_ss << m_sPostfix;
                    m_sPostfix = "";
                }
                Indent();
                m_ss << m_sName;
                if ( m_eFormatType != FOMAT_HDF )  //hdf符合类型不能加=号，基本类型要加
                {
                    m_ss << m_sKvSplit;
                }
                else
                {
                    m_ss << " ";
                }
                m_ss << m_sObj << m_sLineSplit;

                ++m_iIndent;
                d.serialize(*this);
                --m_iIndent;

                m_ss << m_sLineSplit;

                Indent();
                m_ss << m_sObjEnd;
                m_sPostfix =  m_sItemSplit;
                m_sPostfix += m_sLineSplit;
                return *this;
            }


            template<typename DATA>
            CDumper &operator & (std::vector<DATA> &vec)
            {
                SeriaContainerHead();
                typename std::vector<DATA>::iterator it = vec.begin();
                uint32_t dwPos = 0;
                char szId[16] = {0};
                for ( ; it != vec.end(); ++it, ++dwPos )
                {
                    snprintf(szId, sizeof(szId), "%u", dwPos);
                    CDummy o(szId);
                    (*this) << o << *it;
                }
                SeriaContainerTail();
                return *this;
            }

            template<typename DATA>
            CDumper &operator & (std::list<DATA> &vec)
            {
                SeriaContainerHead();
                typename std::list<DATA>::iterator it = vec.begin();
                uint32_t dwPos = 0;
                char szId[16] = {0};
                for ( ; it != vec.end(); ++it, ++dwPos )
                {
                    snprintf(szId, sizeof(szId), "%u", dwPos);
                    CDummy o(szId);
                    (*this) << o << *it;
                }
                SeriaContainerTail();
                return *this;
            }

            template<typename LEFT, typename RIGHT>
            CDumper &operator & (std::pair<LEFT, RIGHT> &p)
            {
                SeriaContainerHead();
                std::ostringstream ss;
                ss << p.first;
                CDummy o(ss.str().c_str());
                (*this) << o << p.second;
                SeriaContainerTail();
                return *this;
            }

            template<typename KEY, typename DATA>
            CDumper &operator & (std::map<KEY, DATA> &map)
            {
                SeriaContainerHead();
                typename std::map<KEY, DATA>::iterator it = map.begin();
                for ( ; it != map.end(); ++it )
                {
                    std::ostringstream ss;
                    ss << it->first;
                    CDummy o(ss.str().c_str());
                    (*this) << o << it->second;
                }
                SeriaContainerTail();
                return *this;
            }

            template<typename KEY, typename DATA>
            CDumper &operator & (std::multimap<KEY, DATA> &map)
            {
                SeriaContainerHead();
                typename std::multimap<KEY, DATA>::iterator it = map.begin();
                for ( ; it != map.end(); ++it )
                {
                    std::ostringstream ss;
                    ss << it->first;
                    CDummy o(ss.str().c_str());
                    (*this) << o << it->second;
                }
                SeriaContainerTail();
                return *this;
            }

            template<typename KEY, typename DATA>
            CDumper &operator & (__gnu_cxx::hash_map<KEY, DATA> &map)
            {
                SeriaContainerHead();
                typename __gnu_cxx::hash_map<KEY, DATA>::iterator it = map.begin();
                for ( ; it != map.end(); ++it )
                {
                    std::ostringstream ss;
                    ss << it->first;
                    CDummy o(ss.str().c_str());
                    (*this) << o << it->second;
                }
                SeriaContainerTail();
                return *this;
            }
            void Indent()
            {
                if ( *m_sLineSplit.rbegin() != '\n' )return;
                for ( int i = 0; i < m_iIndent; ++i )m_ss << "    ";
            }

            bool SetMultiLine(bool b)
            {
                if (m_eFormatType == FOMAT_HDF && !b)
                {
                    return false;//HDF不允许为一行
                }
                m_sPostfix = "";
                if ( !b )
                {
                    m_sLineSplit = ' ';
                }
                else
                {
                    m_sLineSplit = '\n';
                }
                return true;
            }

            eFmtType SetFmt(eFmtType eNew)
            {
                m_sPostfix = "";
                eFmtType old  = m_eFormatType;
                m_eFormatType = eNew;
                switch ( m_eFormatType )
                {
                case FOMAT_JSON:
                    m_sLineSplit = '\n';
                    m_sItemSplit = ",";
                    m_sKvSplit   = ":";
                    m_sObj       = "{";
                    m_sObjEnd    = "}";
                    m_sArray     = "[";
                    m_sArrayEnd  = "]";
                    break;
                case FOMAT_HDF:
                    m_sLineSplit = '\n';
                    m_sItemSplit = "";
                    m_sKvSplit   = "=";
                    m_sObj       = "{";
                    m_sObjEnd    = "}";
                    m_sArray     = "{";
                    m_sArrayEnd  = "}";
                    break;
                case FOMAT_TAB:
                    m_sLineSplit = "";
                    m_sItemSplit = "\t";
                    m_sKvSplit   = "";
                    m_sObj       = "";
                    m_sObjEnd    = "";
                    m_sArray     = "";
                    m_sArrayEnd  = "";
                    break;
                default:
                    m_sLineSplit = '\n';
                    m_sItemSplit = ",";
                    m_sKvSplit   = ":";
                    m_sObj       = "{";
                    m_sObjEnd    = "}";
                    m_sArray     = "[";
                    m_sArrayEnd  = "]";
                    break;
                }
                return old;
            }
        private:
            CDumper &SeriaContainerHead()
            {
                if ( !m_sPostfix.empty() )
                {
                    m_ss << m_sPostfix;
                    m_sPostfix = "";
                }
                Indent();
                m_ss << m_sName;

                if ( m_eFormatType != FOMAT_HDF )  //hdf符合类型不能加=号，基本类型要加
                {
                    m_ss << m_sKvSplit;
                }
                else
                {
                    m_ss << " ";
                }

                m_ss << m_sArray << m_sLineSplit;

                ++m_iIndent;
                return *this;
            }

            CDumper &SeriaContainerTail()
            {
                --m_iIndent;
                m_ss << m_sLineSplit;
                Indent();
                m_ss << m_sArrayEnd;
                m_sPostfix =  m_sItemSplit;
                m_sPostfix += m_sLineSplit;
                return *this;
            }



        private:
            SS         &m_ss;
            std::string m_sName;
            std::string m_sPostfix;
            eFmtType    m_eFormatType;
            int         m_iIndent;    //层级关系
            std::string     m_sLineSplit;
            std::string     m_sItemSplit;
            std::string     m_sKvSplit;
            std::string     m_sObj;
            std::string     m_sObjEnd;
            std::string     m_sArray;
            std::string     m_sArrayEnd;
            bool            m_bNeedName;    //是否需要显示名字
        };

        typedef CDumper<std::ostream> COsDumper;


        /**
         * dump对象成为str，支持嵌套和vector,list,map等常见结构
         * map的key必须是基本类型（int，string）等，目前满足需要，
         * 后面有需要再改进
         *
         * @example:
         *  cout<<CSDumper().c_str(obj,"obj",FOMAT_JSON,true)<<endl;
         *  cout<<CSDumper().c_str(obj,"obj",FOMAT_HDF,true)<<endl;
         *
         * @author codydeng
         * @version 0.2
         * @since 2010.12.17
         */

        class CSDumper
        {
        public:
            /**
             *
             * @return
             * @example cout<<CSDumper().c_str(obj,"obj",FOMAT_JSON,true)<<endl;
             */
            template<typename T>
            const char *c_str(T &t, const char *pTopName = "", eFmtType eType = FOMAT_JSON,
                              bool isEnableMultiLine = false, bool bNeedName = true)
            {
                std::ostringstream ss;
                COsDumper o(ss, pTopName, eType, isEnableMultiLine, bNeedName);
                o << t;
                m_sDump = ss.str();
                return m_sDump.c_str();
            }
        private:
            std::string m_sDump;
        };

        class CToLine
        {
        public:
            template<typename DATA>
            CToLine &operator &(DATA &d)
            {
                d.serialize(*this);
                return *this;
            }

            CToLine &operator &(CDummy &d)
            {
                return *this;
            }

            //基本类型的dump
#define DUMP_TOLINE(T)\
    inline CToLine & operator & (T d) {\
        if (!_ssLine.str().empty())_ssLine<<_cDelim;\
        _ssLine<<d;\
        return *this;\
    }\
     
            DUMP_TOLINE(uint32_t);
            DUMP_TOLINE(int32_t);
            DUMP_TOLINE(uint64_t);
            DUMP_TOLINE(int64_t);
            DUMP_TOLINE(uint16_t);
            DUMP_TOLINE(int16_t);
            DUMP_TOLINE(uint8_t);
            DUMP_TOLINE(int8_t);
            //DUMP_TOLINE(std::string);

            // 对string另处理
            inline CToLine &operator & (std::string d)
            {
                if (!_ssLine.str().empty())
                {
                    _ssLine << _cDelim;
                }
                for (uint32_t i = 0; i < d.length(); ++i)
                {
                    if (d[i] == _cDelim || d[i] == '\n' || d[i] == '\r') // 清掉分隔符[XXX]
                    {
                        d[i] = ' ';
                    }
                }
                _ssLine << d;
                return *this;
            }

            template<typename DATA>
            CToLine &operator & (std::vector<DATA> &d)
            {
                if (!_ssLine.str().empty())
                {
                    _ssLine << _cDelim;
                }

                _ssLine << d.size();

                for (size_t i = 0; i < d.size(); ++i)
                {
                    _ssLine << _cDelim;
                    (*this) & d[i];
                }

                return *this;
            }

            template<typename DATA>
            CToLine &operator <<(DATA &d)
            {
                return (*this)&d;
            }

            CToLine(unsigned char c = 0x09): _cDelim(c)
            {
            }

            inline const std::string getline()
            {
                return _ssLine.str();
            }


            void clear()
            {
                _ssLine.clear();
            }

        private:
            unsigned char      _cDelim;
            std::stringstream  _ssLine;
        };


        class CFromLine
        {
        public:
            template<typename DATA>
            CFromLine &operator &(DATA &d)
            {
                d.serialize(*this);
                return *this;
            }

            CFromLine &operator &(CDummy &d)
            {
                return *this;
            }

            //基本类型的dump
#define FROM_LINE_TO_STRUCT(T)\
    inline CFromLine & operator & (T & d) {\
        shift();\
        char * pNext = NULL;\
        if(*_pCur == _cDelim){\
            d = 0;      \
            return *this;\
        }\
        d = (T)strtoull(_pCur,&pNext,0);\
        _pCur = pNext;\
        return *this;\
    }\
     
            FROM_LINE_TO_STRUCT(uint32_t);
            FROM_LINE_TO_STRUCT(int32_t);
            FROM_LINE_TO_STRUCT(uint64_t);
            FROM_LINE_TO_STRUCT(int64_t);
            FROM_LINE_TO_STRUCT(uint16_t);
            FROM_LINE_TO_STRUCT(int16_t);

            inline CFromLine &operator & (uint8_t &d)
            {
                shift();
                d = *_pCur;
                //printf("==%c,%d %p=%p==\n",*_pCur,*_pCur,_pCur,_pLine);
                ++_pCur;
                return *this;
            }

            inline CFromLine &operator & (int8_t &d)
            {
                return *this & d;
            }

            inline CFromLine &operator & (std::string &d)
            {
                shift();

                while (_pCur < _pLine + _sLine.length() && *_pCur != _cDelim)
                {
                    d += *_pCur;
                    ++_pCur;
                }

                return *this;
            }

            template<typename DATA> CFromLine &operator & (std::vector<DATA> &d)
            {
                shift();
                char *pNext = NULL;
                if (*_pCur == _cDelim)
                {
                    return *this;
                }
                int len = strtoul(_pCur, &pNext, 0);
                _pCur = pNext;

                for (int i = 0; i < len; ++i)
                {
                    shift();
                    DATA tmp;
                    (*this) & tmp;
                    d.push_back(tmp);
                }

                return *this;
            }

            inline void shift()
            {
                if (_pCur >= _pLine + _sLine.length())
                {
                    throw std::string("nothing to be parsed");
                }

                if (!_bFirst )
                {
                    if (*_pCur != _cDelim)
                    {
                        throw std::string("should be a delimit here:") + _pCur;
                    }
                    ++_pCur;
                }
                _bFirst = false;
            }

            CFromLine(const std::string &sLine, unsigned char c = 0x09)
                : _sLine(sLine), _pLine(NULL), _pCur(NULL), _cDelim(c), _bFirst(true)
            {
                _pCur = _pLine = sLine.c_str();
            }

            template<typename DATA>
            CFromLine &operator >>(DATA &d)
            {
                return (*this)&d;
            }

        private:
            const std::string   _sLine;
            const char         *_pLine;
            const char         *_pCur;
            unsigned char       _cDelim;
            bool                _bFirst;
        };

    };
};

#endif
