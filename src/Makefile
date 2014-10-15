#OBJS = spe.o spe_module.o spe_opt.o spe_io.o spe_log.o spe_string.o spe_util.o spe_epoll.o spe_task.o spe_rbtree.o spe_sock.o spe_signal.o spe_server.o spe_shm.o spe_conn.o spe_test.o spe_cycle.o spe_worker.o spe_process.o spe_buf.o
OBJS = spe_io.o spe_buf.o

.c.o:
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -pthread -Wall -c $< -o $@

all:$(OBJS)
	gcc $(OBJS) -lpthread -o spe

clean:
	rm -rf $(OBJS)
	rm -rf spe

.PHONY: all clean
