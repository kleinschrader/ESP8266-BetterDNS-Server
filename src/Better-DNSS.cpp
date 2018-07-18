#include "Better-DNSS.h"
#include <lwip/def.h>
#include <Arduino.h>


DNSServer::DNSServer()
{
  _ttl = htonl(60);
  _errorReplyCode = DNSReplyCode::NonExistentDomain;
}

bool DNSServer::start(uint16_t port)
{
  _port = port;
  _buffer = NULL;
  return _udp.begin(_port) == 1;
}

void DNSServer::addDomain(String domain, IPAddress dest)
{
		_domainNames.push_back(domain);
		_resolvedIPs.push_back(dest);
}

void DNSServer::setErrorReplyCode(const DNSReplyCode &replyCode)
{
  _errorReplyCode = replyCode;
}

void DNSServer::setTTL(const uint32_t &ttl)
{
  _ttl = htonl(ttl);
}

void DNSServer::stop()
{
  _udp.stop();
  free(_buffer);
  _buffer = NULL;
}

void DNSServer::downcaseAndRemoveWwwPrefix(String &domainName)
{
  domainName.toLowerCase();
  domainName.replace("www.", "");
}

void DNSServer::processNextRequest()
{
  _currentPacketSize = _udp.parsePacket();
  if (_currentPacketSize)
  {
    if (_buffer != NULL) free(_buffer);
    _buffer = (unsigned char*)malloc(_currentPacketSize * sizeof(char));
    if (_buffer == NULL) return;
    _udp.read(_buffer, _currentPacketSize);
    _dnsHeader = (DNSHeader*) _buffer;

	bool didSend = false;
	
    if (_dnsHeader->QR == DNS_QR_QUERY && _dnsHeader->OPCode == DNS_OPCODE_QUERY && requestIncludesOnlyOneQuestion())
    {
		
		//(_domainName == "*" || getDomainNameWithoutWwwPrefix() == _domainName)
		int index = 0;
		for(; index < _domainNames.size(); index++)
		{
			if(_domainNames[index] == getDomainNameWithoutWwwPrefix() || _domainNames[index] == "*")
			{
				replyWithIP(_resolvedIPs[index]);
				didSend = true;
				break;
			}
		}
    }
    else if (_dnsHeader->QR == DNS_QR_QUERY || !didSend)
    {
      replyWithCustomCode();
    }

    free(_buffer);
    _buffer = NULL;
  }
}

bool DNSServer::requestIncludesOnlyOneQuestion()
{
  return ntohs(_dnsHeader->QDCount) == 1 &&
         _dnsHeader->ANCount == 0 &&
         _dnsHeader->NSCount == 0 &&
         _dnsHeader->ARCount == 0;
}

String DNSServer::getDomainNameWithoutWwwPrefix()
{
  String parsedDomainName = "";
  if (_buffer == NULL) return parsedDomainName;
  unsigned char *start = _buffer + 12;
  if (*start == 0)
  {
    return parsedDomainName;
  }
  int pos = 0;
  while(true)
  {
    unsigned char labelLength = *(start + pos);
    for(int i = 0; i < labelLength; i++)
    {
      pos++;
      parsedDomainName += (char)*(start + pos);
    }
    pos++;
    if (*(start + pos) == 0)
    {
      downcaseAndRemoveWwwPrefix(parsedDomainName);
      return parsedDomainName;
    }
    else
    {
      parsedDomainName += ".";
    }
  }
}

void DNSServer::replyWithIP(IPAddress dest)
{
  if (_buffer == NULL) return;
  _dnsHeader->QR = DNS_QR_RESPONSE;
  _dnsHeader->ANCount = _dnsHeader->QDCount;
  _dnsHeader->QDCount = _dnsHeader->QDCount; 
  //_dnsHeader->RA = 1;  

  _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
  _udp.write(_buffer, _currentPacketSize);

  _udp.write((uint8_t)192); //  answer name is a pointer
  _udp.write((uint8_t)12);  // pointer to offset at 0x00c

  _udp.write((uint8_t)0);   // 0x0001  answer is type A query (host address)
  _udp.write((uint8_t)1);

  _udp.write((uint8_t)0);   //0x0001 answer is class IN (internet address)
  _udp.write((uint8_t)1);
 
  _udp.write((unsigned char*)&_ttl, 4);

  // Length of RData is 4 bytes (because, in this case, RData is IPv4)
  _udp.write((uint8_t)0);
  _udp.write((uint8_t)4);
  _udp.write((uint8_t)dest[0]);
  _udp.write((uint8_t)dest[1]);
  _udp.write((uint8_t)dest[2]);
  _udp.write((uint8_t)dest[3]);
  _udp.endPacket();



  #ifdef DEBUG_ESP_DNS
    DEBUG_ESP_PORT.printf("DNS responds: %s for %s\n",
            IPAddress(_resolvedIP).toString().c_str(), getDomainNameWithoutWwwPrefix().c_str() );
  #endif
}

void DNSServer::replyWithCustomCode()
{
  if (_buffer == NULL) return;
  _dnsHeader->QR = DNS_QR_RESPONSE;
  _dnsHeader->RCode = (unsigned char)_errorReplyCode;
  _dnsHeader->QDCount = 0;

  _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
  _udp.write(_buffer, sizeof(DNSHeader));
  _udp.endPacket();
}