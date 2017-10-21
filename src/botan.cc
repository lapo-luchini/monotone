// Copyright (C) 2017 Markus Wanner <markus@bluegap.ch>
//
// This program is made available under the GNU GPL version 2.0 or
// greater. See the accompanying file COPYING for details.
//
// This program is distributed WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE.

#include "base.hh"
#include <functional>
#include <string>
#include <utility>

#include "botan.hh"

using std::function;
using std::pair;
using std::string;

#if BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(2,0,0)
function<string ()> pass_req_throw_func =
  [] ()
    {
      throw Passphrase_Required("Passphrase required");
      return string();
    };
#elif BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(1,11,0)
function<pair<bool, string> ()> pass_req_throw_func =
  [] ()
    {
      throw Passphrase_Required("Passphrase required");
      return pair<bool, string>();
    };
#endif
