//
// Created by danil on 02.01.24.
//

#ifndef HTTP_SERVER_CPP_SENDFILE_H
#define HTTP_SERVER_CPP_SENDFILE_H

#include <boost/asio.hpp>
#include <sys/sendfile.h>
#include <boost/asio/awaitable.hpp>
//#include <boost/asio/experimental/co_composed.hpp>

boost::asio::awaitable<std::size_t>
async_sendfile(boost::asio::ip::tcp::socket &socket, int file, off64_t offset, size_t length) {
    namespace asio = boost::asio;
    co_await socket.async_write_some(asio::null_buffers(), asio::deferred);

    if (!socket.native_non_blocking())
        socket.native_non_blocking(true);

    std::size_t total_bytes_transferred_ = 0;
    for (;;) {
        errno = 0;
        const auto n = ::sendfile64(socket.native_handle(), file, &offset, length);
        const auto ec = boost::system::error_code(n < 0 ? errno : 0, boost::asio::error::get_system_category());
        total_bytes_transferred_ += ec ? 0 : n;

        // Retry operation immediately if interrupted by signal.
        if (ec == boost::asio::error::interrupted)
            continue;

        // Check if we need to run the operation again.
        if (ec == boost::asio::error::would_block
            || ec == boost::asio::error::try_again) {
            // We have to wait for the socket to become ready again.
            co_await socket.async_write_some(boost::asio::null_buffers(), asio::deferred);
            continue;
        }

        if (ec || n == 0) {
            // An error occurred, or we have reached the end of the file.
            // Either way we must exit the loop so we can call the handler.
            if (ec)
                std::cout << "2: " << ec.what() << '\n';
            co_return total_bytes_transferred_;
        }
        // Loop around to try calling sendfile again.
    }
}


//
//template<typename CompletionToken>
//auto async_sendfile(boost::asio::ip::tcp::socket &socket, int file, off64_t offset, size_t length, CompletionToken &&token) {
//    std::cout << "send file called\n";
//    return boost::asio::async_initiate<CompletionToken, void(boost::system::error_code)>(
//            boost::asio::experimental::co_composed<void(boost::system::error_code)>(
//                    [](auto state, boost::asio::ip::tcp::socket &socket, int file, off64_t offset, size_t length) -> void {
//                        std::cout << "composed called\n";
//                        try {
//                            state.throw_if_cancelled(true);
//                            state.reset_cancellation_state(boost::asio::enable_terminal_cancellation());
//
//                            namespace asio = boost::asio;
//                            co_await socket.async_write_some(asio::null_buffers(), asio::deferred);
//
//                            if (!socket.native_non_blocking())
//                                socket.native_non_blocking(true);
//
//                            auto iter_count = 0;
//                            std::size_t total_bytes_transferred_ = 0;
//                            off64_t offset_ = offset;
//                            for (;;) {
//                                errno = 0;
//                                const auto n = ::sendfile64(socket.native_handle(), file, &offset_, length);
//                                const auto ec = boost::system::error_code(n < 0 ? errno : 0,boost::asio::error::get_system_category());
//                                total_bytes_transferred_ += ec ? 0 : n;
//                                std::cout << n << ' ' << ec.what() << ' ' << total_bytes_transferred_ << ' ' << iter_count++ << '\n';
//
//                                // Retry operation immediately if interrupted by signal.
//                                if (ec == boost::asio::error::interrupted)
//                                    continue;
//
//                                // Check if we need to run the operation again.
//                                if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again) {
//                                    std::cout << "await\n";
//                                    // We have to wait for the socket to become ready again.
//                                    co_await socket.async_write_some(boost::asio::null_buffers(), asio::deferred);
//                                    continue;
//                                }
//
//                                if (ec || n == 0) {
//                                    // An error occurred, or we have reached the end of the file.
//                                    // Either way we must exit the loop so we can call the handler.
//                                    if (ec)
//                                        std::cout << "2: " << ec.what() << '\n';
//                                    break;
////                                    co_return total_bytes_transferred_;
//                                }
//                                // Loop around to try calling sendfile again.
//                            }
//                        }
//                        catch (const boost::system::system_error &e) {
//                            co_return {e.code()};
//                        }
//                    }, socket), token, std::ref(socket), file, offset, length);
//}

#endif //HTTP_SERVER_CPP_SENDFILE_H
