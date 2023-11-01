CFLAGS = -g3 -Wall -Wextra -Wconversion -Wcast-qual -Wcast-align -g
CFLAGS += -Winline -Wfloat-equal -Wnested-externs
CFLAGS += -pedantic -std=gnu99 -Werror -D_GNU_SOURCE
CC = gcc
EXECS = 33sh 33noprompt
SOURCE = sh.c
PROMPT = -DPROMPT

all: $(EXECS)

33sh: $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) $(PROMPT) -o $@
33noprompt: $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $@
clean:
	rm -f $(EXECS)

