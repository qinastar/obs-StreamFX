// FFMPEG Video Encoder Integration for OBS Studio
// Copyright (c) 2019 Michael Fabian Dirks <info@xaymar.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "debug_handler.hpp"
#include "common.hpp"
#include "handler.hpp"
#include "plugin.hpp"

#include "warning-disable.hpp"
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "warning-enable.hpp"

extern "C" {
#include "warning-disable.hpp"
#include <libavutil/opt.h>
#include "warning-enable.hpp"
}

using namespace streamfx::encoder::ffmpeg::handler;

void debug_handler::get_defaults(obs_data_t*, const AVCodec*, AVCodecContext*, bool) {}

template<typename T>
std::string to_string(T value)
{
	return std::string("Error: to_string not implemented for this type!");
};

template<>
std::string to_string(int64_t value)
{
	std::vector<char> buf(32);
	snprintf(buf.data(), buf.size(), "%" PRId64, value);
	return std::string(buf.data(), buf.data() + buf.size());
}

template<>
std::string to_string(uint64_t value)
{
	std::vector<char> buf(32);
	snprintf(buf.data(), buf.size(), "%" PRIu64, value);
	return std::string(buf.data(), buf.data() + buf.size());
}

template<>
std::string to_string(double_t value)
{
	std::vector<char> buf(32);
	snprintf(buf.data(), buf.size(), "%f", value);
	return std::string(buf.data(), buf.data() + buf.size());
}

void debug_handler::get_properties(obs_properties_t*, const AVCodec* codec, AVCodecContext* context, bool)
{
	if (context)
		return;

	AVCodecContext* ctx = avcodec_alloc_context3(codec);
	if (!ctx->priv_data) {
		avcodec_free_context(&ctx);
		return;
	}

	DLOG_INFO("Options for '%s':", codec->name);

	std::pair<AVOptionType, std::string> opt_type_name[] = {
		{AV_OPT_TYPE_FLAGS, "Flags"},
		{AV_OPT_TYPE_INT, "Int"},
		{AV_OPT_TYPE_INT64, "Int64"},
		{AV_OPT_TYPE_DOUBLE, "Double"},
		{AV_OPT_TYPE_FLOAT, "Float"},
		{AV_OPT_TYPE_STRING, "String"},
		{AV_OPT_TYPE_RATIONAL, "Rational"},
		{AV_OPT_TYPE_BINARY, "Binary"},
		{AV_OPT_TYPE_DICT, "Dictionary"},
		{AV_OPT_TYPE_UINT64, "Unsigned Int64"},
		{AV_OPT_TYPE_CONST, "Constant"},
		{AV_OPT_TYPE_IMAGE_SIZE, "Image Size"},
		{AV_OPT_TYPE_PIXEL_FMT, "Pixel Format"},
		{AV_OPT_TYPE_SAMPLE_FMT, "Sample Format"},
		{AV_OPT_TYPE_VIDEO_RATE, "Video Rate"},
		{AV_OPT_TYPE_DURATION, "Duration"},
		{AV_OPT_TYPE_COLOR, "Color"},
		{AV_OPT_TYPE_CHANNEL_LAYOUT, "Layout"},
		{AV_OPT_TYPE_BOOL, "Bool"},
	};
	std::map<std::string, AVOptionType> unit_types;

	const AVOption* opt = nullptr;
	while ((opt = av_opt_next(ctx->priv_data, opt)) != nullptr) {
		std::string type_name = "";
		for (auto kv : opt_type_name) {
			if (opt->type == kv.first) {
				type_name = kv.second;
				break;
			}
		}

		if (opt->type == AV_OPT_TYPE_CONST) {
			if (opt->unit == nullptr) {
				DLOG_INFO("  Constant '%s' and help text '%s' with unknown settings.", opt->name, opt->help);
			} else {
				auto unit_type = unit_types.find(opt->unit);
				if (unit_type == unit_types.end()) {
					DLOG_INFO("  [%s] Flag '%s' and help text '%s' with value '%" PRId64 "'.", opt->unit, opt->name,
							  opt->help, opt->default_val.i64);
				} else {
					std::string out;
					switch (unit_type->second) {
					case AV_OPT_TYPE_BOOL:
						out = opt->default_val.i64 ? "true" : "false";
						break;
					case AV_OPT_TYPE_INT:
						out = to_string(opt->default_val.i64);
						break;
					case AV_OPT_TYPE_UINT64:
						out = to_string(static_cast<uint64_t>(opt->default_val.i64));
						break;
					case AV_OPT_TYPE_FLAGS:
						out = to_string(static_cast<uint64_t>(opt->default_val.i64));
						break;
					case AV_OPT_TYPE_FLOAT:
					case AV_OPT_TYPE_DOUBLE:
						out = to_string(opt->default_val.dbl);
						break;
					case AV_OPT_TYPE_STRING:
						out = opt->default_val.str;
						break;
					default:
						break;
					}

					DLOG_INFO("  [%s] Constant '%s' and help text '%s' with value '%s'.", opt->unit, opt->name,
							  opt->help, out.c_str());
				}
			}
		} else {
			if (opt->unit != nullptr) {
				unit_types.emplace(opt->name, opt->type);
			}

			std::string minimum = "", maximum = "", out;
			minimum = to_string(opt->min);
			maximum = to_string(opt->max);
			{
				switch (opt->type) {
				case AV_OPT_TYPE_BOOL:
					out = opt->default_val.i64 ? "true" : "false";
					break;
				case AV_OPT_TYPE_INT:
					out = to_string(opt->default_val.i64);
					break;
				case AV_OPT_TYPE_UINT64:
					out = to_string(static_cast<uint64_t>(opt->default_val.i64));
					break;
				case AV_OPT_TYPE_FLAGS:
					out = to_string(static_cast<uint64_t>(opt->default_val.i64));
					break;
				case AV_OPT_TYPE_FLOAT:
				case AV_OPT_TYPE_DOUBLE:
					out = to_string(opt->default_val.dbl);
					break;
				case AV_OPT_TYPE_STRING:
					out = opt->default_val.str ? opt->default_val.str : "<invalid>";
					break;
				default:
					break;
				}
			}

			DLOG_INFO(
				"  Option '%s'%s%s%s with help '%s' of type '%s' with default value '%s', minimum '%s' and maximum "
				"'%s'.",
				opt->name, opt->unit ? " with unit (" : "", opt->unit ? opt->unit : "", opt->unit ? ")" : "", opt->help,
				type_name.c_str(), out.c_str(), minimum.c_str(), maximum.c_str());
		}
	}
}

void debug_handler::update(obs_data_t*, const AVCodec*, AVCodecContext*) {}
