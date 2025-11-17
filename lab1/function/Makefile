CC = gcc

CFLAGS = -Wall -fPIC

LIBDIR = ./sublib
SRCS= $(LIBDIR)/add.c $(LIBDIR)/subtract.c $(LIBDIR)/multiply.c $(LIBDIR)/divide.c
OBJS = $(SRCS:.c=.c)

TARGET = lab
LIBNAME= libl1.so

all : $(LIBDIR)/$(LIBNAME) $(TARGET)

$(LIBDIR)/$(LIBNAME): $(OBJS)
	$(CC) -shared -o $@ $^

$(TARGET): main.c $(LIBDIR)/$(LIBNAME)
	$(CC) main.c -L$(LIBDIR) -ll1 -Wl,-rpath,'$$ORIGIN/$(LIBDIR)' -o $@

clean:
	rm -f $(LIBDIR)/*.o$(LIBDIR)/$(LIBNAME) $(TARGET)
