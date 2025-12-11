// Copyright (C) 2017 Markus Wanner <markus@bluegap.ch>
//
// This program is made available under the GNU GPL version 2.0 or
// greater. See the accompanying file COPYING for details.
//
// This program is distributed WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE.

#include "base.hh"

#include <memory>

#include "botan_glue.hh"
// Botan 3 reorganized headers
#include <botan/pkcs8.h>
#include <botan/pk_keys.h>
#include <botan/rng.h>
#include <botan/x509_key.h>

// UI interface is only needed for older Botan versions
#if defined(BOTAN_VERSION_CODE) && BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(3,0,0)
  // Botan 3 doesn't use UI interface
#elif defined(BOTAN_VERSION_CODE) && BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(2,0,0)
  // Botan 2 doesn't use UI interface
#elif defined(BOTAN_VERSION_CODE) && BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(1,9,11) && \
    BOTAN_VERSION_CODE < BOTAN_VERSION_CODE_FOR(1,11,0)
  #include <botan/ui.h>
#endif

#include "gzip.hh"
#include "lazy_rng.hh"
#include "sanity.hh"

using std::make_shared;
using std::shared_ptr;
using std::string;
// Botan 3 changed the class name for private keys
#if defined(BOTAN_VERSION_CODE) && BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(3,0,0)
using Botan::Private_Key;
using PrivateKeyPtr = std::shared_ptr<Private_Key>;
#else
using Botan::PKCS8_PrivateKey;
using PrivateKeyPtr = std::shared_ptr<PKCS8_PrivateKey>;
#endif


void
initialize_botan(bool for_testing)
{
#if defined(BOTAN_VERSION_CODE) && BOTAN_VERSION_CODE < BOTAN_VERSION_CODE_FOR(3,0,0) \
    && BOTAN_VERSION_CODE < BOTAN_VERSION_CODE_FOR(2,0,0)
  // Set up some global state, like secure memory allocation etc.
  // Botan 3 doesn't need explicit initialization
  Botan::LibraryInitializer acquire_botan(
    for_testing
      ? "thread_safe=0 selftest=1 seed_rng=1 use_engines=0 "
          "secure_memory=1 fips140=0"
      : "thread_safe=0 selftest=0 seed_rng=1 use_engines=0 "
          "secure_memory=1 fips140=0"
  );
#endif
}


// A helper class implementing Botan::User_Interface - which doesn't really
// interface with the user, but provides the necessary plumbing for Botan.
//
// See Botan commit 2d09d7d0cd4bd0e7155d001dd65a4f29103b158c
#if BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(3,0,0)
  // Botan 3 uses different password callback mechanism
#elif BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(2,0,0)
  // Botan 2 uses different password callback mechanism
#elif BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(1,9,11) && \
  BOTAN_VERSION_CODE < BOTAN_VERSION_CODE_FOR(1,11,0)
class Dummy_UI : public Botan::User_Interface
{
public:
  virtual std::string get_passphrase(const std::string &, const std::string &,
                                     Botan::User_Interface::UI_Result &) const
    { throw Passphrase_Required("Passphrase required"); }
};
#endif


// A simplistic function throwing the Passphrase_Required exception. To be
// passed to Botan as a callback.
#if defined(BOTAN_VERSION_CODE) && BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(3,0,0)
std::function<std::string ()> pass_req_throw_func =
  [] ()
    {
      throw Passphrase_Required("Passphrase required");
      return string();
    };
#elif defined(BOTAN_VERSION_CODE) && BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(2,0,0)
std::function<std::string ()> pass_req_throw_func =
  [] ()
    {
      throw Passphrase_Required("Passphrase required");
      return string();
    };
#elif defined(BOTAN_VERSION_CODE) && BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(1,11,0)
std::function<std::pair<bool, std::string> ()> pass_req_throw_func =
  [] ()
    {
      throw Passphrase_Required("Passphrase required");
      return std::pair<bool, string>();
    };
#endif

// A Botan-version agnostic key loader function trying to load an
// unprotected key, i.e. one that loads without any password. Returns
// a pointer to the loaded key, if successful, throws a Passphrase_Required
// exception, if a password is required or throws a Decoding_error in case
// of invalid data.
PrivateKeyPtr
load_pkcs8_key(string const & name, string const & priv_key)
{
  try
    {
      // Botan 3 renamed DataSource_Memory to DataSource_Stream
      Botan::DataSource_Stream ds(priv_key);
      // Botan 3 uses a different API for loading keys
      return PrivateKeyPtr(
        Botan::PKCS8::load_key(ds, pass_req_throw_func));
    }
  catch (Botan::Decoding_Error const & e)
    {
#if defined(BOTAN_VERSION_CODE) && BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(3,0,0)
      // Botan 3 handles exceptions differently
      if (strstr(e.what(), "Passphrase required") != NULL)
        throw Passphrase_Required("Passphrase required");
#elif defined(BOTAN_VERSION_CODE) && BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(1,11,14)
      // Since Botan 1.11.14, the load_key method catches *all* exceptions
      // thrown by the `get_passphrase` function passed and wrapps them in
      // a `Decoding_Error`. The only way to figure it has been thrown is
      // checking the error message.
      //
      // FIXME: instead of blindly calling load_key and let it throw,
      //        consider parsing the PEM header before-hand, instead.
      if (strstr(e.what(), "Passphrase required") != NULL)
        throw Passphrase_Required("Passphrase required");
#endif
      E(false, origin::user,
        F("malformed key_packet: invalid private key data for '%s': %s")
          % name % e.what());
    }
  // Since we do not want to prompt for a password to decode it finally,
  // we ignore the exceptions that indicate a missing password.  These
  // differ slightly between Botan versions.
#if defined(BOTAN_VERSION_CODE) && BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(3,0,0)
  // Botan 3 uses standard exceptions
  catch (Passphrase_Required)
    { throw Passphrase_Required("Passphrase required"); }
#elif defined(BOTAN_VERSION_CODE) && BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(1,11,14)
  // Handled above, no specific exception.
#elif defined(BOTAN_VERSION_CODE) && BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(1,9,11)
  catch (Passphrase_Required)
    { throw Passphrase_Required("Passphrase required"); }
#else
  catch (Botan::Invalid_Argument)
    { throw Passphrase_Required("Passphrase required"); }
#endif
}
