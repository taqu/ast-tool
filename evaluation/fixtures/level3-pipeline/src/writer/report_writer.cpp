#include "report_writer.h"
#include <iostream>

namespace writer {

void ReportWriter::write(const std::string& data) {
    std::cout << "report: writing " << data << "\n";
}

} // namespace writer
