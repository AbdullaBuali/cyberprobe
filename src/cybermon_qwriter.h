/*
 * cybermon_qwriter.h
 *
 *  Created on: 21 Jun 2017
 *      Author: venkata
 */

#ifndef CYBERMON_QWRITER_H_
#define CYBERMON_QWRITER_H_

#include <cybermon/cybermon-lua.h>
#include <cybermon/engine.h>
#include <queue>
#include <cybermon_qargs.h>

namespace cybermon {

    class cybermon_qwriter: public engine {

      public:

	// Constructor
        cybermon_qwriter(const std::string& path,
			 std::queue<q_entry*>& cybermonq,
			 threads::mutex& cqwrlock);
	// Destructor.
	virtual ~cybermon_qwriter() {
	}

	std::queue<q_entry*>& cqueue;

	threads::mutex& lock;

	virtual void connection_up(const context_ptr cp,
				   const pdu_time& tv);

	virtual void connection_down(const context_ptr cp,
				     const pdu_time& tv);

	virtual void sip_ssl(const context_ptr cp, pdu_iter s, pdu_iter e,
			     const pdu_time& tv);

	virtual void smtp_auth(const context_ptr cp, pdu_iter s, pdu_iter e,
			       const pdu_time& tv);

	virtual void rtp_ssl(const context_ptr cp,
			     const pdu_iter s, pdu_iter e,
			     const pdu_time& tv);

	virtual void rtp(const context_ptr cp, pdu_iter s, pdu_iter e,
			 const pdu_time& tv);

	virtual void pop3_ssl(const context_ptr cp, pdu_iter s, pdu_iter e,
			      const pdu_time& tv);

	virtual void pop3(const context_ptr cp, pdu_iter s, pdu_iter e,
			  const pdu_time& tv);

	virtual void imap_ssl(const context_ptr cp, pdu_iter s, pdu_iter e,
			      const pdu_time& tv);

	virtual void imap(const context_ptr cp, pdu_iter s, pdu_iter e,
			  const pdu_time& tv);

	virtual void icmp(const context_ptr cp, unsigned int type,
			  unsigned int code, pdu_iter s, pdu_iter e,
			  const pdu_time& tv);

	virtual void sip_request(const context_ptr cp,
				 const std::string& method,
				 const std::string& from,
				 const std::string& to,
				 pdu_iter s, pdu_iter e,
				 const pdu_time& tv);

	virtual void sip_response(const context_ptr cp, unsigned int code,
				  const std::string& status,
				  const std::string& from,
				  const std::string& to,
				  pdu_iter s, pdu_iter e,
				  const pdu_time& tv);

	virtual void http_request(const context_ptr cp,
				  const std::string& method,
				  const std::string& url,
				  const observer::http_hdr_t& hdr,
				  pdu_iter body_start, pdu_iter body_end,
				  const pdu_time& tv);
	
	virtual void http_response(const context_ptr cp,
				   unsigned int code,
				   const std::string& status,
				   const observer::http_hdr_t& hdr,
				   const std::string& url,
				   pdu_iter body_start, pdu_iter body_end,
				   const pdu_time& tv);

	virtual void smtp_command(const context_ptr cp,
				  const std::string& command,
				  const pdu_time& tv);
	virtual void smtp_response(const context_ptr cp, int status,
				   const std::list<std::string>& text,
				   const pdu_time& tv);
	virtual void smtp_data(const context_ptr cp,
			       const std::string& from,
			       const std::list<std::string>& to,
			       std::vector<unsigned char>::const_iterator s,
			       std::vector<unsigned char>::const_iterator e,
			       const pdu_time& tv);

	virtual void ftp_command(const context_ptr cp,
				 const std::string& command,
				 const pdu_time& tv);
	virtual void ftp_response(const context_ptr cp, int status,
				  const std::list<std::string>& responses,
				  const pdu_time& tv);

	void trigger_up(const std::string& liid, const tcpip::address& a,
			const pdu_time& tv);

	void trigger_down(const std::string& liid, const pdu_time& tv);

	virtual void dns_message(const context_ptr cp,
				 const dns_header hdr,
				 const std::list<dns_query> queries,
				 const std::list<dns_rr> answers,
				 const std::list<dns_rr> authorities,
				 const std::list<dns_rr> additional,
				 const pdu_time& tv);

	virtual void ntp_timestamp_message(const context_ptr cp,
					   const ntp_timestamp& ts,
					   const pdu_time& tv);
	virtual void ntp_control_message(const context_ptr cp,
					 const ntp_control& ctrl,
					 const pdu_time& tv);
	virtual void ntp_private_message(const context_ptr cp,
					 const ntp_private& priv,
					 const pdu_time& tv);

	virtual void unrecognised_stream(const context_ptr cp,
					 pdu_iter s, pdu_iter e,
					 const pdu_time& tv);
	virtual void unrecognised_datagram(const context_ptr cp,
					   pdu_iter s, pdu_iter e,
					   const pdu_time& tv);
	virtual void close();

	// Max size of queue.
	const int q_limit = 1000;

	virtual void push(q_entry* e) {
	    lock.lock();

	    // Sleep until queue is below the queue limit.
	    while (cqueue.size() >= q_limit) {
		lock.unlock();
		usleep(10);
		lock.lock();
	    }

	    cqueue.push(e);
	    lock.unlock();
	}

    };

};

#endif /* CYBERMON_QWRITER_H_ */
