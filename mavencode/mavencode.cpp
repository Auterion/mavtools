//
// Created by thomas on 29.02.24.
//

#include <iostream>
#include "args/args.hxx"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
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

using rapidjson_value_t = rapidjson::GenericValue<rapidjson::UTF8<char>>;

bool isFloatingPoint(const rapidjson_value_t &value) {
    if (value.IsDouble()) {
        return true;
    } else if (value.IsString()) {
        std::string str = value.GetString();
        if (str == "NaN" || str == "Infinity" || str == "-Infinity") {
            return true;
        }
    }
    return false;
}

bool isUnsigned(const rapidjson_value_t &value) {
    return value.IsUint() || value.IsUint64();
}

bool isSigned(const rapidjson_value_t &value) {
    return value.IsInt() || value.IsInt64();
}

bool isString(const rapidjson_value_t &value) {
    return value.IsString() && !isFloatingPoint(value);
}

double parseFloating(const rapidjson_value_t &value) {
    if (value.IsDouble()) {
        return value.GetDouble();
    } else if (value.IsString()) {
        std::string str = value.GetString();
        if (str == "NaN") {
            return NAN;
        } else if (str == "Infinity") {
            return INFINITY;
        } else if (str == "-Infinity") {
            return -INFINITY;
        }
    }
    return NAN;
}

mav::NativeVariantType fromGeneric(const rapidjson_value_t &generic) {
    if (isFloatingPoint(generic)) {
        return parseFloating(generic);
    } else if (isSigned(generic)) {
        return generic.GetInt64();
    } else if (isUnsigned(generic)) {
        return generic.GetUint64();
    } else if (isString(generic)) {
        return std::string{generic.GetString()};
    } else if (generic.IsArray()) {
        if (generic.Size() == 0) {
            return std::vector<int64_t>{};
        }
        auto array = generic.GetArray();
        if (isFloatingPoint(*array.begin())) {
            std::vector<double> r;
            for (auto &value : array) {
                r.emplace_back(parseFloating(value));
            }
            return r;
        } else if (isSigned(*array.begin())) {
            std::vector<int64_t> r;
            for (auto &value : array) {
                r.emplace_back(value.GetInt64());
            }
            return r;
        } else if (isUnsigned(*array.begin())) {
            std::vector<uint64_t> r;
            for (auto &value : array) {
                r.emplace_back(value.GetUint64());
            }
            return r;
        } else {
            throw std::runtime_error("Unknown type");
        }
    } else {
        throw std::runtime_error("Unknown type");
    }
    return 0;
}


int main(int argc, char *argv[]) {
    args::ArgumentParser parser("MAVLink decoder");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<std::string> xml_file(parser, "xml_file", "XML file containing the message set", {'x', "xml"});
    args::ValueFlag<std::string> output_file(parser, "output_file", "Output file for encoded messages. Writes to stdout when set to - or not set.", {'o', "output"});
    args::Positional<std::string> input_file(parser, "input_file", "JSON file containing mavlink messages to encode. Reads stdin when set to - or not set.", args::Options::Single);

    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
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

    std::shared_ptr<std::ostream> output_stream;
    if (!output_file) {
        std::cerr << "Writing to stdout" << std::endl;
        output_stream = std::shared_ptr<std::ostream>(&std::cout, [](std::ostream *) {});
    } else {
        std::cerr << "Writing to file: " << args::get(output_file) << std::endl;
        output_stream = std::make_shared<std::ofstream>(args::get(output_file), std::ios::binary);
    }

    std::atomic_bool interrupted{false};
    int retval = 0;

    auto signal_handler = [&](int signal) {
        interrupted.store(true);
        retval = signal;
        // set the eof bit on cin to stop the input loop
        if (input_stream.get() == &std::cin) {
            std::cin.setstate(std::ios::eofbit);
            std::fclose(stdin);
        } else {
            // we are using a file
            input_stream->setstate(std::ios::eofbit);
            std::ifstream* file = dynamic_cast<std::ifstream*>(input_stream.get());
            if (file) {
                file->close();
            }
        }
    };
    signalHandlerImpl = signal_handler;

    rapidjson::Document document;
    rapidjson::IStreamWrapper isw{*input_stream};

    while (!document.HasParseError()) {
        document.ParseStream<rapidjson::kParseStopWhenDoneFlag>(isw);

        if (document.HasMember("id")) {
            int id = document["id"].GetInt();
            try {
                auto message = message_set.create(id);
                if (document.HasMember("system_id")) {
                    message.header().systemId() = document["system_id"].GetInt();
                }
                if (document.HasMember("component_id")) {
                    message.header().componentId() = document["component_id"].GetInt();
                }

                int seq = 0;
                if (document.HasMember("seq")) {
                    seq = document["seq"].GetInt();
                }

                if (document.HasMember("fields")) {
                    for (auto &field : document["fields"].GetObject()) {
                        std::string name = field.name.GetString();
                        if (message.type().containsField(name)) {
                            try {
                                message.setFromNativeTypeVariant(name, fromGeneric(field.value));
                            } catch (std::runtime_error &e) {
                                std::cerr << "Error setting field " << name << " on message " << message.name() << ": " << e.what() << std::endl;
                            }
                        } else {
                            std::cerr << "Unknown field: " << name << " on message " << message.name() << std::endl;
                        }
                    }
                }
                int len = message.finalize(seq, {1, 1});
                if (len < 0) {
                    std::cerr << "Error finalizing message: " << len << std::endl;
                } else {
                    output_stream->write(reinterpret_cast<const char *>(message.data()), len);
                }
            } catch (std::out_of_range &e) {
                std::cerr << "Unknown message id: " << id << std::endl;
            }
        } else {
            std::cerr << "No name field in JSON object" << std::endl;
        }
    }
    if (interrupted) {
        std::cerr << "Interrupted" << std::endl;
    } else {
        auto error = document.GetParseError();
        if (error == rapidjson::kParseErrorDocumentEmpty) {
            // this is fine, we ran out of stuff to parse
            retval = 0;
        } else {
            std::cerr << "Error parsing JSON: " << error << std::endl;
            retval = error;
        }
    }

    // flush output
    output_stream->flush();

    // close the outfile if it exists
    if (output_stream && output_stream.get() != &std::cout) {
        std::ofstream* file = dynamic_cast<std::ofstream*>(output_stream.get());
        if (file) {
            file->close();
        }
    }

    return retval;
}