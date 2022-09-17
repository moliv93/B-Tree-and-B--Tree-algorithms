PROJ_NAME=BTREE

CC=g++

CC_FLAGS = -std=c++17 -Iglad/include -ldl -lGL -lglfw

BSTAR_FILE = BStar_visual.cpp
BSTAR_TEXT_FILE = BStar_text_mode.cpp

BTREE_FILE = BTree_visual_key_duplication.cpp
BTREE_SIMPLE_FILE = BTree_visual_normal.cpp

bstar:
	$(CC) $(BSTAR_FILE) glad/src/glad.c $(CC_FLAGS) -o BTREE
	./BTREE

bstar_text:
	$(CC) $(BSTAR_TEXT_FILE) -o BTREE
	./BTREE

btree_key_repeat:
	$(CC) $(BTREE_FILE) glad/src/glad.c $(CC_FLAGS) -o BTREE
	./BTREE

btree:
	$(CC) $(BTREE_SIMPLE_FILE) glad/src/glad.c $(CC_FLAGS) -o BTREE
	./BTREE
