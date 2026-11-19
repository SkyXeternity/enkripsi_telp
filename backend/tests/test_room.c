#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "room.h"

static void test_create_and_find_room(void)
{
    unsigned long creator_id = 1001;
    Room *room = create_room(creator_id);

    assert(room != NULL);
    assert(room->client1_id == creator_id);
    assert(room->client2_id == 0);
    assert(room->active == 1);
    assert(strlen(room->code) == ROOM_CODE_LENGTH);

    Room *found = find_room_by_code(room->code);
    assert(found == room);

    Room *not_found = find_room_by_code("999999");
    assert(not_found == NULL);

    printf("[OK] create dan find room\n");
}

static void test_join_room(void)
{
    unsigned long creator_id = 2001;
    unsigned long joiner_id = 2002;
    Room *room = create_room(creator_id);

    assert(room != NULL);

    Room *joined = join_room(room->code, joiner_id);
    assert(joined == room);
    assert(room->client2_id == joiner_id);

    assert(join_room(room->code, creator_id) == NULL);
    assert(join_room(room->code, 2003) == NULL);
    assert(join_room("999999", 2004) == NULL);

    assert(find_room_by_client_id(creator_id) == room);
    assert(find_room_by_client_id(joiner_id) == room);
    assert(find_room_by_client_id(9999) == NULL);

    printf("[OK] join room dan penolakan client ketiga\n");
}

static void test_remove_room(void)
{
    Room *room = create_room(3001);
    char old_code[ROOM_CODE_LENGTH + 1];

    assert(room != NULL);
    strcpy(old_code, room->code);

    remove_room(room);

    assert(room->active == 0);
    assert(room->client1_id == 0);
    assert(room->client2_id == 0);
    assert(room->code[0] == '\0');
    assert(find_room_by_code(old_code) == NULL);

    remove_room(NULL);

    printf("[OK] remove room\n");
}

int main(void)
{
    test_create_and_find_room();
    test_join_room();
    test_remove_room();

    printf("Semua tes room berhasil\n");
    return 0;
}
