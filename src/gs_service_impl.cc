#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "config.h"
#include "gs_service_impl.h"
#include "logger.h"
#include "request.h"
#include "util.h"

using namespace boost;
using namespace boost::property_tree;
using namespace std;

using namespace s3;

namespace
{
  const string GS_HEADER_PREFIX = "x-goog-";
  const string GS_URL_PREFIX = "https://commondatastorage.googleapis.com";
  const string GS_XML_NAMESPACE = "http://doc.s3.amazonaws.com/2006-03-01"; // "http://doc.commondatastorage.googleapis.com/2010-04-03";

  const string GS_EP_TOKEN = "https://accounts.google.com/o/oauth2/token";
  const string GS_OAUTH_SCOPE = "https://www.googleapis.com/auth/devstorage.read_write";

  const string GS_CLIENT_ID = "45323348671.apps.googleusercontent.com";
  const string GS_CLIENT_SECRET = "vuN7FOnK1elQtqze_R9dE3tM";
}

const string & gs_service_impl::get_client_id()
{
  return GS_CLIENT_ID;
}

const string & gs_service_impl::get_oauth_scope()
{
  return GS_OAUTH_SCOPE;
}

void gs_service_impl::get_tokens(get_tokens_mode mode, const string &key, string *access_token, time_t *expiry, string *refresh_token)
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
    S3_LOG(LOG_CRIT, "gs_service_impl::get_tokens", "token endpoint returned %i.\n", req.get_response_code());
    throw runtime_error("failed to get tokens.");
  }

  ss << req.get_response_data();
  read_json(ss, tree);

  *access_token = tree.get<string>("access_token");
  *expiry = time(NULL) + tree.get<int>("expires_in");

  if (mode == GT_AUTH_CODE)
    *refresh_token = tree.get<string>("refresh_token");
}

gs_service_impl::gs_service_impl()
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

const string & gs_service_impl::get_header_prefix()
{
  return GS_HEADER_PREFIX;
}

const string & gs_service_impl::get_url_prefix()
{
  return GS_URL_PREFIX;
}

const string & gs_service_impl::get_xml_namespace()
{
  return GS_XML_NAMESPACE;
}

bool gs_service_impl::is_multipart_download_supported()
{
  return true;
}

bool gs_service_impl::is_multipart_upload_supported()
{
  return false;
}

void gs_service_impl::sign(request *req)
{
  mutex::scoped_lock lock(_mutex);

  if (time(NULL) >= _expiry)
    refresh(lock);

  req->set_header("Authorization", _access_token);
  req->set_header("x-goog-api-version", "2");
}

void gs_service_impl::refresh(const mutex::scoped_lock &lock)
{
  gs_service_impl::get_tokens(
    GT_REFRESH, 
    _refresh_token, 
    &_access_token,
    &_expiry,
    NULL);

  S3_LOG(
    LOG_DEBUG, 
    "gs_service_impl::refresh", 
    "using refresh token [%s], got access token [%s].\n",
    _refresh_token.c_str(),
    _access_token.c_str());

  _access_token = "OAuth " + _access_token;
}
