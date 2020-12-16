#include "util.hh"
#include <fstream>
#include <algorithm>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/locale.hpp>
#include <uchardet/uchardet.h>
#include "preprocess/base64.hh"

namespace util {
    void toLower(std::string& s){
        boost::algorithm::to_lower(s);
    }
    std::string toLowerCopy(const std::string& s){
        return boost::algorithm::to_lower_copy(s);
    }

    void trim(std::string& s){
        boost::algorithm::trim(s);
    }

    // TODO: right now this leaves a single space at the beginning and the end of the line
    void trimLines(std::string& text) {
        std::string::iterator new_end = std::unique(text.begin(), text.end(), [](char lhs, char rhs){return (lhs == '\n' && rhs == '\n') || (std::isspace(lhs) && rhs != '\n' && std::isspace(rhs));});
        text.erase(new_end, text.end());
    }

    void trimLinesCopy(const std::string& original, std::string& result){
        result = "";
        auto first = original.begin();
        auto last = std::find(original.begin(), original.end(), '\n');
        std::string line = "";
        while (first < original.end()) {
            line = std::string(first, last);
            boost::trim_all(line);
            if (!line.empty()){
                result.append(line);
                result.append("\n");
            }
            first = last + 1;
            last = std::find(first, original.end(), '\n');
        }
    }

    bool detectCharset(const std::string& text, std::string& charset, const std::string& original_charset){
        uchardet_t handle = uchardet_new();
        int chardet_result = uchardet_handle_data(handle, text.c_str(), text.size());
        uchardet_data_end(handle);
        bool success = (chardet_result == 0);
        // trust the detected more than the specified charset
        if (success){
            charset = uchardet_get_charset(handle);
            toLower(charset);
        } else {
            // if detection fails, go with the original one
            charset = toLowerCopy(original_charset);
        }
        uchardet_delete(handle);
        if (charset.empty()) return false;

        // check that boost can work with the detected charset
        try {
            boost::locale::conv::to_utf<char>("", charset);
        } catch (const boost::locale::conv::invalid_charset_error& e) {
            return false;
        }
        return true;
    }

    std::string toUTF8(const std::string& text, const std::string& charset) {
        return boost::locale::conv::to_utf<char>(text, charset);
    }
    std::string toUTF8(const char* text, const std::string& charset) {
        return boost::locale::conv::to_utf<char>(text, charset);
    }

    void encodeBase64(const std::string& original, std::string& base64){
        preprocess::base64_encode(original, base64);
    }

    void decodeBase64(const std::string& base64, std::string& output){
        preprocess::base64_decode(base64, output);
    }

    void readTagFilters(const std::string& filename, umap_tag_filters& filters) {
        std::ifstream f(filename);
        std::string line;
        std::vector<std::string> fields;
        while (std::getline(f, line)) {
            fields.clear();
            boost::algorithm::split(fields, line, [](char c){return c == '\t';});
            if (fields.size() < 3)
                break;
            umap_attr_filters& attrs = filters[fields.at(0)];
            std::vector<std::string>& values = attrs[fields.at(1)];
            for (unsigned int i = 2; i < fields.size(); ++i)
                values.emplace_back(fields.at(i));
        }
        f.close();
    }

    bool createDirectories(const std::string& path){
        if (!boost::filesystem::exists(path))
            return boost::filesystem::create_directories(path);
        else return false; // throw exception??
    }

}
