// Copyright (c) 2012-2018 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __ELASTOS_SDK_INVENTORYMESSAGE_H_
#define __ELASTOS_SDK_INVENTORYMESSAGE_H_

#include "IMessage.h"

namespace Elastos {
	namespace ElaWallet {

		class InventoryMessage :
			public IMessage {
		public:
			InventoryMessage();

			virtual int Accept(BRPeer *peer, const uint8_t *msg, size_t msgLen);

			virtual void Send(BRPeer *peer);

			void Send(BRPeer *peer, const UInt256 txHashes[], size_t txCount);

		private:
			void sendTransaction(BRPeer *peer, const UInt256 &txHash);
		};

	}
}


#endif //__ELASTOS_SDK_INVENTORYMESSAGE_H_
