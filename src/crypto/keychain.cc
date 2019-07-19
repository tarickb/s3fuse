/*
 * crypto/keychain.cc
 * -------------------------------------------------------------------------
 * macOS Keychain helpers.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2019, Tarick Bedeir.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "crypto/keychain.h"

#ifdef __APPLE__
#include <CoreFoundation/CFString.h>
#include <Security/SecItem.h>
#endif

#include "base/config.h"
#include "base/logger.h"

namespace s3 {
namespace crypto {

std::string Keychain::BuildIdentifier(const std::string &service,
                                      const std::string &bucket_name,
                                      const std::string &volume_key_id) {
  return "service=" + service + ",bucket=" + bucket_name +
         ",key=" + volume_key_id;
}

bool Keychain::ReadPassword(const std::string &id, std::string *password) {
#ifdef __APPLE__
  if (!base::Config::use_macos_keychain()) return false;

  CFStringRef id_ref =
      CFStringCreateWithCString(nullptr, id.c_str(), kCFStringEncodingUTF8);

  CFStringRef item_keys[] = {kSecClass, kSecAttrAccount, kSecAttrService,
                             kSecReturnData, kSecMatchLimit};
  CFTypeRef item_values[] = {kSecClassGenericPassword, id_ref,
                             CFSTR(PACKAGE_NAME), kCFBooleanTrue,
                             kSecMatchLimitOne};
  static_assert(sizeof(item_keys) == sizeof(item_values),
                "Key/value size mismatch.");

  CFDictionaryRef attributes = CFDictionaryCreate(
      nullptr, reinterpret_cast<const void **>(item_keys),
      reinterpret_cast<const void **>(item_values),
      sizeof(item_keys) / sizeof(item_keys[0]), &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks);

  bool success = false;
  CFTypeRef result = nullptr;

  OSStatus err = SecItemCopyMatching(attributes, &result);

  if (err == errSecSuccess) {
    if (CFGetTypeID(result) == CFDataGetTypeID()) {
      *password = reinterpret_cast<const char *>(
          CFDataGetBytePtr(static_cast<CFDataRef>(result)));
      success = true;
    } else {
      S3_LOG(LOG_WARNING, "Keychain::ReadPassword",
             "Result is not of type CFData.\n");
    }
  } else {
    CFStringRef msg = SecCopyErrorMessageString(err, nullptr);
    S3_LOG(LOG_WARNING, "Keychain::ReadPassword",
           "Failed to read password from Keychain: %s\n",
           CFStringGetCStringPtr(msg, kCFStringEncodingUTF8));
    CFRelease(msg);
  }

  if (result) CFRelease(result);
  CFRelease(attributes);
  CFRelease(id_ref);

  return success;
#else
  return false;
#endif
}

void Keychain::WritePassword(const std::string &id,
                             const std::string &password) {
#ifdef __APPLE__
  if (!base::Config::use_macos_keychain()) return;

  const std::string desc = std::string(PACKAGE_NAME) + " Key: " + id;

  CFStringRef id_ref =
      CFStringCreateWithCString(nullptr, id.c_str(), kCFStringEncodingUTF8);
  CFStringRef password_ref = CFStringCreateWithCString(
      nullptr, password.c_str(), kCFStringEncodingUTF8);
  CFStringRef desc_ref =
      CFStringCreateWithCString(nullptr, desc.c_str(), kCFStringEncodingUTF8);

  CFStringRef item_keys[] = {kSecClass,           kSecAttrAccount,
                             kSecAttrService,     kSecValueData,
                             kSecAttrDescription, kSecAttrLabel};
  CFTypeRef item_values[] = {kSecClassGenericPassword,
                             id_ref,
                             CFSTR(PACKAGE_NAME),
                             password_ref,
                             desc_ref,
                             desc_ref};
  static_assert(sizeof(item_keys) == sizeof(item_values),
                "Key/value size mismatch.");

  CFDictionaryRef attributes = CFDictionaryCreate(
      nullptr, reinterpret_cast<const void **>(item_keys),
      reinterpret_cast<const void **>(item_values),
      sizeof(item_keys) / sizeof(item_keys[0]), &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks);

  OSStatus err = SecItemAdd(attributes, NULL);
  if (err != errSecSuccess) {
    CFStringRef msg = SecCopyErrorMessageString(err, nullptr);
    S3_LOG(LOG_WARNING, "Keychain::WritePassword",
           "Failed to write password to Keychain: %s\n",
           CFStringGetCStringPtr(msg, kCFStringEncodingUTF8));
    CFRelease(msg);
  }

  CFRelease(attributes);
  CFRelease(password_ref);
  CFRelease(desc_ref);
  CFRelease(id_ref);
#endif
}

}  // namespace crypto
}  // namespace s3
