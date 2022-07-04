#include <string.h>
#include <math.h>
#include <vector>
#include <cstdlib>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "ws.h"

#define TEST_TCP_SERVER_IP "REMOVED"
#define TCP_PORT 8082

#define BUF_SIZE 2048

#define TCP_DISCONNECTED 0
#define TCP_CONNECTING   1
#define TCP_CONNECTED    2

typedef struct TCP_CLIENT_T_ {
    struct tcp_pcb *tcp_pcb;
    ip_addr_t remote_addr;
    uint8_t buffer[BUF_SIZE];
    uint8_t rx_buffer[BUF_SIZE];
    int buffer_len;
    int rx_buffer_len;
    int sent_len;
    int connected;
} TCP_CLIENT_T;

char text[BUF_SIZE] = "Disconnected";

static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    printf("tcp_client_sent %u\n", len);
    return ERR_OK;
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    if (err != ERR_OK) {
        printf("Connect failed %d\n", err);
        // return tcp_result(arg, err);
    }
 
    // Write HTTP GET Request with Websocket upgrade 
    state->buffer_len = sprintf((char*)state->buffer, "GET / HTTP/1.1\r\nHost: REMOVED:8082\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\nSec-WebSocket-Protocol: chat, superchat\r\nSec-WebSocket-Version: 13\r\n\r\n");
    err = tcp_write(state->tcp_pcb, state->buffer, state->buffer_len, TCP_WRITE_FLAG_COPY);

    state->connected = TCP_CONNECTED;

    printf("Connected\r\n");
    return ERR_OK;
}

static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
    printf("tcp_client_poll\n");
    return ERR_OK;
}

static void tcp_client_err(void *arg, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    state->connected = TCP_DISCONNECTED;
    if (err != ERR_ABRT) {
        printf("tcp_client_err %d\n", err);
    } else {
        printf("tcp_client_err abort %d\n", err);
    }
}

static err_t tcp_client_close(void *arg) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    err_t err = ERR_OK;
    if (state->tcp_pcb != NULL) {
        tcp_arg(state->tcp_pcb, NULL);
        tcp_poll(state->tcp_pcb, NULL, 0);
        tcp_sent(state->tcp_pcb, NULL);
        tcp_recv(state->tcp_pcb, NULL);
        tcp_err(state->tcp_pcb, NULL);
        err = tcp_close(state->tcp_pcb);
        if (err != ERR_OK) {
            printf("close failed %d, calling abort\n", err);
            tcp_abort(state->tcp_pcb);
            err = ERR_ABRT;
        }
        state->tcp_pcb = NULL;
    }
    state->connected = TCP_DISCONNECTED;
    return err;
}

err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    printf("recv \r\n");
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    if (!p) {
        // return tcp_result(arg, -1);
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    state->rx_buffer_len = 0;

    if(p == NULL) {
        // Close
        tcp_client_close(arg);
        return ERR_OK;
    }

    if (p->tot_len > 0) {
        for (struct pbuf *q = p; q != NULL; q = q->next) {
            if((state->rx_buffer_len + q->len) < BUF_SIZE) {
                WebsocketPacketHeader_t header;
                WS::ParsePacket(&header, (char *)q->payload, q->len);
                memcpy(state->rx_buffer + state->rx_buffer_len, (uint8_t *)q->payload + header.start, header.length);
                state->rx_buffer_len += header.length;
            }
        }
        printf("tcp_recved \r\n");
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);

    return ERR_OK;
}


static err_t connect(void *arg) {
  TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    
  if(state->connected != TCP_DISCONNECTED) return ERR_OK;
  
  state->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&state->remote_addr));
  if (!state->tcp_pcb) {
      return false;
  }

  tcp_arg(state->tcp_pcb, state);
  tcp_poll(state->tcp_pcb, tcp_client_poll, 1);
  tcp_sent(state->tcp_pcb, tcp_client_sent);
  tcp_recv(state->tcp_pcb, tcp_client_recv);
  tcp_err(state->tcp_pcb, tcp_client_err);

  state->buffer_len = 0;
  // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
  // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
  // these calls are a no-op and can be omitted, but it is a good practice to use them in
  // case you switch the cyw43_arch type later.
  cyw43_arch_lwip_begin();
  state->connected = TCP_CONNECTING;
  err_t err = tcp_connect(state->tcp_pcb, &state->remote_addr, TCP_PORT, tcp_client_connected);
  cyw43_arch_lwip_end();
        
  return ERR_ABRT;
}

int main() {

  stdio_usb_init();

  // Connect to network
  char ssid[] = "REMOVED";
  char pass[] = "REMOVED";

  if (cyw43_arch_init_with_country(CYW43_COUNTRY_UK)) {
    printf("failed to initialise\n");
    return 1;
  }

  cyw43_arch_enable_sta_mode();

  if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
    return 1;
  }

  // Set up TCP
  TCP_CLIENT_T *state = (TCP_CLIENT_T *)calloc(1, sizeof(TCP_CLIENT_T));
  if (!state) {
      printf("failed to allocate state\n");
      return false;
  }
  ip4addr_aton(TEST_TCP_SERVER_IP, &state->remote_addr);

  printf("Connecting to %s port %u\n", ip4addr_ntoa(&state->remote_addr), TCP_PORT);
  connect(state);

  uint32_t sw_timer;

  while(true) {

    // Print buffer contents and reset it
    if(state->rx_buffer_len) {
        printf("%.*s \r\n", state->rx_buffer_len, (char*)state->rx_buffer);
        state->rx_buffer_len = 0;
    }
    

    // Every second
    if((to_ms_since_boot(get_absolute_time()) - sw_timer) > 1000)
    {
        if(state->connected == TCP_CONNECTED) {
            // write to tcp
            char test[20] = "test";
            state->buffer_len = WS::BuildPacket((char *)state->buffer, BUF_SIZE, WEBSOCKET_OPCODE_TEXT, test, sizeof(test), 1);
            err_t err = tcp_write(state->tcp_pcb, state->buffer, state->buffer_len, TCP_WRITE_FLAG_COPY);
            if (err != ERR_OK) {
                printf("Failed to write data %d\n", err);
            }
        }
        
        if(state->connected == TCP_DISCONNECTED) {        
            // Reconnect
            connect(state);
        }

        sw_timer = to_ms_since_boot(get_absolute_time());
    }

  }

    return 0;
}

