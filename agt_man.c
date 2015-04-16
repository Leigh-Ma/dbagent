#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "agt_man.h"

extern const unsigned short g_server_port;
pthread_t g_listener_thread = 0;
pthread_mutex_t g_client_handle_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *agt_listener_thread(void *params) {
  struct sockaddr_in local;
  struct sockaddr    remote;
  int sock, con, remote_len;

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(sock <= 0) {
    logger("Open socket error");
    return -1;
  }

  local.sin_family      = AF_INET;
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  local.sin_port        = htons(g_server_port);

  if( 0 != bind(sock, (const struct sockaddr *)&local, sizeof(local)) ) {
    logger("Bind socket error");
    return -1;
  }

  if( 0 != listen(sock, 10) ) {
    logger("Listen socket error");
  }

  while(1) {
    con = 0;
    memset(&remote, 0x00, sizeof(remote));
    if(0 < (con  = accept(sock, &remote, &remote_len)) ) {
      //TODO
      //start a new db connection and assign it to the client
      //notify the client ?
        agt_client_handle(con);
    }
  }

  //should never
  pthread_exit();
}

void *agt_client_handler_thread(void *params){
    db_init();
    while(1){
        /*serving*/
    }
    db_close();
    pthread_exit();
}

int agt_listener_start() {
  //start a thread to waiting connections from clients
    int status = pthread_create(&g_listener_thread, (pthread_attr_t*)NULL,
            agt_listener_thread, (void*)NULL);

    if( 0!= status) {
        logger("pthread_create create socket listener error: %d", status);
    } else {
        logger("Connection Listener thread has been started");
    }
    return status;
}

int agt_client_handle(int client){
    int identifier = client, status;
    pthread_t  handler_thread;

    pthread_mutex_lock(&g_client_handle_mutex);
    status = pthread_create(&handler_thread, (pthread_attr_t*)NULL,
            agt_client_handler_thread, (void*)&identifier);
    if( 0!= status) {
        logger("pthread_create create socket listener error: %d", status);
    } else {
        logger("Connection Listener thread has been started");
    }
    pthread_mutex_unlock(&g_client_handle_mutex);
    return status;
}

int agt_man_main() {
  agt_listener_start();
  //Timely check the status and close connection if necessary
  return 0;
}

