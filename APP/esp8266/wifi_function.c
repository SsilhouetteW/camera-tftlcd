#include "wifi_function.h"
#include "wifi_config.h"
#include "SysTick.h"
#include <string.h> 
#include <stdio.h>  
#include <stdbool.h>
#include <stdint.h>

#define Single_ID 0 

void ESP8266_Choose ( FunctionalState enumChoose )
{
	if ( enumChoose == ENABLE )
		ESP8266_CH_HIGH_LEVEL();
	else
		ESP8266_CH_LOW_LEVEL();
}

void ESP8266_Rst ( void )
{
	ESP8266_RST_LOW_LEVEL();
	delay_ms ( 500 ); 
	ESP8266_RST_HIGH_LEVEL();
}

void ESP8266_AT_Test ( void )
{
	ESP8266_RST_HIGH_LEVEL();
	delay_ms ( 1000 ); 
	while ( ! ESP8266_Cmd ( "AT", "OK", NULL, 200 ) ) ESP8266_Rst ();  	
}

bool ESP8266_Cmd ( char * cmd, char * reply1, char * reply2, u32 waittime )
{    
	strEsp8266_Fram_Record .InfBit .FramLength = 0;
	ESP8266_Usart ( "%s\r\n", cmd );
	if ( ( reply1 == 0 ) && ( reply2 == 0 ) )
		return true;
	
	delay_ms ( waittime );
	strEsp8266_Fram_Record .Data_RX_BUF [ strEsp8266_Fram_Record .InfBit .FramLength ] = '\0';
	PC_Usart ( "%s", strEsp8266_Fram_Record .Data_RX_BUF );
  
	if ( ( reply1 != 0 ) && ( reply2 != 0 ) )
		return ( ( bool ) strstr ( strEsp8266_Fram_Record .Data_RX_BUF, reply1 ) || 
						 ( bool ) strstr ( strEsp8266_Fram_Record .Data_RX_BUF, reply2 ) ); 
	
	else if ( reply1 != 0 )
		return ( ( bool ) strstr ( strEsp8266_Fram_Record .Data_RX_BUF, reply1 ) );
	else
		return ( ( bool ) strstr ( strEsp8266_Fram_Record .Data_RX_BUF, reply2 ) );
}

bool ESP8266_Net_Mode_Choose ( ENUM_Net_ModeTypeDef enumMode )
{
	switch ( enumMode )
	{
		case STA:
			return ESP8266_Cmd ( "AT+CWMODE=1", "OK", "no change", 2500 ); 
		case AP:
			return ESP8266_Cmd ( "AT+CWMODE=2", "OK", "no change", 2500 ); 
		case STA_AP:
			return ESP8266_Cmd ( "AT+CWMODE=3", "OK", "no change", 2500 ); 
		default:
			return false;
  }
}

bool ESP8266_JoinAP ( char * pSSID, char * pPassWord )
{
	char cCmd [120];
	sprintf ( cCmd, "AT+CWJAP=\"%s\",\"%s\"", pSSID, pPassWord );
	return ESP8266_Cmd ( cCmd, "OK", NULL, 7000 );
}

bool ESP8266_BuildAP ( char * pSSID, char * pPassWord, char * enunPsdMode )
{
	char cCmd [120];
	sprintf ( cCmd, "AT+CWSAP=\"%s\",\"%s\",1,%s", pSSID, pPassWord, enunPsdMode );
	return ESP8266_Cmd ( cCmd, "OK", 0, 1000 );
}

bool ESP8266_Enable_MultipleId ( FunctionalState enumEnUnvarnishTx )
{
	char cStr [20];
	sprintf ( cStr, "AT+CIPMUX=%d", ( enumEnUnvarnishTx ? 1 : 0 ) );
	return ESP8266_Cmd ( cStr, "OK", 0, 500 );
}

bool ESP8266_Link_Server ( ENUM_NetPro_TypeDef enumE, char * ip, char * ComNum, ENUM_ID_NO_TypeDef id)
{
	char cStr [100] = { 0 }, cCmd [120];
  switch ( enumE )
  {
		case enumTCP:
			sprintf ( cStr, "\"%s\",\"%s\",%s", "TCP", ip, ComNum );
			break;
		case enumUDP:
			sprintf ( cStr, "\"%s\",\"%s\",%s", "UDP", ip, ComNum );
			break;
		default:
			break;
  }

  if ( id < 5 )
    sprintf ( cCmd, "AT+CIPSTART=%d,%s", id, cStr);
  else
	  sprintf ( cCmd, "AT+CIPSTART=%s", cStr );

	return ESP8266_Cmd ( cCmd, "OK", "ALREAY CONNECT", 500 );
}

bool ESP8266_Link_MQTT ( char * ip, char * ComNum, ENUM_ID_NO_TypeDef id)
{
	char cStr [100] = { 0 }, cCmd [120];
	
	PC_Usart("      Using simplified MQTT connect command...\r\n");
	
	sprintf ( cStr, "\"%s\",%s", ip, ComNum );
    sprintf ( cCmd, "AT+MQTTCONN=0,%s,1", cStr );
    
    PC_Usart("      Sending: %s\r\n", cCmd);
    PC_Usart("      Waiting up to 10 seconds for MQTT connection...\r\n");
    
    // 增加等待时间到 10 秒，因为 MQTT 连接需要较长时间
	return ESP8266_Cmd ( cCmd, "OK", "ALREAY CONNECT", 10000 );
}

bool ESP8266_Set_MQTT_User (void)
{
	char cCmd [120];
	
	PC_Usart("      Disconnecting existing MQTT connection...\r\n");
	ESP8266_Cmd("AT+MQTTDISCONN=0", "OK", NULL, 2000);
	
	sprintf ( cCmd, "AT+MQTTUSERCFG=0,1,\"ESP8266_001\",\"\",\"\",0,0,\"\"" );
	PC_Usart("      Sending: %s\r\n", cCmd);
	return ESP8266_Cmd ( cCmd, "OK", "ALREAY CONNECT", 2000 );
}

bool ESP8266_Set_MQTT_Public (char * topicId, char * val)
{
	char cCmd [120];
	// 修改为使用简单的字符串格式，QoS=1
	sprintf ( cCmd, "AT+MQTTPUB=0,\"%s\",\"%s\",1,0", topicId, val );
	return ESP8266_Cmd ( cCmd, "OK", "ALREAY CONNECT", 2000 );
}

bool ESP8266_Set_MQTT_Public_Light(u16 light)
{
	char cCmd[120];
	char light_str[16];
	sprintf(light_str, "%u", light);
	sprintf(cCmd, "AT+MQTTPUB=0,\"sensor/light\",\"%s\",1,0", light_str);
	return ESP8266_Cmd(cCmd, "OK", "ALREAY CONNECT", 2000);
}

bool ESP8266_Set_MQTT_Sub (const char * topicId, const char * qos)
{
	char cCmd [120];
	sprintf ( cCmd, "AT+MQTTSUB=0,\"%s\",%s", topicId, qos );
	return ESP8266_Cmd (cCmd, "OK", "ALREAY CONNECT", 500);
}

bool ESP8266_StartOrShutServer ( FunctionalState enumMode, char * pPortNum, char * pTimeOver )
{
	char cCmd1 [120], cCmd2 [120];
	if ( enumMode )
	{
		sprintf ( cCmd1, "AT+CIPSERVER=%d,%s", 1, pPortNum );
		sprintf ( cCmd2, "AT+CIPSTO=%s", pTimeOver );
		return ( ESP8266_Cmd ( cCmd1, "OK", 0, 500 ) && ESP8266_Cmd ( cCmd2, "OK", 0, 500 ) );
	}
	else
	{
		sprintf ( cCmd1, "AT+CIPSERVER=%d,%s", 0, pPortNum );
		return ESP8266_Cmd ( cCmd1, "OK", 0, 500 );
	}
}

bool ESP8266_UnvarnishSend ( void )
{
	return ( ESP8266_Cmd ( "AT+CIPMODE=1", "OK", 0, 500 ) &&
	  ESP8266_Cmd ( "AT+CIPSEND", "\r\n", ">", 500 ) );
}

bool ESP8266_SendString ( FunctionalState enumEnUnvarnishTx, char * pStr, u32 ulStrLength, ENUM_ID_NO_TypeDef ucId )
{
	char cStr [20];
	bool bRet = false;
		
	if ( enumEnUnvarnishTx )
		ESP8266_Usart ( "%s", pStr );
	else
	{
		sprintf ( cStr, "AT+CIPSEND=%d", ulStrLength + 2 );
		ESP8266_Cmd ( cStr, "> ", 0, 1000 );
		bRet = ESP8266_Cmd ( pStr, "SEND OK", 0, 1000 );
	}
	
	return bRet;
}

char * ESP8266_ReceiveString ( FunctionalState enumEnUnvarnishTx )
{
	char * pRecStr = 0;
	strEsp8266_Fram_Record .InfBit .FramLength = 0;
	strEsp8266_Fram_Record .InfBit .FramFinishFlag = 0;
	while ( ! strEsp8266_Fram_Record .InfBit .FramFinishFlag );
	strEsp8266_Fram_Record .Data_RX_BUF [ strEsp8266_Fram_Record .InfBit .FramLength ] = '\0';
	
	if ( enumEnUnvarnishTx )
	{
		if ( strstr ( strEsp8266_Fram_Record .Data_RX_BUF, ">" ) )
			pRecStr = strEsp8266_Fram_Record .Data_RX_BUF;
	}
	else 
	{
		if ( strstr ( strEsp8266_Fram_Record .Data_RX_BUF, "+IPD" ) )
			pRecStr = strEsp8266_Fram_Record .Data_RX_BUF;
	}
	return pRecStr;
}

void ESP8266_STA_TCP_Client ( void )
{
	char cStrInput [100] = { 0 }, * pStrDelimiter [2], * pBuf, * pStr;
	u8 uc = 0;
  u32 ul = 0;

	ESP8266_Choose ( ENABLE );	
	ESP8266_AT_Test ();
	ESP8266_Net_Mode_Choose ( STA );
	ESP8266_Cmd ( "AT+CWLAP", "OK", 0, 5000 );
		
  do
	{
		PC_Usart ( "\r\nEnter WiFi SSID,password: " );
		scanf ( "%s", cStrInput );
		pBuf = cStrInput;
		uc = 0;
		while ( ( pStr = strtok ( pBuf, "," ) ) != NULL )
		{
			pStrDelimiter [ uc ++ ] = pStr;
			pBuf = NULL;
		} 
  } while ( ! ESP8266_JoinAP ( pStrDelimiter [0], pStrDelimiter [1] ) );
	
	ESP8266_Enable_MultipleId ( ENABLE );
	
	do 
	{
		PC_Usart ( "\r\nEnter Server IP,port: " );
		scanf ( "%s", cStrInput );
		pBuf = cStrInput;
		uc = 0;
		while ( ( pStr = strtok ( pBuf, "," ) ) != NULL )
		{
			pStrDelimiter [ uc ++ ] = pStr;
			pBuf = NULL;
		} 
  } while ( ! ( ESP8266_Link_Server ( enumTCP, pStrDelimiter [0], pStrDelimiter [1], Multiple_ID_0 ) &&
	              ESP8266_Link_Server ( enumTCP, pStrDelimiter [0], pStrDelimiter [1], Multiple_ID_1 ) &&
	              ESP8266_Link_Server ( enumTCP, pStrDelimiter [0], pStrDelimiter [1], Multiple_ID_2 ) &&
	              ESP8266_Link_Server ( enumTCP, pStrDelimiter [0], pStrDelimiter [1], Multiple_ID_3 ) &&
	              ESP8266_Link_Server ( enumTCP, pStrDelimiter [0], pStrDelimiter [1], Multiple_ID_4 ) ) );

  for ( uc = 0; uc < 5; uc ++ )
	{
		PC_Usart ( "\r\nEnter string for ID%d: ", uc );
		scanf ( "%s", cStrInput );
		ul = strlen ( cStrInput );
		ESP8266_SendString ( DISABLE, cStrInput, ul, ( ENUM_ID_NO_TypeDef ) uc );
	}
	
	PC_Usart ( "\r\nWaiting for data...\r\n" );
	while (1)
	{
	  pStr = ESP8266_ReceiveString ( DISABLE );
		PC_Usart ( "%s", pStr );
	}
}

void ESP8266_STA_TCP_Client_Single ( void )
{
    const char* ssid = "YOUR_WIFI_SSID";                
    const char* password = "YOUR_WIFI_PASSWORD";        
    const char* server_ip = "192.168.xxx.xxx"; 
    const char* server_port = "8080";        
    
    u8 wifi_retry = 0;
    bool wifi_connected = false;
    u8 server_retry = 0;
    bool server_connected = false;
    const char* test_data;
    u32 data_len;

    PC_Usart("\r\nInitializing ESP8266...\r\n");
    ESP8266_Choose(ENABLE);
    ESP8266_AT_Test();
    
    PC_Usart("\r\nConnecting to WiFi: %s...\r\n", ssid);
    ESP8266_Net_Mode_Choose(STA);
    
    while (wifi_retry < 3 && !wifi_connected) {
        wifi_connected = ESP8266_JoinAP((char*)ssid, (char*)password);
        if (!wifi_connected) {
            PC_Usart("\r\nWiFi connection failed, retrying %d/3...\r\n", wifi_retry+1);
            wifi_retry++;
            delay_ms(1000);
        }
    }
    
    PC_Usart("\r\nConnecting to server: %s:%s...\r\n", server_ip, server_port);
    while (server_retry < 3 && !server_connected) {
        server_connected = ESP8266_Link_Server(enumTCP, (char*)server_ip, (char*)server_port, Single_ID_0);
        if (!server_connected) {
            PC_Usart("\r\nServer connection failed, retrying %d/3...\r\n", server_retry+1);
            server_retry++;
            delay_ms(1000);
        }
    }
    
    test_data = "Hello from ESP8266!";
    data_len = strlen(test_data);
    PC_Usart("\r\nSending test data: %s\r\n", test_data);
    if (ESP8266_SendString(DISABLE, (char*)test_data, data_len, Single_ID_0)) {
        PC_Usart("\r\nData sent successfully!\r\n");
    } else {
        PC_Usart("\r\nFailed to send data!\r\n");
    }
    
    while (1) { }
}

void ESP8266_STA_TCP_Client_MQTT ( void )
{
    const char* ssid = "YOUR_WIFI_SSID";                
    const char* password = "YOUR_WIFI_PASSWORD";        
    const char* server_ip = "broker.emqx.io"; 
    const char* server_port = "1883";        
    
    u8 retry = 0;
    bool connected = false;
    u32 baud_rates[] = {115200, 9600, 19200, 38400, 57600};
    u8 baud_count = 5;
    u8 current_baud = 0;

    PC_Usart("\r\n====================================\r\n");
    PC_Usart("ESP8266 MQTT Initialization\r\n");
    PC_Usart("====================================\r\n");

    PC_Usart("\r\n[1/8] Enabling ESP8266...\r\n");
    ESP8266_Choose(ENABLE);
    delay_ms(500);
    
    PC_Usart("[2/8] Resetting ESP8266...\r\n");
    ESP8266_Rst();
    delay_ms(2000);
    
    PC_Usart("\r\n[3/8] Testing AT commands (auto-detect baud rate)...\r\n");
    for (current_baud = 0; current_baud < baud_count; current_baud++) {
        PC_Usart("      Trying baud rate: %d...\r\n", baud_rates[current_baud]);
        USART3_SetBaudRate(baud_rates[current_baud]);
        delay_ms(200);
        
        strEsp8266_Fram_Record.InfBit.FramLength = 0;
        strEsp8266_Fram_Record.InfBit.FramFinishFlag = 0;
        
        ESP8266_Usart("AT\r\n");
        delay_ms(1000);
        
        strEsp8266_Fram_Record.Data_RX_BUF[strEsp8266_Fram_Record.InfBit.FramLength] = '\0';
        
        if (strstr(strEsp8266_Fram_Record.Data_RX_BUF, "OK")) {
            PC_Usart("      AT Response OK!\r\n");
            PC_Usart("      Found correct baud rate: %d\r\n", baud_rates[current_baud]);
            connected = true;
            break;
        }
        PC_Usart("      No response\r\n");
    }
    
    if (!connected) {
        PC_Usart("ERROR: ESP8266 not responding! Check wiring.\r\n");
        PC_Usart("Please check:\r\n");
        PC_Usart("1. ESP8266 VCC connected to 3.3V (NOT 5V!)\r\n");
        PC_Usart("2. TX -> PB11, RX -> PB10 (cross-connected)\r\n");
        PC_Usart("3. CH/EN -> PA4, RST -> PA15\r\n");
        PC_Usart("4. GND connected properly\r\n");
        while(1) {
            delay_ms(2000);
            PC_Usart("Waiting for ESP8266...\r\n");
        }
    }
    
    PC_Usart("\r\n[4/8] Setting STA mode...\r\n");
    if (ESP8266_Net_Mode_Choose(STA)) {
        PC_Usart("      STA mode set OK!\r\n");
    }
    
    PC_Usart("\r\n[5/8] Connecting to WiFi: %s\r\n", ssid);
    connected = false;
    retry = 0;
    while (!connected) {
        PC_Usart("      Attempt %d... ", retry+1);
        connected = ESP8266_JoinAP((char*)ssid, (char*)password);
        if (!connected) {
            PC_Usart("FAIL\r\n");
            retry++;
            delay_ms(3000);
        } else {
            PC_Usart("OK!\r\n");
        }
    }

    PC_Usart("      Waiting for network stability...\r\n");
    delay_ms(3000);

    PC_Usart("\r\n[5.5/8] Checking network status and IP...\r\n");
    ESP8266_Cmd("AT+CIFSR", "OK", NULL, 2000);
    ESP8266_Cmd("AT+CIPSTATUS", "OK", NULL, 2000);
    
    PC_Usart("\r\n[5.6/8] Testing network connectivity...\r\n");
    ESP8266_Cmd("AT+PING=\"broker.emqx.io\"", "OK", NULL, 3000);
    
    PC_Usart("\r\n[5.8/8] Enabling multiple connections...\r\n");
    ESP8266_Enable_MultipleId(ENABLE);

    PC_Usart("\r\n[6/8] Setting MQTT user config...\r\n");
    connected = false;
    retry = 0;
    while (!connected) {
        connected = ESP8266_Set_MQTT_User();
        if (!connected) {
            PC_Usart("      Failed, retrying...\r\n");
            retry++;
            if (retry >= 3) {
                PC_Usart("      Resetting ESP8266 due to MQTT config failure...\r\n");
                ESP8266_Rst();
                delay_ms(2000);
                ESP8266_Net_Mode_Choose(STA);
                ESP8266_JoinAP((char*)ssid, (char*)password);
                delay_ms(3000);
                retry = 0;
            } else {
                delay_ms(1000);
            }
        } else {
            PC_Usart("      MQTT user config OK!\r\n");
        }
    }

    PC_Usart("\r\n[7/8] Connecting to MQTT server: %s:%s\r\n", server_ip, server_port);
    connected = false;
    retry = 0;
    while (!connected) {
        connected = ESP8266_Link_MQTT((char*)server_ip, (char*)server_port, Single_ID_0);
        if (!connected) {
            PC_Usart("      Failed, retrying...\r\n");
            retry++;
            if (retry >= 5) {
                PC_Usart("      Resetting ESP8266 due to MQTT connect failure...\r\n");
                ESP8266_Rst();
                delay_ms(2000);
                ESP8266_Net_Mode_Choose(STA);
                ESP8266_JoinAP((char*)ssid, (char*)password);
                delay_ms(3000);
                ESP8266_Set_MQTT_User();
                retry = 0;
            } else {
                delay_ms(5000);
            }
        } else {
            PC_Usart("      MQTT connected successfully!\r\n");
        }
    }

    PC_Usart("\r\n[8/8] Subscribing to topics...\r\n");
    connected = false;
    retry = 0;
    while (!connected) {
        connected = ESP8266_Set_MQTT_Sub("sensor/temperature", "1");
        if (!connected) {
            PC_Usart("      Failed, retrying...\r\n");
            retry++;
            if (retry >= 3) {
                PC_Usart("      Resetting ESP8266 due to subscribe failure...\r\n");
                ESP8266_Rst();
                delay_ms(2000);
                ESP8266_Net_Mode_Choose(STA);
                ESP8266_JoinAP((char*)ssid, (char*)password);
                delay_ms(3000);
                ESP8266_Set_MQTT_User();
                ESP8266_Link_MQTT((char*)server_ip, (char*)server_port, Single_ID_0);
                retry = 0;
            } else {
                delay_ms(1000);
            }
        } else {
            PC_Usart("      Subscribed to sensor/temperature!\r\n");
        }
    }
    
    PC_Usart("\r\n====================================\r\n");
    PC_Usart("MQTT initialization complete!\r\n");
    PC_Usart("====================================\r\n");
}

void ESP8266_AP_TCP_Server ( void )
{
	char cStrInput [100] = { 0 }, * pStrDelimiter [3], * pBuf, * pStr;
	u8 uc = 0;
    u32 ul = 0;

    ESP8266_Choose ( ENABLE );
	ESP8266_AT_Test ();
	ESP8266_Net_Mode_Choose ( AP );

	PC_Usart ( "\r\nEnter WiFi SSID,mode,password: " );
	scanf ( "%s", cStrInput );

	pBuf = cStrInput;
	uc = 0;
	while ( ( pStr = strtok ( pBuf, "," ) ) != NULL )
	{
		pStrDelimiter [ uc ++ ] = pStr;
		pBuf = NULL;
	} 
	
	ESP8266_BuildAP ( pStrDelimiter [0], pStrDelimiter [2], pStrDelimiter [1] );
	ESP8266_Cmd ( "AT+RST", "OK", "ready", 2500 );

	ESP8266_Enable_MultipleId ( ENABLE );		
	
	PC_Usart ( "\r\nEnter port,timeout: " );
	scanf ( "%s", cStrInput );

	pBuf = cStrInput;
	uc = 0;
	while ( ( pStr = strtok ( pBuf, "," ) ) != NULL )
	{
		pStrDelimiter [ uc ++ ] = pStr;
		pBuf = NULL;
	} 

	ESP8266_StartOrShutServer ( ENABLE, pStrDelimiter [0], pStrDelimiter [1] );
	
	do
	{
		PC_Usart ( "\r\nGetting IP...\r\n" );
	  ESP8266_Cmd ( "AT+CIFSR", "OK", "Link", 500 );
		delay_ms ( 20000 );
	}	while ( ! ESP8266_Cmd ( "AT+CIPSTATUS", "+CIPSTATUS:0", 0, 500 ) );

	PC_Usart ( "\r\nEnter string to send: " );
	scanf ( "%s", cStrInput );
	ul = strlen ( cStrInput );
	ESP8266_SendString ( DISABLE, cStrInput, ul, Multiple_ID_0 );

	PC_Usart ( "\r\nWaiting for data...\r\n" );
	while (1)
	{
	  pStr = ESP8266_ReceiveString ( DISABLE );
		PC_Usart ( "%s", pStr );
	}
}

void ESP8266_StaTcpClient_ApTcpServer ( void )
{
	char cStrInput [100] = { 0 }, * pStrDelimiter [3], * pBuf, * pStr;
	u8 uc = 0;
    u32 ul = 0;

    ESP8266_Choose ( ENABLE );
	ESP8266_AT_Test ();
	ESP8266_Net_Mode_Choose ( STA_AP );

	PC_Usart ( "\r\nEnter AP SSID,mode,password: " );
	scanf ( "%s", cStrInput );

	pBuf = cStrInput;
	uc = 0;
	while ( ( pStr = strtok ( pBuf, "," ) ) != NULL )
	{
		pStrDelimiter [ uc ++ ] = pStr;
		pBuf = NULL;
	} 
	
	ESP8266_BuildAP ( pStrDelimiter [0], pStrDelimiter [2], pStrDelimiter [1] );
	ESP8266_Cmd ( "AT+RST", "OK", "ready", 2500 );
	
	ESP8266_Cmd ( "AT+CWLAP", "OK", 0, 5000 );
		
  do
	{
		PC_Usart ( "\r\nEnter STA SSID,password: " );
		scanf ( "%s", cStrInput );
		pBuf = cStrInput;
		uc = 0;
		while ( ( pStr = strtok ( pBuf, "," ) ) != NULL )
		{
			pStrDelimiter [ uc ++ ] = pStr;
			pBuf = NULL;
		} 
  } while ( ! ESP8266_JoinAP ( pStrDelimiter [0], pStrDelimiter [1] ) );

	ESP8266_Enable_MultipleId ( ENABLE );
		
	PC_Usart ( "\r\nEnter port,timeout: " );
	scanf ( "%s", cStrInput );
	pBuf = cStrInput;
	uc = 0;
	while ( ( pStr = strtok ( pBuf, "," ) ) != NULL )
	{
		pStrDelimiter [ uc ++ ] = pStr;
		pBuf = NULL;
	} 

	ESP8266_StartOrShutServer ( ENABLE, pStrDelimiter [0], pStrDelimiter [1] );
	
	do 
	{
		PC_Usart ( "\r\nEnter Server IP,port: " );
		scanf ( "%s", cStrInput );
		pBuf = cStrInput;
		uc = 0;
		while ( ( pStr = strtok ( pBuf, "," ) ) != NULL )
		{
			pStrDelimiter [ uc ++ ] = pStr;
			pBuf = NULL;
		} 
  } while ( ! ( ESP8266_Link_Server ( enumTCP, pStrDelimiter [0], pStrDelimiter [1], Multiple_ID_0 ) &&
	              ESP8266_Link_Server ( enumTCP, pStrDelimiter [0], pStrDelimiter [1], Multiple_ID_1 ) &&
	              ESP8266_Link_Server ( enumTCP, pStrDelimiter [0], pStrDelimiter [1], Multiple_ID_2 ) ) );		
	
	do
	{
		PC_Usart ( "\r\nGetting IPs...\r\n" );
	  ESP8266_Cmd ( "AT+CIFSR", "OK", "Link", 500 );
		delay_ms ( 20000 );
	}	while ( ! ESP8266_Cmd ( "AT+CIPSTATUS", "+CIPSTATUS:3", 0, 500 ) );

	for ( uc = 0; uc < 3; uc ++ )
	{
		PC_Usart ( "\r\nEnter string for ID%d: ", uc );
		scanf ( "%s", cStrInput );
		ul = strlen ( cStrInput );
		ESP8266_SendString ( DISABLE, cStrInput, ul, ( ENUM_ID_NO_TypeDef ) uc );
	}
	
	PC_Usart ( "\r\nEnter string for ID3: " );
	scanf ( "%s", cStrInput );
	ul = strlen ( cStrInput );
	ESP8266_SendString ( DISABLE, cStrInput, ul, Multiple_ID_3 );

	PC_Usart ( "\r\nWaiting for data...\r\n" );
	while (1)
	{
	  pStr = ESP8266_ReceiveString ( DISABLE );
		PC_Usart ( "%s", pStr );
	}
}

/*******************************************************************************
* �� �� ��         : ESP8266_Start_WebServer
* ��������		   : ����Web������(TCP������ģʽ)
* ��    ��         : ��
* ��    ��         : ��
*******************************************************************************/
void ESP8266_Start_WebServer(void)
{
	char *ip_start, *ip_end;
	char ip_str[16];
	
	PC_Usart("\r\n[WebServer] Starting TCP server on port 80...\r\n");
	
	ESP8266_Enable_MultipleId(ENABLE);
	
	if(!ESP8266_StartOrShutServer(ENABLE, "80", "180"))
	{
		PC_Usart("[WebServer] Failed to start server\r\n");
		return;
	}
	
	PC_Usart("[WebServer] TCP server started on port 80\r\n");
	
	delay_ms(500);
	
	strEsp8266_Fram_Record.InfBit.FramLength = 0;
	strEsp8266_Fram_Record.InfBit.FramFinishFlag = 0;
	ESP8266_Usart("AT+CIFSR\r\n");
	delay_ms(1000);
	strEsp8266_Fram_Record.Data_RX_BUF[strEsp8266_Fram_Record.InfBit.FramLength] = '\0';
	
	ip_start = strstr(strEsp8266_Fram_Record.Data_RX_BUF, "\"192.");
	if(ip_start != NULL)
	{
		ip_start++;
		ip_end = strchr(ip_start, '"');
		if(ip_end != NULL)
		{
			*ip_end = '\0';
			strcpy(ip_str, ip_start);
			PC_Usart("\r\n====================================\r\n");
			PC_Usart("[WebServer] ESP8266 IP Address: %s\r\n", ip_str);
			PC_Usart("[WebServer] Open browser and visit: http://%s/\r\n", ip_str);
			PC_Usart("====================================\r\n");
		}
	}
	else
	{
		PC_Usart("[WebServer] Could not get IP address\r\n");
		PC_Usart("[WebServer] Try accessing via router to find ESP8266 IP\r\n");
	}
}

/*******************************************************************************
* �� �� ��         : ESP8266_Send_WebPage
* ��������		   : ͨ�����ӷ��ͷ���ʽ������Webҳ��
* ��    ��         : temperature: �¶�ֵ(������100��)
*                     light: ��Ч����ֵ
* ��    ��         : �ɹ�����true
*******************************************************************************/
bool ESP8266_Send_WebPage(int temperature, u16 light)
{
	char html_page[1400];
	char temp_str[16];
	char light_str[16];
	char cCmd[32];
	
	sprintf(temp_str, "%d.%02d", temperature/100, temperature%100);
	sprintf(light_str, "%u", light);
	
	sprintf(html_page, 
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 800\r\n"
		"Connection: close\r\n"
		"\r\n"
		"<!DOCTYPE html><html><head><meta charset='utf-8'><title>STM32 Sensor</title>"
		"<style>body{font-family:Arial;margin:40px;background:#f0f0f0}h1{color:#333}"
		".card{background:#fff;padding:20px;margin:10px 0;border-radius:10px;box-shadow:0 2px 5px rgba(0,0,0,0.1)}"
		".label{font-size:14px;color:#666}.value{font-size:28px;color:#2196F3;font-weight:bold}"
		".unit{font-size:16px;color:#999}.time{font-size:12px;color:#999;margin-top:5px}</style></head>"
		"<body><h1>STM32 Sensor Monitor</h1>"
		"<div class='card'><div class='label'>Temperature</div>"
		"<div class='value'>%s <span class='unit'>C</span></div>"
		"<div class='time' id='temp-time'></div></div>"
		"<div class='card'><div class='label'>Light Intensity</div>"
		"<div class='value'>%s <span class='unit'>(0-4095)</span></div>"
		"<div class='time' id='light-time'></div></div>"
		"<script>function updateTime(){var n=new Date();var t=n.toLocaleString('zh-CN');document.getElementById('temp-time').innerHTML=t;document.getElementById('light-time').innerHTML=t}updateTime();setInterval(updateTime,1000)</script>"
		"</body></html>",
		temp_str, light_str);
	
	strEsp8266_Fram_Record.InfBit.FramLength = 0;
	strEsp8266_Fram_Record.InfBit.FramFinishFlag = 0;
	ESP8266_Usart("AT+CIPSEND=0,%d\r\n", strlen(html_page));
	delay_ms(200);
	
	strEsp8266_Fram_Record.InfBit.FramLength = 0;
	strEsp8266_Fram_Record.InfBit.FramFinishFlag = 0;
	ESP8266_Usart("%s", html_page);
	delay_ms(500);
	
	ESP8266_Cmd("AT+CIPCLOSE=0", "OK", NULL, 2000);
	
	return true;
}

/*******************************************************************************
* �� �� ��         : ESP8266_Check_WebServer
* ��������		   : ���Web������Ƿ��ضӿͻ�����ѯ������н���
* ��    ��         : ��
* ��    ��         : �ضӿͻ�����ʱ����true
*******************************************************************************/
bool ESP8266_Check_WebServer(void)
{
	char *ipd_start;
	
	if(strEsp8266_Fram_Record.InfBit.FramFinishFlag)
	{
		strEsp8266_Fram_Record.Data_RX_BUF[strEsp8266_Fram_Record.InfBit.FramLength] = '\0';
		
		ipd_start = strstr(strEsp8266_Fram_Record.Data_RX_BUF, "+IPD,0,");
		if(ipd_start != NULL)
		{
			return true;
		}
		
		strEsp8266_Fram_Record.InfBit.FramLength = 0;
		strEsp8266_Fram_Record.InfBit.FramFinishFlag = 0;
	}
	return false;
}

/*******************************************************************************
* Function       : ESP8266_Init_AP_TCP_Server
* Description    : Initialize ESP8266 as AP + TCP Server for camera images
*                  SSID: PZ103-Camera, Password: 12345678, Port: 8080
*******************************************************************************/
void ESP8266_Init_AP_TCP_Server(void)
{
	u8 retry;

	PC_Usart("\r\n[AP] Setting AP mode...\r\n");
	if (!ESP8266_Net_Mode_Choose(AP)) {
		PC_Usart("[AP] ERROR: Failed to set AP mode!\r\n");
		return;
	}

	PC_Usart("[AP] Creating WiFi AP...\r\n");
	if (!ESP8266_BuildAP("PZ103-Camera", "12345678", "4")) {
		PC_Usart("[AP] WARNING: BuildAP may have failed, continuing...\r\n");
	}

	PC_Usart("[AP] Restarting module...\r\n");
	ESP8266_Cmd("AT+RST", "OK", "ready", 3000);
	delay_ms(2000);

	PC_Usart("[AP] Testing AT communication...\r\n");
	for (retry = 0; retry < 3; retry++) {
		ESP8266_Rst();
		delay_ms(1500);
		if (ESP8266_Cmd("AT", "OK", NULL, 1000))
			break;
		PC_Usart("[AP] AT test retry %d...\r\n", retry + 1);
	}

	PC_Usart("[AP] Enabling multiple connections...\r\n");
	if (!ESP8266_Enable_MultipleId(ENABLE)) {
		PC_Usart("[AP] ERROR: Failed to enable multiple connections!\r\n");
		return;
	}

	PC_Usart("[AP] Starting TCP server on port 8080...\r\n");
	if (!ESP8266_StartOrShutServer(ENABLE, "8080", "180")) {
		PC_Usart("[AP] ERROR: Failed to start TCP server!\r\n");
		return;
	}

	PC_Usart("\r\n[AP TCP Server] Ready on port 8080\r\n");
	PC_Usart("[AP TCP Server] SSID: PZ103-Camera, Password: 12345678\r\n");
}

/*******************************************************************************
* Function       : ESP8266_Init_STA_TCP_Server
* Description    : Initialize ESP8266 as STA + TCP Server
*                  Connect to existing WiFi router, start TCP server on port 8080
* Input          : pSSID - WiFi SSID, pPassWord - WiFi password
*******************************************************************************/
void ESP8266_Init_STA_TCP_Server(char * pSSID, char * pPassWord)
{
	u8 retry;

	PC_Usart("\r\n[STA] Setting STA mode...\r\n");
	if (!ESP8266_Net_Mode_Choose(STA)) {
		PC_Usart("[STA] ERROR: Failed to set STA mode!\r\n");
		return;
	}

	PC_Usart("[STA] Connecting to WiFi: %s\r\n", pSSID);
	for (retry = 0; retry < 10; retry++) {
		if (ESP8266_JoinAP(pSSID, pPassWord)) {
			PC_Usart("[STA] WiFi connected!\r\n");
			break;
		}
		PC_Usart("[STA] Retry %d...\r\n", retry + 1);
		delay_ms(3000);
	}

	delay_ms(2000);

	PC_Usart("[STA] Enabling multiple connections...\r\n");
	if (!ESP8266_Enable_MultipleId(ENABLE)) {
		PC_Usart("[STA] ERROR: Failed to enable multiple connections!\r\n");
		return;
	}

	PC_Usart("[STA] Starting TCP server on port 8080...\r\n");
	if (!ESP8266_StartOrShutServer(ENABLE, "8080", "180")) {
		PC_Usart("[STA] ERROR: Failed to start TCP server!\r\n");
		return;
	}

	PC_Usart("\r\n[STA TCP Server] Ready on port 8080\r\n");
}
