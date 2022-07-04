# picow-websocket
Websocket implementation on Raspberry Pi Pico W 2040

An [example program](https://github.com/samjkent/picow-websocket) that takes the [TCP client](https://github.com/raspberrypi/pico-examples/blob/master/pico_w/tcp_client/picow_tcp_client.c) example and extends it to create a websocket connection

## TCP to Websocket

### HTTP Upgrade

Once the TCP connection has been made, rather than sending bytes of data to be echoed back as per the example, the program writes a HTTP GET request that asks the server to upgrade the connection to a websocket connection.

```
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    if (err != ERR_OK) {
        printf("Connect failed %d\n", err);
        // return tcp_result(arg, err);
    }
 
    // Write HTTP GET Request with Websocket upgrade 
    state->buffer_len = sprintf((char*)state->buffer, "GET / HTTP/1.1\r\nHost: 139.162.236.174:8082\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\nSec-WebSocket-Protocol: test\r\nSec-WebSocket-Version: 13\r\n\r\n");
    err = tcp_write(state->tcp_pcb, state->buffer, state->buffer_len, TCP_WRITE_FLAG_COPY);

    state->connected = TCP_CONNECTED;

    printf("Connected\r\n");
    return ERR_OK;
}

```

The HTTP GET requesting the connection upgrade:

```
GET / HTTP/1.1
Host: 139.162.236.174:8082
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==
Sec-WebSocket-Protocol: test
Sec-WebSocket-Version: 13

```
Note: newlines are important and must be sent as `\r\n` (including the empty terminating line)

The client should check that the server has responded confirming that the upgrade has been successful with a reply along the lines of:

```
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: x73hihjdisjd==
Sec-WebSocket-Protocol: test

```

However this is not implemented here.

### Websocket Communication

Once the connection has been upgraded to a websocket connection data the communication must be made using websocket packets

```
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-------+-+-------------+-------------------------------+
 |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 | |1|2|3|       |K|             |                               |
 +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 |     Extended payload length continued, if payload len == 127  |
 + - - - - - - - - - - - - - - - +-------------------------------+
 |                               |Masking-key, if MASK set to 1  |
 +-------------------------------+-------------------------------+
 | Masking-key (continued)       |          Payload Data         |
 +-------------------------------- - - - - - - - - - - - - - - - +
 :                     Payload Data continued ...                :
 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 |                     Payload Data continued ...                |
 +---------------------------------------------------------------+

```

The WS class in [ws.h](ws.h) / [ws.cpp](ws.cpp) can parse and create packets to send. 
[RFC6455](https://datatracker.ietf.org/doc/html/rfc6455) specifies the protocol and packet structure.

#### Parsing

The code iterates over all `pbuf` received over TCP and passes the payload to the parsing function. The payloads are copied into a contiguous buffer to handle receiving multiple `pbuf` objects at once (e.g. if the TCP polling frequency is low and multiple packets have been queued up)

```
for (struct pbuf *q = p; q != NULL; q = q->next) {
            if((state->rx_buffer_len + q->len) < BUF_SIZE) {
                WebsocketPacketHeader_t header;
                WS::ParsePacket(&header, (char *)q->payload, q->len);
                memcpy(state->rx_buffer + state->rx_buffer_len, (uint8_t *)q->payload + header.start, header.length);
                state->rx_buffer_len += header.length;
            }
        }
```

#### Creating

Websocket packets are created using the `BuildPacket` function. This takes a buffer, payload, and OPCODE and constructs the packet. The buffer can then be sent.

```
char test[20] = "test";
state->buffer_len = WS::BuildPacket((char *)state->buffer, BUF_SIZE, WEBSOCKET_OPCODE_TEXT, test, sizeof(test), 1);
err_t err = tcp_write(state->tcp_pcb, state->buffer, state->buffer_len, TCP_WRITE_FLAG_COPY);
```
