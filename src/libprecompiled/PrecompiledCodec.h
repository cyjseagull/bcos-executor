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
 * @file PrecompiledCodec.h
 * @author: kyonRay
 * @date 2021-06-02
 */

#pragma once

#include <bcos-framework/libcodec/abi/ContractABICodec.h>
#include <bcos-framework/libcodec/scale/Scale.h>

namespace bcos
{
namespace precompiled
{
enum VMType
{
    EVM,
    WASM,
    UNDEFINED
};
class PrecompiledCodec
{
public:
    using Ptr = std::shared_ptr<PrecompiledCodec>;
    PrecompiledCodec(crypto::Hash::Ptr _hash, bool _isWasm) : m_abi(_hash), m_hash(_hash)
    {
        m_type = _isWasm ? VMType::WASM : VMType::EVM;
    }
    template <typename... Args>
    bytes encode(Args&&... _args)
    {
        assert(m_type != VMType::UNDEFINED);
        if (m_type == VMType::EVM)
        {
            return m_abi.abiIn("", _args...);
        }
        else
        {
            codec::scale::ScaleEncoderStream s;
            (s << ... << std::forward<Args>(_args));
            return s.data();
        }
    }
    template <typename... Args>
    bytes encodeWithSig(const std::string& _sig, Args&&... _args)
    {
        assert(m_type != VMType::UNDEFINED);
        if (m_type == VMType::EVM)
        {
            return m_abi.abiIn(_sig, _args...);
        }
        else
        {
            codec::scale::ScaleEncoderStream s;
            (s << ... << std::forward<Args>(_args));
            return m_hash->hash(_sig).ref().getCroppedData(0, 4).toBytes() + s.data();
        }
    }

    bytes encodeWithSig(const std::string& _sig)
    {
        assert(m_type != VMType::UNDEFINED);
        if (m_type == VMType::EVM)
        {
            return m_abi.abiIn(_sig);
        }
        else
        {
            codec::scale::ScaleEncoderStream s;
            return m_hash->hash(_sig).ref().getCroppedData(0, 4).toBytes() + s.data();
        }
    }

    // TODO: test check this decode
    template <typename... T>
    void decode(bytesConstRef _data, T&... _t)
    {
        assert(m_type != VMType::UNDEFINED);
        if (m_type == VMType::EVM)
        {
            m_abi.abiOut(_data, _t...);
        }
        else if (m_type == VMType::WASM)
        {
            auto&& t = _data.toBytes();
            codec::scale::ScaleDecoderStream stream(gsl::make_span(t));
            decodeScale(stream, _t...);
        }
    }
    template <typename T, typename... U>
    void decodeScale(codec::scale::ScaleDecoderStream& _s, T& _t, U&... _u)
    {
        _s >> _t;
        decodeScale(_s, _u...);
    }
    template <typename T>
    void decodeScale(codec::scale::ScaleDecoderStream& _s, T& _t)
    {
        _s >> _t;
    }

    void decodeScale(codec::scale::ScaleDecoderStream&) { return; }

    VMType getVMType() const { return m_type; }
    void setVMType(bool _isWasm) { m_type = _isWasm ? VMType::WASM : VMType::EVM; }

private:
    VMType m_type = VMType::UNDEFINED;
    codec::abi::ContractABICodec m_abi;
    crypto::Hash::Ptr m_hash;
};
}  // namespace precompiled
}  // namespace bcos