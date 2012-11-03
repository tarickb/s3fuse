#ifndef S3_OBJECTS_METADATA_H
#define S3_OBJECTS_METADATA_H

namespace s3
{
  namespace objects
  {
    class metadata
    {
    public:
      static const char *RESERVED_PREFIX;
      static const char *XATTR_PREFIX;

      static const char *LAST_UPDATE_ETAG;
      static const char *MODE;
      static const char *UID;
      static const char *GID;
      static const char *LAST_MODIFIED_TIME;

      static const char *SHA256;
      static const char *ENC_IV;
      static const char *ENC_METADATA;
    };
  }
}

#endif
