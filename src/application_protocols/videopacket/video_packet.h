#pragma once

#include "src/application_protocols/videopacket/VideoCommHead.h"
#include <stdint.h>
#include <stdlib.h>



enum {
	VIDEO_STX = 0x26,
	VIDEO_ETX = 0x28
	
}; 

class CVideoPacket
{
public:
	CVideoPacket();
	virtual ~CVideoPacket();
public:
	virtual int decode(void);
	virtual int encode(void);
	virtual int set_packet(uint8_t *pData, uint32_t wDataLen);

	/*
		@biref:�԰��ĺϷ��Խ��м��
		@return:
		<0, �����������Ϸ�
		=0, ����û�н�������,����0��spp_proxy������������
		>0, ���ذ���ĳ���
		@process:
		1���ȼ����Ƿ��������
		2��Ȼ������Ƿ�Ϸ�
	*/
	static inline int checkPacket(const char *pBuf, uint32_t dwReciveLen)
	{	
		if(dwReciveLen < 3)
			return 0;
		
		uint32_t dwRealPacketLen = getPacketLenFromBuf(pBuf);
		if(dwRealPacketLen > dwReciveLen)
			return 0;
		
		if(dwRealPacketLen < MIN_PACKET_LEN || pBuf[0] != VIDEO_STX || pBuf[dwRealPacketLen - 1] != VIDEO_ETX)
			return -1;
	
		return dwRealPacketLen;
	}
	
public:
	
	/*
		@biref:��ͷ��Ա�Ķ�д����
	*/
	inline uint8_t getSTX(void) const { return m_cStx; }
 	inline uint8_t getETX(void) const { return m_cEtx; }
 	inline uint32_t getLength(void) const { return m_dwPacketLen; }
	inline uint8_t getHeadVersion(void) const {return m_cVersion;}
	inline uint8_t getRedoId(void) const {return m_acReserved[0];}
	inline uint8_t getCtrlId(void) const {return m_acReserved[3];}
	inline void setRedoId(uint8_t cRedoId) {m_acReserved[0] = cRedoId;}
	inline void setCtrlId(uint8_t cCtrlId) {m_acReserved[3] = cCtrlId;}
	inline string &getBody() { return m_stVideoCommHeader.body; }
	inline const string & getBody () const {return m_stVideoCommHeader.body;}
	inline void setBody(const string &strBody) { m_stVideoCommHeader.body = strBody; }
	inline void setBody(const char *pBuf, int iLength) 
	{ 
		if(iLength > 2*1024*1024)
		{
			m_stVideoCommHeader.body = "";		
		}
		else
		{
			m_stVideoCommHeader.body.assign(pBuf, iLength); 
		}
	}
	inline const videocomm::VideoCommHeader& getVideoCommHeader() const {return m_stVideoCommHeader;}
	inline videocomm::VideoCommHeader& getVideoCommHeader() {return m_stVideoCommHeader;}
	inline void setVideoCommHeader(const videocomm::VideoCommHeader &stVideoCommHeader)
	{
		m_stVideoCommHeader = stVideoCommHeader;
	}
	
	/*
	*	��ȡ���޸�VideoCommHeader��ϵ�к�����Ҳ����ͨ��getVideoCommHead�������޸�
	*/
	
	/*
		@biref:HBasicInfo��Ա�Ķ�д����
	*/
	inline taf::Int64 getReqUin() const {return m_stVideoCommHeader.BasicInfo.ReqUin;}
	inline unsigned short getCommand() const
	{
		return m_stVideoCommHeader.BasicInfo.Command;
	}
	
	static unsigned short getCommand(uint8_t *pData, uint32_t wDataLen)
	{
		//��Ϊ֮ǰ�н��а������Լ�飬����ͼ򵥵ļ����ָ������ĺϷ��Լ���
		//���ȷ��ع̶��ֽڵط���������

		if(pData == NULL || wDataLen < MIN_PACKET_LEN)
			return 0;
		
		unsigned short *pwCommand = reinterpret_cast<unsigned short *>(&(pData[7]));

		return ntohs(*pwCommand);
	}
	
	inline unsigned char getServiceType() const {return m_stVideoCommHeader.BasicInfo.ServiceType;}
	inline unsigned char getVersion() const {return m_stVideoCommHeader.BasicInfo.version;}
	inline unsigned int getResult() const {return m_stVideoCommHeader.BasicInfo.Result;}
	inline unsigned int getCallerID() const {return m_stVideoCommHeader.BasicInfo.CallerID;}
	inline taf::Int64 getSeqId() const {return m_stVideoCommHeader.BasicInfo.SeqId;}
	inline int getSubCmd() const {return m_stVideoCommHeader.BasicInfo.SubCmd;}
	inline int getBodyFlag() const {return m_stVideoCommHeader.BasicInfo.BodyFlag;}
	
	/*****************SET OPERATOR*********************/
	inline void setReqUin(taf::Int64 ddwUin) {m_stVideoCommHeader.BasicInfo.ReqUin = ddwUin;}
	inline void setCommand(unsigned short wCommand)
	{
		unsigned short *pwCommand = reinterpret_cast<unsigned short *>(&(m_acReserved[1]));
		m_stVideoCommHeader.BasicInfo.Command = wCommand;
		*pwCommand = htons(wCommand);
	}
	inline void setServiceType(unsigned char cServiceType){m_stVideoCommHeader.BasicInfo.ServiceType = cServiceType;}
	inline void setVersion(uint8_t cVersion){m_stVideoCommHeader.BasicInfo.version = cVersion;}
	inline void setResult(unsigned int iResult){m_stVideoCommHeader.BasicInfo.Result = iResult;}
	inline void setCallerID(unsigned int iCallerID){m_stVideoCommHeader.BasicInfo.CallerID= iCallerID;}
	inline void setSeqId(taf::Int64 ddwSeqId){m_stVideoCommHeader.BasicInfo.SeqId = ddwSeqId;}
	inline void setSubCmd(int iSubCmd) {m_stVideoCommHeader.BasicInfo.SubCmd = iSubCmd;}
	inline void setBodyFlag(int iBodyFlag) {m_stVideoCommHeader.BasicInfo.BodyFlag = iBodyFlag;}
	/*
		@biref:HAccessInfo��Ա�Ķ�д����
	*/
	inline unsigned int getProxyIP() const {return m_stVideoCommHeader.AccessInfo.ProxyIP;}
	inline unsigned int getServerIP() const {return m_stVideoCommHeader.AccessInfo.ServerIP;}
	inline taf::Int64 getClientIP() const {return m_stVideoCommHeader.AccessInfo.ClientIP;}
	inline unsigned short getClientPort() const {return m_stVideoCommHeader.AccessInfo.ClientPort;}
	inline unsigned int getServiceTime() const {return m_stVideoCommHeader.AccessInfo.ServiceTime;}
	inline const string &getServiceName() const {return m_stVideoCommHeader.AccessInfo.ServiceName;}
	inline const string &getRtxName() const {return m_stVideoCommHeader.AccessInfo.RtxName;}
	inline const string &getFileName()const {return m_stVideoCommHeader.AccessInfo.FileName;}
	inline const string &getFuncName() const {return m_stVideoCommHeader.AccessInfo.FuncName;}
	inline unsigned int getLine() const {return m_stVideoCommHeader.AccessInfo.Line;}
	inline const string &getCgiProcId() const {return m_stVideoCommHeader.AccessInfo.CgiProcId;}
	inline const string &getFromInfo() const {return m_stVideoCommHeader.AccessInfo.FromInfo;}
	inline uint64_t getAccIP() const {return m_stVideoCommHeader.AccessInfo.AccIP;}
	inline uint32_t getAccPort() const {return m_stVideoCommHeader.AccessInfo.AccPort;} 
	inline uint64_t getAccId() const {return m_stVideoCommHeader.AccessInfo.AccId;}
	inline uint64_t getClientID() const {return m_stVideoCommHeader.AccessInfo.ClientID;}
	inline const videocomm::HQua &getQUAInfo()const {return m_stVideoCommHeader.AccessInfo.QUAInfo;}
	inline const string &getGuid() const {return m_stVideoCommHeader.AccessInfo.Guid;}
	inline const videocomm::LogReport &getLogReport()const {return m_stVideoCommHeader.AccessInfo.BossReport;}
	inline int getColorFlag()const {return m_stVideoCommHeader.AccessInfo.Flag;}
	inline int getAccSeq()const {return m_stVideoCommHeader.AccessInfo.Seq;}
	inline const vector<videocomm::ExtentAccount> &getExtentAccountList()const {return m_stVideoCommHeader.AccessInfo.extentAccountList;}
    inline const videocomm::HttpData &getHttpBody()const{return m_stVideoCommHeader.HttpBody;}

	/*****************SET OPERATOR*********************/
	
    inline void setHttpBody(const videocomm::HttpData &stHttpBody){m_stVideoCommHeader.HttpBody = stHttpBody;}
	inline void setProxyIP(unsigned int iProxyIP) {m_stVideoCommHeader.AccessInfo.ProxyIP = iProxyIP;}
	inline void setServerIP(unsigned int iServerIP) {m_stVideoCommHeader.AccessInfo.ServerIP = iServerIP;}
	inline void setClientIP(unsigned int iClientIP) {m_stVideoCommHeader.AccessInfo.ClientIP = iClientIP;}
	inline void setClientPort(unsigned short wClientPort) {m_stVideoCommHeader.AccessInfo.ClientPort = wClientPort;}
	inline void setServiceTime(unsigned int iServiceTime) {m_stVideoCommHeader.AccessInfo.ServiceTime = iServiceTime;}
	inline void setServiceName(const string &strServiceName) {m_stVideoCommHeader.AccessInfo.ServiceName = strServiceName;}
	inline void setRtxName(const string &strRtxName) {m_stVideoCommHeader.AccessInfo.RtxName = strRtxName;}
	inline void setFileName(const string &strFileName) {m_stVideoCommHeader.AccessInfo.FileName = strFileName;}
	inline void setFuncName(const string &strFuncName) {m_stVideoCommHeader.AccessInfo.FuncName = strFuncName;}
	inline void setLine(unsigned int iLine) {m_stVideoCommHeader.AccessInfo.Line = iLine;}
	inline void setCgiProcId(const string &strCgiProcId) {m_stVideoCommHeader.AccessInfo.CgiProcId = strCgiProcId;}
	inline void setFromInfo(const string &strFromInfo) {m_stVideoCommHeader.AccessInfo.FromInfo = strFromInfo;}
	inline void setAccIP(uint64_t ddwAccIP) {m_stVideoCommHeader.AccessInfo.AccIP = ddwAccIP;}
	inline void setAccPort(uint32_t dwAccPort) {m_stVideoCommHeader.AccessInfo.AccPort = dwAccPort;}
	inline void setAccId(uint64_t ddwAccId) {m_stVideoCommHeader.AccessInfo.AccId = ddwAccId;}
	inline void setClientID(uint64_t ddwClientID) {m_stVideoCommHeader.AccessInfo.ClientID = ddwClientID;}
	inline void setQUAInfo(const videocomm::HQua &stHQuaInfo){	m_stVideoCommHeader.AccessInfo.QUAInfo = stHQuaInfo;}
	inline void setGuid(const string &strGuid){m_stVideoCommHeader.AccessInfo.Guid = strGuid; }
	inline void seLogReport(const videocomm::LogReport &stLogReport){m_stVideoCommHeader.AccessInfo.BossReport = stLogReport; }
	inline void setColorFlag(int iColorFlag) {m_stVideoCommHeader.AccessInfo.Flag = iColorFlag;}
	inline void setAccSeq(int iAccSeq) {m_stVideoCommHeader.AccessInfo.Seq = iAccSeq;}
	inline void setExtentAccount(const videocomm::ExtentAccount &stExtentAccount)
	{
		m_stVideoCommHeader.AccessInfo.extentAccountList.push_back(stExtentAccount);
	}
	inline void setExtentAccount(const vector<videocomm::ExtentAccount> &vecExtentAccountList)
	{
		m_stVideoCommHeader.AccessInfo.extentAccountList.assign(vecExtentAccountList.begin(), vecExtentAccountList.end());
	}
	/*
		@biref:HLoginToken��Ա�Ķ�д����
	*/
	inline taf::Int64 getLoginUserid()
	{
		for(uint32_t i=0; i<m_stVideoCommHeader.LoginTokens.size(); i++)
		{
			videocomm::HLoginToken &stToken = m_stVideoCommHeader.LoginTokens[i];
			if(stToken.TokenKeyType == 9)
				return stToken.TokenUin;
		}

		return 0;
	}
	inline vector<videocomm::HLoginToken> &getLoginToken()  {return m_stVideoCommHeader.LoginTokens;}
	inline void setLoginToken(const videocomm::HLoginToken & stLoginToken) 
	{
		m_stVideoCommHeader.LoginTokens.push_back(stLoginToken);
	}
	inline void setLoginToken(const vector<videocomm::HLoginToken> &vecLoginToken)
	{
		m_stVideoCommHeader.LoginTokens.assign(vecLoginToken.begin(), vecLoginToken.end());
	}

	inline vector<videocomm::HAccCmdBody> &getAccCmdBody() {return m_stVideoCommHeader.AccCmdBody;}
	inline void setAccCmdBody(const videocomm::HAccCmdBody &stAccCmdBody)
	{
		m_stVideoCommHeader.AccCmdBody.push_back(stAccCmdBody);
	}
	inline void setAccCmdBody(const vector<videocomm::HAccCmdBody> &vecAccCmdBody)
	{
		m_stVideoCommHeader.AccCmdBody.assign(vecAccCmdBody.begin(), vecAccCmdBody.end());
	}

	const uint8_t* getPacket() const {return this->packet;}
	uint32_t getPacketLen() const {return this->packet_len;}
private:
	static inline uint32_t getPacketLenFromBuf(const char *pBuf)
	{
		return ntohl(*reinterpret_cast<uint32_t *>(const_cast<char*>(&pBuf[1])));
	}
	virtual int is_valid_packet() const
	{
	    if(this->m_cStx != VIDEO_STX || this->m_cEtx != VIDEO_ETX || this->m_dwPacketLen < MIN_PACKET_LEN)
	    {
	        return 0;
	    }
		
    	return 1;
	}

	
	
	inline void setPacketHeadLen()
    {
        this->m_dwPacketLen = MIN_PACKET_LEN + m_wVideoCommHeaderLen;
    }


	int allocBuf(uint32_t dwSize) 
	{
		if(packet != NULL)
		{
			free(packet);
			packet = NULL;
		}
		if(dwSize < 256)
		{
			dwSize = 256;
		}
		else if(dwSize < 512)
		{
			dwSize = 512;
		}
		else if(dwSize < 1024)
		{
			dwSize = 1024;
		}
		else if(dwSize < 4 *1024)
		{
			dwSize = 4*1024;
		}
		else if (dwSize < 32 *1024)
		{
			dwSize = 32*1024;
		}
		else if (dwSize < 64 *1024)
		{
			dwSize = 64*1024;
		}
		else if (dwSize < 128 *1024)
		{
			dwSize = 128*1024;
		}
		else if (dwSize < 256 *1024)
		{
			dwSize = 256*1024;
		}
		else if (dwSize < 512 *1024)
		{
			dwSize = 512*1024;
		}
		else
		{
			// dwSize = dwSize;
		}
		
		packet = reinterpret_cast<uint8_t *>(malloc(dwSize));
		if(packet == NULL)
		{
			return -1;
		}

		return 0;

	}
	
	int delBuf() 
	{
			if(packet != NULL)
			{
				free(packet);
				packet = NULL;
			}
			
			return 0;
	
	}
private:
	static const uint16_t MIN_PACKET_LEN;
	uint32_t m_wVideoCommHeaderLen; //�洢m_stVideoCommHeader�ĳ��ȣ����ǲ�����CVideoPacket
	
	/*
		��������ṹ���壬��ȥm_stVideoCommHeader�ĳ���Ϊ17�ֽ�
		@brief:
		1��m_acReserved�ĵ�һ���ֽ�������������id��redoid=m_acReserved[0]
		redoid=0,�����������
		redoid>0�����԰�,redoidΪ���Դ���
		2��m_acReserved�ĵڶ��������ֽ���������ҵ�������֣����㲻�ý�jce���Ϳ���ȡ��������
		3��m_acReserved�ĵ����ֽ��������������id��ctrlid=m_acReserved[3],��λȡֵ
		ctrlid=0,�����������
		ctrlid=0x01�����԰�
			
	*/
	unsigned char m_cStx;
	unsigned int m_dwPacketLen;
	unsigned char m_cVersion;
	char m_acReserved[10];
	videocomm::VideoCommHeader m_stVideoCommHeader;
	char m_cEtx;

	uint32_t packet_len; 
	uint8_t *packet;
};

