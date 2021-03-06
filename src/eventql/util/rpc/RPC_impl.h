/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include "eventql/util/reflect/reflect.h"

template <typename ResultType, typename ArgPackType>
RPC<ResultType, ArgPackType>::RPC(
  const std::string& method,
  const ArgPackType& args) :
  AnyRPC(method),
  args_(args) {}

template <typename ResultType, typename ArgPackType>
void RPC<ResultType, ArgPackType>::success(
    ScopedPtr<ResultType> result) noexcept {
  result_ = std::move(result);
  AnyRPC::ready();
}

template <typename ResultType, typename ArgPackType>
const ArgPackType& RPC<ResultType, ArgPackType>::args() const {
  return args_;
}

template <typename ResultType, typename ArgPackType>
const ResultType& RPC<ResultType, ArgPackType>::result() const {
  status_.raiseIfError();
  return *result_;
}

template <typename ResultType, typename ArgPackType>
void RPC<ResultType, ArgPackType>::onSuccess(
    Function<void(const RPC<ResultType, ArgPackType>& rpc)> fn) {
  onReady([this, fn] {
    if (this->isSuccess()) {
      fn(*this);
    }
  });
}

template <typename ResultType, typename ArgPackType>
void RPC<ResultType, ArgPackType>::onError(
    Function<void(const Status& status)> fn) {
  onReady([this, fn] {
    if (this->isFailure()) {
      fn(this->status());
    }
  });
}

template <typename ResultType, typename ArgPackType>
template <typename Codec>
void RPC<ResultType, ArgPackType>::encode() {
  Codec::encodeRPCRequest(this, &encoded_request_);
  decode_fn_ = [this] (const Buffer& buffer) {
    Codec::decodeRPCResponse(this, buffer);
  };
}

template <class Codec, class ReturnType, typename... ArgTypes>
AutoRef<RPC<ReturnType, std::tuple<ArgTypes...>>> mkRPC(
    const std::string& method,
    ArgTypes... args) {
  auto rpc = AutoRef<RPC<ReturnType, std::tuple<ArgTypes...>>>(
      new RPC<ReturnType, std::tuple<ArgTypes...>>(
          method,
          std::make_tuple(args...)));

  rpc->template encode<Codec>();
  return rpc;
}

template <class Codec, class MethodCall>
AutoRef<RPC<typename MethodCall::ReturnType, typename MethodCall::ArgPackType>>
    mkRPC(const MethodCall* method, typename MethodCall::ArgPackType args) {
  auto rpc = AutoRef<
      RPC<typename MethodCall::ReturnType, typename MethodCall::ArgPackType>>(
          new RPC<
              typename MethodCall::ReturnType,
              typename MethodCall::ArgPackType>(method->name(), args));

  rpc->template encode<Codec>();
  return rpc;
}

template <
    class Codec,
    typename ClassType,
    typename ReturnType,
    typename... ArgTypes>
AutoRef<RPC<ReturnType, std::tuple<ArgTypes...>>> mkRPC(
  ReturnType (ClassType::* method)(ArgTypes...),
  ArgTypes... args) {
  return mkRPC<Codec>(
      reflect::reflectMethod(method),
      std::tuple<typename std::decay<ArgTypes>::type...>(args...));
}

