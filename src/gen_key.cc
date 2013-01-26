#include <getopt.h>

#include <iostream>

#include "base/config.h"
#include "base/logger.h"
#include "base/xml.h"
#include "crypto/buffer.h"
#include "crypto/passwords.h"
#include "fs/bucket_volume_key.h"
#include "fs/encryption.h"
#include "services/service.h"

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::runtime_error;
using std::string;

using s3::base::config;
using s3::base::logger;
using s3::base::xml;
using s3::crypto::buffer;
using s3::crypto::passwords;
using s3::fs::bucket_volume_key;
using s3::fs::encryption;
using s3::services::service;

namespace
{
  // TODO: parameter to force larger key?
  // TODO: parameters for files

  const char *SHORT_OPTIONS = ":c:f";

  const option LONG_OPTIONS[] = {
    { "config-file", required_argument, NULL, 'c'  },
    { "delete-key",  no_argument,       NULL, 'r'  },
    { NULL,          NULL,              NULL, NULL }};
}

string to_lower(string in)
{
  string out(in);

  for (string::iterator itor = out.begin(); itor != out.end(); ++itor)
    *itor = tolower(*itor);

  return out;
}

void confirm_key_delete()
{
  string line;

  cout <<
    "*******************************************************************\n"
    "* WARNING                                                         *\n"
    "* -------                                                         *\n"
    "*                                                                 *\n"
    "* What you are about to do will render inaccessible all encrypted *\n"
    "* objects in this bucket.  This operation is irreversible.        *\n"
    "*******************************************************************\n"
    "\n"
    "You are going to delete the volume encryption key for bucket:\n"
    "  " << config::get_bucket_name() << "\n"
    "\n"
    "To confirm, enter the name of the bucket (case sensitive): ";

  getline(cin, line);

  if (line != config::get_bucket_name())
    throw runtime_error("aborted");

  cout << "Do you understand that existing encrypted files will be lost forever? Type \"yes\": ";

  getline(cin, line);
  line = to_lower(line);

  if (line != "yes")
    throw runtime_error("aborted");

  cout << "Do you understand that this operation cannot be undone? Type \"yes\": ";

  getline(cin, line);
  line = to_lower(line);

  if (line != "yes")
    throw runtime_error("aborted");
}

buffer::ptr get_current_password_key()
{
  string current_password;

  current_password = passwords::read_from_stdin("Enter current bucket password: ");

  if (current_password.empty())
    throw runtime_error("current password not specified");

  return encryption::derive_key_from_password(current_password);
}

buffer::ptr get_new_password_key()
{
  string new_password, confirm;

  new_password = passwords::read_from_stdin("Enter new bucket password: ");

  if (new_password.empty())
    throw runtime_error("password cannot be empty");

  confirm = passwords::read_from_stdin("Confirm new bucket password: ");

  if (confirm != new_password)
    throw runtime_error("passwords do not match");

  return encryption::derive_key_from_password(new_password);
}

void print_usage(const char *arg0)
{
  const char *base_name = strrchr(arg0, '/');

  base_name = base_name ? base_name + 1 : arg0;

  cerr << "Usage: " << base_name << " [--config-file <path>] [--delete-key]" << endl;

  exit(1);
}

int main(int argc, char **argv)
{
  int opt;
  string config_file;
  bool delete_key = false;

  while ((opt = getopt_long(argc, argv, SHORT_OPTIONS, LONG_OPTIONS, NULL)) != -1) {
    switch (opt) {
      case 'c':
        config_file = optarg;
        break;

      case 'r':
        delete_key = true;
        break;

      default:
        print_usage(argv[0]);
    }
  }

  try {
    logger::init(LOG_ERR);
    config::init(config_file);
    service::init(config::get_service());
    xml::init(service::get_xml_namespace());

    if (!config::get_use_encryption())
      throw runtime_error("encryption not enabled in config file");

    if (delete_key) {
      if (!bucket_volume_key::is_present())
        throw runtime_error("bucket does not contain a volume key");

      confirm_key_delete();

      cout << "Deleting key..." << endl;
      bucket_volume_key::remove();

      cout << "Done." << endl;
      return 0;
    }

    if (bucket_volume_key::is_present()) {
      buffer::ptr current_password_key, new_password_key;

      cout << 
        "Bucket already contains an encryption key.\n"
        "\n"
        "If you've forgotten the password for this bucket, or lost the local password\n"
        "key, pass \"--delete-key\" to delete the volume key (and permanently lose any\n"
        "files that are currently encrypted).\n"
        "\n";

      current_password_key = get_current_password_key();
      new_password_key = get_new_password_key();

      cout << "Changing key..." << endl;
      bucket_volume_key::reencrypt(current_password_key, new_password_key);

      cout << "Done." << endl;
      return 0;

    } else {
      buffer::ptr new_password_key;

      cout <<
        "This bucket does not currently have an encryption key. We'll create one.\n"
        "\n";

      new_password_key = get_new_password_key();

      cout << "Generating volume key..." << endl;
      bucket_volume_key::write(new_password_key);

      cout << "Done." << endl;
      return 0;
    }

  } catch (const std::exception &e) {
    cout << "Caught exception: " << e.what() << endl;
  }

  return 1;
}
