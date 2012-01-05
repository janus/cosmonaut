#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <ev.h>


#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>


struct global_config* configuration;
sig_atomic_t server_socket_fd;

#include "log.h"
#include "signals.h"
#include "networking.h"
#include "base_request_handler.h"
#include "configuration.h"
#include "action.h"

struct ev_loop *global_loop;

void action_index(http_request* request, http_response *response) {
  render_file(response, "index.html");
}

void action_upload(http_request* request, http_response *response) {
  render_text(response, "Uploaded!");
}

void accept_cb(struct ev_loop *loop, struct ev_io *w, int revents) {
  int client_sd;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  if (EV_ERROR & revents) {
    perror("got invalid event");
    return;
  }

  while ((client_sd = accept(w->fd, (struct sockaddr *)&client_addr, &client_len)) > 0) {
    fcntl(client_sd, F_SETFL, fcntl(client_sd, F_GETFL, 0) | O_NONBLOCK);
    if (!fork()) {
      info("FORKED");
      close(server_socket_fd); 
      handle_request(client_sd);
      free_configuration();
      exit(0);
    }
  }

  if (errno != EAGAIN && errno != EINTR && errno != EWOULDBLOCK) {
    perror("accept error");
    return;
  }
}

void accept_connections() {
  ev_io accept_watcher;

  ev_io_init(&accept_watcher, accept_cb, server_socket_fd, EV_READ);
  ev_io_start(global_loop, &accept_watcher);
  ev_loop(global_loop, 0);

  // We don't get here.
  close(server_socket_fd);
}

int main(int argc, char *argv[]) {
  int new_connection_fd;

  load_configuration(argc, argv);

  route("/", action_index);
  route("/upload_file", action_upload);

  server_socket_fd = bind_server_socket_fd();
  setup_signal_listeners(server_socket_fd);

  global_loop = ev_default_loop(0);
  accept_connections();

  // 
  // while(1) {
  //   new_connection_fd = accept_connection();
  //   err("new_connection_fd = %d", new_connection_fd);
  // 
  //   if (!fork()) {
  //     struct timeval* start_time = stopwatch_time();
  // 
  //     close(server_socket_fd);
  // 
  //     handle_request(new_connection_fd);
  // 
  //     free_configuration();
  // 
  //     stopwatch_stop(start_time);
  //     exit(0);
  //   }
  // 
  //   close(new_connection_fd);
  // }

  return 0;
}
