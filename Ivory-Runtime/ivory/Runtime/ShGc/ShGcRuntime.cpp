#include <immintrin.h>
#include <iostream>

#include "ShGcRuntime.h"
#include "ivory/Runtime/ShGc/ShGcInt.h"

#include "cryptoTools/Common/Log.h"

#include "libOTe/Base/naor-pinkas.h"

namespace osuCrypto
{

	const std::array<block, 2> ShGcRuntime::mPublicLabels{ toBlock(0,0), toBlock(~0,~0) };
	ShGcRuntime::ShGcRuntime()
		: mRecvBit([this]() {bool b; mChannel->recv((u8*)&b, 1); return b; })
	{
		mTweaks[0] = ZeroBlock;
		mTweaks[1] = AllOneBlock;

		mOtCount = 0;
		mBytesSent = 0;
	}


	ShGcRuntime::~ShGcRuntime()
	{

	}


	void ShGcRuntime::init(Channel& chl, block seed, Role role, u64 partyIdx)
	{
		mPrng.SetSeed(seed);
		mAes.setKey(mPrng.get<block>());
		mChannel = &chl;
		mGlobalOffset = mPrng.get<block>() | OneBlock;
		mZeroAndGlobalOffset[0] = ZeroBlock;
		mZeroAndGlobalOffset[1] = mGlobalOffset;
		mRole = role;
		mPartyIdx = partyIdx;


		if (role == Garbler)
		{
			NaorPinkas base;
			BitVector choices(128);
			std::vector<block> msg(128);
			base.receive(choices, msg, mPrng, chl, 4);
			mOtExtSender.setBaseOts(msg, choices);
		}
		else
		{
			NaorPinkas base;
			std::vector<std::array<block, 2>> msg(128);
			base.send(msg, mPrng, chl, 4);
			mOtExtRecver.setBaseOts(msg);
		}

	}

	ShGc::GarbledMem ShGcRuntime::getNewMem(u64 size)
	{
		return ShGc::GarbledMem(new std::vector<block>(size));
	}

	void ShGcRuntime::freeMem(const ShGc::GarbledMem& mem)
	{
	}

	sInt ShGcRuntime::sIntInput(BitCount bc, u64 partyIdx)
	{
		auto ret = new ShGcInt(*this, bc.mBitCount);
		ShGc::InputItem ii;
		ii.mLabels = ret->mLabels;

		if (mRole == Garbler) mOtCount += bc.mBitCount;

		enqueue(std::move(ii));
		return sIntBasePtr(ret);
	}

	sInt ShGcRuntime::sIntInput(sInt::ValueType v, BitCount bc)
	{
		auto ret = new ShGcInt(*this, bc.mBitCount);
		ShGc::InputItem ii;
		ii.mLabels = ret->mLabels;
		ii.mInputVal.append((u8*)&v, bc.mBitCount);

		if (mRole == Evaluator)
		{
			mOtCount += bc.mBitCount;
			mOtChoices.append(ii.mInputVal);
		}
		enqueue(std::move(ii));

		return sIntBasePtr(ret);
	}

	//void ShGcRuntime::scheduleOp(
	//    Op op,
	//    span<RuntimeData*> io)
	//{
	//    mCrtQueue.emplace();
	//    CircuitItem& item = mCrtQueue.back();

	//    item.mLabels.resize(io.size());
	//    std::vector<u64> sizes(io.size());

	//    for (u64 i = 0; i < io.size(); ++i)
	//    {
	//        item.mLabels[i] = static_cast<ShGcRuntimeData*>(io[i])->mLabels;
	//        sizes[i] = item.mLabels[i]->size();
	//    }


	//    switch (op)
	//    {
	//    case osuCrypto::Op::Add:

	//        item.mCircuit = mLibrary.int_int_add(sizes[0], sizes[1], sizes[2]);
	//        item.mInputBundleCount = 2;
	//        break;
	//    case osuCrypto::Op::Subtract:
	//        item.mCircuit = mLibrary.int_int_subtract(sizes[0], sizes[1], sizes[2]);
	//        item.mInputBundleCount = 2;
	//        break;
	//    case osuCrypto::Op::Multiply:
	//        item.mCircuit = mLibrary.int_int_mult(sizes[0], sizes[1], sizes[2]);
	//        item.mInputBundleCount = 2;
	//        break;
	//    case osuCrypto::Op::Divide:
	//        item.mCircuit = mLibrary.int_int_div(sizes[0], sizes[1], sizes[2]);
	//        item.mInputBundleCount = 2;
	//        break;
	//    case osuCrypto::Op::LT:
	//        item.mCircuit = mLibrary.int_int_lt(sizes[0], sizes[1]);
	//        item.mInputBundleCount = 2;
	//        break;
	//    case osuCrypto::Op::GTEq:
	//        item.mCircuit = mLibrary.int_int_gteq(sizes[0], sizes[1]);
	//        item.mInputBundleCount = 2;
	//        break;
	//    case osuCrypto::Op::Mod:
	//        throw std::runtime_error(LOCATION);
	//        break;
	//    case osuCrypto::Op::And:
	//        throw std::runtime_error(LOCATION);
	//        break;
	//    case osuCrypto::Op::Or:
	//        throw std::runtime_error(LOCATION);
	//        break;
	//    case osuCrypto::Op::Not:
	//        throw std::runtime_error(LOCATION);
	//        break;
	//    case osuCrypto::Op::BitwiseAnd:
	//        item.mCircuit = mLibrary.int_int_bitwiseAnd(sizes[0], sizes[1], sizes[2]);
	//        item.mInputBundleCount = 2;
	//        break;
	//    case osuCrypto::Op::BitWiseOr:
	//        throw std::runtime_error(LOCATION);
	//        break;
	//    case osuCrypto::Op::BitwiseNot:
	//        item.mCircuit = mLibrary.int_bitInvert(sizes[0]);
	//        item.mInputBundleCount = 1;
	//        break;
	//    case osuCrypto::Op::IfElse:
	//        if(sizes[0] != sizes[1] || sizes[0] != sizes[3])
	//            throw std::runtime_error("IfElse must be performed with variables of the same bit length. " LOCATION);
	//        if (sizes[2] != 1)
	//            throw std::runtime_error(LOCATION);

	//        item.mCircuit = mLibrary.int_int_multiplex(sizes[0]);
	//        item.mInputBundleCount = 3;
	//        break;
	//    default:
	//        throw std::runtime_error(LOCATION);
	//        break;
	//    }


	//    //process();

	//}

	//void ShGcRuntime::scheduleInput(
	//    RuntimeData* data, const BitVector& value)
	//{
	//    mInputQueue.emplace();

	//    auto& enc = static_cast<ShGcRuntimeData*>(data)->mLabels;

	//    if (mRole == Evaluator)
	//    {
	//        mOtChoices.append(value);
	//        mOtCount += value.size();
	//    }

	//    auto& item = mInputQueue.back();

	//    item.mInputVal = value;
	//    item.mLabels = enc;

	//    //process();

	//}

	//void ShGcRuntime::scheduleInput(RuntimeData* data, u64 partyIdx)
	//{
	//    auto& input = *static_cast<ShGcRuntimeData*>(data);

	//    if (mRole == Garbler)
	//    {
	//        mOtCount += input.mLabels->size();
	//    }


	//    mInputQueue.emplace();

	//    auto& item = mInputQueue.back();

	//    item.mLabels = input.mLabels;

	//    //process();

	//}

	//void ShGcRuntime::scheduleOutput(RuntimeData* data, u64 partyIdx)
	//{
	//    auto& input = *static_cast<ShGcRuntimeData*>(data);

	//    mOutputQueue.emplace();

	//    auto& item = mOutputQueue.back();

	//    item.mLabels = input.mLabels;
	//    item.mOutputVal = nullptr;

	//    //process();

	//}

	//void ShGcRuntime::scheduleOutput(RuntimeData* data,
	//    std::future<BitVector>& future)
	//{

	//    auto& input = *static_cast<ShGcRuntimeData*>(data);
	//    mOutputQueue.emplace();

	//    auto& item = mOutputQueue.back();

	//    item.mLabels = input.mLabels;
	//    item.mOutputVal = new std::promise<BitVector>();

	//    future = item.mOutputVal->get_future();

	//    //process();
	//}

	ShGc::GarbledMem ShGcRuntime::getPublicGarbledMem(u8* data, u64 bitCount)
	{
		BitIterator iter(data, 0);
		auto ret = ShGc::GarbledMem(new std::vector<block>(bitCount));
		for (u64 i = 0; i < bitCount; ++i)
		{
			(*ret)[i] = mPublicLabels[*iter];
			++iter;
		}

		return ret;
	}

	void ShGcRuntime::enqueue(ShGc::InputItem&& item)
	{
		mInputQueue.emplace(std::move(item));
	}

	void ShGcRuntime::enqueue(ShGc::CircuitItem&& item)
	{
		mCrtQueue.emplace(std::move(item));
	}

	void ShGcRuntime::enqueue(ShGc::OutputItem&& item)
	{
		mOutputQueue.emplace(std::move(item));
	}

	void ShGcRuntime::processesQueue()
	{

		// TODO: add logic to decide when to process the queue
		// and when to simply queue things up. For now,
		// always keep the queue at size 1 or less.
		if (true)
		{
			if (mRole == Garbler)
			{
				garblerInput();
				garblerCircuit();
				garblerOutput();
			}
			else
			{
				evaluatorInput();
				evaluatorCircuit();
				evaluatorOutput();
			}
		}
	}


	void ShGcRuntime::garblerInput()
	{
		std::vector<std::array<block, 2>> messages(mOtCount);
		if (mOtCount)
		{

			mOtCount = 0;

			mOtExtSender.send(messages, mPrng, *mChannel);


		}

		auto iter = messages.begin();

		while (mInputQueue.size())
		{


			auto& item = mInputQueue.front();

			if (item.mInputVal.size())
			{
				mAes.ecbEncCounterMode(mInputIdx, item.mLabels->size(), item.mLabels->data());
				mInputIdx += item.mLabels->size();

				std::vector<block>view(item.mLabels->size());

				for (u64 i = 0; i < item.mLabels->size(); ++i)
				{
					view[i] = (*item.mLabels)[i] ^ mZeroAndGlobalOffset[item.mInputVal[i]];
				}
				mChannel->asyncSend(std::move(view));
			}
			else
			{
				std::vector<block>view(item.mLabels->size());
				for (u64 i = 0; i < item.mLabels->size(); ++i, ++iter)
				{
					(*item.mLabels)[i] = (*iter)[0];
					view[i] = (*iter)[1] ^ (*iter)[0] ^ mGlobalOffset;
				}
				mChannel->asyncSend(std::move(view));
			}

			mInputQueue.pop();
		}
	}

	void ShGcRuntime::evaluatorInput()
	{
		static const std::array<block, 2> zeroAndAllOnesBlk{ ZeroBlock, AllOneBlock };

		if (mOtChoices.size())
		{

			if (sharedMem.size() < mOtCount)
				sharedMem.resize(mOtCount);

			//sharedMem.resize(mOtCount);
			span<block> view(sharedMem.begin(), sharedMem.begin() + mOtCount);

			mOtExtRecver.receive(mOtChoices, sharedMem, mPrng, *mChannel);

			mOtChoices.resize(0);
			mOtCount = 0;

		}

		auto iter = sharedMem.begin();

		while (mInputQueue.size())
		{

			auto& item = mInputQueue.front();

			if (item.mInputVal.size())
			{
				mChannel->recv(sharedBuff);

				for (u64 i = 0; i < item.mLabels->size(); ++i)
				{
					(*item.mLabels)[i] = *iter++ ^ (zeroAndAllOnesBlk[item.mInputVal[i]] & sharedBuff[i]);
				}
			}
			else
			{
				mChannel->recv((u8*)item.mLabels->data(), item.mLabels->size() * sizeof(block));
			}
			mInputQueue.pop();
		}
	}

	void ShGcRuntime::garblerCircuit()
	{

		while (mCrtQueue.size())
		{
			auto& item = mCrtQueue.front();

			if (item.mCircuit)
			{

				if (sharedMem.size() < item.mCircuit->mWireCount) {
					sharedMem.resize(item.mCircuit->mWireCount);
				}



				//std::cout << IoStream::lock;
				auto iter = sharedMem.begin();
				for (u64 i = 0; i < item.mInputBundleCount; ++i) {

					Expects(item.mLabels[i]->size() == item.mCircuit->mInputs[i].mWires.size());

					std::copy(item.mLabels[i]->begin(), item.mLabels[i]->end(), iter);
					iter += item.mLabels[i]->size();
					//for (u64 j = 0; j < item.mLabels[i]->size(); ++j)
					//    std::cout << "garb " << i << " " << j << " " << (*item.mLabels[i])[j] << " " << ((*item.mLabels[i])[j] ^ mGlobalOffset) << std::endl;
				}

				auto gates = std::vector<GarbledGate<2>>(item.mCircuit->mNonXorGateCount);
				garble(*item.mCircuit, sharedMem, mTweaks, gates, mZeroAndGlobalOffset, shareAuxBits);
				if (item.mCircuit->mNonXorGateCount) mChannel->asyncSend(std::move(gates));
				for (auto bit : shareAuxBits)
					mChannel->asyncSendCopy(&bit, 1);
				shareAuxBits.clear();

				for (u64 i = item.mInputBundleCount; i < item.mLabels.size(); ++i) {
					std::copy(iter, iter + item.mLabels[i]->size(), item.mLabels[i]->begin());
					iter += item.mLabels[i]->size();
					//for (u64 j = 0; j < item.mLabels[i]->size(); ++j)
					//    std::cout << "garb " << i << " " << j << " " << (*item.mLabels[i])[j] << " " << ((*item.mLabels[i])[j] ^ mGlobalOffset) << std::endl;
				}


				///////////////////////////////////////////////////////////////////
				//                             DEBUG                             //
				///////////////////////////////////////////////////////////////////
				if (item.mDebugFlag || mDebugFlag)
				{
					std::vector<BitVector>
						inputs(item.mInputBundleCount),
						outputs(item.mLabels.size() - item.mInputBundleCount);

					for (u64 i = 0; i < item.mInputBundleCount; ++i) {
						std::vector<block> evalLabels(item.mLabels[i]->size());
						mChannel->recv((u8*)evalLabels.data(), evalLabels.size() * sizeof(block));
						inputs[i].resize(evalLabels.size());

						for (u64 j = 0; j < item.mLabels[i]->size(); ++j) {
							if (neq(evalLabels[j], (*item.mLabels[i])[j]) &&
								neq(evalLabels[j], (*item.mLabels[i])[j] ^ mGlobalOffset))
							{
								throw std::runtime_error(LOCATION);
							}

							inputs[i][j] = neq(evalLabels[j], (*item.mLabels[i])[j]);
						}
					}

					for (u64 i = 0; i < item.mLabels.size() - item.mInputBundleCount; ++i)
					{
						outputs[i].resize(item.mLabels[i + item.mInputBundleCount]->size());
					}

					item.mCircuit->evaluate(inputs, outputs);

					for (u64 i = 0; i < outputs.size(); ++i)
					{
						std::vector<block> evalLabels(item.mLabels[i + inputs.size()]->size());
						mChannel->recv((u8*)evalLabels.data(), evalLabels.size() * sizeof(block));
						BitVector outputVal(evalLabels.size());

						for (u64 j = 0; j < evalLabels.size(); ++j) {
							if (neq(evalLabels[j], (*item.mLabels[i + inputs.size()])[j]) &&
								neq(evalLabels[j], (*item.mLabels[i + inputs.size()])[j] ^ mGlobalOffset))
							{
								throw std::runtime_error(LOCATION);
							}

							u8 val = neq(evalLabels[j], (*item.mLabels[i + inputs.size()])[j]);
							if (val != outputs[i][j])
							{
								throw std::runtime_error(LOCATION);
							}
						}
					}
				}

				//std::cout << IoStream::unlock;
			}
			else
			{
				Expects(item.mLabels.size() == 2); // copy operation
				*item.mLabels[1] = *item.mLabels[0];
			}

			mCrtQueue.pop();
		}


	}


	void ShGcRuntime::evaluatorCircuit()
	{
		while (mCrtQueue.size())
		{
			auto& item = mCrtQueue.front();

			if (item.mCircuit)
			{
				if (sharedMem.size() < item.mCircuit->mWireCount) {
					sharedMem.resize(item.mCircuit->mWireCount);
				}

				//std::cout << IoStream::lock;
				auto iter = sharedMem.begin();
				for (u64 i = 0; i < item.mInputBundleCount; ++i) {
					std::copy(item.mLabels[i]->begin(), item.mLabels[i]->end(), iter);
					iter += item.mLabels[i]->size();
					//for (u64 j = 0; j < item.mLabels[i]->size(); ++j)
					//    std::cout << "eval " << i << " " << j << " " << (*item.mLabels[i])[j] << std::endl;
				}
				//std::cout << IoStream::unlock;

				if (item.mCircuit->mNonXorGateCount)
				{
					mChannel->recv(sharedBuff);
					Expects(sharedBuff.size() == item.mCircuit->mNonXorGateCount * 2);
				}
				auto gates = span<GarbledGate<2>>(
					(GarbledGate<2>*) sharedBuff.data(),
					item.mCircuit->mNonXorGateCount);

				evaluate(*item.mCircuit, sharedMem, mTweaks, gates, mRecvBit);


				//std::cout << IoStream::lock;
				for (u64 i = item.mInputBundleCount; i < item.mLabels.size(); ++i) {
					std::copy(iter, iter + item.mLabels[i]->size(), item.mLabels[i]->begin());
					iter += item.mLabels[i]->size();
					//for (u64 j = 0; j < item.mLabels[i]->size(); ++j)
					//    std::cout << "eval " << i << " " << j << " " << (*item.mLabels[i])[j] << std::endl;
				}
				//std::cout << IoStream::unlock;


				if (item.mDebugFlag || mDebugFlag)
				{
					for (u64 i = 0; i < item.mLabels.size(); ++i) {
						mChannel->send((u8*)item.mLabels[i]->data(), item.mLabels[i]->size() * sizeof(block));
					}
				}
			}
			else
			{
				Expects(item.mLabels.size() == 2);// copy operation;
				*item.mLabels[1] = *item.mLabels[0];
			}
			//std::cout  << IoStream::lock;
			//for (auto ii = 0; ii < item.mLabels[2]->size(); ++ii)
			//{
			//    std::cout  << "e out[" << ii << "] " << (*item.mLabels[2])[ii] << std::endl;
			//}
			//std::cout  << IoStream::unlock;

			mCrtQueue.pop();
		}

	}


	void ShGcRuntime::garblerOutput()
	{

		while (mOutputQueue.size())
		{
			auto& item = mOutputQueue.front();


			if (item.mOutPartyIdxs[0] == mPartyIdx || item.mOutPartyIdxs.size() == 2)
			{
				if (sharedMem.size() < item.mLabels->size())
				{
					sharedMem.resize(item.mLabels->size());
				}

				mChannel->recv((u8*)sharedMem.data(), item.mLabels->size() * sizeof(block));


				BitVector val(item.mLabels->size());


				for (u64 i = 0; i < item.mLabels->size(); ++i)
				{
					if (neq(sharedMem[i], (*item.mLabels)[i]) && neq(sharedMem[i], (*item.mLabels)[i] ^ mGlobalOffset))
					{
						std::cout << IoStream::lock << "output reveal error at " << i << ":\n   "
							<< sharedMem[i] << "  != " << (*item.mLabels)[i]
							<< " (0) AND \n   "
							<< sharedMem[i] << "  != " << ((*item.mLabels)[i] ^ mGlobalOffset) << std::endl << IoStream::unlock;

						throw std::runtime_error(LOCATION);
					}

					//if (i == 1)
					//{
					//    std::cout  << IoStream::lock << "output reveal at " << i << ":\n   " << sharedMem[i] << "  ?= " << (*item.mLabels)[i]
					//        << " (0) AND \n   " << sharedMem[i] << "  ?= " << ((*item.mLabels)[i] ^ mGlobalOffset) << std::endl << IoStream::unlock;

					//}

					val[i] = PermuteBit(sharedMem[i] ^ (*item.mLabels)[i]);
				}

				item.mOutputProm->set_value(std::move(val));
			}

			if (item.mOutPartyIdxs[0] != mPartyIdx || item.mOutPartyIdxs.size() == 2)
			{
				std::unique_ptr<BitVector> sendBuff(new BitVector(item.mLabels->size()));

				for (u64 i = 0; i < item.mLabels->size(); ++i)
				{
					(*sendBuff)[i] = PermuteBit((*item.mLabels)[i]);
				}

				mChannel->asyncSend(std::move(sendBuff));
			}

			mOutputQueue.pop();
		}
	}

	void ShGcRuntime::evaluatorOutput()
	{
		while (mOutputQueue.size())
		{
			auto& item = mOutputQueue.front();

			if (item.mOutPartyIdxs[0] == mPartyIdx || item.mOutPartyIdxs.size() == 2)
			{
				BitVector val(item.mLabels->size());

				mChannel->recv(val);

				for (u64 i = 0; i < item.mLabels->size(); ++i)
				{
					val[i] = val[i] ^ PermuteBit((*item.mLabels)[i]);
				}

				item.mOutputProm->set_value(std::move(val));
			}
			else
			{
				mChannel->asyncSendCopy((u8*)item.mLabels->data(), item.mLabels->size() * sizeof(block));
			}

			mOutputQueue.pop();
		}
	}

	bool ShGcRuntime::isConstLabel(const block& b)
	{
		return eq(mPublicLabels[0], b) || eq(mPublicLabels[1], b);
	}











	// return values
	// 0: 0
	// 1: not in[constB]
	// 2:     in[constB]
	// 3: 1
	u8 subGate(bool constB, bool aa, bool bb, GateType gt)
	{


		u8 g = static_cast<u8>(gt);
		auto g1 = (aa) * (((g & 2) >> 1) | ((g & 8) >> 2))
			+ (1 ^ aa) * ((g & 1) | ((g & 4) >> 1));
		auto g2 = u8(gt) >> (2 * bb) & 3;
		auto ret = ((constB ^ 1) * g1 | constB * g2);
		{
			u8 subgate;

			if (constB) {
				subgate = u8(gt) >> (2 * bb) & 3;
			}
			else {
				u8 g = static_cast<u8>(gt);

				auto val = aa;
				subgate = val
					? ((g & 2) >> 1) | ((g & 8) >> 2)
					: (g & 1) | ((g & 4) >> 1);
			}
			if (subgate != ret)
				throw std::runtime_error(LOCATION);
		}
		return ret;
	}


	block ShGcRuntime::garbleConstGate(bool constA, bool constB, const std::array<block, 2>& in, const GateType& gt, const block& xorOffset)
	{
		auto aa = PermuteBit(in[0]);
		auto bb = PermuteBit(in[1]);

		if (constA && constB) {
			return ShGcRuntime::mPublicLabels[GateEval(gt, aa, bb)];
		}
		else {
			auto v = subGate(constB, aa, bb, gt);
			return _mm_or_si128(ShGcRuntime::mPublicLabels[v / 3], (in[constA] & zeroAndAllOne[v > 0])) ^ (zeroAndAllOne[v == 1] & xorOffset);
		}
	}

	block ShGcRuntime::evaluateConstGate(bool constA, bool constB, const std::array<block, 2>& in, const GateType& gt)
	{
		auto aa = PermuteBit(in[0]);
		auto bb = PermuteBit(in[1]);
		if (constA && constB) {
			return ShGcRuntime::mPublicLabels[GateEval(gt, aa, bb)];
		}
		else {
			auto v = subGate(constB, aa, bb, gt);
			return _mm_or_si128(ShGcRuntime::mPublicLabels[v / 3], (in[constA] & zeroAndAllOne[v > 0]));
		}
	}

	
	namespace { // tell the compiler that this is a local class, perfectly suited for inlining!
		class DynamicScheduler {
		public:
			void enqueue(BetaGate item) { queue.push_back(item); }
			bool traps(const BetaGate& gateIn) {
				bool enableRight = gateIn.mType != GateType::a;
				for (const auto& item : queue) {
					if (gateIn.mInput[0] == item.mOutput || (enableRight && gateIn.mInput[1] == item.mOutput))
						return true;
				}

				return false;
			}
			const std::vector<BetaGate>& getQueue() { return queue; }
			void clearQueue() { queue.clear(); }


		private:
			std::vector<BetaGate> queue;
		};

		__attribute__((target("avx512f,vaes,avx512vl")))
		static __m512i inserter(__m512i base, __m128i data, u8 pos) {
			switch (pos & 0x03) {
			case 0:
				return _mm512_inserti32x4(base, data, 0);
			case 1:
				return _mm512_inserti32x4(base, data, 1);
			case 2:
				return _mm512_inserti32x4(base, data, 2);
			case 3:
				return _mm512_inserti32x4(base, data, 3);
			}
		}

		__attribute__((target("avx512f,vaes,avx512vl")))
			static __m128i extractor(__m512i base, u8 pos) {
			switch (pos & 0x03) {
			case 0:
				return _mm512_extracti32x4_epi32(base, 0);
			case 1:
				return _mm512_extracti32x4_epi32(base, 1);
			case 2:
				return _mm512_extracti32x4_epi32(base, 2);
			case 3:
				return _mm512_extracti32x4_epi32(base, 3);
			}
		}
		
		template<size_t WIDTH>
		static __attribute__((target("avx512f,vaes,avx512vl")))
			void EvaluateANDGates(
				const std::vector<BetaGate>& queue,
				const span<block>& wires,
				std::array<block, 2>& tweaks,
				span<GarbledGate<2>>::iterator& garbledGateIter
			)
		{
			//constexpr size_t WIDTH = 8;
			static_assert(WIDTH % 4 == 0, "requires width multiple of 4");
			constexpr size_t NUM_REGISTERS = WIDTH / 2;
			constexpr size_t NUM_MASKS = WIDTH / 4;

			const __m512i base_offset = _mm512_set_epi64(0, 3, 0, 2, 0, 1, 0, 0);
			const __m512i additive_offset = _mm512_set_epi64(0, 4, 0, 4, 0, 4, 0, 4);

			__m512i data[NUM_REGISTERS];
			__m512i whitening[NUM_REGISTERS];
			__m128i* targetGateKey[WIDTH];
			__m512i postMask[NUM_MASKS];

			__m512i keys[11];
			for (size_t i = 0; i < 11; ++i)
				keys[i] = _mm512_broadcast_i32x4(mAesFixedKey.mRoundKey[i]);

			//std::cout << "post-key load" << std::endl;

			__m512i wideTweaks[2];
			for (size_t i = 0; i < 2; ++i) {
				wideTweaks[i] = _mm512_broadcast_i32x4(tweaks[i]);
				wideTweaks[i] = _mm512_add_epi64(wideTweaks[i], base_offset);
			}

			//std::cout << "post-tweak load" << std::endl;

			for (size_t progress = 0; progress < queue.size(); ) {
				const size_t batch_size = std::min(WIDTH, queue.size() - progress);
				__mmask8 processing_masks[NUM_MASKS];
				{
					size_t vals_left = batch_size;
					for (size_t r = 0; r < NUM_MASKS; ++r) {
						processing_masks[r] = vals_left >= 4 ? 0xFF : ((1 << (2 * vals_left)) - 1);
						//if (processing_masks[r] == 1)
						//	std::cout << "WTF, mask was 1, " << vals_left << " was vals_left" << std::endl;
						//std::cout << "mask #" << r << " is " << (short) processing_masks[r] << std::endl;
						vals_left -= std::min(size_t(4), vals_left);
						
					}
				}
				//std::cout << "post-mask gen, queue size = "<< queue.size() << std::endl;
				//std::cout << "progress = " << progress << std::endl;

				__m128i* tableptr = nullptr; // safe whenever we actually access it

				for (size_t r = 0; r < NUM_MASKS; ++r) {
					for (size_t l = 0; l < 4; ++l) {
						const size_t currentQueuePos = progress + 4 * r + l;
						const __mmask8 pmask = (processing_masks[r] >> (2*l)) & 0x03;
						const BetaGate gate = queue[std::min(queue.size()-1, currentQueuePos)];
						__m128i lleft = _mm_maskz_load_epi64(pmask, wires.data() + gate.mInput[0]);
						__m128i lright = _mm_maskz_load_epi64(pmask, wires.data() + gate.mInput[1]);
						targetGateKey[4 * r + l] = wires.data() + gate.mOutput;
						//std::cout << "post-gate loads" << std::endl;

						const u8 lpb = _mm_extract_epi8(lleft, 0) & 1;
						const __mmask8 lpm = (lpb << 1) | lpb;
						const u8 rpb = _mm_extract_epi8(lright, 0) & 1;
						const __mmask8 rpm = (rpb << 1) | rpb;				
						
						//std::cout << "post iter access" << std::endl;
						if (currentQueuePos < queue.size())
						{
							auto& garbledTable = garbledGateIter->mGarbledTable;
							tableptr = garbledTable.data();
							garbledGateIter++;
						}
						//std::cout << "post-iter increment" << std::endl;
						__m128i firstTable = _mm_maskz_load_epi64(pmask & lpm, tableptr);
						__m128i secondTable = _mm_maskz_load_epi64(pmask & rpm, tableptr + 1);
						secondTable = _mm_maskz_xor_epi64(rpm, lleft, secondTable);
						__m128i lmask = _mm_xor_si128(firstTable, secondTable);
						//std::cout << "post-table loads" << std::endl;

						data[2 * r] = inserter(data[2 * r], lleft, l);
						data[2 * r + 1] = inserter(data[2 * r + 1], lright, l);
						postMask[r] = inserter(postMask[r], lmask, l);
						//std::cout << "post-insertions" << std::endl;
					}

					//std::cout << "pre wide preprocessing" << std::endl;

					// post insertions

					
					//std::cout << "post preprocessing" << std::endl;
				}

				for (size_t w=0; w < NUM_REGISTERS; ++w) {
					data[w] = _mm512_slli_epi64(data[w], 1);
					data[w] = _mm512_xor_si512(data[w], wideTweaks[w%2]);
					wideTweaks[w%2] = _mm512_add_epi64(wideTweaks[w%2], additive_offset);
					whitening[w] = data[w];
					data[w] = _mm512_xor_si512(data[w], keys[0]);
				}

				for (size_t r = 1; r < 10; ++r)
					for (size_t w = 0; w < NUM_REGISTERS; ++w)
						data[w] = _mm512_aesenc_epi128(data[w], keys[r]);
				for (size_t w = 0; w < NUM_REGISTERS; ++w) {
					data[w] = _mm512_aesenclast_epi128(data[w], keys[10]);
					data[w] = _mm512_xor_si512(data[w], whitening[w]);
				}
				//std::cout << "post AES" << std::endl;

				for (size_t r = 0; r < NUM_MASKS; ++r) {
					__m512i combined = _mm512_xor_si512(data[2 * r], data[2 * r + 1]);
					combined = _mm512_xor_si512(combined, postMask[r]);
					//std::cout << "post wide postprocessing" << std::endl;
					for (size_t l = 0; l < 4; ++l) {
						const __m128i extracted = extractor(combined, l);
						//bool bad = (progress + 4 * r + l >= queue.size()) && (((processing_masks[r] >> (2 * l)) & 0x03) != 0);
						//if (bad)
						//	std::cout << "dodo, mask was "<<(short)(processing_masks[r] >> (2 * l)) << std::endl;
						//if(progress +4*r+l<queue.size())
							_mm_mask_store_epi64(targetGateKey[4 * r + l], processing_masks[r] >> (2 * l), extracted);
						
					}
					//std::cout << "post store" << std::endl;
				}

				progress += batch_size;
			}

			const __m128i tweak_diff = _mm_set_epi64x(0, queue.size());
			tweaks[0] += tweak_diff;
			tweaks[1] += tweak_diff;
			//std::cout << "post-tweak write" << std::endl;
		}
		
	}

	void ShGcRuntime::evaluate(
		const BetaCircuit & cir,
		const span<block>& wires,
		std::array<block, 2>& tweaks,
		const span<GarbledGate<2>>& garbledGates,
		const std::function<bool()>& getAuxilaryBit,
		block* DEBUG_labels)
	{
		u64 i = 0;
		auto garbledGateIter = garbledGates.begin();
		std::array<block, 2> in;
		//std::cout  << IoStream::lock;

		//u64 i = 0;

		block hashs[2], temp[2],
			zeroAndGarbledTable[2][2]
		{ { ZeroBlock,ZeroBlock },{ ZeroBlock,ZeroBlock } };

		DynamicScheduler scheduler;

		for (const auto& gate : cir.mGates)
		{

			auto& gt = gate.mType;


			if (scheduler.traps(gate)) {
				if(scheduler.getQueue().size() > 4)
					EvaluateANDGates<8>(scheduler.getQueue(), wires, tweaks, garbledGateIter);
				else
					EvaluateANDGates<4>(scheduler.getQueue(), wires, tweaks, garbledGateIter);
				scheduler.clearQueue();
			}

			if (GSL_LIKELY(gt != GateType::a))
			{
				auto a = wires[gate.mInput[0]];
				auto b = wires[gate.mInput[1]];
				auto& c = wires[gate.mOutput];
				auto constA = isConstLabel(a);
				auto constB = isConstLabel(b);
				auto constAB = constA || constB;
				

				if (GSL_LIKELY(!constAB))
				{

					if (GSL_LIKELY(gt == GateType::Xor || gt == GateType::Nxor))
					{

						if (GSL_LIKELY(neq(a, b)))
						{
							c = a ^ b;
						}
						else
						{
							c = mPublicLabels[getAuxilaryBit()];
						}
#ifndef  NDEBUG
						if (DEBUG_labels) DEBUG_labels[i++] = c;
#endif
					}
					else
					{
						scheduler.enqueue(gate);
						// compute the hashs
						/*hashs[0] = _mm_slli_epi64(a, 1) ^ tweaks[0];
						hashs[1] = _mm_slli_epi64(b, 1) ^ tweaks[1];
						mAesFixedKey.ecbEncTwoBlocks(hashs, temp);
						hashs[0] = temp[0] ^ hashs[0];
						hashs[1] = temp[1] ^ hashs[1];

						// increment the tweaks
						tweaks[0] = tweaks[0] + OneBlock;
						tweaks[1] = tweaks[1] + OneBlock;

						auto& garbledTable = garbledGateIter++->mGarbledTable;
						zeroAndGarbledTable[1][0] = garbledTable[0];
						zeroAndGarbledTable[1][1] = garbledTable[1] ^ a;

						// compute the output wire label
						c = hashs[0] ^
							hashs[1] ^
							zeroAndGarbledTable[PermuteBit(a)][0] ^
							zeroAndGarbledTable[PermuteBit(b)][1];*/

						//std::cout  << "e " << i++ << gateToString(gate.mType) << std::endl <<
						//    " gt  " << garbledTable[0] << "  " << garbledTable[1] << std::endl <<
						//    " t   " << tweaks[0] << "  " << tweaks[1] << std::endl <<
						//    " a   " << a << std::endl <<
						//    " b   " << b << std::endl <<
						//    " c   " << c << std::endl;
#ifndef  NDEBUG
						if (DEBUG_labels) DEBUG_labels[i++] = c;
#endif
					}

					//if (i == 2 && gt != GateType::a)
					//{
					//    std::cout << "e a " << a << " b " << b << " " << gateToString(gt) << " " << c << std::endl;

					//}
				}
				else
				{
					in[0] = a; in[1] = b;
					c = evaluateConstGate(constA, constB, in, gt);

#ifndef  NDEBUG
					auto ab = constA ? b : a;
					if (isConstLabel(c) == false &&
						neq(c, ab))
						throw std::runtime_error(LOCATION);
					if (DEBUG_labels) DEBUG_labels[i++] = c;
#endif
				}
			}
			else
			{
				u64 src = gate.mInput[0];
				u64 len = gate.mInput[1];
				u64 dest = gate.mOutput;

				memcpy(&*(wires.begin() + dest), &*(wires.begin() + src), i32(len * sizeof(block)));
			}
		}

		if (scheduler.getQueue().size() > 4)
			EvaluateANDGates<8>(scheduler.getQueue(), wires, tweaks, garbledGateIter);
		else
			EvaluateANDGates<4>(scheduler.getQueue(), wires, tweaks, garbledGateIter);
		scheduler.clearQueue();

		//std::cout  << IoStream::unlock;
		for (u64 i = 0; i < cir.mOutputs.size(); ++i)
		{
			auto& out = cir.mOutputs[i].mWires;

			for (u64 j = 0; j < out.size(); ++j)
			{
				if (cir.mWireFlags[out[j]] == BetaWireFlag::InvWire)
				{
					if (isConstLabel(wires[out[j]]))
						wires[out[j]] = wires[out[j]] ^ mPublicLabels[1];
				}
			}
		}
	}

	void ShGcRuntime::garble(
		const BetaCircuit& cir,
		const span<block>& wires,
		std::array<block, 2>& tweaks,
		const span<GarbledGate<2>>& gates,
		const std::array<block, 2>& mZeroAndGlobalOffset,
		std::vector<u8>& auxilaryBits,
		block* DEBUG_labels)
	{
		//auto s = DEBUG_labels;
		u64 i = 0;
		auto gateIter = gates.begin();
		//std::cout  << IoStream::lock;
		std::array<block, 2> in;
		//u64 i = 0;
		auto& mGlobalOffset = mZeroAndGlobalOffset[1];
		//std::cout << mZeroAndGlobalOffset[0] << " " << mZeroAndGlobalOffset[1] << std::endl;

		u8 aPermuteBit, bPermuteBit, bAlphaBPermute, cPermuteBit;
		block hash[4], temp[4];

		for (const auto& gate : cir.mGates)
		{


			auto& gt = gate.mType;


			if (GSL_LIKELY(gt != GateType::a))
			{
				auto a = wires[gate.mInput[0]];
				auto b = wires[gate.mInput[1]];
				auto bNot = b ^ mGlobalOffset;

				auto& c = wires[gate.mOutput];
				auto constA = isConstLabel(a);
				auto constB = isConstLabel(b);
				auto constAB = constA || constB;

				if (GSL_LIKELY(!constAB))
				{
					if (GSL_LIKELY(gt == GateType::Xor || gt == GateType::Nxor))
					{
						// is a == b^1
						auto oneEq = eq(a, bNot);
						if (GSL_LIKELY(!(eq(a, b) || oneEq)))
						{
							c = a ^ b ^ mZeroAndGlobalOffset[(u8)gt & 1];
						}
						else
						{
							u8 bit = oneEq ^ ((u8)gt & 1);
							c = mPublicLabels[bit];

							// must tell the evaluator what the bit is.
							auxilaryBits.push_back(bit);
						}
#ifndef  NDEBUG
						if (DEBUG_labels) DEBUG_labels[i++] = c;
#endif
					}
					else
					{
#ifndef  NDEBUG
						Expects(!(gt == GateType::a ||
							gt == GateType::b ||
							gt == GateType::na ||
							gt == GateType::nb ||
							gt == GateType::One ||
							gt == GateType::Zero));
#endif // ! NDEBUG

						// compute the gate modifier variables
						auto& aAlpha = gate.mAAlpha;
						auto& bAlpha = gate.mBAlpha;
						auto& cAlpha = gate.mCAlpha;

						//signal bits of wire 0 of input0 and wire 0 of input1
						aPermuteBit = PermuteBit(a);
						bPermuteBit = PermuteBit(b);
						bAlphaBPermute = bAlpha ^ bPermuteBit;
						cPermuteBit = ((aPermuteBit ^ aAlpha) && (bAlphaBPermute)) ^ cAlpha;

						// compute the hashs of the wires as H(x) = AES_f( x * 2 ^ tweak) ^ (x * 2 ^ tweak)
						hash[0] = _mm_slli_epi64(a, 1) ^ tweaks[0];
						hash[1] = _mm_slli_epi64((a ^ mGlobalOffset), 1) ^ tweaks[0];
						hash[2] = _mm_slli_epi64(b, 1) ^ tweaks[1];
						hash[3] = _mm_slli_epi64((bNot), 1) ^ tweaks[1];
						mAesFixedKey.ecbEncFourBlocks(hash, temp);
						hash[0] = hash[0] ^ temp[0]; // H( a0 )
						hash[1] = hash[1] ^ temp[1]; // H( a1 )
						hash[2] = hash[2] ^ temp[2]; // H( b0 )
						hash[3] = hash[3] ^ temp[3]; // H( b1 )

													 // increment the tweaks
						tweaks[0] = tweaks[0] + OneBlock;
						tweaks[1] = tweaks[1] + OneBlock;

						// generate the garbled table
						auto& garbledTable = gateIter++->mGarbledTable;

						// compute the table entries
						garbledTable[0] = hash[0] ^ hash[1] ^ mZeroAndGlobalOffset[bAlphaBPermute];
						garbledTable[1] = hash[2] ^ hash[3] ^ a ^ mZeroAndGlobalOffset[aAlpha];

						//std::cout  << "g "<<i<<" " << garbledTable[0] << " " << garbledTable[1] << std::endl;


						// compute the out wire
						c = hash[aPermuteBit] ^
							hash[2 ^ bPermuteBit] ^
							mZeroAndGlobalOffset[cPermuteBit];

						//std::cout  << "g " << i++  << gateToString(gate.mType) << std::endl <<
						//    " gt  " << garbledTable[0] << "  " << garbledTable[1] << std::endl <<
						//    " t   " << tweaks[0] << "  " << tweaks[1] << std::endl <<
						//    " a   " << a << "  " << (a ^ mGlobalOffset) << std::endl <<
						//    " b   " << b << "  " << (b ^ mGlobalOffset) << std::endl <<
						//    " c   " << c << "  " << (c ^ mGlobalOffset) << std::endl;
#ifndef  NDEBUG
						if (DEBUG_labels) DEBUG_labels[i++] = c;
#endif
					}
				}
				else
				{
					auto ab = constA ? b : a;

					in[0] = a; in[1] = b;
					c = garbleConstGate(constA, constB, in, gt, mGlobalOffset);

#ifndef  NDEBUG
					if (isConstLabel(c) == false &&
						neq(c, ab) &&
						neq(c, ab ^ mGlobalOffset))
						throw std::runtime_error(LOCATION);

					if (DEBUG_labels) DEBUG_labels[i++] = c;
#endif

				}

				//if (i == 2 && gt != GateType::a)
				//{
				//    std::cout << "g a " << a << " b " << b << " " << gateToString(gt) << " " << c << " " << (c ^ mGlobalOffset) << std::endl;

				//}
			}
			else
			{
				u64 src = gate.mInput[0];
				u64 len = gate.mInput[1];
				u64 dest = gate.mOutput;

				memcpy(&*(wires.begin() + dest), &*(wires.begin() + src), u32(len * sizeof(block)));
			}

		}

		for (u64 i = 0; i < cir.mOutputs.size(); ++i)
		{
			auto& out = cir.mOutputs[i].mWires;

			for (u64 j = 0; j < out.size(); ++j)
			{
				if (cir.mWireFlags[out[j]] == BetaWireFlag::InvWire)
				{
					if (isConstLabel(wires[out[j]]))
						wires[out[j]] = wires[out[j]] ^ mPublicLabels[1];
					else
						wires[out[j]] = wires[out[j]] ^ mGlobalOffset;
				}
			}
		}
		//std::cout  << IoStream::unlock;

	}


}