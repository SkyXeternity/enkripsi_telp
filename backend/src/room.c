#include<stdio.h>
#include<string.h>

#include"room.h"

static Room rooms[MAX_ROOMS];

Room *create_room(unsigned long client_id){
  for (int i = 0; i < MAX_ROOMS; i++){
    if (!rooms[i].active) {
      memset(&rooms[i], 0, sizeof(Room));

      snprintf(
        rooms[i].code,
        sizeof(rooms[i].code),
          "%06d",
          i + 1
        );

        rooms[i].client1_id = client_id;
        rooms[i].active = 1;

        return &rooms[i];
    }
  }
  return NULL;
}

Room *find_room_by_code(const char *code){
  if (code == NULL){
    return NULL;
  }
  
  for (int i = 0; i < MAX_ROOMS; i++){
    if (
      rooms[i].active && 
      strcmp(rooms[i].code, code) == 0 
    ) {
      return &rooms[i];
    }
  }
  return NULL;
}

Room *find_room_by_client_id(unsigned long client_id){
  for (int i =0; i < MAX_ROOMS; i++ ){
    if (
      rooms[i].active && (
        rooms[i].client1_id == client_id ||
        rooms[i].client2_id == client_id 
      ) 
    ){
      return &rooms[i];
    }
  }
  return NULL;
}

Room *join_room(const char *code, unsigned long client_id){
  Room *rooms = find_room_by_code(code);
  if (rooms == NULL){
    return NULL;
  }

  if (rooms->client1_id == client_id){
    return NULL;
  }

  if (rooms->client2_id != 0){
    return NULL;
  }

  rooms->client2_id = client_id;

  return rooms;
}

void remove_room(Room *room){
  if (room == NULL){
    return;
  }
  memset(room, 0, sizeof(*room));
}
