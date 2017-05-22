# define the C compiler to use
CC = gcc

# define any compile-time flags
CFLAGS = -Wall -lpthread -lwiringPi -lm -I. 

#CFLAGS = -Wall -g -I./wiringPI

# redefine for windows build with raspbain keychain.
# http://gnutoolchains.com/raspberry/
ifeq ($(OS),Windows_NT)
  CFLAGS = -Wall -g -lwiringPi -lpthread -I./win-fakelib -L./win-fakelib -lm
  CC = arm-linux-gnueabihf-gcc
  RM = del
endif


# define the C source files
SRCS = gpioctrld.c utils.c config.c httpd.c w1.c lpd8806led.c lpd8806worker.c sht31.c
SRCS_U = gpioctrl.c utils.c config.c lpd8806led.c

# define the C object files 
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:.c=.o)
OBJS_U = $(SRCS_U:.c=.o)

# define the executable file 
MAIN = gpioctrld
MAIN_U = gpioctrl

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean

all:    $(MAIN) $(MAIN_U)
  @echo: $(MAIN) $(MAIN_U) have been compiled

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

$(MAIN_U): $(OBJS_U) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN_U) $(OBJS_U) $(LFLAGS) $(LIBS)
  
# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN) $(MAIN_U)

depend: $(SRCS)
	makedepend $(INCLUDES) $^