/*
 * service.cc
 * -------------------------------------------------------------------------
 * Implementation for service static methods.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2011, Tarick Bedeir.
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

#include <stdexcept>

#include "config.h"
#include "services/service.h"
#include "services/aws_impl.h"
#include "services/gs_impl.h"

using namespace std;

using namespace s3;
using namespace s3::services;

impl::ptr service::s_impl;

void service::init(const string &service)
{
  if (service == "aws")
    s_impl.reset(new aws_impl());

  else if (service == "google-storage")
    s_impl.reset(new gs_impl());

  else
    throw runtime_error("unrecognized service.");
}
