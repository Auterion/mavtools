//
// Created by thomas on 29.02.24.
//

#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>
#include "args/args.hxx"

#include "../common/DummyInterface.h"
#include "../common/builtinMessageSet.h"


namespace {
    std::function<void(int)> signalHandlerImpl;

    // signal handler function
    void signalHandler(int signal) {
        std::cerr << "Received signal " << signal << std::endl;
        if (signalHandlerImpl) {
            signalHandlerImpl(signal);
        } else {
            exit(0);
        }
    }
}

template<typename T>
std::string jsonStringifyNumber(const T &arg) {
    if constexpr (std::is_floating_point<T>::value) {
        std::stringstream ss;
        if (std::isnan(arg)) {
            ss << "\"NaN\""; // JSON does not support NaN, so we print it as a string
        } else if (std::isinf(arg) && arg > 0) {
            ss << "\"Infinity\""; // JSON does not support Infinity, so we print it as a string
        } else if (std::isinf(arg) && arg < 0) {
            ss << "\"-Infinity\""; // JSON does not support -Infinity, so we print it as a string
        } else {
            ss << arg;
        }
        return ss.str();
    } else if constexpr (mav::is_any<std::decay_t<T>, uint8_t, int8_t>::value) {
        // static cast to int to avoid printing as a char
        return std::to_string(static_cast<int>(arg));
    } else {
        return std::to_string(arg);
    }
}

std::string jsonStringifyNativeType(const mav::NativeVariantType &value) {
    std::stringstream ss;
    std::visit([&ss](auto&& arg) {
        if constexpr (mav::is_string<decltype(arg)>::value) {
            ss << "\"" << arg << "\"";
        } else if constexpr (mav::is_iterable<decltype(arg)>::value) {
            ss << "[";
            for (auto it = arg.begin(); it != arg.end(); it++) {
                if (it != arg.begin())
                    ss << ", ";
                ss << jsonStringifyNumber(*it);
            }
            ss << "]";
        } else {
            ss << jsonStringifyNumber(arg);
        }
    }, value);
    return ss.str();
}

std::string messageAsJson(const mav::Message &message) {
    std::stringstream ss;
    ss << "{";
    ss << "\"id\": " << message.id() << ", ";
    ss << "\"name\": \"" << message.name() << "\", ";
    ss << "\"system_id\": " << static_cast<int>(message.header().systemId()) << ", ";
    ss << "\"component_id\": " << static_cast<int>(message.header().componentId()) << ", ";
    ss << "\"seq\": " << static_cast<int>(message.header().isSigned()) << ", ";
    ss << "\"fields\": { ";
    for (const auto &field : message.type().fieldNames()) {
        ss << "\"" << field << "\":";
        auto field_value = message.getAsNativeTypeInVariant(field);
        ss << jsonStringifyNativeType(field_value);
        if (field != message.type().fieldNames().back()) {
            ss << ",";
        } else {
            ss << "";
        }
    }
    ss << "}";
    ss << "}";
    return ss.str();
}


int main(int argc, char *argv[]) {
    std::signal(SIGINT, signalHandler);
    args::ArgumentParser parser("mavdecode");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<std::string> xml_file(parser, "message_set", "Mavlink message set XML to be used", {'x', "xml"});
    args::Positional<std::string> input_file(parser, "file", "Binary file containing mavlink messages to decode. Reads stdin when set to - or not set.", args::Options::Single);

    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help&) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    mav::MessageSet message_set;

    if (xml_file) {
        std::cerr << "Using XML file: " << args::get(xml_file) << std::endl;
        message_set.addFromXML(args::get(xml_file));
    } else {
        std::cerr << "No XML file provided, loading built-in message set" << std::endl;
        loadBuiltinMessageSet(message_set);
    }

    std::shared_ptr<std::istream> input_stream;
    if (!input_file || args::get(input_file) == "-") {
        std::cerr << "Reading from stdin" << std::endl;
        input_stream = std::shared_ptr<std::istream>(&std::cin, [](std::istream *) {});
    } else {
        std::cerr << "Reading from file: " << args::get(input_file) << std::endl;
        input_stream = std::make_shared<std::ifstream>(args::get(input_file), std::ios::binary);
    }

    DummyInterface dummy_interface;
    mav::StreamParser streamParser{message_set, dummy_interface};

    std::atomic_bool interrupted{false};
    std::unique_ptr<std::thread> parser_thread;
    int retval = 0;

    auto signal_handler = [&](int signal) {
        interrupted.store(true);
        retval = signal;
        dummy_interface.stop();
        // set the eof bit on cin to stop the input loop
        if (input_stream.get() == &std::cin) {
            std::cin.setstate(std::ios::eofbit);
            std::fclose(stdin);
        }
    };
    signalHandlerImpl = signal_handler;

    parser_thread = std::make_unique<std::thread>([&streamParser] {
        bool should_stop = false;
        while (!should_stop) {
            try {
                auto message = streamParser.next();
                std::cout << messageAsJson(message) << std::endl;
            } catch (mav::NetworkInterfaceInterrupt &e) {
                should_stop = true;
            } catch (mav::NetworkError &e) {
                std::cerr << "Network error: " << e.what() << std::endl;
            }
        }
    });

    // read in chunks of 64 bytes until EOF
    char buffer[64];
    while (input_stream->read(buffer, sizeof(buffer)) && !interrupted.load()) {
        std::streamsize bytesRead = input_stream->gcount();
        // Process the read bytes (echoing back in this example)
        dummy_interface.addToReceiveQueue(std::string(buffer, bytesRead));
    }

    // Handle any remaining bytes after the last read
    if (input_stream->gcount() > 0 && !interrupted.load()) {
        dummy_interface.addToReceiveQueue(std::string(buffer, input_stream->gcount()));
    }

    if (!interrupted.load()) { // make sure we parse everything, if we are not interrupted
        dummy_interface.waitUntilReceiveQueueEmpty();
    }
    dummy_interface.stop();
    if (parser_thread->joinable()) {
        parser_thread->join();
    }

    return retval;
}

