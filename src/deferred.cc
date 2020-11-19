#include "deferred.hh"
#include "util.hh"
#include <iostream>

namespace warc2text {
    void DeferredTree::endStandoffSegment(std::string& deferred) {
        if (!deferred.empty() and deferred.back() == '+')
            deferred.back() = ';';
        else if (!deferred.empty() and deferred.back() != ';')
            deferred.push_back(';');
    }

    void DeferredTree::continueStandoffSegment(std::string& deferred) {
        if (!deferred.empty() and deferred.back() != ';' and deferred.back() != '+')
            deferred.push_back('+');
    }

    DeferredTree::DeferredTree() : DeferredTree(true) {};

    DeferredTree::DeferredTree(bool deferred) : deferred(deferred), level(0), tag_stack(), counts() {
        counts.emplace_back();
    }

    void DeferredTree::insertTag(const std::string& tag) {
        if (!deferred) return;
        if (html::isVoidTag(tag))
            return;

        // take care of elements that implicitly close previous one:
        // each new li item implies closing the previous li
        if (!tag_stack.empty()) {
            if (tag_stack.back().tag == "li" and tag == "li") {
                endTag("li");
            }
            // body element implies closing head
            else if (tag_stack.back().tag == "head" and tag == "body") {
                endTag("head");
            }
            // <dt> and <dd> close previous <dd> or <dt>
            else if ((tag_stack.back().tag == "dt" or tag_stack.back().tag == "dd") and (tag == "dt" or tag == "dd")) {
                endTag(tag_stack.back().tag);
            }
            // any block tag closes previous <p> element
            else if (tag_stack.back().tag == "p" and html::isBlockTag(tag)) {
                endTag(tag_stack.back().tag);
            }
        }

        // insert new element
        tag_stack.push_back({tag, 0, 0});
        counts.at(level)[tag]++;
        ++level;
        if (counts.size() < level+1)
            counts.emplace_back();
    }

    void DeferredTree::endTag(const std::string& tag) {
        if (empty()) return;
        if (html::isVoidTag(tag)) return;

        if (tag_stack.back().tag == tag) {
            tag_stack.pop_back();
            counts.at(level).clear();
            --level;
        }
        // <li> <dt> and <dd> may be closed by closing the parent element
        else if (tag_stack.back().tag == "li" or tag_stack.back().tag == "dt" or tag_stack.back().tag == "dd" ) {
            endTag(tag_stack.back().tag);
            endTag(tag);
        }
        // <p> can also be closed by closing the parent element
        // (there are some additional restrictions, but whatever)
        else if (tag_stack.back().tag == "p") {
            endTag(tag_stack.back().tag);
            endTag(tag);
        }
    }

    bool DeferredTree::empty() const {
        return !deferred or tag_stack.empty();
    }

    void DeferredTree::addOffset(int n) {
        if (empty()) return;
        tag_stack.back().offset += n;
    }

    void DeferredTree::addLength(int n) {
        if (empty()) return;
        tag_stack.back().length += n;
    }

    unsigned int DeferredTree::getCurrentOffset() const {
        if (empty()) return 0;
        return tag_stack.back().offset;
    }

    unsigned int DeferredTree::getCurrentLength() const {
        if (empty()) return 0;
        return tag_stack.back().length;
    }

    void DeferredTree::setCurrentOffset(unsigned int n) {
        if (empty()) return;
        tag_stack.back().offset = n;
    }

    void DeferredTree::setCurrentLength(unsigned int n) {
        if (empty()) return;
        tag_stack.back().length = n;
    }

    void DeferredTree::appendAndOffset(std::string& deferred) {
        if (getCurrentLength() > 0) {
            appendStandoff(deferred);
            setCurrentOffset(getCurrentLength());
            setCurrentLength(0);
        }
    }

    void DeferredTree::appendStandoff(std::string& deferred) const {
        if (!this->deferred) return;
        for (unsigned int l = 0; l < tag_stack.size(); ++l) {
            deferred.append(tag_stack.at(l).tag);
            unsigned int i = counts.at(l).at(tag_stack.at(l).tag);
            if (i > 1) {
                deferred.push_back('[');
                deferred.append(std::to_string(i));
                deferred.push_back(']');
            }
            deferred.push_back('/');
        }
        if(!deferred.empty()) deferred.back() = ':';
        deferred.append(std::to_string(getCurrentOffset())); // start position
        deferred.push_back('-');
        deferred.append(std::to_string(getCurrentOffset() + getCurrentLength())); // length of segment
    }

} //warc2text
