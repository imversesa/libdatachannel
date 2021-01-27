/*
 * libdatachannel streamer example
 * Copyright (c) 2020 Filip Klembara (in2core)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 */

#if RTC_ENABLE_MEDIA

#include "rtcpsenderreporter.hpp"

namespace rtc {

void RtcpSenderReporter::startRecording() {
	_previousReportedTimestamp = rtpConfig->timestamp;
	timeOffset = rtpConfig->startTime_s - rtpConfig->timestampToSeconds(rtpConfig->timestamp);
}

void RtcpSenderReporter::sendReport(uint32_t timestamp) {
	auto sr = getSenderReport(timestamp);
	_previousReportedTimestamp = timestamp;
	senderReportOutgoingCallback(move(sr));
}

void RtcpSenderReporter::addToReport(RTP *rtp, uint32_t rtpSize) {
	packetCount += 1;
	assert(!rtp->padding());
	payloadOctets += rtpSize - rtp->getSize();
}

RtcpSenderReporter::RtcpSenderReporter(std::shared_ptr<RtpPacketizationConfig> rtpConfig)
    : rtpConfig(rtpConfig) {}

uint64_t RtcpSenderReporter::secondsToNTP(double seconds) {
	return std::round(seconds * double(uint64_t(1) << 32));
}

void RtcpSenderReporter::setNeedsToReport() { needsToReport = true; }

message_ptr RtcpSenderReporter::getSenderReport(uint32_t timestamp) {
	auto srSize = RTCP_SR::size(0);
	auto msg = make_message(srSize + RTCP_SDES::size({{uint8_t(rtpConfig->cname.size())}}),
	                        Message::Type::Control);
	auto sr = reinterpret_cast<RTCP_SR *>(msg->data());
	auto timestamp_s = rtpConfig->timestampToSeconds(timestamp);
	auto currentTime = timeOffset + timestamp_s;
	sr->setNtpTimestamp(secondsToNTP(currentTime));
	sr->setRtpTimestamp(timestamp);
	sr->setPacketCount(packetCount);
	sr->setOctetCount(payloadOctets);
	sr->preparePacket(rtpConfig->ssrc, 0);

	auto sdes = reinterpret_cast<RTCP_SDES *>(msg->data() + srSize);
	auto chunk = sdes->getChunk(0);
	chunk->setSSRC(rtpConfig->ssrc);
	auto item = chunk->getItem(0);
	item->type = 1;
	item->setText(rtpConfig->cname);
	sdes->preparePacket(1);
	return msg;
}

} // namespace rtc

#endif /* RTC_ENABLE_MEDIA */