#ifndef ROOM_H
#define ROOM_H

#define MAX_ROOMS 100
#define ROOM_CODE_LENGTH 6

typedef struct {
    char code[ROOM_CODE_LENGTH + 1];
    unsigned long client1_id;
    unsigned long client2_id;
    int active;
} Room;

Room *create_room(unsigned long client_id);
Room *find_room_by_code(const char *code);
Room *find_room_by_client_id( unsigned long client_id);
Room *join_room(const char *code, unsigned long client_id);
void remove_room(Room *room);

#endif