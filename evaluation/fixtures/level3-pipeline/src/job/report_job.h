#pragma once

namespace store { class DataStore; }
namespace writer { class ReportWriter; }

namespace job {

class ReportJob {
    store::DataStore& store_;
    writer::ReportWriter& writer_;
public:
    ReportJob(store::DataStore& store, writer::ReportWriter& writer);
    void generate();
};

} // namespace job
