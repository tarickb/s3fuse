#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "config.h"
#include "gs_authenticator.h"
#include "logger.h"
#include "request.h"
#include "util.h"

using namespace boost::property_tree;
using namespace std;

using namespace s3;

namespace
{
  const string GS_EP_TOKEN = "https://accounts.google.com/o/oauth2/token";
  const string GS_CLIENT_ID = "45323348671.apps.googleusercontent.com";
  const string GS_CLIENT_SECRET = "vuN7FOnK1elQtqze_R9dE3tM";
  const string GS_SCOPE = "https://www.googleapis.com/auth/devstorage.read_write";

  /*
  const string AMZ_HEADER_PREFIX = "x-amz-";
  const string EMPTY = "";

  const string & safe_find(const header_map &map, const char *key)
  {
    header_map::const_iterator itor = map.find(key);

    return (itor == map.end()) ? EMPTY : itor->second;
  }
  */
}

const string & gs_authenticator::get_client_id()
{
  return GS_CLIENT_ID;
}

const string & gs_authenticator::get_scope()
{
  return GS_SCOPE;
}

void gs_authenticator::get_tokens(const string &code, string *access_token, string *refresh_token, time_t *expiry)
{
  request req;
  string data;
  stringstream ss;
  ptree tree;

  data = 
    "client_id=" + GS_CLIENT_ID + "&"
    "client_secret=" + GS_CLIENT_SECRET + "&"
    "code=" + code + "&"
    "redirect_uri=urn:ietf:wg:oauth:2.0:oob&"
    "grant_type=authorization_code";

  req.init(HTTP_POST);
  req.set_full_url(GS_EP_TOKEN);
  req.set_input_data(data);
  // req.set_header("Content-Type", "application/x-www-form-urlencoded");

  req.run();

  if (req.get_response_code() != 200) {
    S3_LOG(LOG_CRIT, "gs_authenticator::get_tokens", "token endpoint returned %i.\n", req.get_response_code());
    throw runtime_error("failed to get tokens.");
  }

  ss << req.get_response_data();
  read_json(ss, tree);

  *access_token = tree.get<string>("access_token");
  *refresh_token = tree.get<string>("refresh_token");
  *expiry = time(NULL) + tree.get<int>("expires_in");
}

gs_authenticator::gs_authenticator()
{
  
}

void gs_authenticator::sign(request *req)
{
  req->set_header("Authorization", "");
}
