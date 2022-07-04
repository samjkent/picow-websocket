#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ws.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

uint64_t WS::BuildPacket(char* buffer, uint64_t bufferLen, enum WebSocketOpCode opcode, char* payload, uint64_t payloadLen, int mask) {
    WebsocketPacketHeader_t header;

    int payloadIndex = 2;
    
    // Fill in meta.bits
    header.meta.bits.FIN = 1;
    header.meta.bits.RSV = 0;
    header.meta.bits.OPCODE = opcode;
    header.meta.bits.MASK = mask;

    // Calculate length
    if(payloadLen < 126) {
        header.meta.bits.PAYLOADLEN = payloadLen;
    } else if(payloadLen < 0x10000) {
        header.meta.bits.PAYLOADLEN = 126;
    } else {
        header.meta.bits.PAYLOADLEN = 127;
    }

    buffer[0] = header.meta.bytes.byte0;
    buffer[1] = header.meta.bytes.byte1;

    // Generate mask
    header.mask.maskKey = (uint32_t)rand();

    // Mask payload
    if(header.meta.bits.MASK) {
            for(uint64_t i = 0; i < payloadLen; i++) {
                payload[i] = payload[i] ^ header.mask.maskBytes[i%4];
            }
      }
    
    // Fill in payload length
    if(header.meta.bits.PAYLOADLEN == 126) {
            buffer[2] = (payloadLen >> 8) & 0xFF;
            buffer[3] = payloadLen & 0xFF;
        payloadIndex = 4;
     }

    if(header.meta.bits.PAYLOADLEN == 127) {
         buffer[2] = (payloadLen >> 56) & 0xFF;
         buffer[3] = (payloadLen >> 48) & 0xFF;
         buffer[4] = (payloadLen >> 40) & 0xFF;
         buffer[5] = (payloadLen >> 32) & 0xFF;
         buffer[6] = (payloadLen >> 24) & 0xFF;
         buffer[7] = (payloadLen >> 16) & 0xFF;
         buffer[8] = (payloadLen >> 8)  & 0xFF;
         buffer[9] = payloadLen & 0xFF;
        payloadIndex = 10;
    }

    // Insert masking key
    if(header.meta.bits.MASK) {
        buffer[payloadIndex] = header.mask.maskBytes[0];
        buffer[payloadIndex + 1] = header.mask.maskBytes[1];
        buffer[payloadIndex + 2] = header.mask.maskBytes[2];
        buffer[payloadIndex + 3] = header.mask.maskBytes[3];
        payloadIndex += 4;
    }

    // Ensure the buffer can handle the packet
    if((payloadLen + payloadIndex) > bufferLen) {
        printf("WEBSOCKET BUFFER OVERFLOW \r\n");
        return 1;
    }

    // Copy in payload
    // memcpy(buffer + payloadIndex, payload, payloadLen);
    for(int i = 0; i < payloadLen; i++) {
        buffer[payloadIndex + i] = payload[i];
    }

    return (payloadIndex + payloadLen);

}

int WS::ParsePacket(WebsocketPacketHeader_t *header, char* buffer, uint32_t len)
{
    header->meta.bytes.byte0 = (uint8_t) buffer[0];
    header->meta.bytes.byte1 = (uint8_t) buffer[1];

    // Payload length
    int payloadIndex = 2;
    header->length = header->meta.bits.PAYLOADLEN;

    if(header->meta.bits.PAYLOADLEN == 126) {
        header->length = buffer[2] << 8 | buffer[3];
        payloadIndex = 4;
    }
    
    if(header->meta.bits.PAYLOADLEN == 127) {
        header->length = buffer[6] << 24 | buffer[7] << 16 | buffer[8] << 8 | buffer[9];
        payloadIndex = 10;
    }

    // Mask
    if(header->meta.bits.MASK) {
        header->mask.maskBytes[0] = buffer[payloadIndex + 0];
        header->mask.maskBytes[0] = buffer[payloadIndex + 1];
        header->mask.maskBytes[0] = buffer[payloadIndex + 2];
        header->mask.maskBytes[0] = buffer[payloadIndex + 3];
        payloadIndex = payloadIndex + 4;    
        
        // Decrypt    
        for(uint64_t i = 0; i < header->length; i++) {
                buffer[payloadIndex + i] = buffer[payloadIndex + i] ^ header->mask.maskBytes[i%4];
            }
    }

    // Payload start
    header->start = payloadIndex;

    return 0;

}

