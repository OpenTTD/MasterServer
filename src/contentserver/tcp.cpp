/* $Id$ */

/*
 * This file is part of OpenTTD's content service.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

#include "shared/stdafx.h"
#include "shared/debug.h"
#include "shared/core/alloc_func.hpp"
#include "contentserver.h"
#include "path.h"

/**
 * @file contentserver/tcp.cpp Handler of incoming TCP content server packets
 */

DEF_CONTENT_RECEIVE_COMMAND(Server, PACKET_CONTENT_CLIENT_INFO_LIST)
{
	ContentType type    = (ContentType)p->Recv_uint8();
	uint32 ottd_version = p->Recv_uint32();

	if (this->HasClientQuit()) return false;

	ContentInfo ci[1024];
	uint length = this->cs->GetSQLBackend()->FindContentDetails(ci, lengthof(ci), type, ottd_version);

	this->SendInfo(length, ci);

	return true;
}

DEF_CONTENT_RECEIVE_COMMAND(Server, PACKET_CONTENT_CLIENT_INFO_ID)
{
	uint16 count = p->Recv_uint16();
	ContentInfo *ci = new ContentInfo[count];

	for (uint i = 0; i < count; i++) {
		ci[i].id = (ContentID)p->Recv_uint32();
	}

	if (!this->HasClientQuit()) {
		this->cs->GetSQLBackend()->FillContentDetails(ci, count, SQL::CK_ID);
		this->SendInfo(count, ci);
	}

	delete[] ci;

	return true;
}

DEF_CONTENT_RECEIVE_COMMAND(Server, PACKET_CONTENT_CLIENT_INFO_EXTID)
{
	uint8 count = p->Recv_uint8();

	ContentInfo *ci = new ContentInfo[count];

	for (int i = 0; i < count; i++) {
		ci[i].type = (ContentType)p->Recv_uint8();
		ci[i].unique_id = p->Recv_uint32();

		if (!ci[i].IsValid()) {
			delete[] ci;
			this->Close();
			return false;
		}
	}

	if (!this->HasClientQuit()) {
		this->cs->GetSQLBackend()->FillContentDetails(ci, count, SQL::CK_UNIQUEID);
		this->SendInfo(count, ci);
	}

	delete[] ci;

	return true;
}

DEF_CONTENT_RECEIVE_COMMAND(Server, PACKET_CONTENT_CLIENT_INFO_EXTID_MD5)
{
	uint8 count = p->Recv_uint8();

	ContentInfo *ci = new ContentInfo[count];

	for (int i = 0; i < count; i++) {
		ci[i].type = (ContentType)p->Recv_uint8();
		ci[i].unique_id = p->Recv_uint32();
		for (uint j = 0; j < sizeof(ci->md5sum); j++) {
			ci[i].md5sum[j] = p->Recv_uint8();
		}

		if (!ci[i].IsValid()) {
			delete[] ci;
			this->Close();
			return false;
		}
	}

	if (!this->HasClientQuit()) {
		this->cs->GetSQLBackend()->FillContentDetails(ci, count, SQL::CK_UNIQUEID_MD5);
		this->SendInfo(count, ci);
	}

	delete[] ci;

	return true;
}

void ServerNetworkContentSocketHandler::SendInfo(uint32 count, const ContentInfo *infos)
{
	for (; count != 0; count--, infos++) {
		/* Size of data + Packet size + packet type (byte) */
		if (infos->Size() + sizeof(PacketSize) + sizeof(byte) >= SEND_MTU) {
			DEBUG(misc, 0, "Info data bigger than packet size!");
			continue;
		}

		Packet *p = new Packet(PACKET_CONTENT_SERVER_INFO);
		p->Send_uint8((byte)infos->type);
		p->Send_uint32(infos->id);
		p->Send_uint32(infos->filesize);
		p->Send_string(infos->name);
		p->Send_string(infos->version);
		p->Send_string(infos->url);
		p->Send_string(infos->description);

		p->Send_uint32(infos->unique_id);
		for (uint j = 0; j < sizeof(infos->md5sum); j++) {
			p->Send_uint8 (infos->md5sum[j]);
		}

		p->Send_uint8(infos->dependency_count);
		for (uint i = 0; i < infos->dependency_count; i++) p->Send_uint32(infos->dependencies[i]);
		p->Send_uint8(infos->tag_count);
		for (uint i = 0; i < infos->tag_count; i++) p->Send_string(infos->tags[i]);

		this->Send_Packet(p);
	}
}

DEF_CONTENT_RECEIVE_COMMAND(Server, PACKET_CONTENT_CLIENT_CONTENT)
{
	uint16 count = p->Recv_uint16();
	ContentInfo *ci = new ContentInfo[count];

	for (uint i = 0; i < count; i++) {
		ci[i].id = (ContentID)p->Recv_uint32();
	}

	if (this->HasClientQuit()) {
		delete[] ci;
		return true;
	}

	assert(this->contentQueue == NULL);

	this->cs->GetSQLBackend()->FillContentDetails(ci, count, SQL::CK_ID);
	this->contentQueue = ci;
	this->contentQueueIter = 0;
	this->contentQueueLength = count;
	this->SendQueue();

	/* Don't read the next packet; first handle the current content queue. */
	return false;
}

void ServerNetworkContentSocketHandler::SendQueue()
{
	assert(this->contentQueue != NULL);

	this->last_activity = GetTime();

	ContentInfo *infos = &this->contentQueue[this->contentQueueIter];

	char file_name[MAX_PATH];
	snprintf(file_name, lengthof(file_name), CONTENT_DATA_PATH, infos->id / 100, infos->id);
	FILE *f = fopen(file_name, "rb");
	if (f != NULL) {
		fseek(f, 0, SEEK_END);

		if (ftell(f) != (long)infos->filesize) {
			DEBUG(misc, 0, "File size of file (%i) and DB (%i) of %i do not match",
						ftell(f), infos->filesize, infos->id);
			fclose(f);
		}

		fseek(f, 0, SEEK_SET);
	} else {
		DEBUG(misc, 0, "Opening %s failed", file_name);
	}

	Packet *p = new Packet(PACKET_CONTENT_SERVER_CONTENT);

	p->Send_uint8((byte)infos->type);
	p->Send_uint32(infos->id);
	p->Send_uint32(f == NULL ? 0 : infos->filesize);
	p->Send_string(infos->filename);

	this->Send_Packet(p);
	this->cs->GetSQLBackend()->IncrementDownloadCount(infos->id);

	if (f != NULL) {
		for (;;) {
			Packet *p = new Packet(PACKET_CONTENT_SERVER_CONTENT);
			int res = (int)fread(p->buffer + p->size, 1, SEND_MTU - p->size, f);

			if (res <= 0 || ferror(f)) {
				DEBUG(misc, 0, "Reading file failed...");
				fclose(f);
				this->Close();
				return;
			}

			p->size += res;
			this->Send_Packet(p);

			if (feof(f)) {
				this->Send_Packet(new Packet(PACKET_CONTENT_SERVER_CONTENT));
				fclose(f);
				break;
			}
		}
		this->Send_Packets();
	}

	this->contentQueueIter++;

	if (this->contentQueueIter == this->contentQueueLength) {
		delete [] this->contentQueue;
		this->contentQueue = NULL;
	}
}

bool ServerNetworkContentSocketHandler::HasQueue()
{
	return this->contentQueue != NULL;
}

