#ifndef ANALYZER_PROTOCOL_TCP_TCP_REASSEMBLER_H
#define ANALYZER_PROTOCOL_TCP_TCP_REASSEMBLER_H

#include "Reassem.h"
#include "TCP_Endpoint.h"
#include "TCP_Flags.h"

class BroFile;
class Connection;

namespace analyzer { namespace tcp {

class TCP_Analyzer;

class TCP_Reassembler : public Reassembler {
public:
	enum Type {
		Direct,		// deliver to destination analyzer itself
		Forward,	// forward to destination analyzer's children
	};

	TCP_Reassembler(Analyzer* arg_dst_analyzer, TCP_Analyzer* arg_tcp_analyzer,
	                Type arg_type, TCP_Endpoint* arg_endp);

	~TCP_Reassembler() override;

	void Done();

	void SetDstAnalyzer(Analyzer* analyzer)	{ dst_analyzer = analyzer; }
	void SetType(Type arg_type)	{ type = arg_type; }

	TCP_Analyzer* GetTCPAnalyzer()	{ return tcp_analyzer; }

	// Returns the volume of data buffered in the reassembler.
	// First parameter returns data that is above a hole, and thus is
	// waiting on the hole being filled.  Second parameter returns
	// data that has been processed but is awaiting an ACK to free
	// it up.
	//
	// If we're not processing contents, then naturally each of
	// these is empty.
	void SizeBufferedData(uint64_t& waiting_on_hole, uint64_t& waiting_on_ack) const;

	// How much data is pending delivery since it's not yet reassembled.
	// Includes the data due to holes (so this value is a bit different
	// from waiting_on_hole above; and is computed in a different fashion).
	uint64_t NumUndeliveredBytes() const
		{
		if ( last_block )
			return last_block->upper - last_reassem_seq;
		else
			return 0;
		}

	void SetContentsFile(BroFile* f);
	BroFile* GetContentsFile() const	{ return record_contents_file; }

	void MatchUndelivered(uint64_t up_to_seq, bool use_last_upper);

	// Skip up to seq, as if there's a content gap.
	// Can be used to skip HTTP data for performance considerations.
	void SkipToSeq(uint64_t seq);

	int DataSent(double t, uint64_t seq, int len, const u_char* data,
		     analyzer::tcp::TCP_Flags flags, bool replaying=true);
	void AckReceived(uint64_t seq);

	// Checks if we have delivered all contents that we can possibly
	// deliver for this endpoint.  Calls TCP_Analyzer::EndpointEOF()
	// when so.
	void CheckEOF();

	int HasUndeliveredData() const	{ return HasBlocks(); }
	int HadGap() const	{ return had_gap; }
	int DataPending() const;
	uint64_t DataSeq() const		{ return LastReassemSeq(); }

	void DeliverBlock(uint64_t seq, int len, const u_char* data);
	virtual void Deliver(uint64_t seq, int len, const u_char* data);

	TCP_Endpoint* Endpoint()		{ return endp; }
	const TCP_Endpoint* Endpoint() const	{ return endp; }

	int IsOrig() const	{ return endp->IsOrig(); }

	bool IsSkippedContents(uint64_t seq, int length) const
		{ return seq + length <= seq_to_skip; }

private:
	TCP_Reassembler()	{ }

	void Undelivered(uint64_t up_to_seq) override;
	void Gap(uint64_t seq, uint64_t len);

	void RecordToSeq(uint64_t start_seq, uint64_t stop_seq, BroFile* f);
	void RecordBlock(DataBlock* b, BroFile* f);
	void RecordGap(uint64_t start_seq, uint64_t upper_seq, BroFile* f);

	void BlockInserted(DataBlock* b) override;
	void Overlap(const u_char* b1, const u_char* b2, uint64_t n) override;

	TCP_Endpoint* endp;

	unsigned int deliver_tcp_contents:1;
	unsigned int had_gap:1;
	unsigned int did_EOF:1;
	unsigned int skip_deliveries:1;

	uint64_t seq_to_skip;

	bool in_delivery;
	analyzer::tcp::TCP_Flags flags;

	BroFile* record_contents_file;	// file on which to reassemble contents

	Analyzer* dst_analyzer;
	TCP_Analyzer* tcp_analyzer;

	Type type;
};

} } // namespace analyzer::* 

#endif
