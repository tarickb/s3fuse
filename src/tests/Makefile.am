bin_PROGRAMS = base64 hex issue_4 md5

AM_CPPFLAGS += -I../

base64_SOURCES = base64.cc ../util.h
base64_LDADD = ../util.o $(LDADD)

hex_SOURCES = hex.cc ../util.h
hex_LDADD = ../util.o $(LDADD)

issue_4_SOURCES = issue_4.cc

md5_SOURCES = md5.cc ../md5.h ../util.h
md5_LDADD = ../md5.o $(LDADD)
