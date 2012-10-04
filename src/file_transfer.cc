/*
 * transfer_manager.cc
 * -------------------------------------------------------------------------
 * Single- and multi-part upload/download logic.
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

#include <boost/lexical_cast.hpp>

#include "config.h"
#include "transfer_manager.h"
#include "logger.h"
#include "object.h"
#include "request.h"
#include "service.h"
#include "thread_pool.h"
#include "util.h"
#include "xml.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{

}


