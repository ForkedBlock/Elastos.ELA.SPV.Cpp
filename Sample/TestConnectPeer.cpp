

#include <climits>
#include <iostream>
#include <boost/scoped_ptr.hpp>
#include <SDK/Transaction/Transaction.h>
#include <SDK/ELACoreExt/Payload/PayloadRegisterIdentification.h>
#include <SDK/Common/Utils.h>
#include <SDK/Implement/MainchainSubWallet.h>
#include <SDK/Implement/SidechainSubWallet.h>
#include <SDK/Implement/IdChainSubWallet.h>
#include <SDK/Implement/MasterWallet.h>

#include "BRChainParams.h"

#include "Wrapper/ChainParams.h"
#include "MasterWalletManager.h"
#include "Log.h"

#include "TestConnectPeer.h"

using namespace Elastos::ElaWallet;

static int BRMainNetVerifyDifficulty(const BRMerkleBlock *block, const BRSet *blockSet) {
	const BRMerkleBlock *previous, *b = NULL;
	uint32_t i;

	assert(block != NULL);
	assert(blockSet != NULL);

	// check if we hit a difficulty transition, and find previous transition block
	if ((block->height % BLOCK_DIFFICULTY_INTERVAL) == 0) {
		for (i = 0, b = block; b && i < BLOCK_DIFFICULTY_INTERVAL; i++) {
			b = (const BRMerkleBlock *) BRSetGet(blockSet, &b->prevBlock);
		}
	}

	previous = (const BRMerkleBlock *) BRSetGet(blockSet, &block->prevBlock);
	return BRMerkleBlockVerifyDifficulty(block, previous, (b) ? b->timestamp : 0);
}

void TestConnectPeer::transfer(ISubWallet *subWallet, const std::string &payPassword, uint64_t amount, const std::string &to) {
	nlohmann::json tx =  subWallet->CreateTransaction("", to, amount, "memo", "remark");
	Log::getLogger()->debug("transfer tx = {}", tx.dump());
	nlohmann::json result = subWallet->SendRawTransaction(tx, 10000, payPassword);
	Log::getLogger()->debug("send tx result = {}", result.dump());
}

void TestConnectPeer::deposit(ISubWallet *subWallet, uint64_t amount, const std::string &sidechainAddress, const std::string &payPassword) {

	MainchainSubWallet *mainchainSubWallet = dynamic_cast<MainchainSubWallet *>(subWallet);

	nlohmann::json sidechainAccounts;
	sidechainAccounts.push_back(sidechainAddress);

	nlohmann::json sidechainAmounts;
	sidechainAmounts.push_back(amount);

	nlohmann::json sidechainIndices;
	sidechainIndices.push_back(0);

	nlohmann::json tx = mainchainSubWallet->CreateDepositTransaction("", "XKUh4GLhFJiqAMTF6HyWQrV9pK9HcGUdfJ", amount + 20000,
												 sidechainAccounts, sidechainAmounts, sidechainIndices, "deposit to side chain", "deposit");

	Log::getLogger()->debug("deposit tx = {}", tx.dump());

	nlohmann::json result = mainchainSubWallet->SendRawTransaction(tx, 10000, payPassword);

	Log::getLogger()->debug("send deposit tx result = {}", result.dump());
}

void TestConnectPeer::withdraw(ISubWallet *subWallet, uint64_t amount, const std::string &mainchainAddress, const std::string &payPassword) {
	SidechainSubWallet *sidechainSubWallet = dynamic_cast<SidechainSubWallet *>(subWallet);

	UInt168 u = UINT168_ZERO;
	std::string destroyAddress = Utils::UInt168ToAddress(u);

	nlohmann::json mainchainAccounts;
	mainchainAccounts.push_back(mainchainAddress);

	nlohmann::json mainchainAmounts;
	mainchainAmounts.push_back(amount);

	nlohmann::json mainchainIndexs;
	mainchainIndexs.push_back(0);

	nlohmann::json tx = sidechainSubWallet->CreateWithdrawTransaction("",
					destroyAddress, amount + 20000, mainchainAccounts, mainchainAmounts,
					mainchainIndexs, "memo", "remark");

	Log::getLogger()->debug("withdraw tx = {}", tx.dump());

	nlohmann::json result = sidechainSubWallet->SendRawTransaction(tx, 10000, payPassword);

	Log::getLogger()->debug("withdraw result = {}", result.dump());
}

void TestConnectPeer::registerId(IMasterWallet *imasterWallet, ISubWallet *subWallet, const std::string &payPassword) {
	IdChainSubWallet *idchainSubWallet = dynamic_cast<IdChainSubWallet *>(subWallet);

	MasterWallet *masterWallet = dynamic_cast<MasterWallet *>(imasterWallet);

	std::string id = masterWallet->DeriveIdAndKeyForPurpose(1, 0);

	nlohmann::json payload = "{ \"Id\": \"ij8rfb6A4Ri7c5CRE1nDVdVCUMuUxkk2c6\", \"Contents\": [ { \"Path\": \"kyc/person/identityCard\", \"Values\": [ { \"Proof\": \"\\\"signature\\\":\\\"30450220499a5de3f84e7e919c26b6a8543fd24129634c65ee4d38fe2e3386ec8a5dae57022100b7679de8d181a454e2def8f55de423e9e15bebcde5c58e871d20aa0d91162ff6\\\",\\\"notary\\\":\\\"COOIX\\\"\", \"DataHash\": \"bd117820c4cf30b0ad9ce68fe92b0117ca41ac2b6a49235fabd793fc3a9413c0\" } ] } ] }"_json;
	payload["Id"] = id;

	std::string sign = masterWallet->Sign(id, payload.dump(), payPassword);
	payload["Sign"] = sign;
	nlohmann::json program = masterWallet->GenerateProgram(id, payload.dump(), payPassword);

	nlohmann::json tx = idchainSubWallet->CreateIdTransaction("",
					payload, program, "memo", "remark");

	Log::getLogger()->debug("register id tx = {}", tx.dump());

	nlohmann::json result = idchainSubWallet->SendRawTransaction(tx, 10000, payPassword);

	Log::getLogger()->debug("register id result = {}", result.dump());
}

IMasterWallet *TestConnectPeer::importWithKeystore(boost::shared_ptr<MasterWalletManager> walletFactory, const std::string &payPassword) {
	std::string backupPassword = "s12345678";
	nlohmann::json keystore = "{\"adata\":\"\",\"cipher\":\"aes\",\"ct\":\"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\",\"iter\":10000,\"iv\":\"jzFRpbBc9UOjRpGEyOlOTQ==\",\"ks\":256,\"mode\":\"ccm\",\"salt\":\"XExBAvEGd0s=\",\"ts\":64,\"v\":1}"_json;
	return walletFactory->ImportWalletWithKeystore("ELA", keystore, backupPassword, payPassword, "");
}

IMasterWallet *TestConnectPeer::importWithMnemonic(boost::shared_ptr<MasterWalletManager> walletFactory, const std::string &payPassword) {
	std::string mnemonic = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
	return walletFactory->ImportWalletWithMnemonic("ELA", mnemonic, "", payPassword, "english");
}

void TestConnectPeer::runPeerConnectTest_WalletFactory() {

	std::string payPassword = "s12345678";
	boost::shared_ptr<MasterWalletManager> walletFactory(new MasterWalletManager("Data"));

#if 1
	IMasterWallet *masterWallet = importWithMnemonic(walletFactory, payPassword);
#else
	IMasterWallet *masterWallet = importWithKeystore(walletFactory, payPassword);
#endif

	ISubWallet *sidechainWallet = masterWallet->CreateSubWallet("IdChain", payPassword, false);
	ISubWallet *mainchainWallet = masterWallet->CreateSubWallet("ELA", payPassword, false);

	std::cout << "side chain wallet addrs: " << sidechainWallet->GetAllAddress(0, INT_MAX) << std::endl;
	std::cout << "main chain wallet addrs: " << mainchainWallet->GetAllAddress(0, INT_MAX) << std::endl;
	sleep(4);

	bool hasTransfer = false, hasDeposit = false, hasWithdraw = false, hasRegisterId = false;

	while (true) {
		sidechainWallet->GetAllTransaction(0, 20, "");
		SPDLOG_DEBUG(Log::getLogger(),"side chain balance = {}", sidechainWallet->GetBalance());
		mainchainWallet->GetAllTransaction(0, 20, "");
		SPDLOG_DEBUG(Log::getLogger(),"main chain balance = {}", mainchainWallet->GetBalance());

#if 0
		if (!hasDeposit) {
			deposit(mainchainWallet, 10000000000, "EUjtxVuLk3vA1VSqN3EKiK4dY5u7izJQWi", payPassword);
			hasDeposit = true;
		}
#endif

#if 0
		if (!hasWithdraw) {
			withdraw(sidechainWallet, 100000000, "EZcvtcsT8wXSXBTeijCdSXvT2sk62yPii5", payPassword);
			hasWithdraw = true;
		}
#endif

#if 0
		if (!hasRegisterId) {
			registerId(masterWallet, sidechainWallet, payPassword);
			hasRegisterId = true;
		}
#endif

#if 0
		if (!hasTransfer) {
			transfer(mainchainWallet, payPassword, 10000000, "EZcvtcsT8wXSXBTeijCdSXvT2sk62yPii5");
			hasTransfer = true;
		}
#endif
		sleep(10);
	}

	masterWallet->DestroyWallet(mainchainWallet);
	masterWallet->DestroyWallet(sidechainWallet);
	walletFactory->DestroyWallet(masterWallet->GetId());
}
