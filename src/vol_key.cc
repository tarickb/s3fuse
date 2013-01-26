#include <getopt.h>

#include <fstream>
#include <iostream>

#include "base/config.h"
#include "base/logger.h"
#include "base/xml.h"
#include "crypto/buffer.h"
#include "crypto/passwords.h"
#include "crypto/private_file.h"
#include "fs/bucket_volume_key.h"
#include "fs/encryption.h"
#include "services/service.h"

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::ifstream;
using std::ofstream;
using std::runtime_error;
using std::string;

using s3::base::config;
using s3::base::logger;
using s3::base::xml;
using s3::crypto::buffer;
using s3::crypto::passwords;
using s3::crypto::private_file;
using s3::fs::bucket_volume_key;
using s3::fs::encryption;
using s3::services::service;

namespace
{
  const char *SHORT_OPTIONS = ":c:i:o:";

  const option LONG_OPTIONS[] = {
    { "config-file", required_argument, NULL, 'c'  },
    { "in-key",      required_argument, NULL, 'i'  },
    { "out-key",     required_argument, NULL, 'o'  },
    { NULL,          NULL,              NULL, NULL }};
}

void init(const string &config_file)
{
  logger::init(LOG_ERR);
  config::init(config_file);
  service::init(config::get_service());
  xml::init(service::get_xml_namespace());

  if (!config::get_use_encryption())
    throw runtime_error("encryption not enabled in config file");
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
    "You are going to delete the volume encryption key for bucket:\n"
    "  " << config::get_bucket_name() << "\n"
    "\n"
    "To confirm, enter the name of the bucket (case sensitive): ";

  getline(cin, line);

  if (line != config::get_bucket_name())
    throw runtime_error("aborted");

  cout <<
    "\n"
    "*******************************************************************\n"
    "* WARNING                                                         *\n"
    "* -------                                                         *\n"
    "*                                                                 *\n"
    "* What you are about to do will render inaccessible all encrypted *\n"
    "* objects in this bucket.  This operation is irreversible.        *\n"
    "*******************************************************************\n"
    "\n"
    "Do you understand that existing encrypted files will be lost forever? Type \"yes\": ";

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

buffer::ptr prompt_for_current_password()
{
  string current_password;

  current_password = passwords::read_from_stdin("Enter current bucket password: ");

  if (current_password.empty())
    throw runtime_error("current password not specified");

  return encryption::derive_key_from_password(current_password);
}

buffer::ptr prompt_for_new_password()
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

buffer::ptr read_key_from_file(const string &file)
{
  ifstream f;
  string line;

  cout << "Reading key from [" << file << "]..." << endl;
  private_file::open(file, &f);
  getline(f, line);

  return buffer::from_string(line);
}

buffer::ptr generate_and_write(const string &file)
{
  ofstream f;
  buffer::ptr key;

  key = buffer::generate(bucket_volume_key::key_cipher::DEFAULT_KEY_LEN);

  cout << "Writing key to [" << file << "]..." << endl;
  private_file::open(file, &f);

  f << key->to_string() << endl;

  return key;
}

void generate_new_key(const string &config_file, const string &out_key_file)
{
  buffer::ptr password_key;

  init(config_file);

  if (bucket_volume_key::is_present())
    throw runtime_error("bucket already contains a volume key");

  cout <<
    "This bucket does not currently have an encryption key. We'll create one.\n"
    "\n";

  if (out_key_file.empty())
    password_key = prompt_for_new_password();
  else
    password_key = generate_and_write(out_key_file);

  cout << "Generating volume key..." << endl;
  bucket_volume_key::generate(password_key);

  cout << "Done." << endl;
}

void reencrypt_key(const string &config_file, const string &in_key_file, const string &out_key_file)
{
  buffer::ptr current_key, new_key;

  init(config_file);

  if (!bucket_volume_key::is_present())
    throw runtime_error("bucket does not contain a volume key");

  cout << 
    "Bucket already contains an encryption key.\n"
    "\n"
    "If you've forgotten the password for this bucket, or lost the local password\n"
    "key, use the \"delete\" option to delete the volume key (and permanently lose\n"
    "any files that are currently encrypted).\n"
    "\n";

  if (in_key_file.empty())
    current_key = prompt_for_current_password();
  else
    current_key = read_key_from_file(in_key_file);

  if (out_key_file.empty())
    new_key = prompt_for_new_password();
  else
    new_key = generate_and_write(out_key_file);

  cout << "Changing key..." << endl;
  bucket_volume_key::reencrypt(current_key, new_key);

  cout << "Done." << endl;
}

void delete_key(const string &config_file)
{
  init(config_file);

  if (!bucket_volume_key::is_present())
    throw runtime_error("bucket does not contain a volume key");

  confirm_key_delete();

  cout << "Deleting key..." << endl;
  bucket_volume_key::remove();

  cout << "Done." << endl;
}

void print_usage(const char *arg0)
{
  const char *base_name = strrchr(arg0, '/');

  base_name = base_name ? base_name + 1 : arg0;

  cerr << 
    "Usage: " << base_name << " <command> [options]\n"
    "\n"
    "Where <command> is one of:\n"
    "\n"
    "  generate                  Generate a new volume key and write it to the bucket.\n"
    "  change                    Change the password or key file used to access the\n"
    "                            volume key stored in the bucket.\n"
    "  delete                    Erase the bucket volume key.\n"
    "\n"
    "[options] can be:\n"
    "\n"
    "  -c, --config-file <path>  Use configuration at <path> rather than the default.\n"
    "  -i, --in-key <path>       Use key at <path> rather than prompting for the current\n"
    "                            volume password (only valid with \"change\").\n"
    "  -o, --out-key <path>      Store key at <path> rather than prompting for a new\n"
    "                            volume password.  The operation will fail if a file\n"
    "                            exists at this path (only valid with \"generate\" and\n"
    "                            \"change\").\n"
    "\n"
    "See " << base_name << "(1) for examples and a more detailed explanation." << endl;

  exit(1);
}

int main(int argc, char **argv)
{
  int opt = 0;
  string config_file, in_key_file, out_key_file;
  string command;

  while ((opt = getopt_long(argc, argv, SHORT_OPTIONS, LONG_OPTIONS, NULL)) != -1) {
    switch (opt) {
      case 'c':
        config_file = optarg;
        break;

      case 'i':
        in_key_file = optarg;
        break;

      case 'o':
        out_key_file = optarg;
        break;

      default:
        print_usage(argv[0]);
    }
  }

  if (optind < argc)
    command = argv[optind];

  try {
    if (command == "generate") {
      if (!in_key_file.empty())
        print_usage(argv[0]);

      generate_new_key(config_file, out_key_file);

    } else if (command == "change") {
      reencrypt_key(config_file, in_key_file, out_key_file);

    } else if (command == "delete") {
      if (!in_key_file.empty() || !out_key_file.empty())
        print_usage(argv[0]);

      delete_key(config_file);

    } else {
      print_usage(argv[0]);
    }

    return 0;

  } catch (const std::exception &e) {
    cout << "Caught exception: " << e.what() << endl;
  }

  return 1;
}
