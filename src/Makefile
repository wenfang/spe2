OBJS = spe.o spe_module.o spe_opt.o spe_io.o spe_log.o spe_util.o spe_epoll.o spe_task.o spe_rbtree.o spe_sock.o spe_signal.o spe_server.o spe_shm.o spe_conn.o spe_test.o spe_cycle.o spe_worker.o spe_process.o spe_buf.o cJSON.o spe_map.o

OBJS_BUF = spe_buf.o spe_buf_test.o
OBJS_IO = spe_buf.o spe_io.o spe_io_test.o
OBJS_OPT = spe_buf.o spe_io.o spe_opt.o spe_opt_test.o
OBJS_MAP = spe_map.o spe_util.o spe_map_test.o

.c.o:
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -pthread -Wall -c $< -o $@

all:$(OBJS)
	gcc -pg -g $(OBJS) -lm -lpthread -o spe

buf: $(OBJS_BUF)
	gcc $(OBJS_BUF) -o spe_buf

io: $(OBJS_IO)
	gcc $(OBJS_IO) -o spe_io

opt: $(OBJS_OPT)
	gcc $(OBJS_OPT) -o spe_opt

map: $(OBJS_MAP)
	gcc $(OBJS_MAP) -o spe_map

clean:
	rm -rf *.o
	rm -rf spe spe_buf spe_io spe_opt spe_map

.PHONY: all buf io opt clean map
