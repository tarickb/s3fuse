noinst_PROGRAMS = \
	passwords \
	tests

passwords_SOURCES = passwords.cc
passwords_LDADD = ../libs3fuse_crypto.a $(LDADD)

tests_SOURCES = \
	aes_cbc_256_kat.cc \
	aes_cbc_256_random.cc \
	aes_ctr_256_kat.cc \
	aes_ctr_256_random_par.cc \
	aes_ctr_256_random_seq.cc \
	encoders.cc \
	md5_kat.cc \
	md5_random.cc \
	pbkdf2_sha1_kat.cc \
	random.h \
	sha256_kat.cc \
	symmetric_key.cc

tests_LDADD = ../libs3fuse_crypto.a -lgtest -lgtest_main $(LDADD)
