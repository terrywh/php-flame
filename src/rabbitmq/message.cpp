#include "../coroutine.h"
#include "rabbitmq.h"
#include "message.h"

namespace flame {
namespace rabbitmq {
	void message::declare(php::extension_entry& ext) {
		php::class_entry<message> class_message("flame\\rabbitmq\\message");
		class_message
			.property({"routing_key", ""})
			.property({"body", ""})
			.property({"expiration", ""})
			.property({"reply_to", ""})
			.property({"correlation_id", ""})
			.property({"priority", 0})
			.property({"delivery_mode", 0})
			.property({"header", nullptr})
			.property({"content_encoding", ""})
			.property({"content_type", ""})
			.property({"cluster_id", ""})
			.property({"app_id", ""})
			.property({"user_id", ""})
			.property({"type_name", ""})
			.property({"timestamp", 0})
			.property({"message_id", ""})
			.method<&message::__construct>("__construct", {
				{"body", php::TYPE::STRING, false, true},
			})
			.method<&message::to_string>("__toString");

		ext.add(std::move(class_message));
	}
	php::value message::__construct(php::parameters& params) {
		if(params.size() > 0) {
			set("body", params[0]);
		}
		if(params.size() > 1) {
			set("routing_key", params[1]);
		}
		return nullptr;
	}
	php::value message::to_json(php::parameters& params) {
		php::array data(16);
		data.set("routing_key", get("routing_key"));
		data.set("body", get("body"));
		data.set("expiration",get("expiration"));
		data.set("reply_to", get("reply_to"));
		data.set("correlation_id", get("correlation_id"));
		data.set("priority", get("priority"));
		data.set("delivery_mode", get("delivery_mode"));
		data.set("header", get("header"));
		data.set("content_encoding", get("content_encoding"));
		data.set("content_type", get("content_type"));
		data.set("cluster_id", get("cluster_id"));
		data.set("app_id", get("app_id"));
		data.set("user_id", get("user_id"));
		data.set("type_name", get("type_name"));
		data.set("timestamp", get("timestamp"));
		data.set("message_id", get("message_id"));
		return std::move(data);
	}
	php::value message::to_string(php::parameters& params) {
		return get("body");
	}
	void message::build_ex(const AMQP::Message& msg, std::uint64_t tag) {
		tag_ = tag;
		set("routing_key", msg.routingkey());
		set("body", php::string(msg.body(), msg.bodySize()));
		if(msg.hasExpiration()) set("expiration", msg.expiration());
		if(msg.hasReplyTo()) set("reply_to", msg.replyTo());
		if(msg.hasCorrelationID()) set("correlation_id", msg.correlationID());
		if(msg.hasPriority()) set("priority", msg.priority());
		if(msg.hasDeliveryMode()) set("delivery_mode", msg.deliveryMode());
		if(msg.hasHeaders()) set("header", convert(msg.headers()));
		if(msg.hasContentEncoding()) set("content_encoding", msg.contentEncoding());
		if(msg.hasContentType()) set("content_type", msg.contentType());
		if(msg.hasClusterID()) set("cluster_id", msg.clusterID());
		if(msg.hasAppID()) set("app_id", msg.appID());
		if(msg.hasUserID()) set("user_id", msg.userID());
		if(msg.hasTypeName()) set("type_name", msg.typeName());
		if(msg.hasTimestamp()) set("timestamp", msg.timestamp());
		if(msg.hasMessageID()) set("message_id", msg.messageID());
	}
	void message::build_ex(AMQP::Envelope& env) {
		php::string v;

		v = get("expiration");
		if(!v.empty()) env.setExpiration(v.to_string());
		v = get("reply_to");
		if(!v.empty()) env.setReplyTo(v.to_string());
		v = get("correlation_id");
		if(!v.empty()) env.setCorrelationID(v.to_string());
		v = get("priority");
		if(!v.empty()) env.setPriority(v.to_integer());
		v = get("delivery_mode");
		if(!v.empty()) env.setDeliveryMode(v.to_integer());
		v = get("header");
		if(!v.empty()) env.setHeaders(convert(v));
		v = get("content_encoding");
		if(!v.empty()) env.setContentEncoding(v.to_string());
		v = get("content_type");
		if(!v.empty()) env.setContentType(v.to_string());
		v = get("cluster_id");
		if(!v.empty()) env.setClusterID(v.to_string());
		v = get("app_id");
		if(!v.empty()) env.setAppID(v.to_string());
		v = get("user_id");
		if(!v.empty()) env.setUserID(v.to_string());
		v = get("type_name");
		if(!v.empty()) env.setTypeName(v.to_string());
		v = get("timestamp");
		if(!v.empty()) env.setTimestamp(v.to_integer());
		v = get("message_id");
		if(!v.empty()) env.setMessageID(v.to_string());
	}
}
}
