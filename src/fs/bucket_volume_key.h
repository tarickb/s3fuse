#ifndef S3_FS_BUCKET_VOLUME_KEY_H
#define S3_FS_BUCKET_VOLUME_KEY_H

namespace s3
{
  namespace fs
  {
    class bucket_volume_key
    {
    public:
      static bool is_present();

      static boost::shared_ptr<crypto::buffer> read(const boost::shared_ptr<crypto::buffer> &key);

      static void generate(const boost::shared_ptr<crypto::buffer> &key);
      static void reencrypt(const boost::shared_ptr<crypto::buffer> &old_key, const boost::shared_ptr<crypto::buffer> &new_key);
      static void remove();
    };
  }
}

#endif
