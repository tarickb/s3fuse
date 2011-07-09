#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "config.h"
#include "gs_authenticator.h"
#include "logger.h"
#include "request.h"
#include "util.h"

using namespace boost;
using namespace boost::property_tree;
using namespace std;

using namespace s3;

namespace
{
  const string GS_URL_PREFIX = "https://commondatastorage.googleapis.com";
  const string GS_XML_NAMESPACE = "http://doc.s3.amazonaws.com/2006-03-01"; // "http://doc.commondatastorage.googleapis.com/2010-04-03";
  const string GS_EP_TOKEN = "https://accounts.google.com/o/oauth2/token";
  const string GS_CLIENT_ID = "45323348671.apps.googleusercontent.com";
  const string GS_CLIENT_SECRET = "vuN7FOnK1elQtqze_R9dE3tM";
  const string GS_OAUTH_SCOPE = "https://www.googleapis.com/auth/devstorage.read_write";
}

const string & gs_authenticator::get_client_id()
{
  return GS_CLIENT_ID;
}

const string & gs_authenticator::get_oauth_scope()
{
  return GS_OAUTH_SCOPE;
}

void gs_authenticator::get_tokens(get_tokens_mode mode, const string &key, string *access_token, string *refresh_token, time_t *expiry)
{
  request req;
  string data;
  stringstream ss;
  ptree tree;

  data =
    "client_id=" + GS_CLIENT_ID + "&"
    "client_secret=" + GS_CLIENT_SECRET + "&";

  if (mode == GT_AUTH_CODE)
    data += 
      "code=" + key + "&"
      "redirect_uri=urn:ietf:wg:oauth:2.0:oob&"
      "grant_type=authorization_code";
  else if (mode == GT_REFRESH)
    data +=
      "refresh_token=" + key + "&"
      "grant_type=refresh_token";
  else
    throw runtime_error("unrecognized get_tokens mode.");

  req.init(HTTP_POST);
  req.set_full_url(GS_EP_TOKEN);
  req.set_input_data(data);

  req.run();

  if (req.get_response_code() != 200) {
    S3_LOG(LOG_CRIT, "gs_authenticator::get_tokens", "token endpoint returned %i.\n", req.get_response_code());
    throw runtime_error("failed to get tokens.");
  }

  ss << req.get_response_data();
  read_json(ss, tree);

  if (mode == GT_AUTH_CODE)
    *refresh_token = tree.get<string>("refresh_token");

  *access_token = tree.get<string>("access_token");
  *expiry = time(NULL) + tree.get<int>("expires_in");
}

gs_authenticator::gs_authenticator()
  : _expiry(0)
{
  mutex::scoped_lock lock(_mutex);
  ifstream f(config::get_auth_data().c_str(), ios_base::in);

  // TODO: check permissions on token file

  if (!f.good())
    throw runtime_error("failed to open token file.");

  getline(f, _refresh_token);

  refresh(lock);
}

const string & gs_authenticator::get_url_prefix()
{
  return GS_URL_PREFIX;
}

const string & gs_authenticator::get_xml_namespace()
{
  return GS_XML_NAMESPACE;
}

void gs_authenticator::sign(request *req)
{
  mutex::scoped_lock lock(_mutex);

  if (time(NULL) >= _expiry)
    refresh(lock);

  req->set_header("Authorization", _access_token);
}

void gs_authenticator::refresh(const mutex::scoped_lock &lock)
{
  string new_refresh;

  gs_authenticator::get_tokens(
    GT_REFRESH, 
    _refresh_token, 
    &_access_token,
    &new_refresh,
    &_expiry);

  S3_LOG(
    LOG_DEBUG, 
    "gs_authenticator::refresh", 
    "using refresh [%s], got refresh [%s] and access [%s].\n",
    _refresh_token.c_str(),
    new_refresh.c_str(),
    _access_token.c_str());

  if (new_refresh != _refresh_token) {
    // TODO: write to disk?
  }

  _refresh_token = new_refresh;
  _access_token = "OAuth " + _access_token;
}
