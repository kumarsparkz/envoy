#pragma once

#include "envoy/common/exception.h"

#include "extensions/filters/network/kafka/external/serialization_composite.h"
#include "extensions/filters/network/kafka/serialization.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Kafka {

/**
 * Represents fields that are present in every Kafka request message.
 * @see http://kafka.apache.org/protocol.html#protocol_messages
 */
struct RequestHeader {
  int16_t api_key_;
  int16_t api_version_;
  int32_t correlation_id_;
  NullableString client_id_;

  bool operator==(const RequestHeader& rhs) const {
    return api_key_ == rhs.api_key_ && api_version_ == rhs.api_version_ &&
           correlation_id_ == rhs.correlation_id_ && client_id_ == rhs.client_id_;
  };
};

/**
 * Carries information that could be extracted during the failed parse.
 */
class RequestParseFailure {
public:
  RequestParseFailure(const RequestHeader& request_header) : request_header_{request_header} {};

  /**
   * Request's header.
   */
  const RequestHeader request_header_;
};

typedef std::shared_ptr<RequestParseFailure> RequestParseFailureSharedPtr;

/**
 * Abstract Kafka request.
 * Contains data present in every request (the header with request key, version, etc.).
 * @see http://kafka.apache.org/protocol.html#protocol_messages
 */
class AbstractRequest {
public:
  virtual ~AbstractRequest() = default;

  /**
   * Constructs a request with given header data.
   * @param request_header request's header.
   */
  AbstractRequest(const RequestHeader& request_header) : request_header_{request_header} {};

  /**
   * Encode the contents of this message into a given buffer.
   * @param dst buffer instance to keep serialized message
   */
  virtual size_t encode(Buffer::Instance& dst) const PURE;

  /**
   * Request's header.
   */
  const RequestHeader request_header_;
};

typedef std::shared_ptr<AbstractRequest> AbstractRequestSharedPtr;

/**
 * Concrete request that carries data particular to given request type.
 * @param Data concrete request data type.
 */
template <typename Data> class Request : public AbstractRequest {
public:
  /**
   * Request header fields need to be initialized by user in case of newly created requests.
   */
  Request(const RequestHeader& request_header, const Data& data)
      : AbstractRequest{request_header}, data_{data} {};

  /**
   * Encodes given request into a buffer, with any extra configuration carried by the context.
   */
  size_t encode(Buffer::Instance& dst) const override {
    EncodingContext context{request_header_.api_version_};
    size_t written{0};
    // Encode request header.
    written += context.encode(request_header_.api_key_, dst);
    written += context.encode(request_header_.api_version_, dst);
    written += context.encode(request_header_.correlation_id_, dst);
    written += context.encode(request_header_.client_id_, dst);
    // Encode request-specific data.
    written += context.encode(data_, dst);
    return written;
  }

  bool operator==(const Request<Data>& rhs) const {
    return request_header_ == rhs.request_header_ && data_ == rhs.data_;
  };

private:
  const Data data_;
};

} // namespace Kafka
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
