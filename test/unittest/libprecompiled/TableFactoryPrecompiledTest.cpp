/**
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
 * @file TableFactoryPrecompiledTest.cpp
 * @author: kyonRay
 * @date 2021-06-21
 */

#include "libprecompiled/TableFactoryPrecompiled.h"
#include "PreCompiledFixture.h"
#include "libprecompiled/extension/UserPrecompiled.h"
#include <bcos-framework/interfaces/storage/TableInterface.h>
#include <bcos-framework/testutils/TestPromptFixture.h>

using namespace bcos;
using namespace bcos::precompiled;
using namespace bcos::executor;
using namespace bcos::storage;
using namespace bcos::ledger;

namespace bcos::test
{
class TableFactoryPrecompiledFixture : public PrecompiledFixture
{
public:
    TableFactoryPrecompiledFixture()
    {
        tableFactoryPrecompiled = std::make_shared<TableFactoryPrecompiled>(hashImpl);
        setIsWasm(true);
        tableFactoryPrecompiled->setMemoryTableFactory(context->getTableFactory());
    }

    virtual ~TableFactoryPrecompiledFixture() {}

    TableFactoryPrecompiled::Ptr tableFactoryPrecompiled;
    int addressCount = 0x10000;
};
BOOST_FIXTURE_TEST_SUITE(precompiledTableFactoryTest, TableFactoryPrecompiledFixture)

BOOST_AUTO_TEST_CASE(createTableTest)
{
    BOOST_TEST(tableFactoryPrecompiled->toString() == "TableFactory");
    bytes param = codec->encodeWithSig("createTable(string,string,string)", std::string("t_test"),
        std::string("id"), std::string("item_name,item_id"));
    auto callResult = tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas);
    bytes out = callResult->execResult();
    s256 errCode;
    codec->decode(&out, errCode);
    BOOST_TEST(errCode == 0);

    out.clear();
    // createTable exist
    param = codec->encodeWithSig("createTable(string,string,string)", std::string("t_test"),
        std::string("id"), std::string("item_name,item_id"));
    callResult = tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas);
    out = callResult->execResult();
    codec->decode(&out, errCode);
    BOOST_TEST(errCode == CODE_TABLE_NAME_ALREADY_EXIST);

    // createTable too long tableName, key and filed
    std::string errorStr;
    for (int i = 0; i <= SYS_TABLE_VALUE_FIELD_MAX_LENGTH; i++)
    {
        errorStr += std::to_string(9);
    }
    BOOST_CHECK(errorStr.size() > (size_t)SYS_TABLE_VALUE_FIELD_MAX_LENGTH);
    param = codec->encodeWithSig("createTable(string,string,string)", errorStr, std::string("id"),
        std::string("item_name,item_id"));
    BOOST_CHECK_THROW(tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas),
        PrecompiledError);

    param = codec->encodeWithSig("createTable(string,string,string)", std::string("t_test"),
        errorStr, std::string("item_name,item_id"));
    BOOST_CHECK_THROW(tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas),
        PrecompiledError);

    param = codec->encodeWithSig(
        "createTable(string,string,string)", std::string("t_test"), std::string("id"), errorStr);
    BOOST_CHECK_THROW(tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas),
        PrecompiledError);

    // createTable error key and filed
    std::string errorStr2 = "test&";
    std::string rightStr = "test@1";
    param = codec->encodeWithSig("createTable(string,string,string)", errorStr2, std::string("id"),
        std::string("item_name,item_id"));
    BOOST_CHECK_THROW(tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas),
        PrecompiledError);

    param = codec->encodeWithSig("createTable(string,string,string)", std::string("t_test"),
        errorStr2, std::string("item_name,item_id"));
    BOOST_CHECK_THROW(tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas),
        PrecompiledError);

    param = codec->encodeWithSig(
        "createTable(string,string,string)", std::string("t_test"), std::string("id"), errorStr2);
    BOOST_CHECK_THROW(tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas),
        PrecompiledError);

    param = codec->encodeWithSig("createTable(string,string,string)", std::string("t_test2"),
        rightStr, std::string("item_name,item_id"));
    callResult = tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas);
    out = callResult->execResult();
    codec->decode(&out, errCode);
    BOOST_TEST(errCode == 0);
}

BOOST_AUTO_TEST_CASE(openTableTest)
{
    // test wasm
    {
        bytes param = codec->encodeWithSig("createTable(string,string,string)",
            std::string("t_test"), std::string("id"), std::string("item_name,item_id"));
        auto callResult =
            tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas);
        bytes out = callResult->execResult();
        s256 errCode;
        codec->decode(&out, errCode);
        BOOST_TEST(errCode == 0);

        param = codec->encodeWithSig("openTable(string)", std::string("t_poor"));
        BOOST_CHECK_THROW(
            tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas),
            PrecompiledError);
        param = codec->encodeWithSig("openTable(string)", std::string("t_test"));
        callResult = tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas);
        out = callResult->execResult();
        std::string addressOut;
        codec->decode(&out, addressOut);
        BOOST_TEST(Address(addressOut) == Address(std::to_string(addressCount + 1)));

        param = codec->encodeWithSig("openTable(string)", std::string("/data/t_test"));
        callResult = tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas);
        out = callResult->execResult();
        codec->decode(&out, addressOut);
        BOOST_TEST(Address(addressOut) == Address(std::to_string(addressCount + 2)));
    }

    setIsWasm(false);
    tableFactoryPrecompiled->setMemoryTableFactory(context->getTableFactory());
    {
        bytes param = codec->encodeWithSig("createTable(string,string,string)",
            std::string("t_test"), std::string("id"), std::string("item_name,item_id"));
        auto callResult =
            tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas);
        bytes out = callResult->execResult();
        s256 errCode;
        codec->decode(&out, errCode);
        BOOST_TEST(errCode == 0);
        param = codec->encodeWithSig("openTable(string)", std::string("t_poor"));
        BOOST_CHECK_THROW(
            tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas),
            PrecompiledError);

        out.clear();
        param = codec->encodeWithSig("openTable(string)", std::string("t_test"));
        callResult = tableFactoryPrecompiled->call(context, bytesConstRef(&param), "", "", gas);
        out = callResult->execResult();
        Address addressOutAddress;
        codec->decode(&out, addressOutAddress);
        auto o1 = std::string((char*)addressOutAddress.data(), 20);
        BOOST_TEST(o1 == "00000000000000065537");
    }
}

BOOST_AUTO_TEST_CASE(hash)
{
    auto h = tableFactoryPrecompiled->hash();
    BOOST_TEST(h == HashType());
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace bcos::test
