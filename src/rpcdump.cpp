// Copyright (c) 2011 Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"
#include "init.h" // for pwalletMain
#include "bitcoinrpc.h"

// #include <boost/asio.hpp>
// #include <boost/iostreams/concepts.hpp>
// #include <boost/iostreams/stream.hpp>
#include <boost/lexical_cast.hpp>
// #ifdef USE_SSL
// #include <boost/asio/ssl.hpp> 
// typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SSLStream;
// #endif
// #include <boost/xpressive/xpressive_dynamic.hpp>
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"

#define printf OutputDebugStringF

// using namespace boost::asio;
using namespace json_spirit;
using namespace std;

extern Object JSONRPCError(int code, const string& message);

Value importprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "importprivkey <bitcoinprivkey> [label]\n"
            "Adds a private key (as returned by dumpprivkey) to your wallet.");

    string strSecret = params[0].get_str();
    string strLabel = "";
    if (params.size() > 1)
        strLabel = params[1].get_str();
    CBitcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if (!fGood) throw JSONRPCError(-5,"Invalid private key");

    CKey key;
    bool fCompressed;
    CSecret secret = vchSecret.GetSecret(fCompressed);
    key.SetSecret(secret, fCompressed);
    CBitcoinAddress vchAddress = CBitcoinAddress(key.GetPubKey());

    CRITICAL_BLOCK(cs_main)
    CRITICAL_BLOCK(pwalletMain->cs_wallet)
    {
        pwalletMain->MarkDirty();
        pwalletMain->SetAddressBookName(vchAddress, strLabel);

        if (!pwalletMain->AddKey(key))
            throw JSONRPCError(-4,"Error adding key to wallet");

        pwalletMain->ScanForWalletTransactions(pindexGenesisBlock, true);
        pwalletMain->ReacceptWalletTransactions();
    }

    MainFrameRepaint();

    return Value::null;
}

Value removeprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 1)
        throw runtime_error(
            "removeprivkey <bitcoinprivkey>\n"
            "Removes a private key (as returned by dumpprivkey) from your wallet.\n"
            "Warning: this will remove transactions from your wallet, and may change the balance of unrelated accounts.\n");

    string strSecret = params[0].get_str();
    CBitcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if (!fGood) throw JSONRPCError(-5,"Invalid private key");

    CKey key;
    bool fCompressed = false;
    key.SetSecret(vchSecret.GetSecret(fCompressed), fCompressed);
    CBitcoinAddress address(key.GetPubKey());

    CRITICAL_BLOCK(pwalletMain->cs_wallet)
    {
        pwalletMain->MarkDirty();
        if (!pwalletMain->RemoveKey(address))
            throw JSONRPCError(-4,"Error removing key from wallet");

        pwalletMain->DelAddressBookName(address);
    }

    MainFrameRepaint();

    return Value::null;
}



Value dumpprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "dumpprivkey <bitcoinaddress>\n"
            "Reveals the private key corresponding to <bitcoinaddress>.");

    string strAddress = params[0].get_str();
    CBitcoinAddress address;
    if (!address.SetString(strAddress))
        throw JSONRPCError(-5, "Invalid bitcoin address");
    CSecret vchSecret;
    bool fCompressed;
    if (!pwalletMain->GetSecret(address, vchSecret, fCompressed))
        throw JSONRPCError(-4,"Private key for address " + strAddress + " is not known");
    return CBitcoinSecret(vchSecret, fCompressed).ToString();
}
