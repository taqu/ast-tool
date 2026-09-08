#include "report_job.h"
#include "../store/data_store.h"
#include "../writer/report_writer.h"
#include <iostream>

namespace job {

ReportJob::ReportJob(store::DataStore& store, writer::ReportWriter& writer)
    : store_(store), writer_(writer) {}

void ReportJob::generate() {
    std::cout << "report: generating\n";
    std::string data = store_.query();
    writer_.write(data);
}

} // namespace job
