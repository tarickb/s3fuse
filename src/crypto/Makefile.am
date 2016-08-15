SUBDIRS = .
DIST_SUBDIRS = $(SUBDIRS) tests

if BUILD_TESTS
SUBDIRS += tests
endif

noinst_LTLIBRARIES = libs3fuse_crypto.a

libs3fuse_crypto_a_SOURCES = \
	aes_cbc_256.cc \
	aes_cbc_256.h \
	aes_ctr_256.cc \
	aes_ctr_256.h \
	base64.cc \
	base64.h \
	buffer.h \
	cipher.h \
	encoder.h \
	hash.h \
	hash_list.h \
	hex.cc \
	hex.h \
	hex_with_quotes.h \
	hmac_sha1.cc \
	hmac_sha1.h \
	md5.cc \
	md5.h \
	passwords.cc \
	passwords.h \
	pbkdf2_sha1.cc \
	pbkdf2_sha1.h \
	private_file.cc \
	private_file.h \
	sha256.cc \
	sha256.h \
	symmetric_key.h
