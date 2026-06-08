// Timestamp unit tests
//
// Test coverage:
//   - Default / now() / invalid() validity
//   - toString() / toFormattedString() format
//   - Comparison operators (==, <, !=, >, <=, >=)
//   - timeDifference() / addTime() accuracy

#include "csl/base/timestamp.h"

#include <cassert>
#include <cstring>
#include <iostream>

// Check formatted string matches pattern: YYYYMMDD HH:MM:SS.uuuuuu (24 chars)
static bool matchFormatted(const std::string& s) {
    // "20260602 18:30:45.123456" = 24 chars
    if (s.size() != 24) {
        std::cerr << "  [DEBUG] Expected 24 chars, got " << s.size()
                  << ": '" << s << "'" << std::endl;
        return false;
    }
    if (s[8] != ' ') return false;   // space between date and time
    if (s[11] != ':') return false;  // HH:MM
    if (s[14] != ':') return false;  // MM:SS
    if (s[17] != '.') return false;  // SS.microseconds
    return true;
}

// Check toString format: seconds.microseconds (at least 8 chars, one dot, 6 digits after)
static bool matchToString(const std::string& s) {
    auto dotPos = s.find('.');
    if (dotPos == std::string::npos) return false;
    if (s.size() - dotPos - 1 != 6) return false;
    return true;
}

int main() {
    // ===== 1. Default constructor: valid() == false =====
    {
        csl::Timestamp t;
        assert(!t.valid());
        assert(t.microSecondsSinceEpoch() == 0);
        std::cout << "[PASS] Default ctor valid() == false" << std::endl;
    }

    // ===== 2. now() valid() == true =====
    {
        csl::Timestamp t = csl::Timestamp::now();
        assert(t.valid());
        assert(t.microSecondsSinceEpoch() > 0);
        std::cout << "[PASS] now() valid() == true" << std::endl;
    }

    // ===== 3. invalid() valid() == false =====
    {
        csl::Timestamp t = csl::Timestamp::invalid();
        assert(!t.valid());
        assert(t.microSecondsSinceEpoch() == 0);
        std::cout << "[PASS] invalid() valid() == false" << std::endl;
    }

    // ===== 4. toString() format =====
    {
        csl::Timestamp t(1 * csl::Timestamp::kMicroSecondsPerSecond + 123456);
        std::string s = t.toString();
        assert(matchToString(s));
        assert(s == "1.123456");
        std::cout << "[PASS] toString() format: " << s << std::endl;
    }

    // ===== 5. toFormattedString() format =====
    {
        csl::Timestamp t = csl::Timestamp::now();
        std::string s = t.toFormattedString();
        assert(matchFormatted(s));
        std::cout << "[PASS] toFormattedString() format: " << s << std::endl;
    }

    // ===== 6. Comparison operators =====
    {
        csl::Timestamp t1(1000000);  // 1 second
        csl::Timestamp t2(2000000);  // 2 seconds
        csl::Timestamp t3(1000000);  // same as t1

        assert(t1 == t3);
        assert(!(t1 == t2));

        assert(t1 < t2);
        assert(!(t2 < t1));
        assert(!(t1 < t3));

        assert(t1 != t2);
        assert(!(t1 != t3));

        assert(t2 > t1);
        assert(!(t1 > t2));

        assert(t1 <= t2);
        assert(t1 <= t3);
        assert(!(t2 <= t1));

        assert(t2 >= t1);
        assert(t1 >= t3);
        assert(!(t1 >= t2));

        std::cout << "[PASS] All comparison operators correct" << std::endl;
    }

    // ===== 7. timeDifference() =====
    {
        csl::Timestamp t1(0);
        csl::Timestamp t2(1500000);  // 1.5 seconds

        double diff = csl::timeDifference(t2, t1);
        assert(diff == 1.5);

        double diff2 = csl::timeDifference(t1, t2);
        assert(diff2 == -1.5);

        std::cout << "[PASS] timeDifference() correct" << std::endl;
    }

    // ===== 8. addTime() =====
    {
        csl::Timestamp base(1000000);  // 1 second

        csl::Timestamp t1 = csl::addTime(base, 0.5);
        assert(t1.microSecondsSinceEpoch() == 1500000);

        csl::Timestamp t2 = csl::addTime(base, -0.5);
        assert(t2.microSecondsSinceEpoch() == 500000);

        std::cout << "[PASS] addTime() correct" << std::endl;
    }

    // ===== 9. now() monotonic =====
    {
        csl::Timestamp t1 = csl::Timestamp::now();
        csl::Timestamp t2 = csl::Timestamp::now();
        assert(t1 <= t2);
        std::cout << "[PASS] now() monotonic" << std::endl;
    }

    std::cout << "\n=== All Timestamp tests passed ===" << std::endl;
    return 0;
}
