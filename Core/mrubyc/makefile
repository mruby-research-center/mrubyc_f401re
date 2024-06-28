MRBC = cmd /C mrbc.exe
RBSRCS = $(wildcard *.rb)
CSRCS = $(RBSRCS:.rb=.c)

.PHONY : all
all:	$(CSRCS)

%.c : %.rb
	$(MRBC) -B$(@:.c=) $^
