LUA_DIR = ../support/lua

#OBJS = spe.o spe_module.o spe_opt.o spe_io.o spe_log.o spe_util.o spe_epoll.o spe_task.o spe_rbtree.o spe_sock.o spe_signal.o spe_server.o spe_shm.o spe_conn.o spe_test.o spe_cycle.o spe_worker.o spe_buf.o cJSON.o spe_map.o spe_rpc.o spe_lua.o

OBJS = spe_module.o spe_log.o spe_util.o spe_task.o spe_rbtree.o spe_buf.o spe_cycle.o spe_opt.o spe_io.o spe_signal.o spe_epoll.o spe_sock.o spe_conn.o spe_shm.o spe_server.o spe_worker.o spe.o

OBJS_BUF = spe_buf.o spe_buf_test.o
OBJS_IO = spe_buf.o spe_io.o spe_io_test.o
OBJS_OPT = spe_buf.o spe_io.o spe_opt.o spe_opt_test.o
OBJS_MAP = spe_map.o spe_util.o spe_map_test.o

.c.o:
	gcc -DDEBUG -g -fPIC -std=gnu99 -I$(LUA_DIR)/src -I/usr/local/include -pthread -Wall -c $< -o $@

all:$(OBJS) $(LUA_DIR)/src/liblua.a
	gcc -g $(OBJS) $(LUA_DIR)/src/liblua.a -ldl -lm -lpthread -o spe

$(LUA_DIR)/src/liblua.a:
	make -C $(LUA_DIR) linux

buf: $(OBJS_BUF)
	gcc $(OBJS_BUF) -o spe_buf

io: $(OBJS_IO)
	gcc $(OBJS_IO) -o spe_io

opt: $(OBJS_OPT)
	gcc $(OBJS_OPT) -o spe_opt

map: $(OBJS_MAP)
	gcc $(OBJS_MAP) -o spe_map

clean:
	make -C $(LUA_DIR) clean
	rm -rf *.o
	rm -rf spe spe_buf spe_io spe_opt spe_map

.PHONY: all buf io opt clean map
