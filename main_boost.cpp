#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;
using boost::system::error_code;
using boost::asio::streambuf;

int main() {
    boost::asio::io_service svc;

    tcp::acceptor a(svc);
    a.open(tcp::v4());
    a.set_option(tcp::acceptor::reuse_address(true));
    a.bind({{}, 5000}); // bind to port 6767 on localhost
    a.listen(5);

    using session = std::shared_ptr<tcp::socket>;

    std::function<void()>        do_accept;
    std::function<void(session)> do_session;

    do_session = [&](session s) {
        // do a read
        auto buf = std::make_shared<std::vector<char>>(1024);
        s->async_read_some(boost::asio::buffer(*buf), [&,s,buf](error_code ec, size_t bytes) {
            if (ec)
                std::cerr << "read failed: " << ec.message() << "\n";
            else {
                std::cout << "Echo to " << s->remote_endpoint(ec) << ": " << std::string(buf->data(), bytes);
                if (ec)
                    std::cerr << "endpoint failed: " << ec.message() << "\n";
                else
                    async_write(*s, boost::asio::buffer(*buf), [&,s,buf](error_code ec, size_t) {
                        if (ec) std::cerr << "write failed: " << ec.message() << "\n";
                    });

                do_session(s); // full duplex, can read while writing, using a second buffer
            }

        });
    };

    do_accept = [&] {
        auto s = std::make_shared<session::element_type>(svc);
        a.async_accept(*s, [&,s](error_code ec) {
            if (ec)
                std::cerr << "accept failed: " << ec.message() << "\n";
            else {
                do_session(s);
                do_accept(); // accept the next
            }
        });
    };

    do_accept(); // kick-off
    svc.run();   // wait for shutdown (Ctrl-C or failure)
}
