/*
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @brief block level context
 * @file BlockContext.h
 * @author: xingqiangbai
 * @date: 2021-05-27
 */
#pragma once

#include "bcos-framework/interfaces/dispatcher/DispatcherInterface.h"
#include "bcos-framework/interfaces/executor/ExecutorInterface.h"
#include "bcos-framework/interfaces/ledger/LedgerInterface.h"
#include "bcos-framework/interfaces/protocol/Block.h"
#include "bcos-framework/interfaces/protocol/BlockFactory.h"
#include "bcos-framework/interfaces/protocol/Transaction.h"
#include "bcos-framework/interfaces/protocol/TransactionReceipt.h"
#include "bcos-framework/interfaces/storage/StorageInterface.h"
#include <boost/function.hpp>
#include <algorithm>
#include <functional>
#include <future>
#include <memory>
#include <thread>

namespace bcos
{
DERIVE_BCOS_EXCEPTION(InvalidBlockWithBadRoot);
DERIVE_BCOS_EXCEPTION(BlockExecutionFailed);

class ThreadPool;
namespace protocol
{
class TransactionReceipt;

}  // namespace protocol

namespace executor
{
class Executive;
class BlockContext;
class PrecompiledContract;

enum ExecutorVersion : int32_t
{
    Version_3_0_0 = 1,
};
class Executor : public ExecutorInterface
{
public:
    typedef std::shared_ptr<Executor> Ptr;
    typedef std::function<crypto::HashType(protocol::BlockNumber x)> CallBackFunction;
    Executor(const protocol::BlockFactory::Ptr& _blockFactory,
        const dispatcher::DispatcherInterface::Ptr& _dispatcher,
        const ledger::LedgerInterface::Ptr& _ledger,
        const storage::StorageInterface::Ptr& _stateStorage, bool _isWasm, size_t _poolSize = 2);

    virtual ~Executor() { stop(); }

    std::shared_ptr<BlockContext> executeBlock(const protocol::Block::Ptr& block);

    protocol::TransactionReceipt::Ptr executeTransaction(
        protocol::Transaction::ConstPtr _t, std::shared_ptr<executor::Executive> executive);

    void asyncGetCode(const std::string_view& _address,
        std::function<void(const Error::Ptr&, const std::shared_ptr<bytes>&)> _callback) override;
    void asyncExecuteTransaction(const protocol::Transaction::ConstPtr& _tx,
        std::function<void(const Error::Ptr&, const protocol::TransactionReceipt::ConstPtr&)>
            _callback) override;
    void stop() override;
    void start() override;

    std::shared_ptr<BlockContext> createExecutiveContext(
        const protocol::BlockHeader::Ptr& _currentHeader);

private:
    protocol::BlockHeader::Ptr getLatestHeaderFromStorage();
    protocol::BlockNumber getLatestBlockNumberFromStorage();
    Error::Ptr resultNotifier(const Error::Ptr& _error, const protocol::BlockHeader::Ptr _header)
    {
        // async notify dispatcher
        m_dispatcher->asyncNotifyExecutionResult(
            _error, _header, [_header](const Error::Ptr& error) {
                if (_error)
                {
                    EXECUTOR_LOG(WARNING) << LOG_DESC("asyncNotifyExecutionResult failed")
                                          << LOG_KV("code", _error->errorCode())
                                          << LOG_KV("msg", _error->errorMessage())
                                          << LOG_KV("number", _header->number())
                                          << LOG_KV("hash", _header->hash().abridged());
                    return;
                }
                EXECUTOR_LOG(INFO) << LOG_DESC("asyncNotifyExecutionResult success")
                                   << LOG_KV("number", _header->number())
                                   << LOG_KV("hash", _header->hash().abridged());
            });
    };
    protocol::BlockFactory::Ptr m_blockFactory;
    dispatcher::DispatcherInterface::Ptr m_dispatcher;
    ledger::LedgerInterface::Ptr m_ledger;
    storage::TableFactoryInterface::Ptr m_tableFactory;
    storage::StorageInterface::Ptr m_stateStorage;
    std::shared_ptr<ThreadPool> m_threadPool = nullptr;
    CallBackFunction m_pNumberHash;
    crypto::Hash::Ptr m_hashImpl;
    unsigned int m_threadNum = -1;
    bool m_isWasm = false;
    std::atomic_bool m_stop = {false};
    protocol::BlockHeader::Ptr m_lastHeader;
    std::unique_ptr<std::thread> m_worker = nullptr;
    const ExecutorVersion m_version;
    std::map<std::string, std::shared_ptr<PrecompiledContract>> m_precompiledContract;

    // try to refetch the block every 3seconds
    unsigned m_fetchBlockTimeout = 3000;
};

}  // namespace executor

}  // namespace bcos
