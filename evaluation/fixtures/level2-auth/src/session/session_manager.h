#pragma once
#include <map>
#include <string>

namespace session {

struct Session {
    int id;
    std::string token;
    std::string user;
};

class SessionManager {
    std::map<std::string, Session> sessions_;
public:
    Session create(const std::string& user);
    Session get(const std::string& token);
    bool update(const Session& session);
    bool destroy(const std::string& token);
};

} // namespace session
