#include<stdio.h>
#include"mongoose.h"
#include"server.h"
#include"room.h"

static const char *s_listen_on = "ws://localhost:8000";

static struct mg_connection *find_connection_by_id(
    struct mg_mgr *mgr,
    unsigned long client_id
  ) {
      struct mg_connection *connection = mgr->conns;

      while (connection != NULL){
        if (
          connection->id == client_id &&
          connection->is_websocket
        ){
          return connection;
        }
        connection = connection->next;
      }
      return NULL;
    }

static void handle_create_room(struct mg_connection *c){
  Room *room = create_room(c->id);

  if(room == NULL){
    mg_ws_printf(
      c,
      WEBSOCKET_OP_TEXT,
      "{\"type\":\"error\","
      "\"message\":\"gagal membuat room\"}"
    );
    return;
  }

  mg_ws_printf(
    c,
    WEBSOCKET_OP_TEXT,
    "{\"type\":\"created\","
    "\"code\":\"%s\"}",
    room->code
  );
}

static void handle_join_room(struct mg_connection *c, struct mg_ws_message *wm) {
  char *code = mg_json_get_str(
    wm->data,
    "$.code"
  );

  if (code == NULL){
      mg_ws_printf(
        c,
        WEBSOCKET_OP_TEXT,
        "{\"type\":\"error\","
          "\"message\":\"code wajib diisi\"}"
      );
    return;
  }

  Room *room = join_room(code, c->id);

  if (room == NULL){
    mg_ws_printf(
      c,
      WEBSOCKET_OP_TEXT,
      "{\"type\":\"error\","
        "\"message\":\"room tidak ditemukan atau penuh\"}"
    );

    mg_free(code);
    return;
  }

  mg_ws_printf(
    c,
    WEBSOCKET_OP_TEXT,
    "{\"type\":\"joined\","
    "\"code\":\"%s\"}",
    room->code
  );  

  struct mg_connection *creator =
    find_connection_by_id(
      c->mgr,
      room->client1_id
    );

  if (creator != NULL) {
    mg_ws_printf(
      creator,
      WEBSOCKET_OP_TEXT,
      "{\"type\":\"peer_joined\","
      "\"code\":\"%s\"}",
      room->code
    );  
  }

  mg_free(code);  

}

static void handle_disconnect(struct mg_connection *c){
  Room *room = find_room_by_client_id(c->id);

  if (room == NULL){
    return;
  }

  unsigned long peer_id;

  if (room->client1_id == c->id){
    peer_id = room->client2_id;
  } else {
    peer_id = room->client1_id;
  }

  if (peer_id != 0){
    struct mg_connection *peer = 
      find_connection_by_id(c->mgr, peer_id);
    
    if (peer != NULL) {
      mg_ws_printf(
          peer,
          WEBSOCKET_OP_TEXT,
          "{\"type\":\"peer_left\","
          "\"code\":\"%s\"}",
          room->code
      );
    }
  }

  remove_room(room);
}

static void fn(
  struct mg_connection *c,
  int ev,
  void *ev_data
) {
  if (ev == MG_EV_HTTP_MSG) {
      struct mg_http_message *hm = ev_data;

      if (
          mg_match(
              hm->uri,
              mg_str("/websocket"),
              NULL
          )
      ) {
          mg_ws_upgrade(c, hm, NULL);
      } else {
          mg_http_reply(
              c,
              404,
              "",
              "Not found\n"
          );
      }
  }  else if (ev == MG_EV_WS_MSG) {
    struct mg_ws_message *wm = ev_data;
    int opcode = wm->flags & 0x0F;

    if (opcode == WEBSOCKET_OP_TEXT) {
        char *type = mg_json_get_str(
            wm->data,
            "$.type"
        );

        if (type == NULL) {
            mg_ws_printf(
                c,
                WEBSOCKET_OP_TEXT,
                "{\"type\":\"error\","
                "\"message\":\"type wajib diisi\"}"
            );

            return;
        }

        if (strcmp(type, "create_room") == 0) {
            handle_create_room(c);
        } else if (strcmp(type, "join_room") == 0) {
          handle_join_room(c, wm);
        } else {
            mg_ws_printf(
                c,
                WEBSOCKET_OP_TEXT,
                "{\"type\":\"error\","
                "\"message\":\"request tidak dikenal\"}"
            );
        }
        mg_free(type);
    } else if (opcode == WEBSOCKET_OP_BINARY){
      Room *room = find_room_by_client_id(c->id);

      if (room == NULL){
        mg_ws_printf(
          c,
          WEBSOCKET_OP_TEXT,
          "{\"type\":\"error\","
          "\"message\":\"client belum masuk room\"}"
      );
      return;
      }

      unsigned long peer_id;

      if (room->client1_id == c->id){
        peer_id = room->client2_id;
      } else {
        peer_id = room->client1_id;
      }

      if (peer_id == 0){
        mg_ws_printf(
          c,
          WEBSOCKET_OP_TEXT,
          "{\"type\":\"error\","
          "\"message\":\"lawan bicara belum join\"}"
      );
      return;
      }

      struct mg_connection *peer = 
        find_connection_by_id(c->mgr, peer_id);

      if (peer == NULL) {
        mg_ws_printf(
          c,
          WEBSOCKET_OP_TEXT,
          "{\"type\":\"error\","
          "\"message\":\"koneksi lawan tidak ditemukan\"}"
      );
      return;
      } 
      mg_ws_send(
        peer,
        wm->data.buf,
        wm->data.len,
        WEBSOCKET_OP_BINARY
      );
    } 
  } else if (ev == MG_EV_CLOSE) {
    handle_disconnect(c);
  }
}


int start_server(void)
{
    struct mg_mgr mgr;

    mg_mgr_init(&mgr);

    if (
        mg_http_listen(
            &mgr,
            s_listen_on,
            fn,
            NULL
        ) == NULL
    ) {
        fprintf(stderr, "Gagal menjalankan server\n");
        mg_mgr_free(&mgr);
        return 1;
    }

    printf(
        "Server jalan di "
        "ws://localhost:8000/websocket\n"
    );

    for (;;) {
        mg_mgr_poll(&mgr, 100);
    }

    mg_mgr_free(&mgr);
    return 0;
}