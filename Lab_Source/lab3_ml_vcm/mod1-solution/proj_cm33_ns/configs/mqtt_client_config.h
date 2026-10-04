/******************************************************************************
* File Name:   mqtt_client_config.h
*
* Description: This file contains all the configuration macros used by the
*              MQTT client in this example.
*
* Related Document: See README.md
*
*
*******************************************************************************
 * (c) 2025, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

#ifndef MQTT_CLIENT_CONFIG_H_
#define MQTT_CLIENT_CONFIG_H_

#include "cy_mqtt_api.h"
#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
* Macros
********************************************************************************/

/***************** MQTT CLIENT CONNECTION CONFIGURATION MACROS *****************/
/* MQTT Broker/Server address and port used for the MQTT connection. */
#define MQTT_BROKER_ADDRESS               "192.168.8.213"
#define MQTT_PORT                          50007

/* Set this macro to 1 if a secure (TLS) connection to the MQTT Broker is  
 * required to be established, else 0.
 */
#define MQTT_SECURE_CONNECTION            ( 1 )

/* Configure the user credentials to be sent as part of MQTT CONNECT packet */
#define MQTT_USERNAME                     "user1"
#define MQTT_PASSWORD                     "pwd"


/********************* MQTT MESSAGE CONFIGURATION MACROS **********************/
/* The MQTT topics to be used by the publisher and subscriber. */
#define MQTT_PUB_TOPIC                    "RED_APP_STATUS"
#define MQTT_SUB_TOPIC                    "GREEN_APP_STATUS"

/* Set the QoS that is associated with the MQTT publish, and subscribe messages.
 * Valid choices are 0, 1, and 2. Other values should not be used in this macro.
 */
#define MQTT_MESSAGES_QOS                 ( 1 )

/* Configuration for the 'Last Will and Testament (LWT)'. It is an MQTT message 
 * that will be published by the MQTT broker if the MQTT connection is 
 * unexpectedly closed. This configuration is sent to the MQTT broker during 
 * MQTT connect operation and the MQTT broker will publish the Will message on 
 * the Will topic when it recognizes an unexpected disconnection from the client.
 * 
 * If you want to use the last will message, set this macro to 1 and configure
 * the topic and will message, else 0.
 */
#define ENABLE_LWT_MESSAGE                ( 1 )
#if ENABLE_LWT_MESSAGE
    #define MQTT_WILL_TOPIC_NAME          MQTT_PUB_TOPIC "/will"
    #define MQTT_WILL_MESSAGE             ("MQTT client unexpectedly disconnected!")
#endif

/* MQTT messages which are published on the MQTT_PUB_TOPIC that controls the
 * device (user LED in this example) state in this code example.
 */
#define ON_MESSAGE                 "ON"
#define OFF_MESSAGE                "OFF"

/******************* OTHER MQTT CLIENT CONFIGURATION MACROS *******************/
/* A unique client identifier to be used for every MQTT connection. */
#define MQTT_CLIENT_IDENTIFIER            "PSoCEdge-MQTT-Client"

/* The timeout in milliseconds for MQTT operations in this example. */
#define MQTT_TIMEOUT_MS                   ( 5000 )

/* The keep-alive interval in seconds used for MQTT ping request. */
#define MQTT_KEEP_ALIVE_SECONDS           ( 60 )

/* Every active MQTT connection must have a unique client identifier. If you 
 * are using the above 'MQTT_CLIENT_IDENTIFIER' as client ID for multiple MQTT 
 * connections simultaneously, set this macro to 1. The device will then
 * generate a unique client identifier by appending a timestamp to the 
 * 'MQTT_CLIENT_IDENTIFIER' string. Example: 'psoc6-mqtt-client5927'
 */
#define GENERATE_UNIQUE_CLIENT_ID         ( 1 )

/* The longest client identifier that an MQTT server must accept (as defined
 * by the MQTT 3.1.1 spec) is 23 characters. However some MQTT brokers support 
 * longer client IDs. Configure this macro as per the MQTT broker specification. 
 */
#define MQTT_CLIENT_IDENTIFIER_MAX_LEN    ( 23 )

/* As per Internet Assigned Numbers Authority (IANA) the port numbers assigned 
 * for MQTT protocol are 1883 for non-secure connections and 8883 for secure
 * connections. In some cases there is a need to use other ports for MQTT like
 * port 443 (which is reserved for HTTPS). Application Layer Protocol 
 * Negotiation (ALPN) is an extension to TLS that allows many protocols to be 
 * used over a secure connection. The ALPN ProtocolNameList specifies the 
 * protocols that the client would like to use to communicate over TLS.
 * 
 * This macro specifies the ALPN Protocol Name to be used that is supported
 * by the MQTT broker in use.
 * Note: For AWS IoT, currently "x-amzn-mqtt-ca" is the only supported ALPN 
 *       ProtocolName and it is only supported on port 443.
 * 
 * Uncomment the below line and specify the ALPN Protocol Name to use this 
 * feature.
 */
/* #define MQTT_ALPN_PROTOCOL_NAME           "x-amzn-mqtt-ca" */

/* Server Name Indication (SNI) is extension to the Transport Layer Security 
 * (TLS) protocol. As required by some MQTT Brokers, SNI typically includes the 
 * hostname in the Client Hello message sent during TLS handshake.
 * 
 * Uncomment the below line and specify the SNI Host Name to use this extension
 * as specified by the MQTT Broker.
 */
/* #define MQTT_SNI_HOSTNAME                 "SNI_HOST_NAME" */

/* A Network buffer is allocated for sending and receiving MQTT packets over 
 * the network. Specify the size of this buffer using this macro.
 * 
 * Note: The minimum buffer size is defined by 'CY_MQTT_MIN_NETWORK_BUFFER_SIZE' 
 * macro in the MQTT library. Please ensure this macro value is larger than 
 * 'CY_MQTT_MIN_NETWORK_BUFFER_SIZE'.
 */
#define MQTT_NETWORK_BUFFER_SIZE          ( 2 * CY_MQTT_MIN_NETWORK_BUFFER_SIZE )

/* Maximum MQTT connection re-connection limit. */
#define MAX_MQTT_CONN_RETRIES            (150u)

/* MQTT re-connection time interval in milliseconds. */
#define MQTT_CONN_RETRY_INTERVAL_MS      (2000)


/**************** MQTT CLIENT CERTIFICATE CONFIGURATION MACROS ****************/

/* Configure the below credentials in case of a secure MQTT connection. */
/* PEM-encoded client certificate */
#define CLIENT_CERTIFICATE       \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDYTCCAkkCFENMHBIB1iyLJ6fEvC6CenvSEglvMA0GCSqGSIb3DQEBCwUAMGcx\n"\
"CzAJBgNVBAYTAklOMRIwEAYDVQQIDAlLYXJuYXRha2ExEjAQBgNVBAcMCUJlbmdh\n"\
"bHVydTELMAkGA1UECgwCQ1kxFDASBgNVBAsMC0VuZ2luZWVyaW5nMQ0wCwYDVQQD\n"\
"DARteUNBMB4XDTI2MTAwMjIxNDg1NloXDTM2MDkyOTIxNDg1NlowczELMAkGA1UE\n"\
"BhMCSU4xEjAQBgNVBAgMCUthcm5hdGFrYTESMBAGA1UEBwwJQmVuZ2FsdXJ1MQsw\n"\
"CQYDVQQKDAJDWTEUMBIGA1UECwwLRW5naW5lZXJpbmcxGTAXBgNVBAMMEG1vc3F1\n"\
"aXR0b19jbGllbnQwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC2UmRQ\n"\
"kZ37uZ2H7Xg9o8YarXl1ODufGUURyrvyhE8DZWwDQY/iVBVyGWJRVcaHeZiI+tsI\n"\
"lLhWmpcBVh+7bjESnV7Edpm27/Lj/7/dVZWhJUNNwRiJIAbx0ogmtXp6EKSZ5vym\n"\
"ltmSqzCJ980q2H+UoifaI/xkPt5ZJr1QocpzL2YLWC4J0RBlZkWVOcaBDdMgOfTz\n"\
"4U397zLbWva1dI3g5b7Hk8LC6OR0EtqCexfGFUf8sYWcI5gcvx2LsmBBS7y/UEBH\n"\
"tV2pYPdIP1/u0Y5nDxzLHfKet1ocG/AFwef+AYmL1sKKzEt0SchiiYGmgHfoEGsR\n"\
"1HmOuB6s9UVBId9vAgMBAAEwDQYJKoZIhvcNAQELBQADggEBAJugA+Gta1/bWbOV\n"\
"bK2ZPPtfMgV1fUCpr7jbk4kWLddjsBNZ6fWlNtdracierE8FUZxv2he89efUp3NS\n"\
"N0FZPqwMTCs+f+Tey0PDKlZrReE8hwNzNJR7vdVXWhVjYVwbcLKmGVnh6zayhBm+\n"\
"EI0UxvzcxR56/JMS+3RKbhmNI7CxCFkFbQoEH+qK9lWP/QQfo1Xks/8OpmMjZGqL\n"\
"x9UgcjWwIJH8LStIVzvxfAPegHnm3u/E6ZkeXqZ5Kd6odvQZjnbNeV9QI7zrMzi0\n"\
"7fqMT67i3Ms9Hzr9sL/H2vrPCsdeHHHsS3Yo/OSHUqUEItKgkjGBCGLhY1JXBNIX\n"\
"bde8Dt8=\n"\
"-----END CERTIFICATE-----\n"

/* PEM-encoded client private key */
#define CLIENT_PRIVATE_KEY          \
 "-----BEGIN PRIVATE KEY-----\n"\
"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQC2UmRQkZ37uZ2H\n"\
"7Xg9o8YarXl1ODufGUURyrvyhE8DZWwDQY/iVBVyGWJRVcaHeZiI+tsIlLhWmpcB\n"\
"Vh+7bjESnV7Edpm27/Lj/7/dVZWhJUNNwRiJIAbx0ogmtXp6EKSZ5vymltmSqzCJ\n"\
"980q2H+UoifaI/xkPt5ZJr1QocpzL2YLWC4J0RBlZkWVOcaBDdMgOfTz4U397zLb\n"\
"Wva1dI3g5b7Hk8LC6OR0EtqCexfGFUf8sYWcI5gcvx2LsmBBS7y/UEBHtV2pYPdI\n"\
"P1/u0Y5nDxzLHfKet1ocG/AFwef+AYmL1sKKzEt0SchiiYGmgHfoEGsR1HmOuB6s\n"\
"9UVBId9vAgMBAAECggEAWBH1riuhJmszqujtl8zoUZOxo4t91W0l/aGyZ0Q9TLUt\n"\
"12bQo7IdR+f2I7bs9x0oLycKLht07jSvs/AP1QC2CLlnAT0PJJzE9hjg7AA/DsAK\n"\
"wmD/wqFraV3a8ePhHVyzvjojmi8tO1mhUUwX2dYJztkDqi6O6TerPWJmua/ltPyD\n"\
"BF+O+WeU/mV4/pH4DNJ/vfghDcWjNwnquc0kb2qKa336ZQSVLFCdR+LMWg85rptr\n"\
"6YimoDArlYO/1ZAWDIY0wHWjXOZe9OErCwuXNbg+jdn8QdJjcmpyjda9wBNenuAs\n"\
"df8RcH1kylx+Q/QsKfAorpAv118fjNMnn0AW1Hb9uQKBgQDwWmF5n1V33Iq2Hjgn\n"\
"KVFpVXL56lRkxE7E3EXZDCuSiRtNU/gYjDLXQF9xQmgk8wsWrngfMXAWFOQQQgTa\n"\
"mORUu4nnNwLCpcUbes0KJ1o8WSRvxLI7+lKVhIrIunc4ffsLPzZtQZM6EuHq7f9C\n"\
"5yrzbjQ6Vknhd04VEFfUldG1pwKBgQDCMONmOz6oCm3CD+QW4TsxlS4+5XqUNKod\n"\
"f0BYO0/t8oHvnz5HpL4VtHhuESWqvPiE7+HH7w86OYMyYNem8fo0CogxK3+oSUDU\n"\
"NBeZDX757SVIQ6Buea5qnFXvWUnhHPd+fv4/MGYH6LlJmu5x0jezjuPn3DL/Rroa\n"\
"WQBrOfxQ+QKBgQDPMwsO3vW+I6iMwVZlJDBjnt9EZOcmCzlgagfoyZ4ScBHSQs4A\n"\
"03PMrljY+YdwOvlXL0aslWDsGExXW6J1lBJanWWPppPBm0hlnSJ/W1dl6O8JT0bb\n"\
"f7uL27wMuPqn/6rYkkDoRPyXtsl9Tnicg046lsl9dP+x17i/XdxpjlI/xwKBgGlS\n"\
"jLNQ5K3NYjRD3CjQpgNBbyCr4+zoF3ACKYrxOGvNAM5PJz9CSdqJ1FuWL0DIV136\n"\
"oRGIRlEFCnRTdANW8KYzJCTO++DxQhkV28qmOD0jcvobu7LPilrGShGT8u8Gf/F6\n"\
"vTjWbjBR99TFFBhltNJNaKzDkGFGIf/ST9jYTVI5AoGBAM56Qq8f/gGxh4+wQ4HQ\n"\
"hlua75zpqpAOkeC11X7kMgexWOWwPUyR1dh8MJGPxCz69+gb/BaVE5EOYgB0jySi\n"\
"8jVmy3KYeIaeR9bgYWcgHVrvB4BlmTYE8tR0FOnsSOLbNH5zX4EehbRc67H4oSYY\n"\
"JrzrJfbEfSL+jHlU8K+2HL7O\n"\
"-----END PRIVATE KEY-----\n"

/* PEM-encoded Root CA certificate */
#define ROOT_CA_CERTIFICATE     \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDrzCCApegAwIBAgIUcX4wui1JiAYhvFCjeucFygRIm9EwDQYJKoZIhvcNAQEL\n"\
"BQAwZzELMAkGA1UEBhMCSU4xEjAQBgNVBAgMCUthcm5hdGFrYTESMBAGA1UEBwwJ\n"\
"QmVuZ2FsdXJ1MQswCQYDVQQKDAJDWTEUMBIGA1UECwwLRW5naW5lZXJpbmcxDTAL\n"\
"BgNVBAMMBG15Q0EwHhcNMjYxMDAyMjE0ODUyWhcNMzYwOTI5MjE0ODUyWjBnMQsw\n"\
"CQYDVQQGEwJJTjESMBAGA1UECAwJS2FybmF0YWthMRIwEAYDVQQHDAlCZW5nYWx1\n"\
"cnUxCzAJBgNVBAoMAkNZMRQwEgYDVQQLDAtFbmdpbmVlcmluZzENMAsGA1UEAwwE\n"\
"bXlDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAOFl1F4kJcLcrbYn\n"\
"mA775zElKAxdaEGu0Zf/5UR711CSrabV6U9c5FkehEYCKJDgs+ktKowqyDCE4bJq\n"\
"G1WKiY8rg7zKoCR7G1B+mfIFvdHVQW1CciNDQm/R6GTS7EEfaLO3xgeU8naae+Ad\n"\
"EtMT0EXrD3BXMPFfqnNWqznhUPH4tANlVLPZs4DJuPVCd5QQ4HrI3mtmAXClmzFs\n"\
"zGGaZ07qGPTx87kJHLAJLb1YYjfdFGQtida0JRHVNECPihxzAjB+fMdE3xj3mah1\n"\
"oJw5jHhb4Pm/k8xV7aZSo4JeCIX+ZMzOp0vDq1LUtdoJ7C8p1+P55cnkf1YX9yRf\n"\
"Igw3/XcCAwEAAaNTMFEwHQYDVR0OBBYEFIsFlffM9NOa1XTNn+Rh6cTLdsGeMB8G\n"\
"A1UdIwQYMBaAFIsFlffM9NOa1XTNn+Rh6cTLdsGeMA8GA1UdEwEB/wQFMAMBAf8w\n"\
"DQYJKoZIhvcNAQELBQADggEBALWbZYwZsmKhdqyeAzPM0ScaA3ZBueT4RDpytqcF\n"\
"8FK5X7HAh+kfaifwiboNZTzy09kZAQXfvJaMVMuV/dkVYsmkgAAWgiIsOzLQ/eYn\n"\
"qSJgEDjuNVnKuFOOR2x4g2GC8T1aIi5HKYxj+RUcoRuGxQPYq7831paa2XZUPafh\n"\
"q1Q5ossPshSCAe304X8E7COIs8N9SHx4BG+gaop/uq7+0gYQeNbnADI2vuB8cks4\n"\
"if7OeeTG8UXsfgZCIqbuQIrdxVtP4HZDfuf7aMeELCRK3L/hgLZ6dGOro+Dj9hjM\n"\
"LULMWRYfbrfywzLrvJgcQb5sbTLSjmY92cma6jK5YpUYYA0=\n"\
"-----END CERTIFICATE-----\n"


/******************************************************************************
* Global Variables
*******************************************************************************/
extern cy_mqtt_broker_info_t broker_info;
extern cy_awsport_ssl_credentials_t  *security_info;
extern cy_mqtt_connect_info_t connection_info;

#if defined(__cplusplus)
}
#endif
#endif /* MQTT_CLIENT_CONFIG_H_ */
