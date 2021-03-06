
#include <cybermon/socket.h>

#include <openssl/ssl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/un.h>

bool tcpip::ssl_socket::ssl_init = false;

// Certificate verification callback, always accept certificates which are
// pre-verified.
static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
    if (preverify_ok != 1) return 0;
    return 1;
}

tcpip::ip_address tcpip::ip_address::my_address()
{

    int ret;
    struct utsname uts;
    struct hostent *hent;

    ret = uname(&uts);
    if (ret < 0)
	return ip_address(127 << 24 | 1);

    hent = gethostbyname(uts.nodename);
    if (hent == NULL)
	return ip_address(127 << 24 | 1);

    struct in_addr addr;
    memcpy(&addr, hent->h_addr_list[0], sizeof(addr));
    return ip_address((long)(ntohl(addr.s_addr)));

}

unsigned short tcpip::tcp_socket::bound_port()
{

    struct sockaddr_in addr;

    socklen_t len = sizeof(addr);

    int ret = getsockname(sock, (struct sockaddr *) &addr, &len);
    if (ret < 0)
	throw std::runtime_error("Couldn't get socket address.");

    return ntohs(addr.sin_port);

}

unsigned short tcpip::ssl_socket::bound_port()
{

    struct sockaddr_in addr;

    socklen_t len = sizeof(addr);

    int ret = getsockname(sock, (struct sockaddr *) &addr, &len);
    if (ret < 0)
	throw std::runtime_error("Couldn't get socket address.");

    return ntohs(addr.sin_port);

}

void tcpip::tcp_socket::connect(const std::string& hostname, int port)
{

    struct hostent* hent = ::gethostbyname(hostname.c_str());
    if (hent == 0)
	throw std::runtime_error("Couldn't map hostname to address.");
	    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    memcpy(&addr.sin_addr.s_addr, hent->h_addr_list[0], 
	   sizeof(addr.sin_addr.s_addr));

    int ret = ::connect(sock, (sockaddr*) &addr, sizeof(addr));
    if (ret < 0)
	throw std::runtime_error("Couldn't connect to host.");

}

void tcpip::ssl_socket::connect(const std::string& hostname, int port)
{

    if (ssl == 0) {
	ssl = SSL_new(context);
	if (ssl == 0)
	    throw std::runtime_error("Couldn't initialise SSL.");
    }

    SSL_set_fd(ssl, sock);

    struct hostent* hent = ::gethostbyname(hostname.c_str());
    if (hent == 0)
	throw std::runtime_error("Couldn't map hostname to address.");
	    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    memcpy(&addr.sin_addr.s_addr, hent->h_addr_list[0], 
	   sizeof(addr.sin_addr.s_addr));

    int ret = ::connect(sock, (sockaddr*) &addr, sizeof(addr));
    if (ret < 0)
	throw std::runtime_error("Couldn't connect to host.");

    // Verify server certificate.
    SSL_set_verify(ssl, 
		   SSL_VERIFY_PEER,
		   verify_callback);
    SSL_set_verify_depth(ssl, 5);

    if (SSL_connect(ssl) < 0)
	throw std::runtime_error("SSL connection failed.");

}

void tcpip::udp_socket::connect(const std::string& hostname, int port)
{

    struct hostent* hent = ::gethostbyname(hostname.c_str());
    if (hent == 0)
	throw std::runtime_error("Couldn't map hostname to address.");
	    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    memcpy(&addr.sin_addr.s_addr, hent->h_addr_list[0], 
	   sizeof(addr.sin_addr.s_addr));

    int ret = ::connect(sock, (sockaddr*) &addr, sizeof(addr));
    if (ret < 0)
	throw std::runtime_error("Couldn't connect to host.");

}

bool tcpip::tcp_socket::poll(float timeout) 
{
    struct pollfd fds;
    fds.fd = sock;
    fds.events = POLLIN|POLLPRI;
    int ret = ::poll(&fds, 1, (int) (timeout * 1000));
    if (ret < 0) {
	if (errno != EINTR)
	    throw std::runtime_error("Socket poll failed");
	else
	    return false; // Treat EINTR as timeout.
    }
    
    if (ret == 0) return false;

    if (fds.revents & POLLERR)
	throw std::runtime_error("Socket in error");

    if (fds.revents & POLLHUP)
	throw std::runtime_error("Hangup");

    if (fds.revents & POLLNVAL)
	throw std::runtime_error("Socket closed");

    if (fds.revents)
	return true;
    else
	return false;
}

// FIXME: SSL socket re-uses loads of tcp_socket code!  Should be derived.
bool tcpip::ssl_socket::poll(float timeout) 
{

    struct pollfd fds;
    fds.fd = sock;
    fds.events = POLLIN|POLLPRI;
    int ret = ::poll(&fds, 1, (int) (timeout * 1000));
    if (ret < 0) {
	if (errno != EINTR)
	    throw std::runtime_error("Socket poll failed");
	else
	    return false; // Treat EINTR as timeout.
    }
    
    if (ret == 0) return false;

    if (fds.revents & POLLERR)
	throw std::runtime_error("Socket in error");

    if (fds.revents & POLLHUP)
	throw std::runtime_error("Hangup");

    if (fds.revents & POLLNVAL)
	throw std::runtime_error("Socket closed");

    if (fds.revents)
	return true;
    else
	return false;

}

bool tcpip::udp_socket::poll(float timeout) 
{
    struct pollfd fds;
    fds.fd = sock;
    fds.events = POLLIN|POLLPRI;
    int ret = ::poll(&fds, 1, (int) (timeout * 1000));
    if (ret < 0) {
	if (errno != EINTR)
	    throw std::runtime_error("Socket poll failed");
	else
	    return false; // Treat EINTR as timeout.
    }
    
    if (ret == 0) return false;

    if (fds.revents & POLLERR)
	throw std::runtime_error("Socket in error");

    if (fds.revents & POLLHUP)
	throw std::runtime_error("Hangup");

    if (fds.revents & POLLNVAL)
	throw std::runtime_error("Socket closed");

    if (fds.revents)
	return true;
    else
	return false;
}

int tcpip::tcp_socket::read(std::vector<unsigned char>& buffer, int len)
{
    int needed = len;
    int got = 0;
    
    while (needed > 0) {

	if (bufsize == 0) {
	    bufstart = 0;
	    bufsize = ::recv(sock, buf, buflen, 0);
	    if (bufsize == 0)
		return got;
	    if (bufsize < 0)
		throw std::runtime_error("Socket error");
	}

	if (bufsize > 0) {
	    if (needed >= bufsize) {
		buffer.insert(buffer.end(), buf + bufstart,
			      buf + bufstart + bufsize);
		got += bufsize;
		needed -= bufsize;
		bufsize = 0;
	    } else {
		buffer.insert(buffer.end(), buf + bufstart,
			      buf + bufstart + needed);
		bufsize -= needed;
		bufstart += needed;
		got += needed;
		needed = 0;
	    }
	}

    }
	    
    return got;
}


int tcpip::ssl_socket::read(std::vector<unsigned char>& buffer, int len)
{

    unsigned char tmp[len];
    int got = 0;

    while (got != len) {

	int ret = SSL_read(ssl, tmp, len - got);
	if (ret < 0)
	    throw std::runtime_error("Read error");
	if (ret == 0)
	    return 0;

	buffer.insert(buffer.end(), tmp, tmp + ret);

	got += ret;

    }

    return got;

}

int tcpip::udp_socket::read(std::vector<unsigned char>& buffer, int len)
{

    char tmp[len];

    int ret = ::recv(sock, tmp, len, 0);
    if (ret < 0)
	throw std::runtime_error("Socket error");

    buffer.resize(ret);
    copy(tmp, tmp + ret, buffer.begin());

    return ret;

}

int tcpip::udp_socket::read(char* buffer, int len)
{

    int ret = ::recv(sock, buffer, len, 0);
    if (ret < 0)
	throw std::runtime_error("Socket error");

    return ret;

}

int tcpip::tcp_socket::read(char* buffer, int len)
{
    int needed = len;
    int got = 0;
	    
    while (needed > 0) {

	if (bufsize == 0) {
	    bufstart = 0;
	    bufsize = ::recv(sock, buf, buflen, 0);
	    if (bufsize == 0)
		return got;
	    if (bufsize < 0)
		throw std::runtime_error("Socket error");
	}

	if (bufsize > 0) {
	    if (needed >= bufsize) {
		memcpy(buffer + got, buf + bufstart, bufsize);
		got += bufsize;
		needed -= bufsize;
		bufsize = 0;
	    } else {
		memcpy(buffer + got, buf + bufstart, needed);
		bufsize -= needed;
		bufstart += needed;
		got += needed;
		needed = 0;
	    }
	}

    }
	    
    return got;
}

int tcpip::ssl_socket::read(char* buffer, int len)
{

    int got = 0;

    while (got != len) {

	int ret = SSL_read(ssl, buffer + got, len - got);
	if (ret < 0)
	    throw std::runtime_error("Read error");
	if (ret == 0)
	    return 0;

	got += ret;

    }

    return got;

}

void tcpip::tcp_socket::bind(int port)
{

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
	    
    /* Re-use the socket address in case it's in TIME_WAIT state. */
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &opt,
	       sizeof(opt));
	    
    int ret = ::bind(sock, (struct sockaddr*) &addr, sizeof(addr));
    if (ret < 0) {
	::close(sock);
	throw std::runtime_error("Socket bind failed.");
    }
    socklen_t slen = sizeof(addr);
    ret = ::getsockname(sock, (struct sockaddr*) &addr, &slen);
    if (ret < 0) {
	::close(sock);
	throw std::runtime_error("Socket address failed.");
    }
    port = ntohs(addr.sin_port);

}

void tcpip::ssl_socket::bind(int port)
{

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
	    
    /* Re-use the socket address in case it's in TIME_WAIT state. */
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &opt,
	       sizeof(opt));
	    
    int ret = ::bind(sock, (struct sockaddr*) &addr, sizeof(addr));
    if (ret < 0) {
	::close(sock);
	throw std::runtime_error("Socket bind failed.");
    }
    socklen_t slen = sizeof(addr);
    ret = ::getsockname(sock, (struct sockaddr*) &addr, &slen);
    if (ret < 0) {
	::close(sock);
	throw std::runtime_error("Socket address failed.");
    }
    port = ntohs(addr.sin_port);

}

void tcpip::udp_socket::bind(int port)
{

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    int ret = ::bind(sock, (struct sockaddr*) &addr, sizeof(addr));
    if (ret < 0) {
	::close(sock);
	throw std::runtime_error("Socket bind failed.");
    }
    socklen_t slen = sizeof(addr);
    ret = ::getsockname(sock, (struct sockaddr*) &addr, &slen);
    if (ret < 0) {
	::close(sock);
	throw std::runtime_error("Socket address failed.");
    }
    port = ntohs(addr.sin_port);

}

void tcpip::stream_socket::readline(std::string& line)
{
    unsigned char c;
    line = "";
    while(1) {
	int ret = read((char*) &c, 1);
	if (ret <= 0 && line.size() > 1)
	    return;
	if (ret <= 0)
	    throw std::runtime_error("EOF on socket.");
	if (c == '\r') continue;
	if (c == '\n') return;
	line += c;
    }
}

std::ostream& operator<<(std::ostream& o, const tcpip::address& addr) {
    std::string s;
    addr.to_string(s);
    o << s;
    return o;
}

void tcpip::unix_socket::connect(const std::string& path)
{

    struct sockaddr_un address;

    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, path.c_str());

    int ret = ::bind(sock, reinterpret_cast<struct sockaddr *>(&address),
		   sizeof(address));
    if (ret < 0)
	throw std::runtime_error("Socket bind failed");

}

bool tcpip::unix_socket::poll(float timeout) 
{
    struct pollfd fds;
    fds.fd = sock;
    fds.events = POLLIN|POLLPRI;
    int ret = ::poll(&fds, 1, (int) (timeout * 1000));
    if (ret < 0) {
	if (errno != EINTR)
	    throw std::runtime_error("Socket poll failed");
	else
	    return false; // Treat EINTR as timeout.
    }
    
    if (ret == 0) return false;

    if (fds.revents & POLLERR)
	throw std::runtime_error("Socket in error");

    if (fds.revents & POLLHUP)
	throw std::runtime_error("Hangup");

    if (fds.revents & POLLNVAL)
	throw std::runtime_error("Socket closed");

    if (fds.revents)
	return true;
    else
	return false;
}

int tcpip::unix_socket::read(std::vector<unsigned char>& buffer, int len)
{

    char tmp[len];

    int ret = ::recv(sock, tmp, len, 0);
    if (ret < 0)
	throw std::runtime_error("Socket error");

    buffer.resize(ret);
    copy(tmp, tmp + ret, buffer.begin());

    return ret;

}

int tcpip::unix_socket::read(char* buffer, int len)
{

    int ret = ::recv(sock, buffer, len, 0);
    if (ret < 0)
	throw std::runtime_error("Socket error");

    return ret;

}

void tcpip::unix_socket::bind(const std::string& path)
{

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path.c_str());

    unlink(path.c_str());

    int ret = ::bind(sock, (struct sockaddr*) &addr, sizeof(addr));
    if (ret < 0) {
	::close(sock);
	throw std::runtime_error("Socket bind failed.");
    }

}

void tcpip::raw_socket::connect(const std::string& hostname)
{

    struct hostent* hent = ::gethostbyname(hostname.c_str());
    if (hent == 0)
	throw std::runtime_error("Couldn't map hostname to address.");
	    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = 0;

    memcpy(&addr.sin_addr.s_addr, hent->h_addr_list[0], 
	   sizeof(addr.sin_addr.s_addr));

    int ret = ::connect(sock, (sockaddr*) &addr, sizeof(addr));
    if (ret < 0)
	throw std::runtime_error("Couldn't connect to host.");

}


/** Accept a connection. */
boost::shared_ptr<tcpip::stream_socket> tcpip::ssl_socket::accept()
{

    int ns = ::accept(sock, 0, 0);
    if (-1 == ns) {
	throw std::runtime_error("Socket accept failed");
    }

    SSL* ssl2 = SSL_new(context);
    SSL_set_fd(ssl2, ns);
    SSL_set_verify(ssl2,
		   SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
		   verify_callback);
    SSL_set_verify_depth(ssl2, 5);

    int ret = SSL_accept(ssl2);
    if (ret != 1) {
	::close(ns);
	SSL_free(ssl2);
	throw std::runtime_error("SSL accept failed");
    }

    ssl_socket* conn = new ssl_socket(ns);
    conn->ssl = ssl2;

    return boost::shared_ptr<stream_socket>(conn);

}

/** Close the connection. */
void tcpip::ssl_socket::close()
{

    if (ssl) {
	SSL_shutdown(ssl);
	SSL_free(ssl);
	ssl = 0;
    }
    if (sock >= 0) {
	::shutdown(sock, SHUT_RDWR);
	::close(sock);
	sock = -1;
    }
}

/** Constructor. */
tcpip::ssl_socket::ssl_socket() { 
    bufstart = bufsize = 0;

    if (!ssl_init) {
	SSL_library_init();
	ssl_init = true;
    }

    context = SSL_CTX_new(TLSv1_2_method());
    if (context == 0)
       throw std::runtime_error("Couldn't initialise SSL context.");

#ifdef SSL_CTX_set_ecdh_auto
    // Required when ECDH was not automatically used
    // (OpenSSL between 1.0.2 and 1.1.0)
    SSL_CTX_set_ecdh_auto(context, 1);
#endif    

    ssl = 0;
    
    sock = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
       throw std::runtime_error("Socket creation failed.");
	    
}


/** Constructor. */
tcpip::ssl_socket::ssl_socket(int s) { 
    bufstart = bufsize = 0;

    if (!ssl_init) {
	SSL_library_init();
	ssl_init = true;
    }

    context = SSL_CTX_new(TLSv1_2_method());
    if (context == 0)
       throw std::runtime_error("Couldn't initialise SSL context.");

#ifdef SSL_CTX_set_ecdh_auto
    // Required when ECDH was not automatically used
    // (OpenSSL between 1.0.2 and 1.1.0)
    SSL_CTX_set_ecdh_auto(context, 1);
#endif    

    ssl = SSL_new(context);
    if (ssl == 0)
       throw std::runtime_error("Couldn't initialise SSL.");

    sock = s;
	    
}


/** Provide certificate. */
void tcpip::ssl_socket::use_certificate_file(const std::string& f)
{
    if (context == 0)
	throw std::runtime_error("No SSL context.");
    int ret = SSL_CTX_use_certificate_file(context, f.c_str(),
					   SSL_FILETYPE_PEM);
    if (ret < 0)
	throw std::runtime_error("Couldn't load certificate file.");
}

/** Provide private key. */
void tcpip::ssl_socket::use_key_file(const std::string& f)
{
    if (context == 0)
	throw std::runtime_error("No SSL context.");
    int ret = SSL_CTX_use_PrivateKey_file(context, f.c_str(),
					  SSL_FILETYPE_PEM);
    if (ret < 0)
	throw std::runtime_error("Couldn't load private key file.");
}

/** Provide CA chain. */
void tcpip::ssl_socket::use_certificate_chain_file(const std::string& f)
{
    if (context == 0)
	throw std::runtime_error("No SSL context.");
    int ret = SSL_CTX_load_verify_locations(context, f.c_str(), 0);
    if (ret < 0)
	throw std::runtime_error("Couldn't load certificate file.");

    STACK_OF(X509_NAME) *certs;

    certs = SSL_load_client_CA_file(f.c_str());
    if (certs != 0)
	SSL_CTX_set_client_CA_list(context, certs);
    else 
	throw std::runtime_error("Couldn't load certificate file.");

}

void tcpip::ssl_socket::check_private_key()
{

    if (!SSL_CTX_check_private_key(context))
	throw
	    std::runtime_error("Couldn't verify private key with certificate.");

}
